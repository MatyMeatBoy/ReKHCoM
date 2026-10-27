"""Production bridge texture handles/guest upload epoch, synthetic GPU faults."""
from pathlib import Path
import argparse
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]

def function(s,signature):
 a=s.index(signature);b=s.index('{',a);level=1;i=b+1
 while level:
  level+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]+'\n'

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--baseline',action='store_true');args=parser.parse_args()
 common=r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
typedef struct {int w,h,alive,pixel;} SDL_Texture;
typedef unsigned char Uint8;
typedef int SDL_PixelFormat;
typedef int SDL_ScaleMode;
typedef int SDL_BlendMode;
#define SDL_PIXELFORMAT_ABGR8888 0
#define SDL_TEXTUREACCESS_STATIC 0
#define SDL_BLENDMODE_BLEND 1
static void*sRenderer;
static uint32_t sRendererGeneration;
static int created,destroyed,updated,failUpload,queryMode,queries,draws;
static SDL_Texture pool[100];
static SDL_Texture* SDL_CreateTexture(void*r,int f,int a,int w,int h){SDL_Texture*t=&pool[created++];*t=(SDL_Texture){w,h,1,0};return t;}
static bool SDL_UpdateTexture(SDL_Texture*t,void*r,const void*rgba,int pitch){assert(t&&t->alive);updated++;if(failUpload)return false;t->pixel=*(const unsigned char*)rgba;return true;}
static void SDL_DestroyTexture(SDL_Texture*t){assert(t&&t->alive);t->alive=0;destroyed++;}
static void SDL_SetTextureBlendMode(SDL_Texture*t,int m){}
'''
 if args.baseline:
  old=subprocess.check_output(['git','show','8a6c9016c:port/linux64/linux64_host.c'],cwd=ROOT,text=True)
  a=old.index('static int H_tex_create(');b=old.index('static void H_gfx_clip(',a)
  code=common+'static SDL_Texture*sTex[4096];static int sTexW[4096],sTexSmooth[4096];\n'+old[a:b]+r'''
int main(void){unsigned char rgba[4]={7};int old=H_tex_create(rgba,1,1,0);H_tex_free(old);int newer=H_tex_create(rgba,1,1,0);int before=destroyed;H_tex_free(old);return destroyed!=before?1:0;}
'''
 else:
  host=(ROOT/'port/ps5/shared/host_texture_slots.h').read_text(encoding='utf-8')
  guest=(ROOT/'port/ps5/guest/src/sdl_shim.c').read_text(encoding='utf-8')
  epoch=guest[guest.index('static uint32_t sTextureEpoch;'):guest.index('typedef struct Tex {')]
  tex=guest[guest.index('typedef struct Tex {'):guest.index('static void ToRgba(')]
  drawing=r'''
#include <math.h>
typedef struct {float x,y,u,v;uint32_t argb;}HostVertex;
typedef struct {struct{float x,y;}position,tex_coord;struct{float r,g,b,a;}color;}SDL_Vertex;
#define SDL_SCALEMODE_NEAREST 0
#define SDL_SCALEMODE_LINEAR 1
#define SDL_BLENDMODE_NONE 0
static int sBlend=1;
static void SDL_SetTextureScaleMode(SDL_Texture*t,int m){}
static void SDL_SetRenderDrawBlendMode(void*r,int m){}
static void SDL_RenderGeometry(void*r,SDL_Texture*t,const SDL_Vertex*v,int n,const int*i,int ni){assert(!t||t->alive);draws++;}
'''
  linux=(ROOT/'port/linux64/linux64_host.c').read_text(encoding='utf-8')
  drawing+=function(linux,'static void H_gfx_draw(')
  bridge=r'''
static int h_getenv(const char*name,char*out,int cap){queries++;assert(!strcmp(name,"KHCOM_RENDERER_GENERATION"));if(queryMode==1)return -1;if(queryMode==2){strcpy(out,"invalid");return 7;}return snprintf(out,cap,"bridge-textures-v1:%u",sRendererGeneration);}
static int h_tex_create(const void*p,int w,int h,int s){return H_tex_create(p,w,h,s);}
static void h_tex_free(int id){H_tex_free(id);}
'''
  movie=(ROOT/'port/src/port_hdmovie.c').read_text(encoding='utf-8')
  movie=movie[movie.rindex('static int PlayFile(const char* name) {'):]
  android=(ROOT/'port/android64/host/android64_host.c').read_text(encoding='utf-8')
  movie_bridge=r'''
#define SDL_snprintf snprintf
static int sQuit,sStartAt;
static struct {int hdSubtitles;}gPortConfig;
static void MoviePath(const char*n,char*p,int cap){snprintf(p,cap,"%s",n);}
static void MovieSubtitlePath(const char*n,const char*l,char*p,int cap){snprintf(p,cap,"%s.%s.txt",n,l);}
static void SubsOrder(const char**o){o[0]="en";o[1]="es";}
static const char*PortConfigPath(const char*p){return p;}
static void SDL_strlcpy(char*p,const char*s,int cap){snprintf(p,cap,"%s",s);}
static void(*resetCallback)(void);
static void RendererDeviceReset(void){HostTextureReset();}
static void HdMovieAndroidSetRendererResetCallback(void(*cb)(void)){resetCallback=cb;}
static int HdMovieAndroidPlay(void*r,const char*p,const char*a,const char*b,const char*c){assert(resetCallback);resetCallback();return 0;}
'''
  movie_bridge+=function(android,'static int H_hd_movie(')
  movie_bridge+='static int h_hd_movie(const char*m,const char*a,const char*b,const char*f){return H_hd_movie(m,a,b,f);}'
  movie_bridge+=function(movie,'static int PlayFile(')
  create=function(guest,'SDL_Texture* SDL_CreateTexture(').replace('SDL_CreateTexture(', 'GuestCreateTexture(',1)
  create_stubs='typedef void SDL_Renderer;typedef int SDL_TextureAccess;\n#define SDL_TEXTUREACCESS_STREAMING 1\n#define SetError(...) ((void)0)\n'
  code=common+host+drawing+bridge+epoch+tex+create_stubs+create+movie_bridge+function(guest,'static int HostTex(')+function(guest,'void SDL_DestroyTexture(').replace('SDL_DestroyTexture(', 'GuestDestroyTexture(',1)+r'''
static Tex* make(unsigned char pixel){Tex*t=calloc(1,sizeof(*t));t->pub.w=t->pub.h=1;t->id[0]=t->id[1]=-1;t->epoch=sTextureEpoch;t->rgba=malloc(4);memset(t->rgba,pixel,4);return t;}
int main(void){
 Tex*t=make(11);int old=HostTex(t,false),oldSmooth=HostTex(t,true);assert(old>=0&&oldSmooth>=0);
 HostTextureReset();int before=updated,d=destroyed;
 unsigned char pixel[4]={55};int unrelated=H_tex_create(pixel,1,1,0);assert(unrelated!=old);
 H_tex_update(old,pixel);H_tex_update(oldSmooth,pixel);H_tex_free(old);H_tex_free(oldSmooth);
 assert(updated==before+1&&destroyed==d&&HostTextureSlot(unrelated)>=0);
 HostVertex vertices[3]={0};before=draws;H_gfx_draw(old,vertices,3);assert(draws==before);
 H_gfx_draw(unrelated,vertices,3);assert(draws==before+1);
 queryMode=1;RefreshTextureEpoch();assert(sTextureEpoch==0);queryMode=2;RefreshTextureEpoch();assert(sTextureEpoch==0);
 queryMode=0;RefreshTextureEpoch();assert(sTextureEpoch==1);
 int fresh=HostTex(t,false);assert(fresh!=old&&t->id[1]==-1&&sTex[HostTextureSlot(fresh)]->pixel==11);
 int freshSmooth=HostTex(t,true);assert(freshSmooth!=oldSmooth&&sTex[HostTextureSlot(freshSmooth)]->pixel==11);
 before=created;int queryBefore=queries;assert(HostTex(t,false)==fresh&&created==before&&queries==queryBefore);
 HostTextureReset();RefreshTextureEpoch();unrelated=H_tex_create(pixel,1,1,0);d=destroyed;
 GuestDestroyTexture(&t->pub);assert(destroyed==d&&HostTextureSlot(unrelated)>=0);
 failUpload=1;before=destroyed;assert(H_tex_create(pixel,1,1,0)==-1&&destroyed==before+1);failUpload=0;
 H_tex_update(-1,pixel);H_tex_free(-1);assert(HostTextureSlot(0x7fffffff)==-1);
 /* Movie reset must synchronize before any new guest allocation/upload, even
    before the next input drain. No host texture may become ownerless. */
 HostTextureReset();RefreshTextureEpoch();
 assert(!PlayFile("reset.mp4"));
 Tex*post=(Tex*)GuestCreateTexture(NULL,0,0,1,1);
 assert(post);int postId=HostTex(post,false);assert(postId>=0);
 RefreshTextureEpoch();GuestDestroyTexture(&post->pub);
 int live=0;for(int i=0;i<HOST_TEXTURE_SLOTS;i++)live+=sTex[i]!=NULL;
 assert(live==0);
 HostTextureReset();sTexSerial[0]=HOST_TEXTURE_MAX_SERIAL;int next=H_tex_create(pixel,1,1,0);assert((next&4095)==1);
 sRendererGeneration=0xffffffffu;HostTextureReset();assert(H_tex_create(pixel,1,1,0)==-1);
 return 0;
}
'''
 cc=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
 with tempfile.TemporaryDirectory(prefix='rekhcom-bridge-epoch-') as td:
  src=Path(td)/'test.c';exe=Path(td)/'test.exe';src.write_text(code,encoding='utf-8')
  for target in (['PORT_ANDROID64'] if args.baseline else ['PORT_ANDROID64','PORT_LINUX64_GUEST']):
   subprocess.run([str(cc),'-D'+target,str(src),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
