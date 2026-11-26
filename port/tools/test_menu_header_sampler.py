"""Compare the prepared header sampler to the real original sampler."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'port/src/port_ppu.c').read_text(encoding='utf8')
def function(name):
 a=s.rfind('\n',0,s.index(name+'('))+1;b=s.index('{',a);i=b+1;depth=1
 while depth:depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
a=s.index('typedef struct HeaderSamplerPiece');b=s.index('static int HeaderSamplerInit',a)
types=s[a:b]
program=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct ObjTiles {struct ObjTiles *self;int allocated,index;} ObjTiles;
typedef struct ObjPalette {struct ObjPalette *self;int index;} ObjPalette;
static const int sObjSize[3][4][2]={{{8,8},{16,16},{32,32},{64,64}},{{16,8},{32,8},{32,16},{64,32}},{{8,16},{8,32},{16,32},{32,64}}};
static uint16_t display=0x1000;
#define IO16(x) (display)
static int ObjColor(uint16_t a0,uint16_t a2,int x,int y,int w,int map){
 unsigned v=(a0*13u+a2*31u+x*7u+y*11u+w*19u+map*23u);return (v%7==0)?-1:(int)(v&32767);
}
''' + function('MenuSpriteColor') + types + function('HeaderSamplerInit') + function('HeaderSamplerColor') + r'''
int main(void){
 uint32_t seed=0x21ab43;uint16_t frame[385];ObjTiles t={0};ObjPalette p={0};t.self=&t;p.self=&p;
 for(int sample=0;sample<2500;sample++){
  seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;int n=seed%7;frame[0]=n;
  for(int i=0;i<n;i++){
   seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;frame[1+3*i]=(seed&255)|((seed>>8&3)<<14)|((seed>>10&1)<<13);
   seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;frame[2+3*i]=(seed&511)|((seed>>9&3)<<14)|((seed>>11&3)<<12);
   seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;frame[3+3*i]=seed&0xf3ff;
  }
  t.index=(seed>>8)&1023;t.allocated=seed&1;p.index=(seed>>16)&15;display=(seed&2)?0x1040:0x1000;
  int top=32767,bottom=-32767;HeaderSampler sampler;
  int old=MenuSpriteColor(frame,&t,&p,0,0,&top,&bottom);
  if(!HeaderSamplerInit(&sampler,frame,&t,&p)||old==-2||sampler.top!=top||sampler.bottom!=bottom)return 1;
  for(int y=-16;y<32;y++)for(int x=0;x<96;x++){
   old=MenuSpriteColor(frame,&t,&p,y,x,0,0);
   if(old!=HeaderSamplerColor(&sampler,y,x)){fprintf(stderr,"sample %d x %d y %d\n",sample,x,y);return 2;}
  }
 }
 t.self=0;HeaderSampler sampler;if(HeaderSamplerInit(&sampler,frame,&t,&p))return 3;
 puts("PASS: prepared header sampling matches original across 11.5 million positions");return 0;
}
'''
base=next(p for p in root.parents if (p/'toolchain').is_dir());cc=next((base/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='menu-header-sampler-') as d:
 p=Path(d);(p/'test.c').write_text(program);subprocess.run([str(cc),'-O2','-std=gnu11',str(p/'test.c'),'-o',str(p/'test.exe')],check=True);subprocess.run([str(p/'test.exe')],check=True)
