"""Real host slot/target consumer plus the complete SDL shim producer, without GPU/ROM."""
from pathlib import Path
import shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]

def main():
 cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
 with tempfile.TemporaryDirectory(prefix='rekhcom-gpu-target-') as d:
  d=Path(d);(d/'SDL3').mkdir();shutil.copy2(ROOT/'port/ps5/guest/include/SDL3/SDL.h',d/'SDL3/SDL.h');shutil.copy2(ROOT/'port/ps5/guest/include/bridge_calls.h',d/'bridge_calls.h')
  native=(ROOT/'port/windows64/windows64_host.c').read_text(encoding='utf8')
  a=native.index('static void H_gfx_draw(');b=native.index('static void H_notify(',a)
  (d/'host.c').write_text(HOST.replace('/* REAL_DRAW */',native[a:b]))
  (d/'guest.c').write_text('#include "'+(ROOT/'port/ps5/guest/src/sdl_shim.c').as_posix()+'"\n'+GUEST)
  e=d/'test.exe'
  subprocess.run([str(cc),'-std=gnu11','-ffunction-sections','-fdata-sections','-I'+str(d),'-I'+str(ROOT/'port/ps5/shared'),str(d/'host.c'),str(d/'guest.c'),'-Wl,--gc-sections','-o',str(e)],check=True)
  for old in ['0','1']:
   subprocess.run([str(e),old],check=True,timeout=10,creationflags=0x08004000)
  subprocess.run([str(e),'0','software'],check=True,timeout=10,creationflags=0x08004000)
  for case in ['destroyfail','hostfreefail','resetfail']:
   subprocess.run([str(e),'0',case],check=True,timeout=10,creationflags=0x08004000)
  subprocess.run([str(cc),'-std=gnu11','-D__ANDROID__','-I'+str(d),'-I'+str(ROOT/'port/ps5/shared'),str(d/'host.c'),str(d/'guest.c'),'-o',str(e)],check=True)
  subprocess.run([str(e),'0'],check=True,timeout=10,creationflags=0x08004000)
 print('PASS: actual shim/host targets, legacy fallback, budget, bad bind, premultiplied clear, state and epoch')

HOST=r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
typedef long long Sint64;
typedef unsigned SDL_BlendMode;
typedef struct {int w,h,alive,access;} SDL_Texture;
static void* sRenderer=(void*)1;
static int sBlend=1;
static uint32_t sRendererGeneration;
static SDL_Texture* bound;
int oldHost,failAt,allocations,failBind,software,failUpload;
unsigned textureBlend,drawBlend;
typedef struct {float x,y,u,v;uint32_t argb;} HostVertex;
typedef struct {struct {float x,y;}position,tex_coord;struct {float r,g,b,a;}color;} SDL_Vertex;
unsigned clearColor;
int hostDraws,hostClears;
int hostClip[4];
#define SDL_PIXELFORMAT_ABGR8888 1
#define SDL_TEXTUREACCESS_STATIC 0
#define SDL_TEXTUREACCESS_TARGET 2
#define SDL_SCALEMODE_NEAREST 0
#define SDL_SCALEMODE_LINEAR 1
#define SDL_BLENDMODE_NONE 0u
#define SDL_BLENDMODE_BLEND 1u
#define SDL_BLENDMODE_ADD 2u
#define SDL_BLENDMODE_BLEND_PREMULTIPLIED 16u
#define SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER "max"
static SDL_Texture* SDL_CreateTexture(void*r,int format,int access,int w,int h) {
 (void)r;(void)format;allocations++;if(failAt && allocations==failAt)return NULL;
 SDL_Texture*t=malloc(sizeof(*t));*t=(SDL_Texture){w,h,1,access};return t;
}
static bool SDL_UpdateTexture(SDL_Texture*t,void*rect,const void*p,int pitch){(void)t;(void)rect;(void)p;(void)pitch;return !failUpload;}
static void SDL_DestroyTexture(SDL_Texture*t){assert(t->alive);t->alive=0;free(t);}
static bool SDL_SetTextureBlendMode(SDL_Texture*t,unsigned mode){(void)t;textureBlend=mode;return true;}
static void SDL_SetTextureScaleMode(SDL_Texture*t,int mode){(void)t;(void)mode;}
static void SDL_SetRenderDrawBlendMode(void*r,unsigned mode){(void)r;drawBlend=mode;}
static void SDL_RenderGeometry(void*r,SDL_Texture*t,const SDL_Vertex*v,int n,void*i,int ni){(void)r;(void)t;(void)v;(void)n;(void)i;(void)ni;hostDraws++;}
static int SDL_GetRendererProperties(void*r){(void)r;return 1;}
static Sint64 SDL_GetNumberProperty(int p,const char*n,Sint64 d){(void)p;(void)n;(void)d;return 8192;}
static const char* SDL_GetRendererName(void*r){(void)r;return software?"software":"direct3d";}
static SDL_Texture* SDL_GetRenderTarget(void*r){(void)r;return bound;}
static bool SDL_SetRenderTarget(void*r,SDL_Texture*t){(void)r;if(failBind)return false;bound=t;return true;}
static void SDL_SetRenderDrawColor(void*r,unsigned a,unsigned b,unsigned c,unsigned d){(void)r;clearColor=(d<<24)|(a<<16)|(b<<8)|c;}
static bool SDL_RenderClear(void*r){(void)r;hostClears++;return true;}
#define HOST_GPU_TARGETS 1
#include "host_texture_slots.h"
#include "host_gpu_targets.h"
int h_thread(int op,uint32_t a,uint32_t b,uint32_t c,uint32_t d){return oldHost?(op==0):HostGpuCommand(op,a,b,c,d);}
int h_getenv(const char*n,char*out,int cap){if(!strcmp(n,"KHCOM_RENDERER_GENERATION"))return snprintf(out,cap,"bridge-textures-v1:%u",sRendererGeneration);return oldHost?-1:HostGpuQuery(n,out,cap);}
void h_gfx_size(int*w,int*h){*w=bound?bound->w:800;*h=bound?bound->h:600;}
void h_gfx_begin(uint32_t clear){(void)clear;memset(hostClip,0,sizeof(hostClip));}
void h_gfx_clip(int x,int y,int w,int h){hostClip[0]=x;hostClip[1]=y;hostClip[2]=w;hostClip[3]=h;}
void h_gfx_blend(int b){sBlend=b;}
/* REAL_DRAW */
void h_gfx_draw(int id,const void*v,int n){H_gfx_draw(id,v,n);}
int h_tex_create(const void*p,int w,int h,int smooth){return H_tex_create(p,w,h,smooth);}
void h_tex_free(int id){H_tex_free(id);}
void h_tex_update(int id,const void*p){H_tex_update(id,p);}
void resetHost(void){HostTextureReset();}
int hostBound(void){return bound!=NULL;}
size_t hostBytes(void){return sTargetBytes;}
int hostBlend(void){return sBlend;}
int badBind(int id){return HostGpuCommand(BRIDGE_GPU_BIND,(uint32_t)id,0,0,0);}
int hostBudget(void){return HOST_GPU_TARGET_BUDGET;}
int hostLive(void){int n=0;for(int i=0;i<HOST_TEXTURE_SLOTS;i++)n+=sTex[i]!=NULL;return n;}
'''
GUEST=r'''
#include <assert.h>
extern int oldHost,failAt,allocations,failBind,software,failUpload,hostClip[4];
extern unsigned clearColor,textureBlend,drawBlend;
extern int hostDraws,hostClears;
extern void resetHost(void);
extern int hostBound(void),hostBlend(void),badBind(int),hostBudget(void);
extern int hostLive(void);
extern size_t hostBytes(void);
int gPortMmpx;
uint64_t h_ticks_us(void){return 0;}
void h_sleep_us(uint32_t x){(void)x;}
int h_mkdir(const char*p){(void)p;return 0;}
int h_stat(const char*p){(void)p;return 0;}
void h_log(const char*p){(void)p;}
void PortPs5LockSettings(void){}
void h_notify(const char*p){(void)p;}
void h_gfx_end(void){}
int h_audio_open(int x){return x;}
void h_audio_clear(void){}
void h_audio_ratio(double x){(void)x;}
void h_audio_push(const int16_t*p,uint32_t n){(void)p;(void)n;}
uint32_t h_audio_queued(void){return 0;}
int h_quit_requested(void){return 0;}
int h_pad_poll(HostPad*p,int n){(void)p;(void)n;return 0;}
int h_touch_poll(HostTouch*p,int n){(void)p;(void)n;return 0;}
int h_app_state(void){return 0;}
int main(int argc,char**argv) {
 oldHost=argc>1?atoi(argv[1]):0;software=argc>2 && !strcmp(argv[2],"software");
 SDL_Renderer*r=SDL_CreateRenderer(NULL,NULL);
 /* Traffic generated by old guests remains unchanged on either host family. */
 unsigned char legacy[4]={0};int legacyId=h_tex_create(legacy,1,1,0);
 assert(legacyId>=0);h_gfx_blend(1);assert(hostBlend()==1);h_tex_free(legacyId);
 /* CPU texture readiness must reach the real host slot consumer, including
    direct legacy/software hosts that do not expose GPU target commands. */
 unsigned char probeRgba[4]={3,7,11,99};
 SDL_Surface*probeSurface=SDL_CreateSurfaceFrom(1,1,SDL_PIXELFORMAT_RGBA32,probeRgba,4);
 SDL_Texture*probe=SDL_CreateTextureFromSurface(r,probeSurface);SDL_DestroySurface(probeSurface);
 assert(probe&&((Tex*)probe)->id[1]<0&&hostLive()==0);
 int before=allocations;failAt=before+1;
 assert(!PortBridgeTextureReady(probe)&&hostLive()==0&&allocations==before+1);
 assert(!memcmp(((Tex*)probe)->rgba,probeRgba,4)&&((Tex*)probe)->id[1]<0);
 failAt=0;failUpload=1;assert(!PortBridgeTextureReady(probe)&&hostLive()==0);
 failUpload=0;assert(PortBridgeTextureReady(probe)&&hostLive()==1);
 int smoothId=((Tex*)probe)->id[1];before=allocations;
 assert(PortBridgeTextureReady(probe)&&allocations==before);
 SDL_SetTextureScaleMode(probe,SDL_SCALEMODE_NEAREST);
 assert(PortBridgeTextureReady(probe)&&hostLive()==2);
 before=allocations;r->outputBlocked=true;
 assert(!PortBridgeTextureReady(probe)&&allocations==before);r->outputBlocked=false;
 resetHost();PortBridgeRefreshTextureEpoch();assert(hostLive()==0);
 assert(PortBridgeTextureReady(probe)&&hostLive()==1&&((Tex*)probe)->id[1]<0);
 SDL_SetTextureScaleMode(probe,SDL_SCALEMODE_LINEAR);
 assert(PortBridgeTextureReady(probe)&&((Tex*)probe)->id[1]!=smoothId&&hostLive()==2);
 SDL_DestroyTexture(probe);assert(hostLive()==0);
 if(software){assert(!GpuTargets());assert(!strcmp(SDL_GetRendererName(r),"software"));return 0;}
 if(oldHost){assert(!SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,20,20));assert(!strcmp(SDL_GetRendererName(r),"bridge-unknown"));return 0;}
 assert(!strcmp(SDL_GetRendererName(r),"direct3d"));
 SDL_Rect view={7,8,90,80},clip={1,2,30,40},v;
 SDL_SetRenderViewport(r,&view);SDL_SetRenderClipRect(r,&clip);SDL_SetRenderScale(r,2,3);
 SDL_Texture*t=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,100,50);
 assert(t && !((Tex*)t)->rgba && hostBytes()==20000);
 if(argc>2 && !software) {
  assert(SDL_SetRenderTarget(r,t));int held=((Tex*)t)->id[0];failBind=1;
  if(!strcmp(argv[2],"destroyfail")) {
   SDL_DestroyTexture(t);
   assert(SDL_GetRenderTarget(r)==t && hostBound() && hostBytes()==20000);
   int priorDraws=hostDraws,priorClears=hostClears;
   assert(!SDL_RenderClear(r));assert(!SDL_RenderTexture(r,t,NULL,NULL));assert(!SDL_RenderPresent(r));
   assert(hostDraws==priorDraws && hostClears==priorClears);
   failBind=0;assert(SDL_SetRenderTarget(r,NULL));
   assert(!SDL_GetRenderTarget(r) && !hostBound() && !hostBytes());return 0;
  }
  if(!strcmp(argv[2],"hostfreefail"))h_tex_free(held);
  else resetHost();
  assert(hostBound() && hostBytes()==20000);
  failBind=0;assert(SDL_SetRenderTarget(r,NULL));
  assert(!hostBound() && !hostBytes());
  if(!strcmp(argv[2],"resetfail"))PortBridgeRefreshTextureEpoch();
  SDL_DestroyTexture(t);return 0;
 }
 assert(SDL_SetRenderTarget(r,t) && hostBound());
 assert(!r->hasView && !r->hasClip && r->sx==1 && r->sy==1);
 int id=((Tex*)t)->id[0];assert(!badBind(123) && hostBound());
 failBind=1;assert(!SDL_SetRenderTarget(r,NULL) && SDL_GetRenderTarget(r)==t && hostBound());failBind=0;assert(SDL_SetRenderTarget(r,t));
 SDL_SetRenderDrawColor(r,0,0,0,0);assert(SDL_RenderClear(r));assert(clearColor==0);
 SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND_PREMULTIPLIED);SetBlend(r,((Tex*)t)->blend);assert(hostBlend()==16);
 SDL_SetTextureScaleMode(t,SDL_SCALEMODE_NEAREST);assert(HostTex((Tex*)t,false)==id);assert(HostTex((Tex*)t,true)==id);
 assert(SDL_SetRenderTarget(r,NULL));SDL_GetRenderViewport(r,&v);
 assert(!memcmp(&v,&view,sizeof(v)) && r->hasClip && r->sx==2 && r->sy==3);
 assert(hostClip[0]==8 && hostClip[1]==10);
 SDL_FRect sample={0,0,100,50};SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND_PREMULTIPLIED);
 assert(SDL_RenderTexture(r,t,&sample,&sample));assert(textureBlend==16 && drawBlend==16);
 assert(!SDL_UpdateTexture(t,NULL,"",0));
 failAt=allocations+2;
 SDL_Texture*a=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,20,20);
 SDL_Texture*b=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,20,20);
 assert(a && !b);SDL_DestroyTexture(a);failAt=0;
 assert(!SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,8192,8192));
 #ifdef __ANDROID__
 assert(hostBudget()==12*1024*1024);
#else
 assert(hostBudget()==32*1024*1024);
#endif
 /* Enforce the live cumulative GPU budget, including a partially allocated chain. */
 int handles[8],count=0;
 while(count<8) {int x=h_thread(BRIDGE_GPU_CREATE,1024,1024,0,0);if(x<0)break;handles[count++]=x;}
 assert(count>0 && count<8 && hostBytes()<=(size_t)hostBudget());
 for(int i=0;i<count;i++)h_tex_free(handles[i]);assert(hostBytes()==20000);
 failAt=allocations+3;
 a=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,20,20);
 b=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,20,20);
 SDL_Texture*c=SDL_CreateTexture(r,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,20,20);
 assert(a&&b&&!c);SDL_DestroyTexture(a);SDL_DestroyTexture(b);failAt=0;assert(hostBytes()==20000);
 assert(SDL_SetRenderTarget(r,t));resetHost();assert(!hostBound() && !hostBytes());
 PortBridgeRefreshTextureEpoch();assert(SDL_SetRenderTarget(r,t));assert(((Tex*)t)->id[0]!=id);
 assert(!badBind(id) && hostBound());SDL_DestroyTexture(t);assert(!hostBound()&&!hostBytes());
 /* Old guest's existing boolean blend / static texture path remains valid on new host. */
 unsigned char rgba[4]={1,2,3,4};int ordinary=h_tex_create(rgba,1,1,0);assert(ordinary>=0);h_gfx_blend(1);assert(hostBlend()==1);h_tex_free(ordinary);
 return 0;
}
'''
if __name__=='__main__':main()
