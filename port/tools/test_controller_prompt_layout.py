"""Real Latin/SJIS message-layout consumers and immutable symbol-tile producer."""
from pathlib import Path
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
    core=function(inp,'PortControllerGlyphTiles')+'\n'+function(msg,'LayoutCardMsgGlyphsPage')+'\n'+function(msg,'LayoutCardMsgGlyphsPageSjis')
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-prompt-layout-') as d:
        d=Path(d)
        for version in ('EU','US'):
            src=d/'layout.c';exe=d/'layout.exe';src.write_text(PRE+core+MAIN,encoding='utf8')
            subprocess.run([str(cc),'-std=gnu11','-Wall','-Wextra','-Werror','-DPLATFORM_PC','-DVERSION_'+version,'-I'+str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True,timeout=10,creationflags=0x08004000)
    print('PASS: actual Latin/SJIS layout consumes one immutable 4bpp symbol per token, keeps positions and source bytes')

PRE=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include "port/controller_prompt_glyphs.h"
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
typedef struct {s32 x,y;void*tiles;int visible,useAlternatePalette;} TextGlyphSprite;
static TextGlyphSprite storage[128],*sCardMsgGlyphSprites=storage;
static u8 sTextEntryCount;
static u16 sCardMsgGlyphCodes[128];
static struct {u16 widths[256];}sLatinGlyphWidths;
static const u16* gMsgLatinFontFrames[256];
static const u16 *gMsgFontBank0Frames[256],*gMsgFontBank1Frames[256],*gMsgFontBank2Frames[256],*gMsgFontBank3Frames[256];
static u8 gMsgLatinFontTiles[128],gMsgFontBank0Tiles[128],gMsgFontBank1Tiles[128],gMsgFontBank2Tiles[128],gMsgFontBank3Tiles[128];
static int loads,releases;
static void* LoadObjTiles(const void*p,int bytes){assert(p && bytes==128);loads++;return(void*)p;}
static void ReleaseObjTiles(void*p){assert(p);releases++;}
static void GetSjisGlyph(u16 c,u16*g,u8*b){(void)c;*g=0;*b=0;}
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
 return 0;
}
'''
if __name__=='__main__':main()
