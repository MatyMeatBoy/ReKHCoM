"""Exercise the real Agrabah resolver where the first facade leaves the map."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]


class Agrabah107Roof(unittest.TestCase):
    def test_complete_face_and_native_roof(self):
        header=(ROOT/'port/include/port/port_agrabah_tiles.h').read_text()
        missing='' if 'PortAgrabahPixelSource(' in header else '''
static int PortAgrabahPixelSource(int bg,int x,int y,int*sx,int*sy){return 0;}
'''
        code='''#include <assert.h>
#include "port/port_agrabah_tiles.h"
'''+missing+r'''
static void Resolve(int bg,int x,int y,int*sx,int*sy){
 if(PortAgrabahPixelSource(bg,x,y,sx,sy))return;
 int tx=x>>3,ty=y>>3;
 if(!PortAgrabahTileSource(bg,&tx,&ty,0)){*sx=*sy=-1;return;}
 *sx=tx*8+(x&7);*sy=ty*8+(y&7);
}
int main(void){int sx,sy;
 /* The native first-front corner is (200,368): invariant536.
  * Its complete parallel face at x256..287 retains the foot. */
 Resolve(3,600,568,&sx,&sy);assert(sx==280&&sy==408);
 Resolve(2,500,530,&sx,&sy);assert(sx==276&&sy==418);
 /* At the bottom-right handoff the previous tall wall still continues
  * until invariant536. Painting roof here creates one final rectangular bite. */
 Resolve(3,500,512,&sx,&sy);assert(sx==468&&sy==496);
 /* The last BG2 block is transparent padding, not a complete facade. */
 Resolve(2,500,500,&sx,&sy);assert(sx==468&&sy==484);
 for(int x=512;x<1536;x++)for(int d=536+(x&1);d<576;d+=2){
  int y=(x+d)/2;
  Resolve(2,x,y,&sx,&sy);
  assert(sx>=256&&sx<288&&sy>=0&&sy<512);
  assert(2*sy-sx==d); /* same pixel phase across repeated donor columns */
 }
 /* Keep the accepted yellow roof and the left extension exactly intact. */
 Resolve(3,800,100,&sx,&sy);assert(sx==480&&sy==244);
 Resolve(2,800,100,&sx,&sy);assert(sx==-1&&sy==-1);
 Resolve(3,-128,700,&sx,&sy);assert(sx==0&&sy==508);
 for(int bg=2;bg<=3;bg++)for(int y=240;y<640;y+=3)for(int x=448;x<640;x++){
  int expected=0,run=8-PortSceneTileMod(x,8);
  for(int k=0;k<run;k++)expected|=PortAgrabahPixelSource(bg,x+k,y,&sx,&sy);
  assert(!!PortAgrabahPixelRunNeeded(bg,x,y)==!!expected);
 }
 for(int bg=2;bg<=3;bg++)for(int y=0;y<512;y++)for(int x=0;x<512;x++){
  int padding=bg==2&&x>=496&&y>=496;
  assert(!!PortAgrabahPixelRunNeeded(bg,x,y)==!!padding);
  Resolve(bg,x,y,&sx,&sy);
  assert(sx==x-(padding?32:0)&&sy==y-(padding?16:0));
 }
 return 0;
}
'''
        compiler=os.environ.get('CC') or shutil.which('gcc') or shutil.which('cc')
        local=ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
        if not compiler and os.name=='nt' and local.is_file():compiler=str(local)
        if not compiler:self.skipTest('Set CC to a native C compiler')
        with tempfile.TemporaryDirectory() as directory:
            src=Path(directory)/'roof.c';exe=Path(directory)/'roof.exe';src.write_text(code)
            p=subprocess.run([compiler,'-I',str(ROOT/'port/include'),str(src),'-o',str(exe)],capture_output=True,
                             creationflags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0))
            self.assertEqual(p.returncode,0,p.stderr.decode(errors='replace'))
            p=subprocess.run([str(exe)],capture_output=True)
            self.assertEqual(p.returncode,0,p.stderr.decode(errors='replace'))


if __name__=='__main__':unittest.main()
