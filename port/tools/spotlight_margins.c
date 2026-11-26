/* Production PPU frame of a synthesis-like spotlight over random memory.
 * test_spotlight_margins.py compares every presenter-visible output with
 * KHCOM_PPU_SPOTLIGHT_SKIP_MARGINS off and on. */
#include <stdlib.h>
/* The PPU is built without SDL here. */
#define SDL_getenv getenv
#define SDL_atoi atoi
#define SDL_clamp(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))
#define SDL_GetPerformanceCounter() 0
#define SDL_GetPerformanceFrequency() 1
#define PPU_FIXTURE_OWN_STUBS
#define main PpuConformanceMain
#include "ppu_conformance.c"
#undef main
int PortFixed480p(void) { return 1; }
void PortDarksidePrepareBodyRow(PortDarksideBodyRow* row, const void* const* maps, int y, int cx, int cy) {}
int PortDarksideBodyRowPixel(const PortDarksideBodyRow* row, int x, uint16_t* color) { return 0; }

static uint32_t seed = 90210;
static uint32_t Random(void) { seed = seed * 1664525u + 1013904223u; return seed >> 8; }

/* Glyphs placed above or below the picture: only margin rows emit those. */
static unsigned MarginGlyphs(void) {
    unsigned found = 0;
    for (int i = 0; i < sBgTextCount; i++) {
        const int y = sBgTextGlyphs[i].glyph.y;
        found += sBgTextGlyphs[i].screen ? y < -7 || y >= GBA_SCREEN_H : y < gPortScreen.originY - 7 || y >= gPortScreen.originY + GBA_SCREEN_H;
    }
    return found;
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    if (!VirtualAlloc((void*)GBA_IO, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_PLTT, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_VRAM, 0x20000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_OAM, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) return 3;
    PortCanvasAlloc();
    FILE* f = fopen(argv[1], "wb");
    if (!f) return 4;
    PortPpuSetResolution(320, 240);
    gPortOverlayW = 240;
    unsigned glyphTotal = 0, skipped = 0, marginGlyphs = 0;
    for (int frame = 0; frame < 96; frame++) {
        uint8_t* v = (uint8_t*)(uintptr_t)GBA_VRAM;
        uint16_t* pal = (uint16_t*)(uintptr_t)GBA_PLTT;
        uint16_t* oam = (uint16_t*)(uintptr_t)GBA_OAM;
        for (int i = 0; i < 0x18000; i++) v[i] = (uint8_t)Random();
        for (int i = 0; i < 512; i++) pal[i] = Random() & 0x7FFF;
        for (int i = 0; i < 512; i++) oam[i] = (uint16_t)Random();
        for (int b = 0; b < 4; b++) {
            IO16(8 + b * 2) = (uint16_t)(b | ((24 + b) << 8) | ((Random() & 3) << 14));
            IO16(0x10 + b * 4) = Random() & 511;
            IO16(0x12 + b * 4) = Random() & 511;
        }
        /* Registered text on tiles the maps really use, so margin rows find glyphs. */
        for (int t = 0; t < 512; t += 61) PortRegisterBgTextTile(v + t * 32, '0' + t % 10, 2 | (2 << 8) | (3 << 12), v + t * 32);
        for (int b = 0; b < 4; b++) {
            uint16_t* map = (uint16_t*)(v + (24 + b) * 0x800);
            for (int i = 0; i < 4096; i++) map[i] = (uint16_t)((Random() % 600) | (Random() & 0xF000));
        }
        IO16(0) = (uint16_t)(0x1F40 | (frame % 17 == 16 ? 0x80 : 0));
        IO16(0x50) = frame & 1 ? 0x0e41 : (uint16_t)(Random() & 0x3FFF);
        IO16(0x52) = 0x0808; IO16(0x54) = Random() & 31; IO16(0x4C) = 0;
        gPortConfig.hdText = frame % 3 != 0;
        PortSetSpotlight(1);
        for (int b = 0; b < 4; b++) PortScreenBg(b, b == 1 || (frame % 4 == b));
        if (frame % 11 == 10) PortScreenBg(1, 0); /* the world is visible: full rows */
        gPortScreenLayer = 1;
        sFixtureWideField = frame % 13 != 12; /* the field view has world rows around the picture */
        PortPpuRenderFrame();
        skipped += gPortScreen.wide && SpotlightMarginRow(-1, IO16(0));
        marginGlyphs += MarginGlyphs();
        const size_t n = (size_t)gPortScreen.width * gPortScreen.height;
        fwrite(&gPortScreen.wide, sizeof(gPortScreen.wide), 1, f);
        fwrite(gPortFrame, 4, n, f);
        fwrite(gPortMenuBackground, 4, n, f);
        fwrite(gPortOverlay, 4, (size_t)gPortOverlayW * GBA_SCREEN_H, f);
        fwrite(sOccBg, 1, n, f); fwrite(sOccBgLayer, 1, n, f); fwrite(sOccObj, 2, n, f); fwrite(sMsgMask, 1, n, f);
        fwrite(&sBgTextCount, sizeof(sBgTextCount), 1, f);
        for (int i = 0; i < sBgTextCount; i++) {
            const BgTextGlyph* g = &sBgTextGlyphs[i];
            fwrite(&g->glyph, sizeof(g->glyph), 1, f);
            fwrite(g->ink, sizeof(g->ink), 1, f);
            fputc(g->bg, f); fputc(g->priority, f); fputc(g->screen, f);
        }
        glyphTotal += (unsigned)sBgTextCount;
    }
    fclose(f);
    printf("%u %u %u\n", skipped, glyphTotal, marginGlyphs);
    return 0;
}
