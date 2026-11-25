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
''' + function('static int ObjClearRun(') + chr(10) + function('static void ObjAffineClip(') + r'''
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
static void checkAffine(int pa, int pb, int pc, int pd, int ow, int oh, int dbl) {
    const int bw = ow << dbl, bh = oh << dbl;
    for (int line = 0; line < bh; line++) {
        int first = 0, end = bw;
        const int cx0 = -(bw / 2), cy = line - bh / 2;
        ObjAffineClip(pa, pa * cx0 + pb * cy, -(ow / 2) * 256, (ow - ow / 2) * 256, &first, &end);
        ObjAffineClip(pc, pc * cx0 + pd * cy, -(oh / 2) * 256, (oh - oh / 2) * 256, &first, &end);
        for (int sx = 0; sx < bw; sx++) {
            const int cx = sx - bw / 2;
            const int tx = ((pa * cx + pb * cy) >> 8) + ow / 2;
            const int ty = ((pc * cx + pd * cy) >> 8) + oh / 2;
            const int inside = tx >= 0 && ty >= 0 && tx < ow && ty < oh;
            assert(inside == (sx >= first && sx < end));
        }
    }
}
static void affineSpans(void) {
    static const int sizes[][2] = {{8,8},{16,16},{32,32},{64,64},{16,8},{32,8},{32,16},{64,32},
                                   {8,16},{8,32},{16,32},{32,64}};
    static const int edge[] = {-32768,-32767,-512,-257,-256,-255,-1,0,1,255,256,257,512,32767};
    const int edges = (int)(sizeof(edge) / sizeof(edge[0]));
    for (int s = 0; s < 12; s++)
        for (int dbl = 0; dbl < 2; dbl++) {
            for (int a = 0; a < edges; a++)
                for (int b = 0; b < edges; b++) {
                    checkAffine(edge[a], edge[b], edge[(a + 5) % edges], edge[(b + 9) % edges], sizes[s][0], sizes[s][1], dbl);
                    checkAffine(edge[b], 0, edge[a], 256, sizes[s][0], sizes[s][1], dbl);
                }
            for (int t = 0; t < 1500; t++) {
                int m[4];
                for (int k = 0; k < 4; k++)
                    m[k] = t & 1 ? (int)(rnd() % 65536) - 32768 : (int)(rnd() % 1537) - 768;
                checkAffine(m[0], m[1], m[2], m[3], sizes[s][0], sizes[s][1], dbl);
            }
        }
}
int main(void) {
    clearRuns();
    affineSpans();
    puts("PASS affine sprite columns: every size, double size, edge and random matrices");
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
