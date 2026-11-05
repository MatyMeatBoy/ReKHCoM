"""Compile the scene110 resolver and verify a whole wall module at its right seam."""
import os,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <assert.h>
#include "port/port_preliminar_v1_tiles.h"
int main(void){PortPreliminarRecipe p={0};p.v[0]=110;p.v[1]=3;p.v[4]=p.v[5]=64;
 for(int x=512;x<2048;x++)for(int y=-256;y<72;y++){
  int sx=-1,sy=-1;assert(PortPreliminarPixelSource(&p,x,y,&sx,&sy));
  int h=PortSceneTileMod(x-160,704),expected=160+(h<352?h:703-h);
  assert(sx==expected&&sx>=160&&sx<512);
  if(y<0){int v=PortSceneTileMod(-y-1,128);assert(sy==(v<64?v:127-v));}else assert(sy==y);
 }
 for(int y=0;y<72;y++){int sx=-1,sy=-1;assert(PortPreliminarPixelSource(&p,512,y,&sx,&sy));assert(sx==511&&sy==y);}
 for(int x=512;x<2048;x++){int sx=-1,sy=-1;assert(PortPreliminarPixelSource(&p,x,72,&sx,&sy));assert(sx==320+PortSceneTileMod(x-320,192)&&sy==72);}
 for(int x=0;x<512;x+=7)for(int y=0;y<512;y+=7){int sx=-1,sy=-1;assert(!PortPreliminarPixelSource(&p,x,y,&sx,&sy));}
 return 0;}
'''
class Scene110Test(unittest.TestCase):
 def test_wall_module_reflects_at_its_actual_edge(self):
  compiler=os.environ.get('CC') or shutil.which('gcc') or str(ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe')
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(CODE);exe=Path(d)/'check.exe'
   subprocess.run([compiler,'-I',str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True,capture_output=True)
   result=subprocess.run([str(exe)],capture_output=True,text=True)
   self.assertEqual(result.returncode,0,result.stderr)
if __name__=='__main__':unittest.main()
