"""KHCOM_GA36_SKIP_EMPTY_SIDES: with the picture as wide as the panel, the side
repairs (edge bands, header carry-on, flat side fills) leave no pixel."""
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "port/src/port_main.c").read_text(encoding="utf-8")
layout = (ROOT / "port/include/port/jiminy_layout.h").read_text(encoding="utf-8")


def block(start_text):
    start = source.index(start_text)
    while source.index(";", start) < source.index("{", start):  # a prototype
        start = source.index(start_text, start + 1)
    depth = 0
    for i in range(source.index("{", start), len(source)):
        depth += (source[i] == "{") - (source[i] == "}")
        if not depth:
            return source[start:i + 1]
    raise ValueError(start_text)


functions = "\n".join(block(s) for s in (
    "static int DimmedOf(", "static float PixelEdge(", "static void DrawEdgeBands(",
    "static void DrawHeaderLeft("))

program = r"""
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
""" + layout + r"""
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define PORT_MAX_W 512
#define PORT_MAX_H 320
static struct { int width, height; } gPortScreen = {320, 240};
static uint32_t sComposite[320 * 240];
static SDL_Renderer* sRenderer;
static uint32_t sBandGround;
static int sSideStyle, letterbox;
static void DetectLetterbox(const SDL_FRect* src, int* top, int* bottom) { (void)src; *top = 20; *bottom = 140; }
void PortPpuSpriteMask(uint8_t* out) { for (int i = 0; i < 320 * 240; i++) out[i] = (i / 320) % 3 == 0; }
int PortNarrowEdgeKind(int side, int y) { return letterbox ? -1 : (side + y) % 3; }
static uint32_t PortMenuHeaderPixelAt(int y, int x) { return (x + y) & 1 ? 0xffff00ffu : 0; }
""" + functions + r"""
int main(void) {
    SDL_Surface* surface = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
    SDL_Surface* keep = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
    sRenderer = SDL_CreateSoftwareRenderer(surface);
    if (!surface || !keep || !sRenderer) return 2;
    uint32_t seed = 99;
    const SDL_FRect src = {40, 40, 240, 160};
    const SDL_FRect dst = {0, 26, 240 * (640.0f / 240.0f), 160 * (640.0f / 240.0f)};
    if (PortJiminyHasSideMargins(dst.x, dst.w, 640)) return 3;
    for (int round = 0; round < 60; round++) {
        sSideStyle = round % 2 ? 11 : 8;
        letterbox = round % 5 == 0;
        sBandGround = round % 3 == 0 ? 0x102030 : 0;
        /* long flat edge runs, so that bars are really found */
        for (int y = 0; y < 240; y++) {
            const uint32_t c = (seed = seed * 1664525u + 1013904223u) & 0x30303;
            for (int x = 0; x < 320; x++)
                sComposite[y * 320 + x] = round % 4 ? c : (seed = seed * 1664525u + 1013904223u) & 0xffffff;
        }
        for (int y = 0; y < 480; y++)
            for (int x = 0; x < 640; x++)
                ((uint32_t*)((uint8_t*)surface->pixels + y * surface->pitch))[x] =
                    (seed = seed * 1664525u + 1013904223u) & 0xffffff;
        for (int y = 0; y < 480; y++)
            memcpy((uint8_t*)keep->pixels + y * keep->pitch, (uint8_t*)surface->pixels + y * surface->pitch, 640 * 4);
        DrawEdgeBands(&src, &dst, 640, 3, GBA_SCREEN_H, GBA_SCREEN_H);
        DrawHeaderLeft(&src, &dst);
        /* the style 9/12 and flat-picture fills */
        const SDL_FRect sides[3] = {{0, dst.y, dst.x, dst.h}, {dst.x + dst.w, dst.y, 640 - dst.x - dst.w, dst.h},
                                    {dst.x + dst.w, 30, 640 - dst.x - dst.w, 3}};
        SDL_SetRenderDrawColor(sRenderer, 255, 255, 255, 255);
        for (int i = 0; i < 3; i++) SDL_RenderFillRect(sRenderer, &sides[i]);
        SDL_FlushRenderer(sRenderer);
        for (int y = 0; y < 480; y++)
            if (memcmp((uint8_t*)surface->pixels + y * surface->pitch, (uint8_t*)keep->pixels + y * keep->pitch, 640 * 4)) {
                printf("round %d changed row %d\n", round, y);
                return 4;
            }
    }
    /* the same calls do paint when there are sides */
    const SDL_FRect narrow = {80, 80, 480, 320};
    for (int i = 0; i < 320 * 240; i++) sComposite[i] = 0x123456;
    sSideStyle = 8; letterbox = 0; sBandGround = 0;
    SDL_FillSurfaceRect(surface, NULL, 0);
    DrawEdgeBands(&src, &narrow, 640, 3, GBA_SCREEN_H, GBA_SCREEN_H);
    SDL_FlushRenderer(sRenderer);
    if (!(((uint32_t*)((uint8_t*)surface->pixels + 200 * surface->pitch))[10] & 0xffffff)) return 5;
    puts("PASS full-width side repairs change no pixel (60 pictures)");
    return 0;
}
"""


def main():
    if os.name == "nt":
        toolchain = next(p / "toolchain" for p in ROOT.parents if (p / "toolchain/SDL3-3.4.18").is_dir())
        sdl = toolchain / "SDL3-3.4.18/x86_64-w64-mingw32"
        compiler = toolchain / "llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe"
        flags = ["-I" + str(sdl / "include"), "-L" + str(sdl / "lib"), "-lSDL3"]
        env = dict(os.environ, PATH=str(sdl / "bin") + os.pathsep + os.environ["PATH"])
    else:
        compiler = os.environ.get("CC", "cc")
        flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", "sdl3"], text=True).split()
        env = dict(os.environ)
    with tempfile.TemporaryDirectory() as tmp:
        c_file, exe = Path(tmp) / "sides.c", Path(tmp) / "sides.exe"
        c_file.write_text(program, encoding="utf-8")
        subprocess.run([str(compiler), "-std=gnu11", "-O1", str(c_file), *flags, "-o", str(exe)], check=True)
        subprocess.run([str(exe)], env=env, check=True, timeout=60)


if __name__ == "__main__":
    main()
