/* Guest <-> host interface of the PS5 build.
 *
 * The game runs as 32-bit-pointer code (x32) inside the 64-bit process, in
 * memory the host maps below 4 GiB. The guest calls host functions through a
 * trampoline (guest/src/crt0.S); everything crossing it is an integer, a float
 * or a pointer into low memory, so the same signatures work on both sides. */
#ifndef PS5_BRIDGE_H
#define PS5_BRIDGE_H

#include <stdint.h>

#define PS5G_MAGIC 0x35474B52u /* "RKG5" */
#define PS5G_BASE 0x10000000u
#define PS5G_HEAP_SIZE 0x0C000000u
#define PS5G_STACK_SIZE 0x00400000u

/* Placed at PS5G_BASE by the linker script. */
typedef struct Ps5GuestHeader {
    uint32_t magic;
    uint32_t imageEnd;   /* end of the loaded image including bss */
    uint32_t boot;       /* int boot(uint32_t stackTop) */
    uint32_t hostTable;  /* uint64_t[HF_COUNT], filled by the host before boot */
    uint32_t heapStart;  /* [heapStart, heapEnd) is the guest malloc arena */
    uint32_t heapEnd;
    uint32_t stackTop;
    uint32_t regionEnd;  /* the host maps [PS5G_BASE, regionEnd) */
    uint32_t biosArea;   /* the stand-in for the BIOS (gPortBiosArea) that stray pointers are redirected to */
} Ps5GuestHeader;

enum {
#define X(name) HF_##name,
#include "bridge_list.h"
#undef X
    HF_COUNT
};

/* The GBA regions the host maps before boot (see GBA_* in port/port.h). */
typedef struct HostVertex {
    float x, y, u, v;
    uint32_t argb;
} HostVertex;

/* Buttons use the RetroPad numbering: 0 B, 1 Y, 2 Select, 3 Start, 4-7 d-pad up/down/left/right,
 * 8 A, 9 X, 10 L1, 11 R1, 12 L2, 13 R2, 14 L3, 15 R3, 16 touchpad click. */
typedef struct HostPad {
    uint32_t connected;
    uint32_t buttons;
    int16_t lx, ly, rx, ry;
    uint8_t l2, r2, pad0, pad1;
} HostPad;
#define HOSTPAD_MAX 4

/* A finger on the touch screen: id, position as a fraction of the screen (0..1). */
typedef struct HostTouch {
    uint32_t id;
    float x, y;
} HostTouch;
#define HOSTTOUCH_MAX 10

#endif
