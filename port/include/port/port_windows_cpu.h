#ifndef PORT_WINDOWS_CPU_H
#define PORT_WINDOWS_CPU_H
#include <stdint.h>

/* The shared x32 guest is compiled with -msse4.1, unlike the native host. */
enum { PORT_WINDOWS64_CPU_UNSUPPORTED = 5 };
static inline int PortWindowsCpuHasSse41(uint32_t leaf1Ecx) {
    return (leaf1Ecx & (UINT32_C(1) << 19)) != 0;
}
#ifdef _WIN32
#include <intrin.h>
static inline uint32_t PortWindowsCpuLeaf1Ecx(void) {
    int regs[4];
    __cpuid(regs, 1);
    return (uint32_t)regs[2];
}
#endif
#endif
