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
#define PORT_PAD_AXIS_BASE 100
#include "port/controller_prompts.h"
int main(void){
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
 return 0;
}
''',encoding='utf8')
        subprocess.run([str(cc),'-std=c99','-Wall','-Wextra','-Werror','-I'+str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=10)
    print('PASS: effective button position/family, Nintendo normalization, custom/axis bindings, exact semantic tokens')

if __name__=='__main__':main()
