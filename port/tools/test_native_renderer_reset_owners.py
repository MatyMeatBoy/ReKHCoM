"""Production reset hooks preserve UI state and abort native HD movies without skipping story."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def function(file,signature):
 s=(ROOT/file).read_text(encoding='utf-8');a=s.index(signature);return s[a:s.index('\n}',a)+2]
CASES=[]
CASES.append((r'''
#include <assert.h>
#include <stdlib.h>
typedef int SDL_Texture;typedef int SDL_Renderer;
static SDL_Texture*sTexture;static SDL_Renderer*sRenderer;static int sVisible,sCount=2,sHead=3,sStart=77,destroyed;
static void SDL_DestroyTexture(SDL_Texture*p){destroyed++;free(p);}
'''+function('port/src/port_achievement_toast.c','void PortAchievementToastInvalidate(')+r'''
int main(void){sTexture=malloc(4);sRenderer=(void*)1;sVisible=1;PortAchievementToastInvalidate();assert(destroyed==1&&!sTexture&&!sRenderer&&!sVisible);assert(sCount==2&&sHead==3&&sStart==77);PortAchievementToastInvalidate();assert(destroyed==1);return 0;}
''','toast queued state'))
CASES.append((r'''
#include <assert.h>
#include <stdlib.h>
typedef int SDL_Texture;typedef int SDL_Renderer;
static SDL_Texture*sPreviewTexture;static SDL_Renderer*sPreviewRenderer;static int sTextureSerial=7,sTextureW=3,sTextureH=4,destroyed;
static int sPreviewReady=1,sPreviewSerial=9;
static void SDL_DestroyTexture(SDL_Texture*p){if(p){destroyed++;free(p);}}
'''+function('port/src/port_scene_gallery.c','void PortSceneGalleryPreviewInvalidate(')+r'''
int main(void){sPreviewTexture=malloc(4);sPreviewRenderer=(void*)1;PortSceneGalleryPreviewInvalidate();assert(destroyed==1&&!sPreviewTexture&&!sPreviewRenderer&&sTextureSerial==-1&&!sTextureW&&!sTextureH);assert(sPreviewReady&&sPreviewSerial==9);PortSceneGalleryPreviewInvalidate();assert(destroyed==1);return 0;}
''','gallery CPU preview state'))
movie=(ROOT/'port/src/port_hdmovie.c').read_text()
# Compile the exact result expression of both native PlayFile implementations.
returns=[];cursor=0
for unused in range(2):
 a=movie.index('static int PlayFile(const char* name) {',cursor);b=movie.index('\n}',a)+2
 returns.append(movie[a:b][movie[a:b].rindex('    return '):].split('\n')[0].strip());cursor=b
movie_tests=r'''
#include <assert.h>
#include <stdint.h>
typedef int SDL_Keycode;
typedef struct{uint32_t type;struct{int repeat,key;}key;struct{int button;}gbutton;}SDL_Event;
#define SDL_EVENT_RENDER_DEVICE_RESET 1
#define SDL_EVENT_RENDER_TARGETS_RESET 2
#define SDL_EVENT_QUIT 3
#define SDL_EVENT_KEY_DOWN 4
#define SDL_EVENT_GAMEPAD_BUTTON_DOWN 5
#define SDL_EVENT_FINGER_DOWN 6
#define SDL_EVENT_MOUSE_BUTTON_DOWN 7
#define SDLK_ESCAPE 27
#define SDL_GAMEPAD_BUTTON_START 11
#define PORT_ACT_START 0
static struct{int keys[1][2];}gPortConfig;
static int sMovieDeviceReset,eventType,resets;
static int PortRendererHandleReset(uint32_t t){if(t==1){resets++;return 1;}return t==2;}
static int SDL_PollEvent(SDL_Event*e){if(!eventType)return 0;e->type=eventType;e->key.repeat=0;e->key.key=27;eventType=0;return 1;}
'''+function('port/src/port_hdmovie.c','static int MovieResetEvent(')+function('port/src/port_hdmovie.c','static int PauseWanted(')
for i,expression in enumerate(returns):movie_tests+=f'\nstatic int result{i}(int played){{{expression}}}\n'
movie_tests+=r'''
int main(void){int quit=0;eventType=2;assert(!PauseWanted(&quit)&&!sMovieDeviceReset&&!quit);assert(result0(1)&&result1(1));eventType=1;assert(!PauseWanted(&quit)&&sMovieDeviceReset&&resets==1&&!quit);assert(!result0(1)&&!result1(1));sMovieDeviceReset=0;eventType=4;assert(PauseWanted(&quit));assert(!sMovieDeviceReset&&result0(1));return 0;}
'''
CASES.append((movie_tests,'native movie reset versus target reset and input'))
CASES.append((r'''
#include <assert.h>
static int sHdMoviePause=1,sHdMovieSkip,closed;
static void ScenePauseClose(int skip){assert(!skip);sHdMoviePause=0;sHdMovieSkip=skip;closed++;}
'''+function('port/src/port_ui.c','void PortUiCancelHdMoviePause(')+r'''
int main(void){PortUiCancelHdMoviePause();assert(closed==1&&!sHdMoviePause&&!sHdMovieSkip);PortUiCancelHdMoviePause();assert(closed==1);return 0;}
''','HD pause cancel does not skip story'))
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory() as td:
 p=Path(td)
 for i,(code,label) in enumerate(CASES):
  src=p/f'{i}.c';exe=p/f'{i}.exe';src.write_text(code);subprocess.run([str(cc),'-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True);print('PASS production '+label)
