"""Run the actual scene120 actor filter, including decorative multi-part OBJ."""
import os,shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene120Test(unittest.TestCase):
 def test_actor_filter(self):
  compiler=os.environ.get('CC') or shutil.which('gcc') or str(ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe')
  source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
  start=source.index('        if (sceneActorFrame && !sOamTag[i].screen')
  block=source[start:source.index('        line = gy - oy;',start)]
  arrays='\n'.join(l for l in source.splitlines() if l.startswith(('static unsigned sStagedActorFirstSeen[','static unsigned char sStagedActorEntered[')))
  code=r'''#include <assert.h>
#include <stddef.h>
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
static int gCheshirePalette[1],gAladdinPalette[1],gGeniePalette[1],gHookPalette[1],gJiminyPalette[1],gMarefPalette[1];
static int gBtlShadowTiles[1],gBtlShadowSmallTiles[1],gBtlShadowLargeTiles[1];
static struct {const void*palette;const void*tiles;}sOamHd[1];
static struct{int screen,hud,text,eventShadowActor;}sOamTag[1];
static int Preliminar130Shadow(int i){(void)i;return 0;}
static unsigned sActorSceneFrames;static unsigned char sActorEntered[1];
'''+arrays+r'''
static int Visible(const void*p,const void*t,int ox,int oy,unsigned age){int i,visible=0,bw=32,bh=64,sceneActorFrame=1,sceneActorId=120,prio=1;sOamHd[0].palette=p;sOamHd[0].tiles=t;sActorSceneFrames=age;for(i=0;i<1;i++){
'''+block+r'''
visible=1;}return visible;}
int main(void){assert(Visible(0,0,100,-80,10));assert(Visible(0,0,300,0,10));assert(!Visible(gJiminyPalette,0,300,0,11));assert(Visible(0,gBtlShadowTiles,100,100,12));assert(!sStagedActorEntered[4]);assert(!Visible(0,gBtlShadowTiles,300,0,13));assert(Visible(gJiminyPalette,0,100,50,16));assert(sStagedActorEntered[4]);assert(Visible(gJiminyPalette,0,300,0,17));assert(Visible(0,gBtlShadowTiles,300,0,17));return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code);exe=Path(d)/'check.exe'
   subprocess.run([compiler,str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
