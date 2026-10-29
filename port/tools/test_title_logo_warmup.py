"""The actual startup warmer must prepare only the title's six imported logos."""
from pathlib import Path
import argparse, json, subprocess, tempfile
from test_mapcard_definition_bounds import function

R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);p.add_argument('--baseline-ref');a=p.parse_args()
source=subprocess.check_output(['git','show',a.baseline_ref+':port/src/port_hdsprites.c'],cwd=R,text=True,encoding='utf8') if a.baseline_ref else (R/'port/src/port_hdsprites.c').read_text(encoding='utf8')
entries=[]
for line in (R/'port/data/hd_sprites.tsv').read_text(encoding='utf8').splitlines():
 c=line.split('\t')
 if len(c)>2 and c[0].startswith('gTitleLogo'):
  assert c[1]=='2';entries.append(c[0])
assert len(entries)==6 and len(set(entries))==6
entries += ['gTitleMenuSelectedBar','gKingdomKeyCardTiles','gTitlePressStartEnglish']
defs=','.join('{'+json.dumps(name)+','+str(2 if i<7 else 1 if i==7 else 7)+'}' for i,name in enumerate(entries))
unit=r'''#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef void SDL_Renderer;
#define SDL_strncmp strncmp
static struct {int hdTitle;} gPortConfig;
static SDL_Renderer *sRenderer;
static struct {const char* name;int group;} kHdSprites[]={'''+defs+r'''};
#define HD_SPRITE_COUNT (sizeof(kHdSprites)/sizeof(kHdSprites[0]))
static int calls[16],ready[16],reject=-1,poster,light;
static int SpriteReady(int i){calls[i]++;if(i==reject)return 0;ready[i]=1;return 1;}
static int PortHdTitleArtReady(void*p){(void)p;poster++;return 1;}
static int MenuLightReady(SDL_Renderer*p){(void)p;light++;return 1;}
'''+function(source,'PortHdMenuAssetsWarm')+r'''
int main(void){
 SDL_Renderer*r=(void*)1;
 PortHdMenuAssetsWarm(r);assert(!poster&&!light&&!sRenderer);
 gPortConfig.hdTitle=1;PortHdMenuAssetsWarm(0);assert(!poster&&!light);
 reject=2;PortHdMenuAssetsWarm(r);assert(poster==1&&light==1&&sRenderer==r);
 for(int i=0;i<6;i++)assert(calls[i]==1&&ready[i]==(i!=reject));
 for(int i=6;i<9;i++)assert(!calls[i]&&!ready[i]);
 /* Transient readiness rejection must not abort warming the remaining logos. */
 reject=-1;PortHdMenuAssetsWarm(r);for(int i=0;i<6;i++)assert(ready[i]);
 puts("PASS actual startup warmer: six title logos, HD-off/null guards, failure isolation, no cards/menu words");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='title-logo-warm-') as folder:
 c=Path(folder)/'test.c';exe=Path(folder)/'test.exe';c.write_text(unit,encoding='utf8')
 subprocess.run([str(a.cc.resolve()),'-std=c11','-O2',str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
