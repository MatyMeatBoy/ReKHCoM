/* HD mod importer (port/progress/HD_MOD.md): takes the Re:Chain of Memories movies and soundtrack from the player's
 * own KINGDOM HEARTS -HD 1.5+2.5 ReMIX- (Steam or Epic, found by itself or chosen), or from a folder with those files
 * loose, and writes plain files the game plays: hd/movies/<name>.mp4 (opening, credits, endings), hd/scenes/
 * zz_event_<n>.mp4 (the story scenes), hd/subs/<movie>.<lang>.txt,
 * hd/music/<track>.ogg + .loop and hd/sprites/<sprite>.png (HD cards, port_hdsprites.c).
 *
 * Formats (port/tools/khhd_pkg.py, khhd_scd.py and khhd_import.py are the Python reference):
 * - movies: MP4 whose first 0x100 bytes are XORed with a fixed 16-byte key;
 * - music: Recom.hed/.pkg (OpenKh's documented format: per-asset seed, XOR on the first 0x100 bytes, zlib) holding
 *   STREAM/0001/<track>.win32.scd, an Ogg Vorbis stream whose Vorbis header is XORed with one byte. */
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#ifdef LAUNCHER_ANDROID
#include "hd_android.inc"
#else
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <d3d11.h>
#include <d3d10.h>

namespace hd {

static const unsigned char kMovieKey[16] = { 0x3f, 0x1e, 0xc1, 0x93, 0x26, 0x42, 0x58, 0xb8,
                                             0xaa, 0x9d, 0x53, 0x75, 0xbf, 0x62, 0x9a, 0x97 };
/* with the movies, the endings' scenes too (Sora's capsule room, Riku's ending and crossroads): they lead into RSE / RRE */
static const char* const kBaseMovies[] = { "ROP", "RSE", "RRE", "zz_event_151", "zz_event_621", "zz_event_631" };
static const char* const kTracks[] = {
    "01F_Town_B", "01F_Town_F", "03F_Hercul", "03F_Hercules_B", "12_Event_Yuttari", "13_Event_Namine",
    "14_Event_Odayaka", "15_Event_XIII", "16_XIIIBoss1", "17_Halloween_F", "18_Helloween_B", "19_Alice_Field",
    "20_Event_13thFloor", "21_Alice_Battle", "22_PeterPan_Field", "23_PeterPan_Battle", "24_Pinocchio_Field",
    "25_Aladdin_F", "26_Aladdin_B", "27_Pinocchio_B", "28_Forget_F", "29_Forget_B", "30_Twilight_F", "31_Twilight_B",
    "32_Destiny_F", "33_Destiny_B", "34_Boss_RikuAnsem", "35_Boss_NiseRiku", "36_WinnieThePooh", "37_LastBoss1",
    "38_UnderTheSea", "39_LittleMermaid_B", "40_LastBoss2", "41_LastBoss3", "42_Hollow_F", "43_Hollow_B",
    "44_PooGame1", "45_PoohGame2", "Boss1", "Boss2_World", "Event_Unrest", "Event1_Kinpak", "Event2_", "Event4",
    "Title",
};

/* ---- OpenKh's package format ---------------------------------------------------------------------------------- */

static const unsigned char kMaster[256] = {
    0x7E, 0x88, 0x97, 0x55, 0x0B, 0x06, 0xF1, 0x08, 0xEB, 0xBB, 0x14, 0x1C, 0xD8, 0x7A, 0xEC, 0x41, 0x34, 0xB2, 0xA3,
    0x46, 0xEF, 0x6B, 0xFE, 0xE1, 0xCF, 0x53, 0xA5, 0x05, 0x12, 0xD2, 0x8E, 0x52, 0x4A, 0x80, 0xE9, 0x81, 0xB0, 0xF0,
    0xB4, 0x9C, 0xFF, 0x0F, 0x15, 0x13, 0xDA, 0x73, 0x4E, 0x77, 0xBE, 0xD7, 0x30, 0xE5, 0xF6, 0x5A, 0x11, 0x37, 0x67,
    0xBC, 0x83, 0x6F, 0x27, 0x76, 0xD0, 0xCD, 0x69, 0x0D, 0x2E, 0x51, 0x42, 0x90, 0xB8, 0xB6, 0x4C, 0xAD, 0xCE, 0x5B,
    0x1A, 0x1F, 0xF5, 0xAF, 0x01, 0xF8, 0x5E, 0x3A, 0x6E, 0x68, 0x8B, 0xE8, 0x9F, 0xC9, 0xD9, 0x26, 0x92, 0x29, 0xC8,
    0x33, 0x98, 0x32, 0x54, 0xD4, 0x44, 0x25, 0x66, 0xAC, 0x5F, 0x99, 0x21, 0xE4, 0x8F, 0x1D, 0xC2, 0xD5, 0xA4, 0x62,
    0xF9, 0x02, 0x61, 0xDE, 0x59, 0xE7, 0x07, 0x9A, 0xFA, 0x2F, 0x95, 0x3F, 0x86, 0xD3, 0x78, 0xA7, 0x75, 0xED, 0xD6,
    0x2D, 0x64, 0x87, 0xBD, 0xC7, 0xC1, 0xAA, 0xF2, 0x8C, 0x17, 0xCB, 0x31, 0x8A, 0xC3, 0xCC, 0x04, 0xEE, 0x6A, 0xAB,
    0x5C, 0x22, 0x70, 0xCA, 0x9E, 0x71, 0x6D, 0x85, 0x45, 0x5D, 0xB9, 0xA9, 0xA6, 0x10, 0x47, 0xFB, 0x82, 0x7D, 0x84,
    0x7B, 0xC6, 0xE2, 0x38, 0xFC, 0x2B, 0x0E, 0x20, 0x9D, 0xC5, 0xF3, 0x39, 0xA8, 0xA0, 0x65, 0x58, 0x43, 0x7C, 0xE3,
    0x36, 0x18, 0x72, 0x49, 0x79, 0xAE, 0xD1, 0x74, 0x40, 0xC4, 0x91, 0x4F, 0x24, 0x63, 0xBF, 0xBA, 0x23, 0x96, 0x50,
    0xB3, 0x57, 0xDF, 0x1E, 0x03, 0x48, 0x7F, 0x35, 0x4D, 0x3E, 0xE6, 0xA1, 0xDD, 0x09, 0x3C, 0x3D, 0x3B, 0x56, 0x8D,
    0x93, 0x2A, 0x9B, 0x4B, 0x0C, 0x28, 0xB1, 0xE0, 0x60, 0x89, 0x19, 0xDB, 0x2C, 0xF7, 0x6C, 0xB5, 0x1B, 0x94, 0xC0,
    0xDC, 0xEA, 0xB7, 0x0A, 0xF4, 0x16, 0xFD, 0xA2, 0x00,
};
static const unsigned char kScramble[10] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36 };

static void MakeKey(const unsigned char seed[16], unsigned char key[0xB0]) {
    memset(key, 0, 0xB0);
    for (int i = 0; i < 16; i++)
        key[i] = seed[i] ? seed[i] : (unsigned char)i;
    for (int i = 0; i < 40; i++) {
        unsigned char frame[4] = { key[0x0C + i * 4], key[0x0D + i * 4], key[0x0E + i * 4], key[0x0F + i * 4] };
        if (i % 4 == 0) {
            unsigned char f[4] = { (unsigned char)(kMaster[frame[1]] ^ kScramble[i / 4]), kMaster[frame[2]],
                                   kMaster[frame[3]], kMaster[frame[0]] };
            memcpy(frame, f, 4);
        }
        for (int k = 0; k < 4; k++)
            key[0x10 + i * 4 + k] = key[i * 4 + k] ^ frame[k];
    }
}

static void DecryptHead(const unsigned char key[0xB0], std::vector<unsigned char>& data) {
    for (size_t off = 0; off < std::min<size_t>(data.size(), 0x100); off += 0x10) {
        size_t n = std::min<size_t>(data.size() - off, 0x10);
        for (int p = 10; p >= 0; p--)
            for (size_t j = 0; j < n; j++)
                data[off + j] ^= key[0x10 * p + j];
    }
}

static std::string Md5Hex(const std::string& text) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    unsigned char digest[16] = { 0 };
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, nullptr, 0) != 0)
        return {};
    if (BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0) {
        BCryptHashData(hash, (PUCHAR)text.data(), (ULONG)text.size(), 0);
        BCryptFinishHash(hash, digest, sizeof(digest), 0);
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(alg, 0);
    static const char* hex = "0123456789abcdef";
    std::string out;
    for (unsigned char b : digest) {
        out += hex[b >> 4];
        out += hex[b & 15];
    }
    return out;
}

/* One asset of a .pkg (its original data; the remastered extras are not needed here). */
static bool ReadAsset(std::ifstream& pkg, long long offset, std::vector<unsigned char>& out) {
    unsigned char seed[16];
    pkg.seekg(offset);
    if (!pkg.read((char*)seed, 16))
        return false;
    int32_t dlen, rcount, clen;
    memcpy(&dlen, seed, 4);
    memcpy(&rcount, seed + 4, 4);
    memcpy(&clen, seed + 8, 4);
    pkg.seekg(offset + 16 + (long long)rcount * 0x30);
    std::vector<unsigned char> packed((size_t)(clen >= 0 ? clen : dlen));
    if (!pkg.read((char*)packed.data(), (std::streamsize)packed.size()))
        return false;
    if (clen > -2) {
        unsigned char key[0xB0];
        MakeKey(seed, key);
        DecryptHead(key, packed);
    }
    if (clen <= -1) {
        out.swap(packed);
        return true;
    }
    out.resize((size_t)dlen);
    int got = stbi_zlib_decode_noheader_buffer((char*)out.data(), dlen, (const char*)packed.data() + 2, (int)packed.size() - 2);
    return got == dlen;
}

/* ---- SCD → Ogg Vorbis ----------------------------------------------------------------------------------------- */

static uint32_t U32(const std::vector<unsigned char>& d, size_t at) {
    return at + 4 <= d.size() ? (uint32_t)d[at] | (uint32_t)d[at + 1] << 8 | (uint32_t)d[at + 2] << 16 | (uint32_t)d[at + 3] << 24 : 0;
}

/* The Ogg file and its loop points (samples): the granule of the last page ending before each byte offset. */
static bool ScdToOgg(const std::vector<unsigned char>& scd, std::vector<unsigned char>& ogg, uint64_t& loopStart, uint64_t& loopEnd) {
    if (scd.size() < 0x100 || memcmp(scd.data(), "SEDBSSCF", 8) != 0)
        return false;
    size_t entry = U32(scd, U32(scd, 0x3C));
    uint32_t size = U32(scd, entry), codec = U32(scd, entry + 12), ls = U32(scd, entry + 16), le = U32(scd, entry + 20);
    uint32_t aux = U32(scd, entry + 28);
    if (codec != 6)
        return false;
    size_t pos = entry + 0x20;
    for (uint32_t i = 0; i < aux; i++)
        pos += U32(scd, pos + 4);
    if (pos + 0x20 > scd.size())
        return false;
    unsigned version = scd[pos], headerSize = scd[pos + 1], xorByte = scd[pos + 2];
    uint32_t seekSize = U32(scd, pos + 0x10), vorbisSize = U32(scd, pos + 0x14);
    size_t head = pos + headerSize + seekSize;
    if (head + vorbisSize + size > scd.size())
        return false;
    ogg.assign(scd.begin() + (long)head, scd.begin() + (long)(head + vorbisSize + size));
    if (version == 2 && xorByte)
        for (size_t i = 0; i < vorbisSize; i++)
            ogg[i] ^= (unsigned char)xorByte;
    if (memcmp(ogg.data(), "OggS", 4) != 0)
        return false;
    auto sampleAt = [&](uint64_t offset) -> uint64_t {
        uint64_t best = 0, target = vorbisSize + offset;
        size_t p = 0;
        while (p + 27 <= ogg.size() && memcmp(&ogg[p], "OggS", 4) == 0) {
            int64_t granule;
            memcpy(&granule, &ogg[p + 6], 8);
            unsigned segs = ogg[p + 26];
            size_t body = 0;
            for (unsigned s = 0; s < segs && p + 27 + s < ogg.size(); s++)
                body += ogg[p + 27 + s];
            size_t end = p + 27 + segs + body;
            if (end > target)
                break;
            if (granule > 0)
                best = (uint64_t)granule;
            p = end;
        }
        return best;
    };
    uint64_t total = sampleAt(size);
    loopStart = sampleAt(ls);
    loopEnd = le > ls ? sampleAt(le) : total;
    return true;
}

/* ---- movie subtitles (port/tools/khhd_subs.py) ---------------------------------------------------------------- */

/* One asset of a .pkg and its remastered entries (the 1.5+2.5's additions: HD textures, language tables), by name. */
static bool ReadAssetExtras(std::ifstream& pkg, long long offset, std::vector<unsigned char>& base,
                            std::map<std::string, std::vector<unsigned char>>& extras) {
    unsigned char seed[16];
    pkg.clear(); /* a padded read at the package's end leaves eof set: it must not fail the assets after it */
    pkg.seekg(offset);
    if (!pkg.read((char*)seed, 16))
        return false;
    int32_t dlen, rcount, clen;
    memcpy(&dlen, seed, 4);
    memcpy(&rcount, seed + 4, 4);
    memcpy(&clen, seed + 8, 4);
    if (rcount < 0 || rcount > 64)
        return false;
    std::vector<std::pair<std::string, std::pair<int32_t, int32_t>>> extra;
    for (int i = 0; i < rcount; i++) {
        unsigned char e[0x30];
        if (!pkg.read((char*)e, 0x30))
            return false;
        int32_t rdlen, rclen;
        memcpy(&rdlen, e + 0x28, 4);
        memcpy(&rclen, e + 0x2C, 4);
        extra.push_back({ std::string((const char*)e, strnlen((const char*)e, 32)), { rdlen, rclen } });
    }
    unsigned char key[0xB0];
    MakeKey(seed, key);
    auto unpack = [&](size_t stored, int32_t real, int32_t packed, std::vector<unsigned char>& data) {
        std::vector<unsigned char> raw(stored);
        pkg.read((char*)raw.data(), (std::streamsize)raw.size());
        size_t got = (size_t)pkg.gcount();
        pkg.clear();
        if (got < (size_t)(packed >= 0 ? packed : real)) /* only the 16-byte padding may be missing */
            return false;
        if (packed > -2)
            DecryptHead(key, raw);
        if (packed <= -1) {
            raw.resize((size_t)real);
            data.swap(raw);
            return true;
        }
        /* some language tables inflate past the size their header gives: keep all of it, as zlib does in Python */
        int inflatedLen = 0;
        char* inflated = stbi_zlib_decode_noheader_malloc((const char*)raw.data() + 2, (int)packed - 2, &inflatedLen);
        if (!inflated)
            return false;
        data.assign((unsigned char*)inflated, (unsigned char*)inflated + inflatedLen);
        free(inflated);
        return inflatedLen >= real;
    };
    if (!unpack((size_t)(clen >= 0 ? clen : dlen), dlen, clen, base))
        return false;
    for (auto& [name, len] : extra) {
        long long pos = (long long)pkg.tellg();
        pkg.seekg((pos + 15) & ~15LL);
        size_t n = (size_t)(len.second >= 0 ? len.second : len.first);
        std::vector<unsigned char> data;
        if (!unpack(n + (-n & 15), len.first, len.second, data))
            return false;
        extras[name].swap(data);
    }
    return true;
}

/* FORM/5xxx/FO5xxx.BIN: the US table at 0x40, a remastered <XX>_MVS*.binl per language. Each language's table
   becomes hd/subs/<movie>.<lang>.txt ("<first frame> <last frame> <text>", 30 frames a second, \n for breaks). */
static bool ReadAssetLanguages(std::ifstream& pkg, long long offset, std::map<std::string, std::vector<unsigned char>>& out) {
    std::vector<unsigned char> base;
    std::map<std::string, std::vector<unsigned char>> extras;
    if (!ReadAssetExtras(pkg, offset, base, extras) || base.size() < 0x40)
        return false;
    out[""] = std::vector<unsigned char>(base.begin() + 0x40, base.end());
    for (auto& [name, data] : extras)
        if (name.size() > 6 && name.compare(2, 4, "_MVS") == 0)
            out[name.substr(0, 2)].swap(data);
    return true;
}

/* the game's own letters: 0x99 then a code (capitals from 0x80, small letters from 0x9A), or Shift-JIS */
static std::string MvsText(const unsigned char* s, size_t n) {
    static const char* const kCaps[] = { "À", "Á", "Â", "Ä", "Ã", "Ç", "È", "É", "Ê", "Ë", "Ì", "Í", "Î", "Ï", "Ñ", "Ò",
                                         "Ó", "Ô", "Õ", "Ö", "Œ", "Ù", "Ú", "Û", "Ü", "ß" };
    static const char* const kSmall[] = { "à", "á", "â", "ä", "ã", "ç", "è", "é", "ê", "ë", "ì", "í", "î", "ï", "ñ", "ò",
                                          "ó", "ô", "õ", "ö", "ù", "ú", "û", "ü", "œ", "¿", "¡", "«", "„", "»", "º", "ª" };
    std::string out;
    for (size_t i = 0; i < n;) {
        unsigned char b = s[i];
        if (b < 0x80) {
            out += b == '\n' ? std::string("\\n") : std::string(1, (char)b);
            i++;
            continue;
        }
        unsigned c = i + 1 < n ? s[i + 1] : 0, code = (unsigned)b << 8 | c;
        i += 2;
        if (b == 0x99 && c >= 0x80 && c < 0x9A)
            out += kCaps[c - 0x80];
        else if (b == 0x99 && c >= 0x9A && c < 0xBA)
            out += kSmall[c - 0x9A];
        else if (b == 0x99 && c == 0xCF)
            out += "°";
        else if (code == 0x815C)
            out += "—";
        else if (code == 0x81F4)
            ;
        else if (code == 0x8167)
            out += "“";
        else if (code == 0x8168)
            out += "”";
        else if (code == 0x83C3)
            out += "é";
        else if (code == 0x8140)
            out += " ";
        else {
            char sj[2] = { (char)b, (char)c };
            WCHAR w[2];
            char u[8];
            int k = MultiByteToWideChar(932, 0, sj, 2, w, 2);
            int m = k > 0 ? WideCharToMultiByte(CP_UTF8, 0, w, k, u, sizeof(u), NULL, NULL) : 0;
            out.append(m > 0 ? std::string(u, (size_t)m) : std::string("?"));
        }
    }
    return out;
}

static int WriteSubtitles(const fs::path& install, const fs::path& data) {
    static const std::map<std::string, const char*> kLangs = { { "", "us" }, { "UK", "uk" }, { "FR", "fr" },
                                                               { "GR", "de" }, { "IT", "it" }, { "SP", "es" } };
    fs::path hed = install / "Image" / "dt" / "Recom.hed", out = data / "hd" / "subs";
    std::error_code ec;
    if (!fs::exists(hed, ec))
        return 0;
    fs::create_directories(out, ec);
    std::map<std::string, int> want;
    for (int sid = 5000; sid < 5700; sid++) {
        std::string id = std::to_string(sid);
        want[Md5Hex("FORM/" + id + "/FO" + id + ".BIN")] = sid;
    }
    std::ifstream h(hed, std::ios::binary), pkg(fs::path(hed).replace_extension(".pkg"), std::ios::binary);
    unsigned char e[0x20];
    int written = 0;
    while (h.read((char*)e, 0x20)) {
        static const char* hex = "0123456789abcdef";
        std::string md5;
        for (int i = 0; i < 16; i++) {
            md5 += hex[e[i] >> 4];
            md5 += hex[e[i] & 15];
        }
        auto it = want.find(md5);
        if (it == want.end())
            continue;
        long long offset;
        memcpy(&offset, e + 16, 8);
        std::map<std::string, std::vector<unsigned char>> tables;
        if (!ReadAssetLanguages(pkg, offset, tables))
            continue;
        int sid = it->second;
        char movie[32];
        if (sid == 5000)
            snprintf(movie, sizeof(movie), "ROP");
        else if (sid == 5200)
            snprintf(movie, sizeof(movie), "RSE");
        else if (sid == 5699)
            snprintf(movie, sizeof(movie), "RRE");
        else
            snprintf(movie, sizeof(movie), "zz_event_%03d", sid - 5000);
        for (auto& [prefix, t] : tables) {
            auto lang = kLangs.find(prefix);
            if (lang == kLangs.end() || t.size() < 0x10 || memcmp(t.data(), "MVS", 4) != 0)
                continue;
            uint16_t count;
            memcpy(&count, t.data() + 6, 2);
            std::string text;
            bool japanese = false;
            for (unsigned i = 0; i < count && 0x10 + i * 8 + 8 <= t.size(); i++) {
                uint16_t first, last;
                uint32_t at;
                memcpy(&first, t.data() + 0x10 + i * 8, 2);
                memcpy(&last, t.data() + 0x12 + i * 8, 2);
                memcpy(&at, t.data() + 0x14 + i * 8, 4);
                if (at >= t.size())
                    break;
                size_t end = at;
                while (end < t.size() && t[end])
                    end++;
                std::string line = MvsText(t.data() + at, end - at);
                for (unsigned char ch : line)
                    japanese |= ch == 0xE3 || (ch >= 0xE4 && ch <= 0xE9); /* U+3000 and up */
                text += std::to_string(first) + " " + std::to_string(last) + " " + line + "\n";
            }
            if (text.empty())
                continue;
            std::ofstream(out / (std::string(movie) + "." + (japanese ? "ja" : lang->second) + ".txt"), std::ios::binary) << text;
            written++;
        }
    }
    return written;
}

/* ---- finding the game ----------------------------------------------------------------------------------------- */

static fs::path MovieFolder(const fs::path& install) {
    for (const char* store : { "STEAM", "EPIC" }) {
        fs::path p = install / store / "dt" / "KHCReSource" / "BIN" / "movie";
        std::error_code ec;
        if (fs::is_directory(p, ec))
            return p;
    }
    return {};
}

static bool IsInstall(const fs::path& p) {
    std::error_code ec;
    return fs::exists(p / "Image" / "dt" / "Recom.hed", ec) || !MovieFolder(p).empty();
}

/* Every quoted value after "key" in a JSON or VDF text, with \\ unescaped */
static std::vector<std::string> QuotedValues(const std::string& text, const char* key) {
    std::vector<std::string> out;
    std::string k = std::string("\"") + key + "\"";
    for (size_t at = text.find(k); at != std::string::npos; at = text.find(k, at + k.size())) {
        size_t a = text.find('"', at + k.size());
        if (a == std::string::npos)
            break;
        /* only ':' or blanks between the key and its value */
        if (text.find_first_not_of(" \t:\r\n", at + k.size()) != a)
            continue;
        std::string clean;
        size_t i = a + 1;
        for (; i < text.size() && text[i] != '"'; i++) {
            if (text[i] == '\\' && i + 1 < text.size())
                i++;
            clean += text[i];
        }
        out.push_back(clean);
    }
    return out;
}

static std::string ReadText(const fs::path& f) {
    std::ifstream in(f, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

static fs::path RegPath(HKEY root, const wchar_t* key, const wchar_t* value) {
    WCHAR buf[MAX_PATH];
    DWORD size = sizeof(buf);
    if (RegGetValueW(root, key, value, RRF_RT_REG_SZ, nullptr, buf, &size) == ERROR_SUCCESS)
        return fs::path(buf);
    return {};
}

static fs::path KnownFolder(const KNOWNFOLDERID& id) {
    PWSTR path = nullptr;
    fs::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &path)))
        out = path;
    CoTaskMemFree(path);
    return out;
}

/* The game's folder names: Steam's, Epic's, and the store's title as some people name it by hand */
static const char* const kInstallNames[] = { "KINGDOM HEARTS -HD 1.5+2.5 ReMIX-", "KH_1.5_2.5", "KINGDOM HEARTS HD 1.5+2.5 ReMIX",
                                             "KINGDOM HEARTS HD 1.5 + 2.5 ReMIX", "Kingdom Hearts HD 1.5 2.5 ReMIX" };

/* KINGDOM HEARTS HD 1.5+2.5 ReMIX on this PC. On PC it is sold on Steam and the Epic Games Store; it is looked for
   through every launcher that installs it (Steam, the Epic Games Launcher, Heroic and Legendary for Epic), Windows'
   list of installed programs, and, last, the usual game folders on every fixed drive. */
static fs::path FindInstall() {
    std::vector<fs::path> candidates;
    auto add = [&](const fs::path& p) {
        if (!p.empty())
            candidates.push_back(p);
    };
    /* Steam: its folder (user and machine registry, default places) and every library in libraryfolders.vdf */
    std::vector<fs::path> steams = { RegPath(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"),
                                     RegPath(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath"),
                                     RegPath(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath"),
                                     KnownFolder(FOLDERID_ProgramFilesX86) / "Steam", KnownFolder(FOLDERID_ProgramFiles) / "Steam" };
    for (const fs::path& steam : steams) {
        if (steam.empty())
            continue;
        add(steam);
        for (const char* vdf : { "steamapps/libraryfolders.vdf", "config/libraryfolders.vdf" })
            for (const std::string& path : QuotedValues(ReadText(steam / vdf), "path"))
                add(fs::u8path(path));
    }
    std::vector<fs::path> found;
    for (const fs::path& root : candidates)
        for (const char* name : kInstallNames)
            found.push_back(root / "steamapps" / "common" / name);
    /* Epic Games Launcher: its manifests (where the registry says, or the default) and LauncherInstalled.dat */
    fs::path programData = KnownFolder(FOLDERID_ProgramData);
    std::vector<fs::path> manifests = { programData / "Epic" / "EpicGamesLauncher" / "Data" / "Manifests" };
    fs::path epicData = RegPath(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Epic Games\\EpicGamesLauncher", L"AppDataPath");
    if (!epicData.empty())
        manifests.push_back(epicData / "Manifests");
    std::error_code ec;
    for (const fs::path& dir : manifests)
        for (const auto& item : fs::directory_iterator(dir, ec))
            if (item.path().extension() == ".item")
                for (const std::string& path : QuotedValues(ReadText(item.path()), "InstallLocation"))
                    found.push_back(fs::u8path(path));
    for (const std::string& path : QuotedValues(ReadText(programData / "Epic" / "UnrealEngineLauncher" / "LauncherInstalled.dat"), "InstallLocation"))
        found.push_back(fs::u8path(path));
    /* Heroic and Legendary (other launchers for Epic games) */
    fs::path roaming = KnownFolder(FOLDERID_RoamingAppData), home = KnownFolder(FOLDERID_Profile);
    for (const fs::path& json : { roaming / "heroic" / "legendaryConfig" / "legendary" / "installed.json",
                                  home / ".config" / "legendary" / "installed.json" })
        for (const std::string& path : QuotedValues(ReadText(json), "install_path"))
            found.push_back(fs::u8path(path));
    /* Windows' installed programs (Steam and Epic register their games there too) */
    for (HKEY root : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER })
        for (const wchar_t* key : { L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                                    L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall" }) {
            HKEY uninstall;
            if (RegOpenKeyExW(root, key, 0, KEY_READ, &uninstall) != ERROR_SUCCESS)
                continue;
            WCHAR sub[256];
            for (DWORD i = 0, n = 256; RegEnumKeyExW(uninstall, i, sub, &n, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS; i++, n = 256) {
                WCHAR name[512];
                DWORD size = sizeof(name);
                if (RegGetValueW(uninstall, sub, L"DisplayName", RRF_RT_REG_SZ, nullptr, name, &size) != ERROR_SUCCESS)
                    continue;
                std::wstring title(name);
                std::transform(title.begin(), title.end(), title.begin(), ::towupper);
                if (title.find(L"KINGDOM HEARTS") != std::wstring::npos && title.find(L"1.5") != std::wstring::npos) {
                    WCHAR where[MAX_PATH];
                    size = sizeof(where);
                    if (RegGetValueW(uninstall, sub, L"InstallLocation", RRF_RT_REG_SZ, nullptr, where, &size) == ERROR_SUCCESS)
                        found.push_back(fs::path(where));
                }
            }
            RegCloseKey(uninstall);
        }
    /* last: the usual folders on every fixed drive */
    DWORD drives = GetLogicalDrives();
    for (int d = 0; d < 26; d++) {
        if (!(drives & (1u << d)))
            continue;
        wchar_t letter[4] = { (wchar_t)(L'A' + d), L':', L'\\', 0 };
        if (GetDriveTypeW(letter) != DRIVE_FIXED)
            continue;
        fs::path drive(letter);
        for (const char* base : { "SteamLibrary/steamapps/common", "Steam/steamapps/common", "Program Files (x86)/Steam/steamapps/common",
                                  "Program Files/Steam/steamapps/common", "Games/SteamLibrary/steamapps/common",
                                  "Program Files/Epic Games", "Epic Games", "Games/Epic Games", "Games", "Juegos" })
            for (const char* name : kInstallNames)
                found.push_back(drive / fs::u8path(base) / name);
    }
    for (const fs::path& p : found)
        if (IsInstall(p))
            return p;
    return {};
}

/* ---- the import job ------------------------------------------------------------------------------------------- */

/* ---- HD sprites (port/tools/khhd_sprites.py is the reference; recipe: port/data/hd_sprites.tsv) ------------------ */

#include "hd_sprites_recipe.inc"
extern "C" void* tdefl_write_image_to_png_file_in_memory_ex(const void* image, int w, int h, int chans, size_t* len,
                                                             unsigned level, int flip);
extern "C" void mz_free(void* p);

struct Rgba {
    int w = 0, h = 0;
    std::vector<unsigned char> px;
    unsigned char* at(int x, int y) { return &px[((size_t)y * w + x) * 4]; }
    void Resize(int nw, int nh) { w = nw; h = nh; px.assign((size_t)nw * nh * 4, 0); }
};

static uint16_t U16(const std::vector<unsigned char>& d, size_t at) {
    return at + 2 <= d.size() ? (uint16_t)(d[at] | d[at + 1] << 8) : 0;
}

/* An uncompressed 32-bit DDS, a PNG, or the index-th IMGD of an IMGZ (32-bit in the remastered entries). */
static bool DecodeTexture(const std::vector<unsigned char>& b, int index, Rgba& img) {
    if (b.size() >= 128 && memcmp(b.data(), "DDS ", 4) == 0) {
        uint32_t h = U32(b, 12), w = U32(b, 16), masks[4] = { U32(b, 92), U32(b, 96), U32(b, 100), U32(b, 104) };
        if (U32(b, 84) != 0 || U32(b, 88) != 32 || w == 0 || h == 0 || w > 8192 || h > 8192 || b.size() < 128 + (size_t)w * h * 4)
            return false;
        img.Resize((int)w, (int)h);
        for (size_t i = 0; i < (size_t)w * h; i++) {
            uint32_t p = U32(b, 128 + i * 4);
            for (int c = 0; c < 4; c++) {
                uint32_t m = masks[c];
                int shift = 0;
                while (m && !((m >> shift) & 1))
                    shift++;
                img.px[i * 4 + c] = m ? (unsigned char)((p & m) >> shift) : 255;
            }
        }
        return true;
    }
    if (b.size() >= 8 && memcmp(b.data(), "\x89PNG", 4) == 0) {
        int w, h, n;
        unsigned char* p = stbi_load_from_memory(b.data(), (int)b.size(), &w, &h, &n, 4);
        if (!p)
            return false;
        img.Resize(w, h);
        memcpy(img.px.data(), p, img.px.size());
        stbi_image_free(p);
        return true;
    }
    size_t at = 0, size = b.size();
    if (b.size() >= 16 && memcmp(b.data(), "IMGZ", 4) == 0) {
        if ((int)U32(b, 12) <= index)
            return false;
        at = U32(b, 16 + index * 8);
        size = U32(b, 20 + index * 8);
    } else if (index != 0) {
        return false;
    }
    if (at + 0x20 > b.size() || memcmp(b.data() + at, "IMGD", 4) != 0)
        return false;
    uint32_t boff = U32(b, at + 8), blen = U32(b, at + 12);
    int w = U16(b, at + 0x1C), h = U16(b, at + 0x1E);
    if (w <= 0 || h <= 0 || blen != (uint32_t)w * h * 4 || at + boff + blen > b.size() || boff + blen > size)
        return false;
    img.Resize(w, h);
    memcpy(img.px.data(), b.data() + at + boff, blen);
    unsigned char top = 0;
    for (size_t i = 3; i < img.px.size(); i += 4)
        top = std::max(top, img.px[i]);
    if (top <= 128) /* PS2 alpha runs 0..128 */
        for (size_t i = 3; i < img.px.size(); i += 4)
            img.px[i] = (unsigned char)std::min(255, img.px[i] * 2);
    return true;
}

/* Separable resize, a triangle filter widened when shrinking, on premultiplied colour (khhd_sprites.resample). */
static Rgba Resample(const Rgba& src, int w, int h) {
    std::vector<double> f(src.px.size());
    for (size_t i = 0; i < src.px.size(); i += 4) {
        double a = src.px[i + 3];
        for (int c = 0; c < 3; c++)
            f[i + c] = src.px[i + c] * a / 255.0;
        f[i + 3] = a;
    }
    auto axis = [](const std::vector<double>& in, int iw, int ih, int n, bool horizontal) {
        int m = horizontal ? iw : ih, ow = horizontal ? n : iw, oh = horizontal ? ih : n;
        std::vector<double> out((size_t)ow * oh * 4, 0.0);
        double scale = (double)m / n, support = std::max(1.0, scale);
        for (int i = 0; i < n; i++) {
            double centre = (i + 0.5) * scale;
            int lo = std::max((int)std::floor(centre - support), 0), hi = std::min((int)std::ceil(centre + support), m);
            std::vector<std::pair<int, double>> taps;
            double sum = 0;
            for (int j = lo; j < hi; j++) {
                double wt = std::max(0.0, 1.0 - std::fabs((j + 0.5 - centre) / support));
                if (wt > 0) {
                    taps.push_back({ j, wt });
                    sum += wt;
                }
            }
            if (sum == 0) {
                taps = { { std::min(std::max((int)centre, 0), m - 1), 1.0 } };
                sum = 1;
            }
            int other = horizontal ? ih : iw;
            for (int k = 0; k < other; k++)
                for (auto& [j, wt] : taps) {
                    size_t s = horizontal ? ((size_t)k * iw + j) * 4 : ((size_t)j * iw + k) * 4;
                    size_t d = horizontal ? ((size_t)k * ow + i) * 4 : ((size_t)i * ow + k) * 4;
                    for (int c = 0; c < 4; c++)
                        out[d + c] += in[s + c] * wt / sum;
                }
        }
        return out;
    };
    std::vector<double> v = axis(axis(f, src.w, src.h, h, false), src.w, h, w, true);
    Rgba out;
    out.Resize(w, h);
    for (size_t i = 0; i < out.px.size(); i += 4) {
        double a = v[i + 3];
        for (int c = 0; c < 3; c++) {
            double x = a > 0 ? v[i + c] * 255.0 / std::max(a, 1e-9) : 0;
            out.px[i + c] = (unsigned char)std::min(255.0, std::max(0.0, std::floor(x + 0.5)));
        }
        out.px[i + 3] = (unsigned char)std::min(255.0, std::max(0.0, std::floor(a + 0.5)));
    }
    return out;
}

/* Keep only the largest 8-connected region of alpha > 32 (a glyph without its italic neighbours' corners). */
static void Solo(Rgba& img) {
    int w = img.w, h = img.h, best = 0, bestSize = 0, n = 0;
    std::vector<int> label((size_t)w * h, 0);
    auto on = [&](int x, int y) { return img.px[((size_t)y * w + x) * 4 + 3] > 32; };
    for (int y0 = 0; y0 < h; y0++)
        for (int x0 = 0; x0 < w; x0++) {
            if (!on(x0, y0) || label[(size_t)y0 * w + x0])
                continue;
            label[(size_t)y0 * w + x0] = ++n;
            std::vector<std::pair<int, int>> stack = { { x0, y0 } };
            int size = 0;
            while (!stack.empty()) {
                auto [x, y] = stack.back();
                stack.pop_back();
                size++;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int xx = x + dx, yy = y + dy;
                        if (xx >= 0 && yy >= 0 && xx < w && yy < h && on(xx, yy) && !label[(size_t)yy * w + xx]) {
                            label[(size_t)yy * w + xx] = n;
                            stack.push_back({ xx, yy });
                        }
                    }
            }
            if (size > bestSize) {
                best = n;
                bestSize = size;
            }
        }
    Rgba out = img;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            bool keep = label[(size_t)y * w + x] == best, adjacent = false;
            if (!keep && !on(x, y) && x > 0 && y > 0 && x < w - 1 && y < h - 1)
                for (int dy = -1; dy <= 1 && !adjacent; dy++)
                    for (int dx = -1; dx <= 1 && !adjacent; dx++)
                        adjacent = label[(size_t)(y + dy) * w + x + dx] == best;
            if (!keep && !adjacent)
                memset(out.at(x, y), 0, 4);
        }
    img = std::move(out);
}

/* Keep the 8-connected regions of alpha > 32 whose box is centred in the middle three fifths of the picture, and the
   faint pixels touching them (khhd_sprites.middle): one letter of an italic word, without its neighbours' corners. */
static void Middle(Rgba& img) {
    int w = img.w, h = img.h, n = 0;
    std::vector<int> label((size_t)w * h, 0);
    std::vector<char> keep((size_t)w * h, 0);
    auto on = [&](int x, int y) { return img.px[((size_t)y * w + x) * 4 + 3] > 32; };
    for (int y0 = 0; y0 < h; y0++)
        for (int x0 = 0; x0 < w; x0++) {
            if (!on(x0, y0) || label[(size_t)y0 * w + x0])
                continue;
            label[(size_t)y0 * w + x0] = ++n;
            std::vector<std::pair<int, int>> stack = { { x0, y0 } }, cells;
            int lo = x0, hi = x0;
            while (!stack.empty()) {
                auto [x, y] = stack.back();
                stack.pop_back();
                cells.push_back({ x, y });
                lo = std::min(lo, x);
                hi = std::max(hi, x);
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int xx = x + dx, yy = y + dy;
                        if (xx >= 0 && yy >= 0 && xx < w && yy < h && on(xx, yy) && !label[(size_t)yy * w + xx]) {
                            label[(size_t)yy * w + xx] = n;
                            stack.push_back({ xx, yy });
                        }
                    }
            }
            if (2 * w <= (lo + hi + 1) * 5 && (lo + hi + 1) * 5 <= 8 * w)
                for (auto& c : cells)
                    keep[(size_t)c.second * w + c.first] = 1;
        }
    Rgba out = img;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            bool kept = keep[(size_t)y * w + x], adjacent = false;
            if (!kept && !on(x, y) && x > 0 && y > 0 && x < w - 1 && y < h - 1)
                for (int dy = -1; dy <= 1 && !adjacent; dy++)
                    for (int dx = -1; dx <= 1 && !adjacent; dx++)
                        adjacent = keep[(size_t)(y + dy) * w + x + dx];
            if (!kept && !adjacent)
                memset(out.at(x, y), 0, 4);
        }
    img = std::move(out);
}

/* src pasted over dst at (x0, y0), alpha-blended (khhd_sprites.over). */
static void BlitOver(Rgba& dst, const Rgba& src, int x0, int y0) {
    for (int y = std::max(y0, 0); y < std::min(y0 + src.h, dst.h); y++)
        for (int x = std::max(x0, 0); x < std::min(x0 + src.w, dst.w); x++) {
            const unsigned char* s = &src.px[((size_t)(y - y0) * src.w + (x - x0)) * 4];
            unsigned char* d = dst.at(x, y);
            double sa = s[3] / 255.0, da = d[3] / 255.0, oa = sa + da * (1 - sa);
            for (int c = 0; c < 3; c++) {
                double v = oa > 0 ? (s[c] * sa + d[c] * da * (1 - sa)) / std::max(oa, 1e-9) : 0;
                d[c] = (unsigned char)std::min(255.0, std::max(0.0, std::floor(v + 0.5)));
            }
            d[3] = (unsigned char)std::min(255.0, std::max(0.0, std::floor(oa * 255.0 + 0.5)));
        }
}

/* The polygon pts (x, y pairs) filled with rgba over img: a pixel is in when its centre is, even-odd rule
   (khhd_sprites.poly_fill). */
static void PolyFill(Rgba& img, unsigned rgba, const std::vector<int>& pts) {
    size_t n = pts.size() / 2;
    int lo = 1 << 30, hi = -(1 << 30);
    Rgba layer;
    layer.Resize(img.w, img.h);
    for (size_t i = 0; i < n; i++) {
        lo = std::min(lo, pts[2 * i + 1]);
        hi = std::max(hi, pts[2 * i + 1]);
    }
    for (int y = std::max(0, lo); y < std::min(img.h, hi + 1); y++) {
        double cy = y + 0.5;
        std::vector<double> cross;
        for (size_t i = 0; i < n; i++) {
            double x0 = pts[2 * i], y0 = pts[2 * i + 1], x1 = pts[2 * ((i + 1) % n)], y1 = pts[2 * ((i + 1) % n) + 1];
            if ((y0 <= cy) != (y1 <= cy))
                cross.push_back(x0 + (cy - y0) * (x1 - x0) / (y1 - y0));
        }
        std::sort(cross.begin(), cross.end());
        for (size_t k = 0; k + 1 < cross.size(); k += 2) {
            int a = std::max(0, (int)std::ceil(cross[k] - 0.5)), b = std::min(img.w - 1, (int)std::floor(cross[k + 1] - 0.5));
            for (int x = a; x <= b; x++) {
                unsigned char* d = layer.at(x, y);
                d[0] = (unsigned char)(rgba >> 24);
                d[1] = (unsigned char)(rgba >> 16);
                d[2] = (unsigned char)(rgba >> 8);
                d[3] = (unsigned char)rgba;
            }
        }
    }
    BlitOver(img, layer, 0, 0);
}

/* part with its colour replaced by ink (one colour, or a top-to-bottom gradient), alpha kept (khhd_sprites.inked). */
static void Ink(Rgba& part, const std::vector<unsigned>& ink) {
    if (ink.empty())
        return;
    for (int y = 0; y < part.h; y++) {
        double t = part.h > 1 ? (double)y / (part.h - 1) : 0.0;
        for (int k = 0; k < 3; k++) {
            int top = (ink.front() >> (16 - 8 * k)) & 255, bottom = (ink.back() >> (16 - 8 * k)) & 255;
            unsigned char c = (unsigned char)std::floor(top + (bottom - top) * t + 0.5);
            for (int x = 0; x < part.w; x++)
                part.at(x, y)[k] = c;
        }
    }
}

/* The recipe's operations on the texture, in order (khhd_sprites.apply); false on a malformed recipe. */
static bool ApplyRecipe(Rgba img, const std::string& ops, int boxW, int boxH, Rgba& out,
                        const std::function<bool(const std::string&, Rgba&)>& load) {
    int place[4] = { 0, 0, boxW, boxH };
    const Rgba own = img;
    Rgba other;
    const Rgba* src = &own;
    std::vector<unsigned> ink;
    size_t start = 0;
    while (start <= ops.size()) {
        size_t end = ops.find(';', start);
        std::string op = ops.substr(start, end == std::string::npos ? std::string::npos : end - start);
        start = end == std::string::npos ? ops.size() + 1 : end + 1;
        std::string key = op.substr(0, op.find('=')), val = op.find('=') == std::string::npos ? "" : op.substr(op.find('=') + 1);
        std::vector<int> v;
        if (key != "tint" && key != "ink" && key != "poly" && key != "src")
            for (size_t p = 0; p < val.size();) {
                size_t q = val.find(',', p);
                v.push_back(atoi(val.substr(p, q == std::string::npos ? std::string::npos : q - p).c_str()));
                p = q == std::string::npos ? val.size() : q + 1;
            }
        if (key == "crop" && v.size() == 4) {
            if (v[0] < 0 || v[1] < 0 || v[2] > img.w || v[3] > img.h || v[2] <= v[0] || v[3] <= v[1])
                return false;
            Rgba c;
            c.Resize(v[2] - v[0], v[3] - v[1]);
            for (int y = 0; y < c.h; y++)
                memcpy(c.at(0, y), img.at(v[0], v[1] + y), (size_t)c.w * 4);
            img = std::move(c);
        } else if (key == "rot" && v.size() == 1) {
            for (int k = 0; k < (v[0] / 90) % 4; k++) { /* a quarter turn counter-clockwise (numpy.rot90) */
                Rgba r;
                r.Resize(img.h, img.w);
                for (int y = 0; y < r.h; y++)
                    for (int x = 0; x < r.w; x++)
                        memcpy(r.at(x, y), img.at(img.w - 1 - y, x), 4);
                img = std::move(r);
            }
        } else if (key == "erase" && v.size() == 4) {
            for (int y = std::max(v[1], 0); y < std::min(v[3], img.h); y++)
                for (int x = std::max(v[0], 0); x < std::min(v[2], img.w); x++)
                    memset(img.at(x, y), 0, 4);
        } else if (key == "shift" && v.size() == 5) {
            Rgba copy = img;
            for (int y = std::max(v[1], 0); y < std::min(v[3], img.h); y++)
                for (int x = std::max(v[0], 0); x < std::min(v[2], img.w); x++)
                    memset(img.at(x, y), 0, 4);
            for (int y = std::max(v[1], 0); y < std::min(v[3], img.h); y++)
                for (int x = std::max(v[0], 0); x < std::min(v[2], img.w); x++)
                    if (x + v[4] >= 0 && x + v[4] < img.w)
                        memcpy(img.at(x + v[4], y), copy.at(x, y), 4);
        } else if (key == "solo") {
            Solo(img);
        } else if (key == "canvas" && v.size() == 2) {
            if (v[0] < 1 || v[1] < 1 || v[0] > 8192 || v[1] > 8192)
                return false;
            img.Resize(v[0], v[1]);
        } else if ((key == "blit" || key == "sblit") && v.size() == 8) {
            if (v[0] < 0 || v[1] < 0 || v[2] > src->w || v[3] > src->h || v[2] <= v[0] || v[3] <= v[1] ||
                v[6] <= v[4] || v[7] <= v[5])
                return false;
            Rgba part;
            part.Resize(v[2] - v[0], v[3] - v[1]);
            for (int y = 0; y < part.h; y++)
                memcpy(part.at(0, y), &src->px[((size_t)(v[1] + y) * src->w + v[0]) * 4], (size_t)part.w * 4);
            if (key == "sblit")
                Middle(part);
            Rgba sized = Resample(part, v[6] - v[4], v[7] - v[5]);
            Ink(sized, ink);
            BlitOver(img, sized, v[4], v[5]);
        } else if (key == "src") {
            if (val.empty())
                src = &own;
            else if (load(val, other))
                src = &other;
            else
                return false;
        } else if (key == "ink") {
            ink.clear();
            for (size_t p = 0; p < val.size();) {
                size_t q = val.find(',', p);
                ink.push_back((unsigned)strtoul(val.substr(p, q == std::string::npos ? std::string::npos : q - p).c_str(), nullptr, 16));
                p = q == std::string::npos ? val.size() : q + 1;
            }
        } else if (key == "poly") {
            std::vector<int> pts;
            size_t p = val.find(',');
            if (p == std::string::npos)
                return false;
            unsigned rgba = (unsigned)strtoul(val.substr(0, p).c_str(), nullptr, 16);
            for (p++; p < val.size();) {
                size_t q = val.find(',', p);
                pts.push_back(atoi(val.substr(p, q == std::string::npos ? std::string::npos : q - p).c_str()));
                p = q == std::string::npos ? val.size() : q + 1;
            }
            if (pts.size() < 6 || pts.size() % 2)
                return false;
            PolyFill(img, rgba, pts);
        } else if (key == "tint") {
            unsigned c = (unsigned)strtoul(val.c_str(), nullptr, 16);
            for (size_t i = 0; i < img.px.size(); i += 4)
                for (int k = 0; k < 3; k++)
                    img.px[i + k] = (unsigned char)(img.px[i + k] * ((c >> (16 - 8 * k)) & 255) / 255);
        } else if (key == "place" && v.size() == 4) {
            memcpy(place, v.data(), sizeof(place));
        } else if (key == "scale" && v.size() == 1 && start > ops.size()) {
            int k = v[0];
            if (k < 1 || k > 32 || place[2] <= place[0] || place[3] <= place[1])
                return false;
            out.Resize(boxW * k, boxH * k);
            Rgba r = Resample(img, (place[2] - place[0]) * k, (place[3] - place[1]) * k);
            for (int y = 0; y < r.h; y++)
                if (place[1] * k + y >= 0 && place[1] * k + y < out.h)
                    for (int x = 0; x < r.w; x++)
                        if (place[0] * k + x >= 0 && place[0] * k + x < out.w)
                            memcpy(out.at(place[0] * k + x, place[1] * k + y), r.at(x, y), 4);
            return true;
        } else {
            return false;
        }
    }
    return false; /* no final scale */
}

struct Job {
    std::atomic<bool> running{ false };
    std::atomic<bool> cancel{ false }; /* the launcher is closing: stop at the next block (see the import journal) */
    std::atomic<int> done{ 0 }, total{ 0 };
    /* how far each file being worked on went, in the same units as done/total (a movie counts by its size; several
       re-encode at once, one slot each) */
    std::atomic<int> partial[4] = {};
    /* the same per part (PART_*), for the tiles: what each holds in all, what is finished, which part each slot is on */
    std::atomic<int> partTotal[4] = {}, partDone[4] = {}, slotPart[4] = {};
    float Partial() {
        int n = 0;
        for (auto& p : partial)
            n += p;
        return (float)n;
    }
    float PartProgress(int part) {
        int total = partTotal[part], n = partDone[part];
        for (int i = 0; i < 4; i++)
            if (slotPart[i] == part)
                n += partial[i];
        return total > 0 ? std::min(1.0f, (float)n / (float)total) : 0.0f;
    }
    void Plan(int part, int units) { /* known up front, so the total never jumps back */
        partTotal[part] += units;
        total += units;
    }
    void Done(int part, int units) {
        partDone[part] += units;
        done += units;
    }
    void Finish(int part) { /* the part's rest, when fewer files came than planned */
        int rest = partTotal[part] - partDone[part];
        if (rest > 0)
            Done(part, rest);
    }
    void Reset() {
        cancel = false;
        done = 0;
        total = 0;
        for (int i = 0; i < 4; i++)
            partial[i] = partTotal[i] = partDone[i] = slotPart[i] = 0;
    }
    std::mutex lock;
    std::string status;
    void Say(const std::string& s) {
        std::lock_guard<std::mutex> g(lock);
        status = s;
    }
    std::string Status() {
        std::lock_guard<std::mutex> g(lock);
        return status;
    }
};
enum { PART_MOVIES, PART_SCENES, PART_MUSIC, PART_SPRITES };
static Job sJob;
static thread_local int tSlot = 0, tUnits = 1; /* the worker's progress slot and the weight of its file */

/* Every sprite of the recipe from the installation's Recom.hed/.pkg into data/hd/sprites/<name>.png. */
static int WriteSprites(const fs::path& hed, const fs::path& data) {
    std::error_code ec;
    fs::path outDir = data / "hd" / "sprites";
    fs::create_directories(outDir, ec);
    std::map<std::string, long long> offsets;
    {
        std::ifstream h(hed, std::ios::binary);
        unsigned char e[0x20];
        static const char* hex = "0123456789abcdef";
        while (h.read((char*)e, 0x20)) {
            std::string md5;
            for (int i = 0; i < 16; i++) {
                md5 += hex[e[i] >> 4];
                md5 += hex[e[i] & 15];
            }
            memcpy(&offsets[md5], e + 16, 8);
        }
    }
    /* rows by asset: each asset is read once */
    std::map<std::string, std::vector<std::vector<std::string>>> byAsset;
    for (const char* line : kHdSpriteRecipe) {
        std::vector<std::string> f;
        std::string s = line;
        for (size_t p = 0; p <= s.size();) {
            size_t q = s.find('\t', p);
            f.push_back(s.substr(p, q == std::string::npos ? std::string::npos : q - p));
            p = q == std::string::npos ? s.size() + 1 : q + 1;
        }
        if (f.size() >= 10)
            byAsset[f[8].substr(0, f[8].find(':'))].push_back(f);
    }
    std::ifstream pkg(fs::path(hed).replace_extension(".pkg"), std::ios::binary);
    int written = 0;
    /* another texture a recipe takes letters from (src=asset:texture#n) */
    std::map<std::string, std::map<std::string, std::vector<unsigned char>>> others;
    auto load = [&](const std::string& source, Rgba& img) {
        std::string asset = source.substr(0, source.find(':')), tex = source.substr(source.find(':') + 1);
        int index = 0;
        if (size_t hash = tex.find('#'); hash != std::string::npos) {
            index = atoi(tex.c_str() + hash + 1);
            tex = tex.substr(0, hash);
        }
        if (!others.count(asset)) {
            std::vector<unsigned char> base;
            auto at = offsets.find(Md5Hex(asset));
            if (at == offsets.end() || !ReadAssetExtras(pkg, at->second, base, others[asset]))
                others[asset].clear();
        }
        return others[asset].count(tex) && DecodeTexture(others[asset][tex], index, img);
    };
    for (auto& [asset, rows] : byAsset) {
        if (sJob.cancel)
            break;
        sJob.Say(std::string(T("Gráficos HD: ", "HD graphics: ")) + asset);
        auto it = offsets.find(Md5Hex(asset));
        std::vector<unsigned char> base;
        std::map<std::string, std::vector<unsigned char>> extras;
        bool ok = it != offsets.end() && ReadAssetExtras(pkg, it->second, base, extras);
        for (auto& f : rows) {
            sJob.Done(PART_SPRITES, 1);
            std::string tex = f[8].substr(f[8].find(':') + 1);
            int index = 0;
            if (size_t hash = tex.find('#'); hash != std::string::npos) {
                index = atoi(tex.c_str() + hash + 1);
                tex = tex.substr(0, hash);
            }
            if (f[9] == "raw") {
                /* a file of a .SPR container (the HUD's layouts, gauge.lad...), as it is (khhd_sprites.spr_entries),
                   or its remastered entry of that name (UK_gauge.lad, laid out on the HD texture) */
                if (ok && extras.count(tex)) {
                    std::ofstream(outDir / (f[0] + fs::path(tex).extension().string()), std::ios::binary)
                        .write((const char*)extras[tex].data(), (std::streamsize)extras[tex].size());
                    written++;
                    continue;
                }
                uint32_t first = ok && base.size() >= 4 ? base[0] | base[1] << 8 | base[2] << 16 | (uint32_t)base[3] << 24 : 0;
                for (size_t pos = 0; ok && pos + 0x20 <= first && pos + 0x20 <= base.size(); pos += 0x20) {
                    const unsigned char* e = base.data() + pos;
                    uint32_t off = e[0] | e[1] << 8 | e[2] << 16 | (uint32_t)e[3] << 24;
                    uint32_t size = (e[0x1c] | e[0x1d] << 8 | e[0x1e] << 16 | (uint32_t)e[0x1f] << 24) & 0x7FFFFFFF;
                    std::string name((const char*)e + 4, strnlen((const char*)e + 4, 24));
                    if (name.empty())
                        break;
                    if (name == tex && (size_t)off + size <= base.size()) {
                        std::ofstream(outDir / (f[0] + fs::path(tex).extension().string()), std::ios::binary)
                            .write((const char*)base.data() + off, (std::streamsize)size);
                        written++;
                        break;
                    }
                }
                continue;
            }
            Rgba img, out;
            if (!ok || !extras.count(tex) || !DecodeTexture(extras[tex], index, img) ||
                !ApplyRecipe(img, f[9], atoi(f[6].c_str()), atoi(f[7].c_str()), out, load))
                continue;
            size_t len = 0;
            void* png = tdefl_write_image_to_png_file_in_memory_ex(out.px.data(), out.w, out.h, 4, &len, 10, 0);
            if (png) {
                std::ofstream(outDir / (f[0] + ".png"), std::ios::binary).write((const char*)png, (std::streamsize)len);
                mz_free(png);
                written++;
            }
        }
    }
    return written;
}

static bool CopyMovie(const fs::path& src, const fs::path& dst) {
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dst, std::ios::binary | std::ios::trunc);
    if (!in || !out)
        return false;
    std::vector<char> buf(1 << 22);
    in.read(buf.data(), 0x100);
    std::streamsize n = in.gcount();
    if (n >= 8 && memcmp(buf.data() + 4, "ftyp", 4) != 0) /* the game's copy: un-XOR the head */
        for (std::streamsize i = 0; i < n; i++)
            buf[i] ^= (char)kMovieKey[i % 16];
    out.write(buf.data(), n);
    while (in.read(buf.data(), (std::streamsize)buf.size()) || in.gcount() > 0) {
        if (sJob.cancel)
            return false;
        out.write(buf.data(), in.gcount());
    }
    return (bool)out;
}

/* The movies' quality (hd/quality, the launcher's own): a compression, source (as it is), good or saving, at the
   source's size (the opening, credits and endings are 1080p ~10 Mb/s, the story scenes 720p ~5 Mb/s). A 720p choice
   was dropped on 24/10: the scenes are 720p already, so it saved 6% of the full mod (3.5 GB against 3.7 GB).
   Re-encoded here with Windows' own Media Foundation into H.264 + AAC, which every player of the port takes. The
   bitrates were measured with VMAF against the source (23/10, HD_MOD.md): good ~95 (hardly told apart), saving ~88. */
struct Quality {
    int maxHeight;   /* 0: the source's size */
    int compression; /* 0 source, 1 good, 2 saving */
};
/* bits a second at 1080p and at 720p, per compression (the source's own at 0) */
static const unsigned kBitrate1080[3] = { 10000000, 5000000, 2500000 };
static const unsigned kBitrate720[3] = { 5000000, 2500000, 1200000 };

/* hd/quality and --quality from before 24/10 counted 0..5 (3..5 the 720p ones): the same compression now */
static int Compression(int quality) {
    if (quality >= 3 && quality <= 5)
        quality -= 3;
    return quality < 0 || quality > 2 ? 1 : quality;
}

template <class T> static void Release(T*& p) {
    if (p)
        p->Release();
    p = nullptr;
}

/* src (a plain MP4) re-encoded into dst at quality q. False when anything failed (dst is then removed). */
/* gpu: the graphics card's device manager, so decoding and encoding both run there and the frames never come back
   to the processor; nullptr decodes on the processor (TranscodeBest picks) */
static bool Transcode(const fs::path& src, const fs::path& dst, const Quality& q, IMFDXGIDeviceManager* gpu = nullptr) {
    IMFAttributes* attr = nullptr;
    IMFSourceReader* reader = nullptr;
    IMFSinkWriter* writer = nullptr;
    IMFMediaType *native = nullptr, *videoIn = nullptr, *videoOut = nullptr, *audioIn = nullptr, *audioOut = nullptr;
    DWORD videoStream = 0, audioStream = 0;
    bool haveAudio = false, ok = false;
    UINT32 w = 0, h = 0, num = 30, den = 1, outW, outH;
    LONGLONG duration = 0;
    PROPVARIANT var;
    HRESULT hr = MFCreateAttributes(&attr, 3);
    if (SUCCEEDED(hr))
        attr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    if (SUCCEEDED(hr) && gpu) {
        attr->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, gpu);
        attr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    }
    if (SUCCEEDED(hr))
        hr = MFCreateSourceReaderFromURL(src.wstring().c_str(), attr, &reader);
    Release(attr);
    if (FAILED(hr))
        return false;
    PropVariantInit(&var);
    if (SUCCEEDED(reader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var)))
        duration = (LONGLONG)var.uhVal.QuadPart;
    PropVariantClear(&var);
    hr = reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &native);
    if (SUCCEEDED(hr)) {
        MFGetAttributeSize(native, MF_MT_FRAME_SIZE, &w, &h);
        MFGetAttributeRatio(native, MF_MT_FRAME_RATE, &num, &den);
        UINT32 bitrate = MFGetAttributeUINT32(native, MF_MT_AVG_BITRATE, 0);
        if (!bitrate)
            bitrate = h >= 1080 ? 10000000 : 5000000;
        Release(native);
        outW = w, outH = h;
        if (q.maxHeight && h > (UINT32)q.maxHeight) {
            outH = (UINT32)q.maxHeight;
            outW = (w * outH / h + 1) & ~1u;
        }
        if (q.compression == 0 && outH == h) { /* nothing to change: the plain copy stays */
            Release(reader);
            return false;
        }
        UINT32 rate = (outH >= 1080 ? kBitrate1080 : kBitrate720)[q.compression];
        if (rate > bitrate)
            rate = bitrate;
        if (const char* e = getenv("KHCOM_ENC_RATE")) /* the quality measurements' overrides */
            rate = (UINT32)atoi(e);
        hr = MFCreateMediaType(&videoIn);
        if (SUCCEEDED(hr)) {
            videoIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            videoIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
            videoIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            MFSetAttributeSize(videoIn, MF_MT_FRAME_SIZE, outW, outH);
            MFSetAttributeRatio(videoIn, MF_MT_FRAME_RATE, num, den);
            MFSetAttributeRatio(videoIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            hr = reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, videoIn);
        }
        if (SUCCEEDED(hr))
            hr = MFCreateMediaType(&videoOut);
        if (SUCCEEDED(hr)) {
            videoOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            videoOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
            videoOut->SetUINT32(MF_MT_AVG_BITRATE, rate);
            videoOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            videoOut->SetUINT32(MF_MT_MPEG2_PROFILE, getenv("KHCOM_ENC_PROFILE") ? atoi(getenv("KHCOM_ENC_PROFILE")) : 100);
            MFSetAttributeSize(videoOut, MF_MT_FRAME_SIZE, outW, outH);
            MFSetAttributeRatio(videoOut, MF_MT_FRAME_RATE, num, den);
            MFSetAttributeRatio(videoOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
        }
    }
    /* the sound as 16-bit PCM in, AAC 128 kb/s out (the source's rate and channels) */
    if (SUCCEEDED(hr) && SUCCEEDED(MFCreateMediaType(&audioIn))) {
        audioIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        audioIn->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        audioIn->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        if (SUCCEEDED(reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioIn))) {
            Release(audioIn);
            if (SUCCEEDED(reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &audioIn))) {
                UINT32 rate = MFGetAttributeUINT32(audioIn, MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
                UINT32 ch = MFGetAttributeUINT32(audioIn, MF_MT_AUDIO_NUM_CHANNELS, 2);
                if ((rate == 44100 || rate == 48000) && (ch == 1 || ch == 2) && SUCCEEDED(MFCreateMediaType(&audioOut))) {
                    audioOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
                    audioOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
                    audioOut->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
                    audioOut->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, rate);
                    audioOut->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, ch);
                    audioOut->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000);
                    haveAudio = true;
                }
            }
        }
    }
    DWORD videoIndex = 0; /* ReadSample names streams by their index in the file */
    for (DWORD i = 0;; i++) {
        IMFMediaType* t = nullptr;
        GUID major = GUID_NULL;
        if (FAILED(reader->GetNativeMediaType(i, 0, &t)))
            break;
        t->GetGUID(MF_MT_MAJOR_TYPE, &major);
        t->Release();
        if (major == MFMediaType_Video) {
            videoIndex = i;
            break;
        }
    }
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    if (haveAudio)
        reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    if (SUCCEEDED(hr))
        hr = MFCreateAttributes(&attr, 1);
    if (SUCCEEDED(hr)) {
        attr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, getenv("KHCOM_ENC_HW") ? atoi(getenv("KHCOM_ENC_HW")) : TRUE);
        attr->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4); /* MP4 also under a .part name */
        if (gpu)
            attr->SetUnknown(MF_SINK_WRITER_D3D_MANAGER, gpu);
        hr = MFCreateSinkWriterFromURL(dst.wstring().c_str(), nullptr, attr, &writer);
    }
    Release(attr);
    if (SUCCEEDED(hr))
        hr = writer->AddStream(videoOut, &videoStream);
    /* the encoder's settings: variable bitrate, best quality for the time, B frames */
    static const GUID kRateControl = { 0x1c0608e9, 0x370c, 0x4710, { 0x8a, 0x58, 0xcb, 0x61, 0x81, 0xc4, 0x24, 0x23 } };
    static const GUID kQualityVsSpeed = { 0x98332df8, 0x03cd, 0x476b, { 0x89, 0xfa, 0x3f, 0x9e, 0x44, 0x2d, 0xec, 0x9f } };
    static const GUID kBFrames = { 0x8d390aac, 0xdc5c, 0x4200, { 0xb5, 0x7f, 0x81, 0x4d, 0x04, 0xba, 0xba, 0xb2 } };
    static const GUID kCommonQuality = { 0xfcbf57a3, 0x7ea5, 0x4b0c, { 0x96, 0x44, 0x69, 0xb4, 0x0c, 0x39, 0xc3, 0x91 } };
    IMFAttributes* params = nullptr;
    if (SUCCEEDED(hr) && SUCCEEDED(MFCreateAttributes(&params, 4))) {
        params->SetUINT32(kRateControl, getenv("KHCOM_ENC_RC") ? atoi(getenv("KHCOM_ENC_RC")) : 2 /* unconstrained VBR */);
        params->SetUINT32(kQualityVsSpeed, getenv("KHCOM_ENC_QVS") ? atoi(getenv("KHCOM_ENC_QVS")) : 100);
        params->SetUINT32(kBFrames, getenv("KHCOM_ENC_B") ? atoi(getenv("KHCOM_ENC_B")) : 2);
        if (const char* e = getenv("KHCOM_ENC_Q")) /* measurements: the quality level of the quality-based rate control (RC 3) */
            params->SetUINT32(kCommonQuality, (UINT32)atoi(e));
    }
    if (SUCCEEDED(hr))
        hr = writer->SetInputMediaType(videoStream, videoIn, params);
    Release(params);
    if (SUCCEEDED(hr) && haveAudio && (FAILED(writer->AddStream(audioOut, &audioStream)) ||
                                       FAILED(writer->SetInputMediaType(audioStream, audioIn, nullptr))))
        hr = E_FAIL;
    if (SUCCEEDED(hr))
        hr = writer->BeginWriting();
    bool videoDone = false, audioDone = !haveAudio;
    while (SUCCEEDED(hr) && !(videoDone && audioDone)) {
        if (sJob.cancel) {
            hr = E_ABORT;
            break;
        }
        DWORD index = 0, flags = 0;
        LONGLONG time = 0;
        IMFSample* sample = nullptr;
        hr = reader->ReadSample((DWORD)MF_SOURCE_READER_ANY_STREAM, 0, &index, &flags, &time, &sample);
        if (FAILED(hr))
            break;
        bool isVideo = index == videoIndex;
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            (isVideo ? videoDone : audioDone) = true;
        if (sample) {
            hr = writer->WriteSample(isVideo ? videoStream : audioStream, sample);
            if (isVideo && duration > 0)
                sJob.partial[tSlot] = (int)(tUnits * ((double)time / (double)duration));
            sample->Release();
        } else if (flags & MF_SOURCE_READERF_STREAMTICK) {
            writer->SendStreamTick(isVideo ? videoStream : audioStream, time);
        }
    }
    ok = SUCCEEDED(hr) && SUCCEEDED(writer->Finalize());
    Release(writer);
    Release(reader);
    Release(videoIn);
    Release(videoOut);
    Release(audioIn);
    Release(audioOut);
    if (!ok) {
        std::error_code ec;
        fs::remove(dst, ec);
    }
    return ok;
}

static void WriteTrack(const fs::path& folder, const std::string& name, const std::vector<unsigned char>& ogg, uint64_t a, uint64_t b) {
    std::ofstream(folder / (name + ".ogg"), std::ios::binary).write((const char*)ogg.data(), (std::streamsize)ogg.size());
    std::ofstream(folder / (name + ".loop")) << a << " " << b << "\n";
}

/* source: an installation, or a folder with the movies and/or the soundtrack loose (.mp4, .scd or .ogg). */
/* The story scenes go to hd/scenes (23/10); an import made before keeps them in hd/movies: they move over once
   (a rename, nothing copied). The game still finds a scene left in hd/movies. */
static void MoveScenes(const fs::path& hdDir) {
    std::error_code ec;
    fs::path from = hdDir / "movies", to = hdDir / "scenes";
    if (!fs::is_directory(from, ec))
        return;
    for (const auto& f : fs::directory_iterator(from, ec)) {
        std::string name = f.path().filename().string();
        if (f.path().extension() != ".mp4" || name.rfind("zz_event_", 0) != 0)
            continue;
        fs::create_directories(to, ec);
        if (!fs::exists(to / name, ec))
            fs::rename(f.path(), to / name, ec);
    }
}

/* The graphics card's path, as HandBrake, OBS and Microsoft's Media Foundation samples do it: a Direct3D 11 device
   shared through a DXGI device manager by the decoder and the encoder, so the frames stay in the card's memory and the
   processor only moves the file. ~5 processor threads a movie less than decoding there. KHCOM_ENC_GPU=0 turns it off. */
static bool Mp4Ok(const fs::path& f);

struct Gpu {
    ID3D11Device* device = nullptr;
    IMFDXGIDeviceManager* manager = nullptr;
    UINT token = 0;
    ~Gpu() {
        if (manager)
            manager->Release();
        if (device)
            device->Release();
    }
    bool Open() {
        static const PFN_D3D11_CREATE_DEVICE create = [] {
            HMODULE d3d = LoadLibraryW(L"d3d11.dll");
            return d3d ? (PFN_D3D11_CREATE_DEVICE)(void*)GetProcAddress(d3d, "D3D11CreateDevice") : nullptr;
        }();
        static const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
        if (!create || FAILED(create(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, levels, 3,
                                     D3D11_SDK_VERSION, &device, nullptr, nullptr)))
            return false;
        ID3D10Multithread* threads = nullptr; /* the decoder and the encoder use it from their own threads */
        if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D10Multithread), (void**)&threads))) {
            threads->SetMultithreadProtected(TRUE);
            threads->Release();
        }
        return SUCCEEDED(MFCreateDXGIDeviceManager(&token, &manager)) && SUCCEEDED(manager->ResetDevice(device, token));
    }
};

static std::atomic<bool> sGpuFailed{ false }; /* once the card's path failed, the rest go by the processor */

static bool TranscodeBest(const fs::path& src, const fs::path& dst, const Quality& q) {
    const char* e = getenv("KHCOM_ENC_GPU");
    if (!(e && atoi(e) == 0) && !sGpuFailed) {
        Gpu gpu;
        if (gpu.Open() && Transcode(src, dst, q, gpu.manager) && Mp4Ok(dst))
            return true;
        if (sJob.cancel)
            return false;
        if (!sGpuFailed.exchange(true))
            fprintf(stderr, "hd import: the graphics card's path failed on %s, the processor does the rest\n", src.filename().u8string().c_str());
        std::error_code ec;
        fs::remove(dst, ec);
        sJob.partial[tSlot] = 0;
    }
    return Transcode(src, dst, q);
}

/* ---- an import cut short (the launcher closed, a crash, the power) ---------------------------------------------
   While an import runs, hd/import.journal holds its choices on the first line and then every file it created that was
   not there before, as each one is finished. Files are written under a .part name and renamed once complete, so a
   finished name is always whole. On the next start the player resumes it (the journal's files are checked and
   skipped) or discards it (only what that import created goes; what was there before stays). */
static fs::path sJournal; /* the running import's journal */
static std::mutex sJournalLock;

static fs::path JournalPath(const fs::path& data) { return data / "hd" / "import.journal"; }

struct JournalInfo {
    bool found = false, movies = false, scenes = false, music = false, sprites = false;
    int quality = 1;
    fs::path source;
    std::vector<std::string> files; /* relative to hd/ ("movies/ROP.mp4"; "music/" a whole folder it created) */
};

static JournalInfo ReadJournal(const fs::path& data) {
    JournalInfo j;
    std::ifstream in(JournalPath(data));
    std::string line;
    if (!std::getline(in, line))
        return j;
    int m = 0, sc = 0, mu = 0, sp = 0, q = 1;
    char src[1024] = {};
    if (sscanf(line.c_str(), "%d %d %d %d %d %1023[^\n]", &q, &m, &sc, &mu, &sp, src) < 5)
        return j;
    j.found = true;
    j.quality = q, j.movies = m, j.scenes = sc, j.music = mu, j.sprites = sp;
    j.source = fs::u8path(src);
    while (std::getline(in, line))
        if (!line.empty())
            j.files.push_back(line);
    return j;
}

/* a finished file this import created (relative to hd/) */
static void JournalAdd(const std::string& relative) {
    std::lock_guard<std::mutex> g(sJournalLock);
    if (!sJournal.empty())
        std::ofstream(sJournal, std::ios::app) << relative << "\n";
}

/* ---- what is already installed, movie by movie --------------------------------------------------------------
   hd/import.list: one line per finished movie ("movies/ROP.mp4 1 123456": where, its quality, its size). A new
   import keeps every movie already there whole at the quality asked and does only the missing, broken or other
   quality ones (the user, 24/10: loading again started from zero). Installs from before the list are judged by
   hd/quality, a whole MP4 and a size that fits (a re-encoding is smaller than the game's file, a copy the same). */
static std::map<std::string, std::pair<int, uintmax_t>> sInstalled; /* from hd/import.list */

static fs::path InstalledListPath(const fs::path& data) { return data / "hd" / "import.list"; }

static void ReadInstalledList(const fs::path& data) {
    std::lock_guard<std::mutex> g(sJournalLock);
    sInstalled.clear();
    std::ifstream in(InstalledListPath(data));
    std::string line;
    while (std::getline(in, line)) {
        char name[512] = {};
        int q = -1;
        unsigned long long size = 0;
        if (sscanf(line.c_str(), "%511s %d %llu", name, &q, &size) == 3)
            sInstalled[name] = { q, (uintmax_t)size };
    }
}

/* a movie finished at that quality (relative to hd/) */
static void InstalledAdd(const fs::path& data, const std::string& relative, int quality, uintmax_t size) {
    std::lock_guard<std::mutex> g(sJournalLock);
    sInstalled[relative] = { quality, size };
    std::ofstream(InstalledListPath(data), std::ios::app) << relative << " " << quality << " " << size << "\n";
}

/* the .part files an import or a compression left half-written */
static void RemoveParts(const fs::path& data) {
    std::error_code ec;
    for (const char* sub : { "movies", "scenes", "music", "sprites" })
        for (const auto& f : fs::directory_iterator(data / "hd" / sub, ec))
            if (f.path().extension() == ".part")
                fs::remove(f.path(), ec);
}

/* Discard an import cut short: what it created, its half-written files and the journal; nothing else */
static void DiscardImport(const fs::path& data) {
    std::error_code ec;
    JournalInfo j = ReadJournal(data);
    for (const std::string& f : j.files) {
        fs::path at = data / "hd" / fs::u8path(f);
        if (!f.empty() && f.back() == '/')
            fs::remove_all(at, ec);
        else
            fs::remove(at, ec);
    }
    RemoveParts(data);
    fs::remove(JournalPath(data), ec);
}

/* A whole MP4: its top-level boxes add up to the file and the index (moov) and the data (mdat) are there. Reads a few
   bytes a box: quick even for the opening. */
static bool Mp4Ok(const fs::path& f) {
    std::error_code ec;
    uintmax_t size = fs::file_size(f, ec), at = 0;
    std::ifstream in(f, std::ios::binary);
    bool moov = false, mdat = false;
    if (ec || !in)
        return false;
    while (at + 8 <= size) {
        unsigned char h[16];
        in.seekg((std::streamoff)at);
        if (!in.read((char*)h, 8))
            return false;
        uint64_t box = (uint64_t)h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];
        if (box == 1) {
            if (!in.read((char*)h + 8, 8))
                return false;
            box = 0;
            for (int i = 8; i < 16; i++)
                box = box << 8 | h[i];
        } else if (box == 0) {
            box = size - at;
        }
        if (box < 8)
            return false;
        moov |= memcmp(h + 4, "moov", 4) == 0;
        mdat |= memcmp(h + 4, "mdat", 4) == 0;
        at += box;
    }
    return at == size && moov && mdat;
}

/* One movie into dst: copied as it is (quality 0), or re-encoded from a plain copy beside it (that copy becomes the
   movie when the re-encoding fails). Written under .part names: dst appears only once whole. */
/* quality: the compression, 0 source, 1 good, 2 saving */
static bool ImportMovie(const fs::path& src, const fs::path& dst, int quality) {
    std::error_code ec;
    Quality q = { 0, quality };
    bool existed = fs::exists(dst, ec);
    fs::path part = dst, plain = dst;
    part += ".part";
    plain += ".copy.part";
    bool ok;
    if (quality <= 0 || quality > 2) {
        ok = CopyMovie(src, part);
        if (ok)
            fs::rename(part, dst, ec);
    } else {
        ok = CopyMovie(src, plain);
        if (ok) {
            sJob.partial[tSlot] = 0;
            if (TranscodeBest(plain, part, q)) {
                fs::rename(part, dst, ec);
            } else if (!sJob.cancel) {
                fs::remove(part, ec);
                fs::rename(plain, dst, ec);
            } else {
                ok = false;
            }
            sJob.partial[tSlot] = 0;
        }
    }
    ok = ok && !ec;
    fs::remove(part, ec);
    fs::remove(plain, ec);
    if (ok && !existed)
        JournalAdd(dst.parent_path().filename().u8string() + "/" + dst.filename().u8string());
    return ok;
}

/* A movie's weight in the progress, about the time it takes: its size, 8 MB a unit when re-encoded (~1 s), 64 MB when
   only copied; a music track or an HD graphic is about one unit */
static int sUnitShift = 23;
static int Units(const fs::path& f) {
    std::error_code ec;
    uintmax_t size = fs::file_size(f, ec);
    return ec ? 1 : (int)std::max<uintmax_t>(1, size >> sUnitShift);
}
static int MoviePart(const fs::path& f) {
    return f.filename().string().rfind("zz_event_", 0) == 0 ? PART_SCENES : PART_MOVIES;
}

/* How many movies to work on at once, from this computer: copying is the disk's (2); re-encoding decodes on the
   processor (~5 threads a movie) and encodes on the graphics card when it has an H.264 encoder (several sessions),
   else all on the processor. Never past the threads, nor ~700 MB of free memory a movie. KHCOM_IMPORT_JOBS forces it. */
static int Workers(int quality) {
    const int most = (int)(sizeof(sJob.partial) / sizeof(sJob.partial[0]));
    if (const char* e = getenv("KHCOM_IMPORT_JOBS"))
        return std::clamp(atoi(e), 1, most);
    if (quality <= 0)
        return 2;
    int threads = (int)std::max(1u, std::thread::hardware_concurrency());
    bool hardware = false;
    MFT_REGISTER_TYPE_INFO h264 = { MFMediaType_Video, MFVideoFormat_H264 };
    IMFActivate** found = nullptr;
    UINT32 count = 0;
    if (SUCCEEDED(MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER, nullptr,
                            &h264, &found, &count))) {
        hardware = count > 0;
        for (UINT32 i = 0; i < count; i++)
            found[i]->Release();
        CoTaskMemFree(found);
    }
    int n = hardware ? std::clamp(threads / 5, 1, 3) : std::clamp(threads / 8, 1, 2);
    MEMORYSTATUSEX mem = { sizeof(mem) };
    if (GlobalMemoryStatusEx(&mem))
        n = std::min(n, std::max(1, (int)(mem.ullAvailPhys / (700ull << 20))));
    return n;
}

/* fn(item) for every item, n at once (each thread with its own progress slot and COM) */
static void Parallel(const std::vector<fs::path>& list, int n, const std::function<void(const fs::path&)>& fn) {
    std::atomic<size_t> next{ 0 };
    auto work = [&](int slot) {
        HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        tSlot = slot;
        for (size_t i; !sJob.cancel && (i = next++) < list.size();) {
            tUnits = Units(list[i]);
            int units = tUnits, part = MoviePart(list[i]); /* the file may change size while it is worked on */
            sJob.slotPart[slot] = part;
            fn(list[i]);
            sJob.partial[slot] = 0;
            sJob.Done(part, units);
        }
        if (SUCCEEDED(com))
            CoUninitialize();
    };
    std::vector<std::thread> threads;
    for (int i = 1; i < n && i < (int)list.size(); i++)
        threads.emplace_back(work, i);
    work(0);
    for (auto& t : threads)
        t.join();
}

static void Run(fs::path source, fs::path data, bool movies, bool scenes, bool music, bool sprites, int quality,
                bool resume = false) {
    std::error_code ec;
    quality = Compression(quality);
    /* the journal: a new import starts one; resuming keeps the files it lists (checked) and goes on from there */
    JournalInfo previous = resume ? ReadJournal(data) : JournalInfo();
    fs::create_directories(data / "hd", ec);
    RemoveParts(data);
    {
        std::lock_guard<std::mutex> g(sJournalLock);
        sJournal = JournalPath(data);
        if (!previous.found)
            std::ofstream(sJournal, std::ios::trunc) << quality << " " << movies << " " << scenes << " " << music << " "
                                                     << sprites << " " << source.u8string() << "\n";
    }
    auto finished = [&](const fs::path& f) { /* resuming: made by the import cut short, and whole */
        std::string rel = f.parent_path().filename().u8string() + "/" + f.filename().u8string();
        return std::find(previous.files.begin(), previous.files.end(), rel) != previous.files.end() && Mp4Ok(f);
    };
    ReadInstalledList(data);
    int installedQuality = -1; /* an install from before import.list: the quality it was made at */
    if (!fs::exists(InstalledListPath(data), ec))
        std::ifstream(data / "hd" / "quality") >> installedQuality;
    auto kept = [&](const fs::path& src, const fs::path& dst) { /* already there, whole, at the quality asked */
        std::error_code ec2;
        if (!fs::exists(dst, ec2) || !Mp4Ok(dst))
            return false;
        uintmax_t have = fs::file_size(dst, ec2), game = fs::file_size(src, ec2);
        if (ec2)
            return false;
        std::string rel = dst.parent_path().filename().u8string() + "/" + dst.filename().u8string();
        std::pair<int, uintmax_t> known(-1, 0);
        {
            std::lock_guard<std::mutex> g(sJournalLock);
            auto it = sInstalled.find(rel);
            if (it != sInstalled.end())
                known = it->second;
        }
        bool fits = quality == 0 ? have == game : have < game;
        bool keep = known.first >= 0 ? known.first == quality && known.second == have
                                     : installedQuality == quality && fits;
        if (keep && known.first < 0)
            InstalledAdd(data, rel, quality, have); /* recorded now, so the next time knows it */
        return keep;
    };
    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool mf = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
    if (!mf)
        quality = 0;
    fs::path outMovies = data / "hd" / "movies", outScenes = data / "hd" / "scenes", outMusic = data / "hd" / "music";
    MoveScenes(data / "hd");
    int imported = 0;
    sUnitShift = quality ? 23 : 26;
    /* everything to do, weighed up front: the bar moves with the time left, and never back */
    fs::path hed = source / "Image" / "dt" / "Recom.hed";
    bool haveHed = fs::exists(hed, ec);
    std::vector<fs::path> list;
    if (movies || scenes) {
        fs::path folder = MovieFolder(source);
        if (folder.empty())
            folder = source;
        for (const char* m : kBaseMovies)
            if (movies && fs::exists(folder / (std::string(m) + ".mp4"), ec))
                list.push_back(folder / (std::string(m) + ".mp4"));
        if (scenes)
            for (const auto& f : fs::directory_iterator(folder, ec))
                if (f.path().extension() == ".mp4" && f.path().filename().string().rfind("zz_event_", 0) == 0)
                    list.push_back(f.path());
        for (const fs::path& f : list)
            sJob.Plan(MoviePart(f), Units(f));
    }
    if (music)
        sJob.Plan(PART_MUSIC, (int)(sizeof(kTracks) / sizeof(kTracks[0])));
    if (sprites && haveHed)
        sJob.Plan(PART_SPRITES, (int)(sizeof(kHdSpriteRecipe) / sizeof(kHdSpriteRecipe[0])));
    if (movies || scenes) {
        fs::create_directories(outMovies, ec);
        fs::create_directories(outScenes, ec);
        std::atomic<int> movieCount{ 0 };
        int workers = Workers(quality);
        Parallel(list, workers, [&](const fs::path& f) {
            sJob.Say(std::string(quality ? T("Recodificando ", "Re-encoding ") : T("Copiando ", "Copying ")) + f.filename().string() +
                     (workers > 1 ? " (" + std::to_string(workers) + T(" a la vez)", " at once)") : std::string()));
            fs::path dst = (f.filename().string().rfind("zz_event_", 0) == 0 ? outScenes : outMovies) / f.filename();
            if (kept(f, dst)) { /* nothing to do for this one */
                movieCount++;
                return;
            }
            if (finished(dst) || ImportMovie(f, dst, quality)) {
                movieCount++;
                std::error_code ec2;
                InstalledAdd(data, dst.parent_path().filename().u8string() + "/" + dst.filename().u8string(), quality,
                             fs::file_size(dst, ec2));
            }
        });
        imported += movieCount;
        sJob.Finish(PART_MOVIES);
        sJob.Finish(PART_SCENES);
        sJob.Say(T("Subtítulos", "Subtitles"));
        imported += WriteSubtitles(source, data);
    }
    if (music && !sJob.cancel) {
        if (!fs::exists(outMusic, ec) && !previous.found)
            JournalAdd("music/"); /* a folder this import makes: discarding takes it away */
        fs::create_directories(outMusic, ec);
        if (haveHed) {
            std::map<std::string, std::string> want;
            for (const char* t : kTracks)
                want[Md5Hex(std::string("STREAM/0001/") + t + ".win32.scd")] = t;
            std::ifstream h(hed, std::ios::binary), pkg(fs::path(hed).replace_extension(".pkg"), std::ios::binary);
            unsigned char e[0x20];
            while (!sJob.cancel && h.read((char*)e, 0x20)) {
                static const char* hex = "0123456789abcdef";
                std::string md5;
                for (int i = 0; i < 16; i++) {
                    md5 += hex[e[i] >> 4];
                    md5 += hex[e[i] & 15];
                }
                auto it = want.find(md5);
                if (it == want.end())
                    continue;
                long long offset;
                memcpy(&offset, e + 16, 8);
                sJob.Say(std::string(T("Música: ", "Music: ")) + it->second);
                std::vector<unsigned char> scd, ogg;
                uint64_t a = 0, b = 0;
                if (ReadAsset(pkg, offset, scd) && ScdToOgg(scd, ogg, a, b)) {
                    WriteTrack(outMusic, it->second, ogg, a, b);
                    imported++;
                }
                sJob.Done(PART_MUSIC, 1);
            }
        } else { /* a folder with the tracks: .scd converted, .ogg copied */
            for (const auto& f : fs::directory_iterator(source, ec)) {
                std::string stem = f.path().stem().string();
                if (size_t d = stem.find(".win32"); d != std::string::npos)
                    stem = stem.substr(0, d);
                if (std::find_if(std::begin(kTracks), std::end(kTracks), [&](const char* t) { return stem == t; }) == std::end(kTracks))
                    continue;
                std::ifstream in(f.path(), std::ios::binary);
                std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()), ogg;
                uint64_t a = 0, b = 0;
                if (f.path().extension() == ".scd" && ScdToOgg(bytes, ogg, a, b)) {
                    WriteTrack(outMusic, stem, ogg, a, b);
                    imported++;
                } else if (f.path().extension() == ".ogg") {
                    fs::copy_file(f.path(), outMusic / (stem + ".ogg"), fs::copy_options::overwrite_existing, ec);
                    imported++;
                }
                sJob.Done(PART_MUSIC, 1);
            }
        }
        sJob.Finish(PART_MUSIC);
    }
    if (sprites && haveHed && !sJob.cancel) {
        if (!fs::exists(data / "hd" / "sprites", ec) && !previous.found)
            JournalAdd("sprites/");
        imported += WriteSprites(hed, data);
        sJob.Finish(PART_SPRITES);
    }
    {
        std::lock_guard<std::mutex> g(sJournalLock);
        if (!sJob.cancel)
            fs::remove(sJournal, ec);
        sJournal.clear();
    }
    sJob.Say(sJob.cancel ? std::string(T("Carga detenida.", "Import stopped."))
             : imported ? std::string(T("Listo: ", "Done: ")) + std::to_string(imported) + T(" archivos importados.", " files imported.")
                        : std::string(T("No se encontró nada para importar en esa carpeta.", "Nothing to import was found in that folder.")));
    if (mf)
        MFShutdown();
    if (SUCCEEDED(com))
        CoUninitialize();
    sJob.running = false;
}

/* The movie's height and bits a second (its own figure, or its size over its length); false when unreadable. */
static bool MovieRate(const fs::path& f, UINT32& height, double& bitrate) {
    IMFSourceReader* reader = nullptr;
    IMFMediaType* native = nullptr;
    PROPVARIANT var;
    UINT32 w = 0;
    std::error_code ec;
    height = 0;
    bitrate = 0;
    if (FAILED(MFCreateSourceReaderFromURL(f.wstring().c_str(), nullptr, &reader)))
        return false;
    if (SUCCEEDED(reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &native))) {
        MFGetAttributeSize(native, MF_MT_FRAME_SIZE, &w, &height);
        bitrate = MFGetAttributeUINT32(native, MF_MT_AVG_BITRATE, 0);
        Release(native);
    }
    PropVariantInit(&var);
    if (!bitrate && SUCCEEDED(reader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var)) &&
        var.uhVal.QuadPart > 0)
        bitrate = (double)fs::file_size(f, ec) * 8.0 / ((double)var.uhVal.QuadPart / 1e7);
    PropVariantClear(&var);
    Release(reader);
    return height > 0;
}

/* A movie already at or under the quality asked: left alone (encoding it again would only lose detail). */
static bool AtOrUnder(const fs::path& f, int quality) {
    UINT32 h;
    double bitrate;
    if (!MovieRate(f, h, bitrate))
        return true; /* unreadable: never touched */
    double target = (h >= 1080 ? kBitrate1080 : kBitrate720)[quality];
    return bitrate <= target * 1.1;
}

/* A compression stopped half way (the launcher closed) leaves name.source.mp4: it goes back to its name. */
static void RestoreLeftovers(const fs::path& folder) {
    std::error_code ec;
    for (const auto& f : fs::directory_iterator(folder, ec)) {
        std::string name = f.path().filename().string();
        size_t at = name.rfind(".source.mp4");
        if (at == std::string::npos || at + 11 != name.size())
            continue;
        fs::path back = folder / (name.substr(0, at) + ".mp4");
        fs::remove(back, ec); /* a half-written re-encoding */
        fs::rename(f.path(), back, ec);
    }
}

/* The movies and scenes already installed, re-encoded in place to a smaller quality (1, 2): for a full mod imported at
   source quality, without the game's installation. Each file is kept as it was when its re-encoding fails. */
static void Compress(fs::path data, int quality) {
    std::error_code ec;
    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool mf = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
    std::vector<fs::path> list;
    uintmax_t before = 0, after = 0;
    int changed = 0;
    for (const char* sub : { "movies", "scenes" }) {
        fs::path folder = data / "hd" / sub;
        RestoreLeftovers(folder);
        for (const auto& f : fs::directory_iterator(folder, ec))
            if (f.path().extension() == ".mp4")
                list.push_back(f.path());
    }
    quality = Compression(quality);
    sUnitShift = 23;
    for (const fs::path& f : list)
        sJob.Plan(MoviePart(f), Units(f));
    Quality q = { 0, quality };
    std::atomic<uintmax_t> sizeBefore{ 0 }, sizeAfter{ 0 };
    std::atomic<int> compressed{ 0 };
    int workers = mf ? Workers(quality) : 1;
    Parallel(list, workers, [&](const fs::path& f) {
        std::error_code ec;
        uintmax_t size = fs::file_size(f, ec);
        sizeBefore += size;
        if (!mf || quality <= 0 || AtOrUnder(f, quality)) {
            sizeAfter += size;
            return;
        }
        sJob.Say(std::string(T("Comprimiendo ", "Compressing ")) + f.filename().string() +
                 (workers > 1 ? " (" + std::to_string(workers) + T(" a la vez)", " at once)") : std::string()));
        fs::path plain = fs::path(f).replace_extension(".source.mp4");
        fs::rename(f, plain, ec);
        if (!ec && TranscodeBest(plain, f, q) && fs::file_size(f, ec) < size) {
            fs::remove(plain, ec);
            compressed++;
            InstalledAdd(data, f.parent_path().filename().u8string() + "/" + f.filename().u8string(), quality,
                         fs::file_size(f, ec));
        } else { /* failed, or not smaller: the movie as it was */
            fs::remove(f, ec);
            fs::rename(plain, f, ec);
        }
        sizeAfter += fs::file_size(f, ec);
    });
    before = sizeBefore;
    after = sizeAfter;
    changed = compressed;
    if (changed)
        std::ofstream(data / "hd" / "quality") << quality << "\n";
    char msg[160];
    snprintf(msg, sizeof(msg), T("Listo: %d videos comprimidos, %.1f GB -> %.1f GB.", "Done: %d movies compressed, %.1f GB -> %.1f GB."),
             changed, before / 1073741824.0, after / 1073741824.0);
    sJob.Say(mf ? std::string(msg) : std::string(T("Media Foundation no está disponible: no se comprimió nada.",
                                                     "Media Foundation is not available: nothing was compressed.")));
    if (mf)
        MFShutdown();
    if (SUCCEEDED(com))
        CoUninitialize();
    sJob.running = false;
}

static void StartCompress(const fs::path& data, int quality) {
    if (sJob.running)
        return;
    sJob.running = true;
    sJob.Reset();
    sJob.Say(T("Comprimiendo...", "Compressing..."));
    std::thread(Compress, data, quality).detach();
}

/* The size of the installed movies and scenes, in bytes. */
static uintmax_t InstalledMovieBytes(const fs::path& data) {
    std::error_code ec;
    uintmax_t n = 0;
    for (const char* sub : { "movies", "scenes" })
        for (const auto& f : fs::directory_iterator(data / "hd" / sub, ec))
            if (f.path().extension() == ".mp4")
                n += fs::file_size(f.path(), ec);
    return n;
}

static void Start(const fs::path& source, const fs::path& data, bool movies, bool scenes, bool music, bool sprites,
                  int quality, bool resume = false) {
    if (sJob.running)
        return;
    sJob.running = true;
    sJob.Reset();
    sJob.Say(T("Importando...", "Importing..."));
    std::thread(Run, source, data, movies, scenes, music, sprites, quality, resume).detach();
}

static int Count(const fs::path& folder, const char* ext) {
    std::error_code ec;
    int n = 0;
    for (const auto& f : fs::directory_iterator(folder, ec))
        n += f.path().extension() == ext;
    return n;
}

/* A folder chosen in the system dialog. */
static fs::path PickFolder() {
    fs::path out;
    IFileOpenDialog* dialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
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
    }
    return out;
}

}  // namespace hd

/* ---- the HD mod for another platform: one file (ReKHCoM-HD.zip) the game there unpacks on start ------------------
   (port/portable/hd_pack.c). What that platform can use: music, cards/HUD sprites and subtitles everywhere; the movies
   and scenes only where they play (Android; Linux and Windows import their own). Stored, not deflated: the files are
   compressed already and the consoles unpack faster. */
#include "../ps5/third_party/miniz/miniz.h"
namespace hdexport {
struct Target { const char* name; const char* where; bool movies; };
static const Target kTargets[] = {
    { "Android", "Instalar ROM... > este archivo / Install ROM... > this file", true },
    { "Nintendo Switch", "sdmc:/switch/ReKHCoM/", false },
    { "PS Vita", "ux0:data/ReKHCoM/", false },
    { "PS4", "/data/ReKHCom/", false },
    { "PS5", "/data/ReKHCom/", false },
};
static std::atomic<bool> sRunning{false};
static std::mutex sLock;
static std::string sStatus, sDone;

static void Run(fs::path root, int target) {
    const Target& t = kTargets[target];
    fs::path outDir = root / "mod-hd-para-otra-plataforma" / t.name;
    fs::path out = outDir / "ReKHCoM-HD.zip", tmp = outDir / "ReKHCoM-HD.zip.part";
    std::error_code ec;
    fs::create_directories(outDir, ec);
    mz_zip_archive zip = {};
    std::string result;
    if (!mz_zip_writer_init_file(&zip, tmp.u8string().c_str(), 0)) {
        result = T("No se pudo crear el archivo.", "Could not create the file.");
    } else {
        static const char* const kinds[] = { "music", "sprites", "subs", "movies", "scenes" };
        int files = 0;
        bool ok = true;
        for (const char* kind : kinds) {
            if (!t.movies && (strcmp(kind, "movies") == 0 || strcmp(kind, "scenes") == 0))
                continue;
            fs::path dir = root / "hd" / kind;
            if (!fs::is_directory(dir, ec))
                continue;
            for (auto it = fs::recursive_directory_iterator(dir, ec); ok && it != fs::recursive_directory_iterator(); it.increment(ec)) {
                if (!it->is_regular_file(ec) || it->path().filename().u8string().rfind(".", 0) == 0)
                    continue;
                std::string name = "hd/" + fs::relative(it->path(), root / "hd", ec).generic_u8string();
                {
                    std::lock_guard<std::mutex> g(sLock);
                    sStatus = std::string(T("Empaquetando ", "Packing ")) + name;
                }
                ok = mz_zip_writer_add_file(&zip, name.c_str(), it->path().u8string().c_str(), nullptr, 0, MZ_NO_COMPRESSION);
                files += ok;
            }
        }
        ok = mz_zip_writer_finalize_archive(&zip) && ok;
        mz_zip_writer_end(&zip);
        fs::remove(out, ec);
        if (ok && files > 0 && (fs::rename(tmp, out, ec), !ec)) {
            char buf[700];
            snprintf(buf, sizeof(buf), T("Listo: %d archivos en un solo ReKHCoM-HD.zip. Cópialo a %s y abre el juego: lo instala solo.",
                                         "Done: %d files in one ReKHCoM-HD.zip. Copy it to %s and start the game: it installs itself."),
                     files, t.where);
            result = buf;
            if (!SDL_getenv("DECOMPUI_HD_EXPORT")) /* the harness opens no window */
                SDL_OpenURL(("file:///" + outDir.generic_u8string()).c_str());
        } else {
            fs::remove(tmp, ec);
            result = files ? T("No se pudo escribir el archivo (¿disco lleno?).", "Could not write the file (disk full?).")
                           : T("No hay mod HD cargado todavía.", "No HD mod loaded yet.");
        }
    }
    std::lock_guard<std::mutex> g(sLock);
    sStatus.clear();
    sDone = result;
    sRunning = false;
}

/* the section on the HD page */
static void Section(const fs::path& root, bool haveAny) {
    static int target = 1;
    ImGui::SeparatorText(T("Llevar el mod HD a otra plataforma", "Take the HD mod to another platform"));
    ImGui::TextWrapped("%s", T("Prepara un solo archivo con lo cargado aquí para Android o una consola: se copia a la "
                               "carpeta del juego allí y el juego lo instala solo al abrirse.",
                               "Makes one file with what is loaded here for Android or a console: copy it to the game's "
                               "folder there and the game installs it by itself when it starts."));
    const char* names[5];
    for (int i = 0; i < 5; i++) names[i] = kTargets[i].name;
    ImGui::SetNextItemWidth(Px(200));
    ImGui::Combo("##hdtarget", &target, names, 5);
    ImGui::SameLine();
    ImGui::BeginDisabled(!haveAny || sRunning);
    if (ImGui::Button(T("Crear ReKHCoM-HD.zip", "Make ReKHCoM-HD.zip"))) {
        sRunning = true;
        sDone.clear();
        std::thread(Run, root, target).detach();
    }
    ImGui::EndDisabled();
    std::lock_guard<std::mutex> g(sLock);
    if (!sStatus.empty()) ImGui::TextWrapped("%s", sStatus.c_str());
    if (!sDone.empty()) ImGui::TextWrapped("%s", sDone.c_str());
}
}  // namespace hdexport
#endif

static bool sQuitAsked, sQuitNow; /* the window's close while the HD mod loads: asked first (main loop) */
static int sQuitChoice;           /* closing anyway: 1 keep what is finished to resume, 2 remove what this import made */

/* Closing while the HD mod loads: the player is told it is better to wait; closing anyway stops the import at the next
   block, then keeps it to resume on the next start or removes only what this import created */
static void HdQuitPopup() {
    if (sQuitAsked) {
        ImGui::OpenPopup("hdquit");
        sQuitAsked = false;
    }
    if (sQuitChoice && !hd::sJob.running) {
        if (sQuitChoice == 2)
            hd::DiscardImport(sRoot);
        sQuitNow = true;
    }
    ImGui::SetNextWindowSize(ImVec2(Px(560), 0));
    if (!ImGui::BeginPopupModal("hdquit", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize))
        return;
    Eyebrow(T("MOD HD · CARGANDO", "HD MOD · LOADING"));
    if (sQuitChoice) {
        ImGui::TextWrapped("%s", T("Deteniendo la carga...", "Stopping the import..."));
        ImGui::EndPopup();
        return;
    }
    ImGui::TextWrapped("%s", T("El mod HD se está cargando. Cerrar ahora no es lo ideal: lo mejor es esperar a que termine.",
                               "The HD mod is loading. Closing now is not ideal: it is best to wait until it finishes."));
    ImGui::Spacing();
    if (ImGui::Button(T("Seguir cargando", "Keep loading"), ImVec2(-FLT_MIN, Px(40))))
        ImGui::CloseCurrentPopup();
    if (ImGui::Button(T("Cerrar y retomar después", "Close and resume later"), ImVec2(-FLT_MIN, Px(40)))) {
        hd::sJob.cancel = true;
        sQuitChoice = 1;
    }
    if (ImGui::Button(T("Cerrar y borrar lo de esta carga", "Close and remove this import"), ImVec2(-FLT_MIN, Px(40)))) {
        hd::sJob.cancel = true;
        sQuitChoice = 2;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("Retomar conserva lo ya cargado y sigue desde ahí al abrir de nuevo. Borrar quita solo lo que "
                               "copió esta carga: lo que ya tenías y tus partidas quedan.",
                               "Resuming keeps what is loaded and goes on from there next time. Removing takes away only "
                               "what this import copied: what you had before and your saves stay."));
    ImGui::PopStyleColor();
    ImGui::EndPopup();
}

/* An import cut short (closed, a crash, a power cut): offered on the next start, to resume or discard */
static void HdResumePopup() {
    static bool checked;
    static hd::JournalInfo journal;
    if (!checked && !hd::sJob.running && !SDL_getenv("DECOMPUI_SHOT")) {
        checked = true;
        journal = hd::ReadJournal(sRoot);
        if (journal.found)
            ImGui::OpenPopup("hdresume");
    }
    ImGui::SetNextWindowSize(ImVec2(Px(560), 0));
    if (!ImGui::BeginPopupModal("hdresume", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize))
        return;
    Eyebrow(T("MOD HD · CARGA A MEDIAS", "HD MOD · IMPORT CUT SHORT"));
    char text[256];
    snprintf(text, sizeof(text), T("La carga del mod HD no terminó (se cerró o se apagó el PC). %d archivos ya están listos: "
                                   "se comprueban y se sigue desde ahí.",
                                   "The HD mod's import did not finish (closed, or the PC went off). %d files are ready: "
                                   "they are checked and it goes on from there."),
             (int)journal.files.size());
    ImGui::TextWrapped("%s", text);
    ImGui::Spacing();
    if (ImGui::Button(T("Retomar", "Resume"), ImVec2(-FLT_MIN, Px(40)))) {
        std::error_code ec;
        fs::path source = hd::IsInstall(journal.source) || fs::is_directory(journal.source, ec) ? journal.source : hd::FindInstall();
        hd::Start(source, sRoot, journal.movies, journal.scenes, journal.music, journal.sprites, journal.quality, true);
        sPage = PAGE_EXTRAS; /* the progress is there */
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(T("Descartar lo de esa carga", "Discard that import"), ImVec2(-FLT_MIN, Px(40)))) {
        hd::DiscardImport(sRoot);
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button(T("Más tarde", "Later"), ImVec2(-FLT_MIN, 0)))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

/* The launcher card: find or choose the source, import, and the three switches. */
static bool HdModCard() {
    static fs::path source;
    static bool searched, sScenesMoved;
    bool changed = false;
    if (!searched) {
        searched = true;
        source = hd::FindInstall();
    }
    if (!sScenesMoved) {
        sScenesMoved = true;
        hd::MoveScenes(sRoot / "hd");
    }
    fs::path movies = sRoot / "hd" / "movies", music = sRoot / "hd" / "music";
    int haveScenes = hd::Count(sRoot / "hd" / "scenes", ".mp4");
    int haveMovies = hd::Count(movies, ".mp4") + haveScenes, haveTracks = hd::Count(music, ".ogg");
    int haveSprites = hd::Count(sRoot / "hd" / "sprites", ".png");
    std::error_code ec;
    bool haveBase = fs::exists(movies / "ROP.mp4", ec);

    Eyebrow(T("MOD HD (BETA) · KINGDOM HEARTS 1.5+2.5", "HD MOD (BETA) · KINGDOM HEARTS 1.5+2.5"));
    ImGui::TextWrapped("%s", T("Usa los videos y la banda sonora de Re:Chain of Memories de tu Kingdom Hearts HD 1.5+2.5 ReMIX "
                               "de PC (Steam o Epic). ReKHCoM no incluye nada de ese juego: lo copia de tu instalación.",
                               "Uses the Re:Chain of Memories movies and soundtrack of your PC Kingdom Hearts HD 1.5+2.5 ReMIX "
                               "(Steam or Epic). ReKHCoM ships none of it: it copies them from your installation."));
    ImGui::Spacing();
    {
        /* what is loaded, at a glance: one tile per part, the game's red square when it is there */
        struct Tile { const char* name; int count; const char* unit; } tiles[4] = {
            { T("Videos", "Movies"), haveMovies - haveScenes, T("videos", "movies") },
            { T("Escenas", "Scenes"), haveScenes, T("escenas", "scenes") },
            { T("Música", "Music"), haveTracks, T("pistas", "tracks") },
            { T("HUD HD", "HD HUD"), haveSprites, T("gráficos", "graphics") } };
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float gap = Px(8), w = (ImGui::GetContentRegionAvail().x - 3 * gap) / 4, h = Px(58);
        ImVec2 p = ImGui::GetCursorScreenPos();
        for (int i = 0; i < 4; i++) {
            bool ok = tiles[i].count > 0;
            ImVec2 a(p.x + i * (w + gap), p.y), b(a.x + w, a.y + h);
            dl->AddRectFilled(a, b, ImGui::GetColorU32(ok ? kPanel : kControl), Px(6));
            dl->AddRect(a, b, ImGui::GetColorU32(ok ? kAccent : kBorder), Px(6), 0, ok ? Px(2) : Px(1));
            GameSquare(dl, ImVec2(a.x + Px(16), a.y + Px(17)), Px(12), ok);
            dl->AddText(ImVec2(a.x + Px(30), a.y + Px(8)), ImGui::GetColorU32(kText), tiles[i].name);
            char line[64];
            bool busy = hd::sJob.running && hd::sJob.partTotal[i] > 0;
            if (busy) { /* this part is being imported: how far it went, as a bar along the tile's foot */
                float f = hd::sJob.PartProgress(i);
                snprintf(line, sizeof(line), "%d %%", (int)(f * 100));
                float x0 = a.x + Px(10), x1 = b.x - Px(10), y = b.y - Px(10);
                dl->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + Px(4)), ImGui::GetColorU32(kBorder), Px(2));
                dl->AddRectFilled(ImVec2(x0, y), ImVec2(x0 + (x1 - x0) * f, y + Px(4)), ImGui::GetColorU32(kAccent), Px(2));
            } else if (ok)
                snprintf(line, sizeof(line), "%d %s", tiles[i].count, tiles[i].unit);
            else
                snprintf(line, sizeof(line), "%s", T("Sin cargar", "Not loaded"));
            dl->AddText(ImVec2(a.x + Px(12), a.y + Px(28)), ImGui::GetColorU32(ok || busy ? kText : kMuted), line);
        }
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, h));
    }
    ImGui::Spacing();
#ifdef LAUNCHER_ANDROID
    /* on the phone the mod comes ready from the PC: the hd folder the PC launcher builds (in ReKHCoM's folder),
       copied to the phone and chosen here; the activity copies what is new (launcher_android.inc) */
    (void)haveBase;
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", T("En el teléfono el mod llega armado desde el PC: copia al teléfono la carpeta hd que arma "
                               "el launcher de PC (en la carpeta de ReKHCoM) y elígela aquí. Las escenas ocupan varios GB: "
                               "copia solo lo que quieras. Una segunda copia solo agrega lo nuevo.",
                               "On the phone the mod comes ready from the PC: copy to the phone the hd folder the PC "
                               "launcher builds (in ReKHCoM's folder) and choose it here. The scenes take several GB: copy "
                               "only what you want. A second copy only adds what is new."));
    ImGui::PopStyleColor();
    ImGui::BeginDisabled(hd::sJob.running);
    if (PlayButton(T("COPIAR LA CARPETA HD DEL PC", "COPY THE PC'S HD FOLDER"),
                   ImVec2(ImGui::GetContentRegionAvail().x, Px(52)), !hd::sJob.running))
        AndroidPick(ANDROID_PICK_HD, 0, 0);
#else
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s %s", source.empty() ? T("Juego:", "Game:") : T("Encontrado:", "Found:"),
                       source.empty() ? T("no se encontró KINGDOM HEARTS HD 1.5+2.5 en Steam ni Epic; si lo tienes en otra "
                                          "plataforma o carpeta, elígela.",
                                          "KINGDOM HEARTS HD 1.5+2.5 was not found on Steam or Epic; if it is on another "
                                          "platform or folder, choose it.")
                                      : source.u8string().c_str());
    ImGui::PopStyleColor();
    ImGui::BeginDisabled(hd::sJob.running);
    if (ImGui::Button(T("Buscar el juego", "Find the game")))
        source = hd::FindInstall();
    ImGui::SameLine();
    if (ImGui::Button(T("Elegir carpeta...", "Choose folder..."))) {
        fs::path p = hd::PickFolder();
        if (!p.empty())
            source = p;
    }
    /* everything loaded (movies, scenes, music and HUD, no import cut short): no big load button to press by mistake
       (the user, 24/10), only "already loaded" and a small button that opens the load again, to change the quality
       or fill what is missing (a load keeps every movie already right) */
    static bool sReload;
    bool loaded = haveBase && haveScenes > 0 && haveTracks > 0 && haveSprites > 0 && !hd::sJob.running &&
                  !fs::exists(sRoot / "hd" / "import.journal", ec);
    if (!loaded)
        sReload = false;
    /* the movies' quality, kept in hd/quality (the launcher's own: the game plays whatever was imported): the
       compression (0 source, 1 good, 2 saving); good by default */
    static int quality = -1;
    if (quality < 0) {
        quality = 1;
        std::ifstream(sRoot / "hd" / "quality") >> quality;
        quality = hd::Compression(quality);
    }
    int compression = quality;
    const char* const kCompressionNames[3] = { T("Fuente", "Source"), T("Buena (sugerido)", "Good (suggested)"),
                                               T("Ahorro", "Saving") };
    if (loaded && !sReload) {
        static uintmax_t bytes = (uintmax_t)-1;
        if (bytes == (uintmax_t)-1)
            bytes = hd::InstalledMovieBytes(sRoot);
        char done[160];
        snprintf(done, sizeof(done), "%s  ·  %s  ·  %.1f GB", T("DATOS YA CARGADOS", "ALREADY LOADED"), kCompressionNames[quality],
                 bytes / 1073741824.0);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 a = ImGui::GetCursorScreenPos(), b(a.x + ImGui::GetContentRegionAvail().x, a.y + Px(44));
        dl->AddRectFilled(a, b, ImGui::GetColorU32(kPanel), Px(6));
        dl->AddRect(a, b, ImGui::GetColorU32(kAccent), Px(6), 0, Px(2));
        GameSquare(dl, ImVec2(a.x + Px(18), a.y + Px(22)), Px(12), true);
        dl->AddText(ImVec2(a.x + Px(34), a.y + Px(13)), ImGui::GetColorU32(kText), done);
        ImGui::Dummy(ImVec2(b.x - a.x, b.y - a.y));
        if (ImGui::SmallButton(T("Volver a cargar o cambiar la calidad...", "Load again or change the quality...")))
            sReload = true;
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
        ImGui::TextWrapped("%s", T("Solo hace lo que falte, esté roto o tenga otra calidad.",
                                   "Only does what is missing, broken or at another quality."));
        ImGui::PopStyleColor();
    } else {
    if (loaded && ImGui::SmallButton(T("Cancelar", "Cancel")))
        sReload = false;
    /* the full mod measured (24/10, hd_sizes): source 6.7 GB, good 3.7 GB */
    static const char* const kScenesSize[3] = { "~6.7 GB", "~3.7 GB", "~2 GB" };
    bool pick = CycleRow(T("Compresión de los videos", "Movie compression"),
                     T("Fuente: tal cual. Buena: la mitad de espacio, casi sin diferencia visible. Ahorro: un cuarto "
                       "del espacio, se nota algo en escenas con mucho movimiento. Se aplica al importar, o a lo ya instalado con "
                       "\"Comprimir lo instalado\".",
                       "Source: as it is. Good: half the space, hardly a visible difference. Saving: a quarter of the "
                       "space, somewhat visible in busy scenes. Applied when importing, or to what is installed with "
                       "\"Compress what is installed\"."),
                     compression, 3, kCompressionNames);
    if (pick) {
        quality = compression;
        fs::create_directories(sRoot / "hd", ec);
        std::ofstream(sRoot / "hd" / "quality") << quality << "\n";
    }
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    if (quality == 0)
        ImGui::TextWrapped("%s", T("Fuente ocupa el máximo (el mod completo, unos 6,7 GB); Buena, unos 3,7 GB.",
                                   "Source takes the most space (the full mod, about 6.7 GB); Good, about 3.7 GB."));
    else
        ImGui::TextWrapped("%s", T("Recodificar lleva unos minutos (el mod completo, unos 10 min en un PC actual; más en uno antiguo).",
                                   "Re-encoding takes a few minutes (the full mod, about 10 min on a current PC; more on an older one)."));
    ImGui::PopStyleColor();
    {
        char full[96];
        snprintf(full, sizeof(full), "%s  %s", T("CARGAR TODO DESDE EL JUEGO", "LOAD EVERYTHING FROM THE GAME"),
                 kScenesSize[quality]);
        if (PlayButton(full, ImVec2(ImGui::GetContentRegionAvail().x, Px(52)), !source.empty() && !hd::sJob.running)) {
            hd::Start(source, sRoot, true, true, true, true, quality);
            sReload = false;
        }
        ImGui::BeginDisabled(source.empty());
        if (ImGui::Button(T("Solo lo básico: intros, música y HUD (~0,6 GB)", "Just the basics: intros, music and HUD (~0.6 GB)")))
            hd::Start(source, sRoot, true, false, true, true, quality);
        ImGui::EndDisabled();
    }
    /* what is already installed (a full mod imported at source quality, say), re-encoded here to the quality above:
       no need for the game's installation; a second click confirms, since the larger files are replaced */
    {
        static bool confirm;
        static uintmax_t installed = (uintmax_t)-1;
        if (installed == (uintmax_t)-1 || hd::sJob.running)
            installed = hd::InstalledMovieBytes(sRoot);
        ImGui::BeginDisabled(haveMovies <= 0 || quality == 0);
        char label[160];
        if (confirm)
            snprintf(label, sizeof(label), "%s", T("¿Seguro? Reemplaza tus videos por los más livianos (pulsa de nuevo)",
                                                   "Sure? Replaces your movies with the lighter ones (click again)"));
        else
            snprintf(label, sizeof(label), "%s (%.1f GB %s)", T("Comprimir lo instalado a esta calidad", "Compress what is installed to this quality"),
                     installed / 1073741824.0, T("ahora", "now"));
        if (ImGui::Button(label)) {
            if (confirm)
                hd::StartCompress(sRoot, quality);
            confirm = !confirm;
        }
        ImGui::EndDisabled();
        if (!ImGui::IsItemHovered() && !ImGui::IsItemActive() && confirm && ImGui::IsMouseClicked(0))
            confirm = false;
    }
    } /* loaded */
#endif
    ImGui::EndDisabled();
    if (hd::sJob.running || !hd::sJob.Status().empty()) {
        int total = hd::sJob.total, done = hd::sJob.done;
        if (hd::sJob.running && total > 0) /* the file being re-encoded counts as far as it went */
            ImGui::ProgressBar(std::min(1.0f, ((float)done + hd::sJob.Partial()) / (float)total),
                               ImVec2(-1, 0));
        ImGui::TextWrapped("%s", hd::sJob.Status().c_str());
    }
#ifndef LAUNCHER_ANDROID /* the phone receives the file; it does not make one */
    hdexport::Section(sRoot, haveTracks > 0 || haveSprites > 0);
#endif
    ImGui::Separator();
    changed |= GatedToggle(T("Videos HD", "HD movies"),
                         T("La apertura, los créditos y los finales de Sora y Riku de Re:Chain of Memories.",
                           "Re:Chain of Memories' opening, credits and Sora's and Riku's endings."), Ini("hd_movies"), !(!haveBase));
    changed |= GatedToggle(T("Música HD", "HD music"),
                         T("La banda sonora orquestada de Re:Chain of Memories en lugar de la música de GBA.",
                           "Re:Chain of Memories' soundtrack in place of the GBA music."), Ini("hd_music"), !(haveTracks == 0));
    changed |= GatedToggle(T("Escenas HD (mod completo)", "HD scenes (full mod)"),
                         T("Las escenas de historia de Re:Chain of Memories en lugar de las de GBA donde coinciden, "
                           "con subtítulos en el idioma del juego. Desactivado por defecto.",
                           "Re:Chain of Memories' story scenes in place of the GBA ones where they match, with subtitles "
                           "in the game's language. Off by default."), Ini("hd_scenes"), !(haveScenes <= 0));
    changed |= GatedToggle(T("Subtítulos HD", "HD subtitles"),
                         T("Los subtítulos de Re:Chain of Memories en los videos y escenas HD, en el idioma del juego.",
                           "Re:Chain of Memories' subtitles over the HD movies and scenes, in the game's language."), Ini("hd_subtitles"), !(haveMovies <= 0));
    /* one switch, as the game's HD HUD: the cards, the battle HUD and the title logo */
    if (GatedToggle(T("HUD HD", "HD HUD"),
                  T("Las cartas, el HUD de combate y el logo del título con el arte de Re:Chain of Memories. "
                    "Necesita la vista amplia.",
                    "The cards, the battle HUD and the title logo with Re:Chain of Memories' art. Needs the wide view."),
                  Ini("hd_cards"), haveSprites > 0)) {
        Ini("hd_title") = Ini("hd_cards");
        changed = true;
    }
    return changed;
}
