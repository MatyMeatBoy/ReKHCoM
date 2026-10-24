/* PS4 host (OpenOrbis toolchain, homebrew enabler with RWX mappings such as GoldHEN).
 *
 * The game image is the PS5 build's guest (port/ps5/guest): the game and the port layer compiled as x32 (32-bit
 * pointers) into one flat image at PS5G_BASE. The PS4 runs the same x86-64 code, so this host only has to do what
 * app/ps5_host.c does with the PS4's own system: map the GBA regions at their hardware addresses and the image below
 * 4 GiB, fill the bridge table (shared/bridge_list.h) and start the game.
 *
 * Video is a small software rasterizer: every draw the game makes is an axis-aligned quad (shared/bridge.h), drawn
 * into SDL2's window surface (OpenOrbis' SDL2 presents it through VideoOut). Sound goes to sceAudioOut at 48 kHz from
 * a thread, controllers come from scePad.
 *
 * Built with -DREKHCOM_PS4 for the console; without it, the same file is a headless Linux test host (no SDL: the
 * frame stays in memory, --shot N file.ppm writes one, KHCOM_PADS scripts buttons as in port/ps5/test/linux_host.c). */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef REKHCOM_PS4
#include <SDL2/SDL.h>
#include <orbis/AudioOut.h>
#include <orbis/Pad.h>
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>
#define DATA_ROOT "/data/ReKHCom"
/* the toolchain's signal.h names the member __sa_sigaction but defines sa_sigaction without the underscores */
#undef sa_sigaction
#define sa_sigaction __sa_handler.__sa_sigaction
#endif

#include "bridge.h"

#ifndef DATA_ROOT
static char sDataRoot[512] = ".";
#define DATA_ROOT sDataRoot
#endif

#define GBA_LOW 0x02000000u
#define GBA_HIGH 0x0E010000u

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
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000u + (uint64_t)t.tv_nsec / 1000u;
}

/* ---- bridge: process, files --------------------------------------------- */

static void AudioClose(void);

static void H_log(const char* s) { Log("%s", s); }

static void H_exit(int code) {
    Log("the game ended (%d)", code);
    AudioClose();
    if (sLog)
        fflush(sLog);
#ifdef REKHCOM_PS4
    sceSystemServiceLoadExec("exit", NULL);
#endif
    _exit(code);
}

static uint64_t H_ticks_us(void) { return NowUs(); }
static void H_sleep_us(uint32_t us) { usleep(us); }

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
static int H_rename(const char* a, const char* b) { return rename(a, b); }
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

/* ---- video: software rasterizer into the screen ------------------------- */

static int sW = 1920, sH = 1080;
static uint32_t* sFb;   /* XRGB, sPitch pixels a row: the window surface on the console, a buffer in tests */
static int sPitch;
#ifdef REKHCOM_PS4
static SDL_Window* sWindow;
static SDL_Surface* sSurface;
static int sDirect;     /* the surface is XRGB8888: draw straight into it */
static uint32_t* sOwnFb;
#else
static int sShotFrame = -1;
static const char* sShotFile;
#endif
static int sFrame; /* frames shown (the test host's --shot and KHCOM_PADS count them) */

typedef struct Tex {
    int w, h, smooth;
    uint32_t* argb;     /* A R G B in a word, as the frame */
} Tex;
static Tex sTex[4096];
static int sClip[4], sHasClip, sBlend = 1;
static int* sColumns;   /* per output column: the texel column of the quad being drawn */

static void H_gfx_size(int* w, int* h) {
    *w = sW;
    *h = sH;
}

static void VideoInit(void) {
#ifdef REKHCOM_PS4
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        Log("SDL: %s", SDL_GetError());
    sWindow = SDL_CreateWindow("ReKHCoM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);
    sSurface = sWindow ? SDL_GetWindowSurface(sWindow) : NULL;
    if (sSurface) {
        sW = sSurface->w;
        sH = sSurface->h;
        sDirect = sSurface->format->format == SDL_PIXELFORMAT_ARGB8888 || sSurface->format->format == SDL_PIXELFORMAT_RGB888;
        Log("screen: %dx%d, %s%s", sW, sH, SDL_GetPixelFormatName(sSurface->format->format), sDirect ? "" : " (converted)");
    } else {
        Log("screen: no window surface (%s)", SDL_GetError());
    }
    if (!sDirect) {
        sOwnFb = calloc((size_t)sW * sH, 4);
        sFb = sOwnFb;
        sPitch = sW;
    }
#else
    sFb = calloc((size_t)sW * sH, 4);
    sPitch = sW;
#endif
    sColumns = malloc(sizeof(int) * (size_t)sW);
}

static void Present(void) {
#ifdef REKHCOM_PS4
    if (!sSurface)
        return;
    if (sDirect)
        SDL_UnlockSurface(sSurface);
    else
        SDL_ConvertPixels(sW, sH, SDL_PIXELFORMAT_ARGB8888, sOwnFb, sW * 4, sSurface->format->format, sSurface->pixels,
                          sSurface->pitch);
    SDL_UpdateWindowSurface(sWindow);
#else
    if (sFrame == sShotFrame && sShotFile) {
        FILE* f = fopen(sShotFile, "wb");
        if (f) {
            fprintf(f, "P6\n%d %d\n255\n", sW, sH);
            for (int i = 0; i < sW * sH; i++) {
                uint8_t p[3] = { (uint8_t)(sFb[i] >> 16), (uint8_t)(sFb[i] >> 8), (uint8_t)sFb[i] };
                fwrite(p, 1, 3, f);
            }
            fclose(f);
            Log("frame %d saved to %s", sFrame, sShotFile);
        }
    }
#endif
    sFrame++;
}

static void H_gfx_begin(uint32_t clear) {
#ifdef REKHCOM_PS4
    if (sDirect && sSurface) {
        SDL_LockSurface(sSurface);
        sFb = sSurface->pixels;
        sPitch = sSurface->pitch / 4;
    }
#endif
    sHasClip = 0;
    if (sFb && (clear >> 24))
        for (int y = 0; y < sH; y++) {
            uint32_t* row = sFb + (size_t)y * sPitch;
            for (int x = 0; x < sW; x++)
                row[x] = clear | 0xFF000000u;
        }
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
                sTex[i].argb[k] = ((uint32_t)s[3] << 24) | ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | s[2];
            return i;
        }
    return -1;
}
static void H_tex_update(int id, const void* rgba) {
    const uint8_t* s = rgba;
    Tex* t = &sTex[id];
    for (int k = 0; k < t->w * t->h; k++, s += 4)
        t->argb[k] = ((uint32_t)s[3] << 24) | ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | s[2];
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

/* src (A R G B) tinted by color (A R G B), over *d */
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

/* An axis-aligned quad (q[0] top left, q[5] bottom right; flips are in the texture coordinates). The shim adds 2, 4 or
   6 to u for pixel art and the MMPX filters (port/ps5/guest/src/sdl_shim.c): those are drawn with crisp texels. */
static void Rect(const Tex* t, const HostVertex* q) {
    float qx0 = q[0].x, qy0 = q[0].y, qx1 = q[5].x, qy1 = q[5].y;
    float u0 = q[0].u, v0 = q[0].v, u1 = q[5].u, v1 = q[5].v;
    int x0 = (int)ceilf(qx0 - 0.5f), y0 = (int)ceilf(qy0 - 0.5f), x1 = (int)ceilf(qx1 - 0.5f), y1 = (int)ceilf(qy1 - 0.5f);
    uint32_t color = q[0].argb;
    int crisp = 1;

    ClipBox(&x0, &y0, &x1, &y1);
    if (x0 >= x1 || y0 >= y1 || qx1 <= qx0 || qy1 <= qy0)
        return;
    if (!t) {
        for (int y = y0; y < y1; y++) {
            uint32_t* d = sFb + (size_t)y * sPitch;
            for (int x = x0; x < x1; x++)
                Put(&d[x], 0xFFFFFFFFu, color);
        }
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
            uint32_t* d = sFb + (size_t)y * sPitch;
            float v = v0 + (y + 0.5f - qy0) * dv;
            for (int x = x0; x < x1; x++)
                Put(&d[x], Bilinear(t, u0 + (x + 0.5f - qx0) * du, v), color);
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
        if (color == 0xFFFFFFFFu && !sBlend) {
            for (int x = x0; x < x1; x++)
                d[x] = s[sColumns[x]] | 0xFF000000u;
        } else {
            for (int x = x0; x < x1; x++)
                Put(&d[x], s[sColumns[x]], color);
        }
    }
}

/* Any other triangle (the shim's SDL_RenderGeometry): the edge-function rasterizer of the Linux test host. */
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
            Put(&sFb[(size_t)y * sPitch + x], s, a->argb);
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
    if (!sFb || (id >= 0 && !t))
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

/* A box the app draws over the game for five seconds (assets/<name>.rgba), fading out at the end. */
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
    sHasClip = 0;
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
            DrawBox(sToast, sToastW, sToastH, sH - sToastH * (sW / 1920.0f) - 90.0f * (sW / 1920.0f), (alpha << 24) | 0xFFFFFFu);
            sBlend = blend;
        }
    }
    Present();
}

/* ---- audio: 48 kHz stereo, a ring the output thread drains -------------- */

#define RING_FRAMES 16384
static int16_t sRing[RING_FRAMES * 2];
static volatile uint32_t sRead, sWrite; /* in frames, wrapping */
static pthread_mutex_t sRingLock = PTHREAD_MUTEX_INITIALIZER;
static double sRatio = 1.0, sPos;
static int16_t sLast[2];
static int sAudioOpen;
#ifdef REKHCOM_PS4
static int32_t sUser = -1, sPort = -1;
static pthread_t sAudioThread;
#endif

static uint32_t Queued(void) { return sWrite - sRead; }

static void RingPut(const int16_t* f, uint32_t n) {
    pthread_mutex_lock(&sRingLock);
    for (uint32_t i = 0; i < n && Queued() < RING_FRAMES; i++) {
        uint32_t at = (sWrite % RING_FRAMES) * 2;
        sRing[at] = f[i * 2];
        sRing[at + 1] = f[i * 2 + 1];
        sWrite++;
    }
    pthread_mutex_unlock(&sRingLock);
}

#ifdef REKHCOM_PS4
#define GRAIN 256
static void* AudioThread(void* arg) {
    static int16_t block[GRAIN * 2];
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&sRingLock);
        for (uint32_t i = 0; i < GRAIN; i++) {
            if (Queued()) {
                uint32_t at = (sRead % RING_FRAMES) * 2;
                block[i * 2] = sRing[at];
                block[i * 2 + 1] = sRing[at + 1];
                sRead++;
            } else {
                block[i * 2] = block[i * 2 + 1] = 0;
            }
        }
        pthread_mutex_unlock(&sRingLock);
        sceAudioOutOutput(sPort, block); /* blocks until the port takes the next grain */
    }
    return NULL;
}
#endif

static int sGameRate = 48000; /* the rate the game opened the audio at */
static int H_audio_open(int rate) {
    sGameRate = rate;
    if (rate != 48000)
        Log("audio: the game asked for %d Hz; the output is 48000 Hz", rate);
    if (sAudioOpen)
        return 0;
#ifdef REKHCOM_PS4
    sceAudioOutInit();
    sPort = sceAudioOutOpen(sUser, ORBIS_AUDIO_OUT_PORT_TYPE_MAIN, 0, GRAIN, 48000, ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
    if (sPort < 0) {
        Log("audio: sceAudioOutOpen failed (0x%08x)", (unsigned)sPort);
        return -1;
    }
    pthread_create(&sAudioThread, NULL, AudioThread, NULL);
#endif
    sAudioOpen = 1;
    Log("audio: 48000 Hz");
    return 0;
}

static void AudioClose(void) {
#ifdef REKHCOM_PS4
    if (sPort >= 0)
        sceAudioOutClose(sPort);
#endif
}

/* The game paces audio to its own 59.7275 Hz frame clock and stretches the sound by a hair (linear), as the PS5 does. */
static void GamePush(const int16_t* frames, uint32_t count) {
    static int16_t* out;
    static size_t cap;
    if (!count)
        return;
#ifndef REKHCOM_PS4
    /* the test host has no device: what is queued plays at once */
    sRead = sWrite;
#endif
    if (sRatio > 0.9999 && sRatio < 1.0001) {
        sPos = 0;
        sLast[0] = frames[count * 2 - 2];
        sLast[1] = frames[count * 2 - 1];
        RingPut(frames, count);
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
    RingPut(out, (uint32_t)n);
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
#ifdef REKHCOM_PS4
    return Queued();
#else
    return 800 * 4;
#endif
}
static void H_audio_clear(void) {
    pthread_mutex_lock(&sRingLock);
    sRead = sWrite;
    pthread_mutex_unlock(&sRingLock);
}
static void H_audio_ratio(double ratio) { sRatio = ratio > 0.5 && ratio < 2.0 ? ratio : 1.0; }

/* ---- controllers --------------------------------------------------------- */

static bool sQuit;
#ifdef REKHCOM_PS4
static int32_t sPad = -1;
#endif

static int H_pad_poll(HostPad* out, int count) {
    memset(out, 0, sizeof(HostPad) * (size_t)count);
#ifdef REKHCOM_PS4
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT)
                sQuit = true;
    }
    if (sPad < 0 || count < 1)
        return 0;
    OrbisPadData d;
    if (scePadReadState(sPad, &d) != 0 || !d.connected)
        return 0;
    /* RetroPad numbering (bridge.h): 0 B 1 Y 2 Select 3 Start 4-7 up down left right 8 A 9 X 10 L1 11 R1 12 L2 13 R2
       14 L3 15 R3 16 touchpad; Cross is B (south), Circle A (east), Square Y (west), Triangle X (north). The game turns
       the touchpad into Select and the touchpad + L3 / R3 into the save states, as on the PS5. */
    static const struct { uint32_t orbis; int bit; } map[] = {
        { ORBIS_PAD_BUTTON_CROSS, 0 }, { ORBIS_PAD_BUTTON_SQUARE, 1 }, { ORBIS_PAD_BUTTON_OPTIONS, 3 },
        { ORBIS_PAD_BUTTON_UP, 4 }, { ORBIS_PAD_BUTTON_DOWN, 5 }, { ORBIS_PAD_BUTTON_LEFT, 6 },
        { ORBIS_PAD_BUTTON_RIGHT, 7 }, { ORBIS_PAD_BUTTON_CIRCLE, 8 }, { ORBIS_PAD_BUTTON_TRIANGLE, 9 },
        { ORBIS_PAD_BUTTON_L1, 10 }, { ORBIS_PAD_BUTTON_R1, 11 }, { ORBIS_PAD_BUTTON_L2, 12 }, { ORBIS_PAD_BUTTON_R2, 13 },
        { ORBIS_PAD_BUTTON_L3, 14 }, { ORBIS_PAD_BUTTON_R3, 15 }, { ORBIS_PAD_BUTTON_TOUCH_PAD, 16 },
    };
    out[0].connected = 1;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (d.buttons & map[i].orbis)
            out[0].buttons |= 1u << map[i].bit;
    out[0].lx = (int16_t)((d.leftStick.x - 128) * 256);
    out[0].ly = (int16_t)((d.leftStick.y - 128) * 256);
    out[0].rx = (int16_t)((d.rightStick.x - 128) * 256);
    out[0].ry = (int16_t)((d.rightStick.y - 128) * 256);
    out[0].l2 = (d.buttons & ORBIS_PAD_BUTTON_L2) ? 255 : d.analogButtons.l2;
    out[0].r2 = (d.buttons & ORBIS_PAD_BUTTON_R2) ? 255 : d.analogButtons.r2;
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
/* no touch screen and no background state here (bridge entries of the Android 64-bit host) */
static int H_touch_poll(HostTouch* fingers, int max) { (void)fingers; (void)max; return 0; }
static int H_app_state(void) { return 0; }

/* ---- what the GBA does with stray pointers and divisions by zero --------------------------------------------------
 * The same handler as the PS5 host (port/ps5/app/ps5_host.c, where the reasoning is written out): a read or write the
 * GBA bus would decode is moved to where the bus sends it for one instruction, and a division by zero gives 0. The
 * register slots of the signal context are found with a probe. */
static int sGpr[16];
static int sRip = -1, sFlags = -1;
static uint32_t sBiosArea, sGuestEnd;
static struct sigaction sOldSegv, sOldFpe, sOldTrap;
static struct {
    bool active;
    int reg;
    uint64_t old, now;
} sPending;
static uint32_t sSeen[128];
static int sSeenCount;

#define SLOT(ctx, i) (((uint64_t*)(ctx))[i])
#define SENTINEL(n) (0xC0DE000000000000ull + 0x1010ull * (unsigned)((n) + 1))

static volatile uint64_t sProbeRip, sProbeRsp, sProbeRbp;

static void OnProbe(int sig, siginfo_t* info, void* context) {
    const uint64_t* w = (const uint64_t*)context;
    (void)sig, (void)info;
    for (int r = 0; r < 16; ++r) {
        if (r == 4 || r == 5)
            continue;
        for (int i = 0; i < 160; ++i)
            if (w[i] == SENTINEL(r)) {
                sGpr[r] = i;
                break;
            }
    }
    for (int i = 0; i < 160; ++i) {
        if (w[i] == sProbeRip && sRip < 0)
            sRip = i;
        if (w[i] == sProbeRsp && sGpr[4] < 0)
            sGpr[4] = i;
        if (w[i] == sProbeRbp && sGpr[5] < 0)
            sGpr[5] = i;
    }
    if (sRip >= 0)
        SLOT(context, sRip) += 2; /* past the ud2 */
}

static bool ProbeContext(void) {
    struct sigaction sa, old;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = OnProbe;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    for (int i = 0; i < 16; ++i)
        sGpr[i] = -1;
    sRip = -1;
    sigaction(SIGILL, &sa, &old);
    __asm__ volatile("movq %%rsp, %[rsp]\n\t"
                     "movq %%rbp, %[rbp]\n\t"
                     "leaq 1f(%%rip), %%rax\n\t"
                     "movq %%rax, %[rip]\n\t"
                     "movabsq $0xC0DE000000001010, %%rax\n\t"
                     "movabsq $0xC0DE000000002020, %%rcx\n\t"
                     "movabsq $0xC0DE000000003030, %%rdx\n\t"
                     "movabsq $0xC0DE000000004040, %%rbx\n\t"
                     "movabsq $0xC0DE000000007070, %%rsi\n\t"
                     "movabsq $0xC0DE000000008080, %%rdi\n\t"
                     "movabsq $0xC0DE000000009090, %%r8\n\t"
                     "movabsq $0xC0DE00000000A0A0, %%r9\n\t"
                     "movabsq $0xC0DE00000000B0B0, %%r10\n\t"
                     "movabsq $0xC0DE00000000C0C0, %%r11\n\t"
                     "movabsq $0xC0DE00000000D0D0, %%r12\n\t"
                     "movabsq $0xC0DE00000000E0E0, %%r13\n\t"
                     "movabsq $0xC0DE00000000F0F0, %%r14\n\t"
                     "movabsq $0xC0DE000000010100, %%r15\n\t"
                     "1: ud2\n\t"
                     : [rsp] "=m"(sProbeRsp), [rbp] "=m"(sProbeRbp), [rip] "=m"(sProbeRip)
                     :
                     : "rax", "rcx", "rdx", "rbx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory",
                       "cc");
    sigaction(SIGILL, &old, NULL);
    if (sRip < 0 || sGpr[4] < 1)
        return false;
    sFlags = sGpr[4] - 1; /* rip, cs, rflags, rsp, ss */
    for (int r = 0; r < 16; ++r)
        if (sGpr[r] < 0)
            return false;
    return true;
}

static uint32_t BusTarget(uint32_t a, int write) {
    uint32_t low = a & 0xFFFFu;
    uint32_t bios = sBiosArea + (low > 0xFFE0u ? low - 0x100u : low);
    uint32_t o;
    switch ((a >> 24) & 0xF) {
        case 0x2: return 0x02000000u | (a & 0x3FFFFu);
        case 0x3: return 0x03000000u | (a & 0x7FFFu);
        case 0x4: return (a & 0xFFFFFFu) < 0x400u ? 0x04000000u | (a & 0x3FFu) : bios;
        case 0x5: return 0x05000000u | (a & 0x3FFu);
        case 0x6:
            o = a & 0x1FFFFu;
            return 0x06000000u | (o >= 0x18000u ? o - 0x8000u : o);
        case 0x7: return 0x07000000u | (a & 0x3FFu);
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC: case 0xD: return write ? bios : 0x08000000u | (a & 0x1FFFFFFu);
        case 0xE: case 0xF: return 0x0E000000u | (a & 0xFFFFu);
        default: return bios;
    }
}

static int HasModRm1(uint8_t op) {
    if (op < 0x40)
        return (op & 7) < 4;
    switch (op) {
        case 0x62: case 0x63: case 0x69: case 0x6B: case 0xC0: case 0xC1: case 0xC4: case 0xC5:
        case 0xC6: case 0xC7: case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xF6: case 0xF7:
        case 0xFE: case 0xFF:
            return 1;
    }
    return (op >= 0x80 && op <= 0x8F) || (op >= 0xD8 && op <= 0xDF);
}

static int NoModRm2(uint8_t op) {
    return (op >= 0x05 && op <= 0x0B) || op == 0x0E || (op >= 0x30 && op <= 0x37) || op == 0x77 || (op >= 0x80 && op <= 0x8F) ||
           (op >= 0xA0 && op <= 0xA2) || (op >= 0xA8 && op <= 0xAA) || op >= 0xC8;
}

static int StrayRegister(const uint8_t* p, int64_t shift, int* reg, int64_t* delta, int* write) {
    int rex = 0;
    uint8_t op;
    *write = 0;
    for (int i = 0; i < 15; ++i, ++p)
        if (*p != 0x66 && *p != 0x67 && *p != 0xF2 && *p != 0xF3 && *p != 0xF0 && *p != 0x2E && *p != 0x3E && *p != 0x26 &&
            *p != 0x36 && *p != 0x64 && *p != 0x65)
            break;
    if ((*p & 0xF0) == 0x40)
        rex = *p++;
    op = *p++;
    if ((op >= 0xA4 && op <= 0xA7) || (op >= 0xAA && op <= 0xAF)) {
        *reg = (op == 0xAA || op == 0xAB || op == 0xAE || op == 0xAF) ? 7 : 6;
        *delta = shift;
        *write = op == 0xA4 || op == 0xA5 || op == 0xAA || op == 0xAB;
        return 1;
    }
    if (op == 0x0F) {
        op = *p++;
        if (op == 0x38 || op == 0x3A)
            ++p;
        else if (NoModRm2(op))
            return 0;
    } else {
        if (!HasModRm1(op))
            return 0;
        *write = op == 0x88 || op == 0x89 || op == 0xC6 || op == 0xC7 || (op < 0x40 && !(op & 2));
    }
    {
        uint8_t modrm = *p++;
        int mod = modrm >> 6, rm = modrm & 7, base = -1, index = -1, scale = 1;
        if (mod == 3)
            return 0;
        if (rm == 4) {
            uint8_t sib = *p++;
            scale = 1 << (sib >> 6);
            index = ((sib >> 3) & 7) | ((rex & 2) ? 8 : 0);
            base = (sib & 7) | ((rex & 1) ? 8 : 0);
            if (index == 4)
                index = -1;
            if ((sib & 7) == 5 && mod == 0)
                base = -1;
        } else if (rm == 5 && mod == 0) {
            return 0;
        } else {
            base = rm | ((rex & 1) ? 8 : 0);
        }
        if (base >= 0 && base != index) {
            *reg = base;
            *delta = shift;
            return 1;
        }
        if (index >= 0 && base < 0 && shift % scale == 0) {
            *reg = index;
            *delta = shift / scale;
            return 1;
        }
    }
    return 0;
}

static void Pass(const struct sigaction* old, int sig, siginfo_t* info, void* context) {
    static const char* const names[16] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
                                           "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };
    char regs[400];
    int n = 0;
    uint64_t frame = SLOT(context, sGpr[5]);
    Log("CRASH: signal %d at address 0x%llx, rip 0x%llx", sig, (unsigned long long)(uintptr_t)info->si_addr,
        (unsigned long long)SLOT(context, sRip));
    for (int r = 0; r < 16; ++r)
        n += snprintf(regs + n, sizeof(regs) - (size_t)n, "%s %llx ", names[r], (unsigned long long)SLOT(context, sGpr[r]));
    Log("  %s", regs);
    for (int depth = 0; depth < 24 && frame >= PS5G_BASE && frame + 16 < sGuestEnd && (frame & 7) == 0; ++depth) {
        Log("  frame %d: return 0x%llx", depth, (unsigned long long)((uint64_t*)(uintptr_t)frame)[1]);
        uint64_t next = ((uint64_t*)(uintptr_t)frame)[0];
        if (next <= frame)
            break;
        frame = next;
    }
    if ((old->sa_flags & SA_SIGINFO) && old->sa_sigaction)
        old->sa_sigaction(sig, info, context);
    else {
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

static void OnSegv(int sig, siginfo_t* info, void* context) {
    uint64_t fault = (uint64_t)(uintptr_t)info->si_addr, rip = SLOT(context, sRip);
    int reg, write;
    int64_t delta;
    if (sPending.active || rip < PS5G_BASE || rip >= sGuestEnd || fault >= 0x100000000ull || (fault >= GBA_LOW && fault < GBA_HIGH) ||
        (fault >= PS5G_BASE && fault < sGuestEnd) || (fault >= 0x10000 && fault < GBA_LOW)) {
        Pass(&sOldSegv, sig, info, context);
        return;
    }
    uint32_t target = BusTarget((uint32_t)fault, 0);
    bool seen = false;
    if (!StrayRegister((const uint8_t*)(uintptr_t)rip, (int64_t)target - (int64_t)fault, &reg, &delta, &write)) {
        Pass(&sOldSegv, sig, info, context);
        return;
    }
    if (write)
        target = BusTarget((uint32_t)fault, 1);
    delta = (int64_t)target - (int64_t)fault;
    for (int i = 0; i < sSeenCount; ++i)
        seen |= sSeen[i] == (uint32_t)rip;
    if (!seen && sSeenCount < 128) {
        sSeen[sSeenCount++] = (uint32_t)rip;
        Log("bios: %s %08x at %08x", write ? "write" : "read", (unsigned)fault, (unsigned)rip);
    }
    sPending.active = true;
    sPending.reg = reg;
    sPending.old = SLOT(context, sGpr[reg]);
    sPending.now = sPending.old + (uint64_t)delta;
    SLOT(context, sGpr[reg]) = sPending.now;
    SLOT(context, sFlags) |= 0x100; /* trap after this one instruction */
}

static void OnTrap(int sig, siginfo_t* info, void* context) {
    if (!sPending.active) {
        Pass(&sOldTrap, sig, info, context);
        return;
    }
    if (SLOT(context, sGpr[sPending.reg]) == sPending.now)
        SLOT(context, sGpr[sPending.reg]) = sPending.old;
    SLOT(context, sFlags) &= ~(uint64_t)0x100;
    sPending.active = false;
}

static int DivLength(const uint8_t* p, int* bytes) {
    const uint8_t* start = p;
    int size16 = 0;
    while (*p == 0x66 || *p == 0x67 || *p == 0x2E || *p == 0x3E || *p == 0x26 || *p == 0x36 || *p == 0x64 || *p == 0x65 ||
           (*p & 0xF0) == 0x40) {
        size16 |= *p == 0x66;
        ++p;
    }
    if (*p != 0xF6 && *p != 0xF7)
        return 0;
    *bytes = *p == 0xF6 ? 1 : (size16 ? 2 : 4);
    uint8_t modrm = p[1];
    if (((modrm >> 3) & 7) < 6)
        return 0;
    int mod = modrm >> 6, rm = modrm & 7;
    p += 2;
    if (mod != 3) {
        if (rm == 4) {
            uint8_t sib = *p++;
            if (mod == 0 && (sib & 7) == 5)
                p += 4;
        } else if (mod == 0 && rm == 5)
            p += 4;
        p += mod == 1 ? 1 : (mod == 2 ? 4 : 0);
    }
    return (int)(p - start);
}

static void OnFpe(int sig, siginfo_t* info, void* context) {
    uint64_t rip = SLOT(context, sRip);
    int bytes = 0, len = 0;
    if (rip >= PS5G_BASE && rip < sGuestEnd)
        len = DivLength((const uint8_t*)(uintptr_t)rip, &bytes);
    if (len == 0) {
        Pass(&sOldFpe, sig, info, context);
        return;
    }
    if (bytes == 1)
        SLOT(context, sGpr[0]) &= ~0xFFFFull;
    else if (bytes == 2) {
        SLOT(context, sGpr[0]) &= ~0xFFFFull;
        SLOT(context, sGpr[2]) &= ~0xFFFFull;
    } else {
        SLOT(context, sGpr[0]) = 0;
        SLOT(context, sGpr[2]) = 0;
    }
    SLOT(context, sRip) = rip + (uint64_t)len;
}

static void InstallFaultHandlers(const Ps5GuestHeader* h) {
    struct sigaction sa;
    sBiosArea = h->biosArea;
    sGuestEnd = h->regionEnd;
    if (!ProbeContext()) {
        Log("faults: the signal context could not be probed; stray pointers and divisions by zero are not covered");
        return;
    }
    Log("faults: context slots rip %d rsp %d rbp %d", sRip, sGpr[4], sGpr[5]);
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = OnSegv;
    sigaction(SIGSEGV, &sa, &sOldSegv);
    sigaction(SIGBUS, &sa, NULL);
    sa.sa_sigaction = OnFpe;
    sigaction(SIGFPE, &sa, &sOldFpe);
    sa.sa_sigaction = OnTrap;
    sigaction(SIGTRAP, &sa, &sOldTrap);
}

/* ---- the game image ------------------------------------------------------ */

/* Readable, writable and executable memory at a fixed low address (the GBA regions, the image). On the console this
   needs a homebrew enabler that allows RWX mappings (GoldHEN does). */
static bool MapAt(uint32_t address, size_t bytes, const char* what) {
#ifdef REKHCOM_PS4
    int flags = MAP_PRIVATE | MAP_ANON | MAP_FIXED;
#else
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE;
#endif
    void* p = mmap((void*)(uintptr_t)address, bytes, PROT_READ | PROT_WRITE | PROT_EXEC, flags, -1, 0);
    if (p != (void*)(uintptr_t)address) {
        Log("memory: %s at 0x%08x (%zu bytes) failed: %s", what, (unsigned)address, bytes, strerror(errno));
        return false;
    }
    Log("memory: %s at 0x%08x, %zu bytes", what, (unsigned)address, bytes);
    return true;
}

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
        char path[1024], header[0xB0];
        snprintf(path, sizeof(path), "%s/%s", DATA_ROOT, e->d_name);
        FILE* f = fopen(path, "rb");
        if (!f)
            continue;
        size_t got = fread(header, 1, sizeof(header), f);
        fclose(f);
        if (got != sizeof(header))
            continue;
        for (size_t i = 0; i < 3; ++i)
            if (memcmp(header + 0xAC, known[i].code, 4) == 0 && (!wanted || strcmp(wanted, known[i].region) == 0)) {
                found = known[i].region;
                Log("cartridge: %s (%s)", e->d_name, known[i].region);
                break;
            }
    }
    if (d)
        closedir(d);
    return found;
}

static bool LoadImage(const char* region, Ps5GuestHeader* header) {
    char path[700];
    snprintf(path, sizeof(path), "%s/guest-%s.bin", sAssetRoot, region);
    FILE* f = fopen(path, "rb");
    if (!f) {
        Log("the game image %s was not found", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    Ps5GuestHeader h;
    if (fread(&h, 1, sizeof(h), f) != sizeof(h) || h.magic != PS5G_MAGIC || h.imageEnd < (uint32_t)size) {
        fclose(f);
        Log("%s is not a game image", path);
        return false;
    }
    fseek(f, 0, SEEK_SET);
    /* the whole GBA range as one zeroed block, as on the desktop and the PS5: mirrors and gaps read zeros */
    if (!MapAt(GBA_LOW, GBA_HIGH - GBA_LOW, "the GBA address space") || !MapAt(PS5G_BASE, h.regionEnd - PS5G_BASE, "the game")) {
        fclose(f);
        return false;
    }
    bool ok = fread((void*)(uintptr_t)PS5G_BASE, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    if (ok) {
        *header = h;
        Log("game image: %s (%ld bytes, ends at 0x%08x)", path, size, h.imageEnd);
    }
    return ok;
}

static void ShowNotice(const char* name, int frames) {
    int w = 0, h = 0, id = LoadNotice(name, &w, &h);
    for (int i = 0; i < frames; i++) {
        HostPad pads[HOSTPAD_MAX];
        H_pad_poll(pads, HOSTPAD_MAX);
        H_gfx_begin(0xFF0A0A18u);
        if (id >= 0)
            DrawBox(id, w, h, (sH - h * (sW / 1920.0f)) / 2, 0xFFFFFFFFu);
        H_gfx_end();
        usleep(16000);
    }
    if (id >= 0)
        H_tex_free(id);
}

int main(int argc, char** argv) {
#ifdef REKHCOM_PS4
    (void)argc, (void)argv;
    {
        OrbisUserServiceInitializeParams param;
        param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
        sceUserServiceInitialize(&param);
        sceUserServiceGetInitialUser(&sUser);
    }
    scePadInit();
    sPad = scePadOpen(sUser, 0, 0, NULL);
    sAssetRoot = "/app0/assets";
    setenv("KHCOM_ASSETS", sAssetRoot, 1);
    setenv("KHCOM_DATA", DATA_ROOT, 1);
    mkdir(DATA_ROOT, 0777);
    mkdir(DATA_ROOT "/logs", 0777);
    rename(DATA_ROOT "/logs/ReKHCoM.log", DATA_ROOT "/logs/ReKHCoM.old.log");
    sLog = fopen(DATA_ROOT "/logs/ReKHCoM.log", "w");
#else
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--data") && i + 1 < argc)
            snprintf(sDataRoot, sizeof(sDataRoot), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--assets") && i + 1 < argc)
            sAssetRoot = argv[++i];
        else if (!strcmp(argv[i], "--shot") && i + 2 < argc) {
            sShotFrame = atoi(argv[++i]);
            sShotFile = argv[++i];
        }
    }
    setenv("KHCOM_ASSETS", sAssetRoot, 0);
    setenv("KHCOM_DATA", sDataRoot, 0);
#endif
    Log("ReKHCoM (PS4) starting; pad %d",
#ifdef REKHCOM_PS4
        sPad
#else
        -1
#endif
    );
    VideoInit();

    const char* region = FindRegion();
    if (!region) {
        Log("no cartridge found in %s: waiting for one", DATA_ROOT);
        while (!(region = FindRegion()) && !sQuit)
            ShowNotice("need-rom", 60);
        if (!region)
            return 0;
    }
    Ps5GuestHeader header;
    if (!LoadImage(region, &header)) {
        for (;;)
            ShowNotice("load-error", 600);
    }
    InstallFaultHandlers(&header);

    uint64_t* table = (uint64_t*)(uintptr_t)header.hostTable;
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
    int (*boot)(uint32_t) = (int (*)(uint32_t))(uintptr_t)header.boot;
    int rc = boot(header.stackTop);
    Log("the game returned %d", rc);
    H_exit(rc);
    return rc;
}
