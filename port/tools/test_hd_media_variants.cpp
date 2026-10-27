#include "../decompui/hd_media_variants.h"
#include <cassert>
#include <iostream>
using namespace hdvariants;
static std::string dataRoot;
static uint64_t ticks;
static struct { int hdMediaLanguage = 0, undub = 1, hdSubtitles = 1, textLanguage = 4; } gPortConfig;
static uint64_t SDL_GetTicks() { return ticks; }
static const char* PortConfigPath(const char*) { return dataRoot.c_str(); }
static void SDL_strlcpy(char* p, const char* s, size_t cap) { snprintf(p, cap, "%s", s); }
#include "hd_media_path_cache.inc"
static int invalidations, saved;
static bool available[2][2];
static int PortHdMediaVariantAvailable(int mode, int part) { return mode > 0 && mode < 3 && available[mode - 1][part]; }
static void PortHdMediaInvalidate() { ++invalidations; PortHdMovieInvalidatePaths(); PortHdMediaAvailabilityReset(); }
static void PortConfigSave() { ++saved; }
#include "hd_media_ui.inc"
static void Put(const fs::path& p, const std::string& s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
static std::string Movie(char marker) {
    std::string s("\0\0\0\x08moov\0\0\0\x10mdat", 16); s += std::string(8, marker); return s;
}
int main(int argc, char** argv) {
    assert(argc == 2); fs::path data = fs::u8path(argv[1]);
    uint64_t number = 0;
    assert(PortHdMediaNumber("1073741824", 1073741824, &number) && number == 1073741824);
    assert(!PortHdMediaNumber("18446744073709551616", 1073741824, &number));
    assert(!PortHdMediaNumber("-24", 1073741824, &number));
    assert(PortHdMediaClipAllowed(0, 170) && PortHdMediaClipAllowed(1, 222));
    assert(PortHdMediaClipAllowed(2, 0) && !PortHdMediaClipAllowed(2, 170) && !PortHdMediaClipAllowed(2, 222));
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
    assert(PortHdMediaCachedAvailable(data.u8string().c_str(), 1, 0));
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
    // In-game controls reject missing variants without changing text/Undub.
    assert(HdMediaSelectionEnabled(0) && !HdMediaSelectionEnabled(1));
    HdMediaSelect(1); assert(!saved && gPortConfig.hdMediaLanguage == 0);
    available[0][0] = true; HdMediaSelect(1);
    assert(saved == 1 && invalidations == 1 && gPortConfig.hdMediaLanguage == 1);
    assert(gPortConfig.undub == 1 && gPortConfig.hdSubtitles == 1 && gPortConfig.textLanguage == 4);
    dataRoot = data.u8string(); ticks = 0;
    assert(PortHdMovieResolvePath("ROP", path, sizeof(path)) && original == path);
    Transaction replacement(data, 1, "1234", 1, true, false, required);
    for (const auto& p : required) { Put(replacement.Path(p), Movie('R')); assert(replacement.Add(p)); }
    assert(replacement.Commit(false));
    assert(PortHdMovieResolvePath("ROP", path, sizeof(path)) && original == path); // cached, no repeated MP4 validation
    PortHdMediaInvalidate();
    assert(PortHdMovieResolvePath("ROP", path, sizeof(path)) && original != path); // explicit rescan updates generation
    ticks = 1001;
    assert(PortHdMovieResolvePath("ROP", path, sizeof(path)));
    HdMediaSelect(0);
    assert(PortHdMovieResolvePath("ROP", path, sizeof(path)) && fs::path(path) == data / "hd/movies/ROP.mp4");
    std::cout << "PASS EN/JP generation commit, cancel/fail preservation, provenance cache, legacy and corrupt paths\n";
}
