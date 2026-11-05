"""Compiled regression for the scene061 decorative object extension."""
import os, shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
class Scene061PropsTest(unittest.TestCase):
 def test_original_slots_excluded_and_authored_spacing(self):
  compiler=os.environ.get('CC') or shutil.which('gcc')
  local=ROOT.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
  if not compiler and local.is_file():compiler=str(local)
  if not compiler:self.skipTest('C compiler unavailable')
  source=(ROOT/'src/card/card_lvup_logo.c').read_text(encoding='utf-8')
  start=source.index('void Ev_mapObj_2(');end=source.index('\nvoid Ev_mapObj_3',start)
  code=r'''#include <assert.h>
#include <stddef.h>
#define PLATFORM_PC
#define SPRITE_PRIORITY(n) ((n)<<10)
#include "port_scene061_props.h"
typedef unsigned char u8;
typedef struct {int x,y,spriteIndex;} EventMapObjectPlacement;
typedef struct {EventMapObjectPlacement* placements;int placementCount;void** sprites;} EventMapObjectDef;
typedef struct {EventMapObjectDef* definition;int background;void*tiles[1];void*palettes[1];} EventMapObjectWork;
typedef struct {int dummy;} EventBackgroundDef;
static EventBackgroundDef gEventBg061,other,*gEventBackgroundDefs[1];
static struct {int x,y;} state,*gEventState=&state;
static int margin,clip0,clip1,native,extra;
static int PortWideMarginX(void){return margin;}
static int PortWideMarginY(void){return 100;}
static void PortSpriteClipX(int a,int b){clip0=a;clip1=b;}
static void DrawSprite(int x,int y,void*s,void*t,void*p,void*a,int flags,int priority){
 if(clip0!=clip1){assert(clip1<=0||clip0>=240);extra++;}
 else{assert(native<4);assert(x==16+64*native);assert(y==160+32*native-128);native++;}
}
'''+source[start:end]+r'''
int main(void){int k,x,y;EventMapObjectPlacement entries[4];void*sprites[1]={0};EventMapObjectDef def={entries,4,sprites};EventMapObjectWork work={&def,0,{0},{0}};
for(k=-64;k<=64;k++){int ok=PortScene061PropPosition(k,&x,&y);if(k>=0&&k<4)assert(!ok);else{assert(ok);assert(x==16+64*k);assert(y==160+32*k);}}
for(k=0;k<4;k++){entries[k].x=16+64*k;entries[k].y=160+32*k;entries[k].spriteIndex=0;}
state.y=128<<8;gEventBackgroundDefs[0]=&gEventBg061;
margin=0;Ev_mapObj_2(&work);assert(native==4&&extra==0);
native=extra=0;margin=200;Ev_mapObj_2(&work);assert(native==4&&extra>0);assert(clip0==0&&clip1==0);
native=extra=0;gEventBackgroundDefs[0]=&other;Ev_mapObj_2(&work);assert(native==4&&extra==0);
return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'check.c';src.write_text(code)
   exe=Path(d)/('check.exe' if os.name=='nt' else 'check')
   subprocess.run([compiler,'-I',str(ROOT/'port/include/port'),str(src),'-o',str(exe)],check=True,capture_output=True)
   subprocess.run([str(exe)],check=True,capture_output=True)
if __name__=='__main__':unittest.main()
