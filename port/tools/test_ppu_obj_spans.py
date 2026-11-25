"""ROM-free check of the opt-in sprite sampling shortcuts against per-texel sampling."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'port/src/port_ppu.c').read_text(encoding='utf-8')


def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    for i in range(opening + 1, len(source)):
        depth += (source[i] == '{') - (source[i] == '}')
        if not depth:
            return source[start:i + 1]
    raise ValueError(signature)


program = r'''
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
''' + function('static int ObjClearRun(') + r'''
static unsigned rng = 7331;
static unsigned rnd(void) { rng = rng * 1664525u + 1013904223u; return rng >> 8; }
static int texel(const uint8_t* row, int tx, int bpp8) {
    return bpp8 ? row[tx & 7] : (tx & 1) ? row[(tx & 7) >> 1] >> 4 : row[(tx & 7) >> 1] & 15;
}
static void clearRuns(void) {
    for (int bpp8 = 0; bpp8 < 2; bpp8++)
    for (int pattern = 0; pattern < 256; pattern++)
    for (int variant = 0; variant < 40; variant++) {
        uint8_t row[8] = {0};
        for (int i = 0; i < 8; i++)
            if (pattern & (1 << i)) {
                int v = variant == 0 ? 1 : variant == 1 ? (bpp8 ? 128 : 8) : 1 + rnd() % (bpp8 ? 255 : 15);
                if (bpp8) row[i] = (uint8_t)v; else row[i >> 1] |= (uint8_t)(v << ((i & 1) * 4));
            }
        for (int flip = 0; flip < 2; flip++)
            for (int tx = 0; tx < 64; tx++) {
                if (texel(row, tx, bpp8)) continue;
                int want = 0;
                for (int at = tx & 7; at >= 0 && at < 8 && !texel(row, at, bpp8); at += flip ? -1 : 1) want++;
                assert(ObjClearRun(row, tx, bpp8, flip) == want);
            }
    }
}
int main(void) {
    clearRuns();
    puts("PASS transparent sprite runs: 4bpp/8bpp, flips, every opacity mask");
    return 0;
}
'''
cc = os.environ.get('CC') or shutil.which('cc') or str(
    root.parent.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
with tempfile.TemporaryDirectory() as directory:
    src = Path(directory) / 'test.c'
    exe = Path(directory) / 'test.exe'
    src.write_text(program)
    subprocess.run([cc, '-O2', str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
