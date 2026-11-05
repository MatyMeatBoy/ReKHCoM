"""Compiled regression for the scene115 exterior-only recipe."""
import os, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene095HedgeTest(unittest.TestCase):
 def test_whole_flower_face_in_both_extensions(self):
  compiler=os.environ.get('CC') or shutil.which('gcc')
  local=ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
  if not compiler and local.is_file():compiler=str(local)
  if not compiler:self.skipTest('C compiler unavailable')
  code=r'''#include <assert.h>
#include "port_preliminar_v1_tiles.h"
int main(void){unsigned i;int wx,wy,sx,sy;const PortPreliminarRecipe*p=0;
for(i=0;i<sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]);i++)if(kPreliminarRecipes[i].v[0]==95&&kPreliminarRecipes[i].v[1]==3){p=&kPreliminarRecipes[i];break;}assert(p);
for(wx=-192;wx<48;wx++)for(wy=260;wy<420;wy++){
 int d=2*wy+wx;if(d<576||d>=672)continue;
 assert(PortPreliminarPixelSource(p,wx,wy,&sx,&sy));assert(sx>=64&&sx<160);assert(2*sy+sx==d||2*sy+sx==d-1);
}
for(wx=480;wx<704;wx++)for(wy=280;wy<420;wy++){
 int d=2*wy-wx;if(d<64||d>=96)continue;
 assert(PortPreliminarPixelSource(p,wx,wy,&sx,&sy));assert(sx>=64&&sx<160);assert(2*sy+sx==d+576||2*sy+sx==d+575);
}return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code)
   exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   subprocess.run([compiler,'-I',str(ROOT/'port/include/port'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
