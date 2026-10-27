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
// The 64-bit game ended by a Windows exception (access violation, illegal instruction, stack overflow...), before or
// after the game started; a closed console (0xc000013a) is not a crash. The launcher then offers 32 bits (#360).
inline bool ReKHCoMCrashed64(uint32_t code) { return code >= 0xc0000000u && code != 0xc000013au; }
