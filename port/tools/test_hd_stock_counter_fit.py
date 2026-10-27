"""Real HUD coordinate callsite and active-gauge policy; no game/GPU."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def fn(s,name):
 a=s.rfind("\n",0,s.index(name+"("))+1;b=s.index("{",a);e=b+1;d=1
 while d:d+=(s[e]=="{")-(s[e]=="}");e+=1
 return s[a:e]
ppu=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf8');layout=(ROOT/'port/src/port_hdlayout.c').read_text(encoding='utf8')
core=fn(layout,'PortHdStockValueTop') if 'int PortHdStockValueTop(' in layout else 'int PortHdStockValueTop(int opponent){(void)opponent;return 0;}\n'
if 'static int HudStockValueY(' in ppu:core+='\n'+fn(ppu,'HudStockValueY')
a=ppu.index('        hy = TagHudY(tag, yg, h, HudRaise());');b=ppu.index('        if (!affine',a)
core+='\nstatic int Position(const void*src,int opponent,int yg){int i=0,h=160,hy;Tag t={opponent?1:-1};const Tag*tag=&t;sOamHd[i].tiles=src;'+ppu[a:b]+'return hy;}\n'
pre=r'''
#include <stdint.h>
#include <stdio.h>
static unsigned frame=10;
static struct{int hdCards;}gPortConfig={1};
static struct{int active,frame;}sGauge={1,10},sEnemy={1,10};
static const unsigned char gStockValueTiles[27*128]={0};
static struct{const void*tiles;}sOamHd[1];
typedef struct{int ax;}Tag;
static int TagHudY(const Tag*t,int y,int h,int r){(void)t;(void)h;(void)r;return y;}
static int HudRaise(void){return 8;}
static unsigned PortFrameCount(void){return frame;}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 77;}}while(0)
'''
test=r'''
int main(void){
 for(int side=0;side<2;side++)for(int count=1;count<=27;count++){
  const void*source=gStockValueTiles+(count-1)*128;
  CHECK(Position(source,side,4)==9); /* bbox y13..24, white14..20: below HD bar<=12 */
  CHECK(Position(source,side,20)==20);
  gPortConfig.hdCards=0;CHECK(Position(source,side,4)==4);gPortConfig.hdCards=1;
 }
 CHECK(Position(NULL,0,4)==4&&Position(gStockValueTiles+1,0,4)==4&&Position(gStockValueTiles+sizeof(gStockValueTiles),0,4)==4);
 sGauge.active=0;CHECK(Position(gStockValueTiles,0,4)==4);sGauge.active=1;
 sEnemy.active=0;CHECK(Position(gStockValueTiles,1,4)==4);sEnemy.active=1;
 frame++;CHECK(Position(gStockValueTiles,0,4)==4&&Position(gStockValueTiles,1,4)==4);
 puts("PASS actual stock counter coordinate hook: both sides, sums1..27, HD active only, no fallback/card changes");return 0;
}
'''
base=next(p for p in ROOT.parents if (p/'toolchain').is_dir());cc=next((base/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='stock-counter-fit-') as d:
 p=Path(d);(p/'test.c').write_text(pre+core+test,encoding='utf8')
 subprocess.run([str(cc),'-std=gnu11',str(p/'test.c'),'-o',str(p/'test.exe')],check=True)
 r=subprocess.run([str(p/'test.exe')],capture_output=True,text=True,timeout=10,creationflags=0x08000000);print(r.stdout+r.stderr,end='');r.check_returncode()
