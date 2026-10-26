"""Probe production poster painting, fades, modal occlusion and texture lifetime on SDL software."""
import os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=Path('D:/01Claude/ReKHCoM-build/previews-gba-20261026/render-probe');OUT.mkdir(parents=True,exist_ok=True)
def function(source,signature):
 start=source.index(signature);brace=source.index('{',start);end=brace+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
gallery=(ROOT/'port/src/port_scene_gallery.c').read_text(encoding='utf-8');ui=(ROOT/'port/src/port_ui.c').read_text(encoding='utf-8')
unit=r'''#include <SDL3/SDL.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint32_t sPreview[640*360],canvas[640*360];
static int sPreviewReady=1,sPreviewW=640,sPreviewH=360,sPreviewHd=1,sPreviewSerial=1,sTextureSerial=-1,sTextureW,sTextureH;
static int sPreviewEvent=10;
static SDL_Texture* sPreviewTexture;
static SDL_Renderer* sPreviewRenderer;
static uint32_t *sFb=canvas,*sHdUiFrame=canvas;
static int sFbW=640,sFbH=360,sHdUiCount,sHdUiRun,sHdUiInText,glyphCalls;
static struct {int hdTitle;} gPortConfig={1};
static struct { int code,x,y,alive,pw,run;uint32_t face,shadow;} sHdUiGlyphs[2048];
static void PortSceneGalleryPreviewPoll(void) {}
float PortHdTextUiAdvance(SDL_Renderer*r,int c,float s){return s;}
void PortHdTextUiGlyph(SDL_Renderer*r,int c,float x,float y,float s,float a,uint32_t f,uint32_t b){glyphCalls++;}
int PortHdMenuLightDraw(SDL_Renderer*r,float x,float y,float z,uint64_t t,float s,float w){return 1;}
uint64_t PortUiMenuLightTicks(void){return 0;}
'''
for signature in ('int PortSceneGalleryPreviewDraw(','void PortSceneGalleryPreviewCancel('):unit+=function(gallery,signature)+'\n'
for signature in ('static void Pixel(','static uint32_t Mix(','static float Advance(','void PortUiHdDraw(','static void Shade(','static void Fill('):unit+=function(ui,signature)+'\n'
unit+=r'''
static unsigned pixel(SDL_Renderer*r,int x,int y){SDL_Surface*a=SDL_RenderReadPixels(r,NULL);assert(a);SDL_Surface*b=SDL_ConvertSurface(a,SDL_PIXELFORMAT_ARGB8888);assert(b);unsigned c=((unsigned*)((char*)b->pixels+y*b->pitch))[x]&0xFFFFFF;SDL_DestroySurface(b);SDL_DestroySurface(a);return c;}
static void queue(void){sHdUiCount=2;sHdUiGlyphs[0]=(typeof(sHdUiGlyphs[0])){-2,0,0,1,640,360,0xFFFFFFFF,0xFF000000};sHdUiGlyphs[1]=(typeof(sHdUiGlyphs[0])){'X',30,30,1,8,99,0xFF000000,0xFFFFFFFF};glyphCalls=0;}
static void draw(SDL_Renderer*r){SDL_SetRenderDrawColor(r,255,255,255,255);SDL_RenderClear(r);PortUiHdDraw(r,0,0,1);assert(glyphCalls==1);}
int main(){assert(SDL_Init(SDL_INIT_VIDEO));SDL_Window*w=SDL_CreateWindow("poster",640,360,SDL_WINDOW_HIDDEN);assert(w);SDL_Renderer*r=SDL_CreateRenderer(w,"software");assert(r);
 for(int y=0;y<360;y++)for(int x=0;x<640;x++)sPreview[y*640+x]=x&1?0xFFEE8844:0xFF1155AA;
 queue();draw(r);assert(pixel(r,100,100)==0x1155AA&&pixel(r,101,100)==0xEE8844); /* preserves adjacent source pixels, not a 90px canvas */
 queue();Shade(0,0,640,360,0,256);draw(r);assert(pixel(r,100,100)==0);
 queue();Shade(0,0,640,360,0xFFFFFF,256);draw(r);assert(pixel(r,100,100)==0xFFFFFF);
 queue();Shade(0,0,640,360,0,128);draw(r);unsigned c=pixel(r,100,100);assert((c&255)>=83&&(c&255)<=86);
 queue();Fill(100,100,20,20,0);assert(!sHdUiGlyphs[0].alive);draw(r);assert(pixel(r,101,100)==0xFFFFFF); /* modal middle intersects, even though top-left lies outside */
 SDL_Window*w2=SDL_CreateWindow("new renderer",640,360,SDL_WINDOW_HIDDEN);SDL_Renderer*r2=SDL_CreateRenderer(w2,"software");assert(r2);queue();draw(r2);assert(sPreviewRenderer==r2&&pixel(r2,101,100)==0xEE8844);
 PortSceneGalleryPreviewCancel();assert(!sPreviewTexture&&!sPreviewRenderer&&!sPreviewReady&&sPreviewEvent==-1);
 SDL_DestroyRenderer(r2);SDL_DestroyWindow(w2);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();puts("PASS poster source resolution, text markers, black/white/half fades, partial modal occlusion, renderer change and cleanup");}
'''
(OUT/'probe.c').write_text(unit,encoding='utf-8')
tc=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang.exe';sdl=ROOT.parent/'toolchain/SDL3-3.4.18/i686-w64-mingw32'
subprocess.run([str(tc),'-std=gnu11','-O1','-I'+str(sdl/'include'),str(OUT/'probe.c'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(OUT/'probe.exe')],check=True)
subprocess.run([str(OUT/'probe.exe')],env=dict(os.environ,PATH=str(sdl/'bin')+';'+os.environ['PATH'],SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy'),check=True)
