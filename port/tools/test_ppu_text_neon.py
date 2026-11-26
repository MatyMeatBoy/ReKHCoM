"""ROM-free check of the NEON 4bpp tile-row decoder against per-pixel sampling.

The production DecodeTile4 NEON branch is compiled on the host with the few intrinsics it uses emulated in C."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'port/src/port_ppu.c').read_text(encoding='utf-8')
start = source.index('static inline void DecodeTile4(')
end = source.index('\n}\n', start) + 3

program = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define TRANSPARENT 0x8000
#define __ARM_NEON 1
typedef struct { uint8_t v[8]; } uint8x8_t;
typedef struct { uint8_t v[16]; } uint8x16_t;
typedef struct { uint16_t v[8]; } uint16x8_t;
typedef struct { uint32_t v[2]; } uint32x2_t;
typedef struct { uint8x8_t val[2]; } uint8x8x2_t;
typedef struct { uint8x16_t val[2]; } uint8x16x2_t;
#define EACH for (int i = 0; i < 8; i++)
static uint16x8_t vdupq_n_u16(uint16_t a) { uint16x8_t r; EACH r.v[i] = a; return r; }
static void vst1q_u16(uint16_t* p, uint16x8_t a) { memcpy(p, a.v, sizeof(a.v)); }
static uint32x2_t vdup_n_u32(uint32_t a) { uint32x2_t r = {{a, a}}; return r; }
static uint8x8_t vreinterpret_u8_u32(uint32x2_t a) { uint8x8_t r; memcpy(r.v, a.v, 8); return r; } /* little endian */
static uint8x8_t vdup_n_u8(uint8_t a) { uint8x8_t r; EACH r.v[i] = a; return r; }
static uint8x8_t vand_u8(uint8x8_t a, uint8x8_t b) { EACH a.v[i] &= b.v[i]; return a; }
static uint8x8_t vbic_u8(uint8x8_t a, uint8x8_t b) { EACH a.v[i] &= (uint8_t)~b.v[i]; return a; }
static uint8x8_t vshr_n_u8(uint8x8_t a, int n) { EACH a.v[i] >>= n; return a; }
static uint8x8_t vceq_u8(uint8x8_t a, uint8x8_t b) { EACH a.v[i] = a.v[i] == b.v[i] ? 255 : 0; return a; }
static uint8x8_t vbsl_u8(uint8x8_t m, uint8x8_t a, uint8x8_t b) { EACH m.v[i] = (uint8_t)((m.v[i] & a.v[i]) | (~m.v[i] & b.v[i])); return m; }
static uint8x8x2_t vzip_u8(uint8x8_t a, uint8x8_t b) {
    uint8x8x2_t r;
    for (int i = 0; i < 4; i++) {
        r.val[0].v[i * 2] = a.v[i]; r.val[0].v[i * 2 + 1] = b.v[i];
        r.val[1].v[i * 2] = a.v[i + 4]; r.val[1].v[i * 2 + 1] = b.v[i + 4];
    }
    return r;
}
static uint8x8_t vrev64_u8(uint8x8_t a) { uint8x8_t r; EACH r.v[i] = a.v[7 - i]; return r; }
static uint8x16x2_t vld2q_u8(const uint8_t* p) {
    uint8x16x2_t r;
    for (int i = 0; i < 16; i++) { r.val[0].v[i] = p[i * 2]; r.val[1].v[i] = p[i * 2 + 1]; }
    return r;
}
static uint8x8_t vget_low_u8(uint8x16_t a) { uint8x8_t r; memcpy(r.v, a.v, 8); return r; }
static uint8x8_t vget_high_u8(uint8x16_t a) { uint8x8_t r; memcpy(r.v, a.v + 8, 8); return r; }
static uint8x8_t vtbl2_u8(uint8x8x2_t t, uint8x8_t n) {
    uint8x8_t r;
    EACH r.v[i] = n.v[i] < 16 ? t.val[n.v[i] >> 3].v[n.v[i] & 7] : 0;
    return r;
}
static void vst2_u8(uint8_t* p, uint8x8x2_t a) { EACH { p[i * 2] = a.val[0].v[i]; p[i * 2 + 1] = a.val[1].v[i]; } }
''' + source[start:end] + r'''
static uint32_t seed = 4242;
static uint32_t Random(void) { seed = seed * 1664525u + 1013904223u; return seed >> 8; }
int main(void) {
    uint16_t colors[16], out[10];
    uint8_t row[4];
    int cases = 0;
    for (int pass = 0; pass < 300000; pass++) {
        for (int i = 0; i < 16; i++) colors[i] = (uint16_t)Random(); /* bit 15 included */
        for (int i = 0; i < 4; i++) row[i] = (uint8_t)Random();
        if (pass % 5 == 0) memset(row, 0, 4);
        if (pass % 7 == 0) row[pass % 4] &= pass & 1 ? 0x0F : 0xF0;
        if (pass % 11 == 0) memset(row, (pass % 16) * 17, 4);
        for (int flip = 0; flip < 2; flip++, cases++) {
            out[0] = out[9] = 0x1234;
            DecodeTile4(out + 1, row, colors, flip ? 0x400 : 0);
            for (int i = 0; i < 8; i++) {
                const int x = flip ? 7 - i : i;
                const int c = (row[x >> 1] >> ((x & 1) * 4)) & 15;
                assert(out[1 + i] == (c ? colors[c] & 0x7FFF : TRANSPARENT));
            }
            assert(out[0] == 0x1234 && out[9] == 0x1234);
        }
    }
    printf("PASS NEON tile decode: %d rows identical\n", cases);
    return 0;
}
'''
toolchain_root = Path(os.environ.get('KHCOM_TOOLCHAIN', ROOT.parent / 'toolchain'))
toolchain = sorted(toolchain_root.glob('llvm-mingw-*'))[-1]
with tempfile.TemporaryDirectory() as directory:
    c = Path(directory) / 'neon.c'
    c.write_text(program)
    exe = Path(directory) / 'neon.exe'
    subprocess.run([str(toolchain / 'bin/x86_64-w64-mingw32-clang.exe'), '-O2', '-std=gnu11', str(c), '-o', str(exe)],
                   check=True)
    subprocess.run([str(exe)], check=True, timeout=120)
