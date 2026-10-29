/* ROM-free regression through the actual object and screen compositors. */
#include <stdlib.h>
static int rejectScratch;
static int rejectedCalls;
static void* worldAlloc(size_t size);
#define malloc worldAlloc
#define HUD_CANVAS_NO_MAIN
#include "hud_canvas_ppu_conformance.c"
#undef malloc
static void* worldAlloc(size_t size) {
    if (rejectScratch && size == 240u * 160u * 9u) { rejectedCalls++; return NULL; }
    return malloc(size);
}

int main(void) {
    CHECK(VirtualAlloc((void*)GBA_IO,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_PLTT,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_VRAM,0x20000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_OAM,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    PortCanvasAlloc();PortPpuSetResolution(240,160);BuildLut();
    sLeft=0;sRight=240;sTop=0;sBottom=160;
    uint16_t* oam=(uint16_t*)(uintptr_t)GBA_OAM;
    uint16_t* pal=(uint16_t*)(uintptr_t)GBA_PLTT;
    for(int i=0;i<128;i++)oam[i*4]=0x200;
    pal[257]=31;pal[258]=31<<10;pal[259]=31|(31<<5);
    memset((void*)(uintptr_t)(GBA_VRAM+0x10000),0x11,32);
    memset((void*)(uintptr_t)(GBA_VRAM+0x10020),0x22,32);
    memset((void*)(uintptr_t)(GBA_VRAM+0x10040),0x33,32);
    oam[0]=20;oam[1]=40;oam[2]=2<<10; /* red illustration */
    oam[4]=20;oam[5]=40;oam[6]=(2<<10)|1; /* blue back */
    sOamTag[0].screen=sOamTag[1].screen=1;
    sOverlay=1;sScreenBg[2]=1;gPortConfig.hdCards=1;
    sMainHdPart[0]=sMainHdScreen[0]=sScreenHdAny=1;sOamHd[0].id=1;
    sMainHdBox[0][0]=40;sMainHdBox[0][1]=20;sMainHdBox[0][2]=sMainHdBox[0][3]=8;
    IO16(0)=0x1440;IO16(0x50)=0x1044;IO16(0x52)=0x1010;
    int on[4]={0,0,1,0},priority[4]={0,0,1,0},k=20*240+40;
    WorldBlendPrepare();CHECK(sWorldBlendActive);
    memset(sWorldBlendMask,1,240*160);
    for(int x=0;x<240;x++){sWinMask[x]=0x3f;sBgLine[2][x]=31<<5;}
    for(int y=20;y<28;y++) {
        DrawObjs(y,IO16(0));ComposeOverlayLine(y,IO16(0),on,priority);
    }
    /* Red + green = yellow in the original result. The old PPU instead
     * returned cyan, having selected the blue back after omitting red. */
    CHECK(sWorldBlendNative[k]==0xffffff00);
    CHECK(gPortOverlay[k]==0xff0000ff);
    CHECK(PortPpuScreenAdditive()[k]==0xff00ff00);
    sScreenOverUsed=0;
    HdOverLayer(1,240,160,sScreenOccBg,sScreenOccObj,gPortOverlay,sScreenOver,&sScreenOverUsed,0);
    CHECK(!sScreenOverUsed); /* no opaque blue/light copy above the HD art */
    CHECK(PortPpuScreenNativeWorld()[k]==0xffffff00);
    CHECK(!sScreenOverUsed); /* fallback does not repaint the blue back */
    sMainHdPart[0]=0;sWorldBlendActive=0;
    DrawObjs(20,IO16(0));ComposeOverlayLine(20,IO16(0),on,priority);
    CHECK(gPortOverlay[k]==0xffffff00); /* independent substitution-off control */

    sMainHdPart[0]=1;WorldBlendPrepare();memset(sWorldBlendMask,1,240*160);
    /* A priority-zero native foreground stays opaque and above the light. */
    oam[8]=20;oam[9]=40;oam[10]=2;sOamTag[2].screen=1;
    DrawObjs(20,IO16(0));ComposeOverlayLine(20,IO16(0),on,priority);
    CHECK(!sWorldBlendAdd[k]&&gPortOverlay[k]==0xffffff00&&sWorldBlendNative[k]==0xffffff00);
    oam[8]=0x200;
    WorldBlendPrepare();memset(sWorldBlendMask,1,240*160);DrawObjs(20,IO16(0));
    sWinMask[40]&=~WIN_OBJ;
    ComposeOverlayLine(20,IO16(0),on,priority);
    CHECK(!sWorldBlendAdd[k]&&gPortOverlay[k]==0xff00ff00&&sWorldBlendNative[k]==0xff00ff00);
    sWinMask[40]=0x3f&~WIN_SFX;
    ComposeOverlayLine(20,IO16(0),on,priority);
    CHECK(!sWorldBlendAdd[k]&&gPortOverlay[k]==0xff00ff00);
    /* Original/fixed views keep this menu on the main canvas. Exercise that
     * real producer as well; passing only a forced overlay misses the bug. */
    sOverlay=0;sScreenBg[2]=0;sOamTag[0].screen=sOamTag[1].screen=0;
    sMainHdScreen[0]=0;sMainHdAny=1;
    WorldBlendPrepare();CHECK(sWorldBlendActive&&!sWorldBlendScreen&&!PortPpuScreenAdditive());
    memset(sWorldBlendMask,1,240*160);
    for(int x=0;x<240;x++){sWinMask[x]=0x3f;sInWorld[x]=1;}
    for(int y=20;y<28;y++){
        DrawObjs(y,IO16(0));ComposeLine(y,IO16(0),on,priority,gPortFrame,1);
    }
    CHECK(sWorldBlendNative[k]==0xffffff00&&gPortFrame[k]==0xff0000ff);
    CHECK(PortPpuMainAdditive()[k]==0xff00ff00);
    CHECK(PortPpuMainNativeWorld()[k]==0xffffff00);
    sMainHdPart[0]=0;sWorldBlendActive=0;
    DrawObjs(20,IO16(0));ComposeLine(20,IO16(0),on,priority,gPortFrame,1);
    CHECK(gPortFrame[k]==0xffffff00);
    sMainHdPart[0]=1;
    IO16(0x52)=0x0808;WorldBlendPrepare();CHECK(!PortPpuScreenAdditive());
    IO16(0x52)=0x1010;gPortConfig.hdCards=0;WorldBlendPrepare();CHECK(!PortPpuScreenAdditive());
    gPortConfig.hdCards=1;gPortConfig.colorCorrection=1;WorldBlendPrepare();CHECK(!PortPpuScreenAdditive());
    gPortConfig.colorCorrection=0;
    free(sWorldBlendMemory);sWorldBlendMemory=NULL;sWorldBlendPixels=0;
    rejectScratch=1;WorldBlendPrepare();CHECK(!sWorldBlendActive&&!PortPpuScreenAdditive());
    WorldBlendPrepare();CHECK(rejectedCalls==1&&!sWorldBlendActive);
    rejectScratch=0;IO16(0x50)=0;WorldBlendPrepare();IO16(0x50)=0x1044;
    WorldBlendPrepare();CHECK(sWorldBlendActive);
    free(sWorldBlendMemory);
    puts("PASS production world-card main/screen composition: additive terms, native fallback parity, foreground, windows, inactive modes, allocation failure/recovery");
    return 0;
}
