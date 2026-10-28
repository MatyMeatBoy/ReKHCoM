"""CPU integration of actual Linux host, shim refresh, and game input consumers.

SDL device calls are controlled fixtures; no Wayland/Steam/hardware claim.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(text, name):
    start = text.index(name)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


PREAMBLE = r'''
#include <SDL3/SDL.h>
#include "bridge_calls.h"
#define SDL_SCANCODE_A 4
#define SDL_SCANCODE_S 22
#define SDL_SCANCODE_Z 29
#define SDL_SCANCODE_X 27
#define SDL_SCANCODE_Q 20
#define SDL_SCANCODE_W 26
#define SDL_SCANCODE_RETURN 40
#define SDL_SCANCODE_BACKSPACE 42
#define SDL_SCANCODE_UP 82
#define SDL_SCANCODE_DOWN 81
#define SDL_SCANCODE_LEFT 80
#define SDL_SCANCODE_RIGHT 79
#define SDL_EVENT_RENDER_DEVICE_RESET 0x2001
#define SDL_EVENT_GAMEPAD_REMOVED 0x654
static SDL_Window* sWindow;
static SDL_Gamepad* sPad;
static int sQuit, physical, fullscreen;
static uint32_t sRendererGeneration;
static bool keyboard[512];
static SDL_Event incoming[1024];static int inRead,inWrite;
typedef struct {int unused;} PortCursorIdle;
static void PortCursorIdleEvent(PortCursorIdle* c,SDL_Window* w,SDL_Event* e){}
static void PortCursorIdlePoll(PortCursorIdle* c,SDL_Window* w){}
static void RendererDeviceReset(void){}
static bool HostPoll(SDL_Event* e){if(inRead==inWrite)return false;*e=incoming[inRead++];return true;}
static const bool* HostKeyboard(int* n){return keyboard;}
static bool HostConnected(SDL_Gamepad* p){return physical;}
static SDL_GamepadType HostType(SDL_Gamepad* p){return SDL_GAMEPAD_TYPE_XBOXONE;}
static bool HostButton(SDL_Gamepad* p,SDL_GamepadButton b){return false;}
static Sint16 HostAxis(SDL_Gamepad* p,SDL_GamepadAxis a){return 0;}
static SDL_Gamepad* HostOpen(SDL_JoystickID id){return sPad;}
static SDL_JoystickID HostID(SDL_Gamepad* p){return 1;}
static void HostClose(SDL_Gamepad* p){}
bool SDL_SetWindowFullscreen(SDL_Window* w,bool on){fullscreen++;return true;}
Uint64 SDL_GetWindowFlags(SDL_Window* w){return 0;}
static const char* PortDeviceHostQuery(const char* n){return NULL;}
static int HostGpuQuery(const char* n,char* out,int cap){return -2;}
#define SDL_PollEvent HostPoll
#define SDL_GetKeyboardState HostKeyboard
#define SDL_GamepadConnected HostConnected
#define SDL_GetGamepadType HostType
#define SDL_GetGamepadButton HostButton
#define SDL_GetGamepadAxis HostAxis
#define SDL_OpenGamepad HostOpen
#define SDL_GetGamepadID HostID
#define SDL_CloseGamepad HostClose
#include "host_gamepad_info.h"
HOST_QUEUE
HOST_ENV
HOST_PUMP
HOST_PAD
#undef SDL_PollEvent
#undef SDL_GetKeyboardState
#undef SDL_GamepadConnected
#undef SDL_GetGamepadType
#undef SDL_GetGamepadButton
#undef SDL_GetGamepadAxis
#undef SDL_OpenGamepad
#undef SDL_GetGamepadID
#undef SDL_CloseGamepad
int h_getenv(const char* n,char* out,int cap){return H_getenv(n,out,cap);}
int h_pad_poll(HostPad* p,int n){return H_pad_poll(p,n);}
uint64_t h_ticks_us(void){return 0;}
SHIM
#define PORT_ACT_COUNT 13
#define PORT_ACT_STOCK 10
#define PORT_ACT_DODGE 11
#define PORT_PAD_AXIS_BASE 100
#define PORT_KEY_STOCK 0x400
#define PORT_KEY_DODGE 0x800
#define PORT_KEY_UNSTOCK 0x1000
#define STICK_DEADZONE 12000
#define TRIGGER_THRESHOLD 16000
KEY_BITS
ACTION_BITS
static struct {int keys[13][2],pad[13],padLayout,dodgeButton,stockButton,unstockButton;} gPortConfig;
static uint16_t sKeyboard;static int sSuppress;
static int sBattleMenuKeys,sBattleMenuChord,sZoomSelect,sZoomEat,sDodgeHeld,sDodgePending,sUnstockHeld,sUnstockPending;
static bool KhPadButtonDown(SDL_Gamepad* p,int binding,int layout){return SDL_GetGamepadButton(p,(SDL_GamepadButton)binding);}
static void VirtualPadUpdate(void){}
INPUT_EVENT
GAMEPAD_KEYS
INPUT_KEYS
static void cycle(void){Refresh();while(sQHead!=sQTail){PortInputHandleEvent(&sQueue[sQHead]);sQHead=(sQHead+1)&63;}}
static void inject(unsigned type,unsigned key,unsigned mod,bool repeat){SDL_Event e={0};e.type=type;e.key.key=key;e.key.mod=mod;e.key.repeat=repeat;incoming[inWrite++]=e;}
int main(int argc,char** argv){
 int mode=atoi(argv[1]);physical=mode==1;sPad=physical?(SDL_Gamepad*)(uintptr_t)1:NULL;
 for(int i=0;i<13;i++)gPortConfig.pad[i]=-1;
 gPortConfig.keys[4][0]=SDLK_RETURN;gPortConfig.keys[0][0]='k';gPortConfig.keys[0][1]='l';gPortConfig.keys[1][0]='j';gPortConfig.keys[5][0]=SDLK_BACKSPACE;gPortConfig.pad[4]=SDL_GAMEPAD_BUTTON_START;
 keyboard[SDL_SCANCODE_RETURN]=true;inject(SDL_EVENT_KEY_DOWN,SDLK_RETURN,0,false);
 HostPad legacy[HOSTPAD_MAX];H_pad_poll(legacy,HOSTPAD_MAX);
 if(!(legacy[0].buttons&(1u<<3)))return 19; /* Old guests retain keyboard-to-pad defaults. */
 cycle();
 if(!(PortInputKeys()&KEY_START))return 10; /* no pad and a real pad both work */
 keyboard[SDL_SCANCODE_RETURN]=false;inject(SDL_EVENT_KEY_UP,SDLK_RETURN,0,false);cycle();if(PortInputKeys())return 11;
 inject(SDL_EVENT_KEY_DOWN,'k',0,false);cycle();if(PortInputKeys()!=KEY_A)return 12; /* configured non-default binding */
 inject(SDL_EVENT_WINDOW_FOCUS_LOST,0,0,false);cycle();if(PortInputKeys())return 13;
 inject(SDL_EVENT_KEY_DOWN,'k',0,true);cycle();if(PortInputKeys())return 14; /* repeat ignored by actual game */
 if(mode==0){int count;SDL_JoystickID* ids=SDL_GetGamepads(&count);free(ids);if(count)return 15;} /* no invented physical controller */
 inject(SDL_EVENT_KEY_DOWN,SDLK_F11,0,false);cycle();if(fullscreen)return 16; /* guest channel owns shortcut, no duplicate host toggle */
 inject(SDL_EVENT_KEY_DOWN,'j',0,false);cycle();if(PortInputKeys()!=KEY_B)return 20;
 inject(SDL_EVENT_KEY_UP,'j',0,false);inject(SDL_EVENT_KEY_DOWN,SDLK_BACKSPACE,0,false);cycle();if(PortInputKeys()!=KEY_SELECT)return 21;
 inject(SDL_EVENT_KEY_UP,SDLK_BACKSPACE,0,false);inject(SDL_EVENT_KEY_DOWN,'l',0,false);cycle();if(PortInputKeys()!=KEY_A)return 22;
 physical=!physical;sPad=physical?(SDL_Gamepad*)(uintptr_t)1:NULL;cycle();if(PortInputKeys()!=KEY_A)return 23;
 physical=!physical;sPad=physical?(SDL_Gamepad*)(uintptr_t)1:NULL;cycle();if(PortInputKeys()!=KEY_A)return 24;
 for(int i=0;i<80;i++)inject(SDL_EVENT_KEY_UP,'l',0,false);
 cycle();cycle();if(PortInputKeys())return 25; /* Overflow cannot strand a held action. */
 for(int i=0;i<100;i++){inject(SDL_EVENT_KEY_DOWN,'k',0,false);cycle();if(PortInputKeys()!=KEY_A)return 26;inject(SDL_EVENT_KEY_UP,'k',0,false);cycle();if(PortInputKeys())return 27;}
 char text[96];if(H_getenv("KHCOM_HOST_KEY_EVENT",text,1)!=-1)return 28;
 printf("PASS physical=%d custom binding/release/focus/repeat/no fake pad\n",physical);return 0;
}
'''


def run(out, host_source=None):
    host = (host_source or ROOT / 'port/linux64/linux64_host.c').read_text(encoding='utf-8')
    shim = (ROOT / 'port/ps5/guest/src/sdl_shim.c').read_text(encoding='utf-8')
    game = (ROOT / 'port/src/port_input.c').read_text(encoding='utf-8')
    queue = ''
    if '/* Optional event channel over getenv' in host:
        queue = host[host.index('/* Optional event channel over getenv'):host.index('static uint32_t sRendererGeneration')]
    consumer = shim[shim.index('typedef struct Pad {'):shim.index('/* Touch screen and background state')]
    consumer += shim[shim.index('SDL_JoystickID* SDL_GetGamepads('):shim.index('const char* SDL_GetGamepadStringForButton(')]
    consumer += function(shim, 'bool SDL_GetGamepadButton(') + function(shim, 'Sint16 SDL_GetGamepadAxis(')
    replacements = {
        'HOST_QUEUE': queue, 'HOST_ENV': function(host, 'static int H_getenv('),
        'HOST_PUMP': function(host, 'static void Pump(void) {'),
        'HOST_PAD': function(host, 'static int H_pad_poll('), 'SHIM': consumer,
        'KEY_BITS': game[game.index('#define KEY_A'):game.index('/* Frames a dodge')],
        'ACTION_BITS': game[game.index('static const uint16_t sActionBits'):game.index('static uint16_t sKeyboard')],
        'INPUT_EVENT': function(game, 'void PortInputHandleEvent('),
        'GAMEPAD_KEYS': function(game, 'static uint16_t GamepadKeys('),
        'INPUT_KEYS': function(game, 'uint16_t PortInputKeys('),
    }
    code = PREAMBLE
    for marker, actual in replacements.items():
        code = code.replace(marker, actual)
    (out / 'SDL3').mkdir(exist_ok=True)
    shutil.copy2(ROOT / 'port/ps5/guest/include/SDL3/SDL.h', out / 'SDL3/SDL.h')
    shutil.copy2(ROOT / 'port/ps5/guest/include/bridge_calls.h', out / 'bridge_calls.h')
    fixture = out / 'keyboard.c'
    fixture.write_text(code, encoding='utf-8')
    if os.name == 'nt':
        tools = next(p / 'toolchain' for p in ROOT.parents if (p / 'toolchain').is_dir())
        binary = next(tools.glob('llvm-mingw-*/bin'))
        compilers = [(arch, binary / (arch + '-w64-mingw32-clang.exe')) for arch in ('i686', 'x86_64')]
    else:
        cc = shutil.which('clang') or shutil.which('cc')
        if not cc:
            raise RuntimeError('C compiler required')
        compilers = [('native', Path(cc))]
    results = []
    for arch, cc in compilers:
        exe = out / ('keyboard-' + arch + ('.exe' if os.name == 'nt' else ''))
        compiled = subprocess.run([str(cc), '-std=gnu11', '-I' + str(out), '-I' + str(ROOT / 'port/ps5/shared'),
                                   str(fixture), '-o', str(exe)], capture_output=True, text=True)
        (out / ('COMPILE-' + arch + '.txt')).write_text(compiled.stdout + compiled.stderr, encoding='utf-8')
        compiled.check_returncode()
        for mode in (0, 1):
            result = subprocess.run([str(exe), str(mode)], capture_output=True, text=True)
            results.append(dict(arch=arch, physical=bool(mode), exit=result.returncode, stdout=result.stdout, stderr=result.stderr))
    (out / 'RESULTS.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    if any(r['exit'] for r in results):
        raise RuntimeError(results)
    print('PASS actual Linux host→shim→PortInputKeys keyboard path, with/without physical controller')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path)
    parser.add_argument('--host-source', type=Path, help='optional preserved baseline host source for RED reproduction')
    args = parser.parse_args()
    if args.out:
        args.out.mkdir(parents=True, exist_ok=True)
        run(args.out, args.host_source)
    else:
        with tempfile.TemporaryDirectory(prefix='linux-keyboard-') as directory:
            run(Path(directory), args.host_source)
