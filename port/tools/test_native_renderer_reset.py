"""Production native renderer reset ownership, generation, targets-only and deferred-detach test."""
from pathlib import Path
import subprocess,tempfile,re
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'port/src/port_main.c').read_text()
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
NAMES='sTexture sMainOverTexture sOverlayTex sScreenOverTex sHudTexture sHudOverTexture sBorderTex sBorderMaterialTex sBorderAdaptiveTex sLcdGridXTex sLcdGridYTex sBlurTex sSkipTexture sHd2dTex sMenuAlpha sSampleTex sNoticeTex'.split()
# Fail if another renderer-owned local/static texture escapes the reset inventory.
declared=set()
for declaration in re.findall(r'static\s+SDL_Texture\s*\*([^;]+);',s):
 for item in declaration.split(','):
  declared.add(re.match(r'\s*\*?\s*(\w+)',item).group(1))
assert declared==set(NAMES+['sMenuSideTex','sFadeSideTex']),declared
PRE=r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#define SDL_EVENT_RENDER_DEVICE_RESET 1
#define SDL_EVENT_RENDER_TARGETS_RESET 2
typedef struct{int generation;}SDL_Texture;typedef int SDL_Renderer;
static SDL_Renderer renderer=1,*sRenderer=&renderer;
static int mainThread=1,detach=1,destroyed,modules,redraw;
static int sRendererResetPending;
static uint64_t sRendererGeneration;
static int sTexW,sTexH,sMainOverW,sMainOverH,sOverlayTexW,sScreenOverTexW,sHudTexW,sHudTexH,sHudOverW,sHudOverH,sBorderShown,sBorderAttempted,sLcdGridReady,sLcdGridAttempted;
static int sMenuSideW[2],sFadeSideW[2],sSkipBufW,sSkipBufH,sHd2dW,sHd2dH,sMenuAlphaW,sMenuAlphaH;
static SDL_Texture*sMenuSideTex[2],*sFadeSideTex[2];
/* port_screen_world_blend.h's renderer-owned scratch: the 22nd texture the reset releases */
static struct{SDL_Texture*texture;int width,height,blockedWidth,blockedHeight,failed;}sWorldLight;
static int SDL_IsMainThread(void){return mainThread;}
static int SDL_SetRenderTarget(SDL_Renderer*r,void*t){(void)r;(void)t;return detach;}
static void SDL_DestroyTexture(SDL_Texture*t){assert(detach);assert(t&&t->generation==1);destroyed++;free(t);}
static void PortLog(const char*f,...){(void)f;}
static void PortHdHudInvalidate(void){modules++;}
static void PortHdSpritesShutdown(void){modules++;}
static void PortHdLayoutShutdown(void){modules++;}
static void PortHdTextShutdown(void){modules++;}
static void PortAchievementToastInvalidate(void){modules++;}
static void PortSceneGalleryPreviewInvalidate(void){modules++;}
'''+''.join('static SDL_Texture*'+n+';\n' for n in NAMES)
TEST=r'''
static SDL_Texture*old(void){SDL_Texture*t=malloc(sizeof(*t));t->generation=1;return t;}
int main(void){
'''+''.join(n+'=old();\n' for n in NAMES)+r'''
 for(int i=0;i<2;i++){sMenuSideTex[i]=old();sFadeSideTex[i]=old();sMenuSideW[i]=sFadeSideW[i]=16;}
 sBorderShown=sBorderAttempted=3;sLcdGridReady=sLcdGridAttempted=1;
 sTexW=sTexH=sSkipBufW=sSkipBufH=sMenuAlphaW=sMenuAlphaH=100;
 sWorldLight.texture=old();sWorldLight.width=sWorldLight.height=sWorldLight.failed=sWorldLight.blockedWidth=sWorldLight.blockedHeight=5;
 assert(PortRendererHandleReset(SDL_EVENT_RENDER_TARGETS_RESET));assert(!destroyed&&!modules&&!sRendererGeneration);
 mainThread=0;assert(!PortRendererHandleReset(SDL_EVENT_RENDER_DEVICE_RESET));assert(!destroyed);mainThread=1;
 detach=0;assert(PortRendererHandleReset(SDL_EVENT_RENDER_DEVICE_RESET));assert(sRendererResetPending&&!destroyed&&!modules);
 detach=1;assert(RendererResetService());assert(!sRendererResetPending&&sRendererGeneration==1&&destroyed==22&&modules==6);
'''+''.join('assert(!'+n+');\n' for n in NAMES)+r'''
 assert(!sLcdGridXTex&&!sLcdGridYTex&&!sLcdGridReady&&!sLcdGridAttempted);
 assert(!sBorderTex&&!sBorderMaterialTex&&!sBorderAdaptiveTex&&!sBorderShown&&!sBorderAttempted);
 assert(!sWorldLight.texture&&!sWorldLight.width&&!sWorldLight.height&&!sWorldLight.failed&&!sWorldLight.blockedWidth&&!sWorldLight.blockedHeight);
 assert(!sTexW&&!sTexH&&!sSkipBufW&&!sSkipBufH&&!sMenuAlphaW&&!sMenuAlphaH);
 for(int i=0;i<2;i++)assert(!sMenuSideTex[i]&&!sFadeSideTex[i]&&!sMenuSideW[i]&&!sFadeSideW[i]);
 assert(RendererResetService());assert(destroyed==22&&modules==6);
 /* A later device loss re-arms owner modules even if optional loading previously failed. */
 assert(PortRendererHandleReset(SDL_EVENT_RENDER_DEVICE_RESET));assert(sRendererGeneration==2&&destroyed==22&&modules==12);
 assert(!PortRendererHandleReset(999));
 (void)redraw;
 puts("PASS production native reset: all 22 main textures, module owners, generation, targets-only, thread affinity, deferred target detach");return 0;
}
'''
core=(function('static int RendererResetService(')+function('int PortRendererHandleReset(')) if 'static int RendererResetService(' in s else 'static int RendererResetService(void){return 1;}\nint PortRendererHandleReset(uint32_t type){(void)type;return 0;}\n'
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory() as td:
 p=Path(td);(p/'reset.c').write_text(PRE+core+TEST);subprocess.run([str(cc),'-std=c99',str(p/'reset.c'),'-o',str(p/'reset.exe')],check=True,creationflags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0));subprocess.run([str(p/'reset.exe')],check=True,creationflags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0))
