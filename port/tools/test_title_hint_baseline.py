"""Exercise the production title sign geometry: HD lamp and underline share a base."""
from pathlib import Path
import os, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT/'port/src/port_ui.c').read_text(encoding='utf-8')

def function(signature):
    start=source.index(signature); brace=source.index('{',start); end=brace+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

unit=r'''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define GBA_SCREEN_W 240
#define MENU_INK 0xFF000000u
#define MENU_SHADE 0xFFAAAAAAu
#define MENU_GREY 0xFFBBBBBBu
static uint32_t pixels[160][240];
static struct {int originX,originY;} gPortScreen;
static struct {int hdTitle;} gPortConfig;
static int textY;
static const char* sTitleHint[]={"SELECT OPTIONS"};
static int TextWidth(const char* text){(void)text;return 100;}
static void Fill(int x,int y,int w,int h,uint32_t c){for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)pixels[j][i]=c;}
static void Shade(int x,int y,int w,int h,uint32_t c,int a){(void)a;Fill(x,y,w,h,c);}
static void MenuLight(int x,int y){
 if(gPortConfig.hdTitle)Fill(x-3,y-3,11,11,MENU_INK);
 Fill(x,y,5,5,0xFFFF9900u);
}
static void DrawText(const char* t,int x,int y,uint32_t f,uint32_t s){(void)t;(void)x;(void)f;(void)s;textY=y;}
'''
if 'static void MenuBarBase(' in source:
    unit+=function('static void MenuBarBase(')+'\n'
unit+=function('static void MenuBar(')+'\n'+function('static void DrawTitleHint(')+'\n'
unit+=r'''
int main(void){
 for(int hd=0;hd<=1;hd++){
  for(int y=0;y<160;y++)for(int x=0;x<240;x++)pixels[y][x]=0xFFFFFFFFu;
  gPortConfig.hdTitle=hd;DrawTitleHint(0);
  int signX=240-6-(100+23),lampBottom=-1,lineBottom=-1;
  for(int y=0;y<30;y++){
   if(pixels[y][signX]==MENU_INK)lampBottom=y;
   if(pixels[y][signX+40]==MENU_INK)lineBottom=y;
  }
  printf("HD=%d lamp-base=%d underline-base=%d text-origin=%d\n",hd,lampBottom,lineBottom,textY);
  fflush(stdout);
  assert(lampBottom==lineBottom);
  assert(textY==(hd?6:5));
 }
 return 0;
}'''
with tempfile.TemporaryDirectory() as d:
    path=Path(d);(path/'sign.c').write_text(unit,encoding='utf-8')
    cc=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
    subprocess.run([str(cc),str(path/'sign.c'),'-o',str(path/'sign.exe')],check=True)
    subprocess.run([str(path/'sign.exe')],check=True)
