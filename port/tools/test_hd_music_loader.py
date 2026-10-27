"""Fault injection against the real hosted music loader and handoff functions."""
from pathlib import Path
import subprocess
import tempfile

R = Path(__file__).resolve().parents[2]


def main():
    source = (R / 'port/src/port_hdmusic.c').read_text(encoding='utf-8')
    start = source.index('static SDL_Thread* sLoader;')
    end = source.index('static void Close(void)', start)
    code = r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#define SDLCALL
#define SDL_snprintf snprintf
typedef int SDL_Thread;
typedef int SDL_AtomicInt;
typedef int SDL_IOStream;
typedef int64_t Sint64;
static int mode, opens, reads, closes, allocs, frees, worker, thread;
static int (*job)(void*);
static const char* kTracks[]={NULL,"one","two"};
static void MusicPath(char* out,size_t size,const char* track,const char* ext) {snprintf(out,size,"%s.%s",track,ext);}
static int SDL_SetAtomicInt(int* p,int n) {*p=n;return n;}
static int SDL_GetAtomicInt(int* p) {return *p;}
static void PortLog(const char* fmt,...) {(void)fmt;}
static void* SDL_malloc(size_t n) {allocs++;return mode==7?NULL:malloc(n);}
static void SDL_free(void* p) {if(p)frees++;free(p);}
static SDL_IOStream* SDL_IOFromFile(const char* p,const char* m) {if(!worker)exit(50);opens++;return mode==8?NULL:&thread;}
static Sint64 SDL_GetIOSize(SDL_IOStream* s) {return mode==1?((Sint64)1024*1024*1024*3):mode==4?0:mode==5?-1:mode==6?64*1024*1024+1:16;}
static size_t SDL_ReadIO(SDL_IOStream* s,void* p,size_t n) {reads++;memset(p,0,n);return mode==2?n-1:n;}
static int SDL_CloseIO(SDL_IOStream* s) {closes++;return 1;}
static void* SDL_LoadFile(const char* p,size_t* size) {if(!worker)exit(50);opens++;allocs++;*size=16;return malloc(16);}
static SDL_Thread* SDL_CreateThread(int (*fn)(void*),const char* n,void* u) {job=fn;return mode==3?NULL:&thread;}
static void SDL_WaitThread(SDL_Thread* t,int* status) {}
'''
    code += source[start:end] + r'''
int main(int argc,char** argv) {
    mode=atoi(argv[1]); size_t size=0;int pending=0;
    void* data=TakeLoaded(1,&size,&pending);
    if(data || opens || reads)return 1;
    if(mode==3)return pending || sLoaderSong!=-1 ? 2:0;
    if(!pending)return 3;
    worker=1;job(NULL);worker=0;
    if(mode==9) {
        data=TakeLoaded(2,&size,&pending);
        if(data || !pending || frees!=1 || sLoaderSong!=2)return 9;
        worker=1;job(NULL);worker=0;
        data=TakeLoaded(2,&size,&pending);
    } else data=TakeLoaded(1,&size,&pending);
    if(pending || sLoader || sLoaderSong!=-1)return 4;
    if(mode==0 || mode==9) {if(!data || size!=16 || reads!=(mode==9?2:1))return 5;SDL_free(data);}
    else {if(data)return 6;}
    if(mode==1 || mode==4 || mode==5 || mode==6) {if(allocs || reads)return 7;}
    if(mode!=8 && closes!=(mode==9?2:1))return 8;
    if(mode==2 && frees!=1)return 10;
    return 0;
}
'''
    cc = R.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe'
    with tempfile.TemporaryDirectory(prefix='rekhcom-music-loader-') as temp:
        src=Path(temp)/'test.c'; exe=Path(temp)/'test.exe'
        src.write_text(code,encoding='utf-8')
        subprocess.run([str(cc),str(src),'-o',str(exe)],check=True)
        failures=[]
        for mode,label in enumerate(('valid file','over INT_MAX','short read','thread failure without game-thread IO','empty file','unknown size','budget exceeded','allocation failure','open failure','obsolete song discarded')):
            result=subprocess.run([str(exe),str(mode)],timeout=2)
            if result.returncode:
                failures.append((label,result.returncode))
            else:
                print('PASS:',label)
        assert not failures,failures


if __name__=='__main__':
    main()
