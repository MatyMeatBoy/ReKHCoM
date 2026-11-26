/*
 * Re:CoM 1.5's 2D layouts (LAD) and sequences (SEQD), drawn the way its engine does, for the HD battle HUD
 * (HUD_HD_RECOM.md): the player's own gauge.lad / Cockpit.lad (their remastered UK_ versions, imported as hd/sprites/gHud*HdLad.lad, with their
 * whole textures as PNGs) and the logic read from Re:CoM's executable. The formats and the drawing are OpenKh's
 * (OpenKh.Kh2/Layout.cs, Sequence.cs, OpenKh.Engine/Renders/SequenceRenderer.cs: KH2 shares the engine): an
 * animation group plays its animations at a frame; each animation moves, pivots, scales, turns and tints one sprite
 * group; each sprite of a group is a rectangle of the texture with a colour per corner. Our game keeps its own rules;
 * only the pictures follow Re:CoM.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "port/port.h"
#include "port/port_hdlayout.h"
#include "port/port_hdsprites.h"
#include "../decompui/third_party/stb_image.h"

const char* PortHdSpritePath(char* file, size_t size, const char* name);

typedef struct HdLayout {
    unsigned char* data;
    int size;
    int validated; /* immutable bytes: full SEQD validation is done once at load */
} HdLayout;

typedef struct HdLayoutTexture {
    SDL_Texture* tex;
    int w, h, tried, requested;
    int texW, texH; /* the loaded picture's own size (w, h: the layout's coordinates) */
    SDL_Texture* flipped[4]; /* software renderer: the picture mirrored (1 across, 2 upside down, 3 both) */
} HdLayoutTexture;

static HdLayout sLayouts[HD_LAYOUT_COUNT];
static int sLayoutTried[HD_LAYOUT_COUNT];
static HdLayoutTexture sTextures[HD_LAYOUT_TEXTURE_COUNT];
/* the gauge cache's own renderer (SDL's software renderer on a surface) needs its own textures */
static SDL_Renderer* sCacheRenderer;
static HdLayoutTexture sCacheTextures[HD_LAYOUT_TEXTURE_COUNT];
/* software renderer: the decoded pictures (and their mirrored copies) stay, so the gauge cache's renderer gets its
   textures without decoding and mirroring them again */
static SDL_Surface* sPictures[HD_LAYOUT_TEXTURE_COUNT][4];
/* the 1.5+2.5's remastered layouts (UK_gauge.lad...): their UVs are the HD textures' own; the PS2 ones (gauge.lad)
   doubled miss the pieces the remaster moved (the ring's quarters, the bar) */
static const char* const kLayoutFiles[HD_LAYOUT_COUNT] = { "gHudGaugeHdLad", "gHudCockpitHdLad" };
static const char* const kTextureFiles[HD_LAYOUT_TEXTURE_COUNT] = { "gHudGaugeTex0", "gHudGaugeTex1",
                                                                     "gHudRikuGaugeTex0", "gHudCockpitTex" };

static int32_t I32(const unsigned char* p) { return (int32_t)(p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24); }
static int16_t I16(const unsigned char* p) { return (int16_t)(p[0] | p[1] << 8); }
static float F32(const unsigned char* p) { float f; uint32_t v = (uint32_t)I32(p); memcpy(&f, &v, 4); return f; }

static int ValidateLayout(const HdLayout* l);

static const HdLayout* Layout(int which) {
    if ((unsigned)which >= HD_LAYOUT_COUNT)
        return NULL;
    if (!sLayouts[which].data && !sLayoutTried[which]) {
        char file[PORT_HD_PATH_MAX], name[64];
        size_t size = 0;
        void* data;
        sLayoutTried[which] = 1;
        SDL_snprintf(name, sizeof(name), "%s", kLayoutFiles[which]);
        PortHdSpritePath(file, sizeof(file), name);
        /* the path helper names .png: the layout is the same name with .lad */
        if (strlen(file) > 4 && SDL_strcmp(file + strlen(file) - 4, ".png") == 0)
            memcpy(file + strlen(file) - 4, ".lad", 4);
        data = SDL_LoadFile(file, &size);
        if (data) {
            HdLayout candidate = { data, 0, 0 };
            if (size <= INT_MAX) {
                candidate.size = (int)size;
                if (ValidateLayout(&candidate)) {
                    candidate.validated = 1;
                    sLayouts[which] = candidate;
                } else {
                    SDL_free(data);
                }
            } else {
                SDL_free(data);
            }
        }
    }
    return sLayouts[which].data ? &sLayouts[which] : NULL;
}

static HdLayoutTexture* Texture(SDL_Renderer* renderer, int which) {
    HdLayoutTexture* t;
    if ((unsigned)which >= HD_LAYOUT_TEXTURE_COUNT)
        return NULL;
    t = renderer && renderer == sCacheRenderer ? &sCacheTextures[which] : &sTextures[which];
    if (!t->tex && !t->tried && renderer && t == &sCacheTextures[which] && sPictures[which][0]) {
        int f;
        t->tried = 1;
        t->tex = SDL_CreateTextureFromSurface(renderer, sPictures[which][0]);
        for (f = 1; f < 4; f++)
            if (sPictures[which][f]) t->flipped[f] = SDL_CreateTextureFromSurface(renderer, sPictures[which][f]);
        if (t->tex) SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND);
        t->w = sTextures[which].w;
        t->h = sTextures[which].h;
        t->texW = sTextures[which].texW;
        t->texH = sTextures[which].texH;
    }
    if (!t->tex && !t->tried && renderer) {
        char file[PORT_HD_PATH_MAX];
        int n;
        unsigned char* px;
        t->tried = 1;
        px = stbi_load(PortHdSpritePath(file, sizeof(file), kTextureFiles[which]), &t->w, &t->h, &n, 4);
        if (px) {
            SDL_Surface* s = SDL_CreateSurfaceFrom(t->w, t->h, SDL_PIXELFORMAT_RGBA32, px, t->w * 4);
            const int keep = which != HD_LAYOUT_TEX_COCKPIT && t == &sTextures[which] &&
                             SDL_strcmp(SDL_GetRendererName(renderer), SDL_SOFTWARE_RENDERER) == 0;
            if (s) {
                t->tex = SDL_CreateTextureFromSurface(renderer, s);
                /* SDL's software renderer copies only unmirrored rectangles: the gauges' mirrored parts (the ring's
                   quarters, the bars drawn upward) went to its triangle rasteriser, ten times the cost per pixel.
                   Mirrored copies of the gauge pictures (not the cockpit's) turn them into plain copies. */
                if (t->tex && which != HD_LAYOUT_TEX_COCKPIT && !SDL_getenv("KHCOM_HD_GAUGE_TRIANGLES") &&
                    SDL_strcmp(SDL_GetRendererName(renderer), SDL_SOFTWARE_RENDERER) == 0) {
                    int f;
                    for (f = 1; f < 4; f++) {
                        SDL_Surface* m = SDL_DuplicateSurface(s);
                        if (!m) continue;
                        if (((f & 1) && !SDL_FlipSurface(m, SDL_FLIP_HORIZONTAL)) ||
                            ((f & 2) && !SDL_FlipSurface(m, SDL_FLIP_VERTICAL))) {
                            SDL_DestroySurface(m);
                            continue;
                        }
                        t->flipped[f] = SDL_CreateTextureFromSurface(renderer, m);
                        if (keep && !sPictures[which][f]) sPictures[which][f] = m;
                        else SDL_DestroySurface(m);
                    }
                }
                if (keep && t->tex && !sPictures[which][0]) sPictures[which][0] = SDL_DuplicateSurface(s);
                SDL_DestroySurface(s);
            }
            stbi_image_free(px);
            if (t->tex)
                SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND);
            t->texW = t->w;
            t->texH = t->h;
            {
                /* a pack pre-scaled for a small panel: the layouts address the remaster's own texture size */
                static const int kOriginal[HD_LAYOUT_TEXTURE_COUNT] = { 256, 512, 256, 1024 };
                if (t->w < kOriginal[which] && t->w == t->h) t->w = t->h = kOriginal[which];
            }
        }
    }
    return t->tex ? t : NULL;
}

int PortHdLayoutReady(int layout, int texture) {
    if ((unsigned)texture >= HD_LAYOUT_TEXTURE_COUNT || !Layout(layout))
        return 0;
    /* Game logic may request a replacement, but only the renderer uploads it.
       Keep native drawing until decode, surface creation and upload succeeded. */
    sTextures[texture].requested = 1;
    return sTextures[texture].tex != NULL;
}

int PortHdHudPack(void); /* port_hdsprites.c: the HUD-only HD pack */
static int GaugeCachePrepare(SDL_Renderer* renderer, int w, int h);

void PortHdLayoutPrepare(SDL_Renderer* renderer) {
    int i;
    if (!gPortConfig.hdCards && !PortHdHudPack())
        return;
    for (i = 0; i < HD_LAYOUT_TEXTURE_COUNT; i++)
        if (sTextures[i].requested)
            Texture(renderer, i);
    /* The HD HUD pack (handhelds) is the battle HUD alone: its gauge pictures load before the first battle, one a
       frame, and stay, with the gauge cache and its own copies of them. The first battle had stalled on their decode
       (user, 26/11: "the first battle was slow, the second better"). */
    if (PortHdHudPack() && renderer && !SDL_getenv("KHCOM_HD_HUD_NO_PRELOAD")) {
        static const int kGauge[3] = { HD_LAYOUT_TEX_GAUGE, HD_LAYOUT_TEX_GAUGE_FACES, HD_LAYOUT_TEX_RIKU_FACES };
        static uint32_t lastStep;
        uint32_t frame = PortFrameCount();
        int ow = 0, oh = 0;
        if (frame == lastStep)
            return;
        for (i = 0; i < 3; i++)
            if (!sTextures[kGauge[i]].tried) {
                lastStep = frame;
                Texture(renderer, kGauge[i]);
                PortLog("hd hud preload: %s %s", kTextureFiles[kGauge[i]], sTextures[kGauge[i]].tex ? "loaded" : "missing");
                return;
            }
        if (SDL_strcmp(SDL_GetRendererName(renderer), SDL_SOFTWARE_RENDERER) != 0 || sCacheRenderer)
            ;
        else if (SDL_GetCurrentRenderOutputSize(renderer, &ow, &oh) && ow > 0 && oh > 0) {
            lastStep = frame;
            GaugeCachePrepare(renderer, SDL_min(ow, 1024), SDL_min(oh, 768));
            return;
        }
        if (sCacheRenderer)
            for (i = 0; i < 3; i++)
                if (!sCacheTextures[kGauge[i]].tried && sTextures[kGauge[i]].tex) {
                    lastStep = frame;
                    Texture(sCacheRenderer, kGauge[i]);
                    return;
                }
    }
}

/* The layout's sequence n: its bytes and size. Validate ranges before forming
   pointers, using division/subtraction so file-controlled products cannot wrap. */
static int TableRange(int size, int offset, int count, int stride) {
    return size >= 0 && offset >= 0 && offset <= size && count >= 0 &&
           count <= (size - offset) / stride;
}

static int IndexRange(int total, int first, int count) {
    return first >= 0 && first <= total && count >= 0 && count <= total - first;
}

static int ValidateSequence(const unsigned char* s, int size) {
    static const int stride[5] = { 44, 20, 4, 144, 36 };
    int counts[5], offsets[5], i;
    if (size < 48 || I32(s) != 0x44514553)
        return 0;
    for (i = 0; i < 5; i++) {
        counts[i] = I32(s + 8 + i * 8);
        offsets[i] = I32(s + 12 + i * 8);
        if (!TableRange(size, offsets[i], counts[i], stride[i]))
            return 0;
    }
    for (i = 0; i < counts[1]; i++) {
        int sprite = I32(s + offsets[1] + i * 20 + 16);
        if (sprite < 0 || sprite >= counts[0])
            return 0;
    }
    for (i = 0; i < counts[2]; i++) {
        const unsigned char* g = s + offsets[2] + i * 4;
        if (!IndexRange(counts[1], I16(g), I16(g + 2)))
            return 0;
    }
    for (i = 0; i < counts[3]; i++) {
        int group = I32(s + offsets[3] + i * 144 + 4);
        if (group < 0 || group >= counts[2])
            return 0;
    }
    for (i = 0; i < counts[4]; i++) {
        const unsigned char* g = s + offsets[4] + i * 36;
        if (!IndexRange(counts[3], I16(g), I16(g + 2)))
            return 0;
    }
    return 1;
}

static const unsigned char* Sequence(const HdLayout* l, int n, int* size) {
    const unsigned char* b;
    int count, table, start, end;
    if (!l || !l->data || l->size < 32)
        return NULL;
    b = l->data;
    count = I32(b + 24);
    table = I32(b + 28);
    if (n < 0 || n >= count || !TableRange(l->size, table, count, 4))
        return NULL;
    start = I32(b + table + 4 * n);
    end = n + 1 < count ? I32(b + table + 4 * (n + 1)) : l->size;
    if (start < 0 || end < start || end > l->size || end - start < 48 ||
        I32(b + start) != 0x44514553)
        return NULL;
    if (!l->validated && !ValidateSequence(b + start, end - start))
        return NULL;
    *size = end - start;
    return b + start;
}

static int ValidateLayout(const HdLayout* l) {
    int count, i, size;
    if (!l || !l->data || l->size < 32 || I32(l->data) != 0x4459414C)
        return 0;
    count = I32(l->data + 24);
    if (!TableRange(l->size, I32(l->data + 28), count, 4))
        return 0;
    for (i = 0; i < count; i++)
        if (!Sequence(l, i, &size))
            return 0;
    return 1;
}

int PortHdLayoutGroupFrames(int layout, int sequence, int group) {
    const HdLayout* l = Layout(layout);
    const unsigned char *s, *ag;
    int size, i, end = 0, count;
    if (!l || !(s = Sequence(l, sequence, &size)) || group < 0 || group >= I32(s + 40))
        return 0;
    ag = s + I32(s + 44) + group * 36;
    for (i = 0, count = I16(ag + 2); i < count; i++) {
        const unsigned char* a = s + I32(s + 36) + (I16(ag) + i) * 144;
        if (I32(a + 12) > end) end = I32(a + 12);
    }
    return end;
}

typedef struct Ctx {
    float x, y, px, py, sx, sy, rx, ry, rz;
    float c[4];
} Ctx;

static float Lerp(float t, float a, float b) { return a * (1.0f - t) + b * t; }

static void Color(uint32_t c, float* out) {
    out[0] = (c & 255) / 128.0f;
    out[1] = ((c >> 8) & 255) / 128.0f;
    out[2] = ((c >> 16) & 255) / 128.0f;
    out[3] = ((c >> 24) & 255) / 128.0f;
}

/* The polygon in (n vertices, a convex fan) cut to the rectangle clip (Sutherland-Hodgman), positions, texture
   coordinates and colours interpolated: what a mask lets through. Returns the vertices left in out (at most n + 4). */
static int ClipToRect(const SDL_Vertex* in, int n, const SDL_FRect* clip, SDL_Vertex* out) {
    SDL_Vertex a[12], b[12];
    int e, i, m = n;
    memcpy(a, in, sizeof(SDL_Vertex) * n);
    for (e = 0; e < 4 && m > 0; e++) {
        int k = 0;
        for (i = 0; i < m; i++) {
            const SDL_Vertex *p = &a[i], *q = &a[(i + 1) % m];
            float dp = e == 0 ? p->position.x - clip->x : e == 1 ? clip->x + clip->w - p->position.x
                       : e == 2 ? p->position.y - clip->y : clip->y + clip->h - p->position.y;
            float dq = e == 0 ? q->position.x - clip->x : e == 1 ? clip->x + clip->w - q->position.x
                       : e == 2 ? q->position.y - clip->y : clip->y + clip->h - q->position.y;
            if (dp >= 0)
                b[k++] = *p;
            if ((dp >= 0) != (dq >= 0) && k < 12) {
                float f = dp / (dp - dq);
                SDL_Vertex v;
                v.position.x = p->position.x + (q->position.x - p->position.x) * f;
                v.position.y = p->position.y + (q->position.y - p->position.y) * f;
                v.tex_coord.x = p->tex_coord.x + (q->tex_coord.x - p->tex_coord.x) * f;
                v.tex_coord.y = p->tex_coord.y + (q->tex_coord.y - p->tex_coord.y) * f;
                v.color.r = p->color.r + (q->color.r - p->color.r) * f;
                v.color.g = p->color.g + (q->color.g - p->color.g) * f;
                v.color.b = p->color.b + (q->color.b - p->color.b) * f;
                v.color.a = p->color.a + (q->color.a - p->color.a) * f;
                b[k++] = v;
            }
        }
        m = k;
        memcpy(a, b, sizeof(SDL_Vertex) * m);
    }
    memcpy(out, a, sizeof(SDL_Vertex) * m);
    return m;
}

/* SDL's software renderer (handhelds): an unturned quad of one colour is a scaled copy, many times cheaper than its
   triangle rasteriser (the HD gauge cost five frames' worth of instructions there). The source rectangle is clamped
   inside the texture, which SDL's own conversion did not do. Mirrored quads use mirrored copies of the picture;
   turned quads and gradients keep the triangles. */
static int SoftwareCopy(SDL_Renderer* r, const HdLayoutTexture* t, const SDL_Vertex* v, const SDL_Vertex* over,
                        int bright, const float* tu, const float* tv, int blend) {
    static int software = -1;
    SDL_Texture* tex = t->tex;
    SDL_FRect src, dst;
    float x0 = v[0].position.x, x1 = v[1].position.x, y0 = v[0].position.y, y1 = v[2].position.y, q;
    float u0 = tu[0], u1 = tu[1], w0 = tv[0], w1 = tv[2];
    int k, flip = 0;
    if (software < 0) software = SDL_strcmp(SDL_GetRendererName(r), SDL_SOFTWARE_RENDERER) == 0 && !SDL_getenv("KHCOM_HD_GAUGE_TRIANGLES");
    if (!software)
        return 0;
    /* corners 0 1 / 2 3: unturned means rows and columns share coordinates */
    if (SDL_fabsf(v[0].position.y - v[1].position.y) > 0.01f || SDL_fabsf(v[2].position.y - v[3].position.y) > 0.01f ||
        SDL_fabsf(v[0].position.x - v[2].position.x) > 0.01f || SDL_fabsf(v[1].position.x - v[3].position.x) > 0.01f)
        return 0;
    /* drawn leftward or upward: the same rectangle with its texture coordinates swapped, then a picture running the
       other way is the mirrored copy's (its coordinates 1 - u) */
    if (x1 < x0) { q = x0; x0 = x1; x1 = q; q = u0; u0 = u1; u1 = q; }
    if (y1 < y0) { q = y0; y0 = y1; y1 = q; q = w0; w0 = w1; w1 = q; }
    if (x1 - x0 < 0.01f || y1 - y0 < 0.01f)
        return 0;
    if (u1 < u0) { flip |= 1; u0 = 1.0f - u0; u1 = 1.0f - u1; }
    if (w1 < w0) { flip |= 2; w0 = 1.0f - w0; w1 = 1.0f - w1; }
    if (flip && !(tex = t->flipped[flip]))
        return 0;
    for (k = 1; k < 4; k++)
        if (v[k].color.r != v[0].color.r || v[k].color.g != v[0].color.g || v[k].color.b != v[0].color.b ||
            v[k].color.a != v[0].color.a || (bright && (over[k].color.r != over[0].color.r ||
            over[k].color.g != over[0].color.g || over[k].color.b != over[0].color.b)))
            return 0;
    src.x = SDL_clamp(u0 * t->texW, 0.0f, (float)t->texW);
    src.y = SDL_clamp(w0 * t->texH, 0.0f, (float)t->texH);
    src.w = SDL_clamp(u1 * t->texW, 0.0f, (float)t->texW) - src.x;
    src.h = SDL_clamp(w1 * t->texH, 0.0f, (float)t->texH) - src.y;
    if (src.w <= 0.0f) { src.w = 1.0f; if (src.x >= t->texW) src.x = (float)t->texW - 1.0f; } /* a stretched column */
    if (src.h <= 0.0f) { src.h = 1.0f; if (src.y >= t->texH) src.y = (float)t->texH - 1.0f; }
    dst.x = x0; dst.y = y0;
    dst.w = x1 - x0; dst.h = y1 - y0;
    SDL_SetTextureBlendMode(tex, blend == 1 ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorModFloat(tex, v[0].color.r, v[0].color.g, v[0].color.b);
    SDL_SetTextureAlphaModFloat(tex, v[0].color.a);
    SDL_RenderTexture(r, tex, &src, &dst);
    if (bright) {
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
        SDL_SetTextureColorModFloat(tex, over[0].color.r, over[0].color.g, over[0].color.b);
        SDL_RenderTexture(r, tex, &src, &dst);
    }
    SDL_SetTextureColorModFloat(tex, 1.0f, 1.0f, 1.0f);
    SDL_SetTextureAlphaModFloat(tex, 1.0f);
    return 1;
}

static SDL_Surface* sCacheSurface;

/* A turned quad drawn straight into the gauge cache's surface (corners top-left, top-right, bottom-left,
   bottom-right: a parallelogram), nearest texel, SDL's own 8-bit blend (BLEND: c = s a + d (1 - a), alpha
   a + d (1 - a); ADD: c = s a + d). SDL's triangle rasteriser cost about 620 instructions a pixel there; the low-HP
   alarm turns four large pieces and their bright pass every frame (user, 26/11: slow again at low HP). Only one
   colour for the whole quad; otherwise 0 and SDL draws it. */
static int CacheRasterQuad(const HdLayoutTexture* t, const SDL_Vertex* v, const SDL_FRect* clip, int add) {
    const SDL_Surface* pic;
    const unsigned char* tex;
    int which = (int)(t - sCacheTextures), x0, y0, x1, y1, x, y, k, tw, th, tpitch, pitch;
    float ex = v[1].position.x - v[0].position.x, ey = v[1].position.y - v[0].position.y;
    float fx = v[2].position.x - v[0].position.x, fy = v[2].position.y - v[0].position.y, det, minx, miny, maxx, maxy;
    float ia, ib, ic, id, du, dv;
    unsigned cr, cg, cb, ca;
    uint32_t* dst;
    if (!sCacheSurface || which < 0 || which >= HD_LAYOUT_TEXTURE_COUNT || !(pic = sPictures[which][0]))
        return 0;
    for (k = 1; k < 4; k++)
        if (v[k].color.r != v[0].color.r || v[k].color.g != v[0].color.g || v[k].color.b != v[0].color.b ||
            v[k].color.a != v[0].color.a)
            return 0;
    det = ex * fy - ey * fx;
    if (det > -0.01f && det < 0.01f)
        return 1; /* no area */
    /* parallelogram only: the fourth corner where the other three put it */
    if (SDL_fabsf(v[3].position.x - (v[1].position.x + fx)) > 0.5f || SDL_fabsf(v[3].position.y - (v[1].position.y + fy)) > 0.5f)
        return 0;
    SDL_FlushRenderer(sCacheRenderer); /* what SDL queued before goes under */
    minx = maxx = v[0].position.x;
    miny = maxy = v[0].position.y;
    for (k = 1; k < 4; k++) {
        minx = SDL_min(minx, v[k].position.x); maxx = SDL_max(maxx, v[k].position.x);
        miny = SDL_min(miny, v[k].position.y); maxy = SDL_max(maxy, v[k].position.y);
    }
    if (clip) {
        minx = SDL_max(minx, clip->x); miny = SDL_max(miny, clip->y);
        maxx = SDL_min(maxx, clip->x + clip->w); maxy = SDL_min(maxy, clip->y + clip->h);
    }
    x0 = SDL_max(0, (int)SDL_floorf(minx)); y0 = SDL_max(0, (int)SDL_floorf(miny));
    x1 = SDL_min(sCacheSurface->w, (int)SDL_ceilf(maxx)); y1 = SDL_min(sCacheSurface->h, (int)SDL_ceilf(maxy));
    if (x0 >= x1 || y0 >= y1)
        return 1;
    /* pixel centre -> (a, b) across the two edges */
    ia = fy / det; ib = -fx / det; ic = -ey / det; id = ex / det;
    du = v[1].tex_coord.x - v[0].tex_coord.x;
    dv = v[2].tex_coord.y - v[0].tex_coord.y;
    {
        /* the texture's corners need not run along the edges (mirrored quads): a full map */
        float dua = v[1].tex_coord.x - v[0].tex_coord.x, dub = v[2].tex_coord.x - v[0].tex_coord.x;
        float dva = v[1].tex_coord.y - v[0].tex_coord.y, dvb = v[2].tex_coord.y - v[0].tex_coord.y;
        tw = pic->w; th = pic->h; tpitch = pic->pitch;
        tex = (const unsigned char*)pic->pixels;
        cr = (unsigned)(SDL_clamp(v[0].color.r, 0.0f, 1.0f) * 255.0f + 0.5f);
        cg = (unsigned)(SDL_clamp(v[0].color.g, 0.0f, 1.0f) * 255.0f + 0.5f);
        cb = (unsigned)(SDL_clamp(v[0].color.b, 0.0f, 1.0f) * 255.0f + 0.5f);
        ca = (unsigned)(SDL_clamp(v[0].color.a, 0.0f, 1.0f) * 255.0f + 0.5f);
        pitch = sCacheSurface->pitch / 4;
        dst = (uint32_t*)sCacheSurface->pixels;
        (void)du; (void)dv;
        for (y = y0; y < y1; y++) {
            float py = y + 0.5f - v[0].position.y, px = x0 + 0.5f - v[0].position.x;
            float a = ia * px + ib * py, b = ic * px + id * py;
            uint32_t* line = dst + (size_t)y * pitch;
            for (x = x0; x < x1; x++, a += ia, b += ic) {
                float u, w;
                int tx, ty;
                const unsigned char* s;
                unsigned sa, sr, sg, sb;
                uint32_t d;
                if (a < 0.0f || a >= 1.0f || b < 0.0f || b >= 1.0f)
                    continue;
                u = v[0].tex_coord.x + a * dua + b * dub;
                w = v[0].tex_coord.y + a * dva + b * dvb;
                tx = (int)(u * tw);
                ty = (int)(w * th);
                if ((unsigned)tx >= (unsigned)tw || (unsigned)ty >= (unsigned)th)
                    continue;
                s = tex + (size_t)ty * tpitch + (size_t)tx * 4;
                sa = s[3] * ca / 255;
                if (!sa)
                    continue;
                sr = s[0] * cr / 255; sg = s[1] * cg / 255; sb = s[2] * cb / 255;
                d = line[x];
                if (add) {
                    unsigned r = ((d >> 16) & 255) + sr * sa / 255, g = ((d >> 8) & 255) + sg * sa / 255,
                             bl = (d & 255) + sb * sa / 255;
                    line[x] = (d & 0xFF000000u) | (SDL_min(r, 255u) << 16) | (SDL_min(g, 255u) << 8) | SDL_min(bl, 255u);
                } else {
                    unsigned inv = 255 - sa;
                    unsigned r = (sr * sa + ((d >> 16) & 255) * inv) / 255, g = (sg * sa + ((d >> 8) & 255) * inv) / 255,
                             bl = (sb * sa + (d & 255) * inv) / 255, al = sa + (d >> 24) * inv / 255;
                    line[x] = (al << 24) | (r << 16) | (g << 8) | bl;
                }
            }
        }
    }
    return 1;
}

/* The gauge cache's dry run: what would be drawn goes into a hash instead of the renderer. */
static int sGeoDry;
static uint32_t sGeoHash;
static void GeoMix(const void* p, size_t n) {
    const unsigned char* b = (const unsigned char*)p;
    while (n--) sGeoHash = (sGeoHash ^ *b++) * 16777619u;
}

/* One sprite of a part. A sprite with no texture at all (its UVs one point) is a mask: its box is returned in mask
   (1) and limits what the group draws after it, the way the gauge's ring shows only the turned quarters' part inside
   the box (the remaster's gauge.lad: groups 27-29). */
static int DrawSprite(SDL_Renderer* r, const HdLayoutTexture* t, const unsigned char* s, const Ctx* c,
                      const unsigned char* part, float uvScale, float ox, float oy, float scale, int blend,
                      const SDL_FRect* clip, SDL_FRect* mask) {
    static const int kIndex[6] = { 0, 1, 2, 1, 3, 2 };
    const unsigned char* sp = s + I32(s + 12) + I32(part + 16) * 44; /* sprites: count at 8, offset at 12 */
    float L = (float)I32(part), T = (float)I32(part + 4), R = (float)I32(part + 8), B = (float)I32(part + 12);
    float cx[4] = { L, R, L, R }, cy[4] = { T, T, B, B };
    /* a zero-wide (or zero-tall) source is one texel column (row) stretched over the part: the gauge's bars */
    int su0 = I32(sp + 4), sv0 = I32(sp + 8), su1 = I32(sp + 12), sv1 = I32(sp + 16);
    float u0 = (su0 == su1 ? su0 + 0.5f : (float)su0) * uvScale / t->w, v0 = (sv0 == sv1 ? sv0 + 0.5f : (float)sv0) * uvScale / t->h;
    float u1 = (su0 == su1 ? su1 + 0.5f : (float)su1) * uvScale / t->w, v1 = (sv0 == sv1 ? sv1 + 0.5f : (float)sv1) * uvScale / t->h;
    float tu[4] = { u0, u1, u0, u1 }, tv[4] = { v0, v0, v1, v1 };
    float cr = SDL_cosf(-c->rz), sr = SDL_sinf(-c->rz), crx = SDL_cosf(-c->rx), cry = SDL_cosf(-c->ry);
    SDL_Vertex v[4], over[4];
    int bright = 0;
    int k;
    for (k = 0; k < 4; k++) {
        float col[4], x = cx[k] + c->px, y = cy[k] + c->py, nx;
        x *= c->sx;
        y *= c->sy;
        y *= crx; /* a turn about X or Y, seen flat: the picture narrows */
        x *= cry;
        nx = x * cr - y * sr;
        y = x * sr + y * cr;
        x = nx + c->x;
        y += c->y;
        Color((uint32_t)I32(sp + 28 + 4 * k), col);
        v[k].position.x = ox + x * scale;
        v[k].position.y = oy + y * scale;
        v[k].color.r = SDL_min(1.0f, col[0] * c->c[0]);
        v[k].color.g = SDL_min(1.0f, col[1] * c->c[1]);
        v[k].color.b = SDL_min(1.0f, col[2] * c->c[2]);
        v[k].color.a = SDL_min(1.0f, col[3] * c->c[3]);
        /* the PS2's modulate goes to twice the texel (0x80 is 1.0, 0xFF nearly 2.0): what a vertex colour asks past
           1.0 is added in a second pass (the enemy gauge's green is 1.16/1.81/0.06 at the top) */
        over[k].position = v[k].position;
        over[k].color.r = SDL_max(0.0f, col[0] * c->c[0] - 1.0f);
        over[k].color.g = SDL_max(0.0f, col[1] * c->c[1] - 1.0f);
        over[k].color.b = SDL_max(0.0f, col[2] * c->c[2] - 1.0f);
        over[k].color.a = v[k].color.a;
        bright |= over[k].color.r > 0.0f || over[k].color.g > 0.0f || over[k].color.b > 0.0f;
        v[k].tex_coord.x = over[k].tex_coord.x = tu[k];
        v[k].tex_coord.y = over[k].tex_coord.y = tv[k];
    }
    if (sGeoDry) {
        GeoMix(v, sizeof(v));
        GeoMix(&bright, sizeof(bright));
        GeoMix(&blend, sizeof(blend));
        GeoMix(&t, sizeof(t));
        if (clip) GeoMix(clip, sizeof(*clip));
    }
    if (su0 == su1 && sv0 == sv1) {
        float x0 = v[0].position.x, y0 = v[0].position.y, x1 = x0, y1 = y0;
        for (k = 1; k < 4; k++) {
            x0 = SDL_min(x0, v[k].position.x);
            y0 = SDL_min(y0, v[k].position.y);
            x1 = SDL_max(x1, v[k].position.x);
            y1 = SDL_max(y1, v[k].position.y);
        }
        mask->x = x0;
        mask->y = y0;
        mask->w = x1 - x0;
        mask->h = y1 - y0;
        return 1;
    }
    if (sGeoDry)
        return 0;
    if (r == sCacheRenderer && r && !SDL_getenv("KHCOM_HD_GAUGE_SDL_TURNED") &&
        (clip || !(SDL_fabsf(v[0].position.y - v[1].position.y) <= 0.01f && SDL_fabsf(v[0].position.x - v[2].position.x) <= 0.01f)) &&
        CacheRasterQuad(t, v, clip, blend == 1)) {
        if (bright)
            CacheRasterQuad(t, over, clip, 1);
        return 0;
    }
    SDL_SetTextureBlendMode(t->tex, blend == 1 ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    if (clip) {
        /* the quad as a fan (top-left, top-right, bottom-right, bottom-left), cut to the mask */
        SDL_Vertex quad[4] = { v[0], v[1], v[3], v[2] }, out[8];
        int idx[18], n = ClipToRect(quad, 4, clip, out), i;
        if (n < 3)
            return 0;
        for (i = 0; i < n - 2; i++) {
            idx[i * 3] = 0;
            idx[i * 3 + 1] = i + 1;
            idx[i * 3 + 2] = i + 2;
        }
        out[1].position.x += 0.01f; /* off SDL's software copy path, as below */
        SDL_RenderGeometry(r, t->tex, out, n, idx, (n - 2) * 3);
        if (bright) {
            SDL_Vertex quad2[4] = { over[0], over[1], over[3], over[2] };
            if ((n = ClipToRect(quad2, 4, clip, out)) >= 3) {
                out[1].position.x += 0.01f;
                SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_ADD);
                SDL_RenderGeometry(r, t->tex, out, n, idx, (n - 2) * 3);
            }
        }
        return 0;
    }
    if (SoftwareCopy(r, t, v, over, bright, tu, tv, blend))
        return 0;
    /* SDL's software renderer turns an axis-aligned quad into a copy that reads past the texture (port_hdsprites.c,
       Quad): a hundredth of a pixel keeps it on the triangle path */
    v[1].position.x += 0.01f;
    v[1].position.y += 0.01f;
    SDL_RenderGeometry(r, t->tex, v, 4, kIndex, 6);
    if (bright) {
        over[1].position = v[1].position;
        SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_ADD);
        SDL_RenderGeometry(r, t->tex, over, 4, kIndex, 6);
    }
    return 0;
}

int PortHdLayoutDraw(SDL_Renderer* renderer, int layout, int texture, int sequence, int group, int frame, float x,
                     float y, float scale, float alpha, const float* tint) {
    const HdLayout* l = Layout(layout);
    HdLayoutTexture* t = Texture(renderer, texture);
    const unsigned char *s, *ag;
    int size, i, count, loopStart, loopEnd, masked = 0;
    SDL_FRect clip = { 0, 0, 0, 0 };
    if (!l || !t || !(s = Sequence(l, sequence, &size)) || group < 0 || group >= I32(s + 40))
        return 0;
    ag = s + I32(s + 44) + group * 36;
    count = I16(ag + 2);
    loopStart = I32(ag + 8);
    loopEnd = I32(ag + 12);
    if (frame & HD_LAYOUT_HOLD(0)) {
        frame &= ~HD_LAYOUT_HOLD(0); /* held: the gauge's full length is its last frame, not its loop's start */
    } else if (I16(ag + 4) == 0) { /* looping: from LoopStart to LoopEnd (or the last frame) */
        if (loopEnd == 0)
            loopEnd = PortHdLayoutGroupFrames(layout, sequence, group);
        if (loopEnd > loopStart && frame >= loopEnd)
            frame = loopStart + (frame - loopStart) % (loopEnd - loopStart);
    }
    for (i = 0; i < count; i++) {
        const unsigned char* a = s + I32(s + 36) + (I16(ag) + i) * 144;
        int flags = I32(a), f0 = I32(a + 8), f1 = I32(a + 12), sg = I32(a + 4), p;
        SDL_FRect box;
        const unsigned char* g;
        float delta, tt, c0[4], c1[4];
        Ctx c;
        if (frame < f0 || frame > f1)
            continue;
        delta = f1 > f0 ? (float)(frame - f0) / (f1 - f0) : 0.0f;
        tt = (flags & 1) ? delta : (SDL_sinf(delta * SDL_PI_F - SDL_PI_F / 2) + 1.0f) / 2.0f;
        memset(&c, 0, sizeof(c));
        c.x = (flags & 0x1000) ? (float)I32(a + 16) : Lerp(tt, (float)I32(a + 16), (float)I32(a + 20));
        c.y = (flags & 0x1000) ? (float)I32(a + 24) : Lerp(tt, (float)I32(a + 24), (float)I32(a + 28));
        if (!(flags & 0x40)) {
            float sc = Lerp(tt, F32(a + 72), F32(a + 76));
            c.sx = sc * Lerp(tt, F32(a + 80), F32(a + 84));
            c.sy = sc * Lerp(tt, F32(a + 88), F32(a + 92));
        } else {
            c.sx = c.sy = 1.0f;
        }
        c.c[0] = c.c[1] = c.c[2] = 1.0f;
        c.c[3] = alpha;
        if (tint)
            for (p = 0; p < 4; p++) c.c[p] *= tint[p];
        if (!(flags & 0x400)) {
            Color((uint32_t)I32(a + 136), c0);
            Color((uint32_t)I32(a + 140), c1);
            for (p = 0; p < 4; p++)
                c.c[p] *= (flags & 0x80) ? c0[p] : Lerp(tt, c0[p], c1[p]);
        }
        if (!(flags & 0x20)) {
            c.rx = Lerp(tt, F32(a + 48), F32(a + 52));
            c.ry = Lerp(tt, F32(a + 56), F32(a + 60));
            c.rz = Lerp(tt, F32(a + 64), F32(a + 68));
        }
        if (!(flags & 0x4000)) {
            c.px = Lerp(tt, (float)I32(a + 32), (float)I32(a + 36));
            c.py = Lerp(tt, (float)I32(a + 40), (float)I32(a + 44));
        }
        if (!(flags & 0x10)) {
            float bx = SDL_sinf(delta * I16(a + 128) * SDL_PI_F), by = SDL_sinf(delta * I16(a + 130) * SDL_PI_F);
            c.x += bx * Lerp(tt, F32(a + 112), F32(a + 116));
            c.y += by * Lerp(tt, F32(a + 120), F32(a + 124));
        }
        if (sg < 0 || sg >= I32(s + 24))
            continue;
        g = s + I32(s + 28) + sg * 4;
        for (p = 0; p < I16(g + 2); p++)
            if (DrawSprite(renderer, t, s, &c, s + I32(s + 20) + (I16(g) + p) * 20, 1.0f, x, y, scale, I32(a + 132),
                           masked ? &clip : NULL, &box)) {
                clip = box; /* the latest mask limits what follows */
                masked = 1;
            }
    }
    SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND);
    return 1;
}

#if __STDC_HOSTED__ || defined(SDL_BRIDGE_GPU_TARGETS)
static void GaugeTargetsRelease(void);
#endif

static void GaugeCacheRelease(void);

void PortHdLayoutShutdown(void) {
    int i;
    for (i = 0; i < HD_LAYOUT_TEXTURE_COUNT; i++) {
        int f;
        if (sTextures[i].tex)
            SDL_DestroyTexture(sTextures[i].tex);
        for (f = 1; f < 4; f++)
            if (sTextures[i].flipped[f])
                SDL_DestroyTexture(sTextures[i].flipped[f]);
        memset(&sTextures[i], 0, sizeof(sTextures[i]));
    }
#if __STDC_HOSTED__ || defined(SDL_BRIDGE_GPU_TARGETS)
    GaugeTargetsRelease();
#endif
    GaugeCacheRelease();
    for (i = 0; i < HD_LAYOUT_TEXTURE_COUNT; i++) {
        int f;
        for (f = 0; f < 4; f++) {
            if (sPictures[i][f]) SDL_DestroySurface(sPictures[i][f]);
            sPictures[i][f] = NULL;
        }
    }
}

/* ---- the player's HP gauge (Gauge::CPlayerGauge, read from Re:CoM's executable, HUD_HD_RECOM.md) ---------------
   gauge.lad's group "gauge:01" (layout group 1, at 60,60) has five places: 0 the gauge's state (14 appearing, 15
   normal, 10 low HP, 11..13 a hit), 1 the track (group 7), 2 the red trail (8), 3 the green HP (9), and "gauge:00"'s
   face (sequence 0: 3 normal, 1 hurt, 0 low, 2 down). The lengths are frames: value / 560 x 470. A hit drops the
   green at once; the trail waits 1.5 s, then closes a tenth of the gap and one more each step; healing moves both.
   Low HP is 25 % of the maximum. Only the picture is Re:CoM's: the HP and its rules are the game's own. */
#define GAUGE_LEN(v) ((int)((float)SDL_clamp((v), 0, 560) / 560.0f * 470.0f))
static struct {
    int active, frame, hp, maxHp, trail, delayMs, hurtMs, riku, started;
    int stateAnim, stateFrame, faceAnim, faceFrame;
} sGauge;

static void GaugeSet(int* anim, int* frame, int value) {
    if (*anim != value) {
        *anim = value;
        *frame = 0;
    }
}

int PortHdGauge(int hp, int maxHp, int riku) {
    int msPerFrame = 17;
    int i, bodyReady, faceReady;
    static const int groups[] = { 7, 8, 9, 10, 13, 14, 15 };
    if (!gPortConfig.hdCards && !PortHdHudPack())
        return 0;
    /* Structurally valid imports may still lack this gauge's animations. */
    for (i = 0; i < (int)(sizeof(groups) / sizeof(groups[0])); i++)
        if (!PortHdLayoutGroupFrames(HD_LAYOUT_GAUGE, 1, groups[i]))
            return 0;
    for (i = 0; i < 4; i++)
        if (!PortHdLayoutGroupFrames(HD_LAYOUT_GAUGE, 0, i))
            return 0;
    bodyReady = PortHdLayoutReady(HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE);
    faceReady = PortHdLayoutReady(HD_LAYOUT_GAUGE, riku ? HD_LAYOUT_TEX_RIKU_FACES : HD_LAYOUT_TEX_GAUGE_FACES);
    if (!bodyReady || !faceReady)
        return 0;
    {
        /* harness: KHCOM_HD_GAUGE_TEST_HP=n shows at most n HP (the low-HP alarm without losing a battle) */
        static int testHp = -2;
        if (testHp == -2) {
            const char* v = SDL_getenv("KHCOM_HD_GAUGE_TEST_HP");
            testHp = v ? SDL_atoi(v) : -1;
        }
        if (testHp >= 0) hp = SDL_min(hp, testHp);
    }
    hp = SDL_clamp(hp, 0, 560);
    maxHp = SDL_clamp(maxHp, 1, 560);
    if (!sGauge.started || sGauge.frame + 1 != (int)PortFrameCount() || sGauge.riku != riku) {
        /* a new battle (or the gauge was away): it appears, full as the game has it */
        memset(&sGauge, 0, sizeof(sGauge));
        sGauge.started = 1;
        sGauge.riku = riku;
        sGauge.hp = sGauge.trail = hp;
        sGauge.stateAnim = 14;
        sGauge.faceAnim = 3;
    }
    sGauge.frame = (int)PortFrameCount();
    sGauge.maxHp = maxHp;
    if (hp != sGauge.hp) {
        if (hp < sGauge.hp) { /* a hit: the trail waits, the gauge and the face react */
            sGauge.delayMs = 1500;
            sGauge.hurtMs = 3000;
            GaugeSet(&sGauge.stateAnim, &sGauge.stateFrame, 13);
            GaugeSet(&sGauge.faceAnim, &sGauge.faceFrame, 1);
        }
        sGauge.hp = hp;
        if (sGauge.trail < hp) {
            sGauge.trail = hp;
            sGauge.delayMs = 0;
        }
    }
    if (sGauge.trail > sGauge.hp) {
        if (sGauge.delayMs <= 0) {
            sGauge.trail -= (sGauge.trail - sGauge.hp) / 10 + 1;
            if (sGauge.trail < sGauge.hp)
                sGauge.trail = sGauge.hp;
        }
        sGauge.delayMs -= msPerFrame;
    }
    if (sGauge.hurtMs > 0) {
        sGauge.hurtMs -= msPerFrame;
    } else if (sGauge.stateAnim != 14 || sGauge.stateFrame >= PortHdLayoutGroupFrames(HD_LAYOUT_GAUGE, 1, 14)) {
        if (hp == 0) {
            GaugeSet(&sGauge.stateAnim, &sGauge.stateFrame, 15);
            GaugeSet(&sGauge.faceAnim, &sGauge.faceFrame, 2);
        } else if (hp <= maxHp * 25 / 100) {
            GaugeSet(&sGauge.stateAnim, &sGauge.stateFrame, 10);
            GaugeSet(&sGauge.faceAnim, &sGauge.faceFrame, 0);
        } else {
            GaugeSet(&sGauge.stateAnim, &sGauge.stateFrame, 15);
            GaugeSet(&sGauge.faceAnim, &sGauge.faceFrame, 3);
        }
    }
    sGauge.stateFrame++;
    sGauge.faceFrame++;
    sGauge.active = 1;
    return 1;
}

static void GaugeParts(SDL_Renderer* renderer, float x, float y, float s, const float* tint);

/* The gauge drawn supersampled (23/10, the player: its face's and its bar's cut edges were jagged): its pieces go on
   a texture at GAUGE_SS times the output, which comes down to the output in halves with linear filtering (each half a
   2x2 box), premultiplied all the way; then the masks' hard cuts and the ring's edges are smooth. The consoles and
   Android, with less memory and fill rate, use at most 2. Optional RGBA targets
   are capped at 32 MiB on desktop and 12 MiB on constrained hosted builds:
   SS4 holds 21 base-resolution surfaces' worth of pixels, SS2 holds 5. These
   are scratch-cache limits, not total GPU-memory estimates or an OOM claim. */
#if defined(PORT_GBA_RELOC) || defined(PORT_ANDROID64) || defined(__ANDROID__)
#define GAUGE_SS 2
#define GAUGE_TARGET_BUDGET (12ull * 1024 * 1024)
#else
#define GAUGE_SS 4
#define GAUGE_TARGET_BUDGET (32ull * 1024 * 1024)
#endif
#if __STDC_HOSTED__ || defined(SDL_BRIDGE_GPU_TARGETS) /* optional bridge hosts negotiate target support; old hosts return no maximum */
static SDL_Texture* sGaugeTarget[3];
static int sGaugeTargetW[3], sGaugeTargetH[3];
static int sGaugeTargetScale;
static int sGaugeRestorePending, sGaugeRestoreClipped;
static SDL_Texture* sGaugeRestoreTarget;
static SDL_Renderer* sGaugeRestoreRenderer;
static SDL_Rect sGaugeRestoreViewport, sGaugeRestoreClip;

/* A failed destination restore must not turn the following copy into feedback.
   Keep the original destination alive, and retry before any further gauge work. */
static int GaugeRestoreOutput(void) {
    SDL_Texture* retained;
    if (!sGaugeRestorePending) return 1;
    if (!SDL_SetRenderTarget(sGaugeRestoreRenderer, sGaugeRestoreTarget)) return 0;
    SDL_SetRenderViewport(sGaugeRestoreRenderer, &sGaugeRestoreViewport);
    SDL_SetRenderClipRect(sGaugeRestoreRenderer, sGaugeRestoreClipped ? &sGaugeRestoreClip : NULL);
    retained = sGaugeRestoreTarget;
    sGaugeRestoreTarget = NULL;
    sGaugeRestorePending = 0;
    if (retained) SDL_DestroyTexture(retained); /* release our failure-recovery reference */
    return 1;
}

static void GaugeTargetsRelease(void) {
    int i;
    if (!GaugeRestoreOutput()) return;
    for (i = 0; i < 3; i++) {
        if (sGaugeTarget[i]) SDL_DestroyTexture(sGaugeTarget[i]);
        sGaugeTarget[i] = NULL;
        sGaugeTargetW[i] = sGaugeTargetH[i] = 0;
    }
    sGaugeTargetScale = 0;
}

static SDL_Texture* GaugeTarget(SDL_Renderer* renderer, int i, int w, int h) {
    if (sGaugeTarget[i] && (sGaugeTargetW[i] != w || sGaugeTargetH[i] != h)) {
        SDL_DestroyTexture(sGaugeTarget[i]);
        sGaugeTarget[i] = NULL;
    }
    if (!sGaugeTarget[i]) {
        sGaugeTarget[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if (!sGaugeTarget[i])
            return NULL;
        sGaugeTargetW[i] = w;
        sGaugeTargetH[i] = h;
        SDL_SetTextureScaleMode(sGaugeTarget[i], (SDL_ScaleMode)PortSmoothScaleMode());
    }
    return sGaugeTarget[i];
}

/* Reserve the complete downsample chain before drawing. Release the old chain
   before resize so old and new allocations cannot overlap above the budget. */
static int GaugeTargetsPrepare(SDL_Renderer* renderer, float width, float height, int* w0, int* h0) {
    double w = (double)SDL_ceilf(width), h = (double)SDL_ceilf(height);
    int64_t maxSize = SDL_GetNumberProperty(SDL_GetRendererProperties(renderer),
                                          SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
    int k, first = GAUGE_SS;
    if (!(w > 0 && h > 0 && w <= INT_MAX && h <= INT_MAX) || maxSize <= 0) {
        GaugeTargetsRelease();
        return 0;
    }
    *w0 = (int)w;
    *h0 = (int)h;
    /* Once an allocation fell back to SS2, retain that usable chain for the
       same geometry instead of retrying the rejected larger allocation. */
    if (sGaugeTargetScale && sGaugeTarget[0] &&
        sGaugeTargetW[0] / sGaugeTargetScale == *w0 && sGaugeTargetH[0] / sGaugeTargetScale == *h0)
        first = sGaugeTargetScale;
    for (k = first; k >= 2; k /= 2) {
        int tw[3], th[3], count = k == 4 ? 3 : 2, i, fits = 1, cached = 1;
        uint64_t bytes = 0;
        if (*w0 > INT_MAX / k || *h0 > INT_MAX / k)
            continue;
        for (i = 0; i < count; i++) {
            uint64_t size;
            tw[i] = *w0 * (k >> i);
            th[i] = *h0 * (k >> i);
            size = (uint64_t)tw[i] * (uint64_t)th[i] * 4;
            if (tw[i] > maxSize || th[i] > maxSize || size > GAUGE_TARGET_BUDGET - bytes) {
                fits = 0;
                break;
            }
            bytes += size;
        }
        if (!fits)
            continue;
        for (i = 0; i < 3; i++) {
            if (i < count) {
                if (!sGaugeTarget[i] || sGaugeTargetW[i] != tw[i] || sGaugeTargetH[i] != th[i]) cached = 0;
            } else if (sGaugeTarget[i]) cached = 0;
        }
        if (cached)
            return k;
        GaugeTargetsRelease();
        for (i = 0; i < count; i++)
            if (!GaugeTarget(renderer, i, tw[i], th[i]))
                break;
        if (i == count) {
            sGaugeTargetScale = k;
            return k;
        }
        GaugeTargetsRelease();
    }
    GaugeTargetsRelease();
    return 0; /* Direct HD geometry needs no scratch textures. */
}

#endif

/* SDL's software renderer (handhelds): the gauge drawn piece by piece cost 3.3 million instructions a frame
   (QEMU, GA36), while its picture changes only on a hit or a heal. It is drawn once into a surface of its own
   (premultiplied: the pieces over transparency, then that over the screen, is the same picture) and copied while
   nothing it draws changes; the copy is cut to the rows' covered spans, since the ring and the bar leave most of the
   box empty. A hit's animation rebuilds it at most every other frame. KHCOM_HD_GAUGE_NO_CACHE draws it directly. */
static SDL_Texture* sCacheTexture;
static SDL_Renderer* sCacheOwner;
static int sCacheW, sCacheH, sCacheValid, sCacheFailed; /* sCacheW/H: the surface's size, the box may be smaller */
static uint32_t sCacheHash, sCacheBuilt;
static int16_t* sCacheSpan; /* per row: first and last covered column, -1 none */

/* Pictures the gauge has shown, by their geometry hash: the low-HP alarm and the faces loop, so after one round
   every frame of the loop is a copy (user, 26/11: "at low HP it went slow again"). Only the rows' covered spans are
   kept. Least recently shown goes first past the budget. */
#define GAUGE_FRAMES 96
#define GAUGE_FRAME_BUDGET (6u * 1024 * 1024)
typedef struct {
    uint32_t hash, used;
    int w, h;
    int16_t* span;
    uint32_t* px;
    size_t bytes;
} GaugeFrame;
static GaugeFrame sFrames[GAUGE_FRAMES];
static size_t sFrameBytes;

static void GaugeFrameDrop(GaugeFrame* e) {
    SDL_free(e->span);
    SDL_free(e->px);
    sFrameBytes -= e->bytes;
    memset(e, 0, sizeof(*e));
}

static GaugeFrame* GaugeFrameFind(uint32_t hash, int w, int h) {
    int i;
    for (i = 0; i < GAUGE_FRAMES; i++)
        if (sFrames[i].px && sFrames[i].hash == hash && sFrames[i].w == w && sFrames[i].h == h)
            return &sFrames[i];
    return NULL;
}

/* the surface's box (with sCacheSpan) into a new entry */
static void GaugeFrameStore(uint32_t hash, int w, int h, uint32_t frame) {
    const uint32_t* src = (const uint32_t*)sCacheSurface->pixels;
    int pitch = sCacheSurface->pitch / 4, row, i, slot = -1;
    size_t count = 0, bytes;
    uint32_t* out;
    for (row = 0; row < h; row++)
        if (sCacheSpan[row * 2] >= 0) count += (size_t)(sCacheSpan[row * 2 + 1] - sCacheSpan[row * 2] + 1);
    bytes = count * 4 + sizeof(int16_t) * 2 * (size_t)h;
    if (bytes > GAUGE_FRAME_BUDGET / 4)
        return;
    for (;;) {
        int oldest = -1;
        slot = -1;
        for (i = 0; i < GAUGE_FRAMES; i++) {
            if (!sFrames[i].px) { if (slot < 0) slot = i; continue; }
            if (oldest < 0 || sFrames[i].used < sFrames[oldest].used) oldest = i;
        }
        if (slot >= 0 && sFrameBytes + bytes <= GAUGE_FRAME_BUDGET)
            break;
        if (oldest < 0)
            return;
        GaugeFrameDrop(&sFrames[oldest]);
    }
    sFrames[slot].span = (int16_t*)SDL_malloc(sizeof(int16_t) * 2 * (size_t)h);
    sFrames[slot].px = (uint32_t*)SDL_malloc(count ? count * 4 : 4);
    if (!sFrames[slot].span || !sFrames[slot].px) {
        SDL_free(sFrames[slot].span);
        SDL_free(sFrames[slot].px);
        memset(&sFrames[slot], 0, sizeof(sFrames[slot]));
        return;
    }
    memcpy(sFrames[slot].span, sCacheSpan, sizeof(int16_t) * 2 * (size_t)h);
    out = sFrames[slot].px;
    for (row = 0; row < h; row++)
        if (sCacheSpan[row * 2] >= 0) {
            int n = sCacheSpan[row * 2 + 1] - sCacheSpan[row * 2] + 1;
            memcpy(out, src + (size_t)row * pitch + sCacheSpan[row * 2], (size_t)n * 4);
            out += n;
        }
    sFrames[slot].hash = hash;
    sFrames[slot].used = frame;
    sFrames[slot].w = w;
    sFrames[slot].h = h;
    sFrames[slot].bytes = bytes;
    sFrameBytes += bytes;
}

/* an entry back into the surface (its old spans cleared) and the copy */
static int GaugeFrameShow(GaugeFrame* e, int w, int h) {
    uint32_t* dst = (uint32_t*)sCacheSurface->pixels;
    const uint32_t* in = e->px;
    int pitch = sCacheSurface->pitch / 4, row;
    SDL_Rect area = { 0, 0, w, h };
    for (row = 0; row < h; row++) {
        uint32_t* line = dst + (size_t)row * pitch;
        if (sCacheSpan[row * 2] >= 0)
            memset(line + sCacheSpan[row * 2], 0, (size_t)(sCacheSpan[row * 2 + 1] - sCacheSpan[row * 2] + 1) * 4);
        if (e->span[row * 2] >= 0) {
            int n = e->span[row * 2 + 1] - e->span[row * 2] + 1;
            memcpy(line + e->span[row * 2], in, (size_t)n * 4);
            in += n;
        }
    }
    memcpy(sCacheSpan, e->span, sizeof(int16_t) * 2 * (size_t)h);
    return SDL_UpdateTexture(sCacheTexture, &area, sCacheSurface->pixels, sCacheSurface->pitch);
}

static void GaugeCacheRelease(void) {
    int i;
    for (i = 0; i < GAUGE_FRAMES; i++)
        if (sFrames[i].px || sFrames[i].span) GaugeFrameDrop(&sFrames[i]);
    for (i = 0; i < HD_LAYOUT_TEXTURE_COUNT; i++) {
        int f;
        if (sCacheTextures[i].tex) SDL_DestroyTexture(sCacheTextures[i].tex);
        for (f = 1; f < 4; f++)
            if (sCacheTextures[i].flipped[f]) SDL_DestroyTexture(sCacheTextures[i].flipped[f]);
        memset(&sCacheTextures[i], 0, sizeof(sCacheTextures[i]));
    }
    if (sCacheRenderer) SDL_DestroyRenderer(sCacheRenderer);
    if (sCacheSurface) SDL_DestroySurface(sCacheSurface);
    if (sCacheTexture) SDL_DestroyTexture(sCacheTexture);
    SDL_free(sCacheSpan);
    sCacheRenderer = NULL;
    sCacheSurface = NULL;
    sCacheTexture = NULL;
    sCacheSpan = NULL;
    sCacheOwner = NULL;
    sCacheW = sCacheH = sCacheValid = 0;
}

/* The cache's surface, renderer and copy for boxes up to w x h (made before the first battle when preloading). */
static int GaugeCachePrepare(SDL_Renderer* renderer, int w, int h) {
    GaugeCacheRelease();
    sCacheSurface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ARGB8888);
    sCacheRenderer = sCacheSurface ? SDL_CreateSoftwareRenderer(sCacheSurface) : NULL;
    sCacheTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    sCacheSpan = (int16_t*)SDL_malloc(sizeof(int16_t) * 2 * (size_t)h);
    if (!sCacheRenderer || !sCacheTexture || !sCacheSpan ||
        !SDL_SetTextureBlendMode(sCacheTexture, SDL_BLENDMODE_BLEND_PREMULTIPLIED)) {
        PortLog("gauge cache: unavailable (%s)", SDL_GetError());
        GaugeCacheRelease();
        sCacheFailed = 1;
        return 0;
    }
    SDL_SetTextureScaleMode(sCacheTexture, SDL_SCALEMODE_NEAREST);
    sCacheOwner = renderer;
    sCacheW = w;
    sCacheH = h;
    PortLog("gauge cache: %dx%d", w, h);
    return 1;
}

static int GaugeCached(SDL_Renderer* renderer, float x, float y, float s, const float* tint) {
    /* the box of every frame the gauge has (PortHdGaugeDraw's measured bounds) */
    const float ux0 = -92.0f, uy0 = -92.0f, ux1 = 206.0f, uy1 = 92.0f;
    float rx = SDL_floorf(x + ux0 * s), ry = SDL_floorf(y + uy0 * s), lx = x - rx, ly = y - ry;
    int w = (int)SDL_ceilf((ux1 - ux0) * s) + 2, h = (int)SDL_ceilf((uy1 - uy0) * s) + 2, row;
    uint32_t frame = PortFrameCount();
    static int disabled = -1;
    if (disabled < 0) disabled = SDL_getenv("KHCOM_HD_GAUGE_NO_CACHE") != NULL;
    if (disabled || sCacheFailed || w <= 0 || h <= 0 || w > 2048 || h > 2048)
        return 0;
    if (renderer != sCacheOwner || w > sCacheW || h > sCacheH) {
        if (!GaugeCachePrepare(renderer, w, h))
            return 0;
    }
    {
        static int lastW, lastH;
        if (w != lastW || h != lastH) sCacheValid = 0; /* another box: rebuild */
        lastW = w;
        lastH = h;
    }
    /* what the gauge would draw this frame, placed in the box */
    sGeoDry = 1;
    sGeoHash = 2166136261u;
    GeoMix(tint, sizeof(float) * 4);
    GaugeParts(sCacheRenderer, lx, ly, s, tint);
    sGeoDry = 0;
    if (sCacheValid && sGeoHash != sCacheHash) {
        GaugeFrame* e = GaugeFrameFind(sGeoHash, w, h);
        if (e) {
            e->used = frame;
            if (!GaugeFrameShow(e, w, h)) {
                sCacheValid = 0;
                return 0;
            }
            sCacheHash = sGeoHash;
        }
    }
    if (!sCacheValid || (sGeoHash != sCacheHash && frame - sCacheBuilt >= 2)) {
        const uint32_t* px;
        int pitch, xx;
        SDL_FRect box = { 0, 0, (float)w, (float)h };
        SDL_Rect area = { 0, 0, w, h };
        SDL_SetRenderDrawBlendMode(sCacheRenderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(sCacheRenderer, 0, 0, 0, 0);
        SDL_RenderFillRect(sCacheRenderer, &box);
        GaugeParts(sCacheRenderer, lx, ly, s, tint);
        if (!SDL_FlushRenderer(sCacheRenderer) || !SDL_UpdateTexture(sCacheTexture, &area, sCacheSurface->pixels,
                                                                       sCacheSurface->pitch)) {
            sCacheValid = 0;
            return 0;
        }
        px = (const uint32_t*)sCacheSurface->pixels;
        pitch = sCacheSurface->pitch / 4;
        for (row = 0; row < h; row++) {
            const uint32_t* p = px + (size_t)row * pitch;
            int first = -1, last = -1;
            for (xx = 0; xx < w; xx++)
                if (p[xx]) { /* any colour or coverage: ADD leaves colour at alpha 0 */
                    if (first < 0) first = xx;
                    last = xx;
                }
            sCacheSpan[row * 2] = (int16_t)first;
            sCacheSpan[row * 2 + 1] = (int16_t)last;
        }
        sCacheHash = sGeoHash;
        sCacheBuilt = frame;
        sCacheValid = 1;
        if (!GaugeFrameFind(sGeoHash, w, h))
            GaugeFrameStore(sGeoHash, w, h, frame);
    }
    /* bands of 8 rows, each cut to its covered columns */
    for (row = 0; row < h; row += 8) {
        int r, first = INT_MAX, last = -1, bh = SDL_min(8, h - row);
        for (r = row; r < row + bh; r++)
            if (sCacheSpan[r * 2] >= 0) {
                first = SDL_min(first, sCacheSpan[r * 2]);
                last = SDL_max(last, sCacheSpan[r * 2 + 1]);
            }
        if (last >= 0) {
            SDL_FRect src = { (float)first, (float)row, (float)(last - first + 1), (float)bh };
            SDL_FRect dst = { rx + first, ry + row, src.w, src.h };
            SDL_RenderTexture(renderer, sCacheTexture, &src, &dst);
        }
    }
    return 1;
}

void PortHdGaugeDraw(SDL_Renderer* renderer, float ox, float oy, float scale, float shade) {
    /* gauge:01 sits at (60, 60) of Re:CoM's 640-wide screen: the GBA HUD's 240 gives 0.375 of a GBA pixel a unit */
    const float unit = 240.0f / 640.0f;
    float x = ox + 60.0f * unit * scale, y = oy + 60.0f * unit * scale, s = unit * scale;
    float tint[4] = { shade, shade, shade, 1.0f };
#if __STDC_HOSTED__ || defined(SDL_BRIDGE_GPU_TARGETS)
    if (!GaugeRestoreOutput()) return;
#endif
    if (!sGauge.active || sGauge.frame != (int)PortFrameCount())
        return;
    if (PortHdDrawLogged())
        PortLog("hd gauge %u hp %d/%d trail %d state %d face %d", (unsigned)PortFrameCount(), sGauge.hp, sGauge.maxHp,
                sGauge.trail, sGauge.stateAnim, sGauge.faceAnim);
#if __STDC_HOSTED__ || defined(SDL_BRIDGE_GPU_TARGETS)
    /* not on SDL's software renderer (the tests' offscreen window): restoring the window as the target crashed
       inside it */
    if (!SDL_getenv("KHCOM_GAUGE_NO_SS") && SDL_strcmp(SDL_GetRendererName(renderer), SDL_SOFTWARE_RENDERER) != 0) {
        /* Conservative emitted geometry of the imported HD gauge, measured
           with the real layout drawer across every HP/state/face frame:
           x[-89.521,202.010], y[-89.521,89.521] (legacy x reaches203.010).
           Keep padding and one stable crop: no HP-driven cache resizing.
           The old -60 lower bound also cut off the low-HP alarm's glow.
           test_hd_gauge_visible_bounds.py checks private LAD imports. */
        const float ux0 = -92.0f, uy0 = -92.0f, ux1 = 206.0f, uy1 = 92.0f;
        float rx = x + ux0 * s, ry = y + uy0 * s, rw = (ux1 - ux0) * s, rh = (uy1 - uy0) * s;
        int w0 = 0, h0 = 0, level = 0, k = GaugeTargetsPrepare(renderer, rw, rh, &w0, &h0);
        SDL_Texture* big = k ? sGaugeTarget[0] : NULL;
        static int reported = -1;
        if (reported != k) {
            reported = k;
            PortLog("gauge-aa: %s scale=%d renderer=%s", k ? "GPU targets" : "direct (targets unavailable or allocation limit)",
                    k, SDL_GetRendererName(renderer));
        }
        if (big) {
            SDL_Texture* prev = SDL_GetRenderTarget(renderer);
            SDL_Rect viewport, clip;
            int clipped = SDL_RenderClipEnabled(renderer);
            SDL_FRect src, dst;
            SDL_Texture* cur = big;
            int cw = w0 * k, ch = h0 * k;
            SDL_GetRenderViewport(renderer, &viewport);
            SDL_GetRenderClipRect(renderer, &clip);
            if (!SDL_SetRenderTarget(renderer, big)) {
                if (!SDL_SetRenderTarget(renderer, prev)) return;
                GaugeTargetsRelease();
                PortLog("gauge-aa: direct (target bind failed) renderer=%s", SDL_GetRendererName(renderer));
                GaugeParts(renderer, x, y, s, tint);
                return;
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            GaugeParts(renderer, (x - rx) * k, (y - ry) * k, s * k, tint);
            /* down in halves: each a 2x2 box of premultiplied pixels */
            while (cw > w0) {
                int nw = SDL_max(cw / 2, w0), nh = SDL_max(ch / 2, h0);
                SDL_Texture* next = GaugeTarget(renderer, 1 + (level & 1), nw, nh);
                if (!next)
                    break;
                if (!SDL_SetRenderTarget(renderer, next)) break;
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderClear(renderer);
                SDL_SetTextureBlendMode(cur, SDL_BLENDMODE_NONE);
                src.x = src.y = 0;
                src.w = (float)cw;
                src.h = (float)ch;
                dst.x = dst.y = 0;
                dst.w = (float)nw;
                dst.h = (float)nh;
                SDL_RenderTexture(renderer, cur, &src, &dst);
                cur = next;
                cw = nw;
                ch = nh;
                level++;
            }
            if (!SDL_SetRenderTarget(renderer, prev)) {
                sGaugeRestorePending = 1;
                sGaugeRestoreTarget = prev;
                sGaugeRestoreRenderer = renderer;
                sGaugeRestoreViewport = viewport;
                sGaugeRestoreClip = clip;
                sGaugeRestoreClipped = clipped;
                if (prev) prev->refcount++;
                PortLog("gauge-aa: destination restore failed; copy skipped until recovery");
                return;
            }
            SDL_SetRenderViewport(renderer, &viewport);
            SDL_SetRenderClipRect(renderer, clipped ? &clip : NULL);
            SDL_SetTextureBlendMode(cur, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
            src.x = src.y = 0;
            src.w = (float)cw;
            src.h = (float)ch;
            dst.x = rx;
            dst.y = ry;
            dst.w = (float)w0;
            dst.h = (float)h0;
            SDL_RenderTexture(renderer, cur, &src, &dst);
            return;
        }
    } else {
        GaugeTargetsRelease();
        if (SDL_strcmp(SDL_GetRendererName(renderer), SDL_SOFTWARE_RENDERER) == 0 && GaugeCached(renderer, x, y, s, tint))
            return;
    }
#endif
    GaugeParts(renderer, x, y, s, tint);
}

static void GaugeParts(SDL_Renderer* renderer, float x, float y, float s, const float* tint) {
    PortHdLayoutDraw(renderer, HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE, 1, sGauge.stateAnim, sGauge.stateFrame, x, y, s, 1.0f, tint);
    PortHdLayoutDraw(renderer, HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE, 1, 7, HD_LAYOUT_HOLD(GAUGE_LEN(sGauge.maxHp)), x, y, s,
                     1.0f, tint);
    PortHdLayoutDraw(renderer, HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE, 1, 8, HD_LAYOUT_HOLD(GAUGE_LEN(sGauge.trail)), x, y, s,
                     1.0f, tint);
    PortHdLayoutDraw(renderer, HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE, 1, 9, HD_LAYOUT_HOLD(GAUGE_LEN(sGauge.hp)), x, y, s,
                     1.0f, tint);
    PortHdLayoutDraw(renderer, HD_LAYOUT_GAUGE, sGauge.riku ? HD_LAYOUT_TEX_RIKU_FACES : HD_LAYOUT_TEX_GAUGE_FACES, 0,
                     sGauge.faceAnim, sGauge.faceFrame, x, y, s, 1.0f, tint);
}

/* ---- the enemy HP gauge (Gauge::CEnemyGauge, read from Re:CoM's executable, HUD_HD_RECOM.md) ------------------
   gauge.lad's layout group 0 has seven places in sequence 1. Re:CoM's rules: a hit drops the green at once and holds
   the trail 1.5 s, then closes a tenth of the gap and one more each step; healing moves both. Its bars hold 200 HP
   each; here a bar
   is the cartridge's own 560 (its gauge's layers: green, blue, yellow, red), so the HP and its layers stay the game's.
   The gauge sits where the cartridge's is, its right end on the cartridge bar's.
   The imported sequence's groups are value controls, not elapsed animations:
   0 the length/tip, 1 red, 2 constant overlay, 3 green, 4..6 layer-count pieces. */
#define ENEMY_LAYER 560
static const int kEnemyPlaceGroup[7] = { 2, 0, 1, 3, 4, 5, 6 };
static struct {
    int active, frame, hp, maxHp, trail, delayMs, started;
    const void* who;
} sEnemy;

int PortHdEnemyGauge(int visible, int hp, int maxHp, const void* who) {
    int i;
    if (!gPortConfig.hdCards && !PortHdHudPack())
        return 0;
    for (i = 0; i < 7; i++)
        if (!PortHdLayoutGroupFrames(HD_LAYOUT_GAUGE, 1, kEnemyPlaceGroup[i]))
            return 0;
    if (!PortHdLayoutReady(HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE))
        return 0;
    if (!visible || !who) {
        sEnemy.active = 0;
        sEnemy.started = 0;
        return 1; /* nothing shown, as the cartridge then shows nothing */
    }
    hp = SDL_max(hp, 0);
    sEnemy.maxHp = maxHp;
    if (!sEnemy.started || sEnemy.who != who || sEnemy.frame + 1 != (int)PortFrameCount()) {
        memset(&sEnemy, 0, sizeof(sEnemy));
        sEnemy.started = 1;
        sEnemy.who = who;
        sEnemy.hp = sEnemy.trail = hp;
    }
    sEnemy.frame = (int)PortFrameCount();
    sEnemy.maxHp = maxHp;
    if (hp != sEnemy.hp) {
        if (hp < sEnemy.hp)
            sEnemy.delayMs = 1500;
        sEnemy.hp = hp;
        if (sEnemy.trail < hp) {
            sEnemy.trail = hp;
            sEnemy.delayMs = 0;
        }
    }
    if (sEnemy.trail > sEnemy.hp) {
        if (sEnemy.delayMs <= 0) {
            sEnemy.trail -= (sEnemy.trail - sEnemy.hp) / 10 + 1;
            if (sEnemy.trail < sEnemy.hp)
                sEnemy.trail = sEnemy.hp;
        }
        sEnemy.delayMs -= 17;
    }
    sEnemy.active = 1;
    return 1;
}

/* Stock sums use a 16px GBA sprite with ink beginning at local y5.
 * With an imported HP bar active, anchor9 puts the full plate below its
 * y12 lower edge; otherwise preserve the cartridge's authored anchor4. */
/* Re:CoM's player gauge was drawn this frame or the last (the cards may draw before the gauge's task). */
int PortHdGaugeShown(void) {
    return sGauge.active && (int)PortFrameCount() - sGauge.frame <= 1;
}

int PortHdStockValueTop(int opponent) {
    if (!gPortConfig.hdCards && !PortHdHudPack()) return 0;
    return opponent ? (sEnemy.active && sEnemy.frame == (int)PortFrameCount() ? 9 : 0)
                    : (sGauge.active && sGauge.frame == (int)PortFrameCount() ? 9 : 0);
}

float gPortHdEnemyGaugeZoom = 1.0f; /* port_hdsprites.c: larger on a small panel's shared canvas */

void PortHdEnemyGaugeDraw(SDL_Renderer* renderer, float ox, float oy, float scale, float shade) {
    const float unit = 240.0f / 640.0f;
    const float zoom = gPortHdEnemyGaugeZoom > 0 ? gPortHdEnemyGaugeZoom : 1.0f;
    float s = unit * scale * zoom, tint[4] = { shade, shade, shade, 1.0f };
    /* Re:CoM's bar ends at x 495 of its 640; the cartridge's at 236 of its 240, which the HUD keeps from its right */
    float x = ox + ((float)(gPortHudW - GBA_SCREEN_W) + 236.0f - 495.0f * unit * zoom) * scale, y = oy;
    int layer, layers, hpFrame, trailFrame, trailLayer, backFrame, top, i;
    if (!sEnemy.active || sEnemy.frame != (int)PortFrameCount())
        return;
    layer = sEnemy.hp > 0 ? (sEnemy.hp - 1) / ENEMY_LAYER : 0;
    trailLayer = sEnemy.trail > 0 ? (sEnemy.trail - 1) / ENEMY_LAYER : 0;
    /* Re:CoM's frames are absolute HP (CEnemyGauge: 200 a whole bar; its background is the maximum, or what of it is
       left in the top bar), never HP over the maximum. A bar holds 560 HP, the scale of Sora's gauge, so a weak
       enemy gets a short whole bar and both grow with their maximum HP (user 27/10). */
    hpFrame = (sEnemy.hp - layer * ENEMY_LAYER) * 200 / ENEMY_LAYER;
    trailFrame = trailLayer > layer ? 200 : (sEnemy.trail - layer * ENEMY_LAYER) * 200 / ENEMY_LAYER;
    top = sEnemy.maxHp > 0 ? (sEnemy.maxHp - 1) / ENEMY_LAYER : 0;
    backFrame = layer >= top ? (sEnemy.maxHp - top * ENEMY_LAYER) * 200 / ENEMY_LAYER : 200;
    layers = layer;
    if (PortHdDrawLogged())
        PortLog("hd enemy gauge %u hp %d trail %d layer %d", (unsigned)PortFrameCount(), sEnemy.hp, sEnemy.trail, layer);
    for (i = 0; i < 7; i++) {
        int frame;
        if (i >= 4 && layers <= 0)
            continue; /* the bars' counter: only with bars left */
        if (i == 0 && (top <= 0 || layer < top))
            continue; /* place 0 (the full-length overlay): Re:CoM plays it only on the top of several bars (FUN_14020d270) */
        /* Green/red already normalize the current layer to 0..200. Hold the
           matching full-length border; playing group 0 grows and wraps it.
           Every counter piece must use the layer count, never a frame clock. */
        frame = i == 0 ? HD_LAYOUT_HOLD(0)
              : i == 1 ? HD_LAYOUT_HOLD(SDL_clamp(backFrame, 0, 200))
              : i == 2 ? HD_LAYOUT_HOLD(SDL_clamp(trailFrame, 0, 200))
              : i == 3 ? HD_LAYOUT_HOLD(SDL_clamp(hpFrame, 0, 200))
              : HD_LAYOUT_HOLD(layers);
        PortHdLayoutDraw(renderer, HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE, 1, kEnemyPlaceGroup[i], frame, x, y, s, 1.0f, tint);
    }
}
