/*
 * Entry point and frame loop.
 *
 * The game keeps its own main loop (AgbMain). Every time it waits for the
 * vertical blank, PortFrame() draws the frame, delivers the interrupts the
 * hardware would raise, presents the picture, mixes audio and paces the loop.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "port/pad_layout.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"
#include "port/port_netlink.h"
#include "port/port_version.h"
#include "mode.h"
#include "port/port_aspect.h"
#include "port/port_hd2d.h"
#include "port/port_hdlayout.h"
#include "port/port_hdsprites.h"
#include "../decompui/third_party/stb_image.h"
#include "port/port_ui.h"
#include "port/port_state.h"
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
#include "port/port_android.h"
#endif
#if defined(__ANDROID__) || defined(PORT_ANDROID64)
static int sAndroidBackground;
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include "port/port_web.h"
EM_JS(int, PortWebCanvasWidth, (void), { return Module.canvas.width; });
EM_JS(int, PortWebCanvasHeight, (void), { return Module.canvas.height; });
EM_ASYNC_JS(int, PortWebResetSave, (void), { return Module.saveBeforeReset ? await Module.saveBeforeReset() : 0; });
#endif

typedef volatile unsigned int vu32;

typedef void (*IntrFunc)(void);

extern void AgbMain(void);
extern Mode* gCurrentMode;
extern Mode gModeBattle;
extern IntrFunc gIntrTable[];

/* The game's mutable globals (see port/game_prefix.h). */
static char* sGameData;
static size_t sGameDataSize;
static char* sGameBss;
static size_t sGameBssSize;

static SDL_Window* sWindow;
#ifdef __EMSCRIPTEN__
void PortWebApplyResize(int w,int h) {
    if(sWindow) {
        extern void PortWebClearInput(void);
        PortWebClearInput(); /* no held finger may jump to a new button after rotation */
        emscripten_set_canvas_element_size("#canvas",w,h);
        SDL_SetWindowSize(sWindow,w,h);
    }
}
#endif
static SDL_Renderer* sRenderer;
static SDL_Texture* sTexture;
static int sTexW, sTexH;
static jmp_buf sResetJmp;
static void* sDataSnapshot;
static size_t sDataSize;
static uint64_t sNextFrame;
static uint64_t sPerfFreq;
static uint32_t sFrameCount;
uint32_t PortFrameCount(void) {
    return (uint32_t)sFrameCount;
}
static float sFitScale; /* see PortFitScreen */
static int sInIrq;
static uint32_t* sCompositeBuf; /* Writable presentation copy, on the heap. */
/* The picture being presented: the copy above when our UI draws on it this frame, else the PPU's frame itself (no
   8 MB copy per frame at x1 1080p). Read by the margin helpers; written only through the copy. */
static uint32_t* sComposite;
static size_t sCompositeCapacity, sHudCapacity;
static void GrowPixelBuffer(uint32_t** buffer, size_t* capacity, size_t pixels) {
    uint32_t* grown;
    if (pixels <= *capacity)
        return;
    grown = (uint32_t*)SDL_realloc(*buffer, pixels * sizeof(*grown));
    if (!grown)
        PortFatal("Out of memory for the presentation buffers");
    *buffer = grown;
    *capacity = pixels;
}
static int HarnessListHas(const char* list, uint32_t frame);
static FILE* sLog;
static int sPacedByVsync;
static float sFps;
static float sLogicFps;      /* game main-loop updates per second */
static uint32_t sLogicLast;
static uint64_t sFpsTime;
static uint32_t sFpsFrames;

void PortLog(const char* fmt, ...) {
    va_list ap;

    if (sLog == NULL)
        sLog = fopen(PortConfigPath("kh_com.log"), "w");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    if (sLog != NULL) {
        va_start(ap, fmt);
        vfprintf(sLog, fmt, ap);
        fputc('\n', sLog);
        fflush(sLog);
        va_end(ap);
    }
}

void PortFatal(const char* fmt, ...) {
    char msg[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    PortLog("FATAL: %s", msg);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kingdom Hearts: Chain of Memories", msg, sWindow);
    exit(1);
}

uint32_t PortTicks(void) {
    return (uint32_t)SDL_GetTicks();
}

/* ---- interrupts -------------------------------------------------------------- */

static int IrqIndex(uint16_t flag) {
    static const uint16_t order[14] = { 0x0080, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
                                        0x0040, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000 };
    int i;

    for (i = 0; i < 14; i++)
        if (order[i] == flag)
            return i;
    return -1;
}

void PortRaiseIrq(uint16_t flag) {
    uint16_t ie, ime;
    int index;

    IO16(0x202) |= flag;
    ime = IO16(0x208);
    ie = IO16(0x200);
    if (!(ime & 1) || !(ie & flag) || sInIrq)
        return;
    index = IrqIndex(flag);
    if (index < 0 || gIntrTable[index] == NULL)
        return;

    /* Same bookkeeping as the crt0 handler: acknowledge, mask, call. */
    IO16(0x202) &= ~flag;
    sInIrq = 1;
    gIntrTable[index]();
    sInIrq = 0;
    IO16(0x200) = ie;
    IO16(0x208) = ime;
}

/* ---- presentation --------------------------------------------------------------- */

/* Fullscreen uses the configured display mode (closest the monitor offers);
   0x0 selects the desktop resolution without a mode change. */
#ifdef PORT_LINUX
/* port/linux/fbdev_linux.c: handhelds without a usable 32-bit GPU driver. */
int PortFbInit(int* w, int* h);
int PortFbActive(void);
void PortFbPresent(SDL_Renderer* renderer);
static int sFbW, sFbH;
#endif

static void ApplyWindowMode(void) {
#ifdef PORT_LINUX
    /* The framebuffer is the screen: one window of its size, paced by the game. */
    if (PortFbActive()) {
        SDL_SetRenderVSync(sRenderer, 0);
        return;
    }
#endif
#ifdef __EMSCRIPTEN__
    /* Browser fullscreen is requested by a user gesture in the launcher. */
    SDL_SetRenderVSync(sRenderer, 0);
    return;
#endif
#if defined(__ANDROID__) || defined(PORT_ANDROID64)
    /* Android has no desktop window mode; keep system bars out of the game. */
    SDL_SetWindowFullscreen(sWindow, true);
    SDL_SetRenderVSync(sRenderer, gPortConfig.vsync ? 1 : 0);
    return;
#endif
    SDL_DisplayID display = SDL_GetDisplayForWindow(sWindow);
    SDL_DisplayMode mode;

    if (gPortConfig.fullscreen && gPortConfig.fullscreenWidth > 0 && gPortConfig.fullscreenHeight > 0
        && SDL_GetClosestFullscreenDisplayMode(display, gPortConfig.fullscreenWidth, gPortConfig.fullscreenHeight,
                                               0.0f, true, &mode)) {
        SDL_SetWindowFullscreenMode(sWindow, &mode);
    } else {
        SDL_SetWindowFullscreenMode(sWindow, NULL);
    }
    SDL_SetWindowFullscreen(sWindow, gPortConfig.fullscreen != 0);
    SDL_SetRenderVSync(sRenderer, gPortConfig.vsync ? 1 : 0);
}

/* Distinct fullscreen resolutions of the window's display, largest first. */
int PortVideoListModes(int* w, int* h, int max) {
    SDL_DisplayMode** modes;
    int count = 0, n, i, j;

    modes = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(sWindow), &n);
    if (modes == NULL)
        return 0;
    for (i = 0; i < n && count < max; i++) {
        for (j = 0; j < count; j++)
            if (w[j] == modes[i]->w && h[j] == modes[i]->h)
                break;
        if (j == count) {
            w[count] = modes[i]->w;
            h[count] = modes[i]->h;
            count++;
        }
    }
    SDL_free(modes);
    return count;
}

static void UpdateResolution(void);

void PortVideoApplyConfig(void) {
    ApplyWindowMode();
    UpdateResolution();
}

static SDL_ScaleMode sPictureScaleMode = SDL_SCALEMODE_NEAREST; /* sTexture's scaling, for the layers over it */

/* The "smooth" filter's scaling. SDL's software renderer (no GPU, remote desktop) reads past its source when it
   stretches with LINEAR to a fullscreen window and closes the game (test_text_options.py, 24/10): there the
   pixel-art scaling, which it does draw safely. */
int PortSmoothScaleMode(void) {
    static int software = -1;
    if (software < 0) {
        const char* name = SDL_GetRendererName(sRenderer);
        software = name != NULL && SDL_strcmp(name, "software") == 0;
    }
    return software ? SDL_SCALEMODE_PIXELART : SDL_SCALEMODE_LINEAR;
}
static SDL_Texture* sMainOverTexture; /* the picture's pixels in front of its HD sprites */
static int sMainOverW, sMainOverH;

static void EnsureTexture(void) {
    SDL_ScaleMode mode;

    if (sTexture != NULL && sTexW == gPortScreen.width && sTexH == gPortScreen.height)
        goto filter;
    if (sTexture != NULL)
        SDL_DestroyTexture(sTexture);
    sTexW = gPortScreen.width;
    sTexH = gPortScreen.height;
    sTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, sTexW, sTexH);
    if (sTexture == NULL)
        PortFatal("Could not create the frame texture: %s", SDL_GetError());
filter:
    switch (gPortConfig.filter) {
        case PORT_FILTER_LINEAR:
            mode = (SDL_ScaleMode)PortSmoothScaleMode();
            break;
        case PORT_FILTER_SHARP:
            mode = SDL_SCALEMODE_PIXELART;
            break;
        default:
            /* fractional fit scale: keep pixels even */
            mode = sFitScale > 0 ? SDL_SCALEMODE_PIXELART : SDL_SCALEMODE_NEAREST;
            break;
    }
    SDL_SetTextureScaleMode(sTexture, mode);
    sPictureScaleMode = mode;
}

static int sAutoScale;
/* GBA-size screens (menus, battle start): the game marks each frame. The
   160 lines then fill the window's height at a fractional scale. */
static int sFitMark, sFit;
/* Screen layer (menus over the world), see port_ppu.c. */
extern uint32_t gPortOverlay[];
extern int gPortOverlayW, gPortScreenLayer;
int PortTakeScreenMark(void);
int PortPpuOverlayActive(void);
static SDL_Texture* sOverlayTex;
static int sOverlayTexW;
static SDL_Texture* sScreenOverTex; /* the menu's pixels in front of its HD parts */
static int sScreenOverTexW;
static float sOverlayScale;

void PortFitScreen(void) {
    sFitMark = 1;
}
static SDL_Texture* sHudTexture;
static int sHudTexW, sHudTexH;
static SDL_Texture* sHudOverTexture; /* HUD sprites in front of HD sprites */
static char sHarnessShot[64];      /* PortHarnessShot: the next presented picture's file */
static int sHudOverW, sHudOverH;
static uint32_t* sHudBuf;

int gPortIntroRequested;
int gPortEventTest = -1; /* KHCOM_EVENT=list:index (0 Sora, 1 Riku) */
int gPortBattleTest = -1; /* KHCOM_BATTLE=entry:stage[:riku] of the debug battle menu */
int gPortMovieTest = -1; /* KHCOM_MOVIE=id (mode_movie.h) */
int gPortStartupLanguage = -1;

/* The part of the window the picture uses: all of it, or the widest 16:9 or
   21:9 rectangle (Aspect ratio option), centred with black bars at the sides. */
static SDL_Rect AspectRect(int outW, int outH) {
    PortAspectBounds fit = PortAspectFit(outW, outH, gPortConfig.aspect);
    SDL_Rect r = { fit.x, fit.y, fit.w, fit.h };
    return r;
}

/* A shared integer UI scale: about 216 logical lines at desktop sizes.
   1080p uses x5 (was x6.75), 1440p x6 (was x9). Keep it independent
   of world zoom, and fit small windows without cropping their UI. */
/* A battle's opening animation (GBA-size, before the arena appears) is
   scaled to cover the window, cropping the top and bottom, instead of
   sitting at full height between margins. */
static uint32_t sCoverFrom = 0xFFFF0000u;
void PortCoverIntro(void) {
    sCoverFrom = PortFrameCount();
}
static int CoverActive(void) {
    return PortFrameCount() - sCoverFrom < 180 && (sFitScale > 0 || !gPortScreen.wide);
}

static float MenuScale(int outW, int outH) {
    /* menus and every GBA-size scene always fill the window's height */
    return SDL_min(outH / (float)GBA_SCREEN_H, outW / (float)GBA_SCREEN_W);
}

/* Auto view: size the canvas to the window every frame. */
static void UpdateResolution(void) {
    int outW, outH, w, h;

    if (gPortConfig.viewMode != PORT_VIEW_AUTO) {
        sAutoScale = 0;
        PortConfigView(gPortConfig.viewMode, &w, &h);
    } else {
        if (!SDL_GetCurrentRenderOutputSize(sRenderer, &outW, &outH) || outW <= 0 || outH <= 0)
            return;
        {
            SDL_Rect area = AspectRect(outW, outH);
            outW = area.w;
            outH = area.h;
        }
        PortConfigAutoView(outW, outH, gCurrentMode == &gModeBattle || gCurrentMode == &gModeVsbattle,
                           &w, &h, &sAutoScale);
    }
    /* screen layer: the 160 GBA lines at the window's height */
    gPortOverlayW = GBA_SCREEN_W;
    sOverlayScale = 0;
    if (gPortConfig.viewMode == PORT_VIEW_AUTO) {
        float f = MenuScale(outW, outH);
        gPortOverlayW = SDL_max(GBA_SCREEN_W, SDL_min((int)SDL_ceilf(outW / f), PORT_MAX_W));
        /* an even margin on both sides: with an odd width the menus' side
           margin ((width - 240) / 2) left the last column on the right
           empty, showing the field through it at 1280x720 */
        if (((gPortOverlayW - GBA_SCREEN_W) & 1) && gPortOverlayW < PORT_MAX_W)
            gPortOverlayW++;
        sOverlayScale = f;
    }
    sFitScale = 0;
    if (sFit && gPortConfig.viewMode == PORT_VIEW_AUTO && !PortUiMenuCanvas()) {
        float f = MenuScale(outW, outH);
        w = SDL_min((int)SDL_ceilf(outW / f), PORT_MAX_W);
        h = GBA_SCREEN_H;
        sFitScale = f;
        sAutoScale = 0;
    }
    /* HUD layer: a larger whole-number scale, keeping at least 144 lines and
       the GBA width so corner-anchored elements never collide. */
    {
        extern int gPortWorldScale;
        gPortWorldScale = sAutoScale;
    }
    gPortHudScale = 0;
    if (gPortConfig.viewMode == PORT_VIEW_AUTO && sAutoScale > 0 && sFitScale == 0 && gPortConfig.hudSize > 0) {
        int hs = sAutoScale + gPortConfig.hudSize;
        while (hs > sAutoScale && (outH / hs < 144 || outW / hs < GBA_SCREEN_W))
            hs--;
        if (hs > sAutoScale) {
            gPortHudScale = hs;
            gPortHudW = SDL_min((outW + hs - 1) / hs, PORT_MAX_W);
            gPortHudH = SDL_min((outH + hs - 1) / hs, PORT_MAX_H);
        }
    }
    if (w != gPortScreen.width || h != gPortScreen.height)
        PortPpuSetResolution(w, h);
}

/* GBA classic (gba_border 1..3): the GBA picture inside a decorative border of 384x216 GBA pixels, the picture's
   hole at (72, 28) (gen_gba_borders.py), both at the largest whole scale that fits the window. */
#include "port/gba_borders_png.inc"
#define BORDER_W 384
#define BORDER_H 216
static SDL_Texture* sBorderTex;
static int sBorderShown; /* which border sBorderTex holds */

static int ClassicBorder(void) {
    return gPortConfig.viewMode == PORT_VIEW_ORIGINAL && gPortConfig.gbaBorder >= 1 && gPortConfig.gbaBorder <= 3 &&
           gPortScreen.width == GBA_SCREEN_W && gPortScreen.height == GBA_SCREEN_H && sFitScale == 0 && sAutoScale == 0;
}

/* Border 1's sides: six of the game's map cards (illustration over its colour's border), drawn from the ROM. */
extern const unsigned char gMapCardBorderPalettes[];
extern const unsigned char gMapCardBorderRedFrame0[], gMapCardBorderRedTiles[];
extern const unsigned char gMapCardBorderGreenFrame0[], gMapCardBorderGreenTiles[];
extern const unsigned char gMapCardBorderBlueFrame0[], gMapCardBorderBlueTiles[];
extern const unsigned char gCardRoom01Frame0[], gCardRoom01Tiles[], gCardRoom01Palette[];
extern const unsigned char gCardRoom03Frame0[], gCardRoom03Tiles[], gCardRoom03Palette[];
extern const unsigned char gCardRoom09Frame0[], gCardRoom09Tiles[], gCardRoom09Palette[];
extern const unsigned char gCardRoom10Frame0[], gCardRoom10Tiles[], gCardRoom10Palette[];
extern const unsigned char gCardRoom14Frame0[], gCardRoom14Tiles[], gCardRoom14Palette[];
extern const unsigned char gCardRoom15Frame0[], gCardRoom15Tiles[], gCardRoom15Palette[];

/* A sprite frame (a count, then each part's three OAM attributes; 4bpp tiles in order) onto px, its origin at
   (ox, oy). Only colours 1..15 of its palette bank, never the white value-slot mark at the frame's right edge. */
static void BlitFrame(unsigned char* px, int w, int h, const unsigned char* frame, const unsigned char* tiles,
                      const unsigned char* palette, int ox, int oy, int clipRight) {
    static const unsigned char kSize[3][4][2] = { { { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 } },
                                                  { { 16, 8 }, { 32, 8 }, { 32, 16 }, { 64, 32 } },
                                                  { { 8, 16 }, { 8, 32 }, { 16, 32 }, { 32, 64 } } };
    int n = frame[0] | frame[1] << 8, k;
    for (k = 0; k < n && k < 64; k++) {
        const unsigned char* a = frame + 2 + k * 6;
        int a0 = a[0] | a[1] << 8, a1 = a[2] | a[3] << 8, a2 = a[4] | a[5] << 8;
        int shape = a0 >> 14, size = a1 >> 14, pw, ph, x = a1 & 0x1FF, y = a0 & 0xFF, tx, ty;
        if (shape > 2)
            continue;
        pw = kSize[shape][size][0];
        ph = kSize[shape][size][1];
        if (x & 0x100) x -= 512;
        if (y & 0x80) y -= 256;
        for (ty = 0; ty < ph; ty++)
            for (tx = 0; tx < pw; tx++) {
                int sx = (a1 & 0x1000) ? pw - 1 - tx : tx, sy = (a1 & 0x2000) ? ph - 1 - ty : ty;
                int tile = (a2 & 0x3FF) + (sy >> 3) * (pw >> 3) + (sx >> 3);
                int b = tiles[tile * 32 + (sy & 7) * 4 + ((sx & 7) >> 1)], v = (sx & 1) ? b >> 4 : b & 15;
                int X = ox + x + tx, Y = oy + y + ty, c;
                unsigned char* d;
                if (!v || X < 0 || Y < 0 || X >= w || Y >= h || x + tx >= clipRight)
                    continue;
                c = palette[(a2 >> 12) * 32 + v * 2] | palette[(a2 >> 12) * 32 + v * 2 + 1] << 8;
                d = px + (Y * w + X) * 4;
                d[0] = (unsigned char)((c & 31) * 255 / 31);
                d[1] = (unsigned char)(((c >> 5) & 31) * 255 / 31);
                d[2] = (unsigned char)(((c >> 10) & 31) * 255 / 31);
                d[3] = 255;
            }
    }
}

static void BorderMapCards(unsigned char* px, int w, int h) {
    static const struct { const unsigned char *frame, *tiles, *palette; int colour; } kCards[6] = {
        { gCardRoom01Frame0, gCardRoom01Tiles, gCardRoom01Palette, 0 },
        { gCardRoom09Frame0, gCardRoom09Tiles, gCardRoom09Palette, 1 },
        { gCardRoom14Frame0, gCardRoom14Tiles, gCardRoom14Palette, 2 },
        { gCardRoom15Frame0, gCardRoom15Tiles, gCardRoom15Palette, 2 },
        { gCardRoom10Frame0, gCardRoom10Tiles, gCardRoom10Palette, 1 },
        { gCardRoom03Frame0, gCardRoom03Tiles, gCardRoom03Palette, 0 },
    };
    const unsigned char* borderFrame[3] = { gMapCardBorderRedFrame0, gMapCardBorderGreenFrame0, gMapCardBorderBlueFrame0 };
    const unsigned char* borderTiles[3] = { gMapCardBorderRedTiles, gMapCardBorderGreenTiles, gMapCardBorderBlueTiles };
    int i;
    for (i = 0; i < 6; i++) {
        /* frames centre on (16, 20) of the card's 32x39: columns at x 20 and 332, rows at y 36, 85, 134 */
        int ox = (i < 3 ? 20 : BORDER_W - 20 - 32) + 16, oy = 36 + (i % 3) * 49 + 20;
        BlitFrame(px, w, h, borderFrame[kCards[i].colour], borderTiles[kCards[i].colour], gMapCardBorderPalettes,
                  ox, oy, 15);
        BlitFrame(px, w, h, kCards[i].frame, kCards[i].tiles, kCards[i].palette, ox, oy, 16);
    }
}

static void DrawClassicBorder(const SDL_FRect* picture) {
    float s = picture->w / GBA_SCREEN_W;
    SDL_FRect r;
    if (!ClassicBorder())
        return;
    if (!sBorderTex || sBorderShown != gPortConfig.gbaBorder) {
        int w, h, n, i = gPortConfig.gbaBorder - 1;
        unsigned char* px = stbi_load_from_memory(kGbaBorders[i], (int)kGbaBorderSizes[i], &w, &h, &n, 4);
        if (sBorderTex)
            SDL_DestroyTexture(sBorderTex);
        sBorderTex = NULL;
        if (px) {
            SDL_Surface* surf;
            if (i == 0)
                BorderMapCards(px, w, h);
            surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4);
            if (surf) {
                sBorderTex = SDL_CreateTextureFromSurface(sRenderer, surf);
                SDL_DestroySurface(surf);
            }
            stbi_image_free(px);
        }
        if (sBorderTex) {
            SDL_SetTextureScaleMode(sBorderTex, SDL_SCALEMODE_NEAREST);
            SDL_SetTextureBlendMode(sBorderTex, SDL_BLENDMODE_BLEND);
        }
        sBorderShown = gPortConfig.gbaBorder;
    }
    if (!sBorderTex)
        return;
    r.x = picture->x - 72 * s;
    r.y = picture->y - 28 * s;
    r.w = BORDER_W * s;
    r.h = BORDER_H * s;
    SDL_RenderTexture(sRenderer, sBorderTex, NULL, &r);
}

static void OutputRect(int outW, int outH, SDL_FRect* dst) {
    float sw = (float)gPortScreen.width, sh = (float)gPortScreen.height;
    float scale;

    if (ClassicBorder() && outW >= BORDER_W && outH >= BORDER_H) {
        scale = (float)(int)SDL_min(outW / (float)BORDER_W, outH / (float)BORDER_H);
        dst->w = sw * scale;
        dst->h = sh * scale;
        dst->x = (float)(int)((outW - BORDER_W * scale) / 2 + 72 * scale);
        dst->y = (float)(int)((outH - BORDER_H * scale) / 2 + 28 * scale);
        return;
    }

    if (sFitScale > 0) {
        dst->w = sw * sFitScale;
        dst->h = sh * sFitScale;
        dst->x = (float)(int)((outW - dst->w) / 2);
        dst->y = (float)(int)((outH - dst->h) / 2);
        return;
    }
    if (sAutoScale > 0) {
        /* the Auto view's scale (whole with Integer scale on); the canvas
           may overhang the window by < 1 pixel */
        extern float gPortAutoScaleF;
        float f = gPortAutoScaleF > 0 ? gPortAutoScaleF : (float)sAutoScale;
        dst->w = SDL_roundf(sw * f);
        dst->h = SDL_roundf(sh * f);
        dst->x = (float)((outW - (int)dst->w) / 2);
        dst->y = (float)((outH - (int)dst->h) / 2);
        return;
    }

    switch (gPortConfig.scaleMode) {
        case PORT_SCALE_STRETCH:
            dst->x = 0;
            dst->y = 0;
            dst->w = (float)outW;
            dst->h = (float)outH;
            return;
        case PORT_SCALE_FIT:
            scale = SDL_min(outW / sw, outH / sh);
            break;
        default:
            scale = (float)(int)SDL_min(outW / sw, outH / sh);
            if (scale < 1.0f)
                scale = SDL_min(outW / sw, outH / sh);
            break;
    }
    dst->w = sw * scale;
    dst->h = sh * scale;
    dst->x = (float)(int)((outW - dst->w) / 2);
    dst->y = (float)(int)((outH - dst->h) / 2);
}

/* Scanlines as the MiSTer scaler draws them (Template_MiSTer sys/scanlines.v):
   each game line is shown as a bright line followed by a darkened one, the
   first line of the frame bright; the darkened half keeps 75/50/25% of its
   brightness (the setting's 25/50/75%; 100% is a black line). Every game
   line's span is rounded on its own, so non-integer scales stay even, and
   the side margins share the picture's spacing and phase (#27, #29). */
static void DrawScanlines(const SDL_FRect* dst, int outW) {
    SDL_FRect rects[1024];
    int lines = gPortScreen.height, n = 0, i;
    float step = dst->h / lines;

    if (gPortConfig.scanlines <= 0 || step < 2.0f || lines > 1024)
        return;
    for (i = 0; i < lines; i++) {
        float top = SDL_floorf(dst->y + i * step + 0.5f);
        float bottom = SDL_floorf(dst->y + (i + 1) * step + 0.5f);
        float mid = top + SDL_floorf((bottom - top) / 2.0f + 0.5f);
        if (bottom > mid)
            rects[n++] = (SDL_FRect){ 0, mid, (float)outW, bottom - mid };
    }
    SDL_SetRenderDrawBlendMode(sRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, (Uint8)(gPortConfig.scanlines * 255 / 100));
    SDL_RenderFillRects(sRenderer, rects, n);
}


void PortHdTextDraw(SDL_Renderer* renderer, int hudLayer, float originX, float originY, float scale);

/* Side bars of GBA-size scenes, filled the way the screen chooses
   (PortSideStyle): its edge columns stretched (default), its background
   colour, or its paper texture repeated mirrored from clean bands. */
#define SIDE_STRIP 16
/* one texture per side: both margins are drawn in the same frame, and a
   shared texture let one side show the other's (or last frame's) pixels */
static SDL_Texture* sMenuSideTex[2];
static SDL_Texture* sFadeSideTex[2];
static int sFadeSideW[2], sMenuSideW[2];
#define sMenuSideTexture sMenuSideTex[side > 0]
#define sFadeSideTexture sFadeSideTex[side > 0]
#define sFadeSideWidth sFadeSideW[side > 0]
#define sMenuSideWidth sMenuSideW[side > 0]
static int sSideStyle; /* engine.h: stretch, colour, texture, blur, menu, pattern */
static int sMenuPanelSide = -1;
extern int PortMenuBackgroundOnly(void);
void PortMenuPanelSide(int side) { sMenuPanelSide = side; }

void PortSideStyle(int style) {
    sSideStyle = style;
}

static uint32_t StripPixel(int sx, int sy, int x, int y) {
    return sComposite[(sy + y) * gPortScreen.width + sx + x];
}

static float PixelEdge(float origin, int r, float scale);

/* The GBA picture is one flat colour (every sampled pixel within a few
   levels of the first): the end of a fade, or a blank frame between modes. */
static int UniformPicture(const SDL_FRect* src, uint32_t* color) {
    int x, y;
    uint32_t c0 = sComposite[(int)src->y * gPortScreen.width + (int)src->x];
    for (y = 0; y < (int)src->h; y += 4)
        for (x = 0; x < (int)src->w; x += 4) {
            uint32_t c = sComposite[((int)src->y + y) * gPortScreen.width + (int)src->x + x];
            int dr = (int)((c >> 16) & 255) - (int)((c0 >> 16) & 255);
            int dg = (int)((c >> 8) & 255) - (int)((c0 >> 8) & 255);
            int db = (int)(c & 255) - (int)(c0 & 255);
            if (dr * dr + dg * dg + db * db > 3 * 10 * 10)
                return 0;
        }
    *color = c0;
    return 1;
}

/* A margin texture (column 0 = the outermost) drawn column by column, each
   GBA column in the exact window pixels the picture's own sampling would
   give it: at a scale like 6.75 the scaler alone breaks ties between two
   columns differently for a texture placed elsewhere, shifting a margin's
   stipple by a pixel every few columns. */
static void RenderMarginColumns(SDL_Texture* tex, int fw, int side, float picX, float y, float scale, float limit) {
    int xx;
    for (xx = 0; xx < fw; xx++) {
        int c = side < 0 ? xx - fw : GBA_SCREEN_W + xx;
        float a = PixelEdge(picX, c, scale), b = PixelEdge(picX, c + 1, scale);
        SDL_FRect src = { (float)xx, 0, 1, GBA_SCREEN_H }, d;
        /* next to the picture the margin starts where the picture's own
           pixels stop: a window pixel whose centre lies exactly on its
           right edge is not drawn by the picture (at 1920x997 the edge is
           at x 1707.5), and the tie rule above would skip it as well,
           leaving a black column down the screen */
        if (c == GBA_SCREEN_W)
            a = SDL_ceilf(picX + GBA_SCREEN_W * scale - 0.5f);
        if (side < 0 ? b <= 0 : a >= limit)
            continue;
        d.x = a;
        d.y = y;
        d.w = b - a;
        d.h = GBA_SCREEN_H * scale;
        SDL_RenderTexture(sRenderer, tex, &src, &d);
    }
}

extern int PortMenuSliding(void);

static void SideBar(int sx, int sy, int side, float y, float scale, float x0, float x1, int edgeX) {
    float w = SIDE_STRIP * scale;
    int band;
    int backgroundOnly = PortMenuBackgroundOnly();

    if (x1 - x0 <= 0)
        return;
    /* The spotlight's map has coloured padding outside its GBA canvas.
       The presentation was copying that padding into otherwise black bars. */
    if (PortSpotlightActive())
        return;
    if (sSideStyle == 6 || sSideStyle == 7 || sSideStyle == 11) {
        static const uint8_t dither[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
        static uint32_t pixels[PORT_MAX_W * GBA_SCREEN_H];
        int fw = SDL_clamp((int)SDL_ceilf((x1 - x0) / scale), 1, PORT_MAX_W);
        int xx, yy;
        uint32_t base = gPortMenuBackground[(sy + 16) * gPortScreen.width + edgeX];
        if (sFadeSideWidth != fw) {
            if (sFadeSideTexture) SDL_DestroyTexture(sFadeSideTexture);
            sFadeSideTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_XRGB8888,
                                                 SDL_TEXTUREACCESS_STREAMING, fw, GBA_SCREEN_H);
            sFadeSideWidth = sFadeSideTexture ? fw : 0;
        }
        if (!sFadeSideTexture) return;
        if (sSideStyle == 7 || sSideStyle == 11) {
            extern void PortNarrowSidesFinish(void);
            PortNarrowSidesFinish();
        }
        for (yy = 0; yy < GBA_SCREEN_H; yy++) {
            for (xx = 0; xx < fw; xx++) {
                int distance = side < 0 ? fw - xx - 1 : xx;
                if (sSideStyle == 7 || sSideStyle == 11) {
                    /* the PPU's own margin: scenery mirrored, bands carried on */
                    extern uint32_t PortNarrowSidePixel(int side, int y, int distance);
                    uint32_t color = PortNarrowSidePixel(side, yy, distance);
                    if (yy < 16 && side < 0) {
                        uint32_t header = PortMenuHeaderPixel(yy, xx);
                        if (header) color = header;
                    }
                    pixels[yy * fw + xx] = color;
                    continue;
                }
                int phase = distance & 15;
                int sample = edgeX + (side < 0 ? phase : -phase);
                uint32_t color = gPortMenuBackground[(sy + yy) * gPortScreen.width + sample];
                /* Ordered pixel-art fade: retain original palette colours,
                   never magnify a framebuffer column or copy foreground OBJs. */
                if (yy >= 16 && yy < 152 && !(side > 0 && yy >= 24 && yy < 48) &&
                    dither[(yy & 3) * 4 + (distance & 3)] <
                        SDL_min(distance * 16 / SDL_max(1, SDL_min(fw - 1, 48)), 16))
                    color = base;
                if (yy < 16 && side < 0) {
                    uint32_t header = PortMenuHeaderPixel(yy, xx);
                    if (header) color = header;
                }
                pixels[yy * fw + xx] = color;
            }
        }
        SDL_UpdateTexture(sFadeSideTexture, NULL, pixels, fw * 4);
        /* on the picture's own pixel grid, anchored at its edge (see the
           menu style below), so the margin's columns and rows line up */
        SDL_SetTextureScaleMode(sFadeSideTexture, gPortConfig.filter == PORT_FILTER_LINEAR ? (SDL_ScaleMode)PortSmoothScaleMode()
                                                                                           : SDL_SCALEMODE_PIXELART);
        RenderMarginColumns(sFadeSideTexture, fw, side, side < 0 ? x1 : x0 - GBA_SCREEN_W * scale, y, scale, x1);
        return;
    }
    SDL_SetTextureScaleMode(sTexture, SDL_SCALEMODE_NEAREST);
    if (sSideStyle == 4 || sSideStyle == 5 || (sSideStyle == 0 && PortMenuHeaderPixel(4, 0))) {
        /* Extend background tiles, never the foreground labels or cursor.
           The margin is drawn as GBA columns beyond the picture's edge, on
           the picture's own pixel grid: column d (0 next to the picture)
           carries on the screen's patterns at their own phase, and the
           texture is sampled exactly like the picture, so every row and
           column lines up with it pixel for pixel. */
        static uint32_t strip[PORT_MAX_W * GBA_SCREEN_H];
        int fw = SDL_clamp((int)SDL_ceilf((x1 - x0) / scale) + 1, 1, PORT_MAX_W);
        int yy, xx;
        if (sMenuSideWidth != fw) {
            if (sMenuSideTexture) SDL_DestroyTexture(sMenuSideTexture);
            sMenuSideTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_XRGB8888,
                SDL_TEXTUREACCESS_STREAMING, fw, GBA_SCREEN_H);
            sMenuSideWidth = sMenuSideTexture ? fw : 0;
        }
        if (!sMenuSideTexture)
            return;
        for (yy = 0; yy < GBA_SCREEN_H; yy++) {
            /* World map (style 5): its bands repeat every 16 columns, so the
               edge's last 16 columns carry on in every row. The band's colours
               are those found at both edges of the row; when the two edges
               share some, any other colour is something on the band (the
               world's logo) and takes the commonest shared colour. Edges that
               share none (the floor badge on the left) continue their edge
               column, so the badge's letters are not repeated. */
            uint32_t common = 0;
            int best = 0, k2, plain = 0, badge;
            /* the world map's bands come from the picture as shown (sprites,
               fades and all, the same frame), so the margins never lag a
               frame behind them while the menu slides */
            const uint32_t* bgRow = sSideStyle == 5 ? &sComposite[(sy + yy) * gPortScreen.width]
                                                    : &gPortMenuBackground[(sy + yy) * gPortScreen.width];
            const uint32_t* own = &bgRow[sx];
            const uint32_t* other = &bgRow[side < 0 ? sx + GBA_SCREEN_W - 16 : sx - (GBA_SCREEN_W - 16)];
            if (sSideStyle == 5)
                for (xx = 0; xx < 16; xx++) {
                    int n2 = 0, shared = 0;
                    for (k2 = 0; k2 < 16; k2++) {
                        n2 += own[k2] == own[xx];
                        shared |= other[k2] == own[xx];
                    }
                    if (shared && n2 > best) { best = n2; common = own[xx]; }
                }
            /* edges with nothing in common: a band that repeats every 1, 2 or
               4 columns (plain or a checkerboard) still repeats; a badge with
               letters does not */
            /* the column touching the margin in a colour the other edge
               lacks is the floor badge reaching the screen edge (also while
               it slides in over the bands): the row carries it on plainly */
            badge = 0;
            if (sSideStyle == 5 && side < 0) { /* the floor badge sits on the left */
                uint32_t adj = sComposite[(sy + yy) * gPortScreen.width + edgeX];
                int adjShared = 0;
                for (k2 = 0; k2 < 16; k2++) adjShared |= other[k2] == adj;
                if (!adjShared) {
                    best = 0;
                    badge = 1;
                }
            }
            if (sSideStyle == 5 && !best && !badge) {
                int per;
                for (per = 1; per <= 4 && !plain; per *= 2) {
                    for (xx = 0; xx + per < 16 && own[xx] == own[xx + per]; xx++) {}
                    plain = xx + per >= 16;
                }
            }
            uint32_t shownEdge = gPortFrame[(sy + yy) * gPortScreen.width + edgeX];
            /* the edge's background hidden this frame (rows of plain colour
               in the picture where the background has something else) */
            int hidden = sSideStyle == 4 && !backgroundOnly && PortMenuSliding() && shownEdge != bgRow[edgeX];
            for (xx = 0; xx < fw; xx++) {
                int d = side < 0 ? fw - 1 - xx : xx, sample = edgeX;
                uint32_t* px = &strip[yy * fw + xx];
                /* The outside gutter is clear of the grid silhouettes.
                   Repeat its stipple on either side when the panels swap:
                   period 4, so GBA column -1-d (or 240+d) is column
                   (-1-d) & 3 (or d & 3) of the pattern. */
                if (sSideStyle == 4 && side == sMenuPanelSide && yy >= 32 && yy < 132)
                    sample = side < 0 ? sx + ((-1 - d) & 3) : edgeX - 3 + (d & 3);
                /* Only the world's ornamental bands repeat horizontally;
                   its floor badge and text retain the plain edge fill. */
                if (sSideStyle == 5 && (best || plain))
                    sample = side < 0 ? sx + ((-1 - d) & 15) : sx + (d & 15);
                *px = bgRow[sample];
                /* the badge's rows: the edge as it shows, sprites included
                   (while it slides in, its lower rows are a sprite) */
                if (badge)
                    *px = sComposite[(sy + yy) * gPortScreen.width + edgeX];
                if (sSideStyle == 5 && best) {
                    int shared = 0;
                    for (k2 = 0; k2 < 16; k2++) shared |= other[k2] == *px;
                    if (!shared)
                        *px = common;
                }
                /* the band sprites pass behind the floor badge (a row whose
                   edges share nothing): while they slide in, the badge's
                   continuation stays in front of them, as in the picture */
                if (0 && sSideStyle == 5 && !badge && (best || plain)) { /* the composite has the bands */
                    uint32_t bandColor = PortMenuBandPixel(yy, side < 0 ? (-1 - d) & 15 : d & 15);
                    if (bandColor) *px = bandColor;
                }
                if (yy < 16 && side < 0) {
                    uint32_t header = PortMenuHeaderPixelAt(yy, d);
                    if (header) *px = header;
                }
                /* a menu still coming in: its header's line (a sprite) runs
                   across the picture before the title arrives; a sprite row
                   of one colour 64 pixels long from the edge carries on */
                /* (never over the header itself: the title stays on top) */
                if (sSideStyle == 4 && yy < 32 && !PortMenuHeaderPixel(yy & 15, 0) &&
                    !(yy < 16 && PortMenuHeaderPixelAt(yy, d))) {
                    const uint32_t* fr = &gPortFrame[(sy + yy) * gPortScreen.width];
                    uint32_t c = fr[edgeX];
                    int k3, step = side < 0 ? 1 : -1;
                    for (k3 = 0; k3 < 64 && fr[edgeX + k3 * step] == c; k3++) {}
                    if (k3 >= 64 && c != gPortMenuBackground[(sy + yy) * gPortScreen.width + edgeX])
                        *px = c;
                }
                if (sSideStyle == 4 && side == sMenuPanelSide && yy >= 16 && yy < 32 && !hidden) {
                    uint32_t tab = PortMenuTabPixel(yy, d, side);
                    if (tab) *px = tab;
                }
                /* while a menu slides in or out, a window or the sliding art
                   hides the background at the picture's edge: the margin
                   carries on what the edge shows, never what is not there yet */
                if (hidden && !(yy < 16 && side < 0 && PortMenuHeaderPixelAt(yy, d)))
                    *px = shownEdge;
            }
        }
        /* The tab strip: a line between rows of the panel's own colour is
           the end of a tab's art at the screen edge (the eye tab's stroke
           on the right), not the panel, so the margin keeps the panel plain. */
        if (sSideStyle == 4 && side == sMenuPanelSide)
            for (xx = 0; xx < fw; xx++) {
                uint32_t panel = strip[32 * fw + xx];
                int above = 0;
                for (yy = 16; yy < 32; yy++) {
                    uint32_t* px = &strip[yy * fw + xx];
                    int k;
                    if (*px == panel) {
                        above = 1;
                        continue;
                    }
                    for (k = yy + 1; k <= 32 && strip[k * fw + xx] != panel; k++) {}
                    if (above && k <= 32)
                        *px = panel;
                }
            }
        SDL_UpdateTexture(sMenuSideTexture, NULL, strip, fw * 4);
        SDL_SetTextureScaleMode(sMenuSideTexture, gPortConfig.filter == PORT_FILTER_LINEAR ? (SDL_ScaleMode)PortSmoothScaleMode()
                                                                                           : SDL_SCALEMODE_PIXELART);
        RenderMarginColumns(sMenuSideTexture, fw, side, side < 0 ? x1 : x0 - GBA_SCREEN_W * scale, y, scale, x1);
        return;
    }
    if (sSideStyle == 1) {
        /* One shared background for both sides. Counting exact colours
           matters during fades: 4-bit bins merged grey paper with black
           artwork, making only the right margin go dark. */
        static uint16_t count[8192];
        static uint32_t colors[8192];
        uint32_t bd = 0;
        int i, best = 0, left = side < 0 ? sx : sx - (GBA_SCREEN_W - SIDE_STRIP);
        memset(count, 0, sizeof(count));
        for (i = 0; i < GBA_SCREEN_H * SIDE_STRIP * 2; i++) {
            int strip = i / (GBA_SCREEN_H * SIDE_STRIP);
            int offset = i % (GBA_SCREEN_H * SIDE_STRIP);
            uint32_t c = StripPixel(left + strip * (GBA_SCREEN_W - SIDE_STRIP), sy,
                                    offset % SIDE_STRIP, offset / SIDE_STRIP);
            unsigned q = (c * 2654435761u) >> 19;
            while (count[q] && colors[q] != c) q = (q + 1) & 8191;
            colors[q] = c;
            if (++count[q] > best) {
                best = count[q];
                bd = c;
            }
        }
        /* Snap both boundaries, not just the width: truncating a
           fractional x plus a fractional width left the final column
           black in windows such as 1920x997 (software SDL renderer). */
        float marginLeft = SDL_ceilf(x0 - 0.5f), marginRight = SDL_ceilf(x1 - 0.5f);
        SDL_FRect r = { marginLeft, y, marginRight - marginLeft, GBA_SCREEN_H * scale };
        SDL_SetRenderDrawColor(sRenderer, (bd >> 16) & 255, (bd >> 8) & 255, bd & 255, 255);
        SDL_RenderFillRect(sRenderer, &r);
        /* Title bands use the same native row grid as the picture, at
           every output size. The original 240x160 image stays untouched. */
        for (i = 0; i < GBA_SCREEN_H; i++) {
            uint32_t c = PortTitleBandPixel(side, i);
            if (c) {
                float top = PixelEdge(y, i, scale), bottom = PixelEdge(y, i + 1, scale);
                SDL_FRect bandRect = { marginLeft, top, marginRight - marginLeft, bottom - top };
                SDL_SetRenderDrawColor(sRenderer, (c >> 16) & 255, (c >> 8) & 255, c & 255, 255);
                SDL_RenderFillRect(sRenderer, &bandRect);
            }
        }
        return;
    }
    if (sSideStyle != 2) {
        /* the picture's edge column, stretched to the window's edge */
        SDL_FRect es = { (float)edgeX, (float)sy, 1, GBA_SCREEN_H }, ed = { x0, y, x1 - x0, GBA_SCREEN_H * scale };
        SDL_RenderTexture(sRenderer, sTexture, &es, &ed);
        return;
    }
    {
        /* texture: 8-line bands; a band with drawing in it (an emblem)
           takes the nearest clean band above (or below) instead. Sampled
           like the picture, so each 16-column tile sits on its pixel grid */
        /* Palette fades subtract brightness and clip dark ink to black;
           they do not scale texture contrast. Choose the least contrasted
           paper bands in this frame, so emblems never become clean merely
           because their black ink has reached the fade's floor. */
        int clean[GBA_SCREEN_H / 8], n = GBA_SCREEN_H / 8, b;
        int spread[GBA_SCREEN_H / 8], best = 10000;
        for (b = 0; b < n; b++) {
            int x, yy, lo = 255, hi = 0, chroma = 0;
            for (yy = b * 8; yy < b * 8 + 8; yy++) {
                for (x = 0; x < SIDE_STRIP; x++) {
                    uint32_t c = StripPixel(sx, sy, x, yy);
                    int r = (c >> 16) & 255, g = (c >> 8) & 255, bl = c & 255;
                    int l = (r * 3 + g * 6 + bl) / 10;
                    lo = SDL_min(lo, l);
                    hi = SDL_max(hi, l);
                    chroma = SDL_max(chroma, SDL_max(r, SDL_max(g, bl)) - SDL_min(r, SDL_min(g, bl)));
                }
            }
            spread[b] = hi - lo + chroma;
            best = SDL_min(best, spread[b]);
        }
        for (b = 0; b < n; b++)
            clean[b] = spread[b] == best;
        for (band = 0; band < n; band++) {
            int srcBand = band, k;
            float by = y + band * 8 * scale, pos;
            int flip = 1;
            for (k = 0; !clean[srcBand] && k < n; k++) {
                if (band - k >= 0 && clean[band - k]) {
                    srcBand = band - k;
                    break;
                }
                if (band + k < n && clean[band + k]) {
                    srcBand = band + k;
                    break;
                }
            }
            for (pos = side < 0 ? x1 : x0; side < 0 ? pos > x0 : pos < x1; pos += side * w, flip ^= 1) {
                SDL_FRect src = { (float)sx, (float)(sy + srcBand * 8), SIDE_STRIP, 8 };
                SDL_FRect d = { side < 0 ? pos - w : pos, by, w, 8 * scale };
                SDL_SetTextureScaleMode(sTexture, gPortConfig.filter == PORT_FILTER_LINEAR ? (SDL_ScaleMode)PortSmoothScaleMode()
                                                                                           : SDL_SCALEMODE_PIXELART);
                SDL_RenderTextureRotated(sRenderer, sTexture, &src, &d, 0, NULL, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            }
        }
    }
}

/* Side bars of style 3: the picture itself, cropped to the window's shape,
   shrunk to a few pixels and enlarged again (a blurred reflection), dimmed. */
static SDL_Texture* sBlurTex;
#define BLUR_W 24
#define BLUR_H 14
#define BLUR_OUT_W 320
#define BLUR_OUT_H 180

static void DrawBlurredReflection(const SDL_FRect* src, int outW, int outH) {
    static uint32_t cell[BLUR_W * BLUR_H], big[BLUR_OUT_W * BLUR_OUT_H];
    float aspect = outW / (float)outH, cw = src->w, ch = src->h;
    int x0, y0, w, h, cx, cy;

    if (sBlurTex == NULL) {
        sBlurTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, BLUR_OUT_W,
                                     BLUR_OUT_H);
        if (sBlurTex == NULL)
            return;
        SDL_SetTextureScaleMode(sBlurTex, SDL_SCALEMODE_NEAREST);
    }
    if (aspect >= cw / ch)
        ch = cw / aspect; /* wider window: keep the full width, crop rows */
    else
        cw = ch * aspect;
    w = (int)cw;
    h = (int)ch;
    x0 = (int)src->x + ((int)src->w - w) / 2;
    y0 = (int)src->y + ((int)src->h - h) / 2;
    /* mean colour of each cell of the cropped picture, dimmed */
    for (cy = 0; cy < BLUR_H; cy++) {
        for (cx = 0; cx < BLUR_W; cx++) {
            int xa = x0 + cx * w / BLUR_W, xb = x0 + (cx + 1) * w / BLUR_W;
            int ya = y0 + cy * h / BLUR_H, yb = y0 + (cy + 1) * h / BLUR_H, x, y, n = 0;
            unsigned r = 0, g = 0, b = 0;
            for (y = ya; y < yb; y++) {
                for (x = xa; x < xb; x++) {
                    uint32_t c = sComposite[y * gPortScreen.width + x];
                    r += (c >> 16) & 255;
                    g += (c >> 8) & 255;
                    b += c & 255;
                    n++;
                }
            }
            if (n == 0)
                n = 1;
            cell[cy * BLUR_W + cx] = 0xFF000000u | ((r * 110 / 255 / n) << 16) | ((g * 110 / 255 / n) << 8) | (b * 110 / 255 / n);
        }
    }
    /* bilinear enlargement in code: smooth, and nothing but a plain copy
       is left for the renderer */
    for (cy = 0; cy < BLUR_OUT_H; cy++) {
        float fy = (cy + 0.5f) * BLUR_H / BLUR_OUT_H - 0.5f;
        int iy = (int)SDL_floorf(fy), jy;
        float ty = fy - iy;

        jy = SDL_clamp(iy + 1, 0, BLUR_H - 1);
        iy = SDL_clamp(iy, 0, BLUR_H - 1);
        for (cx = 0; cx < BLUR_OUT_W; cx++) {
            float fx = (cx + 0.5f) * BLUR_W / BLUR_OUT_W - 0.5f;
            int ix = (int)SDL_floorf(fx), jx, sh;
            float tx = fx - ix;
            uint32_t out = 0xFF000000u;

            jx = SDL_clamp(ix + 1, 0, BLUR_W - 1);
            ix = SDL_clamp(ix, 0, BLUR_W - 1);
            for (sh = 0; sh < 24; sh += 8) {
                float a = (float)((cell[iy * BLUR_W + ix] >> sh) & 255);
                float b = (float)((cell[iy * BLUR_W + jx] >> sh) & 255);
                float c = (float)((cell[jy * BLUR_W + ix] >> sh) & 255);
                float d = (float)((cell[jy * BLUR_W + jx] >> sh) & 255);
                float v = (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty;

                out |= (uint32_t)(v + 0.5f) << sh;
            }
            big[cy * BLUR_OUT_W + cx] = out;
        }
    }
    SDL_UpdateTexture(sBlurTex, NULL, big, BLUR_OUT_W * 4);
    SDL_RenderTexture(sRenderer, sBlurTex, NULL, NULL);
}

/* Style 8 over the blurred reflection: a row whose sprites reach the edge
   as one colour for 24 pixels or more, or whose layers do for 80, is a bar
   (a title, the world warp's player bars) and is drawn on to the window's
   edge in that colour. */
static uint32_t sBandGround; /* style 9: the ground colour, 0 otherwise */
static void DetectLetterbox(const SDL_FRect* src, int* top, int* bottom);

/* d is c darkened (the GBA's brightness fade): every channel lower by the
   same proportion, within rounding */
static int DimmedOf(uint32_t d, uint32_t c) {
    int sh, f = -1;
    if ((d & 0xFFFFFF) == (c & 0xFFFFFF))
        return 0;
    for (sh = 0; sh <= 16; sh += 8) {
        int cc = (int)((c >> sh) & 255), dd = (int)((d >> sh) & 255);
        if (dd > cc) return 0;
        if (cc >= 48) {
            int r = dd * 64 / cc;
            if (f < 0) f = r;
            else if (r < f - 3 || r > f + 3) return 0;
        } else if (dd > cc / 2 + 8 && cc - dd > 24) {
            return 0;
        }
    }
    return f >= 8 && f < 62;
}

/* Where GBA row (or column) r starts on the window, exactly as the picture's
   own sampling puts it: a window pixel shows the GBA row under its centre,
   so at a 4.5 scale rows alternate 5 and 4 pixels from the top. Margin rows
   drawn with these edges line up pixel for pixel with the picture's. */
static float PixelEdge(float origin, int r, float scale) {
    return SDL_floorf(origin + r * scale - 0.5f) + 1.0f;
}

static void DrawEdgeBands(const SDL_FRect* src, const SDL_FRect* dst, int outW, int minBlock) {
    static uint8_t* mask;
    if (!mask && !(mask = (uint8_t*)SDL_calloc((size_t)PORT_MAX_W * PORT_MAX_H, 1)))
        return;
    extern void PortPpuSpriteMask(uint8_t* out);
    int side, y;
    float scale = dst->w / src->w;
    static uint8_t bars[2][GBA_SCREEN_H];
    static uint32_t colors[2][GBA_SCREEN_H];
    static int runs[2][GBA_SCREEN_H];
    int lbTop = 0, lbBottom = GBA_SCREEN_H;
    PortPpuSpriteMask(mask); /* bars are sprites; the scenery's own runs are not bars */
    {
        extern int PortNarrowEdgeKind(int side, int y);
        if (PortNarrowEdgeKind(0, 0) < 0)
            DetectLetterbox(src, &lbTop, &lbBottom);
    }
    for (side = 0; side < 2; side++) {
        int edge = side ? (int)(src->x + src->w) - 1 : (int)src->x, step = side ? -1 : 1;
        uint8_t* bar = bars[side];
        uint32_t* color = colors[side];
        for (y = 0; y < GBA_SCREEN_H; y++) {
            const uint32_t* row = &sComposite[((int)src->y + y) * gPortScreen.width];
            uint32_t c = row[edge];
            int k, run = 0, sprites = 0;
            for (k = 0; k < 120 && row[edge + k * step] == c; k++) {
                run++;
                sprites += mask[((int)src->y + y) * gPortScreen.width + edge + k * step] != 0;
            }
            /* the same bar dimmed by the scene (the world warp darkens its
               middle box over the footer) carries on the run */
            if (k < 120 && run > 0 && DimmedOf(row[edge + k * step], c)) {
                uint32_t dim = row[edge + k * step];
                for (; k < 120 && row[edge + k * step] == dim; k++)
                    run++;
            }
            color[y] = c;
            runs[side][y] = run;
            /* a sprite bar, or a BG bar far longer than any run of scenery;
               away from the frame's top and bottom bars (often drawn on the
               scenery layer), a run of the scenery layer itself in a
               GBA-size scene is not a bar (Bonus élite's black gaps) */
            {
                extern int PortNarrowEdgeKind(int side, int y);
                int frame = y < 24 || y >= GBA_SCREEN_H - 24, kind = PortNarrowEdgeKind(side, y);
                bar[y] = (sprites >= 24 && sprites == run) ||
                         (run >= 80 && (frame || kind != 0));
                /* outside the menus (a movie) only the letterbox is a bar,
                   never the video: the rows from the picture's top and
                   bottom edges in its colour, compression noise and all */
                if (kind < 0 && !(sprites >= 24 && sprites == run))
                    bar[y] = y < lbTop || y >= lbBottom;
            }
        }
    }
    /* the frame's top and bottom bars run across the whole screen: in the
       top and bottom 24 rows, a row that is a bar at one edge is one at the
       other where that edge shows the same colour (the world warp's footer,
       cut short on the right by the player badge) */
    for (y = 0; y < GBA_SCREEN_H; y++) {
        if (y >= 24 && y < GBA_SCREEN_H - 24)
            continue;
        /* style 11 (the save files): a frame line both edges show in the
           same colour is the frame's, even where a box in front of it (the
           message window over the bottom bar) cuts both runs short */
        if (sSideStyle == 11 && !bars[0][y] && !bars[1][y] && colors[0][y] == colors[1][y] &&
            runs[0][y] >= 16 && runs[1][y] >= 16)
            bars[0][y] = bars[1][y] = 1;
        for (side = 0; side < 2; side++)
            if (bars[side][y] == 1 && !bars[!side][y] && colors[!side][y] == colors[side][y])
                bars[!side][y] = 4;
    }
    for (side = 0; side < 2; side++) {
        uint8_t* bar = bars[side];
        uint32_t* color = colors[side];
        for (y = 0; y < GBA_SCREEN_H; y++)
            if (bar[y] == 4) bar[y] = 1;
        /* rows of a bar's own colour next to it are the bar, even where its
           lettering (the world card banner's name) cuts their run short */
        for (y = 1; y < GBA_SCREEN_H; y++)
            if (!bar[y] && bar[y - 1] && color[y] == color[y - 1])
                bar[y] = 1;
        for (y = GBA_SCREEN_H - 2; y >= 0; y--)
            if (!bar[y] && bar[y + 1] && color[y] == color[y + 1])
                bar[y] = 1;
        /* a box's own border and shadow rows (up to three either side of a
           bar, not the ground's colour) belong to it, so it is drawn whole */
        if (sBandGround)
            for (y = 0; y < GBA_SCREEN_H; y++)
                if (bar[y] == 1) {
                    int k2;
                    for (k2 = 1; k2 <= 3 && y + k2 < GBA_SCREEN_H && !bar[y + k2] && color[y + k2] != sBandGround; k2++)
                        bar[y + k2] = 3;
                    for (k2 = 1; k2 <= 3 && y - k2 >= 0 && !bar[y - k2] && color[y - k2] != sBandGround; k2++)
                        bar[y - k2] = 3;
                }
        /* a bar's textured rows (gaps of up to four) belong to it; lone rows are scenery */
        for (y = 0; y < GBA_SCREEN_H; y++)
            if (!bar[y]) {
                int up, dn;
                for (up = 1; up <= 4 && y - up >= 0 && !bar[y - up]; up++) {}
                for (dn = 1; dn <= 4 && y + dn < GBA_SCREEN_H && !bar[y + dn]; dn++) {}
                if (up <= 4 && y - up >= 0 && dn <= 4 && y + dn < GBA_SCREEN_H)
                    bar[y] = 2;
            }
        for (y = 0; y < GBA_SCREEN_H; y++) {
            int a2 = y, b2 = y;
            if (!bar[y])
                continue;
            while (a2 > 0 && bar[a2 - 1]) a2--;
            while (b2 < GBA_SCREEN_H - 1 && bar[b2 + 1]) b2++;
            if (b2 - a2 + 1 >= minBlock) {
                uint32_t c = color[y];
                SDL_FRect r;
                r.x = side ? dst->x + dst->w : 0.0f;
                r.w = side ? (float)outW - r.x : dst->x;
                /* whole pixels: fractional rows would leave seams between them */
                r.y = PixelEdge(dst->y, y, scale);
                r.h = PixelEdge(dst->y, y + 1, scale) - r.y;
                SDL_SetRenderDrawColor(sRenderer, (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c, 255);
                SDL_RenderFillRect(sRenderer, &r);
            }
        }
    }
}

/* A movie's letterbox, baked into its 240x160 frames: the rows from the top
   (and from the bottom) whose every pixel is within 24 of the edge row's own
   uniform colour, compression noise and all. */
static void DetectLetterbox(const SDL_FRect* src, int* top, int* bottom) {
    int pass;
    *top = 0;
    *bottom = GBA_SCREEN_H;
    for (pass = 0; pass < 2; pass++) {
        int y0 = pass ? GBA_SCREEN_H - 1 : 0, dy = pass ? -1 : 1, yy2, x2;
        const uint32_t* r0 = &sComposite[((int)src->y + y0) * gPortScreen.width + (int)src->x];
        uint32_t lb = r0[0];
        for (x2 = 0; x2 < GBA_SCREEN_W && r0[x2] == lb; x2++) {}
        if (x2 < GBA_SCREEN_W)
            continue; /* no letterbox on this side */
        for (yy2 = y0; yy2 >= 0 && yy2 < GBA_SCREEN_H; yy2 += dy) {
            const uint32_t* r = &sComposite[((int)src->y + yy2) * gPortScreen.width + (int)src->x];
            for (x2 = 0; x2 < GBA_SCREEN_W; x2++) {
                int sh, far = 0;
                for (sh = 0; sh <= 16; sh += 8)
                    far |= SDL_abs((int)((r[x2] >> sh) & 255) - (int)((lb >> sh) & 255)) > 24;
                if (far) break;
            }
            if (x2 < GBA_SCREEN_W) break;
        }
        if (pass) *bottom = yy2 + 1; else *top = yy2;
    }
}

/* The movie's crop: its letterbox measured over the first frames of each
   movie (the widest bars seen, at most 24 rows a side) and then kept, so
   dark scenes never make the picture jump. */
static void MovieCrop(const SDL_FRect* src, int* top, int* bottom) {
    static uint32_t lastFrame;
    static int frames, cropTop, cropBottom, started;
    uint32_t now = PortFrameCount();
    int t, b;
    if (!started || now - lastFrame > 4) {
        started = 1;
        frames = 0;
        cropTop = 0;
        cropBottom = GBA_SCREEN_H;
    }
    lastFrame = now;
    if (frames < 120) {
        DetectLetterbox(src, &t, &b);
        if (t > 0 && t <= 24 && b < GBA_SCREEN_H && b >= GBA_SCREEN_H - 24) {
            if (t > cropTop) cropTop = t;
            if (b < cropBottom) cropBottom = b;
            frames++;
        }
    }
    *top = cropTop;
    *bottom = cropBottom;
}

/* The opening movie's skip prompt over a cropped movie: drawn on a copy of
   the picture and shown at the picture's normal full-height place and size
   (normal), exactly where it sits outside movies. */
static void DrawSkipLayer(const SDL_FRect* normal) {
    static uint32_t* buf;
    static SDL_Texture* tex;
    static int bufW, bufH;
    int r[4];
    float sc = normal->w / GBA_SCREEN_W;
    SDL_FRect src, dst;

    if (bufW != gPortScreen.width || bufH != gPortScreen.height) {
        SDL_free(buf);
        if (tex) SDL_DestroyTexture(tex);
        bufW = gPortScreen.width;
        bufH = gPortScreen.height;
        buf = (uint32_t*)SDL_malloc((size_t)bufW * bufH * 4);
        tex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, bufW, bufH);
        if (tex) SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_PIXELART);
    }
    if (!buf || !tex)
        return;
    memcpy(buf, sComposite, (size_t)bufW * bufH * 4);
    if (!PortUiDrawSkipLayer(buf, bufW, bufH, r))
        return;
    SDL_UpdateTexture(tex, NULL, buf, bufW * 4);
    src.x = (float)r[0]; src.y = (float)r[1]; src.w = (float)r[2]; src.h = (float)r[3];
    dst.x = normal->x + (r[0] - gPortScreen.originX) * sc;
    dst.y = normal->y + (r[1] - gPortScreen.originY) * sc;
    dst.w = r[2] * sc;
    dst.h = r[3] * sc;
    SDL_RenderTexture(sRenderer, tex, &src, &dst);
}

/* The title bar on the left margin: its own piece of the header asset.
   Where the header is transparent the picture shows the frame's bar behind
   it (the journal's dark green top row), so the margin shows that bar too. */
static void DrawHeaderLeft(const SDL_FRect* src, const SDL_FRect* dst) {
    int yy, xx, any = 0;
    float sc = dst->w / src->w;
    /* the header in the margin follows its sprite (slide in and out) */
    for (yy = 0; yy < 16 && !any; yy++)
        for (xx = 0; xx < (int)SDL_ceilf(dst->x / sc) && !any; xx++)
            any = PortMenuHeaderPixelAt(yy, xx) != 0;
    if (!any)
        return;
    for (yy = 0; yy < 16; yy++) {
        const uint32_t* row = &sComposite[((int)src->y + yy) * gPortScreen.width + (int)src->x];
        uint32_t behind = 0;
        int k;
        for (k = 0; k < 120 && row[k] == row[0]; k++) {}
        if (k >= 80)
            behind = row[0];
        for (xx = 0; xx < (int)SDL_ceilf(dst->x / sc); xx++) {
            uint32_t h = PortMenuHeaderPixelAt(yy, xx);
            SDL_FRect r;
            if (!h) h = behind;
            if (!h) continue;
            r.x = PixelEdge(dst->x, -(xx + 1), sc);
            r.w = PixelEdge(dst->x, -xx, sc) - r.x;
            r.y = PixelEdge(dst->y, yy, sc);
            r.h = PixelEdge(dst->y, yy + 1, sc) - r.y;
            SDL_SetRenderDrawColor(sRenderer, (Uint8)(h >> 16), (Uint8)(h >> 8), (Uint8)h, 255);
            SDL_RenderFillRect(sRenderer, &r);
        }
    }
}

/* Menu headers: on the cartridge the title's arrow stops one pixel short of
   the line the screen draws on from its tip, leaving a one-pixel notch of
   background between them. The user asked for them to meet, so that pixel
   takes the line's colour: a non-white pixel right after a white outline
   pixel, followed by a thin line (two rows at most, so never the title's
   band around its letters) of one colour 24 pixels long. */
static void JoinHeaderLine(void) {
    int y, x, k;
    if (gPortScreen.wide) /* only the GBA-size menu scenes */
        return;
    for (y = 0; y < 16 && gPortScreen.originY + y < gPortScreen.height; y++) {
        uint32_t* row = &sComposite[(gPortScreen.originY + y) * gPortScreen.width + gPortScreen.originX];
        for (x = 8; x + 26 < GBA_SCREEN_W; x++) {
            uint32_t line = row[x + 2];
            if ((row[x] & 0xFFFFFF) != 0xFFFFFF || (row[x + 1] & 0xFFFFFF) == 0xFFFFFF || row[x + 1] == line)
                continue;
            for (k = 2; k < 26 && row[x + k] == line; k++) {}
            if (k < 26)
                continue;
            /* a thin line (two rows at most), not an area of that colour */
            if ((y >= 2 && row[x + 5 - 2 * gPortScreen.width] == line) ||
                (y + 2 < GBA_SCREEN_H && row[x + 5 + 2 * gPortScreen.width] == line))
                continue;
            row[x + 1] = line;
            break;
        }
    }
}

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
/* ffmpeg for KHCOM_RECORD, started with no console window (the player may
   be playing in another window) and fed through a pipe */
static HANDLE sRecordProcess;
static FILE* OpenRecordPipe(const char* cmdline) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE rd, wr, nul;
    char buf[1024];
    int fd;
    if (!CreatePipe(&rd, &wr, &sa, 1 << 20))
        return NULL;
    SetHandleInformation(wr, HANDLE_FLAG_INHERIT, 0);
    nul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    SDL_zero(si);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = rd;
    si.hStdOutput = nul;
    si.hStdError = nul;
    SDL_strlcpy(buf, cmdline, sizeof(buf));
    if (!CreateProcessA(NULL, buf, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(rd);
        CloseHandle(wr);
        CloseHandle(nul);
        return NULL;
    }
    CloseHandle(pi.hThread);
    CloseHandle(rd);
    CloseHandle(nul);
    sRecordProcess = pi.hProcess;
    fd = _open_osfhandle((intptr_t)wr, _O_BINARY);
    return fd >= 0 ? _fdopen(fd, "wb") : NULL;
}
static void CloseRecordPipe(FILE* f) {
    fclose(f);
    if (sRecordProcess) {
        WaitForSingleObject(sRecordProcess, 60000);
        CloseHandle(sRecordProcess);
        sRecordProcess = NULL;
    }
}
#else
static FILE* OpenRecordPipe(const char* cmdline) { return popen(cmdline, "w"); }
static void CloseRecordPipe(FILE* f) { pclose(f); }
#endif

static void Present(void) {
    int outW, outH;
    SDL_FRect dst, normal;
    uint32_t uniform = 0;
    float worldX, worldY, worldScale;
    int moviePrompt;
    /* Snapshot the phase before PortUiDraw advances the fade: canvas, game
     * layers and presentation must all use the same phase for this frame. */
    int menuCanvas = PortUiMenuCanvas();
    int sceneFade = PortUiSceneFade();
    void PortHdHudPrepare(SDL_Renderer* renderer);
    void PortHdHudDraw(SDL_Renderer* renderer, int width, int height);
    void PortHdHudDrawLabel(SDL_Renderer* renderer, int width, int height);
    void PortHdHudDrawBar(SDL_Renderer* renderer, int width, int height);
    void PortHdHudDrawSprites(SDL_Renderer* renderer, int width, int height);
    void PortHdTextHudDigits(SDL_Renderer* renderer, float originX, float originY, float scale);

    EnsureTexture();
    GrowPixelBuffer(&sCompositeBuf, &sCompositeCapacity, (size_t)gPortScreen.width * gPortScreen.height);
    PortHdHudPrepare(sRenderer);
    /* movies shown cropped: the skip prompt is drawn after the movie, on its
       own, so it neither grows with the video nor shifts the crop */
    moviePrompt = sSideStyle == 10 && sFitScale <= 0 && !menuCanvas && !gPortScreen.wide
                  && gPortConfig.narrowFit && (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H);
    PortUiSkipOwnLayer(moviePrompt);
    /* Menus and paused frames require an independent presentation buffer. */
    {
        extern int PortUiDrawsOnFrame(void);
        if (PortUiDrawsOnFrame() || !gPortScreen.wide) {
            sComposite = sCompositeBuf;
            memcpy(sComposite, gPortFrame, (size_t)gPortScreen.width * gPortScreen.height * 4);
            JoinHeaderLine();
        } else {
            sComposite = gPortFrame; /* nothing is drawn over it: PortUiDraw only steps its fades */
        }
    }
    PortUiDraw(sComposite, gPortScreen.width, gPortScreen.height);
    SDL_UpdateTexture(sTexture, NULL, sComposite, gPortScreen.width * 4);
    SDL_GetCurrentRenderOutputSize(sRenderer, &outW, &outH);
    SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sRenderer);
    {
        /* Aspect ratio option: draw into the chosen rectangle; the rest of
           the window stays black */
        SDL_Rect area = AspectRect(outW, outH);
        SDL_SetRenderViewport(sRenderer, area.x || area.y || area.w != outW || area.h != outH ? &area : NULL);
        outW = area.w;
        outH = area.h;
    }
    if ((sFitScale > 0 || (!gPortScreen.wide && gPortConfig.narrowFit)) && !menuCanvas
        && (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H)) {
        /* Scene without a wide view: show the GBA picture at full height
           (bars only at the sides), sampled with the pixel-art filter. */
        SDL_FRect src = { (float)gPortScreen.originX, (float)gPortScreen.originY, GBA_SCREEN_W, GBA_SCREEN_H };
        float scale = CoverActive() ? SDL_max(outW / (float)GBA_SCREEN_W, outH / (float)GBA_SCREEN_H)
                      : sSideStyle == 10 ? SDL_min(outW / (float)GBA_SCREEN_W, outH / (float)GBA_SCREEN_H)
                                         : MenuScale(outW, outH);
        dst.w = GBA_SCREEN_W * scale;
        dst.h = GBA_SCREEN_H * scale;
        dst.x = (float)(int)((outW - dst.w) / 2);
        dst.y = (float)(int)((outH - dst.h) / 2);
        normal = dst;
        if (sSideStyle == 10 && sFitScale <= 0 && !menuCanvas) {
            /* movies: only the video, without its letterbox, as large as the
               window allows at the video's own proportions */
            int top, bottom;
            MovieCrop(&src, &top, &bottom);
            if (bottom - top >= 64) {
                src.y += top;
                src.h = (float)(bottom - top);
                scale = SDL_min(outW / src.w, outH / src.h);
                dst.w = src.w * scale;
                dst.h = src.h * scale;
                dst.x = (float)(int)((outW - dst.w) / 2);
                dst.y = (float)(int)((outH - dst.h) / 2);
            }
        }
        if ((sSideStyle == 3 || sSideStyle == 8 || sSideStyle == 10 || sFitScale > 0) && !menuCanvas)
            DrawBlurredReflection(&src, outW, outH);
        SDL_SetTextureScaleMode(sTexture, gPortConfig.filter == PORT_FILTER_LINEAR ? (SDL_ScaleMode)PortSmoothScaleMode()
                                                                                 : SDL_SCALEMODE_PIXELART);
        SDL_RenderTexture(sRenderer, sTexture, &src, &dst);
        if (moviePrompt)
            DrawSkipLayer(&normal);
        if (!menuCanvas && sSideStyle == 8 && sFitScale <= 0) {
            DrawEdgeBands(&src, &dst, outW, 3);
            DrawHeaderLeft(&src, &dst);
        }
        if (!menuCanvas && (sSideStyle == 9 || sSideStyle == 12) && sFitScale <= 0) {
            /* style 9: the scene's ground (its commonest colour) and its thick
               bars (blocks of six rows or more, so a frame's thin border is
               not one) carried on; nothing mirrored, nothing repeated */
            static uint32_t colors[64];
            static int counts[64];
            int n = 0, best = 0, i, gx, gy2;
            SDL_FRect left = { 0.0f, dst.y, dst.x, dst.h }, right = { dst.x + dst.w, dst.y, (float)outW - dst.x - dst.w, dst.h };
            /* the ground: the commonest colour along the picture's two outer edges */
            for (gy2 = 16; gy2 < GBA_SCREEN_H - 8; gy2++)
                for (gx = 0; gx < 16; gx++) {
                    int col = gx < 8 ? gx : GBA_SCREEN_W - 16 + gx;
                    uint32_t c = sComposite[((int)src.y + gy2) * gPortScreen.width + (int)src.x + col];
                    for (i = 0; i < n && colors[i] != c; i++) {}
                    if (i == n) {
                        if (n == 64) continue;
                        colors[n] = c;
                        counts[n++] = 0;
                    }
                    if (++counts[i] > counts[best]) best = i;
                }
            if (n && sSideStyle == 9) {
                uint32_t g = colors[best];
                SDL_SetRenderDrawColor(sRenderer, (Uint8)(g >> 16), (Uint8)(g >> 8), (Uint8)g, 255);
                SDL_RenderFillRect(sRenderer, &left);
                SDL_RenderFillRect(sRenderer, &right);
            } else if (n) {
                /* style 12, each side's own ground (23/10, the journal's Story page): a page cut by the GBA's edge carries on
                   (its edge columns are its inside's colour); a thin border before a page is the ground behind it
                   (the green paper left of the Story page); a sheet's shaded edge (a gradient: four colours or more
                   in its last twelve columns) gives way to the other side's ground, as the sheet ends there */
                uint32_t g = colors[best], side[2];
                int kind[2], s2;
                for (s2 = 0; s2 < 2; s2++) {
                    int edgeCol = s2 ? GBA_SCREEN_W - 1 : 0, innerCol = s2 ? GBA_SCREEN_W - 12 : 11, grad = 0, rows = 0;
                    uint32_t ec = 0, ic = 0;
                    static uint32_t ecs[2][GBA_SCREEN_H];
                    int y3, same = 0;
                    for (y3 = 24; y3 < GBA_SCREEN_H - 24; y3++) {
                        const uint32_t* row = &sComposite[((int)src.y + y3) * gPortScreen.width + (int)src.x];
                        uint32_t seen[12];
                        int ns = 0, k2, j2;
                        for (k2 = 0; k2 < 12; k2++) {
                            uint32_t c = row[s2 ? GBA_SCREEN_W - 1 - k2 : k2];
                            for (j2 = 0; j2 < ns && seen[j2] != c; j2++) {}
                            if (j2 == ns) seen[ns++] = c;
                        }
                        grad += ns >= 4;
                        rows++;
                        ecs[s2][y3] = row[edgeCol];
                        same += row[edgeCol] == row[innerCol];
                        ec = row[edgeCol];
                        ic = row[innerCol];
                    }
                    (void)ic;
                    {
                        /* the edge column's commonest colour */
                        int cnt = 0, bestCnt = 0, y4, y5;
                        for (y4 = 24; y4 < GBA_SCREEN_H - 24; y4++) {
                            cnt = 0;
                            for (y5 = 24; y5 < GBA_SCREEN_H - 24; y5++) cnt += ecs[s2][y5] == ecs[s2][y4];
                            if (cnt > bestCnt) { bestCnt = cnt; ec = ecs[s2][y4]; }
                        }
                    }
                    side[s2] = ec;
                    kind[s2] = same * 2 >= rows ? 0 : grad * 2 >= rows ? 2 : 1; /* 0 page, 2 shaded sheet, 1 border */
                }
                for (s2 = 0; s2 < 2; s2++) {
                    uint32_t c = kind[s2] == 2 ? (kind[!s2] != 2 ? side[!s2] : g) : side[s2];
                    SDL_SetRenderDrawColor(sRenderer, (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c, 255);
                    SDL_RenderFillRect(sRenderer, s2 ? &right : &left);
                }
            }
            sBandGround = n ? colors[best] : 0;
            DrawEdgeBands(&src, &dst, outW, 6);
            sBandGround = 0;
            /* the frame's top and bottom bars run the whole width: where a
               panel covers one end on the GBA (the journal's notebook over
               the footer), the bar seen at the other end carries on there */
            {
                int yy, k;
                float sc = dst.w / src.w;
                for (yy = 0; yy < GBA_SCREEN_H; yy++) {
                    const uint32_t* row = &sComposite[((int)src.y + yy) * gPortScreen.width + (int)src.x];
                    uint32_t lc = row[0], rc = row[GBA_SCREEN_W - 1];
                    int lrun = 0, rrun = 0;
                    if (yy >= 24 && yy < GBA_SCREEN_H - 24)
                        continue;
                    for (k = 0; k < 120 && row[k] == lc; k++) lrun++;
                    for (k = 0; k < 120 && row[GBA_SCREEN_W - 1 - k] == rc; k++) rrun++;
                    /* not over a bar that reaches the edge itself (the journal's yellow PAGE bar, 23/10) */
                    if (lrun >= 80 && rrun < 16) {
                        SDL_FRect r = { dst.x + dst.w, PixelEdge(dst.y, yy, sc), (float)outW - dst.x - dst.w, 0 };
                        r.h = PixelEdge(dst.y, yy + 1, sc) - r.y;
                        SDL_SetRenderDrawColor(sRenderer, (Uint8)(lc >> 16), (Uint8)(lc >> 8), (Uint8)lc, 255);
                        SDL_RenderFillRect(sRenderer, &r);
                    } else if (rrun >= 80 && lrun < 16) {
                        SDL_FRect r = { 0, PixelEdge(dst.y, yy, sc), dst.x, 0 };
                        r.h = PixelEdge(dst.y, yy + 1, sc) - r.y;
                        SDL_SetRenderDrawColor(sRenderer, (Uint8)(rc >> 16), (Uint8)(rc >> 8), (Uint8)rc, 255);
                        SDL_RenderFillRect(sRenderer, &r);
                    }
                }
            }
            DrawHeaderLeft(&src, &dst);
        }
        if (SDL_getenv("KHCOM_DBGSIDE")) PortLog("side f%u style %d fit %.2f wide %d src %.0f,%.0f %.0fx%.0f", (unsigned)PortFrameCount(), sSideStyle, sFitScale, gPortScreen.wide, src.x, src.y, src.w, src.h);
        if (!menuCanvas && sSideStyle != 10 && sFitScale <= 0 && UniformPicture(&src, &uniform)) {
            /* a fade has reached one flat colour (or the screen is blank
               between modes): the margins are that colour too, so the
               transition stays one smooth fade */
            SDL_FRect a = { 0, dst.y, dst.x, dst.h }, b2 = { dst.x + dst.w, dst.y, outW - dst.x - dst.w, dst.h };
            SDL_SetRenderDrawColor(sRenderer, (Uint8)(uniform >> 16), (Uint8)(uniform >> 8), (Uint8)uniform, 255);
            SDL_RenderFillRect(sRenderer, &a);
            SDL_RenderFillRect(sRenderer, &b2);
        } else if (!menuCanvas && sSideStyle != 3 && sSideStyle != 8 && sSideStyle != 9 && sSideStyle != 10 &&
                   sSideStyle != 12 && sFitScale <= 0) {
            SideBar((int)src.x, (int)src.y, -1, dst.y, scale, 0.0f, dst.x, (int)src.x);
            SideBar((int)(src.x + src.w) - SIDE_STRIP, (int)src.y, 1, dst.y, scale, dst.x + dst.w, (float)outW,
                    (int)(src.x + src.w) - 1);
            /* style 11 (the save files): the frame's bars and title band
               carried on over the scenery's own margin */
            if (sSideStyle == 11) {
                DrawEdgeBands(&src, &dst, outW, 3);
                DrawHeaderLeft(&src, &dst);
            }
        }
        /* Sprite-only cutscene messages have a plain authored backdrop.
           Sample the rendered background (including fades), not a stale
           side style or palette from the preceding scene. */
        if (!(IO16(0) & 0x0F00) && !menuCanvas) {
            uint32_t c = gPortMenuBackground[gPortScreen.originY * gPortScreen.width + gPortScreen.originX];
            SDL_FRect margins[4] = {
                {0, 0, dst.x, (float)outH},
                {dst.x + dst.w, 0, outW - dst.x - dst.w, (float)outH},
                {dst.x, 0, dst.w, dst.y},
                {dst.x, dst.y + dst.h, dst.w, outH - dst.y - dst.h}
            };
            int i;
            SDL_SetRenderDrawColor(sRenderer, (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c, 255);
            for (i = 0; i < 4; i++) if (margins[i].w > 0 && margins[i].h > 0)
                SDL_RenderFillRect(sRenderer, &margins[i]);
        }
        worldScale = scale;
        worldX = dst.x - src.x * scale;
        worldY = dst.y - src.y * scale;
    } else if (PortHd2dActive() && !menuCanvas) {
        /* HD-2D build: the processed, larger picture, filtered to the window */
        static uint8_t* sMask;
        if (!sMask)
            sMask = (uint8_t*)SDL_calloc((size_t)PORT_MAX_W * PORT_MAX_H, 1);
        static SDL_Texture* sHd2dTex;
        static int sHd2dW, sHd2dH;
        extern void PortPpuSpriteMask(uint8_t* out);
        int hw, hh;
        const uint32_t* pic;

        PortPpuSpriteMask(sMask);
        pic = PortHd2dProcess(gPortFrame, sMask, gPortScreen.width, gPortScreen.height, &hw, &hh);
        if (sHd2dTex == NULL || sHd2dW != hw || sHd2dH != hh) {
            if (sHd2dTex != NULL)
                SDL_DestroyTexture(sHd2dTex);
            sHd2dW = hw;
            sHd2dH = hh;
            sHd2dTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, hw, hh);
            SDL_SetTextureScaleMode(sHd2dTex, SDL_SCALEMODE_LINEAR);
        }
        SDL_UpdateTexture(sHd2dTex, NULL, pic, hw * 4);
        OutputRect(outW, outH, &dst);
        SDL_RenderTexture(sRenderer, sHd2dTex, NULL, &dst);
        worldScale = dst.w / gPortScreen.width;
        worldX = dst.x;
        worldY = dst.y;
    } else {
        OutputRect(outW, outH, &dst);
        if (sFitScale > 0 && !menuCanvas) {
            /* Battle-entry animation stays at its authored aspect ratio.
               Fill the surrounding window with the same dim blur as movies. */
            SDL_FRect backdrop = { (float)gPortScreen.originX,
                                   (float)gPortScreen.originY,
                                   GBA_SCREEN_W, GBA_SCREEN_H };
            DrawBlurredReflection(&backdrop, outW, outH);
        }
        DrawClassicBorder(&dst);
        SDL_RenderTexture(sRenderer, sTexture, NULL, &dst);
        worldScale = dst.w / gPortScreen.width;
        worldX = dst.x;
        worldY = dst.y;
    }
    if (!menuCanvas) {
        /* HD sprites of the game's picture (the title's logo), with the game's fade */
        uint16_t bld = *(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x50);
        int effect = (bld >> 6) & 3, amount = SDL_min(*(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x54) & 31, 16);
        PortHdSpritesDraw(sRenderer, 0, worldX, worldY, worldScale, (bld & 0x10) && effect == 3 ? amount : 0,
                          (bld & 0x10) && effect == 2 ? amount : 0);
        {
            const uint32_t* over = PortPpuMainOver();
            if (over) {
                SDL_FRect od = { worldX, worldY, gPortScreen.width * worldScale, gPortScreen.height * worldScale };
                if (sMainOverTexture == NULL || sMainOverW != gPortScreen.width || sMainOverH != gPortScreen.height) {
                    if (sMainOverTexture != NULL)
                        SDL_DestroyTexture(sMainOverTexture);
                    sMainOverW = gPortScreen.width;
                    sMainOverH = gPortScreen.height;
                    sMainOverTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888,
                                                         SDL_TEXTUREACCESS_STREAMING, sMainOverW, sMainOverH);
                    SDL_SetTextureBlendMode(sMainOverTexture, SDL_BLENDMODE_BLEND);
                }
                if (sMainOverTexture != NULL) {
                    SDL_SetTextureScaleMode(sMainOverTexture, sPictureScaleMode); /* the picture's own scaling */
                    SDL_UpdateTexture(sMainOverTexture, NULL, over, sMainOverW * 4);
                    SDL_RenderTexture(sRenderer, sMainOverTexture, NULL, &od);
                }
            }
        }
        PortHdTextDraw(sRenderer, 0, worldX, worldY, worldScale);
    }
    /* the options menu covers everything: no layer is drawn over it */
    if (PortPpuOverlayActive() && sOverlayScale > 0 && !menuCanvas) {
        SDL_FRect od;
        if (sOverlayTex == NULL || sOverlayTexW != gPortOverlayW) {
            if (sOverlayTex != NULL)
                SDL_DestroyTexture(sOverlayTex);
            sOverlayTexW = gPortOverlayW;
            sOverlayTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                            sOverlayTexW, GBA_SCREEN_H);
            SDL_SetTextureBlendMode(sOverlayTex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(sOverlayTex, SDL_SCALEMODE_PIXELART);
        }
        SDL_UpdateTexture(sOverlayTex, NULL, gPortOverlay, sOverlayTexW * 4);
        od.w = sOverlayTexW * sOverlayScale;
        od.h = GBA_SCREEN_H * sOverlayScale;
        od.x = (float)(int)((outW - od.w) / 2);
        od.y = (float)(int)((outH - od.h) / 2);
        SDL_RenderTexture(sRenderer, sOverlayTex, NULL, &od);
        PortHdSpritesDraw(sRenderer, 2, od.x, od.y, sOverlayScale, 0, 0); /* menus' HD parts, the title's logo */
        if (PortPpuScreenOver()) {
            /* what the menu draws in front of its HD parts */
            if (sScreenOverTex == NULL || sScreenOverTexW != gPortOverlayW) {
                if (sScreenOverTex != NULL)
                    SDL_DestroyTexture(sScreenOverTex);
                sScreenOverTexW = gPortOverlayW;
                sScreenOverTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                                   sScreenOverTexW, GBA_SCREEN_H);
                SDL_SetTextureBlendMode(sScreenOverTex, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(sScreenOverTex, SDL_SCALEMODE_PIXELART);
            }
            if (sScreenOverTex != NULL) {
                SDL_UpdateTexture(sScreenOverTex, NULL, PortPpuScreenOver(), sScreenOverTexW * 4);
                SDL_RenderTexture(sRenderer, sScreenOverTex, NULL, &od);
            }
        }
        if (!menuCanvas)
            PortHdTextDraw(sRenderer, 2, od.x, od.y, sOverlayScale);
    }
    if (PortPpuHudActive() && !menuCanvas) {
        SDL_FRect hd;
        if (sHudTexture == NULL || sHudTexW != gPortHudW || sHudTexH != gPortHudH) {
            if (sHudTexture != NULL)
                SDL_DestroyTexture(sHudTexture);
            sHudTexW = gPortHudW;
            sHudTexH = gPortHudH;
            sHudTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                            sHudTexW, sHudTexH);
            SDL_SetTextureBlendMode(sHudTexture, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(sHudTexture, SDL_SCALEMODE_NEAREST);
        }
        GrowPixelBuffer(&sHudBuf, &sHudCapacity, (size_t)gPortHudW * gPortHudH);
        PortPpuRenderHud(sHudBuf);
        SDL_UpdateTexture(sHudTexture, NULL, sHudBuf, sHudTexW * 4);
        hd.w = (float)(sHudTexW * gPortHudScale);
        hd.h = (float)(sHudTexH * gPortHudScale);
        hd.x = (float)((outW - (int)hd.w) / 2);
        hd.y = (float)((outH - (int)hd.h) / 2);
        PortHdHudDraw(sRenderer, outW, outH);
        SDL_RenderTexture(sRenderer, sHudTexture, NULL, &hd);
        {
            /* HD sprites (cards), then what the cartridge draws in front of them */
            uint16_t bld = *(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x50);
            int effect = (bld >> 6) & 3, amount = SDL_min(*(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x54) & 31, 16);
            const uint32_t* over = PortPpuHudOver();
            PortHdSpritesDraw(sRenderer, 1, hd.x, hd.y, (float)gPortHudScale,
                              (bld & 0x10) && effect == 3 ? amount : 0, (bld & 0x10) && effect == 2 ? amount : 0);
            if (over) {
                if (sHudOverTexture == NULL || sHudOverW != sHudTexW || sHudOverH != sHudTexH) {
                    if (sHudOverTexture != NULL)
                        SDL_DestroyTexture(sHudOverTexture);
                    sHudOverW = sHudTexW;
                    sHudOverH = sHudTexH;
                    sHudOverTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888,
                                                        SDL_TEXTUREACCESS_STREAMING, sHudOverW, sHudOverH);
                    SDL_SetTextureBlendMode(sHudOverTexture, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureScaleMode(sHudOverTexture, SDL_SCALEMODE_NEAREST);
                }
                if (sHudOverTexture != NULL) {
                    SDL_UpdateTexture(sHudOverTexture, NULL, over, sHudOverW * 4);
                    SDL_RenderTexture(sRenderer, sHudOverTexture, NULL, &hd);
                }
            }
        }
        PortHdHudDrawSprites(sRenderer, outW, outH);
        PortHdHudDrawBar(sRenderer, outW, outH);
        PortHdHudDrawLabel(sRenderer, outW, outH);
        if (!menuCanvas) PortHdTextHudDigits(sRenderer, hd.x, hd.y, (float)gPortHudScale);
        PortHdTextDraw(sRenderer, 1, hd.x, hd.y, (float)gPortHudScale);
    }
    {
        /* the scene pause's panel above every HD layer (it is drawn on the picture, under them) */
        extern int PortUiScenePauseRect(int r[4]);
        int r[4];
        if (!menuCanvas && PortUiScenePauseRect(r)) {
            SDL_FRect ps = { (float)r[0], (float)r[1], (float)r[2], (float)r[3] };
            SDL_FRect pd = { worldX + r[0] * worldScale, worldY + r[1] * worldScale, r[2] * worldScale, r[3] * worldScale };
            SDL_RenderTexture(sRenderer, sTexture, &ps, &pd);
        }
    }
    DrawScanlines(&dst, outW);
    {
        /* state / slot notices: topmost, at the window's scale */
        static uint32_t sNoticeBuf[256 * 14];
        static SDL_Texture* sNoticeTex;
        int nw;

        memset(sNoticeBuf, 0, sizeof(sNoticeBuf));
        PortUiSetFps(sFps, sLogicFps);
        nw = PortUiDrawNotice(sNoticeBuf, 256, 14);
        if (nw > 0) {
            float f = SDL_max(1.0f, SDL_floorf(outH / 360.0f)); /* about a sixth of the old size at 1080p: x3 */
            SDL_FRect src = { 0, 0, (float)nw, 14 };
            SDL_FRect nd = { 4 * f, 4 * f, nw * f, 14 * f };

            if (sNoticeTex == NULL) {
                sNoticeTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 14);
                if (sNoticeTex != NULL) {
                    SDL_SetTextureBlendMode(sNoticeTex, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureScaleMode(sNoticeTex, SDL_SCALEMODE_PIXELART);
                }
            }
            if (sNoticeTex != NULL) {
                SDL_UpdateTexture(sNoticeTex, NULL, sNoticeBuf, 256 * 4);
                SDL_RenderTexture(sRenderer, sNoticeTex, &src, &nd);
            }
        }
    }
    SDL_SetRenderViewport(sRenderer, NULL);
    {
        int fullW = 0, fullH = 0;
        SDL_GetCurrentRenderOutputSize(sRenderer, &fullW, &fullH);
        PortAchievementToastDraw(sRenderer, fullW, fullH);
    }
    if (sceneFade > 0) {
        /* Fade the final scene rather than its world buffer alone. This keeps
         * GBA-fit scenes, movies, separate HUD and HD text at their real size. */
        SDL_SetRenderDrawBlendMode(sRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, (Uint8)sceneFade);
        SDL_RenderFillRect(sRenderer, NULL);
        SDL_SetRenderDrawBlendMode(sRenderer, SDL_BLENDMODE_NONE);
    }
    {
        /* KHCOM_RECORD="from-to:file.mov": frames from..to of the window piped
           straight to ffmpeg (KHCOM_FFMPEG, else ffmpeg on the PATH) as ProRes,
           one frame at a time, so a recording holds no frames in memory */
        static FILE* pipe;
        static unsigned recFrom, recTo;
        static int recState; /* 0 unparsed, 1 waiting, 2 recording, 3 done */
        const char* rec = SDL_getenv("KHCOM_RECORD");
        if (rec && recState == 0) {
            recState = SDL_sscanf(rec, "%u-%u:", &recFrom, &recTo) == 2 && SDL_strchr(rec, ':') ? 1 : 3;
        }
        if (recState == 1 && sFrameCount >= recFrom) {
            char cmd[1024];
            const char* ff = SDL_getenv("KHCOM_FFMPEG");
            int fullW = 0, fullH = 0;
            SDL_GetCurrentRenderOutputSize(sRenderer, &fullW, &fullH); /* the whole window, bars included */
            SDL_snprintf(cmd, sizeof(cmd),
                         "\"%s\" -loglevel error -y -f rawvideo -pix_fmt bgra -s %dx%d -r 60 -i - "
                         "-c:v prores_ks -profile:v 3 -pix_fmt yuv422p10le -threads 2 \"%s\"",
                         ff ? ff : "ffmpeg", fullW, fullH, SDL_strchr(rec, ':') + 1);
            pipe = OpenRecordPipe(cmd);
            recState = pipe ? 2 : 3;
            PortLog("record: %s", pipe ? "started" : "ffmpeg not found");
        }
        if (recState == 2) {
            SDL_Surface* s = SDL_RenderReadPixels(sRenderer, NULL);
            SDL_Surface* c = s ? SDL_ConvertSurface(s, SDL_PIXELFORMAT_BGRA32) : NULL;
            if (c) {
                int y;
                for (y = 0; y < c->h; y++)
                    fwrite((const uint8_t*)c->pixels + (size_t)y * c->pitch, 4, (size_t)c->w, pipe);
            }
            SDL_DestroySurface(c);
            SDL_DestroySurface(s);
            if (sFrameCount >= recTo) {
                CloseRecordPipe(pipe);
                pipe = NULL;
                recState = 3;
                PortLog("record: done");
            }
        }
    }
    {
        /* KHCOM_OUTPUT_SHOT=frames: save the presented window image; a window of ours asks for one with
           PortHarnessShot (the game's frames stand still under it). */
        const char* out = SDL_getenv("KHCOM_OUTPUT_SHOT");
        if (sHarnessShot[0] || (out && HarnessListHas(out, sFrameCount))) {
            SDL_Surface* s;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
            if (SDL_getenv("KHCOM_SHOT_TOUCH")) /* the touch controls in the shot too (their look, test_touch_themes) */
                PortAndroidTouchDraw(sRenderer);
#endif
            s = SDL_RenderReadPixels(sRenderer, NULL);
            char name[64];
            if (sHarnessShot[0])
                SDL_strlcpy(name, sHarnessShot, sizeof(name));
            else
                SDL_snprintf(name, sizeof(name), "out_%05u.bmp", (unsigned)sFrameCount);
            sHarnessShot[0] = 0;
            if (s) {
                SDL_SaveBMP(s, PortConfigPath(name));
                SDL_DestroySurface(s);
            }
        }
    }
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
    PortAndroidTouchDraw(sRenderer);
#endif
#ifdef PORT_LINUX
    PortFbPresent(sRenderer);
#endif
    {
        /* KHCOM_PRESENTSHOTS=a-b (harness): the window as shown, frames a..b (present_<frame>.bmp) */
        const char* range = SDL_getenv("KHCOM_PRESENTSHOTS");
        int from = 0, to = -1;
        if (range && SDL_sscanf(range, "%d-%d", &from, &to) == 2 && (int)sFrameCount >= from && (int)sFrameCount <= to) {
            SDL_Surface* s = SDL_RenderReadPixels(sRenderer, NULL);
            if (s) {
                char name[64];
                SDL_snprintf(name, sizeof(name), "present_%05u.bmp", (unsigned)sFrameCount);
                SDL_SaveBMP(s, PortConfigPath(name));
                SDL_DestroySurface(s);
            }
        }
    }
    {
        /* KHCOM_LUMALOG=1 (harness): the window as shown, each presented frame: "luma <frame> <mean 0..255>
           <dark per mille>" (a grid of samples), so a sweep finds a lone black frame between two normal ones
           (test_black_frames.py) */
        const char* luma;
        PORT_ENV_ONCE(luma, SDL_getenv, "KHCOM_LUMALOG");
        if (luma) {
            SDL_Surface* s = SDL_RenderReadPixels(sRenderer, NULL);
            SDL_Surface* c = s ? SDL_ConvertSurface(s, SDL_PIXELFORMAT_XRGB8888) : NULL;
            if (c) {
                uint64_t sum = 0;
                unsigned n = 0, dark = 0;
                int x, y, stepX = c->w / 64 > 0 ? c->w / 64 : 1, stepY = c->h / 36 > 0 ? c->h / 36 : 1;
                for (y = stepY / 2; y < c->h; y += stepY)
                    for (x = stepX / 2; x < c->w; x += stepX) {
                        uint32_t p = ((const uint32_t*)((const uint8_t*)c->pixels + (size_t)y * c->pitch))[x];
                        unsigned l = (((p >> 16) & 255) * 77 + ((p >> 8) & 255) * 150 + (p & 255) * 29) >> 8;
                        sum += l;
                        dark += l < 12;
                        n++;
                    }
                if (n)
                    PortLog("luma %u %u %u", (unsigned)sFrameCount, (unsigned)(sum / n), dark * 1000 / n);
            }
            SDL_DestroySurface(c);
            SDL_DestroySurface(s);
        }
    }
    SDL_RenderPresent(sRenderer);
}

static void Quit(void);

/* ---- test harness ------------------------------------------------------------------
 * KHCOM_SHOTS=60,120   save shot_<frame>.bmp at those frames
 * KHCOM_KEYS=100-105:START,200:A   hold GBA keys during frame ranges
 * KHCOM_QUIT=400       exit after that frame
 * KHCOM_BATTLE=10:4[:1] debug battle menu preset to entry 10, stage 4, as Riku with :1 (A starts it)
 * KHCOM_MODE=30:Mode_Deck[:arg]   at frame 30 switch to that game mode
 * KHCOM_SONG=30:29    at frame 30 start song / sound effect 29
 * KHCOM_FAST=1         do not pace frames (run as fast as possible)
 * KHCOM_VBM=movie.vbm  play a VBA-rr input movie (power-on start, controller 1);
 *                      KHCOM_VBM_OFFSET=n shifts it by n frames
 */
static int HarnessListHas(const char* list, uint32_t frame) {
    const char* p = list;

    while (p && *p) {
        if ((uint32_t)SDL_atoi(p) == frame)
            return 1;
        p = SDL_strchr(p, ',');
        if (p)
            p++;
    }
    return 0;
}

/* Input of a VBA-rr movie for one frame (0 past its end). */
static uint16_t MovieKeys(uint32_t frame) {
    static uint8_t* data;
    static size_t frames;
    static int loaded;
    const char* path = SDL_getenv("KHCOM_VBM");
    long index;

    if (path == NULL)
        return 0;
    if (!loaded) {
        size_t size = 0;
        loaded = 1;
        data = SDL_LoadFile(path, &size);
        if (data != NULL && size >= 0x100 && SDL_memcmp(data, "VBM", 4) == 0) {
            uint32_t start = data[0x3C] | data[0x3D] << 8 | data[0x3E] << 16 | (uint32_t)data[0x3F] << 24;
            frames = start < size ? (size - start) / 2 : 0;
            SDL_memmove(data, data + start, frames * 2);
            PortLog("vbm: %s, %u frames", path, (unsigned)frames);
        } else {
            PortLog("vbm: cannot read %s", path);
            frames = 0;
        }
    }
    index = (long)frame + (SDL_getenv("KHCOM_VBM_OFFSET") ? SDL_atoi(SDL_getenv("KHCOM_VBM_OFFSET")) : 0);
    if (index < 0 || (size_t)index >= frames)
        return 0;
    return (uint16_t)((data[index * 2] | data[index * 2 + 1] << 8) & 0x3FF);
}

/* KHCOM_MONKEY=seed:start: from frame `start` on, play with random but
   reproducible input: walk in one direction for a while, attack often, roll
   and use L / R in battle, open and leave the pause menu now and then.
   Never Start + Select together (options) nor A + B + Select + Start. */
static uint16_t MonkeyKeys(uint32_t frame) {
    static uint32_t seed, start, state, until, dir, menuUntil;
    static int ready;
    const char* m;

    if (!ready) {
        ready = 1;
        m = SDL_getenv("KHCOM_MONKEY");
        if (m == NULL)
            return 0;
        seed = (uint32_t)SDL_atoi(m) * 2654435761u + 1;
        start = SDL_strchr(m, ':') ? (uint32_t)SDL_atoi(SDL_strchr(m, ':') + 1) : 0;
        state = seed;
    }
    if (seed == 0 || frame < start)
        return 0;
    if (frame >= until) {
        static const uint16_t dirs[8] = { 16, 32, 64, 128, 16 | 64, 16 | 128, 32 | 64, 32 | 128 };
        state = state * 1664525u + 1013904223u;
        dir = (state >> 24) % 10 < 8 ? dirs[(state >> 8) & 7] : 0;
        until = frame + 20 + (state >> 16) % 90;
        /* about one menu visit a minute: START, a few frames, B out */
        if ((state >> 4) % 40 == 0)
            menuUntil = frame + 40;
    }
    if (frame < menuUntil) {
        uint32_t t = menuUntil - frame;
        return t == 39 ? 8 : (t < 20 && (t & 7) == 0 ? 2 : 0);
    }
    {
        uint32_t h = (frame * 2246822519u) ^ seed;
        uint16_t keys = (uint16_t)dir;
        h ^= h >> 13;
        h *= 3266489917u;
        h ^= h >> 16;
        if ((frame % 12) < 3)
            keys |= 1; /* A: attack, talk, confirm */
        if (h % 97 == 0)
            keys |= 2; /* B */
        if (h % 61 == 0)
            keys |= 256; /* R: next card */
        if (h % 83 == 0)
            keys |= 512; /* L: previous card */
        return keys;
    }
}

extern uint16_t PortBotKeys(uint32_t frame); /* port_testbot.c, KHCOM_BOT */

static uint16_t HarnessKeys(uint32_t frame) {
    static const struct { const char* name; uint16_t bit; } names[] = {
        { "A", 1 }, { "B", 2 }, { "SELECT", 4 }, { "START", 8 }, { "RIGHT", 16 },
        { "LEFT", 32 }, { "UP", 64 }, { "DOWN", 128 }, { "R", 256 }, { "L", 512 },
        { "STOCK", PORT_KEY_STOCK }, { "DODGE", PORT_KEY_DODGE }, { "UNSTOCK", PORT_KEY_UNSTOCK },
    };
    const char* p = SDL_getenv("KHCOM_KEYS");
    uint16_t keys = MovieKeys(frame) | MonkeyKeys(frame);

    while (p && *p) {
        int a = SDL_atoi(p), b = a;
        const char* dash = SDL_strchr(p, '-');
        const char* colon = SDL_strchr(p, ':');
        const char* comma = SDL_strchr(p, ',');
        size_t i;

        if (colon == NULL)
            break;
        if (dash && dash < colon)
            b = SDL_atoi(dash + 1);
        if (frame >= (uint32_t)a && frame <= (uint32_t)b) {
            for (i = 0; i < SDL_arraysize(names); i++) {
                size_t len = SDL_strlen(names[i].name);
                if (SDL_strncmp(colon + 1, names[i].name, len) == 0
                    && (colon[1 + len] == ',' || colon[1 + len] == '+' || colon[1 + len] == 0))
                    keys |= names[i].bit;
            }
        }
        p = comma ? comma + 1 : NULL;
    }
    return keys;
}

static void Present(void);

/* the window's renderer, for what draws outside the game's frame (port_hdmovie.c) */
SDL_Renderer* PortRenderer(void) {
    return sRenderer;
}

/* Test harness: save the next presented picture under this name. */
void PortHarnessShot(const char* name) {
    SDL_strlcpy(sHarnessShot, name, sizeof(sHarnessShot));
}

/* The log's header: what a report needs to tell machines apart (version, system, Wine/Proton, display, settings). */
static void LogSystem(void) {
#if !__STDC_HOSTED__ /* the consoles' guest: its SDL shim has none of the system queries */
    PortLog("ReKHCoM %s (built %s)", REKHCOM_VERSION, __DATE__);
#else
    const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(sWindow));
    PortLog("ReKHCoM %s (built %s), %s, %d cores, %d MB RAM", REKHCOM_VERSION, __DATE__, SDL_GetPlatform(),
            SDL_GetNumLogicalCPUCores(), SDL_GetSystemRAM());
#ifdef _WIN32
    {
        typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW*);
        typedef const char*(CDECL * WineVersionFn)(void);
        typedef void(CDECL * WineHostFn)(const char**, const char**);
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        RtlGetVersionFn rtl = ntdll ? (RtlGetVersionFn)(void*)GetProcAddress(ntdll, "RtlGetVersion") : NULL;
        WineVersionFn wine = ntdll ? (WineVersionFn)(void*)GetProcAddress(ntdll, "wine_get_version") : NULL;
        WineHostFn host = ntdll ? (WineHostFn)(void*)GetProcAddress(ntdll, "wine_get_host_version") : NULL;
        OSVERSIONINFOW v = { sizeof(v) };
        if (rtl && rtl(&v) == 0)
            PortLog("OS: Windows %lu.%lu.%lu", v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);
        if (wine) {
            const char *sys = "", *rel = "";
            if (host)
                host(&sys, &rel);
            PortLog("Wine %s on %s %s%s", wine(), sys, rel, SDL_getenv("STEAM_COMPAT_DATA_PATH") ? " (Proton)" : "");
        }
    }
#endif
    if (dm)
        PortLog("Display: %dx%d @ %.2f Hz", dm->w, dm->h, dm->refresh_rate);
#endif
    PortLog("Settings: graphics mode %d, view %d, fullscreen %d, vsync %d, frame skip %d, HD movies %d music %d "
            "scenes %d HUD %d text %d", gPortConfig.graphicsMode, gPortConfig.viewMode, gPortConfig.fullscreen,
            gPortConfig.vsync, gPortConfig.frameSkip, gPortConfig.hdMovies, gPortConfig.hdMusic, gPortConfig.hdScenes,
            gPortConfig.hdCards, gPortConfig.hdText);
}

static void HarnessFrame(void) {
    const char* shots = SDL_getenv("KHCOM_SHOTS");
    const char* quit = SDL_getenv("KHCOM_QUIT");
    const char* ui = SDL_getenv("KHCOM_OPTIONS_SHOT");
    const char* oam = SDL_getenv("KHCOM_OAMDUMP");
    const char* saveAt = SDL_getenv("KHCOM_SAVESTATE");

    {
        /* every mode change in the log (a report then tells where the game was); the tests' own wording with
           KHCOM_TRACE_MODES */
        extern const char* PortHarnessModeName(void);
        static const char* previous;
        const char* name = PortHarnessModeName();
        if (name != previous) {
            if (SDL_getenv("KHCOM_TRACE_MODES"))
                PortLog("harness: frame %u mode %s", (unsigned)sFrameCount, name);
            else
                PortLog("mode: %s (frame %u)", name, (unsigned)sFrameCount);
            previous = name;
        }
    }

    /* KHCOM_RESIZE=frame:WxH: resize the window then, as a player would */
    {
        const char* r = SDL_getenv("KHCOM_RESIZE");
        unsigned f;
        int w, h;
        if (r && SDL_sscanf(r, "%u:%dx%d", &f, &w, &h) == 3 && f == sFrameCount)
            SDL_SetWindowSize(sWindow, w, h);
    }

    /* KHCOM_MODE=frame:name[:arg][,frame:name...]: switch to a game mode
       (menus, screens) or run a harness action (main.c PortHarnessMode). */
    {
        const char* m = SDL_getenv("KHCOM_MODE");
        while (m && *m) {
            const char* comma = SDL_strchr(m, ',');
            const char* colon = SDL_strchr(m, ':');
            if (colon && (!comma || colon < comma) && (uint32_t)SDL_atoi(m) == sFrameCount) {
                extern int PortHarnessMode(const char* name, int arg);
                char item[96], *name = item, *arg;
                size_t n = comma ? (size_t)(comma - colon - 1) : SDL_strlen(colon + 1);
                SDL_strlcpy(item, colon + 1, SDL_min(n + 1, sizeof(item)));
                arg = SDL_strchr(item, ':');
                if (arg)
                    *arg++ = 0;
                if (!PortHarnessMode(name, arg ? SDL_atoi(arg) : 0))
                    PortLog("harness: no mode %s", name);
            }
            m = comma ? comma + 1 : NULL;
        }
    }

    /* KHCOM_SONG=frame:id: start a song or sound effect. */
    {
        const char* m = SDL_getenv("KHCOM_SONG");
        const char* colon = m ? SDL_strchr(m, ':') : NULL;
        if (colon && (uint32_t)SDL_atoi(m) == sFrameCount) {
            extern void PortHarnessSong(int song);
            PortHarnessSong(SDL_atoi(colon + 1));
        }
    }

    /* KHCOM_SAVESTATE=frame:path[,frame:path...] */
    while (saveAt && *saveAt) {
        const char* colon = SDL_strchr(saveAt, ':');
        const char* comma = SDL_strchr(saveAt, ',');
        if (colon && (uint32_t)SDL_atoi(saveAt) == sFrameCount) {
            char path[512];
            size_t n = comma ? (size_t)(comma - colon - 1) : SDL_strlen(colon + 1);
            SDL_strlcpy(path, colon + 1, SDL_min(n + 1, sizeof(path)));
            PortStateRequestSave(PortConfigPath(path));
        }
        saveAt = comma ? comma + 1 : NULL;
    }

    if (oam && HarnessListHas(oam, sFrameCount)) {
        extern void PortPpuDumpOam(void);
        PortLog("frame %u", (unsigned)sFrameCount);
        PortPpuDumpOam();
    }

    if (HarnessListHas(SDL_getenv("KHCOM_OPTIONS_TOGGLE"), sFrameCount))
        PortUiHarnessToggle();
    if (ui && HarnessListHas(ui, sFrameCount)) {
        extern void PortUiForceClose(void);
        SDL_Surface* s;
        PortUiOpenOptions();
        UpdateResolution(); /* as the menu's own loop does */
        PortPpuRenderFrame();
        Present();
        s = SDL_CreateSurfaceFrom(gPortScreen.width, gPortScreen.height, SDL_PIXELFORMAT_XRGB8888, sComposite,
                                  gPortScreen.width * 4);
        if (s) {
            SDL_SaveBMP(s, PortConfigPath("shot_options.bmp"));
            SDL_DestroySurface(s);
        }
        PortUiForceClose();
    }

    {
        /* KHCOM_RECOM=1 (harness): the game being played is the Re:CoM version (as chosen at New Game) */
        static int recom;
        if (!recom && SDL_getenv("KHCOM_RECOM")) {
            recom = 1;
            PortRecomSetActive(1);
        }
    }
    {
        /* KHCOM_OPTIONS_AT=frame (harness): the Options overlay opens there; KHCOM_OPTIONS_HOLD closes it */
        const char* optionsAt = SDL_getenv("KHCOM_OPTIONS_AT");
        if (optionsAt && sFrameCount == (uint32_t)SDL_atoi(optionsAt))
            PortUiOpenOptions();
    }
    {
        /* KHCOM_WIZARD_OPEN=frame (harness): the new game page opens there, for KHCOM_WIZARD_KEYS */
        const char* wizard = SDL_getenv("KHCOM_WIZARD_OPEN");
        if (wizard && sFrameCount == (uint32_t)SDL_atoi(wizard)) {
            extern void PortUiOpenNewGame(void);
            PortUiOpenNewGame();
        }
    }
    if (shots && HarnessListHas(shots, sFrameCount)) {
        SDL_Surface* s = SDL_CreateSurfaceFrom(gPortScreen.width, gPortScreen.height, SDL_PIXELFORMAT_XRGB8888,
                                               gPortFrame, gPortScreen.width * 4);
        char name[64];
        SDL_snprintf(name, sizeof(name), "shot_%05u.bmp", (unsigned)sFrameCount);
        if (s) {
            SDL_SaveBMP(s, PortConfigPath(name));
            SDL_DestroySurface(s);
        }
    }
    if (quit && sFrameCount >= (uint32_t)SDL_atoi(quit))
        Quit();
}

/* ---- events / pacing --------------------------------------------------------------- */

static void Quit(void) {
    void PortHdHudShutdown(void);
    PortSramSyncIfDirty();
    PortConfigSave();
    PortAchievementsShutdown();
    PortDiscordShutdown();
    PortHdMusicShutdown();
    PortAudioShutdown();
    PortHdHudShutdown();
    PortHdSpritesShutdown();
    PortHdLayoutShutdown();
    SDL_Quit();
    exit(0);
}

static void PumpEvents(void) {
    SDL_Event ev;

    while (SDL_PollEvent(&ev)) {
        KhPadNormalizeEvent(&ev, gPortConfig.padLayout);
#ifdef __EMSCRIPTEN__
        if(ev.type==SDL_EVENT_RENDER_DEVICE_RESET) {
            extern void PortWebHdClear(void);PortWebHdClear();
        }
#endif
#if defined(__ANDROID__) || defined(PORT_ANDROID64)
        if (ev.type == SDL_EVENT_WILL_ENTER_BACKGROUND || ev.type == SDL_EVENT_DID_ENTER_BACKGROUND) {
            sAndroidBackground = 1;
            PortAndroidTouchClear();
            PortSramSyncIfDirty();
            PortConfigSave();
            PortAudioReset();
            continue;
        }
        if (ev.type == SDL_EVENT_DID_ENTER_FOREGROUND) {
            sAndroidBackground = 0;
            sNextFrame = 0;
            PortAudioReset();
            continue;
        }
#endif
        switch (ev.type) {
            case SDL_EVENT_QUIT:
                Quit();
                break;
            case SDL_EVENT_KEY_DOWN:
                /* quick states only when turned on in Cheats (off by default, so a run plays as on the cartridge;
                   no achievements while on): F5 save, F8 load, F6/F7 the slot */
                if (!ev.key.repeat && ev.key.key == SDLK_F5 && gPortConfig.cheatSaveStates) {
                    PortStateRequestSave(NULL);
                    continue;
                }
                if (!ev.key.repeat && ev.key.key == SDLK_F8 && gPortConfig.cheatSaveStates) {
                    PortStateRequestLoad(NULL);
                    continue;
                }
                /* +/-: the main zoom (Auto view), unless the key is bound to a game action */
                if ((ev.key.key == SDLK_PLUS || ev.key.key == SDLK_EQUALS || ev.key.key == SDLK_KP_PLUS ||
                     ev.key.key == SDLK_MINUS || ev.key.key == SDLK_KP_MINUS) && !PortInputKeyBound(ev.key.key)) {
                    PortUiZoomStep(ev.key.key == SDLK_MINUS || ev.key.key == SDLK_KP_MINUS ? 1 : -1);
                    continue;
                }
                if ((ev.key.key == SDLK_F6 || ev.key.key == SDLK_F7) && gPortConfig.cheatSaveStates) {
                    char msg[32];
                    gPortStateSlot = (gPortStateSlot + (ev.key.key == SDLK_F7 ? 1 : 8)) % 9;
                    if (gPortStateSlot == 0)
                        gPortStateSlot = 9;
                    SDL_snprintf(msg, sizeof(msg), "Slot %d", gPortStateSlot);
                    PortUiNotice(msg);
                    continue;
                }
                if ((ev.key.key == SDLK_RETURN && (ev.key.mod & SDL_KMOD_ALT)) || ev.key.key == SDLK_F11) {
                    gPortConfig.fullscreen = !gPortConfig.fullscreen;
                    ApplyWindowMode();
                    PortConfigSave();
                    continue;
                }
                break;
        }
        if (PortUiHandleEvent(&ev))
            continue;
        PortInputHandleEvent(&ev);
    }
}

/* The frame timer always paces the game; vsync only removes tearing. On a
   display within ~1.5 Hz of the GBA's rate the game follows the display so
   every refresh shows exactly one frame (audio is resampled to match). */
static void Pace(void) {
#ifdef __EMSCRIPTEN__
    int yielded = 0;
#endif
    uint64_t now = SDL_GetPerformanceCounter();
    double hz = GBA_FPS;
    uint64_t period;

    if (SDL_getenv("KHCOM_FAST")) {
        sNextFrame = 0;
        return;
    }
    sPacedByVsync = 0;
    period = (uint64_t)(sPerfFreq / hz);

    /* Exact GBA rate (59.7275 Hz) whatever the display does: sleep most of
       the wait, then spin for the last fraction of a millisecond. */
    if (sNextFrame == 0 || now > sNextFrame + period * 4)
        sNextFrame = now;
    sNextFrame += period;
    while ((now = SDL_GetPerformanceCounter()) < sNextFrame) {
        uint64_t leftNs = (sNextFrame - now) * 1000000000ull / sPerfFreq;
        /* SDL already implements the platform's precise wait. Reserving a
           second explicit millisecond of busy-spin wasted CPU every frame. */
        #ifdef __EMSCRIPTEN__
        emscripten_sleep((unsigned)((leftNs + 999999) / 1000000));
        yielded = 1;
#else
        SDL_DelayPrecise(leftNs);
#endif
    }

#ifdef __EMSCRIPTEN__
    /* Always yield, even when rendering took longer than the frame budget. */
    if (!yielded) emscripten_sleep(0);
#endif
    sFpsFrames++;
    if (now - sFpsTime >= sPerfFreq) {
        extern volatile uint32_t gFrameCounter; /* main-loop updates (main.c) */
        uint32_t logic = gFrameCounter;
        double secs = (double)(now - sFpsTime) / (double)sPerfFreq;

        sFps = (float)(sFpsFrames / secs);
        sLogicFps = (float)((uint32_t)(logic - sLogicLast) / secs);
        sLogicLast = logic;
        sFpsFrames = 0;
        sFpsTime = now;
        if (SDL_getenv("KHCOM_FPSLOG"))
            PortLog("fps: display %.2f logic %.2f", sFps, sLogicFps);
    }
}

int PortPacedByVsync(void) {
    return sPacedByVsync;
}

/* The network link's time sync (src/sio/sio.c): the game running ahead of the other player gives one frame of time
   back (its pacing schedule moves a frame later), so neither keeps playing on guesses. */
void PortLinkYieldFrame(void) {
    if (sNextFrame)
        sNextFrame += (uint64_t)(sPerfFreq / GBA_FPS);
}

static uint64_t sPpuTicks;

/* experimental_frame_skip: a frame that starts half a frame late is not drawn nor presented (at most three in a
   row); the game, its line events and the sound run as always, so it keeps the original speed on devices that
   cannot draw every frame (Switch, Vita: #35, #36). A new canvas size is always drawn. */
static int FrameSkipDue(int sizeChanged) {
    static int skipped;
    uint64_t now;

    /* KHCOM_SKIPTEST=1 (harness): two frames of three are skipped whatever the clock; shots taken on multiples of 3
       must match a run without it pixel for pixel (the game's logic does not depend on drawing) */
    if (SDL_getenv("KHCOM_SKIPTEST") && !sizeChanged && !PortUiActive())
        return sFrameCount % 3 != 0;
    if (!gPortConfig.frameSkip || sizeChanged || !sNextFrame || SDL_getenv("KHCOM_FAST") || PortUiActive()) {
        skipped = 0;
        return 0;
    }
    now = SDL_GetPerformanceCounter();
    {
        /* the log says how often it acts: "frame skip: N of the last 600 frames" when any were skipped */
        static int window, total;
        int skip = now > sNextFrame + (uint64_t)(sPerfFreq / GBA_FPS / 2) && skipped < 3;
        skipped = skip ? skipped + 1 : 0;
        total += skip;
        if (++window == 600) {
            if (total)
                PortLog("frame skip: %d of the last 600 frames not drawn", total);
            window = total = 0;
        }
        return skip;
    }
}

/* One display frame: draw, vertical blank, present, input, pacing. */
/* A frame the network link's rollback plays again (port_rollback.c): the game's frame as VBlankCycle runs it when it
   skips drawing (the same logic, test_frameskip.py), with nothing shown, sounded or paced, and the keys it had. */
static void ResimCycle(void) {
    extern int gPortInMainLoop;
    *(volatile uint16_t*)(uintptr_t)(GBA_IWRAM + 0x7FF8) &= ~1; /* BIOS IntrCheck */
    PortBiosAreaRefresh();
    (void)PortTakeScreenMark();
    PortPpuRunFrameEvents();
    IO16(0x06) = GBA_SCREEN_H;
    IO16(0x04) = (IO16(0x04) & ~6) | 1 | (((IO16(0x04) >> 8) == GBA_SCREEN_H) ? 4 : 0);
    PortDmaVBlank();
    if (IO16(0x04) & 0x08)
        PortRaiseIrq(0x0001);
    PortPpuVBlankLines();
    IO16(0x04) &= ~1;
    {
        extern void PortMenuLatch(void);
        PortMenuLatch();
    }
    IO16(0x130) = (uint16_t)(~PortInputResolve(PortInputKeys() | HarnessKeys(sFrameCount) | PortBotKeys(sFrameCount)) & 0x3FF);
    PortRollbackBoundary(gPortInMainLoop);
}

/* One line a second in the log of where a frame's time goes: the game's logic (between two VBlanks), the PPU, the
   presentation, the port's other per-frame work, and the frames skipped. Always on the consoles and Android 64
   (PORT_GBA_RELOC: the logs players send are how their speed is measured, SWITCH_MOTOR.md); KHCOM_TIMING elsewhere. */
static uint64_t sCycleEnd, sTimeLogic, sTimeWork, sTimePresent, sTimePpu;
static int sTimeFrames, sTimeSkipped;
static uint64_t sPresentTicks;

static void TimingLine(int skip, uint64_t cycleStart, uint64_t workEnd) {
#ifndef PORT_GBA_RELOC
    const char* on;
    PORT_ENV_ONCE(on, SDL_getenv, "KHCOM_TIMING");
    if (!on)
        return;
#endif
    if (sCycleEnd && cycleStart > sCycleEnd)
        sTimeLogic += cycleStart - sCycleEnd;
    sTimeWork += workEnd - cycleStart;
    sTimePresent += skip ? 0 : sPresentTicks;
    sTimePpu += skip ? 0 : sPpuTicks;
    sTimeSkipped += skip;
    if (++sTimeFrames == 60) {
        double ms = 1000.0 / SDL_GetPerformanceFrequency() / 60;
        double drawn = 60 - sTimeSkipped ? 60.0 / (60 - sTimeSkipped) : 0;
        PortLog("timing: logic %.1f ms, ppu %.1f, present %.1f, port %.1f (a frame; ppu/present per drawn frame) | "
                "skipped %d of 60 | canvas %dx%d", sTimeLogic * ms, sTimePpu * ms * drawn, sTimePresent * ms * drawn,
                (sTimeWork - sTimePpu - sTimePresent) * ms, sTimeSkipped, gPortScreen.width, gPortScreen.height);
        sTimeLogic = sTimeWork = sTimePresent = sTimePpu = 0;
        sTimeFrames = sTimeSkipped = 0;
    }
}

static void VBlankCycle(void) {
    uint64_t cycleStart = SDL_GetPerformanceCounter();
    const char* frameLogPath = SDL_getenv("KHCOM_FRAMELOG");
    uint64_t frameLogStart = frameLogPath ? SDL_GetPerformanceCounter() : 0;
    uint64_t frameLogWork = 0;
    *(volatile uint16_t*)(uintptr_t)(GBA_IWRAM + 0x7FF8) &= ~1; /* BIOS IntrCheck */
    PortBiosAreaRefresh();
    sFit = sFitMark;
    sFitMark = 0;
    {
        /* a menu's sprites reach OAM one frame after it marks the frame:
           keep the layer one frame longer so they never land in the world */
        static int sPrevMark;
        int mark = PortTakeScreenMark();
        gPortScreenLayer = (mark || sPrevMark) && gPortConfig.viewMode == PORT_VIEW_AUTO;
        sPrevMark = mark;
    }
    int skip;
    {
        int w = gPortScreen.width, h = gPortScreen.height;
        UpdateResolution();
        skip = FrameSkipDue(w != gPortScreen.width || h != gPortScreen.height);
    }

    {
        /* KHCOM_REGLOG=from-to (harness): the display registers each frame of that range */
        const char* reg = SDL_getenv("KHCOM_REGLOG");
        int from = 0, to = -1;
        if (reg && SDL_sscanf(reg, "%d-%d", &from, &to) == 2 && (int)sFrameCount >= from && (int)sFrameCount <= to)
            PortLog("reg %u: DISPCNT %04x BG0 %04x BG1 %04x BG2 %04x BG3 %04x BLDCNT %04x BLDALPHA %04x BLDY %04x "
                    "WININ %04x WINOUT %04x WIN0H %04x WIN0V %04x MOSAIC %04x", (unsigned)sFrameCount, IO16(0x00),
                    IO16(0x08), IO16(0x0A), IO16(0x0C), IO16(0x0E), IO16(0x50), IO16(0x52), IO16(0x54), IO16(0x48),
                    IO16(0x4A), IO16(0x40), IO16(0x44), IO16(0x4C));
    }

    /* Visible lines (HBlank / VCount events in between). */
    if (skip) {
        PortPpuRunFrameEvents();
    } else {
        uint64_t t0 = SDL_GetPerformanceCounter();
        PortPpuRenderFrame();
        sPpuTicks = SDL_GetPerformanceCounter() - t0;
    }

    /* Line 160: vertical blank. */
    IO16(0x06) = GBA_SCREEN_H;
    IO16(0x04) = (IO16(0x04) & ~6) | 1 | (((IO16(0x04) >> 8) == GBA_SCREEN_H) ? 4 : 0);
    PortDmaVBlank();
    if (IO16(0x04) & 0x08)
        PortRaiseIrq(0x0001);
    PortPpuVBlankLines();
    IO16(0x04) &= ~1;

    PortAudioFrame();
    {
        /* KHCOM_PROFILE: mean PPU and present time every 240 frames */
        static uint64_t ppuSum, presSum;
        static int n;
        uint64_t p0 = SDL_GetPerformanceCounter();
        if (!skip)
            Present();
        sPresentTicks = SDL_GetPerformanceCounter() - p0;
        {
            extern void PortMenuLatch(void);
            PortMenuLatch(); /* this frame's menu sprites show from the next one */
        }
        if (SDL_getenv("KHCOM_PROFILE")) {
            presSum += SDL_GetPerformanceCounter() - p0;
            ppuSum += sPpuTicks;
            if (++n == 240) {
                double f = 1000.0 / SDL_GetPerformanceFrequency() / n;
                extern void PortPpuTakeStats(int out[5]);
                int st[5] = { 0 };
#ifdef PORT_PPU_THREADS
                PortPpuTakeStats(st);
#endif
                PortLog("profile: canvas %dx%d ppu %.2f ms present %.2f ms | outer rows multi-core %d (iso floor %d), one core: "
                        "mosaic %d header %d small %d", gPortScreen.width, gPortScreen.height, ppuSum * f, presSum * f,
                        st[0], st[1], st[2], st[3], st[4]);
                ppuSum = presSum = 0;
                n = 0;
            }
        }
    }
    PumpEvents();

    /* Options overlay: the game is paused until it closes. */
#if defined(__ANDROID__) || defined(PORT_ANDROID64)
    while (sAndroidBackground) { SDL_Delay(50); PumpEvents(); }
#endif
    if (PortUiActive()) {
        extern void PortMenuSoundBegin(void);
        extern void PortMenuSoundDecide(int confirm);
        extern int PortUiTakeMenuSound(int open);
        PortMenuSoundBegin();
        if (PortUiTakeMenuSound(1))
            PortMenuSoundDecide(0); /* SYS_CANSEL: the field menu's opening */
    }
    while (PortUiActive()) {
        extern void PortMenuSoundFrame(void);
        extern void PortAudioMenuFrame(void);
#ifdef __EMSCRIPTEN__
        PortWebWaitVisible();
#endif
        PortMenuSoundFrame();
        PortAudioMenuFrame();
        int w = gPortScreen.width, h = gPortScreen.height;

        UpdateResolution();
        if (w != gPortScreen.width || h != gPortScreen.height)
            PortPpuRenderFrame();
        Present();
        PumpEvents();
#if defined(__ANDROID__) || defined(PORT_ANDROID64)
        while (sAndroidBackground) { SDL_Delay(50); PumpEvents(); }
#endif
        Pace();
        /* Harness: with KHCOM_VPAD the menu frames count too, so the
           virtual pad can drive the overlay and KHCOM_RECORD films it. */
        if (SDL_getenv("KHCOM_VPAD")) {
            (void)PortInputKeys();
            sFrameCount++;
        }
        if (!PortUiActive()) {
            extern void PortMenuSoundEnd(void);
            extern void PortMenuSoundClose(void);
            extern int PortUiTakeMenuSound(int open);
            PortMenuSoundEnd();
            if (PortUiTakeMenuSound(0))
                PortMenuSoundClose(); /* back in the game's own sound */
        }
    }
    IO16(0x130) = (uint16_t)(~PortInputResolve(PortInputKeys() | HarnessKeys(sFrameCount) | PortBotKeys(sFrameCount)) & 0x3FF);
    HarnessFrame();
    {
        extern int gPortInMainLoop;
        PortRollbackBoundary(gPortInMainLoop); /* the network link: a wrong guess taken back, this frame kept */
    }
    if (frameLogStart) frameLogWork = SDL_GetPerformanceCounter();
    TimingLine(skip, cycleStart, SDL_GetPerformanceCounter());
    Pace();
    sCycleEnd = SDL_GetPerformanceCounter();

    if (++sFrameCount % 60 == 0)
        PortSramSyncIfDirty();
    if (frameLogStart) {
        static FILE* frameLog;
        static int attempted;
        static uint64_t previous;
        uint64_t end = SDL_GetPerformanceCounter();
        extern volatile uint32_t gFrameCounter;
        if (!attempted) {
            attempted = 1;
            frameLog = fopen(frameLogPath, "w");
            if (frameLog) fprintf(frameLog, "frame,interval_ms,render_work_ms,logic,screen,fit\n");
        }
        if (frameLog && previous)
            fprintf(frameLog, "%u,%.6f,%.6f,%u,%d,%d\n", (unsigned)sFrameCount,
                    1000.0 * (end - previous) / sPerfFreq,
                    1000.0 * (frameLogWork - frameLogStart) / sPerfFreq,
                    (unsigned)gFrameCounter, gPortScreenLayer, (int)sFit);
        previous = end;
    }
    if (SDL_getenv("KHCOM_TIMING") && sFrameCount % 300 == 0) {
        static uint64_t last;
        uint64_t t = SDL_GetPerformanceCounter();
        extern vu32 gVBlankCounter, gFrameCounter;
        {
            extern char gMPlayInfo_BGM[];
            extern void* gSoundInfoPtr;
            const uint8_t* m = (const uint8_t*)gMPlayInfo_BGM;
            const uint8_t* si = (const uint8_t*)gSoundInfoPtr;
            PortLog("bgm: song %08x clock %u tempoD %u tempoI %u status %08x ident %08x | sound ident %08x mix freq %d"
                    " spv %d | dma1 %08x cnt %08x tm0 %04x",
                    *(uint32_t*)m, *(uint32_t*)(m + 0x0C), *(uint16_t*)(m + 0x1C), *(uint16_t*)(m + 0x20),
                    *(uint32_t*)(m + 0x04), *(uint32_t*)(m + 0x34), si ? *(uint32_t*)si : 0,
                    si ? *(int32_t*)(si + 0x14) : 0, si ? *(int32_t*)(si + 0x10) : 0, IO32(0xBC), IO32(0xC4),
                    IO16(0x102));
        }
        if (last)
            PortLog("timing: 300 frames in %.3f s (%.2f Hz), vblanks %u, game updates %u",
                    (double)(t - last) / sPerfFreq, 300.0 * sPerfFreq / (double)(t - last), (unsigned)gVBlankCounter,
                    (unsigned)gFrameCounter);
        last = t;
    }
}

/* The network link's lockstep (src/sio/sio.c): while the other player's packet for this link frame is on its way the
   game waits inside the frame; the last picture stays on screen (with the waiting notice) and the window answers. */
void PortLinkIdle(void) {
    Present();
    PumpEvents();
    SDL_Delay(1);
}

/*
 * One frame per vertical blank, like the cartridge: measured on hardware-
 * accurate timing (port/tools/bizhawk_timing.lua), the game's
 * main loop makes one update per VBlank in every scene, at 59.7275 Hz.
 */
void PortFrame(void) {
#ifdef __EMSCRIPTEN__
    PortWebWaitVisible();
#endif
    if (PortRollbackResimulating()) { /* the network link plays a frame again: the game's frame alone */
        ResimCycle();
        return;
    }
    PortStateService();
    {
        /* Options > Return to title screen: the game's own soft reset, as its Continue screen does */
        extern int PortUiTakeTitleRequest(void);
        extern void SoftReset(int32_t flags);
        if (PortUiTakeTitleRequest()) {
#ifdef VERSION_EU
            DoSoftReset();
#else
            SoftReset(0xFF); /* RESET_ALL */
#endif
        }
    }
    PortAchievementsFrame();
    PortDiscordFrame();
    {
        extern void PortRematchFrame(void);
        PortRematchFrame();
    }
    VBlankCycle();
}

/* Movies own their playback loop and run at the cartridge's frame rate,
   independently of the gameplay speed setting and main-loop save states. */
void PortMovieFrame(void) {
#ifdef __EMSCRIPTEN__
    PortWebWaitVisible();
#endif
    VBlankCycle();
}

/* ---- reset ----------------------------------------------------------------------- */

void PortSoftReset(void) {
#ifdef __EMSCRIPTEN__
    PortSramSyncIfDirty();
    PortConfigSave();
    PortAudioReset();
    if(!PortWebResetSave())return;
    EM_ASM({ location.reload(); });
    emscripten_exit_with_live_runtime();
    return;
#endif
    memcpy(sGameData, sDataSnapshot, sDataSize);
    memset(sGameBss, 0, sGameBssSize);
    memset((void*)(uintptr_t)GBA_IO, 0, GBA_IO_SIZE);
    longjmp(sResetJmp, 1);
}

int main(int argc, char** argv) {
    int winW, winH;

    {
        int i;
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--intro") == 0)
                gPortIntroRequested = 1;
            else if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc)
                PortConfigSetDataPath(argv[++i]);
            else if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc)
                gPortRomPath = argv[++i];
            else if (strcmp(argv[i], "--language") == 0 && i + 1 < argc) {
                const char* language = argv[++i];
                if (strcmp(language, "es") == 0) gPortStartupLanguage = 4;
                else if (strcmp(language, "en") == 0) gPortStartupLanguage = 0;
                else if (strcmp(language, "fr") == 0) gPortStartupLanguage = 1;
                else if (strcmp(language, "de") == 0) gPortStartupLanguage = 2;
                else if (strcmp(language, "it") == 0) gPortStartupLanguage = 3;
            }
        }
    }

    if (SDL_getenv("KHCOM_MOVIE"))
        gPortMovieTest = SDL_atoi(SDL_getenv("KHCOM_MOVIE"));
    if (SDL_getenv("KHCOM_BATTLE")) {
        const char* b = SDL_getenv("KHCOM_BATTLE");
        const char* colon = SDL_strchr(b, ':');
        const char* riku = colon ? SDL_strchr(colon + 1, ':') : NULL;
        gPortBattleTest = (SDL_atoi(b) & 0xFF) | ((colon ? SDL_atoi(colon + 1) & 0xFF : 0) << 8)
                          | ((riku && SDL_atoi(riku + 1)) ? 0x10000 : 0);
    }
    if (SDL_getenv("KHCOM_EVENT")) {
        const char* e = SDL_getenv("KHCOM_EVENT");
        const char* colon = SDL_strchr(e, ':');
        if (colon)
            gPortEventTest = (SDL_atoi(e) << 8) | (SDL_atoi(colon + 1) & 0xFF);
    }

    /* Claim the GBA address range before anything else can land there. */
    if (!PortMemoryInit())
        return 1;

#ifdef __EMSCRIPTEN__
    /* Pace owns browser yields; don't add another timer in RenderPresent. */
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif
#ifdef PORT_LINUX
    if (PortFbInit(&sFbW, &sFbH))
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
#endif
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
        PortFatal("SDL_Init failed: %s", SDL_GetError());
    PortConfigLoad();
    PortAchievementsInit();
    PortLog("KHCoM PC - SDL %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    {
        int w, h;
        PortConfigView(gPortConfig.viewMode, &w, &h);
        PortPpuSetResolution(w, h);
    }
    winW = gPortScreen.width * gPortConfig.windowScale;
    winH = gPortScreen.height * gPortConfig.windowScale;
    if (SDL_getenv("KHCOM_WINDOW")) {
        /* KHCOM_WINDOW=WxH: an exact window size (tests of other aspect ratios) */
        int w = 0, h = 0;
        if (SDL_sscanf(SDL_getenv("KHCOM_WINDOW"), "%dx%d", &w, &h) == 2 && w >= GBA_SCREEN_W && h >= GBA_SCREEN_H) {
            winW = w;
            winH = h;
        }
    }
#ifdef PORT_LINUX
    if (PortFbActive()) {
        winW = sFbW;
        winH = sFbH;
    }
#endif
#ifdef __EMSCRIPTEN__
    winW = PortWebCanvasWidth();
    winH = PortWebCanvasHeight();
    gPortConfig.hdText = EM_ASM_INT({return Module.hdReady ? 1 : 0;}) && gPortConfig.hdText;
#endif
    sWindow = SDL_CreateWindow("Kingdom Hearts: Chain of Memories", winW, winH,
                               SDL_WINDOW_RESIZABLE
#ifndef __EMSCRIPTEN__
                               | SDL_WINDOW_HIGH_PIXEL_DENSITY
#endif
#ifdef _WIN32
                               /* shown once in its final mode: a fullscreen start no longer flashes a window */
                               | SDL_WINDOW_HIDDEN
#endif
                               );
    if (sWindow == NULL)
        PortFatal("Could not create the window: %s", SDL_GetError());
    SDL_SetWindowMinimumSize(sWindow, GBA_SCREEN_W, GBA_SCREEN_H);
#if defined(PORT_LINUX) && !defined(__ANDROID__)
    {
        /* Windows takes the icon from the executable; X11/Wayland need it set. */
        SDL_Surface* icon = SDL_LoadBMP(PortConfigPath("assets/icon.bmp"));
        if (icon != NULL) {
            SDL_SetWindowIcon(sWindow, icon);
            SDL_DestroySurface(icon);
        }
    }
#endif
    PortCanvasAlloc();
    GrowPixelBuffer(&sCompositeBuf, &sCompositeCapacity, (size_t)gPortScreen.width * gPortScreen.height);
    sComposite = sCompositeBuf;
    sRenderer = SDL_CreateRenderer(sWindow, NULL);
    if (sRenderer == NULL)
        PortFatal("Could not create the renderer: %s", SDL_GetError());
    PortLog("Renderer: %s", SDL_GetRendererName(sRenderer));
    LogSystem();
    ApplyWindowMode();
#ifdef _WIN32
    SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sRenderer);
    SDL_RenderPresent(sRenderer);
    /* harness: KHCOM_HIDDEN keeps the window hidden, so timing runs draw on the real GPU without showing it */
    if (!SDL_getenv("KHCOM_HIDDEN")) {
        SDL_ShowWindow(sWindow);
        SDL_SyncWindow(sWindow);
    }
#endif
    sPerfFreq = SDL_GetPerformanceFrequency();
    sFpsTime = SDL_GetPerformanceCounter();

    if (!PortRomLoad())
        return 1;
    PortUndubApply();
    PortSramLoad();
    PortInputInit();
    PortAudioInit();
    PortUiInit();

    PortBiosAreaRefresh();
    PortGameSections(&sGameData, &sGameDataSize, &sGameBss, &sGameBssSize);
    sDataSize = sGameDataSize;
    sDataSnapshot = malloc(sDataSize ? sDataSize : 1);
    memcpy(sDataSnapshot, sGameData, sDataSize);
    PortLog("Game globals: data %u bytes, bss %u bytes", (unsigned)sGameDataSize, (unsigned)sGameBssSize);

    {
        extern void PortWatchdogStart(void);
        extern void PortCrashHandlerInstall(void);
        extern void PortBiosFaultInstall(void);
        PortCrashHandlerInstall();
        PortBiosFaultInstall();
        PortWatchdogStart();
    }
    if (SDL_getenv("KHCOM_LOADSTATE"))
        PortStateRequestLoad(PortConfigPath(SDL_getenv("KHCOM_LOADSTATE")));
#ifndef __EMSCRIPTEN__
    setjmp(sResetJmp);
#endif
    IO16(0x130) = 0x3FF;
    AgbMain();
    Quit();
    return 0;
}
