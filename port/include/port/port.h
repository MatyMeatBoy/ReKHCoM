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
    int vsync;
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
    int cardHudRaise;     /* battle: card hand lifted by this many GBA pixels (0..16) */
    int unstockButton;    /* experimental return-stocked-cards control; opt-in, off by default */
    int saveBank;         /* 1..PORT_SAVE_BANKS: which save file holds the game's two slots */
    int hd2d;             /* experimental HD-2D look (on by default in --hd2d builds) */
    int smoothAudio;      /* experimental: cubic sample interpolation and a gentle low-pass */
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
    int moogleNotice;           /* experimental, off: show the moogle points picked up and the total */
    int gbaMapLogic;            /* cheats > debug mode, off: field enemies leave at the GBA screen's edges */
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
    int graphicsMode;          /* PORT_MODE_*: chosen on the first start (0 asks), each with its presets (PortApplyGraphicsMode) */
    int graphicsModeApplied;   /* the mode whose presets were last applied: the launcher only changes graphics_mode */
    int gbaBorder;             /* GBA classic: the decorative border around the GBA picture (0 none, 1..3 designs) */
    int netDelay;              /* net_delay: the online link's input delay in frames (0 automatic, from the round trip) */
    int netStats;              /* net_stats: during an online link, the round trip, input delay and last rollback on screen */
    char onlineName[25];       /* online_name: the player's name on the lobby's rooms (relay servers; Steam uses its own) */
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
void PortConfigSetDataPath(const char* path);
void PortConfigSave(void);
/* Graphics modes (first start, launcher, Options > reset to a mode's defaults). */
enum { PORT_MODE_CHOOSE, PORT_MODE_HD, PORT_MODE_GBA_ENHANCED, PORT_MODE_GBA_CLASSIC, PORT_MODE_COUNT };
void PortApplyGraphicsMode(int mode);
int PortSaveSlotSelectEnabled(void);
void PortConfigView(int mode, int* w, int* h);
/* Internal size and pixel scale that fill an outW x outH output (auto view). */
void PortConfigAutoView(int outW, int outH, int battle, int* w, int* h, int* scale);

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
/* Writes the current bank, then loads gPortConfig.saveBank. */
void PortSramSwitchBank(void);
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
void PortAudioMovieHold(int hold); /* port_audio.c: 1 fades the game's sound out for an HD movie, 0 brings it back */
int PortHdScenePlay(int eventId);  /* port_hdmovie.c: 1 if the event's imported Re:CoM scene played (full mod) */
/* badgeIndex: position in the official set (its badges.png atlas); -1 = none */
void PortAchievementToast(const char* title, const char* detail, const char* label, int badgeIndex);
void PortAchievementToastDraw(void* renderer, int width, int height);
void PortAchievementToastShutdown(void);
/* The game's globals: initialised data and zero-initialised data. */
void PortGameSections(char** data, size_t* dataSize, char** bss, size_t* bssSize);

/* ---- system / frame loop (port_main.c) --------------------------------- */

void PortFrame(void);
void PortVideoApplyConfig(void);
/* port_ps2qol.c: the optional Re:Chain of Memories version of a game. */
int PortRecomActive(void);
void PortRecomSetActive(int active);
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
int PortSpotlightActive(void);
uint32_t PortMenuTabPixel(int y, int phase, int side);

void PortPpuSetResolution(int w, int h);
void PortPpuRenderFrame(void);
void PortPpuRunFrameEvents(void); /* a skipped frame's line events, nothing drawn */
void PortPpuVBlankLines(void);
/* HUD layer: sprites anchored with PortHudBegin, drawn at their own scale. */
extern int gPortHudScale;   /* output pixels per HUD pixel; 0 = no separate layer */
extern int gPortHudW;
extern int gPortHudH;
int PortPpuHudActive(void);
void PortPpuRenderHud(uint32_t* buf);
/* The HUD sprites in front of HD sprites, after PortPpuRenderHud (NULL: none this frame). */
const uint32_t* PortPpuHudOver(void);
/* What the cartridge draws in front of the HD parts of the game's picture, canvas-sized (NULL: none this frame). */
const uint32_t* PortPpuMainOver(void);
/* The same for the screen layer (menus), gPortOverlayW x 160 (NULL: none this frame). */
const uint32_t* PortPpuScreenOver(void);
/* HD text: a dialogue glyph the PPU left out of the picture, at its place on
   the world canvas (hud 0) or the HUD canvas (hud 1), with its colours. */
typedef struct PortTextGlyph {
    int x, y;
    uint16_t code;
    uint8_t semi; /* 0 opaque, else the alpha (0..255) of a blended sprite */
    uint8_t style; /* 0 dialogue / menu font, 1 movie subtitles, 2 numbers in cells */
    uint8_t cap, base; /* style 2: digit height and baseline row in the cell (0: 5 and 7) */
    uint8_t clip0, clip1; /* visible columns [clip0, clip1) of the 8-pixel cell; 0,0: all */
    uint32_t face, shadow;
} PortTextGlyph;
int PortPpuTextGlyphs(PortTextGlyph* out, int max, int hud);
/* Frames presented since start (port_main.c). */
uint32_t PortFrameCount(void);

/* ---- audio (port_audio.c) ------------------------------------------------ */

int PortAudioInit(void);
void PortAudioFrame(void);
void PortAudioShutdown(void);

/* ---- input (port_input.c) ------------------------------------------------ */

void PortInputInit(void);
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

#endif
