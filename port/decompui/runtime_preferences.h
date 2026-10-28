#pragma once
#include "runtime_selection.h"
#include "save_file.h"
#include <fstream>
#include <sstream>
#include <utility>

namespace RekhcomRuntime {
struct State {
    int mode = 0;
    std::string version, failedVersion;
    bool migrated = false, persisted = true;
};
inline State Parse(const std::string& text) {
    State state;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("force_32=", 0) == 0) state.mode = ReKHCoMForce32(line.substr(9)) ? 1 : 0;
        else if (line.rfind("runtime_mode=", 0) == 0) state.mode = ReKHCoMRuntimeMode(line.substr(13));
        else if (line.rfind("runtime_auto_version=", 0) == 0) state.version = line.substr(21);
        else if (line.rfind("auto64_failed=", 0) == 0) state.failedVersion = line.substr(14);
    }
    return state;
}
using Updates = std::vector<std::pair<std::string, std::string>>;
/* Keep unowned settings/comments byte-for-byte; empty values remove old keys. */
inline std::string Patch(const std::string& text, const Updates& values) {
    std::string result;
    for (size_t at = 0; at < text.size();) {
        size_t end = text.find('\n', at);
        end = end == std::string::npos ? text.size() : end + 1;
        std::string line = text.substr(at, end - at);
        bool owned = false;
        for (const auto& item : values)
            if (line.rfind(item.first + "=", 0) == 0) { owned = true; break; }
        if (!owned) result += line;
        at = end;
    }
    const std::string newline = text.find("\r\n") == std::string::npos ? "\n" : "\r\n";
    if (!result.empty() && result.back() != '\n') result += newline;
    for (const auto& item : values)
        if (!item.second.empty()) result += item.first + "=" + item.second + newline;
    return result;
}
inline bool Read(const std::filesystem::path& path, std::string& text) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::error_code ec;
        return !std::filesystem::exists(path, ec) && !ec;
    }
    input.seekg(0, std::ios::end);
    auto size = input.tellg();
    if (size < 0 || size > 1024 * 1024) return false;
    input.seekg(0);
    text.assign(std::istreambuf_iterator<char>(input), {});
    return !input.bad();
}
inline bool Save(const std::filesystem::path& path, const Updates& values) {
    std::string text;
    if (!Read(path, text)) return false; // Never replace an unreadable original.
    auto output = Patch(text, values);
    return RekhcomSave::Replace(path, std::vector<unsigned char>(output.begin(), output.end()));
}
inline State Migrate(const std::filesystem::path& path, const std::string& version) {
    std::string text;
    bool readable = Read(path, text);
    State state = Parse(text);
    if (version.empty() || state.version == version) return state;
    state.mode = 0;
    state.failedVersion.clear();
    state.version = version;
    state.migrated = true;
    state.persisted = readable && Save(path, {{"runtime_mode", "0"}, {"runtime_auto_version", version},
                                             {"force_32", ""}, {"auto64_failed", ""}});
    return state; // A read-only install still runs safely in Auto; no file is destroyed.
}
}
