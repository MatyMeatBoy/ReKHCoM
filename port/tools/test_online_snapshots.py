"""Exercise the production rollback allocator and modern failure paths."""
import argparse
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "port/port_online.h"
static int engine,error,allocations,failAt=-1;
static uint16_t io;
static char game[32];
static void* TestMalloc(size_t n){if(allocations++==failAt)return 0;return malloc(n);}
#define malloc TestMalloc
#include "port/src/port_rollback.c"
#undef malloc
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int PortOnlineModern(void){return engine;}
int PortOnlineError(void){return error;}
void PortOnlineFail(int e){error=e;}
void PortLog(const char* f,...){(void)f;}
void PortGameSections(char** d,size_t* ds,char** b,size_t* bs){*d=game;*ds=16;*b=game+16;*bs=16;}
void PortDmaVisitState(PortStateVisitor v){(void)v;}
void PortPpuVisitState(PortStateVisitor v){(void)v;}
void PortAudioVisitState(PortStateVisitor v){(void)v;}
int NetRollbackSession(void){return 1;}
uint32_t NetLinkFrame(void){return 10;}
uint32_t NetLinkMade(void){return 10;}
uint32_t NetTakeRollback(void){return 1;}
int PortNetlinkTrace(void){return 0;}
static void cleanup(void){
    for(unsigned i=0;i<64;i++){free(sSnap[i]);sSnap[i]=0;}
    allocations=0;sFailed=0;sSession=0;error=0;
}
int main(void){
    engine=0;CHECK(PortRollbackReserve());CHECK(allocations==32);cleanup();
    engine=1;CHECK(PortRollbackReserve());CHECK(allocations==64);cleanup();
    failAt=3;CHECK(!PortRollbackReserve());CHECK(error==MODERN_ERR_STATE);
    for(unsigned i=0;i<64;i++)CHECK(!sSnap[i]);cleanup();failAt=-1;
    CHECK(PortRollbackReserve());PortRollbackBoundary(1);
    CHECK(error==MODERN_ERR_HISTORY);CHECK(!PortRollbackResimulating());cleanup();
    return 0;
}
'''
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cc',required=True);args=ap.parse_args()
    with tempfile.TemporaryDirectory(prefix='online-snapshots-') as tmp:
        w=Path(tmp);(w/'port').mkdir();(w/'SDL3').mkdir()
        (w/'SDL3/SDL.h').write_text('#include <stdlib.h>\nstatic const char* SDL_getenv(const char* n){(void)n;return 0;}\n#define SDL_atoi atoi\n')
        (w/'port/port_state.h').write_text('#ifndef TEST_STATE_H\n#define TEST_STATE_H\n#include <stddef.h>\ntypedef void (*PortStateVisitor)(void*,size_t);\nvoid PortGameSections(char**,size_t*,char**,size_t*);\nvoid PortDmaVisitState(PortStateVisitor);\nvoid PortPpuVisitState(PortStateVisitor);\nvoid PortAudioVisitState(PortStateVisitor);\n#endif\n')
        (w/'port/port.h').write_text('#include <stdint.h>\n#include <stddef.h>\nvoid PortLog(const char*,...);\n#define IO16(x) io\n'+''.join('#define GBA_'+name+' 0\n#define GBA_'+name+'_SIZE 16\n' for name in ('EWRAM','IWRAM','IO','PLTT','VRAM','OAM')))
        src,exe=w/'test.c',w/'test.exe';src.write_text(CODE)
        subprocess.run([args.cc,'-std=c99','-Wall','-Wextra','-Werror','-static','-I',str(w),
                        '-I',str(ROOT),'-I',str(ROOT/'port/include'),str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print('PASS production snapshots: classic32/modern64, allocation failure, too-old correction fails explicitly')
if __name__=='__main__':main()
