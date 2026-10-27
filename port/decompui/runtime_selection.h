#pragma once
#include <string>
#include <cstdint>
// These predicates deliberately contain no clock-based crash heuristic.
inline int ReKHCoMRuntimeMode(const std::string& value) { return value == "1" ? 1 : value == "2" ? 2 : 0; }
inline bool ReKHCoMForce32(const std::string& value) { return value == "1"; }
inline bool ReKHCoMSelect64(bool os64, bool forced32, bool host, bool guest, bool sdl) {
    return os64 && !forced32 && host && guest && sdl;
}
inline bool ReKHCoMRetry32(uint32_t code, bool enteredGame) {
    if (enteredGame) return false;
    // Host initialization errors and Windows loader failures only. Never game quits/crashes.
    return code == 2 || code == 3 || code == 4 || code == 0xc0000135u ||
           code == 0xc000007bu || code == 0xc0000142u;
}
