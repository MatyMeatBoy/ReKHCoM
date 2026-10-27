"""Exercise the real enemy gauge update/draw with recorded layout frame controls.

No game, ROM, imported textures, GPU or save writes are required. Time must not
drive the layout's length or layer-count groups; HP still drives green/red fill.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT/'port/src/port_hdlayout.c').read_text(encoding='utf8')
    enemy = source[source.index('#define ENEMY_LAYER'):]
    compiler = os.environ.get('CC') or shutil.which('clang')
    if not compiler:
        compiler = str(next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe')))
    with tempfile.TemporaryDirectory(prefix='rekhcom-enemy-gauge-') as folder:
        src, exe = Path(folder)/'enemy.c', Path(folder)/'enemy.exe'
        src.write_text(PREAMBLE + enemy + HARNESS, encoding='utf8')
        subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], timeout=10, check=True)
    print('PASS: constant enemy geometry 1000 frames; HP layers, trail, healing, target and visibility resets')


PREAMBLE = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef int SDL_Renderer;
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_clamp(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define GBA_SCREEN_W 240
#define HD_LAYOUT_GAUGE 0
#define HD_LAYOUT_TEX_GAUGE 1
#define HD_LAYOUT_HOLD(frame) ((frame)|0x100000)
static int gPortHudW=320, now, calls, seen[7], frames[7], ready=1, missing=-1;
static struct {int hdCards;} gPortConfig={1};
static unsigned PortFrameCount(void){return (unsigned)now;}
static int PortHdLayoutGroupFrames(int l,int s,int g){(void)l;(void)s;return g==missing?0:200;}
static int PortHdLayoutReady(int l,int t){(void)l;(void)t;return ready;}
static int PortHdDrawLogged(void){return 0;}
#define PortLog(...) ((void)0)
static int PortHdLayoutDraw(SDL_Renderer*r,int l,int t,int s,int g,int f,float x,float y,float z,float a,const float*c){
    (void)r;(void)l;(void)t;(void)s;(void)x;(void)y;(void)z;(void)a;(void)c;
    assert(g>=0&&g<7);calls++;seen[g]++;frames[g]=f;return 1;
}
'''

HARNESS = r'''
static int actorA, actorB;
static void update(int visible,int hp,int max,const void*who){
    now++;calls=0;memset(seen,0,sizeof(seen));memset(frames,0,sizeof(frames));
    assert(PortHdEnemyGauge(visible,hp,max,who));
    PortHdEnemyGaugeDraw(NULL,0,0,4,1);
}
static void controls(int green,int red,int layers){
    /* Imported gauge.lad: group0 length/tip, 1 red, 2 constant overlay,
       3 green, 4/5/6 cumulative counter pieces. None is a clock animation. */
    assert(calls==(layers?7:4));
    for(int i=0;i<4;i++)assert(seen[i]==1);
    assert(frames[0]==HD_LAYOUT_HOLD(200));
    assert(frames[2]==HD_LAYOUT_HOLD(0));
    assert(frames[1]==HD_LAYOUT_HOLD(red));
    assert(frames[3]==HD_LAYOUT_HOLD(green));
    for(int i=4;i<7;i++){
        assert(seen[i]==(layers?1:0));
        if(layers)assert(frames[i]==HD_LAYOUT_HOLD(layers));
    }
}
int main(void){
    for(int i=0;i<1000;i++){update(1,80,160,&actorA);controls(100,100,0);}
    for(int i=0;i<1000;i++){update(1,840,1680,&actorB);controls(100,100,1);}
    /* All layer boundaries, including zero and short maximums, retain the
       existing normalized HP scale. Switching actor resets any old trail. */
    const int hp[]={0,1,80,559,560,561,1120,1121,1680,1681,2240};
    for(unsigned i=0;i<sizeof(hp)/sizeof(hp[0]);i++){
        int layer=hp[i]>0?(hp[i]-1)/560:0;
        int green=(hp[i]-layer*560)*200/560;
        update(1,hp[i],2240,i%2?&actorA:&actorB);controls(green,green,layer);
    }
    update(1,40,80,&actorA);controls(100,100,0);
    update(1,1,0,&actorB);controls(0,0,0);
    update(1,160,160,&actorA);controls(200,200,0);
    update(1,80,160,&actorA);controls(100,200,0);
    for(int i=0;i<87;i++){update(1,80,160,&actorA);controls(100,200,0);}
    for(int i=0;i<100;i++){update(1,80,160,&actorA);assert((frames[1]&~HD_LAYOUT_HOLD(0))>=100);}
    controls(100,100,0);
    update(1,120,160,&actorA);controls(150,150,0);
    update(1,80,160,&actorA);controls(100,150,0);
    update(1,160,160,&actorA);controls(200,200,0);
    update(1,160,320,&actorA);controls(100,100,0);
    update(1,40,160,&actorB);controls(50,50,0);
    update(0,40,160,&actorB);assert(!calls);
    update(1,20,160,&actorB);controls(25,25,0);
    update(1,20,160,NULL);assert(!calls);
    now+=10;update(1,80,160,&actorA);controls(100,100,0);
    update(1,561,1120,&actorB);controls(0,0,1);
    update(1,560,1120,&actorB);controls(200,200,0);
    gPortConfig.hdCards=0;assert(!PortHdEnemyGauge(1,80,160,&actorA));
    gPortConfig.hdCards=1;ready=0;assert(!PortHdEnemyGauge(1,80,160,&actorA));
    ready=1;missing=0;assert(!PortHdEnemyGauge(1,80,160,&actorA));
    return 0;
}
'''

if __name__ == '__main__':
    main()
