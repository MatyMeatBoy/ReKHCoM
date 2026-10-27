/* ROM-free integration fixture: use the actual renderer with synthetic GBA
 * memory. The independent pixel oracle lives in test_ppu_conformance.py. */
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#undef TRANSPARENT
#ifndef PPU_SOURCE
#define PPU_SOURCE "../src/port_ppu.c"
#endif
#include PPU_SOURCE

PortConfig gPortConfig;
static int sFixtureBackdropPanel = -1;
int PortEventBackdropPanel(void) { return sFixtureBackdropPanel; }
static int sFixtureTiledBackdrop = -1;
int PortEventTiledBackdrop(void) { return sFixtureTiledBackdrop; }
const void* gMsgwinMapBlocks[4];
static int hblank;
/* Optional HD artwork is disabled in this pixel-renderer oracle. */
static int sFixtureCanvasReady = 1;
int PortHudCanvasReady(int w, int h) { (void)w; (void)h; return sFixtureCanvasReady; }
void PortHudCanvasRetry(void) {}
uint32_t* PortHdHudExportBuffer(void) { return NULL; }
void PortHdHudExportSprite(int w, int h, int part, int ax, int ay) {}
int PortHdHudLettersReady(void) { return 0; }
int PortHdHudPlayerFrameReady(void) { return 0; }
void PortHdHudSpriteBegin(void) {}
int PortHdHudSpriteRegister(const uint32_t* pixels, int w, int h, int slot, int x, int y, int flip, int alpha) { return 0; }

void PortFatal(const char* fmt, ...) { (void)fmt; abort(); }
void PortLog(const char* fmt, ...) { (void)fmt; }
int PortHd2dActive(void) { return 0; }
uint32_t PortFrameCount(void) { return 0; }
void PortRaiseIrq(uint16_t flags) { (void)flags; }
void PortDmaHBlank(void) {
    if (hblank) IO16(0x10) = (IO16(0x06) + 1) * 3;
}

static uint16_t color(int i) {
    return ((i * 3) & 31) | (((i * 5) & 31) << 5) | (((i * 7) & 31) << 10);
}

static void text(int bg, int bpp8, int size, int mosaic) {
    uint8_t* v = (uint8_t*)(uintptr_t)GBA_VRAM;
    uint16_t* p = (uint16_t*)(uintptr_t)GBA_PLTT;
    int t, x, y, tx, ty;
    int width = size & 1 ? 64 : 32, height = size & 2 ? 64 : 32;
    IO16(8 + bg * 2) = (16 << 8) | (size << 14) | (bpp8 << 7) | (mosaic << 6);
    for (t = 0; t < 16; t++) for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) {
        int ci = 1 + ((t + x + 3*y) % (bpp8 ? 255 : 15));
        if (bpp8) v[t*64+y*8+x] = ci;
        else {
            int off = t*32+y*4+x/2;
            v[off] |= ci << ((x&1)*4);
        }
    }
    for (ty = 0; ty < height; ty++) for (tx = 0; tx < width; tx++) {
        int block = tx/32 + (ty/32)*(width/32);
        int tile = (tx + 5*ty) % 16;
        int flip = ((tx&1) << 10) | ((ty&1) << 11);
        ((uint16_t*)(v+0x8000))[block*1024+(ty%32)*32+tx%32] = tile | flip | (((tx+ty)%4) << 12);
    }
    for (t = 0; t < 256; t++) p[t] = color(t);
    p[0] = color(17);
    IO16(0x10 + bg*4) = 249;
    IO16(0x12 + bg*4) = 251;
    IO16(0x4c) = 0x23; /* 4 columns, 3 rows */
}

static void affine(int bg, int size, int wrap, int mosaic, int rotated) {
    uint8_t* v = (uint8_t*)(uintptr_t)GBA_VRAM;
    int t, x, y, side = 16 << size, a = bg-2;
    IO16(8+bg*2) = (16 << 8) | (size << 14) | (wrap << 13) | (mosaic << 6);
    for (t=0;t<16;t++) for(y=0;y<8;y++) for(x=0;x<8;x++)
        v[t*64+y*8+x] = 1 + ((t+3*x+5*y)%255);
    for(y=0;y<side;y++) for(x=0;x<side;x++) v[0x8000+y*side+x]=(x+2*y)%16;
    IO16(0x20+a*0x10)=rotated ? 192 : 256;
    IO16(0x22+a*0x10)=rotated ? 96 : 0;
    IO16(0x24+a*0x10)=rotated ? -96 : 0;
    IO16(0x26+a*0x10)=rotated ? 192 : 256;
    IO32(0x28+a*0x10)=(uint32_t)(-7*256) & 0xfffffff;
    IO32(0x2c+a*0x10)=11*256;
    IO16(0x4c)=0x23;
}

static void bitmap(int mode, int page, int mosaic, int rotated) {
    uint8_t* v = (uint8_t*)(uintptr_t)GBA_VRAM;
    int x,y,width=mode==5?160:240,height=mode==5?128:160;
    int offset=mode!=3&&page?0xa000:0;
    IO16(0) = mode | 0x400 | (page ? 0x10 : 0);
    IO16(0x0c)=mosaic << 6;
    for(y=0;y<height;y++) for(x=0;x<width;x++) {
        int ci=(x+3*y+page*13)%256;
        if(mode==4) v[offset+y*width+x]=ci;
        else ((uint16_t*)(v+offset))[y*width+x]=color(ci);
    }
    IO16(0x20)=rotated?192:256; IO16(0x22)=rotated?64:0;
    IO16(0x24)=rotated?-64:0; IO16(0x26)=rotated?192:256;
    IO32(0x28)=rotated?7*256:0; IO32(0x2c)=rotated?13*256:0;
    IO16(0x4c)=0x23;
}

static void obj(int bpp8, int mapping, int flip, int affineObj, int mosaic) {
    uint8_t* v=(uint8_t*)(uintptr_t)GBA_VRAM;
    uint16_t* o=(uint16_t*)(uintptr_t)GBA_OAM;
    int x,y,ow=16,oh=16;
    IO16(0)=0x1000|(mapping?0x40:0);
    o[0]=5|(bpp8?0x2000:0)|(affineObj?0x300:0)|(mosaic?0x1000:0);
    o[1]=3|0x4000|(flip?0x3000:0); o[2]=0;
    if(affineObj) { o[3]=192;o[7]=64;o[11]=-64;o[15]=192; }
    for(y=0;y<oh;y++) for(x=0;x<ow;x++) {
        int ci=1+(x+3*y)%(bpp8?255:15);
        int tile=mapping?((y/8)*2+x/8)*(bpp8?2:1):(y/8)*32+(x/8)*(bpp8?2:1);
        int off=0x10000+tile*32+(y%8)*(bpp8?8:4)+(x%8)/(bpp8?1:2);
        if(bpp8) v[off]=ci; else v[off]|=ci<<((x&1)*4);
    }
    IO16(0x4c)=0x2300;
}

static void composition(const char* name) {
    uint8_t* v=(uint8_t*)(uintptr_t)GBA_VRAM;
    uint16_t* p=(uint16_t*)(uintptr_t)GBA_PLTT;
    uint16_t* o=(uint16_t*)(uintptr_t)GBA_OAM;
    int withObj=strstr(name,"obj")||strstr(name,"semi")||strstr(name,"window");
    IO16(0)=0x300|0x40|(withObj?0x1000:0);
    IO16(8)=(16<<8)|1;
    IO16(10)=(17<<8)|4|2;
    memset(v,0x11,32); memset(v+0x4000,0x22,32);
    p[1]=31; p[2]=31<<5; p[257]=31<<10; p[258]=31|(31<<5);
    if(strstr(name,"bg_tie")) IO16(10)=(17<<8)|4|1;
    if(withObj) {
        memset(v+0x10000,0x11,128);
        o[0]=30; o[1]=40|0x4000; o[2]=1<<10;
        if(strstr(name,"obj_behind")) o[2]=3<<10;
        if(strstr(name,"oam_tie")) {
            memset(v+0x10000+128,0x22,128);
            o[4]=30;o[5]=40|0x4000;o[6]=(1<<10)|4;
        }
        if(strstr(name,"semi")) o[0]|=0x400;
    }
    if(strstr(name,"alpha")) {
        IO16(0x50)=1|0x40|(2<<8);
        IO16(0x52)=8|(8<<8);
        if(strstr(name,"clamp")) IO16(0x52)=31|(31<<8);
        if(strstr(name,"blocked")) IO16(0x50)=1|0x40|(0x20<<8);
        if(strstr(name,"backdrop")) { IO16(0)&=~0x200; IO16(0x50)=1|0x40|(0x20<<8); }
    }
    if(strstr(name,"bright")) { IO16(0x50)=0x3f|0x80; IO16(0x54)=9; }
    if(strstr(name,"dark")) { IO16(0x50)=0x3f|0xc0; IO16(0x54)=9; }
    if(strstr(name,"semi")) { IO16(0x50)=1<<8;IO16(0x52)=8|(8<<8); }
    if(strstr(name,"window")) {
        IO16(0)|=0xe000;
        IO16(0x40)=(30<<8)|90; IO16(0x44)=(10<<8)|70;
        IO16(0x42)=(60<<8)|120; IO16(0x46)=(40<<8)|100;
        IO16(0x48)=1|(2<<8); IO16(0x4a)=0x13|(0x10<<8);
        o[4]=50|0x800;o[5]=70|0x4000;o[6]=0;
        if(strstr(name,"wrapped")) { IO16(0x40)=(220<<8)|20; IO16(0x44)=(150<<8)|10; }
        if(strstr(name,"semi")) {
            /* All zones allow layers but prohibit SFX: semi-transparent
               OBJ alpha still applies when the next layer is a target. */
            IO16(0x48)=0x13|(0x13<<8);IO16(0x4a)=0x13|(0x13<<8);
        }
    }
}

static void semantic(const char* name) {
    uint16_t* o=(uint16_t*)(uintptr_t)GBA_OAM;
    uint8_t* v=(uint8_t*)(uintptr_t)GBA_VRAM;
    uint16_t* p=(uint16_t*)(uintptr_t)GBA_PLTT;
    int x,y;
    composition("compose_obj_tie");
    gPortConfig.hdText=1;
    o[0]=0x200;
    o[4]=30;o[5]=40|0x4000;o[6]=1<<10;
    PortOamSetText(1,'A',strstr(name,"compact")?2:0);
    if(strstr(name,"cover_bg")) {
        if(strstr(name,"bgtext")) IO16(10)&=~3;
        else IO16(8)&=~3;
    }
    if(strstr(name,"cover_obj")) { o[0]=30;o[1]=40|0x4000;o[2]=0; }
    if(strstr(name,"window")) {
        IO16(0)|=0x2000;IO16(0x40)=(30<<8)|90;IO16(0x44)=(10<<8)|70;
        IO16(0x48)=3;IO16(0x4a)=0x3f;
        o[6]=0; /* strongest glyph must still obey a window hiding OBJ */
    }
    if(strstr(name,"screen")) { gPortScreenLayer=1;sOamTag[1].screen=1;sOamTag[0].screen=1; }
    if(strstr(name,"hud")) {
        IO16(0)|=1|0x400;IO16(0x0c)=(2<<14)|(24<<8)|(3<<2);
        PortPpuSetResolution(320,180);gPortHudScale=1;gPortHudW=320;gPortHudH=180;
        sOamTag[1].hud=1;
        if(strstr(name,"message")) sBgDockedLatched[0]=1;
    }
    if(strstr(name,"bgtext")) {
        o[4]=0x200;
        memset(v+32,0x11,32);
        for(x=0;x<1024;x++) ((uint16_t*)(v+0x8000))[x]=1;
        ((uint16_t*)(v+0x8000))[3*32+5]=0;
        memset(v,0xdd,32);
        for(y=2;y<6;y++) for(x=2;x<6;x++) {
            int off=y*4+x/2,ci=(x+y)&1?15:14;
            v[off]=(v[off]&~(15<<((x&1)*4)))|(ci<<((x&1)*4));
        }
        p[13]=0x4000;p[14]=0;p[15]=0x7fff;
        PortRegisterBgTextTile((void*)GBA_VRAM,'5',2,v);
        if(strstr(name,"explicit")) {
            memset(v,0x11,32);
            for(y=1;y<=5;y++) for(x=1;x<=4;x++) {
                int off=y*4+x/2,ci=2;
                v[off]=(v[off]&~(15<<((x&1)*4)))|(ci<<((x&1)*4));
            }
            memset(v+6*4,0x33,4); /* dense shadow is deliberately the most frequent ink */
            PortRegisterBgTextTile((void*)GBA_VRAM,'2',2|(2<<8)|(3<<12)|(1<<16),v);
        }
        if(strstr(name,"stale")) v[0]^=1;
        if(strstr(name,"clear")) PortClearBgTextTiles();
        if(strstr(name,"cover_obj")) o[0]=24;
    }
}

/* Differential wide-world fixture. PPU_SOURCE can select a prior renderer
   when compiling this same harness; no ROM or game fixtures are required. */
static int WideTile(int bg, int tx, int ty) {
    if (tx < -96 || tx >= 120 || ty < -48 || ty >= 64) return -1;
    return ((tx + 5 * ty) & 15) | ((tx & 1) ? 0x400 : 0) |
           ((ty & 1) ? 0x800 : 0) | (((tx + ty) & 3) << 12);
}
static void wide_world(const char* name) {
    int y, bpp8 = strstr(name, "8bpp") != NULL;
    int mosaic = strstr(name, "mosaic") != NULL;
    IO16(0) = 0x100;
    text(0, bpp8, 3, mosaic);
    PortPpuSetResolution(1920, 1080);
    PortPpuBeginFrame();
    gPortScreen.wide = 1;
    sBgWorldLatched[0].valid = 1;
    sBgWorldLatched[0].fn = WideTile;
    sBgWorldLatched[0].x = -13;
    sBgWorldLatched[0].y = 27;
    sBgWorldLatched[0].hofs = IO16(0x10);
    sBgWorldLatched[0].vofs = IO16(0x12);
    sLeft = -gPortScreen.originX;
    sRight = gPortScreen.width - gPortScreen.originX;
    sTop = -gPortScreen.originY;
    sBottom = gPortScreen.height - gPortScreen.originY;
    for (y = 0; y < gPortScreen.height; y++)
        RenderLine(y, y - gPortScreen.originY);
}

int main(int argc, char** argv) {
    const char* name;
    uint16_t* p;
    int i, forceWide;
    FILE* f;
    if(argc!=3 && argc!=4) return 2;
    name=argv[1];
    forceWide=!strncmp(name,"fast_",5);
    if(forceWide) name+=5;
    if(!VirtualAlloc((void*)GBA_IO,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE) ||
       !VirtualAlloc((void*)GBA_PLTT,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE) ||
       !VirtualAlloc((void*)GBA_VRAM,0x20000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE) ||
       !VirtualAlloc((void*)GBA_OAM,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE)) return 3;
    PortCanvasAlloc(); /* Reserve fixed hardware addresses before the large heap buffers. */
    p=(uint16_t*)(uintptr_t)GBA_PLTT;
    for(i=0;i<512;i++) p[i]=color(i%256);
    p[0]=color(17);
    for(i=0;i<128;i++) ((uint16_t*)(uintptr_t)GBA_OAM)[i*4]=0x200;
    PortPpuSetResolution(240,160);
    if(!strcmp(name,"forced")) IO16(0)=0x80;
    else if(!strcmp(name,"backdrop")) IO16(0)=0;
    else if(!strncmp(name,"wide_world",10)) {
        int repeat = argc == 4 ? atoi(argv[3]) : 1;
        clock_t start = clock();
        if (repeat < 1 || repeat > 1000) return 2;
        for (i = 0; i < repeat; i++) wide_world(name);
        if (argc == 4) printf("wide benchmark: %d frames, %.3f ms/frame\n", repeat,
            1000.0 * (clock() - start) / CLOCKS_PER_SEC / repeat);
        f=fopen(argv[2],"wb"); if(!f) return 5;
        fwrite(gPortFrame,4,gPortScreen.width*gPortScreen.height,f); fclose(f);
        return 0;
    }
    else if(!strncmp(name,"text",4)) {
        int bpp8=strstr(name,"8bpp")!=NULL, size=0;
        if(strstr(name,"size1")) size=1;
        if(strstr(name,"size2")) size=2;
        if(strstr(name,"size3")) size=3;
        IO16(0)=0x100;
        text(0,bpp8,size,strstr(name,"mosaic")!=NULL);
        if(strstr(name,"hblank")) { hblank=1; IO16(0x10)=0; }
    } else if(!strncmp(name,"affine",6)) {
        int bg=strstr(name,"bg3")?3:2, size=strstr(name,"size1")?1:0;
        IO16(0)=(bg==3?2:1)|(0x100<<bg);
        affine(bg,size,strstr(name,"wrap")&&!strstr(name,"nowrap"),strstr(name,"mosaic")!=NULL,strstr(name,"rotate")!=NULL);
    } else if(!strncmp(name,"bitmap",6)) {
        bitmap(name[6]-'0',strstr(name,"page1")!=NULL,strstr(name,"mosaic")!=NULL,strstr(name,"rotate")!=NULL);
    } else if(!strncmp(name,"semantic",8)) {
        semantic(name);
    } else if(!strncmp(name,"compose",7)) {
        composition(name);
    } else if(!strncmp(name,"obj",3)) {
        obj(strstr(name,"8bpp")!=NULL,strstr(name,"1d")!=NULL,strstr(name,"flip")!=NULL,strstr(name,"affine")!=NULL,strstr(name,"mosaic")!=NULL);
    } else return 4;
    if(forceWide) {
        /* Exercise the wide-world compositor at native dimensions so the
           independent hardware oracle remains directly comparable. */
        gPortConfig.hdText=1; /* Also collect the HUD/text priority metadata. */
        PortPpuBeginFrame(); gPortScreen.wide=1; LatchAffine(1);
        for(i=0;i<160;i++) {
            LineStart(i); LatchAffine(0); RenderLine(i,i); StepAffine(1); HBlank(i);
        }
    } else PortPpuRenderFrame();
    if(!strncmp(name,"semantic",8)) {
        PortTextGlyph glyphs[128] = {0}; /* OBJ text has no BG cap/base metadata. */
        int n0=PortPpuTextGlyphs(glyphs,128,0),n1=PortPpuTextGlyphs(glyphs,128,1),n2=PortPpuTextGlyphs(glyphs,128,2);
        printf("{\"counts\":[%d,%d,%d],\"cap\":%d,\"base\":%d}\n",n0,n1,n2,
               n0+n1+n2 ? glyphs[0].cap : -1,n0+n1+n2 ? glyphs[0].base : -1);
    }
    f=fopen(argv[2],"wb"); if(!f) return 5;
    fwrite(gPortFrame,4,240*160,f); fclose(f);
    return 0;
}
