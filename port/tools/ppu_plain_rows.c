/* Text layer rows of the production PPU over random memory: DrawTextBg with KHCOM_PPU_BG_PLAIN_ROWS against the
 * general sampler, comparing the layer line and the world marks (test_ppu_plain_rows.py). */
#include <stdlib.h>
/* The PPU is built without SDL here. */
#define SDL_getenv getenv
#define SDL_atoi atoi
#define SDL_clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))
#define SDL_GetPerformanceCounter() 0
#define SDL_GetPerformanceFrequency() 1
#define main PpuConformanceMain
#include "ppu_conformance.c"
#undef main
int PortFixed480p(void) { return 1; }
void PortDarksidePrepareBodyRow(PortDarksideBodyRow* row, const void* const* maps, int y, int cx, int cy) {}
int PortDarksideBodyRowPixel(const PortDarksideBodyRow* row, int x, uint16_t* color) { return 0; }

static uint32_t seed = 31415;
static uint32_t Random(void) { seed = seed * 1664525u + 1013904223u; return seed >> 8; }

static int WorldFn(int bg, int tx, int ty) {
    const unsigned h = (unsigned)(tx * 2654435761u) ^ (unsigned)(ty * 40503u) ^ (unsigned)bg;
    return (h >> 5) % 7 == 0 ? -1 : (int)((h >> 9) & 0xFFFF);
}

int main(void) {
    if (!VirtualAlloc((void*)GBA_IO, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_PLTT, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_VRAM, 0x20000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_OAM, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) return 3;
    PortCanvasAlloc();
    PortPpuSetResolution(320, 240);
    static uint16_t blocks[4][1024];
    static const void* map[4];
    static uint16_t line[PORT_MAX_W];
    static uint8_t marks[PORT_MAX_W];
    long long rows = 0, plain = 0, text = 0;
    for (int pass = 0; pass < 4000; pass++) {
        for (int i = 0; i < 0x18000 / 4; i += pass % 50 ? 97 : 1) ((uint32_t*)(uintptr_t)GBA_VRAM)[i] = Random() & (Random() | Random());
        for (int i = 0; i < 512; i++) ((uint16_t*)(uintptr_t)GBA_PLTT)[i] = (uint16_t)Random();
        for (int b = 0; b < 4; b++)
            for (int i = 0; i < 1024; i++) blocks[b][i] = (uint16_t)Random();
        gPortScreen.width = 320; gPortScreen.height = 240;
        gPortScreen.originX = (int)(Random() % 49); gPortScreen.originY = 40; gPortScreen.wide = pass % 6 != 0;
        sLeft = -gPortScreen.originX + (int)(Random() % 3 ? 0 : Random() % 40);
        sRight = 320 - gPortScreen.originX - (int)(Random() % 3 ? 0 : Random() % 40);
        sHdText = pass % 2;
        memset(sBgTextTiles, 0, sizeof(sBgTextTiles));
        if (pass % 4 == 1) /* registrations: some match their tile, some are stale */
            for (int i = 0; i < 40; i++) {
                const unsigned n = Random() % 2048;
                sBgTextTiles[n].code = 0x41;
                if (i & 1) memcpy(sBgTextTiles[n].source, (const void*)(VRAM8 + n * 32), 32);
            }
        const int bg = pass % 4;
        const uint16_t cnt = (uint16_t)((Random() & 0xC000) | ((Random() % 4) << 2) | ((Random() % 32) << 8) | bg % 4);
        *(uint16_t*)(uintptr_t)(GBA_IO + 8 + bg * 2) = cnt;
        *(uint16_t*)(uintptr_t)(GBA_IO + 0x10 + bg * 4) = (uint16_t)Random();
        *(uint16_t*)(uintptr_t)(GBA_IO + 0x12 + bg * 4) = (uint16_t)Random();
        sBgShiftLatched[bg] = pass % 5 ? 0 : (int)(Random() % 61) - 30;
        sScreenBg[bg] = pass % 17 == 0;
        sBgDockedLatched[bg] = pass % 19 == 0;
        sWorldPadding[bg] = pass % 3 ? TRANSPARENT : (uint16_t)(Random() & 0x7FFF);
        memset(&sBgLatch[bg], 0, sizeof(sBgLatch[bg]));
        memset(&sBgWorldLatched[bg], 0, sizeof(sBgWorldLatched[bg]));
        if (pass % 3 == 0) {
            sBgWorldLatched[bg].fn = WorldFn; sBgWorldLatched[bg].valid = 1;
            sBgWorldLatched[bg].x = (int)(Random() % 1200) - 300; sBgWorldLatched[bg].y = (int)(Random() % 900) - 200;
            sBgWorldLatched[bg].hofs = Random() & 511; sBgWorldLatched[bg].vofs = Random() & 511;
        } else if (pass % 3 == 1) {
            for (int b = 0; b < 4; b++) map[b] = blocks[b];
            sBgLatch[bg].valid = 1; sBgLatch[bg].map = (void*)map; sBgLatch[bg].width = 2; sBgLatch[bg].height = 2;
            sBgLatch[bg].x = (int)(Random() % 700) - 200; sBgLatch[bg].y = (int)(Random() % 600) - 150;
            sBgLatch[bg].hofs = Random() & 511; sBgLatch[bg].vofs = Random() & 511;
        }
        for (int gy = -40; gy < 200; gy += 1 + (int)(Random() % 3)) {
            const int start = sBgTextCount = 0;
            sBgPlainRows = 0;
            memset(sInWorld, 0, sizeof(sInWorld));
            memset(sBgLine[bg], 0x11, sizeof(sBgLine[bg]));
            DrawTextBg(bg, gy, IO16(0), NULL);
            memcpy(line, sBgLine[bg], sizeof(line));
            memcpy(marks, sInWorld, sizeof(marks));
            const int glyphs = sBgTextCount - start;
            sBgPlainRows = 1;
            sBgTextCount = 0;
            memset(sInWorld, 0, sizeof(sInWorld));
            memset(sBgLine[bg], 0x22, sizeof(sBgLine[bg]));
            const int done = DrawTextBgPlain(bg, gy, cnt, IO16(0x10 + bg * 4) & 0x1FF, IO16(0x12 + bg * 4) & 0x1FF);
            plain += done;
            memset(sInWorld, 0, sizeof(sInWorld));
            DrawTextBg(bg, gy, IO16(0), NULL);
            if (memcmp(line, sBgLine[bg], 320 * 2) || memcmp(marks, sInWorld, 320) || sBgTextCount != glyphs) {
                printf("FAIL pass %d row %d bg %d\n", pass, gy, bg);
                return 1;
            }
            text += !done;
            rows++;
        }
    }
    printf("%lld %lld %lld\n", rows, plain, text);
    return 0;
}
