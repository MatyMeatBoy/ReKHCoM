/*
 * HD sprites (HD mod, beta): sprites of the cartridge drawn with Re:CoM's HD
 * art where the two are the same picture (HUD_HD_RECOM.md). The first group
 * is the battle cards (illustration, category border and value digit), the
 * second the title screen's logo.
 *
 * A sprite is recognised by the ROM tiles it shows (and, for the digits that
 * share one tile set, by its frame), so no game code is touched: engine.c
 * hands every OAM part its tiles, frame and box in the frame's coordinates
 * (PortOamSetHdPart). The importer (khhd_import.py, the launcher) writes one
 * PNG per sprite under hd/sprites/, covering the sprite's own box at a
 * higher resolution; each OAM part is drawn as a quad of its piece of that
 * image, with the part's own affine transform, so rotation, scaling and
 * flips follow the game exactly. Nothing is shipped with ReKHCoM.
 *
 * Missing files keep the cartridge's sprite.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "port/port_hdlayout.h"
#include "port/port_hdsprites.h"
#include "../decompui/third_party/stb_image.h"

typedef struct HdSpriteDef {
    const void* tiles;
    const void* frame; /* NULL: any frame with these tiles */
    const char* name;
    int16_t x0, y0, w, h; /* the sprite's box in its frame's coordinates */
    uint8_t group;
} HdSpriteDef;

#include "port/hd_sprites.inc"

#define HD_SPRITE_COUNT ((int)(sizeof(kHdSprites) / sizeof(kHdSprites[0])))
#define MAX_DRAWS 128

typedef struct HdSpriteState {
    SDL_Texture *image, *white;
    uint32_t lastUsed;
    size_t bytes;
    uint8_t present; /* 0 not looked up yet, 1 missing, 2 on disk */
} HdSpriteState;

static HdSpriteState sState[HD_SPRITE_COUNT];
static size_t sBytes;
static SDL_Renderer* sRenderer;

typedef struct HdSpriteDraw {
    int id;
    float x[4], y[4]; /* HUD canvas corners: top-left, top-right, bottom-right, bottom-left of the part */
    float u0, v0, u1, v1;
    uint8_t alpha, dark, white;
} HdSpriteDraw;
/* per layer: 0 the game's picture, 1 the HUD canvas, 2 the screen layer (menus, the title's logo) */
#define HD_LAYERS 3
static HdSpriteDraw sDraws[HD_LAYERS][MAX_DRAWS];
static int sDrawCount[HD_LAYERS];

static int GroupOn(int group) {
    switch (group) {
    case 1: return gPortConfig.hdCards;
    case 2: return gPortConfig.hdTitle;
    case 3: return gPortConfig.hdCards; /* the battle HUD's faces and words go with the cards */
    case 4: return gPortConfig.hdCards; /* the world select's worlds */
    case 5: return gPortConfig.hdCards; /* the menus' title banners, in Re:CoM's lettering */
    case 6: return gPortConfig.hdCards; /* the menus' small cards: Re:CoM's picture in the window, the cartridge's frame */
    default: return 0;
    }
}

/* hd/sprites/<name>.png in the data folder (KHCOM_HD_SPRITES=<folder>: the test batteries' shared copy) */
const char* PortHdSpritePath(char* file, size_t size, const char* name);
static const char* SpritePath(char* file, size_t size, const char* name) { return PortHdSpritePath(file, size, name); }
const char* PortHdSpritePath(char* file, size_t size, const char* name) {
    const char* dir = SDL_getenv("KHCOM_HD_SPRITES");
    if (dir) {
        SDL_snprintf(file, size, "%s/%s.png", dir, name);
        return file;
    }
    SDL_snprintf(file, size, "hd/sprites/%s.png", name);
    return PortConfigPath(file);
}

int PortHdSpriteFind(const void* tiles, const void* frame) {
    int i;
    if (!tiles || (!gPortConfig.hdCards && !gPortConfig.hdTitle))
        return 0;
    for (i = 0; i < HD_SPRITE_COUNT; i++) {
        const HdSpriteDef* d = &kHdSprites[i];
        if (d->tiles != tiles || (d->frame && d->frame != frame) || !GroupOn(d->group))
            continue;
        if (!sState[i].present) {
            char file[160];
            FILE* f;
            f = fopen(SpritePath(file, sizeof(file), d->name), "rb");
            sState[i].present = f ? 2 : 1;
            if (f) fclose(f);
        }
        return sState[i].present == 2 ? i + 1 : 0;
    }
    return 0;
}

int PortHdSpriteAny(const void* tiles) {
    int i;
    for (i = 0; i < HD_SPRITE_COUNT; i++)
        if (kHdSprites[i].tiles == tiles && PortHdSpriteFind(tiles, kHdSprites[i].frame))
            return 1;
    return 0;
}

/* 1: the cartridge still draws its sprite under the HD part (the small cards keep their pixel crown, border and the
   value's notch; the picture covers only their window). */
int PortHdSpriteKeepsCartridge(int id) {
    return id >= 1 && id <= HD_SPRITE_COUNT && kHdSprites[id - 1].group == 6;
}

int PortHdSpriteGroup(int id) {
    return id >= 1 && id <= HD_SPRITE_COUNT ? kHdSprites[id - 1].group : 0;
}

void PortHdSpriteBox(int id, int* x0, int* y0, int* w, int* h) {
    const HdSpriteDef* d = &kHdSprites[id - 1];
    *x0 = d->x0; *y0 = d->y0; *w = d->w; *h = d->h;
}

static void Unload(int i) {
    if (sState[i].image) SDL_DestroyTexture(sState[i].image);
    if (sState[i].white) SDL_DestroyTexture(sState[i].white);
    sState[i].image = sState[i].white = NULL;
    sBytes -= sState[i].bytes;
    sState[i].bytes = 0;
}

static int Load(int i) {
    char file[160];
    int w, h, n, k;
    unsigned char* px;
    SDL_Surface* s;
    size_t need;
    px = stbi_load(SpritePath(file, sizeof(file), kHdSprites[i].name), &w, &h, &n, 4);
    if (!px || w < 1 || h < 1 || w > 4096 || h > 4096) {
        if (px) stbi_image_free(px);
        sState[i].present = 1;
        PortLog("HD sprites: %s unreadable, the cartridge's sprite stays", file);
        return 0;
    }
    need = (size_t)w * h * 8;
    /* the cards on screen stay; the oldest others make room (32 MB; 12 MB on Switch and PS Vita, whose memory is
       shared with the game's image and the host's own copy of each texture) */
#ifdef PORT_GBA_RELOC
    const size_t budget = 12u * 1024 * 1024;
#else
    const size_t budget = 32u * 1024 * 1024;
#endif
    while (sBytes + need > budget) {
        int victim = -1;
        for (k = 0; k < HD_SPRITE_COUNT; k++)
            if (sState[k].image && sState[k].lastUsed != PortFrameCount() &&
                (victim < 0 || sState[k].lastUsed < sState[victim].lastUsed))
                victim = k;
        if (victim < 0) break;
        Unload(victim);
    }
    s = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4);
    if (s) {
        sState[i].image = SDL_CreateTextureFromSurface(sRenderer, s);
        /* the white copy for the game's brightening fades */
        for (k = 0; k < w * h; k++) px[k * 4] = px[k * 4 + 1] = px[k * 4 + 2] = 255;
        sState[i].white = SDL_CreateTextureFromSurface(sRenderer, s);
        SDL_DestroySurface(s);
    }
    stbi_image_free(px);
    if (!sState[i].image || !sState[i].white) {
        Unload(i);
        sState[i].present = 1;
        return 0;
    }
    SDL_SetTextureBlendMode(sState[i].image, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(sState[i].white, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(sState[i].image, SDL_SCALEMODE_LINEAR);
    SDL_SetTextureScaleMode(sState[i].white, SDL_SCALEMODE_LINEAR);
    sState[i].bytes = need;
    sBytes += need;
    return 1;
}

void PortHdSpritesBegin(int layer) {
    if ((unsigned)layer < HD_LAYERS)
        sDrawCount[layer] = 0;
}

/* Test aid (KHCOM_HDDRAWLOG=frame,frame,...): this frame's HD drawing is logged ("hd draw <frame> <name> layer <n>",
   "hd gauge ..."), so test_hd_menus.py can tell an HD title still on screen from one that fell back to the cartridge. */
int PortHdDrawLogged(void) {
    static const char* list;
    static int checked;
    const char* p;
    unsigned frame = PortFrameCount();
    if (!checked) {
        list = SDL_getenv("KHCOM_HDDRAWLOG");
        checked = 1;
    }
    for (p = list; p && *p;) {
        if ((unsigned)SDL_atoi(p) == frame)
            return 1;
        p = SDL_strchr(p, ',');
        if (p)
            p++;
    }
    return 0;
}

int PortHdSpriteQueue(int layer, int id, const float* x, const float* y, int px, int py, int pw, int ph, int alpha,
                      int dark, int white) {
    const HdSpriteDef* d;
    HdSpriteDraw* q;
    int k;
    if ((unsigned)layer >= HD_LAYERS)
        return 0;
    if (id < 1 || id > HD_SPRITE_COUNT || sDrawCount[layer] == MAX_DRAWS)
        return 0;
    d = &kHdSprites[id - 1];
    /* a part outside the picture's box (another frame of these tiles) or a
       degenerate transform keeps the cartridge's pixels */
    if (px < d->x0 || py < d->y0 || px + pw > d->x0 + d->w || py + ph > d->y0 + d->h) {
        static int sLogged;
        if (sLogged++ < 8)
            PortLog("HD sprites: %s part %d,%d %dx%d outside its box", d->name, px, py, pw, ph);
        return 0;
    }
    for (k = 0; k < 4; k++)
        if (!(x[k] > -4096.0f && x[k] < 8192.0f && y[k] > -4096.0f && y[k] < 8192.0f))
            return 0;
    if (PortHdDrawLogged()) {
        static uint32_t sLoggedAt[HD_SPRITE_COUNT];
        if (sLoggedAt[id - 1] != PortFrameCount() + 1) {
            sLoggedAt[id - 1] = PortFrameCount() + 1;
            PortLog("hd draw %u %s layer %d", (unsigned)PortFrameCount(), d->name, layer);
        }
    }
    q = &sDraws[layer][sDrawCount[layer]++];
    q->id = id - 1;
    memcpy(q->x, x, sizeof(q->x));
    memcpy(q->y, y, sizeof(q->y));
    q->u0 = (float)(px - d->x0) / d->w;
    q->v0 = (float)(py - d->y0) / d->h;
    q->u1 = (float)(px + pw - d->x0) / d->w;
    q->v1 = (float)(py + ph - d->y0) / d->h;
    q->alpha = (uint8_t)alpha;
    q->dark = (uint8_t)SDL_clamp(dark, 0, 255);
    q->white = (uint8_t)SDL_clamp(white, 0, 255);
    return 1;
}

static void Quad(SDL_Texture* t, const HdSpriteDraw* q, float ox, float oy, float scale, SDL_FColor c) {
    SDL_Vertex v[4];
    static const int kIndex[6] = { 0, 1, 2, 0, 2, 3 };
    float area;
    int k;
    for (k = 0; k < 4; k++) {
        v[k].position.x = ox + q->x[k] * scale;
        v[k].position.y = oy + q->y[k] * scale;
        v[k].color = c;
    }
    v[0].tex_coord.x = q->u0; v[0].tex_coord.y = q->v0;
    v[1].tex_coord.x = q->u1; v[1].tex_coord.y = q->v0;
    v[2].tex_coord.x = q->u1; v[2].tex_coord.y = q->v1;
    v[3].tex_coord.x = q->u0; v[3].tex_coord.y = q->v1;
    area = (v[1].position.x - v[0].position.x) * (v[3].position.y - v[0].position.y) -
           (v[1].position.y - v[0].position.y) * (v[3].position.x - v[0].position.x);
    if (area < 0.5f && area > -0.5f)
        return;
    /* An axis-aligned quad is cut to the window first (texture coordinates in proportion): SDL's software
       renderer drops a triangle that reaches far past the canvas (the title's logo in its intro zoom was not
       drawn at all), and it saves the work of what cannot be seen. */
    if (v[0].position.y == v[1].position.y && v[0].position.x == v[3].position.x) {
        int ow = 0, oh = 0;
        /* Corners on whole pixels: pieces of one picture meet on a shared edge, and with the canvas on a half pixel
           (the title's logo at x4.5) that edge ran through the pixel centres; the hundredth-of-a-pixel tilt below
           then left that row to neither piece and the background showed as a thin line (23/10). Rounded, shared
           edges stay shared. */
        for (k = 0; k < 4; k++) {
            v[k].position.x = SDL_roundf(v[k].position.x);
            v[k].position.y = SDL_roundf(v[k].position.y);
        }
        if (SDL_GetCurrentRenderOutputSize(sRenderer, &ow, &oh) && ow > 0 && oh > 0) {
            float x0 = v[0].position.x, x1 = v[1].position.x, y0 = v[0].position.y, y1 = v[3].position.y;
            float u0 = q->u0, u1 = q->u1, w0 = q->v0, w1 = q->v1;
            float cx0 = SDL_clamp(x0, 0, (float)ow), cx1 = SDL_clamp(x1, 0, (float)ow);
            float cy0 = SDL_clamp(y0, 0, (float)oh), cy1 = SDL_clamp(y1, 0, (float)oh);
            if (cx0 == cx1 || cy0 == cy1)
                return; /* entirely outside */
            v[0].position.x = v[3].position.x = cx0;
            v[1].position.x = v[2].position.x = cx1;
            v[0].position.y = v[1].position.y = cy0;
            v[2].position.y = v[3].position.y = cy1;
            v[0].tex_coord.x = v[3].tex_coord.x = u0 + (u1 - u0) * (cx0 - x0) / (x1 - x0);
            v[1].tex_coord.x = v[2].tex_coord.x = u0 + (u1 - u0) * (cx1 - x0) / (x1 - x0);
            v[0].tex_coord.y = v[1].tex_coord.y = w0 + (w1 - w0) * (cy0 - y0) / (y1 - y0);
            v[2].tex_coord.y = v[3].tex_coord.y = w0 + (w1 - w0) * (cy1 - y0) / (y1 - y0);
        }
    }
    /* SDL turns an axis-aligned quad into a texture copy, and the software
       renderer's scaled copy reads outside the texture when the quad is
       mirrored, turned a quarter (a card spinning) or partly off the canvas (a
       face at the HUD's edge): it crashed. A hundredth of a pixel keeps every
       such quad on the triangle path, which clips and maps it correctly. */
    if (v[0].position.x == v[1].position.x || v[0].position.y == v[1].position.y) {
        v[1].position.x += 0.01f;
        v[1].position.y += 0.01f;
    }
    const char* spriteLog;
    PORT_ENV_ONCE(spriteLog, SDL_getenv, "KHCOM_HDSPRITES_LOG");
    if (spriteLog)
        PortLog("hdsprite f=%u %s (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) uv %.3f,%.3f-%.3f,%.3f", PortFrameCount(),
                kHdSprites[q->id].name, v[0].position.x, v[0].position.y, v[1].position.x, v[1].position.y,
                v[2].position.x, v[2].position.y, v[3].position.x, v[3].position.y, q->u0, q->v0, q->u1, q->v1);
    {
        /* A large quad goes as a grid of smaller ones: SDL's software renderer draws nothing of a triangle
           this big (the title's logo, 1700 pixels wide in the intro). Corners and texture coordinates are
           interpolated, exact for any affine transform. */
        float span = SDL_max(SDL_fabsf(v[2].position.x - v[0].position.x), SDL_fabsf(v[2].position.y - v[0].position.y));
        int n = SDL_clamp((int)(span / 256.0f) + 1, 1, 8), i, j;
        SDL_Vertex grid[81];
        int index[6 * 64], ni = 0;
        if (n == 1) {
            SDL_RenderGeometry(sRenderer, t, v, 4, kIndex, 6);
            return;
        }
        for (j = 0; j <= n; j++)
            for (i = 0; i <= n; i++) {
                float a = (float)i / n, b = (float)j / n;
                SDL_Vertex* g = &grid[j * (n + 1) + i];
#define LERP2(f) ((1 - b) * ((1 - a) * v[0].f + a * v[1].f) + b * ((1 - a) * v[3].f + a * v[2].f))
                g->position.x = LERP2(position.x);
                g->position.y = LERP2(position.y);
                g->tex_coord.x = LERP2(tex_coord.x);
                g->tex_coord.y = LERP2(tex_coord.y);
#undef LERP2
                g->color = c;
            }
        for (j = 0; j < n; j++)
            for (i = 0; i < n; i++) {
                int p = j * (n + 1) + i;
                index[ni++] = p; index[ni++] = p + 1; index[ni++] = p + n + 2;
                index[ni++] = p; index[ni++] = p + n + 2; index[ni++] = p + n + 1;
            }
        SDL_RenderGeometry(sRenderer, t, grid, (n + 1) * (n + 1), index, ni);
    }
}

/* ---- HP ring (Re:CoM's player gauge, the user's pick 22/10) ---------------------------------------------------
   hd/sprites/gBtlHpRing.png is pl_gauge at 8 pixels a GBA pixel: a ring (green between radii 128 and 170 around
   RING_CX, RING_CY, open at the top right) that runs into a bar along the top. Re:CoM's own fill logic is in its
   executable, not in its data: this gauge fills with the game's HP, the bar's tip emptying first, then the ring back
   to its lower right end. The face in it is the cartridge's (its HD picture, with its expressions): the ring's baked
   face is cut out when it loads. The empty part is the same picture in dark grey. */
#define RING_CX 178.7f
#define RING_CY 179.5f
#define RING_RMID 149.0f
#define RING_HOLE 118.0f
#define RING_BAR_END 577.0f
#define RING_FACE_X 9.0f /* the ring's centre in the face frame's coordinates (GBA pixels) */
#define RING_FACE_Y 8.0f
static SDL_Texture *sRingFull, *sRingEmpty;
static int sRingTried, sRingRatio = -1;

static int RingLoad(void) {
    char file[160];
    int w, h, n, k;
    unsigned char *px, *dark;
    SDL_Surface* sf;
    if (sRingFull)
        return 1;
    if (sRingTried || !sRenderer)
        return 0;
    sRingTried = 1;
    px = stbi_load(SpritePath(file, sizeof(file), "gBtlHpRing"), &w, &h, &n, 4);
    if (!px)
        return 0;
    for (k = 0; k < w * h; k++) {
        float dx = (k % w) * 592.0f / w - RING_CX, dy = (k / w) * 360.0f / h - RING_CY;
        if (dx * dx + dy * dy < RING_HOLE * RING_HOLE)
            px[k * 4 + 3] = 0; /* the baked face: the game's own (HD) face shows there */
    }
    dark = SDL_malloc((size_t)w * h * 4);
    if (dark) {
        SDL_memcpy(dark, px, (size_t)w * h * 4);
        for (k = 0; k < w * h; k++) {
            unsigned char* c = dark + k * 4;
            if (c[1] > c[0] + 30 && c[1] > c[2] + 30) /* the green: the empty track's dark grey */
                c[0] = c[1] = c[2] = (unsigned char)(28 + c[1] / 16);
        }
    }
    sf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4);
    if (sf) {
        sRingFull = SDL_CreateTextureFromSurface(sRenderer, sf);
        SDL_DestroySurface(sf);
    }
    sf = dark ? SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, dark, w * 4) : NULL;
    if (sf) {
        sRingEmpty = SDL_CreateTextureFromSurface(sRenderer, sf);
        SDL_DestroySurface(sf);
    }
    SDL_free(dark);
    stbi_image_free(px);
    if (!sRingFull || !sRingEmpty) {
        if (sRingFull) SDL_DestroyTexture(sRingFull);
        if (sRingEmpty) SDL_DestroyTexture(sRingEmpty);
        sRingFull = sRingEmpty = NULL;
        return 0;
    }
    return 1;
}

int PortHdHpRing(const void* faceTiles, const void* faceFrame, int ratio) {
    char file[160];
    FILE* f;
    if (!gPortConfig.hdCards || !PortHdSpriteFind(faceTiles, faceFrame))
        return 0;
    if (!sRingFull) {
        if (sRingTried)
            return 0;
        f = fopen(SpritePath(file, sizeof(file), "gBtlHpRing"), "rb"); /* loaded when first drawn */
        if (!f)
            return 0;
        fclose(f);
    }
    sRingRatio = SDL_clamp(ratio, 0, 256);
    return 1;
}

/* A point of the ring picture, its centre placed at (cx, cy) on the screen, s screen pixels a picture pixel. */
static SDL_Vertex RingVertex(float px, float py, float cx, float cy, float s, SDL_FColor c) {
    SDL_Vertex v;
    v.position.x = cx + (px - RING_CX) * s;
    v.position.y = cy + (py - RING_CY) * s;
    v.tex_coord.x = px / 592.0f;
    v.tex_coord.y = py / 360.0f;
    v.color = c;
    return v;
}

static void RingDraw(float cx, float cy, float s, SDL_FColor c) {
    static const int kQuad[6] = { 0, 1, 2, 0, 2, 3 };
    const float ringLen = 1.5f * SDL_PI_F * RING_RMID, barLen = RING_BAR_END - RING_CX;
    float filled = (ringLen + barLen) * sRingRatio / 256.0f, end;
    SDL_Vertex v[4], fan[40];
    int idx[3 * 38], n = 0, ni = 0, k, steps;
    /* the empty track whole, then the filled part over it */
    v[0] = RingVertex(0, 0, cx, cy, s, c);
    v[1] = RingVertex(592, 0, cx, cy, s, c);
    v[2] = RingVertex(592, 360, cx, cy, s, c);
    v[3] = RingVertex(0, 360, cx, cy, s, c);
    v[1].position.x += 0.01f; /* off the triangle path SDL's software renderer copies, and reads past (Quad) */
    v[1].position.y += 0.01f;
    SDL_RenderGeometry(sRenderer, sRingEmpty, v, 4, kQuad, 6);
    if (filled <= 0)
        return;
    /* the ring: a fan from its lower right end (angle 0), clockwise on the screen */
    end = SDL_min(filled, ringLen) / RING_RMID;
    steps = SDL_clamp((int)(end / 0.13f) + 1, 1, 37);
    fan[n++] = RingVertex(RING_CX, RING_CY, cx, cy, s, c);
    for (k = 0; k <= steps; k++) {
        float a = end * k / steps, r = 182.0f;
        fan[n++] = RingVertex(RING_CX + r * SDL_cosf(a), RING_CY + r * SDL_sinf(a), cx, cy, s, c);
        if (k > 0) {
            idx[ni++] = 0;
            idx[ni++] = n - 2;
            idx[ni++] = n - 1;
        }
    }
    SDL_RenderGeometry(sRenderer, sRingFull, fan, n, idx, ni);
    /* the bar: the top right quarter, up to the HP's length */
    if (filled > ringLen) {
        float x = filled - ringLen >= barLen ? 592.0f : RING_CX + (filled - ringLen);
        v[0] = RingVertex(RING_CX, 0, cx, cy, s, c);
        v[1] = RingVertex(x, 0, cx, cy, s, c);
        v[2] = RingVertex(x, RING_CY, cx, cy, s, c);
        v[3] = RingVertex(RING_CX, RING_CY, cx, cy, s, c);
        v[1].position.x += 0.01f;
        v[1].position.y += 0.01f;
        SDL_RenderGeometry(sRenderer, sRingFull, v, 4, kQuad, 6);
    }
}

void PortHdSpritesDraw(SDL_Renderer* renderer, int layer, float ox, float oy, float scale, int darken, int brighten) {
    int i;
    sRenderer = renderer;
    if ((unsigned)layer >= HD_LAYERS)
        return;
    if (layer == 1) /* Re:CoM's HP gauge on the HUD canvas, under the cards */
        PortHdGaugeDraw(renderer, ox, oy, scale, 1.0f - (darken > 0 ? darken / 16.0f : 0.0f));
    if (layer == 1)
        PortHdEnemyGaugeDraw(renderer, ox, oy, scale, 1.0f - (darken > 0 ? darken / 16.0f : 0.0f));
    for (i = 0; i < sDrawCount[layer]; i++) {
        const HdSpriteDraw* q = &sDraws[layer][i];
        HdSpriteState* st = &sState[q->id];
        /* the game's brightness fade (BLDY) and the part's palette fade, whichever is further */
        float shade = 1.0f - SDL_max(darken > 0 ? darken / 16.0f : 0.0f, q->dark / 255.0f);
        float white = SDL_max(brighten > 0 ? brighten / 16.0f : 0.0f, q->white / 255.0f);
        if (!st->image && !Load(q->id))
            continue;
        st->lastUsed = PortFrameCount();
        if (layer == 1 && sRingRatio >= 0 && SDL_strstr(kHdSprites[q->id].name, "FaceTiles") && RingLoad()) {
            /* the HP ring behind the face: the face frame's origin from this part's corner and its place in the
               picture (faces are drawn unturned) */
            const HdSpriteDef* d = &kHdSprites[q->id];
            float fx = q->x[0] - (q->u0 * d->w + d->x0), fy = q->y[0] - (q->v0 * d->h + d->y0);
            RingDraw(ox + (fx + RING_FACE_X) * scale, oy + (fy + RING_FACE_Y) * scale, scale / 8.0f,
                     (SDL_FColor){ shade, shade, shade, q->alpha / 255.0f });
            sRingRatio = -1; /* once a frame, behind its first face part */
        }
        Quad(st->image, q, ox, oy, scale, (SDL_FColor){ shade, shade, shade, q->alpha / 255.0f });
        if (white > 0)
            Quad(st->white, q, ox, oy, scale, (SDL_FColor){ 1, 1, 1, q->alpha / 255.0f * white });
    }
}

void PortPaletteFade(const uint16_t* original, const volatile uint16_t* current, int count, int* dark, int* white) {
    int i, so = 0, sc = 0, room = 0, err = 0, k;
    *dark = *white = 0;
    if (!original || count < 2)
        return;
    for (i = 1; i < count; i++) /* entry 0 is transparent */
        for (k = 0; k < 15; k += 5) {
            int o = (original[i] >> k) & 31, c = (current[i] >> k) & 31;
            so += o;
            sc += c;
            room += 31 - o;
        }
    if (sc < so && so > 0) {
        int f = 256 * (so - sc) / so; /* c = o (1 - f) */
        for (i = 1; i < count; i++)
            for (k = 0; k < 15; k += 5) {
                int o = (original[i] >> k) & 31, c = (current[i] >> k) & 31;
                err += SDL_abs(c - o * (256 - f) / 256);
            }
        if (err <= 2 * 3 * (count - 1) && f > 6)
            *dark = SDL_min(f, 255);
    } else if (sc > so && room > 0) {
        int f = 256 * (sc - so) / room; /* c = o + (31 - o) f */
        for (i = 1; i < count; i++)
            for (k = 0; k < 15; k += 5) {
                int o = (original[i] >> k) & 31, c = (current[i] >> k) & 31;
                err += SDL_abs(c - (o + (31 - o) * f / 256));
            }
        if (err <= 2 * 3 * (count - 1) && f > 6)
            *white = SDL_min(f, 255);
    }
}

/* Renderer gone (or the HD files re-imported): drop the textures and look the files up again. */
void PortHdSpritesShutdown(void) {
    int i;
    if (sRingFull) SDL_DestroyTexture(sRingFull);
    if (sRingEmpty) SDL_DestroyTexture(sRingEmpty);
    sRingFull = sRingEmpty = NULL;
    sRingTried = 0;
    for (i = 0; i < HD_SPRITE_COUNT; i++) {
        Unload(i);
        sState[i].present = 0;
    }
}
