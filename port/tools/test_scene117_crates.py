"""Compiled regression for the scene115 exterior-only recipe."""
import os, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene117CrateTest(unittest.TestCase):
 def test_matching_caps_right_tip_and_diagonal_deck(self):
  compiler=os.environ.get('CC') or shutil.which('gcc')
  local=ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
  if not compiler and local.is_file():compiler=str(local)
  if not compiler:self.skipTest('C compiler unavailable')
  code=r'''#include <assert.h>
#include "port_preliminar_v1_tiles.h"
static const PortPreliminarRecipe* recipe(int bg){unsigned i;for(i=0;i<sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]);i++)if(kPreliminarRecipes[i].v[0]==117&&kPreliminarRecipes[i].v[1]==bg)return &kPreliminarRecipes[i];return 0;}
int main(void){int x,y,sx,sy,want;const PortPreliminarRecipe*p=recipe(2);assert(p);
for(x=40;x<152;x++)for(y=-48;y<0;y++){assert(PortPreliminarPixelSource(p,x,y,&sx,&sy));want=x+120;if(want>=256){want=431-want;assert(sy==y+48);}else assert(sy==y+56);assert(sx==want);}
for(x=256;x<272;x++)for(y=56;y<124;y++){assert(PortPreliminarPixelSource(p,x,y,&sx,&sy));assert(sx==x-120&&sy==y-56);}
for(x=-48;x<0;x++)for(y=104;y<220;y++){assert(PortPreliminarPixelSource(p,x,y,&sx,&sy));assert(sx==x+(y>=152?88:208));assert(sy==y-(y>=152?152:96));}
p=recipe(3);assert(p);for(x=-128;x<0;x++)for(y=-32;y<0;y++){if(2*y-x< -64)continue;assert(PortPreliminarPixelSource(p,x,y,&sx,&sy));assert(sx>=128&&sx<192&&sy>=192&&sy<224);}
return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code)
   exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   subprocess.run([compiler,'-I',str(ROOT/'port/include/port'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
