"""Compile the actual native importer poster helper against one read-only borrowed movie."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from test_hd_poster_preview import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--movie', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    original = hashlib.sha256(args.movie.read_bytes()).hexdigest()
    source = (ROOT / 'port/decompui/hd_import.inc').read_text(encoding='utf-8')
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
static struct {bool cancel=false;}sJob;
static std::vector<std::string> journal;
static void JournalAdd(const std::string&s){journal.push_back(s);}
'''
    unit += function(source, 'static bool WriteMoviePoster(') + '\n'
    unit += r'''
int main(int argc,char**argv){
    fs::path movie=fs::u8path(argv[1]);char name[2048];
    assert(PortHdPosterPath(movie.u8string().c_str(),1,name,sizeof(name)));
    fs::path poster=fs::u8path(name);
    HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);assert(SUCCEEDED(com));
    assert(SUCCEEDED(MFStartup(MF_VERSION,MFSTARTUP_LITE)));
    ULONGLONG start=GetTickCount64();assert(WriteMoviePoster(movie,1));
    printf("{\"initial_ms\":%llu,",(unsigned long long)(GetTickCount64()-start));fflush(stdout);
    start=GetTickCount64();assert(WriteMoviePoster(movie,1));
    printf("\"cached_ms\":%llu,",(unsigned long long)(GetTickCount64()-start));fflush(stdout);
    assert(journal.size()==1);
    SDL_Surface* image=SDL_LoadPNG(poster.u8string().c_str());assert(image);
    assert(image->w==640&&image->h==360);SDL_DestroySurface(image);
    std::ofstream(poster,std::ios::binary|std::ios::trunc)<<"corrupt cached poster exceeding 24 bytes";
    assert(WriteMoviePoster(movie,1));
    image=SDL_LoadPNG(poster.u8string().c_str());
    if(!image)return 77; /* a fresh but corrupt cache must be rebuilt */
    SDL_DestroySurface(image);
    sJob.cancel=true;assert(!WriteMoviePoster(movie,1));
    fs::copy_file(poster,fs::u8path(argv[2]),fs::copy_options::overwrite_existing);
    printf("\"corrupt_rebuilt\":true,\"cancel_ok\":true}\n");
    MFShutdown();CoUninitialize();return 0;
}
'''
    unit_path = args.out / 'native-import.cpp'
    unit_path.write_text(unit)
    sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
    compiler = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang++.exe'
    with tempfile.TemporaryDirectory(dir=args.out) as directory:
        directory = Path(directory)
        movie = directory / 'fixture.mp4'
        os.link(args.movie, movie)  # no video bytes copied; unlinked with this temporary fixture
        exe = directory / 'probe.exe'
        subprocess.run([str(compiler), '-std=c++17', '-O1', '-static', '-I' + str(sdl / 'include'),
            '-I' + str(ROOT / 'port/include'), str(unit_path), '-o', str(exe), '-L' + str(sdl / 'lib'),
            str(sdl / 'lib/libSDL3.dll.a'), '-lmfplat', '-lmfreadwrite', '-lmfuuid', '-lole32'], check=True)
        shutil.copy2(sdl / 'bin/SDL3.dll', directory / 'SDL3.dll')
        result = subprocess.run([str(exe), str(movie), str(args.out / 'native-import-poster.png')],
            capture_output=True, text=True, timeout=90, check=True)
    assert hashlib.sha256(args.movie.read_bytes()).hexdigest() == original
    report = json.loads(result.stdout)
    (args.out / 'native-import.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
