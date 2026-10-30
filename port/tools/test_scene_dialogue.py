"""Execute the original message draw consumer with controlled rendering seams."""
import argparse
import re
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <stdio.h>
#define PLATFORM_PC
#define MSG_WIN_POSITION_TOP_FACE_RIGHT 0
#define MSG_WIN_POSITION_BOTTOM_FACE_RIGHT 1
#define MSG_WIN_POSITION_TOP_FACE_LEFT 2
#define MSG_WIN_POSITION_BOTTOM_FACE_LEFT 3
#define PORTRAIT_NONE -1
typedef struct {int portraitId;} MessageScriptEntry;
typedef struct {MessageScriptEntry* script;int scriptIndex,shownChars,bg,tasks;void*palette;} MsgWinWork;
static struct {int msgWinPosition,msgWinCentered;} state;
static void* nothing;
static typeof(state)*gEventState=&state;
static int active,poster,depth,bgScreen,disabled,gPortSpriteShiftX,glyphShift,glyphDepth,taskDepth;
static int PortSceneFramingActive(void){return active;}
static int PortScenePosterRendering(void){return poster;}
static int PortWideMarginX(void){return 40;}
static int PortScreenMarginX(void){return 120;}
static void PortScreenBg(int bg,int on){(void)bg;bgScreen=on;}
static void PortScreenBegin(void){depth++;}
static void PortScreenEnd(void){depth--;}
static void DisableBg(int bg){(void)bg;disabled=1;}
static void PortDockBg(int bg,int shift){(void)bg;(void)shift;}
static void DrawMsgGlyphs(int chars){(void)chars;glyphShift=gPortSpriteShiftX;glyphDepth=depth;}
static void DrawMsgGlyphsWithPalette(int chars,void* palette){(void)palette;DrawMsgGlyphs(chars);}
static void TaskPoolDraw(int* tasks){(void)tasks;taskDepth=depth;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
FUNCTIONS
int main(void){
    MessageScriptEntry entry={1};MsgWinWork work={&entry,0,9,2,0,0};work.palette=nothing;
    active=1;state.msgWinPosition=MSG_WIN_POSITION_BOTTOM_FACE_RIGHT;msgwin_2(&work);
    CHECK(bgScreen==1&&glyphShift==120&&glyphDepth==1&&taskDepth==1&&depth==0&&gPortSpriteShiftX==0);
    state.msgWinPosition=MSG_WIN_POSITION_TOP_FACE_LEFT;msgwin_2(&work);CHECK(glyphShift==-120);
    active=0;msgwin_2(&work);CHECK(bgScreen==0&&glyphShift==-40&&glyphDepth==0&&taskDepth==0);
    active=1;entry.portraitId=PORTRAIT_NONE;msgwin_2(&work);CHECK(bgScreen==1&&glyphShift==0&&glyphDepth==1);
    poster=1;disabled=0;msgwin_2(&work);CHECK(disabled&&depth==0);
    puts("PASS production dialogue consumer: box/portrait/text together, own canvas, left/right/center, extended and poster isolation");
    return 0;
}
'''
def function(source,name):
    match=re.search(re.escape(name)+r'[^;{}]*\{',source)
    if not match:raise ValueError(name)
    start=match.start();brace=source.index('{',start);depth=0
    for i in range(brace,len(source)):
        depth+=(source[i]=='{')-(source[i]=='}')
        if depth==0:return source[start:i+1]
    raise ValueError(name)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cc',required=True);a=ap.parse_args()
    source=(ROOT/'src/evt/event_message.c').read_text(encoding='utf-8')
    extracted='\n'.join(function(source,name) for name in ('static int MsgwinDockShift(void)','static void msgwin_2('))
    with tempfile.TemporaryDirectory(prefix='scene-dialogue-') as tmp:
        src,exe=Path(tmp)/'test.c',Path(tmp)/'test.exe';src.write_text(CODE.replace('FUNCTIONS',extracted))
        subprocess.run([a.cc,'-std=gnu11','-static',str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
