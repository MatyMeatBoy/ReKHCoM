"""Exercise real message initializers with every valid/NULL palette mix, without ROM or GPU.

Extract complete production functions to isolate their allocator/graphics dependencies.
The real types and BIOS_PTR macro are used; rendering services are inert stubs.
Removing a BIOS_PTR from glyph initialization/return or sysmsg initialization must fail.
"""
from pathlib import Path
import subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
def function(path,name):
    src=(ROOT/path).read_text(); start=src.rfind('\n',0,src.index(name+'('))+1
    brace=src.index('{',start); depth=1; end=brace+1
    while depth:
        depth += (src[end]=='{')-(src[end]=='}'); end+=1
    return src[start:end]
def main():
    compiler=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang.exe'
    pre=(ROOT/'src/card/card_sysmsgwin.c').read_text().split('static SysMsgWinWork*')[0]
    pre='\n'.join(line for line in pre.splitlines() if 'sprites_' not in line and 'sprite_palettes.h' not in line and 'common_text.h' not in line)
    unit='#include "macros.h"\n#undef STATIC_ASSERT\n#define STATIC_ASSERT(a,b)\n'+pre+'\n#include "msg.h"\n#include "malloc.h"\n#include <assert.h>\n#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n'
    unit+=r'''
const u16 gTextBrownPalette[16]={0},gTextWhitePalette[16]={0},gTextGrayPalette[16]={0},gBStatesPalette[16]={0};
u8 gFEventTiles[32]; void* gFEventAnims; void* gFEventFrames;
u16 gDialogBoxPalette[16],gCard00Palette[16];
u8 gHandCursorTiles[32],gMsgBoxTiles[32],gYesChoiceText[1],gNoChoiceText[1];
void* gHandCursorAnims; void* gHandCursorFrames;
const s32 gMsgwaitYesnoCursorY[2]={0};
u8 gPortBiosArea[0x10000];
static TextGlyphSprite* sCardMsgGlyphSprites;
static u8 sTextEntryCount;
static SysMsgWinWork* sActiveSysmsgwin;
static const s32 sSysmsgwinTextY[4]={0};
static ObjPalette valid;
static int mask, calls, fades, banks;
CardMessageDef gCardMessageDefs[1];
u8 gMessageWindowOpen,gMessageWindowAnswerYes;
const void* PortCardScrollTutorialText(int m,const void* p) { return p; }
u8 LayoutCardMsgGlyphsPage(s32 x,s32 y,MsgLatinChar* p,MsgLatinChar** n) { return 0; }
void* EwramAlloc(u32 n) { return calloc(1,n); }
ObjPalette* LoadObjPalette(const void* p,u16 n) { return mask & (1 << (calls++ % 3)) ? NULL : &valid; }
void* LoadTextPalette(s32 p) { return LoadObjPalette(NULL,32); }
void FadeSetPaletteExcluded(u16 slot,u8 on) { assert(slot==23 || slot==29 || slot==(u16)(0xe3a0+16)); fades++; }
void LoadObjPaletteBank(u16 bank,void* p) { assert(bank==7); banks++; }
u16 LoadTextSlots(const void* p,TextSlot* s) { return 0; }
ObjTiles* LoadObjTiles(const void* p,u16 n) { return AllocObjTiles(n,NULL); }
void SetTaskUpdate(Task* t,TaskUpdateFunc f) {}
u8 UpdateSysmsgwinChoiceInput(SysMsgWinWork* w,void* t) { return 1; }
u8 UpdateSysmsgwinChoice(SysMsgWinWork* w,void* t) { return 1; }
s32 UpdateSysmsgwinClose(SysMsgWinWork* w) { return 1; }
u16 GetKeysPressed(void) { return A_BUTTON; }
void m4aSongNumStart(u16 n) {}
void* AnimUpdate(AnimState* a) { return NULL; }
void ReleaseObjTiles(void* p) {}
void ReleaseObjPalette(ObjPalette* p) {}
void CpuSet(const void* a,void* b,u32 c) { memset(b,0,(c&0x1fffff)*4); }
void InitTextSlots(TextSlot* p,s32 n) {}
ObjTiles* AllocObjTiles(u16 n,const void* p) { static ObjTiles t; return &t; }
void SetObjTileSource(ObjTiles* t,const void* p) {}
void AnimInit(AnimState* a,void* b,void* c) {}
void AnimStart(AnimState* a,u16 b,u16 c) {}
void* AnimGetGfx(AnimState* a) { return NULL; }
void SetBgPriority(s32 a,u16 b) {}
'''
    unit+=function('src/msg/msg.c','InitCardMsgGlyphSprites')+'\n'
    unit+=function('src/card/card_sysmsgwin.c','sysmsgwin_0')+'\n'
    unit+=function('src/card/card_sysmsgwin.c','UpdateSysmsgwinChoiceSetup')+'\n'
    unit+=function('src/card/card_sysmsgwin.c','UpdateSysmsgwinWaitInput')+'\n'
    unit+=r'''
int main(void) {
 valid.index=7; ((ObjPalette*)gPortBiosArea)->index=13;
 for(mask=0;mask<8;mask++) {
   calls=fades=0; u16 result=InitCardMsgGlyphSprites(1,0);
   assert(result==((mask&1)?13:7)); assert(fades==256); free(sCardMsgGlyphSprites);
   for(int mode=0;mode<4;mode++) {
     SysMsgWinWork work; CardMessageArgs args={0}; args.mode=mode;
     calls=fades=0; sysmsgwin_0(&work,&args);
     assert(gMessageWindowOpen && !gMessageWindowAnswerYes);
     assert(fades==257+((mode==CARD_MESSAGE_MODE_BG_WINDOW || mode==CARD_MESSAGE_MODE_SPRITE_WINDOW)?1:0));
     free(sCardMsgGlyphSprites);
   }
 }
 ((ObjPalette*)gPortBiosArea)->index=0xe3a0;
 for(mask=0;mask<8;mask++) {
   SysMsgWinWork work={0}; calls=banks=0;
   UpdateSysmsgwinChoiceSetup(&work,NULL);
   assert(banks==((mask&1)?0:1)); assert(work.choiceVisible);
   memset(&work,0,sizeof(work)); gCardMessageDefs[0].flags=CARD_MSG_FLAG_CHOICE_AT_END;
   work.messageDef=&gCardMessageDefs[0]; calls=banks=0;
   UpdateSysmsgwinWaitInput(&work,NULL);
   assert(banks==((mask&1)?0:1)); assert(work.choiceVisible);
 }
 puts("message palettes: eight NULL mixes, four modes, two choice paths, BIOS 0xe3a0 and valid bank passed");
 return 0;
}
'''
    with tempfile.TemporaryDirectory() as d:
        p=Path(d); (p/'test.c').write_text(unit)
        cmd=[str(compiler),'-std=gnu11','-DPLATFORM_PC','-DVERSION_US','-ffunction-sections','-fdata-sections']
        for inc in ['port/include','include'] + [str(p.relative_to(ROOT)) for p in (ROOT/'include').rglob('*') if p.is_dir()]:
            cmd+=['-I'+str(ROOT/inc)]
        subprocess.run(cmd+[str(p/'test.c'),'-Wl,--gc-sections','-o',str(p/'test.exe')],check=True)
        subprocess.run([str(p/'test.exe')],check=True)
if __name__=='__main__': main()
