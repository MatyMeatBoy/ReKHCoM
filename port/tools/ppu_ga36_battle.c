/* Differential fixture for the GA36 battle/field vector routes: the whole
 * production renderer (through ppu_conformance.c) draws random wide 320x240
 * frames with the host NEON stand-in, and prints one hash of every output per
 * frame. test_ppu_ga36_battle.py compares the runs with the flags off and on. */
#include <arm_neon.h> /* SIMDe on the host */
#define __ARM_NEON 1
#include <stdlib.h>
/* the flags come from the environment; PORT_ENV_ONCE reads them once */
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

static int RandomTile(int bg, int tx, int ty) {
    const uint32_t v = (uint32_t)(tx * 73856093) ^ (uint32_t)(ty * 19349663) ^ (uint32_t)(bg * 83492791);
    return v % 11 ? (int)((v >> 4) & 0xFFFF) : -1;
}

static uint64_t Hash(uint64_t h, const void* data, size_t size) {
    const uint8_t* p = data;
    while (size--) h = (h ^ *p++) * 1099511628211u;
    return h;
}

static void Fill(void* address, size_t size) {
    uint16_t* p = address;
    for (size_t i = 0; i < size / 2; i++) p[i] = (uint16_t)Random();
}

int main(void) {
    if (!VirtualAlloc((void*)GBA_IO, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_PLTT, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_VRAM, 0x20000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_OAM, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
        return 3;
    const bool on = getenv("GA36_FAST") != NULL;
    sNeonOpaqueCompose = sNeonNarrowCompose = on;
    sNeonEffectCompose = on;
    gPortConfig.hdText = 1;
    PortCanvasAlloc();
    BuildLut();
    for (int frame = 0; frame < 600; frame++) {
        uint8_t* vram = (uint8_t*)(uintptr_t)GBA_VRAM;
        uint16_t* oam = (uint16_t*)(uintptr_t)GBA_OAM;
        Fill((void*)(uintptr_t)GBA_PLTT, 1024);
        Fill(vram, 0x18000);
        /* long runs of palette index 0, so layers and sprites have holes */
        for (int i = 0; i < 0x18000 / 8; i++)
            if (!Pick(2)) memset(vram + i * 8, 0, Pick(3) ? 4 : 8);
        for (int i = 0; i < 128; i++) {
            const int affine = !Pick(6), mode = Pick(4) ? 0 : Pick(3);
            oam[i * 4] = (uint16_t)(Pick(256) | affine << 8 | (affine ? Pick(2) << 9 : !Pick(8) << 9) |
                                    mode << 10 | !Pick(12) << 12 | !Pick(5) << 13 | Pick(3) << 14);
            oam[i * 4 + 1] = (uint16_t)((Pick(3) ? (Pick(360) - 60) & 0x1FF : Pick(512)) | (Pick(32) << 9) | Pick(4) << 14);
            oam[i * 4 + 2] = (uint16_t)Random();
            oam[i * 4 + 3] = (uint16_t)(Pick(1024) - 512);
        }
        const int mode = frame % 3 == 2 ? 1 : 0;
        const int windows = !Pick(8), blank = 0;
        for (int offset = 0; offset < 0x58; offset += 2) IO16(offset) = 0;
        IO16(0x00) = (uint16_t)(mode | blank | 0x40 * Pick(2) | (Pick(16) | (Pick(3) ? 1 : 0)) << 8 | (Pick(5) ? 0x1000 : 0) |
                                (windows ? Pick(8) << 13 : 0));
        for (int b = 0; b < 4; b++) {
            IO16(0x08 + b * 2) = (uint16_t)(Pick(4) | Pick(3) << 2 | !Pick(10) << 6 | !Pick(6) << 7 | Pick(32) << 8 |
                                            Pick(2) << 13 | Pick(4) << 14);
            IO16(0x10 + b * 4) = (uint16_t)Pick(512);
            IO16(0x12 + b * 4) = (uint16_t)Pick(512);
        }
        IO16(0x20) = IO16(0x30) = 256; IO16(0x26) = IO16(0x36) = 256;
        for (int offset = 0x40; offset < 0x4C; offset += 2) IO16(offset) = (uint16_t)Random();
        IO16(0x4C) = (uint16_t)Random();
        IO16(0x50) = (uint16_t)(Pick(4) ? Pick(0x4000) : 0);
        IO16(0x52) = (uint16_t)Random();
        IO16(0x54) = (uint16_t)Random();
        PortPpuSetResolution(320, 240);
        PortPpuBeginFrame();
        gPortScreen.wide = frame % 5 != 4;
        sBattleView = Pick(2);
        for (int b = 0; b < 4; b++) {
            sBgWorldLatched[b].valid = gPortScreen.wide && Pick(3) != 0;
            sBgWorldLatched[b].fn = RandomTile;
            sBgWorldLatched[b].x = Pick(64) - 32;
            sBgWorldLatched[b].y = Pick(64) - 32;
            sBgWorldLatched[b].hofs = IO16(0x10 + b * 4);
            sBgWorldLatched[b].vofs = IO16(0x12 + b * 4);
            sWorldPadding[b] = Pick(3) ? TRANSPARENT : (uint16_t)Pick(0x8000);
            sBgDockedLatched[b] = !Pick(8);
        }
        sHudLayer = !Pick(4);
        sLeft = gPortScreen.wide ? -gPortScreen.originX : 0;
        sRight = gPortScreen.wide ? gPortScreen.width - gPortScreen.originX : GBA_SCREEN_W;
        sTop = -gPortScreen.originY;
        sBottom = gPortScreen.height - gPortScreen.originY;
        LatchAffine(1);
        for (int y = 0; y < gPortScreen.height; y++)
            RenderLine(y, y - gPortScreen.originY);
        const size_t pixels = (size_t)gPortScreen.width * gPortScreen.height;
        uint64_t h = 14695981039346656037u;
        h = Hash(h, gPortFrame, pixels * 4);
        if (!gPortScreen.wide) h = Hash(h, gPortMenuBackground, pixels * 4);
        h = Hash(h, sOccBg, pixels);
        h = Hash(h, sOccBgLayer, pixels);
        h = Hash(h, sOccObj, pixels * 2);
        h = Hash(h, sMsgMask, pixels);
        printf("%d wide %d %016llx\n", frame, gPortScreen.wide, (unsigned long long)h);
    }
    fprintf(stderr, "routes: effect %d\n", sNeonEffectUsed);
    return 0;
}
