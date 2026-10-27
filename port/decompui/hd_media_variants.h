#ifndef REKHCOM_HD_MEDIA_VARIANTS_H
#define REKHCOM_HD_MEDIA_VARIANTS_H
#include "../include/port/port_hd_media.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>
namespace hdvariants {
namespace fs = std::filesystem;
inline bool AtomicReplace(const fs::path& from, const fs::path& to) {
#ifdef _WIN32
    return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
struct Status { bool movies = false, scenes = false; };
inline Status Inspect(const fs::path& data, int language) {
    auto root = data.u8string();
    return {PortHdMediaAvailable(root.c_str(), language, 0) != 0, PortHdMediaAvailable(root.c_str(), language, 1) != 0};
}
inline bool Exportable(const fs::path& data, const std::string& relative) {
    int member = PortHdMediaTransferMember(relative.c_str());
    if (!member) return false;
    int language = relative.rfind("media/en/", 0) == 0 ? 1 : 2;
    char generations[2][64];
    if (!PortHdMediaActive(data.u8string().c_str(), language, generations)) return false;
    if (member > 1)
        return PortHdMediaPendingValid(data.u8string().c_str(), language, (data / "hd" / relative).u8string().c_str()) != 0;
    auto end = relative.find('/', 9);
    std::string generation = relative.substr(9, end - 9);
    return generation == generations[0] || generation == generations[1];
}
inline bool Reusable(const fs::path& data, int language, const std::string& source, int quality, const std::vector<std::string>& required) {
    char generations[2][64], base[1300], path[1600]; PortHdMediaInfo info;
    auto root = data.u8string();
    if (!PortHdMediaActive(root.c_str(), language, generations)) return false;
    for (const auto& member : required) {
        bool found = false;
        if (!PortHdMediaMember(member.c_str())) return false;
        for (int i = member.rfind("scenes/", 0) == 0 ? 1 : 0; i >= 0 && !found; --i) {
            uint64_t bytes = 0;
            if (!PortHdMediaGeneration(root.c_str(), language, generations[i], base, sizeof(base)) ||
                !PortHdMediaCheck(base, language, i, &info, member.c_str(), &bytes) || source != info.source || quality != info.quality) continue;
            snprintf(path, sizeof(path), "%s/%s", base, member.c_str());
            found = PortHdMediaMp4(path, bytes) != 0;
        }
        if (!found) return false;
    }
    return !required.empty();
}
class Transaction {
    fs::path data, directory, stage;
    int language, quality;
    bool movies, scenes, ready = false, renamed = false;
    std::string source, generation;
    std::vector<std::string> required;
    std::map<std::string, uint64_t> entries;
public:
    Transaction(const fs::path& root, int mode, std::string fingerprint, int q, bool m, bool sc, std::vector<std::string> files)
        : data(root), language(mode), quality(q), movies(m), scenes(sc), source(std::move(fingerprint)), required(std::move(files)) {
        if (!PortHdMediaLabel(mode) || !PortHdMediaToken(source.c_str(), "") || source.size() > 63 || q < 0 || q > 2 ||
            (!m && !sc) || required.empty() || required.size() > 256) return;
        for (const auto& member : required) if (!PortHdMediaMember(member.c_str())) return;
        static std::atomic<unsigned> sequence{0};
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        generation = "g-" + std::to_string((unsigned long long)now) + "-" + std::to_string(++sequence);
        directory = data / "hd/media" / PortHdMediaLabel(mode);
        stage = directory / (".stage-" + generation);
        std::error_code ec;
        fs::create_directories(directory, ec);
        ready = !ec && fs::create_directory(stage, ec) && !ec;
    }
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    ~Transaction() {
        // Only our newly-created private stage; never a committed generation.
        if (ready && !renamed && stage.parent_path() == directory && stage.filename().u8string().rfind(".stage-g-", 0) == 0) {
            std::error_code ec; fs::remove_all(stage, ec);
        }
    }
    bool valid() const { return ready; }
    fs::path Root() const { return ready ? stage : fs::path(); }
    fs::path Path(const std::string& member) const { return ready && PortHdMediaMember(member.c_str()) ? stage / fs::u8path(member) : fs::path(); }
    bool Add(const std::string& member) {
        fs::path path = Path(member); std::error_code ec;
        if (path.empty()) return false;
        auto size = fs::file_size(path, ec);
        if (ec || !PortHdMediaMp4(path.u8string().c_str(), size)) return false;
        entries[member] = size; return true;
    }
    bool Commit(bool cancelled, bool failReplace = false, const std::atomic<bool>* cancelFlag = nullptr) {
        if (!ready || renamed || cancelled || (cancelFlag && cancelFlag->load()) || entries.size() != required.size()) return false;
        for (const auto& member : required) if (!entries.count(member)) return false;
        std::ofstream meta(stage / "media.meta", std::ios::binary);
        meta << "REKHCOM_MEDIA_V1 " << PortHdMediaLabel(language) << " user " << source << ' ' << quality << ' '
             << movies << ' ' << scenes << ' ' << entries.size() << '\n';
        for (const auto& entry : entries) meta << entry.first << ' ' << entry.second << '\n';
        meta.close(); if (!meta) return false;
        PortHdMediaInfo info;
        if (!PortHdMediaCheck(stage.u8string().c_str(), language, -1, &info, NULL, NULL)) return false;
        std::error_code ec;
        fs::rename(stage, directory / generation, ec);
        if (ec) return false;
        renamed = true; // immutable, unreferenced generation is harmless on pointer failure
        char old[2][64] = {"-", "-"};
        if (!PortHdMediaActive(data.u8string().c_str(), language, old)) { strcpy(old[0], "-"); strcpy(old[1], "-"); }
        fs::path pending = directory / (generation + ".active.part");
        std::ofstream active(pending, std::ios::binary);
        active << "REKHCOM_MEDIA_ACTIVE_V1\nmovies " << (movies ? generation : old[0])
               << "\nscenes " << (scenes ? generation : old[1]) << '\n';
        active.close();
        if (!active || failReplace || (cancelFlag && cancelFlag->load()) || !AtomicReplace(pending, directory / "active.txt")) { fs::remove(pending, ec); return false; }
        return true;
    }
};
} // namespace hdvariants
#endif
