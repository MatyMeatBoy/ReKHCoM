"""Run the actual actor gate with a shadow whose OAM slot belonged to the party."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class Scene112Shadow(unittest.TestCase):
    def test_only_owned_shadows_use_scene112_entry_gate(self):
        source = (ROOT / 'port/src/port_ppu.c').read_text(encoding='utf-8')
        start = source.index('if (sceneActorFrame && !sOamTag[i].screen') + 4
        depth = 1
        end = start
        while depth:
            if source[end] == '(':
                depth += 1
            elif source[end] == ')':
                depth -= 1
            end += 1
        condition = source[start:end-1]
        code = r'''
#include <assert.h>
static int gAladdinPalette[1],gGeniePalette[1],gCheshirePalette[1],gHookPalette[1],gJiminyPalette[1],gMarefPalette[1];
static int Preliminar130Shadow(int i){(void)i;return 0;}
static int gBtlShadowTiles[1],gBtlShadowSmallTiles[1],gBtlShadowLargeTiles[1];
static struct{int screen,hud,text,eventShadowActor;}sOamTag[1];
static struct{const void*palette,*tiles;}sOamHd[1];
static int Eligible(int sceneActorId,const void*p,const void*t,int owner){
 int i=0,sceneActorFrame=1,prio=2;
 sOamHd[0].palette=p;sOamHd[0].tiles=t;sOamTag[0].eventShadowActor=owner;
 return ''' + condition + r''';}
int main(void){
 assert(!Eligible(112,0,gBtlShadowTiles,0)); /* Jafar/party shadow must draw normally */
 assert(!Eligible(112,0,gBtlShadowTiles,255)); /* old checkpoint padding is not a role */
 assert(Eligible(112,0,gBtlShadowTiles,3));
 assert(Eligible(112,0,gBtlShadowSmallTiles,2));
 assert(Eligible(112,gGeniePalette,0,0));
 assert(Eligible(112,gAladdinPalette,0,0));
 assert(Eligible(118,0,gBtlShadowTiles,0));return 0;}
'''
        self.run_c(code)

    def run_c(self, code):
        compiler = os.environ.get('CC') or shutil.which('gcc') or shutil.which('cc')
        local = ROOT.parent.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
        if not compiler and os.name == 'nt' and local.is_file():
            compiler = str(local)
        if not compiler:
            self.skipTest('Set CC to a native C compiler')
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory) / 'gate.c'
            exe = Path(directory) / 'gate.exe'
            src.write_text(code)
            compiled = subprocess.run([compiler, str(src), '-o', str(exe)], capture_output=True,
                                      creationflags=getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0))
            self.assertEqual(compiled.returncode, 0, compiled.stderr.decode(errors='replace'))
            result = subprocess.run([str(exe)], capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace'))

    def test_emitter_tags_exact_owner_even_when_body_floats(self):
        source = (ROOT / 'src/evt/evt_obj.c').read_text(encoding='utf-8')
        function = source.split('void task_evt_shadow_2(')[1].split('void task_evt_shadow_3(')[0]
        code = r'''
#include <assert.h>
#include <stddef.h>
#define PLATFORM_PC 1
#define FALSE 0
#define EVENT_112_AGRABAH_E3 112
#define EVTOBJ_FLAG_NO_SHADOW 4
#define EVTOBJ_FLAG_SHADOW_WIDE 8
#define EVTOBJ_FLAG_SHADOW_SMALL 16
#define Q_8_8(x) ((int)((x)*256))
typedef int s32;typedef int ObjAffine;
typedef struct{int x,y,z,groundZ,flags,drawFlags;}EvtObj;
typedef struct{EvtObj*obj;void*palette,*tiles,*tiles2,*tiles3;}EvtShadowWork;
static struct{int eventId,x,y;EvtObj*charaObjs[16];}state,*gEventState=&state;
static int frame;static void*gBtlShadowLargeFrame0=&frame,*gBtlShadowSmallFrame0=&frame,*gBtlShadowFrame0=&frame;
static int context,emitted=-1,draws;
static void PortSpriteEventShadowOwner(int role){context=role;}
static ObjAffine*AllocObjAffine(int a,int b,int c,int d){return 0;}
static int DrawSprite(int x,int y,void*a,void*b,void*c,void*d,int e,int f){
 emitted=context;draws++;return 0; /* even a rejected draw must reset context */}
void task_evt_shadow_2(''' + function + r'''
int main(void){EvtObj aladdin={0},genie={0},party={0};EvtShadowWork w={0};
 state.eventId=112;state.charaObjs[3]=&aladdin;state.charaObjs[4]=&genie;
 genie.z=-7680;genie.groundZ=0;w.obj=&genie;
 task_evt_shadow_2(&w);assert(emitted==3&&context==0);
 w.obj=&aladdin;task_evt_shadow_2(&w);assert(emitted==2&&context==0);
 w.obj=&party;task_evt_shadow_2(&w);assert(emitted==0&&context==0);
 state.eventId=111;w.obj=&genie;task_evt_shadow_2(&w);assert(emitted==0&&context==0);
 state.eventId=112;genie.flags=4;int before=draws;
 task_evt_shadow_2(&w);assert(draws==before&&context==0);return 0;}
'''
        self.run_c(code)

    def test_shadow_uses_its_real_actor_and_cannot_unlock_that_actor(self):
        source = (ROOT / 'port/src/port_ppu.c').read_text(encoding='utf-8')
        start = source.index('            int hookShadow = sceneActorId == 118')
        block = source[start:source.index('        line = gy - oy;', start)].rstrip().rsplit('}', 1)[0]
        arrays = '\n'.join(line for line in source.splitlines() if
                           line.startswith('static unsigned sStagedActorFirstSeen[') or
                           line.startswith('static unsigned char sStagedActorEntered['))
        code = r'''
#include <assert.h>
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
static int gCheshirePalette[1],gAladdinPalette[1],gGeniePalette[1],gHookPalette[1];
static int gBtlShadowTiles[1],gBtlShadowSmallTiles[1],gBtlShadowLargeTiles[1];
static int sceneActorId=112;static unsigned sActorSceneFrames;
static unsigned char sActorEntered[1]={1}; /* a party actor previously entered */
static struct {const void*palette;const void*tiles;}sOamHd[1];
static struct {unsigned char eventShadowActor;}sOamTag[1];
''' + arrays + r'''
static int Visible(const void*p,const void*t,int owner,int ox,unsigned age){
 int i,visible=0,oy=50,bw=32,bh=64;
 sOamHd[0].palette=p;sOamHd[0].tiles=t;
 sOamTag[0].eventShadowActor=owner;sActorSceneFrames=age;
 for(i=0;i<1;i++){
''' + block + r'''
 visible=1;}return visible;}
int main(void){
 /* Owner 3 encodes Genie role2. Slot0 has already been used by the party. */
 assert(!Visible(0,gBtlShadowTiles,3,300,10));
 assert(!sStagedActorEntered[2]);
 assert(Visible(0,gBtlShadowTiles,3,100,11)); /* never remove native pixels */
 assert(!sStagedActorEntered[2]); /* shadow alone must not grant body entry */
 assert(!Visible(gGeniePalette,0,0,300,12));
 assert(Visible(gGeniePalette,0,0,100,16));
 assert(sStagedActorEntered[2]);
 assert(Visible(0,gBtlShadowTiles,3,300,17)); /* continuity after entry */
 assert(!Visible(0,gBtlShadowSmallTiles,2,300,18)); /* independent Aladdin */
 assert(!sStagedActorEntered[1]);
 assert(Visible(0,gBtlShadowTiles,0,300,19)); /* unowned party keeps its slot */
 assert(Visible(0,gBtlShadowTiles,255,300,20)); /* invalid restored tag stays unowned */
 return 0;
}
'''
        self.run_c(code)


if __name__ == '__main__':
    unittest.main()
