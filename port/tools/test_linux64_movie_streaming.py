"""Extract production Linux64 clock/pipe helpers; test clock locally and emit a POSIX harness."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'port/linux64/hdmovie_ffmpeg.inc'


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


CLOCK_CASES = r'''
#include <assert.h>
#include <math.h>
static void clock_cases(void) {
    double epoch = 0;
    assert(MovieClock(1,0,192000,96000,20,&epoch)==0.5);
    /* PCM EOF with half a second still queued: preserve position, then use wall time. */
    assert(MovieClock(1,1,192000,96000,20,&epoch)==0.5);
    assert(MovieClock(1,1,192000,0,20.5,&epoch)==1);
    assert(MovieClock(1,1,192000,0,29.5,&epoch)==10);
    /* A valid video with no audio samples must not accumulate increasing delays. */
    epoch=0;
    MovieClock(1,0,0,0,0,&epoch);
    double now=0,totalWait=0;
    for(int frame=0;frame<300;frame++) {
        double clock=MovieClock(1,1,0,0,now,&epoch),due=frame/30.0;
        if(due>clock){totalWait+=due-clock;now+=due-clock;}
    }
    assert(totalWait<10.01 && totalWait>9.9);
    epoch=5;
    assert(MovieClock(0,1,0,0,8,&epoch)==3);
}
'''
POSIX_CASES = r'''
#include <time.h>
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(int argc,char**argv){
    if(argc>1){
        if(!strcmp(argv[1],"--stalled")){for(;;)pause();}
        assert(write(1,"ab",2)==2);usleep(10000);assert(write(1,"cde",3)==3);
        if(!strcmp(argv[1],"--partial")){for(;;)pause();}
        return 0;
    }
    clock_cases();
    char* stalled[]={argv[0],"--stalled",NULL};
    MoviePipe p=MovieSpawn(stalled);assert(p.fd>=0 && p.pid>0);
    char out[10];
    double start=seconds();
    for(int events=0;events<20;events++){assert(MovieRead(&p,out,sizeof(out))==0);usleep(1000);}
    assert(seconds()-start<1); /* no data still allows event ticks */
    pid_t child=p.pid;start=seconds();MovieClose(&p);MovieClose(&p);
    assert(seconds()-start<1 && p.pid==-1 && p.fd==-1);
    assert(waitpid(child,NULL,WNOHANG)==-1 && errno==ECHILD);
    char* partial[]={argv[0],"--partial",NULL};
    p=MovieSpawn(partial);assert(p.fd>=0);
    assert(fcntl(p.fd,F_GETFL)&O_NONBLOCK);assert(fcntl(p.fd,F_GETFD)&FD_CLOEXEC);
    size_t used=0;start=seconds();
    while(used<5 && seconds()-start<2){ssize_t n=MovieRead(&p,out+used,5-used);assert(n>=0);used+=(size_t)n;usleep(1000);}
    assert(used==5 && !memcmp(out,"abcde",5));MovieClose(&p);
    char* complete[]={argv[0],"--complete",NULL};
    p=MovieSpawn(complete);MoviePipe other=MovieSpawn(stalled);used=0;int eof=0;start=seconds();
    while(seconds()-start<2){ssize_t n=MovieRead(&p,out+used,sizeof(out)-used);if(n==-1){eof=1;break;}assert(n>=0);used+=(size_t)n;usleep(1000);}
    assert(eof && used==5);MovieClose(&p);MovieClose(&other);
    char* absent[]={"/no-such-rekhcom-decoder",NULL};p=MovieSpawn(absent);assert(p.pid==-1 && p.fd==-1);MovieClose(&p);
    puts("PASS production Linux64: EOF clock, absent PCM, stalled/partial/EOF pipes, cancellation/reaping, exec failure");
    return 0;
}
'''


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--emit-posix',type=Path)
    args=parser.parse_args()
    source=SOURCE.read_text(encoding='utf-8')
    if 'static double MovieClock(' in source:
        clock=function(source,'static double MovieClock(')
    else:
        # Baseline expression, for an observed red regression before implementation.
        clock='static double MovieClock(int audio,int eof,long long bytes,int queued,double now,double* began){(void)eof;return audio?(bytes-queued)/192000.0:now-*began;}'
    if args.emit_posix:
        start=source.index('/* Each decoder is our direct child:')
        end=source.index('static double MovieClock(',start)
        headers='#define _GNU_SOURCE\n#include <stdio.h>\n#include <stdlib.h>\n#include <stdint.h>\n#include <string.h>\n#include <unistd.h>\n#include <errno.h>\n'
        args.emit_posix.parent.mkdir(parents=True,exist_ok=True)
        args.emit_posix.write_text(headers+source[start:end]+clock+CLOCK_CASES+POSIX_CASES)
        print(args.emit_posix)
        return
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory() as directory:
        folder=Path(directory)
        (folder/'clock.c').write_text(clock+CLOCK_CASES+'\nint main(void){clock_cases();return 0;}\n')
        subprocess.run([str(cc),'-Wall','-Wextra','-Werror',str(folder/'clock.c'),'-o',str(folder/'clock.exe')],check=True)
        subprocess.run([str(folder/'clock.exe')],check=True)
    print('PASS production Linux64 EOF clock: queued PCM continuity, 300 frames without PCM, no audio device')


if __name__=='__main__':main()
