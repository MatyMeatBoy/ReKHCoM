"""Execute the production presenter resolution consumer without a GPU."""
from gen_scene_framing import generate
import argparse
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <math.h>
#include <stdio.h>
#include "port/scene_framing.h"
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_ceilf ceilf
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define PORT_MAX_W 2560
#define PORT_MAX_H 1440
#define PORT_VIEW_AUTO 0
typedef struct {int w,h;} SDL_Rect;
typedef struct {int value;} Mode;
static Mode gModeEvent,gModeEventDebug,gModeBattle,gModeVsbattle,field;
static Mode* gCurrentMode=&gModeEvent;
static struct {int viewMode,zoom,hudSize;} gPortConfig={0,8,0};
static struct {int width,height;} gPortScreen;
static int outWidth=1920,outHeight=1080,menu,event=7,sRenderer;
static int sAutoScale,sFit,gPortOverlayW,gPortHudScale,gPortHudW,gPortHudH,gPortWorldScale;
static float sOverlayScale,sFitScale,gPortAutoScaleF;
static int gPortSceneFraming,gPortSceneCameraAdjusted,tall;
static int PortEventId(void){return event;}
static int PortUiMenuCanvas(void){return menu;}
static int SDL_GetCurrentRenderOutputSize(int r,int*w,int*h){(void)r;*w=outWidth;*h=outHeight;return 1;}
static SDL_Rect AspectRect(int w,int h){SDL_Rect r={w,h};return r;}
static void PortConfigView(int mode,int*w,int*h){(void)mode;*w=240;*h=160;}
static float MenuScale(int w,int h){return SDL_min(h/160.0f,w/240.0f);}
static void PortConfigAutoView(int W,int H,int battle,int*w,int*h,int*s){
    (void)battle;*s=menu?3:gPortConfig.zoom/2;gPortAutoScaleF=tall?40.0f/9.0f:(float)*s;*w=(W+*s-1)/ *s;*h=(H+*s-1)/ *s;
}
static void PortPpuSetResolution(int w,int h){gPortScreen.width=w;gPortScreen.height=h;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
/* PRODUCTION */
int main(void){
    UpdateResolution();
    CHECK(sAutoScale==6&&gPortAutoScaleF==6&&gPortWorldScale==6&&gPortSceneCameraAdjusted);
    CHECK(gPortHudScale==4&&gPortHudW==480&&gPortHudH==270);
    CHECK(sOverlayScale==4&&gPortOverlayW==480);
    CHECK(gPortScreen.width==320&&gPortScreen.height==180&&gPortConfig.zoom==8);
    gCurrentMode=&gModeEventDebug;UpdateResolution();CHECK(sAutoScale==6&&gPortSceneCameraAdjusted);
    gCurrentMode=&gModeEvent;
    event=121;UpdateResolution();CHECK(!gPortSceneCameraAdjusted&&sAutoScale==4&&gPortHudScale==0);
    event=7;gPortSceneFraming=1;UpdateResolution();CHECK(sAutoScale==4&&!gPortSceneCameraAdjusted);
    gPortSceneFraming=0;gCurrentMode=&field;UpdateResolution();CHECK(sAutoScale==4&&!gPortSceneCameraAdjusted);
    gCurrentMode=&gModeBattle;UpdateResolution();CHECK(sAutoScale==4&&!gPortSceneCameraAdjusted);
    gCurrentMode=&gModeEvent;menu=1;UpdateResolution();CHECK(sAutoScale==3&&!gPortSceneCameraAdjusted);
    menu=0;sFit=1;UpdateResolution();CHECK(sFitScale==6&&gPortScreen.height==160&&gPortHudScale==4);
    sFit=0;outWidth=1280;outHeight=720;gPortConfig.zoom=4;UpdateResolution();
    CHECK(sAutoScale==4&&gPortHudScale==2&&gPortHudH==360);
    gPortConfig.hudSize=1;UpdateResolution();CHECK(gPortHudScale==3);
    outHeight=800;gPortConfig.zoom=8;tall=1;UpdateResolution();
    CHECK(sAutoScale==5&&fabsf(sOverlayScale-40.0f/9.0f)<0.001f);
    tall=0;gPortConfig.viewMode=1;UpdateResolution();CHECK(!gPortSceneCameraAdjusted&&gPortHudScale==0&&sAutoScale==0);
    gPortConfig.viewMode=0;event=-1;UpdateResolution();CHECK(!gPortSceneCameraAdjusted&&sAutoScale==4);
    puts("PASS production presenter: camera/UI independence, honors/extended, narrow scene, resize, battle/menu/Original isolation");
    return 0;
}
'''
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cc',required=True);a=ap.parse_args()
    source=(ROOT/'port/src/port_main.c').read_text(encoding='utf-8')
    start=source.index('static void UpdateResolution(void) {');end=source.index('\n/* GBA classic',start)
    function=source[start:end]
    helper=source.find('int PortSceneFramingActive(void) {')
    if helper>=0:function=source[helper:source.index('\n}\n',helper)+3]+'\n'+function
    with tempfile.TemporaryDirectory(prefix='scene-presenter-') as tmp:
        generate(ROOT,Path(tmp)/'scene_framing_grants.h')
        src,exe=Path(tmp)/'test.c',Path(tmp)/'test.exe';src.write_text(CODE.replace('/* PRODUCTION */',function))
        subprocess.run([a.cc,'-std=c99','-static','-I',tmp,'-I',str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
