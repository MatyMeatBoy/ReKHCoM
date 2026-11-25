"""Host check of the GA36 240x160 -> 640x426 menu fit.

Compares, over a dirty surface, SDL's software path (full clear plus nearest
scale of the 320x240 frame) with the letterbox-band clear plus the production
NEON row kernel, whose intrinsics are emulated here in plain C."""

import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = r"""
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct { uint32_t v[2]; } uint32x2_t;
typedef struct { uint32_t v[4]; } uint32x4_t;
static uint32x2_t vld1_u32(const uint32_t* p) { return (uint32x2_t){{p[0], p[1]}}; }
static uint32x2_t vld1_dup_u32(const uint32_t* p) { return (uint32x2_t){{p[0], p[0]}}; }
static uint32x2_t vdup_lane_u32(uint32x2_t a, int n) { return (uint32x2_t){{a.v[n], a.v[n]}}; }
static uint32x2_t vext_u32(uint32x2_t a, uint32x2_t b, int n) { assert(n == 1); return (uint32x2_t){{a.v[1], b.v[0]}}; }
static uint32x4_t vcombine_u32(uint32x2_t a, uint32x2_t b) { return (uint32x4_t){{a.v[0], a.v[1], b.v[0], b.v[1]}}; }
static void vst1q_u32(uint32_t* p, uint32x4_t a) { memcpy(p, a.v, sizeof(a.v)); }
typedef struct { uint32x4_t val[2]; } uint32x4x2_t;
static uint32x4_t vld1q_u32(const uint32_t* p) { return (uint32x4_t){{p[0], p[1], p[2], p[3]}}; }
static uint32x4x2_t vzipq_u32(uint32x4_t a, uint32x4_t b) {
    return (uint32x4x2_t){{{{a.v[0], b.v[0], a.v[1], b.v[1]}}, {{a.v[2], b.v[2], a.v[3], b.v[3]}}}};
}
KERNEL

static uint32_t frame[240 * 320];

int main(void) {
    SDL_Surface* sdl = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
    SDL_Surface* fit = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
    SDL_Renderer* a = SDL_CreateSoftwareRenderer(sdl);
    SDL_Renderer* b = SDL_CreateSoftwareRenderer(fit);
    SDL_Texture* texture = SDL_CreateTexture(a, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
    assert(sdl && fit && a && b && texture);
    assert(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST));
    uint32_t random = 0x12345678u;
    for (int pass = 0; pass < 8; pass++) {
        for (int i = 0; i < 240 * 320; i++) {
            random = random * 1664525u + 1013904223u;
            frame[i] = random | 0xFF000000u;
        }
        assert(SDL_UpdateTexture(texture, NULL, frame, 320 * 4));
        memset(sdl->pixels, 0x5A, (size_t)sdl->pitch * 480);
        memset(fit->pixels, 0xA5, (size_t)fit->pitch * 480);
        const SDL_FRect src = {40, 40, 240, 160};
        const SDL_FRect dst = {0, 26, 640, 160.0f * (640.0f / 240.0f)};
        SDL_SetRenderDrawColor(a, 0, 0, 0, 255);
        assert(SDL_RenderClear(a) && SDL_RenderTexture(a, texture, &src, &dst) && SDL_FlushRenderer(a));
        const SDL_FRect bands[2] = {{0, 0, 640, 26}, {0, 452, 640, 28}};
        SDL_SetRenderDrawColor(b, 0, 0, 0, 255);
        assert(SDL_RenderFillRects(b, bands, 2) && SDL_FlushRenderer(b));
        const unsigned stepY = (160u << 16) / 426u;
        /* The production band kernel: whole, or in reverse 16-row bands as the
         * PPU workers may take them (KHCOM_FBDEV_PARALLEL_PRESENT). */
        FitJob job = { (uint8_t*)fit->pixels + 26 * (size_t)fit->pitch, fit->pitch, 426, 320, stepY,
                       frame + 40 * 320 + 40, NULL, (pass & 1) != 0 };
        if (pass & 2)
            for (int b = 144; b >= 0; b -= 16) FitRowsBand(&job, b, b + 16);
        else
            FitRowsBand(&job, 0, 160);
        for (int y = 0; y < 480; y++) {
            const uint32_t* x = (const uint32_t*)((uint8_t*)sdl->pixels + (size_t)y * sdl->pitch);
            const uint32_t* z = (const uint32_t*)((uint8_t*)fit->pixels + (size_t)y * fit->pitch);
            for (int col = 0; col < 640; col++)
                if ((x[col] & 0xFFFFFFu) != (z[col] & 0xFFFFFFu)) {
                    printf("mismatch pass %d at %d,%d: %08x %08x\n", pass, col, y, x[col], z[col]);
                    return 1;
                }
        }
    }
    /* Doubled rows: the whole 320x240 frame, and the 240x160 world map at
     * 2x centred over a dirty surface with only its surroundings cleared. */
    for (int map = 0; map < 2; map++) {
        memset(sdl->pixels, 0x5A, (size_t)sdl->pitch * 480);
        memset(fit->pixels, 0xA5, (size_t)fit->pitch * 480);
        const SDL_FRect src = {40, 40, 240, 160};
        const SDL_FRect dst = {80, 80, 480, 320};
        const SDL_FRect around[4] = {{0, 0, 640, 80}, {0, 400, 640, 80}, {0, 80, 80, 320}, {560, 80, 80, 320}};
        assert(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART));
        SDL_SetRenderDrawColor(a, 0, 0, 0, 255);
        SDL_SetRenderDrawColor(b, 0, 0, 0, 255);
        assert(SDL_RenderClear(a) && SDL_RenderTexture(a, texture, map ? &src : NULL, map ? &dst : NULL) &&
               SDL_FlushRenderer(a));
        if (map) assert(SDL_RenderFillRects(b, around, 4) && SDL_FlushRenderer(b));
        const int w = map ? 240 : 320, h = map ? 160 : 240;
        const uint32_t* input = frame + (map ? 40 * 320 + 40 : 0);
        if (map) {
            Allmap2xJob job = { (uint8_t*)fit->pixels + 80 * (size_t)fit->pitch + 80 * 4, fit->pitch, 320, input };
            for (int b = 144; b >= 0; b -= 16) Allmap2xBand(&job, b, b + 16);
        } else
        for (int y = 0; y < h; y++, input += 320) {
            uint32_t* upper = (uint32_t*)((uint8_t*)fit->pixels + (size_t)((map ? 80 : 0) + 2 * y) * fit->pitch) +
                              (map ? 80 : 0);
            FbDoubleRow(upper, input, w);
            memcpy((uint8_t*)upper + fit->pitch, upper, (size_t)w * 8);
        }
        for (int y = 0; y < 480; y++) {
            const uint32_t* x = (const uint32_t*)((uint8_t*)sdl->pixels + (size_t)y * sdl->pitch);
            const uint32_t* z = (const uint32_t*)((uint8_t*)fit->pixels + (size_t)y * fit->pitch);
            for (int col = 0; col < 640; col++)
                if ((x[col] & 0xFFFFFFu) != (z[col] & 0xFFFFFFu)) {
                    printf("2x mismatch case %d at %d,%d: %08x %08x\n", map, col, y, x[col], z[col]);
                    return 1;
                }
        }
    }
    puts("GA36 menu fit: PASS");
    return 0;
}
"""


def main():
    toolchain = next(p / "toolchain" for p in (Path(os.environ.get("REKHCOM_WORK", ".")), *ROOT.parents)
                     if (p / "toolchain/SDL3-3.4.18").is_dir())
    sdl = toolchain / "SDL3-3.4.18/x86_64-w64-mingw32"
    compiler = toolchain / "llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe"
    kernel = (ROOT / "port/linux/fb_fit_row.h").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory() as tmp:
        source, exe = Path(tmp) / "fit.c", Path(tmp) / "fit.exe"
        fb = (ROOT / "port/linux/fbdev_linux.c").read_text(encoding="utf-8")
        start = fb.index("static int FirstOutputRow(")
        bands = fb[start:fb.index("#endif", fb.index("static void Allmap2xBand("))]
        bands = bands.replace("#ifdef __ARM_NEON\n", "")
        source.write_text(FIXTURE.replace("KERNEL", kernel + "\n" + bands), encoding="utf-8")
        subprocess.run([str(compiler), "-O1", str(source), "-o", str(exe), "-I" + str(sdl / "include"),
                        "-L" + str(sdl / "lib"), "-lSDL3"], check=True)
        env = dict(os.environ, PATH=str(sdl / "bin") + os.pathsep + os.environ["PATH"])
        subprocess.run([str(exe)], check=True, env=env)


if __name__ == "__main__":
    main()
