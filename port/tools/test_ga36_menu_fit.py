"""Compare the handheld's direct 240x160 -> 640x426 fit with SDL's software blit."""

import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "port/linux/fbdev_linux.c").read_text(encoding="utf-8")


def block(start_text):
    start = source.index(start_text)
    depth = 0
    for i in range(source.index("{", start), len(source)):
        depth += (source[i] == "{") - (source[i] == "}")
        if not depth:
            return source[start:i + 1]
    raise ValueError(start_text)


# The scalar fractional path itself, taken from DrawNearest2x.
loop = block("    if (fractionalFit) {")
# The NEON rows have their own check (test_ga36_fit_neon.py): drop them here.
if "static int fitNeon" in loop:
    neon = loop.index("static int fitNeon")
    loop = loop[:neon] + loop[loop.index("for (int y = 0; y < outH; y++, posY += stepY)", neon):]
rows = "hiddenRows" in loop

program = r"""
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static int Fit(uint8_t* pixels, int pitch, const uint32_t* src, int stride, int outH, const uint8_t* hiddenRows) {
    const bool fractionalFit = true;
    (void)hiddenRows;
FRACTIONAL
    return 0;
}
static uint32_t picture[320 * 240], layer[240 * 160];
int main(void) {
    uint32_t seed = 12345;
    SDL_Surface* surface = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
    SDL_Surface* direct = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
    if (!surface || !direct || !renderer || !texture || !SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART))
        return 2;
    for (int round = 0; round < 24; round++) {
        /* canvas origins: centred, both corners and random ones */
        const int ox = round == 0 ? 40 : round == 1 ? 0 : round == 2 ? 80 : (int)(seed % 81);
        const int oy = round == 0 ? 40 : round == 1 ? 0 : round == 2 ? 80 : (int)((seed >> 8) % 81);
        for (int i = 0; i < 320 * 240; i++) picture[i] = (seed = seed * 1664525u + 1013904223u) & 0xffffff;
        const SDL_FRect src = {(float)ox, (float)oy, 240, 160};
        const SDL_FRect dst = {0, 26, 640, 160.0f * (640.0f / 240.0f)};
        SDL_UpdateTexture(texture, NULL, picture, 320 * 4);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, &src, &dst);
        SDL_FlushRenderer(renderer);
        SDL_FillSurfaceRect(direct, NULL, 0);
        uint8_t* out = (uint8_t*)direct->pixels + 26 * direct->pitch;
        if (!Fit(out, direct->pitch, picture + oy * 320 + ox, 320, 426, NULL)) return 3;
        for (int y = 0; y < 480; y++)
            for (int x = 0; x < 640; x++) {
                const uint32_t a = ((uint32_t*)((uint8_t*)surface->pixels + y * surface->pitch))[x];
                const uint32_t b = ((uint32_t*)((uint8_t*)direct->pixels + y * direct->pitch))[x];
                if ((a ^ b) & 0xffffff) { printf("round %d differs at %d,%d\n", round, x, y); return 4; }
            }
#if COVERED
        /* Skipping fully opaque layer rows, then drawing the layer with the
           same sampling, gives the same pixels as drawing every base row. */
        uint8_t hidden[160];
        for (int y = 0; y < 160; y++) {
            const int kind = round == 3 ? 1 : round == 4 ? 0 : (int)((seed = seed * 1664525u + 1013904223u) >> 29) & 1;
            hidden[y] = (uint8_t)kind;
            for (int x = 0; x < 240; x++)
                layer[y * 240 + x] = (seed = seed * 1664525u + 1013904223u) | (kind ? 0xff000000u : 0);
        }
        SDL_Surface* skipped = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
        SDL_FillSurfaceRect(skipped, NULL, 0);
        if (!Fit((uint8_t*)skipped->pixels + 26 * skipped->pitch, skipped->pitch, picture + oy * 320 + ox, 320, 426,
                 hidden)) return 5;
        const unsigned dx = (240u << 16) / 640u, dy = (160u << 16) / 426u;
        for (int y = 0; y < 426; y++) {
            const int row = (int)((dy / 2 + y * dy) >> 16);
            uint32_t* a = (uint32_t*)((uint8_t*)direct->pixels + (26 + y) * direct->pitch);
            uint32_t* b = (uint32_t*)((uint8_t*)skipped->pixels + (26 + y) * skipped->pitch);
            for (int x = 0; x < 640; x++) {
                const uint32_t pixel = layer[row * 240 + ((dx / 2 + x * dx) >> 16)];
                if (hidden[row]) a[x] = b[x] = pixel; /* opaque: replaces the base */
            }
            if (memcmp(a, b, 640 * 4)) return 6;
        }
        SDL_DestroySurface(skipped);
#endif
    }
    puts("PASS menu fit equals SDL pixel-art blit (24 canvas origins)");
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
    SDL_DestroySurface(direct);
    return 0;
}
""".replace("FRACTIONAL", loop)


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
        c_file, exe = Path(tmp) / "fit.c", Path(tmp) / "fit.exe"
        c_file.write_text(program, encoding="utf-8")
        subprocess.run([str(compiler), "-std=c11", "-O2", "-DCOVERED=%d" % rows, str(c_file), *flags, "-o", str(exe)],
                       check=True)
        subprocess.run([str(exe)], env=env, check=True, timeout=60)


if __name__ == "__main__":
    main()
