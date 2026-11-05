"""Compiled regression for the scene116 exterior-only recipe."""
import os, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene116RecipeTest(unittest.TestCase):
 def test_native_identity_and_clean_exterior_floor(self):
  compiler=os.environ.get('CC') or shutil.which('gcc')
  local=ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
  if not compiler and local.is_file():compiler=str(local)
  if not compiler:self.skipTest('C compiler unavailable')
  code=r'''#include <assert.h>
#include "port_preliminar_v1_tiles.h"
int main(void){ unsigned i; int x,y,bg;
for(bg=2;bg<=3;bg++){
 const PortPreliminarRecipe* p=0;
 for(i=0;i<sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]);i++)if(kPreliminarRecipes[i].v[0]==116&&kPreliminarRecipes[i].v[1]==bg){p=&kPreliminarRecipes[i];break;}
 assert(p);
 for(y=-32;y<80;y++)for(x=-64;x<96;x++){
  int sx=x,sy=y,ok=PortPreliminarTileSource(p,&sx,&sy);
  if(x>=0&&x<32&&y>=0&&y<32){assert(ok&&sx==x&&sy==y);}
  else if(bg==3&&2*y+x>=(x>=28?48:32)&&2*y+x<60){assert(ok&&sy>=20&&sy<24);}
 }
}return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code)
   exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   subprocess.run([compiler,'-I',str(ROOT/'port/include/port'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
