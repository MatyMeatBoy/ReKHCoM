/*
 * GBA BIOS services used by the game, reimplemented in C.
 */
#include <math.h>
#include <string.h>

#include "port/port.h"
#include "port/port_oam.h"

typedef struct BgAffineSrcData {
    int32_t texX;
    int32_t texY;
    int16_t scrX;
    int16_t scrY;
    int16_t sx;
    int16_t sy;
    uint16_t alpha;
} BgAffineSrcData;

typedef struct BgAffineDstData {
    int16_t pa;
    int16_t pb;
    int16_t pc;
    int16_t pd;
    int32_t dx;
    int32_t dy;
} BgAffineDstData;

void PortSoftReset(void);

static const uint32_t sZero[2];

/* BIOS reads from its own area return zero to the caller's view; writes to
   it are dropped. */
#define BIOS_AREA(p) ((uintptr_t)(p) < 0x10000)

void CpuSet(const void* src, void* dst, uint32_t ctrl) {
    uint32_t count = ctrl & 0x1FFFFF;
    int fixed = (ctrl >> 24) & 1;
    uint32_t i;

    if (BIOS_AREA(dst))
        return;
    if (BIOS_AREA(src)) {
        src = sZero;
        fixed = 1;
    }

    if (ctrl & (1u << 26)) {
        const uint32_t* s = (const uint32_t*)((uintptr_t)src & ~3u);
        uint32_t* d = (uint32_t*)((uintptr_t)dst & ~3u);
        if (fixed) {
            uint32_t v = *s;
            for (i = 0; i < count; i++)
                d[i] = v;
        } else {
            memmove(d, s, count * 4);
        }
    } else {
        const uint16_t* s = (const uint16_t*)((uintptr_t)src & ~1u);
        uint16_t* d = (uint16_t*)((uintptr_t)dst & ~1u);
        if (fixed) {
            uint16_t v = *s;
            for (i = 0; i < count; i++)
                d[i] = v;
        } else {
            memmove(d, s, count * 2);
        }
    }
}

void CpuFastSet(const void* src, void* dst, uint32_t ctrl) {
    uint32_t count = ((ctrl & 0x1FFFFF) + 7) & ~7u;
    const uint32_t* s = (const uint32_t*)((uintptr_t)src & ~3u);
    uint32_t* d = (uint32_t*)((uintptr_t)dst & ~3u);
    uint32_t i;

    if (BIOS_AREA(dst))
        return;
    if (BIOS_AREA(src)) {
        s = sZero;
        ctrl |= 1u << 24;
    }

    if (ctrl & (1u << 24)) {
        uint32_t v = *s;
        for (i = 0; i < count; i++)
            d[i] = v;
    } else {
        memmove(d, s, count * 4);
    }
}

static void Lz77(const void* src, void* dst, int vram) {
    const uint8_t* s = (const uint8_t*)src;
    uint8_t* d = (uint8_t*)dst;
    uint32_t size;
    uint32_t out = 0;

    if (BIOS_AREA(src) || BIOS_AREA(dst))
        return;
    size = (s[1] | (s[2] << 8) | (s[3] << 16));

    (void)vram;
    s += 4;
    while (out < size) {
        uint8_t flags = *s++;
        int bit;

        for (bit = 0; bit < 8 && out < size; bit++, flags <<= 1) {
            if (flags & 0x80) {
                uint32_t len = (s[0] >> 4) + 3;
                uint32_t disp = (((s[0] & 15) << 8) | s[1]) + 1;
                s += 2;
                while (len-- && out < size) {
                    d[out] = d[out - disp];
                    out++;
                }
            } else {
                d[out++] = *s++;
            }
        }
    }
}

void LZ77UnCompVram(const void* src, void* dst) {
    Lz77(src, dst, 1);
}

void LZ77UnCompWram(const void* src, void* dst) {
    Lz77(src, dst, 0);
}

void RegisterRamReset(uint32_t flags) {
    if (flags & 0x01)
        memset((void*)(uintptr_t)GBA_EWRAM, 0, GBA_EWRAM_SIZE);
    if (flags & 0x02)
        memset((void*)(uintptr_t)GBA_IWRAM, 0, GBA_IWRAM_SIZE - 0x200);
    if (flags & 0x04)
        memset((void*)(uintptr_t)GBA_PLTT, 0, GBA_PLTT_SIZE);
    if (flags & 0x08)
        memset((void*)(uintptr_t)GBA_VRAM, 0, GBA_VRAM_SIZE);
    if (flags & 0x10) {
        memset((void*)(uintptr_t)GBA_OAM, 0, GBA_OAM_SIZE);
        PortOamTagsReset();
    }
    if (flags & 0x40)
        memset((void*)(uintptr_t)(GBA_IO + 0x60), 0, 0x48);
    if (flags & 0x80) {
        uint16_t keep = IO16(0x130);
        memset((void*)(uintptr_t)GBA_IO, 0, 0x60);
        memset((void*)(uintptr_t)(GBA_IO + 0xB0), 0, 0x60);
        memset((void*)(uintptr_t)(GBA_IO + 0x200), 0, 0x10);
        IO16(0x130) = keep;
        IO16(0x20) = 0x100;
        IO16(0x26) = 0x100;
        IO16(0x30) = 0x100;
        IO16(0x36) = 0x100;
    }
    IO16(0x00) = 0x80;
}

uint32_t Sqrt(uint32_t value) {
    uint32_t r = (uint32_t)sqrt((double)value);

    while (r * r > value)
        r--;
    while ((r + 1) * (r + 1) <= value)
        r++;
    return r;
}

static int16_t sSine[256];

static void InitSine(void) {
    int i;

    if (sSine[64] != 0)
        return;
    for (i = 0; i < 256; i++)
        sSine[i] = (int16_t)lround(sin(i * 3.14159265358979323846 * 2.0 / 256.0) * 16384.0);
}

void BgAffineSet(BgAffineSrcData* src, BgAffineDstData* dst, int32_t count) {
    InitSine();
    for (; count > 0; count--, src++, dst++) {
        int theta = src->alpha >> 8;
        int32_t sn = sSine[theta];
        int32_t cs = sSine[(theta + 64) & 255];
        int32_t pa = (cs * src->sx) >> 14;
        int32_t pb = -(sn * src->sx) >> 14;
        int32_t pc = (sn * src->sy) >> 14;
        int32_t pd = (cs * src->sy) >> 14;

        dst->pa = (int16_t)pa;
        dst->pb = (int16_t)pb;
        dst->pc = (int16_t)pc;
        dst->pd = (int16_t)pd;
        dst->dx = src->texX - (pa * src->scrX + pb * src->scrY);
        dst->dy = src->texY - (pc * src->scrX + pd * src->scrY);
    }
}

void VBlankIntrWait(void) {
    extern void AgbMain(void);
    extern int gPortInMainLoop;
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);

    /* Save states may only be taken/restored at the main loop's wait. */
    gPortInMainLoop = caller > (uintptr_t)AgbMain && caller < (uintptr_t)AgbMain + 0x400;
    PortFrame();
    gPortInMainLoop = 0;
}

void SoftReset(int32_t flags) {
    (void)flags;
    PortSoftReset();
}
