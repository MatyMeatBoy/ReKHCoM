"""Compare real SDL framebuffer output over simulated Linux screen metadata."""

import argparse
import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FIXTURE = r"""
#include <SDL3/SDL.h>
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct fb_var_screeninfo {
    uint32_t xres, yres, xres_virtual, yres_virtual, xoffset, yoffset, bits_per_pixel;
    struct { uint32_t offset; } red;
};
struct fb_fix_screeninfo { uint32_t line_length, smem_len; };

enum { FBIOGET_VSCREENINFO, FBIOGET_FSCREENINFO, O_RDWR, PROT_READ, PROT_WRITE, MAP_SHARED };
#define MAP_FAILED ((void*)-1)

static struct fb_var_screeninfo var;
static struct fb_fix_screeninfo fix;
_Alignas(16) static uint8_t dst[2 * 1024 * 1024], expected[sizeof(dst)];
static size_t mapped;

static int TestOpen(const char* path, int flags) {
    assert(strcmp(path, "fixture-fb") == 0 && flags == O_RDWR);
    return 1;
}
static int TestClose(int fd) { assert(fd == 1); return 0; }
static int TestIoctl(int fd, int request, void* out) {
    assert(fd == 1);
    if (request == FBIOGET_VSCREENINFO) memcpy(out, &var, sizeof(var));
    else { assert(request == FBIOGET_FSCREENINFO); memcpy(out, &fix, sizeof(fix)); }
    return 0;
}
static void* TestMap(void* addr, size_t len, int prot, int flags, int fd, int off) {
    assert(!addr && prot == (PROT_READ | PROT_WRITE) && flags == MAP_SHARED && fd == 1 && off == 0);
    assert(len > 0 && len <= sizeof(dst));
    mapped = len;
    return dst;
}
static void PortLog(const char *format, ...) { (void)format; }

#define open TestOpen
#define close TestClose
#define ioctl TestIoctl
#define mmap TestMap

PRODUCTION

static void Setup(uint32_t w, uint32_t h, uint32_t x, uint32_t y, uint32_t stride,
                  uint32_t bits, uint32_t red) {
    var = (struct fb_var_screeninfo){ w, h, w + x, h + y + 2, x, y, bits, {red} };
    fix = (struct fb_fix_screeninfo){ stride, stride * var.yres_virtual };
    assert(fix.smem_len <= sizeof(dst));
    SDL_memset(dst, 0x9D, sizeof(dst));
    SDL_memset(expected, 0x9D, sizeof(expected));
    sFb = NULL;
    mapped = 0;
}

static void LegacyPresent(SDL_Renderer* renderer, uint8_t* out, SDL_PixelFormat format) {
    SDL_Surface* frame = SDL_RenderReadPixels(renderer, NULL);
    assert(frame);
    SDL_Surface* converted = SDL_ConvertSurface(frame, format);
    assert(converted);
    SDL_DestroySurface(frame);
    const size_t bytes = var.bits_per_pixel / 8;
    const int w = SDL_min(converted->w, (int)var.xres);
    const int h = SDL_min(converted->h, (int)var.yres);
    out += var.yoffset * fix.line_length + var.xoffset * bytes;
    for (int y = 0; y < h; y++)
        memcpy(out + y * fix.line_length, (uint8_t*)converted->pixels + y * converted->pitch, w * bytes);
    SDL_DestroySurface(converted);
}

static void Paint(SDL_Renderer* renderer) {
    const SDL_FRect rect = { 1, 1, 5, 6 }, other = { 3, 3, 5, 7 };
    assert(SDL_SetRenderDrawColor(renderer, 0x79, 0xA1, 0x8B, 0x28));
    assert(SDL_RenderClear(renderer));
    assert(SDL_SetRenderDrawColor(renderer, 0xF1, 0x3F, 0xB3, 0xA7));
    assert(SDL_RenderFillRect(renderer, &rect));
    assert(SDL_SetRenderDrawColor(renderer, 0x93, 0x7A, 0x07, 0xD0));
    assert(SDL_RenderFillRect(renderer, &other));
}

static void Compare(SDL_PixelFormat src, SDL_PixelFormat target, int w, int h, int x, int y, int pitch) {
    SDL_Surface* surface = SDL_CreateSurface(11, 13, src);
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    assert(surface && renderer);
    Paint(renderer);
    const int bits = target == SDL_PIXELFORMAT_RGB565 ? 16 : 32;
    Setup(w, h, x, y, pitch, bits, target == SDL_PIXELFORMAT_XBGR8888 ? 0 : 16);
    int outW, outH;
    assert(PortFbInit(&outW, &outH) && PortFbActive() && outW == w && outH == h);
    LegacyPresent(renderer, expected, target);
    PortFbPresent(renderer);
    assert(memcmp(dst, expected, sizeof(dst)) == 0);
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
}

static void Geometry(void) {
    int w, h;
    Setup(11, 13, 1, 2, 24, 16, 11);
    /* Kernels may export less storage than all declared virtual rows. */
    fix.smem_len -= 2 * fix.line_length;
    assert(PortFbInit(&w, &h) && mapped == fix.smem_len);

    Setup(11, 13, 1, 2, 24, 16, 11);
    fix.smem_len = (var.yoffset + var.yres) * fix.line_length - 1;
    assert(!PortFbInit(&w, &h) && !mapped);
    Setup(11, 13, 1, 2, 22, 16, 11);
    assert(!PortFbInit(&w, &h) && !mapped);
    Setup(11, 13, 1, 2, 24, 16, 11);
    var.yoffset = UINT32_MAX;
    assert(!PortFbInit(&w, &h) && !mapped);
    Setup(11, 13, 1, 2, 24, 16, 11);
    fix.smem_len = 0;
    assert(!PortFbInit(&w, &h) && !mapped);
}

static SDL_malloc_func prevMalloc;
static SDL_calloc_func prevCalloc;
static SDL_realloc_func prevRealloc;
static SDL_free_func prevFree;
static size_t allocBytes;
static void* SDLCALL CountMalloc(size_t size) { allocBytes += size; return prevMalloc(size); }
static void* SDLCALL CountCalloc(size_t n, size_t size) { allocBytes += n * size; return prevCalloc(n, size); }
static void* SDLCALL CountRealloc(void* p, size_t size) { allocBytes += size; return prevRealloc(p, size); }

static void Benchmark(int repeats) {
    SDL_Surface* surface = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_XRGB8888);
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    assert(surface && renderer);
    Setup(640, 480, 0, 0, 1280, 16, 11);
    int w, h;
    assert(PortFbInit(&w, &h));
    Paint(renderer);
    LegacyPresent(renderer, expected, SDL_PIXELFORMAT_RGB565);
    PortFbPresent(renderer);
    assert(memcmp(dst, expected, sizeof(dst)) == 0);
    size_t allocations[4];
    for (int row = 0; row < 4; row++) {
        const bool direct = row == 1 || row == 2;
        for (int i = 0; i < 10; i++) {
            if (direct) PortFbPresent(renderer);
            else LegacyPresent(renderer, dst, SDL_PIXELFORMAT_RGB565);
        }
        allocBytes = 0;
        uint64_t start = SDL_GetPerformanceCounter();
        for (int i = 0; i < repeats; i++) {
            if (direct) PortFbPresent(renderer);
            else LegacyPresent(renderer, dst, SDL_PIXELFORMAT_RGB565);
        }
        uint64_t duration = SDL_GetPerformanceCounter() - start;
        double frameMs = duration * 1000.0 / SDL_GetPerformanceFrequency() / repeats;
        allocations[row] = allocBytes / (size_t)repeats;
        printf("%s ms/frame=%.5f allocated_bytes/frame=%zu frames=%d\n",
               direct ? "direct" : "legacy", frameMs, allocations[row], repeats);
    }
    const size_t convertedPixels = 640 * 480 * 2;
    assert(allocations[1] + convertedPixels <= allocations[0]);
    assert(allocations[2] + convertedPixels <= allocations[3]);
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
}

int main(int argc, char** argv) {
    SDL_GetOriginalMemoryFunctions(&prevMalloc, &prevCalloc, &prevRealloc, &prevFree);
    assert(SDL_SetMemoryFunctions(CountMalloc, CountCalloc, CountRealloc, prevFree));
    assert(SDL_Init(SDL_INIT_VIDEO));
    assert(SDL_setenv_unsafe("KHCOM_FBDEV", "fixture-fb", 1) == 0);
    if (argc > 1) {
        Benchmark(SDL_atoi(argv[1]));
    } else {
        Geometry();
        Compare(SDL_PIXELFORMAT_XRGB8888, SDL_PIXELFORMAT_RGB565, 11, 13, 1, 2, 28);
        Compare(SDL_PIXELFORMAT_XRGB8888, SDL_PIXELFORMAT_XBGR8888, 13, 15, 1, 2, 60);
        Compare(SDL_PIXELFORMAT_XRGB8888, SDL_PIXELFORMAT_XRGB8888, 9, 8, 1, 1, 44);
        Compare(SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_XRGB8888, 11, 13, 1, 2, 52);
        Compare(SDL_PIXELFORMAT_XRGB8888, SDL_PIXELFORMAT_RGB565, 11, 13, 1, 2, 27);
        Compare(SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_RGB565, 11, 13, 1, 2, 28);
        puts("fbdev geometry and six pixel/padding comparisons PASS");
    }
    SDL_Quit();
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", type=int, default=0, metavar="FRAMES")
    args = parser.parse_args()
    if args.benchmark < 0:
        parser.error("frame count must be positive")
    if os.name == "nt":
        toolchain = next(p / "toolchain" for p in ROOT.parents
                         if (p / "toolchain/SDL3-3.4.18").is_dir())
        sdl = toolchain / "SDL3-3.4.18/x86_64-w64-mingw32"
        compiler = toolchain / "llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe"
        flags = ["-I" + str(sdl / "include"), "-L" + str(sdl / "lib"), "-lSDL3"]
        env = dict(os.environ, PATH=str(sdl / "bin") + os.pathsep + os.environ["PATH"])
    else:
        compiler = os.environ.get("CC", "cc")
        flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", "sdl3"], text=True).split()
        env = dict(os.environ)
    env["SDL_VIDEO_DRIVER"] = "dummy"
    source = (ROOT / "port/linux/fbdev_linux.c").read_text(encoding="utf-8")
    code = FIXTURE.replace("PRODUCTION", source[source.index("static uint8_t* sFb;"):])
    with tempfile.TemporaryDirectory() as tmp:
        c_file, exe = Path(tmp) / "fbdev.c", Path(tmp) / "fbdev.exe"
        c_file.write_text(code, encoding="utf-8")
        subprocess.run([str(compiler), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        str(c_file), *flags, "-o", str(exe)], check=True)
        command = [str(exe), *([str(args.benchmark)] if args.benchmark else [])]
        subprocess.run(command, env=env, check=True, timeout=20)


if __name__ == "__main__":
    main()
