/* Headless Linux host for the guest image: the same bridge as the PS5 host,
 * with files on the local disk and a small software rasterizer instead of the
 * GPU, so the game can be run and checked without a console.
 *
 *   linux_host guest.bin [--data DIR] [--shot N file.ppm] [--frames N]
 *   KHCOM_PADS="60-63:A,200-210:START"  scripted buttons (frame ranges)
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "bridge.h"

static uint64_t NowUs(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000u + (uint64_t)t.tv_nsec / 1000u;
}

/* ---- files ---- */
static FILE* sFiles[64];
static DIR* sDirs[16];

static int sFrame;
static void L_log(const char* s) { fprintf(stderr, "[game] %s\n", s); }
static void L_exit(int code) { fprintf(stderr, "[host] exit(%d) after %d presented frames\n", code, sFrame); fflush(NULL); _exit(code); }
static uint64_t L_ticks_us(void) { return NowUs(); }
static void L_sleep_us(uint32_t us) { usleep(us); }
static int L_getenv(const char* name, char* out, int cap) {
    const char* v = getenv(name);
    if (!v) return -1;
    snprintf(out, (size_t)cap, "%s", v);
    return (int)strlen(out);
}
static int L_fopen(const char* path, int mode) {
    static const char* const modes[] = { "rb", "wb", "ab", "r+b" };
    for (int i = 0; i < 64; i++)
        if (!sFiles[i]) {
            sFiles[i] = fopen(path, modes[mode & 3]);
            return sFiles[i] ? i : -1;
        }
    return -1;
}
static int L_fread(int h, void* buf, uint32_t n) { return (int)fread(buf, 1, n, sFiles[h]); }
static int L_fwrite(int h, const void* buf, uint32_t n) { return (int)fwrite(buf, 1, n, sFiles[h]); }
static int L_fseek(int h, int32_t off, int whence) { return fseek(sFiles[h], off, whence); }
static int L_ftell(int h) { return (int)ftell(sFiles[h]); }
static int L_fclose(int h) { int r = fclose(sFiles[h]); sFiles[h] = NULL; return r; }
static int L_fflush(int h) { return fflush(sFiles[h]); }
static int L_stat(const char* p) { struct stat s; return stat(p, &s) ? 0 : S_ISDIR(s.st_mode) ? 2 : 1; }
static int L_remove(const char* p) { return remove(p); }
static int L_rename(const char* a, const char* b) { return rename(a, b); }
static int L_mkdir(const char* p) { return mkdir(p, 0755); }
static int L_dir_open(const char* p) {
    for (int i = 0; i < 16; i++)
        if (!sDirs[i]) {
            sDirs[i] = opendir(p);
            return sDirs[i] ? i : -1;
        }
    return -1;
}
static int L_dir_next(int d, char* name, int cap) {
    struct dirent* e;
    while ((e = readdir(sDirs[d]))) {
        if (e->d_name[0] == '.') continue;
        snprintf(name, (size_t)cap, "%s", e->d_name);
        return 1;
    }
    return 0;
}
static void L_dir_close(int d) { closedir(sDirs[d]); sDirs[d] = NULL; }

/* ---- software GPU ---- */
#define OUT_W 1920
#define OUT_H 1080
static uint32_t sFb[OUT_W * OUT_H];
typedef struct Tex { int w, h, smooth; uint8_t* rgba; } Tex;
static Tex sTex[4096];
static int sClip[4], sHasClip, sBlend = 1;
static int sFrame;
static const char* sShotFile;
static int sShotFrame = -1;

static void L_gfx_size(int* w, int* h) { *w = OUT_W; *h = OUT_H; }
static void L_gfx_begin(uint32_t c) {
    sHasClip = 0;
    if (c >> 24)
        for (int i = 0; i < OUT_W * OUT_H; i++) sFb[i] = c | 0xFF000000u;
}
static void L_gfx_end(void) {
    if (sFrame == sShotFrame && sShotFile) {
        FILE* f = fopen(sShotFile, "wb");
        fprintf(f, "P6\n%d %d\n255\n", OUT_W, OUT_H);
        for (int i = 0; i < OUT_W * OUT_H; i++) {
            uint8_t p[3] = { (uint8_t)(sFb[i] >> 16), (uint8_t)(sFb[i] >> 8), (uint8_t)sFb[i] };
            fwrite(p, 1, 3, f);
        }
        fclose(f);
        fprintf(stderr, "[host] frame %d saved to %s\n", sFrame, sShotFile);
    }
    sFrame++;
}
static int L_tex_create(const void* rgba, int w, int h, int smooth) {
    for (int i = 0; i < 4096; i++)
        if (!sTex[i].rgba) {
            sTex[i].w = w; sTex[i].h = h; sTex[i].smooth = smooth;
            sTex[i].rgba = malloc((size_t)w * h * 4);
            memcpy(sTex[i].rgba, rgba, (size_t)w * h * 4);
            return i;
        }
    return -1;
}
static void L_tex_update(int id, const void* rgba) { memcpy(sTex[id].rgba, rgba, (size_t)sTex[id].w * sTex[id].h * 4); }
static void L_tex_free(int id) { free(sTex[id].rgba); sTex[id].rgba = NULL; }
static void L_gfx_clip(int x, int y, int w, int h) {
    sHasClip = w > 0;
    sClip[0] = x; sClip[1] = y; sClip[2] = x + w; sClip[3] = y + h;
}
static void L_gfx_blend(int on) { sBlend = on; }

static void Texel(const Tex* t, float u, float v, float* c) {
    if (!t->smooth) {
        int x = (int)floorf(u * t->w), y = (int)floorf(v * t->h);
        x = x < 0 ? 0 : x >= t->w ? t->w - 1 : x;
        y = y < 0 ? 0 : y >= t->h ? t->h - 1 : y;
        const uint8_t* p = t->rgba + ((size_t)y * t->w + x) * 4;
        for (int k = 0; k < 4; k++) c[k] = p[k];
        return;
    }
    float fx = u * t->w - 0.5f, fy = v * t->h - 0.5f;
    int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
    float ax = fx - x0, ay = fy - y0;
    for (int k = 0; k < 4; k++) c[k] = 0;
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) {
            int x = x0 + i, y = y0 + j;
            x = x < 0 ? 0 : x >= t->w ? t->w - 1 : x;
            y = y < 0 ? 0 : y >= t->h ? t->h - 1 : y;
            float wgt = (i ? ax : 1 - ax) * (j ? ay : 1 - ay);
            const uint8_t* p = t->rgba + ((size_t)y * t->w + x) * 4;
            for (int k = 0; k < 4; k++) c[k] += p[k] * wgt;
        }
}

static void Tri(const Tex* t, const HostVertex* a, const HostVertex* b, const HostVertex* c) {
    float area = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
    if (area == 0) return;
    float minx = fminf(a->x, fminf(b->x, c->x)), maxx = fmaxf(a->x, fmaxf(b->x, c->x));
    float miny = fminf(a->y, fminf(b->y, c->y)), maxy = fmaxf(a->y, fmaxf(b->y, c->y));
    int x0 = (int)floorf(minx), x1 = (int)ceilf(maxx), y0 = (int)floorf(miny), y1 = (int)ceilf(maxy);
    int cx0 = sHasClip ? sClip[0] : 0, cy0 = sHasClip ? sClip[1] : 0;
    int cx1 = sHasClip ? sClip[2] : OUT_W, cy1 = sHasClip ? sClip[3] : OUT_H;
    if (x0 < cx0) x0 = cx0;
    if (y0 < cy0) y0 = cy0;
    if (x1 > cx1) x1 = cx1;
    if (y1 > cy1) y1 = cy1;
    if (x1 > OUT_W) x1 = OUT_W;
    if (y1 > OUT_H) y1 = OUT_H;
    float sg = area < 0 ? -1.0f : 1.0f;
    float ia = 1.0f / (area * sg);
    const HostVertex* v[3] = { a, b, c };
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            float px = x + 0.5f, py = y + 0.5f, w[3];
            int inside = 1;
            for (int i = 0; i < 3; i++) {
                const HostVertex *p = v[(i + 1) % 3], *q = v[(i + 2) % 3];
                float e = ((q->x - p->x) * (py - p->y) - (q->y - p->y) * (px - p->x)) * sg;
                int topLeft = ((q->y - p->y) * sg < 0) || ((q->y == p->y) && ((q->x - p->x) * sg > 0));
                w[i] = e * ia;
                if (e < 0 || (e == 0 && !topLeft)) { inside = 0; break; }
            }
            if (!inside) continue;
            float s[4] = { 255, 255, 255, 255 };
            if (t) Texel(t, w[0] * a->u + w[1] * b->u + w[2] * c->u - (a->u > 1.5f ? 2.0f : 0.0f), w[0] * a->v + w[1] * b->v + w[2] * c->v, s);
            uint32_t col = a->argb;
            float ca = ((col >> 24) & 255) / 255.0f, cr = ((col >> 16) & 255) / 255.0f, cg = ((col >> 8) & 255) / 255.0f,
                  cb = (col & 255) / 255.0f;
            float sr = s[0] * cr, sgc = s[1] * cg, sb = s[2] * cb, sa = s[3] / 255.0f * ca;
            uint32_t* d = &sFb[y * OUT_W + x];
            if (!sBlend) {
                *d = 0xFF000000u | ((uint32_t)(sr + 0.5f) << 16) | ((uint32_t)(sgc + 0.5f) << 8) | (uint32_t)(sb + 0.5f);
            } else {
                float dr = (*d >> 16) & 255, dg = (*d >> 8) & 255, db = *d & 255;
                uint32_t r = (uint32_t)(sr * sa + dr * (1 - sa) + 0.5f), g = (uint32_t)(sgc * sa + dg * (1 - sa) + 0.5f),
                         bl = (uint32_t)(sb * sa + db * (1 - sa) + 0.5f);
                *d = 0xFF000000u | (r << 16) | (g << 8) | bl;
            }
        }
}

static void L_gfx_draw(int id, const void* verts, int count) {
    const HostVertex* v = verts;
    const Tex* t = id >= 0 ? &sTex[id] : NULL;
    for (int i = 0; i + 2 < count; i += 3) Tri(t, &v[i], &v[i + 1], &v[i + 2]);
}

/* ---- audio, pads ---- */
static uint64_t sAudioFrames;
static int L_audio_open(int rate) { (void)rate; return 0; }
static void L_audio_push(const int16_t* f, uint32_t n) { (void)f; sAudioFrames += n; }
static uint32_t L_audio_queued(void) { return 800 * 4; }
static void L_audio_clear(void) {}
static void L_audio_ratio(double r) { (void)r; }

static int L_pad_poll(HostPad* pads, int count) {
    static const char* const names[] = { "B", "Y", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT", "A", "X", "L", "R" };
    const char* script = getenv("KHCOM_PADS");
    memset(pads, 0, sizeof(HostPad) * (size_t)count);
    if (!script) return 0;
    pads[0].connected = 1;
    while (*script) {
        unsigned a, b, n = 0;
        char name[16];
        if (sscanf(script, "%u-%u:%15[A-Z]%n", &a, &b, name, &n) >= 3 && n) {
            if ((unsigned)sFrame >= a && (unsigned)sFrame <= b)
                for (unsigned i = 0; i < 12; i++)
                    if (!strcmp(name, names[i])) pads[0].buttons |= 1u << i;
            script += n;
        }
        while (*script == ',') script++;
        if (!n) break;
    }
    return 1;
}
static int L_quit_requested(void) { return 0; }
static void L_set_title(const char* s) { (void)s; }
static void L_notify(const char* s) { (void)s; }
static int L_touch_poll(HostTouch* f, int max) { (void)f; (void)max; return 0; }
static int L_app_state(void) { return 0; }
static int L_hd_music(int op, const char* path, int gain) { (void)op; (void)path; (void)gain; return 0; }
static int L_hd_movie(const char* m, const char* a, const char* b, const char* f) { (void)m; (void)a; (void)b; (void)f; return 0; }

/* the network link's socket (shared/udp_host.h): BSD sockets */
#define UDP_HOST_POSIX
#include "udp_host.h"
#define L_udp_open H_udp_open
#define L_udp_resolve H_udp_resolve
#define L_udp_send H_udp_send
#define L_udp_recv H_udp_recv
#define HF(name) [HF_##name] = (uint64_t)(uintptr_t)L_##name
#define HM(name, fn) [HF_##name] = (uint64_t)(uintptr_t)fn
static void* MapFixed(uint32_t base, size_t size) {
    void* p = mmap((void*)(uintptr_t)base, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void*)(uintptr_t)base) {
        fprintf(stderr, "cannot map %08x (+%zx): %s\n", base, size, strerror(errno));
        exit(2);
    }
    return p;
}

int main(int argc, char** argv) {
    const char* image = argc > 1 ? argv[1] : "guest.bin";
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--data") && i + 1 < argc) setenv("KHCOM_DATA", argv[++i], 1);
        else if (!strcmp(argv[i], "--shot") && i + 2 < argc) { sShotFrame = atoi(argv[++i]); sShotFile = argv[++i]; }
    }
    FILE* f = fopen(image, "rb");
    if (!f) { perror(image); return 2; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* tmp = malloc((size_t)size);
    if (fread(tmp, 1, (size_t)size, f) != (size_t)size) return 2;
    fclose(f);
    Ps5GuestHeader h;
    memcpy(&h, tmp, sizeof(h));
    if (h.magic != PS5G_MAGIC) { fprintf(stderr, "not a guest image\n"); return 2; }

    MapFixed(0x02000000, 0x40000);
    MapFixed(0x03000000, 0x10000);
    MapFixed(0x04000000, 0x10000);
    MapFixed(0x05000000, 0x10000);
    MapFixed(0x06000000, 0x20000);
    MapFixed(0x07000000, 0x10000);
    MapFixed(0x08000000, 0x02000000);
    MapFixed(0x0E000000, 0x10000);
    MapFixed(PS5G_BASE, h.regionEnd - PS5G_BASE);
    memcpy((void*)(uintptr_t)PS5G_BASE, tmp, (size_t)size);
    free(tmp);

    uint64_t* table = (uint64_t*)(uintptr_t)h.hostTable;
    static const uint64_t fns[HF_COUNT] = {
#define X(name) [HF_##name] = 0,
#include "bridge_list.h"
#undef X
    };
    (void)fns;
    uint64_t t[HF_COUNT] = {
        HF(log), HF(exit), HF(ticks_us), HF(sleep_us), HF(getenv), HF(fopen), HF(fread), HF(fwrite), HF(fseek), HF(ftell),
        HF(fclose), HF(fflush), HF(stat), HF(remove), HF(rename), HF(mkdir), HF(dir_open), HF(dir_next), HF(dir_close),
        HF(gfx_size), HF(gfx_begin), HF(gfx_end), HF(tex_create), HF(tex_update), HF(tex_free), HF(gfx_draw), HF(gfx_clip),
        HF(gfx_blend), HF(audio_open), HF(audio_push), HF(audio_queued), HF(audio_clear), HF(audio_ratio), HF(pad_poll),
        HF(quit_requested), HM(sin, sin), HM(cos, cos), HM(tan, tan), HM(atan2, atan2), HM(pow, pow), HM(exp, exp),
        HM(log_e, log), HM(fmod, fmod), HM(sinf, sinf), HM(cosf, cosf), HM(powf, powf), HF(set_title), HF(notify), HF(touch_poll), HF(app_state), HF(hd_music), HF(hd_movie),
        HF(udp_open), HF(udp_resolve), HF(udp_send), HF(udp_recv),
    };
    _Static_assert(sizeof(t) / sizeof(t[0]) == HF_COUNT, "bridge table out of step with bridge_list.h");
    memcpy(table, t, sizeof(t));

    int (*boot)(uint32_t) = (int (*)(uint32_t))(uintptr_t)h.boot;
    int rc = boot(h.stackTop);
    fprintf(stderr, "[host] guest returned %d\n", rc);
    return rc;
}
