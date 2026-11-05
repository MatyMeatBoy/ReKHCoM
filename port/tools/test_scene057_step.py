"""Compile the actual scene057 profile and check the right-hand window step."""
import os,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <assert.h>
#include "port/port_scene057_tiles.h"
int main(void){
 for(int x=28;x<35;x++)for(int y=-8;y<8;y++){
  int sx=x,sy=y;assert(PortScene057TileSource(2,&sx,&sy));
  if(x<32&&y+10<8){assert(sx==0&&sy==0);continue;}
  assert(sx>=0&&sx<32&&sy>=0&&sy<32);
  assert(2*sy+sx==2*y+x);
  if(x<32){assert(sx==x-20&&sy==y+10);}else{assert(sx==x-28&&sy==y+14);}
 }
 for(int bg=2;bg<=3;bg++)for(int x=5;x<28;x++)for(int y=0;y<32;y++){
  int sx=x,sy=y;assert(PortScene057TileSource(bg,&sx,&sy));assert(sx==x&&sy==y);
 }
 return 0;
}
'''
class Scene057Test(unittest.TestCase):
 def test_caps_keep_diagonal_and_other_native_entries(self):
  compiler=os.environ.get('CC') or shutil.which('gcc') or str(ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe')
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(CODE);exe=Path(d)/'check.exe'
   subprocess.run([compiler,'-I',str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
