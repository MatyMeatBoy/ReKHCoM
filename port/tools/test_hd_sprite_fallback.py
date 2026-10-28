"""First-frame fallback through real QueueHdPart, sprite loader and PPU DrawObjs.

Synthetic OAM/VRAM and injected decoder/texture failures; no SDL, ROM or GPU.
"""
from pathlib import Path
import subprocess
import tempfile
import re
from test_hd_sprite_budget import ROOT, function, preamble, source

ppu = (ROOT / 'port/src/port_ppu.c').read_text(encoding='utf-8')
def ppu_function(signature):
    start = ppu.index(signature)
    end = ppu.index('{', start) + 1
    depth = 1
    while depth:
        depth += (ppu[end] == '{') - (ppu[end] == '}')
        end += 1
    return ppu[start:end]

backend = '#include "' + (ROOT/'port/include/port/synthesis_requirement_hd.h').as_posix() + '"\n' + r'''
const unsigned char gSynthesisHdIdentities[SYNTHESIS_HD_COUNT] = {0};
#define HD_LAYERS 3
#define MAX_DRAWS 128
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define TRANSPARENT 0xFFFF
#define SDL_clamp(x,l,h) ((x)<(l)?(l):(x)>(h)?(h):(x))
#define PORT_STUTTER_NOW() 0
#define PORT_STUTTER_ADD(field, start) ((void)0)
static int Load(int i);
#define LoadCounted Load /* the stutter view's counting wrapper: the production loader */
static struct {int hdCards,hdTitle;} gPortConfig={1,1};
static int mainThread=1;
#if __STDC_HOSTED__
static int SDL_IsMainThread(void){return mainThread;}
#endif
static int GroupOn(int group){return 1;}
static int readyCalls,failReady;
#ifdef TEST_BRIDGE_READY
#define PORT_SDL_BRIDGE_TEXTURE_READY 1
static int PortBridgeTextureReady(SDL_Texture*t){return t && ++readyCalls!=failReady;}
#endif
static int PortHdDrawLogged(void){return 0;}
static void PortPaletteFade(const uint16_t*a,const volatile uint16_t*b,int c,int*d,int*e){*d=*e=0;}
static void PortHdSpriteHudOwner(int i){}
static int HdTextGlyph(int i){return 0;}
static int HeaderBandColor(int y,int x){return 0;}
static int sDrawCount[3];
static uint8_t sMainHdPart[128];
static struct {int width,height,originX,originY,wide;}gPortScreen={16,16,0,0,0};
typedef struct {int id,x,y,w,h,flags,bank;const void*tiles;const void*frame;const uint16_t*palette;} OamHdPart; static OamHdPart sOamHd[128];
static struct {int screen,hud,text,style,clipX0,clipX1,header;}sOamTag[128];
static struct {int x,y;}sOamExt[128];
static int sObjSize[3][4][2]={{{16,16}}};
static uint16_t oam[128*4],palette[512],regs[256];
static unsigned char vram[0x20000];
#define OAM16 oam
#define PLTT16 palette
#define VRAM8 vram
#define IO16(n) regs[(n)/2]
static int sEventBackdropProfile,sEnvActorFrameOff,sJafarFight,sLeft,sRight=16,sBottom=16,sHudLayer,sTitleBarHd,sOverlay,sLineHasObjects;
static struct {int v[1];}*sPreliminarRecipe[4];
static uint8_t sStagedActorEntered[3],sActorEntered[128];
static unsigned sStagedActorFirstSeen[3],sActorSceneFrames;
static const char gCheshirePalette[1],gAladdinPalette[1],gGeniePalette[1],gBtlShadowTiles[1],gBtlShadowSmallTiles[1],gBtlShadowLargeTiles[1];
static const unsigned char gOlympiaSmallCardTiles[1];
static uint16_t sObjLine[16],sScrLine[16],sTitleSampleLine[16];
static uint8_t sObjPrio[16],sObjSemi[16],sObjIndex[16],sScrPrio[16],sScrSemi[16],sScrIndex[16],sObjWin[16],sTitleSamplePrio[16],sTitleSampleSemi[16];
'''
cases = r'''
static void reset(void){
 for(int i=0;i<3;i++)Unload(i);assert(!decodedBytes);
 memset(sState,0,sizeof(sState));memset(sDrawCount,0,sizeof(sDrawCount));memset(sMainHdPart,0,sizeof(sMainHdPart));
 memset(regs,0,sizeof(regs));
 memset(oam,0,sizeof(oam));for(int i=1;i<128;i++)oam[i*4]=0x200;
 memset(sOamHd,0,sizeof(sOamHd));sOamHd[0].id=1;sOamHd[0].w=sOamHd[0].h=16;
 memset(vram,0x11,sizeof(vram));palette[257]=0x1234;
 kHdSprites[0].tiles=(void*)42;kHdSprites[0].frame=NULL;kHdSprites[0].group=2;
 kHdSprites[2]=kHdSprites[0];sBytes=0;width=height=decodedW=decodedH=16;infoOK=decodeOK=1;decodes=creates=destroys=failTexture=0;
 sRenderer=(void*)1;mainThread=1;readyCalls=failReady=surfaces=failSurface=failDecodeAlloc=0;
}
static int queue(void){return QueueHdPart(0,0,0,0,16,16,16,16,0,256,0,0,256,255);}
static int local_white(void){float x[4]={0,16,16,0},y[4]={0,0,16,16};return PortHdSpriteQueue(0,1,x,y,0,0,16,16,255,0,128);}
static void native_frame(void){
 sMainHdPart[0]=queue();DrawObjs(0,0x1000);
 assert(sObjLine[0]==0x1234); /* real native pixel survives in the very first PPU row */
 assert(!sMainHdPart[0]&&!sDrawCount[0]&&!sState[0].image);
}
int main(void){
 FILE*f=fopen("test","wb");assert(f);fputs("exists",f);fclose(f);
 reset();infoOK=0;native_frame();
 reset();decodeOK=0;native_frame();
 reset();failTexture=2;native_frame();assert(destroys==1);
 reset();failSurface=1;native_frame();assert(!creates&&!decodedBytes);
 reset();failDecodeAlloc=1;native_frame();assert(!creates&&!decodedBytes);
#ifdef TEST_BRIDGE_READY
 /* Host allocation/upload can fail after eager CPU textures exist. */
 for(int mode=0;mode<3;mode++){
  reset();failReady=mode?2:1;
  if(mode==2){regs[0x50/2]=0x10|(2<<6);regs[0x54/2]=8;}
  sMainHdPart[0]=mode==1?local_white():queue();DrawObjs(0,0x1000);
  assert(!sMainHdPart[0]&&!sDrawCount[0]&&sObjLine[0]==0x1234&&readyCalls==failReady);
  assert(sState[0].image&&sState[0].white&&creates==2&&!decodedBytes);
  assert(mode==1?local_white():queue());assert(creates==2); /* retained CPU data retries without decode/create */
 }
#endif
 /* Both eager textures are prepared before the first fade: no new white
    conversion or texture creation is deferred to that queue call. */
 reset();assert(queue()&&creates==2);sDrawCount[0]=0;
 assert(local_white()&&creates==2&&!decodedBytes);
 for(int effectCase=0;effectCase<4;effectCase++){
  reset();regs[0x50/2]=effectCase==1?(2<<6):0x10|((effectCase==2?3:2)<<6);
  regs[0x54/2]=effectCase==0?0:8;
  float x[4]={0,16,16,0},y[4]={0,0,16,16};
  assert(PortHdSpriteQueue(effectCase==3?2:0,1,x,y,0,0,16,16,255,0,0));
  assert(creates==2);
#ifdef TEST_BRIDGE_READY
  assert(readyCalls==1); /* unrelated/zero effects never request white upload */
#endif
 }
 reset();width=height=4096;native_frame();assert(!decodes);
 reset();sBytes=32u*1024*1024;sState[1].image=(void*)1;sState[1].bytes=sBytes;sState[1].lastUsed=9;native_frame();assert(!creates);
 reset();sRenderer=NULL;native_frame();assert(!decodes);
#if __STDC_HOSTED__
 reset();mainThread=0;native_frame();assert(!decodes);
#endif
 reset();sRenderer=NULL;assert(!PortHdSpriteFind((void*)42,NULL)&&!decodes);
#if __STDC_HOSTED__
 reset();mainThread=0;assert(!PortHdSpriteFind((void*)42,NULL)&&!decodes);
#endif
 reset();assert(queue());assert(sState[0].image&&sState[0].white&&sState[0].lastUsed==9);
 reset();infoOK=0;assert(!PortHdSpriteFind((void*)42,NULL));
 reset();decodeOK=0;assert(!PortHdSpriteFind((void*)42,NULL));
 reset();failTexture=2;assert(!PortHdSpriteFind((void*)42,NULL));
 reset();assert(PortHdSpriteFind((void*)42,NULL)==1);assert(sState[0].image&&sState[0].white&&sState[0].lastUsed==9);
 sMainHdPart[0]=queue();DrawObjs(0,0x1000);assert(sObjLine[0]==TRANSPARENT&&sDrawCount[0]==1&&creates==2);
 /* Subsequent loads cannot evict a replacement whose native pixels have been omitted. */
 sBytes=32u*1024*1024;sState[0].bytes=sBytes;assert(!Load(2));assert(sState[0].image);
 puts("PASS first-frame native pixel: invalid header/decode/upload, oversize/pinned budget, no renderer/off-main-thread; valid replacement acquired/pinned before omission");
 return 0;
}
'''
a = source.index('typedef struct HdSpriteDraw {')
b = source.index('} HdSpriteDraw;', a) + len('} HdSpriteDraw;')
unit = preamble + backend + source[a:b] + '\nstatic HdSpriteDraw sDraws[3][128];\n'
unit += function('static void Unload(') + function('static int Load(')
if 'static int SpriteReady(' in source:
    unit += function('static int SpriteTextureReady(')
    unit += function('static int SpriteReady(')
unit += function('int PortHdSpriteFind(') + function('int PortHdSpriteKeepsCartridge(') + function('int PortHdSpriteQueue(')
unit += '\nstatic int sSpotlight; static unsigned sScreenBgMask;\n'
unit += ppu_function('static int SpriteOnScreen(')
native_color=ppu_function('static int SmallCardNativeColor(')
unit += '\n'.join('static const unsigned char '+n+'[1]={0};' for n in sorted(set(re.findall(r'\bg\w+SmallCardTiles\b',native_color))-{'gOlympiaSmallCardTiles'}))
unit += native_color
if 'static int TrimHdPiece(' in ppu:  # each texel of an overlapping picture drawn once
    unit += ppu[ppu.index('static struct { int layer, id, ox, oy'):ppu.index('static int TrimHdPiece(')]
    unit += ppu_function('static int TrimHdPiece(')
    if 'static void RememberHdPiece(' in ppu:
        unit += ppu_function('static void RememberHdPiece(')
    cases=cases.replace('static void reset(void){','static void reset(void){sHdPieceCount=0;')
unit += ppu_function('static int QueueHdPart(') + ppu_function('static void DrawObjs(') + cases
compiler = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='rekhcom-sprite-fallback-') as folder:
    folder = Path(folder)
    src, exe = folder / 'fallback.c', folder / 'fallback.exe'
    src.write_text(unit, encoding='utf-8')
    for freestanding in (False, True):
        flags = ['-ffreestanding','-DTEST_BRIDGE_READY'] if freestanding else []
        subprocess.run([str(compiler), '-std=c11', '-Werror=implicit-function-declaration', *flags,
                        '-Wno-compare-distinct-pointer-types', str(src), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], cwd=folder, check=True)
