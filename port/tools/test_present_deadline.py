"""Actual Pace + production cycle ordering: work jitter must not become image jitter."""
from pathlib import Path
import subprocess,tempfile,shutil,os,re
R=Path(__file__).resolve().parents[2]
s=(R/'port/src/port_main.c').read_text(encoding='utf8')
new='static uint64_t AdvanceFrameDeadline(void) {' in s
start=s.index('static uint64_t AdvanceFrameDeadline(void) {' if new else 'static void Pace(void) {');end=s.index('\nint PortPacedByVsync',start)
pace=s[start:end]
start=s.index('static void VBlankCycle(void) {')
cycle=re.sub(r'/\*.*?\*/','',s[start:s.index('\nvoid ',start)],flags=re.S)
if new:
 present=s[s.index('static void Present('):s.index('static uint64_t AdvanceFrameDeadline(void) {')]
 assert present.index('PaceBeforePresent();')<present.index('int presented=SDL_RenderPresent(sRenderer);')
 assert 'if (paced) PaceBeforePresent();' in present
 assert 'Present(1);' in cycle
 assert 'Pace(sPresentPaceEpoch == presentPaceStart);' in cycle
 order='PaceBeforePresent();Show();Pace(1);'
else:
 before=cycle.index('Pace();')<cycle.index('Present();')
 order='Pace();Show();' if before else 'Show();Pace();'
pace_call='Pace(1)' if new else 'Pace()'
extra='''
/* A skipped draw still consumes exactly one period. */
{uint64_t previous=sNextFrame;int d=delays;clockNow+=3000;Pace(1);
 assert(sNextFrame==previous+period && CLOCK_ON_TARGET && delays==d+1 && imageCount==6);}
/* A failed presentation already waited; the cycle must not wait twice. */
{uint64_t previous=sNextFrame;int d=delays;clockNow+=3000;PaceBeforePresent();Pace(1);
 assert(sNextFrame==previous+period && delays==d+1 && imageCount==6);}
/* Menus can present repeatedly within a game cycle. Closing one adds no idle period. */
PaceBeforePresent();Show();Pace(1);PaceBeforePresent();Show();Pace(1);
assert(GAP_ON_TARGET(images[7]-images[6]));
{uint64_t previous=sNextFrame;int d=delays;Pace(0);assert(sNextFrame==previous && delays==d);}
''' if new else ''
code='''#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#define GBA_FPS 59.727500569606
static uint64_t clockNow=1000,sPerfFreq=1000000,sNextFrame,sFpsTime;
static uint32_t sFpsFrames,sLogicLast;
static uint64_t sPresentPaceEpoch,sPresentWaitTotal;
volatile uint32_t gFrameCounter;
static float sFps,sLogicFps;
static int sPacedByVsync,fast,delays;
static uint64_t images[10];static int imageCount;
static uint64_t SDL_GetPerformanceCounter(void){return clockNow;}
static const char* SDL_getenv(const char* s){return fast && s[6]=='F' ? "1" : NULL;}
static void SDL_DelayPrecise(uint64_t ns){clockNow+=(ns+999)/1000;delays++;}
#ifdef __EMSCRIPTEN__
static void emscripten_sleep(unsigned ms){clockNow+=(uint64_t)ms*1000;if(ms)delays++;}
#define CLOCK_ON_TARGET (clockNow>=sNextFrame && clockNow-sNextFrame<1000)
#define GAP_ON_TARGET(d) (((d)>period ? (d)-period : period-(d))<1000)
#else
#define CLOCK_ON_TARGET (clockNow==sNextFrame)
#define GAP_ON_TARGET(d) ((d)==period)
#endif
static void PortLog(const char* f,...){(void)f;}
'''+pace+'''
static void Show(void){images[imageCount++]=clockNow;}
static void Frame(uint64_t work){clockNow+=work;gFrameCounter++;
'''+order+'''
}
int main(void){int i;uint64_t period=(uint64_t)(sPerfFreq/GBA_FPS);
static const unsigned work[]={3000,11000,6000,9000,2000,10000};
for(i=0;i<6;i++)Frame(work[i]);
for(i=1;i<6;i++)assert(GAP_ON_TARGET(images[i]-images[i-1]));
assert(delays==6);
EXTRA_CASES
clockNow=sNextFrame+5*period;
{int d=delays;PACE_CALL;assert(delays==d && sNextFrame==clockNow);}
fast=1;{int d=delays;PACE_CALL;assert(delays==d && sNextFrame==0);}
puts("PASS actual pacing/order: variable render cost, deadline cadence, stall recovery and unpaced override");return 0;}
'''
code=code.replace('PACE_CALL',pace_call).replace('EXTRA_CASES',extra)
cc=os.environ.get('CC') or shutil.which('cc')
if not cc:cc=str(R.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'deadline.c').write_text(code)
 for target,flags in [('native',[]),('web-yield',['-D__EMSCRIPTEN__'])]:
  exe=p/(target+'.exe')
  subprocess.run([cc,*flags,str(p/'deadline.c'),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
