"""KHCOM_PPU_HEADER_GONE_PARALLEL: the real HeaderPick/PpuRowsIndependent with a cleared header source."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'port/src/port_ppu.c').read_text(encoding='utf-8')


def function(signature):
    start = source.index(signature + '\n') if signature + '\n' in source else source.index(signature)
    while source[source.index(')', start) + 1:].lstrip().startswith(';'):
        start = source.index(signature, start + 1)
    opening = source.index('{', start)
    depth = 1
    for i in range(opening + 1, len(source)):
        depth += (source[i] == '{') - (source[i] == '}')
        if not depth:
            return source[start:i + 1]
    raise ValueError(signature)


program = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define SDL_getenv getenv
#define PORT_ENV_ONCE(v, get, name) do { static int once; static const char* kept; \
    if (!once) { kept = get(name); once = 1; } v = kept; } while (0)
typedef int ObjTiles, ObjPalette;
typedef struct { int top, bottom; } HeaderSampler;
static const uint16_t* sMenuHeaderFrame;
static const ObjTiles* sMenuHeaderTiles;
static const ObjPalette* sMenuHeaderPalette;
static uint32_t frameNo;
static uint32_t PortFrameCount(void) { return frameNo; }
static int HeaderSamplerInit(HeaderSampler* o, const uint16_t* f, const ObjTiles* t, const ObjPalette* p) {
    (void)t; (void)p; if (!f) return 0; o->top = 0; o->bottom = 32; return 1;
}
static int HeaderSamplerColor(const HeaderSampler* s, int y, int col) { (void)s; return col == 6 ? 5 : (y + col) & 3; }
static int MenuSpriteColor(const uint16_t* f, const ObjTiles* t, const ObjPalette* p, int y, int col,
                           int* top, int* bottom) {
    (void)t; (void)p; if (!f) return -2; if (top) *top = 0; if (bottom) *bottom = 32;
    return col == 6 ? 5 : (y + col) & 3;
}
static int sHeaderBand[2][32];
static int sBgIsoLatched[4], sPpuStats[5];
static struct { int header; } sOamTag[4];
static int OamCount(void) { return 4; }
static int PpuAffineMosaicDependent(void) { return 0; }
static int HeaderPick(void);
''' + source[source.index('static const void* sHeaderKey[3];'):source.index('static int HeaderPick(void) {')] \
    + function('static int HeaderPick(void) {') + '\n' + function('static int PpuRowsIndependent(bool allowMosaic) {') + r'''
int main(void) {
    static const uint16_t asset[2];
    static const int tiles, palette;
    sOamTag[2].header = 1;
    frameNo = 1;                      /* never picked: stays on one core either way */
    printf("%d %d\n", PpuRowsIndependent(0), HeaderPick() >= 0);
    frameNo = 2;
    sMenuHeaderFrame = asset; sMenuHeaderTiles = &tiles; sMenuHeaderPalette = &palette;
    printf("%d %d\n", PpuRowsIndependent(0), HeaderPick());
    for (frameNo = 3; frameNo < 5; frameNo++) { /* the next mode cleared the source */
        sMenuHeaderFrame = 0; sMenuHeaderTiles = 0; sMenuHeaderPalette = 0;
        int rows = PpuRowsIndependent(0);
        printf("R%d", rows);
        for (int y = 0; y < 160; y++) printf(" %d", HeaderPick());
        puts("");
    }
    sMenuHeaderFrame = asset; sMenuHeaderTiles = &tiles; sMenuHeaderPalette = &palette;
    printf("%d %d %d\n", PpuRowsIndependent(0), HeaderPick(), sHeaderBand[1][7]);
    return 0;
}
'''
program = '#include <stdbool.h>\n' + program
compiler = sys.argv[1] if len(sys.argv) > 1 else str(next(
    (p for base in (ROOT, *ROOT.parents) for p in (base / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe')),
    'clang'))
with tempfile.TemporaryDirectory() as directory:
    src, exe = Path(directory) / 'test.c', Path(directory) / 'test.exe'
    src.write_text(program)
    subprocess.run([compiler, '-O2', str(src), '-o', str(exe)], check=True)
    for fast in (False, True):
        env = {k: v for k, v in os.environ.items() if not k.startswith('KHCOM_')}
        env['KHCOM_PPU_MENU_HEADER_PARALLEL'] = '1'
        if fast:
            env['KHCOM_FAST_MENU_HEADER_PICK'] = '1'
        off = subprocess.run([str(exe)], env=env, check=True, capture_output=True, text=True).stdout.splitlines()
        env['KHCOM_PPU_HEADER_GONE_PARALLEL'] = '1'
        on = subprocess.run([str(exe)], env=env, check=True, capture_output=True, text=True).stdout.splitlines()
        assert off[0] == on[0] and off[0].startswith('0'), (off[0], on[0])
        assert off[1] == on[1] and off[1].startswith('1'), (off[1], on[1])
        for a, b in zip(off[2:4], on[2:4]):
            assert a.startswith('R0 ') and b.startswith('R1 '), (a[:12], b[:12])
            assert a.split()[1:] == b.split()[1:] == ['-1'] * 160
        assert off[4] == on[4], (off[4], on[4])
print('PASS cleared header source: same per-row result, rows shared only with the flag')
