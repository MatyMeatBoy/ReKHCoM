#include "../decompui/hd_media_source.h"
#include <cassert>
#include <iostream>
using namespace hdmedia;
static void Put(const fs::path& path, const std::string& value) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << value;
}
static std::string ManifestText(bool jp = false, const std::string& lang = "spanish",
                                const std::string& state = "4", const std::string& build = "15194255") {
    return "\"AppState\" {\"appid\" \"2552430\" \"installdir\" \"Custom KH Folder\" "
           "\"StateFlags\" \"" + state + "\" \"buildid\" \"" + build + "\" "
           "\"UserConfig\" {\"language\" \"" + lang + "\"} \"InstalledDepots\" {" +
           (jp ? "\"2552432\" {\"manifest\" \"8878064114965627002\"} \"2552434\" {\"manifest\" \"7005798054209199771\"}"
               : "\"2552433\" {\"manifest\" \"2946731077053901934\"} \"2552435\" {\"manifest\" \"3908821002986173448\"}") + "}}";
}
int main(int argc, char** argv) {
    assert(argc == 2);
    fs::path root = fs::u8path(argv[1]);
    auto manifest = ParseManifest(ManifestText());
    assert(manifest.valid && manifest.language == "spanish" && manifest.depots.size() == 2);
    assert(!ParseManifest(ManifestText() + " garbage").valid);
    std::string invalidUtf8 = ManifestText(false, std::string(1, (char)0xff));
    assert(!ParseManifest(invalidUtf8).valid);
    assert(!ParseManifest("\"AppState\" {\"appid\" \"2552430\"").valid);
    std::string duplicate = ManifestText();
    duplicate.insert(duplicate.size() - 1, "\"appid\" \"2552430\"");
    assert(!ParseManifest(duplicate).valid);
    assert(!ParseManifest(std::string(1024 * 1024 + 1, ' ')).valid);
    std::string nested = "\"AppState\" {";
    for (int i = 0; i < 20; ++i) nested += "\"deep\" {";
    nested += std::string(21, '}');
    Node tree;
    assert(!Vdf(nested).Parse("AppState", tree));
    auto escaped = ParseManifest("\"AppState\" {\"appid\" \"2552430\" \"installdir\" \"..\\\\Other\"}");
    assert(!escaped.valid);
    fs::path library = root / fs::u8path(u8"Library B 日本語"), install = library / "steamapps/common/Custom KH Folder";
    fs::path acf = library / "steamapps/appmanifest_2552430.acf";
    Put(acf, ManifestText());
    const auto movies = install / "STEAM/dt/KHCReSource/BIN/movie";
    Put(movies / "ROP.mp4", std::string(512, 'v'));
    Put(movies / "zz_event_011.mp4", std::string(512, 's'));
    Put(install / "Image/dt/Recom.hed", std::string(512, 'h'));
    Put(install / "Image/dt/Recom.pkg", std::string(512, 'p'));
    fs::path steam = root / "Steam";
    std::string path = library.generic_u8string();
    Put(steam / "steamapps/libraryfolders.vdf", "\"libraryfolders\" {\"0\" {\"path\" \"" + steam.generic_u8string() + "\"} \"1\" {\"path\" \"" + path + "\"}} ");
    auto found = FindSteamInstalls({steam});
    assert(found.size() == 1 && found[0] == install); // custom folder, second library
    auto probe = Inspect(install);
    assert(probe.steam && probe.complete && probe.movies && probe.scenes && probe.music);
    assert(probe.signal == EnglishSignal && probe.declaredLanguage == "spanish"); // never Spanish audio
    assert(!probe.fingerprint.empty());
    Cache cache;
    cache.Refresh(install);
    assert(!cache.changed);
    auto old = cache.probe.fingerprint;
    auto stamp = fs::last_write_time(movies / "ROP.mp4");
    Put(movies / "ROP.mp4", std::string(512, 'c'));
    fs::last_write_time(movies / "ROP.mp4", stamp);
    assert(Inspect(install).fingerprint != old); // same size/mtime, sampled content differs
    Put(root / "user/hd/movies/ROP.mp4", "old EN import");
    // Steam replaces homonymous movie files; rescan must never reuse cached EN evidence.
    Put(acf, ManifestText(true, "japanese"));
    Put(movies / "ROP.mp4", std::string(512, 'j'));
    cache.Refresh(install);
    assert(cache.changed && cache.probe.signal == JapaneseSignal && cache.probe.fingerprint != old);
    // Old imported data stays untouched and must never inherit the selected source language.
    assert(ReadBounded(root / "user/hd/movies/ROP.mp4") == "old EN import");
    Put(acf, ManifestText(true, "japanese", "6"));
    cache.Refresh(install);
    assert(!cache.probe.complete && cache.probe.signal == Unknown);
    std::string pending = ManifestText();
    pending.insert(pending.size() - 1, "\"BytesToDownload\" \"100\" \"BytesDownloaded\" \"50\"");
    Put(acf, pending);
    assert(!Inspect(install).complete && Inspect(install).signal == Unknown);
    Put(acf, ManifestText(false, "english", "4", "future-build"));
    assert(Inspect(install).signal == Unknown);
    std::string mixed = ManifestText();
    mixed.insert(mixed.size() - 2, "\"2552436\" {\"manifest\" \"foreign-deck\"}");
    Put(acf, mixed);
    assert(Inspect(install).signal == Unknown);
    Put(acf, ManifestText(false, "japanese")); // conflicting text/depot signals
    assert(Inspect(install).signal == Unknown);
    Put(acf, ManifestText(true, "japanese"));
    fs::remove(movies / "ROP.mp4");
    probe = Inspect(install);
    assert(!probe.movies && probe.scenes && probe.signal == JapaneseSignal);
    Put(acf, "broken");
    probe = Inspect(install);
    assert(probe.signal == Unknown && !probe.complete);
    assert(FindSteamInstalls({steam}).empty());
    // Epic/loose files have availability, but never borrow nearby Steam language.
    Put(root / "Epic/EPIC/dt/KHCReSource/BIN/movie/RSE.mp4", std::string(512, 'e'));
    Put(root / "Epic/EPIC/dt/KHCReSource/BIN/movie/zz_event_099.mp4", std::string(512, 'e'));
    probe = Inspect(root / "Epic");
    assert(probe.movies && probe.scenes && !probe.steam && probe.signal == Unknown);
    Put(root / "loose/ROP.mp4", std::string(512, 'l'));
    assert(Inspect(root / "loose").movies && Inspect(root / "loose").signal == Unknown);
    std::cout << "PASS bounded ACF, multiple libraries, provenance, partial source and EN->JP rescan\n";
}
