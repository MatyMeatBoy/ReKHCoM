#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../decompui/runtime_preferences.h"
#include <cassert>
#include <iostream>

static void Write(const std::filesystem::path& path, const std::string& value) {
    std::ofstream(path, std::ios::binary) << value;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    std::filesystem::path folder = std::filesystem::u8path(argv[1]);
    assert(!std::filesystem::exists(folder));
    std::filesystem::create_directories(folder);
    auto ini = folder / "decompui.ini";
    const std::string other = "version=jp\r\nui_language=4\r\nhd_offer=0\r\ncustom_setting=keep\r\n";
    Write(ini, "runtime_mode=1\r\nruntime_auto_version=v2.1\r\nauto64_failed=v2.2\r\n" + other);
    auto state = RekhcomRuntime::Migrate(ini, "v2.2");
    assert(state.migrated && state.persisted && state.mode == 0 && state.version == "v2.2" && state.failedVersion.empty());
    std::string updated;
    assert(RekhcomRuntime::Read(ini, updated) && updated.find(other) != std::string::npos);
    assert(RekhcomRuntime::Save(ini, {{"runtime_mode", "1"}}));
    state = RekhcomRuntime::Migrate(ini, "v2.2");
    assert(!state.migrated && state.mode == 1);
    state = RekhcomRuntime::Migrate(ini, "v2.2.1");
    assert(state.migrated && state.persisted && state.mode == 0);
    Write(ini, "force_32=1\ncustom_setting=keep\n");
    state = RekhcomRuntime::Migrate(ini, "v2.2");
    assert(state.mode == 0 && state.persisted);
    assert(RekhcomRuntime::Read(ini, updated) && updated.find("force_32=") == std::string::npos);
    assert(RekhcomRuntime::Save(ini, {{"auto64_failed", "v2.2"}}));
    assert(RekhcomRuntime::Migrate(ini, "v2.2").failedVersion == "v2.2");
    char program[] = "launcher", force32[] = "--force32", force64[] = "--force64";
    char* cli[] = {program, force32, force64};
    assert(ReKHCoMRuntimeOverride(3, cli) == 2);
    assert(ReKHCoMEffectiveRuntimeMode(0, 1) == 1 && ReKHCoMEffectiveRuntimeMode(1, -1) == 1);
    assert(!ReKHCoMSelect64(true, false, true, true, true, false));
    assert(!ReKHCoMSelect64(true, false, false, true, true, true));
    assert(ReKHCoMSelect64(true, false, true, true, true, true));
#ifdef _WIN32
    const std::string original = "runtime_mode=1\ncustom_setting=read-only\n";
    Write(ini, original);
    DWORD attributes = GetFileAttributesW(ini.c_str());
    assert(attributes != INVALID_FILE_ATTRIBUTES && SetFileAttributesW(ini.c_str(), attributes | FILE_ATTRIBUTE_READONLY));
    state = RekhcomRuntime::Migrate(ini, "v2.2");
    bool read = RekhcomRuntime::Read(ini, updated);
    assert(SetFileAttributesW(ini.c_str(), attributes));
    assert(state.mode == 0 && !state.persisted && read && updated == original);
    for (const auto& entry : std::filesystem::directory_iterator(folder))
        assert(entry.path().filename() == "decompui.ini");
#endif
    std::cout << "PASS runtime update/same-version/manual/CLI/capability/read-only transactions\n";
}
