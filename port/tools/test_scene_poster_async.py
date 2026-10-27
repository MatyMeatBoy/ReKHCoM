"""Exercise actual cache/job lifecycle with a slow one-frame decoder and no media/ROM copies."""
import os,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];temporary=tempfile.TemporaryDirectory(prefix='khcom-scene-poster-');OUT=Path(temporary.name)
source=(ROOT/'port/src/port_scene_gallery.c').read_text(encoding='utf-8');start=source.index('#define PREVIEW_W');end=source.index('/* Offline only:',start)
unit=r'''#include <SDL3/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "port/port_hd_poster.h"
static Uint64 ticks;
static SDL_AtomicInt calls,active,peak;
static Uint64 FakeTicks(void){return ticks;}
static bool DiscardPng(SDL_Surface*s,const char*p){return true;}
#define SDL_GetTicks FakeTicks
#define SDL_SavePNG DiscardPng
static const char* PortConfigPath(const char*n){return "Z:/poster-probe-never-created/";}
static void PortLog(const char*format,...){(void)format;}
const char* PortHdGalleryMovieName(int id){return id==10?"first":"latest";}
int PortHdGalleryMovieOffset(int id){return 0;}
static int generation;
int PortHdScenePreviewPath(const char*name,char*out,size_t size){if(generation)SDL_snprintf(out,size,"%s-g%d",name,generation);else SDL_strlcpy(out,name,size);return 1;}
int PortHdMovieResolvePath(const char*name,char*out,size_t size){return PortHdScenePreviewPath(name,out,size);}
int PortHdScenePreviewFile(const char*path,uint32_t*out,int w,int h,int second){int n=SDL_AddAtomicInt(&active,1)+1;assert(n==1);SDL_SetAtomicInt(&peak,n);SDL_AddAtomicInt(&calls,1);SDL_Delay(400);for(int i=0;i<w*h;i++)out[i]=!SDL_strcmp(path,"first")?0xFFAA0000:0xFF0000BB;SDL_AddAtomicInt(&active,-1);return 1;}
int PortScenePosterPreview(int event,uint32_t*out,int*w,int*h){return 0;}
int PortScenePosterDecode(const void*data,size_t n,uint32_t*out,int*w,int*h){return 0;}
int PortSceneBackgroundPreview(int event,uint32_t*out,int w,int h){return 0;}
'''+source[start:end]+r'''
int main(){assert(SDL_Init(0));ticks=0;assert(!PortSceneGalleryPreview(10,1));ticks=100;assert(!PortSceneGalleryPreview(10,1));for(int i=0;i<100&&!SDL_GetAtomicInt(&active);i++)SDL_Delay(1);assert(SDL_GetAtomicInt(&active)==1);
Uint64 t=SDL_GetPerformanceCounter();PortSceneGalleryPreviewCancel();double ms=(SDL_GetPerformanceCounter()-t)*1000.0/SDL_GetPerformanceFrequency();assert(ms<50);assert(sPreviewJob.thread);assert(!sPreviewReady);
ticks=200;assert(!PortSceneGalleryPreview(11,1));assert(SDL_GetAtomicInt(&calls)==1);SDL_Delay(450);PortSceneGalleryPreviewPoll();assert(!sPreviewReady&&!sPreviewJob.thread); /* discarded stale red poster */
ticks=300;assert(!PortSceneGalleryPreview(11,1));SDL_Delay(450);PortSceneGalleryPreviewPoll();assert(sPreviewReady&&sPreview[0]==0xFF0000BB);assert(SDL_GetAtomicInt(&calls)==2&&SDL_GetAtomicInt(&peak)==1);assert(!sPreviewJob.thread&&!sPreviewJob.pixels);
generation=1;ticks=400;assert(!PortSceneGalleryPreview(11,1));assert(!sPreviewReady);ticks=500;assert(!PortSceneGalleryPreview(11,1));SDL_Delay(450);PortSceneGalleryPreviewPoll();assert(sPreviewReady&&SDL_GetAtomicInt(&calls)==3);assert(SDL_strstr(sPreviewMediaPath,"-g1"));
PortSceneGalleryPreviewCancel();assert(!sPreviewReady&&!sPreviewTexture);SDL_Quit();printf("PASS Close %.3f ms while 400ms decoder pending; stale selection/generation discarded, one worker/cache, all jobs reaped\n",ms);}
'''
(OUT/'probe.c').write_text(unit,encoding='utf-8');tc=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang.exe';sdl=ROOT.parent/'toolchain/SDL3-3.4.18/i686-w64-mingw32'
subprocess.run([str(tc),'-std=gnu11','-O1','-I'+str(sdl/'include'),'-I'+str(ROOT/'port/include'),str(OUT/'probe.c'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(OUT/'probe.exe')],check=True)
subprocess.run([str(OUT/'probe.exe')],env=dict(os.environ,PATH=str(sdl/'bin')+';'+os.environ['PATH']),check=True)
temporary.cleanup()
