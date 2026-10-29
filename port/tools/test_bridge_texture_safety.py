"""Complete shared SDL shim: real allocation failures and texture API contracts."""
from pathlib import Path
import shutil, subprocess, tempfile
import test_bridge_gpu_targets as fixture

ROOT = Path(__file__).resolve().parents[2]
TEST = r'''
int main(int argc,char**argv) {
 SDL_Renderer*r=SDL_CreateRenderer(NULL,NULL);
 unsigned char rgba[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
 int which=atoi(argv[1]);
 failCountdown=which==1?2:1;
 SDL_Texture*missing=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,2,2);
 assert(!missing);failCountdown=0;
 switch(which) {
 case 0: case 1: assert(!SDL_SetTextureScaleMode(missing,SDL_SCALEMODE_NEAREST));break;
 case 2: assert(!SDL_SetTextureBlendMode(missing,SDL_BLENDMODE_BLEND));break;
 case 3: assert(!SDL_SetTextureColorMod(missing,1,2,3));break;
 case 4: assert(!SDL_SetTextureAlphaMod(missing,1));break;
 case 5: assert(!SDL_UpdateTexture(missing,NULL,rgba,8));break;
 case 6: assert(!SDL_RenderTexture(r,missing,NULL,NULL));break;
 case 7: assert(!SDL_CreateTextureFromSurface(r,NULL));break;
 case 8: {
  SDL_Texture*t=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,2,2);assert(t);
  assert(SDL_UpdateTexture(t,NULL,rgba,8));assert(!memcmp(((Tex*)t)->rgba,rgba,16));
  assert(!SDL_UpdateTexture(t,NULL,NULL,8));assert(!SDL_UpdateTexture(t,NULL,rgba,0));
  assert(!SDL_UpdateTexture(t,NULL,rgba,4));assert(!SDL_UpdateTexture(t,NULL,rgba,-8));
  assert(!memcmp(((Tex*)t)->rgba,rgba,16));
  SDL_Rect outside={-200,-200,2,2};assert(SDL_UpdateTexture(t,&outside,rgba,8));
  assert(!memcmp(((Tex*)t)->rgba,rgba,16));
  SDL_Rect clipped={-1,-1,2,2};assert(SDL_UpdateTexture(t,&clipped,rgba+4,8));
  assert(!memcmp(((Tex*)t)->rgba,rgba+4,4));
  assert(!memcmp(((Tex*)t)->rgba+4,rgba+4,12));
  assert(SDL_SetTextureColorMod(t,10,20,30)&&SDL_SetTextureAlphaMod(t,40));
  assert(((Tex*)t)->cr==10&&((Tex*)t)->cg==20&&((Tex*)t)->cb==30&&((Tex*)t)->ca==40);
  assert(SDL_SetTextureScaleMode(t,SDL_SCALEMODE_NEAREST)&&SDL_SetTextureBlendMode(t,SDL_BLENDMODE_NONE));
  assert(SDL_RenderTexture(r,t,NULL,NULL));SDL_DestroyTexture(t);break;
 }
 case 9: failCountdown=1;assert(!SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,0x40000001,4));assert(failCountdown==1);break;
 case 10: {
  SDL_Surface s={0};s.w=s.h=2;s.format=SDL_PIXELFORMAT_RGBA32;s.pixels=rgba;s.pitch=4;
  assert(!SDL_CreateTextureFromSurface(r,&s));break;
 }
 }
 SDL_DestroyTexture(NULL);assert(liveAllocations==0);return 0;
}
'''

def run(test, cases):
 tc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin'))
 with tempfile.TemporaryDirectory(prefix='bridge-texture-safety-') as temp:
  out=Path(temp);(out/'SDL3').mkdir()
  shutil.copyfile(ROOT/'port/ps5/guest/include/SDL3/SDL.h',out/'SDL3/SDL.h')
  shutil.copyfile(ROOT/'port/ps5/guest/include/bridge_calls.h',out/'bridge_calls.h')
  native=(ROOT/'port/windows64/windows64_host.c').read_text()
  a=native.index('static void H_gfx_draw(');b=native.index('static void H_notify(',a)
  (out/'host.c').write_text(fixture.HOST.replace('/* REAL_DRAW */',native[a:b]))
  prefix='#include <stdlib.h>\nstatic int failCountdown,liveAllocations;\nstatic void*FaultCalloc(size_t n,size_t size){if(failCountdown&&!--failCountdown)return NULL;void*p=calloc(n,size);if(p)liveAllocations++;return p;}\nstatic void FaultFree(void*p){if(p)liveAllocations--;free(p);}\n#define calloc FaultCalloc\n#define free FaultFree\n'
  shim='#include "'+(ROOT/'port/ps5/guest/src/sdl_shim.c').as_posix()+'"\n'
  (out/'guest.c').write_text(prefix+shim+'#define main OldTargetMain\n'+fixture.GUEST+'\n#undef main\n'+test)
  exe=out/'test.exe'
  for arch in ('i686','x86_64'):
   for android in (False,True):
    subprocess.run([str(tc/(arch+'-w64-mingw32-clang.exe')),'-std=gnu11',*(['-D__ANDROID__'] if android else []),'-I'+str(out),'-I'+str(ROOT/'port/ps5/shared'),str(out/'host.c'),str(out/'guest.c'),'-o',str(exe)],check=True)
    failures=[]
    for case in cases:
     p=subprocess.run([str(exe),str(case)],cwd=out,capture_output=True,text=True,timeout=10,creationflags=0x08004000)
     if p.returncode:failures.append(dict(case=case,exit=p.returncode,stderr=p.stderr[-500:]))
    assert not failures,(arch,android,failures)
def main():
 run(TEST,range(11))
 print('PASS complete shim: two allocation failures, null texture APIs/surface, valid pixel/modulation/draw, clipping, invalid pitch and oversized dimensions')

if __name__=='__main__':main()
