"""Exercise the production partial-video-read consumer with real frame sizes.

The pipe is represented by controlled positive/empty/EOF reads. No decoder,
display, proprietary media, VM or long-running benchmark is required.
"""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'port/linux64/hdmovie_ffmpeg.inc').read_text(encoding='utf-8')
start = source.index('        if (frameUsed < frameBytes) {')
end = source.index('        double clock =', start)
consumer = source[start:end]
parser=argparse.ArgumentParser()
parser.add_argument('--emit-posix', type=Path)
args=parser.parse_args()
if args.emit_posix:
    helpers=source[source.index('/* Each decoder is our direct child:'):source.index('static double MovieClock(')]
    helpers=helpers.replace('static ssize_t MovieRead(', 'static ssize_t PipeRead(')
    helpers=helpers.replace('static void MovieWaitReadable(', 'static void PipeWaitReadable(')
    native=r'''
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern char** environ;
HELPERS
static size_t empty, waits, reads;
static ssize_t MovieRead(MoviePipe* p,void* data,size_t count) {
    ssize_t n=PipeRead(p,data,count);reads++;if(!n)empty++;return n;
}
static void MovieWaitReadable(MoviePipe* p){waits++;PipeWaitReadable(p);}
int main(int argc,char**argv) {
    const size_t frameBytes=1920*1080*4;
    if(argc>1) {
        uint8_t chunk[4096]={0};size_t left=frameBytes;
        while(left){size_t amount=left<sizeof(chunk)?left:sizeof(chunk);
            ssize_t n=write(1,chunk,amount);if(n<0&&errno==EINTR)continue;assert(n>0);left-=(size_t)n;}
        return 0;
    }
    char* command[]={argv[0],"--producer",NULL};
    MoviePipe video=MovieSpawn(command);assert(video.fd>=0);
    uint8_t* frame=malloc(frameBytes);assert(frame);
    size_t frameUsed=0,events=0,audio=0;int complete=0;
    for(size_t bound=0;bound<50000;bound++) {
        events++;audio++;
PRODUCTION
        complete=1;break;
    }
    MovieClose(&video);free(frame);
    printf("real pipe1080p: reads=%zu empty=%zu waits=%zu event/audio=%zu/%zu complete=%d\n",reads,empty,waits,events,audio,complete);
    assert(complete&&waits==empty&&events==reads&&audio==reads);
    puts("PASS real POSIX producer + production pipe/read and video fragment consumer");
}
'''.replace('HELPERS',helpers).replace('PRODUCTION',consumer)
    args.emit_posix.parent.mkdir(parents=True,exist_ok=True)
    args.emit_posix.write_text(native,encoding='utf-8')
    print(args.emit_posix)
    raise SystemExit(0)
harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
typedef long long ssize_t;
static size_t remaining, chunk, calls, waits, fixedSleeps, emptyEvery, events, audio;
static ssize_t MovieRead(void* pipe, void* bytes, size_t cap) {
    (void)pipe; (void)bytes; calls++;
    if (emptyEvery && calls % emptyEvery == 0) return 0;
    if (!remaining) return -1;
    size_t n=remaining<chunk?remaining:chunk;
    if(n>cap)n=cap;
    remaining-=n;
    return (ssize_t)n;
}
void SDL_Delay(int ms) {assert(ms==1); fixedSleeps++;}
void MovieWaitReadable(void*pipe) {(void)pipe;waits++;}
static void run(size_t frameBytes,size_t part,size_t empty,size_t available) {
    int video=0, complete=0;
    uint8_t* frame=malloc(frameBytes);assert(frame);size_t frameUsed=0;
    remaining=available; chunk=part; calls=waits=fixedSleeps=events=audio=0; emptyEvery=empty;
    for(size_t bound=0;bound<10000;bound++) {
        events++;audio++; /* these stay outside the production read block */
PRODUCTION
        complete=1;break;
    }
    assert(events==calls && audio==calls);
    assert(complete==(available>=frameBytes));
    /* Positive pipe progress must never incur an artificial millisecond wait. */
    if(fixedSleeps) {fprintf(stderr,"fixed sleep on pipe readiness: %zu times\n",fixedSleeps);exit(3);}
    if(waits!=(empty?calls/empty:0)) {
        fprintf(stderr,"positive reads incurred %zu artificial waits; chunk=%zu frame=%zu calls=%zu\n",waits,chunk,frameBytes,calls);
        exit(2);
    }
    if(complete)assert(frameUsed==frameBytes);
    free(frame);
}
int main(void) {
    for(size_t part=4096;part<=65536;part*=2) {
        run(1920*1080*4,part,0,1920*1080*4);
        run(1280*720*4,part,3,1280*720*4);
        run(1920*1080*4,part,0,part*2); /* partial frame then EOF */
    }
    run(16,16,0,16);run(16,8,2,16);
    puts("PASS17 fragment consumers: positive progress, EAGAIN waits, EOF, event/audio pumping");
}
'''.replace('PRODUCTION', consumer)
compiler = shutil.which('cc') or shutil.which('clang') or shutil.which('gcc')
if not compiler:
    compiler = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
flags={'creationflags':0x08004000} if os.name=='nt' else {}
with tempfile.TemporaryDirectory(prefix='rekhcom-movie-fragments-') as directory:
    out = Path(directory)
    (out / 'test.c').write_text(harness, encoding='utf-8')
    subprocess.run([str(compiler), '-std=c11', '-Wall', '-Wextra', '-Werror', str(out / 'test.c'), '-o', str(out / 'test.exe')], check=True, **flags)
    result=subprocess.run([str(out / 'test.exe')], capture_output=True, text=True, **flags)
    print(result.stdout+result.stderr, end='')
    assert result.returncode==0, result.returncode
