"""Approved one-row residue locations; all other native cells stay unchanged.

Exercises the actual PPU world/HUD branches, including HD-unavailable controls.
The coordinates below come from the native sprite audit, not an HD repaint.
"""
from pathlib import Path
import re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
start=source.index('static int SmallCardNativeColor(');end=source.index('{',start)+1;depth=1
while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
helper=source[start:end]
world=source[source.index('static void DrawObjs('):source.index('/* ---- windows')]
world=world[world.index('            if (col < 0)'):world.index('            if (bandOnly) {')]
hud=source[source.index('void PortPpuRenderHud('):]
hud=hud[hud.index('                if (col < 0)'):hud.index('                if (reloadMask &&')]
targets={'Crabclaw':[9,10],'DivineRose':[8,9,10],'FairyHarp':[11,12],
         'MetalChocobo':[11],'Oathkeeper':[10,13],'Pumpkinhead':[8,9,10,11],
         'Spellbinder':[8,9,10,11],'UltimaWeapon':[10],'WishingStar':[8,9,10,11]}
controls=['KingdomKey','Olympia','ThreeWishes','DiamondDust','OneWingedAngel','SoulEater','Cure','Potion','Simba']
names=list(targets)+controls
code='#include <assert.h>\n#include <stdint.h>\n#include <stdio.h>\n'
code+='\n'.join('static const unsigned char g'+n+'SmallCardTiles[1]={0};' for n in names)
code+='\nstatic struct {const void*tiles;int x,y;}sOamHd[1];static unsigned char sMainHdPart[1];\n'+helper
code+='\nstatic int Pixel(int route,int ready,int tx,int ty,int col){int i=0,hdPart=ready;sMainHdPart[0]=ready;for(int once=0;once<1;once++){if(route){'+hud+'}else{'+world+'}}return col;}\n'
code+='static const struct {const void*tiles;unsigned expected;} cases[]={\n'
code+=',\n'.join('{g'+n+'SmallCardTiles,'+str(sum(1<<x for x in targets.get(n,[])))+'u}' for n in names)+'};\n'
code+=r'''
int main(void){unsigned checks=0;
 for(unsigned c=0;c<sizeof(cases)/sizeof(cases[0]);c++)for(int route=0;route<2;route++)for(int ready=0;ready<2;ready++)
 for(int origin=0;origin<2;origin++)for(int y=0;y<32;y++)for(int x=0;x<16;x++){
  sOamHd[0].tiles=cases[c].tiles;sOamHd[0].x=origin?0:-8;sOamHd[0].y=origin?0:-20;
  int gx=x+sOamHd[0].x+8,gy=y+sOamHd[0].y+20;
  int target=ready&&gy==15&&gx>=0&&gx<16&&(cases[c].expected&(1u<<gx));
  int olympia=ready&&cases[c].tiles==gOlympiaSmallCardTiles&&gx==14&&gy==19;
  int pumpkin=ready&&cases[c].tiles==gPumpkinheadSmallCardTiles&&gy==16&&gx>=2&&gx<=13;
  int colors[]={0x7fff,0x0421,0x7c00,0,-1};
  for(int k=0;k<5;k++){int expected=colors[k]>=0&&(target||olympia||pumpkin)?0:colors[k];
   assert(Pixel(route,ready,x,y,colors[k])==expected);checks++;
  }
 }
 printf("PASS: %u native pixel checks, 23 audited top-row cells, two PPU routes and unavailable-HD controls\n",checks);
}
'''
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='attack-top-seam-') as d:
 c=Path(d)/'test.c';exe=Path(d)/'test.exe';c.write_text(code)
 subprocess.run([str(cc),'-std=c99',str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True,timeout=15,creationflags=0x08004000)
print('PASS: 23 top-seam cells and 12 Pumpkinhead first-interior cells; other cells/cards and HD-unavailable controls unchanged')

# All ordinary values share the same small-card image; the value is rendered
# separately. Check the source catalog rather than assuming one tested value.
catalog=(ROOT/'src/card/card_catalog.c').read_text(encoding='utf-8')
rows=re.findall(r'\{\s*([^{}]+?)\{[^{}]*\},\s*\}',catalog)
values=[]
for row in rows:
    match=re.search(r'CARD_KIND_PUMPKINHEAD,\s*0x0,\s*(\d+),',row)
    if match:
        assert 'gPumpkinheadSmallCardFrame0, gPumpkinheadSmallCardTiles, gPumpkinheadSmallCardPalette' in row
        values.append(int(match[1]))
assert sorted(values)==list(range(10)),values
print('PASS: Pumpkinhead values0..9 share the corrected artwork; separate digit rendering unchanged')
