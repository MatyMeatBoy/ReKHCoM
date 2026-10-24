/*
 * BUS_PTR (include/types.h): a value the original uses as a pointer although it is none (an open-bus word read
 * through NULL, a word past the end of a ROM table), decoded as the GBA bus does it. The desktop ports get the same
 * result from their fault handler (port_biosfault.c, BusTarget); Android 64, Switch and Vita have none and would
 * close, so the known sites (marked "@bug") go through this instead. A pointer into the GBA space is returned as it
 * is, so wrapping a site never changes what valid pointers do.
 */
#include <stdint.h>

#include "port/port.h"

extern uint8_t gPortBiosArea[0x10000];

/* The region's canonical bus address (mirrors folded), or 0 for the BIOS and open-bus regions. */
static uint32_t BusDecode(uint32_t a) {
    uint32_t o;

    switch ((a >> 24) & 0xF) {
        case 0x2: return 0x02000000u | (a & 0x3FFFFu);
        case 0x3: return 0x03000000u | (a & 0x7FFFu);
        case 0x4: return (a & 0xFFFFFFu) < 0x400u ? 0x04000000u | (a & 0x3FFu) : 0;
        case 0x5: return 0x05000000u | (a & 0x3FFu);
        case 0x6:
            o = a & 0x1FFFFu;
            return 0x06000000u | (o >= 0x18000u ? o - 0x8000u : o);
        case 0x7: return 0x07000000u | (a & 0x3FFu);
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC: case 0xD: return 0x08000000u | (a & 0x1FFFFFFu);
        case 0xE: case 0xF: return 0x0E000000u | (a & 0xFFFFu);
        default: return 0;
    }
}

void* PortBusPtr(__UINTPTR_TYPE__ p) {
    uint32_t a = (uint32_t)p, bus;

#ifdef PORT_GBA_RELOC
    if (GbaSpaceBusAddress((const void*)p))
        return (void*)p;
#else
    if (a >= 0x02000000u && a < 0x10000000u && BusDecode(a) == a)
        return (void*)p;
#endif
    bus = BusDecode(a);
    if (bus) {
#ifdef PORT_GBA_RELOC
        void* q = GbaSpacePtr(bus);
        if (q)
            return q;
#else
        return (void*)(__UINTPTR_TYPE__)bus;
#endif
    }
    /* the BIOS stand-in, as BusTarget picks it (the last bytes keep room for a word read) */
    return gPortBiosArea + ((a & 0xFFFFu) > 0xFFE0u ? (a & 0xFFFFu) - 0x100u : (a & 0xFFFFu));
}
