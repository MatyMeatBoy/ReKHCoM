"""Actual FPS sampler: restoring game time must not create a bogus Hz sample."""
from pathlib import Path
import os,shutil,subprocess,tempfile

root=Path(__file__).resolve().parents[2]
source=(root/'port/src/port_main.c').read_text(encoding='utf8')
def function(signature):
    a=source.index(signature)
    return source[a:source.index('\n}',a)+2]
timing=source[source.index('static uint64_t AdvanceFrameDeadline(void) {'):source.index('\nint PortPacedByVsync')]
reset=function('void PortFpsResetAfterStateLoad(void) {') if 'void PortFpsResetAfterStateLoad(void) {' in source else 'static void PortFpsResetAfterStateLoad(void) {}'
code='''#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#define GBA_FPS 59.7275
static uint64_t now=1000000,sPerfFreq=1000000,sNextFrame,sFpsTime;
static uint64_t sPresentPaceEpoch,sPresentWaitTotal;
static uint32_t sFpsFrames,sLogicLast;
static int sPacedByVsync;
static float sFps,sLogicFps;
volatile uint32_t gFrameCounter;
static uint64_t SDL_GetPerformanceCounter(void){return now;}
static const char* SDL_getenv(const char*n){(void)n;return NULL;}
static void SDL_DelayPrecise(uint64_t n){now+=n/1000;}
static void PortLog(const char* f,...){(void)f;}
'''+reset+'\n'+timing+'''
static void NextSecond(void){now+=1000000;gFrameCounter+=60;sFpsFrames+=60;Pace(0);}
int main(void){
gFrameCounter=600;sLogicLast=540;sFpsFrames=60;Pace(0);
assert(sFps==60 && sLogicFps==60);
/* Restore an earlier checkpoint after the first sample was established. */
gFrameCounter=20;PortFpsResetAfterStateLoad();
assert(sFps==60 && sLogicFps==60);NextSecond();
assert(sFps==60 && sLogicFps==60);
/* Restore a later state, including the initial boot-load case. */
gFrameCounter=6000;PortFpsResetAfterStateLoad();NextSecond();
assert(sFps==60 && sLogicFps==60);
/* An ordinary uint32 wrap remains a valid sixty-frame interval. */
gFrameCounter=0xfffffffeu;PortFpsResetAfterStateLoad();NextSecond();
assert(sFps==60 && sLogicFps==60);
puts("PASS actual FPS sampler: checkpoint rewind, forward load, uint32 wrap; prior displayed sample retained");
return 0;}
'''
cc=os.environ.get('CC') or shutil.which('cc') or str(root.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
with tempfile.TemporaryDirectory() as directory:
    p=Path(directory);(p/'fps.c').write_text(code)
    subprocess.run([cc,str(p/'fps.c'),'-o',str(p/'fps.exe')],check=True)
    subprocess.run([str(p/'fps.exe')],check=True)
