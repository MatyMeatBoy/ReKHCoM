"""Exercise original Sora/opponent-Riku reload update/draw contracts without SDL.

Extracts production functions; animation, audio and OAM submission are isolated.
Prepared independently by the reload decomp reviewer.
"""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2];SOURCE=ROOT
def fn(s,name):
 a=s.index('\nvoid '+name)+1;b=s.index('{',a);n=b+1;depth=1
 while depth: depth+=(s[n]=='{')-(s[n]=='}');n+=1
 return s[a:n]
def draw(s,name,new):
 a=s.index('static void '+name+'(CardDisplayWork* work) {');a=s.index('#endif',a)+6;b=s.index('\n}\n',a)
 return 'static void '+new+'(CardDisplayWork* work) {'+s[a:b]+'\n}\n'
a=(SOURCE/'src/card/card_battle.c').read_text(encoding='utf-8');b=(SOURCE/'src/card/card_battle_riku.c').read_text(encoding='utf-8')
c=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef int32_t s32;typedef int16_t s16;typedef int8_t s8;typedef uint8_t u8;typedef uint16_t u16;
#define TRUE 1
#define FALSE 0
#define Q_8_8(n) ((s32)((n)*256))
#define CARD_DISP_FLAG_SELECTED 1
#define CARD_DISP_FLAG_SETTLED 2
#define CARD_DISP_FLAG_GFX_LOADED 4
#define CARD_DISP_FLAG_RELOAD_DONE 8
#define GAME_FLAG_RIKU 1
#define BTL_FLAG_RELOAD_CHARGING 1
#define HC_EFFECT_OVERDRIVE 1
#define ANIM_FLAG_LOOP 1
#define ANIM_FLAG_KEEP_FRAME 2
#define SONG_SYS_CHAGE 1
#define SONG_SYS_CHAGEF1 2
#define SONG_SYS_CHAGEF2 3
#define FADE_MODE_ADD_WHITE 1
#define SPRITE_PRIORITY(n) ((n)<<10)
#define SPRITE_FLAG_NO_MOSAIC 1
#define VERSION_EU 1
static int nativeFrames[25],counterFrames[19];static void* gRiCardF0RedFrames[25];static void* gReloadCounterWhiteFrames[19];
typedef struct {int mode,type;} Anim;
typedef struct {int chargeTick;Anim anim,anim2,anim3;void *gfx,*gfx2,*gfx3;} ReloadGauge;
typedef struct {int flags,x,y,bobAngle,scaleX,priority,phase;ReloadGauge* reloadGauge;void *tiles,*tiles2,*tiles3,*tiles4;} CardDisplayWork;
static struct {int soraReloadGauge,rikuReloadGauge,soraReloadCounter,rikuReloadCounter,soraGaugeFullFrame,rikuGaugeFullFrame,soraGaugeAnim,rikuGaugeAnim,soraReloadCharging,rikuReloadCharging,reloadGaugeFull[2];void* palette;} state;
static typeof(state)*gCardBattleState=&state;static struct {int flags;}gGameState;static struct {int flags,hcEffect;}bs,br;
static typeof(bs)*gBtlWork=&bs;static typeof(br)*gRikuBtlWork=&br;
static int gSineTable[256],message,fadeCalls;static struct {void* gfx2;}gCardBacks[4];
typedef struct {int sx,sy;}ObjAffine;static ObjAffine affine[8];static int affineCount;
static void AnimStart(Anim*a,int mode,int flags){(void)flags;a->mode=mode;}
static void* AnimGetGfx(Anim*a){return gReloadCounterWhiteFrames[a->mode];}
static void* AnimUpdate(Anim*a){return gRiCardF0RedFrames[a->type==1?3:4+(a->mode-2)*3];}
static void m4aSongNumStart(int song){(void)song;}static void m4aSongNumStop(int song){(void)song;}
static int FadeGetAmount(void){return 0;}static void FadeFromAmount(int mode,int a,int b){assert(mode==1&&a==16&&b==20);fadeCalls++;}
static int IsMessageWindowOpen(void){return message;}
static ObjAffine*AllocObjAffine(int angle,int sx,int sy,int dbl){assert(!angle&&!dbl);if(sx==256&&sy==256)return NULL;assert(affineCount<8);affine[affineCount]=(ObjAffine){sx,sy};return &affine[affineCount++];}
typedef struct {int x,y,priority;void*gfx;ObjAffine*affine;}Draw;static Draw draws[8];static int drawCount;
static void DrawSprite(int x,int y,void*gfx,void*tiles,void*pal,ObjAffine*af,int flags,int pri){(void)tiles;(void)pal;(void)flags;assert(drawCount<8);draws[drawCount++]=(Draw){x,y,pri,gfx,af};}
'''
for text,pre in [(a,'Sora'),(b,'Riku')]:
 for name in ['Set'+pre+'ReloadCounterAnim','Advance'+pre+'ReloadGaugeAnim','Reset'+pre+'ReloadGaugeAnim','Set'+pre+'ReloadGaugeIdleFrames','Update'+pre+'ReloadGaugeAnims','Update'+pre+'ReloadGauge']:
  c+=fn(text,name+'(')+'\n'
c+=draw(a,'card_reload_2_Body','DrawSora')+draw(b,'Reload_Card_2_Body','DrawRiku')
c+=r'''
static ReloadGauge gauge;static CardDisplayWork work;
static void setup(int count){memset(&state,0,sizeof(state));memset(&gauge,0,sizeof(gauge));memset(&work,0,sizeof(work));bs.flags=br.flags=bs.hcEffect=br.hcEffect=0;gGameState.flags=0;fadeCalls=0;
state.soraReloadCounter=state.rikuReloadCounter=count;state.soraGaugeFullFrame=state.rikuGaugeFullFrame=4;state.soraGaugeAnim=state.rikuGaugeAnim=2;
gauge.anim2=(Anim){1,1};gauge.anim3=(Anim){2,2};gauge.gfx=gRiCardF0RedFrames[3];gauge.gfx2=gRiCardF0RedFrames[6];gauge.gfx3=gReloadCounterWhiteFrames[count];
work.flags=1|2|4;work.scaleX=256;work.priority=60;work.x=40*256;work.y=100*256;work.reloadGauge=&gauge;}
static void resetDraw(){drawCount=affineCount=0;}
static void checkDraw(int riku,int fraction,int full,int scale){setup(0);work.scaleX=scale;state.soraReloadGauge=state.rikuReloadGauge=fraction;state.reloadGaugeFull[0]=state.reloadGaugeFull[1]=full;resetDraw();if(riku)DrawRiku(&work);else DrawSora(&work);
int expected=riku&&scale<=0?0:2+(fraction>0)+(fraction>0&&full);assert(drawCount==expected);if(!expected)return;
assert(draws[0].priority==60&&draws[1].priority==58);assert(draws[0].x==40&&draws[0].y==100);
if(riku&&scale!=256){assert(draws[0].affine&&draws[0].affine->sx==scale);assert(draws[1].affine==draws[0].affine);}else assert(!draws[0].affine&&!draws[1].affine);
if(fraction>0){assert(draws[2].y==117&&draws[2].priority==59);if(scale==256&&fraction==256)assert(!draws[2].affine);else assert(draws[2].affine&&draws[2].affine->sx==scale&&draws[2].affine->sy==fraction);}
if(full&&fraction>0){assert(draws[3].y==100&&draws[3].priority==59);assert(draws[3].affine==(riku?draws[0].affine:NULL));}
work.flags=0;resetDraw();if(riku)DrawRiku(&work);else DrawSora(&work);assert(!drawCount);}
static void checkCharge(int riku,int count,int overdrive){setup(count);if(overdrive)bs.hcEffect=br.hcEffect=1;int previous=count,completed=0,first=0,oldFrame=4,firstFull=0;
for(int tick=1;tick<300;tick++){
if(riku){state.rikuReloadCharging=1;UpdateRikuReloadGauge(&work);}else{state.soraReloadCharging=1;UpdateSoraReloadGauge(&work);}
int g=riku?state.rikuReloadGauge:state.soraReloadGauge,n=riku?state.rikuReloadCounter:state.soraReloadCounter,f=riku?state.rikuGaugeFullFrame:state.soraGaugeFullFrame;
if(g==256&&!firstFull){firstFull=tick;assert(firstFull==(overdrive?45:23));}
if(f!=oldFrame&&n==previous)assert(f==oldFrame+3);oldFrame=f;
assert(g>=0&&g<=256);assert(f>=4&&f<=19&&(f-4)%3==0);if(g>0&&!first){first=tick;assert(first==3&&g==(overdrive?12:25));}
if(n!=previous){assert(n==previous-1&&g==0&&f==4);completed++;previous=n;if(n>=0)assert(gauge.gfx3==gReloadCounterWhiteFrames[n]);}
if(n<0){assert(completed==count+1&&(work.flags&8)&&!gauge.gfx3&&fadeCalls==1);return;}}
assert(!"charging failed to finish");}
static void checkLatch(int riku){setup(1);for(int i=0;i<9;i++){if(riku){state.rikuReloadCharging=1;UpdateRikuReloadGauge(&work);}else{state.soraReloadCharging=1;UpdateSoraReloadGauge(&work);}}
int before=riku?state.rikuReloadGauge:state.soraReloadGauge;assert(before>0&&before<256);
if(riku)UpdateRikuReloadGauge(&work);else UpdateSoraReloadGauge(&work);assert((riku?state.rikuReloadGauge:state.soraReloadGauge)==before&&gauge.chargeTick==0&&gauge.gfx==gRiCardF0RedFrames[3]);
resetDraw();if(riku)DrawRiku(&work);else DrawSora(&work);assert(drawCount==3&&draws[2].affine->sy==before);}
int main(){for(int i=0;i<25;i++)gRiCardF0RedFrames[i]=&nativeFrames[i];for(int i=0;i<19;i++)gReloadCounterWhiteFrames[i]=&counterFrames[i];
int values[]={0,25,128,250,256};for(int r=0;r<2;r++){for(int j=0;j<5;j++)for(int f=0;f<2;f++)checkDraw(r,values[j],f,256);for(int n=0;n<3;n++)for(int o=0;o<2;o++)checkCharge(r,n,o);checkLatch(r);}
for(int sx=0;sx<=256;sx+=64)checkDraw(1,256,1,sx);
setup(0);gSineTable[0]=256;resetDraw();DrawSora(&work);assert(draws[0].y==101);message=1;resetDraw();DrawSora(&work);assert(draws[0].y==100);gSineTable[0]=message=0;
setup(0);SetSoraReloadCounterAnim(&gauge,18);assert(gauge.gfx3==gReloadCounterWhiteFrames[18]);SetSoraReloadCounterAnim(&gauge,19);assert(!gauge.gfx3);SetSoraReloadCounterAnim(&gauge,-1);assert(!gauge.gfx3);
setup(0);gGameState.flags=1;resetDraw();DrawSora(&work);assert(drawCount==1);
puts("PASS real decomp Sora/Riku: draw0/25/128/250/256+NULLfull, cap, counter1/2/3, overdrive, partial latch, RikuScale0..256, GFX lifetime");}
'''
out=ROOT/'build/reviewer-reload-decomp';out.mkdir(parents=True,exist_ok=True);src=out/'probe.c';src.write_text(c)
cc=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe';exe=out/'probe.exe'
subprocess.run([str(cc),'-std=gnu11','-O1',str(src),'-o',str(exe)],check=True)
r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout+r.stderr,end='');r.check_returncode()
