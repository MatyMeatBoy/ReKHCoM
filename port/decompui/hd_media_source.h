#ifndef REKHCOM_HD_MEDIA_SOURCE_H
#define REKHCOM_HD_MEDIA_SOURCE_H
/* Read-only, bounded Steam source evidence. A language signal is not decoded
 * audio verification and must never label an existing imported movie. */
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace hdmedia {
namespace fs = std::filesystem;
constexpr size_t kMaxText = 1024 * 1024;
enum Signal { Unknown, EnglishSignal, JapaneseSignal };
inline bool ValidUtf8(const std::string& text) {
    for (size_t i = 0; i < text.size();) {
        unsigned char first = (unsigned char)text[i++];
        if (first < 0x80) continue;
        unsigned count = first >= 0xc2 && first <= 0xdf ? 1 :
                         first >= 0xe0 && first <= 0xef ? 2 : first >= 0xf0 && first <= 0xf4 ? 3 : 0;
        if (!count || count > text.size() - i) return false;
        uint32_t value = first & (0x7f >> count);
        for (unsigned j = 0; j < count; ++j) {
            unsigned char c = (unsigned char)text[i++];
            if ((c & 0xc0) != 0x80) return false;
            value = (value << 6) | (c & 0x3f);
        }
        if (value < (count == 1 ? 0x80u : count == 2 ? 0x800u : 0x10000u) ||
            value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return false;
    }
    return true;
}
struct Node {
    bool object = false;
    std::string value;
    std::map<std::string, Node> children;
    const Node* Child(const std::string& key) const {
        auto at = children.find(key);
        return at == children.end() ? nullptr : &at->second;
    }
    std::string Text(const std::string& key) const {
        const Node* n = Child(key);
        return n && !n->object ? n->value : std::string();
    }
};
class Vdf {
    const std::string& text;
    size_t at = 0, nodes = 0;
    void Space() {
        for (;;) {
            while (at < text.size() && std::isspace((unsigned char)text[at])) ++at;
            if (at + 1 >= text.size() || text[at] != '/' || text[at + 1] != '/') return;
            while (at < text.size() && text[at] != '\n') ++at;
        }
    }
    bool Quoted(std::string& out) {
        Space();
        if (at >= text.size() || text[at++] != '"') return false;
        out.clear();
        while (at < text.size()) {
            char c = text[at++];
            if (c == '"') return true;
            if ((unsigned char)c < 32 || out.size() >= 4096) return false;
            if (c == '\\') {
                if (at >= text.size()) return false;
                c = text[at++];
                if (c != '\\' && c != '"') return false;
            }
            out += c;
        }
        return false;
    }
    bool Object(Node& node, int depth) {
        if (depth >= 16) return false;
        Space();
        if (at >= text.size() || text[at++] != '{') return false;
        node.object = true;
        for (;;) {
            Space();
            if (at >= text.size()) return false;
            if (text[at] == '}') { ++at; return true; }
            std::string key;
            if (++nodes > 8192 || !Quoted(key) || node.children.count(key)) return false;
            Node value;
            Space();
            if (at < text.size() && text[at] == '{') {
                if (!Object(value, depth + 1)) return false;
            } else if (!Quoted(value.value)) return false;
            node.children.emplace(key, std::move(value));
        }
    }
public:
    explicit Vdf(const std::string& source) : text(source) {}
    bool Parse(const char* expected, Node& root) {
        if (text.size() > kMaxText || !ValidUtf8(text)) return false;
        std::string name;
        if (!Quoted(name) || name != expected || !Object(root, 0)) return false;
        Space();
        return at == text.size();
    }
};
inline std::string ReadBounded(const fs::path& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec || size > kMaxText) return {};
    std::ifstream in(path, std::ios::binary);
    std::string value((size_t)size, '\0');
    if (size && !in.read(&value[0], (std::streamsize)size)) return {};
    return value;
}
struct Manifest {
    bool valid = false, pending = false;
    std::string installDir, language, build, state;
    std::map<std::string, std::string> depots;
};
inline bool SafeDir(const std::string& value) {
    return !value.empty() && value != "." && value != ".." &&
           value.find_first_of("/\\:") == std::string::npos;
}
inline Manifest ParseManifest(const std::string& text) {
    Manifest result;
    Node root;
    if (!Vdf(text).Parse("AppState", root) || root.Text("appid") != "2552430") return result;
    result.installDir = root.Text("installdir");
    if (!SafeDir(result.installDir)) return Manifest();
    result.build = root.Text("buildid");
    result.state = root.Text("StateFlags");
    std::string target = root.Text("TargetBuildID");
    result.pending = !target.empty() && target != "0" && target != result.build;
    auto number = [](const std::string& text, uint64_t& value) {
        value = 0;
        if (text.empty() || text.size() > 19) return false;
        for (unsigned char c : text) {
            if (c < '0' || c > '9') return false;
            value = value * 10 + (c - '0');
        }
        return true;
    };
    std::string download = root.Text("BytesToDownload");
    if (!download.empty()) {
        uint64_t total = 0, done = 0;
        result.pending |= !number(download, total) ||
                          (!root.Text("BytesDownloaded").empty() && !number(root.Text("BytesDownloaded"), done)) || done < total;
    }
    if (const Node* config = root.Child("UserConfig")) result.language = config->Text("language");
    if (const Node* depots = root.Child("InstalledDepots"))
        for (const auto& entry : depots->children) result.depots[entry.first] = entry.second.Text("manifest");
    result.valid = true;
    return result;
}
inline std::vector<fs::path> FindSteamInstalls(const std::vector<fs::path>& steamRoots) {
    std::set<fs::path> libraries;
    // Only configured roots/libraries; never recurse over disks or app content.
    size_t roots = 0;
    for (const auto& steam : steamRoots) {
        if (++roots > 16) break;
        if (steam.empty()) continue;
        if (libraries.size() < 64) libraries.insert(steam);
        for (const char* relative : {"steamapps/libraryfolders.vdf", "config/libraryfolders.vdf"}) {
            Node root;
            std::string text = ReadBounded(steam / relative);
            if (!Vdf(text).Parse("libraryfolders", root)) continue;
            for (const auto& entry : root.children) {
                std::string path = entry.second.object ? entry.second.Text("path") : entry.second.value;
                if (libraries.size() < 64 && !path.empty() && fs::u8path(path).is_absolute()) libraries.insert(fs::u8path(path));
            }
        }
    }
    std::vector<fs::path> result;
    for (const auto& library : libraries) {
        Manifest m = ParseManifest(ReadBounded(library / "steamapps/appmanifest_2552430.acf"));
        std::error_code ec;
        auto install = library / "steamapps/common" / fs::u8path(m.installDir);
        if (m.valid && fs::is_directory(install, ec)) result.push_back(install);
    }
    return result;
}
inline void Hash(uint64_t& hash, const std::string& text) {
    for (unsigned char c : text) { hash ^= c; hash *= UINT64_C(1099511628211); }
    hash ^= 0xff; hash *= UINT64_C(1099511628211);
}
inline bool Sample(uint64_t& hash, const fs::path& path) {
    Hash(hash, path.generic_u8string());
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    if (ec || size < 256) { Hash(hash, "absent-or-short"); return false; }
    Hash(hash, std::to_string(size));
    auto time = fs::last_write_time(path, ec);
    if (!ec) Hash(hash, std::to_string((long long)time.time_since_epoch().count()));
    std::ifstream in(path, std::ios::binary);
    char head[256], tail[256];
    if (!in.read(head, sizeof(head))) return false;
    in.seekg((std::streamoff)(size - sizeof(tail)));
    if (!in.read(tail, sizeof(tail))) return false;
    Hash(hash, std::string(head, sizeof(head))); Hash(hash, std::string(tail, sizeof(tail)));
    return true;
}
struct Probe {
    bool steam = false, complete = false, movies = false, scenes = false, music = false;
    Signal signal = Unknown;
    std::string declaredLanguage, build, depotSummary, fingerprint;
};
inline Probe Inspect(const fs::path& source) {
    Probe probe;
    if (source.empty()) return probe;
    uint64_t hash = UINT64_C(14695981039346656037);
    Hash(hash, source.generic_u8string());
    // The exact selected directory must be the ACF's app directory; a nearby
    // manifest must not lend language evidence to a loose/custom asset folder.
    auto acf = source.parent_path().parent_path() / "appmanifest_2552430.acf";
    Manifest m = ParseManifest(ReadBounded(acf));
    std::error_code ec;
    auto expected = fs::weakly_canonical(acf.parent_path() / "common" / fs::u8path(m.installDir), ec);
    auto actual = fs::weakly_canonical(source, ec);
    probe.steam = m.valid && !ec && expected == actual;
    if (probe.steam) {
        probe.complete = m.state == "4" && !m.pending;
        probe.declaredLanguage = m.language; probe.build = m.build;
        Hash(hash, m.language); Hash(hash, m.build); Hash(hash, m.state); Hash(hash, m.pending ? "pending" : "ready");
        for (const auto& depot : m.depots) {
            if (!probe.depotSummary.empty()) probe.depotSummary += ", ";
            probe.depotSummary += depot.first;
            Hash(hash, depot.first); Hash(hash, depot.second);
        }
        // Explicit revision mapping, signal only. General files verified locally;
        // JP metadata cross-checked against published listings (see project note).
        // Unknown revisions, conflicting/mixed depots or languages stay unknown.
        auto is = [&](const char* id, const char* manifest) {
            auto at = m.depots.find(id); return at != m.depots.end() && at->second == manifest;
        };
        if (probe.complete && m.build == "15194255" && m.depots.size() == 2) {
            bool general = is("2552433", "2946731077053901934") && is("2552435", "3908821002986173448");
            bool japanese = is("2552432", "8878064114965627002") && is("2552434", "7005798054209199771");
            if (general && !m.depots.count("2552432") && !m.depots.count("2552434") &&
                (m.language == "english" || m.language == "spanish" || m.language == "french" ||
                 m.language == "german" || m.language == "italian")) probe.signal = EnglishSignal;
            if (japanese && !m.depots.count("2552433") && !m.depots.count("2552435") && m.language == "japanese")
                probe.signal = JapaneseSignal;
        }
    }
    auto movies = source / "STEAM/dt/KHCReSource/BIN/movie";
    if (!fs::is_directory(movies, ec)) movies = source / "EPIC/dt/KHCReSource/BIN/movie";
    if (!fs::is_directory(movies, ec)) movies = source;
    // Deterministic inventory, at most 512 entries and 512 bytes sampled per
    // recognized movie. No decoding or scan of multi-GB package contents.
    std::vector<fs::path> media;
    size_t entries = 0;
    for (fs::directory_iterator it(movies, ec), end; !ec && it != end && entries < 512; it.increment(ec), ++entries) {
        std::string name = it->path().filename().u8string();
        if ((name == "ROP.mp4" || name == "RSE.mp4" || name == "RRE.mp4" ||
             (name.rfind("zz_event_", 0) == 0 && it->path().extension() == ".mp4")) && it->is_regular_file(ec))
            media.push_back(it->path());
    }
    std::sort(media.begin(), media.end());
    for (const auto& path : media) {
        if (!Sample(hash, path)) continue;
        if (path.filename().u8string().rfind("zz_event_", 0) == 0) probe.scenes = true;
        else probe.movies = true;
    }
    if (ec || entries == 512) { probe.signal = Unknown; Hash(hash, "inventory-incomplete"); }
    bool hed = Sample(hash, source / "Image/dt/Recom.hed");
    bool pkg = Sample(hash, source / "Image/dt/Recom.pkg");
    probe.music = hed && pkg;
    if (!probe.movies && !probe.scenes) probe.signal = Unknown;
    std::ostringstream out;
    out << std::hex << hash;
    probe.fingerprint = out.str(); // change detector, not an authenticity/content hash
    return probe;
}
struct Cache {
    Probe probe;
    bool initialized = false, changed = false;
    void Refresh(const fs::path& source) {
        Probe next = Inspect(source);
        if (initialized && probe.fingerprint != next.fingerprint) changed = true;
        probe = std::move(next); initialized = true;
    }
};
} // namespace hdmedia
#endif
