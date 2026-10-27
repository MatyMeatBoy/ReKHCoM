"""Pure binding-to-prompt contract: positions, family, rebinding and exact tokens."""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]

def main():
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-prompts-') as folder:
        src=Path(folder)/'test.c';exe=Path(folder)/'test.exe'
        src.write_text('''#include <assert.h>
#include <string.h>
#include <windows.h>
#define PORT_PAD_AXIS_BASE 100
#include "port/controller_prompts.h"
#include "port/controller_prompt_glyphs.h"
int main(void){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
 assert(!strcmp(KhPromptButtonLabel(0,KH_PROMPT_PLAYSTATION,0),"Cross"));
 assert(!strcmp(KhPromptButtonLabel(1,KH_PROMPT_PLAYSTATION,0),"Circle"));
 assert(!strcmp(KhPromptButtonLabel(0,KH_PROMPT_NINTENDO,1),"A"));
 assert(!strcmp(KhPromptButtonLabel(1,KH_PROMPT_NINTENDO,1),"B"));
 assert(!strcmp(KhPromptButtonLabel(0,KH_PROMPT_XBOX,1),"B"));
 assert(!strcmp(KhPromptButtonLabel(0,KH_PROMPT_GENERIC,0),"South"));
 assert(!strcmp(KhPromptButtonLabel(108,KH_PROMPT_PLAYSTATION,0),"L2"));
 assert(!strcmp(KhPromptButtonLabel(110,KH_PROMPT_NINTENDO,1),"ZR"));
 assert(!strcmp(KhPromptButtonLabel(-1,KH_PROMPT_XBOX,0),"-"));
 assert(!strcmp(KhPromptButtonLabel(999,KH_PROMPT_XBOX,0),"?"));
 /* Exact highlighted tokens only: no replacing letters inside prose. */
 assert(KhPromptTokenAction("A",1)==0);
 assert(KhPromptTokenAction("START",5)==4);
 assert(KhPromptTokenAction("SELECT",6)==5);
 assert(KhPromptTokenAction("Attack",6)==-1);
 assert(KhPromptTokenAction("A button",8)==-1);
 assert(KhPromptTokenAction("Riku",4)==-1);
 assert(KhPromptTokenAction("A Button",8)==0);
 assert(KhPromptTokenAction("Bouton B ",9)==1);
 assert(KhPromptTokenAction("Pulsante L",10)==2);
 assert(KhPromptTokenAction("R-Taste",7)==3);
 assert(KhPromptTokenAction("",0)==-1);
 int actions[2];size_t bytes=0;
 int confirm=99,cancel=99;
 assert(KhPromptPresetBindings(KH_PROMPT_PLAYSTATION,0,&confirm,&cancel) && confirm==1 && cancel==0);
 assert(KhPromptPresetBindings(KH_PROMPT_NINTENDO,1,&confirm,&cancel) && confirm==0 && cancel==1);
 assert(!KhPromptPresetBindings(KH_PROMPT_GENERIC,0,&confirm,&cancel) && confirm==0 && cancel==1);
 assert(KhPromptTokenActions("L and R Buttons",15,actions)==2 && actions[0]==2 && actions[1]==3);
 assert(KhPromptTokenActions("R+L",3,actions)==2 && actions[0]==3 && actions[1]==2);
 const unsigned char jp[]={0x82,0x6b,0x81,0x45,0x82,0x71,0x83,0x7b,0x83,0x5e,0x83,0x93};
 assert(KhPromptSjisActions(jp,sizeof(jp),actions,&bytes)==2 && bytes==6 && actions[0]==2 && actions[1]==3);
 assert(!KhPromptSjisActions(jp,5,actions,&bytes));
 assert(KhPromptGlyphId("Cross")==1);
 assert(KhPromptGlyphId("Circle")==2);
 assert(KhPromptGlyphId("A")=='A');
 assert(KhPromptGlyphId("unsupported")==0);
 for(int id=1;id<256;id++){
  unsigned char pixels[128];KhPromptGlyphPixels(id,pixels);
  for(int i=0;i<128;i++)assert((pixels[i]&15)<=2 && (pixels[i]>>4)<=2);
 }
 unsigned char pixels[128];KhPromptGlyphPixels(1,pixels);int ink=0;
 for(int i=0;i<128;i++)ink+=pixels[i]!=0;assert(ink>10);
 return 0;
}
''',encoding='utf8')
        subprocess.run([str(cc),'-std=c99','-Wall','-Wextra','-Werror','-I'+str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=10)
    print('PASS: effective button position/family, Nintendo normalization, custom/axis bindings, exact semantic tokens')

if __name__=='__main__':main()
