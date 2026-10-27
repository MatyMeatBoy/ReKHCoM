"""Execute Android movie reset dispatch and the actual bridge public wrapper."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'port/portable/hdmovie_android.c').read_text(encoding='utf-8')
a=source.index('static void (*sRendererResetCallback)');b=source.index('typedef struct {\n    AMediaExtractor* ex;',a)
production=source[a:b]
header=r'''
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#define HdMovieLog(...) ((void)0)
typedef int SDL_AudioStream;
typedef struct {int type;struct{int repeat,key,scancode;}key;struct{int button;}gbutton;struct{int which;}gdevice;}SDL_Event;
enum {SDL_EVENT_RENDER_DEVICE_RESET=1,SDL_EVENT_QUIT,SDL_EVENT_TERMINATING,SDL_EVENT_DID_ENTER_BACKGROUND,SDL_EVENT_DID_ENTER_FOREGROUND,SDL_EVENT_KEY_DOWN,SDL_EVENT_GAMEPAD_BUTTON_DOWN,SDL_EVENT_RENDER_TARGETS_RESET,SDL_EVENT_GAMEPAD_ADDED};
enum {SDLK_ESCAPE=10,SDLK_AC_BACK,SDL_GAMEPAD_BUTTON_START,SDL_SCANCODE_X,SDL_SCANCODE_Z,SDL_SCANCODE_RETURN,SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST};
static SDL_Event events[8];static int at,count,callbacks,paused,resumed;
static int SDL_PollEvent(SDL_Event*e){if(at==count)return 0;*e=events[at++];return 1;}
static int SDL_WaitEvent(SDL_Event*e){return SDL_PollEvent(e);}
static void SDL_PauseAudioStreamDevice(void*a){paused++;}
static void SDL_ResumeAudioStreamDevice(void*a){resumed++;}
static void RendererDeviceReset(void){callbacks++;}
'''
# Source-level producer return is tested separately by test_hd_movie_failures.
# Run the actual Android64 wrapper with an internal reset already mapped to 0.
host=(ROOT/'port/android64/host/android64_host.c').read_text(encoding='utf-8')
a=host.index('static int H_hd_movie(const char* movie,');b=host.index('\n}',a)+2
wrapper=host[a:b]
body=r'''
static void*sRenderer;static int sQuit;
static int publicResult;
static int HdMovieAndroidPlay(void*r,const char*p,const char*a,const char*b,const char*c){
 assert(sRendererResetCallback==RendererDeviceReset);
 return publicResult; /* production internal-to-public mapping tested separately */
}
'''
native=(ROOT/'port/src/port_hdmovie.c').read_text(encoding='utf-8')
a=native.rindex('static int PlayFile(const char* name) {');b=native.index('\n}',a)+2
native_wrapper=native[a:b]
native_stubs=r'''
#include <stdio.h>
#define SDL_snprintf snprintf
#define SDL_zero(e) memset(&(e),0,sizeof(e))
static int sStartAt;
static struct {int hdSubtitles;}gPortConfig;
static void MoviePath(const char*n,char*p,int size){snprintf(p,size,"%s",n);}
static void SubsOrder(const char**o){o[0]="en";o[1]="es";}
static const char*PortConfigPath(const char*p){return p;}
static void SDL_strlcpy(char*p,const char*s,int cap){snprintf(p,cap,"%s",s);}
static int SDL_GetPathInfo(const char*p,void*i){return 1;}
static void PortAudioMovieHold(int on){}
static void*PortRenderer(void){return sRenderer;}
static int SDL_PushEvent(SDL_Event*e){return 1;}
static int h_hd_movie(const char*m,const char*a,const char*b,const char*f){return H_hd_movie(m,a,b,f);}
'''
linux=(ROOT/'port/linux64/hdmovie_ffmpeg.inc').read_text(encoding='utf-8')
a=linux.index('static int MovieEvents(');b=linux.index('\n}',a)+2
linux_events=linux[a:b]
linux_return=next(line.strip() for line in linux.splitlines() if 'return rendererLost ?' in line)
linux_return='static int LinuxReturn(int rendererLost,int played,int skip){'+linux_return+'}\n'
linux_stubs=r'''
static int*sPad;
static int*SDL_OpenGamepad(int id){static int pad;return &pad;}
'''
cases=r'''
int main(void){
 HdMovieAndroidSetRendererResetCallback(RendererDeviceReset);
 events[0].type=SDL_EVENT_RENDER_DEVICE_RESET;count=1;assert(SkipPressed(NULL)==3&&callbacks==1);
 at=0;events[0].type=SDL_EVENT_DID_ENTER_BACKGROUND;events[1].type=SDL_EVENT_RENDER_DEVICE_RESET;events[2].type=SDL_EVENT_DID_ENTER_FOREGROUND;count=3;
 assert(SkipPressed((void*)1)==3&&callbacks==2&&paused==1&&resumed==0&&at==3);
 at=0;events[0].type=SDL_EVENT_RENDER_TARGETS_RESET;count=1;assert(!SkipPressed(NULL)&&callbacks==2);
 at=0;events[0].type=SDL_EVENT_QUIT;assert(SkipPressed(NULL)==2);
 assert(H_hd_movie("x",NULL,NULL,NULL)==0&&!sQuit&&!sRendererResetCallback);
 HdMovieAndroidSetRendererResetCallback(RendererDeviceReset);
 assert(PlayFile("x")==0&&!sQuit);
 int skip=0;at=0;count=1;events[0].type=SDL_EVENT_RENDER_DEVICE_RESET;
 int before=callbacks;assert(MovieEvents(&skip)==1&&callbacks==before+1&&!skip);
 at=0;events[0].type=SDL_EVENT_RENDER_TARGETS_RESET;assert(!MovieEvents(&skip));
 publicResult=1;
 HdMovieAndroidSetRendererResetCallback(RendererDeviceReset);
 assert(PlayFile("x")==1);
 assert(LinuxReturn(1,1,1)==0&&LinuxReturn(0,0,1)==1);
 return 0;
}
'''
cc=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
with tempfile.TemporaryDirectory(prefix='rekhcom-movie-reset-') as td:
 src=Path(td)/'test.c';exe=Path(td)/'test.exe';src.write_text(header+production+body+wrapper+native_stubs+native_wrapper+linux_stubs+linux_events+linux_return+cases,encoding='utf-8')
 subprocess.run([str(cc),str(src),'-o',str(exe)],check=True,capture_output=True)
 subprocess.run([str(exe)],check=True,capture_output=True)
 subprocess.run([str(cc),'-DPORT_ANDROID64',str(src),'-o',str(exe)],check=True,capture_output=True)
 subprocess.run([str(exe)],check=True,capture_output=True)
