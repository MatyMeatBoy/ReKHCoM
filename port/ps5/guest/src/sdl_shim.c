/* SDL3 subset over the host bridge: the renderer draws textured triangles and
 * rectangles on the host's GPU layer, audio and pads go to the host. */
#include <SDL3/SDL.h>
#include "bridge_calls.h"

static char sError[128] = "";
static Uint32 sInit;

const char* SDL_GetError(void) { return sError; }

static void SetError(const char* text) { SDL_strlcpy(sError, text, sizeof(sError)); }

size_t SDL_strlcpy(char* dst, const char* src, size_t size) {
    size_t n = strlen(src);
    if (size) {
        size_t c = n < size - 1 ? n : size - 1;
        memcpy(dst, src, c);
        dst[c] = 0;
    }
    return n;
}

size_t SDL_strlcat(char* dst, const char* src, size_t size) {
    size_t d = strlen(dst);
    if (d >= size) return d + strlen(src);
    return d + SDL_strlcpy(dst + d, src, size - d);
}

bool SDL_Init(Uint32 flags) { sInit |= flags; return true; }
void SDL_Quit(void) {}

Uint64 SDL_GetTicks(void) { return h_ticks_us() / 1000; }
Uint64 SDL_GetPerformanceCounter(void) { return h_ticks_us(); }
Uint64 SDL_GetPerformanceFrequency(void) { return 1000000; }
void SDL_Delay(Uint32 ms) { h_sleep_us(ms * 1000u); }
void SDL_DelayPrecise(Uint64 ns) { h_sleep_us((uint32_t)(ns / 1000)); }

const char* SDL_GetBasePath(void) {
    static char path[256];
    int n = h_getenv("KHCOM_DATA", path, 250);
    if (n <= 0) { SDL_strlcpy(path, "/data/ReKHCom", sizeof(path)); n = (int)strlen(path); }
    if (path[n - 1] != '/') { path[n++] = '/'; path[n] = 0; }
    return path;
}

bool SDL_CreateDirectory(const char* path) {
    char tmp[512];
    size_t i;
    SDL_strlcpy(tmp, path, sizeof(tmp));
    for (i = 1; tmp[i]; i++)
        if (tmp[i] == '/') {
            tmp[i] = 0;
            h_mkdir(tmp);
            tmp[i] = '/';
        }
    return h_mkdir(tmp) == 0 || h_stat(tmp) == 2;
}

void* SDL_LoadFile(const char* file, size_t* datasize) {
    FILE* f = fopen(file, "rb");
    long n;
    char* data;
    if (!f && file[0] != '/') {
        /* a relative name is a packaged asset (PortLoadAsset): /app0/assets, or KHCOM_ASSETS */
        char path[512];
        const char* root = SDL_getenv("KHCOM_ASSETS");
        SDL_snprintf(path, sizeof(path), "%s/%s", root && *root ? root : "/app0/assets", file);
        f = fopen(path, "rb");
    }
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc((size_t)n + 1);
    if (data && fread(data, 1, (size_t)n, f) == (size_t)n) {
        data[n] = 0;
        if (datasize) *datasize = (size_t)n;
    } else {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

/* ---- surfaces ---- */

static int BytesPer(SDL_PixelFormat f) { (void)f; return 4; }

SDL_Surface* SDL_CreateSurface(int w, int h, SDL_PixelFormat format) {
    SDL_Surface* s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->w = w; s->h = h; s->format = format; s->pitch = w * BytesPer(format);
    s->pixels = calloc((size_t)s->pitch, (size_t)h);
    s->owned = 1;
    if (!s->pixels) { free(s); return NULL; }
    return s;
}

SDL_Surface* SDL_CreateSurfaceFrom(int w, int h, SDL_PixelFormat format, void* pixels, int pitch) {
    SDL_Surface* s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->w = w; s->h = h; s->format = format; s->pitch = pitch; s->pixels = pixels;
    return s;
}

void SDL_DestroySurface(SDL_Surface* s) {
    if (!s) return;
    if (s->owned) free(s->pixels);
    free(s);
}

bool SDL_SetSurfaceBlendMode(SDL_Surface* s, SDL_BlendMode m) { (void)s; (void)m; return true; }

/* Bytes of a pixel as r, g, b, a. */
static void Unpack(SDL_PixelFormat f, const uint8_t* p, uint8_t* o) {
    if (f == SDL_PIXELFORMAT_RGBA32) { o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = p[3]; }
    else { o[0] = p[2]; o[1] = p[1]; o[2] = p[0]; o[3] = f == SDL_PIXELFORMAT_XRGB8888 ? 255 : p[3]; }
}

static void Pack(SDL_PixelFormat f, uint8_t* p, const uint8_t* c) {
    if (f == SDL_PIXELFORMAT_RGBA32) { p[0] = c[0]; p[1] = c[1]; p[2] = c[2]; p[3] = c[3]; }
    else { p[0] = c[2]; p[1] = c[1]; p[2] = c[0]; p[3] = f == SDL_PIXELFORMAT_XRGB8888 ? 255 : c[3]; }
}

SDL_Surface* SDL_ConvertSurface(SDL_Surface* s, SDL_PixelFormat format) {
    SDL_Surface* d = SDL_CreateSurface(s->w, s->h, format);
    int x, y;
    if (!d) return NULL;
    for (y = 0; y < s->h; y++)
        for (x = 0; x < s->w; x++) {
            uint8_t c[4];
            Unpack(s->format, (uint8_t*)s->pixels + y * s->pitch + x * 4, c);
            Pack(format, (uint8_t*)d->pixels + y * d->pitch + x * 4, c);
        }
    return d;
}

bool SDL_BlitSurfaceScaled(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, const SDL_Rect* dr, SDL_ScaleMode mode) {
    SDL_Rect s = { 0, 0, src->w, src->h }, d = { 0, 0, dst->w, dst->h };
    int x, y;
    (void)mode;
    if (sr) s = *sr;
    if (dr) d = *dr;
    if (d.w <= 0 || d.h <= 0) return true;
    for (y = 0; y < d.h; y++)
        for (x = 0; x < d.w; x++) {
            int sx = s.x + x * s.w / d.w, sy = s.y + y * s.h / d.h, dx = d.x + x, dy = d.y + y;
            uint8_t c[4], o[4];
            if (dx < 0 || dy < 0 || dx >= dst->w || dy >= dst->h || sx < 0 || sy < 0 || sx >= src->w || sy >= src->h)
                continue;
            Unpack(src->format, (uint8_t*)src->pixels + sy * src->pitch + sx * 4, c);
            Unpack(dst->format, (uint8_t*)dst->pixels + dy * dst->pitch + dx * 4, o);
            if (c[3] != 255) {
                int k;
                for (k = 0; k < 3; k++) c[k] = (uint8_t)((c[k] * c[3] + o[k] * (255 - c[3])) / 255);
                c[3] = (uint8_t)(c[3] + o[3] * (255 - c[3]) / 255);
            }
            Pack(dst->format, (uint8_t*)dst->pixels + dy * dst->pitch + dx * 4, c);
        }
    return true;
}

bool SDL_SaveBMP(SDL_Surface* s, const char* file) {
    FILE* f = fopen(file, "wb");
    uint8_t hdr[54];
    int y, rowBytes, size, x;
    uint8_t* row;
    if (!f) return false;
    rowBytes = s->w * 3;
    rowBytes = (rowBytes + 3) & ~3;
    size = 54 + rowBytes * s->h;
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    memcpy(hdr + 2, &size, 4);
    hdr[10] = 54; hdr[14] = 40;
    memcpy(hdr + 18, &s->w, 4);
    memcpy(hdr + 22, &s->h, 4);
    hdr[26] = 1; hdr[28] = 24;
    fwrite(hdr, 1, 54, f);
    row = calloc(1, (size_t)rowBytes);
    if (!row) { fclose(f); return false; }
    for (y = s->h - 1; y >= 0; y--) {
        for (x = 0; x < s->w; x++) {
            uint8_t c[4];
            Unpack(s->format, (uint8_t*)s->pixels + y * s->pitch + x * 4, c);
            row[x * 3] = c[2]; row[x * 3 + 1] = c[1]; row[x * 3 + 2] = c[0];
        }
        fwrite(row, 1, (size_t)rowBytes, f);
    }
    free(row);
    fclose(f);
    return true;
}

/* ---- window ---- */

struct SDL_Window { int w, h; };
struct SDL_Renderer {
    SDL_Window* window;
    Uint8 r, g, b, a;
    SDL_BlendMode blend;
    SDL_Rect view;
    bool hasView;
    SDL_Rect clip;
    bool hasClip;
    float sx, sy;
    int hostBlend;
    int vsync;
};
static SDL_Window sWindow;
static SDL_Renderer sRenderer;
static SDL_Window* sWindows[2] = { &sWindow, NULL };

SDL_Window* SDL_CreateWindow(const char* t, int w, int h, Uint64 f) { (void)t; (void)f; sWindow.w = w; sWindow.h = h; return &sWindow; }

SDL_Renderer* SDL_CreateRenderer(SDL_Window* w, const char* n) {
    (void)n;
    memset(&sRenderer, 0, sizeof(sRenderer));
    sRenderer.window = w;
    sRenderer.sx = sRenderer.sy = 1.0f;
    sRenderer.blend = SDL_BLENDMODE_BLEND;
    sRenderer.hostBlend = 1;
    sRenderer.vsync = 1;
    h_gfx_blend(1);
    return &sRenderer;
}
SDL_Renderer* SDL_GetRenderer(SDL_Window* w) { (void)w; return &sRenderer; }
const char* SDL_GetRendererName(SDL_Renderer* r) { (void)r; return "vulkan"; }
SDL_Window** SDL_GetWindows(int* count) {
    SDL_Window** list = malloc(2 * sizeof(*list)); /* freed by the caller */
    if (list) { list[0] = sWindows[0]; list[1] = NULL; }
    if (count) *count = list ? 1 : 0;
    return list;
}
bool SDL_SyncWindow(SDL_Window* w) { (void)w; return true; }
bool SDL_SetWindowFullscreen(SDL_Window* w, bool f) { (void)w; (void)f; return true; }
bool SDL_SetWindowFullscreenMode(SDL_Window* w, const SDL_DisplayMode* m) { (void)w; (void)m; return true; }
const SDL_DisplayMode* SDL_GetWindowFullscreenMode(SDL_Window* w) { (void)w; return NULL; }
bool SDL_SetWindowSize(SDL_Window* w, int a, int b) { (void)w; (void)a; (void)b; return true; }
bool SDL_SetWindowMinimumSize(SDL_Window* w, int a, int b) { (void)w; (void)a; (void)b; return true; }
bool SDL_GetWindowSizeInPixels(SDL_Window* w, int* a, int* b) { (void)w; h_gfx_size(a, b); return true; }
Uint64 SDL_GetWindowFlags(SDL_Window* w) { (void)w; return SDL_WINDOW_FULLSCREEN; }
SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* w) { (void)w; return 1; }
SDL_DisplayMode** SDL_GetFullscreenDisplayModes(SDL_DisplayID d, int* count) { (void)d; if (count) *count = 0; return NULL; }
bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID d, int w, int h, float r, bool hd, SDL_DisplayMode* m) {
    (void)d; (void)w; (void)h; (void)r; (void)hd; (void)m;
    return false;
}
const char* SDL_GetCurrentVideoDriver(void) { return "ps5"; }
bool SDL_ShowSimpleMessageBox(Uint32 f, const char* t, const char* m, SDL_Window* w) {
    (void)f; (void)t; (void)w;
    h_log(m);
    return true;
}

/* ---- renderer ---- */

bool SDL_SetRenderVSync(SDL_Renderer* r, int v) { r->vsync = v; return true; }
bool SDL_GetRenderVSync(SDL_Renderer* r, int* v) { *v = r->vsync; return true; }
bool SDL_GetCurrentRenderOutputSize(SDL_Renderer* r, int* w, int* h) {
    (void)r;
    h_gfx_size(w, h);
    return true;
}
bool SDL_SetRenderDrawColor(SDL_Renderer* r, Uint8 a, Uint8 b, Uint8 c, Uint8 d) { r->r = a; r->g = b; r->b = c; r->a = d; return true; }
bool SDL_SetRenderDrawBlendMode(SDL_Renderer* r, SDL_BlendMode m) { r->blend = m; return true; }
bool SDL_SetRenderScale(SDL_Renderer* r, float x, float y) { r->sx = x; r->sy = y; return true; }
bool SDL_GetRenderScale(SDL_Renderer* r, float* x, float* y) { *x = r->sx; *y = r->sy; return true; }

static bool sFrameOpen;

/* The host frame opens with the first draw after a present. */
static void Open(void) {
    if (!sFrameOpen) {
        h_gfx_begin(0);
        sFrameOpen = true;
    }
}

static void ApplyClip(SDL_Renderer* r) {
    int ox, oy;
    SDL_Rect c;
    int ow, oh;
    bool any = r->hasView || r->hasClip;
    if (!any) { h_gfx_clip(0, 0, 0, 0); return; }
    h_gfx_size(&ow, &oh);
    c.x = 0; c.y = 0; c.w = ow; c.h = oh;
    if (r->hasView) c = r->view;
    if (r->hasClip) {
        ox = c.x + r->clip.x; oy = c.y + r->clip.y;
        c.x = ox > c.x ? ox : c.x;
        c.y = oy > c.y ? oy : c.y;
        c.w = r->clip.w < c.w ? r->clip.w : c.w;
        c.h = r->clip.h < c.h ? r->clip.h : c.h;
    }
    if (c.w <= 0 || c.h <= 0) c.w = c.h = 1;
    h_gfx_clip(c.x, c.y, c.w, c.h);
}

bool SDL_SetRenderViewport(SDL_Renderer* r, const SDL_Rect* rect) {
    r->hasView = rect != NULL;
    if (rect) r->view = *rect;
    ApplyClip(r);
    return true;
}

bool SDL_SetRenderClipRect(SDL_Renderer* r, const SDL_Rect* rect) {
    r->hasClip = rect != NULL;
    if (rect) r->clip = *rect;
    ApplyClip(r);
    return true;
}

static void SetBlend(SDL_Renderer* r, SDL_BlendMode m) {
    int on = m != SDL_BLENDMODE_NONE;
    if (on != r->hostBlend) {
        h_gfx_blend(on);
        r->hostBlend = on;
    }
}

static uint32_t Argb(Uint8 a, Uint8 r, Uint8 g, Uint8 b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void Corner(HostVertex* v, SDL_Renderer* r, float x, float y, float u, float w, uint32_t c) {
    v->x = x * r->sx + (r->hasView ? r->view.x : 0);
    v->y = y * r->sy + (r->hasView ? r->view.y : 0);
    v->u = u;
    v->v = w;
    v->argb = c;
}

static void Quad(SDL_Renderer* r, int tex, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                 uint32_t c) {
    HostVertex q[6];
    Open();
    Corner(&q[0], r, x0, y0, u0, v0, c);
    Corner(&q[1], r, x1, y0, u1, v0, c);
    Corner(&q[2], r, x0, y1, u0, v1, c);
    q[3] = q[1];
    q[4] = q[2];
    Corner(&q[5], r, x1, y1, u1, v1, c);
    h_gfx_draw(tex, q, 6);
}

bool SDL_RenderClear(SDL_Renderer* r) {
    int w, h;
    SDL_Rect v = r->view;
    bool hv = r->hasView;
    h_gfx_size(&w, &h);
    r->hasView = false;
    SetBlend(r, SDL_BLENDMODE_NONE);
    Quad(r, -1, 0, 0, (float)w, (float)h, 0, 0, 0, 0, Argb(255, r->r, r->g, r->b));
    r->hasView = hv;
    r->view = v;
    return true;
}

bool SDL_RenderFillRect(SDL_Renderer* r, const SDL_FRect* rect) {
    int w, h;
    SDL_FRect f;
    if (rect) f = *rect;
    else {
        h_gfx_size(&w, &h);
        f.x = 0; f.y = 0; f.w = (float)w; f.h = (float)h;
    }
    if (r->a == 0 || f.w <= 0 || f.h <= 0) return true;
    SetBlend(r, r->a == 255 ? SDL_BLENDMODE_NONE : r->blend);
    Quad(r, -1, f.x, f.y, f.x + f.w, f.y + f.h, 0, 0, 0, 0, Argb(r->a, r->r, r->g, r->b));
    return true;
}

bool SDL_RenderFillRects(SDL_Renderer* r, const SDL_FRect* rects, int count) {
    int i;
    for (i = 0; i < count; i++) SDL_RenderFillRect(r, &rects[i]);
    return true;
}

void PortPs5LockSettings(void);

bool SDL_RenderPresent(SDL_Renderer* r) {
    static bool sToast;
    (void)r;
    PortPs5LockSettings();
    if (!sToast) { /* the game is running: tell how the save states are used */
        sToast = true;
        h_notify("toast-states");
    }
    Open();
    h_gfx_end();
    sFrameOpen = false;
    return true;
}
SDL_Surface* SDL_RenderReadPixels(SDL_Renderer* r, const SDL_Rect* rect) { (void)r; (void)rect; SetError("not supported"); return NULL; }
/* SDL's debug text, for the touch controls' labels (port/android/native/touch_android.c): 8-pixel cells, a 5x7
   glyph for the letters they use (each row 5 bits, high bit left); any other character leaves its cell empty. */
bool SDL_RenderDebugText(SDL_Renderer* r, float x, float y, const char* s) {
    static const struct { char c; unsigned char rows[7]; } glyphs[] = {
        { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } }, { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
        { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } }, { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
        { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } }, { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
        { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } }, { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
        { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
    };
    for (; *s; s++, x += 8) {
        size_t g;
        for (g = 0; g < sizeof(glyphs) / sizeof(glyphs[0]) && glyphs[g].c != *s; g++) {
        }
        if (g == sizeof(glyphs) / sizeof(glyphs[0]))
            continue;
        for (int row = 0; row < 7; row++)
            for (int col = 0; col < 5; col++)
                if (glyphs[g].rows[row] & (0x10 >> col)) {
                    SDL_FRect px = { x + 1 + col, y + row, 1, 1 };
                    SDL_RenderFillRect(r, &px);
                }
    }
    return true;
}

/* ---- textures ---- */

typedef struct Tex {
    SDL_Texture pub;
    SDL_PixelFormat format;
    int id[2];            /* host texture: [0] nearest, [1] smooth; -1 not made */
    uint8_t* rgba;        /* the pixels as the host wants them */
    SDL_ScaleMode scale;
    SDL_BlendMode blend;
    Uint8 cr, cg, cb, ca;
    bool picture;         /* a streaming texture: the game's picture, as opposed to glyphs and icons */
} Tex;

static void ToRgba(SDL_PixelFormat f, const uint8_t* src, int pitch, uint8_t* dst, int dstPitch, int w, int h) {
    int x, y;
    for (y = 0; y < h; y++) {
        const uint8_t* s = src + (size_t)y * pitch;
        uint8_t* d = dst + (size_t)y * dstPitch;
        if (f == SDL_PIXELFORMAT_RGBA32) { memcpy(d, s, (size_t)w * 4); continue; }
        for (x = 0; x < w; x++, s += 4, d += 4) {
            d[0] = s[2]; d[1] = s[1]; d[2] = s[0];
            d[3] = f == SDL_PIXELFORMAT_XRGB8888 ? 255 : s[3];
        }
    }
}

SDL_Texture* SDL_CreateTexture(SDL_Renderer* r, SDL_PixelFormat format, SDL_TextureAccess access, int w, int h) {
    Tex* t;
    (void)r;
    if (w <= 0 || h <= 0) { SetError("bad texture size"); return NULL; }
    t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->pub.w = w; t->pub.h = h;
    t->format = format;
    t->id[0] = t->id[1] = -1;
    t->rgba = calloc((size_t)w * h, 4);
    t->blend = SDL_BLENDMODE_NONE;
    t->scale = SDL_SCALEMODE_LINEAR;
    t->cr = t->cg = t->cb = t->ca = 255;
    t->picture = access == SDL_TEXTUREACCESS_STREAMING;
    if (!t->rgba) { free(t); SetError("out of memory"); return NULL; }
    return &t->pub;
}

SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s) {
    SDL_Texture* t = SDL_CreateTexture(r, s->format, SDL_TEXTUREACCESS_STATIC, s->w, s->h);
    if (t) SDL_UpdateTexture(t, NULL, s->pixels, s->pitch);
    return t;
}

void SDL_DestroyTexture(SDL_Texture* p) {
    Tex* t = (Tex*)p;
    if (!t) return;
    if (t->id[0] >= 0) h_tex_free(t->id[0]);
    if (t->id[1] >= 0) h_tex_free(t->id[1]);
    free(t->rgba);
    free(t);
}

bool SDL_UpdateTexture(SDL_Texture* p, const SDL_Rect* rect, const void* pixels, int pitch) {
    Tex* t = (Tex*)p;
    SDL_Rect r = { 0, 0, t->pub.w, t->pub.h };
    int k;
    if (rect) r = *rect;
    ToRgba(t->format, pixels, pitch, t->rgba + ((size_t)r.y * t->pub.w + r.x) * 4, t->pub.w * 4, r.w, r.h);
    for (k = 0; k < 2; k++)
        if (t->id[k] >= 0) h_tex_update(t->id[k], t->rgba);
    return true;
}

bool SDL_SetTextureScaleMode(SDL_Texture* p, SDL_ScaleMode m) { ((Tex*)p)->scale = m; return true; }
bool SDL_SetTextureBlendMode(SDL_Texture* p, SDL_BlendMode m) { ((Tex*)p)->blend = m; return true; }
bool SDL_SetTextureColorMod(SDL_Texture* p, Uint8 r, Uint8 g, Uint8 b) { Tex* t = (Tex*)p; t->cr = r; t->cg = g; t->cb = b; return true; }
bool SDL_SetTextureAlphaMod(SDL_Texture* p, Uint8 a) { ((Tex*)p)->ca = a; return true; }

/* The host texture for the filter this draw wants: nearest, or smooth (also used by pixel art). */
static int HostTex(Tex* t, bool smooth) {
    int k = smooth ? 1 : 0;
    if (t->id[k] < 0) t->id[k] = h_tex_create(t->rgba, t->pub.w, t->pub.h, smooth);
    return t->id[k];
}

/* Pixel art is sampled by the host's shader (crisp texels, blended seams) on a smooth texture; u + 2 asks for it,
 * u + 4 for the MMPX 3.2 filter (Options). */
#define PIXELART_U 2.0f
#define MMPX_U 4.0f      /* without edge blending */
#define MMPX_BLEND_U 6.0f /* with it */
extern int gPortMmpx;

bool SDL_RenderTextureRotated(SDL_Renderer* r, SDL_Texture* p, const SDL_FRect* src, const SDL_FRect* dst, double angle,
                              const SDL_FPoint* center, SDL_FlipMode flip) {
    Tex* t = (Tex*)p;
    SDL_FRect s = { 0, 0, (float)t->pub.w, (float)t->pub.h }, d;
    int ow, oh, id;
    float u0, v0, u1, v1, tmp;
    (void)angle; (void)center;
    if (src) s = *src;
    if (dst) d = *dst;
    else { h_gfx_size(&ow, &oh); d.x = 0; d.y = 0; d.w = (float)ow; d.h = (float)oh; }
    if (t->ca == 0 || d.w <= 0 || d.h <= 0 || s.w <= 0 || s.h <= 0) return true;
    id = HostTex(t, t->scale != SDL_SCALEMODE_NEAREST);
    if (id < 0) return false;
    u0 = s.x / t->pub.w; u1 = (s.x + s.w) / t->pub.w;
    v0 = s.y / t->pub.h; v1 = (s.y + s.h) / t->pub.h;
    if (flip & SDL_FLIP_HORIZONTAL) { tmp = u0; u0 = u1; u1 = tmp; }
    if (flip & SDL_FLIP_VERTICAL) { tmp = v0; v0 = v1; v1 = tmp; }
    /* MMPX covers every picture the game draws, whatever scaling mode the game asked for */
    if (gPortMmpx && t->picture) {
        float k = gPortMmpx == 1 ? MMPX_BLEND_U : MMPX_U; /* 1 on, 2 on without edge blending */
        u0 += k;
        u1 += k;
    } else if (t->scale == SDL_SCALEMODE_PIXELART) {
        u0 += PIXELART_U;
        u1 += PIXELART_U;
    }
    SetBlend(r, t->blend);
    Quad(r, id, d.x, d.y, d.x + d.w, d.y + d.h, u0, v0, u1, v1, Argb(t->ca, t->cr, t->cg, t->cb));
    return true;
}

bool SDL_RenderTexture(SDL_Renderer* r, SDL_Texture* p, const SDL_FRect* src, const SDL_FRect* dst) {
    return SDL_RenderTextureRotated(r, p, src, dst, 0, NULL, SDL_FLIP_NONE);
}

bool SDL_RenderGeometry(SDL_Renderer* r, SDL_Texture* p, const SDL_Vertex* v, int nv, const int* idx, int ni) {
    HostVertex out[300];
    Tex* t = (Tex*)p;
    int id = -1, n = 0, i, count = idx ? ni : nv;
    Open();
    if (t) {
        id = HostTex(t, t->scale != SDL_SCALEMODE_NEAREST);
        if (id < 0) return false;
    }
    SetBlend(r, t ? t->blend : r->blend);
    for (i = 0; i + 2 < count; i += 3) {
        int k;
        for (k = 0; k < 3; k++) {
            const SDL_Vertex* s = &v[idx ? idx[i + k] : i + k];
            Uint8 a = (Uint8)(s->color.a * 255.0f + 0.5f), cr = (Uint8)(s->color.r * 255.0f + 0.5f),
                  cg = (Uint8)(s->color.g * 255.0f + 0.5f), cb = (Uint8)(s->color.b * 255.0f + 0.5f);
            Corner(&out[n++], r, s->position.x, s->position.y, s->tex_coord.x, s->tex_coord.y, Argb(a, cr, cg, cb));
        }
        if (n == 300) { h_gfx_draw(id, out, n); n = 0; }
    }
    if (n) h_gfx_draw(id, out, n);
    return true;
}

/* ---- audio ---- */

struct SDL_AudioStream {
    int outRate, inRate;
    float ratio;
    bool open;
    float residue;
};
static SDL_AudioStream sAudio;

static void SetRatio(SDL_AudioStream* s) {
    h_audio_ratio((double)s->ratio * (double)s->inRate / (double)s->outRate);
}

SDL_AudioStream* SDL_OpenAudioDeviceStream(SDL_AudioDeviceID d, const SDL_AudioSpec* spec, SDL_AudioStreamCallback cb, void* u) {
    (void)d; (void)cb; (void)u;
    if (h_audio_open(spec->freq) < 0) { SetError("audio device unavailable"); return NULL; }
    sAudio.outRate = sAudio.inRate = spec->freq;
    sAudio.ratio = 1.0f;
    sAudio.open = true;
    return &sAudio;
}
void SDL_DestroyAudioStream(SDL_AudioStream* s) { s->open = false; h_audio_clear(); }
bool SDL_PauseAudioStreamDevice(SDL_AudioStream* s) { (void)s; return true; }
bool SDL_ResumeAudioStreamDevice(SDL_AudioStream* s) { (void)s; return true; }
bool SDL_ClearAudioStream(SDL_AudioStream* s) { (void)s; h_audio_clear(); return true; }
/* the fade before an HD movie (PortAudioMovieHold): the hosts have no output level; the movie just starts */
bool SDL_SetAudioStreamGain(SDL_AudioStream* s, float gain) { (void)s; (void)gain; return true; }
bool SDL_SetAudioStreamFrequencyRatio(SDL_AudioStream* s, float ratio) {
    if (ratio != s->ratio) { s->ratio = ratio; SetRatio(s); }
    return true;
}
bool SDL_SetAudioStreamFormat(SDL_AudioStream* s, const SDL_AudioSpec* in, const SDL_AudioSpec* out) {
    (void)out;
    if (in && in->freq != s->inRate) { s->inRate = in->freq; SetRatio(s); }
    return true;
}
bool SDL_GetAudioStreamFormat(SDL_AudioStream* s, SDL_AudioSpec* in, SDL_AudioSpec* out) {
    if (in) { in->format = SDL_AUDIO_F32; in->channels = 2; in->freq = s->inRate; }
    if (out) { out->format = SDL_AUDIO_F32; out->channels = 2; out->freq = s->outRate; }
    return true;
}

bool SDL_PutAudioStreamData(SDL_AudioStream* s, const void* buf, int len) {
    const float* in = buf;
    int16_t out[1024];
    int frames = len / 8, done = 0;
    (void)s;
    while (done < frames) {
        int n = frames - done < 512 ? frames - done : 512, i;
        for (i = 0; i < n * 2; i++) {
            float v = in[done * 2 + i] * 32767.0f;
            out[i] = (int16_t)(v > 32767.0f ? 32767 : v < -32768.0f ? -32768 : v);
        }
        h_audio_push(out, (uint32_t)n);
        done += n;
    }
    return true;
}

/* SDL counts what is queued in the input's format: the host reports frames at the device rate (outRate) */
int SDL_GetAudioStreamQueued(SDL_AudioStream* s) {
    uint64_t frames = h_audio_queued();
    if (s->outRate > 0 && s->inRate > 0)
        frames = frames * (uint64_t)s->inRate / (uint64_t)s->outRate;
    return (int)(frames * 8u);
}

/* ---- events, gamepads ---- */

typedef struct Pad { SDL_JoystickID id; HostPad now, prev; bool present; } Pad;
struct SDL_Gamepad { Pad* pad; };
static Pad sPads[HOSTPAD_MAX];
static struct SDL_Gamepad sPadHandles[HOSTPAD_MAX];
static SDL_Event sQueue[64];
static int sQHead, sQTail;

static void Push(const SDL_Event* e) {
    int next = (sQTail + 1) & 63;
    if (next == sQHead) return;
    sQueue[sQTail] = *e;
    sQTail = next;
}

/* RetroPad bit -> SDL gamepad button (the DualSense as the Xbox pad the game's controls are written for) */
static const signed char kButtonOf[17] = {
    SDL_GAMEPAD_BUTTON_SOUTH,       /* 0 Cross: A */
    SDL_GAMEPAD_BUTTON_WEST,        /* 1 Square: dodge roll */
    SDL_GAMEPAD_BUTTON_BACK,        /* 2 Select */
    SDL_GAMEPAD_BUTTON_START,       /* 3 Start (Options) */
    SDL_GAMEPAD_BUTTON_DPAD_UP,     /* 4 */
    SDL_GAMEPAD_BUTTON_DPAD_DOWN,   /* 5 */
    SDL_GAMEPAD_BUTTON_DPAD_LEFT,   /* 6 */
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT,  /* 7 */
    SDL_GAMEPAD_BUTTON_EAST,        /* 8 Circle: B */
    SDL_GAMEPAD_BUTTON_NORTH,       /* 9 Triangle: stock (L + R) */
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  /* 10 L1: L */
    SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, /* 11 R1: R */
    -1, -1,                         /* 12, 13 triggers: axes */
    SDL_GAMEPAD_BUTTON_LEFT_STICK,  /* 14 */
    SDL_GAMEPAD_BUTTON_RIGHT_STICK, /* 15 R3: return the stocked cards */
    SDL_GAMEPAD_BUTTON_BACK         /* 16 touchpad click: Select (the DualSense has no other) */
};

static bool Held(const HostPad* p, int button) {
    int bit;
    if (!p->connected) return false;
    /* touchpad held: L3 and R3 drive the save states (below), not the game */
    if (((p->buttons >> 16) & 1) && (button == SDL_GAMEPAD_BUTTON_LEFT_STICK || button == SDL_GAMEPAD_BUTTON_RIGHT_STICK))
        return false;
    for (bit = 0; bit < 17; bit++) /* several buttons can stand for one (Select) */
        if (kButtonOf[bit] == button && ((p->buttons >> bit) & 1))
            return true;
    return false;
}

/* Holding the touchpad for three seconds is the Escape key of the PC: it opens the options. */
static void TouchpadHold(const HostPad* pads) {
    static uint64_t sDownAt;
    static bool sFired;
    bool down = false;
    int i;

    for (i = 0; i < HOSTPAD_MAX; i++)
        if (pads[i].connected && ((pads[i].buttons >> 16) & 1))
            down = true;
    if (!down) {
        sDownAt = 0;
        sFired = false;
        return;
    }
    if (!sDownAt)
        sDownAt = h_ticks_us();
    if (!sFired && h_ticks_us() - sDownAt >= 3000000) {
        SDL_Event e;
        memset(&e, 0, sizeof(e));
        e.type = SDL_EVENT_KEY_DOWN;
        e.key.key = SDLK_ESCAPE;
        e.key.down = true;
        Push(&e);
        sFired = true;
    }
}

/* Save states, as F5 / F9 on the PC: the touchpad with L3 saves, the touchpad with R3 loads. */
static void StateKeys(const HostPad* prev, const HostPad* now) {
    static const struct { int bit; SDL_Keycode key; } keys[2] = { { 14, SDLK_F5 }, { 15, SDLK_F9 } };
    int i;

    if (!now->connected)
        return;
    for (i = 0; i < 2; i++) {
        bool both = ((now->buttons >> 16) & 1) && ((now->buttons >> keys[i].bit) & 1);
        bool before = ((prev->buttons >> 16) & 1) && ((prev->buttons >> keys[i].bit) & 1);
        if (both && !before) {
            SDL_Event e;
            memset(&e, 0, sizeof(e));
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = keys[i].key;
            e.key.down = true;
            Push(&e);
        }
    }
}

static void Refresh(void) {
    HostPad now[HOSTPAD_MAX];
    int i, b;
    memset(now, 0, sizeof(now));
    h_pad_poll(now, HOSTPAD_MAX);
    TouchpadHold(now);
    for (i = 0; i < HOSTPAD_MAX; i++) {
        Pad* p = &sPads[i];
        p->id = (SDL_JoystickID)(i + 1);
        p->prev = p->now;
        p->now = now[i];
        StateKeys(&p->prev, &p->now);
        if (p->now.connected && !p->present) {
            SDL_Event e;
            memset(&e, 0, sizeof(e));
            e.type = SDL_EVENT_GAMEPAD_ADDED;
            e.gdevice.which = p->id;
            Push(&e);
        }
        p->present = p->now.connected != 0;
        for (b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
            bool a = Held(&p->prev, b), c = Held(&p->now, b);
            if (a != c) {
                SDL_Event e;
                memset(&e, 0, sizeof(e));
                e.type = c ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
                e.gbutton.which = p->id;
                e.gbutton.button = (Uint8)b;
                e.gbutton.down = c;
                Push(&e);
            }
        }
    }
}

/* Touch screen and background state (the Android 64-bit host; other hosts report none): the fingers now against the
   last ones become FINGER_DOWN / MOTION / UP, a change of state WILL_ENTER_BACKGROUND / DID_ENTER_FOREGROUND. */
static void RefreshTouch(void) {
    static HostTouch last[HOSTTOUCH_MAX];
    static int lastCount, background;
    HostTouch now[HOSTTOUCH_MAX];
    int count = h_touch_poll(now, HOSTTOUCH_MAX), i, j, state = h_app_state();
    SDL_Event e;

    if (state != background) {
        memset(&e, 0, sizeof(e));
        e.type = state ? SDL_EVENT_WILL_ENTER_BACKGROUND : SDL_EVENT_DID_ENTER_FOREGROUND;
        Push(&e);
        background = state;
    }
    if (count < 0)
        count = 0;
    for (i = 0; i < count; i++) {
        const HostTouch* before = NULL;
        for (j = 0; j < lastCount; j++)
            if (last[j].id == now[i].id)
                before = &last[j];
        if (before && before->x == now[i].x && before->y == now[i].y)
            continue;
        memset(&e, 0, sizeof(e));
        e.type = before ? SDL_EVENT_FINGER_MOTION : SDL_EVENT_FINGER_DOWN;
        e.tfinger.touchID = 1;
        e.tfinger.fingerID = now[i].id;
        e.tfinger.x = now[i].x;
        e.tfinger.y = now[i].y;
        e.tfinger.dx = before ? now[i].x - before->x : 0;
        e.tfinger.dy = before ? now[i].y - before->y : 0;
        e.tfinger.pressure = 1.0f;
        Push(&e);
    }
    for (j = 0; j < lastCount; j++) {
        for (i = 0; i < count && now[i].id != last[j].id; i++) {
        }
        if (i < count)
            continue;
        memset(&e, 0, sizeof(e));
        e.type = SDL_EVENT_FINGER_UP;
        e.tfinger.touchID = 1;
        e.tfinger.fingerID = last[j].id;
        e.tfinger.x = last[j].x;
        e.tfinger.y = last[j].y;
        Push(&e);
    }
    memcpy(last, now, sizeof(HostTouch) * (size_t)count);
    lastCount = count;
}

bool SDL_PollEvent(SDL_Event* e) {
    static bool fresh = true; /* the pads are read once per drain of the queue */
    if (sQHead == sQTail && fresh) {
        fresh = false;
        if (h_quit_requested()) {
            SDL_Event q;
            memset(&q, 0, sizeof(q));
            q.type = SDL_EVENT_QUIT;
            Push(&q);
        }
        Refresh();
        RefreshTouch();
    }
    if (sQHead == sQTail) {
        fresh = true;
        return false;
    }
    *e = sQueue[sQHead];
    sQHead = (sQHead + 1) & 63;
    return true;
}

const bool* SDL_GetKeyboardState(int* n) { static bool keys[512]; if (n) *n = 512; return keys; }
SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode k, SDL_Keymod* m) { (void)k; if (m) *m = 0; return 0; }

SDL_JoystickID* SDL_GetGamepads(int* count) {
    /* the caller frees it with SDL_free */
    SDL_JoystickID* ids = malloc((HOSTPAD_MAX + 1) * sizeof(*ids));
    int i, n = 0;
    if (!ids) { if (count) *count = 0; return NULL; }
    for (i = 0; i < HOSTPAD_MAX; i++)
        if (sPads[i].present) ids[n++] = sPads[i].id;
    ids[n] = 0;
    if (count) *count = n;
    return ids;
}

SDL_Gamepad* SDL_GetGamepadFromID(SDL_JoystickID id) {
    if (id < 1 || id > HOSTPAD_MAX || !sPads[id - 1].present) return NULL;
    sPadHandles[id - 1].pad = &sPads[id - 1];
    return &sPadHandles[id - 1];
}
SDL_Gamepad* SDL_OpenGamepad(SDL_JoystickID id) { return SDL_GetGamepadFromID(id); }
/* the console's own controllers: never a Nintendo layout */
SDL_GamepadType SDL_GetGamepadType(SDL_Gamepad* gamepad) { (void)gamepad; return SDL_GAMEPAD_TYPE_STANDARD; }
const char* SDL_GetGamepadStringForButton(SDL_GamepadButton button) {
    static const char* const names[SDL_GAMEPAD_BUTTON_COUNT] = {
        "South", "East", "West", "North", "Back", "Guide", "Start", "L3", "R3",
        "L1", "R1", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "Misc"
    };
    return button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT ? names[button] : NULL;
}
const char* SDL_GetKeyName(SDL_Keycode key) {
    static char name[2];
    switch (key) {
    case SDLK_RETURN: return "Return";
    case SDLK_BACKSPACE: return "Backspace";
    case SDLK_DELETE: return "Delete";
    case SDLK_ESCAPE: return "Escape";
    case SDLK_SPACE: return "Space";
    case SDLK_UP: return "Up";
    case SDLK_DOWN: return "Down";
    case SDLK_LEFT: return "Left";
    case SDLK_RIGHT: return "Right";
    default:
        if (key > 0 && key < 128) { name[0] = (char)key; name[1] = 0; return name; }
        return "";
    }
}
bool SDL_GetGamepadButton(SDL_Gamepad* g, SDL_GamepadButton b) { return g && Held(&g->pad->now, b); }

Sint16 SDL_GetGamepadAxis(SDL_Gamepad* g, SDL_GamepadAxis a) {
    const HostPad* p;
    if (!g) return 0;
    p = &g->pad->now;
    switch (a) {
    case SDL_GAMEPAD_AXIS_LEFTX: return p->lx;
    case SDL_GAMEPAD_AXIS_LEFTY: return p->ly;
    case SDL_GAMEPAD_AXIS_RIGHTX: return p->rx;
    case SDL_GAMEPAD_AXIS_RIGHTY: return p->ry;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return (Sint16)(p->l2 * 128);
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return (Sint16)(p->r2 * 128);
    default: return 0;
    }
}
void SDL_SetGamepadEventsEnabled(bool e) { (void)e; }

SDL_JoystickID SDL_AttachVirtualJoystick(const SDL_VirtualJoystickDesc* d) { (void)d; SetError("no virtual joysticks"); return 0; }
SDL_Joystick* SDL_OpenJoystick(SDL_JoystickID id) { (void)id; return NULL; }
bool SDL_SetJoystickVirtualButton(SDL_Joystick* j, int b, bool d) { (void)j; (void)b; (void)d; return false; }
bool SDL_SetJoystickVirtualAxis(SDL_Joystick* j, int a, Sint16 v) { (void)j; (void)a; (void)v; return false; }
void SDL_UpdateJoysticks(void) {}
