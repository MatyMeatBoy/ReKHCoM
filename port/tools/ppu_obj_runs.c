/* Sprite rows of the production PPU over random memory: DrawObjs with KHCOM_PPU_OBJ_ROW_RUNS off and on, comparing
 * every row buffer it writes (test_ppu_obj_runs.py). */
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

static uint32_t seed = 27182;
static uint32_t Random(void) { seed = seed * 1664525u + 1013904223u; return seed >> 8; }

typedef struct {
    uint16_t obj[320], scr[320], index[320], scrIndex[320];
    uint8_t prio[320], semi[320], scrPrio[320], scrSemi[320], window[320];
    int any;
} Rows;

static void Keep(Rows* r) {
    memset(r, 0, sizeof(*r));
    memcpy(r->obj, sObjLine, 640); memcpy(r->scr, sScrLine, 640);
    memcpy(r->prio, sObjPrio, 320); memcpy(r->semi, sObjSemi, 320);
    memcpy(r->scrPrio, sScrPrio, 320); memcpy(r->window, sObjWin, 320);
    memcpy(r->scrSemi, sScrSemi, 320);
    memcpy(r->index, sObjIndex, 640); memcpy(r->scrIndex, sScrIndex, 640);
    r->any = sLineHasObjects;
}

int main(void) {
    if (!VirtualAlloc((void*)GBA_IO, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_PLTT, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_VRAM, 0x20000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) ||
        !VirtualAlloc((void*)GBA_OAM, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) return 3;
    PortCanvasAlloc();
    PortPpuSetResolution(320, 240);
    static Rows off, on;
    long long rows = 0, drawn = 0;
    for (int pass = 0; pass < 1500; pass++) {
        for (int i = 0; i < 0x18000 / 4; i += pass % 20 ? 89 : 1) {
            const uint32_t mask = Random() % 3 ? Random() | (Random() << 16) : 0;
            ((uint32_t*)(uintptr_t)GBA_VRAM)[i] = (Random() | (Random() << 16)) & mask;
        }
        for (int i = 0; i < 512; i++) ((uint16_t*)(uintptr_t)GBA_PLTT)[i] = (uint16_t)Random();
        gPortScreen.width = 320; gPortScreen.height = 240;
        gPortScreen.originX = (int)(Random() % 81) - 8; gPortScreen.originY = 40; gPortScreen.wide = pass % 6 != 0;
        sLeft = -gPortScreen.originX + (int)(Random() % 3 ? 0 : Random() % 40);
        sRight = 320 - gPortScreen.originX - (int)(Random() % 3 ? 0 : Random() % 40);
        sTop = -40; sBottom = 200;
        sHdText = pass % 2;
        sOverlay = pass % 3 == 0; sSpotlight = pass % 9 == 0; sScreenBgMask = pass % 18 ? 0 : 2;
        sHudLayer = pass % 7 == 0;
        uint16_t* oam = (uint16_t*)(uintptr_t)GBA_OAM;
        for (int i = 0; i < 128; i++) {
            uint16_t a0 = (uint16_t)Random(), a1 = (uint16_t)Random();
            if (Random() % 4) a0 &= (uint16_t)~0x1F00; /* mostly plain sprites */
            oam[i * 4] = (uint16_t)((a0 & 0xFF00) | ((int)(Random() % 220) - 30 & 0xFF));
            oam[i * 4 + 1] = (uint16_t)((a1 & 0xFE00) | (((int)(Random() % 400) - 80) & 0x1FF));
            oam[i * 4 + 2] = (uint16_t)Random();
            if (!(i & 3)) oam[i * 4 + 3] = (uint16_t)((int)(Random() % 513) - 256);
            memset(&sOamTag[i], 0, sizeof(sOamTag[0]));
            sOamExt[i].x = oam[i * 4 + 1] & 0x1FF; sOamExt[i].y = oam[i * 4] & 0xFF;
            if (Random() % 3 == 0) { sOamExt[i].x += 600; }
            if (Random() % 5 == 0) { sOamTag[i].clipX0 = (int)(Random() % 200) - 20; sOamTag[i].clipX1 = sOamTag[i].clipX0 + (int)(Random() % 90) - 10; }
            sOamTag[i].screen = Random() % 4 == 0;
            sOamTag[i].hud = Random() % 11 == 0;
            sOamTag[i].header = Random() % 13 == 0;
            if (Random() % 5 == 0) { sOamTag[i].text = 0x41; sOamTag[i].style = Random() % 3; }
            sMainHdPart[i] = 0;
        }
        *(uint16_t*)(uintptr_t)GBA_IO = (uint16_t)(0x1000 | (Random() & 0x40));
        *(uint16_t*)(uintptr_t)(GBA_IO + 0x4C) = (uint16_t)(Random() % 3 ? 0 : Random());
        for (int gy = -40; gy < 200; gy += 1 + (int)(Random() % 3)) {
            sObjRowRuns = 0;
            DrawObjs(gy, IO16(0));
            Keep(&off);
            sObjRowRuns = 1;
            DrawObjs(gy, IO16(0));
            Keep(&on);
            if (memcmp(&off, &on, sizeof(off))) {
                printf("FAIL pass %d row %d\n", pass, gy);
                return 1;
            }
            drawn += off.any;
            rows++;
        }
    }
    printf("%lld %lld\n", rows, drawn);
    return 0;
}
