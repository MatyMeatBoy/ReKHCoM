"""Compile the actual Maleficent gate, keeping unrelated actors and shadows."""
import os, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]


class LibraryStagingTest(unittest.TestCase):
    def test_maleficent_palette_role_and_exact_event_gate_are_registered(self):
        source=(ROOT/'port/src/port_ppu.c').read_text()
        self.assertTrue('gMarefPalette' in source, 'Maleficent palette is not registered')
        self.assertTrue('sStagedActorEntered[6]' in source, 'Six stable actor roles are required')
        self.assertTrue('sActorSceneKey == EVENT_130_HOLLOWBASTION_E1' in source, 'Exact regional event gate is absent')

    def test_actual_actor_gate_and_owned_shadows(self):
        source=(ROOT/'port/src/port_ppu.c').read_text()
        self.assertTrue('static int Preliminar130Shadow(' in source, 'Owned Maleficent shadow classifier is absent')
        compiler=os.environ.get('CC') or shutil.which('gcc')
        if not compiler:
            compiler=str(ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
        start=source.index('        if (sceneActorFrame && !sOamTag[i].screen')
        block=source[start:source.index('        line = gy - oy;',start)]
        helper_start=source.index('static int Preliminar130Shadow(')
        helper=source[helper_start:source.index('static void DrawObjs(',helper_start)].replace('Preliminar130Shadow','ActualOwnedShadow')
        tag_start=source.index('void PortOamSetSource(')
        tagging=source[tag_start:source.index('int PortPpuOverlayActive(',tag_start)]
        arrays='\n'.join(l for l in source.splitlines() if l.startswith(('static unsigned sStagedActorFirstSeen[','static unsigned char sStagedActorEntered[')))
        code=r'''#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
static int gCheshirePalette[1],gAladdinPalette[1],gGeniePalette[1],gMarefPalette[1],gHookPalette[1],partyPalette[1];
static int gBtlShadowTiles[1],gBtlShadowSmallTiles[1],gBtlShadowLargeTiles[1];
static struct {const void*palette;const void*tiles;}sOamHd[4];
typedef struct {int screen,hud,text,anchorX,anchorY,anchorValid;} Tag;
static Tag sOamTag[4],sEntryTag[128];
#define PORT_OAM_CAPACITY 4
static struct {int entryCount;struct {int x,y;}entries[128];} work,*gSpriteWork=&work;
static unsigned sActorSceneFrames;static unsigned char sActorEntered[1];
static int ownedShadow;
static int Preliminar130Shadow(int i){return ownedShadow;}
static struct{int x,y;}sOamExt[4];
static int attrs[4][2];
static int OamAttr(int i,int a){return attrs[i][a];}
static int OamCount(void){return 4;}
'''+tagging+helper+arrays+r'''
static int Visible(const void*p,const void*t,int ox,int oy,unsigned age,int owned){int i,visible=0,bw=32,bh=64,sceneActorFrame=1,sceneActorId=130,prio=0;sOamHd[0].palette=p;sOamHd[0].tiles=t;sActorSceneFrames=age;ownedShadow=owned;for(i=0;i<1;i++){
'''+block+r'''
visible=1;}return visible;}
int main(void){
work.entryCount=2;work.entries[0].x=100;work.entries[0].y=100;
work.entries[1].x=101;work.entries[1].y=100;
PortOamSetSource(0,0);PortOamSetSource(1,0);
sOamHd[0].tiles=gBtlShadowTiles;sOamHd[1].palette=gMarefPalette;
assert(sOamTag[0].anchorValid && sOamTag[0].anchorX==100 && sOamTag[0].anchorY==100);
assert(ActualOwnedShadow(0));
PortOamSetSource(0,1);assert(!ActualOwnedShadow(0)); /* nearby is not the same emitter anchor */
PortOamSetSource(0,0);sOamTag[1].hud=1;assert(!ActualOwnedShadow(0));sOamTag[1].hud=0;
assert(sizeof(sStagedActorEntered)>=6);
sActorEntered[0]=1; /* a recycled party OAM slot does not admit Maleficent */
assert(Visible(partyPalette,0,300,0,10,0));
assert(Visible(0,gBtlShadowTiles,300,0,10,0)); /* unrelated party shadow */
assert(!Visible(gMarefPalette,0,300,0,11,0));
sOamTag[0].screen=1;assert(Visible(gMarefPalette,0,300,0,11,0));sOamTag[0].screen=0;
assert(!sStagedActorEntered[5]);
assert(Visible(0,gBtlShadowTiles,100,100,12,1)); /* native shadow preserved */
assert(!sStagedActorEntered[5]);
assert(!Visible(0,gBtlShadowTiles,300,0,13,1));
assert(Visible(gMarefPalette,0,100,50,16,0));
assert(sStagedActorEntered[5]);
assert(Visible(gMarefPalette,0,300,0,17,0));
assert(Visible(0,gBtlShadowTiles,300,0,17,1));
return 0;}
'''
        with tempfile.TemporaryDirectory() as directory:
            src=Path(directory)/'check.c';src.write_text(code)
            exe=Path(directory)/('check.exe' if os.name=='nt' else 'check')
            subprocess.run([compiler,str(src),'-o',str(exe)],check=True,capture_output=True,
                           creationflags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0))
            subprocess.run([str(exe)],check=True,capture_output=True)


if __name__=='__main__':unittest.main()
