"""Desktop cursor deadline and focus lifecycle with the actual SDL event types."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
unit = r'''
#include <assert.h>
#include <stdio.h>
#include <SDL3/SDL.h>
static SDL_Window *game=(SDL_Window*)1,*other=(SDL_Window*)2;
static SDL_Window *mouse,*keyboard;
static SDL_WindowFlags flags;
static Uint64 ticks;
static unsigned hides,shows;
static bool failHide,failShow;
static Uint64 TestTicks(void){return ticks;}
static SDL_Window* TestMouse(void){return mouse;}
static SDL_Window* TestKeyboard(void){return keyboard;}
static SDL_WindowFlags TestFlags(SDL_Window*w){assert(w==game);return flags;}
static SDL_WindowID TestId(SDL_Window*w){return w==game?1:2;}
static bool TestHide(void){hides++;return !failHide;}
static bool TestShow(void){shows++;return !failShow;}
#define SDL_GetTicks TestTicks
#define SDL_GetMouseFocus TestMouse
#define SDL_GetKeyboardFocus TestKeyboard
#define SDL_GetWindowFlags TestFlags
#define SDL_GetWindowID TestId
#define SDL_HideCursor TestHide
#define SDL_ShowCursor TestShow
#include "port/cursor_idle.h"
static void event(PortCursorIdle*s,Uint32 type,unsigned id,float dx){
 SDL_Event e={0};e.type=type;
 if(type==SDL_EVENT_MOUSE_MOTION){e.motion.windowID=id;e.motion.xrel=dx;}
 else if(type==SDL_EVENT_MOUSE_BUTTON_DOWN){e.button.windowID=id;}
 else e.window.windowID=id;
 PortCursorIdleEvent(s,game,&e);
}
int main(void){
 PortCursorIdle s={0};
 ticks=100000;PortCursorIdlePoll(&s,game);assert(!hides&&!s.hidden);
 mouse=keyboard=game;PortCursorIdlePoll(&s,game);
 ticks+=1999;PortCursorIdlePoll(&s,game);assert(!hides);
 ticks++;PortCursorIdlePoll(&s,game);assert(hides==1&&s.hidden);
 ticks+=500;PortCursorIdlePoll(&s,game);assert(hides==1);
 event(&s,SDL_EVENT_MOUSE_MOTION,2,1);assert(s.hidden&&!shows);
 event(&s,SDL_EVENT_MOUSE_MOTION,1,0);assert(s.hidden&&!shows);
 event(&s,SDL_EVENT_MOUSE_MOTION,1,1);assert(!s.hidden&&shows==1);
 ticks+=2000;event(&s,SDL_EVENT_MOUSE_MOTION,1,1);
 PortCursorIdlePoll(&s,game);assert(hides==1); /* deadline motion wins */
 ticks+=2000;PortCursorIdlePoll(&s,game);assert(s.hidden&&hides==2);
 keyboard=other;PortCursorIdlePoll(&s,game);assert(!s.hidden&&shows==2);
 ticks+=10000;PortCursorIdlePoll(&s,game);assert(hides==2);
 keyboard=game;PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=2000;PortCursorIdlePoll(&s,game);assert(s.hidden);
 mouse=other;event(&s,SDL_EVENT_WINDOW_MOUSE_LEAVE,1,0);
 PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=10000;mouse=game;event(&s,SDL_EVENT_WINDOW_MOUSE_ENTER,1,0);
 PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=2000;PortCursorIdlePoll(&s,game);assert(s.hidden);
 flags=SDL_WINDOW_MINIMIZED;PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=10000;flags=0;PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=2000;failHide=true;PortCursorIdlePoll(&s,game);assert(!s.hidden);
 failHide=false;PortCursorIdlePoll(&s,game);assert(s.hidden);
 failShow=true;event(&s,SDL_EVENT_MOUSE_BUTTON_DOWN,1,0);assert(s.hidden);
 failShow=false;PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=2000;PortCursorIdlePoll(&s,game);assert(s.hidden);
 flags=SDL_WINDOW_HIDDEN;PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=10000;flags=0;event(&s,SDL_EVENT_WINDOW_SHOWN,1,0);
 PortCursorIdlePoll(&s,game);assert(!s.hidden);
 ticks+=2000;PortCursorIdlePoll(&s,game);assert(s.hidden);
 PortCursorIdlePoll(&s,NULL);assert(!s.hidden);
 puts("PASS cursor: exact 2s deadline, movement, window ownership, focus/leave/reentry/minimize and SDL failure retry");
}
'''
compiler = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
sdl = next((ROOT.parent / 'toolchain').glob('SDL3-*/x86_64-w64-mingw32/include'))
with tempfile.TemporaryDirectory(prefix='rekhcom-cursor-') as tmp:
    tmp = Path(tmp)
    source, exe = tmp / 'cursor.c', tmp / 'cursor.exe'
    source.write_text(unit, encoding='utf-8')
    subprocess.run([str(compiler), '-std=c11', '-Wall', '-Werror', f'-I{sdl}',
                    f'-I{ROOT / "port/include"}', str(source), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
