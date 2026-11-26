/* Differential fixture for KHCOM_PPU_ARENA_FAST: DrawAffineBg draws random battle-arena rows (wide margins,
 * reflected ornament bands, extended floor, wrap and no wrap, extreme and random matrices) with the reference
 * per-pixel route and with the fast row route, and every output pixel and world flag must match. */
#include <stdlib.h>
#define PPU_FIXTURE_OWN_STUBS 1
#define main ConformanceMain
#include "ppu_conformance.c"
#undef main

const char* SDL_getenv(const char* name) { return getenv(name); }
uint64_t SDL_GetPerformanceCounter(void) { return 0; }
uint64_t SDL_GetPerformanceFrequency(void) { return 1; }
int SDL_atoi(const char* text) { return atoi(text); }
int PortFixed480p(void) { return 0; }
void PortDarksidePrepareBodyRow(PortDarksideBodyRow* row, const void* const* maps, int y, int cx, int cy) {}
int PortDarksideBodyRowPixel(const PortDarksideBodyRow* row, int x, uint16_t* color) { return 0; }

Mode gModeMapinspect, gModeWorldinspect;

static uint32_t Random(void) {
    static uint32_t s = 20261125u;
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return s;
}
static int Pick(int n) { return (int)(Random() % (uint32_t)n); }

static int Extreme(void) {
    static const int values[] = { 0, 1, -1, 256, -256, 255, 257, 128, -128, 512, -512, 32767, -32768, 64, 3 };
    return Pick(3) ? values[Pick((int)(sizeof(values) / sizeof(values[0])))] : (int16_t)Random();
}

int main(void) {
    static uint16_t refLine[PORT_MAX_W];
    static uint8_t refWorld[PORT_MAX_W];
    static const int widths[][2] = { { 320, 240 }, { 640, 480 }, { 426, 240 }, { 240, 160 }, { 854, 480 } };
    long rows = 0, banded = 0, floor = 0, fastRows = 0;
    if (!VirtualAlloc((void*)GBA_IO, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_PLTT, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_VRAM, 0x20000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_OAM, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
        return 3;
    PortCanvasAlloc();
    for (int frame = 0; frame < 400; frame++) {
        uint8_t* vram = (uint8_t*)(uintptr_t)GBA_VRAM;
        uint16_t* pltt = (uint16_t*)(uintptr_t)GBA_PLTT;
        for (int i = 0; i < 0x18000; i++) vram[i] = Pick(4) ? (uint8_t)Random() : 0;
        for (int i = 0; i < 512; i++) pltt[i] = (uint16_t)Random();
        const int* wh = widths[Pick(5)];
        PortPpuSetResolution(wh[0], wh[1]);
        gPortScreen.wide = Pick(6) != 0;
        sBattleView = Pick(5) != 0;
        sHalloweenFloor = !Pick(10);
        sLeft = gPortScreen.wide ? -gPortScreen.originX : 0;
        sRight = gPortScreen.wide ? gPortScreen.width - gPortScreen.originX : GBA_SCREEN_W;
        if (!Pick(8)) { sLeft += Pick(40); sRight -= Pick(40); }
        const int sizeBits = Pick(4) ? 2 + Pick(2) : Pick(2);
        const int size = 128 << sizeBits;
        if (Pick(5)) {
            const int left = Pick(4) ? Pick(64) : Pick(400), right = left + 128 + Pick(1024 - left - 127);
            const int lw = 1 + Pick(PPU_MIN(96, right - left)), rw = 1 + Pick(PPU_MIN(96, right - left));
            PortSetBattleArenaBands(left, right, left + Pick(right - left - lw + 1), lw,
                                    left + Pick(right - left - rw + 1), rw);
        } else {
            memset(sArenaBands, 0, sizeof(sArenaBands));
        }
        const int arena = Pick(5) != 0;
        const uint16_t cnt = (uint16_t)((arena ? 0 : Pick(4)) << 2 | (arena ? 12 : Pick(32)) << 8 |
                                        Pick(2) << 13 | sizeBits << 14 | (!Pick(12)) << 6 | 0x80);
        const int bg = 2 + Pick(2);
        const int a = bg - 2;
        IO16(0x08 + bg * 2) = cnt;
        IO16(0x4C) = (uint16_t)Random();
        const int straight = !Pick(6);
        IO16(0x20 + a * 0x10) = (uint16_t)(straight ? 256 : Extreme());
        IO16(0x24 + a * 0x10) = (uint16_t)(straight ? 0 : Extreme());
        const int32_t baseX = Pick(3) ? (Pick(size + 600) - 300) << 8 : (int32_t)Random() >> 4;
        const int32_t baseY = Pick(3) ? (Pick(size + 200) - 100) << 8 : (int32_t)Random() >> 4;
        const int dy = Pick(3) ? Pick(512) - 256 : Extreme();
        for (int gy = -gPortScreen.originY; gy < gPortScreen.height - gPortScreen.originY; gy += 1 + Pick(4)) {
            sAffX[a] = baseX + dy * gy / 4;
            sAffY[a] = baseY + 256 * gy + Pick(2);
            sArenaFast = 0;
            sMosNextGy[a] = 0x7FFFFFFF;
            memset(sInWorld, 0, sizeof(sInWorld));
            for (int x = 0; x < PORT_MAX_W; x++) sBgLine[bg][x] = 0x1234;
            DrawAffineBg(bg, gy, 0);
            memcpy(refLine, sBgLine[bg], sizeof(refLine));
            memcpy(refWorld, sInWorld, sizeof(refWorld));
            sArenaFast = 1;
            sMosNextGy[a] = 0x7FFFFFFF;
            memset(sInWorld, 0, sizeof(sInWorld));
            for (int x = 0; x < PORT_MAX_W; x++) sBgLine[bg][x] = 0x1234;
            DrawAffineBg(bg, gy, 0);
            for (int x = 0; x < gPortScreen.width; x++)
                if (refLine[x] != sBgLine[bg][x] || refWorld[x] != sInWorld[x]) {
                    printf("mismatch frame %d gy %d x %d: %04x/%d %04x/%d cnt %04x pa %d pc %d bands %d %d\n", frame,
                           gy, x, refLine[x], refWorld[x], sBgLine[bg][x], sInWorld[x], cnt,
                           (int16_t)IO16(0x20 + a * 0x10), (int16_t)IO16(0x24 + a * 0x10), sArenaBands[0],
                           sArenaBands[1]);
                    return 1;
                }
            rows++;
            banded += arena && sBattleView && sArenaBands[1] && size >= 512;
            floor += arena && sBattleView && gPortScreen.wide && gy >= GBA_SCREEN_H && size >= 512;
            fastRows += sArenaLutValid;
        }
    }
    printf("rows %ld banded %ld floor %ld lut %ld\n", rows, banded, floor, fastRows);
    return 0;
}
