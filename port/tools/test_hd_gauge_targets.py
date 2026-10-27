"""Run the real gauge draw/cache against a counted renderer with allocation faults."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class GaugeTargetTests(unittest.TestCase):
    def test_budget_and_allocation_failure_preserve_draw_and_release_cache(self):
        source = (ROOT / 'port/src/port_hdlayout.c').read_text(encoding='utf-8')
        start = source.index('/* The gauge drawn supersampled')
        gpu = source[start:source.index('\nstatic void GaugeParts(', start)]
        start = source.index('void PortHdLayoutShutdown(void)')
        cleanup = source[start:source.index('\n/* ---- the player', start)]
        with tempfile.TemporaryDirectory() as temporary:
            temporary = Path(temporary)
            flags = ['-std=c99', '-Wall', '-Wextra']
            if os.environ.get('HD_LAYOUT_SANITIZE') == '1':
                flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
            for mobile in [False, True]:
                file = temporary / f'targets-{mobile}.c'
                file.write_text(PREAMBLE + gpu + cleanup + HARNESS, encoding='utf-8')
                exe = file.with_suffix('.exe')
                define = ['-D__ANDROID__=1'] if mobile else []
                subprocess.run([os.environ.get('CC', 'clang'), *flags, *define, str(file), '-o', str(exe)], check=True)
                for case in ['valid', 'budget', 'highdpi', 'maxsize', 'smallmax', 'unknownmax',
                             'overflow', 'intoverflow', 'nan', 'infinity', 'partial', 'cached_failure', 'allfail', 'resize', 'shutdown', 'bindfail', 'restorefail', 'bindordinal']:
                    with self.subTest(mobile=mobile, case=case):
                        run = subprocess.run([str(exe), case], capture_output=True, text=True, timeout=10)
                        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


PREAMBLE = r'''
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef struct {int w,h;uint64_t bytes;int refcount;} SDL_Texture;
typedef struct {int unused;} SDL_Renderer;
typedef struct {int x,y,w,h;} SDL_Rect;
typedef struct {float x,y,w,h;} SDL_FRect;
typedef int SDL_ScaleMode;
#define SDL_PIXELFORMAT_ABGR8888 1
#define SDL_TEXTUREACCESS_TARGET 1
#define SDL_BLENDMODE_NONE 0
#define SDL_BLENDMODE_BLEND_PREMULTIPLIED 2
#define SDL_SOFTWARE_RENDERER "software"
#define SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER "max_texture_size"
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_ceilf ceilf
#define SDL_strcmp strcmp
#define SDL_getenv(name) ((void)(name),(const char*)NULL)
static int64_t maxSize=16384;
static uint64_t liveBytes, peakBytes;
static int created, destroyed, attempts, failAt, failAll, failAbove, parts, direct, failBind, failRestore, bindCalls, bindFailAt;
static SDL_Texture* bound;
static SDL_Renderer renderer;
static struct {int active,frame,hp,maxHp,trail,stateAnim,faceAnim;} sGauge={1,1,50,100,50,15,3};
#define HD_LAYOUT_TEXTURE_COUNT 4
static struct {SDL_Texture*tex;}sTextures[4];
static int PortFrameCount(void){return 1;}
static int PortHdDrawLogged(void){return 0;}
#define PortLog(...) ((void)0)
static int PortSmoothScaleMode(void){return 1;}
static uint64_t budget(void){
#ifdef __ANDROID__
 return 12ull*1024*1024;
#else
 return 32ull*1024*1024;
#endif
}
static void check(int yes,const char*text){if(!yes){fprintf(stderr,"%s (live=%llu peak=%llu created=%d attempts=%d)\n",text,(unsigned long long)liveBytes,(unsigned long long)peakBytes,created,attempts);exit(1);}}
static uint32_t SDL_GetRendererProperties(SDL_Renderer*r){(void)r;return 1;}
static int64_t SDL_GetNumberProperty(uint32_t props,const char*name,int64_t fallback){(void)props;(void)name;(void)fallback;return maxSize;}
static SDL_Texture* SDL_CreateTexture(SDL_Renderer*r,int format,int access,int w,int h){
 (void)r;(void)format;(void)access;attempts++;
 check(w>0&&h>0,"dimensions must not overflow");
 check(w<=maxSize&&h<=maxSize,"renderer maximum must be checked before allocation");
 uint64_t bytes=(uint64_t)w*h*4;
 check(liveBytes+bytes<=budget(),"sum of cached targets must remain within budget, including resize overlap");
 if(failAll||attempts==failAt||(failAbove&&w>failAbove))return NULL;
 SDL_Texture*t=malloc(sizeof(*t));check(t!=NULL,"fixture allocation");*t=(SDL_Texture){w,h,bytes,1};
 liveBytes+=bytes;if(liveBytes>peakBytes)peakBytes=liveBytes;created++;return t;
}
static void SDL_DestroyTexture(SDL_Texture*t){if(t){if(t->refcount>1){t->refcount--;return;}check(t!=bound,"must not destroy active render target");liveBytes-=t->bytes;destroyed++;free(t);}}
static int SDL_SetTextureScaleMode(SDL_Texture*t,SDL_ScaleMode m){(void)t;(void)m;return 1;}
static const char* SDL_GetRendererName(SDL_Renderer*r){(void)r;return "gpu";}
static SDL_Texture* SDL_GetRenderTarget(SDL_Renderer*r){(void)r;return bound;}
static int SDL_RenderClipEnabled(SDL_Renderer*r){(void)r;return 0;}
static int SDL_GetRenderViewport(SDL_Renderer*r,SDL_Rect*v){(void)r;memset(v,0,sizeof(*v));return 1;}
static int SDL_GetRenderClipRect(SDL_Renderer*r,SDL_Rect*v){(void)r;memset(v,0,sizeof(*v));return 1;}
static int SDL_SetRenderTarget(SDL_Renderer*r,SDL_Texture*t){(void)r;if(++bindCalls==bindFailAt||(failBind&&t)||(failRestore&&!t))return 0;bound=t;return 1;}
static int SDL_SetRenderDrawColor(SDL_Renderer*r,int a,int b,int c,int d){(void)r;(void)a;(void)b;(void)c;(void)d;return 1;}
static int SDL_RenderClear(SDL_Renderer*r){(void)r;return 1;}
static int SDL_SetTextureBlendMode(SDL_Texture*t,int m){(void)t;(void)m;return 1;}
static int SDL_RenderTexture(SDL_Renderer*r,SDL_Texture*t,const SDL_FRect*s,const SDL_FRect*d){(void)r;(void)s;(void)d;check(t!=bound,"must never sample the active target after failed restoration");return 1;}
static int SDL_SetRenderViewport(SDL_Renderer*r,const SDL_Rect*v){(void)r;(void)v;return 1;}
static int SDL_SetRenderClipRect(SDL_Renderer*r,const SDL_Rect*v){(void)r;(void)v;return 1;}
static void GaugeParts(SDL_Renderer*r,float x,float y,float scale,const float*tint){(void)r;(void)x;(void)y;(void)scale;(void)tint;parts++;if(!bound)direct++;}
'''

HARNESS = r'''
static void draw(float scale){PortHdGaugeDraw(&renderer,0,0,scale,1);check(!bound,"draw restores renderer target");}
int main(int argc,char**argv){
 const char*c=argc>1?argv[1]:"valid";
 int expected=0;
 (void)SDL_GetRendererProperties;(void)SDL_GetNumberProperty;
 if(!strcmp(c,"bindordinal")){for(int ordinal=1;ordinal<=4;ordinal++){PortHdLayoutShutdown();bindCalls=0;bindFailAt=ordinal;PortHdGaugeDraw(&renderer,0,0,1,1);bindFailAt=0;PortHdGaugeDraw(&renderer,0,0,1,1);check(!bound,"every failed bind ordinal recovers before the next gauge");}}
 if(!strcmp(c,"restorefail")){failRestore=1;PortHdGaugeDraw(&renderer,0,0,1,1);check(bound!=NULL,"failed restoration must retain live target ownership");int oldParts=parts;PortHdLayoutShutdown();check(bound&&liveBytes,"persistent failure retains cache ownership and budget");PortHdGaugeDraw(&renderer,0,0,1,1);check(parts==oldParts,"unrestored output must not draw more gauge parts");failRestore=0;check(SDL_SetRenderTarget(&renderer,NULL),"recovery restores output");draw(1);}
 if(!strcmp(c,"bindfail")){failBind=1;draw(1);check(direct==1&&!liveBytes,"failed bind must never clear or draw into the original target");}
 if(!strcmp(c,"valid")){draw(1);expected=GAUGE_SS==4?3:2;check(created==expected&&!direct,"valid supersampling remains enabled");draw(1);check(created==expected,"same-size targets are reused");}
 if(!strcmp(c,"budget")){draw(7);check(created==2||direct==1,"large output reduces supersampling or draws directly");}
 if(!strcmp(c,"highdpi")){draw(13.5f);check(!created&&direct==1,"4K-sized HUD bypasses offscreen targets");}
 if(!strcmp(c,"maxsize")){maxSize=512;draw(1);check(created==2&&!direct,"renderer cap degrades SS4 to usable SS2");}
 if(!strcmp(c,"smallmax")){maxSize=400;draw(1);check(!created&&direct==1,"renderer cap permits direct fallback");}
 if(!strcmp(c,"unknownmax")){maxSize=0;draw(1);check(!created&&direct==1,"unknown renderer limit is handled conservatively");}
 if(!strcmp(c,"overflow")){draw(1e30f);check(!attempts,"huge geometry must not be cast or allocated");}
 if(!strcmp(c,"intoverflow")){draw(8e6f);check(!attempts,"integer-scaled target dimensions must not overflow");}
 if(!strcmp(c,"nan")){draw(NAN);check(!attempts,"NaN geometry must not allocate");}
 if(!strcmp(c,"infinity")){draw(INFINITY);check(!attempts,"infinite geometry must not allocate");}
 if(!strcmp(c,"partial")){failAt=2;draw(1);check(parts==1,"partial allocation failure still draws gauge once");
     check(GAUGE_SS==4?(created==3&&destroyed==1&&!direct):(created==1&&destroyed==1&&direct==1),"partial chain is released before a lower-quality fallback");}
 if(!strcmp(c,"cached_failure")){failAbove=600;draw(1);int before=attempts;draw(1);check(attempts==before,"reuse lower-quality chain instead of retrying rejected larger targets every frame");}
 if(!strcmp(c,"allfail")){failAll=1;draw(1);check(parts==1&&direct==1&&!liveBytes,"all allocation failures draw directly without partial cache");}
 if(!strcmp(c,"resize")){draw(1);draw(7);draw(14);check(!liveBytes&&direct>=1&&parts==3,"fallback releases supersampling cache");}
 if(!strcmp(c,"shutdown")){draw(1);PortHdLayoutShutdown();check(!liveBytes&&destroyed==created,"shutdown releases every gauge target");}
 PortHdLayoutShutdown();check(!liveBytes&&destroyed==created,"all cached allocations have one owner and are released");
 return 0;
}
'''

if __name__ == '__main__':
    unittest.main()
