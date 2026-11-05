"""Compile scene099's actual pixel resolver for the missing right-hand timber."""
import os,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <assert.h>
#include "port/port_preliminar_v1_tiles.h"
int main(void){PortPreliminarRecipe p={0};p.v[0]=99;p.v[1]=3;p.v[4]=p.v[5]=32;
 for(int x=224;x<240;x++)for(int y=-512;y<24;y++){
  int sx=-1,sy=-1;assert(PortPreliminarPixelSource(&p,x,y,&sx,&sy));
  int phase=PortSceneTileMod(23-y,64),expected=24+(phase<32?phase:63-phase);
  assert(sx==x&&sy==expected);assert(sy>=24&&sy<56);
 }
 for(int x=224;x<240;x++){int sx=-1,sy=-1;assert(PortPreliminarPixelSource(&p,x,23,&sx,&sy));assert(sx==x&&sy==24);}
 /* Other authored objects and floor still use their original map pixels. */
 for(int x=0;x<256;x++)for(int y=24;y<256;y++){int sx=-1,sy=-1;assert(!PortPreliminarPixelSource(&p,x,y,&sx,&sy));}
 return 0;}
'''
class Scene099Test(unittest.TestCase):
 def test_missing_timber_uses_its_whole_grain_without_crosspieces(self):
  compiler=os.environ.get('CC') or shutil.which('gcc') or str(ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe')
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(CODE);exe=Path(d)/'check.exe'
   subprocess.run([compiler,'-I',str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
