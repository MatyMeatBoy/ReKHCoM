"""Exercise production DXGI/VRR selection with mocked device capabilities (no display)."""
import shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'port/src/port_main.c').read_text(encoding='utf-8')
start=s.index('static int VrrPresentEligible(void) {');end=s.index('static void ApplyWindowMode(void)',start)
production=s[start:end]
preamble=r"""
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#define _WIN32 1
#define SDL_PROP_RENDERER_D3D11_SWAPCHAIN_POINTER "11"
#define SDL_PROP_RENDERER_D3D12_SWAPCHAIN_POINTER "12"
#define DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING 2048
#define SUCCEEDED(v) ((v)>=0)
typedef int SDL_PropertiesID;
typedef struct { int unused; } IDXGISwapChain;
typedef struct { int Windowed; unsigned Flags; } DXGI_SWAP_CHAIN_DESC;
static struct { int vsync; } gPortConfig;
static void* sRenderer;
static IDXGISwapChain chain;
static int api,windowed,flags,queryOk,failZero,calls[4],count;
static SDL_PropertiesID SDL_GetRendererProperties(void* p) { (void)p; return 1; }
static void* SDL_GetPointerProperty(SDL_PropertiesID p,const char* key,void* fallback) {
 (void)p; return ((api==11 && key[0]=='1' && key[1]=='1') || (api==12 && key[1]=='2'))?&chain:fallback;
}
static int IDXGISwapChain_GetDesc(IDXGISwapChain* p,DXGI_SWAP_CHAIN_DESC* d) {
 (void)p;d->Windowed=windowed;d->Flags=(unsigned)flags;return queryOk?0:-1;
}
static int SDL_SetRenderVSync(void* p,int value) { (void)p;calls[count++]=value;return !(failZero && value==0); }
static void PortLog(const char* f,...) { (void)f; }
static const char* SDL_getenv(const char* n) { (void)n; return 0; } /* KHCOM_FAST (test override) unset */
"""
cases=r"""
static void check(int requested,int driver,int full,int capable,int query,int fail,int expected,int retry) {
 gPortConfig.vsync=requested;api=driver;windowed=!full;flags=capable?2048:0;queryOk=query;failZero=fail;count=0;
 ApplyVideoSync(1);assert(count==(retry?2:1));assert(calls[count-1]==expected);
}
int main(void) {
 check(0,0,0,0,1,0,0,0);check(1,11,0,1,1,0,1,0);
 check(2,11,0,1,1,0,0,0);check(2,12,0,1,1,0,0,0);
 check(2,11,0,0,1,0,1,0);check(2,11,1,1,1,0,1,0);
 check(2,11,0,1,0,0,1,0);check(2,0,0,1,1,0,1,0);
 check(2,11,0,1,1,1,1,1);
 check(2,11,0,1,1,0,0,0);count=0;ApplyVideoSync(0);assert(count==0);
 windowed=0;count=0;ApplyVideoSync(0);assert(count==1 && calls[0]==1);
 return 0;
}
"""
compiler=shutil.which('cc')
if not compiler:
 roots=[ROOT.parent/'toolchain',Path('D:/01Claude/ReKHCoM-build/toolchain')]
 candidates=[p for root in roots for p in root.glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe')]
 compiler=str(candidates[0]) if candidates else None
if not compiler:raise SystemExit('C compiler required')
with tempfile.TemporaryDirectory(prefix='rekhcom-sync-') as d:
 d=Path(d);src=d/'sync.c';exe=d/'sync.exe';src.write_text(preamble+production+cases)
 subprocess.run([compiler,'-std=c11',str(src),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS: 11 production VRR selection/fallback cases')
