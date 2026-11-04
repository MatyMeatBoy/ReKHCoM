/* Complete production PPU, synthetic cartridge memory; no game/ROM/GPU. */
#define main fixture_main_unused
#include "ppu_conformance.c"
#undef main
/* The stubs for the production PPU's optional-art/battle-backdrop links live in ppu_conformance.c, shared by every PPU fixture. */
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); return 77; } } while (0)
#ifndef HUD_CANVAS_NO_MAIN
int main(void) {
    int i;
    uint32_t* fallback;
    size_t bytes;
    CHECK(VirtualAlloc((void*)GBA_IO,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_PLTT,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_VRAM,0x20000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_OAM,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    PortCanvasAlloc();
    for(i=0;i<512;i++) ((uint16_t*)(uintptr_t)GBA_PLTT)[i]=color(i%256);
    for(i=0;i<128;i++) ((uint16_t*)(uintptr_t)GBA_OAM)[i*4]=0x200;
    PortPpuSetResolution(240,160);
    semantic("semantic_hd_hud");
    gPortConfig.hdHud=1;gPortConfig.hdCards=1;
    sFixtureCanvasReady=0;
    PortPpuRenderFrame();
    CHECK(sHudCanvasFallback&&!PortPpuHudActive());
    CHECK(!HdTextGlyph(1));
    bytes=(size_t)gPortScreen.width*gPortScreen.height*4;
    fallback=malloc(bytes);CHECK(fallback);memcpy(fallback,gPortFrame,bytes);
    /* Independent composition control: explicitly render all cartridge ink
       without extracting HUD or substituting glyphs/art. Preferences of the
       failure case were untouched; only the comparison control disables HD. */
    gPortHudScale=0;gPortConfig.hdHud=0;gPortConfig.hdCards=0;gPortConfig.hdText=0;
    PortPpuRenderFrame();
    CHECK(!memcmp(fallback,gPortFrame,bytes));
    gPortHudScale=1;gPortConfig.hdHud=1;gPortConfig.hdCards=1;gPortConfig.hdText=1;
    sFixtureCanvasReady=1;PortPpuRenderFrame();
    CHECK(!sHudCanvasFallback&&PortPpuHudActive()&&HdTextGlyph(1));
    CHECK(memcmp(fallback,gPortFrame,bytes)); /* HUD once again separate */
    /* Exact review repro: the paused world641x360 stays fixed while output
       1281->1282 changes desired HUD427x240->428x240. No extra PPU frame. */
    PortPpuSetResolution(641,360);
    gPortHudW=427;gPortHudH=240;gPortHudScale=3;PortPpuRenderFrame();
    CHECK(PortPpuHudActive());
    size_t capacity=427u*240u;
    uint32_t* guarded=malloc((capacity+240)*sizeof(*guarded));CHECK(guarded);
    for(size_t k=0;k<capacity+240;k++)guarded[k]=0xcdcdcdcd;
    int saved[4];gPortHudW=428;gPortWorldScale++;int desiredWorldScale=gPortWorldScale;
    PortPpuHudFrameBegin(saved);PortPpuRenderHud(guarded);PortPpuHudFrameEnd(saved);
    CHECK(gPortHudW==428&&gPortHudH==240&&gPortHudScale==3&&gPortWorldScale==desiredWorldScale);
    for(size_t k=capacity;k<capacity+240;k++)CHECK(guarded[k]==0xcdcdcdcd);
    free(guarded);
    free(fallback);puts("PASS full production PPU: failed canvas pixels equal cartridge control; recovery restores separate HUD");return 0;
}
#endif
