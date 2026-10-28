"""CPU contract for the actual Parasite Cage draw path and its wide body map."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/bos/bos_pc.c').read_text(encoding='utf-8')


def function(name):
    start = source.index(name)
    brace = source.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


extension = ''
if 'static int sPortBodyGfxSet' in source:
    start = source.index('static int sPortBodyGfxSet')
    extension = source[start:source.index('\n#endif', start)]
draw = function('void BosPcDraw(')
destroy = function('void task_bos_pc_3(')
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
typedef int16_t s16;typedef int32_t s32;typedef uint16_t u16;typedef uint8_t u8;
#define PLATFORM_PC 1
#define TRUE 1
#define OAM_Y_MASK 255
#define OAM_X_MASK 511
#define PC_ANIM_OP_FRAME 0
#define PC_SPRITE_CMD_END 255
#define PC_SPRITE_CMD_STANDALONE 1
#define PLTT ((void*)0)
typedef struct {int op,cmdList,gfxSet;} PcAnimStep;
typedef struct {int flags,x,y,gfxIndex,layer;} PcSpriteCmd;
typedef struct {u16 attr0,attr1,attr2;} PcSpriteDef;
typedef struct {int count;u16 attr[512];} PcOam;
typedef struct {const void *tiles;int tilesSize;const void *map;int mapSize;} PcGfxSet;
typedef struct {const PcAnimStep *animSteps;int animFrame,bgFrame,x,y,z,defeated,flash,prevFlash,body,body2,collider;struct{int inEvent;}shared;void *palette,*palette2,*tiles,*tiles2[96];PcOam oam[24];} PcWork;
static PcSpriteCmd endcmd={255,0,0,0,0};static const PcSpriteCmd *sBosPcSpriteCmdLists[]={&endcmd};
static PcSpriteDef *gBosPcLayerFrames[1];static void *gBosPcStandaloneFrames[1];
static int gBosPcBgPalettes[1][1],gHitFlashPalette[1];static PcAnimStep gBosPcTackleAnim[1];
static struct {int shakeX,shakeY;} evt,*gEventState=&evt;static struct {int bossPriorityOffset;} btl,*gBtlWork=&btl;
static u16 map0[1024],map1[1024];static const PcGfxSet sPcGfxSets[]={{NULL,0,map0,sizeof map0},{NULL,0,map1,sizeof map1}};
static int (*world)(int,int,int);static int worldX,worldY;
static void PortSetBgWorld(int bg,int(*fn)(int,int,int),int x,int y){assert(bg==1);world=fn;worldX=x;worldY=y;}
static int StepHitFlash(int *p){(void)p;return 0;}
static void LoadPalette(const void*a,void*b,int n){(void)a;(void)b;(void)n;}
static void WorldToScreen(s16*x,s16*y,int a,int b,int c){*x=a/256;*y=b/256-c/256;}
static int PcLayerDepth(int i){return i*1024-0x3300;}
static int BosPcGetSpritePriority(PcWork*w,int y){(void)w;return y;}
static int BosPcGetSpriteDepth(PcWork*w,int y,int z){(void)w;return y+z;}
static void DrawSprite(int a,int b,const void*c,void*d,void*e,void*f,int g,int h){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;}
static u16 *PcOamGfx(PcOam*p){return p->attr;}
static void LoadBgTiles(int bg,const void*p,int n){(void)bg;(void)p;(void)n;}
static void LoadBgMap(int bg,const void*p,int n){(void)bg;(void)p;(void)n;}
static void SetBgScroll(int bg,int x,int y){(void)bg;(void)x;(void)y;}
static void ReleaseEnemyBtlObj(int*p){(void)p;}
static void ColliderUnregister(int*p){(void)p;}
static void ReleaseObjTiles(void*p){(void)p;}
static void ReleaseObjPalette(void*p){(void)p;}
static void BosPcStopPaletteCycle(PcWork*p){(void)p;}
EXTENSION
DRAW
DESTROY
int main(void){
 PcAnimStep step={0,0,0};PcWork work={0};work.animSteps=&step;work.bgFrame=-1;
 for(int i=0;i<1024;i++){map0[i]=(u16)(i+1);map1[i]=(u16)(i+1001);}map0[0]=map1[0]=0;
 for(int x=-160;x<=480;x+=16)for(int y=-120;y<=240;y+=16){
  work.x=x*256;work.y=y*256;work.z=-8*256;BosPcDraw(&work);
  if(!world){fputs("FAIL actual BosPcDraw omitted world body registration; exterior body is clipped\n",stderr);return 1;}
  assert(worldX==192-x);assert(worldY==100-y); /* second WorldToScreen uses -112,-100 */
  for(int ty=-2;ty<34;ty++)for(int tx=-2;tx<34;tx++)assert(world(1,tx,ty)==(tx<0||tx>=32||ty<0||ty>=32?0:map0[ty*32+tx]));
 }
 step.gfxSet=1;BosPcDraw(&work);assert(world(1,10,11)==map1[11*32+10]);
 assert(world(0,10,11)<0);work.animSteps=NULL;BosPcDraw(&work);assert(world==NULL);
 work.animSteps=&step;step.op=1;BosPcDraw(&work);assert(world==NULL);step.op=0;BosPcDraw(&work);assert(world);task_bos_pc_3(&work);assert(world==NULL);
 puts("PASS actual BosPcDraw:943camera positions, finite32x32 singlebody map, animation change, inactive/destructor cleanup");return 0;
}
'''.replace('EXTENSION', extension).replace('DRAW', draw).replace('DESTROY', destroy)
compiler = shutil.which('clang') or shutil.which('gcc')
if not compiler:
    candidates = sorted((ROOT.parents[1] / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    if not candidates:
        raise SystemExit('Native C compiler unavailable')
    compiler = str(candidates[-1])
with tempfile.TemporaryDirectory(prefix='parasite-body-') as temp:
    c = Path(temp) / 'test.c'
    exe = Path(temp) / ('test.exe' if os.name == 'nt' else 'test')
    c.write_text(harness, encoding='utf-8')
    flags = {'creationflags': 0x08004000} if os.name == 'nt' else {}
    for command in ([compiler, '-std=c11', '-O1', str(c), '-o', str(exe)], [str(exe)]):
        result = subprocess.run(command, capture_output=True, text=True, **flags)
        print(result.stdout + result.stderr, end='')
        if result.returncode:
            raise SystemExit(result.returncode)
