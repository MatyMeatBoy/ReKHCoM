"""Exercise the production reload-counter pass against real PPU OAM ownership.

Native fill behind the counter must show through its transparent pixels;
front sprites still occlude the HD glyph, including scaled/clipped cases.
"""
import subprocess,tempfile,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2]
s=(R/'port/src/port_hdsprites.c').read_text(encoding='utf-8')
def function(text,name):
 a=text.index('void '+name+'(');b=text.index('{',a);i=b+1;depth=1
 while depth:depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[a:i]
a=s.index('typedef struct HdSpriteDraw {');b=s.index('} HdSpriteDraw;',a)+len('} HdSpriteDraw;');drawtype=s[a:b]
ppu=subprocess.check_output(['git','show','5e617a108:port/src/port_ppu.c'],cwd=R,text=True) if '--baseline-ownership' in sys.argv else (R/'port/src/port_ppu.c').read_text(encoding='utf-8');a=ppu.index('int PortPpuHudSpriteVisible(');b=ppu.index('\n}',a)+2;visible=ppu[a:b]
preamble=r'''
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_floorf floorf
#define SDL_ceilf ceilf
#define SDL_strncmp strncmp
#define HD_LAYERS 3
#define MAX_DRAWS 128
typedef void SDL_Renderer;typedef void SDL_Texture;
typedef struct {int x,y,w,h;} SDL_Rect;
typedef struct {float r,g,b,a;} SDL_FColor;
static int gPortHudW=8,gPortHudH=8;
static uint8_t owners[64],*sHudDigitOwners=owners;
static uint16_t oam[128*4];
#define PPU_OAM_BASE oam
'''
backend=r'''
typedef struct {SDL_Texture*image,*white;uint32_t lastUsed;} HdSpriteState;
static HdSpriteState sState[2];static HdSpriteDraw sDraws[3][128];static int sDrawCount[3],sArtCount,sBarCount;
static struct {const char*name;}kHdSprites[2]={{"gReloadCounterWhiteTiles_0"},{"gRevCountRedTiles"}};
static SDL_Renderer*sRenderer;static SDL_Rect clip;static int clipped,draws,ink[64*64];static float lastAlpha,lastShade;
static int Load(int id){(void)id;return 0;}
#define LoadCounted Load /* the stutter view's counting wrapper */
#define WhiteOf(i) (sState[i].white) /* the production white copy is made on demand; here it is ready */
static uint32_t PortFrameCount(void){return 123;}
static void SDL_SetRenderClipRect(SDL_Renderer*r,const SDL_Rect*c){(void)r;clipped=c!=0;if(c)clip=*c;}
static void Quad(SDL_Texture*t,const HdSpriteDraw*q,float ox,float oy,float scale,SDL_FColor color){
 (void)t;(void)q;(void)ox;(void)oy;(void)scale;assert(clipped);draws++;lastAlpha=color.a;lastShade=color.r;
 for(int y=clip.y;y<clip.y+clip.h;y++)for(int x=clip.x;x<clip.x+clip.w;x++)if(x>=0&&x<64&&y>=0&&y<64)ink[y*64+x]=1;
}
'''
cases=r'''
static void setup(void){
 memset(ink,0,sizeof(ink));memset(oam,0,sizeof(oam));memset(owners,31,sizeof(owners));memset(sDraws,0,sizeof(sDraws));sDrawCount[1]=1;
 sDraws[1][0]=(HdSpriteDraw){.id=0,.x={1,5,5,1},.y={1,1,5,5},.alpha=255};
 sState[0].image=(void*)1;sState[0].white=(void*)2;draws=clipped=0;PortHdSpriteHudOwner(10);
 assert(sDraws[1][0].hudOwner==11);
}
int main(void){
 setup();PortHdSpritesHudFront(0,0,0,1,0,0);assert(ink[2*64+2]&&draws&&!clipped); /* fill owner31 behind slot10 */
 setup();memset(owners,5,sizeof(owners));PortHdSpritesHudFront(0,0,0,1,0,0);assert(!draws); /* front native object */
 setup();for(int y=0;y<8;y++)for(int x=3;x<8;x++)owners[y*8+x]=5;
 PortHdSpritesHudFront(0,0,0,1,0,0);assert(ink[2*64+2]&&!ink[2*64+3]); /* partial occlusion */
 setup();PortHdSpritesHudFront(0,0,0,4,0,0);assert(ink[8*64+8]&&!ink[2*64+2]); /* zoom follows original quad */
 setup();PortHdSpritesHudFront(0,-8,0,4,0,0);assert(ink[8*64]&&!ink[8*64+12]); /* canvas clipping */
 setup();sDraws[1][0].alpha=160;sDraws[1][0].dark=64;PortHdSpritesHudFront(0,0,0,1,8,0);
 assert(fabsf(lastAlpha-160/255.f)<.001f&&fabsf(lastShade-.5f)<.001f);
 setup();PortHdSpritesHudFront(0,0,0,1,0,16);assert(lastShade==1); /* native full white fade */
 setup();oam[10*4+2]=1<<10;memset(owners,41,sizeof(owners));PortHdSpritesHudFront(0,0,0,1,0,0);assert(!draws); /* larger OAM slot but higher native priority */
 setup();oam[5*4+2]=2<<10;oam[10*4+2]=1<<10;memset(owners,6,sizeof(owners));PortHdSpritesHudFront(0,0,0,1,0,0);assert(draws); /* smaller slot, lower priority behind counter */
 setup();sState[0].image=0;PortHdSpritesHudFront(0,0,0,1,0,0);assert(!draws); /* unavailable asset */
 setup();PortHdSpritesBegin(1);PortHdSpritesHudFront(0,0,0,1,0,0);assert(!draws); /* next-frame/task lifetime */
 setup();sDraws[1][0].id=1;sDraws[1][0].hudOwner=0;PortHdSpriteHudOwner(10);assert(!sDraws[1][0].hudOwner); /* deck not gauge */
 puts("PASS production counter pass: fill/foreground OAM order, partial mask, zoom, clip, alpha/fades, fallback, lifecycle");return 0;
}
'''
cc=next((R.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='rekhcom-reload-mask-') as folder:
 folder=Path(folder);src=folder/'counter.c';exe=folder/'counter.exe';src.write_text(preamble+drawtype+visible+backend+function(s,'PortHdSpritesBegin')+function(s,'PortHdSpriteHudOwner')+function(s,'PortHdSpritesHudFront')+cases,encoding='utf-8');subprocess.run([str(cc),'-std=gnu11',str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
