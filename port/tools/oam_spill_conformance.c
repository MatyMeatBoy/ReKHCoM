/* Full production PPU with the existing ROM-free fixtures and absent artwork. */
#define HUD_CANVAS_NO_MAIN
#include "hud_canvas_ppu_conformance.c"

static unsigned char *saved;
static size_t stateBytes,stateAt;
static void countState(void*p,size_t n){(void)p;stateBytes+=n;}
static void saveState(void*p,size_t n){memcpy(saved+stateAt,p,n);stateAt+=n;}
static void loadState(void*p,size_t n){memcpy(p,saved+stateAt,n);stateAt+=n;}
int main(int argc,char**argv){
    /* Reuse the independent native-pixel oracle cases through the complete
     * current optional-art stubs, rather than the older unlinked fixture. */
    if(argc==3)return fixture_main_unused(argc,argv);
    CHECK(VirtualAlloc((void*)GBA_IO,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_PLTT,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_VRAM,0x20000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(VirtualAlloc((void*)GBA_OAM,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    PortCanvasAlloc();PortPpuSetResolution(240,160);
    uint16_t* native=(uint16_t*)(uintptr_t)GBA_OAM;
    uint16_t* palette=(uint16_t*)(uintptr_t)GBA_PLTT;
    for(int i=0;i<128;i++)native[i*4]=0x200;
    palette[0]=0;palette[257]=31;palette[258]=31<<5;
    memset((void*)(uintptr_t)(GBA_VRAM+0x10000),0x11,32);
    memset((void*)(uintptr_t)(GBA_VRAM+0x10020),0x22,32);
    IO16(0)=0x1040;
    uint16_t* last=PortOamPartBuffer(PORT_OAM_CAPACITY-1);CHECK(last);
    last[0]=30;last[1]=40;last[2]=0;PortOamSetPosition(PORT_OAM_CAPACITY-1,40,30);
    PortPpuRenderFrame();CHECK(gPortFrame[30*240+40]==0xffff0000);
    /* A lower native OAM index wins a same-priority tie. GPU priority still
     * wins before index, including the last spill slot and its occupancy key. */
    native[0]=30;native[1]=40;native[2]=1;
    PortPpuRenderFrame();CHECK(gPortFrame[30*240+40]==0xff00ff00);
    native[2]=1|(1<<10);PortPpuRenderFrame();CHECK(gPortFrame[30*240+40]==0xffff0000);
    CHECK(PortOamPartBuffer(-1)==NULL&&PortOamPartBuffer(PORT_OAM_CAPACITY)==NULL);
    /* Native affine matrix words remain at their original hardware addresses. */
    native[3]=0x1234;CHECK(OAM16[3]==0x1234);
    PortOamTagsReset();native[0]=0x200;PortPpuRenderFrame();CHECK(gPortFrame[30*240+40]==0xff000000);
    /* HD glyphs beyond index255 must neither wrap nor alias a native slot. */
    semantic("semantic_hd_world");
    uint16_t* glyph=PortOamPartBuffer(300);memcpy(glyph,native+4,6);native[4]=0x200;
    PortOamSetText(300,'A',0);PortPpuRenderFrame();
    PortTextGlyph text[8];CHECK(PortPpuTextGlyphs(text,8,0)==1);
    /* Save/load restores spill attributes and matching ownership as one image. */
    PortPpuVisitState(countState);saved=malloc(stateBytes);CHECK(saved);
    stateAt=0;PortPpuVisitState(saveState);CHECK(stateAt==stateBytes);
    uint32_t* before=malloc(240*160*4);CHECK(before);memcpy(before,gPortFrame,240*160*4);
    PortOamTagsReset();PortPpuRenderFrame();CHECK(PortPpuTextGlyphs(text,8,0)==0);
    stateAt=0;PortPpuVisitState(loadState);PortPpuRenderFrame();
    CHECK(PortPpuTextGlyphs(text,8,0)==1&&memcmp(before,gPortFrame,240*160*4)==0);
    free(before);free(saved);
    /* HUD ownership stores slot+1; slot511 must not wrap to the empty value. */
    PortOamTagsReset();semantic("semantic_hd_hud");native[4]=0x200;
    gPortConfig.hdText=0;gPortConfig.hdHud=1;gPortConfig.hdCards=1;
    last=PortOamPartBuffer(PORT_OAM_CAPACITY-1);last[0]=30;last[1]=40;last[2]=0;
    PortOamSetPosition(PORT_OAM_CAPACITY-1,40,30);
    sOamTag[PORT_OAM_CAPACITY-1].hud=1;sOamTag[PORT_OAM_CAPACITY-1].digit=1;
    PortPpuRenderFrame();CHECK(PortPpuHudActive());
    uint32_t* hud=calloc(320*180,4);CHECK(hud);PortPpuRenderHud(hud);
    int owned=-1;for(int i=0;i<320*180;i++)if(sHudDigitOwners[i]==PORT_OAM_CAPACITY){owned=i;break;}
    CHECK(owned>=0&&PortPpuHudSpriteVisible(PORT_OAM_CAPACITY-1,owned%320,owned/320));
    PortHudDigit digits[8];CHECK(PortPpuHudDigits(digits,8)==1&&digits[0].slot==PORT_OAM_CAPACITY-1);
    free(hud);
    puts("PASS full PPU spill: last slot, priorities, reset, matrices, HD glyph300, HUD owner512, state roundtrip");return 0;
}
