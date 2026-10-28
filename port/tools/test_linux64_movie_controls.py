"""Run production movie pause decisions without SDL, a decoder or a display."""
from pathlib import Path
import os, shutil, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
path=ROOT/'port/linux64/hdmovie_ui.inc'
assert path.exists(), 'Linux64 has no movie pause/confirmation implementation'
s=path.read_text(encoding='utf-8')
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}',a)+2]
code=function('static int MoviePauseChoice(')+'\n'+function('static int MovieLanguage(')
movie=(ROOT/'port/linux64/hdmovie_ffmpeg.inc').read_text(encoding='utf-8')
event_start=movie.index('static int MovieEvents(')
events=movie[event_start:movie.index('\n}',event_start)+2]
clock_start=movie.index('static double MovieClock(')
clock=movie[clock_start:movie.index('\n}',clock_start)+2]
shift=s[s.index('    *began += SDL_GetTicksNS()'):s.index('\n',s.index('    *began += SDL_GetTicksNS()'))]
test=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
static double now;
static double SDL_GetTicksNS(void){return now*1e9;}
CLOCK
static void pause_clock(double*began,double before){SHIFT}
typedef struct {int type;struct{int key,repeat;}key;struct{int button;}gbutton;struct{int which;}gdevice;}SDL_Event;
enum {SDL_EVENT_KEY_DOWN=1,SDL_EVENT_GAMEPAD_BUTTON_DOWN,SDL_EVENT_RENDER_DEVICE_RESET,SDL_EVENT_QUIT,SDL_EVENT_MOUSE_BUTTON_DOWN,SDL_EVENT_FINGER_DOWN,SDL_EVENT_GAMEPAD_ADDED};
enum {SDLK_ESCAPE=27,SDL_GAMEPAD_BUTTON_START=6};
static int sQuit,reset;static void*sPad;static int sMovieStartKeys[2]={13,99};
static SDL_Event pending;static int ready;
static int SDL_PollEvent(SDL_Event*e){if(!ready)return 0;*e=pending;ready=0;return 1;}
static void RendererDeviceReset(void){reset++;}
static void* SDL_OpenGamepad(int id){(void)id;return &sQuit;}
EVENTS
CODE
static void event(int type,int key,int button,int repeat,int expected){
 pending=(SDL_Event){0};pending.type=type;pending.key.key=key;pending.key.repeat=repeat;pending.gbutton.button=button;
 ready=1;int pause=0;MovieEvents(&pause);assert(pause==expected);
}
int main(void){
 int confirm=0,selected=0;
 assert(MoviePauseChoice(&confirm,&selected,0)==0);
 assert(MoviePauseChoice(&confirm,&selected,4)==1); /* Continue */
 confirm=selected=0;
 assert(MoviePauseChoice(&confirm,&selected,2)==0&&selected==1);
 assert(MoviePauseChoice(&confirm,&selected,4)==0&&confirm&&selected==1); /* No default */
 assert(MoviePauseChoice(&confirm,&selected,4)==0&&!confirm&&selected==1); /* No */
 assert(MoviePauseChoice(&confirm,&selected,4)==0&&confirm&&selected==1);
 assert(MoviePauseChoice(&confirm,&selected,3)==0&&!confirm&&selected==1); /* Back */
 assert(MoviePauseChoice(&confirm,&selected,4)==0&&confirm);
 assert(MoviePauseChoice(&confirm,&selected,1)==0&&selected==0);
 assert(MoviePauseChoice(&confirm,&selected,4)==2); /* Explicit Yes */
 confirm=selected=0;
 assert(MoviePauseChoice(&confirm,&selected,3)==1);
 event(SDL_EVENT_KEY_DOWN,'x',0,0,0);event(SDL_EVENT_KEY_DOWN,'z',0,0,0);
 event(SDL_EVENT_GAMEPAD_BUTTON_DOWN,0,0,0,0);event(SDL_EVENT_GAMEPAD_BUTTON_DOWN,0,1,0,0);
 event(SDL_EVENT_KEY_DOWN,13,0,0,1);event(SDL_EVENT_KEY_DOWN,99,0,0,1);
 event(SDL_EVENT_KEY_DOWN,13,0,1,0);event(SDL_EVENT_KEY_DOWN,27,0,0,1);
 event(SDL_EVENT_GAMEPAD_BUTTON_DOWN,0,SDL_GAMEPAD_BUTTON_START,0,1);
 event(SDL_EVENT_MOUSE_BUTTON_DOWN,0,0,0,1);event(SDL_EVENT_FINGER_DOWN,0,0,0,1);
 event(SDL_EVENT_QUIT,0,0,0,0);assert(sQuit);
 event(SDL_EVENT_RENDER_DEVICE_RESET,0,0,0,0);assert(reset==1);
 event(SDL_EVENT_GAMEPAD_ADDED,0,0,0,0);assert(sPad);
 assert(MovieLanguage(0,"es",NULL)==4); /* --language with automatic setting */
 assert(MovieLanguage(0,NULL,"/hd/subs/intro.es.txt")==4); /* guest's actual language */
 assert(MovieLanguage(5,NULL,NULL)==4); /* remembered choice, subtitles off */
 assert(MovieLanguage(2,"es","/hd/subs/intro.de.txt")==2); /* current guest wins */
 assert(MovieLanguage(5,"es","/hd/subs/intro.us.txt")==0);
 assert(MovieLanguage(0,NULL,NULL)==0);assert(MovieLanguage(99,"invalid",NULL)==0);
 double began=0;
 assert(MovieClock(1,0,192000,96000,20,&began)==.5);
 now=25;pause_clock(&began,20);
 assert(MovieClock(1,0,192000,96000,25,&began)==.5); /* PCM waits, no sample subtraction */
 assert(MovieClock(1,0,192000,48000,25.25,&began)==.75);
 assert(MovieClock(1,1,192000,48000,25.25,&began)==.75); /* EOF fallback is continuous */
 began=0;now=6;pause_clock(&began,1);
 assert(MovieClock(0,1,0,0,6,&began)==1); /* silent movie waits5s */
 assert(MovieClock(0,1,0,0,7,&began)==2);
 puts("PASS production movie controls: pause triggers, A/B ignored, Continue/Skip/No-default/Back/Yes, language, PCM and wall-clock pause");
}
'''.replace('CODE',code).replace('EVENTS',events).replace('CLOCK',clock).replace('SHIFT',shift)
compiler=shutil.which('cc') or shutil.which('clang') or shutil.which('gcc') or next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
flags={'creationflags':0x08004000} if os.name=='nt' else {}
with tempfile.TemporaryDirectory() as directory:
 p=Path(directory);(p/'test.c').write_text(test,encoding='utf-8')
 result=subprocess.run([str(compiler),'-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test.exe')],capture_output=True,text=True,**flags)
 assert result.returncode==0,result.stdout+result.stderr
 result=subprocess.run([str(p/'test.exe')],capture_output=True,text=True,**flags)
 print(result.stdout+result.stderr,end='')
 assert result.returncode==0 and 'PASS production movie controls:' in result.stdout,result.returncode
 # Compile the complete UI source against the actual SDL API without starting
 # SDL, linking a host or decoding any media. Tiny stubs provide host context.
 sdl=ROOT.parent/'toolchain/SDL3-3.4.18/x86_64-w64-mingw32/include'
 if not (sdl/'SDL3/SDL.h').exists():
  print('SKIP UI API compilation: SDL development headers unavailable')
 else:
  ui=r'''
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stb_truetype.h"
static char sData[1024]=".";static int sQuit;static SDL_Renderer*sRenderer;
static void Log(const char*format,...){(void)format;}
static void RendererDeviceReset(void){}
static SDL_Texture*SubTexture(stbtt_fontinfo*f,const char*t,float px,int*w,int*h){(void)f;(void)t;(void)px;(void)w;(void)h;return NULL;}
'''+f'\n#include "{path.as_posix()}"\n'
  (p/'ui.c').write_text(ui,encoding='utf-8')
  result=subprocess.run([str(compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function',
   '-I'+str(sdl),'-I'+str(ROOT/'port/ps5/third_party/stb'),'-c',str(p/'ui.c'),'-o',str(p/'ui.o')],capture_output=True,text=True,**flags)
  assert result.returncode==0,result.stdout+result.stderr
  print('PASS complete production movie UI compiles against SDL3 headers; no GUI/link/runtime claim')
