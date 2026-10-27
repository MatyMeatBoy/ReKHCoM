"""Fault injection for real canvas transaction, PPU selection/ink suppression and Present consumer.
Small extracted production blocks, not a complete frame/GPU integration test.
"""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def fn(s,name):
 a=s.rfind("\n",0,s.index(name+"("))+1;b=s.index("{",a);e=b+1;d=1
 while d:d+=(s[e]=="{")-(s[e]=="}");e+=1
 return s[a:e]
main=(ROOT/'port/src/port_main.c').read_text(encoding='utf8')
ppu=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf8')
start=ppu.index('    sHudLayer = gPortScreen.wide')
select=ppu[start:ppu.index('    sHdText =',start)]
core=''
if 'int PortHudCanvasReady(' in main:
 a=main.index('/* HUD canvas transaction:');b=main.index('static void Present(',a)
 core=main[a:b]
else:core='static int PortHudCanvasReady(int w,int h){(void)w;(void)h;return 0;}\n'
core+=fn(ppu,'ObjColor')+'\n'
a=ppu.index('        if (!sOamHd[i].id ||',ppu.index('static void QueueMainHdParts('));b=ppu.index('continue;',a)+len('continue;')
core+='static int QueueAllowed(int i){int a0=0,affine=0,dbl=0,shape=0,mode=0;for(int once=0;once<1;once++){'+ppu[a:b]+'return 1;}return 0;}\n'
if 'void PortPpuHudFrameBegin(' in ppu:core+=fn(ppu,'PortPpuHudFrameBegin')+'\n'+fn(ppu,'PortPpuHudFrameEnd')+'\n'
core+='static void SelectLayer(void){'+select+'}\n'+fn(ppu,'HdTextGlyph')+'\n'
a=ppu.index('        if (sHudLayer && sOamTag[i].hud)');b=ppu.index('        if (bandOnly',a)
core+='static int NativeInk(int i){for(int once=0;once<1;once++){'+ppu[a:b]+'return 1;}return 0;}\n'
# Exact Present canvas branch prefix, including its actual allocation path.
a=main.index('    if (PortPpuHudActive() && !menuCanvas)');b=main.index('        /* The field HUD',a)
restore='        PortPpuHudFrameEnd(savedHud);\n' if 'PortPpuHudFrameEnd(savedHud)' in main else ''
core+='static void PresentCanvas(void){int menuCanvas=0;'+main[a:b]+restore+'}}\n'
pre=r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
typedef int SDL_Renderer;typedef struct{int id;}SDL_Texture;
typedef struct{float x,y,w,h;}SDL_FRect;
static SDL_Renderer renderer=1,*sRenderer=&renderer;
static SDL_Texture *sHudTexture;static int sHudTexW,sHudTexH;
static uint32_t *sHudBuf;static size_t sHudCapacity;
static uint64_t sRendererGeneration=1;
static int allocs,texCreates,texDestroys,logs,failAlloc,failTex,draws;
static int gPortHudW=240,gPortHudH=160,gPortHudScale=1;
static struct{int wide;}gPortScreen={1};
static struct{int hdHud,hdText;}gPortConfig={1,1};
static int sHudLayer,sHdText=1,sHudCanvasFallback;
static int sHudFrameW,sHudFrameH,sHudFrameScale;
static unsigned char VRAM8[0x18000];static uint16_t PLTT16[512];
static uint16_t IO16(int i){(void)i;return 0;}
static struct{int id;}sOamHd[2]={{1},{1}};
static struct{int hud,text,style;}sOamTag[2]={{1,'A',0},{0,'B',0}};
#define GBA_SCREEN_W 240
#define SDL_PIXELFORMAT_ARGB8888 0
#define SDL_TEXTUREACCESS_STREAMING 0
#define SDL_BLENDMODE_BLEND 0
#define SDL_SCALEMODE_NEAREST 0
#define SDL_free free
static void*SDL_malloc(size_t n){allocs++;return failAlloc?NULL:malloc(n);}
static SDL_Texture*SDL_CreateTexture(SDL_Renderer*r,int f,int a,int w,int h){(void)r;(void)f;(void)a;(void)w;(void)h;texCreates++;if(failTex)return NULL;SDL_Texture*t=malloc(sizeof(*t));t->id=texCreates;return t;}
static int SDL_SetTextureBlendMode(SDL_Texture*t,int m){(void)t;(void)m;return 1;}
static int SDL_SetTextureScaleMode(SDL_Texture*t,int m){(void)t;(void)m;return 1;}
static void SDL_DestroyTexture(SDL_Texture*t){if(t){texDestroys++;free(t);}}
static void PortLog(const char*f,...){(void)f;logs++;}
static void GrowPixelBuffer(uint32_t**p,size_t*n,size_t k){if(k>*n){*p=SDL_malloc(k*4);if(!*p)exit(88);*n=k;}}
static int PortPpuHudActive(void){return sHudLayer;}
static void PortPpuRenderHud(uint32_t*p){if(!p)exit(89);if((size_t)gPortHudW*gPortHudH>sHudCapacity){fprintf(stderr,"stale HUD canvas overflow\n");exit(90);}draws++;}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 77;}}while(0)
'''
test=r'''
int main(void){
 failAlloc=1;SelectLayer();CHECK(!sHudLayer&&sHudCanvasFallback&&NativeInk(0));
 CHECK(gPortConfig.hdHud==1&&gPortConfig.hdText==1);
 CHECK(!QueueAllowed(0)&&QueueAllowed(1));
 /* Actual cartridge decoder preserves every pixel/colour of the fixture. */
 for(int n=1;n<16;n++)PLTT16[256+n]=(uint16_t)(n*0x421);
 for(int n=0;n<32;n++)VRAM8[0x10000+n]=(unsigned char)(0x21+(n%7)*0x11);
 for(int y=0;y<8;y++)for(int x=0;x<8;x++){
  int ci=(VRAM8[0x10000+y*4+x/2]>>((x&1)*4))&15;
  CHECK(NativeInk(0)&&ObjColor(0,0,x,y,8,1)==(PLTT16[256+ci]&0x7fff));
 }
 int a=allocs,t=texCreates,l=logs;
 for(int i=0;i<1000;i++){SelectLayer();PresentCanvas();}
 CHECK(allocs==a&&texCreates==t&&logs==l&&draws==0);
 failAlloc=0;gPortHudW=241;SelectLayer();CHECK(sHudLayer&&!sHudCanvasFallback&&texCreates==0);PresentCanvas();CHECK(draws==1&&texCreates==1);
 CHECK(!NativeInk(0));uint32_t*old=sHudBuf;SDL_Texture*oldTex=sHudTexture;size_t cap=sHudCapacity;
 sHudBuf[0]=0x12345678;failAlloc=1;gPortHudW=480;SelectLayer();CHECK(!sHudLayer&&NativeInk(0));
 CHECK(sHudBuf==old&&sHudTexture==oldTex&&sHudCapacity==cap&&sHudBuf[0]==0x12345678);
 a=allocs;t=texCreates;for(int i=0;i<1000;i++)SelectLayer();CHECK(allocs==a&&texCreates==t);
 /* Lower resolution can reuse the still-owned old canvas, even allocator unavailable. */
 gPortHudW=241;SelectLayer();CHECK(sHudLayer&&sHudBuf==old&&sHudTexture==oldTex);
 /* Explicit invalidation after resources are freed permits one retry. */
 failAlloc=0;gPortHudW=480;PortHudCanvasRetry();SelectLayer();CHECK(sHudLayer&&sHudBuf!=old);
 CHECK(sHudTexture==oldTex); /* preflight never creates/destroys GPU textures */
 CHECK(gPortConfig.hdHud==1&&gPortConfig.hdText==1);
 sOamTag[0].text=0x8260;CHECK(!NativeInk(0));failAlloc=1;gPortHudW=600;SelectLayer();CHECK(NativeInk(0));
 CHECK(HdTextGlyph(1));
 a=allocs;failAlloc=0;sRendererGeneration++;SelectLayer();CHECK(sHudLayer&&allocs==a+1);
 CHECK(HdTextGlyph(1)); /* unrelated text remains HD */
 /* Frozen Options frame: world can stay641x360 while desired HUD grows. */
 failAlloc=0;gPortHudW=427;gPortHudH=240;gPortHudScale=3;SelectLayer();CHECK(sHudLayer);
 gPortHudW=428;failAlloc=1;a=allocs;int beforeDraw=draws;
 PresentCanvas();CHECK(draws==beforeDraw+1&&allocs==a&&gPortHudW==428&&gPortHudH==240&&gPortHudScale==3);
 SelectLayer();CHECK(!sHudLayer&&sHudCanvasFallback); /* next real frame falls back */
 puts("PASS canvas transaction + actual PPU/native ink/Present: sticky failure, rollback, lower resolution and generation recovery");return 0;
}
'''
base=next(p for p in ROOT.parents if (p/'toolchain').is_dir());cc=next((base/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='hud-canvas-') as d:
 p=Path(d);(p/'test.c').write_text(pre+core+test,encoding='utf8')
 subprocess.run([str(cc),'-std=gnu11','-Werror',str(p/'test.c'),'-o',str(p/'test.exe')],check=True)
 r=subprocess.run([str(p/'test.exe')],capture_output=True,text=True,timeout=10,creationflags=0x08000000);print(r.stdout+r.stderr,end='');r.check_returncode()

# Compile the existing full PPU fixture separately, preserving its independent
# synthetic cartridge setup; no copied rasterizer or whole-game build.
with tempfile.TemporaryDirectory(prefix='hud-canvas-ppu-') as d:
 p=Path(d);exe=p/'test.exe'
 inc=[ROOT/'include']+[p for p in (ROOT/'include').iterdir() if p.is_dir() and p.name!='gba']
 cc=next((base/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
 args=[str(cc),'-O1','-std=gnu11','-funsigned-char','-fno-strict-aliasing','-fwrapv','-mno-ms-bitfields','-DPLATFORM_PC','-DVERSION_EU','-I'+str(ROOT/'port/include')]
 for path in inc:args+=['-iquote',str(path)]
 subprocess.run(args+[str(ROOT/'port/tools/hud_canvas_ppu_conformance.c'),'-o',str(exe)],check=True)
 r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=20,creationflags=0x08000000)
 print(r.stdout+r.stderr,end='');r.check_returncode()
