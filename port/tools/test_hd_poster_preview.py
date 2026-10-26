"""Small production-decoder probe; borrows one movie read-only and retains no executable/ROM copies."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    end, depth = brace + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--movie', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--baseline', action='store_true')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'port/src/port_hdmovie.c').read_text()
    prefix = r'''#define COBJMACROS
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <psapi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <SDL3/SDL.h>
'''
    if args.baseline:
        prefix += 'static void MoviePath(const char*n,char*p,size_t z){snprintf(p,z,"%s",n);}\n'
        prefix += function(source, 'int PortHdScenePreview(') + '\n'
        entry = 'PortHdScenePreview'
    else:
        assert 'int PortHdScenePreviewFile(' in source, 'preview needs a path-based worker API'
        prefix += '#include "port/port_hd_poster.h"\n'
        prefix += function(source, 'static int ReadImportedPoster(') + '\n'
        prefix += function(source, 'int PortHdScenePreviewFile(') + '\n'
        prefix += '#define TEST_IMPORTED_POSTER 1\n'
        entry = 'PortHdScenePreviewFile'
    prefix += r'''
int main(int argc,char**argv){
    uint32_t* pixels=SDL_malloc(640*360*4);assert(pixels);
#ifdef TEST_IMPORTED_POSTER
    if(argc>2){
        char movie[1400],image[1600];
        snprintf(movie,sizeof(movie),"%s/fixture.mp4",argv[2]);
        FILE*f=fopen(movie,"wb");assert(f);fputs("not a video",f);fclose(f);
        assert(PortHdPosterPath(movie,1,image,sizeof(image)));
        for(int y=0;y<360;y++)for(int x=0;x<640;x++)pixels[y*640+x]=0xFF00002Au|(x&255)<<16|(y&255)<<8;
        SDL_Surface*s=SDL_CreateSurfaceFrom(640,360,SDL_PIXELFORMAT_ARGB8888,pixels,640*4);assert(s);
        assert(SDL_SavePNG(s,image));SDL_DestroySurface(s);
        for(int i=0;i<3;i++){
            memset(pixels,0,640*360*4);ULONGLONG start=GetTickCount64();
            assert(ENTRY(movie,pixels,640,360,1));
            assert(pixels[42*640+20]==(0xFF00002Au|20u<<16|42u<<8));
            PROCESS_MEMORY_COUNTERS m={sizeof(m)};GetProcessMemoryInfo(GetCurrentProcess(),&m,sizeof(m));
            printf("{\"cached_call\":%d,\"ok\":1,\"ms\":%llu,\"peak_kib\":%llu}\n",i,
                (unsigned long long)(GetTickCount64()-start),(unsigned long long)(m.PeakWorkingSetSize/1024));
        }
        assert(!ENTRY(movie,pixels,855,360,1));assert(!ENTRY(movie,pixels,640,481,1));
        assert(!ENTRY(movie,NULL,640,360,1));assert(!ENTRY(movie,pixels,640,360,-1));
        /* Wrong offset cannot accidentally use the still of another segment. */
        assert(!ENTRY(movie,pixels,640,360,2));
        HANDLE h=CreateFileA(movie,FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
        assert(h!=INVALID_HANDLE_VALUE);FILETIME future;GetSystemTimeAsFileTime(&future);
        ULARGE_INTEGER t;t.LowPart=future.dwLowDateTime;t.HighPart=future.dwHighDateTime;t.QuadPart+=100000000;
        future.dwLowDateTime=t.LowPart;future.dwHighDateTime=t.HighPart;assert(SetFileTime(h,NULL,NULL,&future));CloseHandle(h);
        assert(!ENTRY(movie,pixels,640,360,1)); /* stale PNG is ignored */
        SDL_free(pixels);return 0;
    }
#endif
#ifdef TEST_IMPORTED_POSTER
    HRESULT com=E_FAIL; /* the production worker API must initialize its own COM */
#else
    HRESULT com=CoInitializeEx(NULL,COINIT_MULTITHREADED);
#endif
    for(int i=0;i<3;i++){
        ULONGLONG start=GetTickCount64();
        int ok=ENTRY(argv[1],pixels,640,360,1);
        PROCESS_MEMORY_COUNTERS memory={sizeof(memory)};
        GetProcessMemoryInfo(GetCurrentProcess(),&memory,sizeof(memory));
        printf("{\"call\":%d,\"ok\":%d,\"ms\":%llu,\"peak_kib\":%llu}\n",i,ok,
            (unsigned long long)(GetTickCount64()-start),(unsigned long long)(memory.PeakWorkingSetSize/1024));
        assert(ok);
    }
    SDL_free(pixels);if(SUCCEEDED(com))CoUninitialize();return 0;
}
'''.replace('ENTRY', entry)
    unit = args.out / ('baseline.c' if args.baseline else 'poster.c')
    unit.write_text(prefix)
    toolchain = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin'
    sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
    with tempfile.TemporaryDirectory(dir=args.out) as directory:
        directory = Path(directory)
        exe = directory / 'probe.exe'
        subprocess.run([str(toolchain / 'i686-w64-mingw32-clang.exe'), '-O1', '-std=gnu11',
            '-I' + str(sdl / 'include'), '-I' + str(ROOT / 'port/include'), str(unit), '-o', str(exe),
            '-L' + str(sdl / 'lib'), '-lSDL3', '-lmfplat', '-lmfreadwrite', '-lmfuuid', '-lole32', '-lpsapi'], check=True)
        shutil.copy2(sdl / 'bin/SDL3.dll', directory / 'SDL3.dll')
        result = subprocess.run([str(exe), str(args.movie.resolve())], capture_output=True, text=True,
            timeout=90, env=dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy'), check=True)
        cached = None
        if not args.baseline:
            cached = subprocess.run([str(exe), str(args.movie.resolve()), str(directory)], capture_output=True,
                text=True, timeout=90, env=dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy'), check=True)
    rows = [json.loads(line) for line in result.stdout.splitlines() if line.startswith('{')]
    if cached:
        rows += [json.loads(line) for line in cached.stdout.splitlines() if line.startswith('{')]
    (args.out / ('baseline.json' if args.baseline else 'poster.json')).write_text(json.dumps(rows, indent=2))
    print(json.dumps(rows, indent=2))


if __name__ == '__main__':
    main()
