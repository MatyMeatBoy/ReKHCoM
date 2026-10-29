"""Exercise the real lookup: a shared background must not win over a title."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from test_mapcard_definition_bounds import function

root = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--cc', type=Path, required=True)
p.add_argument('--baseline-ref')
a = p.parse_args()
source = subprocess.check_output(['git', '-C', str(root), 'show', a.baseline_ref + ':port/src/port_hdsprites.c'], text=True, encoding='utf8') if a.baseline_ref else (root / 'port/src/port_hdsprites.c').read_text(encoding='utf8')
unit = r'''
#include <assert.h>
#include <stdio.h>
typedef struct HdSpriteDef {const void*tiles;const void*frame;int group;} HdSpriteDef;
static struct {int hdCards,hdTitle;} gPortConfig={1,0};
static int tiles,frame,other;
static const HdSpriteDef kHdSprites[]={{&tiles,0,5},{&tiles,&frame,5},{&other,&frame,5}};
#define HD_SPRITE_COUNT 3
static int ready[]={1,1,1};
static int SpriteReady(int i){return ready[i];}
''' + function(source.replace('static int GroupOn','int GroupOn'), 'GroupOn') + '\n' + function(source, 'PortHdSpriteFind') + r'''
int main(void){
 assert(PortHdSpriteFind(&tiles,&frame)==2);
 assert(PortHdSpriteFind(&tiles,0)==1);
 assert(PortHdSpriteFind(&tiles,&other)==1);
 ready[1]=0;assert(PortHdSpriteFind(&tiles,&frame)==0);
 ready[1]=1;ready[0]=0;assert(PortHdSpriteFind(&tiles,&frame)==2);assert(PortHdSpriteFind(&tiles,0)==0);
 gPortConfig.hdCards=0;assert(!PortHdSpriteFind(&tiles,&frame));assert(!PortHdSpriteFind(0,&frame));
 gPortConfig.hdTitle=1;assert(!PortHdSpriteFind(&tiles,&frame));
 puts("PASS exact frame before shared tiles; unavailable title stays native, not the book background");
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-sprite-key-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_text(unit,encoding='utf8')
    subprocess.run([str(a.cc.resolve()),'-std=c11','-O2',str(c),'-o',str(exe)],check=True)
    raise SystemExit(subprocess.run([str(exe)]).returncode)
