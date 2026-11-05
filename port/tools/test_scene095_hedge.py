"""Native hedge donors must join the authored face at its actual height."""
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
for(wx=480;wx<704;wx++)for(wy=176;wy<420;wy++){
 int d=2*wy-wx;if(d< -128||d>=16)continue;
 assert(PortPreliminarPixelSource(p,wx,wy,&sx,&sy));assert(sx>=64&&sx<160);assert(2*sy+sx==d+656||2*sy+sx==d+655);
}
/* The original right face ends at diagonal16: no 40px drop into the lawn. */
for(wx=480;wx<704;wx++)for(wy=248;wy<420;wy++){
 int d=2*wy-wx;if(d<16||d>=96)continue;
 int tx=wx>>3,ty=wy>>3,ok=PortPreliminarTileSource(p,&tx,&ty);
 assert(PortPreliminarPixelSource(p,wx,wy,&sx,&sy)==ok);
 if(ok){assert(sx==tx*8+(wx&7));assert(sy==ty*8+(wy&7));}
}return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code)
   exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   flags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0)
   subprocess.run([compiler,'-I',str(ROOT/'port/include/port'),str(src),'-o',str(exe)],check=True,capture_output=True,creationflags=flags)
   result=subprocess.run([str(exe)],capture_output=True,text=True,creationflags=flags)
   self.assertEqual(result.returncode,0,result.stderr)
if __name__=='__main__':unittest.main()
