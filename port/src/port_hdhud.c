/* Experimental image-based HUD pilot. Decode once, draw cached GPU texture.
 * Enemy HP values and interpolation remain owned by the original game task.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include "port/port.h"
#include "port/port_ui.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "../decompui/third_party/stb_image.h"

static SDL_Texture* sFrame;
static SDL_Texture* sWhiteFrame;
static SDL_Texture* sLabel;
static SDL_Texture* sWhiteLabel;
static SDL_Texture *sPlayerFrame, *sWhitePlayerFrame;
static SDL_FRect sPlayerSource;
static SDL_Renderer* sAssetRenderer;
typedef struct HudAsset {
    uint64_t hash;
    SDL_Texture *image, *white;
    SDL_FRect source;
    uint32_t lastUsed;
    size_t bytes;
    int available;
} HudAsset;
typedef struct HudAssetDraw { int asset, slot, x, y, w, h, flip, alpha; } HudAssetDraw;
static HudAsset* sAssets;
static HudAssetDraw* sAssetDraws;
static int sAssetCount, sAssetDrawCount;
static size_t sAssetBytes;

void PortHdHudSpriteBegin(void) { sAssetDrawCount = 0; }

/* Content-keyed pieces let the preliminary pack expand beyond hand-tagged
   tasks. Missing pieces remain native, and misses never hit disk every frame. */
static void LoadHudAsset(HudAsset* asset) {
    char file[160];
    int w, h, channels, x, y, l, t, r = -1, b = -1;
    unsigned char* pixels;
    SDL_Surface *src, *dst;
    SDL_ScaleMode mode;
    asset->available = 0;
    SDL_snprintf(file, sizeof(file), "assets/hd-hud/sprites/%016llx.png", (unsigned long long)asset->hash);
    if (!stbi_info(PortConfigPath(file), &w, &h, &channels) || channels != 4 ||
        w < 1 || h < 1 || w > 4096 || h > 4096 || (size_t)w * h > 4194304) return;
    pixels = stbi_load(PortConfigPath(file), &w, &h, &channels, 4);
    if (!pixels) return;
    l = w; t = h;
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        unsigned char* p = pixels + ((size_t)y * w + x) * 4;
        /* Generated PNG transport can leave 1/255 alpha noise in padding.
           Normalize that quantization noise before validation and upload;
           preserve all meaningful antialiasing and authored colours. */
        if (p[3] <= 1) p[3] = 0;
        if (p[3] && (!x || !y || x == w-1 || y == h-1)) { stbi_image_free(pixels); return; }
        if (p[3] >= 32) { l = SDL_min(l, x); t = SDL_min(t, y); r = SDL_max(r, x); b = SDL_max(b, y); }
        if (!p[3]) p[0] = p[1] = p[2] = 0;
    }
    if (r < l) { stbi_image_free(pixels); return; }
    src = SDL_CreateSurfaceFrom(r-l+1, b-t+1, SDL_PIXELFORMAT_RGBA32, pixels+((size_t)t*w+l)*4, w*4);
    if (src) {
        int longest = SDL_max(src->w, src->h);
        int cw = SDL_max(1, src->w * SDL_min(longest, 256) / longest);
        int ch = SDL_max(1, src->h * SDL_min(longest, 256) / longest);
        size_t need = (size_t)cw * ch * 8;
        /* Keep the active HUD pieces, evict old textures instead of falling
           back forever after the player has seen sixteen different cards. */
        while (sAssetBytes + need > 8*1024*1024) {
            int k, victim = -1;
            for (k=0; k<sAssetCount; k++) {
                if (!sAssets[k].image || sAssets[k].lastUsed==PortFrameCount()) continue;
                if (victim<0 || sAssets[k].lastUsed<sAssets[victim].lastUsed) victim=k;
            }
            if (victim<0) break;
            SDL_DestroyTexture(sAssets[victim].image); SDL_DestroyTexture(sAssets[victim].white);
            sAssets[victim].image=sAssets[victim].white=NULL;
            sAssetBytes-=sAssets[victim].bytes; sAssets[victim].bytes=0;
        }
        dst = sAssetBytes + (size_t)cw * ch * 8 <= 8*1024*1024 ? SDL_CreateSurface(cw, ch, SDL_PIXELFORMAT_RGBA32) : NULL;
        SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);
        if (dst && SDL_BlitSurfaceScaled(src, NULL, dst, NULL, SDL_SCALEMODE_LINEAR)) {
            asset->image = SDL_CreateTextureFromSurface(sAssetRenderer, dst);
            for (y = 0; y < ch; y++) for (x = 0; x < cw; x++) {
                unsigned char* p = (unsigned char*)dst->pixels + y*dst->pitch+x*4;
                p[0] = p[1] = p[2] = 255;
            }
            asset->white = SDL_CreateTextureFromSurface(sAssetRenderer, dst);
            asset->source = (SDL_FRect){0, 0, cw, ch};
            if (asset->image && asset->white) { asset->bytes=need; sAssetBytes+=need; asset->available=1; }
        }
        if (dst) SDL_DestroySurface(dst);
        SDL_DestroySurface(src);
    }
    stbi_image_free(pixels);
    if (!asset->image || !asset->white) {
        if (asset->image) SDL_DestroyTexture(asset->image);
        if (asset->white) SDL_DestroyTexture(asset->white);
        asset->image = asset->white = NULL; return;
    }
    mode = SDL_strcmp(SDL_GetRendererName(sAssetRenderer), "software") == 0 ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR;
    SDL_SetTextureBlendMode(asset->image, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(asset->white, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(asset->image, mode);
    SDL_SetTextureScaleMode(asset->white, mode);
    PortLog("HD HUD: cached sprite %016llx", (unsigned long long)asset->hash);
}

int PortHdHudSpriteRegister(const uint32_t* pixels, int w, int h, int slot, int x, int y, int flip, int alpha) {
    uint64_t hash = 14695981039346656037ULL;
    const char* layout;
    int i, k, l=w, t=h, r=-1, b=-1;
    if (!gPortConfig.hdHud || !sAssetRenderer || !gPortHudScale || PortUiActive() || !pixels) return 0;
    hash ^= (unsigned)w; hash *= 1099511628211ULL;
    hash ^= (unsigned)h; hash *= 1099511628211ULL;
    for (i=0; i<w*h; i++) {
        hash ^= pixels[i]; hash *= 1099511628211ULL;
        if (pixels[i] >> 24) { l=SDL_min(l,i%w); r=SDL_max(r,i%w); t=SDL_min(t,i/w); b=SDL_max(b,i/w); }
    }
    if (r<l) return 0;
    PORT_ENV_ONCE(layout, SDL_getenv, "KHCOM_HUD_LAYOUT");
    if (layout && (uint32_t)SDL_atoi(layout)==PortFrameCount())
        PortLog("hud-asset: frame=%u slot=%d hash=%016llx",PortFrameCount(),slot,(unsigned long long)hash);
    if (!sAssets) sAssets = SDL_calloc(512, sizeof(*sAssets));
    if (!sAssetDraws) sAssetDraws = SDL_calloc(128, sizeof(*sAssetDraws));
    if (!sAssets || !sAssetDraws || sAssetDrawCount == 128) return 0;
    for (k=0; k<sAssetCount && sAssets[k].hash != hash; k++) {}
    if (k==sAssetCount) {
        if (sAssetCount==512) return 0;
        sAssets[k].hash=hash; sAssetCount++; LoadHudAsset(&sAssets[k]);
    }
    if (!sAssets[k].image && sAssets[k].available) LoadHudAsset(&sAssets[k]);
    if (!sAssets[k].image) return 0;
    sAssets[k].lastUsed=PortFrameCount();
    sAssetDraws[sAssetDrawCount++] = (HudAssetDraw){k,slot,x+((flip&1)?w-1-r:l),y+((flip&2)?h-1-b:t),r-l+1,b-t+1,flip,alpha};
    return 1;
}

void PortHdHudDrawSprites(SDL_Renderer* renderer, int width, int height) {
    int i, y, x;
    float scale=(float)gPortHudScale, ox=(width-gPortHudW*scale)*0.5f, oy=(height-gPortHudH*scale)*0.5f;
    uint16_t bld=*(volatile uint16_t*)(uintptr_t)(GBA_IO+0x50);
    int effect=(bld>>6)&3, amount=SDL_min(*(volatile uint16_t*)(uintptr_t)(GBA_IO+0x54),16);
    Uint8 tint=(bld&0x10)&&effect==3 ? 255*(16-amount)/16 : 255;
    if (!gPortConfig.hdHud || !scale || PortUiActive()) return;
    for (i=0; i<sAssetDrawCount; i++) {
        HudAssetDraw* d=&sAssetDraws[i]; HudAsset* a=&sAssets[d->asset];
        SDL_FlipMode flip=(SDL_FlipMode)(((d->flip&1)?SDL_FLIP_HORIZONTAL:0)|((d->flip&2)?SDL_FLIP_VERTICAL:0));
        SDL_SetTextureColorMod(a->image,tint,tint,tint);
        SDL_SetTextureAlphaMod(a->image,d->alpha);
        SDL_SetTextureAlphaMod(a->white,amount*d->alpha/16);
        for (y=0; y<d->h; y++) {
            x=0;
            while (x<d->w) {
                int start;
                SDL_FRect src,dst;
                while (x<d->w && !PortPpuHudSpriteVisible(d->slot,d->x+x,d->y+y)) x++;
                start=x;
                while (x<d->w && PortPpuHudSpriteVisible(d->slot,d->x+x,d->y+y)) x++;
                if (x==start) continue;
                src=(SDL_FRect){((d->flip&1)?d->w-x:start)*a->source.w/d->w,
                    ((d->flip&2)?d->h-1-y:y)*a->source.h/d->h,(x-start)*a->source.w/d->w,a->source.h/d->h};
                dst=(SDL_FRect){ox+(d->x+start)*scale,oy+(d->y+y)*scale,(x-start)*scale,scale};
                SDL_RenderTextureRotated(renderer,a->image,&src,&dst,0,NULL,flip);
                if ((bld&0x10)&&effect==2&&amount)
                    SDL_RenderTextureRotated(renderer,a->white,&src,&dst,0,NULL,flip);
            }
        }
    }
}
int PortHdHudPlayerFrameReady(void) {
    /* Fitting probe only: keep native frame until interior-row alignment and
       full-health end caps are verified against the original. */
    const char* probe;
    PORT_ENV_ONCE(probe, SDL_getenv, "KHCOM_HDHUD_FRAME_PROBE");
    return probe && gPortConfig.hdHud &&
           sPlayerFrame && sWhitePlayerFrame && gPortHudScale && !PortUiActive();
}
static void LoadPlayerFrame(SDL_Renderer* renderer) {
    const char* path = PortConfigPath("assets/hd-hud/pilot/player-double-frame-v1.png");
    int w, h, channels, x, y, l, t, r = -1, b = -1;
    unsigned char* pixels;
    SDL_Surface *src, *dst;
    SDL_ScaleMode sampling;
    if (!stbi_info(path, &w, &h, &channels) || channels != 4 || w < 1 || h < 1 ||
        w > 4096 || h > 4096 || (size_t)w * h > 4194304) return;
    pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) return;
    l = w; t = h;
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        unsigned char* p = pixels + ((size_t)y * w + x) * 4;
        if (p[3] && (!x || !y || x == w - 1 || y == h - 1)) { stbi_image_free(pixels); return; }
        if (p[3] >= 32) { l = SDL_min(l, x); r = SDL_max(r, x); t = SDL_min(t, y); b = SDL_max(b, y); }
        if (!p[3]) p[0] = p[1] = p[2] = 0;
    }
    if (r < l) { stbi_image_free(pixels); return; }
    src = SDL_CreateSurfaceFrom(r - l + 1, b - t + 1, SDL_PIXELFORMAT_RGBA32,
                                pixels + ((size_t)t * w + l) * 4, w * 4);
    if (src) {
        int cw = SDL_min(src->w, 512), ch = SDL_max(1, src->h * cw / src->w);
        dst = SDL_CreateSurface(cw, ch, SDL_PIXELFORMAT_RGBA32);
        SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);
        if (dst && SDL_BlitSurfaceScaled(src, NULL, dst, NULL, SDL_SCALEMODE_LINEAR)) {
            sPlayerFrame = SDL_CreateTextureFromSurface(renderer, dst);
            for (y = 0; y < ch; y++) for (x = 0; x < cw; x++) {
                unsigned char* p = (unsigned char*)dst->pixels + y * dst->pitch + x * 4;
                p[0] = p[1] = p[2] = 255;
            }
            sWhitePlayerFrame = SDL_CreateTextureFromSurface(renderer, dst);
            sPlayerSource = (SDL_FRect){0, 0, cw, ch};
        }
        if (dst) SDL_DestroySurface(dst);
        SDL_DestroySurface(src);
    }
    stbi_image_free(pixels);
    sampling = SDL_strcmp(SDL_GetRendererName(renderer), "software") == 0 ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR;
    if (sPlayerFrame) { SDL_SetTextureBlendMode(sPlayerFrame, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(sPlayerFrame, sampling); }
    if (sWhitePlayerFrame) { SDL_SetTextureBlendMode(sWhitePlayerFrame, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(sWhitePlayerFrame, sampling); }
}
static SDL_FRect sLabelSource;
static SDL_FRect sLabelInk;
#ifdef VERSION_EU
extern uint32_t gLanguage;
#endif
static SDL_FRect sSource;
static int sAttempted;
static uint32_t* sExportPixels;
static uint64_t* sExportHashes;
static int sExportCount;
static int sLabelLanguage = -1;

static int HudLanguage(void) {
#ifdef VERSION_EU
    return (int)gLanguage;
#else
    return 0;
#endif
}

int PortHdHudLettersReady(void) { return sLabel != NULL; }

static void LoadLabel(SDL_Renderer* renderer) {
    const char* file = "assets/hd-hud/pilot/label-hp-v2.png";
    int w, h, channels, x, y, left, top, right = -1, bottom = -1;
    int inkLeft, inkTop, inkRight = -1, inkBottom = -1;
    unsigned char* pixels;
    SDL_Surface *source, *compact;
    sLabelLanguage = HudLanguage();
#ifdef VERSION_EU
    if (gLanguage == 4) file = "assets/hd-hud/pilot/label-vt-v2.png";
    else if (gLanguage == 2) file = "assets/hd-hud/pilot/label-lp-v1.png";
    else if (gLanguage == 3) file = "assets/hd-hud/pilot/label-pv-v3.png";
#endif
    if (!stbi_info(PortConfigPath(file), &w, &h, &channels) || channels != 4 ||
        w < 1 || h < 1 || w > 4096 || h > 4096 || (size_t)w * h > 4194304) return;
    pixels = stbi_load(PortConfigPath(file), &w, &h, &channels, 4);
    if (!pixels) return;
    left = w; top = h;
    inkLeft = w; inkTop = h;
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        unsigned char* p = pixels + ((size_t)y * w + x) * 4;
        int shade = SDL_max(p[0], SDL_max(p[1], p[2]));
        if (p[3] && (!x || !y || x == w - 1 || y == h - 1)) { stbi_image_free(pixels); return; }
        if (p[3] >= 32) {
            left = SDL_min(left, x); right = SDL_max(right, x);
            top = SDL_min(top, y); bottom = SDL_max(bottom, y);
        }
        if (p[3] >= 128 && shade >= 128) {
            inkLeft = SDL_min(inkLeft, x); inkRight = SDL_max(inkRight, x);
            inkTop = SDL_min(inkTop, y); inkBottom = SDL_max(inkBottom, y);
        }
        if (!p[3]) p[0] = p[1] = p[2] = 0;
    }
    if (right < left || inkRight < inkLeft) { stbi_image_free(pixels); return; }
    source = SDL_CreateSurfaceFrom(right - left + 1, bottom - top + 1, SDL_PIXELFORMAT_RGBA32,
                                  pixels + ((size_t)top * w + left) * 4, w * 4);
    if (source) {
        int cw = SDL_min(source->w, 256), ch = SDL_max(1, source->h * cw / source->w);
        compact = SDL_CreateSurface(cw, ch, SDL_PIXELFORMAT_RGBA32);
        SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_NONE);
        if (compact && SDL_BlitSurfaceScaled(source, NULL, compact, NULL, SDL_SCALEMODE_LINEAR)) {
            sLabel = SDL_CreateTextureFromSurface(renderer, compact);
            for (y = 0; y < ch; y++) for (x = 0; x < cw; x++) {
                unsigned char* p = (unsigned char*)compact->pixels + y * compact->pitch + x * 4;
                p[0] = p[1] = p[2] = 255;
            }
            sWhiteLabel = SDL_CreateTextureFromSurface(renderer, compact);
            sLabelSource = (SDL_FRect){0, 0, cw, ch};
            sLabelInk = (SDL_FRect){(inkLeft - left) * cw / (float)source->w,
                (inkTop - top) * ch / (float)source->h, (inkRight - inkLeft + 1) * cw / (float)source->w,
                (inkBottom - inkTop + 1) * ch / (float)source->h};
        }
        if (compact) SDL_DestroySurface(compact);
        SDL_DestroySurface(source);
    }
    stbi_image_free(pixels);
    if (sLabel) {
        SDL_SetTextureBlendMode(sLabel, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(sLabel, SDL_strcmp(SDL_GetRendererName(renderer), "software") == 0 ?
                                SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
        PortLog("HD HUD: cached label %s", file);
    }
    if (sWhiteLabel) {
        SDL_SetTextureBlendMode(sWhiteLabel, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(sWhiteLabel, SDL_strcmp(SDL_GetRendererName(renderer), "software") == 0 ?
                                SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
    }
}

/* Developer-only ROM-local references; never ship these extracted sprites. */
static int HudExport(void) {
    const char* v;
    PORT_ENV_ONCE(v, SDL_getenv, "KHCOM_HUD_EXPORT");
    return v != NULL;
}

uint32_t* PortHdHudExportBuffer(void) {
    if (!HudExport() && !(gPortConfig.hdHud && gPortHudScale)) return NULL;
    if (!sExportPixels) sExportPixels = SDL_calloc(64 * 64, sizeof(uint32_t));
    return sExportPixels;
}

void PortHdHudExportSprite(int w, int h, int part, int ax, int ay) {
    uint64_t hash = 14695981039346656037ULL;
    SDL_Surface* surface;
    char name[160];
    int i;
    FILE* f;
    if (!sExportPixels || !HudExport()) return;
    hash ^= (unsigned)w; hash *= 1099511628211ULL;
    hash ^= (unsigned)h; hash *= 1099511628211ULL;
    for (i = 0; i < w * h; i++) { hash ^= sExportPixels[i]; hash *= 1099511628211ULL; }
    if (!sExportHashes) sExportHashes = SDL_calloc(2048, sizeof(uint64_t));
    if (!sExportHashes) return;
    for (i = 0; i < sExportCount; i++) if (sExportHashes[i] == hash) return;
    if (sExportCount == 2048) return;
    if (!SDL_CreateDirectory(PortConfigPath("hud-source"))) return;
    SDL_snprintf(name, sizeof(name), "hud-source/%016llx.bmp", (unsigned long long)hash);
    surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ARGB8888, sExportPixels, w * 4);
    if (!surface) return;
    if (!SDL_SaveBMP(surface, PortConfigPath(name))) { SDL_DestroySurface(surface); return; }
    SDL_DestroySurface(surface);
    sExportHashes[sExportCount++] = hash;
    f = fopen(PortConfigPath("hud-source/index.csv"), "a");
    if (f) {
        fprintf(f, "%016llx,%d,%d,%d,%d,%d\n", (unsigned long long)hash, w, h, part, ax, ay);
        fclose(f);
    }
}

void PortHdHudShutdown(void) {
    int i;
    for (i=0; i<sAssetCount; i++) {
        if (sAssets[i].image) SDL_DestroyTexture(sAssets[i].image);
        if (sAssets[i].white) SDL_DestroyTexture(sAssets[i].white);
    }
    SDL_free(sAssets); sAssets=NULL; SDL_free(sAssetDraws); sAssetDraws=NULL;
    sAssetCount=sAssetDrawCount=0; sAssetBytes=0; sAssetRenderer=NULL;
    if (sPlayerFrame) SDL_DestroyTexture(sPlayerFrame);
    if (sWhitePlayerFrame) SDL_DestroyTexture(sWhitePlayerFrame);
    sPlayerFrame = sWhitePlayerFrame = NULL;
    if (sFrame) SDL_DestroyTexture(sFrame);
    if (sWhiteFrame) SDL_DestroyTexture(sWhiteFrame);
    if (sLabel) SDL_DestroyTexture(sLabel);
    if (sWhiteLabel) SDL_DestroyTexture(sWhiteLabel);
    sFrame = NULL;
    sWhiteFrame = NULL;
    sLabel = NULL;
    sWhiteLabel = NULL;
    SDL_free(sExportPixels); sExportPixels = NULL;
    SDL_free(sExportHashes); sExportHashes = NULL;
}

void PortHdHudPrepare(SDL_Renderer* renderer) {
    sAssetRenderer = renderer;
    int w, h, channels, x, y, left, top, right = -1, bottom = -1;
    unsigned char* pixels;
    SDL_Surface* surface;
    SDL_Surface* compact;
    SDL_ScaleMode sampling;
    const char* path;
    if (!gPortConfig.hdHud) return;
    if (sAttempted) {
        if (sFrame && sLabelLanguage != HudLanguage()) {
            if (sLabel) SDL_DestroyTexture(sLabel);
            if (sWhiteLabel) SDL_DestroyTexture(sWhiteLabel);
            sLabel = NULL;
            sWhiteLabel = NULL;
            LoadLabel(renderer);
        }
        return;
    }
    sAttempted = 1;
    path = PortConfigPath("assets/hd-hud/pilot/enemy-hp-frame-v2.png");
    if (!stbi_info(path, &w, &h, &channels) || channels != 4 || w < 1 || h < 1 ||
        w > 4096 || h > 4096 || (size_t)w * h > 4194304) {
        PortLog("HD HUD: missing or invalid RGBA pilot; original HUD retained");
        return;
    }
    pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) return;
    left = w; top = h;
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        unsigned char* p = pixels + ((size_t)y * w + x) * 4;
        if (!p[3]) { p[0] = p[1] = p[2] = 0; continue; }
        if (!x || !y || x == w - 1 || y == h - 1) {
            PortLog("HD HUD: exterior is not transparent; original HUD retained");
            stbi_image_free(pixels);
            return;
        }
        /* Tiny alpha noise outside the drawing must not change its layout.
           Keep antialiasing inside the crop; locate the shape with alpha 32+. */
        if (p[3] >= 32) {
            left = SDL_min(left, x); right = SDL_max(right, x);
            top = SDL_min(top, y); bottom = SDL_max(bottom, y);
        }
    }
    if (right < left) { stbi_image_free(pixels); return; }
    left = SDL_max(0, left - 1); top = SDL_max(0, top - 1);
    right = SDL_min(w - 1, right + 1); bottom = SDL_min(h - 1, bottom + 1);
    surface = SDL_CreateSurfaceFrom(right - left + 1, bottom - top + 1, SDL_PIXELFORMAT_RGBA32,
                                    pixels + ((size_t)top * w + left) * 4, w * 4);
    if (surface) {
        int cw = SDL_min(surface->w, 512);
        int ch = SDL_max(1, surface->h * cw / surface->w);
        compact = SDL_CreateSurface(cw, ch, SDL_PIXELFORMAT_RGBA32);
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
        if (!compact || !SDL_BlitSurfaceScaled(surface, NULL, compact, NULL, SDL_SCALEMODE_LINEAR)) {
            if (compact) SDL_DestroySurface(compact);
            SDL_DestroySurface(surface);
            stbi_image_free(pixels);
            return;
        }
        sFrame = SDL_CreateTextureFromSurface(renderer, compact);
        /* Cache an alpha-matched white mask for the cartridge's white fades. */
        for (y = 0; y < ch; y++) for (x = 0; x < cw; x++) {
            unsigned char* p = (unsigned char*)compact->pixels + y * compact->pitch + x * 4;
            p[0] = p[1] = p[2] = 255;
        }
        sWhiteFrame = SDL_CreateTextureFromSurface(renderer, compact);
        sSource = (SDL_FRect){0, 0, cw, ch};
        SDL_DestroySurface(compact);
        SDL_DestroySurface(surface);
    }
    stbi_image_free(pixels);
    if (!sFrame || !sWhiteFrame) { PortHdHudShutdown(); return; }
    /* SDL 3.4.18 software linear minification crashes at this panel's small
       native dimensions. Its pixels are already antialiased at load time. */
    sampling = SDL_strcmp(SDL_GetRendererName(renderer), "software") == 0 ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR;
    SDL_SetTextureBlendMode(sWhiteFrame, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(sWhiteFrame, sampling);
    SDL_SetTextureBlendMode(sFrame, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(sFrame, sampling);
    PortLog("HD HUD: cached transparent enemy HP pilot (%dx%d crop)", right - left + 1, bottom - top + 1);
    LoadLabel(renderer);
    if (SDL_getenv("KHCOM_HDHUD_FRAME_PROBE")) LoadPlayerFrame(renderer);
}

int PortHdHudEnemy(int visible, int ratio, int layer) {
    (void)ratio; (void)layer;
    return visible && gPortConfig.hdHud && sFrame && gPortHudScale && !PortUiActive();
}

void PortHdHudDrawLabel(SDL_Renderer* renderer, int width, int height) {
    int rect[4];
    uint32_t color;
    float scale = (float)gPortHudScale;
    SDL_FRect box, shadow;
    uint16_t bld = *(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x50);
    int effect = (bld >> 6) & 3, amount = SDL_min(*(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x54), 16);
    int tint = (bld & 0x10) && effect == 3 ? 16 - amount : 16;
    Uint8 r, g, b;
    if (!gPortConfig.hdHud || !sLabel || !scale || PortUiActive() || !PortPpuEnemyHudLabel(rect, &color)) return;
    box = (SDL_FRect){(width - gPortHudW * scale) * 0.5f + rect[0] * scale,
                      (height - gPortHudH * scale) * 0.5f + rect[1] * scale,
                      (rect[2] - rect[0] + 1) * scale, (rect[3] - rect[1] + 1) * scale};
    /* Fit the letter faces to native ink; the modern bevel/shadow may extend
       into the same dark surround instead of shrinking the actual letters. */
    if (sLabelInk.w > 0 && sLabelInk.h > 0) {
        float sx = box.w / sLabelInk.w, sy = box.h / sLabelInk.h;
        box.x -= sLabelInk.x * sx; box.y -= sLabelInk.y * sy;
        box.w = sLabelSource.w * sx; box.h = sLabelSource.h * sy;
    }
    /* Keep the approved green faces/pale bevel/dark relief as authored;
       monochrome tinting would wash all three into the highlight colour. */
    r = g = b = 255 * tint / 16;
    shadow = box; shadow.x += scale * 0.5f; shadow.y += scale * 0.5f;
    SDL_SetTextureColorMod(sLabel, r / 5, g / 5, b / 5);
    SDL_RenderTexture(renderer, sLabel, &sLabelSource, &shadow);
    SDL_SetTextureColorMod(sLabel, r, g, b);
    SDL_RenderTexture(renderer, sLabel, &sLabelSource, &box);
    if (sWhiteLabel && (bld & 0x10) && effect == 2 && amount) {
        SDL_SetTextureAlphaMod(sWhiteLabel, amount * 255 / 16);
        SDL_RenderTexture(renderer, sWhiteLabel, &sLabelSource, &shadow);
        SDL_RenderTexture(renderer, sWhiteLabel, &sLabelSource, &box);
    }
}

/* Palette-matched HD fill: interpolate native highlight/body/shadow rows at
   output resolution, keeping the cartridge's exact visible shape and ratio. */
static void DrawVitalityBar(SDL_Renderer* renderer, int width, int height, int player) {
    int rect[4], line, lines;
    uint32_t rows[32];
    float scale = (float)gPortHudScale;
    float ox = (width - gPortHudW * scale) * 0.5f, oy = (height - gPortHudH * scale) * 0.5f;
    if (!gPortConfig.hdHud || !scale || PortUiActive() ||
        !(player ? PortPpuPlayerHudBar(rect, rows) : PortPpuEnemyHudBar(rect, rows))) return;
    if (player) {
        const char* trace;
        PORT_ENV_ONCE(trace, SDL_getenv, "KHCOM_HUD_FULLHP_TRACE");
        while (trace && *trace) {
            if ((uint32_t)SDL_atoi(trace) == PortFrameCount())
                PortLog("HD HUD: fullhp frame=%u value=%d", PortFrameCount(), PortPpuPlayerHudFullHp());
            trace = SDL_strchr(trace, ',');
            if (trace) trace++;
        }
    }
    lines = (int)((rect[3] - rect[1] + 1) * scale);
    for (line = 0; line < lines; line++) {
        float y = rect[1] + line / scale;
        int ya = (int)y, yb = SDL_min(ya + 1, rect[3]), x = rect[0];
        float t = y - ya;
        uint32_t a = rows[ya], b = rows[yb];
        Uint8 r = ((a >> 16) & 255) * (1 - t) + ((b >> 16) & 255) * t;
        Uint8 g = ((a >> 8) & 255) * (1 - t) + ((b >> 8) & 255) * t;
        Uint8 blue = (a & 255) * (1 - t) + (b & 255) * t;
        SDL_SetRenderDrawColor(renderer, r, g, blue, 255);
        while (x <= rect[2]) {
            int start;
            while (x <= rect[2] && !(player ? PortPpuPlayerHudBarVisible(x, ya) : PortPpuEnemyHudBarVisible(x, ya))) x++;
            start = x;
            while (x <= rect[2] && (player ? PortPpuPlayerHudBarVisible(x, ya) : PortPpuEnemyHudBarVisible(x, ya))) x++;
            if (x > start) {
                SDL_FRect row = {ox + start * scale, oy + rect[1] * scale + line, (x - start) * scale, 1};
                SDL_RenderFillRect(renderer, &row);
            }
        }
    }
}

void PortHdHudDrawBar(SDL_Renderer* renderer, int width, int height) {
    DrawVitalityBar(renderer, width, height, 0);
    DrawVitalityBar(renderer, width, height, 1);
}

void PortHdHudDraw(SDL_Renderer* renderer, int width, int height) {
    float scale = (float)gPortHudScale;
    SDL_FRect frame;
    int rect[4];
    uint16_t bld = *(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x50);
    int effect = (bld >> 6) & 3;
    int amount = SDL_min(*(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x54), 16);
    Uint8 tint = (bld & 0x10) && effect == 3 ? 255 * (16 - amount) / 16 : 255;
    if (PortHdHudPlayerFrameReady() && PortPpuPlayerHudFrame(rect)) {
        frame = (SDL_FRect){(width - gPortHudW * scale) * 0.5f + rect[0] * scale,
            (height - gPortHudH * scale) * 0.5f + rect[1] * scale,
            (rect[2] - rect[0] + 1) * scale, (rect[3] - rect[1] + 1) * scale};
        SDL_SetTextureColorMod(sPlayerFrame, tint, tint, tint);
        SDL_RenderTexture(renderer, sPlayerFrame, &sPlayerSource, &frame);
        if ((bld & 0x10) && effect == 2 && amount) {
            SDL_SetTextureAlphaMod(sWhitePlayerFrame, amount * 255 / 16);
            SDL_RenderTexture(renderer, sWhitePlayerFrame, &sPlayerSource, &frame);
        }
    }
    if (!gPortConfig.hdHud || !sFrame || !scale || PortUiActive() || !PortPpuEnemyHudRect(rect)) return;
    frame.w = (rect[2] - rect[0] + 1) * scale;
    frame.h = (rect[3] - rect[1] + 1) * scale;
    frame.x = (width - gPortHudW * scale) * 0.5f + rect[0] * scale;
    frame.y = (height - gPortHudH * scale) * 0.5f + rect[1] * scale;
    const char* hudLog;
    PORT_ENV_ONCE(hudLog, SDL_getenv, "KHCOM_HDHUDLOG");
    if (hudLog) PortLog("HD HUD rect: %d,%d,%d,%d -> %.1f,%.1f %.1fx%.1f src %.1fx%.1f", rect[0], rect[1], rect[2], rect[3], frame.x, frame.y, frame.w, frame.h, sSource.w, sSource.h);
    SDL_SetTextureColorMod(sFrame, tint, tint, tint);
    SDL_RenderTexture(renderer, sFrame, &sSource, &frame);
    if ((bld & 0x10) && effect == 2 && amount) {
        SDL_SetTextureAlphaMod(sWhiteFrame, amount * 255 / 16);
        SDL_RenderTexture(renderer, sWhiteFrame, &sSource, &frame);
    }
}
