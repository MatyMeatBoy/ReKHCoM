#ifndef GUARD_PORT_PORT_H
#define GUARD_PORT_PORT_H

/*
 * Kingdom Hearts: Chain of Memories - native PC platform layer.
 *
 * The decompiled game code runs unmodified in a 32-bit process. The GBA
 * address space (work RAM, I/O, palette, VRAM, OAM, cartridge ROM and SRAM)
 * is mapped at its original addresses, so hardware registers and ROM pointers
 * keep working; this layer stands in for the BIOS, DMA, the PPU, the sound
 * hardware and the interrupt controller.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "port/port_zoom.h"

#include "../../../include/gba/gba_space.h"

#define GBA_EWRAM GBA_EWRAM_BASE
#define GBA_EWRAM_SIZE 0x40000u
#define GBA_IWRAM GBA_IWRAM_BASE
#define GBA_IWRAM_SIZE 0x8000u
#define GBA_IO GBA_IO_BASE
#define GBA_IO_SIZE 0x400u
#define GBA_PLTT GBA_PLTT_BASE
#define GBA_PLTT_SIZE 0x400u
#define GBA_VRAM GBA_VRAM_BASE
#define GBA_VRAM_SIZE 0x18000u
#define GBA_OAM GBA_OAM_BASE
#define GBA_OAM_SIZE 0x400u
#define GBA_ROM GBA_ROM_BASE
#define GBA_ROM_SIZE 0x02000000u
#define GBA_SRAM GBA_SRAM_BASE
#define GBA_SRAM_SIZE 0x10000u

/* p lies in the GBA space from work RAM to the end of the ROM (0x02000000-0x09FFFFFF on the bus) */
#ifdef PORT_GBA_RELOC
unsigned int GbaSpaceBusAddress(const void* p);
#define PORT_GBA_MAP_MEMORY(p) (GbaSpaceBusAddress(p) - 0x02000000u < 0x08000000u)
#else
#define PORT_GBA_MAP_MEMORY(p) ((uintptr_t)(p) - 0x02000000u < 0x08000000u)
#endif

/* A test/log switch (KHCOM_...) read once: getenv is slow on Windows (it locks and walks the environment) and some
   switches sit in per-row, per-pixel or audio paths; read there each time it slowed a 32:9 frame to a second. The
   environment never changes while the game runs. A core that races the first read just sees the switch unset.
   getter: the call the site used before (getenv or SDL_getenv: on Android they can differ). */
#define PORT_ENV_ONCE(out, getter, name)           \
    do {                                           \
        static const char* volatile portEnvValue_; \
        static volatile int portEnvRead_;          \
        if (!portEnvRead_) {                       \
            portEnvValue_ = getter(name);          \
            portEnvRead_ = 1;                      \
        }                                          \
        (out) = portEnvValue_;                     \
    } while (0)

#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define GBA_FPS 59.7275

/* Largest internal resolution the renderer supports. */
#define PORT_MAX_W 2560
#define PORT_MAX_H 1440

#define IO8(off) (*(volatile uint8_t*)(uintptr_t)(GBA_IO + (off)))
#define IO16(off) (*(volatile uint16_t*)(uintptr_t)(GBA_IO + (off)))
#define IO32(off) (*(volatile uint32_t*)(uintptr_t)(GBA_IO + (off)))

/* ---- configuration (port_config.c) ------------------------------------ */

enum PortViewMode {
    PORT_VIEW_ORIGINAL,   /* 240x160, the GBA picture */
    PORT_VIEW_WIDE,       /* 16:9 at the original height */
    PORT_VIEW_EXTENDED,   /* 16:9, more of the world in both directions */
    PORT_VIEW_MAXIMUM,    /* 16:9, as far out as the engine allows */
    PORT_VIEW_AUTO,       /* fill the window at a whole-number pixel scale */
    PORT_VIEW_CRT,        /* 320x240 at an exact scale for CRTs and their scalers (#202): 240p, super-res, 480p */
    PORT_VIEW_COUNT
};

enum PortScaleMode {
    PORT_SCALE_INTEGER,   /* sharpest: largest whole-number multiple */
    PORT_SCALE_FIT,       /* fill the window, keep aspect ratio */
    PORT_SCALE_STRETCH,   /* fill the window */
    PORT_SCALE_COUNT
};

enum PortFilter {
    PORT_FILTER_NEAREST,
    PORT_FILTER_SHARP,    /* nearest at integer scale, then smooth to fit */
    PORT_FILTER_LINEAR,
    PORT_FILTER_COUNT
};

/* Remappable actions (kh_com.ini kb_<name>, kb_<name>_2, pad_<name>). */
enum {
    PORT_ACT_A,
    PORT_ACT_B,
    PORT_ACT_L,
    PORT_ACT_R,
    PORT_ACT_START,
    PORT_ACT_SELECT,
    PORT_ACT_UP,
    PORT_ACT_DOWN,
    PORT_ACT_LEFT,
    PORT_ACT_RIGHT,
    PORT_ACT_STOCK,
    PORT_ACT_DODGE,
    PORT_ACT_UNSTOCK,
    PORT_ACT_COUNT
};

/* pad_<action> values 0..BUTTON_COUNT-1 are buttons; this range encodes an
 * axis and sign so analog triggers can be captured and remapped. */
#define PORT_PAD_AXIS_BASE 100

typedef struct PortConfig {
    int viewMode;
    int zoom;             /* PORT_VIEW_AUTO: 1..6, the view as a multiple of the GBA's (1 Original, 2 Enhanced) */
    int scaleMode;
    int filter;
    int fullscreen;
    int vsync; /* 0 off, 1 VSync, 2 opt-in VRR presentation with VSync fallback */
    int hudSize;          /* wide view: HUD drawn this many pixel-scale steps larger (0 = same) */
    int narrowFit;        /* scenes without a wide view: 1 fill the height, 0 exact pixels */
    int scanlines;        /* 0..100 % intensity */
    int colorCorrection;  /* emulate the GBA LCD colours */
    int frameSkip;        /* 1: a late frame runs the game and the line events but is not drawn (slow devices) */
    int showFps;
    int windowScale;
    int fullscreenWidth;  /* 0 = desktop resolution (borderless) */
    int fullscreenHeight;
    int masterVolume;     /* 0..100 */
    int musicVolume;
    int sfxVolume;
    int dodgeButton;      /* one button for the battle dodge roll */
    int stockButton;      /* one button for L + R (stock a card / sleight) */
    int hdText;           /* dialogue text drawn with an outline font at output resolution */
    int hdFont;           /* its face for menus and dialogue: 0 KHMenu (the default), 1 Comic Hearts, 2 the system's */
    int cardHudRaise;     /* battle: card hand lifted by this many GBA pixels (0..16) */
    int unstockButton;    /* experimental return-stocked-cards control; opt-in, off by default */
    int saveBank;         /* 1..PORT_SAVE_BANKS: which save file holds the game's two slots */
    int hd2d;             /* experimental HD-2D look (on by default in --hd2d builds) */
    int smoothAudio;      /* experimental: 0 off, 1 GBA (cubic interpolation and a gentle low-pass), 2 HD (interpolation only) */
    int ps2Mode;          /* experimental Re:Chain of Memories mode; off: the game exactly as on the cartridge */
    int ps2Difficulty;    /* Re:CoM mode: 0 standard, 1 beginner, 2 proud */
    int ps2FastText;      /* experimental: event dialogue types one character a frame */
    int hdHud;           /* experimental HD HUD placeholder: enemy HP panel */
    int aspect;          /* picture shape: 0 fills the window, 1 at most 16:9, 2 at most 21:9, 3 force 21:9, 4 force 32:9, 5 exact 4:3, 6 exact GBA 3:2 */
    int integerZoom;     /* 1: select only whole pixel scales (v1.4 numbering) */
    int keys[PORT_ACT_COUNT][2]; /* SDL keycodes, 0 = none */
    int pad[PORT_ACT_COUNT];     /* SDL gamepad button or PORT_PAD_AXIS_BASE+axis*2+sign */
    int cheatUnlockRiku;         /* opt-in title access; never marks Sora cleared */
    int undub;                  /* Japanese voices; applied when loading the ROM */
    int savePointSlotSelect;    /* experimental: choose the save slot at save points */
    int hideTouchControls;      /* hide gameplay panels; keep access to options */
    int cheatOneHit;            /* cheat: enemies fall to one hit */
    int cheatInfiniteHp;        /* cheat: the player loses no HP */
    int cheatSaveStates;        /* cheat: quick states (F5 save, F8 load, F6/F7 slot); off by default, no achievements while on */
    int moogleNotice;           /* experimental, off: show the moogle points picked up and the total */
    int gbaMapLogic;            /* cheats > debug mode: 0 No (wide view bounds), 1 Yes (GBA bounds, pop), 2 No (Improved): GBA bounds with a fade-out */
    int gameLanguage;          /* European game: 0 asks (once), 1..5 English, French, German, Italian, Spanish */
    int invertCardScroll;      /* off by default: reverse L/R and right-stick card scrolling in battle */
    int battleZoom;            /* experimental: 0 follows main zoom, otherwise a battle-only pixel scale */
    int padLayout;             /* face buttons: 0 detect (Nintendo pads swap A/B, X/Y), 1 Xbox positions, 2 Nintendo */
    int discordPresence;       /* on by default: Discord shows the world, floor, character and level (port_discord.c) */
    int hdMovies;              /* HD mod (HD_MOD.md): imported Re:CoM movies replace the cartridge's (on when imported) */
    int hdMusic;               /* HD mod: the imported Re:CoM soundtrack replaces the background music (port_hdmusic.c) */
    int hdScenes;              /* HD mod, full: the imported Re:CoM cutscenes replace the story scenes they match (off) */
    int hdSubtitles;           /* HD mod: the HD movies and scenes show Re:CoM's subtitles in the game's language (on) */
    int hdCards;               /* HD mod, beta: battle cards drawn with Re:CoM's HD art (port_hdsprites.c, off) */
    int hdTitle;               /* HD mod, beta: the title screen's logo drawn with Re:CoM's HD art (port_hdsprites.c, off) */
    int titleBrightnessButton; /* experimental: restore the cartridge's title brightness control outside Original view */
    int graphicsMode;          /* PORT_MODE_*: chosen on the first start (0 asks), each with its presets (PortApplyGraphicsMode) */
    int graphicsModeApplied;   /* the mode whose presets were last applied: the launcher only changes graphics_mode */
    int gbaBorder;             /* GBA classic: the decorative border around the GBA picture (0 none, 1..3 designs) */
    int netDelay;              /* net_delay: the online link's input delay in frames (0 automatic, from the round trip) */
    int netStats;              /* net_stats: during an online link, the round trip, input delay and last rollback on screen */
    char onlineName[25];       /* online_name: the player's name on the lobby's rooms (relay servers; Steam uses its own) */
    int assetsSeen;            /* assets_seen: the PORT_ASSET_* found on the last start (-1 unknown); new ones are offered */
    int touchTheme;            /* touch_theme: the touch buttons' look, 0 Clear, 1 Dark, 2 Color (touch_android.c) */
    int touchPos[7][2];        /* touch_<group>_x/_y: where the player moved each touch group (d-pad, L, R, face buttons,
                                  SELECT, START, menu), thousandths of the layout from its place (touch_android.c's editor) */
    int crtOutput;             /* PORT_VIEW_CRT: 0 native 320x240, 1 super-res 1920x240 (x6), 2 2560x240 (x8), 3 480p (2x); last: the
                                  positional defaults stay as they are */
    int controllerPrompts;     /* experimental display only: 0 Original, 1 Auto, 2 Nintendo, 3 Xbox, 4 PlayStation */
    int hdMediaLanguage;       /* appended: 0 legacy/Auto, 1 user-labelled EN, 2 user-labelled JP; not text/Undub */
} PortConfig;

extern PortConfig gPortConfig;

/* The cartridge version this executable is built for. */
#if defined(VERSION_US)
#define PORT_ROM_CODE "B8CE"
#define PORT_ROM_REGION "USA"
#define PORT_SAVE_NAME "kh_com_us.sav"
#elif defined(VERSION_JP)
#define PORT_ROM_CODE "B8CJ"
#define PORT_ROM_REGION "Japan"
#define PORT_SAVE_NAME "kh_com_jp.sav"
#else
#define PORT_ROM_CODE "B8CP"
#define PORT_ROM_REGION "Europe"
#define PORT_SAVE_NAME "kh_com.sav"
#endif

/* ROM given on the command line (--rom), tried before the usual names. */
extern const char* gPortRomPath;

int PortRikuAvailable(int soraCleared);
int PortEventBackdropPanel(void); /* single authored backdrop panel, or -1 */
int PortEventTiledBackdrop(void); /* Riku scenery eligible for verified tile-band extension, or -1 */
int PortEventBackdropProfile(void); /* current story event, -1 outside events */
void PortConfigLoad(void);
int PortTitleBrightnessAllowed(void);
void PortConfigSetDataPath(const char* path);
void PortConfigSave(void);
/* Graphics modes (first start, launcher, Options > reset to a mode's defaults). */
enum { PORT_MODE_CHOOSE, PORT_MODE_HD, PORT_MODE_GBA_ENHANCED, PORT_MODE_GBA_CLASSIC, PORT_MODE_COUNT };
void PortApplyGraphicsMode(int mode);
/* Files an option needs (PortAssetPresent): imported by the launcher, or the Japanese ROM of the undub. */
enum { PORT_ASSET_HD_CARDS, PORT_ASSET_HD_MUSIC, PORT_ASSET_HD_MOVIES, PORT_ASSET_HD_SCENES, PORT_ASSET_HD_SUBS,
       PORT_ASSET_UNDUB, PORT_ASSET_COUNT };
int PortAssetPresent(int asset);
int PortAssetsPresentMask(void);
int PortSaveSlotSelectEnabled(void);
void PortConfigView(int mode, int* w, int* h);
/* Internal size and pixel scale that fill an outW x outH output (auto view). */
void PortConfigAutoView(int outW, int outH, int battle, int* w, int* h, int* scale);
float PortConfigMenuScaleF(int outW, int outH);

int PortConfigZoomId(int level);
int PortConfigZoomNext(int level, int dir);
int PortConfigBattleZoomNext(int level, int dir);
float PortConfigZoomScaleF(int level); /* pixel scale of a zoom level here */
int PortConfigZoomScale(int level);    /* ...rounded down (HUD, labels) */
int PortConfigZoomExact(int level);    /* the pixel scale is whole */
int PortConfigZoomHeavy(int zoom);     /* the step draws more than full speed allows */
const char* PortConfigPath(const char* name);
void* PortLoadAsset(const char* name, size_t* size);

/* ---- memory (port_memory.c) -------------------------------------------- */

int PortMemoryInit(void);
int PortRomLoad(void);
int PortUndubAvailable(void);
void PortUndubApply(void);
void PortSramLoad(void);
void PortSramSyncIfDirty(void);
/* Save banks: bank 1 is PORT_SAVE_NAME, bank n kh_com[_us|_jp]_bank<n>.sav. */
#define PORT_SAVE_BANKS 5
const char* PortSaveName(void);
/* Writes the current bank, then loads gPortConfig.saveBank. Failure retains SRAM
   and the loaded bank; callers must restore the previous saveBank selection. */
int PortSramSwitchBank(void);
void PortBiosAreaRefresh(void);
/* RetroAchievements flattened GBA RAM/SRAM map (not hardware bus addresses).
 * Returns 1 on success; invalid ranges are zero-filled and return 0. */
int PortRaReadMemory(uint32_t address, uint8_t* buffer, uint32_t size);
void PortAchievementsInit(void);
void PortAchievementsRomLoaded(const uint8_t* rom, size_t size);
void PortAchievementsFrame(void);
void PortAchievementsInvalidate(const char* reason);
void PortAchievementsShutdown(void);
void PortDiscordFrame(void);     /* port_discord.c: Discord Rich Presence, desktop builds */
void PortDiscordShutdown(void);
int PortDiscordAvailable(void);  /* 0 where there is no Discord (Android, web, consoles) */
void PortHdMusicFrame(void);     /* port_hdmusic.c: the imported Re:CoM soundtrack in place of the BGM */
void PortHdMusicShutdown(void);
const char* PortHdMusicTrack(int song); /* the Re:CoM track of a cartridge song, NULL if none */
int PortHdMoviePlay(int movieId);  /* port_hdmovie.c: 1 if the imported HD movie played in place of the cartridge's */
int PortHdMovieResolvePath(const char* name, char* out, size_t size);
void PortHdMovieInvalidatePaths(void);
void PortHdMediaInvalidate(void);
int PortHdMediaVariantAvailable(int language, int component);
void PortAudioMovieHold(int hold); /* port_audio.c: 1 fades the game's sound out for an HD movie, 0 brings it back */
int PortHdScenePlay(int eventId);  /* port_hdmovie.c: 1 if the event's imported Re:CoM scene played (full mod) */
/* badgeIndex: position in the official set (its badges.png atlas); -1 = none */
void PortAchievementToast(const char* title, const char* detail, const char* label, int badgeIndex);
void PortAchievementToastDraw(void* renderer, int width, int height);
void PortAchievementToastShutdown(void);
/* Renderer-thread invalidation preserves queued notifications and CPU previews. */
void PortAchievementToastInvalidate(void);
void PortSceneGalleryPreviewInvalidate(void);
/* Consume native SDL reset events between draws; guest bridge recovery is separate. */
int PortRendererHandleReset(uint32_t eventType);
/* The game's globals: initialised data and zero-initialised data. */
void PortGameSections(char** data, size_t* dataSize, char** bss, size_t* bssSize);

/* ---- system / frame loop (port_main.c) --------------------------------- */

void PortFrame(void);
void PortVideoApplyConfig(void);
/* SDL_SCALEMODE_LINEAR, or the pixel-art scaling on SDL's software renderer (its LINEAR stretch reads past the
   texture in fullscreen, port_main.c) */
int PortSmoothScaleMode(void);
/* port_ps2qol.c: the optional Re:Chain of Memories version of a game. */
int PortRecomActive(void);
void PortRecomSetActive(int active);
int PortRecomRunRank(void); /* the lowest difficulty this game was played on: 0 Beginner, 1 Standard, 2 Proud */
/* Drop the HUD's GPU resources and re-arm lookup after an explicit renderer
   generation change or asset re-import. Call on the renderer thread, outside a draw.
   Upload failure remains cached until this boundary; this does not reset other caches. */
void PortHdHudInvalidate(void);
int PortHdHudEnemy(int visible, int ratio, int layer);
extern int gPortHdHudPart;
int PortPpuEnemyHudRect(int* rect);
uint32_t* PortHdHudExportBuffer(void);
void PortHdHudExportSprite(int w, int h, int part, int ax, int ay);
extern int gPortHudDigit; /* 0 none, 1..10 for digits 0..9 */
typedef struct PortHudDigit {
    int x, y, w, h, code, slot;
    uint32_t color;
} PortHudDigit;
int PortPpuHudDigits(PortHudDigit* out, int max);
int PortPpuHudDigitVisible(int digit, int x, int y);
int PortHdHudLettersReady(void);
int PortPpuEnemyHudLabel(int* rect, uint32_t* color);
int PortPpuEnemyHudBar(int* rect, uint32_t* rows);
int PortPpuEnemyHudBarVisible(int x, int y);
int PortPpuPlayerHudBar(int* rect, uint32_t* rows);
int PortPpuPlayerHudBarVisible(int x, int y);
int PortHdHudPlayerFrameReady(void);
int PortPpuPlayerHudFrame(int* rect);
int PortPpuPlayerHudFullHp(void);
void PortHdHudSpriteBegin(void);
int PortHdHudSpriteRegister(const uint32_t* pixels, int w, int h, int slot, int x, int y, int flip, int alpha);
int PortPpuHudSpriteVisible(int slot, int x, int y);
int PortQolButtons(void);
void PortSetQolButtons(int on);
int PortVideoListModes(int* w, int* h, int max);
void PortRaiseIrq(uint16_t flag);
void PortFatal(const char* fmt, ...);
void PortLog(const char* fmt, ...);
uint32_t PortTicks(void);

/* ---- DMA (port_dma.c) ---------------------------------------------------- */

void PortDmaHBlank(void);
void PortDmaVBlank(void);

/* ---- video (port_ppu.c) -------------------------------------------------- */

typedef struct PortScreen {
    int width;            /* internal resolution */
    int height;
    int originX;          /* where GBA pixel (0,0) lands in the frame */
    int originY;
    int wide;             /* the current scene draws beyond 240x160 */
} PortScreen;

extern PortScreen gPortScreen;
extern uint32_t* gPortFrame;          /* Active canvas capacity, on the heap (PortCanvasAlloc) */
extern uint32_t* gPortMenuBackground;
void PortCanvasAlloc(void);
uint32_t PortMenuHeaderPixel(int y, int phase);
uint32_t PortMenuHeaderPixelAt(int y, int distance); /* follows the header sprite's slide */
uint32_t PortMenuBandPixel(int y, int phase);
uint32_t PortTitleBandPixel(int side, int y);
uint32_t PortTitleFooterPixel(int side, int y); /* OBJ underline only, above the title's HD illustration */
int PortSpotlightActive(void);
uint32_t PortMenuTabPixel(int y, int phase, int side);

void PortPpuSetResolution(int w, int h);
void PortPpuRenderFrame(void);
/* The presented journal header's fade, before the next frame changes palettes. */
void PortPpuJiminyFade(int* dark, int* white);
const uint8_t* PortPpuJiminyPaperMask(void);
const void* PortPpuJiminyBookFrame(void); /* canonical presented body; NULL for detail */
const uint32_t* PortPpuJiminyNameEdges(void); /* left/right,24 presented rows each */
struct PortJiminyPageRow;
const struct PortJiminyPageRow* PortPpuJiminyPageRows(void);
void PortPpuRunFrameEvents(void); /* a skipped frame's line events, nothing drawn */
void PortPpuVBlankLines(void);
/* HUD layer: sprites anchored with PortHudBegin, drawn at their own scale. */
extern int gPortHudScale;   /* output pixels per HUD pixel; 0 = no separate layer */
extern int gPortHudW;
extern int gPortHudH;
/* Renderer-thread transaction before native HUD suppression. */
int PortHudCanvasReady(int width, int height);
void PortHudCanvasRetry(void);
/* Scope all rendering of the frozen HUD with its captured geometry. */
void PortPpuHudFrameBegin(int saved[4]);
void PortPpuHudFrameEnd(const int saved[4]);
int PortPpuHudActive(void);
void PortPpuRenderHud(uint32_t* buf);
/* The HUD sprites in front of HD sprites, after PortPpuRenderHud (NULL: none this frame). */
const uint32_t* PortPpuHudOver(void);
/* What the cartridge draws in front of the HD parts of the game's picture, canvas-sized (NULL: none this frame). */
const uint32_t* PortPpuMainOver(void);
/* The same for the screen layer (menus), gPortOverlayW x 160 (NULL: none this frame). */
const uint32_t* PortPpuScreenOver(void);
/* Full-weight world-card light, separate from HD art. NativeWorld supplies
 * the original illustration/light and rebuilds its foreground on failure. */
const uint32_t* PortPpuScreenAdditive(void);
const uint32_t* PortPpuScreenNativeWorld(void);
const uint32_t* PortPpuMainAdditive(void);
const uint32_t* PortPpuMainNativeWorld(void);
/* HD text: a dialogue glyph the PPU left out of the picture, at its place on
   the world canvas (hud 0) or the HUD canvas (hud 1), with its colours. */
typedef struct PortTextGlyph {
    int x, y;
    uint16_t code;
    uint8_t semi; /* 0 opaque, else the alpha (0..255) of a blended sprite */
    uint8_t style; /* 0 dialogue / menu font, 1 movie subtitles, 2 numbers in cells */
    uint8_t cap, base; /* style 2: digit height and baseline row in the cell (0: 5 and 7) */
    uint8_t clip0, clip1; /* visible columns [clip0, clip1) of the 8-pixel cell; 0,0: all */
    uint8_t coverage; /* partial native occlusion, sampled in this exact frame */
    uint16_t visible[16]; /* rows y-4..y+11, bits x-4..x+11 */
    uint32_t face, shadow;
} PortTextGlyph;
int PortPpuTextGlyphs(PortTextGlyph* out, int max, int hud);
/* Frames presented since start (port_main.c). */
uint32_t PortFrameCount(void);
/* OptiPort's deep view of a slow frame: the one-off costs each frame paid besides its usual work, reset by the
   frame log every frame. HD sprite decodes/uploads (port_hdsprites.c) and HD glyph renders. Only counted while
   KHCOM_FRAMELOG is on (on); otherwise no clock is read. */
typedef struct PortStutterProbe {
    int on;
    uint32_t hdLoads, glyphs;
    uint64_t hdLoadTicks, glyphTicks;
    uint64_t hdHeaderTicks, hdDecodeTicks, hdUploadTicks; /* the loads' three parts, summed */
    uint64_t worstLoadTicks;
    const char* worstLoad; /* the frame's slowest load, not the last one */
} PortStutterProbe;
extern PortStutterProbe gPortStutter;
#define PORT_STUTTER_NOW() (gPortStutter.on ? SDL_GetPerformanceCounter() : 0)
#define PORT_STUTTER_ADD(field, start) \
    do { if (gPortStutter.on) gPortStutter.field += SDL_GetPerformanceCounter() - (start); } while (0)

/* ---- audio (port_audio.c) ------------------------------------------------ */

int PortAudioInit(void);
void PortAudioFrame(void);
void PortAudioShutdown(void);

/* ---- input (port_input.c) ------------------------------------------------ */

void PortInputInit(void);
/* Deferred to the event pump so normal shutdown also closes the desktop launcher. */
void PortRequestQuitAll(void);
void PortInputResetBindings(void); /* clear keys from the old mapping and suppress held pad buttons */
uint16_t PortInputKeys(void);
/* The port's extra buttons, beyond the GBA's ten key bits. */
#define PORT_KEY_STOCK 0x400
#define PORT_KEY_DODGE 0x800
#define PORT_KEY_UNSTOCK 0x1000
/* Turns the extra buttons into GBA keys and requests; returns the GBA key
   bits. Called once per frame. */
uint16_t PortInputResolve(uint16_t keys);
int PortInputKeyBound(int key); /* the keyboard key is bound to a game action */
void PortInputHandleEvent(const void* sdlEvent);
void PortPromptHandleEvent(const void* sdlEvent); /* raw, before Nintendo normalization and menu consumption */
int PortControllerBindingLabel(int binding, char* out, size_t size);
int PortControllerActionLabel(int action, char* out, size_t size);
int PortControllerActionGlyph(int action); /* 0: use the accessible text label */
const unsigned char* PortControllerGlyphTiles(int glyph); /* immutable 16x16 4bpp, no allocation */
int PortControllerPresetFamily(void); /* identified/forced family, 0 if no honest preset is available */
int PortApplyControllerConfirmPreset(int family); /* explicit, pinned UI choice: replaces only gamepad A/B */
int PortControllerOriginalPromptAt(const void* position,int sjis,int* follower); /* scoped source offsets only */
uint64_t PortControllerPromptRevision(void); /* bounded context snapshot, no texture work */
int PortControllerSemanticGlyph(int token); /* action+1, bit64: card-scroll tutorial inversion */

#endif
