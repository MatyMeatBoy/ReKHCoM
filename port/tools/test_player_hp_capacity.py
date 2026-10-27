"""Actual GBA HP reward and port gauge controls; no ROM, game or GPU.
Private remaster constants/layout are verified separately in audit evidence.
"""
from pathlib import Path
import re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def fn(s,name):
 a=s.rfind("\n",0,s.index(name+"("))+1;b=s.index("{",a);e=b+1;d=1
 while d:d+=(s[e]=="{")-(s[e]=="}");e+=1
 return s[a:e]
s=(ROOT/'port/src/port_hdlayout.c').read_text(encoding='utf8')
a=s.index('#define GAUGE_LEN');b=s.index('static void GaugeParts(',a)
core=s[a:b]+fn(s[s.rindex('static void GaugeParts('):],'GaugeParts')
gba=(ROOT/'src/player_progression.c').read_text(encoding='utf8')
initial=re.search(r'progression->maxHp = (0x[0-9A-Fa-f]+|[0-9]+);',fn(gba,'InitPlayerProgression')).group(1)
core+='\n'+fn(gba,'LevelUpMaxHp')
pre=r'''
#include <stdio.h>
#include <stdint.h>
#include <string.h>
typedef int SDL_Renderer;typedef int s32;
#define SDL_clamp(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))
#define HD_LAYOUT_GAUGE 0
#define HD_LAYOUT_TEX_GAUGE 0
#define HD_LAYOUT_TEX_GAUGE_FACES 1
#define HD_LAYOUT_TEX_RIKU_FACES 2
#define HD_LAYOUT_HOLD(v) ((v)|0x100000)
static struct{int hdCards;}gPortConfig={1};
static struct{struct{int maxHp;}progression;}gGameState;
static unsigned frame;
static int ready=1,controls[16];
static unsigned PortFrameCount(void){return frame;}
static int PortHdLayoutGroupFrames(int l,int seq,int group){(void)l;(void)seq;(void)group;return 1;}
static int PortHdLayoutReady(int l,int t){(void)l;(void)t;return ready;}
static void PortHdLayoutDraw(SDL_Renderer*r,int l,int t,int seq,int group,int f,float x,float y,float scale,float alpha,const float*tint){(void)r;(void)l;(void)t;(void)seq;(void)x;(void)y;(void)scale;(void)alpha;(void)tint;if(seq==1)controls[group]=f&0xffff;}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 77;}}while(0)
'''
test=r'''
int main(void){
 gGameState.progression.maxHp=INITIAL_HP;
 for(int n=0;n<40;n++){int before=gGameState.progression.maxHp;CHECK(LevelUpMaxHp()==15);CHECK(gGameState.progression.maxHp==(before+15>560?560:before+15));}
 CHECK(gGameState.progression.maxHp==560);
 int hp[]={0,1,80,155,280,281,320,321,322,323,560,1000};
 int expected[]={0,0,67,130,235,235,268,269,270,271,470,470};
 for(unsigned i=0;i<sizeof(hp)/sizeof(*hp);i++)CHECK(GAUGE_LEN(hp[i])==expected[i]);
 for(int riku=0;riku<2;riku++)for(int maximum=80;maximum<=560;maximum+=15){
  memset(&sGauge,0,sizeof(sGauge));frame++;
  CHECK(PortHdGauge(maximum,maximum,riku));GaugeParts(NULL,0,0,1,NULL);
  CHECK(controls[7]==GAUGE_LEN(maximum)&&controls[8]==controls[7]&&controls[9]==controls[7]);
 }
 memset(&sGauge,0,sizeof(sGauge));frame++;CHECK(PortHdGauge(155,155,0));
 frame++;CHECK(PortHdGauge(100,155,0));CHECK(sGauge.hp==100&&sGauge.trail==155);
 GaugeParts(NULL,0,0,1,NULL);CHECK(controls[7]==130&&controls[8]==130&&controls[9]==83);
 for(int n=0;n<88;n++){frame++;PortHdGauge(100,155,0);}CHECK(sGauge.trail==155);
 frame++;PortHdGauge(100,155,0);CHECK(sGauge.trail==149);
 frame++;PortHdGauge(155,155,0);CHECK(sGauge.trail==155&&sGauge.delayMs==0);
 frame+=2;PortHdGauge(1,155,0);CHECK(sGauge.trail==1); /* discontinuity resets stale trail */
 memset(&sGauge,0,sizeof(sGauge));frame++;PortHdGauge(38,155,0);frame++;PortHdGauge(38,155,0);CHECK(sGauge.stateAnim==10&&sGauge.faceAnim==0);
 frame++;PortHdGauge(39,155,0);CHECK(sGauge.stateAnim==15&&sGauge.faceAnim==3);
 memset(&sGauge,0,sizeof(sGauge));frame++;PortHdGauge(0,155,0);frame++;PortHdGauge(0,155,0);CHECK(sGauge.stateAnim==15&&sGauge.faceAnim==2);
 ready=0;CHECK(!PortHdGauge(155,155,0));ready=1;gPortConfig.hdCards=0;CHECK(!PortHdGauge(155,155,0));
 puts("PASS actual GBA HP rewards and HD capacity/fill/trail/state controls for Sora/Riku; no visual parity claim");return 0;
}
'''.replace('INITIAL_HP',initial)
base=next(p for p in ROOT.parents if (p/'toolchain').is_dir());cc=next((base/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='player-hp-capacity-') as d:
 p=Path(d);(p/'test.c').write_text(pre+core+test,encoding='utf8')
 subprocess.run([str(cc),'-std=gnu11','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test.exe')],check=True)
 r=subprocess.run([str(p/'test.exe')],capture_output=True,text=True,timeout=10,creationflags=0x08000000);print(r.stdout+r.stderr,end='');r.check_returncode()
