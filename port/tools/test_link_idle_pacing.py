"""Network receive/poll cadence must stay independent of image deadlines."""
from pathlib import Path
import subprocess,tempfile,os,shutil
R=Path(__file__).resolve().parents[2]
s=(R/'port/src/port_main.c').read_text(encoding='utf8')
start=s.index('void PortLinkIdle(void) {')
stop=s.index('\n/*',start)
body=s[start:stop].replace('Present(','MockPresent(').replace('MockPresent();','MockPresent(1);')
code='''#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#define GBA_FPS 59.727500569606
static uint64_t sPerfFreq=1000000,now=100000;
static int draws,polls,blocking;
static uint64_t SDL_GetPerformanceCounter(void){return now;}
static const char* SDL_getenv(const char* s){(void)s;return NULL;}
static void MockPresent(int paced){draws++;if(paced){blocking++;now+=16742;}}
static void PumpEvents(void){polls++;}
static void SDL_Delay(unsigned ms){now+=ms*1000;}
'''+body+'''
int main(void){int i;uint64_t start=now;
for(i=0;i<1000;i++)PortLinkIdle();
assert(polls==1000 && now-start==1000000 && !blocking);
assert(draws>=58 && draws<=61);
puts("PASS actual network idle:1000 nonblocking receive opportunities with bounded image refresh");return 0;
}
'''
cc=os.environ.get('CC') or shutil.which('cc')
if not cc:cc=str(R.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'link.c').write_text(code)
 subprocess.run([cc,str(p/'link.c'),'-o',str(p/'link.exe')],check=True)
 subprocess.run([str(p/'link.exe')],check=True)
