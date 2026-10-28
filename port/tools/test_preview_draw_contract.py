"""Exercise production preview upload/draw acceptance with a fault-injected SDL adapter."""
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
    repo = Path(__file__).resolve().parents[2]
    source = (repo/'port/src/port_scene_gallery.c').read_text(encoding='utf8')
    menu = (repo/'port/src/port_scene_gallery_menu.inc').read_text(encoding='utf8')
    code = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define CHECK(c) do { if(!(c)){fprintf(stderr,"failed: %s\n",#c);exit(97);} } while(0)
typedef struct { int unused; } SDL_Renderer;
typedef struct { int unused; } SDL_Texture;
typedef struct { float x,y,w,h; } SDL_FRect;
#define SDL_PIXELFORMAT_ARGB8888 0
#define SDL_TEXTUREACCESS_STREAMING 0
#define SDL_BLENDMODE_NONE 0
#define SDL_BLENDMODE_BLEND 1
#define SDL_SCALEMODE_LINEAR 0
#define SDL_SCALEMODE_NEAREST 1
#define SDL_clamp(x,l,h) ((x)<(l)?(l):(x)>(h)?(h):(x))
#define SDL_min(x,y) ((x)<(y)?(x):(y))
static SDL_Texture texture,*sPreviewTexture;
static SDL_Renderer *sPreviewRenderer;
static int sPreviewReady=1,sPreviewW=2,sPreviewH=2,sTextureW,sTextureH;
static int sPreviewSerial=4,sTextureSerial=-1,sPreviewHd=1,sPreviewFallback;
static uint32_t sPreview[4];
static int failCreate,failUpload,failDraw,uploads,draws;
static void SDL_DestroyTexture(SDL_Texture*t){(void)t;}
static SDL_Texture* SDL_CreateTexture(SDL_Renderer*r,int f,int a,int w,int h){(void)r;(void)f;(void)a;(void)w;(void)h;return failCreate?NULL:&texture;}
static bool SDL_SetTextureBlendMode(SDL_Texture*t,int m){(void)t;(void)m;return true;}
static bool SDL_SetTextureScaleMode(SDL_Texture*t,int m){(void)t;(void)m;return true;}
static bool SDL_UpdateTexture(SDL_Texture*t,const void*r,const void*p,int pitch){(void)t;(void)r;(void)p;(void)pitch;uploads++;return !failUpload;}
static bool SDL_RenderTexture(SDL_Renderer*r,SDL_Texture*t,const void*s,const SDL_FRect*d){(void)r;(void)t;(void)s;(void)d;draws++;return !failDraw;}
static bool SDL_SetRenderDrawBlendMode(SDL_Renderer*r,int m){(void)r;(void)m;return true;}
static bool SDL_SetRenderDrawColor(SDL_Renderer*r,int a,int b,int c,int d){(void)r;(void)a;(void)b;(void)c;(void)d;return true;}
static bool SDL_RenderFillRect(SDL_Renderer*r,const SDL_FRect*d){(void)r;(void)d;return true;}
static uint32_t *sHdUiFrame,*sFb,canvas[16];
static int sHdUiCount,pixelWrites;
static struct { int code,alive,x,y,pw,run; uint32_t face,shadow; } sHdUiGlyphs[2048];
static void PortSceneGalleryPreviewSize(int*w,int*h){*w=sPreviewW;*h=sPreviewH;}
static void Pixel(int x,int y,uint32_t color){CHECK(x>=0&&x<4&&y>=0&&y<4);canvas[y*4+x]=color;pixelWrites++;}
static void PreviewTrace(const char* stage){(void)stage;}
static void PreviewTraceDraw(int accepted){(void)accepted;}
'''
    code += function(source, 'int PortSceneGalleryPreviewDraw(SDL_Renderer* renderer, float x, float y, float w, float h, uint32_t face, uint32_t shadow)')
    code += '\nstatic void Compose(int queue) { const uint32_t* preview=sPreview; int pw=4,ph=4,px,py,cardX=-6,top=-16; sFb=canvas;sHdUiFrame=queue?canvas:NULL;\n'
    code += function(menu, 'if (preview)') + '\n}\n'
    code += r'''
int main(void){SDL_Renderer r;
 failCreate=1;CHECK(!PortSceneGalleryPreviewDraw(&r,0,0,10,10,0,0));CHECK(!uploads&&!draws);
 failCreate=0;failUpload=1;CHECK(!PortSceneGalleryPreviewDraw(&r,0,0,10,10,0,0));CHECK(sTextureSerial==-1&&!draws);
 failUpload=0;CHECK(PortSceneGalleryPreviewDraw(&r,0,0,10,10,0,0));CHECK(uploads==2&&draws==1&&sTextureSerial==4);
 failDraw=1;CHECK(!PortSceneGalleryPreviewDraw(&r,0,0,10,10,0,0));CHECK(uploads==2&&draws==2);
 failDraw=0;CHECK(PortSceneGalleryPreviewDraw(&r,0,0,10,10,0,0));CHECK(uploads==2&&draws==3);
 sPreviewSerial++;CHECK(PortSceneGalleryPreviewDraw(&r,0,0,10,10,0,0));CHECK(uploads==3&&sTextureSerial==5);
 for(int i=0;i<4;i++)sPreview[i]=0xff112233u;
 Compose(1);CHECK(pixelWrites==16&&sHdUiCount==1);for(int i=0;i<16;i++)CHECK(canvas[i]==0xff112233u);
 pixelWrites=0;Compose(0);CHECK(pixelWrites==16&&sHdUiCount==1);
 puts("PASS create/upload/draw rejection, successful retry and generation upload");return 0;}
'''
    a.work.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='draw-', dir=a.work))
    (out/'probe.c').write_text(code, encoding='utf8')
    subprocess.run([str(a.cc), '-std=c11', '-O0', str(out/'probe.c'), '-o', str(out/'probe.exe')], check=True)
    r = subprocess.run([str(out/'probe.exe')], capture_output=True, timeout=10)
    (out/'run.log').write_bytes(r.stdout+r.stderr)
    print((r.stdout+r.stderr).decode(errors='replace'), end='')
    if r.returncode:
        raise SystemExit(r.returncode)
    loop = 'for (py = 0; py < drawH; py++) for (px = 0; px < drawW; px++)'
    assert code.count(loop) == 1
    (out/'broken.c').write_text(code.replace(loop, 'if (!sHdUiCount) '+loop), encoding='utf8')
    subprocess.run([str(a.cc), '-std=c11', '-O0', str(out/'broken.c'), '-o', str(out/'broken.exe')], check=True)
    broken = subprocess.run([str(out/'broken.exe')], capture_output=True, timeout=10)
    (out/'broken-underlay.log').write_bytes(broken.stdout+broken.stderr)
    assert broken.returncode == 97 and b'pixelWrites==16' in broken.stderr
    print('PASS negative control: missing queued-preview CPU underlay rejected')


if __name__ == '__main__':
    main()
