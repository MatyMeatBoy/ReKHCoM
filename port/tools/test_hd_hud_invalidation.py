"""Fault-inject production HUD prepare/shutdown/invalidate without SDL or a GPU."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'port/src/port_hdhud.c').read_text()
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
PREAMBLE=r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct{int generation;} SDL_Texture;
typedef int SDL_Renderer; typedef int SDL_ScaleMode;
typedef struct{float x,y,w,h;} SDL_FRect;
typedef struct{int w,h,pitch,owned;void*pixels;} SDL_Surface;
#define SDL_PIXELFORMAT_RGBA32 0
#define SDL_BLENDMODE_NONE 0
#define SDL_BLENDMODE_BLEND 1
#define SDL_SCALEMODE_LINEAR 1
#define SDL_SCALEMODE_NEAREST 0
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_free free
#define SDL_getenv getenv
#define SDL_strcmp strcmp
static SDL_Texture *sFrame,*sWhiteFrame,*sLabel,*sWhiteLabel,*sPlayerFrame,*sWhitePlayerFrame;
static SDL_Renderer*sAssetRenderer;
static struct{SDL_Texture*image,*white;}*sAssets;
static void*sAssetDraws;static int sAssetCount,sAssetDrawCount;static size_t sAssetBytes;
static int sAttempted,sExportCount,sLabelLanguage=-1;
static uint32_t*sExportPixels;static uint64_t*sExportHashes;
static SDL_FRect sSource;
static struct{int hdHud;}gPortConfig={1};
static int generation=1,decodes,creates,destroys,failUpload;
static void SDL_DestroyTexture(SDL_Texture*t){assert(t);destroys++;free(t);}
static SDL_Texture*texture(void){creates++;if(failUpload)return NULL;SDL_Texture*t=malloc(sizeof(*t));t->generation=generation;return t;}
static unsigned char*LoadCoreHudImage(const char*p,int*w,int*h,int*c){(void)p;decodes++;*w=*h=4;*c=4;unsigned char*x=calloc(64,1);x[(1*4+1)*4+3]=255;return x;}
static void stbi_image_free(void*p){free(p);}
static int HudLanguage(void){return 2;}
static void LoadLabel(SDL_Renderer*r){(void)r;sLabel=texture();sWhiteLabel=texture();sLabelLanguage=HudLanguage();}
static void LoadPlayerFrame(SDL_Renderer*r){(void)r;}
static void PortLog(const char*f,...){(void)f;}
static SDL_Surface*SDL_CreateSurfaceFrom(int w,int h,int f,void*px,int pitch){(void)f;SDL_Surface*s=calloc(1,sizeof(*s));s->w=w;s->h=h;s->pitch=pitch;s->pixels=px;return s;}
static SDL_Surface*SDL_CreateSurface(int w,int h,int f){(void)f;SDL_Surface*s=calloc(1,sizeof(*s));s->w=w;s->h=h;s->pitch=w*4;s->pixels=calloc((size_t)w*h,4);s->owned=1;return s;}
static void SDL_DestroySurface(SDL_Surface*s){if(s->owned)free(s->pixels);free(s);}
static int SDL_SetSurfaceBlendMode(SDL_Surface*s,int b){(void)s;(void)b;return 1;}
static int SDL_BlitSurfaceScaled(SDL_Surface*a,void*b,SDL_Surface*c,void*d,int e){(void)a;(void)b;(void)c;(void)d;(void)e;return 1;}
static SDL_Texture*SDL_CreateTextureFromSurface(SDL_Renderer*r,SDL_Surface*s){(void)r;(void)s;return texture();}
static const char*SDL_GetRendererName(SDL_Renderer*r){(void)r;return "software";}
static void SDL_SetTextureBlendMode(SDL_Texture*t,int m){(void)t;(void)m;}
static void SDL_SetTextureScaleMode(SDL_Texture*t,int m){(void)t;(void)m;}
'''
TEST=r'''
int main(void){
 SDL_Renderer renderer=1;
 PortHdHudPrepare(&renderer);assert(sFrame&&sWhiteFrame&&sLabel&&sWhiteLabel);assert(decodes==1 && creates==4 && sSource.w>0);
 sAssets=calloc(2,sizeof(*sAssets));sAssetCount=2;sAssetDrawCount=2;sAssetBytes=123;
 for(int i=0;i<2;i++){sAssets[i].image=texture();sAssets[i].white=texture();}
 sAssetDraws=malloc(16);
 sExportHashes=malloc(8*sizeof(*sExportHashes));sExportCount=8;sExportPixels=malloc(16);
 PortHdHudShutdown();assert(!sFrame&&!sLabel&&!sExportHashes&&!sExportPixels);assert(!sExportCount && !sAssetBytes && !sAssets && !sAssetDraws && !sAssetCount && !sAssetDrawCount);
 int released=destroys;PortHdHudShutdown();assert(destroys==released);
 generation=2;PortHdHudInvalidate();assert(!sAttempted&&sLabelLanguage==-1);
 PortHdHudPrepare(&renderer);assert(decodes==2&&sFrame&&sFrame->generation==2&&sLabel->generation==2);
 released=destroys;PortHdHudInvalidate();assert(destroys==released+4&&!sAssetRenderer);
 failUpload=1;int before=decodes;PortHdHudPrepare(&renderer);assert(decodes==before+1&&!sFrame&&!sWhiteFrame);
 for(int frame=0;frame<30;frame++)PortHdHudPrepare(&renderer);
 assert(decodes==before+1); /* upload failure stays cached in this generation */
 generation=3;failUpload=0;PortHdHudInvalidate();PortHdHudPrepare(&renderer);
 assert(decodes==before+2&&sFrame&&sFrame->generation==3);
 PortHdHudInvalidate();PortHdHudInvalidate();assert(!sFrame&&!sAssets&&!sExportHashes&&!sExportCount);
 puts("PASS production HUD: shutdown export state, explicit generation recovery, idempotent cleanup, failed upload cached");
 return 0;
}
'''
core=function('void PortHdHudShutdown(')
core+=function('void PortHdHudInvalidate(') if 'void PortHdHudInvalidate(' in s else '\nvoid PortHdHudInvalidate(void){PortHdHudShutdown();}\n'
core+=function('void PortHdHudPrepare(')
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='rekhcom-hud-reset-') as td:
 p=Path(td);(p/'hud.c').write_text(PREAMBLE+core+TEST);subprocess.run([str(cc),'-std=c99','-Wall','-Wextra','-Werror',str(p/'hud.c'),'-o',str(p/'hud.exe')],check=True);subprocess.run([str(p/'hud.exe')],check=True)

