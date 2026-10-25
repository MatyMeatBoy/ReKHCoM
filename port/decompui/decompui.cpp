/*
 * decompui — the ReKHCoM launcher.
 *
 * Installs the user's cartridge dumps (any of the three versions, checked by
 * SHA-1), edits the game's settings file (kh_com.ini) and starts the game
 * executable built for the chosen version:
 *
 *   ReKHCoM.exe                 this launcher
 *   khcom_eu.exe / _us / _jp    the game, one per cartridge version
 *   roms/<code>.gba             installed dumps
 *   kh_com.ini, *.sav, *.log    settings, saves and logs (shared data folder)
 *
 * Dear ImGui on SDL3 + OpenGL. Its look follows recomp-ui's GBA theme.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#endif
/* LAUNCHER_ANDROID: the same launcher as the Android app's first screen (port/android/CMakeLists.txt): OpenGL ES 3,
   the app's files folder, the system's file pickers through the activity (launcher_android.inc) */

#include <SDL3/SDL.h>
#ifdef LAUNCHER_ANDROID
#include <SDL3/SDL_main.h> /* main() is SDL_main, which LauncherActivity starts */
#include <GLES3/gl3.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#define PORT_PAD_AXIS_BASE 100
#include "../include/port/port_zoom.h"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"

namespace fs = std::filesystem;

/* ---- strings (Spanish / English) ------------------------------------------- */

#include "../include/port/port_aspect.h"
#include "../include/port/pad_layout.h"
#include "save_import.h"
#include "save_file.h"
#include "save_editor.h"
#include "card_table.h"
#include "../achievements/ra_client.h"
#include "../achievements/offline_achievements.h"
#include "../achievements/toast_layout.h"

#include "../include/port/port_version.h"
#ifndef _WIN32
#include <cerrno>
#include <ctime>
#include <strings.h>
#include <sys/stat.h>
#define _stricmp strcasecmp
#ifndef APIENTRY
#define APIENTRY
#endif
/* what the Windows calls give, elsewhere (the Android app: LAUNCHER_ANDROID) */
static void SecureZeroMemory(void* p, size_t n) {
    volatile unsigned char* b = (volatile unsigned char*)p;
    while (n--)
        *b++ = 0;
}
#endif
#ifdef LAUNCHER_ANDROID
#include "launcher_android.inc"
#endif

/* a file opened by its path (UTF-16 on Windows) */
static FILE* OpenFile(const std::filesystem::path& p, const char* mode) {
#ifdef _WIN32
    std::wstring m(mode, mode + std::strlen(mode));
    return _wfopen(p.c_str(), m.c_str());
#else
    return std::fopen(p.c_str(), mode);
#endif
}

/* from replaces to in one step, written through to the disk */
static bool ReplaceWith(const std::filesystem::path& from, const std::filesystem::path& to) {
#ifdef _WIN32
    return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}

static bool sSpanish;

/* The launcher in the game's languages (the user, 24/10): Spanish and English in the code, French, German, Italian
   and Japanese from i18n_table.inc, keyed by the English text; a text not translated yet stays in English. */
#include "i18n_table.inc"
static int sUiColumn; /* kI18n's column: 0 English (and Spanish), 1 fr, 2 de, 3 it, 4 ja */
static std::unordered_map<std::string, const char*> sUiTable;   /* English text -> this language's */
static std::unordered_map<const char*, const char*> sUiSeen;    /* the same by address, once looked up */

static const char* T(const char* es, const char* en) {
    if (sSpanish)
        return es;
    if (sUiColumn == 0)
        return en;
    auto seen = sUiSeen.find(en);
    if (seen != sUiSeen.end())
        return seen->second;
    auto it = sUiTable.find(en);
    const char* text = it != sUiTable.end() ? it->second : en;
    sUiSeen.emplace(en, text);
    return text;
}

/* ---- cartridge versions ---------------------------------------------------- */

struct Version {
    const char* id;
    const char* code;
    const char* sha1;
    const char* regionEs;
    const char* regionEn;
    const char* languages; /* flag:code pairs, e.g. "gb:EN fr:FR" */
};

static const Version kVersions[] = {
    { "eu", "B8CP", "8db73586cdb11b3795907edebf43228dbcd3e6b2", "Europa", "Europe", "gb:EN fr:FR de:DE es:ES it:IT" },
    { "us", "B8CE", "10729bd884f8fdca7a310b6d606c52e46657aa48", "América", "USA", "us:EN" },
    { "jp", "B8CJ", "59ec0a0a4ccd1e6acb3bbd7bfb21d63988958cfa", "Japón", "Japan", "jp:JA" },
};
static const int kVersionCount = 3;
static const size_t kRomSize = 32u << 20;

struct VersionState {
    bool installed;
    fs::path exe;
};

static fs::path sRoot;
static VersionState sState[kVersionCount];
static int sSelected;
static void KeepSelection(); /* the version chosen: European first, then the last chosen */
static std::string sNotice;
static bool sNoticeError;
static double sNoticeTime;

static void Notice(const std::string& text, bool error) {
    sNotice = text;
    sNoticeError = error;
    sNoticeTime = ImGui::GetCurrentContext() ? ImGui::GetTime() : 0.0;
}

#ifndef _WIN32
/* SHA-1 (FIPS 180-1), for the dumps' check where Windows' BCrypt is not at hand */
static std::string Sha1Hex(const std::vector<unsigned char>& data) {
    uint32_t h[5] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u };
    uint64_t bits = (uint64_t)data.size() * 8;
    std::vector<unsigned char> m(data);
    m.push_back(0x80);
    while (m.size() % 64 != 56)
        m.push_back(0);
    for (int i = 7; i >= 0; i--)
        m.push_back((unsigned char)(bits >> (i * 8)));
    auto rol = [](uint32_t v, int n) { return (v << n) | (v >> (32 - n)); };
    for (size_t off = 0; off < m.size(); off += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
            w[i] = (uint32_t)m[off + i * 4] << 24 | (uint32_t)m[off + i * 4 + 1] << 16 | (uint32_t)m[off + i * 4 + 2] << 8 |
                   m[off + i * 4 + 3];
        for (int i = 16; i < 80; i++)
            w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999u; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1u; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6u; }
            uint32_t t = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = t;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
    std::string hex;
    for (uint32_t v : h) {
        char eight[9];
        std::snprintf(eight, sizeof(eight), "%08x", v);
        hex += eight;
    }
    return hex;
}
#else
static std::string Sha1Hex(const std::vector<unsigned char>& data) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    unsigned char digest[20] = {};
    std::string hex;

    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, nullptr, 0) != 0)
        return hex;
    if (BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0) {
        BCryptHashData(hash, (PUCHAR)data.data(), (ULONG)data.size(), 0);
        BCryptFinishHash(hash, digest, sizeof(digest), 0);
        BCryptDestroyHash(hash);
        for (unsigned char b : digest) {
            char two[3];
            std::snprintf(two, sizeof(two), "%02x", b);
            hex += two;
        }
    }
    BCryptCloseAlgorithmProvider(alg, 0);
    return hex;
}
#endif

static bool ReadFile(const fs::path& path, std::vector<unsigned char>& out, size_t limit = 64u << 20) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.seekg(0, std::ios::end);
    std::streamoff size = f.tellg();
    if (size <= 0 || size > (std::streamoff)limit)
        return false;
    out.resize((size_t)size);
    f.seekg(0);
    f.read((char*)out.data(), size);
    return (bool)f;
}

static fs::path RomPath(int v) {
    return sRoot / "roms" / (std::string(kVersions[v].code) + ".gba");
}

/* The player's own files in an install: saves (every bank and version), settings, the RetroAchievements login and the
   achievements earned offline, the online settings, save states, ROMs and the HD mod. */
static bool IsPlayerFile(const std::string& name) {
    auto starts = [&](const char* p) { return name.rfind(p, 0) == 0; };
    auto ends = [&](const char* s) { size_t n = strlen(s); return name.size() >= n && name.compare(name.size() - n, n, s) == 0; };
    if (name == "kh_com.default.ini") /* shipped with the package */
        return false;
    return (starts("kh_com") && (ends(".sav") || ends(".ini"))) || name == "decompui.ini" ||
           name == "retroachievements.ini" || (starts("achievements") && ends(".dat")) || name == "netlink.cfg" ||
           ends(".gba");
}

static bool HasPlayerData(const fs::path& dir) {
    std::error_code ec;
    for (const auto& f : fs::directory_iterator(dir, ec))
        if (f.is_regular_file(ec) && IsPlayerFile(f.path().filename().string()))
            return true;
    return fs::is_directory(dir / "roms", ec) && !fs::is_empty(dir / "roms", ec);
}

/* An update unpacked beside the old install instead of over it: up to v1.9.10 the ZIP had a versioned top folder
   (ReKHCoM-v1.9.10-windows-x86/), so "extract over the same folder" put the new version next to it, with no saves.
   A new install with no player files of its own takes them from the newest ReKHCoM install inside it or beside it:
   files and folders copied (never over one already here), the HD mod moved (gigabytes; a rename on the same disk). */
static void AdoptPreviousInstall() {
    std::error_code ec;
    if (HasPlayerData(sRoot))
        return;
    fs::path best;
    fs::file_time_type bestTime{};
    auto consider = [&](const fs::path& dir) {
        std::string name = dir.filename().string();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (fs::equivalent(dir, sRoot, ec) || name.rfind("rekhcom", 0) != 0 || !HasPlayerData(dir))
            return;
        fs::file_time_type t{};
        for (const auto& f : fs::directory_iterator(dir, ec))
            if (f.is_regular_file(ec) && IsPlayerFile(f.path().filename().string()))
                t = std::max(t, f.last_write_time(ec));
        if (best.empty() || t > bestTime)
            best = dir, bestTime = t;
    };
    for (const auto& d : fs::directory_iterator(sRoot, ec))
        if (d.is_directory(ec))
            consider(d.path());
    for (const auto& d : fs::directory_iterator(sRoot.parent_path(), ec))
        if (d.is_directory(ec))
            consider(d.path());
    if (best.empty())
        return;
    int copied = 0;
    for (const auto& f : fs::directory_iterator(best, ec)) {
        std::string name = f.path().filename().string();
        fs::path to = sRoot / name;
        if (f.is_directory(ec) && (name == "roms" || name == "states")) {
            /* the package has its own roms/ (its notes): merged, nothing here replaced */
            fs::copy(f.path(), to, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);
            copied += !ec;
            continue;
        }
        if (fs::exists(to, ec))
            continue;
        if (f.is_regular_file(ec) && IsPlayerFile(name)) {
            copied += fs::copy_file(f.path(), to, ec) ? 1 : 0;
        } else if (f.is_directory(ec) && name == "hd") {
            fs::rename(f.path(), to, ec); /* another disk: left there, the launcher's import makes a new one */
            copied += !ec;
        }
    }
    if (copied)
        Notice(std::string(T("Se trajeron tus partidas, opciones y ROMs de la instalación anterior: ",
                             "Your saves, settings and ROMs were brought over from the previous install: ")) +
                   best.u8string(),
               false);
}

static void ScanInstall() {
    static const char* const devDirs[] = { "pc", "pc-us", "pc-jp" };

    for (int v = 0; v < kVersionCount; v++) {
        std::vector<unsigned char> rom;
        sState[v].installed = ReadFile(RomPath(v), rom) && Sha1Hex(rom) == kVersions[v].sha1;
        /* the game executables live in bin/, so the launcher is the only one at the root */
        sState[v].exe = sRoot / "bin" / (std::string("khcom_") + kVersions[v].id + ".exe");
        if (!fs::exists(sState[v].exe))
            sState[v].exe = sRoot / (std::string("khcom_") + kVersions[v].id + ".exe");
        if (!fs::exists(sState[v].exe)) {
            /* development tree: build/decompui next to build/pc, build/pc-us, ... */
            fs::path dev = sRoot.parent_path() / devDirs[v] / "khcom.exe";
            if (fs::exists(dev))
                sState[v].exe = dev;
        }
    }
}

/* The game for version v can start: its dump, and its program (the Android app carries the game in its libraries) */
static bool GameReady(int v) {
#ifdef LAUNCHER_ANDROID
    return sState[v].installed;
#else
    return sState[v].installed && fs::exists(sState[v].exe);
#endif
}

/* Checks a dump and copies it into roms/. */
static void InstallRom(const fs::path& path) {
    std::vector<unsigned char> rom;

    if (!ReadFile(path, rom) || rom.size() != kRomSize) {
        Notice(T("Ese archivo no es un volcado de cartucho de 32 MiB.",
                 "That file is not a 32 MiB cartridge dump."), true);
        return;
    }
    for (int v = 0; v < kVersionCount; v++) {
        if (std::memcmp(rom.data() + 0xAC, kVersions[v].code, 4) != 0)
            continue;
        if (Sha1Hex(rom) != kVersions[v].sha1) {
            Notice(std::string(T("La ROM ", "The ")) + T(kVersions[v].regionEs, kVersions[v].regionEn) +
                       T(" no coincide con el volcado original (SHA-1 distinto).",
                         " ROM does not match the original dump (different SHA-1)."),
                   true);
            return;
        }
        std::error_code ec;
        fs::create_directories(sRoot / "roms", ec);
        fs::path tmp = RomPath(v);
        tmp += ".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            out.write((const char*)rom.data(), (std::streamsize)rom.size());
            if (!out) {
                Notice(T("No se pudo escribir en la carpeta roms.", "Could not write to the roms folder."), true);
                return;
            }
        }
        fs::rename(tmp, RomPath(v), ec);
        if (ec) {
            Notice(T("No se pudo instalar la ROM.", "Could not install the ROM."), true);
            return;
        }
        sState[v].installed = true;
        KeepSelection();
        Notice(std::string(T("ROM instalada y verificada: ", "ROM installed and verified: ")) +
                   T(kVersions[v].regionEs, kVersions[v].regionEn),
               false);
        return;
    }
    Notice(T("No es una ROM de Kingdom Hearts: Chain of Memories.",
             "This is not a Kingdom Hearts: Chain of Memories ROM."), true);
}

static void PickRom(SDL_Window* window) {
#ifdef LAUNCHER_ANDROID
    (void)window;
    AndroidPick(ANDROID_PICK_ROM, 0, 0); /* the system's picker; the activity copies it to roms/ and checks it */
#else
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    SDL_PropertiesID props = SDL_GetWindowProperties(window);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    ofn.lpstrFilter = L"GBA ROM (*.gba)\0*.gba;*.agb;*.bin\0*.*\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn))
        InstallRom(fs::path(file));
#endif
}

/* ---- settings (kh_com.ini, shared with the game) ----------------------------- */

struct Setting {
    const char* key;
    int def;
};

/* Same keys, order and defaults as port/src/port_config.c. */
static const Setting kSettings[] = {
    { "undub", 0 },
    { "cheat_unlock_riku", 0 },
    { "cheat_one_hit", 0 },
    { "cheat_infinite_hp", 0 },
    { "cheat_savestates", 0 },
    { "debug_gba_map_logic", 0 },
    { "experimental_save_point_slot_select", 0 },
    { "experimental_hide_touch_controls", 0 },
    { "config_version", PORT_ZOOM_CONFIG_VERSION }, { "view_mode", 4 },   { "zoom", PORT_ZOOM_DEFAULT },         { "scale_mode", 0 },
    { "filter", 0 },         { "fullscreen", 0 },  { "vsync", 1 },        { "hud_size", 1 },
    { "narrow_fit", 1 },     { "scanlines", 0 },   { "color_correction", 0 }, { "show_fps", 0 }, { "experimental_frame_skip", 1 }, { "discord_presence", 1 }, { "net_stats", 1 }, { "net_delay", 0 }, { "hd_movies", 1 }, { "hd_music", 1 }, { "hd_scenes", 0 }, { "hd_subtitles", 1 }, { "hd_cards", 0 }, { "hd_title", 0 },
    { "graphics_mode", 0 }, { "graphics_mode_applied", 0 }, { "gba_border", 0 },
    { "window_scale", 4 },   { "fullscreen_width", 0 }, { "fullscreen_height", 0 },
    { "master_volume", 100 }, { "music_volume", 100 }, { "sfx_volume", 100 },
    { "dodge_button", 1 },   { "stock_button", 1 }, { "pad_layout", 0 }, { "experimental_moogle_notice", 0 }, { "hd_text", 0 }, { "hd_font", 0 },
    { "invert_card_scroll", 0 }, { "game_language", 0 }, { "experimental_battle_zoom", 0 },
    { "card_hud_raise", 8 }, { "save_bank", 1 }, { "hd2d", 0 }, { "smooth_audio", 0 }, { "experimental_unstock", 0 },
    { "ps2_mode", 0 }, { "ps2_difficulty", 0 }, { "ps2_fast_text", 0 }, { "hd_hud", 0 }, { "aspect", 0 }, { "integer_zoom", 0 },
    { "kb_a", SDLK_X },         { "kb_a_2", SDLK_K },       { "pad_a", SDL_GAMEPAD_BUTTON_SOUTH },
    { "kb_b", SDLK_Z },         { "kb_b_2", SDLK_J },       { "pad_b", SDL_GAMEPAD_BUTTON_EAST },
    { "kb_l", SDLK_A },         { "kb_l_2", 0 },            { "pad_l", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER },
    { "kb_r", SDLK_S },         { "kb_r_2", 0 },            { "pad_r", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
    { "kb_start", SDLK_RETURN }, { "kb_start_2", 0 },       { "pad_start", SDL_GAMEPAD_BUTTON_START },
    { "kb_select", SDLK_BACKSPACE }, { "kb_select_2", SDLK_RSHIFT }, { "pad_select", SDL_GAMEPAD_BUTTON_BACK },
    { "kb_up", SDLK_UP },       { "kb_up_2", SDLK_W },      { "pad_up", SDL_GAMEPAD_BUTTON_DPAD_UP },
    { "kb_down", SDLK_DOWN },   { "kb_down_2", 0 },         { "pad_down", SDL_GAMEPAD_BUTTON_DPAD_DOWN },
    { "kb_left", SDLK_LEFT },   { "kb_left_2", 0 },         { "pad_left", SDL_GAMEPAD_BUTTON_DPAD_LEFT },
    { "kb_right", SDLK_RIGHT }, { "kb_right_2", 0 },        { "pad_right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
    { "kb_stock", SDLK_D },     { "kb_stock_2", 0 },        { "pad_stock", SDL_GAMEPAD_BUTTON_NORTH },
    { "kb_dodge", SDLK_C },     { "kb_dodge_2", 0 },        { "pad_dodge", SDL_GAMEPAD_BUTTON_WEST },
    { "kb_unstock", SDLK_V },   { "kb_unstock_2", 0 },      { "pad_unstock", SDL_GAMEPAD_BUTTON_RIGHT_STICK },
};

static std::map<std::string, int> sIni;
/* kh_com.ini as this launcher last read or wrote it: the game may change the
   file meanwhile (its language, in-game options), and those values win over
   the launcher's stale copy unless the launcher changed that key itself. */
static std::map<std::string, int> sIniOnDisk;

static int& Ini(const char* key) {
    return sIni[key];
}

static std::map<std::string, int> ReadIniFile() {
    std::map<std::string, int> values;
    std::ifstream f(sRoot / "kh_com.ini");
    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (line.empty() || line[0] == '#' || line[0] == ';' || eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        while (!key.empty() && std::isspace((unsigned char)key.back()))
            key.pop_back();
        values[key] = std::atoi(line.c_str() + eq + 1);
    }
    return values;
}

static void LoadIni() {
    sIni.clear();
    for (const Setting& s : kSettings)
        sIni[s.key] = s.def;
    for (const auto& kv : ReadIniFile())
        sIni[kv.first] = kv.second;
    for (const char* retired : {"unstock_button"})
        sIni.erase(retired);
    /* Files from before the auto view switch to it once, as the game does. */
    if (sIni["config_version"] < 2) {
        sIni["view_mode"] = 4;
        sIni["config_version"] = 2;
    }
    /* ...and from before v1.0 to filling the height in GBA-size scenes. */
    if (sIni["config_version"] < 3) {
        sIni["narrow_fit"] = 1;
        sIni["config_version"] = 3;
    }
    /* ...and from before v1.5, whose fullscreen was 1920x1080 by default,
       to the monitor's own resolution (ultrawide monitors fill at 32:9). */
    if (sIni["config_version"] < 4) {
        if (sIni["fullscreen_width"] == 1920 && sIni["fullscreen_height"] == 1080) {
            sIni["fullscreen_width"] = 0;
            sIni["fullscreen_height"] = 0;
        }
        sIni["config_version"] = 4;
    }
    if (sIni["config_version"] < 5) {
        sIni["zoom"] = 1; /* Normal, x3 at 1080p */
        sIni["config_version"] = 5;
    }
    if (sIni["config_version"] < 6) {
        sIni["aspect"] = 0; /* fill the window */
        sIni["config_version"] = 6;
    }
    if (sIni["config_version"] < 8) {
        sIni["zoom"] = 2; /* x2 Enhanced: zoom is now the view as a multiple of the GBA's */
        sIni["integer_zoom"] = 1;
        sIni["config_version"] = 8;
    }
    if (sIni["config_version"] < 9) {
        sIni["zoom"] = PORT_ZOOM_DEFAULT;
        sIni["integer_zoom"] = 1;
        sIni["config_version"] = 9;
    }
    if (sIni["zoom"] & 1)
        sIni["zoom"]++;
    if (sIni["zoom"] == PORT_ZOOM_CLASSIC) sIni["zoom"] = PORT_ZOOM_DEFAULT;
    sIni["integer_zoom"] = 0;
    sIni["config_version"] = PORT_ZOOM_CONFIG_VERSION;
    sIniOnDisk = sIni;
}

/* The monitor's fullscreen resolutions, largest first, as the game's own
   list: the desktop first (0x0), then each distinct mode. */
/* the desktop's size (the monitor's current mode), for the zoom levels and the fullscreen row */
static void DesktopSize(int* w, int* h) {
#ifdef _WIN32
    DEVMODEW cur;
    ZeroMemory(&cur, sizeof(cur));
    cur.dmSize = sizeof(cur);
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &cur) && cur.dmPelsWidth && cur.dmPelsHeight) {
        *w = (int)cur.dmPelsWidth;
        *h = (int)cur.dmPelsHeight;
    }
#else
    const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    if (mode && mode->w > 0 && mode->h > 0) {
        *w = std::max(mode->w, mode->h); /* the game is landscape */
        *h = std::min(mode->w, mode->h);
    }
#endif
}

static std::vector<std::pair<int, int>> DisplayModes() {
    std::vector<std::pair<int, int>> modes;
#ifdef _WIN32
    DEVMODEW dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &dm); i++) {
        std::pair<int, int> m((int)dm.dmPelsWidth, (int)dm.dmPelsHeight);
        if (m.first >= 640 && m.second >= 480 && std::find(modes.begin(), modes.end(), m) == modes.end())
            modes.push_back(m);
    }
#endif
    std::sort(modes.begin(), modes.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.first * a.second > b.first * b.second || (a.first * a.second == b.first * b.second && a.first > b.first);
    });
    return modes;
}

static void SaveIni() {
    for (const auto& kv : ReadIniFile()) {
        auto seen = sIniOnDisk.find(kv.first);
        if (seen == sIniOnDisk.end() || sIni[kv.first] == seen->second)
            sIni[kv.first] = kv.second; /* the game's change, not the launcher's */
    }
    sIniOnDisk = sIni;
    std::string onlineName; /* the one text setting (the lobby's name): kept as written */
    {
        std::ifstream in(sRoot / "kh_com.ini");
        std::string l;
        while (std::getline(in, l))
            if (l.rfind("online_name=", 0) == 0)
                onlineName = l;
    }
    std::ofstream f(sRoot / "kh_com.ini", std::ios::trunc);
    f << "# Kingdom Hearts: Chain of Memories (PC) settings\n";
    for (const Setting& s : kSettings)
        f << s.key << "=" << sIni[s.key] << "\n";
    /* keys this launcher does not know (newer game options) are kept */
    for (const auto& kv : sIni) {
        bool known = kv.first == "online_name";
        for (const Setting& s : kSettings)
            known = known || kv.first == s.key;
        if (!known)
            f << kv.first << "=" << kv.second << "\n";
    }
    if (!onlineName.empty())
        f << onlineName << "\n";
}

/* Language the European game starts in (the language screen's cursor):
   0 = the player chooses, then English, French, German, Italian, Spanish. */
static const char* const kGameLanguageIds[] = { "", "en", "fr", "de", "it", "es" };
/* The launcher's languages: 1 Spanish, 2 English, then the European cartridge's other three and Japanese; 0 until
   chosen. ids[] as kh_com's and the flags' (LanguageFlag). */
enum { UI_LANGUAGE_COUNT = 6 };
static const char* const kUiLanguageIds[UI_LANGUAGE_COUNT + 1] = { "", "es", "en", "fr", "de", "it", "ja" };
static const char* const kUiLanguageNames[UI_LANGUAGE_COUNT + 1] = {
    "", "Espa\xC3\xB1ol", "English", "Fran\xC3\xA7" "ais", "Deutsch", "Italiano", "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E" };
static const char* const kUiLanguageFlags[UI_LANGUAGE_COUNT + 1] = { "", "es", "gb", "fr", "de", "it", "jp" };
static int sUiLanguage;

static void ApplyUiLanguage() {
    /* the first time, the system's language is chosen and kept (English when it is none of these) */
    if (sUiLanguage < 1 || sUiLanguage > UI_LANGUAGE_COUNT) {
        std::string system;
#ifdef _WIN32
        switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
            case LANG_SPANISH: system = "es"; break;
            case LANG_FRENCH: system = "fr"; break;
            case LANG_GERMAN: system = "de"; break;
            case LANG_ITALIAN: system = "it"; break;
            case LANG_JAPANESE: system = "ja"; break;
        }
#else
        int count = 0;
        SDL_Locale** locales = SDL_GetPreferredLocales(&count);
        if (locales && count > 0 && locales[0]->language)
            system = locales[0]->language;
        SDL_free(locales);
#endif
        sUiLanguage = 2;
        for (int i = 1; i <= UI_LANGUAGE_COUNT; i++)
            if (system == kUiLanguageIds[i])
                sUiLanguage = i;
    }
    sSpanish = sUiLanguage == 1;
    sUiColumn = sUiLanguage >= 3 ? sUiLanguage - 2 : 0;
    sUiTable.clear();
    sUiSeen.clear();
    if (sUiColumn)
        for (const auto& row : kI18n)
            if (row[sUiColumn])
                sUiTable.emplace(row[0], row[sUiColumn]);
}

static bool sHdOfferDeclined; /* "Don't ask again" on the HD mod's offer (rom_folder.inc) */
static void LoadLauncherState() {
    std::ifstream f(sRoot / "decompui.ini");
    std::string line;
    while (std::getline(f, line)) {
        if (line == "hd_offer=0") {
            sHdOfferDeclined = true;
        } else if (line.rfind("version=", 0) == 0) {
            for (int v = 0; v < kVersionCount; v++)
                if (line.substr(8) == kVersions[v].id)
                    sSelected = v;
        } else if (line.rfind("game_language=", 0) == 0) {
            /* older launchers kept it here; kh_com.ini holds it now (the game
               reads it and remembers the player's first choice there) */
            for (int i = 1; i < 6; i++)
                if (line.substr(14) == kGameLanguageIds[i] && Ini("game_language") == 0) {
                    Ini("game_language") = i;
                    SaveIni();
                }
        } else if (line.rfind("ui_language=", 0) == 0) {
            sUiLanguage = std::clamp(std::atoi(line.c_str() + 12), 0, (int)UI_LANGUAGE_COUNT);
        }
    }
    ApplyUiLanguage();
}

static void SaveLauncherState() {
    std::ofstream f(sRoot / "decompui.ini", std::ios::trunc);
    f << "version=" << kVersions[sSelected].id << "\n";
    f << "ui_language=" << sUiLanguage << "\n";
    if (sHdOfferDeclined)
        f << "hd_offer=0\n"; /* "Don't ask again" on the HD mod's offer */
}

/* The version chosen (the user, 24/10): the European one the first time, then always the last one chosen, saved at
   once. A scan of roms/ or a new ROM never changes it; only when the chosen one is not installed does it move, to the
   European one if it is there, else the first installed. */
static void KeepSelection() {
    if (sState[sSelected].installed)
        return;
    int pick = -1;
    for (int v = 0; v < kVersionCount && pick < 0; v++)
        if (sState[v].installed)
            pick = v; /* kVersions[0] is the European one */
    if (pick >= 0 && pick != sSelected) {
        sSelected = pick;
        SaveLauncherState();
    }
}

/* ---- saves -------------------------------------------------------------------- */

/* The selected save bank's file: bank 1 is the version's .sav, bank n
   <name>_bank<n>.sav (port_memory.c). */
static fs::path SavePath(int v) {
    static const char* const names[] = { "kh_com", "kh_com_us", "kh_com_jp" };
    int bank = std::clamp(Ini("save_bank"), 1, 5);
    return sRoot / (std::string(names[v]) + (bank > 1 ? "_bank" + std::to_string(bank) : "") + ".sav");
}

/* Copies the version's save to backups/<name>-<date>-<time>.sav. */
static bool BackupSave(int v) {
    std::error_code ec;
    char stamp[64];
    static unsigned sequence=0;
#ifdef _WIN32
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::snprintf(stamp, sizeof(stamp), "-%04d%02d%02d-%02d%02d%02d-%03d-%u", t.wYear, t.wMonth, t.wDay,
                  t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, ++sequence);
#else
    std::time_t now = std::time(nullptr);
    std::tm t = *std::localtime(&now);
    std::snprintf(stamp, sizeof(stamp), "-%04d%02d%02d-%02d%02d%02d-%u", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec, ++sequence);
#endif
    fs::path dir = sRoot / "backups";
    fs::create_directories(dir, ec);
    fs::path dst = dir / (SavePath(v).stem().string() + stamp + ".sav");
    if (fs::copy_file(SavePath(v), dst, fs::copy_options::none, ec)) {
        Notice(std::string(T("Copia guardada en backups/", "Backup saved to backups/")) + dst.filename().string(), false);
        return true;
    }
    Notice(T("No se pudo copiar la partida.", "Could not copy the save."), true);
    return false;
}

/* ---- importing a save from an emulator or a backup device ------------------- */

/* Shared, ROM-free parser; raw SRAM and explicitly validated containers. */
static int ConvertGbaSave(const std::vector<unsigned char>& in,int v,
                          std::vector<unsigned char>& out,int& fromVersion) {
    return RekhcomSave::Convert(in,v,out,fromVersion);
}

static bool ImportSavePath(const fs::path& file,int v) {
    std::vector<unsigned char> in,out;
    std::error_code ec;
    int from=0,blocks;
    if (!ReadFile(file, in, 4u << 20)) {
        Notice(T("No se pudo leer el archivo.", "Could not read the file."), true);
        return false;
    }
    blocks = ConvertGbaSave(in, v, out, from);
    if (blocks <= 0) {
        Notice(T("El archivo no contiene una partida de Chain of Memories.",
                 "The file holds no Chain of Memories save."), true);
        return false;
    }
    bool exists=fs::exists(SavePath(v),ec);
    if(ec) { Notice(T("No se pudo verificar el destino.","Could not verify the destination."),true);return false; }
    if(exists&&!BackupSave(v))return false;
    if(!RekhcomSave::Replace(SavePath(v),out)) {
        Notice(T("No se pudo escribir la partida. Original conservado.","Could not write the save. Original preserved."),true);
        return false;
    }
    {
        char msg[200];
        bool cross = (from == 0) != (v == 0);
        std::snprintf(msg, sizeof(msg), "%s %s (%d %s)%s",
                      T("Partida importada en", "Save imported into"), SavePath(v).filename().string().c_str(),
                      blocks, T("bloques", "blocks"),
                      cross ? T(", convertida de otra regi\xC3\xB3n", ", converted from another region") : "");
        Notice(msg, false);
    }
    return true;
}

static void ImportSave(SDL_Window* window, int v) {
#ifdef LAUNCHER_ANDROID
    (void)window;
    AndroidPick(ANDROID_PICK_SAVE, v, std::clamp(Ini("save_bank"), 1, 5));
#else
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    SDL_PropertiesID props = SDL_GetWindowProperties(window);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    ofn.lpstrFilter = L"GBA save (*.sav;*.srm;*.sa1;*.sps;*.xps;*.gsv;*.dsv;*.bin)\0"
                      L"*.sav;*.srm;*.sa1;*.sps;*.xps;*.gsv;*.dsv;*.bin\0*.*\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn))
        return;
    ImportSavePath(fs::path(file),v);
#endif
}


/* ---- theme ------------------------------------------------------------------- */

static float sScale = 1.0f;

static float Px(float v) {
    return v * sScale;
}

static ImU32 Col(float r, float g, float b, float a = 1.0f) {
    return ImGui::GetColorU32(ImVec4(r, g, b, a));
}

/* recomp-ui's "Game Boy Advance" palette. */
/* The game's look (its debug menu, which the in-game options copy): white,
   the grey of its bars, near-black text and the red of its cursor square. */
static const ImVec4 kInk(1.0f, 1.0f, 1.0f, 1.0f);
static const ImVec4 kInkLift(0.965f, 0.957f, 0.957f, 1.0f);
static const ImVec4 kPanel(0.988f, 0.984f, 0.984f, 1.0f);
static const ImVec4 kPanelHover(0.925f, 0.910f, 0.910f, 1.0f);
static const ImVec4 kControl(0.910f, 0.894f, 0.894f, 1.0f);
static const ImVec4 kControlHover(0.835f, 0.812f, 0.812f, 1.0f);
static const ImVec4 kBorder(0.710f, 0.678f, 0.678f, 1.0f);
static const ImVec4 kAccent(1.0f, 0.063f, 0.0f, 1.0f);
static const ImVec4 kAccentDim(0.612f, 0.063f, 0.0f, 1.0f);
static const ImVec4 kText(0.031f, 0.031f, 0.031f, 1.0f);
static const ImVec4 kMuted(0.420f, 0.396f, 0.396f, 1.0f);
static const ImVec4 kGood(0.090f, 0.520f, 0.220f, 1.0f);
static const ImVec4 kWarn(0.780f, 0.420f, 0.0f, 1.0f);
static const ImVec4 kBad(0.800f, 0.063f, 0.0f, 1.0f);
static const ImVec4 kFocus(0.031f, 0.031f, 0.031f, 1.0f);

/* The debug menu's cursor square: black with red inside (on), or grey. */
/* A small flag beside each language of a version (h tall, 3:2), drawn with
   plain shapes: no image files, sharp at every scale. */
static void LanguageFlag(ImDrawList* dl, ImVec2 p, float h, const char* id) {
    const float w = h * 1.5f;
    const ImU32 white = IM_COL32(255, 255, 255, 255), red = IM_COL32(206, 17, 38, 255);
    auto band = [&](float x0, float y0, float x1, float y1, ImU32 c) {
        dl->AddRectFilled(ImVec2(p.x + w * x0, p.y + h * y0), ImVec2(p.x + w * x1, p.y + h * y1), c);
    };
    if (!std::strncmp(id, "fr", 2)) {
        band(0, 0, 1.f / 3, 1, IM_COL32(0, 85, 164, 255)); band(1.f / 3, 0, 2.f / 3, 1, white); band(2.f / 3, 0, 1, 1, IM_COL32(239, 65, 53, 255));
    } else if (!std::strncmp(id, "it", 2)) {
        band(0, 0, 1.f / 3, 1, IM_COL32(0, 146, 70, 255)); band(1.f / 3, 0, 2.f / 3, 1, white); band(2.f / 3, 0, 1, 1, red);
    } else if (!std::strncmp(id, "de", 2)) {
        band(0, 0, 1, 1.f / 3, IM_COL32(0, 0, 0, 255)); band(0, 1.f / 3, 1, 2.f / 3, IM_COL32(221, 0, 0, 255)); band(0, 2.f / 3, 1, 1, IM_COL32(255, 206, 0, 255));
    } else if (!std::strncmp(id, "es", 2)) {
        band(0, 0, 1, 1, IM_COL32(170, 21, 27, 255)); band(0, .25f, 1, .75f, IM_COL32(241, 191, 0, 255));
    } else if (!std::strncmp(id, "jp", 2)) {
        band(0, 0, 1, 1, white);
        dl->AddCircleFilled(ImVec2(p.x + w * .5f, p.y + h * .5f), h * .3f, IM_COL32(188, 0, 45, 255));
    } else if (!std::strncmp(id, "us", 2)) {
        for (int i = 0; i < 7; i++)
            band(0, i / 7.f, 1, (i + 1) / 7.f, i % 2 ? white : IM_COL32(178, 34, 52, 255));
        band(0, 0, .42f, 4 / 7.f, IM_COL32(60, 59, 110, 255));
    } else { /* gb */
        const ImU32 blue = IM_COL32(1, 33, 105, 255);
        band(0, 0, 1, 1, blue);
        dl->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + h), white, h * .2f);
        dl->AddLine(ImVec2(p.x + w, p.y), ImVec2(p.x, p.y + h), white, h * .2f);
        dl->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + h), IM_COL32(200, 16, 46, 255), h * .07f);
        dl->AddLine(ImVec2(p.x + w, p.y), ImVec2(p.x, p.y + h), IM_COL32(200, 16, 46, 255), h * .07f);
        band(.4f, 0, .6f, 1, white); band(0, .33f, 1, .67f, white);
        band(.45f, 0, .55f, 1, IM_COL32(200, 16, 46, 255)); band(0, .4f, 1, .6f, IM_COL32(200, 16, 46, 255));
    }
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(0, 0, 0, 70));
}

static void GameSquare(ImDrawList* dl, ImVec2 c, float size, bool on) {
    float h = size * 0.5f, in = size * 0.25f, edge = size * 0.08f;
    if (!on) {
        dl->AddRectFilled(ImVec2(c.x - h, c.y - h), ImVec2(c.x + h, c.y + h), ImGui::GetColorU32(kBorder));
        return;
    }
    dl->AddRectFilled(ImVec2(c.x - h, c.y - h), ImVec2(c.x + h, c.y + h), ImGui::GetColorU32(kText));
    dl->AddRectFilled(ImVec2(c.x - in, c.y - in), ImVec2(c.x + in, c.y + in), ImGui::GetColorU32(kAccent));
    dl->AddRectFilled(ImVec2(c.x - in, c.y - in), ImVec2(c.x - in + edge, c.y + in), ImGui::GetColorU32(kAccentDim));
    dl->AddRectFilled(ImVec2(c.x - in, c.y - in), ImVec2(c.x + in, c.y - in + edge), ImGui::GetColorU32(kAccentDim));
}

static ImFont* sFontBody;
static ImFont* sFontTitle;
static ImFont* sFontSmall;
static ImFont* sFontMenu; /* the game's menu font (KHMenu): tabs, buttons, headings, as the web page */
static ImFont* sFontMenuSmall; /* the same, for the section headings */

static void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s = ImGuiStyle();
    s.WindowPadding = ImVec2(Px(24), Px(20));
    s.FramePadding = ImVec2(Px(14), Px(9));
    s.ItemSpacing = ImVec2(Px(10), Px(10));
    s.ChildRounding = 0;
    s.TabRounding = 0;
    s.FrameRounding = 0;
    s.GrabRounding = 0;
    s.ScrollbarRounding = 0;
    s.ChildBorderSize = 1.0f;
    s.FrameBorderSize = 1.0f;
    s.ScrollbarSize = Px(12);
    s.GrabMinSize = Px(14);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ChildBg] = kPanel;
    c[ImGuiCol_Border] = kBorder;
    c[ImGuiCol_Text] = kText;
    c[ImGuiCol_TextDisabled] = kMuted;
    c[ImGuiCol_FrameBg] = kControl;
    c[ImGuiCol_FrameBgHovered] = kControlHover;
    c[ImGuiCol_FrameBgActive] = kControlHover;
    c[ImGuiCol_Button] = kControl;
    c[ImGuiCol_ButtonHovered] = kControlHover;
    c[ImGuiCol_ButtonActive] = kAccentDim;
    c[ImGuiCol_SliderGrab] = kAccent;
    c[ImGuiCol_SliderGrabActive] = kAccent;
    c[ImGuiCol_CheckMark] = kAccent;
    c[ImGuiCol_Header] = kControl;
    c[ImGuiCol_HeaderHovered] = kControlHover;
    c[ImGuiCol_HeaderActive] = kAccentDim;
    c[ImGuiCol_NavCursor] = kFocus;
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = kBorder;
    c[ImGuiCol_TableBorderLight] = kBorder;
    c[ImGuiCol_TableBorderStrong] = kBorder;
    c[ImGuiCol_TableRowBgAlt] = ImVec4(0, 0, 0, 0.025f);
    c[ImGuiCol_PopupBg] = kPanel;
    c[ImGuiCol_Tab] = kControl;
    c[ImGuiCol_TabHovered] = kControlHover;
    c[ImGuiCol_TabSelected] = kPanelHover;
    c[ImGuiCol_TabSelectedOverline] = kAccent;
    c[ImGuiCol_TabDimmed] = kControl;
    c[ImGuiCol_TabDimmedSelected] = kPanelHover;
    c[ImGuiCol_TabDimmedSelectedOverline] = kAccent;
}

static void LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
#ifdef _WIN32
    wchar_t windir[MAX_PATH];
    GetWindowsDirectoryW(windir, MAX_PATH);
    fs::path fonts = fs::path(windir) / "Fonts";
    std::string regular = (fonts / "segoeui.ttf").string();
    std::string semibold = (fonts / "seguisb.ttf").string();
#else
    /* Android's own: Roboto (the system font of every version) */
    std::string regular = "/system/fonts/Roboto-Regular.ttf";
    std::string semibold = "/system/fonts/Roboto-Medium.ttf";
#endif
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x2190, 0x2193, 0x25B6, 0x25C0, 0 };
    ImFontConfig cfg;
    cfg.OversampleH = 2;

    io.Fonts->Clear();
    if (fs::exists(regular)) {
        sFontBody = io.Fonts->AddFontFromFileTTF(regular.c_str(), Px(18), &cfg, ranges);
        sFontSmall = io.Fonts->AddFontFromFileTTF(regular.c_str(), Px(14), &cfg, ranges);
        sFontTitle = io.Fonts->AddFontFromFileTTF(fs::exists(semibold) ? semibold.c_str() : regular.c_str(),
                                                  Px(34), &cfg, ranges);
    }
    /* KHMenu (assets/fonts, the game's menu lettering): has the Latin accents but not the arrows, which come from
       Segoe UI merged into it */
    {
        std::string kh = (sRoot / "assets" / "fonts" / "KHMenu.otf").string();
        static const ImWchar khRanges[] = { 0x0020, 0x00FF, 0 };
        static const ImWchar arrows[] = { 0x2190, 0x2193, 0x25B6, 0x25C0, 0 };
        if (fs::exists(kh)) {
            auto load = [&](float size) {
                ImFont* f = io.Fonts->AddFontFromFileTTF(kh.c_str(), size, &cfg, khRanges);
                if (f && fs::exists(regular)) {
                    ImFontConfig merge = cfg;
                    merge.MergeMode = true;
                    io.Fonts->AddFontFromFileTTF(regular.c_str(), size * 0.9f, &merge, arrows);
                }
                return f;
            };
            sFontMenu = load(Px(18));
            sFontMenuSmall = load(Px(15));
            if (ImFont* title = load(Px(34)))
                sFontTitle = title;
        }
    }
    /* Japanese: the system's Japanese font merged into every one, with only the characters the launcher's Japanese
       texts use (and the language's own name, shown in every language's picker), so the atlas stays small and no
       rebuild is needed when the language changes */
    {
#ifdef _WIN32
        std::string jp = (fonts / "YuGothM.ttc").string();
        if (!fs::exists(jp))
            jp = (fonts / "msgothic.ttc").string();
#else
        std::string jp = "/system/fonts/NotoSansCJK-Regular.ttc";
#endif
        static ImVector<ImWchar> jpRanges;
        if (jpRanges.empty()) {
            ImFontGlyphRangesBuilder b;
            b.AddText(kUiLanguageNames[6]);
            for (const auto& row : kI18n)
                if (row[4])
                    b.AddText(row[4]);
            b.BuildRanges(&jpRanges);
        }
        if (fs::exists(jp)) {
            ImFontConfig merge = cfg;
            merge.MergeMode = true;
            merge.OversampleH = 1;
            ImFont* const all[] = { sFontBody, sFontSmall, sFontTitle, sFontMenu, sFontMenuSmall };
            float sizes[5] = {}; /* read first: adding a font may move the atlas' sources */
            for (int i = 0; i < 5; i++)
                sizes[i] = all[i] && all[i]->Sources ? all[i]->Sources->SizePixels : 0;
            for (int i = 0; i < 5; i++)
                if (sizes[i] > 0) {
                    merge.DstFont = all[i];
                    io.Fonts->AddFontFromFileTTF(jp.c_str(), sizes[i], &merge, jpRanges.Data);
                }
        }
    }
    if (sFontBody == nullptr) {
        ImFontConfig def;
        def.SizePixels = Px(16);
        sFontBody = sFontSmall = sFontTitle = io.Fonts->AddFontDefault(&def);
    }
    if (sFontMenu == nullptr)
        sFontMenu = sFontBody;
    if (sFontMenuSmall == nullptr)
        sFontMenuSmall = sFontSmall;
}

/* ---- widgets ----------------------------------------------------------------- */

static void Background(ImVec2 size) {
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    ImU32 ink = ImGui::GetColorU32(kInk), lift = ImGui::GetColorU32(kInkLift);
    float mid = size.y * 0.42f;
    bg->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(size.x, mid), ink, ink, lift, lift);
    bg->AddRectFilledMultiColor(ImVec2(0, mid), size, lift, lift, ink, ink);
}

/* Uppercase section heading with an accent tick. */
static void Eyebrow(const char* text) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushFont(sFontMenuSmall);
    float h = ImGui::GetTextLineHeight();
    GameSquare(dl, ImVec2(p.x + h * 0.35f, p.y + h * 0.5f), h * 0.6f, true);
    float x = p.x + h * 1.1f;
    for (const char* q = text; *q;) {
        char one[5] = {};
        unsigned char lead = (unsigned char)*q;
        int bytes = (lead < 0x80) ? 1 : (lead < 0xE0 ? 2 : (lead < 0xF0 ? 3 : 4));
        int n = 0;
        while (n < bytes && q[n]) { one[n] = q[n]; ++n; }
        q += n;
        dl->AddText(ImVec2(x, p.y), ImGui::GetColorU32(kText), one);
        x += ImGui::CalcTextSize(one).x + Px(1.5f);
    }
    ImGui::Dummy(ImVec2(x - p.x, h));
    ImGui::PopFont();
    ImGui::Spacing();
}

/* A group of a page in its own card, as Experimental's is (the user, 24/10): closes the open card, opens the next */
static void NextCard(const char* id, const char* title);

static bool BeginCard(const char* id, ImVec2 size = ImVec2(0, 0)) {
    ImGuiChildFlags flags = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
    if (size.y == 0)
        flags |= ImGuiChildFlags_AutoResizeY;
    return ImGui::BeginChild(id, size, flags);
}

static void NextCard(const char* id, const char* title) {
    ImGui::EndChild();
    ImGui::Spacing();
    BeginCard(id);
    Eyebrow(title);
}

/* The primary action: glow, accent gradient and a play triangle. */
static bool PlayButton(const char* label, ImVec2 size, bool enabled) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    if (!enabled)
        ImGui::BeginDisabled();
    bool clicked = ImGui::InvisibleButton("##play", size, ImGuiButtonFlags_EnableNav);
    bool hover = enabled && ImGui::IsItemHovered();
    bool focus = ImGui::IsItemFocused();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 mx(p.x + size.x, p.y + size.y);
    float r = Px(8);
    float a = enabled ? 1.0f : 0.35f;

    /* a debug-menu row: black, the red cursor square, a grey shadow; not
       ready yet, light paper with diagonal stripes and grey text (as the
       web page), so it reads as waiting rather than broken */
    (void)r;
    if (enabled) {
        dl->AddRectFilled(ImVec2(p.x + Px(4), p.y + Px(4)), ImVec2(mx.x + Px(4), mx.y + Px(4)),
                          Col(kBorder.x, kBorder.y, kBorder.z, 1));
        dl->AddRectFilled(p, mx, hover ? Col(0.18f, 0.17f, 0.17f, 1) : Col(kText.x, kText.y, kText.z, 1));
    } else {
        float step = Px(20), x;
        dl->AddRectFilled(p, mx, Col(0.953f, 0.941f, 0.945f, 1));
        dl->PushClipRect(p, mx, true);
        for (x = p.x - size.y; x < mx.x; x += step)
            dl->AddQuadFilled(ImVec2(x, mx.y), ImVec2(x + Px(10), mx.y), ImVec2(x + Px(10) + size.y, p.y),
                              ImVec2(x + size.y, p.y), Col(0.922f, 0.902f, 0.910f, 1));
        dl->PopClipRect();
        dl->AddRect(p, mx, Col(kBorder.x, kBorder.y, kBorder.z, 1), 0, 0, Px(2));
        a = 1.0f;
    }
    if (focus)
        dl->AddRect(ImVec2(p.x - Px(3), p.y - Px(3)), ImVec2(mx.x + Px(3), mx.y + Px(3)),
                    ImGui::GetColorU32(kAccent), 0, 0, Px(2.5f));
    ImGui::PushFont(sFontTitle);
    float th = ImGui::GetTextLineHeight(), tw = ImGui::CalcTextSize(label).x;
    float sq = Px(22), gap = Px(16);
    float cx = p.x + (size.x - sq - gap - tw) * 0.5f, cy = p.y + size.y * 0.5f;
    if (enabled) {
        dl->AddRectFilled(ImVec2(cx, cy - sq * 0.5f), ImVec2(cx + sq, cy + sq * 0.5f), Col(1, 1, 1, a));
        GameSquare(dl, ImVec2(cx + sq * 0.5f, cy), sq * 0.8f, true);
    } else {
        GameSquare(dl, ImVec2(cx + sq * 0.5f, cy), sq * 0.8f, false);
    }
    dl->AddText(ImVec2(cx + sq + gap, cy - th * 0.5f), enabled ? Col(1, 1, 1, a) : Col(0.60f, 0.58f, 0.59f, 1), label);
    ImGui::PopFont();
    if (!enabled)
        ImGui::EndDisabled();
    return clicked && enabled;
}

static void StatusDot(const ImVec4& color) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = ImGui::GetTextLineHeight(), r = Px(5);
    dl->AddCircleFilled(ImVec2(p.x + r, p.y + h * 0.5f), r, ImGui::GetColorU32(color));
    ImGui::Dummy(ImVec2(r * 2, h));
    ImGui::SameLine(0, Px(8));
}

/* A settings row: label on the left, "<  value  >" on the right. */
static bool CycleRow(const char* label, const char* hint, int& value, int count, const char* const* names) {
    bool changed = false;
    float w = ImGui::GetContentRegionAvail().x;
    float ctrl = std::min(Px(300), w * 0.5f);

    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    if (hint != nullptr && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", hint);
    ImGui::SameLine(w - ctrl);
    float btn = ImGui::GetFrameHeight();
    if (ImGui::Button("<", ImVec2(btn, 0))) {
        value = (value + count - 1) % count;
        changed = true;
    }
    ImGui::SameLine(0, Px(6));
    ImVec2 p = ImGui::GetCursorScreenPos();
    float mid = ctrl - btn * 2 - Px(12);
    ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + mid, p.y + btn), ImGui::GetColorU32(kControl), Px(6));
    const char* text = names[std::clamp(value, 0, count - 1)];
    float tw = ImGui::CalcTextSize(text).x;
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + (mid - tw) * 0.5f, p.y + (btn - ImGui::GetTextLineHeight()) * 0.5f),
                                        ImGui::GetColorU32(kText), text);
    ImGui::Dummy(ImVec2(mid, btn));
    ImGui::SameLine(0, Px(6));
    if (ImGui::Button(">", ImVec2(btn, 0))) {
        value = (value + 1) % count;
        changed = true;
    }
    ImGui::PopID();
    return changed;
}

static bool ToggleRow(const char* label, const char* description, int& value);
static bool GatedToggle(const char* label, const char* description, int& value, bool available) {
    if (available)
        return ToggleRow(label, description, value);
    int off = 0;
    ImGui::BeginDisabled();
    ToggleRow(label, description, off);
    ImGui::EndDisabled();
    return false;
}

static bool ToggleRow(const char* label, const char* description, int& value) {
    bool changed = false;
    float w = ImGui::GetContentRegionAvail().x;
    float tw = Px(52), th = Px(28);

    ImGui::PushID(label);
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    if (description != nullptr) {
        ImGui::PushFont(sFontSmall);
        ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - tw - Px(24));
        ImGui::TextWrapped("%s", description);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    ImGui::EndGroup();
    float rowH = ImGui::GetItemRectSize().y;
    ImGui::SameLine(w - tw);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (rowH - th) * 0.5f);
    ImVec2 p = ImGui::GetCursorScreenPos();
    if (ImGui::InvisibleButton("##toggle", ImVec2(tw, th), ImGuiButtonFlags_EnableNav)) {
        value = !value;
        changed = true;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    /* on: the debug menu's black square with the red one inside; off: grey */
    GameSquare(dl, ImVec2(p.x + tw - th * 0.5f, p.y + th * 0.5f), th * 0.8f, value);
    ImGui::PushFont(sFontSmall);
    const char* state = value ? T("S\xC3\xAD", "On") : T("No", "Off");
    ImVec2 ss = ImGui::CalcTextSize(state);
    dl->AddText(ImVec2(p.x + tw - th - Px(6) - ss.x, p.y + (th - ss.y) * 0.5f),
                ImGui::GetColorU32(value ? kText : kMuted), state);
    ImGui::PopFont();
    if (ImGui::IsItemFocused())
        dl->AddRect(ImVec2(p.x - Px(3), p.y - Px(3)), ImVec2(p.x + tw + Px(3), p.y + th + Px(3)),
                    ImGui::GetColorU32(kFocus), 0, 0, Px(2));
    ImGui::PopID();
    return changed;
}

static bool SliderRow(const char* label, int& value, int lo, int hi, const char* format) {
    float w = ImGui::GetContentRegionAvail().x;
    float ctrl = std::min(Px(300), w * 0.5f);

    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(w - ctrl);
    ImGui::SetNextItemWidth(ctrl);
    bool changed = ImGui::SliderInt("##v", &value, lo, hi, format);
    ImGui::PopID();
    return changed;
}

/* ---- pages ------------------------------------------------------------------- */

/* One view per screen (the harness shoots each); the bar groups Graphics, Sound and Controls under Settings, with its
   own tabs (user, 24/10: Home, Enhancements, Settings, Achievements, Save editor). */
enum Page { PAGE_HOME, PAGE_EXTRAS, PAGE_GRAPHICS, PAGE_SOUND, PAGE_CONTROLS, PAGE_ACHIEVEMENTS, PAGE_EDITOR, PAGE_COUNT };
static bool SettingsPage(int page) { return page == PAGE_GRAPHICS || page == PAGE_SOUND || page == PAGE_CONTROLS; }

static int sPage;
static GLuint sBoxArt;
static int sBoxW, sBoxH;
static bool sLaunch;

static GLuint sLogo;
static int sLogoW, sLogoH;

static bool LoadTexture(const fs::path& path, GLuint* tex, int* w, int* h) {
    std::vector<unsigned char> file;
    if (!ReadFile(path, file))
        return false;
    int n;
    unsigned char* rgba = stbi_load_from_memory(file.data(), (int)file.size(), w, h, &n, 4);
    if (rgba == nullptr)
        return false;
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *w, *h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    stbi_image_free(rgba);
    return true;
}

static void LoadBoxArt() {
    fs::path path = sRoot / "assets" / "boxart.png";
    if (!fs::exists(path))
        path = sRoot / "assets" / "boxart.jpg";
    LoadTexture(path, &sBoxArt, &sBoxW, &sBoxH);
    LoadTexture(sRoot / "assets" / "logo.png", &sLogo, &sLogoW, &sLogoH);
}

/* Box art, or a typographic cover when assets/boxart.png is missing. */
static void DrawCover(float w, float h) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (sBoxArt != 0) {
        float s = std::min(w / sBoxW, h / sBoxH);
        float iw = sBoxW * s, ih = sBoxH * s;
        ImGui::SetCursorScreenPos(ImVec2(p.x + (w - iw) * 0.5f, p.y + (h - ih) * 0.5f));
        ImGui::Image((ImTextureID)(intptr_t)sBoxArt, ImVec2(iw, ih));
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h));
        ImGui::Dummy(ImVec2(w, 0));
        return;
    }
    ImVec2 mx(p.x + w, p.y + h);
    dl->AddRectFilledMultiColor(p, mx, Col(0.16f, 0.13f, 0.36f), Col(0.16f, 0.13f, 0.36f), Col(0.05f, 0.04f, 0.11f),
                                Col(0.05f, 0.04f, 0.11f));
    dl->AddRect(p, mx, ImGui::GetColorU32(kBorder), Px(10), 0, 1.0f);
    /* a card outline, the game's motif */
    float cw = w * 0.36f, ch = cw * 1.45f;
    ImVec2 c0(p.x + (w - cw) * 0.5f, p.y + h * 0.16f);
    dl->AddRectFilled(c0, ImVec2(c0.x + cw, c0.y + ch), Col(kAccent.x, kAccent.y, kAccent.z, 0.18f), Px(8));
    dl->AddRect(c0, ImVec2(c0.x + cw, c0.y + ch), Col(kAccent.x, kAccent.y, kAccent.z, 0.8f), Px(8), 0, Px(2));
    ImGui::PushFont(sFontTitle);
    const char* t1 = "KINGDOM HEARTS";
    float t1w = ImGui::CalcTextSize(t1).x;
    float scale = std::min(1.0f, (w - Px(24)) / t1w);
    ImGui::PopFont();
    dl->AddText(sFontTitle, sFontTitle->FontSize * scale, ImVec2(p.x + (w - t1w * scale) * 0.5f, c0.y + ch + Px(18)),
                ImGui::GetColorU32(kText), t1);
    const char* t2 = "Chain of Memories";
    float t2w = sFontBody->CalcTextSizeA(sFontBody->FontSize, FLT_MAX, 0, t2).x;
    dl->AddText(sFontBody, sFontBody->FontSize, ImVec2(p.x + (w - t2w) * 0.5f, c0.y + ch + Px(18) + Px(40) * scale),
                ImGui::GetColorU32(kMuted), t2);
    ImGui::Dummy(ImVec2(w, h));
}

static void CreateDesktopShortcut();
static void AddToSteam();
static void RemoveFromSteam();
static bool InSteam();
static void InstallElsewhereRow();

static void ApplyGraphicsMode(int mode);
static void PickRom(SDL_Window* window);
static void StartRomFolderScan(bool pick);
namespace romdir { static bool Busy(); }

/* A switch on Home for an enhancement that needs files (the HD mod, the Japanese ROM): always shown so players
   know it exists, greyed until its files are there, with a link to where they are loaded (24/10). */
static bool HomeFeatureRow(const char* label, const char* description, int& value, bool available, const char* link) {
    ImVec2 start = ImGui::GetCursorScreenPos();
    float labelW = ImGui::CalcTextSize(label).x, labelH = ImGui::GetTextLineHeight();
    bool changed = GatedToggle(label, nullptr, value, available); /* the description as a tooltip: Home fits one screen */
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s", description);
    if (!available && link != nullptr) { /* on the label's line, after it */
        ImVec2 after = ImGui::GetCursorScreenPos();
        ImGui::PushFont(sFontSmall);
        ImGui::SetCursorScreenPos(ImVec2(start.x + labelW + Px(16), start.y + (labelH - ImGui::GetTextLineHeight()) * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_TextLink, kAccent);
        if (ImGui::TextLink(link))
            changed = true; /* the caller reads the click from here */
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SetCursorScreenPos(after);
        ImGui::Dummy(ImVec2(0, 0));
    }
    return changed;
}

/* A header control drawn like the version rows: panel, a rule under it (black when on), the game's square and an
   optional flag before the label (the user, 24/10: the old grey buttons in the game's style). */
static bool HeaderChip(const char* id, const char* label, const char* flag, ImVec2 size, bool on, float indent = 0) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos(), mx(p.x + size.x, p.y + size.y);
    bool clicked = ImGui::InvisibleButton(id, size, ImGuiButtonFlags_EnableNav);
    bool hover = ImGui::IsItemHovered();
    dl->AddRectFilled(p, mx, ImGui::GetColorU32(on ? kPanelHover : (hover ? kControl : kPanel)));
    dl->AddRectFilled(ImVec2(p.x, mx.y - Px(3)), mx, ImGui::GetColorU32(on || hover ? kText : kBorder));
    if (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible) /* the pad's or keyboard's cursor only, not the mouse's */
        dl->AddRect(ImVec2(p.x - Px(2), p.y - Px(2)), ImVec2(mx.x + Px(2), mx.y + Px(2)), ImGui::GetColorU32(kFocus), 0, 0,
                    Px(2));
    float x = p.x + Px(14) + indent, cy = p.y + size.y * 0.5f - Px(1);
    if (flag != nullptr) {
        GameSquare(dl, ImVec2(x + Px(2), cy), Px(12), on);
        x += Px(16);
        float fh = Px(11);
        LanguageFlag(dl, ImVec2(x, cy - fh * 0.5f), fh, flag);
        x += fh * 1.5f + Px(8);
    }
    float th = sFontMenuSmall->FontSize;
    dl->AddText(sFontMenuSmall, th, ImVec2(x, cy - th * 0.5f), ImGui::GetColorU32(kText), label);
    return clicked;
}

/* The launcher's language: the chosen one in the header, its flag and a ▼; a click lists all six as the same chips */
static void LauncherLanguagePicker(ImVec2 size) {
    if (HeaderChip("##uilang", kUiLanguageNames[sUiLanguage], kUiLanguageFlags[sUiLanguage], size, true) ||
        (sPage == PAGE_HOME && SDL_getenv("DECOMPUI_LANG_LIST"))) /* harness: the list open on Home's shot */
        ImGui::OpenPopup("##uilang-list");
    { /* the list's caret, drawn (no font has it) */
        ImVec2 mx = ImGui::GetItemRectMax(), mn = ImGui::GetItemRectMin();
        float cx = mx.x - Px(18), cy = (mn.y + mx.y) * 0.5f - Px(1), s = Px(5);
        ImGui::GetWindowDrawList()->AddTriangleFilled(ImVec2(cx - s, cy - s * 0.5f), ImVec2(cx + s, cy - s * 0.5f),
                                                      ImVec2(cx, cy + s * 0.6f), ImGui::GetColorU32(kText));
    }
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x - ImGui::GetStyle().WindowPadding.x,
                                   ImGui::GetItemRectMax().y + Px(4))); /* the list's chips under the chosen one */
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, Px(4)));
    if (ImGui::BeginPopup("##uilang-list")) {
        for (int i = 1; i <= UI_LANGUAGE_COUNT; i++) {
            ImGui::PushID(i);
            if (HeaderChip("##lang", kUiLanguageNames[i], kUiLanguageFlags[i], size, sUiLanguage == i)) {
                sUiLanguage = i;
                ApplyUiLanguage();
                SaveLauncherState();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

static void PageHome(SDL_Window* window) {
    float avail = ImGui::GetContentRegionAvail().x, availH = ImGui::GetContentRegionAvail().y;
    float coverW = std::clamp(avail * 0.28f, Px(220), Px(330));

    /* one screen (the user, 24/10): the cover shrinks to the height the page has */
    BeginCard("cover", ImVec2(coverW + Px(48), 0));
    Eyebrow(T("JUEGO", "GAME"));
    float below = Px(34) + ImGui::GetStyle().WindowPadding.y * 2 + Px(16);
    DrawCover(coverW, std::clamp(availH - below, Px(200), coverW * 1.25f));
    ImGui::EndChild();
    ImGui::SameLine(0, Px(20));

    ImGui::BeginGroup();
    /* play first: visible from the start, without scrolling */
    bool ready = GameReady(sSelected);
    if (PlayButton(T("JUGAR", "PLAY"), ImVec2(ImGui::GetContentRegionAvail().x, Px(64)), ready))
        sLaunch = true;
    /* the two secondary actions side by side, under the play bar */
    {
        float gap = Px(10), full = ImGui::GetContentRegionAvail().x;
        float third = ready ? (full - 2 * gap) / 3.0f : full;
        /* roms/ first (anything left there is installed by moving it), the file picker when it holds nothing */
        ImGui::BeginDisabled(romdir::Busy());
        if (ImGui::Button(romdir::Busy() ? T("Buscando en roms...", "Looking in roms...") : T("Instalar ROM...", "Install ROM..."),
                          ImVec2(third, 0)))
            StartRomFolderScan(true);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", T("Primero instala lo que hayas dejado en la carpeta roms (ROMs con cualquier nombre, zips, "
                                      "la carpeta hd del mod HD); si no hay nada, elige el archivo (o arrástralo a esta "
                                      "ventana). Se comprueba su SHA-1. No se incluye ninguna ROM.",
                                      "First installs whatever you left in the roms folder (ROMs under any name, zips, the HD "
                                      "mod's hd folder); if there is nothing, choose the file (or drop it on this window). "
                                      "Its SHA-1 is checked. No ROM is included."));
#ifndef LAUNCHER_ANDROID /* a desktop's: no shortcuts or Steam on a phone */
        if (ready) {
            ImGui::SameLine(0, gap);
            if (ImGui::Button(T("Acceso directo", "Desktop shortcut"), ImVec2(third, 0)))
                CreateDesktopShortcut();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", T("Un acceso \"ReKHCoM\" que abre el juego sin pasar por el launcher.",
                                          "A \"ReKHCoM\" shortcut that opens the game without the launcher."));
            ImGui::SameLine(0, gap);
            /* already there (this folder's entry): the same button takes it out (user 25/10) */
            if (InSteam()) {
                if (ImGui::Button(T("Quitar de Steam", "Remove from Steam"), ImVec2(third, 0)))
                    RemoveFromSteam();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", T("Lo quita de tu biblioteca de Steam (con su portada). Tus partidas no "
                                              "se tocan; puedes volver a añadirlo cuando quieras.",
                                              "Takes it out of your Steam library (with its cover). Your saves are not "
                                              "touched; you can add it again any time."));
            } else {
                if (ImGui::Button(T("Añadir a Steam", "Add to Steam"), ImVec2(third, 0)))
                    AddToSteam();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", T("Lo añade a tu biblioteca de Steam como juego externo, con su portada "
                                              "(para jugar con el mando de Steam o en Steam Deck / Big Picture).",
                                              "Adds it to your Steam library as a non-Steam game, with its cover (to play "
                                              "with Steam Input, Big Picture or on a Steam Deck)."));
            }
        }
#endif
    }
    if (!ready) {
        ImGui::PushFont(sFontSmall);
        ImGui::TextColored(kMuted, "%s",
                           sState[sSelected].installed
                               ? T("No se encontró el ejecutable de esta versión.", "This version's executable is missing.")
                               : T("Instala la ROM de esta versión para jugar.", "Install this version's ROM to play."));
        ImGui::PopFont();
    }
    ImGui::Spacing();
    BeginCard("versions");
    Eyebrow(T("VERSIONES", "VERSIONS"));
    for (int v = 0; v < kVersionCount; v++) {
        const Version& ver = kVersions[v];
        ImGui::PushID(v);
        bool selected = sSelected == v;
        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        float rowW = ImGui::GetContentRegionAvail().x;
        float rowH = Px(50);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (ImGui::InvisibleButton("##row", ImVec2(rowW, rowH), ImGuiButtonFlags_EnableNav) && sState[v].installed) {
            sSelected = v;
            SaveLauncherState();
        }
        bool hover = ImGui::IsItemHovered();
        dl->AddRectFilled(rowMin, ImVec2(rowMin.x + rowW, rowMin.y + rowH),
                          ImGui::GetColorU32(selected ? kPanelHover : (hover ? kControl : kPanel)));
        dl->AddRectFilled(ImVec2(rowMin.x, rowMin.y + rowH - Px(3)), ImVec2(rowMin.x + rowW, rowMin.y + rowH),
                          ImGui::GetColorU32(selected ? kText : kBorder));
        if (ImGui::IsItemFocused())
            dl->AddRect(ImVec2(rowMin.x - Px(2), rowMin.y - Px(2)), ImVec2(rowMin.x + rowW + Px(2), rowMin.y + rowH + Px(2)),
                        ImGui::GetColorU32(kFocus), 0, 0, Px(2));
        /* the debug menu's square, as a radio */
        GameSquare(dl, ImVec2(rowMin.x + Px(22), rowMin.y + rowH * 0.5f), Px(16), selected);
        /* region, code and languages */
        float tx = rowMin.x + Px(44);
        dl->AddText(sFontMenu, sFontMenu->FontSize, ImVec2(tx, rowMin.y + Px(4)), ImGui::GetColorU32(kText),
                    T(ver.regionEs, ver.regionEn));
        {
            /* the code, then each language with its flag */
            float fs = sFontSmall->FontSize, y = rowMin.y + Px(28), x = tx;
            dl->AddText(sFontSmall, fs, ImVec2(x, y), ImGui::GetColorU32(kMuted), ver.code);
            x += sFontSmall->CalcTextSizeA(fs, FLT_MAX, 0, ver.code).x + Px(14);
            for (const char* l = ver.languages; *l; ) {
                const char* colon = std::strchr(l, ':');
                const char* end = std::strchr(l, ' ');
                if (!colon)
                    break;
                if (!end)
                    end = l + std::strlen(l);
                float fh = fs * 0.72f;
                LanguageFlag(dl, ImVec2(x, y + (fs - fh) * 0.5f), fh, l);
                x += fh * 1.5f + Px(4);
                dl->AddText(sFontSmall, fs, ImVec2(x, y), ImGui::GetColorU32(kMuted), colon + 1, end);
                x += sFontSmall->CalcTextSizeA(fs, FLT_MAX, 0, colon + 1, end).x + Px(10);
                l = *end ? end + 1 : end;
            }
        }
        /* status */
        const char* status;
        ImVec4 color;
        if (!sState[v].installed) {
            status = T("Sin ROM", "No ROM");
            color = kMuted;
        } else if (!GameReady(v)) {
            status = T("Falta el ejecutable", "Executable missing");
            color = kWarn;
        } else {
            status = T("Verificada", "Verified");
            color = kGood;
        }
        float sw = sFontSmall->CalcTextSizeA(sFontSmall->FontSize, FLT_MAX, 0, status).x;
        ImVec2 sp(rowMin.x + rowW - sw - Px(18), rowMin.y + (rowH - sFontSmall->FontSize) * 0.5f);
        dl->AddCircleFilled(ImVec2(sp.x - Px(12), sp.y + sFontSmall->FontSize * 0.5f), Px(4.5f),
                            ImGui::GetColorU32(color));
        dl->AddText(sFontSmall, sFontSmall->FontSize, sp, ImGui::GetColorU32(color), status);
        ImGui::PopID();
    }
    if (sSelected == 0) {
        /* the European cartridge's language: chosen here, the game starts in it
           and skips its language screen (game_language, shared with kh_com.ini) */
        const char* langs[] = { T("Preguntar al empezar", "Ask at start"), "English", "Fran\xC3\xA7" "ais", "Deutsch",
                                "Italiano", "Espa\xC3\xB1ol" };
        ImGui::Spacing();
        int language = std::clamp(Ini("game_language"), 0, 5);
        if (CycleRow(T("Idioma del juego", "Game language"),
                     T("El juego pregunta solo la primera vez y recuerda tu elección; aquí puedes cambiarlo.",
                       "The game asks only the first time and remembers your choice; change it here."),
                     language, 6, langs)) {
            Ini("game_language") = language;
            SaveIni();
        }
    }
    {
        std::error_code ec;
        bool hd = fs::exists(sRoot / "hd" / "movies" / "ROP.mp4", ec) || fs::exists(sRoot / "hd" / "music" / "Title.ogg", ec) ||
                  fs::exists(sRoot / "hd" / "sprites" / "gCardWep01Tiles.png", ec);
        int on = Ini("graphics_mode") == 1;
        ImGui::Spacing();
        if (HomeFeatureRow(T("Modo 1.5 HD", "1.5 HD mode"),
                           T("Las cartas, el HUD, la música, los videos y las escenas de Re:Chain of Memories, de tu "
                             "KINGDOM HEARTS HD 1.5+2.5.",
                             "Re:Chain of Memories' cards, HUD, music, movies and scenes, from your KINGDOM HEARTS HD 1.5+2.5."),
                           on, hd, T("Cargar los assets HD en Mejoras \xE2\x86\x92", "Load the HD assets in Enhancements \xE2\x86\x92"))) {
            if (hd) {
                ApplyGraphicsMode(on ? 1 : 2); /* off: GBA enhanced, the mode without HD extras */
                SaveIni();
            } else {
                sPage = PAGE_EXTRAS;
            }
        }
        if (sSelected != 2) { /* the Japanese cartridge already speaks Japanese */
            if (HomeFeatureRow(T("Undub (voces japonesas)", "Undub (Japanese voices)"),
                               T("Las voces japonesas originales con los textos de tu versión. Necesita la ROM japonesa.",
                                 "The original Japanese voices with your version's text. Needs the Japanese ROM."),
                               Ini("undub"), sState[2].installed,
                               T("Instalar la ROM japonesa \xE2\x86\x92", "Install the Japanese ROM \xE2\x86\x92"))) {
                if (sState[2].installed)
                    SaveIni();
                else
                    PickRom(window);
            }
        }
    }
#ifdef LAUNCHER_ANDROID /* no tooltips on a phone: the line stays (on a desktop it is Install ROM's tooltip) */
    ImGui::Spacing();
    ImGui::PushFont(sFontSmall);
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("Elige tu volcado del cartucho en el teléfono. Se comprueba su SHA-1 y se copia a la "
                               "app. No se incluye ninguna ROM.",
                               "Choose your cartridge dump on the phone. Its SHA-1 is checked and it is copied into "
                               "the app. No ROM is included."));
    ImGui::PopStyleColor();
    ImGui::PopFont();
#endif
    ImGui::EndChild();

    ImGui::EndGroup();
}

/* The save's card: bank, file, folder, backup, import, and installing the whole folder elsewhere. On the Saves tab
   (the user, 24/10: Home fits one screen; the saves go to their own manager). */
static void SaveCard(SDL_Window* window) {
    BeginCard("save");
    Eyebrow(T("PARTIDA", "SAVE"));
    {
        static const char* const banks[] = { "1", "2", "3", "4", "5" };
        int bank = std::clamp(Ini("save_bank"), 1, 5) - 1;
        if (CycleRow(T("Banco de partidas", "Save bank"),
                     T("Cinco bancos de las dos ranuras del juego; el 1 es la partida de siempre.",
                       "Five banks of the game's two slots; bank 1 is the usual save."),
                     bank, 5, banks)) {
            Ini("save_bank") = bank + 1;
            SaveIni();
        }
    }

    {
        std::error_code ec;
        fs::path save = SavePath(sSelected);
        bool has = fs::exists(save, ec);
        char line[160];
        if (has) {
#ifdef _WIN32
            FILETIME ft, local;
            SYSTEMTIME st;
            WIN32_FILE_ATTRIBUTE_DATA attr;
            if (GetFileAttributesExW(save.c_str(), GetFileExInfoStandard, &attr)) {
                ft = attr.ftLastWriteTime;
                FileTimeToLocalFileTime(&ft, &local);
                FileTimeToSystemTime(&local, &st);
                std::snprintf(line, sizeof(line), "%s %s  -  %02d/%02d/%04d %02d:%02d", T("Partida:", "Save:"),
                              save.filename().string().c_str(), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
            } else {
#else
            struct stat attr;
            if (stat(save.c_str(), &attr) == 0) {
                std::tm st = *std::localtime(&attr.st_mtime);
                std::snprintf(line, sizeof(line), "%s %s  -  %02d/%02d/%04d %02d:%02d", T("Partida:", "Save:"),
                              save.filename().string().c_str(), st.tm_mday, st.tm_mon + 1, st.tm_year + 1900, st.tm_hour,
                              st.tm_min);
            } else {
#endif
                std::snprintf(line, sizeof(line), "%s %s", T("Partida:", "Save:"), save.filename().string().c_str());
            }
        } else {
            std::snprintf(line, sizeof(line), "%s",
                          T("Sin partida guardada para esta versi\xC3\xB3n.", "No save for this version yet."));
        }
        ImGui::PushFont(sFontSmall);
        ImGui::TextColored(has ? kText : kMuted, "%s", line);
        ImGui::PopFont();
        float half = (ImGui::GetContentRegionAvail().x - Px(8)) * 0.5f;
#ifdef LAUNCHER_ANDROID
        /* no folder to open on a phone: the save goes out through the system's "save as" */
        if (ImGui::Button(T("Exportar partida", "Export save"), ImVec2(half, 0)))
            AndroidPick(ANDROID_PICK_EXPORT, sSelected, std::clamp(Ini("save_bank"), 1, 5));
#else
        if (ImGui::Button(T("Abrir carpeta", "Open folder"), ImVec2(half, 0)))
            ShellExecuteW(nullptr, L"explore", sRoot.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
        ImGui::SameLine(0, Px(8));
        ImGui::BeginDisabled(!has);
        if (ImGui::Button(T("Copia de seguridad", "Back up save"), ImVec2(half, 0)))
            BackupSave(sSelected);
        ImGui::EndDisabled();
        if (ImGui::Button(T("Importar partida de GBA... (experimental)", "Import GBA save... (experimental)"),
                          ImVec2(-FLT_MIN, 0)))
            ImportSave(window, sSelected);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", T("Una partida de emulador (mGBA, VBA, NO$GBA...) o de GameShark / Action Replay. "
                                      "Se convierte a esta versión si viene de otra región; la actual "
                                      "se guarda antes en backups.",
                                      "A save from an emulator (mGBA, VBA, NO$GBA...) or GameShark / Action Replay. "
                                      "It is converted to this version if it comes from another region; the current "
                                      "one is backed up first."));
#ifndef LAUNCHER_ANDROID
        InstallElsewhereRow();
#endif
    }
    ImGui::EndChild();
}

static bool ZoomRow(const char* label, const char* help, int& value, bool inherit) {
    static std::string zoomLabels[PORT_ZOOM_LEVELS + 1];
    const char* zooms[PORT_ZOOM_LEVELS + 1];
    int W = 1920, H = 1080;
    {
        DesktopSize(&W, &H);
        if (Ini("fullscreen") && Ini("fullscreen_width") > 0 && Ini("fullscreen_height") > 0) {
            W = Ini("fullscreen_width"); H = Ini("fullscreen_height");
        } else if (!Ini("fullscreen")) {
            int view = Ini("view_mode");
            int baseW = view == 0 ? 240 : view == 1 ? 284 : view == 3 ? 384 : 320;
            int baseH = view == 0 || view == 1 ? 160 : view == 3 ? 216 : 180;
            W = baseW * std::clamp(Ini("window_scale"), 1, 8);
            H = baseH * std::clamp(Ini("window_scale"), 1, 8);
        }
    }
    int zoomCount = PortZoomCount(1, W, H);
    int offset = inherit ? 1 : 0;
    if (inherit) zooms[0] = T("Igual al zoom principal", "Same as main zoom");
    for (int i = 0; i < zoomCount; i++) {
        int z = PortZoomAt(i, 1, W, H);
        float f = z * 0.5f;
        bool heavy = (W / f) * (H / f) > 640 * 360 * 5 / 4;
        zoomLabels[i] = "x" + std::to_string(z / 2) +
            (heavy ? T("  (exigente)", "  (demanding)") : "");
        if (PortZoomReference(z))
            zoomLabels[i] += std::string(T("  (íntegra x", "  (integer x")) + std::to_string(PortZoomReference(z)) + ")";
        if (z == 12) zoomLabels[i] += "  !";
        zooms[i + offset] = zoomLabels[i].c_str();
    }
    int selected = inherit && !value ? 0 : PortZoomIndex(value, 1, W, H) + offset;
    if (!CycleRow(label, help, selected, zoomCount + offset, zooms)) return false;
    value = selected < offset ? 0 : PortZoomAt(selected - offset, 1, W, H);
    return true;
}

/* A graphics mode's presets, as the game's PortApplyGraphicsMode (port_config.c): set here so that what the player
   changes afterwards in the launcher stays (the game applies a mode only when graphics_mode_applied differs). */
static void ApplyGraphicsMode(int mode) {
    bool hd = mode == 1, classic = mode == 3;
    Ini("graphics_mode") = mode;
    if (mode == 0) /* choose: the game asks on its next start */
        return;
    Ini("graphics_mode_applied") = mode;
    Ini("view_mode") = classic ? 0 : 4;
    /* the HD mod's parts only once imported (the game checks the same files: PortAssetPresent) */
    std::error_code ec;
    auto has = [&](const char* rel) { return fs::exists(sRoot / "hd" / rel, ec); };
    Ini("hd_text") = hd;
    Ini("hd_cards") = Ini("hd_title") = hd && has("sprites/gCardWep01Tiles.png");
    Ini("hd_music") = hd && has("music/Title.ogg");
    Ini("hd_movies") = !classic && has("movies/ROP.mp4"); /* HD intros: on unless GBA classic, as in the game */
    Ini("hd_subtitles") = !classic && has("subs/ROP.us.txt");
    Ini("hd_scenes") = hd && (has("scenes/zz_event_011.mp4") || has("movies/zz_event_011.mp4"));
    Ini("dodge_button") = Ini("stock_button") = !classic;
    Ini("gba_border") = classic ? (Ini("gba_border") ? Ini("gba_border") : 1) : 0;
}

static void PageGraphics() {
    const char* views[] = { T("Original 240x160", "Original 240x160"), T("Panorámica 284x160", "Wide 284x160"),
                            T("Ampliada 320x180", "Extended 320x180"), T("Máxima 384x216", "Maximum 384x216"),
                            T("Auto (llenar ventana)", "Auto (fill window)") };
    const char* filters[] = { T("Nítido", "Sharp"), "Pixel art", T("Suave", "Smooth") };
    const char* huds[] = { T("Original", "Original"), T("Grande", "Large"), T("Más grande", "Larger") };
    const char* narrow[] = { T("Píxel exacto", "Exact pixels"), T("Llenar altura", "Fill height") };
    const char* onoff[] = { T("No", "Off"), T("Sí", "On") };
    const char* scan[] = { T("No", "Off"), "25%", "50%", "75%", "100%" };
    const char* windows[] = { "1x", "2x", "3x", "4x", "5x", "6x", "7x", "8x" };
    bool changed = false;

    BeginCard("display");
    Eyebrow(T("PANTALLA", "DISPLAY"));
    {
        const char* modes[] = { T("Escoger al empezar", "Choose on start"), T("Modo HD", "HD mode"),
                                T("GBA mejorado", "GBA enhanced"), T("GBA clásico", "GBA classic") };
        int mode = std::clamp(Ini("graphics_mode"), 0, 3);
        if (CycleRow(T("Modo gráfico", "Graphics mode"),
                     T("HD: cartas, logo, música y videos de Re:CoM (mod HD importado) y todas las mejoras. GBA mejorado: "
                       "vista amplia y mejoras sin extras HD. GBA clásico: la imagen de GBA a escala entera con borde, "
                       "todo apagado. Elegir un modo pone sus valores; luego puedes cambiar cualquiera.",
                       "HD: Re:CoM's cards, logo, music and movies (imported HD mod) and every improvement. GBA enhanced: "
                       "wide view and improvements without HD extras. GBA classic: the GBA picture at a whole scale with a "
                       "border, everything off. Choosing a mode sets its values; you can change any of them afterwards."),
                     mode, 4, modes)) {
            ApplyGraphicsMode(mode);
            changed = true;
        }
        const char* borders[] = { T("No", "Off"), "1", "2", "3" };
        if (Ini("view_mode") == 0)
            changed |= CycleRow(T("Borde (vista Original)", "Border (Original view)"),
                                T("Un marco decorativo alrededor de la imagen de GBA, como el del Super Game Boy.",
                                  "A decorative frame around the GBA picture, like the Super Game Boy's."),
                                Ini("gba_border"), 4, borders);
    }
    changed |= CycleRow(T("Vista", "View"), T("Auto adapta el encuadre a la ventana.",
                                              "Auto adapts the view to the window."),
                        Ini("view_mode"), 5, views);
    {
        const char* aspects[] = { T("Llenar ventana", "Fill window"), "16:9", "21:9", T("Forzar 21:9", "Force 21:9"),
                                  T("Forzar 32:9", "Force 32:9"), "4:3", "GBA 3:2" };
        changed |= CycleRow(T("Relación de aspecto", "Aspect ratio"),
                            T("En monitores más anchos, 16:9 o 21:9 dejan bandas negras a los lados.",
                              "On wider monitors, 16:9 or 21:9 leave black bars at the sides."),
                            Ini("aspect"), PORT_ASPECT_COUNT, aspects);
    }
    changed |= ZoomRow(T("Zoom (vista Auto)", "Zoom (Auto view)"),
                       T("Escala de píxeles: x4 amplía cada píxel cuatro veces. Menos aumento muestra más mundo y exige más al PC.",
                         "Pixel scale: x4 enlarges each pixel four times. Lower scales show more world and cost more CPU."),
                       Ini("zoom"), false);
    NextCard("picture", T("IMAGEN", "PICTURE"));
#ifdef REKHCOM_SHOW_EXTRA_OPTIONS /* filter: an internal setting */
    changed |= CycleRow(T("Filtro", "Filter"), nullptr, Ini("filter"), 3, filters);
#endif
    changed |= CycleRow(T("Tamaño del HUD", "HUD size"), nullptr, Ini("hud_size"), 3, huds);
    changed |= CycleRow(T("Escenas tamaño GBA", "GBA-size scenes"), nullptr, Ini("narrow_fit"), 2, narrow);
    ImGui::EndChild();
    ImGui::Spacing();
    BeginCard("window");
#ifdef LAUNCHER_ANDROID
    Eyebrow(T("PANTALLA", "SCREEN")); /* a phone has no window, resolution or VSync to choose */
#else
    Eyebrow(T("VENTANA", "WINDOW"));
    changed |= CycleRow(T("Pantalla completa", "Fullscreen"), nullptr, Ini("fullscreen"), 2, onoff);
    {
        /* fullscreen resolution: the monitor's own (desktop) by default */
        static std::vector<std::pair<int, int>> modes = DisplayModes();
        static std::vector<std::string> labels;
        static std::vector<const char*> names;
        int curW = 0, curH = 0;
        DesktopSize(&curW, &curH);
        labels.clear();
        labels.push_back(std::string(T("Escritorio", "Desktop")) + " (" + std::to_string(curW) + "x" +
                         std::to_string(curH) + ")");
        for (auto& m : modes)
            labels.push_back(std::to_string(m.first) + "x" + std::to_string(m.second));
        names.clear();
        for (auto& l : labels)
            names.push_back(l.c_str());
        int sel = 0;
        for (size_t i = 0; i < modes.size(); i++)
            if (modes[i].first == Ini("fullscreen_width") && modes[i].second == Ini("fullscreen_height"))
                sel = (int)i + 1;
        if (CycleRow(T("Resolución completa", "Fullscreen resolution"),
                     T("Escritorio usa la resolución del monitor, también ultrawide.",
                       "Desktop uses the monitor's resolution, ultrawide included."),
                     sel, (int)names.size(), names.data())) {
            Ini("fullscreen_width") = sel ? modes[sel - 1].first : 0;
            Ini("fullscreen_height") = sel ? modes[sel - 1].second : 0;
            changed = true;
        }
    }
    int ws = std::clamp(Ini("window_scale"), 1, 8) - 1;
    if (CycleRow(T("Tamaño de ventana", "Window size"), nullptr, ws, 8, windows)) {
        Ini("window_scale") = ws + 1;
        changed = true;
    }
#ifdef _WIN32
    const char* syncModes[] = { T("No", "Off"), "VSync", "VRR (G-SYNC/FreeSync)" };
    changed |= CycleRow(T("Sincronización", "Synchronization"),
                       T("VRR requiere activarlo en monitor y controlador; usa VSync si la ruta no está disponible.",
                         "VRR must be enabled in the monitor and driver; falls back to VSync if unavailable."),
                       Ini("vsync"), 3, syncModes);
#else
    changed |= CycleRow("VSync", nullptr, Ini("vsync"), 2, onoff);
#endif
#endif
    int sc = std::clamp(Ini("scanlines") / 25, 0, 4);
    if (CycleRow("Scanlines", nullptr, sc, 5, scan)) {
        Ini("scanlines") = sc * 25;
        changed = true;
    }
    NextCard("other", T("OTROS", "OTHER"));
    changed |= CycleRow(T("Colores LCD GBA", "GBA LCD colours"), nullptr, Ini("color_correction"), 2, onoff);
    changed |= CycleRow(T("Mostrar FPS", "Show FPS"), nullptr, Ini("show_fps"), 2, onoff);
#ifndef LAUNCHER_ANDROID
    changed |= CycleRow(T("Estado en Discord", "Discord status"),
                        T("Con Discord abierto, tu perfil muestra el mundo, el piso, el personaje y el nivel.",
                          "With Discord open, your profile shows the world, floor, character and level."),
                        Ini("discord_presence"), 2, onoff);
#endif
    ImGui::EndChild();
    if (changed)
        SaveIni();
}

#include "hd_import.inc"
#include "rom_folder.inc"

static void PageSound() {
    bool changed = false;
    BeginCard("sound");
    Eyebrow(T("SONIDO", "SOUND"));
    changed |= SliderRow(T("Volumen general", "Master volume"), Ini("master_volume"), 0, 100, "%d%%");
    changed |= SliderRow(T("Volumen música", "Music volume"), Ini("music_volume"), 0, 100, "%d%%");
    changed |= SliderRow(T("Volumen efectos", "Effects volume"), Ini("sfx_volume"), 0, 100, "%d%%");
    NextCard("voices", T("VOCES Y HD", "VOICES AND HD"));
    changed |= GatedToggle(T("Undub (voces japonesas)", "Undub (Japanese voices)"),
                         T("Requiere la ROM japonesa original verificada en roms/B8CJ.gba. Se aplica al iniciar el juego; conserva los textos europeos o ingleses.",
                           "Requires the verified original Japanese ROM in roms/B8CJ.gba. Applied when starting the game; keeps European or English text."), Ini("undub"), sState[2].installed);
    
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_TextLink, kAccent);
    if (ImGui::TextLink(T("La música, los videos y las escenas HD de Re:Chain of Memories: en Mejoras →",
                          "Re:Chain of Memories' HD music, movies and scenes: in Enhancements →")))
        sPage = PAGE_EXTRAS;
    ImGui::PopStyleColor();
    ImGui::EndChild();
    if (changed)
        SaveIni();
}

/* The link battle over the network (alpha, port/progress/ONLINE_ESTUDIO.md): netlink.cfg holds one line the game
   reads when the link starts: off, steam:host, steam:join, host:<port>, join:<ip>:<port> or
   relay:<server>:<port>:<CODE>:host|join (a relay by a room code, port/tools/netlink_relay.py: crossplay with Android). */
static void NetlinkRows() {
    static int mode = -1, port = 47777;
    static char ip[64] = "", room[17] = "", name[25] = "";
    static const char* const kSpecs[9] = { "off", "steam:host", "steam:join", "host", "join", "relay-host", "relay-join",
                                           "lobby:steam", "lobby-server" };
    const char* const names[9] = { T("Desactivado (cable)", "Off (cable)"), T("Steam: crear sala", "Steam: open a room"),
                                   T("Steam: unirse a un amigo", "Steam: join a friend"),
                                   T("IP: alojar", "IP: host"), T("IP: unirse", "IP: join"),
                                   T("Relay: crear sala", "Relay: open a room"), T("Relay: unirse a una sala", "Relay: join a room"),
                                   T("Salas en el juego (Steam)", "Rooms in the game (Steam)"),
                                   T("Salas en el juego (servidor)", "Rooms in the game (server)") };
    fs::path cfg = sRoot / "netlink.cfg";
    bool save = false;
    if (mode < 0) {
        std::string line;
        std::ifstream f(cfg);
        std::getline(f, line);
        mode = 0;
        for (int i = 1; i < 3; i++)
            if (line == kSpecs[i])
                mode = i;
        if (line == "lobby:steam" || line == "lobby")
            mode = 7;
        else if (line.rfind("lobby:", 0) == 0) { /* lobby:<server>:<port> */
            size_t colon = line.rfind(':');
            mode = 8;
            snprintf(ip, sizeof(ip), "%s", line.substr(6, colon - 6).c_str());
            port = std::atoi(line.c_str() + colon + 1);
        }
        {
            std::ifstream ini(sRoot / "kh_com.ini");
            std::string l;
            while (std::getline(ini, l))
                if (l.rfind("online_name=", 0) == 0)
                    snprintf(name, sizeof(name), "%s", l.substr(12).c_str());
        }
        if (line.rfind("host:", 0) == 0) {
            mode = 3;
            port = std::atoi(line.c_str() + 5);
        } else if (line.rfind("join:", 0) == 0) {
            size_t colon = line.rfind(':');
            mode = 4;
            snprintf(ip, sizeof(ip), "%s", line.substr(5, colon - 5).c_str());
            port = std::atoi(line.c_str() + colon + 1);
        } else if (line.rfind("relay:", 0) == 0) { /* relay:<server>:<port>:<code>:host|join */
            std::vector<std::string> f;
            size_t at = 6, next;
            while ((next = line.find(':', at)) != std::string::npos) {
                f.push_back(line.substr(at, next - at));
                at = next + 1;
            }
            f.push_back(line.substr(at));
            if (f.size() == 4) {
                mode = f[3] == "host" ? 5 : 6;
                snprintf(ip, sizeof(ip), "%s", f[0].c_str());
                port = std::atoi(f[1].c_str());
                snprintf(room, sizeof(room), "%s", f[2].c_str());
            }
        }
        if (port <= 0 || port > 65535)
            port = 47777;
    }
    save |= CycleRow(T("Enlace en red (alpha)", "Network link (alpha)"),
                     T("La batalla de enlace (Título > Batalla de enlace) por internet en vez del cable. Steam: los dos "
                       "con Steam abierto; uno crea la sala y el otro se une (prefiere la sala de un amigo de Steam). "
                       "IP: quien aloja abre el puerto UDP en su router; el otro escribe su IP. Relay: los dos escriben "
                       "el mismo servidor y el mismo código de sala (también con Android).",
                       "The link battle (Title > Link battle) over the internet instead of the cable. Steam: both with "
                       "Steam open; one opens a room and the other joins (a Steam friend's room first). IP: the host "
                       "opens the UDP port on the router; the other types the host's IP. Relay: both type the same server "
                       "and room code (Android too)."),
                     mode, 9, names);
    if (mode == 4 || mode == 5 || mode == 6 || mode == 8) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(mode >= 5 ? T("Servidor", "Server") : T("IP de quien aloja", "Host's IP"));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Px(220));
        save |= ImGui::InputText("##netlink_ip", ip, sizeof(ip));
    }
    if (mode >= 3 && mode != 7) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(T("Puerto UDP", "UDP port"));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Px(120));
        save |= ImGui::InputInt("##netlink_port", &port, 0, 0);
        port = std::clamp(port, 1, 65535);
    }
    if (mode == 8) { /* the name on the rooms this player opens (Steam shows its own) */
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(T("Tu nombre", "Your name"));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Px(220));
        if (ImGui::InputText("##online_name", name, sizeof(name))) {
            std::vector<std::string> lines;
            std::string l;
            {
                std::ifstream ini(sRoot / "kh_com.ini");
                while (std::getline(ini, l))
                    if (l.rfind("online_name=", 0) != 0)
                        lines.push_back(l);
            }
            std::ofstream out(sRoot / "kh_com.ini", std::ios::trunc);
            for (const std::string& x : lines)
                out << x << "\n";
            if (name[0])
                out << "online_name=" << name << "\n";
        }
    }
    if (mode == 5 || mode == 6) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(T("Código de sala", "Room code"));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(Px(160));
        if (ImGui::InputText("##netlink_room", room, sizeof(room), ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank)) {
            char* w = room;
            for (const char* r = room; *r; r++) /* letters and digits: the relay's codes */
                if ((*r >= 'A' && *r <= 'Z') || (*r >= '0' && *r <= '9'))
                    *w++ = *r;
            *w = 0;
            save = true;
        }
    }
    if (mode == 1 || mode == 2 || mode == 7) {
        std::error_code ec;
        bool dll = fs::exists(sRoot / "bin" / "steam_api.dll", ec) || fs::exists(sRoot / "steam_api.dll", ec);
        ImGui::PushFont(sFontSmall);
        ImGui::TextColored(dll ? kMuted : kAccent, "%s",
                           dll ? T("Usa la app de pruebas de Valve (Spacewar, 480): no hace falta tenerla.",
                                   "Uses Valve's test app (Spacewar, 480): you do not need to own it.")
                               : T("Falta steam_api.dll junto al juego.", "steam_api.dll is missing next to the game."));
        ImGui::PopFont();
    }
    if (mode != 0) { /* the link's own settings, in kh_com.ini */
        static const char* const delays[9] = { "Auto", "1", "2", "3", "4", "5", "6", "7", "8" };
        static const char* const onOff[2] = { "No", T("Sí", "Yes") };
        int delay = std::clamp(Ini("net_delay"), 0, 8), stats = Ini("net_stats") != 0;
        if (CycleRow(T("Retardo de entrada (en línea)", "Input delay (online)"),
                     T("Cuadros de demora de las teclas. Auto: según el ping al conectar (1-2); más alto = menos "
                       "correcciones del rollback con mucho ping. Lo elige quien aloja.",
                       "Frames of delay on the keys. Auto: from the ping when linking (1-2); higher = fewer rollback "
                       "corrections on a high ping. The host's choice counts."),
                     delay, 9, delays)) {
            sIni["net_delay"] = delay;
            SaveIni();
        }
        if (CycleRow(T("Mostrar el estado de la red", "Show the network state"),
                     T("Arriba a la izquierda durante el enlace: ping, retardo y el último rollback.",
                       "Top left during the link: ping, delay and the last rollback."),
                     stats, 2, onOff)) {
            sIni["net_stats"] = stats;
            SaveIni();
        }
    }
    if (save) {
        std::ofstream f(cfg, std::ios::trunc);
        if (mode == 3)
            f << "host:" << port << "\n";
        else if (mode == 4)
            f << "join:" << ip << ":" << port << "\n";
        else if ((mode == 5 || mode == 6) && ip[0] && room[0])
            f << "relay:" << ip << ":" << port << ":" << room << ":" << (mode == 5 ? "host" : "join") << "\n";
        else if (mode == 5 || mode == 6)
            f << "off\n"; /* until the server and the code are written */
        else if (mode == 7)
            f << "lobby:steam\n";
        else if (mode == 8 && ip[0])
            f << "lobby:" << ip << ":" << port << "\n";
        else if (mode == 8)
            f << "off\n"; /* until the server is written */
        else
            f << kSpecs[mode] << "\n";
    }
}

static void PageExtras() {
    bool changed = false;
    /* the HD mod first: loading Re:Chain of Memories' assets was hidden under Sound (user, 24/10) */
    BeginCard("hdmod");
    changed |= HdModCard();
    ImGui::EndChild();
    ImGui::Spacing();
    BeginCard("extras");
    Eyebrow(T("MEJORAS", "ENHANCEMENTS"));
#ifdef REKHCOM_SHOW_EXTRA_OPTIONS /* raised cards: an internal setting */
    {
        static const char* raise[] = { "Off", "4 px", "8 px", "12 px", "16 px" };
        int r = std::clamp(Ini("card_hud_raise") / 4, 0, 4);
        if (CycleRow(T("Subir cartas en combate", "Battle cards raised"),
                     T("La mano de cartas queda en parte bajo el borde de la pantalla; subirla muestra más de cada "
                       "carta.",
                       "The card hand sits partly below the screen edge; raising it shows more of each card."),
                     r, 5, raise)) {
            Ini("card_hud_raise") = r * 4;
            changed = true;
        }
    }
#endif
    ImGui::Separator();
    changed |= ToggleRow(T("Voltereta con un botón", "One-button dodge roll"),
                         T("X en el mando, C en el teclado: rueda hacia la dirección que mantienes o hacia donde "
                           "mira el personaje.",
                           "Gamepad X, keyboard C: rolls toward the held direction or the way the character faces."),
                         Ini("dodge_button"));
    ImGui::Separator();
    changed |= ToggleRow(T("Acumular con un botón (L+R)", "One-button stock (L+R)"),
                         T("Y en el mando, D en el teclado: acumula cartas o trucos sin pulsar L y R a la vez.",
                           "Gamepad Y, keyboard D: stock cards or sleights without pressing L and R together."),
                         Ini("stock_button"));
    ImGui::Separator();
    ImGui::EndChild();
    ImGui::Spacing();
    BeginCard("experimental");
    Eyebrow(T("EXPERIMENTALES", "EXPERIMENTAL"));
    changed |= ZoomRow(T("Zoom en combate", "Battle zoom"),
                       T("Solo en vista Auto. Al salir del combate vuelve al zoom principal.",
                         "Auto view only. Leaving battle restores the main zoom."),
                       Ini("experimental_battle_zoom"), true);
    ImGui::Separator();
    changed |= ToggleRow(T("Devolver cartas acumuladas", "Return stocked cards"),
                         T("Solo Sora: mantén V, pulsa el stick derecho o inclínalo hacia abajo durante medio segundo para cancelar la acumulación y devolver esas cartas a la mano. No cancela técnicas en curso. Desactivado por defecto.",
                           "Sora only: hold V, the right-stick button or right stick down for half a second to cancel the stock and return those cards to the hand. Cannot cancel a sleight in progress. Off by default."),
                         Ini("experimental_unstock"));
    ImGui::Separator();
    changed |= ToggleRow(T("Elegir ranura en los puntos de guardado", "Choose the file at save points"),
                         T("En el cristal de guardado, arriba/abajo cambia entre las dos ranuras del personaje. Guardar en la otra la convierte en la partida actual. Desactivado por defecto.",
                           "At the save crystal, up/down switches between the character's two files. Saving to the other one makes it the current game. Off by default."),
                         Ini("experimental_save_point_slot_select"));
    ImGui::Separator();
    changed |= ToggleRow(T("Aviso de puntos moguri", "Moogle points notice"),
                         T("Al recoger puntos moguri en el campo, muestra cuántos y el total. Desactivado por defecto.",
                           "Picking up moogle points on the field shows how many and the total. Off by default."),
                         Ini("experimental_moogle_notice"));
    ImGui::Separator();
    changed |= ToggleRow(T("Invertir selección de cartas", "Invert card scrolling"),
                         T("Invierte L/R y el movimiento horizontal del stick derecho al seleccionar cartas en combate. Los menús y L+R no cambian. Desactivado por defecto.",
                           "Reverses L/R and right-stick horizontal card scrolling in battle. Menus and L+R combinations stay unchanged. Off by default."),
                         Ini("invert_card_scroll"));
    ImGui::Separator();
    changed |= ToggleRow(T("Saltar cuadros si va lento", "Skip frames when slow"),
                         T("Si el equipo no llega a tiempo, ese cuadro no se dibuja: la lógica y el sonido siguen a la velocidad original. Comprobado: la lógica queda idéntica. Activado por defecto.",
                           "When the device falls behind, that frame is not drawn: the logic and the sound keep the original speed. Verified: the logic stays identical. On by default."),
                         Ini("experimental_frame_skip"));
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("En pruebas: pueden tener fallos visuales.", "Being tested: expect visual glitches."));
    ImGui::PopStyleColor();
    changed |= ToggleRow(T("Texto HD", "HD text"),
                         T("Los diálogos se dibujan con una fuente nítida a la resolución de la ventana, en el "
                           "mismo lugar y colores del juego.",
                           "Dialogue is drawn with a crisp font at the window's resolution, in the game's own "
                           "places and colours."),
                         Ini("hd_text"));
    if (Ini("hd_text")) {
        /* the HD text's face (art review, 24/10): the user's KHMenu by default, two others to choose from */
        /* KHMenu or the system's; Comic Hearts is the subtitles' (user 25/10). The ini keeps 0 / 2. */
        static const char* fontsEs[2] = { "KHMenu", "Sistema" };
        static const char* fontsEn[2] = { "KHMenu", "System" };
        int pick = Ini("hd_font") == 2 ? 1 : 0;
        if (CycleRow(T("Fuente del texto HD", "HD text font"),
                     T("KHMenu es la del juego; Sistema, la del equipo. Los subtítulos usan siempre la suya.",
                       "KHMenu is the game's; System is the computer's own. Subtitles always keep theirs."),
                     pick, 2, sSpanish ? fontsEs : fontsEn)) {
            Ini("hd_font") = pick ? 2 : 0;
            changed = true;
        }
    }
    ImGui::Separator();
#ifdef REKHCOM_SHOW_HD_HUD /* HD HUD pending: hidden, and the game forces it off (KHCOM_HD_HUD_DEV=1 for development) */
    changed |= ToggleRow(T("HUD HD (piloto)", "HD HUD (pilot)"),
                         T("Marco de vida del enemigo redibujado en HD. Piloto para vista panoramica; los demas elementos conservan sus graficos originales.",
                           "Redrawn HD enemy HP frame. Pilot for wide views; other elements keep their original graphics."),
                         Ini("hd_hud"));
    ImGui::Separator();
#endif
#ifdef REKHCOM_SHOW_HD2D /* HD-2D pending: hidden, and the game forces it off (KHCOM_HD2D_DEV=1 for development) */
    changed |= ToggleRow(T("Estilo HD-2D", "HD-2D look"),
                         T("Campo y combates con profundidad de campo, resplandor, gradación de color y viñeta, al "
                           "estilo de Octopath Traveler. Los sprites quedan nítidos.",
                           "Field and battles with depth of field, bloom, colour grading and a vignette, in the "
                           "manner of Octopath Traveler. Sprites stay sharp."),
                         Ini("hd2d"));
    ImGui::Separator();
#endif
    changed |= ToggleRow(T("Filtro de audio", "Audio filter"),
                         T("Interpolación cúbica de las muestras y un filtro paso bajo suave: menos aspereza metálica "
                           "en los agudos que dejan las bajas frecuencias de muestreo de la GBA.",
                           "Cubic sample interpolation and a gentle low-pass: less of the metallic edge the GBA's "
                           "low sample rates leave in the treble."),
                         Ini("smooth_audio"));
    ImGui::Separator();
#ifdef REKHCOM_SHOW_EXTRA_OPTIONS /* Re:CoM is chosen at New Game, per save */
    changed |= ToggleRow(T("Modo Re:CoM", "Re:CoM mode"),
                         T("Al empezar una partida nueva puedes elegir la versión: la original de GBA o la Re:CoM, "
                           "con mejoras de Kingdom Hearts Re:Chain of Memories (PS2) como niveles de dificultad y "
                           "diálogos rápidos. Cada partida recuerda la suya. Apagado, todo queda como el "
                           "cartucho original.",
                           "When starting a new game you can choose the version: the original GBA one or Re:CoM, with "
                           "improvements from Kingdom Hearts Re:Chain of Memories (PS2) such as difficulty levels and "
                           "faster dialogue. Each game remembers its own. Off, everything stays as the original "
                           "cartridge."),
                         Ini("ps2_mode"));
#endif
    ImGui::Separator();
    NetlinkRows();
    ImGui::EndChild();
    if (changed)
        SaveIni();
}

/* A binding cell waiting for a key or button (PageControls, MainLoop). */
static std::string sCapture;
static bool sCapturePad;

static const char* PadName(int b) {
    static const char* const names[] = { "A", "B", "X", "Y", "Back", "Guide", "Start", "L3", "R3", "LB", "RB",
                                         "\xE2\x86\x91", "\xE2\x86\x93", "\xE2\x86\x90", "\xE2\x86\x92" };
    if (b < 0)
        return "-";
    if (b >= PORT_PAD_AXIS_BASE) {
        int encoded = b - PORT_PAD_AXIS_BASE, axis = encoded / 2;
        static char label[24];
        static const char* axes[] = { "Left X", "Left Y", "Right X", "Right Y", "LT", "RT" };
        if (axis >= SDL_GAMEPAD_AXIS_COUNT) return "?";
        std::snprintf(label, sizeof(label), "%s %s", axes[axis], (encoded & 1) ? "-" : "+");
        return label;
    }
    if (b < (int)(sizeof(names) / sizeof(names[0])))
        return names[b];
    return SDL_GetGamepadStringForButton((SDL_GamepadButton)b);
}

#include "controller_probe.inc"

/* Event for the cell being captured: true when it was used. */
static bool CaptureEvent(const SDL_Event& ev) {
    if (sCapture.empty())
        return false;
    if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
        if (ev.key.key == SDLK_ESCAPE) {
            sCapture.clear();
        } else if (!sCapturePad) {
            Ini(sCapture.c_str()) = ev.key.key == SDLK_DELETE ? 0 : (int)ev.key.key;
            sCapture.clear();
            SaveIni();
        } else if (ev.key.key == SDLK_DELETE) {
            Ini(sCapture.c_str()) = -1;
            sCapture.clear();
            SaveIni();
        }
        return true;
    }
    if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && sCapturePad) {
        /* stored as the game reads it: on a Nintendo pad the button labelled A is A */
        SDL_Event normalized = ev;
        KhPadNormalizeEvent(&normalized, Ini("pad_layout"));
        Ini(sCapture.c_str()) = normalized.gbutton.button;
        sCapture.clear();
        SaveIni();
        return true;
    }
    if (ev.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && sCapturePad && std::abs(ev.gaxis.value) > 16000) {
        Ini(sCapture.c_str()) = PORT_PAD_AXIS_BASE + ev.gaxis.axis * 2 + (ev.gaxis.value < 0);
        sCapture.clear();
        SaveIni();
        return true;
    }
    return false;
}

static bool BindingConflict(const char* key, bool pad) {
    const int value = Ini(key);
    if (pad ? value < 0 : value == 0)
        return false;
    for (const Setting& st : kSettings) {
        const bool isPad = std::strncmp(st.key, "pad_", 4) == 0;
        const bool isKeyboard = std::strncmp(st.key, "kb_", 3) == 0;
        if (std::strcmp(st.key, key) != 0 && ((pad && isPad) || (!pad && isKeyboard)) && Ini(st.key) == value)
            return true;
    }
    return false;
}

#include "pad_diagram.inc"

static std::string sPadFocus = "pad_a"; /* the action whose gamepad binding the drawing lights */

static void BindCell(const char* key, bool pad) {
    bool waiting = sCapture == key;
    bool conflict = !waiting && BindingConflict(key, pad);
    std::string label;
    if (waiting) {
        label = T("Pulsa...", "Press...");
    } else if (pad) {
        label = PadSymbol(Ini(key));
    } else {
        int k = Ini(key);
        label = k == 0 ? "-" : SDL_GetKeyName((SDL_Keycode)k);
        if (label.empty())
            label = "?";
    }
    label += "##";
    label += key;
    if (waiting)
        ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
    if (conflict)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.18f, 0.16f, 1.0f));
    if (ImGui::Button(label.c_str(), ImVec2(-FLT_MIN, 0))) {
        sCapture = key;
        sCapturePad = pad;
    }
    if (conflict && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", T("Conflicto: otra acción usa esta asignación", "Conflict: another action uses this binding"));
    if (conflict)
        ImGui::PopStyleColor();
    if (waiting)
        ImGui::PopStyleColor();
}

static void PageControls() {
#ifdef LAUNCHER_ANDROID
    {
        /* the touch screen's buttons (touch_android.c): their look and whether they show; moving them is the game's */
        static const char* const themes[3] = { "Clear", "Dark", "Color" };
        const char* onoff[2] = { T("No", "Off"), T("Sí", "On") };
        bool changed = false;
        BeginCard("touch");
        Eyebrow(T("BOTONES TÁCTILES", "TOUCH BUTTONS"));
        changed |= CycleRow(T("Tema de botones", "Button theme"), nullptr, Ini("touch_theme"), 3, themes);
        changed |= CycleRow(T("Ocultar con un mando conectado", "Hide with a controller connected"),
                            T("Sin mando conectado, el botón de menú sigue visible: es la forma de abrir las opciones.",
                              "With no controller connected the menu button stays: it is the way into the options."),
                            Ini("experimental_hide_touch_controls"), 2, onoff);
        ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
        ImGui::TextWrapped("%s", T("Para moverlos: en el juego, Opciones > Mover botones táctiles; arrástralos y toca el visto.",
                                   "To move them: in the game, Options > Move touch buttons; drag them and tap the tick."));
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::Spacing();
        if (changed)
            SaveIni();
    }
#endif
    ControllerProbePanel();
    ImGui::Spacing();
    struct Row {
        const char* es;
        const char* en;
        const char* name;
    };
    static const Row rows[] = {
        { "A", "A", "a" },
        { "B", "B", "b" },
        { "L", "L", "l" },
        { "R", "R", "r" },
        { "Start", "Start", "start" },
        { "Select", "Select", "select" },
        { "Arriba", "Up", "up" },
        { "Abajo", "Down", "down" },
        { "Izquierda", "Left", "left" },
        { "Derecha", "Right", "right" },
        { "L + R (acumular)", "L + R (stock)", "stock" },
        { "Voltereta", "Dodge roll", "dodge" },
        { "Devolver acumuladas (experimental)", "Return stocked (experimental)", "unstock" },
    };
    BeginCard("controls");
    Eyebrow(T("CONTROLES", "CONTROLS"));
    if (!sCapture.empty() && sCapture.compare(0, 4, "pad_") == 0)
        sPadFocus = sCapture;
    PadDiagram(sPadFocus.c_str());
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("Haz clic en una casilla y pulsa la tecla, botón o eje. Supr la deja vacía, Esc cancela. "
                               "Las asignaciones repetidas se marcan en rojo.",
                               "Click a cell and press a key, button, or axis. Delete clears it, Esc cancels. "
                               "Duplicate bindings are marked in red."));
    ImGui::PopStyleColor();
    if (ImGui::BeginTable("binds", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_PadOuterX)) {
        ImGui::TableSetupColumn(T("Acción", "Action"), ImGuiTableColumnFlags_WidthStretch, 1.3f);
        ImGui::TableSetupColumn(T("Teclado", "Keyboard"), ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn(T("Teclado 2", "Keyboard 2"), ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn(T("Mando", "Gamepad"), ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
        ImGui::TableHeadersRow();
        ImGui::PopStyleColor();
        for (const Row& r : rows) {
            std::string kb = std::string("kb_") + r.name, kb2 = kb + "_2", pad = std::string("pad_") + r.name;
            ImGui::TableNextRow(0, Px(36));
            bool focus = false;
            auto seen = [&]() { focus = focus || ImGui::IsItemHovered() || ImGui::IsItemFocused(); };
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(T(r.es, r.en));
            seen();
            ImGui::TableNextColumn();
            BindCell(kb.c_str(), false);
            seen();
            ImGui::TableNextColumn();
            BindCell(kb2.c_str(), false);
            seen();
            ImGui::TableNextColumn();
            BindCell(pad.c_str(), true);
            seen();
            /* the row under the pointer or with the keyboard/gamepad focus */
            if (focus)
                sPadFocus = pad;
        }
        ImGui::EndTable();
    }
    ImGui::Spacing();
    if (ImGui::Button(T("Restablecer controles", "Reset controls"))) {
        for (const Setting& st : kSettings) {
            if (std::strncmp(st.key, "kb_", 3) == 0 || std::strncmp(st.key, "pad_", 4) == 0)
                Ini(st.key) = st.def;
        }
        SaveIni();
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("Fijos: Opciones F1 / Esc o Start + Select (en el inicio, solo Select), pantalla completa F11 o Alt+Enter, "
                               "estados F5 / F8 (si se activan en Trucos), zoom (vista Auto) + / - o Select + arriba / abajo en el mando.",
                               "Fixed: options F1 / Esc or Start + Select (on the title screen, Select alone), fullscreen F11 or Alt+Enter, states "
                               "F5 / F8 (when turned on in Cheats), zoom (Auto view) + / - or Select + Up / Down on the gamepad."));
    ImGui::PopStyleColor();
    ImGui::EndChild();
    /* Issue #23: a Nintendo pad the system reports as an Xbox one (Steam Input, Lutris) */
    ImGui::Spacing();
    BeginCard("nintendo");
    Eyebrow(T("MANDO NINTENDO", "NINTENDO CONTROLLER"));
    int force = Ini("pad_layout") == 2;
    if (ToggleRow(T("Forzar mando Nintendo", "Force Nintendo layout"), nullptr, force)) {
        Ini("pad_layout") = force ? 2 : 0; /* off: detect the pad */
        SaveIni();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("Los mandos de Nintendo (Switch Pro, Joy-Con, 8BitDo en modo Switch) tienen A y B, X e Y al revés que los de Xbox. "
                               "ReKHCoM los detecta solo; activa esta opción si el tuyo se toma por un mando de Xbox (por ejemplo con Steam o Lutris). "
                               "Activada: el botón rotulado A es la A del juego, B es B, X es X e Y es Y, en el juego y en este launcher. "
                               "Las asignaciones de la tabla de arriba no cambian.",
                               "Nintendo pads (Switch Pro, Joy-Con, 8BitDo in Switch mode) have A and B, X and Y the other way round from Xbox pads. "
                               "ReKHCoM detects them; turn this on if yours is taken for an Xbox pad (e.g. through Steam or Lutris). "
                               "On: the button labelled A is the game's A, B is B, X is X and Y is Y, in the game and in this launcher. "
                               "The bindings in the table above stay as they are."));
    ImGui::PopStyleColor();
    ImGui::EndChild();
}

/* ---- header and frame ---------------------------------------------------------- */

static void ReportProblemChip(ImVec2 size);

/* One band, as the web page's header: the logo and the project line beside it, report and language on the right;
   then the tabs between two rules (the active one boxed, with the game's red square). */
static void Header() {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 top = ImGui::GetCursorScreenPos();
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    float logoH = Px(58);
    float x = top.x;
    if (sLogo != 0) {
        float lw = logoH * sLogoW / sLogoH;
        ImGui::Image((ImTextureID)(intptr_t)sLogo, ImVec2(lw, logoH));
        x += lw + Px(22);
    } else {
        ImGui::Dummy(ImVec2(0, logoH));
    }
    /* the project line, centred on the logo's height */
    {
        ImGui::PushFont(sFontMenu);
        float lh = ImGui::GetTextLineHeight();
        float y = top.y + logoH * 0.5f - lh;
        dl->AddText(ImVec2(x, y), ImGui::GetColorU32(kAccent), "REKHCOM  \xC2\xB7  DECOMP PORT  \xC2\xB7  " REKHCOM_VERSION);
        ImGui::PopFont();
        ImGui::PushFont(sFontSmall);
        char by[64];
        std::snprintf(by, sizeof(by), "%s MatyMeatBoy", T("por", "by"));
        dl->AddText(ImVec2(x, y + lh + Px(4)), ImGui::GetColorU32(kMuted), by);
        ImGui::PopFont();
    }
    /* at the right end of the band: report a problem, and the launcher's language (the user, 24/10: both up here, in
       the game's style) */
    {
        ImVec2 after = ImGui::GetCursorScreenPos();
        float h = Px(40), langW = Px(170), gap = Px(10);
        float y = top.y + (logoH - h) * 0.5f;
        ImGui::PushFont(sFontMenuSmall);
        float reportW = ImGui::CalcTextSize(T("Reportar un problema...", "Report a problem...")).x + Px(28) + Px(32);
        ImGui::PopFont();
        ImGui::SetCursorScreenPos(ImVec2(right - langW - gap - reportW, y));
        ReportProblemChip(ImVec2(reportW, h));
        ImGui::SetCursorScreenPos(ImVec2(right - langW, y));
        LauncherLanguagePicker(ImVec2(langW, h));
        ImGui::SetCursorScreenPos(after);
    }
    static int lastSettings = PAGE_GRAPHICS; /* Settings reopens on its last tab */
    if (SettingsPage(sPage))
        lastSettings = sPage;
    enum { TAB_COUNT = 5 };
    const char* tabs[TAB_COUNT] = { T("Inicio", "Home"), T("Mejoras", "Enhancements"), T("Configuración", "Settings"),
                                    T("Logros", "Achievements"), T("Gestor de partidas", "Save manager") };
    const int tabPage[TAB_COUNT] = { PAGE_HOME, PAGE_EXTRAS, -1, PAGE_ACHIEVEMENTS, PAGE_EDITOR };
    ImGui::SetCursorScreenPos(ImVec2(top.x, top.y + logoH + Px(10)));
    ImVec2 band = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(band, ImVec2(right, band.y + Px(3)), ImGui::GetColorU32(kText));
    ImGui::Dummy(ImVec2(0, Px(3)));
    ImGui::PushFont(sFontMenu);
    float tabH = ImGui::GetTextLineHeight() + Px(20);
    ImVec2 row = ImGui::GetCursorScreenPos();
    float tx = row.x;
    for (int i = 0; i < TAB_COUNT; i++) {
        bool active = tabPage[i] < 0 ? SettingsPage(sPage) : sPage == tabPage[i];
        float sq = active ? Px(12) : 0, pad = Px(14);
        float w = ImGui::CalcTextSize(tabs[i]).x + pad * 2 + (active ? sq + Px(8) : 0);
        if (tx + w > right) { /* a narrow window: the tabs wrap */
            tx = row.x;
            row.y += tabH;
        }
        ImGui::SetCursorScreenPos(ImVec2(tx, row.y));
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##tab", ImVec2(w, tabH), ImGuiButtonFlags_EnableNav))
            sPage = tabPage[i] < 0 ? lastSettings : tabPage[i];
        bool hover = ImGui::IsItemHovered();
        ImGui::PopID();
        ImVec2 mn(tx, row.y), mx(tx + w, row.y + tabH);
        if (active) {
            dl->AddRectFilled(mn, mx, ImGui::GetColorU32(kPanelHover));
            dl->AddRectFilled(ImVec2(mn.x, mx.y - Px(3)), mx, ImGui::GetColorU32(kAccent));
            GameSquare(dl, ImVec2(mn.x + pad + sq * 0.5f, row.y + tabH * 0.5f), sq, true);
        } else if (hover) {
            dl->AddRectFilled(ImVec2(mn.x, mx.y - Px(3)), mx, ImGui::GetColorU32(kBorder));
        }
        dl->AddText(ImVec2(mn.x + pad + (active ? sq + Px(8) : 0), row.y + Px(10)), ImGui::GetColorU32(kText), tabs[i]);
        tx += w + Px(4);
    }
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(band.x, row.y + tabH));
    ImVec2 rule = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(rule, ImVec2(right, rule.y + Px(1)), ImGui::GetColorU32(kBorder));
    ImGui::Dummy(ImVec2(0, Px(14)));
}

#include "save_editor_page.inc"
#include "achievements_page.inc"

/* Settings: Graphics, Sound and Controls as tabs inside it, drawn as the launcher's buttons with the game's square */
static void SettingsTabs() {
    const char* names[3] = { T("Gráficos", "Graphics"), T("Sonido", "Sound"), T("Controles", "Controls") };
    const int pages[3] = { PAGE_GRAPHICS, PAGE_SOUND, PAGE_CONTROLS };
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < 3; i++) {
        bool active = sPage == pages[i];
        if (i)
            ImGui::SameLine(0, Px(6));
        ImGui::PushStyleColor(ImGuiCol_Button, active ? kPanelHover : kControl);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kControlHover);
        char label[64];
        snprintf(label, sizeof(label), "%s%s", active ? "    " : "", names[i]);
        if (ImGui::Button(label, ImVec2(0, Px(34))))
            sPage = pages[i];
        ImGui::PopStyleColor(2);
        if (active) {
            ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
            GameSquare(dl, ImVec2(mn.x + Px(14), (mn.y + mx.y) * 0.5f), Px(10), true);
            dl->AddRectFilled(ImVec2(mn.x, mx.y - Px(3)), mx, ImGui::GetColorU32(kAccent));
        }
    }
    ImGui::Spacing();
}

static void Footer() {
    if (!sNotice.empty() && ImGui::GetTime() - sNoticeTime < 8.0) {
        ImGui::PushFont(sFontSmall);
        ImGui::TextColored(sNoticeError ? kBad : kGood, "%s", sNotice.c_str());
        ImGui::PopFont();
    }
}

static void Frame(SDL_Window* window) {
    KhRaPump(sRaUi, 0);
    ControllerProbeTick();
    ImGuiIO& io = ImGui::GetIO();
    Background(io.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("decompui", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    Header();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    if (const char* scroll = SDL_getenv("DECOMPUI_SCROLL")) /* harness: a lower part of the page */
        ImGui::SetNextWindowScroll(ImVec2(0, (float)std::atof(scroll)));
    ImGui::BeginChild("page", ImVec2(0, -Px(28)), ImGuiChildFlags_None, ImGuiWindowFlags_None);
    ImGui::PopStyleColor();
    if (SettingsPage(sPage))
        SettingsTabs();
    switch (sPage) {
        case PAGE_HOME:
            PageHome(window);
            break;
        case PAGE_GRAPHICS:
            PageGraphics();
            break;
        case PAGE_SOUND:
            PageSound();
            break;
        case PAGE_EXTRAS:
            PageExtras();
            break;
        case PAGE_CONTROLS:
            PageControls();
            break;
        case PAGE_EDITOR:
            PageEditor(window);
            break;
        case PAGE_ACHIEVEMENTS:
            PageAchievements();
            break;
    }
    ImGui::EndChild();
    RomFolderPoll(window);
    HdOfferPopup();
    HdQuitPopup();
    HdResumePopup();
    Footer();
    ImGui::End();
    AchievementPreviewDraw();
}

/* ---- launching --------------------------------------------------------------- */

static std::wstring Quote(const std::wstring& s) {
    std::wstring out = L"\"";
    unsigned slashes = 0;
    for (wchar_t c : s) {
        if (c == L'\\') {
            slashes++;
            continue;
        }
        out.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        out += c;
        slashes = 0;
    }
    out.append(slashes * 2, L'\\');
    return out + L"\"";
}

/* A desktop shortcut that starts this version's game directly, like the Play
   button (shared data folder, its ROM; the language comes from kh_com.ini),
   without opening the launcher. */
static void CreateDesktopShortcut() {
#ifdef _WIN32
    const VersionState& st = sState[sSelected];
    const char* region = kVersions[sSelected].regionEn;
    PWSTR desktop = nullptr;
    bool ok = false;

    const char* testDir = SDL_getenv("DECOMPUI_SHORTCUT"); /* harness: write it there instead, then exit */
    fs::path folder;
    if (!st.installed || !fs::exists(st.exe))
        return;
    if (testDir) {
        folder = fs::u8path(testDir);
    } else {
        if (FAILED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktop)))
            return;
        folder = fs::path(desktop);
        CoTaskMemFree(desktop);
    }
    /* "ReKHCoM"; the region only tells them apart when more than one version is installed */
    int installed = 0;
    for (const VersionState& v : sState)
        installed += v.installed;
    std::wstring name = L"ReKHCoM";
    if (installed > 1)
        name += L" (" + std::wstring(region, region + std::strlen(region)) + L")";
    fs::path link = folder / (name + L".lnk");
    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IShellLinkW* shell = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&shell))) {
        std::wstring args = L"--data-dir " + Quote(sRoot.wstring()) + L" --rom " + Quote(RomPath(sSelected).wstring());
        IPersistFile* file = nullptr;
        shell->SetPath(st.exe.wstring().c_str());
        shell->SetArguments(args.c_str());
        shell->SetWorkingDirectory(sRoot.wstring().c_str());
        shell->SetIconLocation((sRoot / "ReKHCoM.exe").wstring().c_str(), 0);
        shell->SetDescription(L"Kingdom Hearts: Chain of Memories (ReKHCoM)");
        if (SUCCEEDED(shell->QueryInterface(IID_IPersistFile, (void**)&file))) {
            ok = SUCCEEDED(file->Save(link.wstring().c_str(), TRUE));
            file->Release();
        }
        shell->Release();
    }
    if (SUCCEEDED(com))
        CoUninitialize();
    Notice(ok ? T("Acceso directo creado en el escritorio: abre el juego sin el launcher.",
                  "Shortcut created on the desktop: it opens the game without the launcher.")
              : T("No se pudo crear el acceso directo.", "The shortcut could not be created."),
           !ok);
#endif
}

/* ---- "Add to Steam": a non-Steam game in Steam's shortcuts.vdf (binary KeyValues: 0x00 a map, 0x01 a string,
   0x02 an int32, 0x07 a uint64, 0x08 the map's end; names and strings end with a zero) ---- */
struct Vdf {
    unsigned char type = 0;
    std::string text;
    uint64_t number = 0;
    std::vector<std::pair<std::string, Vdf>> items;
    Vdf* Find(const std::string& key) {
        for (auto& kv : items)
            if (_stricmp(kv.first.c_str(), key.c_str()) == 0)
                return &kv.second;
        return nullptr;
    }
    Vdf& Set(const std::string& key, unsigned char t) {
        Vdf* v = Find(key);
        if (!v) {
            items.emplace_back(key, Vdf());
            v = &items.back().second;
        }
        v->type = t;
        return *v;
    }
};

static bool VdfRead(const std::string& d, size_t& at, Vdf& map) {
    auto str = [&](std::string& out) {
        size_t end = d.find('\0', at);
        if (end == std::string::npos)
            return false;
        out = d.substr(at, end - at);
        at = end + 1;
        return true;
    };
    while (at < d.size()) {
        unsigned char t = (unsigned char)d[at++];
        if (t == 0x08)
            return true;
        std::string key;
        if (!str(key))
            return false;
        Vdf& v = map.items.emplace_back(key, Vdf()).second;
        v.type = t;
        if (t == 0x00) {
            if (!VdfRead(d, at, v))
                return false;
        } else if (t == 0x01) {
            if (!str(v.text))
                return false;
        } else if (t == 0x02 || t == 0x07) {
            size_t n = t == 0x02 ? 4 : 8;
            if (at + n > d.size())
                return false;
            memcpy(&v.number, d.data() + at, n);
            at += n;
        } else {
            return false; /* a type this does not know: the file is left alone */
        }
    }
    return false;
}

static void VdfWrite(std::string& out, const Vdf& map) {
    for (const auto& kv : map.items) {
        out += (char)kv.second.type;
        out += kv.first;
        out += '\0';
        if (kv.second.type == 0x00) {
            VdfWrite(out, kv.second);
        } else if (kv.second.type == 0x01) {
            out += kv.second.text;
            out += '\0';
        } else {
            out.append((const char*)&kv.second.number, kv.second.type == 0x02 ? 4 : 8);
        }
    }
    out += (char)0x08;
}

static uint32_t Crc32(const std::string& s) {
    uint32_t c = 0xFFFFFFFFu;
    for (unsigned char b : s) {
        c ^= b;
        for (int k = 0; k < 8; k++)
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

/* The Steam account to add it to: the one that signed in last in config/loginusers.vdf ("MostRecent", else the
   newest "timestamp"), else every account folder in userdata. */
static std::vector<fs::path> SteamUserConfigs(const fs::path& steam) {
    std::vector<fs::path> out;
    std::error_code ec;
    std::ifstream f(steam / "config" / "loginusers.vdf");
    std::string line, id, best;
    unsigned long long newest = 0;
    bool mostRecent = false;
    auto value = [](const std::string& l) { /* the second quoted string of a line */
        size_t a = l.find('"', l.find('"', l.find('"') + 1) + 1);
        size_t b = a == std::string::npos ? a : l.find('"', a + 1);
        return b == std::string::npos ? std::string() : l.substr(a + 1, b - a - 1);
    };
    while (std::getline(f, line)) {
        size_t q = line.find('"');
        size_t e = q == std::string::npos ? q : line.find('"', q + 1);
        if (e != std::string::npos && e - q - 1 == 17 && std::isdigit((unsigned char)line[q + 1]) &&
            line.find('"', e + 1) == std::string::npos)
            id = line.substr(q + 1, 17);
        if (id.empty() || mostRecent)
            continue;
        if (line.find("\"MostRecent\"") != std::string::npos && value(line) == "1") {
            best = id;
            mostRecent = true;
        } else if (line.find("\"timestamp\"") != std::string::npos) {
            unsigned long long t = std::strtoull(value(line).c_str(), nullptr, 10);
            if (t >= newest) {
                newest = t;
                best = id;
            }
        }
    }
    if (!best.empty()) {
        fs::path p = steam / "userdata" / std::to_string(std::stoull(best) & 0xFFFFFFFFull) / "config";
        if (fs::is_directory(p, ec))
            return { p };
    }
    for (const auto& e : fs::directory_iterator(steam / "userdata", ec))
        if (e.path().filename() != "0" && fs::is_directory(e.path() / "config", ec))
            out.push_back(e.path() / "config");
    return out;
}

static fs::path SteamFolder() {
    fs::path steam;
    if (const char* test = SDL_getenv("DECOMPUI_STEAM")) { /* harness: a copy of Steam's folders */
        steam = fs::u8path(test);
    } else {
#ifdef _WIN32
        wchar_t buf[MAX_PATH];
        DWORD size = sizeof(buf);
        if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, nullptr, buf, &size) ==
            ERROR_SUCCESS)
            steam = fs::path(buf);
#endif
    }
    return steam;
}

static bool SteamListRead(const fs::path& file, Vdf& root) {
    std::error_code ec;
    if (!fs::exists(file, ec))
        return false;
    std::ifstream in(file, std::ios::binary);
    std::string d((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    size_t at = 0;
    return VdfRead(d, at, root);
}

/* This folder's entry, whichever version it starts: the game is under this folder, or it starts here */
static bool SteamEntryOurs(Vdf& e) {
    Vdf* exe = e.Find("Exe");
    Vdf* dir = e.Find("StartDir");
    std::string root = sRoot.u8string();
    auto lower = [](std::string t) {
        for (char& c : t)
            c = (char)std::tolower((unsigned char)c);
        return t;
    };
    std::string r = lower(root);
    return e.type == 0x00 && ((exe && lower(exe->text).find(r) != std::string::npos) ||
                              (dir && lower(dir->text) == "\"" + r + "\""));
}

static int sSteamState = -1; /* -1 unknown, 0 not there, 1 there */
static Uint64 sSteamChecked;
static bool InSteam() {
    Uint64 now = SDL_GetTicks();
    if (sSteamState >= 0 && now - sSteamChecked < 3000) /* Steam may change it meanwhile: read again every 3 s */
        return sSteamState == 1;
    sSteamChecked = now;
    sSteamState = 0;
    fs::path steam = SteamFolder();
    if (steam.empty())
        return false;
    for (const fs::path& config : SteamUserConfigs(steam)) {
        Vdf root;
        Vdf* list = SteamListRead(config / "shortcuts.vdf", root) ? root.Find("shortcuts") : nullptr;
        if (list)
            for (auto& kv : list->items)
                if (SteamEntryOurs(kv.second))
                    sSteamState = 1;
    }
    return sSteamState == 1;
}

/* Takes this folder's entries out of Steam's list (a backup first, like adding) and their artwork out of grid. */
static void RemoveFromSteam() {
    fs::path steam = SteamFolder();
    std::vector<fs::path> configs = steam.empty() ? std::vector<fs::path>() : SteamUserConfigs(steam);
    int removed = 0;
    std::error_code ec;
    for (const fs::path& config : configs) {
        fs::path file = config / "shortcuts.vdf";
        Vdf root;
        if (!SteamListRead(file, root))
            continue; /* not there or not understood: never overwritten */
        Vdf* list = root.Find("shortcuts");
        if (!list)
            continue;
        std::vector<std::pair<std::string, Vdf>> keep;
        std::vector<uint32_t> gone;
        for (auto& kv : list->items) {
            if (SteamEntryOurs(kv.second)) {
                Vdf* id = kv.second.Find("appid");
                if (id)
                    gone.push_back((uint32_t)id->number);
            } else {
                keep.push_back(kv);
            }
        }
        if (gone.empty())
            continue;
        for (size_t i = 0; i < keep.size(); i++) /* Steam numbers the entries 0, 1, 2 ... */
            keep[i].first = std::to_string(i);
        list->items = keep;
        std::string out;
        VdfWrite(out, root);
        fs::copy_file(file, config / "shortcuts.vdf.rekhcom-backup", fs::copy_options::overwrite_existing, ec);
        std::ofstream(file, std::ios::binary | std::ios::trunc).write(out.data(), (std::streamsize)out.size());
        for (uint32_t id : gone)
            for (const char* suffix : { "p.jpg", "p.png", "_logo.png", "_hero.jpg", "_hero.png", ".jpg", ".png" })
                fs::remove(config / "grid" / (std::to_string(id) + suffix), ec);
        removed++;
    }
    sSteamState = -1;
    Notice(removed ? T("Quitado de Steam. Si Steam está abierto, reinícialo para que desaparezca de la biblioteca.",
                       "Removed from Steam. If Steam is open, restart it to see it gone from the library.")
                   : T("No estaba en la lista de juegos de Steam; no se cambió nada.",
                       "It was not in Steam's game list; nothing was changed."),
           !removed);
}

/* Adds (or updates) "ReKHCoM" in Steam as a non-Steam game that starts this version's game like the desktop
   shortcut, with the launcher's cover and logo as its Steam artwork. Steam reads the list when it starts. */
static void AddToSteam() {
    const VersionState& st = sState[sSelected];
    if (!st.installed || !fs::exists(st.exe))
        return;
    fs::path steam = SteamFolder();
    std::vector<fs::path> configs = steam.empty() ? std::vector<fs::path>() : SteamUserConfigs(steam);
    if (configs.empty()) {
        Notice(T("No se encontró Steam (o ninguna cuenta ha iniciado sesión en él).",
                 "Steam was not found (or no account has signed in to it)."), true);
        return;
    }
    std::string exe = "\"" + st.exe.u8string() + "\"", dir = "\"" + sRoot.u8string() + "\"";
    std::string args = "--data-dir \"" + sRoot.u8string() + "\" --rom \"" + RomPath(sSelected).u8string() + "\"";
    std::string name = "ReKHCoM";
    uint32_t appid = Crc32(exe + name) | 0x80000000u;
    int done = 0;
    std::error_code ec;
    for (const fs::path& config : configs) {
        fs::path file = config / "shortcuts.vdf";
        Vdf root;
        if (fs::exists(file, ec)) {
            std::ifstream in(file, std::ios::binary);
            std::string d((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            size_t at = 0;
            if (!VdfRead(d, at, root))
                continue; /* not understood: never overwritten */
        }
        Vdf& list = root.Find("shortcuts") ? *root.Find("shortcuts") : root.Set("shortcuts", 0x00);
        list.type = 0x00;
        Vdf* entry = nullptr;
        for (auto& kv : list.items) {
            Vdf* e = kv.second.Find("Exe");
            if (kv.second.type == 0x00 && e && _stricmp(e->text.c_str(), exe.c_str()) == 0)
                entry = &kv.second;
        }
        if (!entry) {
            entry = &list.items.emplace_back(std::to_string(list.items.size()), Vdf()).second;
            entry->type = 0x00;
        }
        entry->Set("appid", 0x02).number = appid;
        entry->Set("AppName", 0x01).text = name;
        entry->Set("Exe", 0x01).text = exe;
        entry->Set("StartDir", 0x01).text = dir;
        entry->Set("icon", 0x01).text = (sRoot / "ReKHCoM.exe").u8string();
        entry->Set("LaunchOptions", 0x01).text = args;
        entry->Set("AllowOverlay", 0x02).number = 1;
        entry->Set("AllowDesktopConfig", 0x02).number = 1;
        if (!entry->Find("tags"))
            entry->Set("tags", 0x00);
        std::string out;
        VdfWrite(out, root);
        if (fs::exists(file, ec))
            fs::copy_file(file, config / "shortcuts.vdf.rekhcom-backup", fs::copy_options::overwrite_existing, ec);
        std::ofstream(file, std::ios::binary | std::ios::trunc).write(out.data(), (std::streamsize)out.size());
        /* the artwork: the cover as the library's capsule, the logo over the hero */
        fs::create_directories(config / "grid", ec);
        std::string id = std::to_string(appid);
        fs::copy_file(sRoot / "assets" / "boxart.jpg", config / "grid" / (id + "p.jpg"), fs::copy_options::overwrite_existing, ec);
        fs::copy_file(sRoot / "assets" / "logo.png", config / "grid" / (id + "_logo.png"), fs::copy_options::overwrite_existing, ec);
        done++;
    }
    sSteamState = -1;
    Notice(done ? T("Añadido a Steam. Si Steam está abierto, reinícialo para verlo en la biblioteca.",
                    "Added to Steam. If Steam is open, restart it to see it in the library.")
                : T("No se pudo leer la lista de juegos de Steam; no se cambió nada.",
                    "Steam's game list could not be read; nothing was changed."),
           !done);
}

/* ---- "Install in another folder": the whole ReKHCoM folder (game, launcher, ROMs, saves, HD mod) copied into
   another one, e.g. X:/Games/ReKHCoM, with a progress bar. The folder it came from is never deleted (the player's
   saves stay there too until they remove it). ---- */
struct InstallJob {
    std::atomic<bool> running{ false }, finished{ false }, ok{ false };
    std::atomic<long long> done{ 0 }, total{ 0 };
    fs::path dst;
};
static InstallJob sInstall;

static void InstallCopy(fs::path from, fs::path to) {
    std::error_code ec;
    std::vector<fs::path> files;
    long long total = 0;
    for (auto it = fs::recursive_directory_iterator(from, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
        if (it->is_regular_file(ec)) {
            files.push_back(it->path());
            total += (long long)it->file_size(ec);
        }
    sInstall.total = total;
    bool ok = !ec;
    std::vector<char> buf(1 << 22);
    for (const fs::path& f : files) {
        fs::path out = to / fs::relative(f, from, ec);
        fs::create_directories(out.parent_path(), ec);
        std::ifstream in(f, std::ios::binary);
        std::ofstream o(out, std::ios::binary | std::ios::trunc);
        if (!in || !o) {
            ok = false;
            continue;
        }
        while (in.read(buf.data(), (std::streamsize)buf.size()) || in.gcount() > 0) {
            o.write(buf.data(), in.gcount());
            sInstall.done += in.gcount();
        }
        ok = ok && (bool)o;
    }
    sInstall.ok = ok;
    sInstall.finished = true;
    sInstall.running = false;
}

/* The folder to install into: chosen in the system dialog, opening at <drive>:\Games when there is one. */
static fs::path PickInstallFolder() {
    fs::path out;
#ifdef _WIN32
    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
        return out;
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dialog->SetTitle(sUiLanguage == 1 ? L"Carpeta donde instalar ReKHCoM" : L"Folder to install ReKHCoM in");
    std::error_code ec;
    for (wchar_t d = L'D'; d <= L'Z'; d++) {
        std::wstring games = std::wstring(1, d) + L":\\Games";
        IShellItem* item = nullptr;
        if (fs::is_directory(games, ec) &&
            SUCCEEDED(SHCreateItemFromParsingName(games.c_str(), nullptr, IID_PPV_ARGS(&item)))) {
            dialog->SetFolder(item);
            item->Release();
            break;
        }
    }
    if (SUCCEEDED(dialog->Show(nullptr))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                out = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dialog->Release();
#endif
    return out;
}

static void InstallElsewhereRow() {
    std::error_code ec;
    ImGui::BeginDisabled(sInstall.running);
    if (ImGui::Button(T("Instalar en otra carpeta...", "Install in another folder..."), ImVec2(-FLT_MIN, 0))) {
        fs::path pick = PickInstallFolder();
        if (!pick.empty()) {
            fs::path dst = _stricmp(pick.filename().u8string().c_str(), "ReKHCoM") == 0 ? pick : pick / "ReKHCoM";
            fs::path rel = fs::relative(dst, sRoot, ec);
            bool inside = !ec && !rel.empty() && rel.native()[0] != L'.';
            if (fs::equivalent(dst, sRoot, ec) || inside)
                Notice(T("Elige una carpeta fuera de esta instalación.", "Choose a folder outside this installation."), true);
            else if (fs::exists(dst, ec) && !fs::is_empty(dst, ec))
                Notice(T("Esa carpeta ya tiene archivos: elige una vacía (no se sobrescribe nada).",
                         "That folder is not empty: choose an empty one (nothing is overwritten)."), true);
            else {
                sInstall.dst = dst;
                sInstall.done = 0;
                sInstall.total = 0;
                sInstall.finished = false;
                sInstall.running = true;
                std::thread(InstallCopy, sRoot, dst).detach();
            }
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", T("Copia todo (juego, ROMs, partidas, mod HD) a otra carpeta, p. ej. D:/Games/ReKHCoM. "
                                  "Esta carpeta no se borra.",
                                  "Copies everything (game, ROMs, saves, HD mod) to another folder, e.g. "
                                  "D:/Games/ReKHCoM. This folder is not deleted."));
    if (sInstall.running) {
        long long total = sInstall.total;
        ImGui::ProgressBar(total > 0 ? (float)((double)sInstall.done / (double)total) : 0.0f, ImVec2(-FLT_MIN, 0));
    } else if (sInstall.finished) {
        ImGui::PushFont(sFontSmall);
        ImGui::TextColored(sInstall.ok ? kText : kAccent, "%s %s", sInstall.ok ? T("Instalado en", "Installed in")
                                                                            : T("Copia incompleta en", "Incomplete copy in"),
                           sInstall.dst.u8string().c_str());
        ImGui::PopFont();
        if (sInstall.ok && ImGui::Button(T("Abrir desde ahí (cierra este)", "Open from there (closes this one)"),
                                         ImVec2(-FLT_MIN, 0))) {
#ifdef _WIN32
            ShellExecuteW(nullptr, L"open", (sInstall.dst / "ReKHCoM.exe").wstring().c_str(), nullptr,
                          sInstall.dst.wstring().c_str(), SW_SHOWNORMAL);
#endif
            SDL_Event quit = {};
            quit.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quit);
        }
    }
}

/* ---- "Report a problem": a new GitHub issue opened in the browser, filled with the version, the selected game and
   the last game log's header and end. Nothing is sent by itself: the player sees it and submits it. Personal folders
   (the user profile) are hidden in the text. ---- */
static std::string UrlEncode(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

static void ReportProblem();

/* The header's button: drawn as the language picker beside it (HeaderChip) */
static void ReportProblemChip(ImVec2 size) {
    float icon = Px(26);
    if (HeaderChip("##report", T("Reportar un problema...", "Report a problem..."), nullptr, size, false, icon + Px(6)))
        ReportProblem();
    { /* a wrench before the label (the user, 24/10), in the game's square's colours: black, red in its head */
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        ImVec2 c(mn.x + Px(14) + icon * 0.5f, (mn.y + mx.y) * 0.5f - Px(1));
        ImVec2 d(0.7071f, -0.7071f), n(0.7071f, 0.7071f); /* along the handle (to the head, up right), across it */
        auto at = [&](float along, float across) {
            return ImVec2(c.x + d.x * along + n.x * across, c.y + d.y * along + n.y * across);
        };
        ImU32 ink = ImGui::GetColorU32(kText), paper = ImGui::GetColorU32(ImGui::IsItemHovered() ? kControl : kPanel);
        dl->AddLine(at(-Px(10), 0), at(Px(4), 0), ink, Px(4));       /* the handle */
        dl->AddCircleFilled(at(-Px(10), 0), Px(2.6f), ink);          /* its rounded end */
        dl->AddCircleFilled(at(Px(6), 0), Px(6.5f), ink);            /* the head */
        dl->AddQuadFilled(at(Px(6), -Px(2.6f)), at(Px(14), -Px(2.6f)), /* its open jaw, out along the handle */
                          at(Px(14), Px(2.6f)), at(Px(6), Px(2.6f)), paper);
        dl->AddRectFilled(ImVec2(at(Px(3), 0).x - Px(1.6f), at(Px(3), 0).y - Px(1.6f)),
                          ImVec2(at(Px(3), 0).x + Px(1.6f), at(Px(3), 0).y + Px(1.6f)), ImGui::GetColorU32(kAccent));
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", T("Abre un reporte en GitHub con la versión y el final del registro del juego; lo "
                                  "revisas y lo envías tú (necesitas una cuenta de GitHub).",
                                  "Opens a GitHub report with the version and the end of the game's log; you "
                                  "review and submit it (needs a GitHub account)."));
}

static void ReportProblem() {
    std::vector<std::string> lines;
    {
        std::ifstream f(sRoot / "kh_com.log");
        std::string line;
        while (std::getline(f, line))
            lines.push_back(line);
    }
    std::string log;
    size_t head = std::min<size_t>(lines.size(), 12), tail = lines.size() > 52 ? lines.size() - 40 : head;
    for (size_t i = 0; i < head; i++)
        log += lines[i] + "\n";
    if (tail > head)
        log += "...\n";
    for (size_t i = std::max(tail, head); i < lines.size(); i++)
        log += lines[i] + "\n";
    /* the user profile's folder (it carries the account name) as %USERPROFILE%, in both slash styles */
    if (const char* profile = getenv("USERPROFILE")) {
        std::string a = profile, b = a;
        std::replace(b.begin(), b.end(), '\\', '/');
        for (const std::string& p : { a, b }) {
            for (size_t at; !p.empty() && (at = log.find(p)) != std::string::npos;)
                log.replace(at, p.size(), "%USERPROFILE%");
        }
    }
    if (log.size() > 2000) /* GitHub takes addresses up to ~8 KB, the text grows when encoded */
        log = log.substr(log.size() - 2000);
#ifdef LAUNCHER_ANDROID
    static const char* const kPlatform = "Android";
#else
    static const char* const kPlatform = "Windows";
#endif
    std::string body = std::string("**What happened** (and what you were doing):\n\n\n**Platform / device**: ") + kPlatform +
                       "\n**Version**: " + REKHCOM_VERSION + " " + kVersions[sSelected].regionEn +
                       "\n\n**Log** (kh_com.log):\n```\n" + (log.empty() ? std::string("(no log yet)\n") : log) + "```\n";
    std::string url = "https://github.com/MatyMeatBoy/ReKHCoM/issues/new?title=" + UrlEncode("[Bug] ") +
                      "&body=" + UrlEncode(body);
    if (const char* test = SDL_getenv("DECOMPUI_REPORT_URL")) /* harness: the address written there, no browser */
        std::ofstream(fs::u8path(test)) << url;
    else
        SDL_OpenURL(url.c_str());
}

/* Runs the game with the shared data folder; the launcher waits hidden. */
static void Launch(SDL_Window* window) {
#ifdef LAUNCHER_ANDROID
    /* the game in its own process (GameActivity, :game); this screen stays behind it */
    (void)window;
    SaveIni();
    SaveLauncherState();
    if (!AndroidLaunch(kVersions[sSelected].id, RomPath(sSelected).u8string()))
        Notice(T("No se pudo iniciar el juego.", "Could not start the game."), true);
    return;
#else
    const VersionState& st = sState[sSelected];
    std::wstring cmd = Quote(st.exe.wstring()) + L" --data-dir " + Quote(sRoot.wstring()) + L" --rom " +
                       Quote(RomPath(sSelected).wstring());
    STARTUPINFOW si = {};
    PROCESS_INFORMATION pi = {};

    si.cb = sizeof(si);
    SaveIni();
    SaveLauncherState();
    if (!CreateProcessW(st.exe.c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        st.exe.parent_path().c_str(), &si, &pi)) {
        Notice(T("No se pudo iniciar el juego.", "Could not start the game."), true);
        return;
    }
    CloseHandle(pi.hThread);
    SDL_HideWindow(window);
    while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
        }
    }
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    if (code == REKHCOM_EXIT_QUIT_ALL) { /* Options > Quit the game: the launcher goes too */
        sQuitNow = true;
        return;
    }
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    LoadIni();
    if (code != 0)
        Notice(T("El juego se cerró con un error; revisa kh_com.log.", "The game closed with an error; see kh_com.log."),
               true);
#endif
}

/* ---- test harness ------------------------------------------------------------
 * DECOMPUI_SHOT=dir  render every page off screen (hidden window) and save
 *                    dir/decompui_<page>.bmp, then exit.
 * DECOMPUI_INSTALL=path  install that ROM before the first frame.
 * DECOMPUI_HD_EXPORT=n   make the HD mod's ReKHCoM-HD.zip for platform n (hd_import.inc kTargets), then exit.
 */
typedef void(APIENTRY* GenFn)(GLsizei, GLuint*);
typedef void(APIENTRY* BindFn)(GLenum, GLuint);
typedef void(APIENTRY* StorageFn)(GLenum, GLenum, GLsizei, GLsizei);
typedef void(APIENTRY* AttachRbFn)(GLenum, GLenum, GLenum, GLuint);

static bool ShotFrames(SDL_Window* window, const char* dir) {
    GenFn genFb = (GenFn)SDL_GL_GetProcAddress("glGenFramebuffers");
    BindFn bindFb = (BindFn)SDL_GL_GetProcAddress("glBindFramebuffer");
    GenFn genRb = (GenFn)SDL_GL_GetProcAddress("glGenRenderbuffers");
    BindFn bindRb = (BindFn)SDL_GL_GetProcAddress("glBindRenderbuffer");
    StorageFn storage = (StorageFn)SDL_GL_GetProcAddress("glRenderbufferStorage");
    AttachRbFn attach = (AttachRbFn)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
    int w, h;
    GLuint fb, rb;

    if (!genFb || !bindFb || !genRb || !bindRb || !storage || !attach)
        return false;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    genFb(1, &fb);
    bindFb(0x8D40 /* GL_FRAMEBUFFER */, fb);
    genRb(1, &rb);
    bindRb(0x8D41 /* GL_RENDERBUFFER */, rb);
    storage(0x8D41, GL_RGBA8, w, h);
    attach(0x8D40, 0x8CE0 /* GL_COLOR_ATTACHMENT0 */, 0x8D41, rb);
    std::vector<unsigned char> px((size_t)w * h * 4);
    for (int page = 0; page < PAGE_COUNT; page++) {
        if (SDL_getenv("DECOMPUI_ACHIEVEMENTS_ONLY") && page != PAGE_ACHIEVEMENTS) continue;
        sPage = page;
        if (page == PAGE_ACHIEVEMENTS && SDL_getenv("DECOMPUI_ACHIEVEMENT_FILTER"))
            sAchievementFilter = std::clamp(SDL_atoi(SDL_getenv("DECOMPUI_ACHIEVEMENT_FILTER")), 0, 2);
        Uint64 until = SDL_GetTicks();
        if (page == PAGE_ACHIEVEMENTS && SDL_getenv("DECOMPUI_ACHIEVEMENT_WAIT"))
            until += std::clamp(SDL_atoi(SDL_getenv("DECOMPUI_ACHIEVEMENT_WAIT")), 0, 30000);
        for (int i = 0; i < 3 || SDL_GetTicks() < until; i++) {
            if (page == PAGE_ACHIEVEMENTS && SDL_getenv("DECOMPUI_TOAST_PREVIEW")) {
                sAchievementPreview = true; sAchievementPreviewStart = SDL_GetTicks() - 1000;
            }
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            ImGui::PushFont(sFontMenu);
            Frame(window);
            ImGui::PopFont();
            ImGui::Render();
            glViewport(0, 0, w, h);
            glClearColor(kInk.x, kInk.y, kInk.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (SDL_GetTicks() < until) SDL_Delay(10);
        }
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        SDL_Surface* s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ABGR8888);
        for (int y = 0; y < h; y++)
            std::memcpy((unsigned char*)s->pixels + (size_t)y * s->pitch, px.data() + (size_t)(h - 1 - y) * w * 4,
                        (size_t)w * 4);
        char name[64];
        std::snprintf(name, sizeof(name), "decompui_%d.bmp", page);
        SDL_SaveBMP(s, (fs::u8path(dir) / name).string().c_str());
        SDL_DestroySurface(s);
    }
    return true;
}

static fs::path ExeDir() {
#ifdef LAUNCHER_ANDROID
    return AndroidFilesDir(); /* the app's files folder, the game's data folder too */
#else
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return fs::path(path).parent_path();
#endif
}

/* The player opened this launcher, so the programs it ships are trusted too: drop the "downloaded from the internet"
   mark (Zone.Identifier) that the ZIP passed to them, or Windows SmartScreen stops bin\khcom_*.exe. Only this
   install's own .exe and .dll files. */
static void UnblockOwnFiles() {
#ifdef _WIN32
    std::error_code ec;
    for (const fs::path& dir : { sRoot, sRoot / "bin" })
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            std::wstring ext = entry.path().extension().wstring();
            for (auto& ch : ext)
                ch = (wchar_t)towlower(ch);
            if (entry.is_regular_file(ec) && (ext == L".exe" || ext == L".dll"))
                DeleteFileW((entry.path().wstring() + L":Zone.Identifier").c_str());
        }
#endif
}

#ifdef LAUNCHER_ANDROID
/* A finger dragged up or down scrolls the page under it, as on a phone (ImGui scrolls with a wheel); once it moved
   that far it is no longer a tap: the press SDL made of it is let go away from the buttons, and the rest of its
   pretend mouse is dropped. */
static bool TouchScroll(const SDL_Event& ev) {
    static bool down, scrolling;
    static float startY, lastY;
    ImGuiIO& io = ImGui::GetIO();
    int w = 0, h = 0;
    switch (ev.type) {
        case SDL_EVENT_FINGER_DOWN:
            down = true;
            scrolling = false;
            startY = lastY = ev.tfinger.y;
            return false;
        case SDL_EVENT_FINGER_MOTION:
            if (!down)
                return false;
            SDL_GetWindowSize(SDL_GetWindowFromID(ev.tfinger.windowID), &w, &h);
            if (!scrolling && std::fabs(ev.tfinger.y - startY) * h > Px(14)) {
                float x = ev.tfinger.x * w, y = ev.tfinger.y * h;
                scrolling = true;
                io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
                io.AddMouseButtonEvent(0, false);
                io.AddMousePosEvent(x, y); /* the wheel goes to the page under the finger */
            }
            if (scrolling)
                io.AddMouseWheelEvent(0, (ev.tfinger.y - lastY) * h / (2.0f * ImGui::GetFontSize()));
            lastY = ev.tfinger.y;
            return false;
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED:
            down = false;
            return false;
        case SDL_EVENT_MOUSE_MOTION:
            return scrolling && ev.motion.which == SDL_TOUCH_MOUSEID;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (!scrolling || ev.button.which != SDL_TOUCH_MOUSEID)
                return false;
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP)
                scrolling = false;
            return true;
    }
    return false;
}
#endif

int main(int argc, char** argv) {
    sRoot = ExeDir();
#ifdef LAUNCHER_ANDROID
    AndroidExtractAssets(sRoot);
#endif
    UnblockOwnFiles();
    ApplyUiLanguage();
    /* ReKHCoM.exe --hd-import [<source folder>] [--scenes] [--sprites-only] [--quality 0|1|2]: the HD mod import without the window
       (power users, tests); without a folder the installed game is looked for */
    /* ReKHCoM.exe --hd-compress --quality 1|2: the installed movies and scenes re-encoded in place, without the window */
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--hd-compress") == 0) {
            int quality = 1;
            for (int j = 1; j < argc; j++)
                if (strcmp(argv[j], "--quality") == 0 && j + 1 < argc)
                    quality = atoi(argv[j + 1]);
            hd::sJob.running = true;
            hd::Compress(sRoot, quality);
            fprintf(stderr, "%s\n", hd::sJob.Status().c_str());
            return 0;
        }
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--hd-import") == 0) {
            fs::path source = i + 1 < argc && argv[i + 1][0] != '-' ? fs::u8path(argv[i + 1]) : hd::FindInstall();
            bool scenes = false, spritesOnly = false, resume = false;
            int quality = 0;
            for (int j = 1; j < argc; j++) {
                if (strcmp(argv[j], "--quality") == 0 && j + 1 < argc)
                    quality = atoi(argv[j + 1]);
                scenes |= strcmp(argv[j], "--scenes") == 0;
                spritesOnly |= strcmp(argv[j], "--sprites-only") == 0;
                resume |= strcmp(argv[j], "--resume") == 0; /* an import cut short (hd/import.journal) */
            }
            if (source.empty()) {
                fprintf(stderr, "KINGDOM HEARTS -HD 1.5+2.5 ReMIX- not found\n");
                return 1;
            }
            hd::sJob.running = true;
            hd::Run(source, sRoot, !spritesOnly, scenes && !spritesOnly, !spritesOnly, true, quality, resume);
            fprintf(stderr, "%s\n", hd::sJob.Status().c_str());
            return 0;
        }
    AdoptPreviousInstall();
    hd::MoveScenes(sRoot / "hd"); /* the layout of older versions, moved on every start (not only on the HD page) */
    ScanInstall();
    /* a dump left in roms/ or next to the launcher (the simplest install: drop it in the folder) is installed at start */
    StartRomFolderScan(false);
    LoadIni();
    LoadLauncherState();
    SaveLauncherState(); /* the first time, keeps the language chosen from the system */
    KeepSelection();

    if (SDL_getenv("DECOMPUI_SHORTCUT")) { /* harness: the shortcut alone, then exit */
        CreateDesktopShortcut();
        return 0;
    }
    if (SDL_getenv("DECOMPUI_REPORT_URL")) { /* harness: the report's address alone, then exit */
        ReportProblem();
        return 0;
    }
    if (SDL_getenv("DECOMPUI_STEAM")) { /* harness: "Add to Steam" (or with DECOMPUI_STEAM_REMOVE=1 "Remove") into
                                            that folder, then exit; stderr says whether it is there afterwards */
        if (SDL_getenv("DECOMPUI_STEAM_REMOVE"))
            RemoveFromSteam();
        else
            AddToSteam();
        sSteamState = -1;
        fprintf(stderr, "steam: %s\n", InSteam() ? "in" : "out");
        return 0;
    }
#ifndef LAUNCHER_ANDROID
    if (const char* target = SDL_getenv("DECOMPUI_HD_EXPORT")) { /* harness: Mod HD > another platform n, then exit */
        hdexport::Run(sRoot, SDL_atoi(target));
        fprintf(stderr, "hd export: %s\n", hdexport::sDone.c_str());
        return hdexport::sDone.rfind("Listo", 0) == 0 || hdexport::sDone.rfind("Done", 0) == 0 ? 0 : 1;
    }
#endif
    if (const char* to = SDL_getenv("DECOMPUI_INSTALL_TO")) { /* harness: "Install in another folder" there, then exit */
        InstallCopy(sRoot, fs::u8path(to));
        fprintf(stderr, "install: %s, %lld of %lld bytes\n", sInstall.ok ? "ok" : "failed", (long long)sInstall.done,
                (long long)sInstall.total);
        return sInstall.ok ? 0 : 1;
    }
#ifdef LAUNCHER_ANDROID
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight"); /* as the game: the page is laid out wide */
#endif
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        return 1;
#ifdef LAUNCHER_ANDROID
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    sRaUi = KhRaCreate(sRoot.u8string().c_str(), nullptr, nullptr, nullptr);
    sScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (sScale < 1.0f)
        sScale = 1.0f;
    const char* shotDir = SDL_getenv("DECOMPUI_SHOT");
    int windowW = (int)Px(1180), windowH = (int)Px(760);
    if (shotDir && SDL_getenv("DECOMPUI_WINDOW")) {
        int w = 0, h = 0;
        if (SDL_sscanf(SDL_getenv("DECOMPUI_WINDOW"), "%dx%d", &w, &h) == 2 && w >= 960 && h >= 640) {
            windowW = w; windowH = h;
        }
    }
    sRaAccountShot = shotDir && SDL_getenv("DECOMPUI_RA_ACCOUNT");
    SDL_Window* window = SDL_CreateWindow("ReKHCoM", windowW, windowH,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                              (shotDir != nullptr ? SDL_WINDOW_HIDDEN : 0));
    if (window == nullptr)
        return 1;
    SDL_SetWindowMinimumSize(window, (int)Px(960), (int)Px(640));
#ifdef LAUNCHER_ANDROID
    {
        /* the phone's screen in landscape: the page laid out as on a 1180x720 window, as large as it fits */
        int w = 0, h = 0;
        SDL_GetWindowSize(window, &w, &h);
        sScale = std::max(1.0f, std::min(std::max(w, h) / 1180.0f, std::min(w, h) / 720.0f));
    }
#endif
    SDL_GLContext gl = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    ApplyStyle();
    LoadFonts();
    ImGui_ImplSDL3_InitForOpenGL(window, gl);
#ifdef LAUNCHER_ANDROID
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 130");
#endif
    LoadBoxArt();
    if (shotDir && SDL_getenv("DECOMPUI_REWRITE_SETTINGS")) SaveIni();
    if (shotDir && SDL_getenv("DECOMPUI_PADFOCUS")) /* harness: the action the drawing lights */
        sPadFocus = SDL_getenv("DECOMPUI_PADFOCUS");
    if (shotDir && SDL_getenv("DECOMPUI_EDITOR")) { /* harness: the editor on, this file */
        sEditorOn = true;
        sEdFile = std::clamp(std::atoi(SDL_getenv("DECOMPUI_EDITOR")), 0, 3);
    }
    if (const char* rom = SDL_getenv("DECOMPUI_INSTALL")) {
        InstallRom(fs::u8path(rom));
    }

    if(shotDir)if(const char* save=SDL_getenv("DECOMPUI_IMPORT_SAVE"))
        ImportSavePath(fs::u8path(save),std::clamp(SDL_atoi(SDL_getenv("DECOMPUI_SAVE_REGION")?SDL_getenv("DECOMPUI_SAVE_REGION"):"0"),0,2));
    bool running = shotDir == nullptr || !ShotFrames(window, shotDir);
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (CaptureEvent(ev))
                continue;
#ifdef LAUNCHER_ANDROID
            if (TouchScroll(ev))
                continue;
#endif
            /* Nintendo pads: the button labelled A confirms and B goes back, as in the game */
            if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
                ImGui::GetIO().ConfigNavSwapGamepadButtons =
                    KhPadIsNintendo(SDL_GetGamepadFromID(ev.gbutton.which), Ini("pad_layout")) != 0;
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT || ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                if (hd::sJob.running && !shotDir)
                    sQuitAsked = true; /* the HD mod is loading: asked first (HdQuitPopup) */
                else
                    running = false;
            }
            if (ev.type == SDL_EVENT_DROP_FILE && ev.drop.data != nullptr)
                InstallRom(fs::u8path(ev.drop.data));
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(50);
            continue;
        }
#ifdef LAUNCHER_ANDROID
        {
            /* the activity's copies (a dump, a save, the HD folder): its line under the page, and the files looked at
               again when one finished */
            static int seen = -1;
            AndroidStatus st = AndroidStatusNow();
            hd::sJob.running = st.busy;
            hd::sJob.text = st.busy ? st.text : std::string();
            if (st.serial != seen) {
                if (seen >= 0) {
                    ScanInstall();
                    LoadIni();
                    KeepSelection();
                    if (!st.text.empty())
                        Notice(st.text, st.error);
                }
                seen = st.serial;
            }
        }
#endif
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(sFontMenu);
        Frame(window);
        ImGui::PopFont();
        if (sQuitNow)
            running = false;
        ImGui::Render();
        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(kInk.x, kInk.y, kInk.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        if (sLaunch) {
            sLaunch = false;
            Launch(window);
        }
    }

    StopControllerProbe();
    if (sControllerProbe.pad) SDL_CloseGamepad(sControllerProbe.pad);
    KhRaDestroy(sRaUi);
    sRaUi = nullptr;
    AchievementBadgesDestroy();
    SecureZeroMemory(sRaPassword, sizeof(sRaPassword));
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
