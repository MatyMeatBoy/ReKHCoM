"""Regression: a Windows gallery poster must use the requested time, not seek preroll.

Synthetic local fixture, real production MF decoder; no ROM, window or audio.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from test_hd_poster_preview import function
ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-ref', help='compile production code from a Git ref, e.g. the pre-fix commit for RED')
    parser.add_argument('--out',type=Path,help='save the first decoded poster as visual evidence')
    parser.add_argument('--movie',type=Path,help='borrow a real movie through a private hard link')
    parser.add_argument('--provider', choices=('runtime','importer'), default='runtime')
    args = parser.parse_args()
    def read(path):
        if args.source_ref:
            return subprocess.check_output(['git','show',args.source_ref+':'+path],cwd=ROOT,text=True,encoding='utf-8')
        return (ROOT/path).read_text(encoding='utf-8')
    source = read('port/src/port_hdmovie.c')
    unit = r'''#define COBJMACROS
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <SDL3/SDL.h>
#include "port/port_hd_poster.h"
'''
    unit += function(source, 'static int ReadImportedPoster(') + '\n'
    unit += function(source, 'int PortHdScenePreviewFile(') + '\n'
    unit += r'''
int main(int argc,char**argv) {
    static uint32_t pixels[640*360];
    for(int cycle=0;cycle<8;cycle++) {
        assert(PortHdScenePreviewFile(argv[1],pixels,640,360,1));
        if(cycle==0 && argc>2) {
            SDL_Surface* s=SDL_CreateSurfaceFrom(640,360,SDL_PIXELFORMAT_ARGB8888,pixels,640*4);assert(s);
            assert(SDL_SavePNG(s,argv[2]));SDL_DestroySurface(s);
        }
        if(cycle==0) {
            char name[1600]; assert(PortHdPosterPath(argv[1],1,name,sizeof(name)));
            static uint32_t black[640*360]={0};
            SDL_Surface* s=SDL_CreateSurfaceFrom(640,360,SDL_PIXELFORMAT_ARGB8888,black,640*4);assert(s);
            assert(SDL_SavePNG(s,name));SDL_DestroySurface(s);
        }
        unsigned light=0;
        for(int i=0;i<640*360;i++) light += (pixels[i]&0xffffffu)!=0;
        if(light<200000) {fprintf(stderr,"FAIL: seek(1s) poster is black preroll (%u lit pixels), cycle %d\n",light,cycle);return 1;}
    }
    puts("PASS: repeated real MF poster decoding selects requested timestamp, not black keyframe");
    return 0;
}
'''
    if args.provider == 'importer':
        unit = r'''#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <SDL3/SDL.h>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <cassert>
#include "port/port_hd_poster.h"
namespace fs=std::filesystem;
static struct {bool cancel=false;} sJob;
static void JournalAdd(const std::string&s) {(void)s;}
'''
        unit += function(read('port/decompui/hd_import.inc'), 'static bool WriteMoviePoster(')
        unit += r'''
int main(int argc,char**argv) {
    assert(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)));
    assert(SUCCEEDED(MFStartup(MF_VERSION,MFSTARTUP_LITE)));
    char name[1600]; assert(PortHdPosterPath(argv[1],1,name,sizeof(name)));
    for(int cycle=0;cycle<8;cycle++) {
        if(cycle==1) {
            static uint32_t black[640*360]={0};
            SDL_Surface* s=SDL_CreateSurfaceFrom(640,360,SDL_PIXELFORMAT_ARGB8888,black,640*4);assert(s);
            assert(SDL_SavePNG(s,name));SDL_DestroySurface(s);
        }
        assert(WriteMoviePoster(fs::u8path(argv[1]),1));
        SDL_Surface* s=SDL_LoadPNG(name);assert(s);
        SDL_Surface* argb=SDL_ConvertSurface(s,SDL_PIXELFORMAT_ARGB8888);assert(argb);
        int visible=PortHdPosterHasPicture(argb->pixels,argb->w,argb->h,argb->pitch);
        SDL_DestroySurface(argb);SDL_DestroySurface(s);
        if(!visible){fprintf(stderr,"FAIL: importer persisted black preroll or reused black cache, cycle %d\n",cycle);return 1;}
    }
    MFShutdown();CoUninitialize();puts("PASS: native importer uses requested timestamp and rebuilds old black cache");return 0;
}
'''
    toolchain = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin'
    sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
    with tempfile.TemporaryDirectory(prefix='rekhcom-poster-ts-',dir=args.movie.parent if args.movie else None) as d:
        d=Path(d); fixture=d/'fixture.mp4'; src=d/('poster.cpp' if args.provider=='importer' else 'poster.c'); exe=d/'probe.exe'
        if args.movie: os.link(args.movie.resolve(), fixture)
        else: subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','color=black:s=640x360:r=30:d=0.5',
            '-f','lavfi','-i','color=red:s=640x360:r=30:d=2.5','-filter_complex','[0:v][1:v]concat=n=2:v=1:a=0[v]',
            '-map','[v]','-c:v','libx264','-g','300','-keyint_min','300','-sc_threshold','0','-pix_fmt','yuv420p',str(fixture)],check=True,
            creationflags=subprocess.CREATE_NO_WINDOW)
        src.write_text(unit)
        subprocess.run([str(toolchain/('i686-w64-mingw32-clang++.exe' if args.provider=='importer' else 'i686-w64-mingw32-clang.exe')),'-O1','-static','-std='+('c++17' if args.provider=='importer' else 'gnu11'),
            '-I'+str(sdl/'include'),'-I'+str(ROOT/'port/include'),str(src),'-o',str(exe),
            '-L'+str(sdl/'lib'),str(sdl/'lib/libSDL3.dll.a'),'-lmfplat','-lmfreadwrite','-lmfuuid','-lole32'],check=True)
        shutil.copy2(sdl/'bin/SDL3.dll',d/'SDL3.dll')
        if args.out: args.out.parent.mkdir(parents=True,exist_ok=True)
        r=subprocess.run([str(exe),str(fixture)]+([str(args.out.resolve())] if args.out else []),capture_output=True,text=True,env=dict(os.environ,SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy'),
            creationflags=subprocess.CREATE_NO_WINDOW)
        print(r.stdout+r.stderr,end='')
        r.check_returncode()

if __name__=='__main__':main()
