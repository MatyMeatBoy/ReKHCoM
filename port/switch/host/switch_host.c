/* Switch host (devkitPro libnx, homebrew .nro): loads the game image (build_switch.py: AArch64 code with 32-bit
 * pointers, a Mach-O executable linked at 0x10000000) into the process's code region, fills the bridge table
 * (port/ps5/shared/bridge_list.h) and starts the game. The image keeps its GBA space as arrays of its own
 * (PORT_GBA_RELOC), so nothing has to be mapped at the GBA's low addresses.
 *
 * Video goes to the GPU: the game's quads are SDL2 textures drawn with SDL_RenderGeometry (issue #35: the software
 * rasterizer held the console to 20-25 fps); sound is an SDL2 audio queue at 48 kHz; controllers and the touch screen
 * come from libnx.
 *
 * With __vita__ (port/vita/build_vita.py) the same file is the PS Vita host: the game image is ARMv7 code linked into
 * the same executable (one per region), video is the PS4 host's software rasterizer (port/ps4/ps4_host.c) into a
 * 960x544 display buffer, sound a
 * sceAudioOut port and the controls sceCtrl.
 *
 * Without __SWITCH__ or __vita__ the same file is a headless Linux AArch64 test host (the frame stays in memory,
 * --shot N file.ppm writes one, KHCOM_PADS scripts buttons), to check the image and the bridge without a console.
 * Built for 32-bit ARM Linux with REKHCOM_LINKED_GUEST it links the Vita's game image instead (build_vita.py test). */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __SWITCH__
#include <switch.h>
#include <SDL2/SDL.h>
#define DATA_ROOT "sdmc:/switch/ReKHCoM"
#elif defined(__vita__)
#include <psp2/appmgr.h>
#include <psp2/audioout.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#define DATA_ROOT "ux0:data/ReKHCoM"
#ifdef REKHCOM_VITA_GPU
#include <SDL2/SDL.h>
#endif
int _newlib_heap_size_user = 64 * 1024 * 1024; /* textures of the rasterizer, the notices */
unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024; /* the game runs on the main thread */
#define REKHCOM_LINKED_GUEST
#else
#include <signal.h>
#include <sys/mman.h>
#include <ucontext.h>
#endif

#include "bridge.h"

/* the picture on the GPU through SDL2: Switch, and the Vita built with REKHCOM_VITA_GPU (GXM); else the software
   rasterizer below */
#if defined(__SWITCH__) || (defined(__vita__) && defined(REKHCOM_VITA_GPU))
#define HOST_GPU 1
#endif

#ifdef REKHCOM_LINKED_GUEST
/* the game image is part of this executable (the Vita's, port/vita/build_vita.py) */
extern uint64_t gHostTable[];
extern int guest_main(void);
#endif

#ifndef DATA_ROOT
static char sDataRoot[512] = ".";
#define DATA_ROOT sDataRoot
#endif

/* ---- log ------------------------------------------------------------------ */

static FILE* sLog;

static void Log(const char* fmt, ...) {
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
        line[--n] = 0;
    fprintf(stderr, "[ReKHCoM] %s\n", line);
    if (sLog) {
        fprintf(sLog, "%s\n", line);
        fflush(sLog);
    }
}

static uint64_t NowUs(void) {
#ifdef __vita__
    return sceKernelGetProcessTimeWide();
#endif
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000u + (uint64_t)t.tv_nsec / 1000u;
}

/* ---- bridge: process, files --------------------------------------------- */

static void H_log(const char* s) { Log("%s", s); }

static void H_exit(int code) {
    Log("the game ended (%d)", code);
    if (sLog)
        fflush(sLog);
#ifdef __SWITCH__
    SDL_Quit();
    romfsExit();
#endif
#ifdef __vita__
    sceKernelExitProcess(code);
#endif
    exit(code);
}

static uint64_t H_ticks_us(void) { return NowUs(); }
static void H_sleep_us(uint32_t us) {
#ifdef __vita__
    sceKernelDelayThread(us);
#else
    usleep(us);
#endif
}

static int H_getenv(const char* name, char* out, int cap) {
    const char* v = getenv(name);
    if (!v)
        return -1;
    snprintf(out, (size_t)cap, "%s", v);
    return (int)strlen(out);
}

static FILE* sFiles[64];
static DIR* sDirs[16];

static int H_fopen(const char* path, int mode) {
    static const char* const modes[] = { "rb", "wb", "ab", "r+b" };
    for (int i = 0; i < 64; ++i)
        if (!sFiles[i]) {
            sFiles[i] = fopen(path, modes[mode & 3]);
            return sFiles[i] ? i : -1;
        }
    return -1;
}
static int H_fread(int h, void* buf, uint32_t n) { return (int)fread(buf, 1, n, sFiles[h]); }
static int H_fwrite(int h, const void* buf, uint32_t n) { return (int)fwrite(buf, 1, n, sFiles[h]); }
static int H_fseek(int h, int32_t off, int whence) { return fseek(sFiles[h], off, whence); }
static int H_ftell(int h) { return (int)ftell(sFiles[h]); }
static int H_fclose(int h) {
    int r = fclose(sFiles[h]);
    sFiles[h] = NULL;
    return r;
}
static int H_fflush(int h) { return fflush(sFiles[h]); }
static int H_stat(const char* p) {
    struct stat s;
    return stat(p, &s) ? 0 : S_ISDIR(s.st_mode) ? 2 : 1;
}
static int H_remove(const char* p) { return remove(p); }
static int H_rename(const char* a, const char* b) {
    remove(b); /* the SD card's file system does not replace on rename */
    return rename(a, b);
}
static int H_mkdir(const char* p) { return mkdir(p, 0777); }
static int H_dir_open(const char* p) {
    for (int i = 0; i < 16; ++i)
        if (!sDirs[i]) {
            sDirs[i] = opendir(p);
            return sDirs[i] ? i : -1;
        }
    return -1;
}
static int H_dir_next(int d, char* name, int cap) {
    struct dirent* e;
    while ((e = readdir(sDirs[d]))) {
        if (e->d_name[0] == '.')
            continue;
        snprintf(name, (size_t)cap, "%s", e->d_name);
        return 1;
    }
    return 0;
}
static void H_dir_close(int d) {
    closedir(sDirs[d]);
    sDirs[d] = NULL;
}

/* ---- video: software rasterizer (as port/ps4/ps4_host.c) ----------------- */

#if defined(__vita__) && defined(HOST_GPU)
#define FB_W 960
#define FB_H 544
#define PIX(c) (c)
#elif defined(__vita__)
#define FB_W 960
#define FB_H 544
/* the display reads A8B8G8R8: on the Vita the rasterizer keeps red and blue swapped from textures to screen */
#define PIX(c) (((c) & 0xFF00FF00u) | (((c) >> 16) & 0xFFu) | (((c) & 0xFFu) << 16))
#else
#define FB_W 1280
#define FB_H 720
#define PIX(c) (c)
#endif
static int sW = FB_W, sH = FB_H;
#ifndef HOST_GPU
static uint32_t* sFb; /* XRGB */
static int sPitch = FB_W;
#endif
static int sFrame; /* frames shown (the test host's --shot and KHCOM_PADS count them) */
#ifdef HOST_GPU
static SDL_Window* sWindow;
static SDL_Renderer* sRenderer;
static SDL_Texture* sGpuTex[4096];
static int sGpuW[4096], sGpuSmooth[4096];
#elif defined(__vita__)
static void* sVram[2];
static int sBack;
#else
static int sShotFrame[16], sShots; /* --shot N file, repeatable */
static const char* sShotFile[16];
#endif

static int sBlend = 1;
#ifndef HOST_GPU
typedef struct Tex {
    int w, h, smooth;
    uint32_t* argb;
} Tex;
static Tex sTex[4096];
static int sClip[4], sHasClip;
static int sColumns[FB_W];
#endif

static void H_gfx_size(int* w, int* h) {
    *w = sW;
    *h = sH;
}

static void VideoInit(void) {
#ifdef HOST_GPU
#ifdef __vita__
    SDL_Init(SDL_INIT_VIDEO); /* the sound stays on sceAudioOut */
#else
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
#endif
    sWindow = SDL_CreateWindow("ReKHCoM", 0, 0, FB_W, FB_H, SDL_WINDOW_FULLSCREEN);
    sRenderer = sWindow ? SDL_CreateRenderer(sWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (!sRenderer) {
        Log("video: %s", SDL_GetError());
        return;
    }
    SDL_RenderSetLogicalSize(sRenderer, FB_W, FB_H);
    {
        SDL_RendererInfo info;
        SDL_GetRendererInfo(sRenderer, &info);
        Log("video: %dx%d on the GPU (SDL %s, %s)", FB_W, FB_H, SDL_GetCurrentVideoDriver(), info.name);
    }
#else
    sFb = calloc((size_t)FB_W * FB_H, 4);
#endif
#if defined(__vita__) && !defined(HOST_GPU)
    for (int i = 0; i < 2; i++) {
        SceUID block = sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, 2 * 1024 * 1024, NULL);
        if (block < 0 || sceKernelGetMemBlockBase(block, &sVram[i]) < 0) {
            Log("video: no display memory (0x%08x)", block);
            sVram[0] = NULL;
            return;
        }
        memset(sVram[i], 0, FB_W * FB_H * 4);
    }
    Log("video: %dx%d", FB_W, FB_H);
#endif
}

static void Present(void) {
#ifdef HOST_GPU
    if (sRenderer)
        SDL_RenderPresent(sRenderer);
#elif defined(__vita__)
    if (sVram[0]) {
        SceDisplayFrameBuf fb = { sizeof(fb), sVram[sBack], FB_W, SCE_DISPLAY_PIXELFORMAT_A8B8G8R8, FB_W, FB_H };
        sceDisplayWaitSetFrameBuf(); /* the buffer set last frame is on screen: the other one is free */
        memcpy(sVram[sBack], sFb, FB_W * FB_H * 4);
        sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
        sBack ^= 1;
    }
#else
    for (int s = 0; s < sShots; s++) {
        if (sFrame != sShotFrame[s])
            continue;
        FILE* f = fopen(sShotFile[s], "wb");
        if (f) {
            fprintf(f, "P6\n%d %d\n255\n", sW, sH);
            for (int i = 0; i < sW * sH; i++) {
                uint8_t p[3] = { (uint8_t)(sFb[i] >> 16), (uint8_t)(sFb[i] >> 8), (uint8_t)sFb[i] };
                fwrite(p, 1, 3, f);
            }
            fclose(f);
            Log("frame %d saved to %s", sFrame, sShotFile[s]);
        }
    }
#endif
    sFrame++;
}

#ifdef HOST_GPU
/* ---- video: the GPU (as port/android64/host/android64_host.c, SDL2 here) ---------------------------------------- */

static void H_gfx_begin(uint32_t clear) {
    if (!sRenderer)
        return;
    SDL_RenderSetClipRect(sRenderer, NULL);
    SDL_SetRenderDrawColor(sRenderer, (clear >> 16) & 255, (clear >> 8) & 255, clear & 255, 255);
    if (clear >> 24)
        SDL_RenderClear(sRenderer);
}
static int H_tex_create(const void* rgba, int w, int h, int smooth) {
    if (!sRenderer)
        return -1;
    for (int i = 0; i < 4096; i++)
        if (!sGpuTex[i]) {
            sGpuTex[i] = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
            if (!sGpuTex[i])
                return -1;
            SDL_UpdateTexture(sGpuTex[i], NULL, rgba, w * 4);
            SDL_SetTextureBlendMode(sGpuTex[i], SDL_BLENDMODE_BLEND);
            sGpuW[i] = w;
            sGpuSmooth[i] = smooth;
            return i;
        }
    return -1;
}
static void H_tex_update(int id, const void* rgba) { SDL_UpdateTexture(sGpuTex[id], NULL, rgba, sGpuW[id] * 4); }
static void H_tex_free(int id) {
    SDL_DestroyTexture(sGpuTex[id]);
    sGpuTex[id] = NULL;
}
static void H_gfx_clip(int x, int y, int w, int h) {
    SDL_Rect r = { x, y, w, h };
    SDL_RenderSetClipRect(sRenderer, w > 0 ? &r : NULL);
}
static void H_gfx_blend(int on) { sBlend = on; }

/* HostVertex triangles; the shim adds 2, 4 or 6 to u for pixel art and the MMPX filters (port/ps5/guest/src/sdl_shim.c):
   those are drawn crisp (nearest), as the software hosts read them */
static void H_gfx_draw(int id, const void* verts, int count) {
    const HostVertex* v = verts;
    SDL_Texture* t = id >= 0 && id < 4096 ? sGpuTex[id] : NULL;
    static SDL_Vertex out[8192];
    int crisp = 0;
    float shift = 0;
    if ((id >= 0 && !t) || !sRenderer)
        return;
    if (count > 8192)
        count = 8192;
    if (count > 0 && t) {
        shift = floorf((v[0].u + 0.25f) / 2.0f) * 2.0f;
        crisp = shift > 0;
        SDL_SetTextureScaleMode(t, crisp || !sGpuSmooth[id] ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
        SDL_SetTextureBlendMode(t, sBlend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    }
    for (int i = 0; i < count; i++) {
        out[i].position.x = v[i].x;
        out[i].position.y = v[i].y;
        out[i].tex_coord.x = v[i].u - shift;
        out[i].tex_coord.y = v[i].v;
        out[i].color.r = (v[i].argb >> 16) & 255;
        out[i].color.g = (v[i].argb >> 8) & 255;
        out[i].color.b = v[i].argb & 255;
        out[i].color.a = v[i].argb >> 24;
    }
    SDL_SetRenderDrawBlendMode(sRenderer, sBlend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_RenderGeometry(sRenderer, t, out, count, NULL, 0);
}
#else
static void H_gfx_begin(uint32_t clear) {
    sHasClip = 0;
    if (clear >> 24)
        for (int i = 0; i < sW * sH; i++)
            sFb[i] = PIX(clear) | 0xFF000000u;
}

static uint32_t ToArgb(const uint8_t* s) {
    return PIX(((uint32_t)s[3] << 24) | ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | s[2]);
}

static int H_tex_create(const void* rgba, int w, int h, int smooth) {
    for (int i = 0; i < 4096; i++)
        if (!sTex[i].argb) {
            const uint8_t* s = rgba;
            sTex[i].argb = malloc((size_t)w * h * 4);
            if (!sTex[i].argb)
                return -1;
            sTex[i].w = w;
            sTex[i].h = h;
            sTex[i].smooth = smooth;
            for (int k = 0; k < w * h; k++, s += 4)
                sTex[i].argb[k] = ToArgb(s);
            return i;
        }
    return -1;
}
static void H_tex_update(int id, const void* rgba) {
    const uint8_t* s = rgba;
    Tex* t = &sTex[id];
    for (int k = 0; k < t->w * t->h; k++, s += 4)
        t->argb[k] = ToArgb(s);
}
static void H_tex_free(int id) {
    free(sTex[id].argb);
    sTex[id].argb = NULL;
}
static void H_gfx_clip(int x, int y, int w, int h) {
    sHasClip = w > 0;
    sClip[0] = x;
    sClip[1] = y;
    sClip[2] = x + w;
    sClip[3] = y + h;
}
static void H_gfx_blend(int on) { sBlend = on; }

static inline void Put(uint32_t* d, uint32_t src, uint32_t color) {
    uint32_t a = src >> 24, r = (src >> 16) & 255, g = (src >> 8) & 255, b = src & 255;
    if (color != 0xFFFFFFFFu) {
        a = a * (color >> 24) / 255;
        r = r * ((color >> 16) & 255) / 255;
        g = g * ((color >> 8) & 255) / 255;
        b = b * (color & 255) / 255;
    }
    if (!sBlend || a == 255) {
        *d = 0xFF000000u | (r << 16) | (g << 8) | b;
        return;
    }
    if (a == 0)
        return;
    uint32_t o = *d, ia = 255 - a;
    r = (r * a + ((o >> 16) & 255) * ia + 127) / 255;
    g = (g * a + ((o >> 8) & 255) * ia + 127) / 255;
    b = (b * a + (o & 255) * ia + 127) / 255;
    *d = 0xFF000000u | (r << 16) | (g << 8) | b;
}

static uint32_t Bilinear(const Tex* t, float u, float v) {
    float fx = u * t->w - 0.5f, fy = v * t->h - 0.5f;
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
    float ax = fx - x0, ay = fy - y0, c[4] = { 0, 0, 0, 0 };
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) {
            int x = x0 + i, y = y0 + j;
            x = x < 0 ? 0 : x >= t->w ? t->w - 1 : x;
            y = y < 0 ? 0 : y >= t->h ? t->h - 1 : y;
            float wgt = (i ? ax : 1 - ax) * (j ? ay : 1 - ay);
            uint32_t p = t->argb[(size_t)y * t->w + x];
            c[0] += (p >> 24) * wgt;
            c[1] += ((p >> 16) & 255) * wgt;
            c[2] += ((p >> 8) & 255) * wgt;
            c[3] += (p & 255) * wgt;
        }
    return ((uint32_t)(c[0] + 0.5f) << 24) | ((uint32_t)(c[1] + 0.5f) << 16) | ((uint32_t)(c[2] + 0.5f) << 8) |
           (uint32_t)(c[3] + 0.5f);
}

static void ClipBox(int* x0, int* y0, int* x1, int* y1) {
    if (sHasClip) {
        if (*x0 < sClip[0]) *x0 = sClip[0];
        if (*y0 < sClip[1]) *y0 = sClip[1];
        if (*x1 > sClip[2]) *x1 = sClip[2];
        if (*y1 > sClip[3]) *y1 = sClip[3];
    }
    if (*x0 < 0) *x0 = 0;
    if (*y0 < 0) *y0 = 0;
    if (*x1 > sW) *x1 = sW;
    if (*y1 > sH) *y1 = sH;
}

static void Rect(const Tex* t, const HostVertex* q) {
    float qx0 = q[0].x, qy0 = q[0].y, qx1 = q[5].x, qy1 = q[5].y;
    float u0 = q[0].u, v0 = q[0].v, u1 = q[5].u, v1 = q[5].v;
    int x0 = (int)ceilf(qx0 - 0.5f), y0 = (int)ceilf(qy0 - 0.5f), x1 = (int)ceilf(qx1 - 0.5f), y1 = (int)ceilf(qy1 - 0.5f);
    uint32_t color = PIX(q[0].argb);
    int crisp = 1;

    ClipBox(&x0, &y0, &x1, &y1);
    if (x0 >= x1 || y0 >= y1 || qx1 <= qx0 || qy1 <= qy0)
        return;
    if (!t) {
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++)
                Put(&sFb[(size_t)y * sPitch + x], 0xFFFFFFFFu, color);
        return;
    }
    {
        float k = floorf((fminf(u0, u1) + 0.25f) / 2.0f) * 2.0f;
        if (k > 0) {
            u0 -= k;
            u1 -= k;
        } else {
            crisp = !t->smooth;
        }
    }
    float du = (u1 - u0) / (qx1 - qx0), dv = (v1 - v0) / (qy1 - qy0);
    if (!crisp) {
        for (int y = y0; y < y1; y++) {
            float v = v0 + (y + 0.5f - qy0) * dv;
            for (int x = x0; x < x1; x++)
                Put(&sFb[(size_t)y * sPitch + x], Bilinear(t, u0 + (x + 0.5f - qx0) * du, v), color);
        }
        return;
    }
    for (int x = x0; x < x1; x++) {
        int c = (int)floorf((u0 + (x + 0.5f - qx0) * du) * t->w);
        sColumns[x] = c < 0 ? 0 : c >= t->w ? t->w - 1 : c;
    }
    for (int y = y0; y < y1; y++) {
        int r = (int)floorf((v0 + (y + 0.5f - qy0) * dv) * t->h);
        const uint32_t* s = t->argb + (size_t)(r < 0 ? 0 : r >= t->h ? t->h - 1 : r) * t->w;
        uint32_t* d = sFb + (size_t)y * sPitch;
        if (color == 0xFFFFFFFFu && !sBlend)
            for (int x = x0; x < x1; x++)
                d[x] = s[sColumns[x]] | 0xFF000000u;
        else
            for (int x = x0; x < x1; x++)
                Put(&d[x], s[sColumns[x]], color);
    }
}

static void Tri(const Tex* t, const HostVertex* a, const HostVertex* b, const HostVertex* c) {
    float area = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
    if (area == 0)
        return;
    int x0 = (int)floorf(fminf(a->x, fminf(b->x, c->x))), x1 = (int)ceilf(fmaxf(a->x, fmaxf(b->x, c->x)));
    int y0 = (int)floorf(fminf(a->y, fminf(b->y, c->y))), y1 = (int)ceilf(fmaxf(a->y, fmaxf(b->y, c->y)));
    ClipBox(&x0, &y0, &x1, &y1);
    float sg = area < 0 ? -1.0f : 1.0f, ia = 1.0f / (area * sg);
    const HostVertex* v[3] = { a, b, c };
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            float px = x + 0.5f, py = y + 0.5f, w[3];
            int inside = 1;
            for (int i = 0; i < 3 && inside; i++) {
                const HostVertex *p = v[(i + 1) % 3], *q = v[(i + 2) % 3];
                float e = ((q->x - p->x) * (py - p->y) - (q->y - p->y) * (px - p->x)) * sg;
                w[i] = e * ia;
                inside = e >= 0;
            }
            if (!inside)
                continue;
            uint32_t s = 0xFFFFFFFFu;
            if (t) {
                float u = w[0] * a->u + w[1] * b->u + w[2] * c->u, vv = w[0] * a->v + w[1] * b->v + w[2] * c->v;
                u -= floorf((u + 0.25f) / 2.0f) * 2.0f;
                s = Bilinear(t, u, vv);
            }
            Put(&sFb[(size_t)y * sPitch + x], s, PIX(a->argb));
        }
}

static int IsRect(const HostVertex* q) {
    return q[3].x == q[1].x && q[3].y == q[1].y && q[4].x == q[2].x && q[4].y == q[2].y && q[0].y == q[1].y &&
           q[0].x == q[2].x && q[5].x == q[1].x && q[5].y == q[2].y && q[0].u == q[2].u && q[0].v == q[1].v &&
           q[5].u == q[1].u && q[5].v == q[2].v && q[0].argb == q[5].argb && q[1].argb == q[0].argb && q[2].argb == q[0].argb;
}

static void H_gfx_draw(int id, const void* verts, int count) {
    const HostVertex* v = verts;
    const Tex* t = id >= 0 && id < 4096 && sTex[id].argb ? &sTex[id] : NULL;
    if (id >= 0 && !t)
        return;
    for (int i = 0; i + 2 < count;) {
        if (i + 5 < count && IsRect(&v[i])) {
            Rect(t, &v[i]);
            i += 6;
        } else {
            Tri(t, &v[i], &v[i + 1], &v[i + 2]);
            i += 3;
        }
    }
}
#endif

static const char* sAssetRoot = "assets";
static int sToast = -1, sToastW, sToastH;
static uint64_t sToastUntil;

static int LoadNotice(const char* name, int* w, int* h) {
    char path[600];
    snprintf(path, sizeof(path), "%s/%s.rgba", sAssetRoot, name);
    FILE* f = fopen(path, "rb");
    int id = -1;
    uint32_t size[2];
    if (!f)
        return -1;
    if (fread(size, sizeof(size), 1, f) == 1 && size[0] && size[1] && size[0] <= 4096 && size[1] <= 4096) {
        size_t bytes = (size_t)size[0] * size[1] * 4;
        uint8_t* pixels = malloc(bytes);
        if (pixels && fread(pixels, 1, bytes, f) == bytes) {
            id = H_tex_create(pixels, (int)size[0], (int)size[1], 1);
            *w = (int)size[0];
            *h = (int)size[1];
        }
        free(pixels);
    }
    fclose(f);
    return id;
}

static void DrawBox(int id, int w, int h, float y0, uint32_t tint) {
    float k = sW / 1920.0f, dw = w * k, dh = h * k, x0 = (sW - dw) / 2;
    HostVertex q[6] = { { x0, y0, 0, 0, tint }, { x0 + dw, y0, 1, 0, tint }, { x0, y0 + dh, 0, 1, tint },
                        { x0 + dw, y0, 1, 0, tint }, { x0, y0 + dh, 0, 1, tint }, { x0 + dw, y0 + dh, 1, 1, tint } };
    H_gfx_clip(0, 0, 0, 0);
    sBlend = 1;
    H_gfx_draw(id, q, 6);
}

static void H_notify(const char* name) {
    if (sToast >= 0)
        H_tex_free(sToast);
    sToast = LoadNotice(name, &sToastW, &sToastH);
    sToastUntil = NowUs() + 5000000u;
}

static void H_gfx_end(void) {
    if (sToast >= 0) {
        uint64_t now = NowUs();
        if (now >= sToastUntil) {
            H_tex_free(sToast);
            sToast = -1;
        } else {
            uint64_t left = sToastUntil - now;
            uint32_t alpha = left < 400000u ? (uint32_t)(left * 255u / 400000u) : 255u;
            int blend = sBlend;
            float k = sW / 1920.0f;
            DrawBox(sToast, sToastW, sToastH, sH - sToastH * k - 90.0f * k, (alpha << 24) | 0xFFFFFFu);
            sBlend = blend;
        }
    }
    Present();
}

/* ---- audio ---------------------------------------------------------------- */

#ifdef __SWITCH__
static SDL_AudioDeviceID sAudio;
#elif defined(__vita__)
/* a ring the output thread empties into the port, 256 frames at a time */
#define RING 16384
static int16_t sRing[RING * 2];
static volatile uint32_t sRingHead, sRingTail;
static int sAudioPort = -1;

static int AudioThread(SceSize args, void* argp) {
    static int16_t buf[256 * 2];
    (void)args, (void)argp;
    for (;;) {
        uint32_t tail = sRingTail, avail = sRingHead - tail, n = avail < 256 ? avail : 256;
        for (uint32_t i = 0; i < n; i++) {
            buf[i * 2] = sRing[((tail + i) % RING) * 2];
            buf[i * 2 + 1] = sRing[((tail + i) % RING) * 2 + 1];
        }
        memset(buf + n * 2, 0, (256 - n) * 4);
        __sync_synchronize();
        sRingTail = tail + n;
        sceAudioOutOutput(sAudioPort, buf);
    }
    return 0;
}
#endif
static double sRatio = 1.0, sPos;
static int16_t sLast[2];

static int sGameRate = 48000; /* the rate the game opened the audio at */
static int H_audio_open(int rate) {
    sGameRate = rate;
#ifdef __SWITCH__
    if (sAudio)
        return 0;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    sAudio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!sAudio) {
        Log("audio: %s", SDL_GetError());
        return -1;
    }
    SDL_PauseAudioDevice(sAudio, 0);
#elif defined(__vita__)
    if (sAudioPort >= 0)
        return 0;
    sAudioPort = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, 256, rate, SCE_AUDIO_OUT_MODE_STEREO);
    if (sAudioPort < 0) {
        Log("audio: no port (0x%08x)", sAudioPort);
        return -1;
    }
    SceUID t = sceKernelCreateThread("audio", AudioThread, 64, 0x10000, 0, 0, NULL);
    if (t >= 0)
        sceKernelStartThread(t, 0, NULL);
#endif
    Log("audio: %d Hz", rate);
    return 0;
}

static void Queue(const int16_t* f, uint32_t n) {
#ifdef __SWITCH__
    if (sAudio)
        SDL_QueueAudio(sAudio, f, n * 4);
#elif defined(__vita__)
    uint32_t head = sRingHead;
    if (n > RING - (head - sRingTail))
        n = RING - (head - sRingTail);
    for (uint32_t i = 0; i < n; i++) {
        sRing[((head + i) % RING) * 2] = f[i * 2];
        sRing[((head + i) % RING) * 2 + 1] = f[i * 2 + 1];
    }
    __sync_synchronize();
    sRingHead = head + n;
#else
    (void)f, (void)n;
#endif
}

static void GamePush(const int16_t* frames, uint32_t count) {
    static int16_t* out;
    static size_t cap;
    if (!count)
        return;
    if (sRatio > 0.9999 && sRatio < 1.0001) {
        sPos = 0;
        sLast[0] = frames[count * 2 - 2];
        sLast[1] = frames[count * 2 - 1];
        Queue(frames, count);
        return;
    }
    size_t need = (size_t)(count / sRatio) + 3, n = 0;
    if (need > cap) {
        free(out);
        out = malloc(need * 4);
        cap = out ? need : 0;
        if (!out)
            return;
    }
    double p = sPos;
    while (p < count && n < cap) {
        size_t i = (size_t)p;
        double f = p - (double)i;
        for (int c = 0; c < 2; ++c) {
            double a = i == 0 ? sLast[c] : frames[(i - 1) * 2 + c], b = frames[i * 2 + c];
            out[n * 2 + c] = (int16_t)(a + (b - a) * f);
        }
        ++n;
        p += sRatio;
    }
    sPos = p - count;
    sLast[0] = frames[count * 2 - 2];
    sLast[1] = frames[count * 2 - 1];
    Queue(out, (uint32_t)n);
}

/* HD music: the host decodes the track the game names and mixes it into the game's blocks (shared/hd_music_host.h) */
#define HD_MUSIC_LOG Log
#include "hd_music_host.h"
/* the network link's socket (shared/udp_host.h): none on this host yet, the link stays the cable */
#include "udp_host.h"
static void H_audio_push(const int16_t* frames, uint32_t count) { HdMusicPush(frames, count, sGameRate, GamePush); }
/* HD movies: only the Android host plays them (bridge entry hd_movie); the game plays its own movie */
static int H_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* font) {
    (void)movie, (void)subs1, (void)subs2, (void)font;
    return 0;
}
static uint32_t H_audio_queued(void) {
#ifdef __SWITCH__
    return sAudio ? SDL_GetQueuedAudioSize(sAudio) / 4 : 0;
#elif defined(__vita__)
    return sRingHead - sRingTail;
#else
    return 800 * 4;
#endif
}
static void H_audio_clear(void) {
#ifdef __SWITCH__
    if (sAudio)
        SDL_ClearQueuedAudio(sAudio);
#elif defined(__vita__)
    sRingTail = sRingHead;
#endif
}
static void H_audio_ratio(double ratio) { sRatio = ratio > 0.5 && ratio < 2.0 ? ratio : 1.0; }

/* ---- controllers --------------------------------------------------------- */

static bool sQuit;
#ifdef __SWITCH__
static PadState sPad;
#endif

static int H_pad_poll(HostPad* out, int count) {
    memset(out, 0, sizeof(HostPad) * (size_t)count);
#ifdef __SWITCH__
    if (!appletMainLoop())
        sQuit = true;
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT)
                sQuit = true;
    }
    if (count < 1)
        return 0;
    padUpdate(&sPad);
    u64 b = padGetButtons(&sPad);
    HidAnalogStickState l = padGetStickPos(&sPad, 0), r = padGetStickPos(&sPad, 1);
    /* RetroPad numbering (bridge.h) by position: B south, A east, Y west, X north. The game reads the Switch's
       labels (Nintendo layout, sdl_shim.c's SDL_GetGamepadType), so A confirms as on the console. Minus is Select,
       Plus Start; Minus + L3 / R3 save and load a state (the touchpad of the PS consoles). */
    static const struct { u64 key; int bit; } map[] = {
        { HidNpadButton_B, 0 }, { HidNpadButton_Y, 1 }, { HidNpadButton_Minus, 2 }, { HidNpadButton_Plus, 3 },
        { HidNpadButton_Up, 4 }, { HidNpadButton_Down, 5 }, { HidNpadButton_Left, 6 }, { HidNpadButton_Right, 7 },
        { HidNpadButton_A, 8 }, { HidNpadButton_X, 9 }, { HidNpadButton_L, 10 }, { HidNpadButton_R, 11 },
        { HidNpadButton_ZL, 12 }, { HidNpadButton_ZR, 13 }, { HidNpadButton_StickL, 14 }, { HidNpadButton_StickR, 15 },
    };
    out[0].connected = 1;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (b & map[i].key)
            out[0].buttons |= 1u << map[i].bit;
    /* Minus + Plus held three seconds opens the options, the PS consoles' touchpad hold (issue #36) */
    if ((b & HidNpadButton_Minus) && (b & (HidNpadButton_StickL | HidNpadButton_StickR | HidNpadButton_Plus)))
        out[0].buttons |= 1u << 16;
    /* the game does not see that Select + Start (A + B + Select + Start, the soft reset, still reaches it) */
    if ((b & HidNpadButton_Minus) && (b & HidNpadButton_Plus) && !(b & (HidNpadButton_A | HidNpadButton_B)))
        out[0].buttons &= ~((1u << 2) | (1u << 3));
    out[0].lx = (int16_t)SDL_clamp(l.x, -32767, 32767);
    out[0].ly = (int16_t)SDL_clamp(-l.y, -32767, 32767);
    out[0].rx = (int16_t)SDL_clamp(r.x, -32767, 32767);
    out[0].ry = (int16_t)SDL_clamp(-r.y, -32767, 32767);
    out[0].l2 = (b & HidNpadButton_ZL) ? 255 : 0;
    out[0].r2 = (b & HidNpadButton_ZR) ? 255 : 0;
    return 1;
#elif defined(__vita__)
    SceCtrlData c;
    if (count < 1 || sceCtrlPeekBufferPositive(0, &c, 1) < 0)
        return 0;
    /* RetroPad numbering (bridge.h) by position: Cross south, Circle east, Square west, Triangle north. The Vita has
       no L2/R2/L3/R3: Select + L / R save and load a state (the touchpad + L3 / R3 of the PS4 and PS5). */
    static const struct { uint32_t key; int bit; } map[] = {
        { SCE_CTRL_CROSS, 0 }, { SCE_CTRL_SQUARE, 1 }, { SCE_CTRL_SELECT, 2 }, { SCE_CTRL_START, 3 },
        { SCE_CTRL_UP, 4 }, { SCE_CTRL_DOWN, 5 }, { SCE_CTRL_LEFT, 6 }, { SCE_CTRL_RIGHT, 7 },
        { SCE_CTRL_CIRCLE, 8 }, { SCE_CTRL_TRIANGLE, 9 }, { SCE_CTRL_LTRIGGER, 10 }, { SCE_CTRL_RTRIGGER, 11 },
    };
    out[0].connected = 1;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (c.buttons & map[i].key)
            out[0].buttons |= 1u << map[i].bit;
    if ((c.buttons & SCE_CTRL_SELECT) && (c.buttons & (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER))) {
        out[0].buttons &= ~((1u << 2) | (1u << 10) | (1u << 11));
        out[0].buttons |= (1u << 16) | ((c.buttons & SCE_CTRL_LTRIGGER) ? 1u << 14 : 1u << 15);
    }
    out[0].lx = (int16_t)(c.lx == 255 ? 32767 : (c.lx - 128) * 256);
    out[0].ly = (int16_t)(c.ly == 255 ? 32767 : (c.ly - 128) * 256);
    out[0].rx = (int16_t)(c.rx == 255 ? 32767 : (c.rx - 128) * 256);
    out[0].ry = (int16_t)(c.ry == 255 ? 32767 : (c.ry - 128) * 256);
    return 1;
#else
    static const char* const names[] = { "B", "Y", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT", "A", "X", "L", "R" };
    const char* script = getenv("KHCOM_PADS");
    if (!script || count < 1)
        return 0;
    out[0].connected = 1;
    while (*script) {
        unsigned a, b;
        int n = 0;
        char name[16];
        if (sscanf(script, "%u-%u:%15[A-Z]%n", &a, &b, name, &n) >= 3 && n) {
            if ((unsigned)sFrame >= a && (unsigned)sFrame <= b)
                for (unsigned i = 0; i < 12; i++)
                    if (!strcmp(name, names[i]))
                        out[0].buttons |= 1u << i;
            script += n;
        }
        while (*script == ',')
            script++;
        if (!n)
            break;
    }
    return 1;
#endif
}
static int H_quit_requested(void) { return sQuit; }
static void H_set_title(const char* s) { (void)s; }
#ifdef __SWITCH__
/* the handheld touch screen, normalized as SDL fingers (touch_android.c's options button, issue #35) */
static int H_touch_poll(HostTouch* fingers, int max) {
    HidTouchScreenState st = { 0 };
    int n = 0;
    if (hidGetTouchScreenStates(&st, 1))
        for (int i = 0; i < st.count && n < max; i++, n++) {
            fingers[n].id = st.touches[i].finger_id;
            fingers[n].x = st.touches[i].x / 1280.0f;
            fingers[n].y = st.touches[i].y / 720.0f;
        }
    return n;
}
#else
/* no touch screen here (bridge entry of the Android 64-bit host) */
static int H_touch_poll(HostTouch* fingers, int max) { (void)fingers; (void)max; return 0; }
#endif
/* no background state here */
static int H_app_state(void) { return 0; }

#ifndef REKHCOM_LINKED_GUEST
/* ---- the game image: a Mach-O executable of 32-bit load commands ---------------------------------------------- */

typedef struct {
    uint32_t magic, cputype, cpusubtype, filetype, ncmds, sizeofcmds, flags;
} MachHeader;
typedef struct {
    uint32_t cmd, cmdsize;
    char segname[16];
    uint32_t vmaddr, vmsize, fileoff, filesize, maxprot, initprot, nsects, flags;
} MachSegment;

typedef struct {
    uint32_t vmaddr, vmsize, fileoff, filesize, prot;
    char name[17];
} Segment;

/* Readable and writable (and, for code, executable) memory at the image's own addresses, below 4 GiB. */
#ifdef __SWITCH__
static bool MapSegment(const Segment* s, const uint8_t* file) {
    Handle self = envGetOwnProcessHandle();
    size_t size = (s->vmsize + 0xFFF) & ~(size_t)0xFFF;
    void* src = aligned_alloc(0x1000, size);
    Result rc;
    if (!src) {
        Log("memory: no room for %s (%zu bytes)", s->name, size);
        return false;
    }
    memset(src, 0, size);
    memcpy(src, file + s->fileoff, s->filesize);
    rc = svcMapProcessCodeMemory(self, s->vmaddr, (u64)(uintptr_t)src, size);
    if (R_FAILED(rc)) {
        Log("memory: %s at 0x%08x: map failed (0x%x)", s->name, s->vmaddr, rc);
        return false;
    }
    rc = svcSetProcessMemoryPermission(self, s->vmaddr, size, (s->prot & 4) ? Perm_Rx : Perm_Rw);
    if (R_FAILED(rc)) {
        Log("memory: %s at 0x%08x: permission failed (0x%x)", s->name, s->vmaddr, rc);
        return false;
    }
    Log("memory: %s at 0x%08x, %zu bytes, %s", s->name, s->vmaddr, size, (s->prot & 4) ? "code" : "data");
    return true;
}
#else
static bool MapSegment(const Segment* s, const uint8_t* file) {
    size_t size = (s->vmsize + 0xFFF) & ~(size_t)0xFFF;
    void* p = mmap((void*)(uintptr_t)s->vmaddr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void*)(uintptr_t)s->vmaddr) {
        Log("memory: %s at 0x%08x failed: %s", s->name, s->vmaddr, strerror(errno));
        return false;
    }
    memcpy(p, file + s->fileoff, s->filesize);
    Log("memory: %s at 0x%08x, %zu bytes", s->name, s->vmaddr, size);
    return true;
}
#endif

static bool LoadImage(const char* region, Ps5GuestHeader* header) {
    char path[700];
    snprintf(path, sizeof(path), "%s/guest-%s.macho", sAssetRoot, region);
    FILE* f = fopen(path, "rb");
    if (!f) {
        Log("the game image %s was not found", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* file = malloc((size_t)size);
    bool ok = file && fread(file, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    const MachHeader* mh = (const MachHeader*)file;
    if (!ok || mh->magic != 0xFEEDFACEu) {
        Log("%s is not a game image", path);
        free(file);
        return false;
    }
    const uint8_t* lc = file + sizeof(MachHeader);
    uint32_t headerAt = 0, end = 0;
    for (uint32_t i = 0; i < mh->ncmds && ok; i++) {
        const MachSegment* seg = (const MachSegment*)lc;
        if (seg->cmd == 1 /* LC_SEGMENT */ && seg->vmsize && strncmp(seg->segname, "__PAGEZERO", 16) != 0 &&
            strncmp(seg->segname, "__LINKEDIT", 16) != 0) {
            Segment s = { seg->vmaddr, seg->vmsize, seg->fileoff, seg->filesize, seg->initprot, { 0 } };
            memcpy(s.name, seg->segname, 16);
            ok = MapSegment(&s, file);
            if (!strncmp(seg->segname, "__RKHDR", 16))
                headerAt = seg->vmaddr;
            if (seg->vmaddr + seg->vmsize > end)
                end = seg->vmaddr + seg->vmsize;
        }
        lc += seg->cmdsize;
    }
    free(file);
    if (!ok || !headerAt) {
        Log("%s: %s", path, ok ? "no header segment" : "could not be mapped");
        return false;
    }
    memcpy(header, (const void*)(uintptr_t)headerAt, sizeof(*header));
    header->imageEnd = header->regionEnd = end;
    Log("game image: %s, header at 0x%08x, ends at 0x%08x", path, headerAt, end);
    return header->magic == PS5G_MAGIC;
}
#endif

static const char* FindRegion(void) {
    static const struct { const char* code; const char* region; } known[] = { { "B8CP", "eu" }, { "B8CE", "us" }, { "B8CJ", "jp" } };
    const char* wanted = getenv("KHCOM_REGION");
    const char* found = NULL;
    DIR* d = opendir(DATA_ROOT);
    while (d && !found) {
        struct dirent* e = readdir(d);
        if (!e)
            break;
        size_t n = strlen(e->d_name);
        if (n < 5 || strcasecmp(e->d_name + n - 4, ".gba") != 0)
            continue;
        char path[1024], hdr[0xB0];
        snprintf(path, sizeof(path), "%s/%s", DATA_ROOT, e->d_name);
        FILE* f = fopen(path, "rb");
        if (!f)
            continue;
        size_t got = fread(hdr, 1, sizeof(hdr), f);
        fclose(f);
        if (got != sizeof(hdr))
            continue;
        for (size_t i = 0; i < 3; ++i)
            if (memcmp(hdr + 0xAC, known[i].code, 4) == 0 && (!wanted || strcmp(wanted, known[i].region) == 0)) {
                found = known[i].region;
                Log("cartridge: %s (%s)", e->d_name, known[i].region);
                break;
            }
    }
    if (d)
        closedir(d);
    return found;
}

static void ShowNotice(const char* name, int frames) {
    int w = 0, h = 0, id = LoadNotice(name, &w, &h);
    for (int i = 0; i < frames && !sQuit; i++) {
        HostPad pads[HOSTPAD_MAX];
        H_pad_poll(pads, HOSTPAD_MAX);
        H_gfx_begin(0xFF0A0A18u);
        if (id >= 0)
            DrawBox(id, w, h, (sH - h * (sW / 1920.0f)) / 2, 0xFFFFFFFFu);
        H_gfx_end();
        H_sleep_us(16000);
    }
    if (id >= 0)
        H_tex_free(id);
}

#if !defined(__SWITCH__) && !defined(__vita__)
/* The test host reports where the image stopped: game image addresses go to guest.map (build_switch.py). */
static void OnFault(int sig, siginfo_t* info, void* context) {
    const ucontext_t* uc = context;
#ifdef __arm__
    unsigned long long pc = uc->uc_mcontext.arm_pc, lr = uc->uc_mcontext.arm_lr;
#else
    unsigned long long pc = uc->uc_mcontext.pc, lr = uc->uc_mcontext.regs[30];
#endif
    Log("fault: signal %d at pc 0x%llx, lr 0x%llx, address 0x%llx", sig, pc, lr, (unsigned long long)(uintptr_t)info->si_addr);
    _exit(128 + sig);
}
#endif

int main(int argc, char** argv) {
#ifdef __SWITCH__
    (void)argc, (void)argv;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    hidInitializeTouchScreen();
    padInitializeDefault(&sPad);
    romfsInit();
    sAssetRoot = "romfs:/assets";
    mkdir("sdmc:/switch", 0777);
    mkdir(DATA_ROOT, 0777);
    mkdir(DATA_ROOT "/logs", 0777);
    rename(DATA_ROOT "/logs/ReKHCoM.log", DATA_ROOT "/logs/ReKHCoM.old.log");
    sLog = fopen(DATA_ROOT "/logs/ReKHCoM.log", "w");
    setenv("KHCOM_ASSETS", sAssetRoot, 1);
    setenv("KHCOM_DATA", DATA_ROOT, 1);
#elif defined(__vita__)
    (void)argc, (void)argv;
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
    sAssetRoot = "app0:assets";
    mkdir(DATA_ROOT, 0777);
    mkdir(DATA_ROOT "/logs", 0777);
    rename(DATA_ROOT "/logs/ReKHCoM.log", DATA_ROOT "/logs/ReKHCoM.old.log");
    sLog = fopen(DATA_ROOT "/logs/ReKHCoM.log", "w");
    setenv("KHCOM_ASSETS", sAssetRoot, 1);
    setenv("KHCOM_DATA", DATA_ROOT, 1);
#else
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--data") && i + 1 < argc)
            snprintf(sDataRoot, sizeof(sDataRoot), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--assets") && i + 1 < argc)
            sAssetRoot = argv[++i];
        else if (!strcmp(argv[i], "--shot") && i + 2 < argc && sShots < 16) {
            sShotFrame[sShots] = atoi(argv[++i]);
            sShotFile[sShots++] = argv[++i];
        }
    }
    struct sigaction sa = { 0 };
    sa.sa_sigaction = OnFault;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGTRAP, &sa, NULL);
    setenv("KHCOM_ASSETS", sAssetRoot, 0);
    setenv("KHCOM_DATA", sDataRoot, 0);
#endif
#ifdef REKHCOM_LINKED_GUEST
    Log("ReKHCoM (Vita, %s) starting", REKHCOM_VITA_REGION);
#else
    Log("ReKHCoM (Switch) starting");
#endif
    VideoInit();

    const char* region = FindRegion();
    if (!region) {
        Log("no cartridge found in %s: waiting for one", DATA_ROOT);
        while (!(region = FindRegion()) && !sQuit)
            ShowNotice("need-rom", 60);
        if (!region)
            H_exit(0);
    }
#ifdef REKHCOM_LINKED_GUEST
    /* one executable per region (the game image is linked in): the right one takes over */
    if (strcmp(region, REKHCOM_VITA_REGION) != 0) {
#ifdef __vita__
        char self[64];
        snprintf(self, sizeof(self), strcmp(region, "eu") ? "app0:eboot_%s.self" : "app0:eboot.bin", region);
        Log("starting %s", self);
        if (sLog)
            fflush(sLog);
        sceAppMgrLoadExec(self, NULL, NULL);
#endif
        while (!sQuit)
            ShowNotice("load-error", 600);
        H_exit(1);
    }
    uint64_t* table = gHostTable;
#else
    Ps5GuestHeader header;
    if (!LoadImage(region, &header)) {
        while (!sQuit)
            ShowNotice("load-error", 600);
        H_exit(1);
    }

    uint64_t* table = (uint64_t*)(uintptr_t)header.hostTable;
#endif
#define HF(name) [HF_##name] = (uint64_t)(uintptr_t)H_##name
#define HM(name, fn) [HF_##name] = (uint64_t)(uintptr_t)(fn)
    uint64_t t[HF_COUNT] = {
        HF(log), HF(exit), HF(ticks_us), HF(sleep_us), HF(getenv), HF(fopen), HF(fread), HF(fwrite), HF(fseek), HF(ftell),
        HF(fclose), HF(fflush), HF(stat), HF(remove), HF(rename), HF(mkdir), HF(dir_open), HF(dir_next), HF(dir_close),
        HF(gfx_size), HF(gfx_begin), HF(gfx_end), HF(tex_create), HF(tex_update), HF(tex_free), HF(gfx_draw), HF(gfx_clip),
        HF(gfx_blend), HF(audio_open), HF(audio_push), HF(audio_queued), HF(audio_clear), HF(audio_ratio), HF(pad_poll),
        HF(quit_requested),
        HM(sin, (double (*)(double))sin), HM(cos, (double (*)(double))cos), HM(tan, (double (*)(double))tan),
        HM(atan2, (double (*)(double, double))atan2), HM(pow, (double (*)(double, double))pow), HM(exp, (double (*)(double))exp),
        HM(log_e, (double (*)(double))log), HM(fmod, (double (*)(double, double))fmod), HM(sinf, (float (*)(float))sinf),
        HM(cosf, (float (*)(float))cosf), HM(powf, (float (*)(float, float))powf), HF(set_title), HF(notify), HF(touch_poll), HF(app_state), HF(hd_music), HF(hd_movie),
        HF(udp_open), HF(udp_resolve), HF(udp_send), HF(udp_recv),
    };
    for (int i = 0; i < HF_COUNT; ++i)
        if (!t[i]) {
            Log("bridge entry %d has no host function", i);
            return 1;
        }
    memcpy(table, t, sizeof(t));
    Log("starting the game (%d bridge entries)", HF_COUNT);
#ifdef REKHCOM_LINKED_GUEST
    int rc = guest_main();
#else
    int (*boot)(uint32_t) = (int (*)(uint32_t))(uintptr_t)header.boot;
    int rc = boot(header.stackTop);
#endif
    Log("the game returned %d", rc);
    H_exit(rc);
    return rc;
}
