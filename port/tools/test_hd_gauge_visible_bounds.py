"""Optional private-LAD regression: real C geometry must fit the gauge scratch crop.

Pass --lad once or more for existing imported layouts. No game/GPU is run and
no asset bytes are written or distributed. All frame values used by the player
gauge and all four face states are measured after the production mask clipping.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--lad',type=Path,action='append',default=[])
args=ap.parse_args()
if not args.lad:
    print('SKIP private-layout measurement: pass --lad PATH; synthetic budgets/cache are tested by test_hd_gauge_targets.py')
    raise SystemExit(0)
s=(ROOT/'port/src/port_hdlayout.c').read_text(encoding='utf8')
crop=re.search(r'const float ux0 = ([\d.+-]+)f, uy0 = ([\d.+-]+)f, ux1 = ([\d.+-]+)f, uy1 = ([\d.+-]+)f;',s)
assert crop, 'Production gauge crop declaration not found'
limits=','.join(crop.groups())
typedefs=s[s.index('typedef struct HdLayout {'):s.index('static HdLayout sLayouts')]
readers=s[s.index('static int32_t I32('):s.index('static int ValidateLayout')]
parser=s[s.index('/* The layout\'s sequence n:'):s.index('\nint PortHdLayoutGroupFrames(')]
groups=s[s.index('int PortHdLayoutGroupFrames('):s.index('\ntypedef struct Ctx')]
draw=s[s.index('typedef struct Ctx'):s.index('\n#if __STDC_HOSTED__\nstatic void GaugeTargetsRelease')]
pre=r'''
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
typedef int SDL_Renderer;
typedef int SDL_Texture;
typedef struct {float x,y;} SDL_FPoint;
typedef struct {float r,g,b,a;} SDL_FColor;
typedef struct {SDL_FPoint position; SDL_FColor color; SDL_FPoint tex_coord;} SDL_Vertex;
typedef struct {float x,y,w,h;} SDL_FRect;
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_PI_F 3.14159265358979323846f
#define SDL_cosf cosf
#define SDL_sinf sinf
#define SDL_BLENDMODE_ADD 1
#define SDL_BLENDMODE_BLEND 2
#define HD_LAYOUT_HOLD(x) ((x)|0x100000)
static double xmin,ymin,xmax,ymax;
static int rendered;
static void reset(void){xmin=ymin=1e30;xmax=ymax=-1e30;rendered=0;}
static int SDL_SetTextureBlendMode(SDL_Texture*t,int mode){(void)t;(void)mode;return 1;}
static int SDL_RenderGeometry(SDL_Renderer*r,SDL_Texture*t,const SDL_Vertex*v,int n,const int*idx,int ni){
 (void)r;(void)t;(void)idx;(void)ni;
 int visible=0;for(int i=0;i<n;i++)visible|=v[i].color.a>0;
 if(!visible)return 1;
 for(int i=0;i<n;i++){float x=v[i].position.x,y=v[i].position.y;xmin=SDL_min(xmin,x);ymin=SDL_min(ymin,y);xmax=SDL_max(xmax,x);ymax=SDL_max(ymax,y);}
 rendered++;return 1;
}
'''
stub='''
static HdLayout current;
static HdLayoutTexture texture={0,4096,4096,1,1};
static const HdLayout* Layout(int n){(void)n;return &current;}
static HdLayoutTexture* Texture(SDL_Renderer*r,int n){(void)r;(void)n;return &texture;}
'''
main=r'''
static const double limits[]={CROP_LIMITS};
static void bounds(int seq,int group,int end){
 reset();for(int f=0;f<=end;f++)PortHdLayoutDraw(NULL,0,0,seq,group,HD_LAYOUT_HOLD(f),0,0,1,1,NULL);
 printf("seq%d group%d frames0..%d primitives%d bounds %.4f %.4f %.4f %.4f\n",seq,group,end,rendered,xmin,ymin,xmax,ymax);
 fflush(stdout);
 assert(rendered>0);
 assert(xmin>=limits[0] && ymin>=limits[1] && xmax<=limits[2] && ymax<=limits[3]);
}
int main(int argc,char**argv){
 if(argc!=2)return 1;FILE*f=fopen(argv[1],"rb");if(!f)return 2;
 fseek(f,0,SEEK_END);current.size=ftell(f);rewind(f);current.data=malloc(current.size);fread(current.data,1,current.size,f);fclose(f);
 if(!ValidateLayout(&current))return 3;current.validated=1;
 const int gs[]={7,8,9,10,13,14,15};
 for(int i=0;i<7;i++)bounds(1,gs[i],PortHdLayoutGroupFrames(0,1,gs[i]));
 for(int i=0;i<4;i++)bounds(0,i,PortHdLayoutGroupFrames(0,0,i));
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-gauge-bounds-') as directory:
 out=Path(directory)
 c=out/'measure.c';exe=out/'measure.exe';c.write_text(pre+typedefs+readers+parser+stub+groups+draw+main.replace('CROP_LIMITS',limits),encoding='utf8')
 cc=os.environ.get('CC') or shutil.which('clang')
 if not cc:
  cc=str(next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe')))
 subprocess.run([cc,'-std=c99',str(c),'-o',str(exe)],check=True)
 for lad in args.lad:
  print(lad.name,flush=True)
  subprocess.run([str(exe),str(lad.resolve())],check=True)
