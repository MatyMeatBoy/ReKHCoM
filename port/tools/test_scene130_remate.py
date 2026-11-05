"""Exercise the actual PPU caller: the reviewed notch is authored flat green."""
import os, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CC=os.environ.get('CC') or str(ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
class LibraryRemateTest(unittest.TestCase):
 def test_only_the_reviewed_flat_green_notch_uses_the_native_baseboard_donor(self):
  ppu=(ROOT/'port/src/port_ppu.c').read_text()
  actual=ppu[ppu.index('static int Preliminar130Authored('):ppu.index('static uint16_t SinglePanelColor(')]
  fixture=r'''#include <stdint.h>
#include <stdio.h>
#include "port/port_scene130_library.h"
#define IO16(x) (x)
#define TRANSPARENT 65535
static uint16_t PLTT16[]={0,100,200,300,400};
static int coverBooks,textured;
static int sBgLatch[4];
static struct Recipe{int v[1];} recipe={{130}};
static const struct Recipe*sPreliminarRecipe[]={0,0,&recipe,&recipe};
static int SinglePanelIndex(const int*l,int cnt,int x,int y){
 (void)l;
 if(cnt==12)return coverBooks?4:0; /* BG2 is transparent at the reviewed notch. */
 if(textured && x==32 && y==274)return 4;
 if(x==0 && y==0)return 1; /* outer map's base colour */
 if(x>=88 && x<112 && y>=240 && y<268)return 3; /* complete native baseboard donor */
 return 2; /* authored flat-green floor: deliberately differs from base */
}
'''+actual+r'''
int main(void){
 int x,y,count=0;uint16_t color;
 for(y=264;y<304;y++)for(x=16;x<64;x++){
  int repair=x>=24 && x<48 && y>=272 && y<300 && (x>=32 || y>=280) && x+2*y<624;
  if(!Preliminar130Pixel(3,x,y,&color))return 2;
  if(color!=(repair?300:200)){fprintf(stderr,"reviewed remate (%d,%d) returned %u, expected %u\n",x,y,color,repair?300:200);return 1;}
  count+=repair;
 }
 if(count!=476)return 3;
 coverBooks=1;Preliminar130Pixel(3,32,274,&color);if(color!=200)return 4;
 coverBooks=0;textured=1;Preliminar130Pixel(3,32,274,&color);if(color!=400)return 5;
 return 0;
}
'''
  with tempfile.TemporaryDirectory() as d:
   c=Path(d)/'test.c';c.write_text(fixture);exe=Path(d)/('test.exe' if os.name=='nt' else 'test')
   flags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0)
   built=subprocess.run([CC,'-O1','-I',str(ROOT/'port/include'),str(c),'-o',str(exe)],capture_output=True,text=True,creationflags=flags)
   self.assertEqual(built.returncode,0,built.stdout+built.stderr)
   run=subprocess.run([str(exe)],capture_output=True,text=True,creationflags=flags)
   self.assertEqual(run.returncode,0,run.stdout+run.stderr)
if __name__=='__main__':unittest.main()
