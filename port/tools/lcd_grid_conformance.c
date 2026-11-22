/* Actual production helper/cache code against independent interval overlap. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port/lcd_grid.h"
typedef unsigned char Uint8;
typedef struct {float x,y,w,h;} SDL_FRect;
typedef struct {int w,h;Uint8 alpha;uint32_t* pixels;} SDL_Texture;
typedef int SDL_Renderer;
enum {SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,SDL_SCALEMODE_NEAREST,SDL_BLENDMODE_BLEND};
static SDL_Renderer renderer,*sRenderer=&renderer;
static struct {int lcdGrid;}gPortConfig;
static SDL_Texture *sLcdGridXTex,*sLcdGridYTex;
static PortLcdGridKey sLcdGridKey,sLcdGridAttemptKey;
static int sLcdGridReady,sLcdGridAttempted,sRendererResetPending;
static int creates,updates,draws,destroys,failCreateAt,failUpdate,failAlpha,failDraw,failMalloc;
static size_t uploaded,maxMaskAllocation;
static SDL_Texture* SDL_CreateTexture(SDL_Renderer*r,int format,int access,int w,int h){
 SDL_Texture*t;(void)r;(void)format;(void)access;assert(w==1||h==1);creates++;
 if(creates==failCreateAt)return NULL;t=calloc(1,sizeof(*t));assert(t);t->w=w;t->h=h;t->pixels=calloc((size_t)w*h,4);assert(t->pixels);return t;
}
static int SDL_SetTextureScaleMode(SDL_Texture*t,int mode){assert(t&&mode==SDL_SCALEMODE_NEAREST);return 1;}
static int SDL_SetTextureBlendMode(SDL_Texture*t,int mode){assert(t&&mode==SDL_BLENDMODE_BLEND);return 1;}
static int SDL_UpdateTexture(SDL_Texture*t,const void*rect,const uint32_t*p,int pitch){
 assert(t&&!rect&&pitch==t->w*4);updates++;if(failUpdate)return 0;
 memcpy(t->pixels,p,(size_t)t->w*t->h*4);uploaded+=(size_t)t->w*t->h;return 1;
}
static void SDL_DestroyTexture(SDL_Texture*t){assert(t);destroys++;free(t->pixels);free(t);}
static int SDL_SetTextureAlphaMod(SDL_Texture*t,Uint8 alpha){assert(t);t->alpha=alpha;return !failAlpha;}
static int SDL_RenderTexture(SDL_Renderer*r,SDL_Texture*t,const SDL_FRect*source,const SDL_FRect*dst){
 (void)r;assert(t&&!source&&dst->x==0&&dst->y==0&&dst->w==sLcdGridKey.width&&dst->h==sLcdGridKey.height);draws++;return !failDraw;
}
static void* MaskMalloc(size_t bytes){if(bytes>maxMaskAllocation)maxMaskAllocation=bytes;return failMalloc?NULL:malloc(bytes);}
#define malloc MaskMalloc
#include "../src/port_lcd_grid.inc"
#undef malloc
static int Oracle(int pixel,double origin,double pitch){
 double a=pixel-origin,b=a+1,covered=0,gap=fmin(pitch/8,1);
 int first=(int)floor(a/pitch)-2,last=(int)ceil(b/pitch)+2;
 for(int k=first;k<=last;k++){
  double left=(k+1)*pitch-gap,right=(k+1)*pitch;
  double lo=fmax(a,left),hi=fmin(b,right);if(hi>lo)covered+=hi-lo;
 }
 return (int)fmin(255,fmax(0,floor(covered*255+.5)));
}
static void Coverage(void){
 const double pitches[]={.125,.5,1,1.25,2,2.5,3,4,5.3875,8,16,100};
 const double origins[]={-101.75,-3,-.25,0,.125,.5,1,13.75};int tested=0;
 PortLcdGridKey a,b;
 assert(!PortLcdGridMakeKey(&a,0,10,0,0,2,2));assert(!PortLcdGridMakeKey(&a,10,32769,0,0,2,2));
 assert(!PortLcdGridMakeKey(&a,10,10,0,0,0,2));assert(!PortLcdGridMakeKey(&a,10,10,NAN,0,2,2));
 assert(!PortLcdGridMakeKey(&a,10,10,0,0,INFINITY,2));
 for(size_t p=0;p<sizeof(pitches)/sizeof(*pitches);p++)for(size_t o=0;o<sizeof(origins)/sizeof(*origins);o++){
  double pitch=pitches[p],phase=PortLcdGridPhase(origins[o],pitch);
  for(int i=0;i<1024;i++){int x=PortLcdGridAlpha(i,phase,pitch),want=Oracle(i,origins[o],pitch);assert(abs(x-want)<=1);tested++;}
 }
 assert(PortLcdGridMakeKey(&a,100,200,.25,-.25,3,5));
 assert(PortLcdGridMakeKey(&b,100,200,6.25,9.75,3,5));assert(PortLcdGridSame(&a,&b));
 b.pitchY=4;assert(!PortLcdGridSame(&a,&b));
 printf("PASS %d integrated coverage samples: negative/fractional origins, subpixel/integer pitches, phase periodicity and anisotropic keys\n",tested);
}
static void Caches(void){
 int c,u,d;SDL_Texture *oldX,*oldY;
 DrawLcdGrid(1920,1080,.25f,-.5f,4,4);assert(!creates&&!updates&&!draws&&!maxMaskAllocation);
 gPortConfig.lcdGrid=50;DrawLcdGrid(1920,1080,.25f,-.5f,4,4);
 assert(creates==2&&updates==2&&draws==2&&uploaded==3000&&maxMaskAllocation==1920*4);
 assert(sLcdGridXTex->alpha==128&&sLcdGridYTex->alpha==128);
 for(int x=0;x<1920;x++)assert((int)(sLcdGridXTex->pixels[x]>>24)==Oracle(x,.25,4));
 for(int y=0;y<1080;y++)assert((int)(sLcdGridYTex->pixels[y]>>24)==Oracle(y,-.5,4));
 c=creates;u=updates;DrawLcdGrid(1920,1080,.25f,-.5f,4,4);assert(creates==c&&updates==u&&draws==4);
 gPortConfig.lcdGrid=25;DrawLcdGrid(1920,1080,.25f,-.5f,4,4);assert(creates==c&&updates==u&&draws==6&&sLcdGridXTex->alpha==64);
 DrawLcdGrid(1920,1080,8.25f,7.5f,4,4);assert(creates==c&&updates==u); /* whole-cell shifts */
 oldX=sLcdGridXTex;oldY=sLcdGridYTex;d=draws;failCreateAt=creates+2;
 DrawLcdGrid(1920,1080,.5f,-.5f,4,4);assert(sLcdGridXTex==oldX&&sLcdGridYTex==oldY&&draws==d);
 c=creates;DrawLcdGrid(1920,1080,.5f,-.5f,4,4);assert(creates==c&&draws==d); /* no retry storm */
 failCreateAt=0;DrawLcdGrid(1920,1080,.25f,-.5f,4,4);assert(creates==c&&draws==d+2); /* old cache re-arms */
 DrawLcdGrid(1920,1080,.5f,-.5f,4,4);assert(creates==c+2&&updates==u+3);
 oldX=sLcdGridXTex;oldY=sLcdGridYTex;d=draws;failMalloc=1;
 DrawLcdGrid(640,480,0,0,2,3);assert(sLcdGridXTex==oldX&&sLcdGridYTex==oldY&&draws==d);
 gPortConfig.lcdGrid=0;DrawLcdGrid(640,480,0,0,2,3);gPortConfig.lcdGrid=100;failMalloc=0;
 DrawLcdGrid(640,480,0,0,2,3);assert(draws==d+2&&sLcdGridXTex->alpha==255);
 oldX=sLcdGridXTex;oldY=sLcdGridYTex;d=draws;failUpdate=1;
 DrawLcdGrid(641,480,0,0,2,3);assert(sLcdGridXTex==oldX&&sLcdGridYTex==oldY&&draws==d);
 failUpdate=0;gPortConfig.lcdGrid=0;DrawLcdGrid(641,480,0,0,2,3);gPortConfig.lcdGrid=50;
 DrawLcdGrid(641,480,0,0,2,3);assert(draws==d+2);
 failAlpha=1;DrawLcdGrid(641,480,0,0,2,3);assert(sRendererResetPending);failAlpha=0;sRendererResetPending=0;
 failDraw=1;DrawLcdGrid(641,480,0,0,2,3);assert(sRendererResetPending);failDraw=0;
 SDL_DestroyTexture(sLcdGridXTex);SDL_DestroyTexture(sLcdGridYTex);
 assert(destroys==creates-1); /* the one failed CreateTexture had no owner */
 puts("PASS actual LCD cache: off zeroGPU/allocation, O(w+h)upload, steady/slider zero uploads, two draws, geometry/phase, failures preserve picture and suppress repeated attempts");
}
int main(void){Coverage();Caches();return 0;}
