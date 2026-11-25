"""KHCOM_GA36_BLUR_COVER_ALL: what the reflection leaves undrawn lies strictly
inside the rows and columns the fitted picture then overwrites."""
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "port/src/port_main.c").read_text(encoding="utf-8")


def block(start_text):
    start = source.index(start_text)
    depth = 0
    for i in range(source.index("{", start), len(source)):
        depth += (source[i] == "{") - (source[i] == "}")
        if not depth:
            return source[start:i + 1]
    raise ValueError(start_text)


program = r"""
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
typedef struct { float x, y, w, h; } SDL_FRect;
typedef struct { int x, y, w, h; } SDL_Rect;
#define SDL_clamp(v, a, b) ((v) < (a) ? (a) : (v) > (b) ? (b) : (v))
#define SDL_ceilf ceilf
#define SDL_floorf floorf
""" + block("static SDL_Rect BlurOpaqueCover(") + "\n" + block("static bool PortGa36FitGeometry(") + r"""
int main(void) {
    const SDL_FRect dst = {0, 26, 240 * (640.0f / 240.0f), 160 * (640.0f / 240.0f)};
    if (!PortGa36FitGeometry(&dst)) return 2;
    const SDL_Rect c = BlurOpaqueCover(&dst, 640, 480);
    /* every presenter of this geometry writes columns 0..639 of rows 26..451 */
    if (c.w <= 0 || c.h <= 0 || c.x < 0 || c.x + c.w > 640 || c.y < 26 || c.y + c.h > 26 + (int)dst.h) return 3;
    SDL_FRect other = dst;
    other.y = 8;
    if (PortGa36FitGeometry(&other)) return 4;
    other = (SDL_FRect){80, 80, 480, 320};
    if (PortGa36FitGeometry(&other)) return 5;
    printf("PASS blur cover %d,%d %dx%d inside the picture\n", c.x, c.y, c.w, c.h);
    return 0;
}
"""
cc = os.environ.get("CC") or shutil.which("cc")
if not cc:
    toolchain = next(p / "toolchain" for p in ROOT.parents if (p / "toolchain/SDL3-3.4.18").is_dir())
    cc = str(toolchain / "llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe")
with tempfile.TemporaryDirectory() as tmp:
    c_file, exe = Path(tmp) / "cover.c", Path(tmp) / "cover.exe"
    c_file.write_text(program, encoding="utf-8")
    subprocess.run([cc, "-std=gnu11", "-O1", str(c_file), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True, timeout=60)
