"""Real foreground builder and book draw/caller under renderer failures."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from test_mapcard_definition_bounds import function

root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
main=(root/'port/src/port_main.c').read_text(encoding='utf8')
caller='\n'.join(function(main.replace('static int '+n,'int '+n).replace('static void '+n,'void '+n),n) for n in ['DrawJournalBook','DrawJournalHeader','DrawJournalPicture'])
prefix=r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "port/jiminy_paper.h"
typedef struct SDL_Renderer {int unused;} SDL_Renderer;
typedef struct SDL_Texture {int id;} SDL_Texture;
typedef struct SDL_FRect {float x,y,w,h;} SDL_FRect;
typedef struct SDL_Rect {int x,y,w,h;} SDL_Rect;
typedef struct SDL_FColor {float r,g,b,a;} SDL_FColor;
typedef struct HdSpriteDef {int group,x0,y0,w,h;} HdSpriteDef;
typedef struct HdSpriteState {SDL_Texture*image,*white;} HdSpriteState;
typedef struct HdSpriteDraw {int id,alpha;float x[4],y[4],u0,u1,v0,v1;} HdSpriteDraw;
#define HD_SPRITE_COUNT 1
#define SDL_PIXELFORMAT_ARGB8888 1
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_BLENDMODE_BLEND 1
#define SDL_SCALEMODE_PIXELART 1
typedef int SDL_ScaleMode;
#define PORT_FILTER_LINEAR 1
static struct {int filter;} gPortConfig;
static int PortSmoothScaleMode(void){return 2;}
static int lastScale;
#define SDL_clamp(x,l,h) ((x)<(l)?(l):(x)>(h)?(h):(x))
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_floorf floorf
#define SDL_ceilf ceilf
static SDL_Renderer renderer,*sRenderer=&renderer;
static SDL_Texture base={1},whiteTex={2},fg={3},nativeTex={4},*sTexture=&nativeTex;
static HdSpriteDef kHdSprites[]={{5,0,16,240,144}};
static HdSpriteState sState[]={{&base,&whiteTex}};
static uint32_t native[240*160],composite[240*160];
static uint32_t *gPortFrame=native,*sComposite=composite;
static uint8_t mask[240*160];
static struct {int width,height;} gPortScreen={240,160};
const unsigned char gJiminyBgTiles[]={0};
static int failCreate,failBlend,failScale,failUpdate,failQuad,failForeground,failHeader;
static int baseReady=1,whiteReady=1,foregroundReady=1,group=1,id=1,paperReady=1;
static int quadBase,quadWhite,foregroundDraw,nativeFallback,headerDraw,created,destroyed,dark,white;
static int clipped;static SDL_Rect clip;
static int SDL_RenderClipEnabled(SDL_Renderer*r){(void)r;return clipped;}
static int SDL_GetRenderClipRect(SDL_Renderer*r,SDL_Rect*out){(void)r;*out=clip;return 1;}
static int SDL_SetRenderClipRect(SDL_Renderer*r,const SDL_Rect*in){(void)r;clipped=in!=0;if(in)clip=*in;return 1;}
static int GroupOn(int n){return n==5&&group;}
static int SpriteReady(int n){(void)n;return baseReady;}
static int SpriteTextureReady(SDL_Texture*t){return t&&(t==&base?baseReady:t==&whiteTex?whiteReady:foregroundReady);}
static SDL_Texture* SDL_CreateTexture(SDL_Renderer*r,int format,int access,int w,int h){
 (void)r;(void)format;(void)access;assert(w==240&&h==160);created++;return failCreate?0:&fg;
}
static int SDL_SetTextureBlendMode(SDL_Texture*t,int mode){(void)t;(void)mode;return !failBlend;}
static int SDL_SetTextureScaleMode(SDL_Texture*t,int mode){(void)t;lastScale=mode;return !failScale;}
static int SDL_UpdateTexture(SDL_Texture*t,void*r,const void*p,int pitch){(void)t;(void)r;assert(p&&pitch==960);return !failUpdate;}
static void SDL_DestroyTexture(SDL_Texture*t){assert(t==&fg);destroyed++;}
static int Quad(SDL_Texture*t,const HdSpriteDraw*q,float x,float y,float scale,SDL_FColor color){
 (void)x;(void)y;(void)color;assert(scale>0&&q->id==0&&q->y[0]==16&&q->y[2]==160);
 if(t==&base)quadBase++;else{assert(t==&whiteTex);quadWhite++;}return !failQuad;
}
static int SDL_RenderTexture(SDL_Renderer*r,SDL_Texture*t,const SDL_FRect*from,const SDL_FRect*to){
 (void)r;assert(to->w==960);
 if(t==&nativeTex){assert(from&&from->y==0&&from->h==160&&to->h==640);
  if(clipped&&clip.h<=64){headerDraw++;return !failHeader;}nativeFallback++;return 1;}
 assert(t==&fg&&!from&&to->h==640&&to->y==20);foregroundDraw++;return !failForeground;
}
static const uint8_t* PortPpuJiminyPaperMask(void){return paperReady?mask:0;}
static int PortHdSpriteFind(const void*t,const void*f){assert(t==gJiminyBgTiles&&!f);return id;}
static void PortPpuJiminyFade(int*d,int*w){*d=dark;*w=white;}
'''
unit=prefix+(root/'port/include/port/jiminy_book_draw.inc').read_text(encoding='utf8')+'\n'+caller+r'''
static void reset(void){
 if(sJiminyForeground)SDL_DestroyTexture(sJiminyForeground);sJiminyForeground=0;
 failCreate=failBlend=failScale=failUpdate=failQuad=failForeground=failHeader=0;
 baseReady=whiteReady=foregroundReady=group=id=paperReady=1;dark=white=0;
 quadBase=quadWhite=foregroundDraw=nativeFallback=headerDraw=created=destroyed=0;
 clipped=0;
}
int main(void){
 static uint32_t out[240*160];static uint16_t shownMap[1024],detailMap[1024];
 SDL_FRect src={0,0,240,160},dst={10,20,960,640};
 assert(PortJiminyDetailMap(shownMap,detailMap));shownMap[0]=1;assert(PortJiminyDetailMap(shownMap,detailMap));
 shownMap[64]=1;assert(!PortJiminyDetailMap(shownMap,detailMap));assert(!PortJiminyDetailMap(0,detailMap));
 for(int layer=0;layer<6;layer++)for(int blend=0;blend<2;blend++)
  assert(PortJiminyPaperPixel(layer,blend,20,40)==(layer==1&&!blend));
 assert(!PortJiminyPaperPixel(1,0,-1,40));assert(!PortJiminyPaperPixel(1,0,240,40));
 assert(!PortJiminyPaperPixel(1,0,20,15));assert(!PortJiminyPaperPixel(1,0,20,160));
 for(int i=0;i<240*160;i++){native[i]=composite[i]=0xffeeeeee;mask[i]=1;}
 mask[20*240+5]=0;composite[20*240+6]=0xff00ff00;
 assert(PortJiminyForeground(out,composite,native,mask,240,160,0,0));
 assert(out[20*240+4]==0&&out[20*240+5]==0xffeeeeee&&out[20*240+6]==0xff00ff00);
 for(int i=0;i<16*240;i++)assert(out[i]==0);
 out[0]=123;assert(!PortJiminyForeground(out,composite,native,mask,239,160,0,0)&&out[0]==123);
 assert(!PortJiminyForeground(out,composite,native,mask,240,160,1,0));
 assert(!PortJiminyForeground(out,composite,native,mask,240,160,0,1));
 reset();DrawJournalPicture(&src,&dst);assert(quadBase==1&&foregroundDraw==1&&!nativeFallback&&headerDraw==1);
 DrawJournalPicture(&src,&dst);assert(created==1&&lastScale==SDL_SCALEMODE_PIXELART); /* bounded texture reuse */
 gPortConfig.filter=PORT_FILTER_LINEAR;DrawJournalPicture(&src,&dst);assert(created==1&&lastScale==2);
 gPortConfig.filter=0;DrawJournalPicture(&src,&dst);assert(created==1&&lastScale==SDL_SCALEMODE_PIXELART);
 for(int failure=0;failure<9;failure++){
  reset();switch(failure){case 0:failCreate=1;break;case 1:failBlend=1;break;case 2:failScale=1;break;
   case 3:failUpdate=1;break;case 4:foregroundReady=0;break;case 5:baseReady=0;break;
   case 6:failQuad=1;break;case 7:failForeground=1;break;case 8:white=128;whiteReady=0;break;}
  DrawJournalPicture(&src,&dst);assert(nativeFallback==1&&!headerDraw);
  if(failure<6||failure==8)assert(!quadBase&&!quadWhite&&!foregroundDraw);
 }
 reset();white=255;DrawJournalPicture(&src,&dst);assert(!quadBase&&quadWhite==1&&foregroundDraw==1&&!nativeFallback&&headerDraw==1);
 reset();failHeader=1;DrawJournalPicture(&src,&dst);assert(quadBase==1&&foregroundDraw==1&&nativeFallback==1&&headerDraw==1);
 reset();clipped=1;clip=(SDL_Rect){0,0,1000,900};DrawJournalPicture(&src,&dst);assert(clipped&&clip.h==900&&headerDraw==1&&!nativeFallback);
 reset();id=0;DrawJournalPicture(&src,&dst);assert(!created&&!quadBase&&nativeFallback==1);
 reset();paperReady=0;DrawJournalPicture(&src,&dst);assert(!created&&!quadBase&&nativeFallback==1);
 reset();puts("PASS BG1 ownership/native foreground/UI edits/bounds; allocation/upload/bridge/draw failures restore native; texture reuse/full-white");
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-jiminy-paper-') as d:
 d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_text(unit,encoding='utf8')
 subprocess.run([str(a.cc.resolve()),'-std=c11','-O2','-Wall','-I'+str(root/'port/include'),str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
