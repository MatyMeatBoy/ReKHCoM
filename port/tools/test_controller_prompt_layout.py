"""Real Latin/SJIS message-layout consumers and immutable symbol-tile producer."""
from pathlib import Path
import os
import re
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
    widths=msg[msg.index('static const GlyphWidthTable sLatinGlyphWidths'):msg.index('#ifdef VERSION_EU',msg.index('static const GlyphWidthTable sLatinGlyphWidths'))]
    widths=[int(n) for n in re.findall(r'\b\d+\b',widths)]
    assert all(widths[ord(c)]>=5 for c in 'ABLRSELECT')
    inp=(ROOT/'port/src/port_input.c').read_text(encoding='utf8')
    activity=inp[inp.index('/* Activity is event-driven:'):inp.index('/* After closing an overlay')]
    tutorials=inp[inp.index('/* Keep the original localized instructions'):inp.index('/* Select + Up/Down:')]
    card=(ROOT/'include/card/card_message_data.h').read_text(encoding='utf8');card=card[card.index('enum CardMessageId {'):card.index('};',card.index('enum CardMessageId {'))+2]
    runtime=(ROOT/'port/include/port/controller_prompt_runtime.inc').read_text(encoding='utf8')
    core=activity+'\n'+tutorials+'\n'+runtime+'\n'+function(msg,'LayoutCardMsgGlyphsPage')+'\n'+function(msg,'LayoutCardMsgGlyphsPageSjis')+'\n'+function(msg,'DrawCardMsgGlyphs')+'\n'+function(msg,'FreeCardMsgGlyphSprites')
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
    sdl=next((ROOT.parent/'toolchain').glob('SDL3-*'))/'i686-w64-mingw32'
    with tempfile.TemporaryDirectory(prefix='rekhcom-prompt-layout-') as d:
        d=Path(d)
        for version in ('EU','US','JP'):
            src=d/'layout.c';exe=d/'layout.exe';src.write_text(PRE+card+'\n'+core+MAIN,encoding='utf8')
            subprocess.run([str(cc),'-std=gnu11','-Wall','-Wextra','-Werror','-DPLATFORM_PC','-DVERSION_'+version,'-I'+str(ROOT/'port/include'),'-I'+str(sdl/'include'),str(src),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(exe)],check=True)
            import shutil
            shutil.copy2(sdl/'bin/SDL3.dll',d/'SDL3.dll')
            run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=10,creationflags=0x08004000,env=dict(os.environ,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy'))
            assert run.returncode==0,run.stdout+run.stderr
    print('PASS: actual tutorial producer + EU/US/JP layout/draw/free; Original toggle, Return hotplug, SELECT continuation, OOM, balanced tile references and 1000 stable draws')

PRE=r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/controller_prompts.h"
#include "port/pad_layout.h"
#include "port/controller_prompt_glyphs.h"
#undef assert
#define assert(x) do {if(!(x)){fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
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
#ifdef VERSION_JP
typedef u8 TextChar;
#else
typedef MsgLatinChar TextChar;
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
static struct {const void*source;int references;} refs[1024];static int refCount;
static void* Hold(const void*p){for(int i=0;i<refCount;i++)if(refs[i].source==p){refs[i].references++;return (void*)p;}assert(refCount<1024);refs[refCount].source=p;refs[refCount++].references=1;return (void*)p;}
static void ReleaseObjPalette(void*p){(void)p;}
static void EwramFree(void*p){assert(p==storage);}

static void* LoadObjTiles(const void*p,int bytes){assert(p && bytes==128);loads++;return failLoads?NULL:Hold(p);}
static void ReleaseObjTiles(void*p){assert(p);releases++;for(int i=0;i<refCount;i++)if(refs[i].source==p){assert(refs[i].references>0);refs[i].references--;return;}assert(0);}
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
 const unsigned char* compact=PortControllerGlyphTiles(258);assert(compact==PortControllerGlyphTiles(258));
 for(int y=0;y<16;y++)for(int x=0;x<16;x++)if(x>=5||y>=10){int at=((y/8)*2+x/8)*32+(y%8)*4+(x%8)/2;assert(!((compact[at]>>((x&1)*4))&15));}

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

 static u16 frame[4]={0,0,0,0};for(int i=0;i<256;i++){gMsgLatinFontFrames[i]=frame;gMsgFontBank0Frames[i]=frame;sLatinGlyphWidths.widths[i]=6;}
 gPortConfig.controllerPrompts=0;gPortConfig.invertCardScroll=0;
#ifndef VERSION_JP
 const TextChar original[]={MSG_CODE_HIGHLIGHT,'A',' ','B','u','t','t','o','n',MSG_CODE_PLAIN,'.',0};
 MsgLatinChar* source=(MsgLatinChar*)PortCardScrollTutorialText(CARD_MSG_ROBE_TUTORIAL_00,original);
 assert(source==(MsgLatinChar*)original);
 assert(LayoutCardMsgGlyphsPage(123,456,source,&next)==9&&!next);
 void* originalTile=storage[0].tiles;int originalCode=sCardMsgGlyphCodes[0];TextGlyphSprite originalSprite=storage[0];
 gPortConfig.controllerPrompts=4;gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_SOUTH;DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(257));
 assert(storage[0].x==originalSprite.x&&storage[0].y==originalSprite.y&&storage[0].visible==originalSprite.visible);
 gPortConfig.controllerPrompts=0;DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==originalTile&&sCardMsgGlyphCodes[0]==originalCode);
 gPortConfig.controllerPrompts=4;failLoads=1;DrawCardMsgGlyphs(1);assert(!storage[0].tiles);
 int failedLoads=loads;for(int i=0;i<1000;i++)DrawCardMsgGlyphs(1);assert(loads==failedLoads);
 gPortConfig.controllerPrompts=0;DrawCardMsgGlyphs(1);assert(storage[0].tiles==originalTile&&loads==failedLoads);failLoads=0;

 int defaultLoads=loads;uint64_t defaultRevision=sCardPromptRevision;
 for(int i=0;i<1000;i++)DrawCardMsgGlyphs(1);
 assert(loads==defaultLoads&&sCardPromptRevision==defaultRevision);
 const TextChar select[]={MSG_CODE_HIGHLIGHT,'S','E','L','E','C','T',MSG_CODE_PLAIN,'.',0};
 source=(MsgLatinChar*)PortCardScrollTutorialText(CARD_MSG_ROBE_TUTORIAL_00,select);
 assert(LayoutCardMsgGlyphsPage(123,456,source,&next)==7&&!next);
 void* backups[6];for(int i=0;i<6;i++)backups[i]=storage[i].tiles;
 gPortConfig.pad[PORT_ACT_SELECT]=SDL_GAMEPAD_BUTTON_BACK;gPortConfig.controllerPrompts=4;DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(399));
 for(int i=1;i<6;i++)assert(!storage[i].tiles&&storage[i].visible);
 gPortConfig.controllerPrompts=0;DrawCardMsgGlyphs(1);for(int i=0;i<6;i++)assert(storage[i].tiles==backups[i]);

 /* Continuation offset of SELECT, without recomposing source or advancing VM. */
 assert(LayoutCardMsgGlyphsPage(123,456,source+2,&next)==6);
 gPortConfig.controllerPrompts=4;DrawCardMsgGlyphs(1);assert(storage[0].tiles==PortControllerGlyphTiles(399));
 gPortConfig.controllerPrompts=4;DrawCardMsgGlyphs(1);
 /* New page while overlay owns original references must restore/release correctly. */
 assert(LayoutCardMsgGlyphsPage(0,0,live,&next)==1);
 /* Initial unsupported keyboard binding retains a semantic cell before hotplug. */
 gPortConfig.keys[PORT_ACT_A][0]=SDLK_RETURN;event.type=SDL_EVENT_KEY_DOWN;event.key.key=SDLK_RETURN;PortPromptHandleEvent(&event);
 source=(MsgLatinChar*)PortCardScrollTutorialText(CARD_MSG_ROBE_TUTORIAL_00,original);
 assert(LayoutCardMsgGlyphsPage(123,456,source,&next)==2&&!next);
 assert(storage[0].tiles==PortControllerGlyphTiles('?'));
 event.type=SDL_EVENT_GAMEPAD_BUTTON_DOWN;event.gbutton.which=id;PortPromptHandleEvent(&event);DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(1));
#else
 const TextChar original[]={0x82,0x60,0x83,0x7b,0x83,0x5e,0x83,0x93,0};
 u8*source=(u8*)PortCardScrollTutorialText(CARD_MSG_ROBE_TUTORIAL_00,original);
 assert(source==original&&LayoutCardMsgGlyphsPageSjis(123,456,source,&jpNext)==4&&!jpNext);
 void* originalTile=storage[0].tiles;int originalCode=sCardMsgGlyphCodes[0];
 gPortConfig.controllerPrompts=4;gPortConfig.pad[PORT_ACT_A]=SDL_GAMEPAD_BUTTON_SOUTH;DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(257)&&storage[0].x==123&&storage[0].y==456);
 gPortConfig.controllerPrompts=0;DrawCardMsgGlyphs(1);assert(storage[0].tiles==originalTile&&sCardMsgGlyphCodes[0]==originalCode);
 const TextChar select[]={0x82,0x72,0x82,0x64,0x82,0x6b,0x82,0x64,0x82,0x62,0x82,0x73,0x83,0x7b,0x83,0x5e,0x83,0x93,0};
 source=(u8*)PortCardScrollTutorialText(CARD_MSG_ROBE_TUTORIAL_00,select);assert(LayoutCardMsgGlyphsPageSjis(123,456,source,&jpNext)==9);
 void* backups[6];for(int i=0;i<6;i++)backups[i]=storage[i].tiles;
 gPortConfig.pad[PORT_ACT_SELECT]=SDL_GAMEPAD_BUTTON_BACK;gPortConfig.controllerPrompts=4;DrawCardMsgGlyphs(1);
 for(int i=1;i<6;i++)assert(!storage[i].tiles&&storage[i].visible);
 gPortConfig.controllerPrompts=0;DrawCardMsgGlyphs(1);for(int i=0;i<6;i++)assert(storage[i].tiles==backups[i]);
 gPortConfig.controllerPrompts=4;DrawCardMsgGlyphs(1);

 gPortConfig.keys[PORT_ACT_A][0]=SDLK_RETURN;event.type=SDL_EVENT_KEY_DOWN;event.key.key=SDLK_RETURN;PortPromptHandleEvent(&event);
 source=(u8*)PortCardScrollTutorialText(CARD_MSG_ROBE_TUTORIAL_00,original);assert(LayoutCardMsgGlyphsPageSjis(123,456,source,&jpNext)==4);
 assert(storage[0].tiles==PortControllerGlyphTiles('?'));
 event.type=SDL_EVENT_GAMEPAD_BUTTON_DOWN;event.gbutton.which=id;PortPromptHandleEvent(&event);DrawCardMsgGlyphs(1);
 assert(storage[0].tiles==PortControllerGlyphTiles(1));
#endif
 FreeCardMsgGlyphSprites();assert(!sCardMsgGlyphSprites);
 for(int i=0;i<refCount;i++)assert(!refs[i].references);
 sCardMsgGlyphSprites=storage;memset(storage,0,sizeof(storage));

 SDL_CloseGamepad(SDL_GetGamepadFromID(id));SDL_DetachVirtualJoystick(id);SDL_Quit();
 return 0;
}
'''
if __name__=='__main__':main()
