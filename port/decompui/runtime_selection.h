#pragma once
#include <string>
#include <cstdint>
#include "../include/port/port_windows_cpu.h"
// These predicates deliberately contain no clock-based crash heuristic.
inline int ReKHCoMRuntimeMode(const std::string& value) { return value == "1" ? 1 : value == "2" ? 2 : 0; }
inline bool ReKHCoMForce32(const std::string& value) { return value == "1"; }
enum ReKHCoM64MissingRequirement { ReKHCoM64Ready, ReKHCoM64MissingFiles, ReKHCoM64MissingCpu };
inline ReKHCoM64MissingRequirement ReKHCoM64Missing(bool os64, bool host, bool guest, bool sdl, bool sse41) {
    if (!os64 || !host || !guest || !sdl) return ReKHCoM64MissingFiles;
    return sse41 ? ReKHCoM64Ready : ReKHCoM64MissingCpu;
}
inline bool ReKHCoMSelect64(bool os64, bool forced32, bool host, bool guest, bool sdl, bool sse41) {
    return !forced32 && ReKHCoM64Missing(os64, host, guest, sdl, sse41) == ReKHCoM64Ready;
}
inline bool ReKHCoMRetry32(uint32_t code, bool enteredGame) {
    if (enteredGame) return false;
    // Host initialization errors and Windows loader failures only. Never game quits/crashes.
    return code == 2 || code == 3 || code == 4 || code == PORT_WINDOWS64_CPU_UNSUPPORTED || code == 0xc0000135u ||
           code == 0xc000007bu || code == 0xc0000142u;
}
// The 64-bit game ended by a Windows exception (access violation, illegal instruction, stack overflow...), before or
// after the game started; a closed console (0xc000013a) is not a crash. The launcher then offers 32 bits (#360).
inline bool ReKHCoMCrashed64(uint32_t code) { return code >= 0xc0000000u && code != 0xc000013au; }
