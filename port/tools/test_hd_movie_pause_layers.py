"""Render the production movie-pause composition with a deferred HD text layer.

Checks all cartridge defines and unequal X/Y output scales without a display.
Runtime screenshot checks separately exercise the real glyph rasterizer.
"""
from pathlib import Path
import shutil, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'port/src/port_hdmovie.c').read_text(encoding='utf-8')
a=s.index('        PortUiDraw(work, cw, ch);');b=s.index('        if (test && ',a)
production=s[a:b]
preamble=r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static int hd, queued, textVisible, baseDrawn, outW, outH;
static float expectW, expectH;
static float rx=1,ry=1;
static void PortUiDraw(uint32_t*p,int w,int h){(void)p;(void)w;(void)h;queued=hd;textVisible=!hd;}
static void SDL_UpdateTexture(void*t,void*r,void*p,int n){(void)t;(void)r;(void)p;(void)n;}
static void SDL_SetRenderViewport(void*r,void*v){(void)r;(void)v;}
static void SDL_SetRenderClipRect(void*r,void*v){(void)r;(void)v;}
static void SDL_SetRenderScale(void*r,float x,float y){(void)r;rx=x;ry=y;}
static void SDL_GetCurrentRenderOutputSize(void*r,int*w,int*h){(void)r;*w=outW;*h=outH;}
static void SDL_RenderTexture(void*r,void*t,void*a,void*b){(void)r;(void)t;(void)a;(void)b;baseDrawn=1;}
static void PortUiHdDraw(void*r,float x,float y,float scale){
 (void)r;(void)x;(void)y;if(!queued)return;
 if(!baseDrawn || __builtin_fabsf(rx*scale-expectW)>0.001f || __builtin_fabsf(ry*scale-expectH)>0.001f){fprintf(stderr,"wrong layer order/transform\n");exit(2);}
 textVisible=1;
}
static void check(int useHd,int cw,int ch,int width,int height){
 uint32_t work[1];void* renderer=0;void* layer=0;hd=useHd;queued=textVisible=baseDrawn=0;outW=width;outH=height;
 expectW=(float)width/cw;expectH=(float)height/ch;
'''
cases=r'''
 if(!textVisible){fprintf(stderr,"FAIL: HD pause deferred text never presented\n");exit(1);}
 if(rx!=1 || ry!=1){fprintf(stderr,"renderer scale leaked\n");exit(3);}
}
int main(void){
 check(1,240,160,960,640);check(1,240,160,960,960);check(1,320,180,1920,1080);check(0,240,160,960,640);check(1,284,160,1000,700);
 puts("PASS: HD/GBA pause layers and renderer scale");return 0;
}
'''
compiler=shutil.which('cc') or str(next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe')))
with tempfile.TemporaryDirectory(prefix='rekhcom-pause-') as folder:
 folder=Path(folder);src=folder/'pause.c';src.write_text(preamble+production+cases,encoding='utf-8')
 for region in ('EU','US','JP'):
  for platform in ('_WIN32','PORT_LINUX'):
   exe=folder/(region+platform+'.exe');subprocess.run([compiler,'-std=c11','-DVERSION_'+region,'-D'+platform,str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
