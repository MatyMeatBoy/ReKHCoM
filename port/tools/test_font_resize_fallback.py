"""Exercise the actual guest FontWithCap producer with allocation refusals.

Uses CPU stubs for font metrics/allocation; does not certify console hardware.
Run with --out DIR to retain binaries and JSON receipts outside the repository.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'port/ps5/guest/src/hdtext_ps5.c'
PREAMBLE = r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifdef _WIN32
__declspec(dllimport) unsigned __stdcall SetErrorMode(unsigned);
#endif
typedef struct { int info, loaded; } Face;
typedef struct { Face* face; float scale, xk; } Inst;
typedef Inst* HFONT;
static int allocations, frees, failat, height=8;
static const uint16_t* PortLatinGlyphWidths(void) {
    static uint16_t w[256]; for(int i=0;i<256;i++)w[i]=7; return w;
}
static HFONT MakeFont(Face* face,int px,float xk) {
    allocations++; if(allocations==failat)return NULL;
    Inst* f=calloc(1,sizeof(*f)); if(!f)abort();
    f->face=face;f->scale=px/1000.0f;f->xk=xk;return f;
}
static void release(void* p) { if(p){frees++;free(p);} }
#define SDL_free release
static int GlyphBox(const Inst* f,int cp,int* x0,int* y0,int* x1,int* y1) {
    *x0=*y0=0;*x1=8;*y1=height;return 1;
}
static void stbtt_GetCodepointHMetrics(void* info,int cp,int* adv,int* lsb) {
    *adv=1000;*lsb=0;
}
'''
MAIN = r'''
int main(int argc,char** argv) {
#ifdef _WIN32
    SetErrorMode(3); /* Do not open a Windows crash dialog for the RED fixture. */
#endif
    int mode=atoi(argv[1]);Face face={0,1};
    failat=mode==1?1:mode==2?2:mode==5?2:0;
    height=mode==3?0:mode==4||mode==5?7:8;
    HFONT f=FontWithCap(&face,7,1);
    if(mode==1) { if(f||allocations!=1||frees)return 11; }
    else {
        int replacement=height==8 && failat!=2;
        float expected=(replacement?10:11)/1000.0f;
        if(!f||f->face!=&face||fabsf(f->scale-expected)>.000001f)return 12;
        if(!(f->xk>0 && f->xk<=1))return 13;
        if(frees!=(replacement?1:0))return 14;
        release(f);
        int successes=allocations-(failat && failat<=allocations?1:0);
        if(frees!=successes)return 15; /* Every successful allocation freed once. */
    }
    printf("PASS mode=%d allocations=%d frees=%d\n",mode,allocations,frees);
    return 0;
}
'''


def run(out):
    source = SOURCE.read_text(encoding='utf-8')
    start = source.index('static HFONT FontWithCap(')
    end = source.index('\nstatic void FontRelease(', start)
    fixture = out / 'font_resize.c'
    fixture.write_text(PREAMBLE + source[start:end] + MAIN, encoding='utf-8')
    if os.name == 'nt':
        tools = next(p / 'toolchain' for p in ROOT.parents if (p / 'toolchain').is_dir())
        binary = next(tools.glob('llvm-mingw-*/bin'))
        compilers = [(arch, binary / (arch + '-w64-mingw32-clang.exe'))
                     for arch in ('i686', 'x86_64')]
    else:
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            raise RuntimeError('C compiler required')
        compilers = [('native', Path(compiler))]
    results = []
    for arch, compiler in compilers:
        exe = out / ('font_resize_' + arch + ('.exe' if os.name == 'nt' else ''))
        subprocess.run([str(compiler), '-std=c11', '-O0', str(fixture), '-o', str(exe)], check=True)
        for mode in range(6):
            actual = subprocess.run([str(exe), str(mode)], capture_output=True, text=True)
            results.append(dict(arch=arch, mode=mode, exit=actual.returncode,
                                stdout=actual.stdout, stderr=actual.stderr))
    (out / 'RESULTS.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    failed = [r for r in results if r['exit']]
    if failed:
        raise RuntimeError(f'{len(failed)} actual-producer cases failed: {failed}')
    print(f'PASS {len(results)} actual FontWithCap cases; CPU fixtures, not device QA')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    if args.out:
        args.out.mkdir(parents=True, exist_ok=True)
        run(args.out)
    else:
        with tempfile.TemporaryDirectory(prefix='font-resize-') as directory:
            run(Path(directory))
