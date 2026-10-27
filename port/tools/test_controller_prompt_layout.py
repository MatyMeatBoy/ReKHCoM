"""Real Latin/SJIS message-layout consumers and immutable symbol-tile producer."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]

def function(source,name):
    start=source.rfind('\n',0,source.index(name+'('))+1
    brace=source.index('{',start);end=brace+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

def main():
    msg=(ROOT/'src/msg/msg.c').read_text(encoding='utf8')
    inp=(ROOT/'port/src/port_input.c').read_text(encoding='utf8')
    activity=inp[inp.index('/* Activity is event-driven:'):inp.index('/* After closing an overlay')]
    runtime=(ROOT/'port/include/port/controller_prompt_runtime.inc').read_text(encoding='utf8')
    core=activity+'\n'+runtime+'\n'+function(msg,'LayoutCardMsgGlyphsPage')+'\n'+function(msg,'LayoutCardMsgGlyphsPageSjis')+'\n'+function(msg,'DrawCardMsgGlyphs')
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
    sdl=next((ROOT.parent/'toolchain').glob('SDL3-*'))/'i686-w64-mingw32'
    with tempfile.TemporaryDirectory(prefix='rekhcom-prompt-layout-') as d:
        d=Path(d)
        for version in ('EU','US'):
            src=d/'layout.c';exe=d/'layout.exe';src.write_text(PRE+core+MAIN,encoding='utf8')
            subprocess.run([str(cc),'-std=gnu11','-Wall','-Wextra','-Werror','-DPLATFORM_PC','-DVERSION_'+version,'-I'+str(ROOT/'port/include'),'-I'+str(sdl/'include'),str(src),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(exe)],check=True)
            import shutil
            shutil.copy2(sdl/'bin/SDL3.dll',d/'SDL3.dll')
            subprocess.run([str(exe)],check=True,timeout=10,creationflags=0x08004000,env=dict(os.environ,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy'))
    print('PASS: actual input + Latin/SJIS live symbols; 1000 draws without loads, family/binding/hotplug swaps, reveal/state and source preserved')

PRE=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/controller_prompts.h"
#include "port/pad_layout.h"
#include "port/controller_prompt_glyphs.h"
PortConfig gPortConfig;
void PortInputResetBindings(void){}
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
#ifdef VERSION_EU
typedef u8 MsgLatinChar;
#define MSG_CODE_HIGHLIGHT 29
#define MSG_CODE_PLAIN 30
#define MSG_CODE_NL 31
#else
typedef u16 MsgLatinChar;
#define MSG_CODE_HIGHLIGHT 0x4079
#define MSG_CODE_PLAIN 0x4000
#define MSG_CODE_NL 10
#endif
#define MSG_CODE_END 0
#define MSG_CODE_JP_NL 0x6e6e
#define TRUE 1
#define FALSE 0
typedef struct {s32 x,y;void*tiles;int visible,useAlternatePalette;void*palette;void*alternatePalette;} TextGlyphSprite;
static TextGlyphSprite storage[128],*sCardMsgGlyphSprites=storage;
static u8 sTextEntryCount;
static u16 sCardMsgGlyphCodes[128];
static struct {u16 widths[256];}sLatinGlyphWidths;
static const u16* gMsgLatinFontFrames[256];
static const u16 *gMsgFontBank0Frames[256],*gMsgFontBank1Frames[256],*gMsgFontBank2Frames[256],*gMsgFontBank3Frames[256];
static u8 gMsgLatinFontTiles[128],gMsgFontBank0Tiles[128],gMsgFontBank1Tiles[128],gMsgFontBank2Tiles[128],gMsgFontBank3Tiles[128];
static int loads,releases,failLoads;
static void* LoadObjTiles(const void*p,int bytes){assert(p && bytes==128);loads++;return failLoads?NULL:(void*)p;}
static void ReleaseObjTiles(void*p){assert(p);releases++;}
static void GetSjisGlyph(u16 c,u16*g,u8*b){(void)c;*g=0;*b=0;}
static int gPortTextChar,drawn;
#define MSG_FONT_FRAMES gMsgFontBank0Frames
static void DrawSprite(int x,int y,const void*f,void*t,void*p,void*a,int flags,int pri){
 (void)x;(void)y;(void)f;(void)t;(void)p;(void)a;(void)flags;(void)pri;drawn++;
}
'''
MAIN=r'''
int main(void){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
 MsgLatinChar latin[]={26,2,26,1,0},copy[5];MsgLatinChar*next=NULL;
 memcpy(copy,latin,sizeof(latin));
 assert(LayoutCardMsgGlyphsPage(100,200,latin,&next)==2 && !next && loads==2);
 assert(storage[0].x==100 && storage[1].x==100+(12<<8));
 assert(sCardMsgGlyphCodes[0]==26 && sCardMsgGlyphCodes[1]==26);
 assert(!memcmp(copy,latin,sizeof(latin)));
 const unsigned char*p=PortControllerGlyphTiles(2);assert(p==PortControllerGlyphTiles(2));
 assert(storage[0].tiles==p && !PortControllerGlyphTiles(255));
 u8 sjis[]={26,2,26,1,0},sjisCopy[5],*jpNext=NULL;memcpy(sjisCopy,sjis,sizeof(sjis));
 loads=0;assert(LayoutCardMsgGlyphsPageSjis(100,200,sjis,&jpNext)==2 && !jpNext && loads==2 && releases==2);
 assert(storage[1].x==100+(12<<8) && sCardMsgGlyphCodes[0]==26);
 assert(!memcmp(sjisCopy,sjis,sizeof(sjis)));
 MsgLatinChar many[201];for(int i=0;i<200;i++)many[i]=32;many[200]=0;
 assert(LayoutCardMsgGlyphsPage(0,0,many,&next)==128 && next==many+128);
 MsgLatinChar highlighted[]={MSG_CODE_HIGHLIGHT,26,2,MSG_CODE_PLAIN,0};
 assert(LayoutCardMsgGlyphsPage(100,200,highlighted,&next)==1 && !next && storage[0].useAlternatePalette);
 assert(SDL_Init(SDL_INIT_GAMEPAD));SDL_VirtualJoystickDesc desc;SDL_INIT_INTERFACE(&desc);
 desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;
 SDL_JoystickID id=SDL_AttachVirtualJoystick(&desc);assert(id && SDL_OpenGamepad(id));
 SDL_Event event={0};event.type=SDL_EVENT_GAMEPAD_BUTTON_DOWN;event.gbutton.which=id;PortPromptHandleEvent(&event);
 gPortConfig.controllerPrompts=4;gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_SOUTH;
 MsgLatinChar live[]={PORT_PROMPT_SEMANTIC_TOKEN,1,0};
 assert(LayoutCardMsgGlyphsPage(123,456,live,&next)==1 && !next);
 assert(storage[0].tiles==PortControllerGlyphTiles(1));
 int paletteA,paletteB;storage[0].palette=&paletteA;storage[0].alternatePalette=&paletteB;
 next=live+1;MsgLatinChar* retainedNext=next;
 TextGlyphSprite prior=storage[0];uint64_t revision=PortControllerPromptRevision();int priorLoads=loads;
 for(int i=0;i<1000;i++){DrawCardMsgGlyphs(1);assert(PortControllerPromptRevision()==revision);}
 assert(drawn==1000 && storage[0].tiles==prior.tiles && gPortTextChar==0 && loads==priorLoads);
 gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_EAST;DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(2));
 gPortConfig.controllerPrompts=3;DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles('B'));
 gPortConfig.controllerPrompts=2;DrawCardMsgGlyphs(0);assert(storage[0].tiles==PortControllerGlyphTiles('A'));
 gPortConfig.padLayout=2;DrawCardMsgGlyphs(0);assert(storage[0].tiles==PortControllerGlyphTiles('B'));
 gPortConfig.padLayout=0;
 gPortConfig.controllerPrompts=1;DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles('?'));
 gPortConfig.controllerPrompts=0;DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles('A'));
 gPortConfig.controllerPrompts=4;gPortConfig.keys[PORT_ACT_A][0]=SDLK_X;
 event.type=SDL_EVENT_KEY_DOWN;event.key.key=SDLK_X;PortPromptHandleEvent(&event);
 DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles('X'));
 gPortConfig.keys[PORT_ACT_A][0]=SDLK_Q;DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles('?'));
 assert(storage[0].x==prior.x && storage[0].y==prior.y && storage[0].visible==prior.visible &&
        storage[0].useAlternatePalette==prior.useAlternatePalette && storage[0].palette==prior.palette &&
        storage[0].alternatePalette==prior.alternatePalette && next==retainedNext);
 event.type=SDL_EVENT_GAMEPAD_BUTTON_DOWN;event.gbutton.which=id;PortPromptHandleEvent(&event);
 failLoads=1;gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_SOUTH;DrawCardMsgGlyphs(1);
 assert(!storage[0].tiles);priorLoads=loads;DrawCardMsgGlyphs(1);assert(loads==priorLoads);
 failLoads=0;gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_EAST;DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(2) && next==retainedNext);
 SDL_CloseGamepad(SDL_GetGamepadFromID(id));DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles('?')); /* configured Q fallback, not stale Circle */
 assert(SDL_OpenGamepad(id));PortPromptHandleEvent(&event);
 gPortConfig.pad[PORT_ACT_L]=SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
 gPortConfig.pad[PORT_ACT_R]=SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
 MsgLatinChar scroll[]={PORT_PROMPT_SEMANTIC_TOKEN,67,0};
 assert(LayoutCardMsgGlyphsPage(123,456,scroll,&next)==1);
 gPortConfig.invertCardScroll=0;DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles(132));
 gPortConfig.invertCardScroll=1;DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles(133));
 u8 liveJp[]={PORT_PROMPT_SEMANTIC_TOKEN,1,0};
 assert(LayoutCardMsgGlyphsPageSjis(123,456,liveJp,&jpNext)==1 && !jpNext);
 gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_SOUTH;DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(1) && storage[0].x==123 && storage[0].y==456 && !jpNext);
 SDL_CloseGamepad(SDL_GetGamepadFromID(id));SDL_DetachVirtualJoystick(id);SDL_Quit();
 return 0;
}
'''
if __name__=='__main__':main()
