"""Production HD draw paths: full-white must not retain antialiased base colour.

CPU source-over consumer with real SDL types; this supplements actual GPU scene
captures, not a claim about a physical device or intermediate-fade equivalence.
"""
from pathlib import Path
import argparse, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
def function(text,sig):
 a=text.index(sig);i=text.index('{',a)+1;depth=1
 while depth:depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[a:i]
def structure(text,name):
 a=text.index('typedef struct '+name+' {');b=text.index('} '+name+';',a)+len('} '+name+';');return text[a:b]
def main():
 p=argparse.ArgumentParser();p.add_argument('--source-ref');a=p.parse_args()
 file='port/src/port_hdsprites.c'
 src=subprocess.check_output(['git','show',a.source_ref+':'+file],cwd=ROOT,text=True) if a.source_ref else (ROOT/file).read_text()
 code=r'''
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SDL_strstr strstr
#define SDL_abs abs
#define SDL_floorf floorf
#define SDL_ceilf ceilf
#define HD_LAYERS 3
'''+''.join(structure(src,n) for n in ('HdSpriteDef','HdSpriteState','HdSpriteDraw'))+r'''
static SDL_Renderer*sRenderer;static SDL_Texture* sArt=(SDL_Texture*)1,*sArtWhite=(SDL_Texture*)2;
static HdSpriteDef kHdSprites[1]={{.name="probe",.w=16,.h=16}};
static HdSpriteState sState[1];static HdSpriteDraw sDraws[3][1],sArtDraws[1];
static int sDrawCount[3],sArtCount,sBarCount,sRingRatio=-1,gPortHudW=1,gPortHudH=1;
enum{RING_FACE_X=0,RING_FACE_Y=0};
static void PortHdGaugeDraw(SDL_Renderer*r,float a,float b,float c,float d){}
static void PortHdEnemyGaugeDraw(SDL_Renderer*r,float a,float b,float c,float d){}
static void BarDraw(SDL_Renderer*r,float a,float b,float c,float d){}
static int LoadCounted(int i){return 1;}static int PortFrameCount(void){return 1;}
static int RingLoad(void){return 0;}static void RingDraw(float a,float b,float c,SDL_FColor d){}
static int MenuLightName(const char*n){return 0;}
static void SpriteMenuLight(SDL_Renderer*r,const HdSpriteDraw*q,float a,float b,float c,float d,float e){}
static int PortPpuHudSpriteVisible(int i,int x,int y){return 1;}
static bool TestClip(SDL_Renderer*r,const SDL_Rect*c){return true;}
#define SDL_SetRenderClipRect TestClip
static float pixel,texAlpha,baseColour;static unsigned calls,edgeCalls,edgeBaseCalls;
static void Quad(SDL_Texture*t,const HdSpriteDraw*q,float ox,float oy,float scale,SDL_FColor c){
 if(q->x[0]<0){edgeCalls++;edgeBaseCalls+=t!=(SDL_Texture*)2;return;}
 float alpha=texAlpha*c.a,colour=t==(SDL_Texture*)2?1:baseColour*c.r;
 pixel=colour*alpha+pixel*(1-alpha);calls++;
}
'''+function(src,'void PortHdSpritesDraw(')+function(src,'void PortHdSpritesHudFront(')+function(src,'static int FadeFits(')+function(src,'void PortPaletteFade(')+r'''
static float legacy(float bg,float A,float C,float shade,float white){
 float p=C*shade*A+bg*(1-A);return white>0?A*white+p*(1-A*white):p;
}
int main(void){
 uint16_t original[16],current[16];int dark,white;
 for(int i=0;i<16;i++){original[i]=(i*29)&32767;current[i]=32767;}
 PortPaletteFade(original,current,16,&dark,&white);assert(!dark&&white==255);
 static const unsigned fades[]={0,64,128,254,255},alphas[]={0,1,63,127,191,254,255};
 static const unsigned backgrounds[]={0,37,128,255},colours[]={0,53,191,255},opacity[]={109,255};
 unsigned tested=0;
 for(int path=0;path<7;path++)for(unsigned f=0;f<5;f++)for(unsigned a=0;a<7;a++)
 for(unsigned b=0;b<4;b++)for(unsigned c=0;c<4;c++)for(unsigned o=0;o<2;o++){
  memset(sDrawCount,0,sizeof(sDrawCount));sArtCount=0;
  HdSpriteDraw q={0};q.x[1]=q.x[2]=1;q.y[2]=q.y[3]=1;q.alpha=opacity[o];
  q.white=path==5?0:fades[f];q.dark=32;q.hudOwner=path==4?1:0;
  int layer=path<3?path:0;int global=path==5?(fades[f]==255?16:(int)fades[f]/16):0;
  if(path==3)sArtDraws[0]=q,sArtCount=1;
  else{sDrawCount[path==4?1:layer]=1;sDraws[path==4?1:layer][0]=q;}
  sState[0].image=(SDL_Texture*)1;sState[0].white=(SDL_Texture*)2;
  kHdSprites[0].name=path==6?"PremireChanceTitle":"probe";
  pixel=backgrounds[b]/255.f;texAlpha=alphas[a]/255.f;baseColour=colours[c]/255.f;calls=edgeCalls=edgeBaseCalls=0;
  if(path==4)PortHdSpritesHudFront(NULL,0,0,1,0,0);
  else PortHdSpritesDraw(NULL,layer,path==6?3:0,0,1,0,global);
  float white=path==5?global/16.f:q.white/255.f,A=texAlpha*q.alpha/255.f;
  float expected=white>=1?A+backgrounds[b]/255.f*(1-A):legacy(backgrounds[b]/255.f,A,baseColour,1-32/255.f,white);
  if(fabsf(pixel-expected)>=0.000002f){
   fprintf(stderr,"Mismatch path=%d white=%g texAlpha=%u opacity=%u background=%u colour=%u got=%g expected=%g\n",path,white,alphas[a],opacity[o],backgrounds[b],colours[c],pixel,expected);return 1;
  }
  if(path==6){assert(edgeCalls==(white>0&&white<1?2:1));assert(edgeBaseCalls==(white<1?1:0));}
  tested++;
 }
 printf("PASS %u HD draw endpoint/intermediate controls across art, world, HUD, screen and global-white paths\n",tested);
}
'''
 cc=next(TOOLS.glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'));sdl=next(TOOLS.glob('SDL3-*/x86_64-w64-mingw32/include'))
 with tempfile.TemporaryDirectory(prefix='hd-white-endpoint-') as temp:
  temp=Path(temp);c=temp/'test.c';exe=temp/'test.exe';c.write_text(code)
  subprocess.run([str(cc),'-std=c11','-O2',f'-I{sdl}',str(c),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
