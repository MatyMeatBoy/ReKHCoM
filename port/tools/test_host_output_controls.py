"""CPU fault-injection of actual guest setters and shared SDL host commands."""
from pathlib import Path
import argparse, importlib.util, json, os, subprocess

ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    import re
    match=re.search(r'^.*\b'+name+r'\([^\n]*?\)\s*\{',source,re.M)
    if not match:raise RuntimeError('Missing production function '+name)
    end=match.end();depth=1
    while depth:
        if source[end]=='{':depth+=1
        elif source[end]=='}':depth-=1
        end+=1
    return source[match.start():end]

def run(out):
    out.mkdir(parents=True,exist_ok=True)
    source=(ROOT/'port/ps5/guest/src/sdl_shim.c').read_text()
    names=['SDL_CreateWindow','SDL_SetWindowSize','SDL_SyncWindow','SDL_GetWindowSizeInPixels','SDL_SetRenderVSync','SDL_GetRenderVSync']
    parts=[function(source,name) for name in names]
    helper=function(source,'OutputCapability') if 'static int OutputCapability(' in source else ''
    protocol=(ROOT/'port/ps5/shared/bridge_output.h').exists()
    code=r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
typedef uint64_t Uint64;
typedef struct {int w,h;} SDL_Window;
typedef struct {int vsync;} SDL_Renderer;
static SDL_Window hostObject={1280,720}, *hostWindow=&hostObject;
static SDL_Renderer hostRendererObject, *hostRenderer=&hostRendererObject;
static int failSetWindow,ignoreWindowSet,failSync,failGetWindow,failSetVsync,ignoreVsyncSet,failGetVsync,windowCalls,vsyncCalls,actualVsync=1;
static bool HostSDLSetWindowSize(SDL_Window*w,int a,int b){windowCalls++;if(failSetWindow)return false;if(!ignoreWindowSet){w->w=a;w->h=b;}return true;}
static bool HostSDLSyncWindow(SDL_Window*w){(void)w;return !failSync;}
static bool HostSDLGetWindowSize(SDL_Window*w,int*a,int*b){*a=w->w;*b=w->h;return !failGetWindow;}
static bool HostSDLGetWindowSizeInPixels(SDL_Window*w,int*a,int*b){*a=w->w;*b=w->h;return !failGetWindow;}
static bool HostSDLSetRenderVSync(SDL_Renderer*r,int n){(void)r;vsyncCalls++;if(failSetVsync)return false;if(!ignoreVsyncSet)actualVsync=n;return true;}
static bool HostSDLGetRenderVSync(SDL_Renderer*r,int*n){(void)r;*n=actualVsync;return !failGetVsync;}
#define SDL_SetWindowSize HostSDLSetWindowSize
#define SDL_SyncWindow HostSDLSyncWindow
#define SDL_GetWindowSize HostSDLGetWindowSize
#define SDL_GetWindowSizeInPixels HostSDLGetWindowSizeInPixels
#define SDL_SetRenderVSync HostSDLSetRenderVSync
#define SDL_GetRenderVSync HostSDLGetRenderVSync
#define sWindow hostWindow
#define sRenderer hostRenderer
#define Log(...) ((void)0)
HOST_HEADER
#undef SDL_SetWindowSize
#undef SDL_SyncWindow
#undef SDL_GetWindowSize
#undef SDL_GetWindowSizeInPixels
#undef SDL_SetRenderVSync
#undef SDL_GetRenderVSync
#undef sWindow
#undef sRenderer
static int oldHost,wrongMagic;
static int h_thread(int op,uint32_t a,uint32_t b,uint32_t c,uint32_t d){if(oldHost)return wrongMagic?1:0;return HostOutputCommand(op,a,b,c,d);}
static void h_gfx_size(int*a,int*b){*a=1280;*b=720;}
static SDL_Window sWindow;
bool SDL_SetWindowSize(SDL_Window*,int,int);
HELPER
GUEST
static int checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"failed line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 SDL_Renderer renderer={9};int value=123,a=0,b=0;
 oldHost=1;
 CHECK(!SDL_SetWindowSize(&sWindow,1920,1080));
 CHECK(!SDL_SetRenderVSync(&renderer,0)&&renderer.vsync==9);
 CHECK(!SDL_GetRenderVSync(&renderer,&value)&&value==123);
 CHECK(windowCalls==0&&vsyncCalls==0);
 CHECK(SDL_CreateWindow("legacy",1920,1080,0)==&sWindow);
 wrongMagic=1;CHECK(!SDL_SetRenderVSync(&renderer,0));oldHost=0;
 CHECK(HostOutputCommand(0x210,0,0,0,0)==-2);
 CHECK(HostOutputCommand(0x220,0,0,0,0)==-2);
 CHECK(HostOutputCommand(BRIDGE_OUTPUT_PROBE,0,0,0,0)==BRIDGE_OUTPUT_MAGIC);
 CHECK(SDL_SetRenderVSync(&renderer,0)&&actualVsync==0&&renderer.vsync==0);
 CHECK(SDL_GetRenderVSync(&renderer,&value)&&value==0);
 CHECK(SDL_SetRenderVSync(&renderer,1)&&SDL_GetRenderVSync(&renderer,&value)&&value==1);
 ignoreVsyncSet=1;CHECK(!SDL_SetRenderVSync(&renderer,0)&&renderer.vsync==1);ignoreVsyncSet=0;
 failSetVsync=1;CHECK(!SDL_SetRenderVSync(&renderer,0)&&renderer.vsync==1);failSetVsync=0;
 failGetVsync=1;CHECK(!SDL_SetRenderVSync(&renderer,0));CHECK(!SDL_GetRenderVSync(&renderer,&value));failGetVsync=0;
 CHECK(!SDL_SetRenderVSync(&renderer,-2));
 if(HOST_OUTPUT_WINDOW){
  CHECK(SDL_SetWindowSize(&sWindow,1920,1080));CHECK(hostObject.w==1920&&hostObject.h==1080);
  CHECK(SDL_GetWindowSizeInPixels(&sWindow,&a,&b)&&a==1920&&b==1080);
  CHECK(SDL_CreateWindow("normal",1600,900,0)==&sWindow&&hostObject.w==1600&&hostObject.h==900);
  ignoreWindowSet=1;CHECK(!SDL_SetWindowSize(&sWindow,1000,700)&&sWindow.w==1600);ignoreWindowSet=0;
  failSetWindow=1;CHECK(!SDL_SetWindowSize(&sWindow,1000,700));failSetWindow=0;
  CHECK(SDL_SyncWindow(&sWindow));
  failSync=1;CHECK(!SDL_SetWindowSize(&sWindow,1000,700));CHECK(!SDL_SyncWindow(&sWindow));failSync=0;
  failGetWindow=1;CHECK(!SDL_SetWindowSize(&sWindow,1000,700));CHECK(!SDL_GetWindowSizeInPixels(&sWindow,&a,&b));failGetWindow=0;
  CHECK(!SDL_SetWindowSize(&sWindow,0,700));CHECK(!SDL_SetWindowSize(&sWindow,-1,700));
 }else{
  CHECK(!SDL_SetWindowSize(&sWindow,1920,1080));CHECK(windowCalls==0);
  CHECK(SDL_CreateWindow("fixed",1920,1080,0)==&sWindow&&windowCalls==0);
 }
 hostWindow=0;CHECK(HostOutputCommand(BRIDGE_OUTPUT_PROBE,0,0,0,0)==0);hostWindow=&hostObject;
 hostRenderer=0;CHECK(!SDL_SetRenderVSync(&renderer,0));hostRenderer=&hostRendererObject;
 printf("{\"checks\":%d,\"window_capability\":%d}\n",checks,HOST_OUTPUT_WINDOW);return 0;
}
'''
    fallback='''#define HOST_OUTPUT_WINDOW 1
#define BRIDGE_OUTPUT_PROBE 0x230
#define BRIDGE_OUTPUT_MAGIC 0x4f555431
static int HostOutputCommand(int op,uint32_t a,uint32_t b,uint32_t c,uint32_t d){return -2;}'''
    code=code.replace('HOST_HEADER','#include "host_output_sdl.h"' if protocol else fallback).replace('HELPER',helper).replace('GUEST','\n'.join(parts))
    src=out/'probe.c';src.write_text(code)
    compiler=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
    reports=[]
    for desktop in (1,0):
        exe=out/f'probe-{desktop}.exe'
        subprocess.run([str(compiler),'-O2',f'-DHOST_OUTPUT_WINDOW={desktop}',str(src),'-I'+str(ROOT/'port/ps5/shared'),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr.strip())
        reports.append(json.loads(result.stdout))
    (out/'RESULTS.json').write_text(json.dumps(reports,indent=2));print(json.dumps(reports))

def visibility(out):
    # Compile the exact visibility case emitted by the real producer, rather
    # than mirroring its condition in the test. Both desktop and console paths.
    compiler=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
    for native in (False,True):
        previous=os.environ.get('REKHCOM_GUEST_LINUX64')
        if native:os.environ['REKHCOM_GUEST_LINUX64']='1'
        else:os.environ.pop('REKHCOM_GUEST_LINUX64',None)
        try:
            spec=importlib.util.spec_from_file_location('output_producer',ROOT/'port/ps5/build_ps5.py')
            producer=importlib.util.module_from_spec(spec);spec.loader.exec_module(producer)
            patched=producer.patched_source(ROOT/'port/src/port_ui.c',out/f'patched-{native}',False).read_text(encoding='utf-8')
        finally:
            if previous is None:os.environ.pop('REKHCOM_GUEST_LINUX64',None)
            else:os.environ['REKHCOM_GUEST_LINUX64']=previous
        start=patched.index('case ITEM_VSYNC:',patched.index('static int ItemVisible(int item) {'))
        case=patched[start:patched.index('case ITEM_INTZOOM:',start)]
        src=out/f'visibility-{native}.c';exe=src.with_suffix('.exe')
        src.write_text('''#include <stdio.h>
#include <stddef.h>
typedef struct {int x;} SDL_Renderer;
static SDL_Renderer renderer;static int available;
static SDL_Renderer* SDL_GetRenderer(void*p){return &renderer;}
static int SDL_GetRenderVSync(SDL_Renderer*r,int*v){*v=0;return available;}
enum{ITEM_VSYNC=1};
static int visible(int item){switch(item){'''+case+'''default:return 0;}}
int main(void){available=0;if(visible(1))return 1;available=1;if(!visible(1)){fprintf(stderr,"supported VSync row hidden\\n");return 2;}return 0;}
''')
        subprocess.run([str(compiler),'-O2',str(src),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stderr)
    print('Actual producer VSync visibility: supported/unsupported desktop+console PASS')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--work',type=Path,default=ROOT/'build/host-output-contract');out=parser.parse_args().work.resolve();run(out);visibility(out)
