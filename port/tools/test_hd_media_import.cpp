#include "../decompui/hd_media_source.h"
#include "../decompui/hd_media_variants.h"
#include <functional>
#include <mutex>
#include <cassert>
#include <iostream>
namespace fs = std::filesystem;
enum { PART_MOVIES, PART_SCENES };
static const char* T(const char*, const char* en) { return en; }
static struct Job {
    std::atomic<bool> cancel{false};
    void Plan(int, int) {} void Finish(int) {} void Say(const std::string&) {}
} sJob;
static int sUnitShift, calls, cancelAt, failAt, changeAt;
static fs::path MovieFolder(const fs::path&) { return {}; }
static int MoviePart(const fs::path& p) { return p.stem().u8string().rfind("zz_event_", 0) == 0 ? 1 : 0; }
static int Units(const fs::path&) { return 1; }
static int Workers(int) { return 1; }
static void Parallel(const std::vector<fs::path>& files, int, const std::function<void(const fs::path&)>& work) {
    for (const auto& file : files) work(file);
}
static bool ImportMovie(const fs::path& src, const fs::path& dst, int) {
    ++calls;
    if (calls == cancelAt) { sJob.cancel = true; return false; }
    if (calls == failAt) return false;
    std::error_code ec; fs::copy_file(src, dst, ec);
    if (!ec && calls == changeAt) { std::fstream changed(src, std::ios::in | std::ios::out | std::ios::binary); changed.seekp(-1, std::ios::end); changed.put('X'); }
    return !ec;
}
static void WriteMoviePosters(const fs::path&) {}
static int WriteSubtitles(const fs::path&, const fs::path&, const fs::path&) { return 0; }
#include "hd_media_import.inc"
static std::string Movie(char marker) {
    std::string s("\0\0\0\x08moov\0\0\x02\x08mdat", 16); s += std::string(512, marker); return s;
}
static void Put(const fs::path& p, const std::string& bytes) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << bytes; }
int main(int argc, char** argv) {
    assert(argc == 2); fs::path root = fs::u8path(argv[1]), source = root / "owned-source", data = root / "data";
    for (const char* name : kBaseMovies) Put(source / (std::string(name) + ".mp4"), Movie('E'));
    Put(source / "zz_event_123.mp4", Movie('E')); // Independent movie3 input, even if catalog is incomplete.
    Put(data / "hd/movies/ROP.mp4", Movie('L'));
    // Same source fingerprint, but a pre-fix basic generation selected only six
    // files. It must not authorize reuse when the required movie3 is added.
    std::vector<std::string> previousSix = { "movies/ROP.mp4", "movies/RSE.mp4", "movies/RRE.mp4",
        "scenes/zz_event_151.mp4", "scenes/zz_event_621.mp4", "scenes/zz_event_631.mp4" };
    {
        hdvariants::Transaction old(data, 1, hdmedia::Inspect(source).fingerprint, 0, true, false, previousSix);
        assert(old.valid());
        for (const auto& member : previousSix) { Put(old.Path(member), Movie('E')); assert(old.Add(member)); }
        assert(old.Commit(false));
    }
    int count = 0;
    assert(ImportMediaVariant(source, data, 1, true, false, 0, false, count) && count == 7 && calls == 7);
    assert(sUnitShift == 26);
    char path[1600];
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "ROP", path, sizeof(path)));
    const std::string en = path;
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "zz_event_123", path, sizeof(path)));
    assert(fs::path(path).parent_path().filename() == "scenes");
    assert(ImportMediaVariant(source, data, 1, true, false, 0, false, count) && calls == 7); // persistent same-source cache
    for (const char* name : kBaseMovies) Put(source / (std::string(name) + ".mp4"), Movie('J'));
    calls = 0; cancelAt = 2;
    assert(!ImportMediaVariant(source, data, 2, true, false, 0, false, count));
    assert(!hdvariants::Inspect(data, 2).movies && hdvariants::Inspect(data, 1).movies);
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "ROP", path, sizeof(path)) && en == path);
    cancelAt = 0; calls = 0; sJob.cancel = false;
    assert(ImportMediaVariant(source, data, 2, true, false, 0, false, count) && calls == 7);
    assert(PortHdMediaResolve(data.u8string().c_str(), 2, "ROP", path, sizeof(path)) && en != path);
    const std::string jp = path;
    // New content + a failed copy cannot publish over a complete JP generation.
    Put(source / "ROP.mp4", Movie('N')); calls = 0; failAt = 3;
    assert(!ImportMediaVariant(source, data, 2, true, false, 0, false, count));
    assert(PortHdMediaResolve(data.u8string().c_str(), 2, "ROP", path, sizeof(path)) && jp == path);
    failAt = 0;
    for (const char* name : kBaseMovies) Put(source / (std::string(name) + ".mp4"), Movie('R'));
    calls = 0; changeAt = 6;
    assert(!ImportMediaVariant(source, data, 2, true, false, 0, false, count));
    assert(PortHdMediaResolve(data.u8string().c_str(), 2, "ROP", path, sizeof(path)) && jp == path);
    changeAt = 0;
    // A truncated required source and an incomplete story inventory fail closed.
    Put(source / "ROP.mp4", "corrupt"); calls = 0;
    assert(!ImportMediaVariant(source, data, 2, true, false, 0, false, count));
    assert(!ImportMediaVariant(source, data, 2, false, true, 0, false, count));
    assert(PortHdMediaResolve(data.u8string().c_str(), 2, "ROP", path, sizeof(path)) && jp == path);
    assert(PortHdMediaResolve(data.u8string().c_str(), 0, "ROP", path, sizeof(path)));
    // Load Everything calls this real coordinator with both component flags.
    // Mixed generation metadata must be safe when Inspect checks either part.
    for (const char* name : kBaseMovies) Put(source / (std::string(name) + ".mp4"), Movie('B'));
    for (const char* name : kMediaSceneNames) Put(source / (std::string(name) + ".mp4"), Movie('B'));
    calls = 0;
    assert(ImportMediaVariant(source, data, 1, true, true, 0, false, count));
    std::map<std::string, bool> uniqueMovies;
    for (const char* name : kBaseMovies) uniqueMovies[name] = true;
    for (const char* name : kMediaSceneNames) uniqueMovies[name] = true;
    assert(count == (int)uniqueMovies.size() && calls == count); // Full profile copies each file once.
    auto combined = hdvariants::Inspect(data, 1);
    assert(combined.movies && combined.scenes);
    assert(PortHdMediaAvailable(data.u8string().c_str(), 1, 0));
    assert(PortHdMediaAvailable(data.u8string().c_str(), 1, 1));
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "zz_event_001", path, sizeof(path)));
    const std::string story = path;
    // Partial damage to a non-ending story file leaves movies available.
    Put(fs::u8path(story), "partial transferred scene");
    combined = hdvariants::Inspect(data, 1);
    assert(combined.movies && !combined.scenes);
    assert(!PortHdMediaAvailable(data.u8string().c_str(), 1, 1));
    std::cout << "PASS production import coordinator: EN->JP, persistent cache, cancel/copy/corrupt preservation, incomplete coverage\n";
}
