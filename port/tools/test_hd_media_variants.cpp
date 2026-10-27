#include "../decompui/hd_media_variants.h"
#include <cassert>
#include <iostream>
using namespace hdvariants;
static void Put(const fs::path& p, const std::string& s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
static std::string Movie(char marker) {
    std::string s("\0\0\0\x08moov\0\0\0\x10mdat", 16); s += std::string(8, marker); return s;
}
int main(int argc, char** argv) {
    assert(argc == 2); fs::path data = fs::u8path(argv[1]);
    Put(data / "hd/movies/ROP.mp4", Movie('L'));
    std::vector<std::string> required = {"movies/ROP.mp4", "scenes/zz_event_151.mp4"};
    Transaction en(data, 1, "aabb", 1, true, false, required);
    assert(en.valid());
    for (const auto& p : required) { Put(en.Path(p), Movie('E')); assert(en.Add(p)); }
    assert(en.Commit(false));
    char path[2048];
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "ROP", path, sizeof(path)));
    auto original = std::string(path);
    assert(original.find("/en/") != std::string::npos);
    assert(Inspect(data, 1).movies && !Inspect(data, 1).scenes);
    assert(Reusable(data, 1, "aabb", 1, required));
    assert(!Reusable(data, 1, "ccdd", 1, required)); // same names, changed source cannot hit EN cache
    assert(!PortHdMediaResolve(data.u8string().c_str(), 2, "ROP", path, sizeof(path)));
    {
        Transaction jp(data, 2, "ccdd", 1, true, false, required);
        Put(jp.Path(required[0]), Movie('J')); assert(jp.Add(required[0]));
        assert(!jp.Commit(true)); // cancel leaves EN and legacy intact, no JP availability
    }
    assert(!Inspect(data, 2).movies);
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "ROP", path, sizeof(path)) && original == path);
    {
        Transaction corrupt(data, 1, "ccdd", 1, true, false, required);
        Put(corrupt.Path(required[0]), "broken"); assert(!corrupt.Add(required[0]));
        assert(!corrupt.Commit(false));
    }
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "ROP", path, sizeof(path)) && original == path);
    {
        Transaction failed(data, 1, "ccdd", 1, true, false, required);
        for (const auto& p : required) { Put(failed.Path(p), Movie('N')); assert(failed.Add(p)); }
        assert(!failed.Commit(false, true)); // injected atomic-pointer failure
    }
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "ROP", path, sizeof(path)) && original == path);
    Transaction jp(data, 2, "ccdd", 1, true, false, required);
    for (const auto& p : required) { Put(jp.Path(p), Movie('J')); assert(jp.Add(p)); }
    assert(jp.Commit(false));
    assert(PortHdMediaResolve(data.u8string().c_str(), 2, "ROP", path, sizeof(path)) && original != path);
    assert(Inspect(data, 2).movies);
    // Auto resolves legacy, never relabels it from current Steam configuration.
    assert(PortHdMediaResolve(data.u8string().c_str(), 0, "ROP", path, sizeof(path)));
    assert(fs::path(path) == data / "hd/movies/ROP.mp4");
    assert(!PortHdMediaResolve(data.u8string().c_str(), 1, "../ROP", path, sizeof(path)));
    assert(!PortHdMediaResolve(data.u8string().c_str(), 1, "zz_event_099", path, sizeof(path)));
    assert(!en.Path("../../user.sav").has_filename());
    // Source generation/poster paths differ; corrupt active pointers fail closed.
    Put(data / "hd/media/ja/active.txt", "REKHCOM_MEDIA_ACTIVE_V1\nmovies ../../en\nscenes -\n");
    assert(!PortHdMediaResolve(data.u8string().c_str(), 2, "ROP", path, sizeof(path)));
    assert(!Inspect(data, 2).movies);
    // A separate scene update keeps the already committed movie component.
    std::vector<std::string> scene = {"scenes/zz_event_011.mp4"};
    Transaction scenes(data, 1, "eeff", 1, false, true, scene);
    Put(scenes.Path(scene[0]), Movie('S')); assert(scenes.Add(scene[0]) && scenes.Commit(false));
    assert(Inspect(data, 1).movies && Inspect(data, 1).scenes);
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "ROP", path, sizeof(path)) && original == path);
    assert(PortHdMediaResolve(data.u8string().c_str(), 1, "zz_event_011", path, sizeof(path)));
    Put(fs::path(path), "truncated");
    assert(!Inspect(data, 1).scenes && Inspect(data, 1).movies);
    assert(!PortHdMediaResolve(data.u8string().c_str(), 1, "zz_event_011", path, sizeof(path)));
    std::cout << "PASS EN/JP generation commit, cancel/fail preservation, provenance cache, legacy and corrupt paths\n";
}
