"""Exercise the actual in-game achievements input function, without a ROM/account."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
out = ROOT / 'build/achievement-input-test'
out.mkdir(parents=True, exist_ok=True)
source = (ROOT / 'port/src/port_achievement_menu.inc').read_text(encoding='utf-8')
start = source.index('static int sAchievementTriggerHeld')
end = source.index('static void AchievementMenuText', start)
unit = r'''
#include <assert.h>
#include <stdio.h>
#include "port/pad_layout.h"
enum {PAGE_MAIN,PAGE_CHEATS,PAGE_ACHIEVEMENTS,ITEM_CHEAT_REBEL_LIST,ITEM_ACHIEVEMENTS_MENU};
static int sPage,sCursor,sScroll,sAchievementChoice,sAchievementScroll,sAchievementView,sAchievementList,sMenuLightStart;
static int PortRecomListCount(void){return 1;}
static int PortRecomActive(void){return 0;}
static void PortAchievementsListRefresh(void){}
static int MenuLightNow(void){return 0;}
static int AchievementMenuRows(int* indices,int* earned){*earned=0;return 0;}
static int UiPadButton(const SDL_Event* event){return event->gbutton.button;}
static int Wrap(int value,int count){return (value%count+count)%count;}
static void Close(void){}
'''
unit += source[start:end]
unit += r'''
static void axis(int which,int value){SDL_Event event={0};event.type=SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.axis=(Uint8)which;event.gaxis.value=(Sint16)value;AchievementMenuInput(&event);}
int main(void){
    AchievementMenuOpen();assert(sAchievementList==0);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,0);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,9000);assert(sAchievementList==0);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,16000);assert(sAchievementList==0);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,17000);assert(sAchievementList==1);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,18000);assert(sAchievementList==1);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,15000);assert(sAchievementList==1);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,9000);assert(sAchievementList==1);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,17000);assert(sAchievementList==1);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,8000);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,17000);assert(sAchievementList==0);
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,9000);assert(sAchievementList==0);
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,17000);assert(sAchievementList==1);
    AchievementMenuOpen();assert(sAchievementList==0);
    axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,17000);assert(sAchievementList==1);
    RebelMenuOpen();assert(sAchievementList==2);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,9000);assert(sAchievementList==2);
    axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,17000);assert(sAchievementList==3);
    puts("Achievement menu: slow LT/RT squeeze, threshold jitter, release, reopen and Rebel lists PASS");return 0;
}
'''
(out / 'input.c').write_text(unit, encoding='utf-8')
cc = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
exe = out / 'input.exe'
subprocess.run([str(cc), '-O2', '-I' + str(ROOT / 'port/include'), '-I' + str(sdl / 'include'),
                str(out / 'input.c'), '-o', str(exe)], check=True)
subprocess.run([str(exe)], check=True)
