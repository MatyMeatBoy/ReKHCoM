/* Linux AArch64 uses the desktop SDL services and ARM64 ILP32 image loader. */
#ifndef __aarch64__
#error This host requires AArch64
#endif
#define PORT_LINUXARM64 1
#include "../linux64/linux64_host.c"
