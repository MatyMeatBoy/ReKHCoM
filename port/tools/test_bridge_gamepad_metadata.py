"""Actual optional host producer and shim event-refresh consumer; no GPU/ROM/input devices."""
from pathlib import Path
import subprocess,tempfile,shutil
ROOT=Path(__file__).resolve().parents[2]
CC=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
CODE=r'''
#include <assert.h>
#include <SDL3/SDL.h>
#include "bridge_calls.h"
/* ACTUAL */
static int kind=6, connected=1, mode, queries;
static SDL_Gamepad* sPad=(SDL_Gamepad*)(uintptr_t)0x1234;
static SDL_GamepadType HostSDL_GetGamepadType(SDL_Gamepad*p){assert(p==sPad);return (SDL_GamepadType)kind;}
static bool HostSDL_GamepadConnected(SDL_Gamepad*p){assert(p==sPad);return connected;}
#define SDL_GetGamepadType HostSDL_GetGamepadType
#define SDL_GamepadConnected HostSDL_GamepadConnected
#include "host_gamepad_info.h"
#undef SDL_GetGamepadType
#undef SDL_GamepadConnected
static SDL_KeyboardEvent sHostKeys[64];
static unsigned sHostKeyRead, sHostKeyWrite;
static int sKeyboardEventsActive;
static uint32_t sRendererGeneration;
static const char* PortDeviceHostQuery(const char*n){(void)n;return NULL;}
static int HostGpuQuery(const char*n,char*out,int cap){(void)n;(void)out;(void)cap;return -2;}
size_t SDL_strlcpy(char*d,const char*s,size_t n){return snprintf(d,n,"%s",s);}
/* ACTUAL_HOST_ENV */
static const char* malformed;
int h_getenv(const char*n,char*out,int cap){
 if(strcmp(n,"KHCOM_GAMEPAD_INFO_V1"))return -1;
 queries++;
 if(mode==1)return -1;
 if(mode==2)return snprintf(out,cap,"%s",malformed);
 if(mode==3)return cap;
 int a=HostEnv0(n,out,cap); char other[48];
 assert(a==HostEnv1(n,other,sizeof(other))&&!strcmp(out,other));
 assert(a==HostEnv2(n,other,sizeof(other))&&!strcmp(out,other));
 return a;
}
int h_pad_poll(HostPad*p,int n){memset(p,0,n*sizeof(*p));p[0].connected=1;return 1;}
uint64_t h_ticks_us(void){return 0;}
int main(void){
 assert(sizeof(HostPad)==20);
 for(kind=0;kind<SDL_GAMEPAD_TYPE_COUNT;kind++){
  Refresh();SDL_Gamepad*p=SDL_GetGamepadFromID(1);assert(p&&SDL_GamepadConnected(p));
  assert(SDL_GetGamepadType(p)==kind);assert(!SDL_GetGamepadFromID(2));
  int before=queries;
  for(int i=0;i<1000;i++){assert(SDL_GetGamepadFromID(1)==p);assert(SDL_GetGamepadType(p)==kind);assert(SDL_GamepadConnected(p));}
  assert(queries==before); /* Metadata never crosses the bridge at draw time. */
 }
 kind=999;Refresh();assert(SDL_GetGamepadType(SDL_GetGamepadFromID(1))==SDL_GAMEPAD_TYPE_UNKNOWN);
 SDL_Gamepad*p=SDL_GetGamepadFromID(1);connected=0;Refresh();
 assert(!SDL_GetGamepadFromID(1)&&!SDL_GamepadConnected(p)&&SDL_GetGamepadType(p)==SDL_GAMEPAD_TYPE_UNKNOWN);
 int count=99;SDL_JoystickID*ids=SDL_GetGamepads(&count);assert(ids&&count==0);free(ids);
 connected=1;kind=SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO;Refresh();assert(SDL_GetGamepadType(p)==kind);
 sPad=NULL;Refresh();assert(!SDL_GetGamepadFromID(1));sPad=(SDL_Gamepad*)(uintptr_t)0x1234;
 mode=1;Refresh();assert(SDL_GetGamepadType(SDL_GetGamepadFromID(1))==SDL_GAMEPAD_TYPE_STANDARD);
 const char*bad[]={"", "bridge-pad-v2:1:1:6", "bridge-pad-v1:2:1:6", "bridge-pad-v1:1:2:6", "bridge-pad-v1:1:1:", "bridge-pad-v1:1:1:-1", "bridge-pad-v1:1:1:11", "bridge-pad-v1:1:1:99999999999999999999", "bridge-pad-v1:1:1:6tail"};
 mode=2;
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){malformed=bad[i];Refresh();assert(SDL_GetGamepadType(SDL_GetGamepadFromID(1))==SDL_GAMEPAD_TYPE_STANDARD);}
 mode=3;Refresh();assert(SDL_GetGamepadType(SDL_GetGamepadFromID(1))==SDL_GAMEPAD_TYPE_STANDARD);
 char out[48];assert(HostGamepadInfoQuery("OTHER",out,sizeof(out))==-2);
 assert(HostGamepadInfoQuery("KHCOM_GAMEPAD_INFO_V1",out,0)==-1);
 assert(HostGamepadInfoQuery("KHCOM_GAMEPAD_INFO_V1",out,1)==-1);
 assert(HostGamepadInfoQuery("KHCOM_GAMEPAD_INFO_V1",NULL,48)==-1);
 return 0;
}
'''
def main():
 with tempfile.TemporaryDirectory() as td:
  d=Path(td);(d/'SDL3').mkdir();shutil.copy2(ROOT/'port/ps5/guest/include/SDL3/SDL.h',d/'SDL3/SDL.h');shutil.copy2(ROOT/'port/ps5/guest/include/bridge_calls.h',d/'bridge_calls.h')
  p=d/'test.c'
  shim=(ROOT/'port/ps5/guest/src/sdl_shim.c').read_text()
  actual=shim[shim.index('typedef struct Pad {'):shim.index('/* Touch screen and background state')]
  actual+=shim[shim.index('SDL_JoystickID* SDL_GetGamepads('):shim.index('const char* SDL_GetGamepadStringForButton(')]
  host_env=''
  for i,file in enumerate(['port/windows64/windows64_host.c','port/linux64/linux64_host.c','port/android64/host/android64_host.c']):
   source=(ROOT/file).read_text();start=source.index('static int H_getenv(');end=source.index('\n}',start)+2
   host_env+=source[start:end].replace('H_getenv(',f'HostEnv{i}(',1)+'\n'
  p.write_text(CODE.replace('/* ACTUAL */',actual).replace('/* ACTUAL_HOST_ENV */',host_env))
  exe=p.with_suffix('.exe')
  subprocess.run([str(CC),'-std=gnu11','-ffunction-sections','-fdata-sections','-I'+str(d),'-I'+str(ROOT/'port/ps5/shared'),str(p),'-Wl,--gc-sections','-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True,timeout=10,creationflags=0x08004000)
 print('PASS: actual producer + shim refresh/getters; all families, old hosts, malformed metadata, disconnect/reconnect and bounded queries')
if __name__=='__main__':main()
