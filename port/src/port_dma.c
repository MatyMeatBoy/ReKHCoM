/*
 * DMA controller.
 *
 * Immediate transfers run on the spot. VBlank and HBlank transfers stay armed
 * in the registers and run when the PPU reaches those events. Sound FIFO
 * transfers ("special" timing) are never executed: the audio layer reads the
 * mixer's buffers directly.
 */
#include <string.h>

#include "port/port.h"
#include "port/port_state.h"

#define DMA_DEST_DEC 0x00200000u
#define DMA_DEST_FIXED 0x00400000u
#define DMA_DEST_RELOAD 0x00600000u
#define DMA_SRC_DEC 0x00800000u
#define DMA_SRC_FIXED 0x01000000u
#define DMA_REPEAT 0x02000000u
#define DMA_32BIT 0x04000000u
#define DMA_TIMING_MASK 0x30000000u
#define DMA_TIMING_VBLANK 0x10000000u
#define DMA_TIMING_HBLANK 0x20000000u
#define DMA_ENABLE 0x80000000u

typedef struct DmaChannel {
    uint32_t src;
    uint32_t dst;
    uint32_t regSad;      /* register values the internal state was latched from */
    uint32_t regDad;
    uint32_t regCnt;
    int armed;
} DmaChannel;

static DmaChannel sDma[4];

static uint32_t RegBase(int ch) {
    return 0xB0 + ch * 12;
}

static void Transfer(int ch, uint32_t cnt) {
    DmaChannel* d = &sDma[ch];
    uint32_t count = cnt & 0xFFFF;
    int unit = (cnt & DMA_32BIT) ? 4 : 2;
    int srcStep, dstStep;
    uint32_t i;

    if (count == 0)
        count = (ch == 3) ? 0x10000 : 0x4000;

    switch (cnt & DMA_SRC_FIXED ? 2 : (cnt & DMA_SRC_DEC ? 1 : 0)) {
        case 1:
            srcStep = -unit;
            break;
        case 2:
            srcStep = 0;
            break;
        default:
            srcStep = unit;
            break;
    }
    switch (cnt & DMA_DEST_RELOAD) {
        case DMA_DEST_DEC:
            dstStep = -unit;
            break;
        case DMA_DEST_FIXED:
            dstStep = 0;
            break;
        default:
            dstStep = unit;
            break;
    }

    /* The BIOS area cannot be read by DMA (open bus) nor written. On a PC
       the low 64 KiB are never mapped, so treat it explicitly. */
    if (d->dst < 0x10000) {
        d->src += srcStep * (int32_t)count;
        d->dst += dstStep * (int32_t)count;
        return;
    }
    if (d->src < 0x10000) {
        for (i = 0; i < count; i++) {
            if (unit == 4)
                *(volatile uint32_t*)(uintptr_t)d->dst = 0;
            else
                *(volatile uint16_t*)(uintptr_t)d->dst = 0;
            d->src += srcStep;
            d->dst += dstStep;
        }
        return;
    }

    if (srcStep == unit && dstStep == unit) {
        memmove((void*)(uintptr_t)d->dst, (const void*)(uintptr_t)d->src, count * unit);
        d->src += count * unit;
        d->dst += count * unit;
    } else if (unit == 4) {
        for (i = 0; i < count; i++) {
            *(volatile uint32_t*)(uintptr_t)d->dst = *(const volatile uint32_t*)(uintptr_t)d->src;
            d->src += srcStep;
            d->dst += dstStep;
        }
    } else {
        for (i = 0; i < count; i++) {
            *(volatile uint16_t*)(uintptr_t)d->dst = *(const volatile uint16_t*)(uintptr_t)d->src;
            d->src += srcStep;
            d->dst += dstStep;
        }
    }
    if ((cnt & DMA_DEST_RELOAD) == DMA_DEST_RELOAD)
        d->dst = IO32(RegBase(ch) + 4);
}

static void Latch(int ch) {
    DmaChannel* d = &sDma[ch];
    uint32_t base = RegBase(ch);

    d->regSad = IO32(base);
    d->regDad = IO32(base + 4);
    d->regCnt = IO32(base + 8);
    d->src = d->regSad;
    d->dst = d->regDad;
    d->armed = 1;
}

void PortDmaSet(uint32_t ch, const void* src, void* dst, uint32_t cnt) {
    uint32_t base = RegBase(ch & 3);

    ch &= 3;
    IO32(base) = (uint32_t)(uintptr_t)src;
    IO32(base + 4) = (uint32_t)(uintptr_t)dst;
    IO32(base + 8) = cnt;
    sDma[ch].armed = 0;
    if (!(cnt & DMA_ENABLE))
        return;

    Latch(ch);
    if ((cnt & DMA_TIMING_MASK) == 0) {
        Transfer(ch, cnt);
        IO32(base + 8) = cnt & ~DMA_ENABLE;
        sDma[ch].armed = 0;
    }
}

static int RunTimed(uint32_t timing) {
    int transferred = 0;
    int ch;

    for (ch = 0; ch < 4; ch++) {
        uint32_t base = RegBase(ch);
        uint32_t cnt = IO32(base + 8);
        DmaChannel* d = &sDma[ch];

        if (!(cnt & DMA_ENABLE) || (cnt & DMA_TIMING_MASK) != timing) {
            d->armed = 0;
            continue;
        }
        /* Registers written directly (not through DmaSet) re-arm the channel. */
        if (!d->armed || d->regSad != IO32(base) || d->regDad != IO32(base + 4) || d->regCnt != cnt)
            Latch(ch);
        Transfer(ch, cnt);
        transferred = 1;
        if (!(cnt & DMA_REPEAT)) {
            IO32(base + 8) = cnt & ~DMA_ENABLE;
            d->armed = 0;
        }
    }
    return transferred;
}

void PortDmaVisitState(PortStateVisitor visit) {
    visit(sDma, sizeof(sDma));
}

int PortDmaHBlank(void) {
    return RunTimed(DMA_TIMING_HBLANK);
}

void PortDmaVBlank(void) {
    RunTimed(DMA_TIMING_VBLANK);
}
