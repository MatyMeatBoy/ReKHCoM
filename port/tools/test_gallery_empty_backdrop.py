"""Render the production empty-poster backdrop; guard card bounds and tiny sizes."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'port/src/port_scene_gallery_menu.inc').read_text(encoding='utf8')
name='static void ScenePreviewBackdrop('
assert name in source,'Empty gallery still uses text instead of the requested two backdrops'
start=source.index(name);end=source.index('\nstatic void DrawSceneMenu',start)
body=source[start:end]
assert 'ScenePreviewBackdrop(cardX + 6, top + 16, pw, ph, SceneProviderHd(sSceneChoice))' in source
assert 'Sin vista previa' not in source
out=ROOT/'build/test-gallery-empty-backdrop';out.mkdir(parents=True,exist_ok=True)
unit=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
static uint32_t pixels[256*144];
static void Fill(int x,int y,int w,int h,uint32_t c){
 assert(x>=0&&y>=0&&w>=0&&h>=0&&x+w<=256&&y+h<=144);
 for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)pixels[j*256+i]=c;
}
'''+body+r'''
int main(void){
 uint32_t first[2];
 for(int hd=0;hd<2;hd++){
  for(int i=0;i<256*144;i++)pixels[i]=0x12345678;
  ScenePreviewBackdrop(13,19,158,89,hd);first[hd]=pixels[19*256+13];
  for(int y=0;y<144;y++)for(int x=0;x<256;x++){
   uint32_t c=pixels[y*256+x];
   if(x>=13&&x<171&&y>=19&&y<108)assert((c>>24)==255);
   else assert(c==0x12345678);
  }
  char path[40];sprintf(path,"backdrop-%d.ppm",hd);FILE*f=fopen(path,"wb");assert(f);
  fprintf(f,"P6\n158 89\n255\n");
  for(int y=19;y<108;y++){
   for(int x=13;x<171;x++){
    uint32_t c=pixels[y*256+x];fputc((c>>16)&255,f);fputc((c>>8)&255,f);fputc(c&255,f);
   }
  }
  fclose(f);
 }
 assert(first[0]!=first[1]);
 for(int n=0;n<10;n++){ScenePreviewBackdrop(0,0,n,n,0);ScenePreviewBackdrop(0,0,n,n,1);}
 ScenePreviewBackdrop(0,0,0,10,0);ScenePreviewBackdrop(0,0,10,0,1);
 puts("PASS distinct opaque backdrops, untouched exterior, tiny/zero sizes");
}
'''
(out/'probe.c').write_text(unit,encoding='utf8')
base=next(p for p in ROOT.parents if (p/'toolchain').is_dir())
cc=base/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
subprocess.run([str(cc),'-std=gnu11','-Wall','-Wextra','-Werror',str(out/'probe.c'),'-o',str(out/'probe.exe')],check=True)
subprocess.run([str(out/'probe.exe')],cwd=out,check=True)
