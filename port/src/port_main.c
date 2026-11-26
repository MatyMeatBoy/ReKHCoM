/*
 * Entry point and frame loop.
 *
 * The game keeps its own main loop (AgbMain). Every time it waits for the
 * vertical blank, PortFrame() draws the frame, delivers the interrupts the
 * hardware would raise, presents the picture, mixes audio and paces the loop.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#if defined(__ANDROID__) && !defined(PORT_ANDROID64)
#include "../android/native/android_lifecycle_events.h"
#endif
#if !defined(PORT_SDL_BRIDGE) && !defined(__EMSCRIPTEN__)
#include "port/sdl_startup.h"
#endif
#if (defined(_WIN32) || defined(PORT_LINUX)) && !defined(PORT_GBA_RELOC)
#include "port/cursor_idle.h"
static PortCursorIdle sCursorIdle;
#endif
#if defined(_WIN32) && !defined(PORT_SDL_BRIDGE)
#define COBJMACROS
#include <dxgi1_5.h>
#include "port/video_vrr_windows.h"
#endif
#ifdef PORT_SDL_BRIDGE
#include "bridge_output.h"
extern int PortBridgeVideoCaps(void);
#endif

#include "port/pad_layout.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include "gba/keys.h"
#include <stdlib.h>
#include <string.h>

#if defined(PORT_LINUX) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include "port/port.h"
#include "port/port_audio_rate.h"
#include "port/port_netlink.h"
#include "port/port_logfile.h"
#include "port/loop_phases.h"
#if defined(__linux__) && __STDC_HOSTED__ && !defined(PORT_LINUX64_GUEST) /* getrusage: full Linux builds */
#include <sys/resource.h>
#include <time.h>
#endif
#include "port/port_version.h"
#include "mode.h"
#include "port/port_aspect.h"
#include "port/overlay_vertical_bands.h"
#include "port/gba_border_layout.h"
#include "port/lcd_grid.h"
#include "port/port_hd2d.h"
#include "port/port_hdlayout.h"
#include "port/port_hdsprites.h"
#include "../decompui/third_party/stb_image.h"
#include "port/port_ui.h"
#include "port/jiminy_layout.h"
#include "port/jiminy_paper.h"
#include "port/port_state.h"
#include "port/scene_framing.h"
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || defined(PORT_ANDROID64) || defined(PORT_TOUCH)
#include "port/port_android.h"
#endif
static void CrtScale(int* sx, int* sy); /* the CRT view (#202) */
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
extern Mode gModeJiminy;
extern Mode gModeEvent;
extern Mode gModeEventDebug;
extern int PortEventId(void);
extern float gPortAutoScaleF;
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
static int sRendererResetPending;
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
/* Accepted SDL presentation calls, not physical GPU scanout. Timestamp is
 * sampled immediately after successful RenderPresent; failed calls do not advance. */
static uint64_t sPresentSerial, sPresentAcceptedAt, sPresentGapTicks;
static uint64_t sPresentPaceEpoch, sPresentWaitTotal;
static void PaceBeforePresent(void);

void PortLog(const char* fmt, ...) {
    va_list ap;

    if (sLog == NULL)
        sLog = PortOpenSessionLog(PortConfigPath("kh_com.log"));
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

/* KHCOM_LOOP_PHASES (port/loop_phases.h): off, each mark is one test. */
static PortLoopPhases sLoop;

static void LoopMark(int phase) {
    if (sLoop.threshold)
        PortLoopPhasesMark(&sLoop, phase, SDL_GetPerformanceCounter());
}

/* The end of a cycle: a slow one leaves its line, with what the system did to this thread meanwhile (CPU time well
   under interval - wait: it was not running; involuntary switches; page faults that read from storage). */
static void LoopEnd(uint32_t frame) {
    static char line[640];
    double cpu = -1;
    long switches = -1, faults = -1;

    if (!sLoop.threshold)
        return;
#if defined(__linux__) && __STDC_HOSTED__ && !defined(PORT_LINUX64_GUEST) /* getrusage: full Linux builds */
    {
        static double lastCpu;
        static long lastSwitches, lastFaults;
        static int primed;
        struct timespec t;
        struct rusage use;
        /* one pair of reads a frame, so a slow frame has the previous one's values to compare with */
        if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t) == 0 && getrusage(1 /* RUSAGE_THREAD */, &use) == 0) {
            double now = t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
            if (primed) {
                cpu = now - lastCpu;
                switches = use.ru_nivcsw - lastSwitches;
                faults = use.ru_majflt - lastFaults;
            }
            primed = 1;
            lastCpu = now;
            lastSwitches = use.ru_nivcsw;
            lastFaults = use.ru_majflt;
        }
    }
#endif
    if (PortLoopPhasesEnd(&sLoop, line, sizeof(line))) {
        extern const char* PortHarnessModeName(void);
        PortLog("loop phases: frame %u %s %s | thread cpu %.2f ms, preempted %ld, major faults %ld",
                (unsigned)frame, PortHarnessModeName(), line, cpu, switches, faults);
        LoopMark(PORT_LOOP_REPORT);
    }
}

/* ---- presentation --------------------------------------------------------------- */

/* Fullscreen uses the configured display mode (closest the monitor offers);
   0x0 selects the desktop resolution without a mode change. */
#ifdef PORT_LINUX
/* port/linux/fbdev_linux.c: handhelds without a usable 32-bit GPU driver. */
int PortFbInit(int* w, int* h);
int PortFbActive(void);
SDL_Renderer* PortFbCreateRenderer(void);
void PortFbPresent(SDL_Renderer* renderer);
void PortFbWaitCopy(void);
void PortFbStopCopy(void);
bool PortFbCapturePair(const char* framebufferName, const char* rendererName);
void PortFbStageTicks(uint64_t* draw, uint64_t* copy);
bool PortFbDrawCropped3x(SDL_Renderer* renderer, const uint32_t* source, int stride);
int PortFbDrawWorldInspectFit(SDL_Renderer* renderer, const uint32_t* source, int stride);
int PortFbDrawJournalFit(SDL_Renderer* renderer, const uint32_t* source, int stride);
int PortFbDrawMenuFit(SDL_Renderer* renderer, const uint32_t* source, int stride, const uint8_t* hiddenRows);
int PortFbDrawAllmap2x(SDL_Renderer* renderer, const uint32_t* source, int stride);
int PortFbDrawFit2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, const SDL_FRect* dest);
int PortFbDrawNearest2x(SDL_Renderer* renderer, const uint32_t* src, int w, int h, int outH, const SDL_Rect* cover);
bool PortFbBlendOverlay4x(SDL_Renderer* renderer, const uint32_t* source, int pitch, const SDL_Rect* dest);
bool PortFbBlendOverlay2x(SDL_Renderer* renderer, const uint32_t* source, int stride, const SDL_Rect* dest);
bool PortFbBlendOverlayScaled(SDL_Renderer* renderer, const uint32_t* source, int width, int height, const SDL_Rect* dest);
bool PortFbBlendOverlayScaledSparse(SDL_Renderer* renderer, const uint32_t* source, int width, int height, const SDL_Rect* dest);
static int sFbW, sFbH;
#endif

/* SDL already creates an ALLOW_TEARING flip swap chain when DXGI supports it.
 * This proves eligibility, not that the user enabled VRR in the driver/monitor.
 * Never infer VRR from refresh rate or GPU brand, or use adaptive VSync (-1). */
static int VrrPresentEligible(void) {
#if defined(_WIN32) && !defined(PORT_SDL_BRIDGE)
    return PortWindowsVrrEligible(sRenderer);
#elif defined(PORT_SDL_BRIDGE)
    return (PortBridgeVideoCaps() & BRIDGE_OUTPUT_VRR_ELIGIBLE_CAP)!=0;
#endif
    return 0;
}

int PortVideoVrrOptionAvailable(void) {
#ifdef PORT_SDL_BRIDGE
    return (PortBridgeVideoCaps() & BRIDGE_OUTPUT_VRR_CONTROL_CAP)!=0;
#elif defined(_WIN32)
    return 1;
#else
    return 0;
#endif
}

static void ApplyVideoSync(int force) {
    static int cachedRequested=-1,cachedCapability=-1,cachedApplied=0;
    int requested=gPortConfig.vsync;
    int eligible=requested==2 && VrrPresentEligible();
    /* KHCOM_FAST runs unpaced: vsync would hold it to the display rate. */
    int interval=requested==0 || eligible || SDL_getenv("KHCOM_FAST")?0:1;
    int capability=eligible;
    if (!force && cachedRequested==requested && cachedCapability==capability && cachedApplied) return;
    int applied=SDL_SetRenderVSync(sRenderer,interval);
    if (!applied && eligible) {
        eligible=0; interval=1;
        applied=SDL_SetRenderVSync(sRenderer,interval);
    }
    /* A successful VSync fallback must not cache a failed VRR attempt as
       eligible: the periodic capability poll should retry the recovered path. */
    cachedRequested=requested; cachedCapability=eligible; cachedApplied=applied;
    static int lastRequested=-1,lastInterval=-1,lastEligible=-1,lastApplied=-1;
    if (lastRequested!=requested || lastInterval!=interval || lastEligible!=eligible || lastApplied!=applied) {
        PortLog("video-sync: requested=%d interval=%d applied=%d VRR-path=%s%s",requested,
            interval,applied,eligible?"eligible":"unavailable",
            requested==2?(eligible?" (monitor/driver must enable VRR)":" (VSync fallback)"):"");
        lastRequested=requested; lastInterval=interval; lastEligible=eligible; lastApplied=applied;
    }
}

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
    ApplyVideoSync(1);
    return;
#endif
    SDL_DisplayID display = SDL_GetDisplayForWindow(sWindow);
    SDL_DisplayMode mode;

    if (gPortConfig.viewMode == PORT_VIEW_CRT) {
        /* the CRT view asks the display for its own mode (320x240, 1920x240, 2560x240 or 640x480), and a window of
           that size; without such a mode the picture stays at that size in the middle of the desktop's */
        int sx, sy;
        CrtScale(&sx, &sy);
        if (gPortConfig.fullscreen &&
            SDL_GetClosestFullscreenDisplayMode(display, 320 * sx, 240 * sy, 0.0f, false, &mode) &&
            mode.w == 320 * sx && mode.h == 240 * sy)
            SDL_SetWindowFullscreenMode(sWindow, &mode);
        else
            SDL_SetWindowFullscreenMode(sWindow, NULL);
        if (!gPortConfig.fullscreen)
            SDL_SetWindowSize(sWindow, 320 * sx, 240 * sy);
    } else if (gPortConfig.fullscreen && gPortConfig.fullscreenWidth > 0 && gPortConfig.fullscreenHeight > 0
        && SDL_GetClosestFullscreenDisplayMode(display, gPortConfig.fullscreenWidth, gPortConfig.fullscreenHeight,
                                               0.0f, true, &mode)) {
        SDL_SetWindowFullscreenMode(sWindow, &mode);
    } else {
        SDL_SetWindowFullscreenMode(sWindow, NULL);
    }
    SDL_SetWindowFullscreen(sWindow, gPortConfig.fullscreen != 0);
    ApplyVideoSync(1);
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
int gPortSceneCameraAdjusted;
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
/* The glow must add to the actual HD picture, never an opaque blue-back copy. */
#include "port/port_screen_world_blend.h"
static PortScreenWorldBlend sWorldLight;
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

static int GbaBorderReady(void);
/* The selected aspect remains the outer frame's shape. All image/UI layers
 * share this useful viewport, so the frame cannot cover their text. */
static SDL_Rect ContentArea(int outW, int outH) {
    SDL_Rect area = AspectRect(outW, outH);
    int framed = GbaBorderReady();
    PortGbaBorderArea inside = PortGbaBorderViewport(area.w, area.h, framed);
    if (framed && gPortConfig.viewMode != PORT_VIEW_AUTO) {
        int canvasW, canvasH;
        PortGbaBorderArea picture;
        /* Config only: UpdateResolution may call this before the previous
         * frame's gPortScreen/fit/auto state has been replaced. */
        PortConfigView(gPortConfig.viewMode, &canvasW, &canvasH);
        picture = PortGbaBorderPictureArea(inside.w, inside.h, canvasW, canvasH,
                                          gPortConfig.scaleMode == PORT_SCALE_INTEGER);
        inside.x += picture.x;
        inside.y += picture.y;
        inside.w = picture.w;
        inside.h = picture.h;
    }
    area.x += inside.x;
    area.y += inside.y;
    area.w = inside.w;
    area.h = inside.h;
    return area;
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

/* GA36 4:3: the battle's opening fills the panel, keeping its proportions. */
static int Ga36CoverIntro(void) {
#ifdef PORT_LINUX
    static int enabled = -1;
    if (enabled < 0) enabled = SDL_getenv("KHCOM_GA36_BATTLE_COVER") != NULL;
    return enabled && sFitScale > 0 && CoverActive() && PortFixed480p() && PortFbActive();
#else
    return 0;
#endif
}

static float MenuScale(int outW, int outH) {
    /* menus and every GBA-size scene always fill the window's height */
    return SDL_min(outH / (float)GBA_SCREEN_H, outW / (float)GBA_SCREEN_W);
}

/* Auto view: size the canvas to the window every frame. */
int PortSceneFramingActive(void) {
    return gPortConfig.viewMode == PORT_VIEW_AUTO && PortSceneFramingUse(gPortSceneFraming,
        gCurrentMode == &gModeEvent || gCurrentMode == &gModeEventDebug, PortEventId());
}

static void UpdateResolution(void) {
    int outW, outH, w, h;
    int sceneScale = 0, hudBase = 0;
    float dialogueBase = 0;

    gPortSceneCameraAdjusted = 0;

    if (gPortConfig.viewMode != PORT_VIEW_AUTO) {
        sAutoScale = 0;
        PortConfigView(gPortConfig.viewMode, &w, &h);
    } else {
        if (!SDL_GetCurrentRenderOutputSize(sRenderer, &outW, &outH) || outW <= 0 || outH <= 0)
            return;
        {
            SDL_Rect area = ContentArea(outW, outH);
            outW = area.w;
            outH = area.h;
        }
        PortConfigAutoView(outW, outH, gCurrentMode == &gModeBattle || gCurrentMode == &gModeVsbattle,
                           &w, &h, &sAutoScale);
        hudBase = sAutoScale;
        dialogueBase = gPortAutoScaleF;
        if (!PortUiMenuCanvas() && PortSceneFramingActive()) {
            sceneScale = PortSceneFramingScale(outW, outH);
            sAutoScale = sceneScale;
            gPortAutoScaleF = (float)sceneScale;
            w = SDL_max(GBA_SCREEN_W, (outW + sceneScale - 1) / sceneScale);
            h = SDL_max(GBA_SCREEN_H, (outH + sceneScale - 1) / sceneScale);
            gPortSceneCameraAdjusted = 1;
        }
    }
    /* screen layer: the 160 GBA lines at the window's height */
    gPortOverlayW = GBA_SCREEN_W;
    sOverlayScale = 0;
    if (gPortConfig.viewMode == PORT_VIEW_AUTO) {
        float f = sceneScale > 0 ? dialogueBase : MenuScale(outW, outH);
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
        float f = sceneScale > 0 ? (float)sceneScale : MenuScale(outW, outH);
        w = SDL_min((int)SDL_ceilf(outW / f), PORT_MAX_W);
        h = GBA_SCREEN_H;
        sFitScale = f;
        sAutoScale = sceneScale;
    }
    /* HUD layer: a larger whole-number scale, keeping at least 144 lines and
       the GBA width so corner-anchored elements never collide. */
    {
        extern int gPortWorldScale;
        gPortWorldScale = sAutoScale;
    }
    gPortHudScale = 0;
    if (gPortConfig.viewMode == PORT_VIEW_AUTO && sAutoScale > 0 &&
        (sFitScale == 0 || sceneScale > 0) && (gPortConfig.hudSize > 0 || sceneScale > 0)) {
        int base = sceneScale > 0 ? hudBase : sAutoScale;
        int hs = base + gPortConfig.hudSize;
        while (hs > base && (outH / hs < 144 || outW / hs < GBA_SCREEN_W))
            hs--;
        if (sceneScale > 0 ? hs != sceneScale : hs > sAutoScale) {
            gPortHudScale = hs;
            gPortHudW = SDL_min((outW + hs - 1) / hs, PORT_MAX_W);
            gPortHudH = SDL_min((outH + hs - 1) / hs, PORT_MAX_H);
        }
    }
    /* The screen compositor shares scanline storage with the world. Keep its
       full horizontal span even when the camera shows a smaller centre crop;
       OutputRect still presents the world at sceneScale, never at UI scale. */
    if (sceneScale > 0 && w < gPortOverlayW) w = gPortOverlayW;
    if (w != gPortScreen.width || h != gPortScreen.height)
        PortPpuSetResolution(w, h);
}

/* GBA classic (gba_border 1..3): the GBA picture inside a decorative border of 384x216 GBA pixels, the picture's
   hole at (72, 28) (gen_gba_borders.py), both at the largest whole scale that fits the window. */
#include "port/gba_borders_png.inc"
#define BORDER_W 384
#define BORDER_H 216
static SDL_Texture* sBorderTex;
static SDL_Texture* sBorderMaterialTex;
static SDL_Texture* sBorderAdaptiveTex;
static SDL_Texture *sLcdGridXTex, *sLcdGridYTex;
static PortLcdGridKey sLcdGridKey, sLcdGridAttemptKey;
static int sLcdGridReady, sLcdGridAttempted;
static int sBorderShown; /* which border sBorderTex holds */
static int sBorderAttempted; /* Do not retry a failed selection every frame. */

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

#include "port_gba_border.inc"

/* the CRT view's exact scale (#202): x and y pixels per canvas pixel */
static void CrtScale(int* sx, int* sy) {
    static const int kX[4] = { 1, 6, 8, 2 }, kY[4] = { 1, 1, 1, 2 };
    int o = SDL_clamp(gPortConfig.crtOutput, 0, 3);
    *sx = kX[o];
    *sy = kY[o];
}

static void OutputRect(int outW, int outH, SDL_FRect* dst) {
    float sw = (float)gPortScreen.width, sh = (float)gPortScreen.height;
    float scale;
    int framed = GbaBorderReady();

    if (gPortConfig.viewMode == PORT_VIEW_CRT && !framed) { /* exactly that scale, centred: no fit, no zoom */
        int sx, sy;
        CrtScale(&sx, &sy);
        dst->w = sw * sx;
        dst->h = sh * sy;
        dst->x = (float)(int)((outW - dst->w) / 2);
        dst->y = (float)(int)((outH - dst->h) / 2);
        return;
    }

    if (sFitScale > 0) {
        scale = framed ? SDL_min(sFitScale, SDL_min(outW / sw, outH / sh)) : (float)sFitScale;
        dst->w = sw * scale;
        dst->h = sh * scale;
        dst->x = (float)(int)((outW - dst->w) / 2);
        dst->y = (float)(int)((outH - dst->h) / 2);
        return;
    }
    if (sAutoScale > 0) {
        /* the Auto view's scale (whole with Integer scale on); the canvas
           may overhang the window by < 1 pixel */
        extern float gPortAutoScaleF;
        float f = gPortAutoScaleF > 0 ? gPortAutoScaleF : (float)sAutoScale;
        if (framed) f = SDL_min(f, SDL_min(outW / sw, outH / sh));
        dst->w = framed ? sw * f : SDL_roundf(sw * f);
        dst->h = framed ? sh * f : SDL_roundf(sh * f);
        dst->x = (float)((outW - (int)dst->w) / 2);
        dst->y = (float)((outH - (int)dst->h) / 2);
        return;
    }

    switch (gPortConfig.scaleMode) {
        case PORT_SCALE_STRETCH:
            if (framed) {
                scale = SDL_min(outW / sw, outH / sh);
                break; /* A frame never changes the source pixel proportions. */
            }
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


#include "port_lcd_grid.inc"

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
extern int PortMoogleMenu(void);
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

/* Some menu bottom bands are sprites while entering, before their BG copy
   reaches the edge. Carry only a currently visible uniform 64-pixel run;
   never a badge, tab, cached color or a band waiting behind a window. */
static uint32_t MenuVisibleBottomBand(const uint32_t* row, int width, int edgeX, int side, int y) {
    int step = side < 0 ? 1 : -1, k;
    uint32_t color;
    if (!row || y < 132 || y >= GBA_SCREEN_H || edgeX < 0 || edgeX >= width ||
        edgeX + 63 * step < 0 || edgeX + 63 * step >= width)
        return 0;
    color = row[edgeX];
    for (k = 1; k < 64; k++)
        if (row[edgeX + k * step] != color)
            return 0;
    return color;
}

static uint32_t ColorPaperBackdrop(int sx, int sy) {
    /* One shared background for both sides. Counting exact colours
       matters during fades: 4-bit bins merged grey paper with black
       artwork, making only the right margin go dark. */
    static uint16_t count[8192];
    static uint32_t colors[8192];
    uint32_t bd = 0;
    int best = 0;
    memset(count, 0, sizeof(count));
    for (int i = 0; i < GBA_SCREEN_H * SIDE_STRIP * 2; i++) {
        int strip = i / (GBA_SCREEN_H * SIDE_STRIP);
        int offset = i % (GBA_SCREEN_H * SIDE_STRIP);
        uint32_t c = StripPixel(sx + strip * (GBA_SCREEN_W - SIDE_STRIP), sy,
                                offset % SIDE_STRIP, offset / SIDE_STRIP);
        unsigned q = (c * 2654435761u) >> 19;
        while (count[q] && colors[q] != c) q = (q + 1) & 8191;
        colors[q] = c;
        if (++count[q] > best) {
            best = count[q];
            bd = c;
        }
    }
    return bd;
}

/* Extend plain menu edges without stretching a label or repeating artwork. */
static uint32_t MenuEdgeColor(int sx, int sy) {
    const uint32_t* row = sComposite + sy * gPortScreen.width + sx;
    uint32_t color = 0;
    int balance = 0, count = 0;
    for (int x = 0; x < GBA_SCREEN_W; x++) {
        uint32_t pixel = row[x] & 0x00FFFFFFu;
        if (!balance) color = pixel;
        balance += pixel == color ? 1 : -1;
    }
    for (int x = 0; x < GBA_SCREEN_W; x++)
        count += (row[x] & 0x00FFFFFFu) == color;
    return count > GBA_SCREEN_W / 2 ? color : 0;
}

/* The margin colour of one edge. On the handheld's 4:3 panel a line sliding
   past the edge during a menu's entry must not flash across the whole margin:
   take what most of the outer rows agree on. */
static uint32_t MenuMarginColor(int sx, int sy, int inward) {
    static int steady = -1;
    if (steady < 0) steady = SDL_getenv("KHCOM_GA36_MENU_43_EXTEND") && PortFixed480p();
    if (!steady) return MenuEdgeColor(sx, sy);
    uint32_t colors[6];
    for (int i = 0; i < 6; i++) colors[i] = MenuEdgeColor(sx, sy + i * inward);
    int best = 0, votes = 0;
    for (int i = 0; i < 6; i++) {
        int n = 0;
        for (int k = 0; k < 6; k++) n += colors[k] == colors[i];
        if (colors[i] && n > votes) { votes = n; best = i; }
    }
    return colors[best];
}

/* Non-wide GBA menus own only their 240x160 artwork.  Carry each authored
   edge colour into 4:3's unused vertical rows without touching game pixels. */
static void FillEnhancedVerticalColorMargins(void) {
    const int top = gPortScreen.originY, bottom = top + GBA_SCREEN_H, w = gPortScreen.width;
    uint32_t colors[2];
    int y, x;
    extern int PortUiMenuCanvas(void);
    if (gPortConfig.aspect != PORT_ASPECT_4_3 || gPortConfig.graphicsMode != PORT_MODE_GBA_ENHANCED ||
        gPortScreen.wide || top <= 0 || bottom >= gPortScreen.height)
        return;
    /* the Options panel covers the whole canvas: its top and bottom rows are
       its own (title, help), not the game's margins */
    if (PortUiMenuCanvas())
        return;
    colors[0] = MenuMarginColor(gPortScreen.originX, top, 1);
    colors[1] = MenuMarginColor(gPortScreen.originX, bottom - 1, -1);
    for (y = 0; y < top; y++) for (x = 0; x < w; x++) sComposite[y * w + x] = colors[0];
    for (y = bottom; y < gPortScreen.height; y++) for (x = 0; x < w; x++) sComposite[y * w + x] = colors[1];
}

/* GA36 4:3: deck, map cards and the journal carry their last row down to the
   screen edge, so panels and pages reach it instead of ending on a flat bar. */
static int Menu43Extend(void) {
#ifdef PORT_LINUX
    extern Mode gModeDeck, gModeMapinspect, gModeJiminy;
    const char* extend;
    PORT_ENV_ONCE(extend, SDL_getenv, "KHCOM_GA36_MENU_43_EXTEND");
    return extend && PortFixed480p() && PortFbActive() && gPortConfig.aspect == PORT_ASPECT_4_3 &&
        gPortConfig.graphicsMode == PORT_MODE_GBA_ENHANCED &&
        (gCurrentMode == &gModeDeck || gCurrentMode == &gModeMapinspect || gCurrentMode == &gModeJiminy);
#else
    return 0;
#endif
}

/* The journal closes with the top bar mirrored: the same height, colour and
   bright edge line, drawn over the book so its rings never end cut or
   stretched. Call after the picture. */
static void DrawJournalFooterBar(const SDL_FRect* src, const SDL_FRect* dst, int w, int h) {
    const uint32_t* column = sComposite + (int)src->y * gPortScreen.width + (int)src->x + 200;
    const uint32_t bar = column[0] & 0xFFFFFFu;
    int row = 1;
    while (row < 16 && (column[row * gPortScreen.width] & 0xFFFFFFu) == bar) row++;
    if (!bar || row >= 16) return;
    const uint32_t edge = column[row * gPortScreen.width] & 0xFFFFFFu;
    const float scale = dst->h / src->h, height = dst->y + row * scale;
    SDL_FRect line = { 0, (float)(int)(h - height - scale), (float)w, scale };
    /* The page already ends on that bright line left of the book: carry the
       same line across, over the book, instead of a second one below it. */
    for (int r = (int)src->h - 1; r >= (int)src->h - 20; r--)
        if ((column[r * gPortScreen.width - 200] & 0xFFFFFFu) == edge) {
            line.y = SDL_floorf(dst->y + r * scale);
            line.h = SDL_ceilf(dst->y + (r + 1) * scale) - line.y;
            break;
        }
    const SDL_FRect fill = { 0, line.y + line.h, (float)w, h - line.y - line.h };
    SDL_SetRenderDrawColor(sRenderer, (edge >> 16) & 255, (edge >> 8) & 255, edge & 255, 255);
    SDL_RenderFillRect(sRenderer, &line);
    SDL_SetRenderDrawColor(sRenderer, (bar >> 16) & 255, (bar >> 8) & 255, bar & 255, 255);
    SDL_RenderFillRect(sRenderer, &fill);
}

static void DrawMenu43Bottom(const SDL_FRect* src, const SDL_FRect* dst, int w, int h) {
    const float bottom = SDL_clamp(SDL_floorf(dst->y + dst->h), 0.0f, (float)h);
    extern Mode gModeJiminy;
    if (gCurrentMode == &gModeJiminy) return; /* DrawJournalFooterBar, over the picture */
    const int count = (int)src->w, last = (int)(src->y + src->h) - 1;
    const float step = dst->w / src->w;
    if (bottom >= h || dst->x > 0 || dst->x + dst->w < w || count <= 0 || count > PORT_MAX_W || last < 7) return;
    static uint32_t line[PORT_MAX_W];
    const uint32_t* edge = sComposite + last * gPortScreen.width + (int)src->x;
    const uint32_t* inner = edge - 4 * gPortScreen.width;
    memcpy(line, edge, count * sizeof(*line));
    const uint32_t ground = MenuMarginColor((int)src->x, (int)src->y, 1);
    /* A line sliding past the edge during a menu's entry: a long run over a
       plain stretch of another colour. Take the colour behind it. */
    for (int x = 0, end; x < count; x = end) {
        for (end = x + 1; end < count && line[end] == line[x]; end++) {}
        if (end - x < 40 || (line[x] & 0xFFFFFFu) == ground) continue;
        int behind = 0;
        for (int k = x; k < end; k++) behind += inner[k] == inner[x + (end - x) / 2];
        if (inner[x + (end - x) / 2] != line[x] && behind * 10 >= (end - x) * 9)
            for (int k = x; k < end; k++) line[k] = inner[x + (end - x) / 2];
    }
    /* Up to three odd pixels between equal neighbours are an icon's tip. */
    for (int x = 1, end; x < count - 1; x = end) {
        for (end = x + 1; end < count && line[end] != line[x - 1]; end++) {}
        if (line[x] == line[x - 1] || end >= count || end - x > 3) { end = x + 1; continue; }
        for (int k = x; k < end; k++) line[k] = line[x - 1];
    }
    /* Only the card panel, which touches one side of the screen, reaches the
       bottom. Whatever else ends there (a deck's box) closes on the screen's
       own background. */
    if (ground) {
        int left = 0, right = count;
        while (left < count && (line[left] & 0xFFFFFFu) != ground) left++;
        while (right > left && (line[right - 1] & 0xFFFFFFu) != ground) right--;
        if (left < count)
            for (int x = left; x < right; x++) line[x] = ground;
    }
    for (int x = 0, end; x < count; x = end) {
        for (end = x + 1; end < count && line[end] == line[x]; end++) {}
        const SDL_FRect band = { dst->x + x * step, bottom, (end - x) * step, h - bottom };
        SDL_SetRenderDrawColor(sRenderer, (line[x] >> 16) & 255, (line[x] >> 8) & 255, line[x] & 255, 255);
        SDL_RenderFillRect(sRenderer, &band);
    }
}

/* GA36 4:3: the world map at a smaller scale than the panel's width, so it
   does not look zoomed in. 0 keeps the width fit. */
static float AllmapScale(void) {
#ifdef PORT_LINUX
    extern Mode gModeAllmap;
    static float scale = -1;
    if (scale < 0) {
        const char* value = SDL_getenv("KHCOM_GA36_ALLMAP_SCALE");
        scale = value ? (float)SDL_atof(value) : 0;
        if (scale < 2.0f || scale > 2.66f) scale = 0;
    }
    return scale > 0 && gCurrentMode == &gModeAllmap && PortFixed480p() && PortFbActive() &&
        gPortConfig.aspect == PORT_ASPECT_4_3 ? scale : 0;
#else
    return 0;
#endif
}

/* GA36 4:3 world map at its own fitted zoom: the header goes up to the panel's
   top and the footer down to its bottom; the band rows next to them (one
   colour across the width) stretch over the space they leave, so the view in
   between is no longer framed by the picture's margins. */
static int AllmapBandRow(int sx, int sy, int first, int step, int count, int w) {
    int seen = 0;
    for (int i = 0, r = first; i < count; i++, r += step) {
        const uint32_t* row = sComposite + (size_t)(sy + r) * gPortScreen.width + sx;
        int uniform = 1;
        for (int x = 1; x < w && uniform; x++) uniform = row[x] == row[0];
        if (!uniform) seen = 1;
        else if (seen) return r;
    }
    return -1;
}

int gPortAllmapCoverRows; /* mode_allmap.c: the header rows the rooms stay out of (0: none) */
static void DrawAllmapAnchored(const SDL_FRect* src, const SDL_FRect* dst, int w, int h, bool stale, bool direct) {
#ifdef PORT_LINUX
    gPortAllmapCoverRows = 0;
    extern Mode gModeAllmap;
    const char* on;
    PORT_ENV_ONCE(on, SDL_getenv, "KHCOM_GA36_ALLMAP_ANCHOR");
    if (!on || gCurrentMode != &gModeAllmap || AllmapScale() > 0 || !PortFixed480p() || !PortFbActive() ||
        gPortConfig.aspect != PORT_ASPECT_4_3 || dst->x != 0 || dst->w != w || dst->y < 8 ||
        dst->y + dst->h > h - 8 || src->w != GBA_SCREEN_W || src->h != GBA_SCREEN_H)
        return;
    const int sx = (int)src->x, sy = (int)src->y, H = GBA_SCREEN_H, W = GBA_SCREEN_W;
#define ALLMAP_PX(r, c) sComposite[(size_t)(sy + (r)) * gPortScreen.width + sx + (c)]
    /* the band row under the header (the last one found when a map tile hides every candidate) */
    static int lastA = -1, bigMap;
    static Mode* lastMode;
    if (lastMode != gCurrentMode) { lastMode = gCurrentMode; lastA = -1; bigMap = 0; }
    int a = AllmapBandRow(sx, sy, 0, 1, H / 3, W);
    /* A floor larger than the view scrolls its rooms right up to the header, as on the GBA: once a room shows under
       it, the band row found below the header moves down. The header still goes to the panel's top, at its own band
       row (the smallest seen, 16 on the GBA), and covers what reaches it there: the rooms pass under it as they pass
       under the footer. */
    if (a > 17 || (lastA > 0 && a > lastA + 1)) bigMap = 1; /* the header's band row is 16 */
    if (a <= 0) a = lastA;
    if (a > 0 && (lastA <= 0 || a < lastA)) lastA = a;
    if (bigMap) gPortAllmapCoverRows = a = 16;
    /* The footer: its bar starts at the left edge. Rows' band colours are
       those covering a fifth of the row (the dithered bands have two). */
    uint32_t bands[GBA_SCREEN_H][4];
    int nb[GBA_SCREEN_H] = {0}, f = -1, barRight = 0;
    for (int r = 0; r < H; r++) {
        if (r > a && r < H / 2) continue;
        uint32_t colors[16];
        int counts[16], n = 0;
        for (int c = 0; c < W; c++) {
            const uint32_t p = ALLMAP_PX(r, c);
            int k = 0;
            while (k < n && colors[k] != p) k++;
            if (k == n) {
                if (n == 16) continue;
                colors[n] = p; counts[n++] = 0;
            }
            counts[k]++;
        }
        for (int k = 0; k < n; k++)
            if (counts[k] >= W / 5 && nb[r] < 4) bands[r][nb[r]++] = colors[k];
    }
#define ALLMAP_BAND(r, p) ((nb[r] > 0 && bands[r][0] == (p)) || (nb[r] > 1 && bands[r][1] == (p)) || \
                           (nb[r] > 2 && bands[r][2] == (p)) || (nb[r] > 3 && bands[r][3] == (p)))
    for (int r = H / 2 + 1; r < H && f < 0; r++)
        if (!ALLMAP_BAND(r, ALLMAP_PX(r, 0))) f = r - 1;
    if (f > H / 2 && nb[f - 1] > 0) {
        for (int r = f; r < H; r++) {
            int x = 0;
            while (x < W && !ALLMAP_BAND(r, ALLMAP_PX(r, x))) x++;
            if (x < W / 2) barRight = SDL_max(barRight, x);
        }
    } else {
        f = -1;
    }
    if (a <= 0 && f < 0)
        return;
    /* the header covers the map: what is neither band nor the header bar (which starts at the left edge) in its rows
       and in the band row stretched below it is a map tile, drawn as the band behind it */
    int headRight[GBA_SCREEN_H] = {0};
    for (int r = 0; r < a; r++) {
        /* the bar ends where the backdrop (the row's right edge colour) runs for 8 pixels */
        const uint32_t back = ALLMAP_PX(r, W - 1);
        int x = 0, run = 0;
        while (x < W && run < 8) run = ALLMAP_PX(r, x++) == back ? run + 1 : 0;
        headRight[r] = run >= 8 ? SDL_min(x - run + 2, W) : W; /* +2: the arrow's tip pixel */
    }
#define ALLMAP_UNDER(r, c) ((c) >= 2 && ALLMAP_BAND(r, ALLMAP_PX(r, (c) - 2)) ? ALLMAP_PX(r, (c) - 2) : bands[r][0])
    {
        /* The menu fit drew the picture straight to the panel (rows 26..451, the
           same nearest sampling): the anchored parts are rows moved in place,
           with no texture upload or scaling. */
        extern bool PortFbBeginGlyphBatch(SDL_Renderer*);
        extern void PortFbMoveRows(int, int, int), PortFbRepeatRow(int, int, int);
        extern void PortFbFillRect(int, int, int, int, uint32_t);
        if (direct && w == 640 && h == 480 && dst->y == 26 && PortFbBeginGlyphBatch(sRenderer)) {
            const unsigned stepY = (160u << 16) / 426u, stepX = (240u << 16) / 640u;
            int firstY[GBA_SCREEN_H + 1], firstX[GBA_SCREEN_W + 1];
            for (int r = 0, y = 0; r <= H; r++) {
                while (y < 426 && ((stepY / 2 + (unsigned)y * stepY) >> 16) < (unsigned)r) y++;
                firstY[r] = y;
            }
            for (int c = 0, x = 0; c <= W; c++) {
                while (x < 640 && ((stepX / 2 + (unsigned)x * stepX) >> 16) < (unsigned)c) x++;
                firstX[c] = x;
            }
            const int top = 26, below = 480 - (26 + 426);
            if (a > 0) {
                PortFbMoveRows(0, top, firstY[a]);
                PortFbRepeatRow(top + firstY[a], firstY[a], top);
                for (int r = 0; r <= a; r++)
                    for (int c = r < a ? headRight[r] : 0; c < W && nb[r] > 0; c++) {
                        const uint32_t p = ALLMAP_PX(r, c);
                        if (ALLMAP_BAND(r, p)) continue;
                        const int y0 = r < a ? firstY[r] : firstY[a], rows = r < a ? firstY[r + 1] - firstY[r] : top;
                        PortFbFillRect(firstX[c], y0, firstX[c + 1] - firstX[c], rows, ALLMAP_UNDER(r, c) | 0xFF000000u);
                    }
            }
            if (f > 0) {
                const int from = top + firstY[f];
                PortFbMoveRows(from + below, from, 426 - firstY[f]);
                PortFbFillRect(0, from, 640, below, bands[f - 1][0] | 0xFF000000u);
                for (int pass = 0; pass < 2; pass++)
                    for (int r = f; r < H; r++)
                        for (int c = barRight; c < W && nb[r] > 0; c++) {
                            uint32_t p = ALLMAP_PX(r, c);
                            if (ALLMAP_BAND(r, p))
                                continue;
                            if (!pass)
                                p = c >= 2 && ALLMAP_BAND(r, ALLMAP_PX(r, c - 2)) ? ALLMAP_PX(r, c - 2) : bands[r][0];
                            PortFbFillRect(firstX[c], top + firstY[r] + (pass ? 0 : below), firstX[c + 1] - firstX[c],
                                           firstY[r + 1] - firstY[r], p | 0xFF000000u);
                        }
            }
            static bool loggedDirect;
            if (!loggedDirect) { PortLog("ga36: world map anchored in place (rows %d/%d)", a, f); loggedDirect = true; }
            return;
        }
    }
    if (stale)
        SDL_UpdateTexture(sTexture, NULL, sComposite, gPortScreen.width * 4);
    const float s = dst->h / H, up = dst->y, down = h - (dst->y + dst->h);
    SDL_SetTextureScaleMode(sTexture, SDL_SCALEMODE_NEAREST);
    if (a > 0) {
        const SDL_FRect bandSrc = {(float)sx, (float)(sy + a), (float)W, 1}, bandDst = {0, a * s, (float)w, up};
        const SDL_FRect partSrc = {(float)sx, (float)sy, (float)W, (float)a}, partDst = {0, 0, (float)w, a * s};
        SDL_RenderTexture(sRenderer, sTexture, &bandSrc, &bandDst);
        SDL_RenderTexture(sRenderer, sTexture, &partSrc, &partDst);
        for (int r = 0; r <= a; r++)
            for (int c = r < a ? headRight[r] : 0; c < W && nb[r] > 0; c++) {
                const uint32_t p = ALLMAP_PX(r, c);
                if (ALLMAP_BAND(r, p)) continue;
                const uint32_t q = ALLMAP_UNDER(r, c);
                const float x0 = SDL_floorf(c * s), y0 = SDL_floorf(r < a ? r * s : a * s);
                const SDL_FRect at = {x0, y0, SDL_floorf((c + 1) * s) - x0 + 1,
                                      (r < a ? SDL_floorf((r + 1) * s) : SDL_floorf(a * s + up)) - y0 + 1};
                SDL_SetRenderDrawColor(sRenderer, (q >> 16) & 255, (q >> 8) & 255, q & 255, 255);
                SDL_RenderFillRect(sRenderer, &at);
            }
    }
    if (f > 0) {
        /* the bottom rows go down; what is not band nor footer bar there (the
           world's logo) stays where it was, so it is never split */
        const uint32_t gap = bands[f - 1][0];
        const SDL_FRect gapDst = {0, up + f * s, (float)w, down};
        const SDL_FRect partSrc = {(float)sx, (float)(sy + f), (float)W, (float)(H - f)};
        const SDL_FRect partDst = {0, up + f * s + down, (float)w, (H - f) * s};
        SDL_SetRenderDrawColor(sRenderer, (gap >> 16) & 255, (gap >> 8) & 255, gap & 255, 255);
        SDL_RenderFillRect(sRenderer, &gapDst);
        SDL_RenderTexture(sRenderer, sTexture, &partSrc, &partDst);
        for (int pass = 0; pass < 2; pass++)
            for (int r = f; r < H; r++)
                for (int c = barRight; c < W && nb[r] > 0; c++) {
                    uint32_t p = ALLMAP_PX(r, c);
                    if (ALLMAP_BAND(r, p))
                        continue;
                    if (!pass) /* the moved copy: the band under it */
                        p = c >= 2 && ALLMAP_BAND(r, ALLMAP_PX(r, c - 2)) ? ALLMAP_PX(r, c - 2) : bands[r][0];
                    /* whole output pixels, tiling as the picture's own nearest sampling does */
                    const float base = up + (pass ? 0 : down), x0 = SDL_floorf(c * s), y0 = SDL_floorf(base + r * s);
                    const float grow = pass ? 0 : 1; /* the band over the moved copy also takes rounding's edge */
                    const SDL_FRect at = {x0, y0, SDL_floorf((c + 1) * s) - x0 + grow,
                                          SDL_floorf(base + (r + 1) * s) - y0 + grow};
                    SDL_SetRenderDrawColor(sRenderer, (p >> 16) & 255, (p >> 8) & 255, p & 255, 255);
                    SDL_RenderFillRect(sRenderer, &at);
                }
    }
#undef ALLMAP_UNDER
#undef ALLMAP_BAND
#undef ALLMAP_PX
    static bool logged;
    if (!logged) { PortLog("ga36: world map header/footer anchored (rows %d/%d, bar %d)", a, f, barRight); logged = true; }
#else
    (void)src; (void)dst; (void)w; (void)h; (void)stale; (void)direct;
#endif
}

/* Its backdrop is made of horizontal bands: the edge columns carry them on. */
static void DrawAllmapSides(const SDL_FRect* src, const SDL_FRect* dst, int w) {
    for (int side = 0; side < 2; side++) {
        const SDL_FRect column = { side ? src->x + src->w - 1 : src->x, src->y, 1, src->h };
        const SDL_FRect area = { side ? dst->x + dst->w : 0, dst->y, side ? w - dst->x - dst->w : dst->x, dst->h };
        if (area.w <= 0) continue;
        SDL_SetTextureScaleMode(sTexture, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(sRenderer, sTexture, &column, &area);
    }
}

static void DrawEnhancedVerticalMargins(const SDL_FRect* src, const SDL_FRect* dst, int w, int h) {
    if (gPortConfig.aspect != PORT_ASPECT_4_3 || gPortConfig.graphicsMode != PORT_MODE_GBA_ENHANCED ||
        sFitScale > 0 || (dst->y <= 0 && dst->y + dst->h >= h))
        return;
    if (sSideStyle != 1 && sSideStyle != 2 && sSideStyle != 4 && sSideStyle != 5 && sSideStyle != 9 &&
        !(sSideStyle == 12 && Menu43Extend()))
        return;
    const int sx = (int)src->x, sy = (int)src->y;
    const float top = SDL_clamp(SDL_ceilf(dst->y - 0.5f), 0.0f, (float)h);
    const float bottom = SDL_clamp(SDL_floorf(dst->y + dst->h), 0.0f, (float)h);
    const SDL_FRect bands[2] = { { 0, 0, (float)w, top }, { 0, bottom, (float)w, h - bottom } };
    uint32_t colors[2];
    if (sSideStyle == 1) {
        colors[0] = colors[1] = ColorPaperBackdrop(sx, sy);
    } else {
        colors[0] = MenuMarginColor(sx, sy, 1);
        colors[1] = MenuMarginColor(sx, sy + GBA_SCREEN_H - 1, -1);
    }
    for (int i = 0; i < 2; i++) {
        const uint32_t color = colors[i];
        if (bands[i].h <= 0 || !color) continue;
        SDL_SetRenderDrawColor(sRenderer, (color >> 16) & 255, (color >> 8) & 255, color & 255, 255);
        SDL_RenderFillRect(sRenderer, &bands[i]);
    }
    if (Menu43Extend()) DrawMenu43Bottom(src, dst, w, h);
}

static void SideBar(int sx, int sy, int side, float y, float scale, float x0, float x1, int edgeX) {
    float w = SIDE_STRIP * scale;
    int band;
    int backgroundOnly = PortMenuBackgroundOnly();
    int moogleMenu = PortMoogleMenu();

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
            uint32_t visibleBottom = sSideStyle == 4 ? MenuVisibleBottomBand(
                &gPortFrame[(sy + yy) * gPortScreen.width], gPortScreen.width, edgeX, side, yy) : 0;
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
                *px = moogleMenu ? bgRow[edgeX] : bgRow[sample];
                /* The last charge tab's oval touches the GBA edge. Its ink
                   is foreground decoration, never a horizontal background
                   band. Continue the clear gutter only beside the tabs; lower
                   rows retain their own shading and bottom transition,
                   using this frame's BG so fades stay synchronized. */
                if (moogleMenu == 2 && side > 0 && yy >= 16 && yy < 32)
                    *px = gPortMenuBackground[(sy + 32) * gPortScreen.width + edgeX];
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
                if (visibleBottom)
                    *px = visibleBottom;
            }
        }
        /* The tab strip: a line between rows of the panel's own colour is
           the end of a tab's art at the screen edge (the eye tab's stroke
           on the right), not the panel, so the margin keeps the panel plain. */
        /* Only the right end has the eye's isolated stroke. At the left
           end this is the first tab's real top border; during a dark fade
           its body and the panel briefly quantize to the same black. */
        if (sSideStyle == 4 && side == sMenuPanelSide && side > 0)
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
        int i;
        uint32_t bd = ColorPaperBackdrop(side < 0 ? sx : sx - (GBA_SCREEN_W - SIDE_STRIP), sy);
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

#if defined(PORT_LINUX) && defined(__ARM_NEON)
static SDL_Rect BlurOpaqueCover(const SDL_FRect* picture, int w, int h) {
    SDL_Rect cover = {0};
    if (picture) {
        /* Omit only complete SIMD groups strictly under the opaque picture. */
        const int left = (SDL_clamp((int)SDL_ceilf(picture->x) + 1, 0, w) + 7) & ~7;
        const int right = SDL_clamp((int)SDL_floorf(picture->x + picture->w) - 1, 0, w) & ~7;
        const int top = SDL_clamp((int)SDL_ceilf(picture->y) + 1, 0, h);
        const int bottom = SDL_clamp((int)SDL_floorf(picture->y + picture->h) - 1, 0, h);
        if (right > left && bottom > top)
            cover = (SDL_Rect){left, top, right - left, bottom - top};
    }
    return cover;
}
#endif

static void PresentBlurTexture(const uint32_t* pixels, int w, int h, const SDL_FRect* picture) {
#if defined(PORT_LINUX) && defined(__ARM_NEON)
    const char* directBlur;
    PORT_ENV_ONCE(directBlur, SDL_getenv, "KHCOM_FBDEV_DIRECT_BLUR");
    const char* visibleBlur;
    PORT_ENV_ONCE(visibleBlur, SDL_getenv, "KHCOM_FBDEV_VISIBLE_BLUR");
    SDL_Rect cover = BlurOpaqueCover(visibleBlur ? picture : NULL, w, h);
    if (directBlur && w == BLUR_OUT_W * 2 && PortFixed480p() &&
        PortFbDrawNearest2x(sRenderer, pixels, BLUR_OUT_W, BLUR_OUT_H, h, cover.w ? &cover : NULL))
        return;
#else
    (void)pixels; (void)w; (void)h; (void)picture;
#endif
    SDL_RenderTexture(sRenderer, sBlurTex, NULL, NULL);
}

static void DrawBlurredReflection(const SDL_FRect* src, int outW, int outH, const SDL_FRect* cover) {
    static uint32_t cell[BLUR_W * BLUR_H], uploaded[BLUR_W * BLUR_H], big[BLUR_OUT_W * BLUR_OUT_H];
    static int uploadedValid;
#if defined(PORT_LINUX) && defined(__ARM_NEON)
    static bool partialValid;
    static SDL_Rect partialCover;
    const char *partialInterpolation, *directBlur;
    PORT_ENV_ONCE(partialInterpolation, SDL_getenv, "KHCOM_BLUR_PARTIAL_INTERPOLATION");
    PORT_ENV_ONCE(directBlur, SDL_getenv, "KHCOM_FBDEV_DIRECT_BLUR");
    const SDL_Rect hidden = BlurOpaqueCover(partialInterpolation && directBlur && PortFixed480p() &&
                                           outW == 640 && outH == 480 ? cover : NULL, outW, outH);
#endif
    float aspect = outW / (float)outH, cw = src->w, ch = src->h;
    int x0, y0, w, h, cx, cy;

    if (sBlurTex == NULL) {
        sBlurTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, BLUR_OUT_W,
                                     BLUR_OUT_H);
        if (sBlurTex == NULL)
            return;
        SDL_SetTextureScaleMode(sBlurTex, SDL_SCALEMODE_NEAREST);
        uploadedValid = 0;
#if defined(PORT_LINUX) && defined(__ARM_NEON)
        partialValid = false;
#endif
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
    if (uploadedValid && memcmp(cell, uploaded, sizeof(cell)) == 0) {
        PresentBlurTexture(big, outW, outH, cover);
        return;
    }
#if defined(PORT_LINUX) && defined(__ARM_NEON)
    if (hidden.w && partialValid && SDL_RectsEqual(&hidden, &partialCover) &&
        memcmp(cell, uploaded, sizeof(cell)) == 0) {
        if (PortFbDrawNearest2x(sRenderer, big, BLUR_OUT_W, BLUR_OUT_H, outH, &hidden))
            return;
        partialValid = false;
        DrawBlurredReflection(src, outW, outH, NULL);
        return;
    }
    const int yStep = (BLUR_OUT_H << 16) / 480;
    const int firstHiddenRow = hidden.y ? ((yStep / 2 + (hidden.y - 1) * yStep) >> 16) + 1 : 0;
    const int afterHiddenRow = hidden.y + hidden.h < 480
        ? (yStep / 2 + (hidden.y + hidden.h) * yStep) >> 16 : BLUR_OUT_H;
#endif
    /* The horizontal interpolation depends on only 14 source rows. Reuse
       their unrounded values for all destination rows. */
#if defined(PORT_LINUX) && defined(__ARM_NEON)
    static float horizontal[BLUR_H][BLUR_OUT_W][4];
#else
    static float horizontal[BLUR_H][BLUR_OUT_W][3];
#endif
    for (cx = 0; cx < BLUR_OUT_W; cx++) {
        float fx = (cx + 0.5f) * BLUR_W / BLUR_OUT_W - 0.5f;
        int ix = (int)SDL_floorf(fx);
        float tx = fx - ix;
        int jx = SDL_clamp(ix + 1, 0, BLUR_W - 1);
        ix = SDL_clamp(ix, 0, BLUR_W - 1);
        for (cy = 0; cy < BLUR_H; cy++) {
            const uint32_t a = cell[cy * BLUR_W + ix], b = cell[cy * BLUR_W + jx];
            for (int channel = 0; channel < 3; channel++) {
                const int shift = channel * 8;
                horizontal[cy][cx][channel] =
                    (float)((a >> shift) & 255) * (1 - tx) + (float)((b >> shift) & 255) * tx;
            }
        }
    }
    for (cy = 0; cy < BLUR_OUT_H; cy++) {
        float fy = (cy + 0.5f) * BLUR_H / BLUR_OUT_H - 0.5f;
        int iy = (int)SDL_floorf(fy);
        float ty = fy - iy;
        int jy = SDL_clamp(iy + 1, 0, BLUR_H - 1);
        iy = SDL_clamp(iy, 0, BLUR_H - 1);
#if defined(PORT_LINUX) && defined(__ARM_NEON)
        const bool covered = hidden.w && cy >= firstHiddenRow && cy < afterHiddenRow;
        for (cx = 0; cx < BLUR_OUT_W; cx += 4) {
            if (covered && cx == hidden.x / 2) {
                cx = (hidden.x + hidden.w) / 2;
                if (cx == BLUR_OUT_W)
                    break;
            }
            const float32x4x4_t above = vld4q_f32(horizontal[iy][cx]);
            const float32x4x4_t below = vld4q_f32(horizontal[jy][cx]);
            uint32x4_t channels[3];
            for (int channel = 0; channel < 3; channel++) {
                const float32x4_t value = vaddq_f32(vmulq_n_f32(above.val[channel], 1 - ty),
                                                   vmulq_n_f32(below.val[channel], ty));
                channels[channel] = vcvtq_u32_f32(vaddq_f32(value, vdupq_n_f32(0.5f)));
            }
            const uint32x4_t bg = vorrq_u32(channels[0], vshlq_n_u32(channels[1], 8));
            const uint32x4_t ra = vorrq_u32(vshlq_n_u32(channels[2], 16), vdupq_n_u32(0xFF000000u));
            vst1q_u32(big + cy * BLUR_OUT_W + cx, vorrq_u32(bg, ra));
        }
#else
        for (cx = 0; cx < BLUR_OUT_W; cx++) {
            uint32_t out = 0xFF000000u;
            for (int channel = 0; channel < 3; channel++) {
                float v = horizontal[iy][cx][channel] * (1 - ty) + horizontal[jy][cx][channel] * ty;
                out |= (uint32_t)(v + 0.5f) << (channel * 8);
            }
            big[cy * BLUR_OUT_W + cx] = out;
        }
#endif
    }
#if defined(PORT_LINUX) && defined(__ARM_NEON)
    partialValid = false;
    if (hidden.w) {
        uploadedValid = 0;
        if (PortFbDrawNearest2x(sRenderer, big, BLUR_OUT_W, BLUR_OUT_H, outH, &hidden)) {
            memcpy(uploaded, cell, sizeof(cell));
            partialCover = hidden;
            partialValid = true;
            return;
        }
        /* SDL fallback needs a complete texture. */
        DrawBlurredReflection(src, outW, outH, NULL);
        return;
    }
#endif
    if (SDL_UpdateTexture(sBlurTex, NULL, big, BLUR_OUT_W * 4)) {
        memcpy(uploaded, cell, sizeof(cell));
        uploadedValid = 1;
    }
    PresentBlurTexture(big, outW, outH, cover);
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

static void DrawEdgeBands(const SDL_FRect* src, const SDL_FRect* dst, int outW, int minBlock, int leftEnd, int rightEnd) {
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
        for (y = 0; y < SDL_clamp(side ? rightEnd : leftEnd,0,GBA_SCREEN_H); y++) {
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
                int sh, colorFar = 0;
                for (sh = 0; sh <= 16; sh += 8)
                    colorFar |= SDL_abs((int)((r[x2] >> sh) & 255) - (int)((lb >> sh) & 255)) > 24;
                if (colorFar) break;
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
static uint32_t* sSkipBuf;
static SDL_Texture* sSkipTexture;
static int sSkipBufW, sSkipBufH;
static SDL_Texture *sHd2dTex, *sMenuAlpha, *sSampleTex, *sNoticeTex;
static int sHd2dW, sHd2dH, sMenuAlphaW, sMenuAlphaH;

static void DrawSkipLayer(const SDL_FRect* normal) {
    int r[4];
    float sc = normal->w / GBA_SCREEN_W;
    SDL_FRect src, dst;

    if (sSkipBufW != gPortScreen.width || sSkipBufH != gPortScreen.height) {
        SDL_free(sSkipBuf);
        if (sSkipTexture) SDL_DestroyTexture(sSkipTexture);
        sSkipBufW = gPortScreen.width;
        sSkipBufH = gPortScreen.height;
        sSkipBuf = (uint32_t*)SDL_malloc((size_t)sSkipBufW * sSkipBufH * 4);
        sSkipTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, sSkipBufW, sSkipBufH);
        if (sSkipTexture) SDL_SetTextureScaleMode(sSkipTexture, SDL_SCALEMODE_PIXELART);
    }
    if (!sSkipBuf || !sSkipTexture)
        return;
    memcpy(sSkipBuf, sComposite, (size_t)sSkipBufW * sSkipBufH * 4);
    if (!PortUiDrawSkipLayer(sSkipBuf, sSkipBufW, sSkipBufH, r))
        return;
    SDL_UpdateTexture(sSkipTexture, NULL, sSkipBuf, sSkipBufW * 4);
    src.x = (float)r[0]; src.y = (float)r[1]; src.w = (float)r[2]; src.h = (float)r[3];
    dst.x = normal->x + (r[0] - gPortScreen.originX) * sc;
    dst.y = normal->y + (r[1] - gPortScreen.originY) * sc;
    dst.w = r[2] * sc;
    dst.h = r[3] * sc;
    SDL_RenderTexture(sRenderer, sSkipTexture, &src, &dst);
}

/* Replace only the journal background; native foreground remains on top. */
static bool sJournalUploadPending; /* the direct journal skipped the frame upload */

static int DrawJournalBook(const SDL_FRect* src, const SDL_FRect* dst) {
    extern const unsigned char gJiminyBgTiles[];
    static uint32_t foreground[240 * 160];
    const uint8_t* paper = PortPpuJiminyPaperMask();
    float scale = dst->w / 240;
    int id, dark = 0, white = 0;
    if (!paper) return 0;
    id = PortHdSpriteFind(gJiminyBgTiles, PortPpuJiminyBookFrame());
    if (!id || !PortJiminyForeground(foreground, sComposite, gPortFrame, paper,
            gPortScreen.width, gPortScreen.height, (int)src->x, (int)src->y)) return 0;
    PortPpuJiminyFade(&dark, &white);
    return PortHdJiminyBookDraw(sRenderer, id, foreground, dst->x, dst->y, scale, dark, white);
}

static int DrawJournalHeader(const SDL_FRect* src, const SDL_FRect* dst) {
    SDL_Rect saved, clip;
    int wasClipped = SDL_RenderClipEnabled(sRenderer), ok;
    if (wasClipped && !SDL_GetRenderClipRect(sRenderer, &saved)) return 0;
    clip.x = (int)SDL_floorf(dst->x); clip.y = (int)SDL_floorf(dst->y);
    clip.w = (int)SDL_ceilf(dst->x + dst->w) - clip.x;
    clip.h = (int)SDL_floorf(dst->y + 16 * dst->h / 160) - clip.y;
    if (wasClipped) {
        int right = SDL_min(clip.x + clip.w, saved.x + saved.w);
        int bottom = SDL_min(clip.y + clip.h, saved.y + saved.h);
        clip.x = SDL_max(clip.x, saved.x); clip.y = SDL_max(clip.y, saved.y);
        clip.w = right - clip.x; clip.h = bottom - clip.y;
    }
    if (clip.w <= 0 || clip.h <= 0) return 1;
    if (!SDL_SetRenderClipRect(sRenderer, &clip)) {
        SDL_SetRenderClipRect(sRenderer, wasClipped ? &saved : NULL); return 0;
    }
    /* Keep the original complete source/destination transform. Cropping the
       source to sixteen rows changed software pixel-art sampling at x4.96. */
    ok = SDL_RenderTexture(sRenderer, sTexture, src, dst);
    if (!SDL_SetRenderClipRect(sRenderer, wasClipped ? &saved : NULL)) ok = 0;
    return ok;
}

static void DrawJournalPicture(const SDL_FRect* src, const SDL_FRect* dst) {
    /* A ready book avoids painting the native body underneath it. Restore
       the complete original after any rejected or partially failed draw. */
    if (!DrawJournalBook(src, dst) || !DrawJournalHeader(src, dst)) {
#ifdef PORT_LINUX
        const char* directJournal;
        PORT_ENV_ONCE(directJournal, SDL_getenv, "KHCOM_FBDEV_JOURNAL_FIT");
        if (directJournal && PortFixed480p() && gPortConfig.filter == PORT_FILTER_NEAREST &&
            src->w == 240 && src->h == 160 && dst->x == 0 && (int)dst->y == 8 &&
            dst->w == 640 && (int)dst->h == 426 &&
            PortFbDrawJournalFit(sRenderer, sComposite + (int)src->y * gPortScreen.width + (int)src->x,
                                 gPortScreen.width)) {
            static bool logged;
            if (!logged) { PortLog("fbdev: journal fit active"); logged = true; }
            return;
        }
        /* the centred journal has the world inspect geometry */
        if (directJournal && Menu43Extend() && PortFixed480p() && gPortConfig.filter == PORT_FILTER_NEAREST &&
            src->w == 240 && src->h == 160 && dst->x == 0 && (int)dst->y == 26 &&
            dst->w == 640 && (int)dst->h == 426 &&
            PortFbDrawWorldInspectFit(sRenderer, sComposite + (int)src->y * gPortScreen.width + (int)src->x,
                                      gPortScreen.width))
            return;
#endif
        if (sJournalUploadPending) {
            SDL_UpdateTexture(sTexture, NULL, sComposite, gPortScreen.width * 4);
            sJournalUploadPending = false;
        }
        SDL_RenderTexture(sRenderer, sTexture, src, dst);
    }
}

static void DrawJournalNameMargins(const SDL_FRect* dst, int outW, uint32_t rightGround) {
    const uint32_t* edges = PortPpuJiminyNameEdges();
    float scale = dst->w / 240;
    int row, side;
    if (!edges) return;
    for (row = 16; row < 40; row++) {
        uint32_t colors[2];
        SDL_FRect areas[2];
        /* A PAGE bar at the right edge does not identify a name plate. */
        if (!edges[row - 16]) continue;
        colors[0] = edges[row - 16];
        colors[1] = edges[24 + row - 16] ? edges[24 + row - 16] : rightGround;
        areas[0] = (SDL_FRect){0, PixelEdge(dst->y, row, scale), SDL_max(0.0f, dst->x), 0};
        areas[1] = (SDL_FRect){dst->x + dst->w, areas[0].y, SDL_max(0.0f, outW - dst->x - dst->w), 0};
        areas[0].h = areas[1].h = PixelEdge(dst->y, row + 1, scale) - areas[0].y;
        for (side = 0; side < 2; side++) {
            uint32_t c = colors[side];
            if (!c || areas[side].w <= 0 || areas[side].h <= 0) continue;
            SDL_SetRenderDrawColor(sRenderer, (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c, 255);
            SDL_RenderFillRect(sRenderer, &areas[side]);
        }
    }
}

#include "port/jiminy_margin_draw.inc"

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

static int HdMenuAllocations(void) {
#if defined(PORT_GBA_RELOC) || defined(PORT_GUEST_RELOC) || defined(PORT_PS5_GUEST) || defined(PORT_LINUX64_GUEST) || defined(PORT_ANDROID64)
    return -1; /* guest renderers expose texture handles, not SDL's allocator */
#else
    return SDL_GetNumAllocations();
#endif
}

/* Device-loss recovery runs only between renderer passes. Pending detach keeps
   presentation suppressed until the previous target can be released safely. */
static uint64_t sRendererGeneration;
static int RendererResetService(void) {
#if __STDC_HOSTED__ && !defined(__EMSCRIPTEN__)
    if (!sRendererResetPending) return 1;
    SDL_Texture** textures[] = { &sTexture, &sMainOverTexture, &sOverlayTex, &sScreenOverTex, &sWorldLight.texture,
        &sHudTexture, &sHudOverTexture, &sBorderTex, &sBorderMaterialTex, &sBorderAdaptiveTex, &sLcdGridXTex, &sLcdGridYTex, &sBlurTex, &sSkipTexture,
        &sHd2dTex, &sMenuAlpha, &sSampleTex, &sNoticeTex,
        &sMenuSideTex[0], &sMenuSideTex[1], &sFadeSideTex[0], &sFadeSideTex[1] };
    unsigned i;
    extern void PortHdTextShutdown(void);
    if (!SDL_IsMainThread() || (sRenderer && !SDL_SetRenderTarget(sRenderer, NULL))) return 0;
    for (i = 0; i < sizeof(textures) / sizeof(textures[0]); i++) {
        if (*textures[i]) SDL_DestroyTexture(*textures[i]);
        *textures[i] = NULL;
    }
    sTexW = sTexH = sMainOverW = sMainOverH = sOverlayTexW = sScreenOverTexW = 0;
    sWorldLight.width = sWorldLight.height = sWorldLight.failed = 0;
    sWorldLight.blockedWidth = sWorldLight.blockedHeight = 0;
    sHudTexW = sHudTexH = sHudOverW = sHudOverH = sBorderShown = 0;
    sBorderAttempted = 0;
    sLcdGridReady = sLcdGridAttempted = 0;
    sSkipBufW = sSkipBufH = sHd2dW = sHd2dH = sMenuAlphaW = sMenuAlphaH = 0;
    for (i = 0; i < 2; i++) sMenuSideW[i] = sFadeSideW[i] = 0;
    PortHdHudInvalidate();
    PortHdSpritesShutdown();
    PortHdLayoutShutdown();
    PortHdTextShutdown();
    PortAchievementToastInvalidate();
    PortSceneGalleryPreviewInvalidate();
    sRendererResetPending = 0;
    ++sRendererGeneration;
    PortLog("renderer: invalidated GPU generation %llu after device reset", (unsigned long long)sRendererGeneration);
#endif
    return 1;
}

int PortRendererHandleReset(uint32_t eventType) {
#if __STDC_HOSTED__ && !defined(__EMSCRIPTEN__)
    if (!SDL_IsMainThread()) return 0;
    if (eventType == SDL_EVENT_RENDER_TARGETS_RESET) {
        /* Only render-target contents were lost. Gauge targets are cleared and
           redrawn on every pass; ordinary textures and readiness remain valid. */
        return 1;
    }
    if (eventType == SDL_EVENT_RENDER_DEVICE_RESET) {
        sRendererResetPending = 1;
        RendererResetService();
        return 1;
    }
#else
    (void)eventType;
#endif
    return 0;
}

#if defined(__ANDROID__) && !defined(PORT_ANDROID64)
/* The portable native movie consumes its own events on this same renderer thread. */
static void NativeMovieRendererReset(void) {
    PortRendererHandleReset(SDL_EVENT_RENDER_DEVICE_RESET);
}
#endif

/* HUD canvas transaction: reserve CPU memory before the PPU removes native
 * HUD sprites. Keep the old buffer on failure and cache that size until an
 * explicit resource invalidation or size change. GPU ownership stays in Present. */
static int sHudCanvasFailed, sHudCanvasFailedW, sHudCanvasFailedH;
static uint64_t sHudCanvasFailedGeneration;

void PortHudCanvasRetry(void) { sHudCanvasFailed = 0; }

int PortHudCanvasReady(int width, int height) {
    uint32_t* candidate;
    size_t pixels;
    if (width <= 0 || height <= 0 ||
        (size_t)width > SIZE_MAX / sizeof(uint32_t) / (size_t)height)
        return 0;
    pixels = (size_t)width * height;
    if (sHudBuf && sHudCapacity >= pixels) return 1;
    if (sHudCanvasFailed && sHudCanvasFailedW == width && sHudCanvasFailedH == height &&
        sHudCanvasFailedGeneration == sRendererGeneration)
        return 0;
    candidate = SDL_malloc(pixels * sizeof(*candidate));
    if (!candidate) {
        sHudCanvasFailed = 1;
        sHudCanvasFailedW = width;
        sHudCanvasFailedH = height;
        sHudCanvasFailedGeneration = sRendererGeneration;
        PortLog("HUD canvas: %dx%d CPU buffer unavailable; cartridge HUD retained until size/resource change", width, height);
        return 0;
    }
    SDL_free(sHudBuf);
    sHudBuf = candidate;
    sHudCapacity = pixels;
    sHudCanvasFailed = 0;
    return 1;
}

/* The closing Options frame is still black and game logic is paused. Walk
 * only the current HUD's OAM through the normal consumer so content-keyed
 * replacements decode/upload here, rather than on the first resumed frame.
 * This runs on the same renderer thread as Present; missing pieces retain
 * their normal cached miss and cartridge fallback. */
static void PrepareHudAfterOptions(int menuCanvas) {
    if (!menuCanvas || PortUiActive() || !gPortConfig.hdHud || !PortPpuHudActive())
        return;
    int savedHud[4];
    PortPpuHudFrameBegin(savedHud);
    /* Use the frozen PPU geometry, never the new desired menu dimensions.
     * Optional preparation does not reserve or grow its canvas. */
    if (sHudBuf && gPortHudW > 0 && gPortHudH > 0 &&
        (size_t)gPortHudW <= sHudCapacity / (size_t)gPortHudH) {
        uint64_t started = SDL_GetPerformanceCounter();
        PortPpuRenderHud(sHudBuf);
        if (SDL_getenv("KHCOM_HD_MENU_PROFILE"))
            PortLog("HD HUD: prepared current HUD before gameplay in %.1f ms",
                    (SDL_GetPerformanceCounter() - started) * 1000.0 / SDL_GetPerformanceFrequency());
    }
    PortPpuHudFrameEnd(savedHud);
}


/* Fully transparent straight-alpha overlays leave the fbdev target untouched. */
static bool ScreenOverlayEmpty(const uint32_t* pixels, int width) {
#ifdef PORT_LINUX
    if (PortFbActive() && PortFixedFullscreen()) {
        const size_t count = (size_t)width * GBA_SCREEN_H;
        size_t i = 0;
        while (i < count && !(pixels[i] & 0xff000000u))
            i++;
        return i == count;
    }
#else
    (void)pixels;
    (void)width;
#endif
    return false;
}

#ifdef PORT_LINUX
static SDL_Rect ScreenOverlayAlphaBounds(const uint32_t* pixels, int pitch, SDL_Rect scan) {
    int left = scan.x + scan.w, right = scan.x;
    int top = scan.y + scan.h, bottom = scan.y;
    for (int y = scan.y; y < scan.y + scan.h; y++) {
        const uint32_t* row = pixels + y * pitch;
        int x = scan.x;
        while (x < left && !(row[x] >> 24)) x++;
        if (x < left) left = x;
        x = scan.x + scan.w;
        while (x > right && !(row[x - 1] >> 24)) x--;
        if (x > right) right = x;
        for (x = left; x < right; x++) {
            if (row[x] >> 24) {
                top = SDL_min(top, y);
                bottom = y + 1;
                break;
            }
        }
    }
    if (left >= right || top >= bottom)
        return (SDL_Rect){0, 0, 0, 0};
    return (SDL_Rect){left, top, right - left, bottom - top};
}
#endif

/* Clip on source pixel boundaries before SDL allocates a scaled intermediate
 * for a layer that extends beyond the fixed-size panel. */
int gPortLevelUpActive; /* card_level_up.c: the level up screen is open */
static uint32_t sFlatMenuFrame; /* PortFlatMenu: the frame a flat menu (save) was last drawn */
void PortFlatMenu(void) { sFlatMenuFrame = PortFrameCount() + 1; }

/* GA36 4:3 (flat): the level up screen's top band (its banner and level) goes up to the panel's top and its bottom
   panel down to the bottom; the middle stays. The same for the save point's menu (their title bar and
   bottom band). The bands are split at fully transparent rows, where the picture under them shows through, so
   nothing is cut and nothing needs filling. */
/* The level up screen while a bonus is chosen: the list hangs under the banner with no clear row between them, so
   the whole-row split found nothing until the choice was made, and the header jumped then, and back and forth as the
   list moved (user, 26/11). Here the header is cut per 4-pixel column: each column's run from the top (one clear row
   bridged: the level box starts a row under the top band) goes up; the list, below a gap in every column, stays. The
   footer the same from the bottom: where the third bonus card touches the bottom band, the card goes down whole with
   it (a whole-row split at 152 had cut the card in two, user 26/11). */
#define FLAT_GROUPS (GBA_SCREEN_W / 4)
#define FLAT_HEAD_MAX 40
#define FLAT_FOOT_TOP 60 /* a column drawn from the bottom up past this row is a panel, not footer: it stays */
static uint32_t sFlatUp[GBA_SCREEN_W * FLAT_HEAD_MAX], sFlatMid[GBA_SCREEN_W * GBA_SCREEN_H],
    sFlatDown[GBA_SCREEN_W * GBA_SCREEN_H];

static int FlatGroupDrawn(const uint32_t* overlay, int width, int row, int g) {
    const uint32_t* p = overlay + (size_t)row * width + g * 4;
    return (p[0] | p[1] | p[2] | p[3]) >> 24 != 0;
}

uint64_t gPortPiece[24];
static uint64_t sPieceCk;
static int sPieceFlush = -1;
#define PIECE_CK(i) do { if (sPieceFlush < 0) sPieceFlush = SDL_getenv("KHCOM_PIECE_LOG") != NULL; if (sPieceFlush) SDL_FlushRenderer(sRenderer); const uint64_t _n = SDL_GetPerformanceCounter(); gPortPiece[i] += _n - sPieceCk; sPieceCk = _n; } while (0) /* KHCOM_PIECE_LOG: instructions (icount) spent per HD piece of the presentation */
static void PieceLog(void) {
    static int on = -1;
    static int every = -1;
    if (on < 0) on = SDL_getenv("KHCOM_PIECE_LOG") != NULL;
    if (every < 0) every = SDL_getenv("KHCOM_PIECE_EVERY") != NULL; /* each frame's own, not a hundred's */
    if (!on || (sFrameCount % 100 && !every)) return;
    PortLog("probe piece f=%u spr0 %.2f text0 %.2f spr2 %.2f text2 %.2f uihd %.2f footer %.2f art %.2f bar %.2f parts %.2f mainover %.2f base %.2f | pre %.2f hdlayers %.2f hud %.2f tail %.2f fbpresent %.2f | preover %.2f overlay %.2f layer2 %.2f screenover %.2f | sprites %.2f light %.2f over %.2f",
            (unsigned)sFrameCount, gPortPiece[0] / 1e8, gPortPiece[1] / 1e8, gPortPiece[2] / 1e8, gPortPiece[3] / 1e8,
            gPortPiece[4] / 1e8, gPortPiece[5] / 1e8, gPortPiece[6] / 1e8, gPortPiece[7] / 1e8, gPortPiece[8] / 1e8, gPortPiece[9] / 1e8, gPortPiece[10] / 1e8, gPortPiece[11] / 1e8, gPortPiece[12] / 1e8, gPortPiece[13] / 1e8, gPortPiece[14] / 1e8, gPortPiece[15] / 1e8, gPortPiece[16] / 1e8, gPortPiece[17] / 1e8, gPortPiece[18] / 1e8, gPortPiece[19] / 1e8, gPortPiece[20] / 1e8, gPortPiece[21] / 1e8, gPortPiece[22] / 1e8);
    SDL_memset(gPortPiece, 0, sizeof(gPortPiece));
}

static bool DrawLevelUpColumns(const uint32_t* overlay, const SDL_FRect* od, int width, int outW, int outH) {
#ifdef PORT_LINUX
    extern bool PortFbBlendOverlayScaled(SDL_Renderer*, const uint32_t*, int, int, const SDL_Rect*);
    int head[FLAT_GROUPS], foot[FLAT_GROUPS], g, r, top = 0, f = GBA_SCREEN_H;
    for (g = 0; g < FLAT_GROUPS; g++) {
        int last = 0;
        if (!FlatGroupDrawn(overlay, width, 0, g) && !FlatGroupDrawn(overlay, width, 1, g)) {
            head[g] = 0;
            continue;
        }
        for (r = 0; r < GBA_SCREEN_H; r++) {
            if (FlatGroupDrawn(overlay, width, r, g)) last = r + 1;
            else if (r + 1 < GBA_SCREEN_H && FlatGroupDrawn(overlay, width, r + 1, g) && r < 12) continue;
            else break;
        }
        head[g] = last <= FLAT_HEAD_MAX ? last : 0; /* a column drawn further down is not header: it stays */
        top = SDL_max(top, head[g]);
    }
    for (g = 0; g < FLAT_GROUPS; g++) {
        int first = GBA_SCREEN_H;
        if (FlatGroupDrawn(overlay, width, GBA_SCREEN_H - 1, g) || FlatGroupDrawn(overlay, width, GBA_SCREEN_H - 2, g))
            for (r = GBA_SCREEN_H - 1; r >= 0 && FlatGroupDrawn(overlay, width, r, g); r--)
                first = r;
        foot[g] = first >= FLAT_FOOT_TOP && first > head[g] ? first : GBA_SCREEN_H;
        f = SDL_min(f, foot[g]);
    }
    if (top < 8)
        return false;
    memcpy(sFlatMid, overlay, sizeof(uint32_t) * GBA_SCREEN_W * GBA_SCREEN_H);
    memset(sFlatUp, 0, sizeof(uint32_t) * GBA_SCREEN_W * top);
    for (g = 0; g < FLAT_GROUPS; g++)
        for (r = 0; r < head[g]; r++) {
            memcpy(sFlatUp + (size_t)r * GBA_SCREEN_W + g * 4, sFlatMid + (size_t)r * GBA_SCREEN_W + g * 4, 16);
            memset(sFlatMid + (size_t)r * GBA_SCREEN_W + g * 4, 0, 16);
        }
    if (f < GBA_SCREEN_H) {
        memset(sFlatDown + (size_t)f * GBA_SCREEN_W, 0, sizeof(uint32_t) * GBA_SCREEN_W * (GBA_SCREEN_H - f));
        for (g = 0; g < FLAT_GROUPS; g++)
            for (r = foot[g]; r < GBA_SCREEN_H; r++) {
                memcpy(sFlatDown + (size_t)r * GBA_SCREEN_W + g * 4, sFlatMid + (size_t)r * GBA_SCREEN_W + g * 4, 16);
                memset(sFlatMid + (size_t)r * GBA_SCREEN_W + g * 4, 0, 16);
            }
    }
    {
        const int y0 = (int)od->y, below = outH - (y0 + (int)od->h);
#define Y(r) ((int)((r) * (int)od->h / GBA_SCREEN_H))
        const SDL_Rect up = {0, 0, outW, Y(top)}, mid = {0, y0, outW, Y(GBA_SCREEN_H)},
                       down = {0, y0 + Y(f) + below, outW, Y(GBA_SCREEN_H) - Y(f)};
#undef Y
        if (!PortFbBlendOverlayScaled(sRenderer, sFlatMid, width, GBA_SCREEN_H, &mid) ||
            !PortFbBlendOverlayScaled(sRenderer, sFlatUp, width, top, &up) ||
            (f < GBA_SCREEN_H &&
             !PortFbBlendOverlayScaled(sRenderer, sFlatDown + (size_t)f * GBA_SCREEN_W, width, GBA_SCREEN_H - f, &down)))
            return false;
    }
    {
        static bool logged;
        if (!logged) { PortLog("ga36: level up laid out flat by columns (header %d, footer %d)", top, f); logged = true; }
    }
    return true;
#else
    (void)overlay; (void)od; (void)width; (void)outW; (void)outH;
    return false;
#endif
}

static bool DrawLevelUpFlat(const uint32_t* overlay, const SDL_FRect* od, int width, int outW, int outH) {
#ifdef PORT_LINUX
    extern bool PortFbBlendOverlayScaled(SDL_Renderer*, const uint32_t*, int, int, const SDL_Rect*);
    const char* on;
    int a = -1, f = -1, seen = 0, r;
    PORT_ENV_ONCE(on, SDL_getenv, "KHCOM_GA36_LEVELUP_FLAT");
    const bool flatMenu = sFlatMenuFrame && PortFrameCount() <= sFlatMenuFrame;
    if (!on || !(gPortLevelUpActive || flatMenu) || !PortFbActive() || !PortFixed480p() || gPortConfig.aspect != PORT_ASPECT_4_3 ||
        outW != 640 || outH != 480 || width != GBA_SCREEN_W || (int)od->x != 0 || (int)od->w != outW || od->y < 8 ||
        sOverlayScale < 2.66f || sOverlayScale > 2.67f) {
        return false;
    }
    if (gPortLevelUpActive && !flatMenu && !SDL_getenv("KHCOM_GA36_LEVELUP_ROWS_ONLY"))
        return DrawLevelUpColumns(overlay, od, width, outW, outH);
#define ROW_CLEAR(r) ({ const uint32_t* row_ = overlay + (size_t)(r) * width; int c_ = 0;                         while (c_ < width && !(row_[c_] >> 24)) c_++; c_ == width; })
    /* a band is a run of at least 8 drawn rows touching the top (or the bottom) of the screen */
    for (r = 0; r < GBA_SCREEN_H / 2 && a < 0; r++) {
        if (!ROW_CLEAR(r)) seen++;
        else if (seen) a = r;
        else if (r > 4) break;
    }
    for (r = GBA_SCREEN_H - 1, seen = 0; r >= GBA_SCREEN_H / 2 && f < 0; r--) {
        if (!ROW_CLEAR(r)) seen++;
        else if (seen) f = r + 1;
        else if (r < GBA_SCREEN_H - 5) break;
    }
#undef ROW_CLEAR
    if (a < 8 || f <= a || f > GBA_SCREEN_H - 8)
        return false;
    {
        const int top = (int)od->y, below = outH - (top + (int)od->h);
#define Y(r) ((int)((r) * (int)od->h / GBA_SCREEN_H))
        const SDL_Rect up = {0, 0, outW, Y(a)}, mid = {0, top + Y(a), outW, Y(f) - Y(a)},
                       down = {0, top + Y(f) + below, outW, Y(GBA_SCREEN_H) - Y(f)};
#undef Y
        if (!PortFbBlendOverlayScaled(sRenderer, overlay, width, a, &up) ||
            !PortFbBlendOverlayScaled(sRenderer, overlay + (size_t)a * width, width, f - a, &mid) ||
            !PortFbBlendOverlayScaled(sRenderer, overlay + (size_t)f * width, width, GBA_SCREEN_H - f, &down))
            return false;
        static bool logged;
        if (!logged) { PortLog("ga36: %s laid out flat (rows %d/%d)", flatMenu ? "menu" : "level up", a, f); logged = true; }
        return true;
    }
#else
    (void)overlay; (void)od; (void)width; (void)outW; (void)outH;
    return false;
#endif
}

static bool DrawScreenOverlay(SDL_Texture* texture, const uint32_t* pixels, const SDL_FRect* dest, int width, int outW, int outH) {
#ifdef PORT_LINUX
    const char* clipOverlay;
    const char* trimOverlay;
    const char* nativeOverlay;
    PORT_ENV_ONCE(clipOverlay, SDL_getenv, "KHCOM_FBDEV_CLIP_OVERLAY");
    PORT_ENV_ONCE(trimOverlay, SDL_getenv, "KHCOM_FBDEV_TIGHT_OVERLAY");
    PORT_ENV_ONCE(nativeOverlay, SDL_getenv, "KHCOM_FBDEV_NATIVE_OVERLAY");
    const char* fractionalOverlay;
    PORT_ENV_ONCE(fractionalOverlay, SDL_getenv, "KHCOM_FBDEV_FRACTIONAL_OVERLAY");
    if (fractionalOverlay && PortFbActive() && PortFixed480p() && outW == 640 && outH == 480 &&
        width == GBA_SCREEN_W && sOverlayScale > 2.66f && sOverlayScale < 2.67f &&
        dest->x >= 0 && dest->y >= 0) {
        const char* sparseOverlay;
        extern int PortGa36SynthesisActive(void);
        PORT_ENV_ONCE(sparseOverlay, SDL_getenv, "KHCOM_FBDEV_SPARSE_FRACTIONAL");
        SDL_Rect target = {(int)dest->x, (int)dest->y, (int)dest->w, (int)dest->h};
        /* Same arithmetic over any screen layer, not only synthesis. */
        const char* sparseAll;
        PORT_ENV_ONCE(sparseAll, SDL_getenv, "KHCOM_FBDEV_SPARSE_FRACTIONAL_ALL");
        if ((sparseAll || (sparseOverlay && PortGa36SynthesisActive())) &&
            PortFbBlendOverlayScaledSparse(sRenderer, pixels, width, GBA_SCREEN_H, &target)) return true;
        if (PortFbBlendOverlayScaled(sRenderer, pixels, width, GBA_SCREEN_H, &target)) return true;
    }
    const int scale = (int)sOverlayScale;
    if (clipOverlay && PortFbActive() && PortFixedFullscreen() && outW == 640 && outH == 480 &&
        scale > 1 && sOverlayScale == scale) {
        const int left = (int)dest->x, top = (int)dest->y;
        const int right = left + width * scale, bottom = top + GBA_SCREEN_H * scale;
        const char* native2x;
        PORT_ENV_ONCE(native2x, SDL_getenv, "KHCOM_FBDEV_NATIVE_OVERLAY_2X");
        if (native2x && scale == 2 && left >= 0 && top >= 0 && right <= outW && bottom <= outH) {
            SDL_Rect bounds = {0, 0, width, GBA_SCREEN_H};
            if (trimOverlay)
                bounds = ScreenOverlayAlphaBounds(pixels, width, bounds);
            if (!bounds.w || !bounds.h)
                return true;
            const SDL_Rect panel = {left + bounds.x * 2, top + bounds.y * 2, bounds.w * 2, bounds.h * 2};
            if (PortFbBlendOverlay2x(sRenderer, pixels + bounds.y * width + bounds.x, width, &panel))
                return true;
        }
        if (left < 0 || top < 0 || right > outW || bottom > outH) {
            const int x0 = SDL_max(0, -left), y0 = SDL_max(0, -top);
            const int x1 = SDL_min(outW, right) - left, y1 = SDL_min(outH, bottom) - top;
            if (x0 >= x1 || y0 >= y1)
                return true;
            if (!(x0 % scale || y0 % scale || x1 % scale || y1 % scale)) {
                SDL_Rect bounds = {x0 / scale, y0 / scale, (x1 - x0) / scale, (y1 - y0) / scale};
                if (trimOverlay) {
                    bounds = ScreenOverlayAlphaBounds(pixels, width, bounds);
                    if (!bounds.w || !bounds.h)
                        return true;
                }
                if (nativeOverlay && scale == 4) {
                    const SDL_Rect panel = {left + bounds.x * 4, top + bounds.y * 4, bounds.w * 4, bounds.h * 4};
                    if (PortFbBlendOverlay4x(sRenderer, pixels + bounds.y * width + bounds.x, width, &panel))
                        return true;
                }
                const SDL_FRect source = {bounds.x, bounds.y, bounds.w, bounds.h};
                const SDL_FRect visible = {left + bounds.x * scale, top + bounds.y * scale,
                                            bounds.w * scale, bounds.h * scale};
                return SDL_RenderTexture(sRenderer, texture, &source, &visible);
            }
        }
    }
#else
    (void)pixels;
    (void)width;
    (void)outW;
    (void)outH;
#endif
    return SDL_RenderTexture(sRenderer, texture, NULL, dest);
}

#ifdef PORT_LINUX
/* Rows of the 240x160 screen layer with no transparency at all: the layer
 * replaces whatever lies under them. Returns how many there are. */
static int OpaqueOverlayRows(const uint32_t* overlay, uint8_t hidden[GBA_SCREEN_H]) {
    int count = 0;
    for (int y = 0; y < GBA_SCREEN_H; y++, overlay += GBA_SCREEN_W) {
        int x = 0;
        while (x < GBA_SCREEN_W && (overlay[x] >> 24) == 255)
            x++;
        hidden[y] = x == GBA_SCREEN_W;
        count += hidden[y];
    }
    return count;
}
#endif

#ifdef PORT_LINUX
/* The 240x160 picture at the 640x480 panel's full width. */
static bool PortGa36FitGeometry(const SDL_FRect* dst) {
    return dst->x == 0 && dst->y == 26 && dst->w == 640 && dst->h == 160.0f * (640.0f / 240.0f);
}
#endif

static void Present(int paced) {
#ifdef PORT_LINUX
    PortFbWaitCopy();
#endif
    LoopMark(PORT_LOOP_COPYWAIT);
    sPieceCk = SDL_GetPerformanceCounter();
    if (!RendererResetService()) return;
    const char* hdProfile;
    PORT_ENV_ONCE(hdProfile, SDL_getenv, "KHCOM_HD_MENU_PROFILE");
    uint64_t hdProfileStart = hdProfile ? SDL_GetPerformanceCounter() : 0;
#ifdef PORT_LINUX
    const char* fbDetail;
    PORT_ENV_ONCE(fbDetail, SDL_getenv, "KHCOM_FBDEV_DETAIL");
    const bool measureFb = fbDetail && PortFbActive() && sFrameCount % 30 == 0;
    uint64_t uploadTicks = 0, scaleTicks = 0;
    bool usedScale2x = false;
    const uint64_t detailStart = measureFb ? SDL_GetPerformanceCounter() : 0;
#endif
    uint64_t hdProfileWaitStart = sPresentWaitTotal;
    int hdProfileAlloc = hdProfile ? HdMenuAllocations() : 0;
    int outW, outH;
    SDL_FRect dst, normal;
    SDL_Rect borderOuter = { 0, 0, 0, 0 }, contentArea = { 0, 0, 0, 0 };
    int borderOn = 0;
    uint32_t uniform = 0;
    float worldX, worldY, worldScale;
    float lcdSourceW = (float)gPortScreen.width, lcdSourceH = (float)gPortScreen.height;
    int moviePrompt;
    /* Snapshot the phase before PortUiDraw advances the fade: canvas, game
     * layers and presentation must all use the same phase for this frame. */
    int menuCanvas = PortUiMenuCanvas();
    const uint32_t* mainWorldLight = menuCanvas ? NULL : PortPpuMainAdditive();
    if (!mainWorldLight && !PortPpuScreenAdditive()) PortScreenWorldBlendIdle(&sWorldLight);
    int nativeMainWorld = mainWorldLight && !PortScreenWorldBlendPrepareSize(&sWorldLight, sRenderer,
        mainWorldLight, gPortScreen.width, gPortScreen.height);
    PortHdSpritesWorldFallback(nativeMainWorld);
    int sceneFade = PortUiSceneFade();
    PortJiminyLayout journalLayout = {0};
    int journalShown = 0;
    int journalActive = PortJiminyMenuActive(menuCanvas, gCurrentMode == &gModeJiminy,
        gPortConfig.viewMode == PORT_VIEW_ORIGINAL, gPortConfig.viewMode == PORT_VIEW_CRT, sSideStyle, sFitScale);
    void PortHdHudPrepare(SDL_Renderer* renderer);
    void PortHdHudDraw(SDL_Renderer* renderer, int width, int height);
    void PortHdHudDrawLabel(SDL_Renderer* renderer, int width, int height);
    void PortHdHudDrawBar(SDL_Renderer* renderer, int width, int height);
    void PortHdHudDrawSprites(SDL_Renderer* renderer, int width, int height);
    void PortHdTextHudDigits(SDL_Renderer* renderer, float originX, float originY, float scale);

    EnsureTexture();
    GrowPixelBuffer(&sCompositeBuf, &sCompositeCapacity, (size_t)gPortScreen.width * gPortScreen.height);
    PortHdHudPrepare(sRenderer);
    PortHdLayoutPrepare(sRenderer);
    {
        void PortHdSpritesPreloadStep(SDL_Renderer* renderer);
        PortHdSpritesPreloadStep(sRenderer); /* the HD HUD pack's pictures, before the first battle */
    }
    /* movies shown cropped: the skip prompt is drawn after the movie, on its
       own, so it neither grows with the video nor shifts the crop */
    moviePrompt = sSideStyle == 10 && sFitScale <= 0 && !menuCanvas && !gPortScreen.wide
                  && (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H);
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
    if (nativeMainWorld) sComposite = (uint32_t*)PortPpuMainNativeWorld();
    PortUiDraw(sComposite, gPortScreen.width, gPortScreen.height);
    PrepareHudAfterOptions(menuCanvas);
    {
        extern void PortPpuOverKeepDrawn(const uint32_t* composite);
        PortPpuOverKeepDrawn(sComposite); /* the title's hint over its HD illustration */
    }
    SDL_GetCurrentRenderOutputSize(sRenderer, &outW, &outH);
    SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, 255);
    {
        /* Aspect ratio option: draw into the chosen rectangle; the rest of
           the window stays black */
        SDL_Rect area = ContentArea(outW, outH);
        borderOuter = AspectRect(outW, outH);
        contentArea = area;
        borderOn = GbaBorderReady();
        SDL_SetRenderViewport(sRenderer, area.x || area.y || area.w != outW || area.h != outH ? &area : NULL);
        outW = area.w;
        outH = area.h;
    }
    bool menuFit = false;
#ifdef PORT_LINUX
    {
        /* A GBA-size menu picture at the handheld panel's full width: every
           condition is known before the frame goes to the renderer. */
        const char* menuFitFlag;
        PORT_ENV_ONCE(menuFitFlag, SDL_getenv, "KHCOM_GA36_MENU_FIT");
        menuFit = menuFitFlag && PortFbActive() && PortFixed480p() && !menuCanvas && !borderOn && !journalActive &&
            (sFitScale > 0 || !gPortScreen.wide) && gPortConfig.viewMode != PORT_VIEW_CRT &&
            gPortConfig.filter == PORT_FILTER_NEAREST && sSideStyle != 10 && outW == 640 && outH == 480 &&
            (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H) &&
            gPortScreen.originX >= 0 && gPortScreen.originY >= 0 &&
            gPortScreen.originX + GBA_SCREEN_W <= gPortScreen.width &&
            gPortScreen.originY + GBA_SCREEN_H <= gPortScreen.height && !CoverActive() &&
            PortSceneFramingNarrowScale(gPortSceneCameraAdjusted, outW, outH, MenuScale(outW, outH)) ==
                640.0f / 240.0f && !AllmapScale();
    }
#endif
    bool deferBase = false;
#if defined(PORT_LINUX) && defined(__ARM_NEON)
    const char* deferFlag;
    PORT_ENV_ONCE(deferFlag, SDL_getenv, "KHCOM_FBDEV_DEFER_BASE_UPLOAD");
    if (deferFlag && PortFbActive() && PortFixedFullscreen() && !menuCanvas && gPortScreen.wide &&
        sFitScale <= 0 && !PortHd2dActive() && gPortConfig.filter == PORT_FILTER_NEAREST &&
        outW == 2 * gPortScreen.width && outH == 2 * gPortScreen.height) {
        extern int PortUiScenePauseRect(int rect[4]);
        int pauseRect[4];
        deferBase = !PortUiScenePauseRect(pauseRect);
    }
#endif
    /* The centred 4:3 journal drawn straight to the panel: nothing reads the
       frame texture, and the picture replaces rows 26..451. */
    bool journalLight = false;
#ifdef PORT_LINUX
    {
        const char *light, *directJournal, *skipSides;
        PORT_ENV_ONCE(light, SDL_getenv, "KHCOM_GA36_JOURNAL_LIGHT_BASE");
        PORT_ENV_ONCE(directJournal, SDL_getenv, "KHCOM_FBDEV_JOURNAL_FIT");
        PORT_ENV_ONCE(skipSides, SDL_getenv, "KHCOM_GA36_SKIP_EMPTY_JOURNAL_SIDES");
        if (light && directJournal && skipSides && journalActive && Menu43Extend() && !menuCanvas && !borderOn &&
            !nativeMainWorld && (sSideStyle == 9 || sSideStyle == 12) && sFitScale <= 0 && !gPortScreen.wide &&
            gPortConfig.viewMode != PORT_VIEW_CRT && gPortConfig.filter == PORT_FILTER_NEAREST &&
            outW == 640 && outH == 480 && (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H) &&
            gPortScreen.originX >= 0 && gPortScreen.originY >= 0 &&
            gPortScreen.originX + GBA_SCREEN_W <= gPortScreen.width &&
            gPortScreen.originY + GBA_SCREEN_H <= gPortScreen.height && !CoverActive() &&
            PortSceneFramingNarrowScale(gPortSceneCameraAdjusted, outW, outH, MenuScale(outW, outH)) ==
                640.0f / 240.0f && !PortPpuJiminyPaperMask()) {
            extern int PortUiScenePauseRect(int rect[4]);
            PortJiminyLayout layout;
            int pauseRect[4];
            journalLight = !PortUiScenePauseRect(pauseRect) && PortJiminyFit(outW, outH, &layout);
        }
    }
#endif
    sJournalUploadPending = journalLight;
    bool allmapDirect = false;
#ifdef PORT_LINUX
    {
        const char* direct;
        PORT_ENV_ONCE(direct, SDL_getenv, "KHCOM_FBDEV_ALLMAP_DIRECT");
        allmapDirect = direct && AllmapScale() == 2.0f && !menuCanvas && !borderOn && !journalActive &&
            sSideStyle != 10 && (sFitScale > 0 || !gPortScreen.wide) && gPortConfig.viewMode != PORT_VIEW_CRT &&
            gPortConfig.filter == PORT_FILTER_NEAREST && outW == 640 && outH == 480 &&
            (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H) &&
            gPortScreen.originX >= 0 && gPortScreen.originY >= 0 &&
            gPortScreen.originX + GBA_SCREEN_W <= gPortScreen.width &&
            gPortScreen.originY + GBA_SCREEN_H <= gPortScreen.height;
    }
#endif
    bool deferMenuUpload = false;
#ifdef PORT_LINUX
    if (menuFit && !nativeMainWorld) {
        extern int PortUiScenePauseRect(int rect[4]);
        const char* lazyUpload;
        int pauseRect[4];
        PORT_ENV_ONCE(lazyUpload, SDL_getenv, "KHCOM_GA36_MENU_FIT_LAZY_UPLOAD");
        /* The scene pause's panel is the one later reader of the texture. */
        deferMenuUpload = lazyUpload && !PortUiScenePauseRect(pauseRect);
    }
#endif
    if (deferMenuUpload || journalLight)
        FillEnhancedVerticalColorMargins();
    if (!deferBase && !deferMenuUpload && !journalLight) {
#ifdef PORT_LINUX
        const uint64_t uploadStart = measureFb ? SDL_GetPerformanceCounter() : 0;
#endif
        FillEnhancedVerticalColorMargins();
        SDL_UpdateTexture(sTexture, NULL, sComposite, gPortScreen.width * 4);
#ifdef PORT_LINUX
        if (measureFb)
            uploadTicks = SDL_GetPerformanceCounter() - uploadStart;
#endif
    }
    if ((sFitScale > 0 || (!gPortScreen.wide)) && !menuCanvas && gPortConfig.viewMode != PORT_VIEW_CRT
        && (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H || Ga36CoverIntro())) {
        const char* bandClear = NULL;
#ifdef PORT_LINUX
        if (menuFit)
            PORT_ENV_ONCE(bandClear, SDL_getenv, "KHCOM_GA36_MENU_FIT_BAND_CLEAR");
#endif
        if ((menuFit && bandClear) || journalLight) {
            /* The opaque picture replaces rows 26..451: only the bands need black. */
            const SDL_FRect bands[2] = {{0, 0, 640, 26}, {0, 452, 640, 28}};
            SDL_RenderFillRects(sRenderer, bands, 2);
        } else if (allmapDirect) {
            /* The opaque picture replaces its own rectangle. */
            const SDL_FRect around[4] = {{0, 0, 640, 80}, {0, 400, 640, 80}, {0, 80, 80, 320}, {560, 80, 80, 320}};
            SDL_RenderFillRects(sRenderer, around, 4);
        } else {
            SDL_RenderClear(sRenderer);
        }
        /* Scene without a wide view: show the GBA picture at full height
           (bars only at the sides), sampled with the pixel-art filter. */
        SDL_FRect src = { (float)gPortScreen.originX, (float)gPortScreen.originY, GBA_SCREEN_W, GBA_SCREEN_H };
        float scale = CoverActive() ? SDL_max(outW / (float)GBA_SCREEN_W, outH / (float)GBA_SCREEN_H)
                      : sSideStyle == 10 ? SDL_min(outW / (float)GBA_SCREEN_W, outH / (float)GBA_SCREEN_H)
                                         : MenuScale(outW, outH);
        scale = PortSceneFramingNarrowScale(gPortSceneCameraAdjusted, outW, outH, scale);
        if (AllmapScale() > 0 && !menuCanvas) scale = AllmapScale();
        if (borderOn) scale = SDL_min(scale, SDL_min(outW / (float)GBA_SCREEN_W, outH / (float)GBA_SCREEN_H));
        dst.w = GBA_SCREEN_W * scale;
        dst.h = GBA_SCREEN_H * scale;
        dst.x = (float)(int)((outW - dst.w) / 2);
        dst.y = (float)(int)((outH - dst.h) / 2);
        if (journalActive && PortJiminyFit(outW, outH, &journalLayout)) {
            if (Menu43Extend()) {
                /* centred like the other menus, without the added help row */
                journalLayout = (PortJiminyLayout){ scale, dst.x, dst.y, dst.w, dst.h, dst.y + dst.h, 0 };
            }
            scale = journalLayout.scale;
            dst = (SDL_FRect){ journalLayout.x, journalLayout.y, journalLayout.width, journalLayout.height };
            journalShown = 1;
        }
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
        DrawEnhancedVerticalMargins(&src, &dst, outW, outH);
        if ((sSideStyle == 3 || sSideStyle == 8 || sSideStyle == 10 || sFitScale > 0) && !menuCanvas) {
            const SDL_FRect* blurCover = sFitScale > 0 && !journalShown ? &dst : NULL;
#ifdef PORT_LINUX
            extern Mode gModeWorldinspect;
            const char* worldBlurCover;
            PORT_ENV_ONCE(worldBlurCover, SDL_getenv, "KHCOM_GA36_WORLDINSPECT_BLUR_COVER");
            if (worldBlurCover && gCurrentMode == &gModeWorldinspect && PortFbActive() && PortFixed480p() &&
                gPortConfig.filter == PORT_FILTER_NEAREST && outW == 640 && outH == 480 &&
                dst.x == 0 && dst.y == 26 && dst.w == 640 && dst.h == 160.0f * (640.0f / 240.0f)) {
                static bool logged;
                if (!logged) { PortLog("fbdev: world inspect blur cover active"); logged = true; }
                blurCover = &dst;
            }
            /* The same opaque picture follows the reflection in every mode
               with this geometry (world cards, world warp, ...). */
            const char* blurCoverAll;
            PORT_ENV_ONCE(blurCoverAll, SDL_getenv, "KHCOM_GA36_BLUR_COVER_ALL");
            if (blurCoverAll && !journalShown && PortFbActive() && PortFixed480p() &&
                gPortConfig.filter == PORT_FILTER_NEAREST && outW == 640 && outH == 480 &&
                PortGa36FitGeometry(&dst)) {
                static bool logged;
                if (!logged) { PortLog("fbdev: blur cover active"); logged = true; }
                blurCover = &dst;
            }
#endif
            DrawBlurredReflection(&src, outW, outH, blurCover);
        }
        SDL_SetTextureScaleMode(sTexture, gPortConfig.filter == PORT_FILTER_LINEAR ? (SDL_ScaleMode)PortSmoothScaleMode()
                                                                                 : SDL_SCALEMODE_PIXELART);
        bool drewMenuFit = false;
#ifdef PORT_LINUX
        static uint8_t hiddenRows[GBA_SCREEN_H];
        const uint8_t* covered = NULL;
        if (menuFit && PortPpuOverlayActive() && gPortOverlayW == GBA_SCREEN_W &&
            sOverlayScale > 2.66f && sOverlayScale < 2.67f && !PortPpuScreenAdditive()) {
            /* The screen layer goes over the same rectangle with the same
               row sampling, through the direct fractional blend. */
            const char *skipCovered, *directOverlay;
            PORT_ENV_ONCE(skipCovered, SDL_getenv, "KHCOM_GA36_MENU_FIT_SKIP_COVERED");
            PORT_ENV_ONCE(directOverlay, SDL_getenv, "KHCOM_FBDEV_FRACTIONAL_OVERLAY");
            if (skipCovered && directOverlay && OpaqueOverlayRows(gPortOverlay, hiddenRows)) {
                static bool logged;
                if (!logged) { PortLog("fbdev: menu fit covered rows skipped"); logged = true; }
                covered = hiddenRows;
            }
        }
        drewMenuFit = menuFit && !journalShown && src.w == GBA_SCREEN_W && src.h == GBA_SCREEN_H &&
            dst.x == 0 && dst.y == 26 && dst.w == 640 && dst.h == 160.0f * (640.0f / 240.0f) &&
            PortFbDrawMenuFit(sRenderer, sComposite + (int)src.y * gPortScreen.width + (int)src.x,
                              gPortScreen.width, covered);
#endif
        if (deferMenuUpload && !drewMenuFit)
            SDL_UpdateTexture(sTexture, NULL, sComposite, gPortScreen.width * 4);
        if (drewMenuFit) {
        } else if (journalShown) DrawJournalPicture(&src, &dst);
        else {
#ifdef PORT_LINUX
            extern Mode gModeWorldinspect;
            const char* directWorldInspect;
            PORT_ENV_ONCE(directWorldInspect, SDL_getenv, "KHCOM_FBDEV_WORLDINSPECT_FIT");
            if (!(directWorldInspect && gCurrentMode == &gModeWorldinspect && PortFixed480p() &&
                  gPortConfig.filter == PORT_FILTER_NEAREST && src.x == 40 && src.y == 40 &&
                  src.w == 240 && src.h == 160 && dst.x == 0 && dst.y == 26 && dst.w == 640 &&
                  dst.h == 160.0f * (640.0f / 240.0f) &&
                  gPortScreen.width == 320 && gPortScreen.height >= 200 &&
                  PortFbDrawWorldInspectFit(sRenderer, sComposite + 40 * 320 + 40, 320)) &&
                !(allmapDirect && src.w == GBA_SCREEN_W && src.h == GBA_SCREEN_H &&
                  dst.x == 80 && dst.y == 80 && dst.w == 480 && dst.h == 320 &&
                  PortFbDrawAllmap2x(sRenderer, sComposite + (int)src.y * gPortScreen.width + (int)src.x,
                                     gPortScreen.width)) &&
                !(PortFixed480p() && gPortConfig.filter == PORT_FILTER_NEAREST &&
                  src.w == GBA_SCREEN_W && src.h == GBA_SCREEN_H &&
                  dst.x == -40 && dst.y == 0 && dst.w == 720 && dst.h == 480 &&
                  PortFbDrawCropped3x(sRenderer, sComposite + (int)src.y * gPortScreen.width + (int)src.x,
                                     gPortScreen.width)))
#endif
                SDL_RenderTexture(sRenderer, sTexture, &src, &dst);
        }
        if (journalShown && Menu43Extend()) DrawJournalFooterBar(&src, &dst, outW, outH);
        {
            /* The side repairs below repaint these margins whenever the picture
               is not fitted, so stretching the edge columns first is unseen. */
            const char* sidesOnce;
            PORT_ENV_ONCE(sidesOnce, SDL_getenv, "KHCOM_GA36_ALLMAP_SIDES_ONCE");
            if (AllmapScale() > 0 && !menuCanvas && !(sidesOnce && sFitScale <= 0))
                DrawAllmapSides(&src, &dst, outW);
            DrawAllmapAnchored(&src, &dst, outW, outH, deferMenuUpload, drewMenuFit);
        }
        if (moviePrompt)
            DrawSkipLayer(&normal);
        /* Every side repair below paints outside dst: any mode whose picture
           fills the panel's width has nothing for them to show. */
        bool emptySides = false;
#ifdef PORT_LINUX
        const char* skipEmptySides;
        PORT_ENV_ONCE(skipEmptySides, SDL_getenv, "KHCOM_GA36_SKIP_EMPTY_SIDES");
        emptySides = skipEmptySides && PortFixed480p() && PortFbActive() &&
            !PortJiminyHasSideMargins(dst.x, dst.w, outW);
        if (emptySides) {
            static bool logged;
            if (!logged) { PortLog("fbdev: empty side repairs skipped"); logged = true; }
        }
#endif
        if (!emptySides && !menuCanvas && sSideStyle == 8 && sFitScale <= 0) {
            bool drawSides = true;
#ifdef PORT_LINUX
            extern Mode gModeWorldinspect;
            const char* skipWorldInspectSides;
            PORT_ENV_ONCE(skipWorldInspectSides, SDL_getenv, "KHCOM_GA36_SKIP_WORLDINSPECT_SIDES");
            if (skipWorldInspectSides && gCurrentMode == &gModeWorldinspect && PortFixed480p() && PortFbActive() &&
                !PortJiminyHasSideMargins(dst.x, dst.w, outW)) {
                static bool logged;
                if (!logged) { PortLog("fbdev: world inspect empty sides skipped"); logged = true; }
                drawSides = false;
            }
#endif
            if (drawSides) {
                DrawEdgeBands(&src, &dst, outW, 3, GBA_SCREEN_H, GBA_SCREEN_H);
                DrawHeaderLeft(&src, &dst);
            }
        }
        bool skipJournalSides = false;
#ifdef PORT_LINUX
        const char* skipEmptyJournalSides;
        PORT_ENV_ONCE(skipEmptyJournalSides, SDL_getenv, "KHCOM_GA36_SKIP_EMPTY_JOURNAL_SIDES");
        /* All of the journal side repairs below paint outside dst.  At 4:3
           the journal fills the viewport horizontally, so there is no side
           region to sample, classify or paint. */
        skipJournalSides = skipEmptyJournalSides && PortFixed480p() && PortFbActive() && journalShown &&
            !PortJiminyHasSideMargins(dst.x, dst.w, outW);
#endif
        if (!emptySides && !skipJournalSides && !menuCanvas && (sSideStyle == 9 || sSideStyle == 12) && sFitScale <= 0) {
            /* style 9: the scene's ground (its commonest colour) and its thick
               bars (blocks of six rows or more, so a frame's thin border is
               not one) carried on; nothing mirrored, nothing repeated */
            static uint32_t colors[64];
            static int counts[64];
            int n = 0, best = 0, i, gx, gy2;
            uint32_t journalRightGround = 0;
            int pageExpanded = 0;
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
                    if (s2 == 1) journalRightGround = c;
                    SDL_SetRenderDrawColor(sRenderer, (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c, 255);
                    SDL_RenderFillRect(sRenderer, s2 ? &right : &left);
                }
            }
            sBandGround = n ? colors[best] : 0;
            if (journalShown) pageExpanded = RepairJiminyPageMargin(&src,&dst,outW);
            DrawEdgeBands(&src, &dst, outW, 6,
                journalShown ? 16 : GBA_SCREEN_H,
                journalShown ? 20 : GBA_SCREEN_H);
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
                    if (journalShown && yy >= 16) continue;
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
            if (journalShown && sSideStyle == 12)
                DrawJournalNameMargins(&dst, outW, pageExpanded ? 0 : journalRightGround);
        }
        if (SDL_getenv("KHCOM_DBGSIDE")) PortLog("side f%u style %d fit %.2f wide %d src %.0f,%.0f %.0fx%.0f", (unsigned)PortFrameCount(), sSideStyle, sFitScale, gPortScreen.wide, src.x, src.y, src.w, src.h);
        if (!emptySides && !menuCanvas && sSideStyle != 10 && sFitScale <= 0 && UniformPicture(&src, &uniform)) {
            /* a fade has reached one flat colour (or the screen is blank
               between modes): the margins are that colour too, so the
               transition stays one smooth fade */
            SDL_FRect a = { 0, dst.y, dst.x, dst.h }, b2 = { dst.x + dst.w, dst.y, outW - dst.x - dst.w, dst.h };
            SDL_SetRenderDrawColor(sRenderer, (Uint8)(uniform >> 16), (Uint8)(uniform >> 8), (Uint8)uniform, 255);
            SDL_RenderFillRect(sRenderer, &a);
            SDL_RenderFillRect(sRenderer, &b2);
        } else if (!emptySides && !menuCanvas && sSideStyle != 3 && sSideStyle != 8 && sSideStyle != 9 && sSideStyle != 10 &&
                   sSideStyle != 12 && sFitScale <= 0) {
            SideBar((int)src.x, (int)src.y, -1, dst.y, scale, 0.0f, dst.x, (int)src.x);
            SideBar((int)(src.x + src.w) - SIDE_STRIP, (int)src.y, 1, dst.y, scale, dst.x + dst.w, (float)outW,
                    (int)(src.x + src.w) - 1);
            /* style 11 (the save files): the frame's bars and title band
               carried on over the scenery's own margin */
            if (sSideStyle == 11) {
                DrawEdgeBands(&src, &dst, outW, 3, GBA_SCREEN_H, GBA_SCREEN_H);
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
        lcdSourceW = src.w; lcdSourceH = src.h;
        worldScale = scale;
        worldX = dst.x - src.x * scale;
        worldY = dst.y - src.y * scale;
    } else if (PortHd2dActive() && !menuCanvas) {
        SDL_RenderClear(sRenderer);
        /* HD-2D build: the processed, larger picture, filtered to the window */
        static uint8_t* sMask;
        if (!sMask)
            sMask = (uint8_t*)SDL_calloc((size_t)PORT_MAX_W * PORT_MAX_H, 1);
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
        const uint64_t _base = SDL_GetPerformanceCounter();
        OutputRect(outW, outH, &dst);
        bool drawn2x = false;
#if defined(PORT_LINUX) && defined(__ARM_NEON)
        if (!menuCanvas && sFitScale <= 0 && PortFixedFullscreen() && gPortConfig.filter == PORT_FILTER_NEAREST &&
            dst.x == 0 && dst.y == 0 && dst.w == outW && dst.h == outH) {
            const uint64_t scaleStart = measureFb ? SDL_GetPerformanceCounter() : 0;
            drawn2x = PortFbDrawNearest2x(sRenderer, sComposite, gPortScreen.width, gPortScreen.height, outH, NULL);
            if (measureFb) {
                scaleTicks = SDL_GetPerformanceCounter() - scaleStart;
                usedScale2x = drawn2x;
            }
        }
#endif
        if (!drawn2x)
            SDL_RenderClear(sRenderer);
        if (sFitScale > 0 && !menuCanvas) {
            /* Battle-entry animation stays at its authored aspect ratio.
               Fill the surrounding window with the same dim blur as movies. */
            SDL_FRect backdrop = { (float)gPortScreen.originX,
                                   (float)gPortScreen.originY,
                                   GBA_SCREEN_W, GBA_SCREEN_H };
            DrawBlurredReflection(&backdrop, outW, outH, &dst);
        }
        {
            /* the Options panel's HD picture under its canvas, which leaves it clear (port_ui.c): the canvas then goes
               up with its alpha, in a texture of its own (the frame's has none) */
            extern int PortUiHdArtUnder(SDL_Renderer*, float, float, float);
            SDL_Texture* frameTex = sTexture;
            if (menuCanvas && gPortConfig.hdTitle && PortUiHdArtUnder(sRenderer, dst.x, dst.y, dst.w / gPortScreen.width)) {
                if (sMenuAlpha && (sMenuAlphaW != gPortScreen.width || sMenuAlphaH != gPortScreen.height)) {
                    SDL_DestroyTexture(sMenuAlpha);
                    sMenuAlpha = NULL;
                }
                if (!sMenuAlpha) {
                    sMenuAlphaW = gPortScreen.width;
                    sMenuAlphaH = gPortScreen.height;
                    sMenuAlpha = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                                   sMenuAlphaW, sMenuAlphaH);
                    if (sMenuAlpha)
                        SDL_SetTextureBlendMode(sMenuAlpha, SDL_BLENDMODE_BLEND);
                }
                if (sMenuAlpha) {
                    SDL_SetTextureScaleMode(sMenuAlpha, gPortConfig.filter == PORT_FILTER_LINEAR
                                                            ? (SDL_ScaleMode)PortSmoothScaleMode() : SDL_SCALEMODE_NEAREST);
                    SDL_UpdateTexture(sMenuAlpha, NULL, sComposite, gPortScreen.width * 4);
                    frameTex = sMenuAlpha;
                }
            }
#if defined(PORT_LINUX) && defined(__ARM_NEON)
            if (!drawn2x && sFitScale > 0 && !menuCanvas && frameTex == sTexture &&
                PortFixed480p() && gPortConfig.filter == PORT_FILTER_NEAREST)
                drawn2x = PortFbDrawFit2x(sRenderer, sComposite, gPortScreen.width, gPortScreen.height, &dst);
            if (!drawn2x && !(PortFixedFullscreen() && frameTex == sTexture && gPortConfig.filter == PORT_FILTER_NEAREST &&
                  dst.x == 0 && dst.y == 0 && dst.w == outW && dst.h == outH &&
                  outH == gPortScreen.height * 2 &&
                  PortFbDrawNearest2x(sRenderer, sComposite, gPortScreen.width, gPortScreen.height, outH, NULL)))
#endif
            {
                if (deferBase)
                    SDL_UpdateTexture(sTexture, NULL, sComposite, gPortScreen.width * 4);
                SDL_RenderTexture(sRenderer, frameTex, NULL, &dst);
            }
        }
        worldScale = dst.w / gPortScreen.width;
        worldX = dst.x;
        worldY = dst.y;
        gPortPiece[10] += SDL_GetPerformanceCounter() - _base;
        if (menuCanvas) {
            /* Options > HD text > A: the font samples at the screen's resolution over their boxes (port_ui.c) */
            extern int PortUiHdSamples(int rects[2][4], int faces[2], const unsigned char** rgba, int* sw, int* sh);
            int rects[2][4], faces[2], sw = 0, sh = 0, n, i;
            const unsigned char* rgba = NULL;
            n = PortUiHdSamples(rects, faces, &rgba, &sw, &sh);
            if (n && !sSampleTex) {
                sSampleTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, sw, sh);
                if (sSampleTex) {
                    SDL_UpdateTexture(sSampleTex, NULL, rgba, sw * 4);
                    SDL_SetTextureBlendMode(sSampleTex, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureScaleMode(sSampleTex, SDL_SCALEMODE_LINEAR);
                }
            }
            for (i = 0; i < n && sSampleTex; i++) {
                SDL_FRect src = { 0, (float)(faces[i] * (sh / 2)), (float)sw, (float)(sh / 2) };
                SDL_FRect at = { worldX + rects[i][0] * worldScale, worldY + rects[i][1] * worldScale,
                                 rects[i][2] * worldScale, rects[i][3] * worldScale };
                SDL_RenderTexture(sRenderer, sSampleTex, &src, &at);
            }
        }
    }
    if (!menuCanvas) {
        PIECE_CK(11);
        /* HD sprites of the game's picture (the title's logo), with the game's fade */
        uint16_t bld = *(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x50);
        int effect = (bld >> 6) & 3, amount = SDL_min(*(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x54) & 31, 16);
{ uint64_t _pt = SDL_GetPerformanceCounter();
        PortHdSpritesDraw(sRenderer, 0, worldX, worldY, worldScale, (bld & 0x10) && effect == 3 ? amount : 0,
                          (bld & 0x10) && effect == 2 ? amount : 0);
 gPortPiece[0] += SDL_GetPerformanceCounter() - _pt; }
        PIECE_CK(20);
        if (mainWorldLight && !nativeMainWorld) {
            SDL_FRect lightRect = { worldX, worldY, gPortScreen.width * worldScale, gPortScreen.height * worldScale };
            if (!PortScreenWorldBlendDraw(&sWorldLight, sRenderer, &lightRect)) {
                const uint32_t* native = PortPpuMainNativeWorld();
                if (!SDL_UpdateTexture(sTexture, NULL, native, gPortScreen.width * 4) ||
                    !SDL_RenderTexture(sRenderer, sTexture, NULL, &lightRect)) {
                    sRendererResetPending = 1;
                    return;
                }
                PortHdSpritesWorldFallback(1);
                PortHdSpritesDraw(sRenderer, 0, worldX, worldY, worldScale,
                    (bld & 0x10) && effect == 3 ? amount : 0, (bld & 0x10) && effect == 2 ? amount : 0);
            }
        }
        PIECE_CK(21);
        uint64_t _mo = SDL_GetPerformanceCounter();
        {
            const uint32_t* over = PortPpuMainOver();
            if (over) {
                SDL_FRect od = { worldX, worldY, gPortScreen.width * worldScale, gPortScreen.height * worldScale };
#ifdef PORT_LINUX
                /* the handheld's framebuffer: the sprites over the HD parts (the cards' values and borders over the HD
                   cards) are a sparse whole-screen layer; SDL's scaled copy of all of it cost GA36 17 million
                   instructions a frame (QEMU), the direct blend visits only its drawn spans */
                extern bool PortFbBlendOverlayScaledSparse(SDL_Renderer*, const uint32_t*, int, int, const SDL_Rect*);
                const SDL_Rect odi = { (int)od.x, (int)od.y, (int)od.w, (int)od.h };
                const char* overSdl;
                PORT_ENV_ONCE(overSdl, SDL_getenv, "KHCOM_MAIN_OVER_SDL");
                if (PortFbActive() && sPictureScaleMode == SDL_SCALEMODE_NEAREST && !overSdl &&
                    od.x == odi.x && od.y == odi.y && od.w == odi.w && od.h == odi.h &&
                    PortFbBlendOverlayScaledSparse(sRenderer, over, gPortScreen.width, gPortScreen.height, &odi))
                    over = NULL;
                /* a box past the screen or off whole pixels (the HD title's zoomed canvas): clipped, nearest */
                extern bool PortFbBlendOverlayNearestClipped(SDL_Renderer*, const uint32_t*, int, int, float, float, float,
                                                             float);
                if (over && PortFbActive() && sPictureScaleMode == SDL_SCALEMODE_NEAREST && !overSdl &&
                    PortFbBlendOverlayNearestClipped(sRenderer, over, gPortScreen.width, gPortScreen.height, od.x, od.y,
                                                     od.w, od.h))
                    over = NULL;
#endif
            }
            if (over) {
                SDL_FRect od = { worldX, worldY, gPortScreen.width * worldScale, gPortScreen.height * worldScale };
                if (sPieceFlush > 0 && sFrameCount % 300 == 0)
                    PortLog("probe mainover sdl: fb %d scale %d od %.2f,%.2f %.2fx%.2f", PortFbActive(),
                            (int)sPictureScaleMode, od.x, od.y, od.w, od.h);
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
{ uint64_t _pt = SDL_GetPerformanceCounter();
        gPortPiece[9] += SDL_GetPerformanceCounter() - _mo;
        PIECE_CK(22);
        PortHdTextDraw(sRenderer, 0, worldX, worldY, worldScale);
 gPortPiece[1] += SDL_GetPerformanceCounter() - _pt; }
        uint64_t _footer = SDL_GetPerformanceCounter();
        if (gPortConfig.hdTitle && gPortConfig.viewMode != PORT_VIEW_ORIGINAL) {
            /* The title's HD poster is drawn after the ordinary side fill.
               Reapply the authored margin bands above it, including the
               black OBJ underline which lives outside BG0's gradient. */
            int side, row;
            for (side = 0; side < 2; side++) {
                float edge = PixelEdge(worldX, gPortScreen.originX + (side ? GBA_SCREEN_W : 0), worldScale);
                float left = side ? SDL_clamp(edge, 0.0f, (float)outW) : 0.0f;
                float right = side ? (float)outW : SDL_clamp(edge, 0.0f, (float)outW);
                if (right <= left) continue;
                for (row = 0; row < GBA_SCREEN_H; row++) {
                    uint32_t c = PortTitleFooterPixel(side, row);
                    if (c) {
                        float top = PixelEdge(worldY, gPortScreen.originY + row, worldScale);
                        float bottom = PixelEdge(worldY, gPortScreen.originY + row + 1, worldScale);
                        SDL_FRect band = { left, top, right - left, bottom - top };
#ifdef PORT_LINUX
                        {
                            /* the handheld's framebuffer: the band's rows written straight in; SDL's 320 small
                               fills a frame cost GA36 about 11 million instructions (QEMU) */
                            extern bool PortFbFillRows(SDL_Renderer*, const uint32_t*, int, int, int, int);
                            static uint32_t rowPx[1024];
                            const int bx = (int)SDL_ceilf(left - 0.5f), bw = (int)SDL_ceilf(right - 0.5f) - bx;
                            if (PortFbActive() && bw > 0 && bw <= 1024) {
                                for (int k = 0; k < bw; k++) rowPx[k] = 0xFF000000u | (c & 0xFFFFFFu);
                                if (PortFbFillRows(sRenderer, rowPx, bx, bw, (int)SDL_ceilf(top - 0.5f),
                                                   (int)SDL_ceilf(bottom - 0.5f)))
                                    continue;
                            }
                        }
#endif
                        SDL_SetRenderDrawColor(sRenderer, (c >> 16) & 255, (c >> 8) & 255, c & 255, 255);
                        SDL_RenderFillRect(sRenderer, &band);
                    }
                }
            }
        }
        gPortPiece[5] += SDL_GetPerformanceCounter() - _footer;
    }
    /* Optional sampling to identify which overlay geometry dominates a menu. */
    const char* overlayAudit;
    PORT_ENV_ONCE(overlayAudit, SDL_getenv, "KHCOM_OVERLAY_AUDIT");
    if (overlayAudit && sFrameCount % 120 == 0)
        PortLog("overlay audit: frame %u active=%d menu=%d scale=%.3f width=%d", (unsigned)sFrameCount,
                PortPpuOverlayActive(), menuCanvas, sOverlayScale, gPortOverlayW);
    /* the options menu covers everything: no layer is drawn over it */
    if (PortPpuOverlayActive() && sOverlayScale > 0 && !menuCanvas) {
        SDL_FRect od;
        const uint32_t* worldLight = PortPpuScreenAdditive();
        int nativeWorld = worldLight && !PortScreenWorldBlendPrepare(&sWorldLight, sRenderer, worldLight, gPortOverlayW);
        const uint32_t* overlay = nativeWorld ? PortPpuScreenNativeWorld() : gPortOverlay;
        if (sOverlayTex == NULL || sOverlayTexW != gPortOverlayW) {
            if (sOverlayTex != NULL)
                SDL_DestroyTexture(sOverlayTex);
            sOverlayTexW = gPortOverlayW;
            sOverlayTex = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                            sOverlayTexW, GBA_SCREEN_H);
            SDL_SetTextureBlendMode(sOverlayTex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(sOverlayTex, SDL_SCALEMODE_PIXELART);
        }
        od.w = sOverlayTexW * sOverlayScale;
        od.h = GBA_SCREEN_H * sOverlayScale;
        od.x = (float)(int)((outW - od.w) / 2);
        od.y = (float)(int)((outH - od.h) / 2);
        PIECE_CK(16);
        if (!ScreenOverlayEmpty(overlay, sOverlayTexW)) {
#ifdef PORT_LINUX
            const char* menuBands43;
            extern int PortGa36FieldMenuBars(void);
            PORT_ENV_ONCE(menuBands43, SDL_getenv, "KHCOM_GA36_MENU_43_BANDS");
            if (menuBands43 && PortFixed480p() && PortFbActive() && PortGa36FieldMenuBars() &&
                gPortConfig.aspect == PORT_ASPECT_4_3 && gPortConfig.graphicsMode == PORT_MODE_GBA_ENHANCED &&
                outW == 640 && outH == 480 && sOverlayTexW == GBA_SCREEN_W &&
                (int)od.x == 0 && (int)od.w == outW && od.y > 0) {
                uint32_t color;
                const int top = (int)od.y, bottom = top + (int)od.h;
                if (PortOverlaySolidEdge(overlay, sOverlayTexW, &color)) {
                    SDL_FRect band = {0, 0, (float)outW, (float)top};
                    SDL_SetRenderDrawColor(sRenderer, (color >> 16) & 255, (color >> 8) & 255, color & 255, 255);
                    SDL_RenderFillRect(sRenderer, &band);
                }
                if (bottom < outH && PortOverlaySolidEdge(overlay + (GBA_SCREEN_H - 1) * sOverlayTexW,
                                                          sOverlayTexW, &color)) {
                    SDL_FRect band = {0, (float)bottom, (float)outW, (float)(outH - bottom)};
                    SDL_SetRenderDrawColor(sRenderer, (color >> 16) & 255, (color >> 8) & 255, color & 255, 255);
                    SDL_RenderFillRect(sRenderer, &band);
                }
            }
#endif
            if (!DrawLevelUpFlat(overlay, &od, sOverlayTexW, outW, outH)) {
                SDL_UpdateTexture(sOverlayTex, NULL, overlay, sOverlayTexW * 4);
                DrawScreenOverlay(sOverlayTex, overlay, &od, sOverlayTexW, outW, outH);
            }
        }
        PIECE_CK(17);
        PortHdSpritesWorldFallback(nativeWorld);
{ uint64_t _pt = SDL_GetPerformanceCounter();
        PortHdSpritesDraw(sRenderer, 2, od.x, od.y, sOverlayScale, 0, 0); /* menus' HD parts, the title's logo */
 gPortPiece[2] += SDL_GetPerformanceCounter() - _pt; }
        if (worldLight && !nativeWorld && !PortScreenWorldBlendDraw(&sWorldLight, sRenderer, &od)) {
            /* A failed draw cannot leave the replacement without its light.
             * Repaint this same frame from retained native pixels, with all
             * other HD words/parts intact. No game tick or PPU replay occurs. */
            overlay = PortPpuScreenNativeWorld();
            if (!ScreenOverlayEmpty(overlay, sOverlayTexW) &&
                (!SDL_UpdateTexture(sOverlayTex, NULL, overlay, sOverlayTexW * 4) ||
                 !DrawScreenOverlay(sOverlayTex, overlay, &od, sOverlayTexW, outW, outH))) {
                sRendererResetPending = 1;
                return; /* Keep the last presented frame while audio/gameplay continue. */
            }
            PortHdSpritesWorldFallback(1);
            PortHdSpritesDraw(sRenderer, 2, od.x, od.y, sOverlayScale, 0, 0);
        }
        PIECE_CK(18);
        const uint32_t* screenOver = PortPpuScreenOver();
        if (screenOver && !ScreenOverlayEmpty(screenOver, gPortOverlayW)) {
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
                SDL_UpdateTexture(sScreenOverTex, NULL, screenOver, sScreenOverTexW * 4);
                DrawScreenOverlay(sScreenOverTex, screenOver, &od, sScreenOverTexW, outW, outH);
            }
        }
{ uint64_t _pt = SDL_GetPerformanceCounter();
        PIECE_CK(19);
        if (!menuCanvas)
            PortHdTextDraw(sRenderer, 2, od.x, od.y, sOverlayScale);
 gPortPiece[3] += SDL_GetPerformanceCounter() - _pt; }
    }
    PIECE_CK(12);
    if (PortPpuHudActive() && !menuCanvas) {
        int savedHud[4];
        PortPpuHudFrameBegin(savedHud);
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
        PortPpuRenderHud(sHudBuf);
        /* The field HUD can be just a small floor badge on a window-sized
         * transparent texture. Blending the empty area cost a whole frame on
         * SDL's software renderer. Integer HUD scaling lets us crop the draw
         * to nontransparent pixels without changing any output pixel. */
        SDL_FRect hudSource = { 0, 0, (float)sHudTexW, (float)sHudTexH };
        int hudLeft=sHudTexW, hudTop=sHudTexH, hudRight=0, hudBottom=0, hx,hy;
        if (!SDL_getenv("KHCOM_HUD_FULL_BLIT")) {
            for (hy=0;hy<sHudTexH;hy++) for (hx=0;hx<sHudTexW;hx++)
                if (sHudBuf[hy*sHudTexW+hx] >> 24) {
                    if (hx<hudLeft) hudLeft=hx;
                    if (hy<hudTop) hudTop=hy;
                    if (hx>=hudRight) hudRight=hx+1;
                    if (hy>=hudBottom) hudBottom=hy+1;
                }
            hudSource.x=(float)hudLeft; hudSource.y=(float)hudTop;
            hudSource.w=(float)(hudRight>hudLeft?hudRight-hudLeft:0);
            hudSource.h=(float)(hudBottom>hudTop?hudBottom-hudTop:0);
        }
        SDL_UpdateTexture(sHudTexture, NULL, sHudBuf, sHudTexW * 4);
        hd.w = (float)(sHudTexW * gPortHudScale);
        hd.h = (float)(sHudTexH * gPortHudScale);
        hd.x = (float)((outW - (int)hd.w) / 2);
        hd.y = (float)((outH - (int)hd.h) / 2);
        PortHdHudDraw(sRenderer, outW, outH);
        if (hudSource.w>0 && hudSource.h>0) {
            SDL_FRect hudDraw = { hd.x+hudSource.x*gPortHudScale, hd.y+hudSource.y*gPortHudScale,
                                 hudSource.w*gPortHudScale, hudSource.h*gPortHudScale };
            SDL_RenderTexture(sRenderer, sHudTexture, &hudSource, &hudDraw);
        }
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
        {
            uint16_t bld = *(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x50);
            int effect = (bld >> 6) & 3, amount = SDL_min(*(volatile uint16_t*)(uintptr_t)(GBA_IO + 0x54) & 31, 16);
            PortHdSpritesHudFront(sRenderer, hd.x, hd.y, (float)gPortHudScale,
                                  (bld & 0x10) && effect == 3 ? amount : 0, (bld & 0x10) && effect == 2 ? amount : 0);
        }
        PortHdHudDrawBar(sRenderer, outW, outH);
        PortHdHudDrawLabel(sRenderer, outW, outH);
        if (!menuCanvas) PortHdTextHudDigits(sRenderer, hd.x, hd.y, (float)gPortHudScale);
        PortHdTextDraw(sRenderer, 1, hd.x, hd.y, (float)gPortHudScale);
        PortPpuHudFrameEnd(savedHud);
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
    PieceLog();
    if (gPortConfig.hdTitle) {
        extern void PortUiHdDraw(SDL_Renderer*, float, float, float);
{ uint64_t _pt = SDL_GetPerformanceCounter();
        PortUiHdDraw(sRenderer, worldX, worldY, worldScale);
 gPortPiece[4] += SDL_GetPerformanceCounter() - _pt; }
    }
    PIECE_CK(13);
    if (gPortConfig.viewMode != PORT_VIEW_CRT) /* the CRT or its shader draws them */
        DrawScanlines(&dst, outW);
    if (journalShown && journalLayout.footerHeight > 0) {
        int dark = 0, white = 0;
        /* Bank5 belongs to the header in all journal maps. Bank0 belongs to
           portraits and may be cleared while an otherwise visible book opens. */
        PortPpuJiminyFade(&dark, &white);
        PortUiJournalFooter(sRenderer, outW, journalLayout.footerY, journalLayout.footerHeight,
                             journalLayout.scale, sSideStyle == 12, dark, white);
    }
    if (gPortConfig.lcdGrid > 0)
        DrawLcdGrid(outW, outH, worldX, worldY, dst.w / lcdSourceW, dst.h / lcdSourceH);
    else
        sLcdGridAttempted = 0;
    {
        /* state / slot notices: topmost, at the window's scale */
        static uint32_t sNoticeBuf[256 * 14];
        int nw;

        memset(sNoticeBuf, 0, sizeof(sNoticeBuf));
        PortUiSetFps(sFps, sLogicFps);
        nw = PortUiDrawNotice(sNoticeBuf, 256, 14);
        if (nw > 0) {
            float f = SDL_max(1.0f, SDL_floorf(outH / 360.0f)); /* about a sixth of the old size at 1080p: x3 */
            SDL_FRect src = { 0, 0, (float)nw, 14 };
            SDL_FRect nd = { 4 * f, 4 * f, nw * f, 14 * f };

            if (gCurrentMode == &gModeVsbattle) /* a link battle: between the two HP gauges, not over the player's */
                nd.x = SDL_floorf((outW - nw * f) / 2);

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
    if (borderOn) {
        PortGbaBorderRect aperture = { (float)(contentArea.x - borderOuter.x),
            (float)(contentArea.y - borderOuter.y), (float)contentArea.w, (float)contentArea.h };
        /* Fixed views share the picture-sized viewport above. Auto's
         * extended UI/paper/footer layers may genuinely fill its canvas. */
        if (!menuCanvas && !journalShown && !PortPpuHudActive() && !PortPpuOverlayActive()) {
            aperture.x += dst.x;
            aperture.y += dst.y;
            aperture.w = dst.w;
            aperture.h = dst.h;
        }
        SDL_SetRenderViewport(sRenderer, &borderOuter);
        DrawGbaBorder(borderOuter.w, borderOuter.h, &aperture);
        SDL_SetRenderViewport(sRenderer, NULL);
    }
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
    {
        /* KHCOM_PRESENTSHOTS=a-b[/step] (harness): the window as shown, frames a..b (present_<frame>.bmp) */
        const char* range = SDL_getenv("KHCOM_PRESENTSHOTS");
        int from = 0, to = -1, step = 1;
        if (range && SDL_sscanf(range, "%d-%d/%d", &from, &to, &step) >= 2 && (int)sFrameCount >= from &&
            (int)sFrameCount <= to && ((int)sFrameCount - from) % (step > 0 ? step : 1) == 0) {
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
        extern void PortSceneGalleryPreviewPresented(int accepted);
        /* Finish rendering first, then submit the image on its fixed deadline.
           Sleeping only afterwards let variable draw cost become image jitter. */
#ifdef PORT_LINUX
        const uint64_t ready = measureFb ? SDL_GetPerformanceCounter() : 0;
#endif
        if (paced) PaceBeforePresent();
        LoopMark(PORT_LOOP_DRAW); /* the wait just taken is counted apart */
#ifdef PORT_LINUX
        PIECE_CK(14);
        PortFbPresent(sRenderer);
        PIECE_CK(15);
        if (measureFb) {
            uint64_t drawTicks, copyTicks;
            PortFbStageTicks(&drawTicks, &copyTicks);
            const double ms = 1000.0 / SDL_GetPerformanceFrequency();
            PortLog("fb phases: frame %u prep %.3f upload %.3f scale %.3f direct %d flush %.3f copy %.3f ms",
                    (unsigned)sFrameCount, (ready - detailStart) * ms, uploadTicks * ms,
                    scaleTicks * ms, usedScale2x, drawTicks * ms, copyTicks * ms);
        }
#endif
        int presented=SDL_RenderPresent(sRenderer);
        LoopMark(PORT_LOOP_FLIP);
        if (presented) {
            uint64_t acceptedAt = SDL_GetPerformanceCounter();
            sPresentGapTicks = sPresentSerial ? acceptedAt - sPresentAcceptedAt : 0;
            sPresentAcceptedAt = acceptedAt;
            sPresentSerial++;
            sFpsFrames++;
        }
        PortSceneGalleryPreviewPresented(presented);
    }
    if (hdProfile)
        PortLog("hd-menu-perf: frame=%u menu=%d ms=%.3f alloc_delta=%d allocations=%d", (unsigned)sFrameCount,
                menuCanvas, (SDL_GetPerformanceCounter() - hdProfileStart - (sPresentWaitTotal - hdProfileWaitStart)) * 1000.0 / SDL_GetPerformanceFrequency(),
                hdProfileAlloc >= 0 ? HdMenuAllocations() - hdProfileAlloc : -1, HdMenuAllocations());
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
    {
        extern int gPortGalleryEvent;
        extern unsigned PortSceneGalleryAge(void);
        const char* automatic = SDL_getenv("KHCOM_GALLERY_AUTO_KEYS");
        if (gPortGalleryEvent && automatic) {
            unsigned age = PortSceneGalleryAge();
            if (age % 6 < 2) keys |= 1;
            if (!SDL_strcmp(automatic, "skip") && age % 300 >= 80 && age % 300 < 180) keys |= 8;
            if (!SDL_strcmp(automatic, "cancel") && age % 300 >= 80 && age % 300 < 84) keys |= 8;
        }
    }

    while (p && *p) {
        int a = SDL_atoi(p), b = a;
        const char* dash = SDL_strchr(p, '-');
        const char* colon = SDL_strchr(p, ':');
        const char* comma = SDL_strchr(p, ',');
        size_t i;

        if (colon == NULL || (comma && colon > comma)) {
            p = comma ? comma + 1 : NULL;
            continue;
        }
        if (dash && dash < colon)
            b = SDL_atoi(dash + 1);
        if (frame >= (uint32_t)a && frame <= (uint32_t)b) {
            const char* key = colon + 1;
            const char* end = comma ? comma : p + SDL_strlen(p);
            while (key < end) {
                const char* next = key;
                while (next < end && *next != '+') next++;
                for (i = 0; i < SDL_arraysize(names); i++) {
                    size_t len = SDL_strlen(names[i].name);
                    if ((size_t)(next - key) == len && SDL_strncmp(key, names[i].name, len) == 0)
                        keys |= names[i].bit;
                }
                key = next < end ? next + 1 : end;
            }
        }
        p = comma ? comma + 1 : NULL;
    }
    return keys;
}

static void Present(int paced);

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
        extern int PortUiGalleryHarnessDone(void);
        extern int PortScenePosterFinished(void);
        if (PortUiGalleryHarnessDone() || PortScenePosterFinished()) Quit();
    }

    {
        /* KHCOM_HDMOVIE_PLAY=<frame>:<movie>: that cartridge movie's HD replacement (port_hdmovie.c MovieFile) */
        const char* hdPlay = SDL_getenv("KHCOM_HDMOVIE_PLAY");
        if (hdPlay && (uint32_t)SDL_atoi(hdPlay) == sFrameCount && SDL_strchr(hdPlay, ':')) {
            extern int PortHdMoviePlay(int movieId);
            PortLog("harness: hd movie %d played %d", SDL_atoi(SDL_strchr(hdPlay, ':') + 1),
                    PortHdMoviePlay(SDL_atoi(SDL_strchr(hdPlay, ':') + 1)));
        }
    }
    {
        /* KHCOM_GALLERY_PLAY=<frame>:<event>[,<frame>:<event>...]: Options > Scenes > GBA plays that event from the
           title (several: repeated visits) */
        const char* gallery = SDL_getenv("KHCOM_GALLERY_PLAY");
        while (gallery && *gallery) {
            const char* colon = SDL_strchr(gallery, ':');
            const char* comma = SDL_strchr(gallery, ',');
            if (colon && (!comma || colon < comma) && (uint32_t)SDL_atoi(gallery) == sFrameCount) {
                extern int PortSceneGalleryPlay(int eventId);
                PortLog("harness: gallery play %d: %s", SDL_atoi(colon + 1),
                        PortSceneGalleryPlay(SDL_atoi(colon + 1)) ? "started" : "refused");
            }
            gallery = comma ? comma + 1 : NULL;
        }
    }
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
        Present(1);
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

int gPortExitCode; /* REKHCOM_EXIT_QUIT_ALL after Options > Quit the game */
static int sQuitAllRequested;
void PortRequestQuitAll(void) {
    gPortExitCode = REKHCOM_EXIT_QUIT_ALL;
    sQuitAllRequested = 1;
}
static void Quit(void) {
#ifdef PORT_LINUX
    PortFbStopCopy();
#endif
    void PortHdHudShutdown(void);
    extern void PortSceneGalleryFinalShutdown(void);
    PortSramSyncIfDirty();
    PortSceneGalleryFinalShutdown();
    PortConfigSave();
    PortAchievementsShutdown();
    PortDiscordShutdown();
    PortHdMusicShutdown();
    PortAudioShutdown();
    extern void PortHdTextShutdown(void);
    PortHdTextShutdown();
    PortHdHudShutdown();
    PortHdSpritesShutdown();
    PortHdLayoutShutdown();
#if defined(__ANDROID__) && !defined(PORT_ANDROID64)
    PortAndroidLifecycleStop();
#endif
    SDL_Quit();
    exit(gPortExitCode);
}

static void PumpEvents(void) {
    SDL_Event ev;

    if (sQuitAllRequested)
        Quit();
    while (SDL_PollEvent(&ev)) {
#if (defined(_WIN32) || defined(PORT_LINUX)) && !defined(PORT_GBA_RELOC)
        PortCursorIdleEvent(&sCursorIdle, sWindow, &ev);
#endif
        if (PortRendererHandleReset(ev.type)) continue;
        PortPromptHandleEvent(&ev);
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
                {
                    extern int PortSceneGalleryActive(void);
                    if (PortSceneGalleryActive() && (ev.key.key == SDLK_F5 || ev.key.key == SDLK_F6 ||
                                                    ev.key.key == SDLK_F7 || ev.key.key == SDLK_F8))
                        continue; /* a visual replay must not replace the paused original game with a quick state */
                }
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
                    if (!PortFixedFullscreen()) {
                        gPortConfig.fullscreen = !gPortConfig.fullscreen;
                        ApplyWindowMode();
                        PortConfigSave();
                    }
                    continue;
                }
                break;
        }
        if (PortUiHandleEvent(&ev))
            continue;
        PortInputHandleEvent(&ev);
    }
    PortUiHarnessStep();
#if (defined(_WIN32) || defined(PORT_LINUX)) && !defined(PORT_GBA_RELOC)
    PortCursorIdlePoll(&sCursorIdle, sWindow);
#endif
}

/* Keep the original GBA simulation clock. Present calls are counted where
   they actually occur; a skipped draw is not a displayed game frame. */
void PortFpsResetAfterStateLoad(void) {
    extern volatile uint32_t gFrameCounter;
    sLogicLast = gFrameCounter;
    sFpsTime = SDL_GetPerformanceCounter();
    sFpsFrames = 0;
}

static uint64_t AdvanceFrameDeadline(void) {
#ifdef __EMSCRIPTEN__
    int yielded = 0;
#endif
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t begin = now;
    double hz = GBA_FPS;
    uint64_t period;

    if (SDL_getenv("KHCOM_FAST")) {
        sNextFrame = 0;
        return 0;
    }
    sPacedByVsync = 0;
    period = (uint64_t)(sPerfFreq / hz);

    /* Exact GBA rate (59.7275 Hz) whatever the display does: sleep most of
       the wait, then spin for the last fraction of a millisecond. */
    if (sNextFrame == 0)
        sNextFrame = now;
    else if (now > sNextFrame + period * 4)
        sNextFrame = now - period; /* resume now, without another full-frame stall */
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
    now = SDL_GetPerformanceCounter();
#endif
    if (sLoop.threshold) /* late: only a sleep that returned past its deadline, not a frame already behind */
        PortLoopPhasesWait(&sLoop, now - begin, begin < sNextFrame && now > sNextFrame ? now - sNextFrame : 0);
    return now - begin;
}

static void PaceBeforePresent(void) {
    sPresentWaitTotal += AdvanceFrameDeadline();
    sPresentPaceEpoch++;
}

static void Pace(int idleAdvance) {
    static uint64_t consumedPresentEpoch;
    uint64_t now;
    /* A presented attempt already consumed its deadline, including failed
       renderer calls. A skipped draw still advances the simulation clock. */
    if (idleAdvance && consumedPresentEpoch == sPresentPaceEpoch)
        (void)AdvanceFrameDeadline();
    consumedPresentEpoch = sPresentPaceEpoch;
    if (SDL_getenv("KHCOM_FAST")) return;
    now = SDL_GetPerformanceCounter();
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

/* KHCOM_AUDIO_RESCUE_SKIP=frames (opt-in, port_audio_rate.h): while less than
   that much sound is queued, a frame that starts late is not drawn nor
   presented, never two in a row, so the clock is caught up before the pacer
   drops time and the queue drains; logic and sound run as always. Same guards
   as the frame skip above, which takes its place when it is on. */
static int AudioRescueDue(int sizeChanged) {
    static int frames = -1, skippedLast, window, total;
    uint64_t now, late;
    int skip;

    if (frames < 0)
        frames = PortAudioRescueFrames(SDL_getenv("KHCOM_AUDIO_RESCUE_SKIP"));
    if (!frames)
        return 0;
    if (gPortConfig.frameSkip || sizeChanged || !sNextFrame || SDL_getenv("KHCOM_FAST") || PortUiActive()) {
        skippedLast = 0;
        return 0;
    }
    now = SDL_GetPerformanceCounter();
    late = now > sNextFrame ? now - sNextFrame : 0;
    skip = PortAudioRescueDue(PortAudioQueueBelow(frames), late, (uint64_t)(sPerfFreq / GBA_FPS), &skippedLast);
    total += skip;
    if (++window == 600) {
        if (total)
            PortLog("audio rescue: %d of the last 600 frames not drawn", total);
        window = total = 0;
    }
    return skip;
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
PortStutterProbe gPortStutter;
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
    LoopMark(PORT_LOOP_SERVICES);
    uint64_t cycleStart = SDL_GetPerformanceCounter();
    uint64_t presentSerialStart = sPresentSerial;
    uint64_t presentPaceStart = sPresentPaceEpoch;
    uint64_t presentWaitStart = sPresentWaitTotal;
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
        gPortScreenLayer = gPortConfig.viewMode != PORT_VIEW_AUTO ? 0 : mark ? 1 : sPrevMark ? 2 : 0;
        sPrevMark = mark;
    }
    int skip;
    {
        int w = gPortScreen.width, h = gPortScreen.height;
        UpdateResolution();
        if (gPortConfig.vsync==2 && sFrameCount%60==0) ApplyVideoSync(0);
        const int sizeChanged = w != gPortScreen.width || h != gPortScreen.height;
        skip = FrameSkipDue(sizeChanged);
        if (!skip)
            skip = AudioRescueDue(sizeChanged);
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

    LoopMark(PORT_LOOP_SETUP);
    /* Visible lines (HBlank / VCount events in between). */
    if (skip) {
        PortPpuRunFrameEvents();
    } else {
        uint64_t t0 = SDL_GetPerformanceCounter();
        PortPpuRenderFrame();
        sPpuTicks = SDL_GetPerformanceCounter() - t0;
        {
            extern void PortSceneGalleryCapture(void);
            PortSceneGalleryCapture();
        }
    }

    LoopMark(PORT_LOOP_PPU);

    /* Line 160: vertical blank. */
    IO16(0x06) = GBA_SCREEN_H;
    IO16(0x04) = (IO16(0x04) & ~6) | 1 | (((IO16(0x04) >> 8) == GBA_SCREEN_H) ? 4 : 0);
    PortDmaVBlank();
    LoopMark(PORT_LOOP_DMA);
    if (IO16(0x04) & 0x08)
        PortRaiseIrq(0x0001);
    LoopMark(PORT_LOOP_IRQ);
    PortPpuVBlankLines();
    IO16(0x04) &= ~1;
    LoopMark(PORT_LOOP_VBLINES);

    PortAudioFrame();
    LoopMark(PORT_LOOP_AUDIO);
    {
        /* KHCOM_PROFILE: mean PPU and present time every 240 frames */
        static uint64_t ppuSum, presSum;
        static int n, drawn;
        uint64_t p0 = SDL_GetPerformanceCounter();
        uint64_t waitStart = sPresentWaitTotal, waited;
        if (!skip)
            Present(1);
        waited = sPresentWaitTotal - waitStart;
        sPresentTicks = SDL_GetPerformanceCounter() - p0 - waited;
        {
            extern void PortMenuLatch(void);
            PortMenuLatch(); /* this frame's menu sprites show from the next one */
        }
        if (SDL_getenv("KHCOM_PROFILE")) {
            if (!skip) {
                presSum += SDL_GetPerformanceCounter() - p0 - waited;
                ppuSum += sPpuTicks;
                drawn++;
            }
            if (++n == 240) {
                double f = 1000.0 / SDL_GetPerformanceFrequency() / (drawn ? drawn : 1);
                extern void PortPpuTakeStats(int out[5]);
                int st[5] = { 0 };
#ifdef PORT_PPU_THREADS
                PortPpuTakeStats(st);
#endif
                PortLog("profile: canvas %dx%d ppu %.2f ms present %.2f ms | outer rows multi-core %d (iso floor %d), one core: "
                        "mosaic %d header %d small %d | drawn %d of %d", gPortScreen.width, gPortScreen.height, ppuSum * f, presSum * f,
                        st[0], st[1], st[2], st[3], st[4], drawn, n);
                ppuSum = presSum = 0;
                n = drawn = 0;
            }
        }
    }
    LoopMark(PORT_LOOP_PRESENT_END);
    PumpEvents();
    LoopMark(PORT_LOOP_EVENTS);

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
    int uiProfile = PortUiActive() && SDL_getenv("KHCOM_UI_PROFILE") != NULL, uiProfileFrame = 0;
    static float uiProfileSamples[1024];
    while (PortUiActive()) {
        uint64_t uiStart = uiProfile ? SDL_GetPerformanceCounter() : 0;
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
        Present(1);
        PumpEvents();
#if defined(__ANDROID__) || defined(PORT_ANDROID64)
        while (sAndroidBackground) { SDL_Delay(50); PumpEvents(); }
#endif
        Pace(1);
        if (uiProfile) {
            double ms = (SDL_GetPerformanceCounter() - uiStart) * 1000.0 / SDL_GetPerformanceFrequency();
            if (uiProfileFrame < (int)SDL_arraysize(uiProfileSamples))
                uiProfileSamples[uiProfileFrame++] = (float)ms;
        }
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
    if (uiProfile)
        for (int i = 0; i < uiProfileFrame; i++)
            PortLog("options profile: frame %d %.3f ms", i + 1, (double)uiProfileSamples[i]);
    LoopMark(PORT_LOOP_UI); /* the whole paused menu, when it was open */
    uint16_t keys = PortInputKeys() | HarnessKeys(sFrameCount) | PortBotKeys(sFrameCount);
#if defined(PORT_LINUX)
    static int captureEnabled = -1;
    static unsigned captureFramesHeld, capturedTotal;
    static SDL_Time captureStarted;
    enum { CaptureBurstStride = 5, CaptureBurstLimit = 16, CaptureSessionLimit = 48 };
    const uint16_t captureKeys = L_BUTTON | R_BUTTON | SELECT_BUTTON;
    if (captureEnabled < 0)
        captureEnabled = PortFixedFullscreen() && SDL_getenv("KHCOM_HANDHELD_CAPTURE") != NULL;
    if (captureEnabled) {
        const bool pressed = (keys & captureKeys) == captureKeys;
        if (!pressed) {
            captureFramesHeld = 0;
        } else {
            if (captureFramesHeld == 0) {
                captureStarted = 0;
                SDL_GetCurrentTime(&captureStarted);
            }
            if (captureFramesHeld < CaptureBurstStride * CaptureBurstLimit) {
                if (captureFramesHeld % CaptureBurstStride == 0 && capturedTotal < CaptureSessionLimit) {
                    char framebufferName[64], rendererName[64];
                    SDL_snprintf(framebufferName, sizeof(framebufferName), "panel-%llu-%u.bmp",
                                 (unsigned long long)captureStarted, (unsigned)sFrameCount);
                    SDL_snprintf(rendererName, sizeof(rendererName), "captura-%llu-%u.bmp",
                                 (unsigned long long)captureStarted, (unsigned)sFrameCount);
                    if (!PortFbCapturePair(framebufferName, rendererName))
                        SDL_strlcpy(sHarnessShot, rendererName, sizeof(sHarnessShot));
                    capturedTotal++;
                }
                captureFramesHeld++;
            }
            keys &= (uint16_t)~captureKeys;
        }
    }
#endif
    IO16(0x130) = (uint16_t)(~PortInputResolve(keys) & 0x3FF);
    LoopMark(PORT_LOOP_INPUT);
    HarnessFrame();
    LoopMark(PORT_LOOP_HARNESS);
    {
        extern int gPortInMainLoop;
        PortRollbackBoundary(gPortInMainLoop); /* the network link: a wrong guess taken back, this frame kept */
    }
    LoopMark(PORT_LOOP_ROLLBACK);
    if (frameLogStart) frameLogWork = SDL_GetPerformanceCounter() - (sPresentWaitTotal - presentWaitStart);
    TimingLine(skip, cycleStart, SDL_GetPerformanceCounter() - (sPresentWaitTotal - presentWaitStart));
    LoopMark(PORT_LOOP_TIMING);
    /* Closing a menu already consumed its image deadlines in that loop.
       Only an entirely unpresented cycle needs an additional clock tick. */
    Pace(sPresentPaceEpoch == presentPaceStart);
    sCycleEnd = SDL_GetPerformanceCounter();
    LoopMark(PORT_LOOP_PACE);

    if (++sFrameCount % 60 == 0) {
#ifdef PORT_LINUX
        PortSramSyncPeriodic();
#else
        PortSramSyncIfDirty();
#endif
    }
    LoopMark(PORT_LOOP_SRAM);
    if (frameLogStart) {
        static FILE* frameLog;
        static int attempted;
        static uint64_t previous;
        uint64_t end = SDL_GetPerformanceCounter();
        extern volatile uint32_t gFrameCounter;
        if (!attempted) {
            attempted = 1;
            frameLog = fopen(frameLogPath, "w");
            gPortStutter.on = 1;
            if (frameLog) fprintf(frameLog, "frame,interval_ms,render_work_ms,logic,screen,fit,logic_ms,ppu_ms,present_ms,"
                                            "hd_loads,hd_load_ms,glyphs,glyph_ms,presented,present_interval_ms,skipped,fb_draw_ms,fb_copy_ms\n");
        }
        {
            /* the deep view of each frame: logic_ms is the time from the end of the previous frame's log to this
               cycle's start (the game's logic and whatever runs between cycles, not the autosave or this log),
               then the PPU, the present (HD layers included) and the one-off costs (gPortStutter); a frame past
               KHCOM_STUTTER ms (default 25) also leaves a line in kh_com.log naming them, with its slowest load.
               A diagnosis aid: it measures, it changes nothing. */
            static uint64_t logicStart;
            double ms = 1000.0 / sPerfFreq, interval = previous ? (end - previous) * ms : 0;
            double logic = logicStart && cycleStart > logicStart ? (cycleStart - logicStart) * ms : 0;
            double ppu = skip ? 0 : sPpuTicks * ms, present = skip ? 0 : sPresentTicks * ms;
            const char* limit = SDL_getenv("KHCOM_STUTTER");
            uint64_t fbDraw = 0, fbCopy = 0;
#ifdef PORT_LINUX
            if (sPresentSerial != presentSerialStart)
                PortFbStageTicks(&fbDraw, &fbCopy);
#endif
            if (frameLog && previous)
                fprintf(frameLog, "%u,%.6f,%.6f,%u,%d,%d,%.3f,%.3f,%.3f,%u,%.3f,%u,%.3f,%d,%.6f,%d,%.3f,%.3f\n", (unsigned)sFrameCount,
                        interval, (frameLogWork - frameLogStart) * ms, (unsigned)gFrameCounter, gPortScreenLayer,
                        (int)sFit, logic, ppu, present, (unsigned)gPortStutter.hdLoads, gPortStutter.hdLoadTicks * ms,
                        (unsigned)gPortStutter.glyphs, gPortStutter.glyphTicks * ms,
                        sPresentSerial != presentSerialStart,
                        sPresentSerial != presentSerialStart ? sPresentGapTicks * ms : 0.0, skip, fbDraw * ms, fbCopy * ms);
            if (previous && interval > (limit ? (double)SDL_atoi(limit) : 25.0))
                PortLog("stutter: frame %u %.1f ms: logic %.1f, ppu %.1f, present %.1f; hd loads %u (%.1f ms: header %.1f, "
                        "decode %.1f, upload %.1f; slowest %s %.1f ms), glyphs %u (%.1f ms)", (unsigned)sFrameCount,
                        interval, logic, ppu, present, (unsigned)gPortStutter.hdLoads, gPortStutter.hdLoadTicks * ms,
                        gPortStutter.hdHeaderTicks * ms, gPortStutter.hdDecodeTicks * ms, gPortStutter.hdUploadTicks * ms,
                        gPortStutter.worstLoad ? gPortStutter.worstLoad : "-", gPortStutter.worstLoadTicks * ms,
                        (unsigned)gPortStutter.glyphs, gPortStutter.glyphTicks * ms);
            SDL_memset(&gPortStutter, 0, sizeof(gPortStutter));
            gPortStutter.on = frameLog != NULL;
            logicStart = SDL_GetPerformanceCounter();
        }
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
    LoopMark(PORT_LOOP_FRAMELOG);
    LoopEnd(sFrameCount);
}

/* The network link's lockstep (src/sio/sio.c): while the other player's packet for this link frame is on its way the
   game waits inside the frame; the last picture stays on screen (with the waiting notice) and the window answers. */
void PortLinkIdle(void) {
    static uint64_t nextRefresh;
    uint64_t now = SDL_GetPerformanceCounter();
    /* Poll network/input every millisecond as before. Refreshing the waiting
       image must not block the next NetReceive on a presentation deadline. */
    if (!nextRefresh || now >= nextRefresh || SDL_getenv("KHCOM_FAST")) {
        nextRefresh = now + (uint64_t)(sPerfFreq / GBA_FPS);
        Present(0);
    }
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
    LoopMark(PORT_LOOP_LOGIC);
    PortStateService();
    {
        extern void PortSceneGalleryFrame(void);
        PortSceneGalleryFrame();
    }
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
    LoopMark(PORT_LOOP_LOGIC);
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

#ifdef _WIN32
/* Keep the frame rate steady when the window loses focus or another app is busy: high process priority,
   the 1 ms system timer, no Windows 11 power throttling (EcoQoS slows timers and the CPU of background
   windows), and the "Games" multimedia class for the main thread. Everything is looked up at run time so
   older Windows simply skips what it lacks. */
typedef struct PortPowerThrottling { ULONG Version, ControlMask, StateMask; } PortPowerThrottling;

static void PortWindowsPerformance(void) {
    /* Hidden regression/profiling jobs keep their caller's low priority and
       never join MMCSS Games while the user is playing another game. */
    if (SDL_getenv("KHCOM_BACKGROUND_TEST")) {
        PortLog("Windows: background test keeps caller scheduling");
        return;
    }
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    HMODULE winmm = LoadLibraryA("winmm.dll");
    HMODULE avrt = LoadLibraryA("avrt.dll");

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    if (winmm) {
        UINT(WINAPI * beginPeriod)(UINT) = (UINT(WINAPI*)(UINT))(void*)GetProcAddress(winmm, "timeBeginPeriod");
        if (beginPeriod)
            beginPeriod(1);
    }
    if (kernel) {
        BOOL(WINAPI * setInfo)(HANDLE, int, LPVOID, DWORD) =
            (BOOL(WINAPI*)(HANDLE, int, LPVOID, DWORD))(void*)GetProcAddress(kernel, "SetProcessInformation");
        if (setInfo) {
            /* ProcessPowerThrottling = 4; execution speed (1) and timer resolution (4) never throttled */
            PortPowerThrottling state = { 1, 0x1 | 0x4, 0 };
            setInfo(GetCurrentProcess(), 4, &state, sizeof(state));
        }
    }
    if (avrt) {
        HANDLE(WINAPI * setTask)(LPCWSTR, LPDWORD) =
            (HANDLE(WINAPI*)(LPCWSTR, LPDWORD))(void*)GetProcAddress(avrt, "AvSetMmThreadCharacteristicsW");
        DWORD index = 0;
        if (setTask)
            setTask(L"Games", &index);
    }
    PortLog("Windows: high priority, 1 ms timer, no power throttling");
}
#endif

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

    {
        extern int PortScenePosterConfigure(int argc, char** argv);
        if (PortScenePosterConfigure(argc, argv) < 0) return 2;
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
#if defined(PORT_SDL_BRIDGE) || defined(__EMSCRIPTEN__)
    const bool sdlReady = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);
#else
    const bool sdlReady = PortSdlInit(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS, PortLog);
#endif
    if (!sdlReady)
        PortFatal("SDL_Init failed: %s", SDL_GetError());
#if defined(__ANDROID__) && !defined(PORT_ANDROID64)
    if (!PortAndroidLifecycleStart())
        PortLog("Android lifecycle watch: %s", SDL_GetError());
#endif
    PortConfigLoad();
#ifdef _WIN32
    PortWindowsPerformance();
#endif
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
#ifdef PORT_LINUX
    sRenderer = PortFbCreateRenderer();
#endif
    if (sRenderer == NULL)
        sRenderer = SDL_CreateRenderer(sWindow, NULL);
    if (sRenderer == NULL)
        PortFatal("Could not create the renderer: %s", SDL_GetError());
#if defined(__ANDROID__) && !defined(PORT_ANDROID64)
    extern void HdMovieAndroidSetRendererResetCallback(void (*callback)(void));
    HdMovieAndroidSetRendererResetCallback(NativeMovieRendererReset);
#endif
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
    PortLoopPhasesInit(&sLoop, SDL_getenv("KHCOM_LOOP_PHASES"), sPerfFreq);
    if (sLoop.threshold)
        PortLog("loop phases: reporting frames over %.2f ms", sLoop.threshold * sLoop.msPerTick);
    sFpsTime = SDL_GetPerformanceCounter();

    if (!PortRomLoad()) {
#if defined(__ANDROID__) && !defined(PORT_ANDROID64)
        PortAndroidLifecycleStop();
#endif
        return 1;
    }
    PortUndubApply();
    PortSramLoad();
    PortInputInit();
    PortAudioInit();
    PortUiInit();

    /* The first Options frame must not decode its poster or register/rasterize
       fonts. Do this once before the game starts, using the active output. */
    if (gPortConfig.hdTitle) {
        extern void PortHdTextWarmUi(SDL_Renderer*, float);
        int w = 0, h = 0;
        SDL_FRect at;
        uint64_t start = SDL_GetPerformanceCounter();
        SDL_GetCurrentRenderOutputSize(sRenderer, &w, &h);
        SDL_Rect area = ContentArea(w, h);
        UpdateResolution();
        OutputRect(area.w, area.h, &at);
        float uiScale = at.w / gPortScreen.width;
        if (gPortConfig.viewMode == PORT_VIEW_AUTO) {
            float fit = PortConfigMenuScaleF(area.w, area.h);
            int menuW = SDL_max((int)SDL_ceilf(area.w / fit), GBA_SCREEN_W);
            uiScale = SDL_roundf(menuW * fit) / menuW;
        }
        PortHdMenuAssetsWarm(sRenderer);
        PortHdTextWarmUi(sRenderer, uiScale);
        PortHdTextWarmUi(sRenderer, MenuScale(area.w, area.h));
        PortLog("HD menu: prepared before gameplay in %.1f ms",
                (SDL_GetPerformanceCounter() - start) * 1000.0 / SDL_GetPerformanceFrequency());
    }
    if (gPortConfig.hdText) {
        /* gameplay's HD text too, at the world canvas' scale (port_hdtext_warmup.inc) */
        extern void PortHdTextWarmGame(SDL_Renderer*, float, float);
        int w = 0, h = 0;
        SDL_FRect at;
        uint64_t start = SDL_GetPerformanceCounter();
        SDL_GetCurrentRenderOutputSize(sRenderer, &w, &h);
        SDL_Rect area = ContentArea(w, h);
        UpdateResolution();
        OutputRect(area.w, area.h, &at);
        PortHdTextWarmGame(sRenderer, at.w / gPortScreen.width, MenuScale(area.w, area.h));
        PortLog("HD text: gameplay glyphs prepared in %.1f ms",
                (SDL_GetPerformanceCounter() - start) * 1000.0 / SDL_GetPerformanceFrequency());
    }

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
