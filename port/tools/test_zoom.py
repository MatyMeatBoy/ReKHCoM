"""Check production zoom defaults, bounds and distinct selectable scales."""
from pathlib import Path
import subprocess, shutil, argparse
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'build/zoom-regression'; OUT.mkdir(parents=True,exist_ok=True)
code = r"""
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "port/port.h"
extern float gPortAutoScaleF;
static int menuCanvas;
int PortUiMenuCanvas(void) { return menuCanvas; }
/* port_config.c's links into modules this fixture does not build */
void PortLog(const char* fmt, ...) { (void)fmt; }
int PortUndubAvailable(void) { return 0; }
void PortSetQolButtons(int on) { (void)on; }
void PortHdMovieInvalidatePaths(void) {}
int main(void) {
 int sizes[][2]={{320,180},{960,540},{1280,720},{1280,800},{1920,1080},{2880,1800},{3440,1440},{5120,1440},{7680,2160}};
 int i,z,w,h,s,k,only,prev,cases=0;
 assert(gPortConfig.zoom==PORT_ZOOM_DEFAULT && gPortConfig.integerZoom==0);
 assert(!gPortConfig.cheatUnlockRiku && !PortRikuAvailable(0));
 assert(PortRikuAvailable(32));
 gPortConfig.cheatUnlockRiku=1;assert(PortRikuAvailable(0));
 PortConfigSetDataPath(".");PortConfigSave();
 gPortConfig.cheatUnlockRiku=0;PortConfigLoad();assert(gPortConfig.cheatUnlockRiku==1);
 gPortConfig.cheatUnlockRiku=0;PortConfigSave();PortConfigLoad();assert(!PortRikuAvailable(0));
 for(only=1;only<2;only++) {
  gPortConfig.integerZoom=only;
  for(i=0;i<9;i++) {
   int W=sizes[i][0],H=sizes[i][1],count=PortZoomCount(only,W,H);
   prev=999;
   for(k=0;k<count;k++) {
    z=PortZoomAt(k,only,W,H);
    assert(z<prev);prev=z;
    assert(PortZoomIndex(z,only,W,H)==k);
    assert(PortZoomStep(z,1,only,W,H)==PortZoomAt(k+1,only,W,H));
    assert(PortZoomStep(z,-1,only,W,H)==PortZoomAt(k-1,only,W,H));
    if(only) assert(!(z&1));
    gPortConfig.zoom=z;PortConfigAutoView(W,H,0,&w,&h,&s);
    assert(w<=PORT_MAX_W && h<=PORT_MAX_H && w>=240 && h>=160);
    assert(fabsf(gPortAutoScaleF-z*0.5f)<0.001f);
    assert(abs(h-(int)ceilf(H/(z*0.5f)))<=1);
    cases++;
   }
  }
 }
 gPortConfig.integerZoom=1;gPortConfig.zoom=PORT_ZOOM_CLASSIC;
 gPortConfig.integerZoom=0;gPortConfig.zoom=10; /* x5 on a 16:10 laptop. */
 PortConfigAutoView(1280,800,0,&w,&h,&s);
 assert(w==256 && h==160 && s==5 && fabsf(gPortAutoScaleF-5.0f)<0.001f);
 assert(PortZoomMax(2880,1800)==11); /* the reported 14-inch 2880x1800 display. */
 gPortConfig.integerZoom=1;gPortConfig.zoom=PORT_ZOOM_CLASSIC;
 PortConfigAutoView(1920,1080,0,&w,&h,&s);
 assert(w==480 && h==270 && s==4);
 gPortConfig.zoom=PORT_ZOOM_DEFAULT;PortConfigAutoView(5120,1440,0,&w,&h,&s);
 assert(w==1280 && h==360 && s==4);
 assert(PortZoomClassicScale(5120,1440)==5);
 assert(PortZoomNormalize(9,0,1920,1080)==9);
 assert(PortZoomNormalize(9,1,1920,1080)==10);
 PortConfigSetDataPath(".");
 FILE* f=fopen("kh_com.ini","w"); assert(f);
 fprintf(f,"config_version=8\nzoom=6\ninteger_zoom=0\n"); fclose(f);
 PortConfigLoad(); assert(gPortConfig.zoom==PORT_ZOOM_DEFAULT && !gPortConfig.integerZoom);
 gPortConfig.zoom=10;gPortConfig.integerZoom=0;PortConfigSave();
 gPortConfig.zoom=1;PortConfigLoad();assert(gPortConfig.zoom==10 && !gPortConfig.integerZoom);
 assert(PortZoomReference(12)==1 && PortZoomReference(6)==2 && PortZoomReference(8)==0);
 f=fopen("kh_com.ini","w");assert(f);
 fprintf(f,"config_version=9\nzoom=9\ninteger_zoom=1\n");fclose(f);
 PortConfigLoad();assert(gPortConfig.zoom==10 && !gPortConfig.integerZoom);
 PortConfigAutoView(1920,1080,0,&w,&h,&s);
 gPortConfig.integerZoom=1;assert(PortConfigZoomNext(12,1)==10); /* no intermediate step */
 gPortConfig.integerZoom=0;assert(PortConfigZoomNext(12,1)==10);
 for(z=1;z<=PORT_ZOOM_LEVELS;z++)assert(!(PortConfigZoomId(z)&1));

 /* Battle changes the view, not the saved main zoom or the menu canvas. */
 assert(gPortConfig.battleZoom==0);
 gPortConfig.zoom=8;gPortConfig.battleZoom=10;
 PortConfigAutoView(1920,1080,0,&w,&h,&s);assert(w==480 && h==270 && s==4);
 PortConfigAutoView(1920,1080,1,&w,&h,&s);assert(w==384 && h==216 && s==5);
 assert(gPortConfig.zoom==8 && gPortConfig.battleZoom==10);
 PortConfigAutoView(1920,1080,0,&w,&h,&s);assert(w==480 && h==270 && s==4);
 menuCanvas=1;
 PortConfigAutoView(1920,1080,0,&w,&h,&s);int menuW=w,menuH=h;
 PortConfigAutoView(1920,1080,1,&w,&h,&s);assert(w==menuW && h==menuH);
 menuCanvas=0;
 for(i=0;i<9;i++) {
  int fw,fh,fs,W=sizes[i][0],H=sizes[i][1];
  gPortConfig.battleZoom=0;gPortConfig.zoom=8;
  PortConfigAutoView(W,H,0,&fw,&fh,&fs);
  PortConfigAutoView(W,H,1,&w,&h,&s);assert(w==fw && h==fh && s==fs);
  gPortConfig.zoom=6;
  PortConfigAutoView(W,H,0,&fw,&fh,&fs);
  PortConfigAutoView(W,H,1,&w,&h,&s);assert(w==fw && h==fh && s==fs);
 }
 PortConfigAutoView(1920,1080,0,&w,&h,&s);
 int count=PortZoomCount(1,1920,1080);
 assert(PortConfigBattleZoomNext(0,-1)==0);
 z=0;
 for(k=0;k<count;k++) {z=PortConfigBattleZoomNext(z,1);assert(z==PortZoomAt(k,1,1920,1080));}
 assert(PortConfigBattleZoomNext(z,1)==z);
 for(k=count-2;k>=0;k--) {z=PortConfigBattleZoomNext(z,-1);assert(z==PortZoomAt(k,1,1920,1080));}
 assert(PortConfigBattleZoomNext(z,-1)==0);
 gPortConfig.battleZoom=10;PortConfigSave();gPortConfig.battleZoom=0;
 PortConfigLoad();assert(gPortConfig.battleZoom==10);
 gPortConfig.battleZoom=0;PortConfigSave();gPortConfig.battleZoom=12;
 PortConfigLoad();assert(gPortConfig.battleZoom==0);
 f=fopen("kh_com.ini","w");assert(f);fprintf(f,"config_version=11\nzoom=8\n");fclose(f);
 PortConfigLoad();assert(gPortConfig.battleZoom==0 && gPortConfig.zoom==8);
 f=fopen("kh_com.ini","w");assert(f);fprintf(f,"config_version=11\nexperimental_battle_zoom=-3\n");fclose(f);
 PortConfigLoad();assert(gPortConfig.battleZoom==0);
 f=fopen("kh_com.ini","w");assert(f);fprintf(f,"config_version=11\nexperimental_battle_zoom=999\n");fclose(f);
 PortConfigLoad();assert(gPortConfig.battleZoom==PORT_ZOOM_LEVELS);
 f=fopen("kh_com.ini","w");assert(f);fprintf(f,"config_version=11\nexperimental_battle_zoom=9\n");fclose(f);
 PortConfigLoad();assert(gPortConfig.battleZoom==10);
 puts("PASS: battle/main switching, inherited zoom, menu isolation, selector bounds, INI defaults/persistence/clamping");
 printf("PASS: %d resolution/zoom cases, exact 16:10 256x160 @x5, exact v1.4 x4, numbered selector, INI migration/persistence\n",cases);
 return 0;
}

"""
(OUT/'zoom.c').write_text(code)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--toolchain',type=Path)
parser.add_argument('--sdl',type=Path)
args=parser.parse_args()
cc=(args.toolchain/'bin/i686-w64-mingw32-clang.exe') if args.toolchain else next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
sdl=args.sdl/'i686-w64-mingw32' if args.sdl else next((ROOT.parent/'toolchain').glob('SDL3-*/i686-w64-mingw32'))
shutil.copy2(sdl/'bin/SDL3.dll',OUT/'SDL3.dll')
subprocess.run([str(cc),'-std=gnu11','-I'+str(ROOT/'port/include'),'-I'+str(sdl/'include'),str(OUT/'zoom.c'),str(ROOT/'port/src/port_config.c'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(OUT/'zoom.exe')],check=True)
# Auto view stretches tall (16:10, 4:3) outputs to a 16:9 row count since 24/10 (PortConfigAutoView); this test checks the plain scale ladder.
import os
subprocess.run([str(OUT/'zoom.exe')],cwd=OUT,check=True,env=dict(os.environ,KHCOM_TALL_FIT='0'))
