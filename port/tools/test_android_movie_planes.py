#!/usr/bin/env python3
"""Issue #563: a decoder handed Android's HD movie player frames with a NULL chroma plane and the YUV->RGB upload
read it. PlanesUsable (port/portable/hdmovie_android.c) is compiled alone here and checked on the frame shapes the
player meets, then the player is checked to call it before any upload and to fall back to the cartridge's scene.

    python port/tools/test_android_movie_planes.py
"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / 'port/portable/hdmovie_android.c').read_text(encoding='utf-8')

match = re.search(r'static int PlanesUsable\(.*?\n}\n', SOURCE, re.S)
assert match, 'PlanesUsable not found'
body = SOURCE[match.start():SOURCE.index('\n}\n', match.start()) + 3]

TEST = r'''
#include <stdint.h>
#include <stdio.h>
typedef struct { int32_t left, top, right, bottom; } HdCrop;
''' + body + r'''
static uint8_t Y[1280 * 720], C[1280 * 360];
int main(void) {
    HdCrop full = { 0, 0, 1280, 720 }, odd = { 0, 0, 1279, 719 }, bad = { 0, 0, 0, 720 };
    int failures = 0;
#define EXPECT(v, c) do { if ((c) != (v)) { printf("FAIL line %d\n", __LINE__); failures++; } } while (0)
    /* NV12: U and V interleaved, V one byte after U, one plane of 1280 bytes a row */
    EXPECT(1, PlanesUsable(&full, Y, sizeof Y, 1280, C, sizeof C - 1, 1280, C + 1, sizeof C - 1, 1280, 2));
    /* I420: separate quarter planes, one byte a sample */
    EXPECT(1, PlanesUsable(&full, Y, sizeof Y, 1280, C, 640 * 360, 640, C + 640 * 360, 640 * 360, 640, 1));
    EXPECT(1, PlanesUsable(&odd, Y, sizeof Y, 1280, C, 640 * 360, 640, C + 640 * 360, 640 * 360, 640, 1));
    /* the reported frame: chroma NULL although the call succeeded */
    EXPECT(0, PlanesUsable(&full, Y, sizeof Y, 1280, NULL, sizeof C, 1280, C + 1, sizeof C - 1, 1280, 2));
    EXPECT(0, PlanesUsable(&full, Y, sizeof Y, 1280, C, sizeof C, 1280, NULL, sizeof C, 1280, 2));
    EXPECT(0, PlanesUsable(&full, NULL, sizeof Y, 1280, C, sizeof C, 1280, C + 1, sizeof C - 1, 1280, 2));
    /* planes shorter than the rows read, zero strides or pixel step, an empty crop */
    EXPECT(0, PlanesUsable(&full, Y, sizeof Y - 1, 1280, C, sizeof C - 1, 1280, C + 1, sizeof C - 1, 1280, 2));
    EXPECT(0, PlanesUsable(&full, Y, sizeof Y, 1280, C, 640 * 359, 640, C + 640 * 360, 640 * 360, 640, 1));
    EXPECT(0, PlanesUsable(&full, Y, sizeof Y, 0, C, sizeof C, 1280, C + 1, sizeof C - 1, 1280, 2));
    EXPECT(0, PlanesUsable(&full, Y, sizeof Y, 1280, C, sizeof C, 1280, C + 1, sizeof C - 1, 1280, 0));
    EXPECT(0, PlanesUsable(&bad, Y, sizeof Y, 1280, C, sizeof C, 1280, C + 1, sizeof C - 1, 1280, 2));
    printf(failures ? "%d failure(s)\n" : "PlanesUsable: frame shapes ok\n", failures);
    return failures != 0;
}
'''


def main():
    tc = sorted((ROOT.parent / 'toolchain').glob('llvm-mingw-*'))[-1] / 'bin'
    with tempfile.TemporaryDirectory() as tmp:
        src, exe = Path(tmp) / 'planes.c', Path(tmp) / 'planes.exe'
        src.write_text(TEST, encoding='utf-8')
        subprocess.run([str(tc / 'clang.exe'), '-O2', '-Wall', str(src), '-o', str(exe)], check=True)
        result = subprocess.run([str(exe)], capture_output=True, text=True)
        print(result.stdout.strip())
        if result.returncode:
            return 1
    player = SOURCE[SOURCE.index('int HdMovieAndroidPlay('):]
    check = player.index('PlanesUsable(&crop')
    for upload in ('YuvToRgb(texture', 'SDL_UpdateNVTexture(texture', 'SDL_UpdateYUVTexture(texture'):
        assert player.index(upload) > check, f'{upload} runs before the planes are checked'
    assert 'gives no usable frames, the cartridge' in player, 'no cartridge fallback when no frame is usable'
    print('player: planes checked before every upload; cartridge scene when none is usable')
    return 0


if __name__ == '__main__':
    sys.exit(main())
