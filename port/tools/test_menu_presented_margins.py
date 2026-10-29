"""Actual margin consumers must survive next-frame palette/asset changes.

The controlled decoder stands in for VRAM, palettes and released sprite slots;
the production capture/consumers own timing and coordinate selection.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
from test_mapcard_definition_bounds import function

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--cc', type=Path, required=True)
p.add_argument('--baseline-ref')
a = p.parse_args()
def read(name):
    if a.baseline_ref:
        return subprocess.check_output(['git', '-C', str(ROOT), 'show', a.baseline_ref + ':' + name], text=True, encoding='utf8')
    return (ROOT / name).read_text(encoding='utf8')

ppu = read('port/src/port_ppu.c')
snapshot = '#include "port/menu_margin_snapshot.inc"' in ppu
prefix = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define GBA_SCREEN_H 160
static const uint8_t sObjSize[3][4][2]={
 {{8,8},{16,16},{32,32},{64,64}},
 {{16,8},{32,8},{32,16},{64,32}},
 {{8,16},{8,32},{16,32},{32,64}}};
typedef struct ObjTiles {int unused;} ObjTiles;
typedef struct ObjPalette {int unused;} ObjPalette;
typedef struct MenuBandSource {const uint16_t* frame; const ObjTiles* tiles; const ObjPalette* palette; int y;} MenuBandSource;
static uint16_t frame[]={1,0,0,0};
static struct {int headerDrawn,headerX;} sMenuShown={1,-8};
static const uint16_t* sMenuHeaderFrame=frame;
static ObjTiles tiles;static ObjPalette palette;
static const ObjTiles* sMenuHeaderTiles=&tiles;
static const ObjPalette* sMenuHeaderPalette=&palette;
static MenuBandSource sMenuBands[2]={{frame,&tiles,&palette,16},{0}};
static const void* sMenuTabFrame;
static uint32_t sMenuTabPixels[2][16];
static int live=1,shown=1;
static int HeaderOnScreen(void){return shown;}
static uint32_t HeaderPixelRaw(int y,int phase){return live&&y>=0&&y<16?0xff00aa00u+(phase&1):0;}
static uint32_t MenuSpritePixel(const uint16_t*f,const ObjTiles*t,const ObjPalette*p,int y,int phase,int x){
 (void)t;(void)p;
 return live&&f&&y>=0&&y<16&&x+(phase&1)>=0&&x+(phase&1)<8?0xff120000u+(unsigned)(x+(phase&1)):0;
}
static uint32_t MenuTabPixelRaw(int y,int phase,int side){(void)phase;if(side<0&&!sMenuTabFrame)return 0;return live&&y>=16&&y<32?0xff660000u+(side>0):0;}
uint32_t PortMenuHeaderPixel(int,int);
'''
prefix += function(ppu.replace('static int HeaderLeft', 'int HeaderLeft'), 'HeaderLeft') + '\n'
if snapshot:
    prefix += function(ppu.replace('static int HeaderWidth', 'int HeaderWidth'), 'HeaderWidth') + '\n'
if snapshot:
    bodies = read('port/include/port/menu_margin_snapshot.inc')
else:
    bodies = '\n'.join(function(ppu, name) for name in
        ['PortMenuHeaderPixelAt', 'PortMenuHeaderPixel', 'PortMenuBandPixel', 'PortMenuTabPixel'])
    bodies += '\nstatic void CaptureMenuMarginPixels(void){for(int i=0;i<2;i++)for(int y=0;y<16;y++)sMenuTabPixels[i][y]=MenuTabPixelRaw(y+16,0,i?1:-1);}\n'
unit = prefix + bodies + r'''
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 uint32_t header,band,tab;
 sMenuTabFrame=frame;CaptureMenuMarginPixels();
 header=PortMenuHeaderPixelAt(4,2);band=PortMenuBandPixel(18,0);tab=PortMenuTabPixel(18,0,-1);
 CHECK(header==0xff120005u);CHECK(band==0xff120000u);CHECK(tab==0xff660000u);
 /* Presentation happens after the next frame overwrites palette/VRAM and
    releases/replaces the old mode's descriptors. The shown picture remains. */
 live=0;sMenuHeaderFrame=0;sMenuShown.headerX=200;sMenuShown.headerDrawn=0;sMenuBands[0].frame=0;sMenuTabFrame=0;
 CHECK(PortMenuHeaderPixelAt(4,2)==header);
 CHECK(PortMenuBandPixel(18,0)==band);CHECK(PortMenuTabPixel(18,0,-1)==tab);
 CHECK(PortMenuHeaderPixel(4,1)==0xff00aa01u);
 CHECK(PortMenuHeaderPixelAt(4,1000000)==0xff00aa00u);
 CHECK(PortMenuHeaderPixelAt(-1,2)==0);CHECK(PortMenuHeaderPixelAt(16,2)==0);
 CHECK(PortMenuBandPixel(-1,0)==0);CHECK(PortMenuBandPixel(160,0)==0);
 CHECK(PortMenuTabPixel(15,0,-1)==0);CHECK(PortMenuTabPixel(32,0,-1)==0);
 /* The next rendered frame replaces the snapshot, including transparent
    controls. A skipped/non-rendered frame makes no capture call. */
 CaptureMenuMarginPixels();CHECK(PortMenuHeaderPixelAt(4,2)==0);CHECK(PortMenuBandPixel(18,0)==0);CHECK(PortMenuTabPixel(18,0,-1)==0);
 live=1;sMenuHeaderFrame=frame;sMenuShown.headerDrawn=0;shown=0;CaptureMenuMarginPixels();CHECK(PortMenuHeaderPixelAt(4,100)==0);
'''
if snapshot:
    unit += r'''
 {uint16_t extreme[]={2,0,256|(3<<14),0,0,255|(3<<14),0};
  sMenuHeaderFrame=extreme;sMenuShown.headerDrawn=1;
  CHECK(HeaderWidth()==575);CaptureMenuMarginPixels();
  CHECK(sMenuPixels.width==575);CHECK(sizeof(sMenuPixels)<40000);
 }
 {uint16_t invalid[]={129};sMenuHeaderFrame=invalid;CHECK(HeaderWidth()==0);}
 for(int shape=0;shape<3;shape++)for(int size=0;size<4;size++)for(int x=0;x<512;x++){
  uint16_t parts[]={2,(uint16_t)(shape<<14),(uint16_t)(x|(size<<14)),0,0,256,0};
  sMenuHeaderFrame=parts;CHECK(HeaderWidth()>=0&&HeaderWidth()<576);
 }
'''
unit += r'''
 puts("PASS presented menu header/bands/tabs survive next-frame mutation; sliding/bounds/clear/hidden controls");
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-menu-frame-') as d:
    d = Path(d); c = d / 'test.c'; exe = d / 'test.exe'
    c.write_text(unit, encoding='utf8')
    subprocess.run([str(a.cc.resolve()), '-std=c11', '-O2', '-Wall', '-Wno-unused-function', '-Wno-unused-variable', str(c), '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], check=False)
    raise SystemExit(result.returncode)
