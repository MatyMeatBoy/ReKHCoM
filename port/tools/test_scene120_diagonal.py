"""The right wall has a different native footing from the front wall."""
import os,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene120Diagonal(unittest.TestCase):
 def test_distinct_native_planes(self):
  cc=os.environ.get('CC') or shutil.which('gcc') or str(ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe')
  code=r'''#include <assert.h>
#include "port/port_preliminar_v1_tiles.h"
int main(void){
 PortPreliminarRecipe p={0};int sx,sy,x,y;p.v[0]=120;p.v[1]=2;
 /* Native right plane starts at2y+x176, front at208. */
 assert(PortPreliminarPixelSource(&p,256,-40,&sx,&sy));assert(sx==128&&sy==24);
 assert(PortPreliminarPixelSource(&p,272,-40,&sx,&sy));assert(sx==128&&sy==32);
 assert(PortPreliminarPixelSource(&p,256,-24,&sx,&sy));assert(sx==255&&sy==255);
 assert(PortPreliminarPixelSource(&p,-16,112,&sx,&sy));assert(sx==64&&sy==72);
 /* The right plane also exits the map's TOP before its right edge. */
 assert(PortPreliminarPixelSource(&p,224,-24,&sx,&sy));assert(sx==128&&sy==24);
 for(p.v[1]=2;p.v[1]<=3;p.v[1]++)for(y=0;y<256;y++)for(x=0;x<256;x++)
  assert(!PortPreliminarPixelSource(&p,x,y,&sx,&sy));
 p.v[1]=3;
 assert(PortPreliminarPixelSource(&p,256,-40,&sx,&sy));assert(sy>=160);
 assert(PortPreliminarPixelSource(&p,256,-48,&sx,&sy));assert(sy<32);
 /* Full masonry A above the native map: do not collapse its64px patch
  * back to the first16px, which repeated half bricks. */
 assert(PortPreliminarPixelSource(&p,64,-32,&sx,&sy));assert(sx==64&&sy==0);
 assert(PortPreliminarPixelSource(&p,96,-16,&sx,&sy));assert(sx==32&&sy==16);
 assert(PortPreliminarPixelSource(&p,-16,112,&sx,&sy));assert(sy>=160);
 return 0;
}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code);exe=Path(d)/'check.exe'
   subprocess.run([cc,'-std=c11','-I',str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True,capture_output=True)
   run=subprocess.run([str(exe)],capture_output=True,text=True);self.assertEqual(run.returncode,0,run.stderr)
if __name__=='__main__':unittest.main()
