"""Preview A->B upload rejection using production guest upload and host slots.

The renderer itself is an explicit CPU adapter: it consumes the actual HostTex
handle. This is an ABI/ownership contract, not GPU or latency evidence.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
from test_preview_cancellation_contract import function


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    a = p.parse_args()
    root = Path(__file__).resolve().parents[2]
    guest = (root/'port/ps5/guest/src/sdl_shim.c').read_text(encoding='utf8')
    gallery = (root/'port/src/port_scene_gallery.c').read_text(encoding='utf8')
    code = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"failed: %s\n",#c);exit(97);}}while(0)
typedef struct {int pixel,alive;} HostTexture;
static void* sRenderer;
static uint32_t sRendererGeneration;
static int failUpload,hostCreates,hostDestroys,observed;
static HostTexture pool[32];
static HostTexture* HostCreate(void*r,int f,int a,int w,int h){(void)r;(void)f;(void)a;(void)w;(void)h;HostTexture*t=&pool[hostCreates++];t->alive=1;return t;}
static bool HostUpdate(HostTexture*t,void*r,const void*p,int pitch){(void)r;(void)pitch;if(failUpload)return false;t->pixel=*(const uint8_t*)p;return true;}
static void HostDestroy(HostTexture*t){CHECK(t->alive);t->alive=0;hostDestroys++;}
static void HostBlend(HostTexture*t,int m){(void)t;(void)m;}
#define SDL_Texture HostTexture
#define SDL_CreateTexture HostCreate
#define SDL_UpdateTexture HostUpdate
#define SDL_DestroyTexture HostDestroy
#define SDL_SetTextureBlendMode HostBlend
#define SDL_PIXELFORMAT_ABGR8888 0
#define SDL_TEXTUREACCESS_STATIC 0
#define SDL_BLENDMODE_BLEND 1
#include "host_texture_slots.h"
#undef SDL_Texture
#undef SDL_CreateTexture
#undef SDL_UpdateTexture
#undef SDL_DestroyTexture
#undef SDL_SetTextureBlendMode
typedef struct {int w,h,refcount;} SDL_Texture;
typedef struct {int unused;} SDL_Renderer;
typedef struct {int x,y,w,h;} SDL_Rect;
typedef struct {float x,y,w,h;} SDL_FRect;
typedef int SDL_PixelFormat;
typedef int SDL_TextureAccess;
typedef int SDL_ScaleMode;
typedef int SDL_BlendMode;
typedef uint8_t Uint8;
typedef struct {float sx,sy;} TargetState;
static uint32_t sTextureEpoch;
#define SDL_PIXELFORMAT_ARGB8888 1
#define SDL_PIXELFORMAT_XRGB8888 2
#define SDL_PIXELFORMAT_RGBA32 3
#define SDL_PIXELFORMAT_BGRA32 4
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_TEXTUREACCESS_TARGET 2
#define SDL_BLENDMODE_NONE 0
#define SDL_SCALEMODE_LINEAR 1
#define SDL_SCALEMODE_NEAREST 0
#define BRIDGE_GPU_CREATE 1
#define BRIDGE_GPU_SMOOTH 2
#define SetError(...) ((void)0)
static int GpuTargets(void){return 0;}
static int h_thread(int o,uint32_t a,uint32_t b,uint32_t c,uint32_t d){(void)o;(void)a;(void)b;(void)c;(void)d;return -1;}
static int h_tex_create(const void*p,int w,int h,int s){return H_tex_create(p,w,h,s);}
static void h_tex_update(int id,const void*p){H_tex_update(id,p);}
static void h_tex_free(int id){H_tex_free(id);}
static bool UnbindTexture(SDL_Texture*t){(void)t;return true;}
'''
    start = guest.index('typedef struct Tex {')
    stop = guest.index('SDL_Texture* SDL_CreateTextureFromSurface(', start)
    code += guest[start:stop]
    for signature in ['void SDL_DestroyTexture(SDL_Texture* p)', 'bool SDL_UpdateTexture(SDL_Texture* p, const SDL_Rect* rect, const void* pixels, int pitch)', 'static int HostTex(Tex* t, bool smooth)']:
        code += function(guest, signature)
    code += r'''
static bool SDL_SetTextureBlendMode(SDL_Texture*t,int m){((Tex*)t)->blend=m;return true;}
static bool SDL_SetTextureScaleMode(SDL_Texture*t,int m){((Tex*)t)->scale=m;return true;}
static bool SDL_RenderTexture(SDL_Renderer*r,SDL_Texture*p,const SDL_FRect*s,const SDL_FRect*d){
 (void)r;(void)s;(void)d;int id=HostTex((Tex*)p,((Tex*)p)->scale!=SDL_SCALEMODE_NEAREST);
 if(id<0)return false;int slot=HostTextureSlot(id);CHECK(slot>=0);observed=sTex[slot]->pixel;return true;
}
static bool SDL_SetRenderDrawBlendMode(SDL_Renderer*r,int m){(void)r;(void)m;return true;}
static bool SDL_SetRenderDrawColor(SDL_Renderer*r,int a,int b,int c,int d){(void)r;(void)a;(void)b;(void)c;(void)d;return true;}
static bool SDL_RenderFillRect(SDL_Renderer*r,const SDL_FRect*d){(void)r;(void)d;return true;}
#define SDL_clamp(x,l,h) ((x)<(l)?(l):(x)>(h)?(h):(x))
#define SDL_min(x,y) ((x)<(y)?(x):(y))
static void PreviewTrace(const char*s){(void)s;}
static void PreviewTraceDraw(int a){(void)a;}
static SDL_Texture*sPreviewTexture;
static SDL_Renderer*sPreviewRenderer;
static int sPreviewReady=1,sPreviewW=2,sPreviewH=2,sTextureW,sTextureH,sTextureSerial=-1,sPreviewSerial=1,sPreviewHd=1,sPreviewFallback;
static uint32_t sPreview[4];
'''
    code += function(gallery, 'int PortSceneGalleryPreviewDraw(SDL_Renderer* renderer, float x, float y, float w, float h, uint32_t face, uint32_t shadow)')
    code += r'''
int main(void){SDL_Renderer r;for(int i=0;i<4;i++)sPreview[i]=0xff00000bu;
 failUpload=1;CHECK(!PortSceneGalleryPreviewDraw(&r,0,0,4,4,0,0));
 failUpload=0;CHECK(PortSceneGalleryPreviewDraw(&r,0,0,4,4,0,0));CHECK(observed==0);
 /* ARGB->RGBA conversion: distinct red channel bytes expose stale A. */
 for(int i=0;i<4;i++)sPreview[i]=0xff160000u;sPreviewSerial++;
 failUpload=1;int accepted=PortSceneGalleryPreviewDraw(&r,0,0,4,4,0,0);
 CHECK(!accepted||observed==22);
 failUpload=0;CHECK(PortSceneGalleryPreviewDraw(&r,0,0,4,4,0,0));CHECK(observed==22);
 int creates=hostCreates;CHECK(PortSceneGalleryPreviewDraw(&r,0,0,4,4,0,0));CHECK(hostCreates==creates);
 CHECK(hostDestroys>=1);puts("PASS first creation rejection and A->B failed upload cannot present stale A; retry and reuse");return 0;}
'''
    a.work.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='bridge-preview-', dir=a.work))
    (out/'probe.c').write_text(code, encoding='utf8')
    subprocess.run([str(a.cc), '-std=c11', '-O0', '-DPORT_PS5_GUEST', '-I'+str(root/'port/ps5/shared'), str(out/'probe.c'), '-o', str(out/'probe.exe')], check=True)
    r = subprocess.run([str(out/'probe.exe')], capture_output=True, timeout=10)
    (out/'run.log').write_bytes(r.stdout+r.stderr)
    print((r.stdout+r.stderr).decode(errors='replace'), end='')
    if r.returncode:
        raise SystemExit(r.returncode)


if __name__ == '__main__':
    main()
