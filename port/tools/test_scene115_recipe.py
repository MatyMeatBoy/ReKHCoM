"""Compiled regression for the scene115 exterior-only recipe."""
import os, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene115RecipeTest(unittest.TestCase):
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
 for(i=0;i<sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]);i++)if(kPreliminarRecipes[i].v[0]==115&&kPreliminarRecipes[i].v[1]==bg){p=&kPreliminarRecipes[i];break;}
 assert(p);
 for(y=-32;y<80;y++)for(x=-64;x<96;x++){
  int sx=x,sy=y,ok=PortPreliminarTileSource(p,&sx,&sy);
  if(x>=0&&x<32&&y>=0&&y<32){assert(ok&&sx==x&&sy==y);}
  else if(bg==3&&2*y+x>=36){assert(ok&&sy>=20&&sy<24);}
 }
}
{
 const PortPreliminarRecipe* p=0;int k,j,dx,dy;
 for(i=0;i<sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]);i++)if(kPreliminarRecipes[i].v[0]==115&&kPreliminarRecipes[i].v[1]==2){p=&kPreliminarRecipes[i];break;}
 assert(p);
 for(k=-8;k<=12;k++)for(j=0;j<6;j++){
  int cx=10+8*k,cy=19-4*k+8*j;
  int overlap=cx-4<32&&cx+4>0&&cy-2<32&&cy+3>0;
  if(overlap?j>=2:j%2)continue;
  for(dy=-2;dy<3;dy++)for(dx=-4;dx<4;dx++){
   int wx=cx+dx,wy=cy+dy,sx=wx,sy=wy;
   if(wx>=0&&wx<32&&wy>=0&&wy<32)continue;
   assert(PortPreliminarTileSource(p,&sx,&sy));
   assert(sx==10+dx&&sy==19+dy);
  }
 }
}return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code)
   exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   subprocess.run([compiler,'-I',str(ROOT/'port/include/port'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
