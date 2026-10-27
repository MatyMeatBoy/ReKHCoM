/* ---- what the player left in roms/ (or next to the launcher), installed by moving it (user, 24/10) --------------
   "Install ROM..." looks there first: dumps under any name, zips with a dump or with the HD mod's folders, an hd
   folder. A clean dump is moved to roms/<code>.gba; a zip is extracted (the dump to roms/, hd/... to hd/) and goes to
   the Recycle Bin only when everything in it was recognised, so a backup with saves is never lost; a duplicate of
   an installed dump goes to the Recycle Bin. Nothing found: the file picker, as before. The scan runs on its own
   thread (an HD zip is gigabytes); the main thread applies the outcome. */
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../ps5/third_party/miniz/miniz.h"
#include <atomic>
#include <mutex>

namespace romdir {

struct Result {
    int roms = 0, hdFiles = 0, recycled = 0, kept = 0;
    std::vector<int> versions;
};
static std::atomic<bool> sBusy{false}, sDone{false};
static bool sPickIfEmpty;
static Result sResult;
static std::mutex sLock;

static std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

/* to the Recycle Bin, never a plain delete (DECOMPUI_NO_RECYCLE: tests only, a plain delete) */
static bool Recycle(const fs::path& p) {
    std::error_code ec;
    if (SDL_getenv("DECOMPUI_NO_RECYCLE"))
        return fs::remove_all(p, ec) > 0;
#ifdef _WIN32
    std::wstring from = p.wstring();
    from.append(2, L'\0');
    SHFILEOPSTRUCTW op = {};
    op.wFunc = FO_DELETE;
    op.pFrom = from.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
    return SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
#else
    return fs::remove_all(p, ec) > 0; /* no recycle bin on a phone */
#endif
}

static int VersionOf(const std::vector<unsigned char>& rom) {
    if (rom.size() != kRomSize)
        return -1;
    for (int v = 0; v < kVersionCount; v++)
        if (std::memcmp(rom.data() + 0xAC, kVersions[v].code, 4) == 0 && Sha1Hex(rom) == kVersions[v].sha1)
            return v;
    return -1;
}

static bool Installed(int v) {
    std::vector<unsigned char> rom;
    return ReadFile(RomPath(v), rom) && Sha1Hex(rom) == kVersions[v].sha1;
}

static bool WriteRom(int v, const std::vector<unsigned char>& rom) {
    std::error_code ec;
    fs::create_directories(sRoot / "roms", ec);
    fs::path tmp = RomPath(v);
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out.write((const char*)rom.data(), (std::streamsize)rom.size());
        if (!out)
            return false;
    }
    fs::rename(tmp, RomPath(v), ec);
    return !ec;
}

static bool IsRomFile(const fs::path& p) {
    std::string ext = Lower(p.extension().string());
    return ext == ".gba" || ext == ".agb" || ext == ".bin";
}

/* hd/<part>/... or <part>/... inside a package: the path under hd/, or empty if it is not the HD mod's */
static fs::path HdRel(std::string name) {
    static const char* const parts[] = { "movies", "music", "scenes", "sprites", "subs" };
    std::replace(name.begin(), name.end(), '\\', '/');
    if (Lower(name).rfind("hd/", 0) == 0)
        name = name.substr(3);
    if (name.rfind("media/", 0) == 0)
        return PortHdMediaTransferMember(name.c_str()) ? fs::u8path(name) : fs::path();
    size_t slash = name.find('/');
    if (slash == std::string::npos)
        return {};
    std::string part = Lower(name.substr(0, slash));
    if (std::none_of(std::begin(parts), std::end(parts), [&](const char* p) { return part == p; }))
        return {};
    fs::path rel = fs::u8path(name);
    for (const fs::path& c : rel)
        if (c == "..")
            return {};
    return rel;
}

static void Zip(const fs::path& path, Result& r) {
    FILE* f = OpenFile(path, "rb");
    if (!f)
        return;
    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_cfile(&zip, f, 0, 0)) {
        fclose(f);
        return;
    }
    bool all = true, any = false, ok = true;
    bool hasMedia = false;
    fs::path pendingMedia[2];
    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip); i++) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st)) {
            all = false;
            continue;
        }
        if (st.m_is_directory)
            continue;
        fs::path rel = HdRel(st.m_filename);
        if (IsRomFile(fs::u8path(st.m_filename)) && st.m_uncomp_size == kRomSize) {
            size_t n = 0;
            void* data = mz_zip_reader_extract_to_heap(&zip, i, &n, 0);
            std::vector<unsigned char> rom;
            if (data) {
                rom.assign((unsigned char*)data, (unsigned char*)data + n);
                mz_free(data);
            }
            int v = VersionOf(rom);
            if (v < 0) {
                all = false;
                continue;
            }
            any = true;
            if (!Installed(v)) {
                if (WriteRom(v, rom)) {
                    r.roms++;
                    r.versions.push_back(v);
                } else {
                    ok = false;
                }
            }
        } else if (!rel.empty()) {
            any = true;
            std::error_code ec;
            fs::path dest = sRoot / "hd" / rel;
            int media = PortHdMediaTransferMember(rel.generic_u8string().c_str());
            hasMedia |= media != 0;
            if (media > 1) {
                dest += ".transfer.part";
                pendingMedia[media - 2] = dest;
            }
            if (media <= 1 && fs::exists(dest, ec))
                continue;
            fs::create_directories(dest.parent_path(), ec);
            fs::path tmp = dest;
            tmp += ".part";
            FILE* out = OpenFile(tmp, "wb");
            bool good = out && mz_zip_reader_extract_to_cfile(&zip, i, out, 0);
            if (out)
                fclose(out);
            if (good) {
                good = ReplaceWith(tmp, dest);
            }
            if (good) {
                r.hdFiles++;
            } else {
                fs::remove(tmp, ec);
                ok = false;
            }
        } else {
            all = false;
        }
    }
    mz_zip_reader_end(&zip);
    fclose(f);
    for (int mode = 1; mode <= 2; ++mode) {
        const auto& pending = pendingMedia[mode - 1];
        if (pending.empty()) continue;
        fs::path active = pending.parent_path() / "active.txt";
        if (!ok || !PortHdMediaPendingValid(sRoot.u8string().c_str(), mode, pending.u8string().c_str()) || !ReplaceWith(pending, active)) ok = false;
        std::error_code ec; fs::remove(pending, ec);
    }
    if (any && all && ok && !hasMedia) {
        if (Recycle(path))
            r.recycled++;
    } else if (any) {
        r.kept++;
    }
}

/* roms/hd: its files moved into hd/ (one already there with the same size: the copy left over goes to the bin) */
static void MoveTree(const fs::path& from, Result& r) {
    std::error_code ec;
    std::vector<fs::path> files;
    for (const auto& e : fs::recursive_directory_iterator(from, ec))
        if (e.is_regular_file(ec))
            files.push_back(e.path());
    std::stable_sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        return (a.filename() == "active.txt") < (b.filename() == "active.txt");
    });
    for (const fs::path& file : files) {
        fs::path rel = HdRel(("hd/" + fs::relative(file, from, ec).generic_u8string()));
        if (rel.empty())
            continue;
        fs::path dest = sRoot / "hd" / rel;
        int media = PortHdMediaTransferMember(rel.generic_u8string().c_str());
        if (media) {
            // Copies keep the source package/folder and old active generation.
            if (media == 1 && fs::exists(dest, ec)) continue;
            fs::create_directories(dest.parent_path(), ec);
            fs::path pending = dest; pending += ".transfer.part";
            fs::copy_file(file, pending, fs::copy_options::overwrite_existing, ec);
            if (!ec && (media == 1 || PortHdMediaPendingValid(sRoot.u8string().c_str(), media - 1, pending.u8string().c_str())) && ReplaceWith(pending, dest)) r.hdFiles++;
            fs::remove(pending, ec);
            continue;
        }
        if (fs::exists(dest, ec)) {
            if (fs::file_size(dest, ec) == fs::file_size(file, ec) && Recycle(file))
                r.recycled++;
            continue;
        }
        fs::create_directories(dest.parent_path(), ec);
        fs::rename(file, dest, ec);
        if (ec) { /* another drive: copy, then the original to the bin */
            ec.clear();
            fs::copy_file(file, dest, ec);
            if (ec)
                continue;
            Recycle(file);
        }
        r.hdFiles++;
    }
    /* the emptied folders */
    std::vector<fs::path> dirs;
    for (const auto& e : fs::recursive_directory_iterator(from, ec))
        if (e.is_directory(ec))
            dirs.push_back(e.path());
    std::sort(dirs.begin(), dirs.end(), [](const fs::path& a, const fs::path& b) { return a.native().size() > b.native().size(); });
    for (const fs::path& d : dirs)
        if (fs::is_empty(d, ec))
            fs::remove(d, ec);
    if (fs::is_empty(from, ec))
        fs::remove(from, ec);
}

static bool Busy() { return sBusy; }

static void Scan() {
    Result r;
    std::error_code ec;
    for (const fs::path& dir : { sRoot / "roms", sRoot }) {
        std::vector<fs::path> entries;
        for (const auto& e : fs::directory_iterator(dir, ec))
            entries.push_back(e.path());
        for (const fs::path& p : entries) {
            if (fs::is_directory(p, ec)) {
                if (dir == sRoot / "roms" && Lower(p.filename().string()) == "hd")
                    MoveTree(p, r);
                continue;
            }
            if (IsRomFile(p) && fs::file_size(p, ec) == kRomSize) {
                bool canonical = false;
                for (int v = 0; v < kVersionCount; v++)
                    canonical |= fs::exists(RomPath(v), ec) && fs::equivalent(p, RomPath(v), ec);
                if (canonical)
                    continue;
                std::vector<unsigned char> rom;
                if (!ReadFile(p, rom))
                    continue;
                int v = VersionOf(rom);
                if (v < 0) /* not a clean dump of this game: left alone */
                    continue;
                if (Installed(v)) {
                    if (Recycle(p))
                        r.recycled++;
                    continue;
                }
                fs::create_directories(sRoot / "roms", ec);
                fs::rename(p, RomPath(v), ec);
                if (ec) {
                    ec.clear();
                    if (!WriteRom(v, rom))
                        continue;
                    Recycle(p);
                }
                r.roms++;
                r.versions.push_back(v);
            } else if (Lower(p.extension().string()) == ".zip") {
                Zip(p, r);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(sLock);
        sResult = r;
    }
    sDone = true;
    sBusy = false;
}

}  // namespace romdir

/* pick: open the file picker when nothing was found (the button), not at start */
static void StartRomFolderScan(bool pick) {
    if (romdir::sBusy)
        return;
    romdir::sBusy = true;
    romdir::sDone = false;
    romdir::sPickIfEmpty = pick;
    if (SDL_getenv("DECOMPUI_SCAN_SYNC")) /* harness: before the shots */
        romdir::Scan();
    else
        std::thread(romdir::Scan).detach();
}

static bool sHdOfferOpen, sHdOfferAsked;

/* every frame: the scan's outcome on the main thread, then the HD mod's offer */
static void RomFolderPoll(SDL_Window* window) {
    if (romdir::sDone.exchange(false)) {
        romdir::Result r;
        {
            std::lock_guard<std::mutex> lock(romdir::sLock);
            r = romdir::sResult;
        }
        ScanInstall();
        KeepSelection(); /* what it installed never changes the version chosen (decompui.cpp) */
        if (r.roms || r.hdFiles || r.recycled) {
            std::string text = T("En la carpeta roms: ", "In the roms folder: ");
            for (size_t i = 0; i < r.versions.size(); i++)
                text += std::string(i ? ", " : "") + T("ROM ", "") + T(kVersions[r.versions[i]].regionEs, kVersions[r.versions[i]].regionEn) +
                        T(" instalada", " ROM installed");
            char extra[200];
            if (r.hdFiles) {
                snprintf(extra, sizeof(extra), T("%s%d archivos del mod HD", "%s%d HD mod files"), r.roms ? ", " : "", r.hdFiles);
                text += extra;
            }
            if (r.recycled) {
                snprintf(extra, sizeof(extra), T("; %d sobrantes a la papelera", "; %d leftovers to the Recycle Bin"), r.recycled);
                text += extra;
            }
            if (r.kept)
                text += T("; un paquete se dejó (tenía otros archivos)", "; a package was kept (it held other files)");
            Notice(text + ".", false);
        } else if (romdir::sPickIfEmpty) {
            PickRom(window);
        }
        sHdOfferAsked = false; /* new HD files or a first ROM: look again */
    }
    /* KINGDOM HEARTS HD 1.5+2.5 installed and nothing of the HD mod yet: offer it once, with the weight to choose */
    if (!sHdOfferAsked && !romdir::sBusy && !hd::sJob.running) {
        sHdOfferAsked = true;
        std::error_code ec;
        bool imported = fs::exists(sRoot / "hd" / "movies" / "ROP.mp4", ec) || fs::exists(sRoot / "hd" / "music" / "Title.ogg", ec);
        if (!imported && !sHdOfferDeclined && !SDL_getenv("DECOMPUI_SHOT") && !hd::ReadJournal(sRoot).found &&
            !hd::FindInstall().empty())
            sHdOfferOpen = true;
    }
}

static void SaveLauncherState();
static void KeepSelection();

static void HdOfferPopup() {
    if (sHdOfferOpen) {
        ImGui::OpenPopup("hdoffer");
        sHdOfferOpen = false;
    }
    ImGui::SetNextWindowSize(ImVec2(Px(560), 0));
    if (!ImGui::BeginPopupModal("hdoffer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize))
        return;
    Eyebrow(T("MOD HD · KINGDOM HEARTS 1.5+2.5", "HD MOD · KINGDOM HEARTS 1.5+2.5"));
    ImGui::TextWrapped("%s", T("Encontramos KINGDOM HEARTS HD 1.5+2.5 instalado en este PC. ¿Cargamos los videos, la música, "
                               "las escenas y el HUD de Re:Chain of Memories? Elige cuánto espacio usar:",
                               "KINGDOM HEARTS HD 1.5+2.5 is installed on this PC. Load Re:Chain of Memories' movies, music, "
                               "scenes and HUD? Choose how much space to use:"));
    ImGui::Spacing();
    struct Choice { const char* label; int quality; bool full; } choices[4] = {
        /* by size, largest first (the user, 24/10): the original, the suggested good, the saving one */
        { T("Completo · Calidad original, ~6,7 GB", "Full · Original quality, ~6.7 GB"), 0, true },
        { T("Completo · Buena calidad, ~3,7 GB (sugerido)", "Full · Good quality, ~3.7 GB (suggested)"), 1, true },
        { T("Completo · Ahorro, ~2 GB", "Full · Saving, ~2 GB"), 2, true },
        { T("Solo lo básico: intros, música y HUD, ~0,6 GB", "Just the basics: intros, music and HUD, ~0.6 GB"), 1, false } };
    int picked = -1;
    for (int i = 0; i < 4; i++)
        if (ImGui::Button(choices[i].label, ImVec2(-FLT_MIN, Px(40))))
            picked = i;
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("Tarda unos minutos y puedes seguir usando el launcher; queda todo en Mejoras.",
                               "It takes a few minutes and the launcher stays usable; everything stays in Enhancements."));
    ImGui::PopStyleColor();
    float half = (ImGui::GetContentRegionAvail().x - Px(8)) * 0.5f;
    if (ImGui::Button(T("Ahora no", "Not now"), ImVec2(half, 0)))
        ImGui::CloseCurrentPopup();
    ImGui::SameLine(0, Px(8));
    if (ImGui::Button(T("No volver a preguntar", "Don't ask again"), ImVec2(half, 0))) {
        sHdOfferDeclined = true;
        SaveLauncherState();
        ImGui::CloseCurrentPopup();
    }
    if (picked >= 0) {
        std::error_code ec;
        fs::create_directories(sRoot / "hd", ec);
        std::ofstream(sRoot / "hd" / "quality") << choices[picked].quality << "\n";
        hd::Start(hd::FindInstall(), sRoot, true, choices[picked].full, true, true, choices[picked].quality);
        sPage = PAGE_EXTRAS; /* the progress is there */
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
