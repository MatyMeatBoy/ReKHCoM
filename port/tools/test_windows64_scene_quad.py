"""Exercise the actual Windows64 draw consumer with SDL software at1080p."""
import argparse,os,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ap=argparse.ArgumentParser();ap.add_argument('--cc',required=True);ap.add_argument('--sdl',type=Path,required=True);a=ap.parse_args()
s=(ROOT/'port/windows64/windows64_host.c').read_text();start=s.index('static void H_gfx_draw(');end=s.index('\nstatic void H_notify',start);draw=s[start:end]
code=r'''
#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bridge.h"
static SDL_Renderer*sRenderer;static SDL_Texture*sTex[1];static int sGpuOutputBlocked,sBlend,sTexSmooth[1];
static int HostTextureSlot(int id){return id==1?0:-1;}
HELPER
DRAW
int main(void){
 SDL_Init(SDL_INIT_VIDEO);SDL_Window*w=SDL_CreateWindow("test",1920,1080,SDL_WINDOW_HIDDEN);sRenderer=SDL_CreateRenderer(w,"software");
 uint32_t pixels[320*180];for(int y=0;y<180;y++)for(int x=0;x<320;x++)pixels[y*320+x]=0xff000000u|((y+20)<<8)|(x%200+20);
 sTex[0]=SDL_CreateTexture(sRenderer,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STATIC,320,180);SDL_UpdateTexture(sTex[0],0,pixels,320*4);
 HostVertex v[6]={{0,0,0,0,0xffffffff},{1920,0,1,0,0xffffffff},{0,1080,0,1,0xffffffff},{1920,0,1,0,0xffffffff},{0,1080,0,1,0xffffffff},{1920,1080,1,1,0xffffffff}};
 SDL_SetRenderDrawColor(sRenderer,0,0,0,255);SDL_RenderClear(sRenderer);H_gfx_draw(1,v,6);
 SDL_Surface*out=SDL_RenderReadPixels(sRenderer,0);SDL_Surface*rgba=SDL_ConvertSurface(out,SDL_PIXELFORMAT_ABGR8888);
 for(int y=0;y<1080;y++)for(int x=0;x<1920;x++)if(*(uint32_t*)((char*)rgba->pixels+y*rgba->pitch+x*4)!=pixels[(y/6)*320+x/6]){fprintf(stderr,"render mismatch %d,%d\n",x,y);return 1;}
 SDL_DestroySurface(rgba);SDL_DestroySurface(out);
 for(int i=0;i<6;i++)v[i].u=1-v[i].u+2;
 H_gfx_draw(1,v,6);out=SDL_RenderReadPixels(sRenderer,0);rgba=SDL_ConvertSurface(out,SDL_PIXELFORMAT_ABGR8888);
 for(int y=0;y<1080;y++)for(int x=0;x<1920;x++)if(*(uint32_t*)((char*)rgba->pixels+y*rgba->pitch+x*4)!=pixels[(y/6)*320+319-x/6])return 2;
 v[1].argb=0xff808080;if(HostDrawSdlQuad(sTex[0],v,6,2))return 3;
 if(HostDrawSdlQuad(sTex[0],v,3,2))return 4;
 puts("PASS production Windows64 quad: full1080p patterned texture matches nearest copy");return 0;
}
'''
helper=ROOT/'port/windows64/sdl_quad.h';code=code.replace('HELPER','#include "sdl_quad.h"' if helper.exists() else '').replace('DRAW',draw)
with tempfile.TemporaryDirectory(prefix='scene-host-') as tmp:
 d=Path(tmp);(d/'test.c').write_text(code);sdl=a.sdl/'x86_64-w64-mingw32';shutil.copy2(sdl/'bin/SDL3.dll',d/'SDL3.dll')
 subprocess.run([a.cc,'-O2',str(d/'test.c'),'-I'+str(sdl/'include'),'-I'+str(ROOT/'port/ps5/shared'),'-I'+str(ROOT/'port/windows64'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(d/'test.exe')],check=True)
 env=dict(os.environ,SDL_VIDEO_DRIVER='dummy');subprocess.run([str(d/'test.exe')],env=env,check=True,creationflags=0x08004000)
