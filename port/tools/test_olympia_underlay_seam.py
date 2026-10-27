"""Exercise actual world/HUD native-colour branches, with HD accepted or rejected."""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
def block(marker):
    start=source.index(marker);i=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[i]=='{')-(source[i]=='}');i+=1
    return source[start:i]

world=source[source.index('static void DrawObjs('):source.index('/* ---- windows')]
world=world[world.index('            if (col < 0)'):world.index('            if (bandOnly) {')]
hud=source[source.index('void PortPpuRenderHud('):]
hud=hud[hud.index('                if (col < 0)'):hud.index('                if (reloadMask &&')]
helper=block('static int SmallCardNativeColor(') if 'static int SmallCardNativeColor(' in source else ''
code=r'''
#include <assert.h>
#include <stdio.h>
static const unsigned char gOlympiaSmallCardTiles[1]={0},otherTiles[1]={0};
static struct {const void*tiles;int x,y;} sOamHd[1];
static unsigned char sMainHdPart[1];
'''+helper+r'''
static int Pixel(int hud,int ready,int x,int y,int original) {
  int col=original,i=0,tx=x,ty=y,hdPart=ready;
  sMainHdPart[0]=ready;
  for(int once=0;once<1;once++){
    if(hud) {
'''+hud+r'''
    }else{
'''+world+r'''
    }
  }
  return col;
}
int main(void) {
  int cases=0;
  for(int route=0;route<2;route++)for(int ready=0;ready<2;ready++)for(int named=0;named<2;named++)
  for(int origin=0;origin<2;origin++)for(int y=0;y<32;y++)for(int x=0;x<16;x++){
    sOamHd[0].tiles=named?gOlympiaSmallCardTiles:otherTiles;
    sOamHd[0].x=origin?0:-8;sOamHd[0].y=origin?0:-20;
    int target=ready&&named&&x+sOamHd[0].x==6&&y+sOamHd[0].y==-1;
    int colors[]={0x7fff,0x1ce7,0,0x7c00,-1};
    for(int c=0;c<5;c++) {
      assert(Pixel(route,ready,x,y,colors[c])==(target&&colors[c]>=0?0:colors[c]));
      cases++;
    }
  }
  printf("PASS: %d native pixels, exact Olympia cell only, both world/HUD and HD fallback\n",cases);
}
'''
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='olympia-seam-') as d:
    c=Path(d)/'test.c';exe=Path(d)/'test.exe';c.write_text(code)
    subprocess.run([str(cc),'-std=c99',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,timeout=10,creationflags=0x08004000)
print('PASS: 40960 actual world/HUD branch pixel checks, queued/fallback, palette fades and transparency')

# Every ordinary value uses this same small-card artwork; value digits are
# separate sprites and are outside this native underlay correction.
catalog=(ROOT/'src/card/card_catalog.c').read_text(encoding='utf-8')
import re
rows=re.findall(r'\{\s*(gCardWep03Frame0[^{}]+?)\{[^{}]*\},\s*\}',catalog)
values=[]
for row in rows:
    m=re.search(r'CARD_KIND_OLYMPIA,\s*0x0,\s*(\d+),',row)
    if m:
        assert 'gOlympiaSmallCardFrame0, gOlympiaSmallCardTiles, gOlympiaSmallCardPalette' in row
        values.append(int(m[1]))
assert sorted(values)==list(range(10)),values
print('PASS: values0..9 share Olympia artwork; separate value rendering unchanged')
