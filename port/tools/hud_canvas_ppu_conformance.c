/* Complete production PPU, synthetic cartridge memory; no game/ROM/GPU. */
#define main fixture_main_unused
#include "ppu_conformance.c"
#undef main
/* Optional artwork and battle-specific backgrounds are absent in this fixture. */
int PortHdSpriteFind(const void*a,const void*b){return 0;}
void PortMarluxiaField(int on){}
int PortEventBackdropProfile(void){return -1;}
int PortEventId(void){return -1;}
int PortDarksideLayer(const void*const*m){return 0;}
int PortJafarWide(void){return 0;}
int PortJafarLayer(const void*const*m){return 0;}
int PortMarluxiaFieldOn(void){return 0;}
void PortHdSpritesBegin(int l){}
int PortHdTitleArtReady(float*a){return 0;}
void PortPaletteFade(const uint16_t*a,const volatile uint16_t*b,int c,int*d,int*w){*d=*w=0;}
void PortHdTitleArtQueue(const float*x,const float*y,float a,float b,float c,float d,int e,int f,int g){}
int PortHdSpriteQueue(int l,int id,const float*x,const float*y,int px,int py,int pw,int ph,int a,int d,int w){return 0;}
void PortHdSpriteBox(int id,int*x,int*y,int*w,int*h){*x=*y=*w=*h=0;}
void PortHdTitleBarRow(int y,int x,int n,const uint32_t*a){}
int PortHdSpriteKeepsCartridge(int id){return 0;}
const uint8_t*PortHdSpriteDarkCells(int id,int*x,int*y,int*w,int*h){return NULL;}
void PortHdSpriteHudOwner(int slot){}
int PortJafarMapBuffer(const void*b){return 0;}
int PortGuardArmorBackdropMatches(const void*const*m){return 0;}
int PortGuardArmorBackdropPixel(int x, int y, uint16_t* color){return 0;}
int PortTrickmasterBackdropMatches(const void* const* maps){return 0;}
int PortTrickmasterBackdropPixel(int x, int y, int cx, int cy, uint16_t* color){return 0;}
int PortMaleficentLayer(const void* const* maps){return 0;}
int PortMaleficentDragonLayer(const void* const* maps){return 0;}
int PortMaleficentHallPixel(int x, int y, int cx, int cy, uint16_t* color){return 0;}
int PortMaleficentDragonVisible(int x, int y, int cx, int cy){return 0;}
int PortUrsulaLayer(const void* const* maps){return 0;}
int PortUrsulaFloorPixel(int x, int y, int cy, uint16_t* color){return 0;}
int PortOogieLayer(const void* const* maps){return 0;}
int PortOogieHallPixel(int x, int y, int cx, int cy, uint16_t* color){return 0;}
int PortJafarHallPixel(int x, int y, int cx, int cy, uint16_t* color){return 0;}
int PortJafarGenieCleared(int x, int y){return 0;}
int PortJafarGenieVisible(int x, int y, int cx, int cy){return 0;}
int PortMarluxiaExtendOn(void){return 0;}
int PortMarluxiaBodyEntry(int gx, int gy, int hofs, int vofs){return 0;}
int PortDarksideTileSource(int layer, int* tx, int* ty, int cx, int cy){return 0;}
int PortDarksideShorePixel(int x, int y, uint16_t* color){return 0;}
int PortDarksideBodyPixel(const void* const* maps, int x, int y, int cx, int cy, uint16_t* color){return 0;}
int PortMaleficentDragonPixel(const void* map, int x, int y, int cx, int cy, uint16_t* color){return 0;}
const uint16_t gAladdinPalette[16]={0};
const unsigned char gBtlShadowSmallTiles[32]={0},gBtlShadowLargeTiles[32]={0};
const uint16_t*PortLatinGlyphWidths(void){static uint16_t widths[256];return widths;}
const unsigned char gBtlShadowTiles[32]={0},gMapGmkAgrabah00Tiles[32]={0},gMapGmkAgrabah09Tiles[32]={0};
const uint16_t gCheshirePalette[16]={0},gGeniePalette[16]={0},gMapGmkAgrabah00Palette[16]={0},gMapGmkAgrabah01Palette[16]={0};
u32 gLanguage;
SpriteWork* gSpriteWork;
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); return 77; } } while (0)
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
    free(fallback);puts("PASS full production PPU: failed canvas pixels equal cartridge control; recovery restores separate HUD");return 0;
}
