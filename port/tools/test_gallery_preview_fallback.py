"""Exercise actual cache/job lifecycle with a slow one-frame decoder and no media/ROM copies."""
import os,subprocess
from pathlib import Path
import tempfile
ROOT=Path(__file__).resolve().parents[2];_temp=tempfile.TemporaryDirectory(prefix='gallery-fallback-');OUT=Path(_temp.name)
source=(ROOT/'port/src/port_scene_gallery.c').read_text(encoding='utf-8');start=source.index('#define PREVIEW_W');end=source.index('/* Offline only:',start)
unit=r'''#include <SDL3/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "port/port_hd_poster.h"
static Uint64 ticks;
static SDL_AtomicInt calls,active,peak;
static int posterCalls,backgroundCalls,failDecode,failAlloc;
static void* ProbeAlloc(size_t size){return failAlloc?NULL:SDL_malloc(size);}
#define SDL_malloc ProbeAlloc
static Uint64 FakeTicks(void){return ticks;}
static bool DiscardPng(SDL_Surface*s,const char*p){return true;}
#define SDL_GetTicks FakeTicks
#define SDL_SavePNG DiscardPng
static const char* PortConfigPath(const char*n){return "Z:/poster-probe-never-created/";}
static void PortLog(const char*format,...){(void)format;}
const char* PortHdGalleryMovieName(int id){return id==10?"first":"latest";}
int PortHdGalleryMovieOffset(int id){return 0;}
int PortHdScenePreviewPath(const char*name,char*out,size_t size){SDL_strlcpy(out,name,size);return 1;}
int PortHdMovieResolvePath(const char*name,char*out,size_t size){return PortHdScenePreviewPath(name,out,size);}
int PortHdScenePreviewFile(const char*path,uint32_t*out,int w,int h,int second){int n=SDL_AddAtomicInt(&active,1)+1;assert(n==1);SDL_SetAtomicInt(&peak,n);SDL_AddAtomicInt(&calls,1);SDL_Delay(400);for(int i=0;i<w*h;i++)out[i]=!SDL_strcmp(path,"first")?0xFFAA0000:0xFF0000BB;SDL_AddAtomicInt(&active,-1);return !failDecode;}
int PortScenePosterPreview(int event,uint32_t*out,int*w,int*h){posterCalls++;if(event>=40){*w=640;*h=360;for(int i=0;i<640*360;i++)out[i]=0xFF00AA00;return 1;}if(event!=10)return 0;*w=*h=2;for(int i=0;i<4;i++)out[i]=0xFF00AA00;return 1;}
int PortScenePosterDecode(const void*data,size_t n,uint32_t*out,int*w,int*h){return 0;}
int PortSceneBackgroundPreview(int event,uint32_t*out,int w,int h){backgroundCalls++;for(int i=0;i<w*h;i++)out[i]=0xFFAAAA00;return 1;}
'''+source[start:end]+r'''
int main(){assert(SDL_Init(0));ticks=0;
assert(PreviewGba(10));int n=posterCalls;assert(PreviewGba(10)&&posterCalls==n);
for(int e=20;e<31;e++){assert(PreviewGba(e));assert(sGbaPreviewBytes<=GBA_PREVIEW_CACHE_BYTES);}
n=posterCalls;assert(PreviewGba(30)&&posterCalls==n);assert(PreviewGba(10)&&posterCalls>n);
assert(PreviewGba(40)&&sGbaPreviewBytes<=GBA_PREVIEW_CACHE_BYTES);assert(PreviewGba(41)&&sGbaPreviewBytes<=GBA_PREVIEW_CACHE_BYTES);
PortSceneGalleryPreviewCacheClear();assert(!sGbaPreviewBytes);
failAlloc=1;assert(PreviewGba(10)&&!sGbaPreviewBytes);failAlloc=0;
assert(PortSceneGalleryPreview(10,1) && sPreview[0]==0xFF00AA00); /* immediate clean GBA poster */
ticks=100;assert(PortSceneGalleryPreview(10,1));for(int i=0;i<100&&!SDL_GetAtomicInt(&active);i++)SDL_Delay(1);assert(SDL_GetAtomicInt(&active)==1);
Uint64 t=SDL_GetPerformanceCounter();PortSceneGalleryPreviewCancel();double ms=(SDL_GetPerformanceCounter()-t)*1000.0/SDL_GetPerformanceFrequency();assert(ms<50);assert(sPreviewJob.thread);
ticks=200;assert(PortSceneGalleryPreview(11,1)&&sPreview[0]==0xFFAAAA00); /* fallback ROM background */
SDL_Delay(450);PortSceneGalleryPreviewPoll();assert(sPreviewReady&&sPreview[0]==0xFFAAAA00&&!sPreviewJob.thread);
ticks=300;assert(PortSceneGalleryPreview(11,1));SDL_Delay(450);PortSceneGalleryPreviewPoll();assert(sPreviewReady&&sPreview[0]==0xFF0000BB);assert(SDL_GetAtomicInt(&calls)==2&&SDL_GetAtomicInt(&peak)==1);assert(!sPreviewJob.thread&&!sPreviewJob.pixels);
PortSceneGalleryPreviewCancel();assert(!sPreviewReady&&!sPreviewTexture);
failDecode=1;ticks=500;assert(PortSceneGalleryPreview(12,1));ticks=600;assert(PortSceneGalleryPreview(12,1));
SDL_Delay(450);PortSceneGalleryPreviewPoll();assert(sPreviewReady&&sPreviewFallback&&sPreview[0]==0xFFAAAA00);
assert(SDL_GetAtomicInt(&calls)==3);ticks=700;assert(PortSceneGalleryPreview(12,1));assert(SDL_GetAtomicInt(&calls)==3);
PortSceneGalleryPreviewCancel();PortSceneGalleryPreviewCacheClear();assert(!sGbaPreviewBytes);SDL_Quit();printf("PASS immediate GBA poster/ROM background while slow HD worker runs; close %.3f ms, stale discarded, one worker\n",ms);}

'''
(OUT/'probe.c').write_text(unit,encoding='utf-8');base=next(p for p in ROOT.parents if (p/'toolchain').is_dir());tc=base/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang.exe';sdl=base/'toolchain/SDL3-3.4.18/i686-w64-mingw32'
subprocess.run([str(tc),'-std=gnu11','-O1','-I'+str(sdl/'include'),'-I'+str(ROOT/'port/include'),str(OUT/'probe.c'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(OUT/'probe.exe')],check=True)
subprocess.run([str(OUT/'probe.exe')],env=dict(os.environ,PATH=str(sdl/'bin')+';'+os.environ['PATH']),check=True)
