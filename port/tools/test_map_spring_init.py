#!/usr/bin/env python3
"""Run the real spring initializer/update/culling against recycled work bytes.

Requires configured USA generated headers and an i686 MinGW compiler. Animation,
asset, floor and contact dependencies are controlled; this is a CPU contract
test, not a physical Android or complete room reproduction.
"""
import argparse,hashlib,json,pathlib,re,subprocess
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc',required=True,type=pathlib.Path)
parser.add_argument('--out',required=True,type=pathlib.Path)
args=parser.parse_args()
S=pathlib.Path(__file__).resolve().parents[2]
O=args.out.resolve();O.mkdir(parents=True,exist_ok=True)
def body(path,name):
 text=path.read_text();m=re.search(r'^\w+\s+\*?'+name+r'\([^;]*?\)\s*\{',text,re.M);assert m,name
 i=text.index('{',m.start());level=1;j=i+1
 while level:
  if text[j]=='{':level+=1
  elif text[j]=='}':level-=1
  j+=1
 return text[m.start():j]
functions=[body(S/'src/map/map_tasks.c',n) for n in ['MapGmkJumpWaitStep','MapGmkJumpWaitJump','MapGmkJumpLaunch','Task_MapGmk_Jump_0','Task_MapGmk_Jump_1']]
functions.insert(0,body(S/'src/map/map.c','FldObjIsOutOfView'))
prefix=r'''
#include <stdio.h>
#include <string.h>
#include "map.h"
#include "map_tasks.h"
#include "field_state.h"
#include "btl_collision.h"
#include "obj_api.h"
#include "sprites_map.h"
#include "sprites_map_tasks.h"
FieldState* gFieldState; MapRoomState* gMapRoomState;
u16 gMapGmkJumpPalette[16]; const u8 gMapGmkJumpTiles[2432];
void* gMapGmkJumpFrames[8]; AnimHeader* gMapGmkJumpAnims[3];
static int disabled;
s32 GetFldPosFloor(FldPos* p) { (void)p; return 0; }
s32 IsMapInterrupted(void) { return 0; }
u8 ColliderIsTouchingType(Collider* c,s32 type) { (void)c;(void)type;return 0; }
void ColliderInit(Collider* c,u32 type,u16 radius,u16 height) { memset(c,0,sizeof(*c));c->type=type;c->radius=radius<<8;c->height=height<<8;c->flags=1; }
void ColliderSetPosition(Collider* c,s32 x,s32 y,s32 z) { c->x=x;c->y=y*2;c->z=z; }
void ColliderSetDisabled(Collider* c,u8 on) { (void)c;disabled=on; }
struct ObjPalette* LoadObjPalette(const void* p,u16 size) { (void)p;(void)size;return (struct ObjPalette*)1; }
struct ObjTiles* LoadObjTiles(const void* p,u16 size) { (void)p;(void)size;return (struct ObjTiles*)1; }
void AnimInit(AnimState* a,void* b,void* c) { memset(a,0,sizeof(*a));(void)b;(void)c; }
void AnimStart(AnimState* a,u16 b,u16 c) { (void)a;(void)b;(void)c; }
void* AnimUpdate(AnimState* a) { (void)a;return 0; }
u8 AnimIsFinished(AnimState* a) { (void)a;return 0; }
'''
main=r'''
int main(void) {
 FieldState field;MapRoomState room;MapPlatform platform;MapGmkJumpWork work;
 int i,j,fail=0; const unsigned char fills[]={0,0xa5,0xff,0x7f};
 const int spots[]={MAP_CELL_TYPE_RIGHT_WALL_TOP,MAP_CELL_TYPE_LEFT_WALL_TOP,MAP_CELL_TYPE_FLOOR};
 const int angles[]={FLD_ANGLE_UP_RIGHT,FLD_ANGLE_UP_LEFT,FLD_ANGLE_UP};
 memset(&field,0,sizeof(field));memset(&room,0,sizeof(room));memset(&platform,0,sizeof(platform));
 gFieldState=&field;gMapRoomState=&room;platform.x=4;platform.y=10;platform.spotType=MAP_CELL_TYPE_RIGHT_WALL_TOP;platform.spotLowerZ=0;platform.spotUpperZ=-0x7000;
 for(j=0;j<3;j++) for(i=0;i<4;i++) {
  platform.spotType=spots[j];
  memset(&work,fills[i],sizeof(work));Task_MapGmk_Jump_0(&work,&platform);
  field.actor.fieldPosition=work.obj.fieldPosition;field.x=work.obj.fieldPosition.x-0x7800;field.y=work.obj.fieldPosition.y-0x5000;
  disabled=-1;Task_MapGmk_Jump_1(&work);
  printf("spot=%d fill=%02x obj_height=%d out_of_view=%d collider_disabled=%d\n",spots[j],fills[i],work.obj.height,FldObjIsOutOfView(&work.obj),disabled);
  if(disabled!=0 || work.obj.height!=0 || work.jumpHeight!=0x7000 || work.obj.angle!=angles[j])fail++;
  field.x=work.obj.fieldPosition.x+0x10000;
  disabled=-1;Task_MapGmk_Jump_1(&work);
  if(disabled!=1)fail++;
 }
 printf("failures=%d\n",fail);return fail?1:0;
}
'''
states=re.search(r'enum MapGmkJumpState\s*\{[^}]+\};',(S/'src/map/map_tasks.c').read_text()).group()
c=O/'probe.c';c.write_text(prefix+states+'\n'.join(functions)+main,encoding='utf-8')
cc=args.cc.resolve()
includes=[S/'include',S/'build/us/gen',*[x for x in (S/'include').iterdir() if x.is_dir() and x.name!='gba']]
cmd=[str(cc),'-O2','-std=gnu11','-fno-strict-aliasing','-fwrapv','-mno-ms-bitfields','-DPLATFORM_PC','-DVERSION_US']
for path in includes:cmd+=['-iquote',str(path)]
cmd += ['-I'+str(S/'port/include'),str(c),'-o',str(O/'probe.exe')]
result=subprocess.run(cmd,capture_output=True,text=True);(O/'compile.log').write_text(result.stdout+result.stderr)
if result.returncode:print(result.stderr);raise SystemExit(result.returncode)
result=subprocess.run([str(O/'probe.exe')],capture_output=True,text=True)
record=dict(exit=result.returncode,output=result.stdout,probe_sha256=hashlib.sha256(c.read_bytes()).hexdigest(),functions_sha256=[hashlib.sha256(x.encode()).hexdigest() for x in functions],
            scope='Actual initializer/update/culling bodies with real32-bit game types. Controlled animation/assets/floor/contact dependencies; recycled-work memory precondition only, not reporter/Android reproduction.')
(O/('GREEN.json' if result.returncode==0 else 'RED.json')).write_text(json.dumps(record,indent=2),encoding='utf-8')
print(json.dumps(record));raise SystemExit(result.returncode)
