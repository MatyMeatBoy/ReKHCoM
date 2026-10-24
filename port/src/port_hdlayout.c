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

#include "port/port.h"
#include "port/port_hdlayout.h"
#include "port/port_hdsprites.h"
#include "../decompui/third_party/stb_image.h"

const char* PortHdSpritePath(char* file, size_t size, const char* name);

typedef struct HdLayout {
    unsigned char* data;
    int size;
} HdLayout;

typedef struct HdLayoutTexture {
    SDL_Texture* tex;
    int w, h, tried;
} HdLayoutTexture;

static HdLayout sLayouts[HD_LAYOUT_COUNT];
static int sLayoutTried[HD_LAYOUT_COUNT];
static HdLayoutTexture sTextures[HD_LAYOUT_TEXTURE_COUNT];
/* the 1.5+2.5's remastered layouts (UK_gauge.lad...): their UVs are the HD textures' own; the PS2 ones (gauge.lad)
   doubled miss the pieces the remaster moved (the ring's quarters, the bar) */
static const char* const kLayoutFiles[HD_LAYOUT_COUNT] = { "gHudGaugeHdLad", "gHudCockpitHdLad" };
static const char* const kTextureFiles[HD_LAYOUT_TEXTURE_COUNT] = { "gHudGaugeTex0", "gHudGaugeTex1",
                                                                     "gHudRikuGaugeTex0", "gHudCockpitTex" };

static int32_t I32(const unsigned char* p) { return (int32_t)(p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24); }
static int16_t I16(const unsigned char* p) { return (int16_t)(p[0] | p[1] << 8); }
static float F32(const unsigned char* p) { float f; uint32_t v = (uint32_t)I32(p); memcpy(&f, &v, 4); return f; }

static const HdLayout* Layout(int which) {
    if ((unsigned)which >= HD_LAYOUT_COUNT)
        return NULL;
    if (!sLayouts[which].data && !sLayoutTried[which]) {
        char file[160], name[64];
        size_t size = 0;
        void* data;
        sLayoutTried[which] = 1;
        SDL_snprintf(name, sizeof(name), "%s", kLayoutFiles[which]);
        PortHdSpritePath(file, sizeof(file), name);
        /* the path helper names .png: the layout is the same name with .lad */
        if (strlen(file) > 4 && SDL_strcmp(file + strlen(file) - 4, ".png") == 0)
            memcpy(file + strlen(file) - 4, ".lad", 4);
        data = SDL_LoadFile(file, &size);
        if (data && size >= 32 && I32((const unsigned char*)data) == 0x4459414C) {
            sLayouts[which].data = data;
            sLayouts[which].size = (int)size;
        } else if (data) {
            SDL_free(data);
        }
    }
    return sLayouts[which].data ? &sLayouts[which] : NULL;
}

static HdLayoutTexture* Texture(SDL_Renderer* renderer, int which) {
    HdLayoutTexture* t;
    if ((unsigned)which >= HD_LAYOUT_TEXTURE_COUNT)
        return NULL;
    t = &sTextures[which];
    if (!t->tex && !t->tried && renderer) {
        char file[160];
        int n;
        unsigned char* px;
        t->tried = 1;
        px = stbi_load(PortHdSpritePath(file, sizeof(file), kTextureFiles[which]), &t->w, &t->h, &n, 4);
        if (px) {
            SDL_Surface* s = SDL_CreateSurfaceFrom(t->w, t->h, SDL_PIXELFORMAT_RGBA32, px, t->w * 4);
            if (s) {
                t->tex = SDL_CreateTextureFromSurface(renderer, s);
                SDL_DestroySurface(s);
            }
            stbi_image_free(px);
            if (t->tex)
                SDL_SetTextureBlendMode(t->tex, SDL_BLENDMODE_BLEND);
        }
    }
    return t->tex ? t : NULL;
}

int PortHdLayoutReady(int layout, int texture) {
    char file[160];
    FILE* f;
    if (!Layout(layout))
        return 0;
    if (sTextures[texture].tex)
        return 1;
    if (sTextures[texture].tried)
        return 0;
    f = fopen(PortHdSpritePath(file, sizeof(file), kTextureFiles[texture]), "rb");
    if (f)
        fclose(f);
    return f != NULL;
}

/* The layout's sequence n: its bytes and size. */
static const unsigned char* Sequence(const HdLayout* l, int n, int* size) {
    const unsigned char* b = l->data;
    int count = I32(b + 24), table = I32(b + 28), start, end;
    if (n < 0 || n >= count || table + 4 * count > l->size)
        return NULL;
    start = I32(b + table + 4 * n);
    end = n + 1 < count ? I32(b + table + 4 * (n + 1)) : l->size;
    if (start < 0 || end > l->size || end - start < 48 || I32(b + start) != 0x44514553)
        return NULL;
    *size = end - start;
    return b + start;
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
    SDL_Vertex v[4];
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
        v[k].tex_coord.x = tu[k];
        v[k].tex_coord.y = tv[k];
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
        return 0;
    }
    /* SDL's software renderer turns an axis-aligned quad into a copy that reads past the texture (port_hdsprites.c,
       Quad): a hundredth of a pixel keeps it on the triangle path */
    v[1].position.x += 0.01f;
    v[1].position.y += 0.01f;
    SDL_RenderGeometry(r, t->tex, v, 4, kIndex, 6);
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

void PortHdLayoutShutdown(void) {
    int i;
    for (i = 0; i < HD_LAYOUT_TEXTURE_COUNT; i++) {
        if (sTextures[i].tex)
            SDL_DestroyTexture(sTextures[i].tex);
        memset(&sTextures[i], 0, sizeof(sTextures[i]));
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
    if (!gPortConfig.hdCards || !PortHdLayoutReady(HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE) ||
        !PortHdLayoutReady(HD_LAYOUT_GAUGE, riku ? HD_LAYOUT_TEX_RIKU_FACES : HD_LAYOUT_TEX_GAUGE_FACES))
        return 0;
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
   Android, with less memory and fill rate, use 2. */
#if defined(PORT_GBA_RELOC) || defined(PORT_ANDROID64) || defined(__ANDROID__)
#define GAUGE_SS 2
#else
#define GAUGE_SS 4
#endif
#if __STDC_HOSTED__ /* the consoles' SDL shim has no render targets: there the gauge draws as before */
static SDL_Texture* sGaugeTarget[3];
static int sGaugeTargetW[3], sGaugeTargetH[3];

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
        SDL_SetTextureScaleMode(sGaugeTarget[i], SDL_SCALEMODE_LINEAR);
    }
    return sGaugeTarget[i];
}

#endif

void PortHdGaugeDraw(SDL_Renderer* renderer, float ox, float oy, float scale, float shade) {
    /* gauge:01 sits at (60, 60) of Re:CoM's 640-wide screen: the GBA HUD's 240 gives 0.375 of a GBA pixel a unit */
    const float unit = 240.0f / 640.0f;
    float x = ox + 60.0f * unit * scale, y = oy + 60.0f * unit * scale, s = unit * scale;
    float tint[4] = { shade, shade, shade, 1.0f };
    if (!sGauge.active || sGauge.frame != (int)PortFrameCount())
        return;
    if (PortHdDrawLogged())
        PortLog("hd gauge %u hp %d/%d trail %d state %d face %d", (unsigned)PortFrameCount(), sGauge.hp, sGauge.maxHp,
                sGauge.trail, sGauge.stateAnim, sGauge.faceAnim);
#if __STDC_HOSTED__
    /* not on SDL's software renderer (the tests' offscreen window): restoring the window as the target crashed
       inside it */
    if (!SDL_getenv("KHCOM_GAUGE_NO_SS") && SDL_strcmp(SDL_GetRendererName(renderer), SDL_SOFTWARE_RENDERER) != 0) {
        /* the gauge's area in Re:CoM's units around its origin (ring, face, the bar at its longest, the alarm's glow) */
        const float ux0 = -60.0f, uy0 = -60.0f, ux1 = 620.0f, uy1 = 220.0f;
        float rx = x + ux0 * s, ry = y + uy0 * s, rw = (ux1 - ux0) * s, rh = (uy1 - uy0) * s;
        int w0 = (int)SDL_ceilf(rw), h0 = (int)SDL_ceilf(rh), k = GAUGE_SS, level = 0;
        SDL_Texture* big = w0 > 0 && h0 > 0 ? GaugeTarget(renderer, 0, w0 * k, h0 * k) : NULL;
        if (big) {
            SDL_Texture* prev = SDL_GetRenderTarget(renderer);
            SDL_Rect viewport, clip;
            int clipped = SDL_RenderClipEnabled(renderer);
            SDL_FRect src, dst;
            SDL_Texture* cur = big;
            int cw = w0 * k, ch = h0 * k;
            SDL_GetRenderViewport(renderer, &viewport);
            SDL_GetRenderClipRect(renderer, &clip);
            SDL_SetRenderTarget(renderer, big);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            GaugeParts(renderer, (x - rx) * k, (y - ry) * k, s * k, tint);
            /* down in halves: each a 2x2 box of premultiplied pixels */
            while (cw > w0) {
                int nw = SDL_max(cw / 2, w0), nh = SDL_max(ch / 2, h0);
                SDL_Texture* next = GaugeTarget(renderer, 1 + (level & 1), nw, nh);
                if (!next)
                    break;
                SDL_SetRenderTarget(renderer, next);
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
            SDL_SetRenderTarget(renderer, prev);
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
   gauge.lad's layout group 0: seven places in sequence 1 (0 the track, 1 the HP label, 2 the red trail, 3 the green
   HP, 4..6 the extra bars' counter). Re:CoM's rules: a hit drops the green at once and holds the trail 1.5 s, then the
   trail closes a tenth of the gap and one more each step; healing moves both. Its bars hold 200 HP each; here a bar
   is the cartridge's own 560 (its gauge's layers: green, blue, yellow, red), so the HP and its layers stay the game's.
   The gauge sits where the cartridge's is, its right end on the cartridge bar's. */
#define ENEMY_LAYER 560
static const int kEnemyPlaceGroup[7] = { 2, 0, 1, 3, 4, 5, 6 };
static struct {
    int active, frame, hp, maxHp, trail, delayMs, started, anim;
    const void* who;
} sEnemy;

int PortHdEnemyGauge(int visible, int hp, int maxHp, const void* who) {
    if (!gPortConfig.hdCards || !PortHdLayoutReady(HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE))
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
    sEnemy.anim++;
    sEnemy.active = 1;
    return 1;
}

void PortHdEnemyGaugeDraw(SDL_Renderer* renderer, float ox, float oy, float scale, float shade) {
    const float unit = 240.0f / 640.0f;
    float s = unit * scale, tint[4] = { shade, shade, shade, 1.0f };
    /* Re:CoM's bar ends at x 495 of its 640; the cartridge's at 236 of its 240, which the HUD keeps from its right */
    float x = ox + ((float)(gPortHudW - GBA_SCREEN_W) + 236.0f - 495.0f * unit) * scale, y = oy;
    int layer, layers, hpFrame, trailFrame, trailLayer, i;
    if (!sEnemy.active || sEnemy.frame != (int)PortFrameCount())
        return;
    layer = sEnemy.hp > 0 ? (sEnemy.hp - 1) / ENEMY_LAYER : 0;
    trailLayer = sEnemy.trail > 0 ? (sEnemy.trail - 1) / ENEMY_LAYER : 0;
    {
        /* one bar at most: it is the enemy's whole HP, as the cartridge's gauge (its length by the maximum) */
        int full = sEnemy.maxHp > 0 && sEnemy.maxHp < ENEMY_LAYER ? sEnemy.maxHp : ENEMY_LAYER;
        hpFrame = (sEnemy.hp - layer * ENEMY_LAYER) * 200 / full;
        trailFrame = trailLayer > layer ? 200 : (sEnemy.trail - layer * ENEMY_LAYER) * 200 / full;
    }
    layers = layer;
    if (PortHdDrawLogged())
        PortLog("hd enemy gauge %u hp %d trail %d layer %d", (unsigned)PortFrameCount(), sEnemy.hp, sEnemy.trail, layer);
    for (i = 0; i < 7; i++) {
        int frame;
        if (i >= 4 && layers <= 0)
            continue; /* the bars' counter: only with bars left (0 is the bar's track, its empty length) */
        frame = i == 2 ? HD_LAYOUT_HOLD(SDL_clamp(trailFrame, 0, 200))
              : i == 3 ? HD_LAYOUT_HOLD(SDL_clamp(hpFrame, 0, 200))
              : i == 6 ? HD_LAYOUT_HOLD(layers)
              : sEnemy.anim;
        PortHdLayoutDraw(renderer, HD_LAYOUT_GAUGE, HD_LAYOUT_TEX_GAUGE, 1, kEnemyPlaceGroup[i], frame, x, y, s, 1.0f, tint);
    }
}
