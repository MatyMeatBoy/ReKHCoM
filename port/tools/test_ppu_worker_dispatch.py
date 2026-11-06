"""A fast worker must not consume another slot's wakeup and erase its glyph batch.
Uses the production dispatch loop and worker body with a deterministic scheduler.
"""
from pathlib import Path
import subprocess,tempfile,re,os,shutil
R=Path(__file__).resolve().parents[2]
s=(R/'port/src/port_ppu.c').read_text(encoding='utf8')
start=s.index('static int SDLCALL PpuWorker(void* data) {')
worker=s[start:s.index('\nstatic void PpuStart',start)]
start=s.index('static void PpuRunBatch(')
dispatch=re.search(r'for \(i = 0; i < sPpuWorkers; i\+\+\)\s*SDL_SignalSemaphore\([^;]+;',s[start:])[0]
private='sPpuGo[i]' in dispatch
declaration=re.search(r'static SDL_Semaphore \*sPpuGo[^;]+;',s).group()
init='for(i=0;i<7;i++)sPpuGo[i]=&go[i];' if private else 'sPpuGo=&go[0];'
code='''#include <stdint.h>
#include <setjmp.h>
#include <assert.h>
#include <stdio.h>
#define SDLCALL
#define PPU_MAX_THREADS 8
typedef struct SDL_Semaphore { int tokens; } SDL_Semaphore;
static SDL_Semaphore go[7],done;
static int calls,sPpuWorkers=7;
static jmp_buf blocked;
static void SDL_WaitSemaphore(SDL_Semaphore* s) {
 if(!s->tokens)longjmp(blocked,1);
 s->tokens--;
}
static void SDL_SignalSemaphore(SDL_Semaphore* s){s->tokens++;}
static void PpuRows(int slot){assert(slot==0);calls++;}
'''+declaration+'\n'+worker+'''
int main(void){int i;
'''+init+'sPpuDone=&done;\n'+dispatch+'''
if(!setjmp(blocked))PpuWorker(0);
assert(calls==1); /* All other workers are deliberately delayed. */
puts("PASS actual PPU dispatch: each slot executes its batch exactly once");
return 0;}
'''
cc=os.environ.get('CC') or shutil.which('cc')
if not cc:cc=str(R.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'dispatch.c').write_text(code)
 subprocess.run([cc,str(p/'dispatch.c'),'-o',str(p/'dispatch.exe')],check=True)
 subprocess.run([str(p/'dispatch.exe')],check=True)
