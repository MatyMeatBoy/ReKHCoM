"""Actual production sampler/filter tests; fixtures contain no ROM pixels."""
import os,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CC=os.environ.get('CC') or str(ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
FLAGS=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0)
class Scene081Test(unittest.TestCase):
 def compile_and_run(self,source,include=False):
  with tempfile.TemporaryDirectory() as d:
   c=Path(d)/'check.c';c.write_text(source);exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   args=[CC,'-O1',str(c),'-o',str(exe)]
   if include:args+=['-I',str(ROOT/'port/include')]
   r=subprocess.run(args,capture_output=True,text=True,creationflags=FLAGS)
   self.assertEqual(r.returncode,0,r.stdout+r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True,creationflags=FLAGS)
   self.assertEqual(r.returncode,0,r.stdout+r.stderr)
 def test_native_cave_cap_and_approved_right(self):
  self.compile_and_run((ROOT/'port/tools/scene081_cave_conformance.c').read_text(),include=True)
 def test_idle_shadow_gate_does_not_advance_other_waves(self):
  source=(ROOT/'port/src/port_ppu.c').read_text()
  start=source.index('        if (ox >= objRight || ox + bw <= objLeft)')
  end=source.index('        if (sceneActorFrame && !sOamTag[i].screen',start)
  actual=source[start:end]
  code=r'''#include <stdio.h>
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define EVENT_081_MONSTORO_E3 81
static int gEmy00Palette[1],otherPalette[1];
static int gEmy00L00Tiles[1],gEmy00L02Tiles[1];
static struct{int wide;}gPortScreen;
static int sActorSceneKey,sEnvActorFrameOff;
static struct{int screen,hud,text;}sOamTag[1];
static struct{const void*palette;const void*tiles;}sOamHd[1];
static int Visible(const void*p,const void*t,int x,int y,int w,int h,int event,int wide){
 int i,visible=0,ox=x,oy=y,bw=w,bh=h,objRight=600,objLeft=-360;
 sOamHd[0].palette=p;sOamHd[0].tiles=t;sActorSceneKey=event;gPortScreen.wide=wide;
 for(i=0;i<1;i++){
'''+actual+r'''
 visible=1;}return visible;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"actor gate failed: %s\n",#x);return 1;}}while(0)
int main(void){
 CHECK(!Visible(gEmy00Palette,gEmy00L00Tiles,280,40,32,32,81,1));
 CHECK(!Visible(gEmy00Palette,gEmy00L00Tiles,320,60,32,32,81,1));
 CHECK(Visible(gEmy00Palette,gEmy00L02Tiles,280,40,32,32,81,1)); /* walking wave is independent */
 CHECK(!Visible(gEmy00Palette,gEmy00L00Tiles,320,60,32,32,81,1)); /* first wave cannot release the idle second */
 CHECK(Visible(gEmy00Palette,gEmy00L00Tiles,235,40,32,32,81,1)); /* whole part crossing GBA */
 CHECK(Visible(gEmy00Palette,gEmy00L00Tiles,100,100,32,32,81,1));
 CHECK(Visible(gEmy00Palette,gEmy00L00Tiles,280,40,32,32,82,1));
 CHECK(Visible(gEmy00Palette,gEmy00L00Tiles,280,40,32,32,81,0));
 CHECK(Visible(otherPalette,gEmy00L00Tiles,280,40,32,32,81,1));
 sOamTag[0].screen=1;CHECK(Visible(gEmy00Palette,gEmy00L00Tiles,280,40,32,32,81,1));sOamTag[0].screen=0;
 sEnvActorFrameOff=1;CHECK(Visible(gEmy00Palette,gEmy00L00Tiles,280,40,32,32,81,1));
 return 0;}
'''
  self.compile_and_run(code)
if __name__=='__main__':unittest.main()
