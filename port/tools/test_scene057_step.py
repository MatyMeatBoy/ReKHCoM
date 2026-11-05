"""Compile the actual scene057 profile and check the right-hand window step."""
import os,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <assert.h>
#include "port/port_scene057_tiles.h"
int main(void){
 for(int x=28;x<64;x++)for(int y=-8;y<8;y++){
  int sx=x,sy=y;assert(PortScene057TileSource(2,&sx,&sy));
  int phase=(x-28)%12,ex=phase<4?8+phase:phase-4,ey=y+(x-ex)/2;
  if(ey<0||ey>=32||(ex>=8&&ey<8)){assert(sx==0&&sy==0);continue;}
  assert(sx==ex&&sy==ey);assert(2*sy+sx==2*y+x);
 }
 for(int x=-64;x<96;x++)for(int y=-16;y<80;y++){
  if(x>=0&&x<32&&y>=0&&y<32)continue;
  if(2*y+x<32||(x>=31&&x<35&&y>=5&&y<12))continue;
  int sx=x,sy=y;assert(PortScene057TileSource(3,&sx,&sy));
  int lx=PortSceneTileMod(x,16),ly=PortSceneTileMod(y-23,12);
  int cx=x-lx,cy=y-ly;
  if(lx<12&&ly<8&&2*cy+cx+6>=32){assert(sx==lx&&sy==23+ly);}
  else{assert(sx==PortSceneTileMod(x,2)&&sy==30+PortSceneTileMod(y,2));}
  assert(sx>=0&&sx<12&&sy>=23&&sy<32);
 }
 for(int x=31;x<35;x++)for(int y=5;y<12;y++){
  int sx=x,sy=y;assert(PortScene057TileSource(3,&sx,&sy));assert(sx==x-3&&sy==y+10);
 }
 for(int x=0;x<5;x++)for(int y=18;y<23;y++){
  int sx=x,sy=y;assert(PortScene057TileSource(3,&sx,&sy));assert(sx==x&&sy==y);
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
