"""Compile the actual scene118 actor gate, including shared shadow ownership."""
import os, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene118StagingTest(unittest.TestCase):
 def test_hook_has_independent_actor_slot_and_scene_gate(self):
  compiler=os.environ.get('CC') or shutil.which('gcc')
  local=ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
  if not compiler and local.is_file():compiler=str(local)
  if not compiler:self.skipTest('C compiler unavailable')
  source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
  start=source.index('    if (gPortScreen.wide && !sEnvActorFrameOff) {', source.index('static void DrawObjs('))
  end=source.index('    int mapping1d',start)
  gate=source[start:end]
  start=source.index('            int hookShadow = sceneActorId == 118')
  role=source[start:source.index('            unsigned char* entered',start)]
  block=source[start:source.index('        line = gy - oy;',start)].rstrip().rsplit('}',1)[0]
  arrays='\n'.join(line for line in source.splitlines() if line.startswith('static unsigned sStagedActorFirstSeen[') or line.startswith('static unsigned char sStagedActorEntered['))
  code=r'''#include <assert.h>
#include <stddef.h>
static int gCheshirePalette[1],gAladdinPalette[1],gGeniePalette[1],gHookPalette[1];
static struct{int wide;}gPortScreen={1};static int sEnvActorFrameOff;
static struct Recipe{int v[22];} recipe,*sPreliminarRecipe[4];
static int gBtlShadowTiles[1],gBtlShadowSmallTiles[1],gBtlShadowLargeTiles[1];
static int sceneActorId=118;static unsigned sActorSceneFrames;static unsigned char sActorEntered[1];
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
static struct {const void*palette;const void*tiles;}sOamHd[1];
'''+arrays+r'''
static int Role(const void*p){int i=0;sOamHd[0].palette=p;
'''+role+r'''
return actor;}
static int Gate(int scene){int sceneActorFrame=0,sceneActorId=0;recipe.v[0]=scene;sPreliminarRecipe[1]=&recipe;
'''+gate+r'''
return sceneActorId;}
static int Visible(const void*p,const void*t,int ox,int oy,unsigned age){int i,visible=0,bw=32,bh=64;sOamHd[0].palette=p;sOamHd[0].tiles=t;sActorSceneFrames=age;
for(i=0;i<1;i++){
'''+block+r'''
visible=1;}return visible;}
int main(void){assert(Gate(118)==118);assert(Gate(112)==112);assert(Gate(97)==97);assert(Gate(120)==120);assert(Gate(117)==0);assert(Role(gHookPalette)==3);assert(Role(gCheshirePalette)==0);assert(Role(gAladdinPalette)==1);assert(Role(gGeniePalette)==2);assert(sizeof(sStagedActorEntered)/sizeof(sStagedActorEntered[0])>=4);assert(sizeof(sStagedActorFirstSeen)/sizeof(sStagedActorFirstSeen[0])>=4);assert(Visible(0,gBtlShadowTiles,100,100,10));assert(!sStagedActorEntered[3]);assert(!Visible(gHookPalette,0,300,0,11));assert(!Visible(0,gBtlShadowTiles,300,0,12));assert(Visible(gHookPalette,0,100,50,16));assert(sStagedActorEntered[3]);assert(Visible(0,gBtlShadowTiles,300,0,17));return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code)
   exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   subprocess.run([compiler,'-I',str(ROOT/'port/include/port'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
