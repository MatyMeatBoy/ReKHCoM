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
#include "port/port_menu_light.h"
#include "../decompui/third_party/stb_image.h"

typedef struct HdSpriteDef {
    const void* tiles;
    const void* frame; /* NULL: any frame with these tiles */
    const char* name;
    int16_t x0, y0, w, h; /* the sprite's box in its frame's coordinates */
    uint8_t group;
    uint8_t padL, padR, padT, padB; /* the picture goes on past the box this many GBA pixels (a Re:CoM word in its
                                       own proportion, a card's value as big as Re:CoM's: hd_sprites.tsv pad=L,R[,T,B]);
                                       the edge parts draw it */
} HdSpriteDef;

#include "port/hd_sprites.inc"

#define HD_SPRITE_COUNT ((int)(sizeof(kHdSprites) / sizeof(kHdSprites[0])))
#define MAX_DRAWS 128

typedef struct HdSpriteState {
    SDL_Texture *image, *white;
    uint32_t lastUsed;
    size_t bytes;
    int textureH;
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
    uint8_t hudOwner; /* OAM slot + 1: reload counter above native fill, clipped by final OAM ownership */
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
    case 7: return gPortConfig.hdTitle; /* the title's words, ours (embedded: TitleWordAsset) */
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

/* The HD title's words (group 7, make_title_words.py): pictures of the game's own, assets/hd/title/words/<name>.png
   (embedded), not imported from Re:CoM. */
static void* TitleWordAsset(const char* name, size_t* size) {
    char file[160];
    SDL_snprintf(file, sizeof(file), "assets/hd/title/words/%s.png", name);
    return PortLoadAsset(file, size);
}

/* The same authored light for every HD menu. Five small samples and one
   interpolated texture are allocated lazily, never a second animation/video. */
enum { MENU_LIGHT_SIZE = 88, MENU_LIGHT_BYTES = 88 * 88 * 4 };
static unsigned char* sMenuLightSamples;
static unsigned char* sMenuLightPixels;
static SDL_Texture *sMenuLightImage, *sMenuLightWhite;
static int sMenuLightTried, sMenuLightPhase = -1;

static void MenuLightRelease(void) {
    if (sMenuLightImage) SDL_DestroyTexture(sMenuLightImage);
    if (sMenuLightWhite) SDL_DestroyTexture(sMenuLightWhite);
    sMenuLightImage = sMenuLightWhite = NULL;
    SDL_free(sMenuLightSamples); SDL_free(sMenuLightPixels);
    sMenuLightSamples = sMenuLightPixels = NULL;
    sMenuLightTried = 0; sMenuLightPhase = -1;
}

static int MenuLightReady(SDL_Renderer* renderer) {
    int frame, row;
    SDL_Surface* surface;
    if (sMenuLightTried) return sMenuLightImage && sMenuLightWhite;
    sMenuLightTried = 1;
    sMenuLightSamples = SDL_malloc(MENU_LIGHT_BYTES * 5);
    sMenuLightPixels = SDL_malloc(MENU_LIGHT_BYTES);
    if (!sMenuLightSamples || !sMenuLightPixels) goto fail;
    for (frame = 0; frame < 5; frame++) {
        char name[64]; size_t size = 0;
        int w, h, channels;
        SDL_snprintf(name, sizeof(name), "gTitleMenuCursorFrame%d", frame);
        void* file = TitleWordAsset(name, &size);
        unsigned char* pixels = file && size <= 0x7FFFFFFF ? stbi_load_from_memory(file, (int)size, &w, &h, &channels, 4) : NULL;
        SDL_free(file);
        if (!pixels || w != 128 || h != 128) { if (pixels) stbi_image_free(pixels); goto fail; }
        for (row = 0; row < MENU_LIGHT_SIZE; row++)
            memcpy(sMenuLightSamples + frame * MENU_LIGHT_BYTES + row * MENU_LIGHT_SIZE * 4,
                   pixels + (row * w + 24) * 4, MENU_LIGHT_SIZE * 4);
        stbi_image_free(pixels);
    }
    memcpy(sMenuLightPixels, sMenuLightSamples, MENU_LIGHT_BYTES);
    surface = SDL_CreateSurfaceFrom(MENU_LIGHT_SIZE, MENU_LIGHT_SIZE, SDL_PIXELFORMAT_RGBA32,
                                    sMenuLightPixels, MENU_LIGHT_SIZE * 4);
    if (!surface) goto fail;
    /* This image is updated from RGBA bytes every animation tick. A texture
       converted from a surface may use a renderer-native BGRA format. */
    sMenuLightImage = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                        MENU_LIGHT_SIZE, MENU_LIGHT_SIZE);
    if (sMenuLightImage) SDL_UpdateTexture(sMenuLightImage, NULL, sMenuLightSamples, MENU_LIGHT_SIZE * 4);
    for (row = 0; row < MENU_LIGHT_BYTES; row += 4)
        sMenuLightPixels[row] = sMenuLightPixels[row + 1] = sMenuLightPixels[row + 2] = 255;
    sMenuLightWhite = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!sMenuLightImage || !sMenuLightWhite) goto fail;
    SDL_SetTextureBlendMode(sMenuLightImage, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(sMenuLightWhite, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(sMenuLightImage, (SDL_ScaleMode)PortSmoothScaleMode());
    SDL_SetTextureScaleMode(sMenuLightWhite, (SDL_ScaleMode)PortSmoothScaleMode());
    return 1;
fail:
    MenuLightRelease();
    sMenuLightTried = 1; /* missing samples keep the existing cartridge light */
    return 0;
}

static int MenuLightName(const char* name) {
    return !SDL_strncmp(name, "gTitleMenuCursorFrame", 21) || !SDL_strncmp(name, "gTitlePressStart", 16);
}

static int Load(int i);
static int LoadCounted(int i);

static int SpriteTextureReady(SDL_Texture* texture) {
#ifdef PORT_SDL_BRIDGE_TEXTURE_READY
    return PortBridgeTextureReady(texture);
#else
    return texture != NULL; /* native SDL creates its renderer texture immediately */
#endif
}

/* Find runs in game/OAM setup and Queue runs outside the PPU row workers.
   Promise only a usable base image; Queue also checks white when requested.
   Pin it now: later queues must not evict an image after its native pixels were omitted. */
static int SpriteReady(int i) {
    if (!sRenderer || sState[i].present == 1)
        return 0;
#if __STDC_HOSTED__
    if (!SDL_IsMainThread())
        return 0;
#endif
    /* Freestanding guests use the SDL shim without thread introspection.
       Their game/OAM setup and Queue callers run outside the PPU row workers. */
    if (!sState[i].image && !LoadCounted(i))
        return 0;
    if (!SpriteTextureReady(sState[i].image))
        return 0;
    sState[i].present = 2;
    sState[i].lastUsed = PortFrameCount();
    return 1;
}

int PortHdSpriteFind(const void* tiles, const void* frame) {
    int i;
    if (!tiles || (!gPortConfig.hdCards && !gPortConfig.hdTitle))
        return 0;
    for (i = 0; i < HD_SPRITE_COUNT; i++) {
        const HdSpriteDef* d = &kHdSprites[i];
        if (d->tiles != tiles || (d->frame && d->frame != frame) || !GroupOn(d->group))
            continue;
        return SpriteReady(i) ? i + 1 : 0;
    }
    return 0;
}

/* Which cells (GBA pixels of the sprite's box) of an HD picture are its dark, opaque inside: the reload card's black
   back, where the cartridge's gauge fill may show (port_ppu.c). Read once from the imported picture; NULL if there is
   none. A cell counts when almost all of it is opaque and dark, so the silver edge and the crown keep their art. */
static uint8_t* sCellMask[HD_SPRITE_COUNT];
static uint8_t sCellMaskTried[HD_SPRITE_COUNT];
const uint8_t* PortHdSpriteDarkCells(int id, int* x0, int* y0, int* w, int* h) {
    const HdSpriteDef* d;
    char file[160];
    int pw, ph, n, cx, cy;
    unsigned char* px;
    if (id < 1 || id > HD_SPRITE_COUNT)
        return NULL;
    d = &kHdSprites[id - 1];
    *x0 = d->x0; *y0 = d->y0; *w = d->w; *h = d->h;
    if (sCellMaskTried[id - 1])
        return sCellMask[id - 1];
    sCellMaskTried[id - 1] = 1;
    px = stbi_load(SpritePath(file, sizeof(file), d->name), &pw, &ph, &n, 4);
    if (!px)
        return NULL;
    sCellMask[id - 1] = (uint8_t*)SDL_calloc((size_t)d->w * d->h, 1);
    for (cy = 0; sCellMask[id - 1] && cy < d->h; cy++)
        for (cx = 0; cx < d->w; cx++) {
            int u0 = cx * pw / d->w, u1 = (cx + 1) * pw / d->w, v0 = cy * ph / d->h, v1 = (cy + 1) * ph / d->h, u, v;
            int total = 0, dark = 0;
            for (v = v0; v < v1; v++)
                for (u = u0; u < u1; u++) {
                    const unsigned char* q = px + ((size_t)v * pw + u) * 4;
                    total++;
                    dark += q[3] > 200 && q[0] < 70 && q[1] < 70 && q[2] < 70;
                }
            sCellMask[id - 1][cy * d->w + cx] = total && dark * 10 >= total * 9;
        }
    stbi_image_free(px);
    return sCellMask[id - 1];
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

/* a decode and upload, counted for OptiPort's stutter view while the frame log is on (gPortStutter) */
static int LoadCounted(int i) {
    uint64_t t0, spent;
    int ok;
    if (!gPortStutter.on)
        return Load(i);
    t0 = SDL_GetPerformanceCounter();
    ok = Load(i);
    spent = SDL_GetPerformanceCounter() - t0;
    gPortStutter.hdLoads++;
    gPortStutter.hdLoadTicks += spent;
    if (spent >= gPortStutter.worstLoadTicks) {
        gPortStutter.worstLoadTicks = spent;
        gPortStutter.worstLoad = kHdSprites[i].name;
    }
    return ok;
}

static int Load(int i) {
    char file[160];
    int w = 0, h = 0, n, k, decodedW, decodedH;
    unsigned char* px;
    void* data = NULL;
    size_t size = 0, need;
    SDL_Surface* s;
#ifdef PORT_GBA_RELOC
    const size_t budget = 12u * 1024 * 1024;
#else
    const size_t budget = 32u * 1024 * 1024;
#endif
    if (kHdSprites[i].group == 7) {
        data = TitleWordAsset(kHdSprites[i].name, &size);
        SDL_snprintf(file, sizeof(file), "%s", kHdSprites[i].name);
        if (!data || size > 0x7FFFFFFF || !stbi_info_from_memory(data, (int)size, &w, &h, &n))
            goto invalid;
    } else {
        uint64_t t0 = PORT_STUTTER_NOW();
        int ok = stbi_info(SpritePath(file, sizeof(file), kHdSprites[i].name), &w, &h, &n);
        PORT_STUTTER_ADD(hdHeaderTicks, t0);
        if (!ok)
            goto invalid;
    }
    /* Reject unsafe dimensions, old recipes and oversized texture pairs before decoding. */
    if (w < 1 || h < 1 || w > 4096 || h > 4096)
        goto invalid;
    {
        const HdSpriteDef* d = &kHdSprites[i];
        float want = (float)(d->padL + d->w + d->padR) / (d->padT + d->h + d->padB);
        float got = (float)w / h;
        if (got > want * 1.03f || got < want * 0.97f)
            goto invalid;
    }
    need = (size_t)w * h * 8;
    if (need > budget)
        goto invalid;
    /* Never evict this frame's textures or exceed the texture-pair budget. */
    while (sBytes > budget - need) {
        int victim = -1;
        for (k = 0; k < HD_SPRITE_COUNT; k++)
            if (sState[k].image && sState[k].lastUsed != PortFrameCount() &&
                (victim < 0 || sState[k].lastUsed < sState[victim].lastUsed))
                victim = k;
        if (victim < 0) {
            SDL_free(data);
            return 0; /* transient pressure: allow another attempt next frame */
        }
        Unload(victim);
    }
    {
        uint64_t t0 = PORT_STUTTER_NOW();
        px = kHdSprites[i].group == 7
            ? stbi_load_from_memory(data, (int)size, &decodedW, &decodedH, &n, 4)
            : stbi_load(file, &decodedW, &decodedH, &n, 4);
        PORT_STUTTER_ADD(hdDecodeTicks, t0);
    }
    SDL_free(data);
    data = NULL;
    /* A file may change between the header read and decode. Never upload a different size. */
    if (!px || decodedW != w || decodedH != h) {
        if (px) stbi_image_free(px);
        goto invalid;
    }
    /* Kingdom Key's one-cell seam correction:
       Kingdom Key's native white dot lies outside the HD window; cover just
       that GBA cell with its neighbouring native near-black.
       Cure's black and Simba's blue native top lines, every other illustration
       and all separately rendered value digits stay unchanged. */
    if (kHdSprites[i].group == 6 && w == 128 && h == 256) {
        if (!strcmp(kHdSprites[i].name, "gKingdomKeySmallCardHD")) {
            for (int y = 120; y < 128; y++)
                for (int x = 88; x < 96; x++) {
                    unsigned char* p = px + (y * w + x) * 4;
                    p[0] = p[1] = p[2] = 8; p[3] = 255;
                }
        }
    }
    if (kHdSprites[i].group == 7 && MenuLightName(kHdSprites[i].name) && MenuLightReady(sRenderer)) {
        /* Remove only this light's box, not its words, bar or underline.
           German Press Start has eight extra rows above the same orb. */
        const HdSpriteDef* d = &kHdSprites[i];
        int left = 3 * w / d->w, right = 14 * w / d->w;
        int top = SDL_max(0, d->h - 16) * h / d->h, bottom = top + 11 * h / d->h;
        for (int y = top; y < bottom; y++)
            for (int x = left; x < right; x++) px[(y * w + x) * 4 + 3] = 0;
    }
    sState[i].textureH = h;
    s = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4);
    if (s) {
        uint64_t t0 = PORT_STUTTER_NOW();
        sState[i].image = SDL_CreateTextureFromSurface(sRenderer, s);
        /* the white copy for the game's brightening fades */
        for (k = 0; k < w * h; k++) px[k * 4] = px[k * 4 + 1] = px[k * 4 + 2] = 255;
        sState[i].white = SDL_CreateTextureFromSurface(sRenderer, s);
        SDL_DestroySurface(s);
        PORT_STUTTER_ADD(hdUploadTicks, t0);
    }
    stbi_image_free(px);
    if (!sState[i].image || !sState[i].white) {
        Unload(i);
        sState[i].present = 1;
        return 0;
    }
    SDL_SetTextureBlendMode(sState[i].image, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(sState[i].white, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(sState[i].image, (SDL_ScaleMode)PortSmoothScaleMode()); /* LINEAR, but not on SDL's software renderer */
    SDL_SetTextureScaleMode(sState[i].white, (SDL_ScaleMode)PortSmoothScaleMode());
    sState[i].bytes = need;
    sBytes += need;
    return 1;
invalid:
    SDL_free(data);
    sState[i].present = 1;
    PortLog("HD sprites: %s rejected (header, dimensions, recipe, budget or decode)", file);
    return 0;
}

/* The HD title's illustration (assets/hd/title/art/sora.png: the CoM poster, a mod, port/assets/hd/title/art/LEEME.md), drawn
   on the game's picture under every other HD part, in place of the cartridge's title art (port_ppu.c TitleArtQueue). */
static SDL_Texture *sArt, *sArtWhite;
static int sArtTried, sArtW, sArtH;
static HdSpriteDraw sArtDraws[2];
static int sArtCount;

int PortHdTitleArtReady(float* aspect) {
    if (!sArtTried && sRenderer) {
        size_t size = 0;
        void* data = PortLoadAsset("assets/hd/title/art/sora.png", &size);
        int w = 0, h = 0, n = 0, k;
        unsigned char* px = data && size <= 0x7FFFFFFF ? stbi_load_from_memory(data, (int)size, &w, &h, &n, 4) : NULL;
        SDL_Surface* s = px ? SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4) : NULL;
        sArtTried = 1;
        if (s) {
            sArt = SDL_CreateTextureFromSurface(sRenderer, s);
            for (k = 0; k < w * h; k++) px[k * 4] = px[k * 4 + 1] = px[k * 4 + 2] = 255;
            sArtWhite = SDL_CreateTextureFromSurface(sRenderer, s);
            SDL_DestroySurface(s);
        }
        if (px) stbi_image_free(px);
        SDL_free(data);
        if (sArt && sArtWhite) {
            SDL_SetTextureBlendMode(sArt, SDL_BLENDMODE_BLEND);
            SDL_SetTextureBlendMode(sArtWhite, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(sArt, (SDL_ScaleMode)PortSmoothScaleMode());
            SDL_SetTextureScaleMode(sArtWhite, (SDL_ScaleMode)PortSmoothScaleMode());
            sArtW = w;
            sArtH = h;
        } else {
            if (sArt) SDL_DestroyTexture(sArt);
            if (sArtWhite) SDL_DestroyTexture(sArtWhite);
            sArt = sArtWhite = NULL;
            PortLog("HD title: assets/hd/title/art/sora.png missing, the cartridge's illustration stays");
        }
    }
    if (aspect && sArtH)
        *aspect = (float)sArtW / sArtH;
    return sArt != NULL;
}

/* Keep first-open decoding/upload outside a live menu frame. Only the small
   shared menu set is prepared; imported cards keep their bounded LRU cache. */
void PortHdMenuAssetsWarm(SDL_Renderer* renderer) {
    if (!gPortConfig.hdTitle || !renderer) return;
    sRenderer = renderer;
    PortHdTitleArtReady(NULL);
    MenuLightReady(renderer);
}

/* The illustration in a w x h box at x, y (the Options panel's picture): as tall as the box, its right side kept. */
int PortHdTitleArtDraw(SDL_Renderer* renderer, float x, float y, float w, float h) {
    float aw;
    SDL_FRect src, dst;
    sRenderer = renderer;
    if (w <= 0 || h <= 0 || !PortHdTitleArtReady(NULL))
        return 0;
    aw = h * sArtW / sArtH;
    src.y = 0; src.h = (float)sArtH;
    if (aw > w) { /* cut on the left */
        src.w = sArtW * w / aw; src.x = sArtW - src.w;
        dst = (SDL_FRect){ x, y, w, h };
    } else {
        src.x = 0; src.w = (float)sArtW;
        dst = (SDL_FRect){ x + w - aw, y, aw, h };
    }
    SDL_SetTextureColorMod(sArt, 255, 255, 255);
    SDL_SetTextureAlphaMod(sArt, 255);
    return SDL_RenderTexture(renderer, sArt, &src, &dst);
}

void PortHdTitleArtQueue(const float* x, const float* y, float u0, float v0, float u1, float v1, int alpha, int dark,
                         int white) {
    HdSpriteDraw* q;
    if (sArtCount == 2)
        return;
    q = &sArtDraws[sArtCount++];
    q->id = 0; /* not a table sprite: only its corners and fades are used */
    memcpy(q->x, x, sizeof(q->x));
    memcpy(q->y, y, sizeof(q->y));
    q->u0 = u0; q->v0 = v0;
    q->u1 = u1; q->v1 = v1;
    q->alpha = (uint8_t)SDL_clamp(alpha, 0, 255);
    q->dark = (uint8_t)SDL_clamp(dark, 0, 255);
    q->white = (uint8_t)SDL_clamp(white, 0, 255);
}

/* The title's press-start bar (BG0, a gradient the cartridge steps column by column): its rows as the game colours
   them (port_ppu.c TitleBarLine), drawn smooth at the output's resolution, colours interpolated between the columns'
   centres, over the illustration and under the HD words. */
#define BAR_RUNS 512
typedef struct BarRun {
    int16_t y, x0, n;
    uint32_t c[256];
} BarRun;
static BarRun sBar[BAR_RUNS];
static int sBarCount;

void PortHdTitleBarRow(int y, int x0, int n, const uint32_t* argb) {
    BarRun* r;
    if (sBarCount == BAR_RUNS || n < 1)
        return;
    r = &sBar[sBarCount++];
    r->y = (int16_t)y;
    r->x0 = (int16_t)x0;
    r->n = (int16_t)SDL_min(n, 256);
    memcpy(r->c, argb, (size_t)r->n * 4);
}

static void BarDraw(SDL_Renderer* renderer, float ox, float oy, float scale, float shade) {
    static SDL_Vertex v[(256 + 2) * 2];
    static int index[(256 + 1) * 6];
    int i, k;
    for (i = 0; i < sBarCount; i++) {
        const BarRun* r = &sBar[i];
        /* Match the presenter's nearest-sampled row boundaries. Fractional
           geometry edges otherwise leave a one-pixel slit before the native
           black border, especially at 1920x1080 (scale 6.75). */
        float y0 = SDL_ceilf(oy + r->y * scale - 0.5f);
        float y1 = SDL_ceilf(oy + (r->y + 1) * scale - 0.5f);
        int nv = 0, ni = 0;
        for (k = -1; k <= r->n; k++) {
            /* the run's two ends, then each column's centre */
            float x = k < 0 ? r->x0 : k == r->n ? r->x0 + r->n : r->x0 + k + 0.5f;
            uint32_t c = r->c[SDL_clamp(k, 0, r->n - 1)];
            SDL_FColor f = { ((c >> 16) & 255) / 255.0f * shade, ((c >> 8) & 255) / 255.0f * shade,
                             (c & 255) / 255.0f * shade, 1.0f };
            v[nv].position.x = ox + x * scale; v[nv].position.y = y0; v[nv].color = f; nv++;
            v[nv].position.x = ox + x * scale; v[nv].position.y = y1; v[nv].color = f; nv++;
        }
        for (k = 0; k + 3 < nv; k += 2) {
            index[ni++] = k; index[ni++] = k + 2; index[ni++] = k + 1;
            index[ni++] = k + 1; index[ni++] = k + 2; index[ni++] = k + 3;
        }
        SDL_RenderGeometry(renderer, NULL, v, nv, index, ni);
    }
}

void PortHdSpritesBegin(int layer) {
    if ((unsigned)layer < HD_LAYERS)
        sDrawCount[layer] = 0;
    if (layer == 0)
        sArtCount = sBarCount = 0;
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
    if (!SpriteReady(id - 1))
        return 0;
    {
        /* Match the global OBJ brighten applied by the world/HUD draw calls;
         * screen-menu layer2 only uses the part's palette fade. */
        unsigned bld = layer == 2 ? 0 : IO16(0x50);
        int globalWhite = (bld & 0x10) && ((bld >> 6) & 3) == 2 && (IO16(0x54) & 31);
        if ((white > 0 || globalWhite) && !SpriteTextureReady(sState[id - 1].white))
            return 0;
    }
    if (PortHdDrawLogged()) {
        static uint32_t sLoggedAt[HD_SPRITE_COUNT];
        if (sLoggedAt[id - 1] != PortFrameCount() + 1) {
            sLoggedAt[id - 1] = PortFrameCount() + 1;
            PortLog("hd draw %u %s layer %d", (unsigned)PortFrameCount(), d->name, layer);
        }
    }
    q = &sDraws[layer][sDrawCount[layer]++];
    q->id = id - 1;
    q->hudOwner = 0;
    memcpy(q->x, x, sizeof(q->x));
    memcpy(q->y, y, sizeof(q->y));
    {
        /* the picture is padL + w + padR wide: a part on the box's edge carries the overhang, along its own axis */
        float total = (float)(d->padL + d->w + d->padR);
        int left = d->padL && px == d->x0, right = d->padR && px + pw == d->x0 + d->w;
        float ux = (x[1] - x[0]) / pw, uy = (y[1] - y[0]) / pw;
        q->u0 = (float)(px - d->x0 + d->padL) / total;
        q->u1 = (float)(px + pw - d->x0 + d->padL) / total;
        if (left) {
            q->x[0] -= ux * d->padL; q->y[0] -= uy * d->padL;
            q->x[3] -= ux * d->padL; q->y[3] -= uy * d->padL;
            q->u0 = 0;
        }
        if (right) {
            q->x[1] += ux * d->padR; q->y[1] += uy * d->padR;
            q->x[2] += ux * d->padR; q->y[2] += uy * d->padR;
            q->u1 = 1;
        }
    }
    {
        float total = (float)(d->padT + d->h + d->padB);
        int top = d->padT && py == d->y0, bottom = d->padB && py + ph == d->y0 + d->h;
        float vx = (q->x[3] - q->x[0]) / ph, vy = (q->y[3] - q->y[0]) / ph; /* down the part, as drawn */
        q->v0 = (float)(py - d->y0 + d->padT) / total;
        q->v1 = (float)(py + ph - d->y0 + d->padT) / total;
        if (top) {
            q->x[0] -= vx * d->padT; q->y[0] -= vy * d->padT;
            q->x[1] -= vx * d->padT; q->y[1] -= vy * d->padT;
            q->v0 = 0;
        }
        if (bottom) {
            q->x[3] += vx * d->padB; q->y[3] += vy * d->padB;
            q->x[2] += vx * d->padB; q->y[2] += vy * d->padB;
            q->v1 = 1;
        }
    }
    q->alpha = (uint8_t)alpha;
    q->dark = (uint8_t)SDL_clamp(dark, 0, 255);
    q->white = (uint8_t)SDL_clamp(white, 0, 255);
    return 1;
}

void PortHdSpriteHudOwner(int slot) {
    HdSpriteDraw* q;
    static const char prefix[] = "gReloadCounterWhiteTiles_";
    if (slot < 0 || slot >= 128 || !sDrawCount[1]) return;
    q = &sDraws[1][sDrawCount[1] - 1];
    /* These nineteen frame-specific records are the decomp's gauge->gfx3, not the deck's REV_COUNT. */
    if (!SDL_strncmp(kHdSprites[q->id].name, prefix, sizeof(prefix) - 1)) q->hudOwner = slot + 1;
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
    if (kHdSprites[q->id].group == 5 && sState[q->id].textureH > 0 &&
        SDL_fabsf(scale - SDL_roundf(scale)) > 0.001f &&
        SDL_strstr(kHdSprites[q->id].name, "gWorldInspectBar") == kHdSprites[q->id].name) {
        /* Match the cartridge band's pixel-centre convention. At fractional
         * scales the exact texel-boundary sample otherwise removes a row from
         * both white strokes where the HD header meets its native margin. */
        float halfTexel = 0.5f / sState[q->id].textureH;
        for (k = 0; k < 4; k++)
            v[k].tex_coord.y = SDL_clamp(v[k].tex_coord.y - halfTexel, 0.0f, 1.0f);
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

static int MenuLightPaint(SDL_Renderer* renderer, const float* x, const float* y,
                          const float* u, const float* v, uint64_t ticks, float shade, float white, float alpha) {
    static const int indices[6] = {0,1,2,0,2,3};
    SDL_Vertex vertices[4];
    int a, b, k, phase = (int)(ticks % 64);
    float blend;
    if (!MenuLightReady(renderer)) return 0;
    if (phase != sMenuLightPhase) {
        PortMenuLightFrames(ticks, &a, &b, &blend);
        for (k = 0; k < MENU_LIGHT_BYTES; k++) {
            int first = sMenuLightSamples[a * MENU_LIGHT_BYTES + k];
            int next = sMenuLightSamples[b * MENU_LIGHT_BYTES + k];
            sMenuLightPixels[k] = (unsigned char)(first + (next - first) * blend + 0.5f);
        }
        SDL_UpdateTexture(sMenuLightImage, NULL, sMenuLightPixels, MENU_LIGHT_SIZE * 4);
        sMenuLightPhase = phase;
    }
    for (k = 0; k < 4; k++) {
        vertices[k].position = (SDL_FPoint){x[k],y[k]};
        vertices[k].tex_coord = (SDL_FPoint){u[k],v[k]};
        vertices[k].color = (SDL_FColor){shade,shade,shade,alpha};
    }
    /* Keep SDL's software renderer on its clipped geometry path, as Quad. */
    if (x[0] == x[1] || y[0] == y[1]) {
        vertices[1].position.x += 0.01f;
        vertices[1].position.y += 0.01f;
    }
    SDL_RenderGeometry(renderer, sMenuLightImage, vertices, 4, indices, 6);
    if (white > 0) {
        for (k = 0; k < 4; k++) vertices[k].color = (SDL_FColor){1,1,1,alpha * white};
        SDL_RenderGeometry(renderer, sMenuLightWhite, vertices, 4, indices, 6);
    }
    return 1;
}

int PortHdMenuLightDraw(SDL_Renderer* renderer, float x, float y, float size, uint64_t ticks, float shade, float white) {
    float xx[4] = {x,x+size,x+size,x}, yy[4] = {y,y,y+size,y+size};
    static const float u[4] = {0,1,1,0}, v[4] = {0,0,1,1};
    return gPortConfig.hdTitle && size > 0 && MenuLightPaint(renderer, xx, yy, u, v, ticks, shade, white, 1);
}

static void SpriteMenuLight(SDL_Renderer* renderer, const HdSpriteDraw* q, float ox, float oy,
                            float scale, float shade, float white) {
    extern uint64_t PortUiMenuLightTicks(void);
    const HdSpriteDef* d = &kHdSprites[q->id];
    float top = (float)SDL_max(0, d->h - 16);
    float left = SDL_max(q->u0 * d->w, 3.0f), right = SDL_min(q->u1 * d->w, 14.0f);
    float y0 = SDL_max(q->v0 * d->h, top), y1 = SDL_min(q->v1 * d->h, top + 11);
    float xx[4], yy[4], u[4], v[4];
    if (right <= left || y1 <= y0 || q->u1 <= q->u0 || q->v1 <= q->v0) return;
    for (int k = 0; k < 4; k++) {
        float sx = k == 0 || k == 3 ? left : right, sy = k < 2 ? y0 : y1;
        float a = (sx / d->w - q->u0) / (q->u1 - q->u0);
        float b = (sy / d->h - q->v0) / (q->v1 - q->v0);
        xx[k] = ox + ((1-b)*((1-a)*q->x[0]+a*q->x[1]) + b*((1-a)*q->x[3]+a*q->x[2])) * scale;
        yy[k] = oy + ((1-b)*((1-a)*q->y[0]+a*q->y[1]) + b*((1-a)*q->y[3]+a*q->y[2])) * scale;
        u[k] = (sx - 3) / 11; v[k] = (sy - top) / 11;
    }
    MenuLightPaint(renderer, xx, yy, u, v, PortUiMenuLightTicks(), shade, white, q->alpha / 255.0f);
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
        float d = SDL_sqrtf(dx * dx + dy * dy) - RING_HOLE;
        /* the baked face: the game's own (HD) face shows there; a soft edge of a picture pixel and a half, not a
           cut that left the green's inner rim jagged (art review, 24/10) */
        if (d < 1.5f)
            px[k * 4 + 3] = d <= 0.0f ? 0 : (unsigned char)(px[k * 4 + 3] * d / 1.5f);
    }
    dark = SDL_malloc((size_t)w * h * 4);
    if (dark) {
        SDL_memcpy(dark, px, (size_t)w * h * 4);
        for (k = 0; k < w * h; k++) {
            unsigned char* c = dark + k * 4;
            /* the green: the empty track's dark grey; its antialiased rim too (yellow-green where it meets the black
               outline), which a stricter test left as a bright line along the grey (user report 27/10) */
            if (c[1] >= c[0] && c[1] > c[2] + 8)
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
    /* drawn smaller than the picture: filtered, or its round edges show steps */
    if (sRingFull) SDL_SetTextureScaleMode(sRingFull, (SDL_ScaleMode)PortSmoothScaleMode());
    if (sRingEmpty) SDL_SetTextureScaleMode(sRingEmpty, (SDL_ScaleMode)PortSmoothScaleMode());
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

/* GBA pixels the battle EXP line (btl_exp.c) goes down when the HD HP ring is in use: the ring reaches lower than the
   cartridge's gauge and covered the line's start ("SIG. NIVEL", user report 27/10). */
int PortHdHudExpDrop(void) {
    return gPortConfig.hdCards && sRingFull ? 7 : 0;
}

/* GBA x of the opponent's sleight name (card_friend_card.c draws it at 120, top centre). On the wide HUD canvas the
   opponent's stack docks right while the name stays centred, so a long name ran under the stack's total and first
   card (Axel's "Muro de fuego", user report 27/10). With the HD cards the name ends 2 pixels before the stack: its
   width is its sprite frame's (count, then attr0/attr1/attr2 per piece); the stack starts at its total's x
   (card_battle_riku.c: 184/172/160 for 1/2/3 cards), docked by the canvas' whole margin where the name gets half.
   GBA modes, and a canvas no wider than the cartridge, keep the cartridge's place. */
int PortHdHudStockNameX(int x, const uint16_t* frame, int stockCount) {
    static const uint8_t width[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } };
    int i, right = 0, stack, half = (gPortHudW - GBA_SCREEN_W) / 2;
    if (!gPortConfig.hdCards || !frame || stockCount < 1 || stockCount > 3 || half <= 0)
        return x;
    for (i = 0; i < frame[0]; i++) {
        const uint16_t* p = frame + 1 + i * 3;
        int shape = p[0] >> 14, px = (int16_t)(p[1] << 7) >> 7;
        if (shape < 3 && px + width[shape][p[1] >> 14] > right)
            right = px + width[shape][p[1] >> 14];
    }
    stack = 196 - 12 * stockCount + half;
    return x + right + 2 > stack ? stack - 2 - right : x;
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
    for (i = 0; layer == 0 && i < sArtCount && sArt; i++) { /* the title's illustration, under everything */
        const HdSpriteDraw* q = &sArtDraws[i];
        float shade = 1.0f - SDL_max(darken > 0 ? darken / 16.0f : 0.0f, q->dark / 255.0f);
        float white = SDL_max(brighten > 0 ? brighten / 16.0f : 0.0f, q->white / 255.0f);
        Quad(sArt, q, ox, oy, scale, (SDL_FColor){ shade, shade, shade, q->alpha / 255.0f });
        if (white > 0)
            Quad(sArtWhite, q, ox, oy, scale, (SDL_FColor){ 1, 1, 1, q->alpha / 255.0f * white });
    }
    if (layer == 0 && sBarCount)
        BarDraw(renderer, ox, oy, scale, 1.0f - (darken > 0 ? darken / 16.0f : 0.0f));
    for (i = 0; i < sDrawCount[layer]; i++) {
        const HdSpriteDraw* q = &sDraws[layer][i];
        HdSpriteState* st = &sState[q->id];
        if (q->hudOwner) continue;
        /* the game's brightness fade (BLDY) and the part's palette fade, whichever is further */
        float shade = 1.0f - SDL_max(darken > 0 ? darken / 16.0f : 0.0f, q->dark / 255.0f);
        float white = SDL_max(brighten > 0 ? brighten / 16.0f : 0.0f, q->white / 255.0f);
        if (!st->image && !LoadCounted(q->id))
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
        if (kHdSprites[q->id].group == 7 && MenuLightName(kHdSprites[q->id].name))
            SpriteMenuLight(renderer, q, ox, oy, scale, shade, white);
        if (q->u0 <= 0.0f && SDL_strstr(kHdSprites[q->id].name, "PremireChanceTitle")) {
            /* Bonus Elite's banner starts at the cartridge screen's left edge, over a black band (the other menus'
               banners sit on a band of their own colour that the wide view carries on): its first column goes on to
               the window's edge, so the banner is not cut short (user, 24/10) */
            HdSpriteDraw edge = *q;
            float left = -ox / scale;
            if (left < q->x[0] - 0.5f) {
                edge.x[1] = q->x[0]; edge.x[2] = q->x[3];
                edge.x[0] = edge.x[3] = left;
                edge.y[1] = q->y[0]; edge.y[2] = q->y[3];
                edge.u1 = edge.u0 + 0.25f / kHdSprites[q->id].w; /* a quarter of a cartridge pixel */
                Quad(st->image, &edge, ox, oy, scale, (SDL_FColor){ shade, shade, shade, q->alpha / 255.0f });
                if (white > 0)
                    Quad(st->white, &edge, ox, oy, scale, (SDL_FColor){ 1, 1, 1, q->alpha / 255.0f * white });
            }
        }
    }
}

/* The original draws base -> affine fill/full cap -> counter. The fill remains the game's own tiles,
   palette, matrix and animation; drawing every HD part before the native overlay hid the counter under it. */
void PortHdSpritesHudFront(SDL_Renderer* renderer, float ox, float oy, float scale, int darken, int brighten) {
    int i;
    sRenderer = renderer;
    if (scale <= 0) return;
    for (i = 0; i < sDrawCount[1]; i++) {
        const HdSpriteDraw* q = &sDraws[1][i];
        HdSpriteState* st = &sState[q->id];
        int k, x, y, left, top, right, bottom;
        float l = q->x[0], r = l, t = q->y[0], b = t;
        float shade, white;
        if (!q->hudOwner || (!st->image && !LoadCounted(q->id))) continue;
        st->lastUsed = PortFrameCount();
        for (k = 1; k < 4; k++) {
            l = SDL_min(l, q->x[k]); r = SDL_max(r, q->x[k]);
            t = SDL_min(t, q->y[k]); b = SDL_max(b, q->y[k]);
        }
        left = SDL_max(0, (int)SDL_floorf(l)); right = SDL_min(gPortHudW, (int)SDL_ceilf(r));
        top = SDL_max(0, (int)SDL_floorf(t)); bottom = SDL_min(gPortHudH, (int)SDL_ceilf(b));
        shade = 1.0f - SDL_max(darken > 0 ? darken / 16.0f : 0.0f, q->dark / 255.0f);
        white = SDL_max(brighten > 0 ? brighten / 16.0f : 0.0f, q->white / 255.0f);
        for (y = top; y < bottom; y++) {
            x = left;
            while (x < right) {
                int start;
                SDL_Rect clip;
                while (x < right && !PortPpuHudSpriteVisible(q->hudOwner - 1, x, y)) x++;
                start = x;
                while (x < right && PortPpuHudSpriteVisible(q->hudOwner - 1, x, y)) x++;
                if (x == start) continue;
                clip = (SDL_Rect){(int)(ox + start * scale), (int)(oy + y * scale),
                                  (int)((x - start) * scale), (int)scale};
                SDL_SetRenderClipRect(renderer, &clip);
                Quad(st->image, q, ox, oy, scale, (SDL_FColor){shade, shade, shade, q->alpha / 255.0f});
                if (white > 0)
                    Quad(st->white, q, ox, oy, scale, (SDL_FColor){1, 1, 1, q->alpha / 255.0f * white});
            }
        }
    }
    SDL_SetRenderClipRect(renderer, NULL);
}

/* How far a palette is faded to black (*dark) or white (*white), 0..255, from its original: the game's fades
   (engine.c FadeUpdate) scale it (c = o (1 - f)), take a step off every channel (FADE_MODE_BLACK: c = o - a), add one
   (FADE_MODE_ADD_WHITE: c = o + a) or raise every channel to one (FADE_MODE_WHITE: c = max(o, a)); any of them is
   recognised, and its share of the way (the channels' sum) is what the HD pictures follow. */
static int FadeFits(const uint16_t* original, const volatile uint16_t* current, int count, int kind, int f, int a) {
    int i, k, err = 0;
    for (i = 1; i < count; i++)
        for (k = 0; k < 15; k += 5) {
            int o = (original[i] >> k) & 31, c = (current[i] >> k) & 31, e;
            switch (kind) {
            case 0: e = o * (256 - f) / 256; break;           /* scaled to black */
            case 1: e = SDL_max(0, o - a); break;              /* a step off */
            case 2: e = o + (31 - o) * f / 256; break;         /* scaled to white */
            case 3: e = SDL_min(31, o + a); break;             /* a step on */
            default: e = SDL_max(o, a); break;                 /* raised to a */
            }
            err += SDL_abs(c - e);
        }
    return err <= 2 * 3 * (count - 1);
}

void PortPaletteFade(const uint16_t* original, const volatile uint16_t* current, int count, int* dark, int* white) {
    int i, so = 0, sc = 0, room = 0, k, step = 0, top = 0;
    *dark = *white = 0;
    if (!original || count < 2)
        return;
    for (i = 1; i < count; i++) /* entry 0 is transparent */
        for (k = 0; k < 15; k += 5) {
            int o = (original[i] >> k) & 31, c = (current[i] >> k) & 31;
            so += o;
            sc += c;
            room += 31 - o;
            if (c > 0 && o - c > step) step = o - c; /* the step a black fade took (where it left something) */
            if (c - o > top) top = c - o;            /* ... a white one added */
        }
    if (sc < so && so > 0) {
        int f = 256 * (so - sc) / so; /* c = o (1 - f) */
        if (step == 0) step = 31;     /* everything at black */
        if (f > 6 && (FadeFits(original, current, count, 0, f, 0) || FadeFits(original, current, count, 1, 0, step)))
            *dark = SDL_min(f, 255);
    } else if (sc > so && room > 0) {
        int f = 256 * (sc - so) / room, a; /* c = o + (31 - o) f */
        a = 31; /* FADE_MODE_WHITE's level: the lowest channel left */
        for (i = 1; i < count; i++)
            for (k = 0; k < 15; k += 5)
                a = SDL_min(a, (current[i] >> k) & 31);
        if (f > 6 && (FadeFits(original, current, count, 2, f, 0) || FadeFits(original, current, count, 3, 0, top) ||
                      FadeFits(original, current, count, 4, 0, a)))
            *white = SDL_min(f, 255);
    }
}

/* Renderer gone (or the HD files re-imported): drop the textures and look the files up again. */
void PortHdSpritesShutdown(void) {
    int i;
    MenuLightRelease();
    if (sRingFull) SDL_DestroyTexture(sRingFull);
    if (sRingEmpty) SDL_DestroyTexture(sRingEmpty);
    sRingFull = sRingEmpty = NULL;
    sRingTried = 0;
    for (i = 0; i < HD_SPRITE_COUNT; i++) {
        Unload(i);
        sState[i].present = 0;
    }
    if (sArt) SDL_DestroyTexture(sArt);
    if (sArtWhite) SDL_DestroyTexture(sArtWhite);
    sArt = sArtWhite = NULL;
    sArtTried = sArtW = sArtH = sArtCount = sBarCount = 0;
    SDL_memset(sDrawCount, 0, sizeof(sDrawCount));
    sRenderer = NULL;
}
