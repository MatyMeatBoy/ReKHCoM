"""Actual presenter helper with native/bridge texture failure injection."""
from pathlib import Path
import argparse,subprocess,tempfile
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
fixture=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define PORT_MAX_W 2560
#define SDL_PIXELFORMAT_ARGB8888 1
#define SDL_TEXTUREACCESS_STREAMING 2
#define SDL_BLENDMODE_ADD 3
#define SDL_SCALEMODE_PIXELART 4
#define SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER 5
typedef struct SDL_Renderer {int unused;} SDL_Renderer;
typedef struct SDL_Texture {int width;} SDL_Texture;
typedef struct SDL_FRect {float x,y,w,h;} SDL_FRect;
static int allocation,upload,blend,scale,draw,logs,live,peak,attempts,creates,destroys,pitch,protocol=2560;
static void PortLog(const char* format,...) {(void)format;logs++;}
static int SDL_GetRendererProperties(SDL_Renderer*r){(void)r;return 1;}
static int SDL_GetNumberProperty(int props,int name,int fallback){(void)props;(void)name;(void)fallback;return protocol;}
static SDL_Texture* SDL_CreateTexture(SDL_Renderer*r,int format,int access,int w,int h){
 (void)r;(void)format;(void)access;attempts++;if(allocation)return NULL;
 if(w<240||w>2560||h<160||(size_t)w*h>2560*160)abort();SDL_Texture*t=malloc(sizeof(*t));t->width=w;
 creates++;live++;if(live>peak)peak=live;return t;
}
static void SDL_DestroyTexture(SDL_Texture*t){if(t){free(t);live--;destroys++;}}
static int SDL_SetTextureBlendMode(SDL_Texture*t,int mode){(void)t;if(mode!=SDL_BLENDMODE_ADD)abort();return !blend;}
static int SDL_SetTextureScaleMode(SDL_Texture*t,int mode){(void)t;if(mode!=SDL_SCALEMODE_PIXELART)abort();return !scale;}
static int SDL_UpdateTexture(SDL_Texture*t,const void*rect,const void*px,int stride){(void)t;(void)rect;(void)px;pitch=stride;return !upload;}
static int SDL_RenderTexture(SDL_Renderer*r,SDL_Texture*t,const void*src,const SDL_FRect*dst){(void)r;(void)t;(void)src;(void)dst;return !draw;}
#include "port/port_screen_world_blend.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
 static uint32_t light[2560*160];SDL_Renderer renderer={0};SDL_FRect dst={0,0,1280,720};
 PortScreenWorldBlend state={0};
 CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,239)&&creates==0);
 CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,2561)&&creates==0);
 CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,NULL,240)&&creates==0);
#ifdef PORT_GBA_RELOC
 protocol=0;CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240)&&creates==0&&logs==1);
 CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240)&&logs==1);protocol=2560;
#endif
 allocation=1;CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240)&&!state.texture);
 CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240)&&attempts==1);
 PortScreenWorldBlendIdle(&state);
 allocation=0;upload=1;CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240)&&creates==1&&live==1);
 CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240)&&creates==1); /* bounded retry, no recreations */
 upload=0;blend=1;CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240));
 blend=0;scale=1;CHECK(!PortScreenWorldBlendPrepare(&state,&renderer,light,240));
 scale=0;CHECK(PortScreenWorldBlendPrepare(&state,&renderer,light,240)&&pitch==960);
 CHECK(PortScreenWorldBlendDraw(&state,&renderer,&dst)&&!state.failed);
 int before=logs;draw=1;
 CHECK(!PortScreenWorldBlendDraw(&state,&renderer,&dst)&&logs==before+1);
 CHECK(!PortScreenWorldBlendDraw(&state,&renderer,&dst)&&logs==before+1);
 draw=0;CHECK(PortScreenWorldBlendDraw(&state,&renderer,&dst)&&!state.failed);
 CHECK(PortScreenWorldBlendPrepare(&state,&renderer,light,960)&&pitch==3840&&peak==1&&destroys==1);
 CHECK(PortScreenWorldBlendPrepareSize(&state,&renderer,light,480,240)&&state.height==240&&peak==1&&destroys==2);
 int allocated=creates;
 CHECK(!PortScreenWorldBlendPrepareSize(&state,&renderer,light,2560,200)&&creates==allocated);
 CHECK(!PortScreenWorldBlendPrepareSize(&state,&renderer,light,240,0)&&creates==allocated);
 SDL_DestroyTexture(state.texture);state.texture=NULL;state.width=0;
 CHECK(live==0&&creates==destroys);
 puts("PASS real additive presenter: native/bridge bounds, absent capability, allocation, setup, upload, draw, bounded retries, resize and recovery");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='screen-world-blend-') as d:
 d=Path(d);c=d/'test.c';c.write_text(fixture)
 for bridge in (False,True):
  exe=d/('bridge.exe' if bridge else 'native.exe')
  command=[str(a.cc.resolve()),'-O1','-std=c11','-I'+str(R/'port/include')]
  if bridge:command+=['-DPORT_GBA_RELOC']
  subprocess.run(command+[str(c),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True,timeout=15)
