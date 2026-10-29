"""Run the actual OAM emitter with canaries, native/spill slots and exhaustion."""
from pathlib import Path
import argparse, subprocess, tempfile
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
source=(R/'src/engine.c').read_text(encoding='utf8')
start=source.index('void UpdateSpriteOam()')
emitter=source[start:source.index('void SetSpriteMosaicEnabled(',start)]
unit=r'''#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "port/port_oam.h"
#define PLATFORM_PC 1
typedef uint16_t u16;typedef uint8_t u8;typedef int16_t s16;typedef int32_t s32;typedef uint32_t u32;
#define OAM_Y_MASK 255
#define OAM_X_MASK 511
#define OAM_AFFINE 256
#define OAM_DOUBLE_SIZE 512
#define OAM_PALETTE_MASK 61440
#define OAM_DISABLE 512
#define SPRITE_PRIORITY_MASK 3072
#define SPRITE_FLAG_NO_MOSAIC 1
#define SPRITE_FLAG_MOSAIC 2
#define SPRITE_FLAG_BLEND 4
#define SPRITE_FLAG_HFLIP 8
#define SPRITE_FLAG_VFLIP 16
#define VIEW_LEFT -1000
#define VIEW_TOP -1000
#define VIEW_RIGHT 2000
#define VIEW_BOTTOM 2000
/* Native RAM has ample guard words so the old unbounded writer fails an
 * assertion instead of faulting before we can identify the overwritten area. */
static u16 native[4096],extra[(PORT_OAM_CAPACITY-PORT_OAM_NATIVE)*4+16];
#define OAM native
typedef struct {int allocated,index;const void*src;} ObjTiles;
typedef struct {int index;const void*src;} ObjPalette;
typedef struct {int pa,pb,pc,pd,sx,sy,angle,doubleSize,index;} ObjAffine;
typedef struct {u16 *sprite;ObjAffine*affine;int flags,x,y;void*tiles,*palette;} SpriteEntry;
static struct {int oamUpdatesPaused,affineCount,entryCount,mosaicEnabled;ObjAffine affine[32];
 SpriteEntry entries[128];SpriteEntry*sortPtrs[128];} work,*gSpriteWork=&work;
static int gSineTable[320],highest,tagged[PORT_OAM_CAPACITY],parts[PORT_OAM_CAPACITY];
static void PortLog(const char*fmt,...){(void)fmt;}
void PortOamTagsReset(void){highest=PORT_OAM_NATIVE;memset(tagged,0,sizeof(tagged));memset(parts,0,sizeof(parts));}
uint16_t* PortOamPartBuffer(int n){u16*p=PortOamSlot(native,extra,n);if(p&&n+1>highest)highest=n+1;return p;}
static void PortOamSetPosition(int n,int x,int y){assert(n>=0&&n<PORT_OAM_CAPACITY);(void)x;(void)y;}
static void PortOamSetSource(int n,int entry){assert(n>=0&&n<PORT_OAM_CAPACITY);tagged[n]=entry+1;}
static void PortOamSetHdPart(int n,const void*t,const void*f,int x,int y,int w,int h,int flags,const void*p,int bank){
 assert(n>=0&&n<PORT_OAM_CAPACITY);parts[n]=1;(void)t;(void)f;(void)x;(void)y;(void)w;(void)h;(void)flags;(void)p;(void)bank;}
static void EngineObjSize(u16 a0,u16 a1,s16*w,s16*h){(void)a0;(void)a1;*w=*h=8;}
static int GetObjTileCount(u16 a0,u16 a1){(void)a0;(void)a1;return 1;}
'''+emitter+r'''
static void setup(u16*frame,int count){
 memset(&work,0,sizeof(work));for(int i=0;i<4096;i++)native[i]=0xCAFE;
 for(unsigned i=0;i<sizeof(extra)/sizeof(*extra);i++)extra[i]=0xBEEF;
 static ObjTiles tiles={1,3,(void*)1};static ObjPalette palette={2,(void*)2};
 work.entryCount=count;
 for(int i=0;i<count;i++){work.entries[i]=(SpriteEntry){frame,0,0,i,i,&tiles,&palette};work.sortPtrs[i]=&work.entries[i];}
}
static void guards(void){
 for(int i=PORT_OAM_NATIVE*4;i<4096;i++)assert(native[i]==0xCAFE);
 for(unsigned i=(PORT_OAM_CAPACITY-PORT_OAM_NATIVE)*4;i<sizeof(extra)/sizeof(*extra);i++)assert(extra[i]==0xBEEF);
}
int main(void){
 u16 two[]={2,0,0,0,8,8,0};setup(two,128);UpdateSpriteOam();guards();
 assert(highest==256&&tagged[255]==128&&parts[255]);
 assert((extra[(255-128)*4+1]&511)==135);
 for(int i=0;i<128;i++)assert(native[i*4+3]==0xCAFE); /* affine words untouched */
 /* Normal frames use the exact native slots; a shorter frame forgets spill. */
 setup(two,1);UpdateSpriteOam();guards();assert(highest==128&&tagged[0]==1&&tagged[1]==1);
 for(int i=2;i<128;i++)assert(native[i*4]==OAM_DISABLE);
 /* A malformed or unusually complex frame cannot write past either store. */
 u16 huge[1+600*3];memset(huge,0,sizeof(huge));huge[0]=600;setup(huge,1);
 UpdateSpriteOam();guards();assert(highest==PORT_OAM_CAPACITY&&parts[PORT_OAM_CAPACITY-1]);
 assert(!PortOamSlot(native,extra,-1)&&!PortOamSlot(native,extra,PORT_OAM_CAPACITY));
 assert(PortOamSlot(native,extra,127)==native+508&&PortOamSlot(native,extra,128)==extra);
 puts("PASS actual OAM emitter: native ABI, 256 parts, HD/source metadata, 512 bound, reset, canaries");
}
'''
with tempfile.TemporaryDirectory(prefix='oam-part-budget-') as d:
 c=Path(d)/'test.c';exe=Path(d)/'test.exe';c.write_text(unit,encoding='utf8')
 subprocess.run([str(a.cc.resolve()),'-std=c11','-O2','-I'+str(R/'port/include'),str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
