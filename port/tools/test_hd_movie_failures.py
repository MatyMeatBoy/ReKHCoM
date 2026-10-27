"""Fault injection against the Android production texture upload block (no device)."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'port/portable/hdmovie_android.c').read_text(encoding='utf-8')
start = source.index('                if (w > 0 && h > 0 && (pu == 1 || nv)) {')
end = source.index('            }\n            M.imageDelete(image);', start)
block = source[start:end]
preamble = r'''
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
typedef int SDL_Texture;
typedef int SDL_PropertiesID;
#define SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER 0
#define SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER 0
#define SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER 0
#define SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER 0
#define SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER 0
#define SDL_PIXELFORMAT_ABGR8888 0
#define SDL_PIXELFORMAT_NV12 0
#define SDL_PIXELFORMAT_NV21 0
#define SDL_PIXELFORMAT_IYUV 0
#define SDL_TEXTUREACCESS_STREAMING 0
#define SDL_COLORSPACE_BT709_LIMITED 0
#define SDL_SCALEMODE_LINEAR 0
static int failCreate, failUpload, deletes, imageDeletes, uploads, tex;
static int SDL_CreateProperties(void){return 1;}
static void SDL_SetNumberProperty(int a,int b,int c){}
static SDL_Texture* SDL_CreateTextureWithProperties(void*a,int b){return failCreate?NULL:&tex;}
static void SDL_DestroyProperties(int a){}
static void SDL_DestroyTexture(SDL_Texture*t){deletes++;}
static void SDL_SetTextureScaleMode(SDL_Texture*t,int a){}
static const char* SDL_GetError(void){return "injected";}
#define HdMovieLog(...) ((void)0)
static void deleteImage(void*p){imageDeletes++;}
static struct {void(*imageDelete)(void*);} M={deleteImage};
static int update(void){uploads++;return !failUpload;}
#define YuvToRgb(...) update()
#define SDL_UpdateNVTexture(...) update()
#define SDL_UpdateYUVTexture(...) update()
static int run(int nv) {
 int w=2,h=2,pu=1,sy=2,su=1,sv=1,texW=0,texH=0,texNv12=-1,played=0;
 unsigned char pixels[16]={0}; const uint8_t *y=pixels,*u=pixels,*v=pixels;
 void*renderer=NULL;void*image=pixels;SDL_Texture*texture=NULL;
 struct {int left,top;} crop={0,0};
'''
cases = r'''
 done:
 if(texture)SDL_DestroyTexture(texture);
 return played;
}
int main(void){
 for(int nv=0;nv<4;nv++){
  failCreate=1;failUpload=0;deletes=imageDeletes=uploads=0;
  if(run(nv)||imageDeletes!=1||uploads||deletes)return 1;
  failCreate=0;failUpload=1;deletes=imageDeletes=uploads=0;
  if(run(nv)||imageDeletes!=1||uploads!=1||deletes!=1)return 2;
  failUpload=0;deletes=imageDeletes=uploads=0;
  if(!run(nv)||imageDeletes||uploads!=1||deletes!=1)return 3;
 }
 return 0;
}
'''
compiler = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
with tempfile.TemporaryDirectory(prefix='rekhcom-video-failures-') as td:
    src=Path(td)/'test.c'; exe=Path(td)/'test.exe'
    src.write_text(preamble+block+cases,encoding='utf-8')
    subprocess.run([str(compiler),str(src),'-o',str(exe)],check=True,capture_output=True)
    subprocess.run([str(exe)],check=True,capture_output=True)

# Exercise each actual MF allocation guard with an injected failing allocator.
windows = (ROOT / 'port/src/port_hdmovie.c').read_text(encoding='utf-8')
play = windows[windows.index('static int PlayFile(const char* name) {'):]
blocks=[]
for call in ['hr = MFCreateAttributes(&attrs, 1);', 'hr = MFCreateMediaType(&type);']:
    pos=0
    while True:
        pos=play.find(call,pos)
        if pos<0:break
        end=play.index('goto done;',pos)+len('goto done;')
        blocks.append(play[pos:end]);pos=end
mf_header=r'''
#include <stddef.h>
#define FAILED(hr) ((hr)<0)
static int fail, alive;
static int alloc(void**out){if(fail)return -1;*out=&alive;alive++;return 0;}
#define MFCreateAttributes(p,n) alloc(p)
#define MFCreateMediaType(p) alloc(p)
'''
mf_functions=''
for i,block in enumerate(blocks):
    mf_functions+=f'static int check{i}(void){{void*attrs=NULL;void*type=NULL;int hr;\n'+block+'\n alive--; return 1; done: return 0;}\n'
mf_main='int main(void){\n'
for i in range(len(blocks)):
    mf_main+=f'fail=1;alive=0;if(check{i}()||alive)return 1;fail=0;if(!check{i}()||alive)return 2;\n'
mf_main+='return 0;}\n'
assert len(blocks)==3, 'all MF playback creation sites must have guards'
with tempfile.TemporaryDirectory(prefix='rekhcom-mf-failures-') as td:
    src=Path(td)/'test.c';exe=Path(td)/'test.exe'
    src.write_text(mf_header+mf_functions+mf_main,encoding='utf-8')
    subprocess.run([str(compiler),str(src),'-o',str(exe)],check=True,capture_output=True)
    subprocess.run([str(exe)],check=True,capture_output=True)

# No image, texture or upload must ever count as a completed HD replacement.
setup = source[source.index('int HdMovieAndroidPlay('):start]
assert 'played = 1;' not in setup
assert 'if (played)\n        FreeSubs(&subs);' not in source
watch_start = source.index('        if (!played && SDL_GetTicksNS() - firstPictureStart')
watch_end = source.index('\n        {', watch_start)
watch = source[watch_start:watch_end]
watch_src = r"""
#include <stdint.h>
#define HdMovieLog(...) ((void)0)
static uint64_t now;
static uint64_t SDL_GetTicksNS(void){return now;}
static int expired(int played){uint64_t firstPictureStart=100;
""" + watch + r"""
 return 0; done:return 1;}
int main(void){now=5000000099ULL;if(expired(0))return 1;
now++;if(!expired(0)||expired(1))return 2;return 0;}
"""
with tempfile.TemporaryDirectory(prefix='rekhcom-frame-deadline-') as td:
    src=Path(td)/'test.c';exe=Path(td)/'test.exe'
    src.write_text(watch_src,encoding='utf-8')
    subprocess.run([str(compiler),str(src),'-o',str(exe)],check=True,capture_output=True)
    subprocess.run([str(exe)],check=True,capture_output=True)
