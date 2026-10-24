/*
 * Software GBA PPU.
 *
 * Renders the picture described by the I/O registers, palette, VRAM and OAM
 * into a 32-bit frame buffer. The output can be larger than the GBA's
 * 240x160: GBA pixel (0,0) is placed at (originX, originY) and background
 * layers, sprites and full-screen windows continue past the original edges
 * when the current scene supports a wide view (gPortScreen.wide).
 *
 * Per-line state changes (HBlank interrupts and HBlank DMA) are honoured for
 * the 160 original lines; lines added above and below reuse the state of the
 * nearest original line.
 */
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "port/port.h"
#include "port/port_hd2d.h"
#include "port/port_hdsprites.h"
#include "engine.h"
#include "sprite.h"
#include "port/port_state.h"
#include "port/port_battle_backdrop.h"
#include "port/port_scene057_tiles.h"
#include "port/port_scene061_tiles.h"
#include "port/port_scene074_tiles.h"
#include "port/port_agrabah_tiles.h"
#include "port/port_agrabah109_tiles.h"
#include "port/port_preliminar_v1_tiles.h"

extern const void* gMsgwinMapBlocks[4];

/* PORT_PPU_THREADS (the desktop builds, configure_pc.py): the canvas rows above and below the GBA picture are drawn
   on several cores (RenderOuterRows, below). Each core then has its own line buffers: PPU_TLS marks them. */
#ifdef PORT_PPU_THREADS
#include <SDL3/SDL.h>
#define PPU_TLS _Thread_local
/* While the GBA picture's rows are drawn after the frame's line events (PpuInnerKeep, below), each row reads the
   registers, palettes and OAM its line had: these views point at that row's copy (NULL: the live memory). */
static PPU_TLS const uint8_t* sPpuIoView;
static PPU_TLS const uint16_t* sPpuPlttView;
static PPU_TLS const uint16_t* sPpuOamView;
#define PPU_IO_KEPT 0x60 /* the display registers: DISPCNT to the blend registers */
#undef IO16
#undef IO32
#define IO16(off) (*(volatile uint16_t*)((off) < PPU_IO_KEPT && sPpuIoView ? (uintptr_t)sPpuIoView + (off) \
                                                                         : (uintptr_t)(GBA_IO + (off))))
#define IO32(off) (*(volatile uint32_t*)((off) < PPU_IO_KEPT && sPpuIoView ? (uintptr_t)sPpuIoView + (off) \
                                                                         : (uintptr_t)(GBA_IO + (off))))
#define PPU_PLTT_BASE (sPpuPlttView ? sPpuPlttView : (const uint16_t*)(uintptr_t)GBA_PLTT)
#define PPU_OAM_BASE (sPpuOamView ? sPpuOamView : (const uint16_t*)(uintptr_t)GBA_OAM)
#else
#define PPU_TLS
#define PPU_PLTT_BASE ((const uint16_t*)(uintptr_t)GBA_PLTT)
#define PPU_OAM_BASE ((const uint16_t*)(uintptr_t)GBA_OAM)
#endif

PortScreen gPortScreen = { GBA_SCREEN_W, GBA_SCREEN_H, 0, 0, 0 };

/* Area the game should keep sprites alive in (see VIEW_* in gba/defines.h). */
int gPortViewLeft = 0;
int gPortViewTop = 0;
int gPortViewRight = GBA_SCREEN_W;
int gPortViewBottom = GBA_SCREEN_H;

/* Full-range sprite positions written alongside OAM by UpdateSpriteOam. */
typedef struct OamExt {
    int16_t x;
    int16_t y;
} OamExt;
static OamExt sOamExt[128];

/* Map-block backgrounds as they were when the scroll registers were last
   committed: lets the wide view draw the world beyond the VRAM window. */
typedef struct BgLatch {
    void* const* map;
    int width;
    int height;
    int x;
    int y;
    uint16_t hofs;
    uint16_t vofs;
    int valid;
} BgLatch;
static BgLatch sBgLatch[4];
static int sSinglePanelBg = -1;

/* Display offsets requested by the game for docking UI (see engine.h). */
int gPortSpriteShiftX;
typedef struct BgWorld {
    PortWorldTileFunc fn;
    int x;
    int y;
    uint16_t hofs;
    uint16_t vofs;
    int valid;
} BgWorld;
static BgWorld sBgWorld[4];
static BgWorld sBgWorldLatched[4];
static int sBgIso[4];
static int sBgIsoLatched[4];
static int sEventBackdropProfile = -1; /* derived at frame start, not saved */
static int sUseSceneProfiles;
static int sPreliminarV1;
static int sPreliminarApprovedOnly; /* only the Preliminar V1 scenes approved with honours, for everyone */
/* Preliminar V1 scenes approved with honours in the review app; KHCOM_SCENEnnn_WIDE=0 turns one off */
static const int kPreliminarApproved[] = {53, 54, 55, 121, 122};
static int sPreliminarApprovedOn[sizeof(kPreliminarApproved)/sizeof(kPreliminarApproved[0])];
/* harness switches, read once per frame on the main thread: getenv in the per-row/per-pixel drawing is slow on
   Windows (a frame a second at 32:9) and not safe to call while the cores draw */
static int sEnvNoGuardPatio, sEnvTitleBandsOff, sEnvActorFrameOff, sEnvHudLayoutFrame = -1;
/* Approved wide art is on; its variable set to 0 turns it off for before/after tests (WORKFLOW_ARTE_AMPLIO.md). */
static int PortArtOn(const char* var) {
    const char* v = getenv(var);
    return v == NULL || atoi(v) != 0;
}
static const PortPreliminarRecipe* sPreliminarRecipe[4];
static int sAgrabahLayer[4]; /* rebuilt from actual map data each frame */
static int sAgrabah109Layer[4];
static int sDarksideLayer[4]; /* port_darkside.c: 1 beach, 2 body; from the map blocks each frame */
static int sAgrabah109ShopPal[2]; /* live OBJ palette slots, rebuilt per frame */
extern const uint8_t gMapGmkAgrabah00Tiles[], gMapGmkAgrabah09Tiles[];
extern const uint16_t gMapGmkAgrabah00Palette[], gMapGmkAgrabah01Palette[];

/* Scenes reviewed with the continued floor. Many maps end in an authored
   dark void past their walls, which must stay; only listed scenes extend.
   KHCOM_ISOALL=1 (harness) tries it on every scene for review. */
static const int kIsoScenes[] = {
    4, /* 1F Traverse Town: Sora meets Leon */
};

int PortIsoScene(int eventId) {
    size_t i;
    if (getenv("KHCOM_ISOALL"))
        return 1;
    for (i = 0; i < sizeof(kIsoScenes) / sizeof(kIsoScenes[0]); i++)
        if (kIsoScenes[i] == eventId)
            return 1;
    return 0;
}

void PortSetBgIsoExtend(int bg, int enabled) {
    static int disabled = -1;
    if (disabled < 0)
        disabled = getenv("KHCOM_NOISO") != NULL; /* harness: the scene without it */
    if (disabled)
        enabled = 0;
    if (bg >= 0 && bg < 4)
        sBgIso[bg] = enabled != 0;
}

void PortSetBgWorld(int bg, PortWorldTileFunc fn, int originX, int originY) {
    if (bg >= 0 && bg < 4) {
        sBgWorld[bg].fn = fn;
        sBgWorld[bg].x = originX;
        sBgWorld[bg].y = originY;
    }
}

static int sBgShift[4];
static int sBgShiftLatched[4];
static int sBgDocked[4];
static int sBgDockedLatched[4];
static int sBattleView;
static int sArenaBands[6]; /* bounds, left source/width, right source/width */
typedef struct BgTextTile {
    uint16_t code;
    uint8_t style, faceIndex, shadowIndex, panelIndex;
    uint8_t cap, base; /* rows of the digit's ink: height and baseline */
    uint8_t source[32];
} BgTextTile;
static BgTextTile sBgTextTiles[2048];
/* The registration a tile had before its latest: the game requests the new digit's copy by DMA, done at the next
   vertical blank, and registers it at once; until the copy lands the tile still holds the previous digit, which
   this one matches (the deck editor's PN digits showed their pixel font for that frame: a blink, 23/10). */
static BgTextTile sBgTextTilesPrev[2048];

static const BgTextTile* BgTextTileAt(unsigned offset);
typedef struct BgTextGlyph {
    PortTextGlyph glyph;
    uint8_t bg, priority, screen;
} BgTextGlyph;
static BgTextGlyph sBgTextGlyphs[1024];
static int sBgTextCount;
#ifdef PORT_PPU_THREADS
/* while RenderOuterRows runs, each core collects its rows' glyphs here (tagged with the row) and the rows are merged
   in order afterwards, so the list is the one a single core makes */
typedef struct PendingGlyph {
    int row;
    BgTextGlyph g;
} PendingGlyph;
static PPU_TLS PendingGlyph* sGlyphPend;
static PPU_TLS int sGlyphPendCount, sGlyphRow;
#endif

static int BgTextRoom(void) {
#ifdef PORT_PPU_THREADS
    if (sGlyphPend)
        return sGlyphPendCount < 1024;
#endif
    return sBgTextCount < 1024;
}

static BgTextGlyph* BgTextNew(void) {
#ifdef PORT_PPU_THREADS
    if (sGlyphPend) {
        sGlyphPend[sGlyphPendCount].row = sGlyphRow;
        return &sGlyphPend[sGlyphPendCount++].g;
    }
#endif
    return &sBgTextGlyphs[sBgTextCount++];
}

void PortClearBgTextTiles(void) {
    memset(sBgTextTiles, 0, sizeof(sBgTextTiles));
    memset(sBgTextTilesPrev, 0, sizeof(sBgTextTilesPrev));
}

void PortRegisterBgTextTile(void* address, int code, int style, const void* source32) {
    uintptr_t offset = (uintptr_t)address - GBA_VRAM;
    BgTextTile* tile;
    if (offset >= 0x10000 || (offset & 31) || !source32) return;
    tile = &sBgTextTiles[offset / 32];
    if (tile->code && memcmp(tile->source, source32, 32))
        sBgTextTilesPrev[offset / 32] = *tile;
    tile->code = (uint16_t)code;
    tile->style = (uint8_t)style;
    tile->faceIndex = (uint8_t)((style & 0xFF00) ? (style >> 8) & 15 : 15);
    tile->shadowIndex = (uint8_t)((style & 0xFF00) ? (style >> 12) & 15 : 14);
    tile->panelIndex = (uint8_t)((style & 0xFF00) ? (style >> 16) & 15 : 13);
    if (!(style & 0xFF00)) {
        /* The tile's own colours: the commonest index is the panel behind the
           digit, the next ones its face and shadow. Greyed-out panels use
           other indices than the active ones, so a fixed choice would not
           match what the cartridge shows. */
        int count[16] = { 0 }, order[3] = { -1, -1, -1 }, i, k;
        for (i = 0; i < 32; i++) {
            count[((const uint8_t*)source32)[i] & 15]++;
            count[((const uint8_t*)source32)[i] >> 4]++;
        }
        for (k = 0; k < 3; k++)
            for (i = 1; i < 16; i++)
                if (count[i] && i != order[0] && i != order[1] && (order[k] < 0 || count[i] > count[order[k]]))
                    order[k] = i;
        if (order[1] >= 0) {
            tile->panelIndex = (uint8_t)order[0];
            tile->faceIndex = (uint8_t)order[1];
            tile->shadowIndex = (uint8_t)(order[2] >= 0 ? order[2] : order[0]);
        }
    }
    {
        /* Measure the selected ink, including callers with explicit palette
           indices. A dense digit can have more shadow than panel pixels;
           frequency is not a safe way to identify those numeric families. */
        int top = 8, bottom = -1, y, xx;
        for (y = 0; y < 8; y++) for (xx = 0; xx < 8; xx++) {
            uint8_t v = ((const uint8_t*)source32)[y * 4 + xx / 2];
            int c = (xx & 1) ? v >> 4 : v & 15;
            if (c && c != tile->panelIndex &&
                (c == tile->faceIndex || c == tile->shadowIndex)) {
                if (y < top) top = y;
                if (y > bottom) bottom = y;
            }
        }
        tile->cap = bottom > top ? (uint8_t)(bottom - top) : 0;
        tile->base = bottom > top ? (uint8_t)bottom : 0;
    }
    memcpy(tile->source, source32, 32);
}

/* A docked layer is screen UI moved toward an edge: it shows only what the
   hardware would (no world-map continuation), so parked windows stay hidden. */
static unsigned sScreenBgMask;
static int sBgSplit[4]; /* PortBgSplit, below */
static int sBgSplitBands[4]; /* repeat authored ornamental tiles in the split gap */
static int sBgClip[4]; /* PortBgClip: UI layer kept to the GBA columns */
typedef struct MenuBandSource {
    const uint16_t* frame;
    const ObjTiles* tiles;
    const ObjPalette* palette;
    int y;
} MenuBandSource;
/* The menu assets the margins carry on (header, bands, tabs). The game sets
   them while it draws a frame, but that frame's sprites reach the screen
   only after the next VBlank: the margins use the values latched with them
   (PortMenuLatch), so they never run a frame ahead of the picture (the
   extensions blinked while a menu slid in or out). */
typedef struct MenuSources {
    const uint16_t* headerFrame;
    const ObjTiles* headerTiles;
    const ObjPalette* headerPalette;
    MenuBandSource bands[2];
    const uint16_t* tabFrame;
    const ObjTiles* tabTiles;
    const ObjPalette* tabPalette;
    int tabBg[2], tabX[2], tabY[2];
    int headerX, headerDrawn;
} MenuSources;
static MenuSources sMenuShown = { .tabBg = { -1, -1 } }, sMenuPend = { .tabBg = { -1, -1 } };
/* Sample BG tab colours with the visible picture, before VBlank DMA updates
 * its map/palette for the next frame. Presentation must not read live VRAM. */
static uint32_t sMenuTabPixels[2][16];
static void CaptureMenuTabPixels(void);
#define sMenuHeaderFrame sMenuShown.headerFrame
#define sMenuHeaderTiles sMenuShown.headerTiles
#define sMenuHeaderPalette sMenuShown.headerPalette
#define sMenuBands sMenuShown.bands
#define sMenuTabFrame sMenuShown.tabFrame
#define sMenuTabTiles sMenuShown.tabTiles
#define sMenuTabPalette sMenuShown.tabPalette
#define sMenuTabBg sMenuShown.tabBg
#define sMenuTabX sMenuShown.tabX
#define sMenuTabY sMenuShown.tabY

/* VBlank: the sprites drawn this frame go on screen; so do their sources */
static uint32_t sMenuModeStart, sMenuHeaderMoved;
void PortMenuLatch(void) {
    if (sMenuPend.headerX != sMenuShown.headerX || sMenuPend.headerDrawn != sMenuShown.headerDrawn)
        sMenuHeaderMoved = PortFrameCount();
    sMenuShown = sMenuPend;
    sMenuPend.headerDrawn = 0;
}

/* a menu sliding in (its first frames) or out (its header moving) */
int PortMenuSliding(void) {
    uint32_t now = PortFrameCount();
    return now - sMenuModeStart < 45 || now - sMenuHeaderMoved < 8;
}
static int sSceneNarrow, sNarrowWanted, sNarrowWait; /* PortSceneNarrow, below */
static int sSpotlight;
static int sTitleArtFade, sTitleBands;
static uint32_t sTitleBand[2][GBA_SCREEN_H];
void PortTitleBands(int enabled) { sTitleBands = enabled != 0; }
unsigned int PortTitleBandPixel(int side, int y) {
    return sTitleBands && y >= 0 && y < GBA_SCREEN_H ? sTitleBand[side > 0][y] : 0;
}
static uint16_t sTitlePalette[256], sTitleFadeColors[49][256];
static uint8_t sTitleFadeReady[49][256];
static int sTitlePaletteValid;
void PortTitleArtFade(int enabled) {
    sTitleArtFade = enabled != 0;
    sTitlePaletteValid = 0;
}
/* The first illustration (the developer credit over Sora) is on BG0 until the press-start bar takes that layer: its
   right and bottom edges fade as the title's (BG1) do. */
static int sTitleArtFadeBg0;
void PortTitleArtFadeFirst(int enabled) {
    sTitleArtFadeBg0 = enabled != 0;
}
void PortSetSpotlight(int active) { sSpotlight = active; }
int PortSpotlightActive(void) { return sSpotlight; }
void PortPpuVisitState(PortStateVisitor visit) {
    visit(sBgLatch, sizeof(sBgLatch));
    visit(sBgShift, sizeof(sBgShift));
    visit(sBgShiftLatched, sizeof(sBgShiftLatched));
    visit(sBgDocked, sizeof(sBgDocked));
    visit(sBgDockedLatched, sizeof(sBgDockedLatched));
    visit(&sBattleView, sizeof(sBattleView));
    visit(sArenaBands, sizeof(sArenaBands));
    visit(sBgTextTiles, sizeof(sBgTextTiles));
    visit(sOamExt, sizeof(sOamExt));
    visit(sBgWorld, sizeof(sBgWorld));
    visit(sBgWorldLatched, sizeof(sBgWorldLatched));
    visit(sBgIso, sizeof(sBgIso));
    visit(sBgIsoLatched, sizeof(sBgIsoLatched));
    visit(sBgSplit, sizeof(sBgSplit));
    visit(&sScreenBgMask, sizeof(sScreenBgMask));
    visit(sBgSplitBands, sizeof(sBgSplitBands));
    visit(sBgClip, sizeof(sBgClip));
    visit(&sSceneNarrow, sizeof(sSceneNarrow));
    visit(&sNarrowWanted, sizeof(sNarrowWanted));
    visit(&sNarrowWait, sizeof(sNarrowWait));
    visit(&sSpotlight, sizeof(sSpotlight));
    visit(&sTitleArtFade, sizeof(sTitleArtFade));
    visit(&sTitleBands, sizeof(sTitleBands));
    visit(&sMenuShown, sizeof(sMenuShown));
    visit(&sMenuPend, sizeof(sMenuPend));
}

void PortSetBgShiftX(int bg, int shift) {
    if (bg >= 0 && bg < 4) {
        sBgShift[bg] = shift;
        sBgDocked[bg] = 0;
    }
}

void PortDockBg(int bg, int shift) {
    if (bg >= 0 && bg < 4) {
        sBgShift[bg] = shift;
        sBgDocked[bg] = 1;
    }
}

int PortWideMarginX(void) {
    return gPortScreen.wide ? gPortScreen.originX : 0;
}

int gPortSpriteShiftY;

int PortWideMarginY(void) {
    return gPortScreen.wide ? gPortScreen.originY : 0;
}

/* Anchors nest (a HUD task may draw child tasks), so keep a small stack. */
static int sHudStack[16][5];
static int sHudDepth;
static int sHudAx, sHudAy;
static int sHudAyQ; /* ay 3: vertical anchor in 1/256ths, -256 top .. 256 bottom */

void PortHudBegin(int ax, int ay) {
    if (sHudDepth < 16) {
        sHudStack[sHudDepth][0] = gPortSpriteShiftX;
        sHudStack[sHudDepth][1] = gPortSpriteShiftY;
        sHudStack[sHudDepth][2] = sHudAx;
        sHudStack[sHudDepth][3] = sHudAy;
        sHudStack[sHudDepth][4] = sHudAyQ;
    }
    sHudDepth++;
    sHudAx = ax;
    sHudAy = ay;
    sHudAyQ = ay * 256;
    /* ax 2: the HUD's scale, but placed where the world puts it (the
       enemy's card follows its enemy across the arena) */
    gPortSpriteShiftX = (ax == 2 ? 0 : ax) * PortWideMarginX();
    gPortSpriteShiftY = ay * PortWideMarginY();
}

int gPortWorldScale; /* the world's pixel scale (presenter), for ax 2 */

/* x on the HUD canvas of a world-placed sprite (ax 2): its centre stays
   over the same window point as in the world picture */
static int HudWorldX(int ox, int bw, int w) {
    int hs = gPortHudScale > 0 ? gPortHudScale : 1, ws = gPortWorldScale > 0 ? gPortWorldScale : hs;
    int centre = ox + bw / 2 + gPortScreen.originX - gPortScreen.width / 2;
    return w / 2 + centre * ws / hs - bw / 2;
}

void PortHudEnd(void) {
    if (sHudDepth > 0)
        sHudDepth--;
    if (sHudDepth < 16) {
        gPortSpriteShiftX = sHudStack[sHudDepth][0];
        gPortSpriteShiftY = sHudStack[sHudDepth][1];
        sHudAx = sHudStack[sHudDepth][2];
        sHudAy = sHudStack[sHudDepth][3];
        sHudAyQ = sHudStack[sHudDepth][4];
    }
}

/* Cards that travel between the deck (bottom) and the stock under the HP
   gauge (top): each sprite's vertical anchor follows its own height, so it
   ends docked to whichever edge it reaches without jumping on the way. */
void PortHudBeginFollowY(int ax) {
    PortHudBegin(ax, 0);
    sHudAy = 3;
    sHudAyQ = 0;
    gPortSpriteShiftY = 0;
}


/* Pointers/highlights that refer to HUD elements: anchor by screen zone,
   matching the corners the HUD itself is docked to. */
void PortHudBeginAt(int x, int y) {
    int ax = x < 80 ? -1 : (x >= 160 ? 1 : 0);
    int ay = y < 54 ? -1 : (y >= 80 ? 1 : 0);

    PortHudBegin(ax, ay);
}

/* Which sprite entries / OAM slots belong to the HUD, and their anchor. */
typedef struct SpriteTag {
    uint8_t hud;
    int8_t ax;
    int8_t ay;
    uint16_t text; /* HD text: pixel-font character (Shift-JIS in Japanese), 0 for other sprites */
    uint8_t style; /* HD text font: 0 dialogue / menu, 1 movie subtitles */
    uint8_t screen; /* drawn by a menu: goes on the screen layer */
    uint8_t header; /* a menu header (DrawMenuHeader) */
    uint8_t hdHudPart; /* HD enemy panel: frame, cap, live bar */
    uint8_t digit;
    int16_t ayq; /* ay 3 only */
    int16_t clipX0, clipX1; /* x0==x1: no clip; coordinates include docking shift */
} SpriteTag;
static SpriteTag sEntryTag[128];
static SpriteTag sOamTag[128];
static int sSpriteClipX0, sSpriteClipX1;

void PortSpriteClipX(int x0, int x1) {
    sSpriteClipX0 = x0;
    sSpriteClipX1 = x1;
}

static int FollowQ(int y) {
    return y >= 96 ? 256 : y <= 40 ? -256 : -256 + (y - 40) * 512 / 56;
}

/* The vertical shift of a sprite drawn now (engine.c, sprite.c). */
int PortSpriteShiftY(int entry, int y) {
    int q;
    if (sHudDepth == 0 || sHudAy != 3)
        return gPortSpriteShiftY;
    q = FollowQ(y);
    if (entry >= 0 && entry < 128)
        sEntryTag[entry].ayq = (int16_t)q;
    return q * PortWideMarginY() / 256;
}

/* The tag's vertical shift on the world canvas, and its place on the HUD. */
static int TagShiftY(const SpriteTag* t) {
    return t->ay == 3 ? t->ayq * gPortScreen.originY / 256 : t->ay * gPortScreen.originY;
}
static int TagHudY(const SpriteTag* t, int yg, int h, int raise) {
    int q;
    if (t->ay != 3)
        return t->ay < 0 ? yg : (t->ay > 0 ? h - (GBA_SCREEN_H - yg) - raise : yg + (h - GBA_SCREEN_H) / 2);
    q = t->ayq + 256; /* 0 top .. 512 bottom */
    return yg + (h - GBA_SCREEN_H) * q / 512 - raise * q / 512;
}

static int sHudLayer;
static int sHdText;
int gPortHdHudPart;
int gPortHudDigit;
static PortHudDigit* sHudDigits;
static int sHudDigitCount;
static uint8_t* sHudDigitOwners;
static size_t sHudDigitOwnerSize;
static uint8_t* sEnemyBarMask;
static size_t sEnemyBarMaskSize;
static int sEnemyBarRect[4], sEnemyBarSlot;
static uint32_t sEnemyBarRows[32];
static uint8_t* sPlayerBarMask;
static size_t sPlayerBarMaskSize;
static int sPlayerBarRect[4], sPlayerBarSlot;
static uint32_t sPlayerBarRows[32];
static int sPlayerFullHp;
int PortPpuPlayerHudFullHp(void) { return sPlayerFullHp; }
static int sPlayerFrameRect[4];
int PortPpuPlayerHudFrame(int* rect) {
    if (sPlayerFrameRect[2] < sPlayerFrameRect[0]) return 0;
    memcpy(rect, sPlayerFrameRect, sizeof(sPlayerFrameRect));
    return 1;
}
int PortPpuPlayerHudBar(int* rect, uint32_t* rows) {
    if (sPlayerBarRect[2] < sPlayerBarRect[0]) return 0;
    memcpy(rect, sPlayerBarRect, sizeof(sPlayerBarRect));
    memcpy(rows, sPlayerBarRows, sizeof(sPlayerBarRows));
    return 1;
}
int PortPpuPlayerHudBarVisible(int x, int y) {
    int front;
    if (!sPlayerBarMask || !sHudDigitOwners || x < 0 || y < 0 || x >= gPortHudW || y >= 32) return 0;
    front = sHudDigitOwners[y * gPortHudW + x];
    return sPlayerBarMask[y * gPortHudW + x] && (front == 0 || front >= sPlayerBarSlot + 1);
}
int PortPpuEnemyHudBar(int* rect, uint32_t* rows) {
    if (sEnemyBarRect[2] < sEnemyBarRect[0]) return 0;
    memcpy(rect, sEnemyBarRect, sizeof(sEnemyBarRect));
    memcpy(rows, sEnemyBarRows, sizeof(sEnemyBarRows));
    return 1;
}
int PortPpuEnemyHudBarVisible(int x, int y) {
    int at = y * gPortHudW + x, front;
    if (!sEnemyBarMask || !sHudDigitOwners || x < 0 || y < 0 || x >= gPortHudW || y >= gPortHudH) return 0;
    front = sHudDigitOwners[at];
    return sEnemyBarMask[at] && (front == 0 || front >= sEnemyBarSlot + 1);
}
int PortPpuHudDigits(PortHudDigit* out, int max) {
    int n = sHudDigitCount < max ? sHudDigitCount : max;
    if (n) memcpy(out, sHudDigits, n * sizeof(*out));
    return n;
}
int PortPpuHudDigitVisible(int digit, int x, int y) {
    int front;
    if (!sHudDigitOwners || digit >= sHudDigitCount || x < 0 || y < 0 || x >= gPortHudW || y >= gPortHudH)
        return 0;
    front = sHudDigitOwners[y * gPortHudW + x];
    return front == 0 || front >= sHudDigits[digit].slot + 1;
}
int PortPpuHudSpriteVisible(int slot, int x, int y) {
    int front;
    if (!sHudDigitOwners || x<0 || y<0 || x>=gPortHudW || y>=gPortHudH) return 0;
    front=sHudDigitOwners[y*gPortHudW+x];
    return front != 0 && front >= slot+1;
}
static int sEnemyHudRect[4]; /* left, top, right, bottom in HUD pixels */
static int sEnemyLabelRect[4];
static uint32_t sEnemyLabelColor;
int PortPpuEnemyHudLabel(int* rect, uint32_t* color) {
    if (sEnemyLabelRect[2] < sEnemyLabelRect[0]) return 0;
    memcpy(rect, sEnemyLabelRect, sizeof(sEnemyLabelRect));
    *color = sEnemyLabelColor;
    return 1;
}
int PortPpuEnemyHudRect(int* rect) {
    if (sEnemyHudRect[2] < sEnemyHudRect[0]) return 0;
    memcpy(rect, sEnemyHudRect, sizeof(sEnemyHudRect));
    return 1;
}
int gPortTextChar;
int gPortTextStyle;

/* ---- screen layer ---------------------------------------------------------------
 * Menus over the world (pause, save, world select, level up): their sprites
 * (drawn between PortScreenBegin/End) and the layers that are not world maps
 * are composed into gPortOverlay, gPortOverlayW x 160, which the presenter
 * scales to the window's height; the world keeps its own scale underneath.
 * The game marks every frame it draws such a menu.
 */
uint32_t gPortOverlay[PORT_MAX_W * GBA_SCREEN_H];
int gPortOverlayW = GBA_SCREEN_W;
int gPortScreenLayer; /* this frame uses the screen layer (set by the presenter) */
static int sScreenDepth, sScreenMark, sOverlay;
static int sScreenBg[4];
void PortScreenBg(int bg, int on) {
    if ((unsigned)bg < 4) {
        if (on) sScreenBgMask |= 1u << bg;
        else sScreenBgMask &= ~(1u << bg);
    }
}
static PPU_TLS uint16_t sScrLine[PORT_MAX_W];
static PPU_TLS uint8_t sScrPrio[PORT_MAX_W];
static PPU_TLS uint8_t sScrIndex[PORT_MAX_W];
static uint8_t sScreenOccBg[PORT_MAX_W * GBA_SCREEN_H];
static uint8_t sScreenOccBgLayer[PORT_MAX_W * GBA_SCREEN_H];
static uint16_t sScreenOccObj[PORT_MAX_W * GBA_SCREEN_H];
static PPU_TLS uint8_t sScrSemi[PORT_MAX_W];

void PortScreenBegin(void) {
    sScreenDepth++;
    sScreenMark = 1;
}

void PortScreenEnd(void) {
    if (sScreenDepth > 0)
        sScreenDepth--;
}

int PortTakeScreenMark(void) {
    int m = sScreenMark;
    sScreenMark = 0;
    return m;
}

/* Screen-layer BG split (as in the Vita port): the columns left of `column`
   move into the left margin, the rest into the right one. 0: off. */

void PortBgSplit(int bg, int column) {
    if (bg >= 0 && bg < 4)
        sBgSplit[bg] = column;
}

int PortScreenMarginX(void) {
    return (gPortOverlayW - GBA_SCREEN_W) / 2;
}

int gPortHudScale;
int gPortHeaderDraw;
static int HeaderBandColor(int y, int phase);
int gPortHudW;
int gPortHudH;

void PortSpriteTag(int entry) {
    if (entry >= 0 && entry < 128) {
        sEntryTag[entry].hud = sHudDepth > 0;
        sEntryTag[entry].ax = (int8_t)sHudAx;
        sEntryTag[entry].ay = (int8_t)sHudAy;
        sEntryTag[entry].ayq = (int16_t)sHudAyQ;
        sEntryTag[entry].text = (uint16_t)gPortTextChar;
        sEntryTag[entry].style = (uint8_t)gPortTextStyle;
        sEntryTag[entry].screen = sScreenDepth > 0;
        sEntryTag[entry].header = gPortHeaderDraw != 0;
        sEntryTag[entry].hdHudPart = (uint8_t)gPortHdHudPart;
        sEntryTag[entry].digit = (uint8_t)gPortHudDigit;
        sEntryTag[entry].clipX0 = (int16_t)sSpriteClipX0;
        sEntryTag[entry].clipX1 = (int16_t)sSpriteClipX1;
    }
}

/* HD sprites: which HD picture an OAM slot shows and the part's box in its frame (PortOamSetHdPart). */
typedef struct OamHdPart {
    uint16_t id;
    int16_t x, y, w, h;
    uint8_t flags, bank;
    const void *tiles, *frame; /* for the export (KHCOM_HDSPRITE_EXPORT) */
    const uint16_t* palette;   /* the part's original palette (ROM), for its fade */
} OamHdPart;
static OamHdPart sOamHd[128];
static uint8_t sMainHdPart[128]; /* slot queued as an HD part of the game's picture (QueueMainHdParts) */
static int16_t sMainHdBox[128][4]; /* its box on the canvas: x, y, w, h */
static uint8_t sMainHdScreen[128];  /* ... on the screen layer (menus), its box in the screen layer's coordinates */
static int sMainHdAny;             /* some part queued this frame: the occupancy is collected for MainHdOver */
static int sScreenHdAny;           /* some part queued on the screen layer */
/* The title's logo as an affine background (BG2: its zoom in the intro, the staff roll's end), drawn HD instead
   while the title says so (PortTitleLogoBg) and the HD title is on. */
static int sTitleLogoBg, sTitleLogoBgHd;
extern const unsigned char gTitleLogoBgTiles[];
extern const unsigned char gTitleLogoBgPalette[];
extern uint8_t gBgPaletteBank[4]; /* engine.c: the palette bank each BG's palette was loaded at */
void PortTitleLogoBg(int on) {
    sTitleLogoBg = on != 0;
}
static void QueueMainHdParts(void);
static void MainHdOver(void);

void PortOamTagsReset(void) {
    memset(sOamTag, 0, sizeof(sOamTag));
    memset(sOamHd, 0, sizeof(sOamHd));
}

void PortOamSetHdPart(int oamIndex, const void* tiles, const void* frame, int x, int y, int w, int h, int flags,
                      const void* palette, int bank) {
    if (oamIndex < 0 || oamIndex >= 128)
        return;
    sOamHd[oamIndex].id = (uint16_t)PortHdSpriteFind(tiles, frame);
    sOamHd[oamIndex].x = (int16_t)x;
    sOamHd[oamIndex].y = (int16_t)y;
    sOamHd[oamIndex].w = (int16_t)w;
    sOamHd[oamIndex].h = (int16_t)h;
    sOamHd[oamIndex].flags = (uint8_t)flags;
    sOamHd[oamIndex].tiles = tiles;
    sOamHd[oamIndex].frame = frame;
    sOamHd[oamIndex].palette = palette;
    sOamHd[oamIndex].bank = (uint8_t)bank;
}

static void ExportSprites(void);
int PortHdSpriteKeepsCartridge(int id);

/* Characters the outline font draws: printable Windows-1252 codes. The codes
   below 0x20 are icons (arrows, button shapes) and stay pixel art. */
static int HdTextGlyph(int oamIndex) {
    int c = sOamTag[oamIndex].text;

    if (!sHdText || c <= 0x20 || c == 0x7F)
        return 0;
    if (c > 0xFF) /* Shift-JIS (Japanese version); 0x8140 is the wide space */
        return c >= 0x8141;
    return c != 0x81 && c != 0x8D && c != 0x8F && c != 0x90 && c != 0x9D;
}

/* HD text for sprites written straight into OAM (movie subtitles). */
void PortOamSetText(int oamIndex, int code, int style) {
    if (oamIndex >= 0 && oamIndex < 128) {
        memset(&sOamTag[oamIndex], 0, sizeof(sOamTag[0]));
        /* Cartridge horizontal bar is glyph 25, below printable Latin.
           Subtitle punctuation belongs in the HD line, unlike button icons. */
        sOamTag[oamIndex].text = (uint16_t)(style == 1 && code == 25 ? 0x97 : code);
        sOamTag[oamIndex].style = (uint8_t)style;
    }
}

void PortOamSetSource(int oamIndex, int entry) {
    if (oamIndex >= 0 && oamIndex < 128 && entry >= 0 && entry < 128)
        sOamTag[oamIndex] = sEntryTag[entry];
}

int PortPpuOverlayActive(void) {
    return sOverlay;
}

int PortPpuHudActive(void) {
    return sHudLayer;
}
uint32_t* gPortFrame;
uint32_t* gPortMenuBackground;

#define PLTT16 PPU_PLTT_BASE
#define VRAM8 ((const uint8_t*)(uintptr_t)GBA_VRAM)

/* The registration that matches what the tile holds now: the latest, else the one before. */
static const BgTextTile* BgTextTileAt(unsigned offset) {
    const BgTextTile* t = &sBgTextTiles[offset / 32];
    if (t->code && !memcmp(t->source, VRAM8 + offset, 32))
        return t;
    t = &sBgTextTilesPrev[offset / 32];
    return t->code && !memcmp(t->source, VRAM8 + offset, 32) ? t : &sBgTextTiles[offset / 32];
}

#define OAM16 PPU_OAM_BASE

/* Line buffers hold BGR555 colours; bit 15 marks a transparent pixel. */
#define TRANSPARENT 0x8000

enum {
    LAYER_BG0,
    LAYER_BG1,
    LAYER_BG2,
    LAYER_BG3,
    LAYER_OBJ,
    LAYER_BD,
};

#define WIN_BG(n) (1 << (n))
#define WIN_OBJ 0x10
#define WIN_SFX 0x20

static PPU_TLS uint16_t sBgLine[4][PORT_MAX_W];
static PPU_TLS uint16_t sObjLine[PORT_MAX_W];
static PPU_TLS int sLineHasObjects;
static PPU_TLS uint8_t sObjPrio[PORT_MAX_W];
static PPU_TLS uint8_t sObjIndex[PORT_MAX_W];
/* HUD layer: what covers each canvas pixel in the GBA's own compositing, so
   HUD sprites stay behind what the cartridge draws in front of them: the
   highest-priority opaque BG (4: none) and the winning other OBJ as
   priority * 128 + OAM index (0xFFFF: none). A window that hides OBJs there
   reads as 0 / 0. */
static uint8_t* sOccBg;
static uint8_t* sOccBgLayer;
/* HD-2D: 1 where a message window (docked layer) shows; allocated on use. */
static uint8_t* sMsgMask; /* 1 where a message window shows */
static uint16_t* sOccObj;
static PPU_TLS uint8_t sObjSemi[PORT_MAX_W];
static PPU_TLS uint8_t sObjWin[PORT_MAX_W];
static PPU_TLS uint8_t sWinMask[PORT_MAX_W];
/* Wide view: 1 where some layer has world data (or inside the GBA picture);
   the backdrop shows black elsewhere, i.e. beyond the edge of the world. */
static PPU_TLS uint8_t sInWorld[PORT_MAX_W];
static uint16_t sWorldPadding[4]; /* Opaque, uniform map-corner padding; never scenery repeated. */
/* Ephemeral bounds of an entirely opaque rectangular scenery plane. Empty
 * authoring margins may surround it; partial tiles / foreground art reject it. */
static int sTiledBackdropBg = -1;
static int sTiledBackdropBounds[4];

/* Affine background reference points (internal registers). */
static PPU_TLS int32_t sAffX[2], sAffY[2];
static uint32_t sAffLatchX[2], sAffLatchY[2];

static uint32_t sColorLut[0x8000];
static int sLutMode = -1;

/* Visible span of the current frame in GBA coordinates. */
static int sLeft, sRight, sTop, sBottom;

static int32_t Sext28(uint32_t v) {
    return (int32_t)(v << 4) >> 4;
}

static void BuildLut(void) {
    int c;

    if (sLutMode == gPortConfig.colorCorrection)
        return;
    sLutMode = gPortConfig.colorCorrection;
    for (c = 0; c < 0x8000; c++) {
        int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        int R, G, B;

        if (sLutMode) {
            /* Approximation of the GBA's darker, desaturated LCD. */
            double lr = r / 31.0, lg = g / 31.0, lb = b / 31.0;
            double gr = lr * lr * 0.84 + lg * lg * 0.18 + lb * lb * 0.0;
            double gg = lr * lr * 0.09 + lg * lg * 0.67 + lb * lb * 0.26;
            double gb = lr * lr * 0.15 + lg * lg * 0.10 + lb * lb * 0.73;
            R = (int)(255.0 * (gr > 1 ? 1 : gr) + 0.5);
            G = (int)(255.0 * (gg > 1 ? 1 : gg) + 0.5);
            B = (int)(255.0 * (gb > 1 ? 1 : gb) + 0.5);
            /* gamma back up so the result is not overly dark */
            {
                R = (int)(sqrt(R / 255.0) * 255.0 + 0.5);
                G = (int)(sqrt(G / 255.0) * 255.0 + 0.5);
                B = (int)(sqrt(B / 255.0) * 255.0 + 0.5);
            }
        } else {
            R = (r << 3) | (r >> 2);
            G = (g << 3) | (g >> 2);
            B = (b << 3) | (b >> 2);
        }
        sColorLut[c] = 0xFF000000u | ((uint32_t)R << 16) | ((uint32_t)G << 8) | (uint32_t)B;
    }
}

void PortPpuSetResolution(int w, int h) {
    if (w > PORT_MAX_W)
        w = PORT_MAX_W;
    if (h > PORT_MAX_H)
        h = PORT_MAX_H;
    if (w < GBA_SCREEN_W)
        w = GBA_SCREEN_W;
    if (h < GBA_SCREEN_H)
        h = GBA_SCREEN_H;
    gPortScreen.width = w;
    gPortScreen.height = h;
    gPortScreen.originX = (w - GBA_SCREEN_W) / 2;
    gPortScreen.originY = (h - GBA_SCREEN_H) / 2;
    gPortViewLeft = -gPortScreen.originX;
    gPortViewTop = -gPortScreen.originY;
    gPortViewRight = w - gPortScreen.originX;
    gPortViewBottom = h - gPortScreen.originY;
    if (gPortFrame)
        PortCanvasAlloc();
}

/* Development aid (KHCOM_OAMDUMP): log the sprites of the current frame. */
void PortPpuDumpOam(void) {
    int i;

    for (i = 0; i < 128; i++) {
        uint16_t a0 = OAM16[i * 4], a1 = OAM16[i * 4 + 1], a2 = OAM16[i * 4 + 2];
        if ((a0 & 0x300) == 0x200)
            continue;
        PortLog("oam %3d ext(%4d,%4d) raw(%3d,%3d) shape %d size %d prio %d pal %2d tile %4d mode %d hud %d ax %d ay %d "
                "text %d '%c' style %d", i, sOamExt[i].x, sOamExt[i].y, a1 & 0x1FF, a0 & 0xFF, a0 >> 14, a1 >> 14,
                (a2 >> 10) & 3, a2 >> 12, a2 & 0x3FF, (a0 >> 10) & 3, sOamTag[i].hud, sOamTag[i].ax, sOamTag[i].ay,
                sOamTag[i].text, sOamTag[i].text > 0x20 && sOamTag[i].text < 0x7F ? sOamTag[i].text : '.', sOamTag[i].style);
    }
}

void PortOamSetPosition(int index, int x, int y) {
    if (index >= 0 && index < 128) {
        sOamExt[index].x = (int16_t)x;
        sOamExt[index].y = (int16_t)y;
    }
}

void PortLatchBgMaps(const BgEntry* entries) {
    int b;

    for (b = 0; b < 4; b++) {
        BgLatch* l = &sBgLatch[b];
        l->map = entries[b].map;
        l->width = entries[b].width;
        l->height = entries[b].height;
        l->x = entries[b].x;
        l->y = entries[b].y;
        l->hofs = IO16(0x10 + b * 4) & 0x1FF;
        l->vofs = IO16(0x12 + b * 4) & 0x1FF;
        sBgShiftLatched[b] = sBgShift[b];
        sBgWorldLatched[b] = sBgWorld[b];
        sBgWorldLatched[b].hofs = l->hofs;
        sBgWorldLatched[b].vofs = l->vofs;
        sBgDockedLatched[b] = sBgDocked[b];
        sBgIsoLatched[b] = sBgIso[b];
    }
}

void PortSetBattleArenaBands(int left, int right, int leftStart, int leftWidth,
                             int rightStart, int rightWidth) {
    if (left < 0 || right > 1024 || left >= right || leftWidth <= 0 || rightWidth <= 0 ||
        leftStart < left || leftStart + leftWidth > right ||
        rightStart < left || rightStart + rightWidth > right) {
        memset(sArenaBands, 0, sizeof(sArenaBands));
        return;
    }
    sArenaBands[0] = left; sArenaBands[1] = right;
    sArenaBands[2] = leftStart; sArenaBands[3] = leftWidth;
    sArenaBands[4] = rightStart; sArenaBands[5] = rightWidth;
}

static int ArenaBandPixel(int distance, int start, int width, int right) {
    int phase = distance % (width * 2);
    if (phase >= width) phase = width * 2 - 1 - phase;
    return start + (right ? width - 1 - phase : phase);
}

/* Battle: the card hand sits partly below the GBA screen; the sprites are
   whole, so the bottom HUD can be lifted to show more of them. */
static int HudRaise(void) {
    int r = gPortConfig.cardHudRaise;

    return !sBattleView || r < 0 ? 0 : (r > 16 ? 16 : r);
}

void PortSetBattleView(int on) {
    sBattleView = on;
    if (!on)
        PortMarluxiaField(0);
    memset(sBgClip, 0, sizeof(sBgClip));
    memset(sArenaBands, 0, sizeof(sArenaBands));
}

/* Screen entry of the world map at world pixel (wx, wy); -1 outside it. */
/*
 * Event scenes are isometric: the floor and walls repeat along the 2:1
 * diagonal, two tiles across for one tile down. Past the map's edge, the tile
 * at (tx, ty) is the one reached by stepping inward along that diagonal, with
 * the direction per edge chosen by how often the map repeats itself along
 * each one. One-off tiles (stains, props) are skipped by stepping further, so
 * only repeating scenery is continued. Original tiles only, no new pixels.
 */
#define ISO_COMMON 6
/* one table per background: two iso layers with different maps used to share one, and every lookup of one threw
   the other's away (a 4096-tile recount per pixel: event 004 ran under 5 fps at x1). Filled by IsoPrime before the
   outer rows are split between cores, read-only while they draw. */
#define ISO_MEMO_MARGIN 256 /* tiles around the map whose walk is remembered (2048 px: any canvas) */
typedef struct {
    void* const* map;
    int width, height, ok;
    int leftDy, rightDy, topDx, bottomDx;
    uint16_t freq[65536];
    /* per tile around the map: the map tile its walk ends on, as y << 16 | x, plus 1; 0 not walked yet, -1 none.
       The coordinate, not the entry: the map is read live, so animated tiles keep moving. Cores may store the same
       value at once (aligned 32-bit stores). */
    int32_t* memo;
    int memoW, memoH;
} IsoTable;
static IsoTable sIsoTables[4];

static int MapTile(const BgLatch* l, int tx, int ty) {
    const uint16_t* block = (const uint16_t*)l->map[(ty >> 5) * l->width + (tx >> 5)];
    if (!PORT_GBA_MAP_MEMORY(block))
        return -1;
    return block[(ty & 31) * 32 + (tx & 31)];
}

/* Matches between each tile near an edge and its diagonal neighbour inward. */
static int IsoScore(const BgLatch* l, int x0, int x1, int y0, int y1, int dx, int dy) {
    int W = l->width * 32, H = l->height * 32, x, y, score = 0;
    for (y = y0; y < y1; y++)
        for (x = x0; x < x1; x++) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && ny >= 0 && nx < W && ny < H && MapTile(l, x, y) == MapTile(l, nx, ny))
                score++;
        }
    return score;
}

static IsoTable* IsoPrepare(const BgLatch* l) {
    IsoTable* t = &sIsoTables[(l - sBgLatch) & 3];
    int W = l->width * 32, H = l->height * 32, x, y;
    if (t->map == l->map && t->width == l->width && t->height == l->height)
        return t->ok ? t : NULL;
    t->map = l->map;
    t->width = l->width;
    t->height = l->height;
    t->ok = 0;
    if (W < 8 || H < 8)
        return NULL;
    t->memoW = W + 2 * ISO_MEMO_MARGIN;
    t->memoH = H + 2 * ISO_MEMO_MARGIN;
    free(t->memo);
    t->memo = (int32_t*)calloc((size_t)t->memoW * t->memoH, sizeof(int32_t));
    memset(t->freq, 0, sizeof(t->freq));
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            int e = MapTile(l, x, y);
            if (e < 0)
                return NULL;
            if (t->freq[e] < 0xFFFF)
                t->freq[e]++;
        }
    t->leftDy = IsoScore(l, 0, 8, 0, H, 2, -1) >= IsoScore(l, 0, 8, 0, H, 2, 1) ? -1 : 1;
    t->rightDy = IsoScore(l, W - 8, W, 0, H, -2, -1) >= IsoScore(l, W - 8, W, 0, H, -2, 1) ? -1 : 1;
    t->topDx = IsoScore(l, 0, W, 0, 4, 2, 1) >= IsoScore(l, 0, W, 0, 4, -2, 1) ? 2 : -2;
    t->bottomDx = IsoScore(l, 0, W, H - 4, H, 2, -1) >= IsoScore(l, 0, W, H - 4, H, -2, -1) ? 2 : -2;
    t->ok = 1;
    return t;
}

/* every iso layer's table, before the cores read them */
static void IsoPrime(void) {
    int b;
    for (b = 0; b < 4; b++)
        if (sBgIsoLatched[b] && sBgLatch[b].map != NULL && sBgLatch[b].width > 0 && sBgLatch[b].height > 0)
            (void)IsoPrepare(&sBgLatch[b]);
}

static int IsoEntry(const BgLatch* l, int tx, int ty) {
    int W = l->width * 32, H = l->height * 32, dx = 0, dy = 0, i, e;
    const IsoTable* t = IsoPrepare(l);
    int32_t* memo = NULL;
    if (!t)
        return -1;
    if (t->memo && tx >= -ISO_MEMO_MARGIN && ty >= -ISO_MEMO_MARGIN && tx < W + ISO_MEMO_MARGIN &&
        ty < H + ISO_MEMO_MARGIN) {
        int32_t m;
        memo = &t->memo[(ty + ISO_MEMO_MARGIN) * t->memoW + tx + ISO_MEMO_MARGIN];
        m = *memo;
        if (m < 0)
            return -1;
        if (m > 0)
            return MapTile(l, (m - 1) & 0xFFFF, (m - 1) >> 16);
    }
    /* Walk inward: across a side edge along that edge's diagonal, across the
       top or bottom along theirs (both inward at a corner). */
    for (i = 0; i < 4096; i++) {
        int outX = tx < 0 || tx >= W, outY = ty < 0 || ty >= H;
        if (!outX && !outY)
            break;
        if (outX) {
            dx = tx < 0 ? 2 : -2;
            dy = outY ? (ty < 0 ? 1 : -1) : (tx < 0 ? t->leftDy : t->rightDy);
        } else {
            dy = ty < 0 ? 1 : -1;
            dx = ty < 0 ? t->topDx : t->bottomDx;
        }
        tx += dx;
        ty += dy;
    }
    if (tx < 0 || ty < 0 || tx >= W || ty >= H) {
        if (memo)
            *memo = -1;
        return -1;
    }
    e = MapTile(l, tx, ty);
    while (e >= 0 && t->freq[e] < ISO_COMMON && i++ < 4096) {
        int nx = tx + dx, ny = ty + dy;
        if (nx < 0 || ny < 0 || nx >= W || ny >= H)
            break;
        tx = nx;
        ty = ny;
        e = MapTile(l, tx, ty);
    }
    if (memo && e >= 0)
        *memo = (ty << 16 | tx) + 1;
    return e;
}

static int WorldEntry(const BgLatch* l, int wx, int wy) {
    const uint16_t* block;
    if (sSinglePanelBg >= 0 && l == &sBgLatch[sSinglePanelBg] &&
        (wx < 0 || wy < 0 || wx >= 256 || wy >= 256)) return -1;

    /* battle stages continue sideways instead of ending in the void */
    if (sBattleView && l->width > 0 && (wx < 0 || wx >= l->width * 256))
        wx = ((wx % (l->width * 256)) + l->width * 256) % (l->width * 256);

    if (l->map != NULL && sBgIsoLatched[l - sBgLatch] && !sBattleView && l->width > 0 && l->height > 0 &&
        (wx < 0 || wy < 0 || wx >= l->width * 256 || wy >= l->height * 256))
        return IsoEntry(l, wx >> 3, wy >> 3);
    if (l->map == NULL || wx < 0 || wy < 0 || wx >= l->width * 256 || wy >= l->height * 256)
        return -1;
    block = (const uint16_t*)l->map[(wy >> 8) * l->width + (wx >> 8)];
    /* Blocks live in work RAM or the ROM; anything else is not a world map. Jafar's room and genie keep a block in
       a work RAM buffer, a native global in the PC build (without it his fight had no wide view) */
    if (!PORT_GBA_MAP_MEMORY(block) && !(PortJafarWide() && PortJafarMapBuffer(block)))
        return -1;
    return block[((wy & 255) >> 3) * 32 + ((wx & 255) >> 3)];
}

/* Accepted per-layer source recipes. Called only for the extra viewport
 * pixels; native drawing/validation always uses the original map entries.
 * A declared 64x64 map can have only one occupied 32x32 block (scene 074),
 * so apply profiles before testing declared map bounds. */
static int ExtendedWorldEntry(const BgLatch* l, int wx, int wy) {
    int bg = (int)(l - sBgLatch), tx = wx >> 3, ty = wy >> 3, supported = 0, ok = 0;
    if (sPreliminarRecipe[bg]) {
        ok = PortPreliminarTileSource(sPreliminarRecipe[bg], &tx, &ty);
        return ok ? MapTile(l, tx, ty) : -1;
    }
    if (sDarksideLayer[bg]) {
        ok = PortDarksideTileSource(sDarksideLayer[bg], &tx, &ty, (l->x + GBA_SCREEN_W / 2) >> 3,
                                    (l->y + GBA_SCREEN_H / 2) >> 3);
        return ok ? MapTile(l, tx, ty) : -1;
    }
    if (sAgrabah109Layer[bg]) {
        ok = PortAgrabah109TileSource(sAgrabah109Layer[bg], &tx, &ty);
        return ok ? MapTile(l, tx, ty) : -1;
    }
    if (sAgrabahLayer[bg]) {
        ok = PortAgrabahTileSource(sAgrabahLayer[bg], &tx, &ty, sPreliminarV1);
        return ok ? MapTile(l, tx, ty) : -1;
    }
    if (!sBattleView && l->map && sUseSceneProfiles) {
        if (sEventBackdropProfile == 57 && (bg == 2 || bg == 3)) {
            supported = 1;
            ok = PortScene057TileSource(bg, &tx, &ty);
        } else if (sEventBackdropProfile == 61 && (bg == 2 || bg == 3)) {
            supported = 1;
            ok = PortScene061TileSource(bg, l->width * 32, l->height * 32, &tx, &ty);
        } else if (sEventBackdropProfile == 74 && bg >= 1 && bg <= 3) {
            supported = 1;
            ok = PortScene074TileSource(bg, &tx, &ty);
        }
    }
    if (supported)
        return ok && tx >= 0 && ty >= 0 && tx < l->width * 32 && ty < l->height * 32 ? MapTile(l, tx, ty) : -1;
    return WorldEntry(l, wx, wy);
}

/* Only a completely uniform, opaque corner tile is safe exterior padding.
 * Carry its palette colour beyond the map without stretching scenery or UI. */
static uint16_t WorldPadding(int bg, const BgLatch* l) {
    int corner, bpp8 = (IO16(8 + bg * 2) & 0x80) != 0;
    unsigned base = ((IO16(8 + bg * 2) >> 2) & 3) * 0x4000;
    if (sDarksideLayer[bg] == 2) return TRANSPARENT; /* one Darkside: nothing beside his frame */
    if (!l->valid || sBgDockedLatched[bg] || sScreenBg[bg] ||
        (sPreliminarRecipe[bg] && bg!=3)) return TRANSPARENT;
    /* With the floor continued past the map, the layers above it end there. */
    if (!sBgIsoLatched[bg] && (sBgIsoLatched[0] | sBgIsoLatched[1] | sBgIsoLatched[2] | sBgIsoLatched[3]))
        return TRANSPARENT;
    for (corner = 0; corner < 4; corner++) {
        int entry = WorldEntry(l, (corner & 1) ? l->width * 256 - 1 : 0,
                                  (corner & 2) ? l->height * 256 - 1 : 0);
        unsigned off; int ci, i, bytes = bpp8 ? 64 : 32;
        const uint8_t* tile;
        if (entry < 0) continue;
        off = base + (entry & 1023) * bytes;
        if (off + bytes > 0x10000) continue;
        tile = VRAM8 + off; ci = bpp8 ? tile[0] : tile[0] & 15;
        if (!ci || (!bpp8 && tile[0] != ci * 17)) continue;
        for (i = 1; i < bytes && tile[i] == tile[0]; i++) {}
        if (i == bytes) return PLTT16[(bpp8 ? 0 : (entry >> 12) * 16) + ci] & 0x7FFF;
    }
    return TRANSPARENT;
}

static void FindTiledBackdrop(int bg) {
    const BgLatch* l;
    signed char opacity[1024];
    int x, y, count = 0, left, top, right = 0, bottom = 0;
    int bpp8, bytes;
    unsigned base;
    sTiledBackdropBg = -1;
    if (bg < 0 || bg > 3 || sSinglePanelBg >= 0 || sBattleView || sOverlay ||
        sScreenBg[bg] || sBgDockedLatched[bg] || sWorldPadding[bg] != TRANSPARENT) return;
    l = &sBgLatch[bg];
    if (!l->valid || l->width > 4 || l->height > 4) return;
    left = l->width * 32; top = l->height * 32;
    bpp8 = (IO16(8 + bg * 2) & 0x80) != 0; bytes = bpp8 ? 64 : 32;
    base = ((IO16(8 + bg * 2) >> 2) & 3) * 0x4000;
    memset(opacity, -1, sizeof(opacity));
    for (y = 0; y < l->height * 32; y++) for (x = 0; x < l->width * 32; x++) {
        int entry = WorldEntry(l, x * 8, y * 8), tile, kind, i;
        const uint8_t* pixels;
        int zero = 0, solid = 0;
        if (entry < 0) return;
        tile = entry & 1023; kind = opacity[tile];
        if (kind < 0) {
            if (base + tile * bytes + bytes > 0x10000) return;
            pixels = VRAM8 + base + tile * bytes;
            for (i = 0; i < bytes; i++) {
                zero += bpp8 ? pixels[i] == 0 : ((pixels[i] & 15) == 0) + ((pixels[i] >> 4) == 0);
                solid += bpp8 ? pixels[i] != 0 : ((pixels[i] & 15) != 0) + ((pixels[i] >> 4) != 0);
            }
            kind = zero ? (solid ? 2 : 0) : 1; opacity[tile] = kind;
        }
        if (kind == 2) return; /* Never extend silhouettes or scenery edges. */
        if (kind == 1) {
            count++; if (x < left) left = x; if (y < top) top = y;
            if (x + 1 > right) right = x + 1; if (y + 1 > bottom) bottom = y + 1;
        }
    }
    if (right - left < 30 || bottom - top < 20 ||
        count != (right - left) * (bottom - top) || count == l->width * l->height * 1024) return;
    sTiledBackdropBg = bg;
    sTiledBackdropBounds[0] = left * 8; sTiledBackdropBounds[1] = top * 8;
    sTiledBackdropBounds[2] = right * 8; sTiledBackdropBounds[3] = bottom * 8;
}

/* Repeat whole native tile bands, with reflected phase at the seam. Only
 * added view pixels use this; the original 240x160 picture is never changed. */
static void TiledBackdropCoordinates(int bg, int* wx, int* wy) {
    if (bg == sTiledBackdropBg) {
        int left = sTiledBackdropBounds[0], top = sTiledBackdropBounds[1];
        int right = sTiledBackdropBounds[2], bottom = sTiledBackdropBounds[3];
        if (*wx < left) *wx = ArenaBandPixel(left - 1 - *wx, left, 64, 0);
        else if (*wx >= right) *wx = ArenaBandPixel(*wx - right, right - 64, 64, 1);
        if (*wy < top) *wy = ArenaBandPixel(top - 1 - *wy, top, 32, 0);
        else if (*wy >= bottom) *wy = ArenaBandPixel(*wy - bottom, bottom - 32, 32, 1);
    }
}

/* Scene 102's BG3 is an isometric floor/water silhouette, not a rectangle.
 * Fill only its transparent added-view pixels from plain floor or water
 * tiles, leaving the original composition and its diagonal trim untouched. */
static uint16_t Preliminar102Underlay(const uint8_t* chars, int bpp8, int wx, int wy) {
    /* The raised path (lighter, between the ledges at 2y-x=-4 and 304), the
     * darker floor on both sides and the sea go on along their diagonal (user
     * 24/10): each pixel slides along 2y-x, two tiles at a time, into a
     * clean strip of the map, the path's back from columns 32..33, its front
     * from columns 12..13. The back ledge curves away up-left at the map's
     * top (along 2x-y=224..287): it goes on that way from row 4, and left of
     * it the path is open. Past the second step below the path (2y-x=400)
     * the floor is one plain colour (columns 0..1, row 29). */
    int d = 2 * wy - wx, k = 2 * wx - wy, x0 = d < 150 ? 256 : 96, n = wx - x0;
    int sx, sy, entry, px, py, ci, pal, tile;
    n = n >= 0 ? n / 16 : -((-n + 15) / 16);
    if (wy < 32 && k >= 224 && k < 288) {
        n = (32 - wy + 15) / 16;
        wx += 8 * n; wy += 16 * n;
    } else if (d >= 416) {
        wx &= 15; wy = 232 + (wy & 7);
    } else {
        wx -= 16 * n; wy -= 8 * n;
        if (d < 56 && k < 224) wy += 16 * ((56 - d + 31) / 32);   /* the open path */
        else if (d < -152) wy += 16 * ((-152 - d + 31) / 32);    /* the open sea */
    }
    sx = wx >> 3; sy = wy >> 3;
    entry = MapTile(&sBgLatch[3], sx, sy);
    px = wx & 7; py = wy & 7;
    if (entry < 0) return PLTT16[0] & 0x7FFF;
    tile = entry & 0x3FF;
    if (entry & 0x400) px = 7 - px;
    if (entry & 0x800) py = 7 - py;
    if (bpp8) {
        ci = chars[tile * 64 + py * 8 + px];
        return PLTT16[ci] & 0x7FFF;
    }
    ci = (chars[tile * 32 + py * 4 + (px >> 1)] >> ((px & 1) * 4)) & 15;
    pal = (entry >> 12) * 16;
    return PLTT16[pal + ci] & 0x7FFF;
}

static uint16_t Preliminar102ViewUnderlay(int gx, int gy) {
    const BgLatch* l = &sBgLatch[3];
    uint16_t cnt = IO16(0x0E);
    int hofs = IO16(0x1C) & 0x1FF, vofs = IO16(0x1E) & 0x1FF;
    int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
    const uint8_t* chars = VRAM8 + ((cnt >> 2) & 3) * 0x4000;
    TiledBackdropCoordinates(3, &wx, &wy);
    return Preliminar102Underlay(chars, (cnt >> 7) & 1, wx, wy);
}

static int VramEntry(int bg, int gx, int gy) {
    uint16_t cnt = IO16(0x08 + bg * 2);
    const uint16_t* screen = (const uint16_t*)(VRAM8 + ((cnt >> 8) & 31) * 0x800);
    int size = cnt >> 14;
    int px = (gx + (IO16(0x10 + bg * 4) & 0x1FF)) & ((size & 1) ? 511 : 255);
    int py = (gy + (IO16(0x12 + bg * 4) & 0x1FF)) & ((size & 2) ? 511 : 255);
    int tx = px >> 3, ty = py >> 3, block = 0;

    if (tx >= 32)
        block += 1;
    if (ty >= 32)
        block += (size == 3) ? 2 : 1;
    return screen[block * 0x400 + (ty & 31) * 32 + (tx & 31)];
}

/* A map-block BG can be extended when what VRAM shows is the world map. */
static int ValidateBgLatch(int bg, uint16_t dispcnt) {
    static const int xs[3] = { 8, 120, 232 }, ys[3] = { 8, 80, 152 };
    BgLatch* l = &sBgLatch[bg];
    int mode = dispcnt & 7, i, j, match = 0, outsideBlank;

    if (!(dispcnt & (0x100 << bg)) || l->map == NULL || l->width == 0 || l->height == 0)
        return 0;
    /* Darkside's body stands at the bottom of his map: below it VRAM holds the blank tile */
    outsideBlank = PortDarksideLayer((const void* const*)l->map) == 2;
    /* Message window layers are screen UI, never part of the world. */
    for (i = 0; i < 4; i++)
        if ((const void*)l->map == gMsgwinMapBlocks[i])
            return 0;
    if (mode > 1 || (mode == 1 && bg == 2))
        return 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            int we = WorldEntry(l, l->x + xs[i], l->y + ys[j]), ve = VramEntry(bg, xs[i], ys[j]);
            if (we >= 0 ? we == ve : outsideBlank && (ve & 1023) == 0)
                match++;
        }
    }
    return match >= 7;
}

/* Same check for a game-provided world (cell rooms). */
static int ValidateBgWorld(int bg, uint16_t dispcnt) {
    static const int xs[3] = { 8, 120, 232 }, ys[3] = { 8, 80, 152 };
    BgWorld* w = &sBgWorldLatched[bg];
    int mode = dispcnt & 7, i, j, match = 0;

    if (!(dispcnt & (0x100 << bg)) || w->fn == NULL || mode > 1 || (mode == 1 && bg == 2))
        return 0;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            if (w->fn(bg, (w->x + xs[i]) >> 3, (w->y + ys[j]) >> 3) == VramEntry(bg, xs[i], ys[j]))
                match++;
    return match >= 7;
}

/* Extend the missing arcs from the panel's own edge samples. Work in
   palette-index space, preserving each native 4x4 dither phase. No image
   filtering or generated RGB shades: palette animation/fades remain exact. */
static int SinglePanelIndex(const BgLatch* l, uint16_t cnt, int x, int y) {
    int entry = WorldEntry(l, x, y), xx, yy, off, value;
    if (entry < 0) return -1;
    xx = (entry & 1024) ? 7 - (x & 7) : x & 7;
    yy = (entry & 2048) ? 7 - (y & 7) : y & 7;
    off = ((cnt >> 2) & 3) * 0x4000 + (entry & 1023) * 32 + yy * 4 + xx / 2;
    if (off >= 0x10000) return -1;
    value = (VRAM8[off] >> ((xx & 1) * 4)) & 15;
    return (entry >> 12) * 16 + value;
}

/* Reconstruct one complete, original fir from its overlapping occurrences.
 * Work in palette indices, then keep the component attached to its tip.
 * No raster art is stored in the repository or extra game objects created.
 * The cache is derived from exact live character bytes, so state loads and
 * reused VRAM allocations cannot retain an unrelated tree. Palette remains live. */
static uint8_t sPreliminarFir[88*152], sPreliminarFirChars[0x4000];
static uint8_t sPreliminarFirSeen[88*152];
static uint16_t sPreliminarFirQueue[88*152];
static int sPreliminarFirValid;
static void PreparePreliminarFir(void) {
    const BgLatch* l=&sBgLatch[2];
    uint16_t cnt=IO16(12);
    const uint8_t* chars=VRAM8+((cnt>>2)&3)*0x4000;
    static const int shifts[5][2]={{0,0},{264,8},{-48,56},{280,112},{208,128}};
    int x,y,i,j,head=0,tail=0;
    if (sPreliminarFirValid && !memcmp(chars,sPreliminarFirChars,0x4000)) return;
    memcpy(sPreliminarFirChars,chars,0x4000);
    for (y=0;y<152;y++) for (x=0;x<88;x++) {
        int c[5],best=x+80<116?1:0,bestCount=0;
        for (i=0;i<5;i++) {
            c[i]=SinglePanelIndex(l,cnt,x+80+shifts[i][0],y+224+shifts[i][1]);
            if (c[i]<0 || !(c[i]&15)) c[i]=0;
        }
        for (j=0;j<5;j++) if (c[j]==c[best]) bestCount++;
        for (i=0;i<5;i++) {
            int count=0;
            for (j=0;j<5;j++) if (c[i]==c[j]) count++;
            if (count>bestCount) { best=i; bestCount=count; }
        }
        sPreliminarFir[y*88+x]=(uint8_t)c[best];
    }
    memset(sPreliminarFirSeen,0,sizeof(sPreliminarFirSeen));
    if (sPreliminarFir[43]) { sPreliminarFirQueue[tail++]=43; sPreliminarFirSeen[43]=1; }
    while (head<tail) {
        int pos=sPreliminarFirQueue[head++],xx,yy;
        x=pos%88; y=pos/88;
        for (yy=y-1;yy<=y+1;yy++) for (xx=x-1;xx<=x+1;xx++) {
            int n;
            if (xx<0 || xx>=88 || yy<0 || yy>=152) continue;
            n=yy*88+xx;
            if (!sPreliminarFirSeen[n] && sPreliminarFir[n]) {
                sPreliminarFirSeen[n]=1;
                sPreliminarFirQueue[tail++]=(uint16_t)n;
            }
        }
    }
    for (i=0;i<88*152;i++) if (!sPreliminarFirSeen[i]) sPreliminarFir[i]=0;
    sPreliminarFirValid=1;
}

static int PreliminarFirPixel(int wx,int wy,uint16_t* color) {
    static const int anchors[][2]={{-880,544},{-608,376},{-400,488},{-168,560},
                                  {616,408},{856,496},{1080,384},{1312,560}};
    unsigned i;
    if (!sPreliminarFirValid || (wx>=0 && wx<512) || wy<224 || wy>=560) return 0;
    for (i=0;i<sizeof(anchors)/sizeof(anchors[0]);i++) {
        int x=wx-anchors[i][0]+44, y=wy-anchors[i][1]+152,ci;
        if (x<0 || x>=88 || y<0 || y>=152) continue;
        ci=sPreliminarFir[y*88+x];
        if (ci) { *color=PLTT16[ci]&0x7fff; return 1; }
    }
    return 0;
}
static uint16_t SinglePanelColor(const BgLatch* l, uint16_t cnt, int wx, int wy) {
    int edge, inward, distance, a, b, c, slope, shade, bank;
    int white = SinglePanelIndex(l, cnt, 0, 0);
    if (white < 0) return TRANSPARENT;
    if (wy < 0 || wy >= 256 || wx < -64 || wx >= 320 || (cnt & 128))
        return PLTT16[white] & 0x7FFF;
    edge = wx < 0 ? wx & 3 : 252 + (wx & 3);
    inward = wx < 0 ? 16 : -16;
    distance = wx < 0 ? edge - wx : wx - edge;
    a = SinglePanelIndex(l, cnt, edge, wy);
    b = SinglePanelIndex(l, cnt, edge + inward, wy);
    c = SinglePanelIndex(l, cnt, edge + inward * 2, wy);
    if (a < 0 || b < 0 || c < 0 || (a >> 4) != (white >> 4) ||
        (a >> 4) != (b >> 4) || (a >> 4) != (c >> 4)) return PLTT16[white] & 0x7FFF;
    bank = a & ~15;
    /* The native panel's indices run from white (1) to the dark centre (15).
       A non-increasing outward slope prevents reflected/repeated centres. */
    slope = (a - b) * 16;
    if ((a - c) * 8 < slope) slope = (a - c) * 8;
    if (slope >= 0) slope = -16;
    shade = (a & 15) * 256 + slope * distance;
    shade = shade < 256 ? 1 : shade >> 8;
    return PLTT16[bank + shade] & 0x7FFF;
}

/* ---- backgrounds --------------------------------------------------------- */

/* Requested market dressing, rendered as backdrop pixels rather than new
 * game objects/OAM entries. Original props, actors and the GBA stay intact. */
static int Agrabah109DecorPixel(const BgLatch* l, const uint8_t* chars,
                               int wx, int wy, uint16_t* color) {
    static const struct { int x, kind; } stalls[] = {
        {-752, 1}, {-544, 0}, {-304, 1}, {-64, 0},
        {296, 0}, {536, 1}, {760, 0}, {1008, 1}
    };
    static const int pots[] = {-864, -640, -424, -176, 248, 408, 648, 896, 1128};
    int i, dx, dy, sx, sy, entry, ci, diagonal = 2 * wy + wx;
    if ((wx >= 0 && wx < 256 && wy >= 0 && wy < 256) || diagonal < 112 || diagonal > 328)
        return 0;
    for (i = 0; i < (int)(sizeof(stalls) / sizeof(stalls[0])); i++) {
        int kind = stalls[i].kind;
        int anchorY = ((kind ? 262 : 244) - stalls[i].x) / 2;
        const uint8_t* tiles = kind ? gMapGmkAgrabah09Tiles : gMapGmkAgrabah00Tiles;
        dx = wx - (stalls[i].x - 32); dy = wy - (anchorY - 48);
        if (dx < 0 || dx >= 64 || dy < 0 || dy >= 64 || sAgrabah109ShopPal[kind] < 0)
            continue;
        /* Both authored frames are one 64x64, 4bpp, unflipped 1D piece. */
        ci = (tiles[((dy >> 3) * 8 + (dx >> 3)) * 32 + (dy & 7) * 4 + ((dx & 7) >> 1)]
              >> ((dx & 1) * 4)) & 15;
        if (ci) {
            *color = PLTT16[256 + sAgrabah109ShopPal[kind] * 16 + ci] & 0x7FFF;
            return 1;
        }
    }
    for (i = 0; i < (int)(sizeof(pots) / sizeof(pots[0])); i++) {
        dx = pots[i] - 24; dy = -dx / 2;
        sx = wx - dx; sy = wy - dy;
        /* Complete urn-and-bowl group, including its authored ground contact. */
        if (sx < 8 || sx >= 40 || sy < 80 || sy >= 112) continue;
        entry = MapTile(l, sx >> 3, sy >> 3);
        if (entry < 0) continue;
        sx &= 7; sy &= 7;
        if (entry & 0x400) sx = 7 - sx;
        if (entry & 0x800) sy = 7 - sy;
        ci = (chars[(entry & 1023) * 32 + sy * 4 + sx / 2] >> ((sx & 1) * 4)) & 15;
        if (ci) {
            *color = PLTT16[(entry >> 12) * 16 + ci] & 0x7FFF;
            return 1;
        }
    }
    return 0;
}

/* the boss arenas' Preliminar V2 (BATTLE_SCENERY_AUDIT.md): on in the review copy only until approved */
int PortArenasV2(void) {
    static int on = -1;
    if (on < 0)
        on = getenv("KHCOM_ARENAS_V2") && *getenv("KHCOM_ARENAS_V2") == '1';
    return on;
}

static void DrawTextBg(int bg, int gy, uint16_t dispcnt) {
    uint16_t cnt = IO16(0x08 + bg * 2);
    int hofs = IO16(0x10 + bg * 4) & 0x1FF;
    int vofs = IO16(0x12 + bg * 4) & 0x1FF;
    const uint8_t* chars = VRAM8 + ((cnt >> 2) & 3) * 0x4000;
    const uint16_t* screen = (const uint16_t*)(VRAM8 + ((cnt >> 8) & 31) * 0x800);
    int size = cnt >> 14;
    int wmask = (size & 1) ? 511 : 255;
    int hmask = (size & 2) ? 511 : 255;
    int bpp8 = (cnt >> 7) & 1;
    int mosaic = (cnt >> 6) & 1;
    int mosH = mosaic ? (IO16(0x4C) & 15) + 1 : 1;
    int mosV = mosaic ? ((IO16(0x4C) >> 4) & 15) + 1 : 1;
    uint16_t* out = sBgLine[bg];
    int x, py, ty, fy, outside;
    int w = gPortScreen.width;
    int cachedTx = 0, cachedTy = 0, cachedEntry = -1, cachedValid = 0;
    /* also in the event before the fight (008), where the armour falls and assembles on the same patio */
    int guardPatio = bg == 0 && sBgLatch[bg].valid &&
        PortGuardArmorBackdropMatches((const void* const*)sBgLatch[bg].map) &&
        !sEnvNoGuardPatio; /* isolated before/after harness */
    /* Trickmaster's chessboard below the map (the wide view's bottom band): the Preliminar V2 of the boss arenas,
       waiting for the user's approval (KHCOM_ARENAS_V2=1, the review copy) */
    int tmBoard = sBgLatch[bg].valid && PortTrickmasterBackdropMatches((const void* const*)sBgLatch[bg].map) &&
        PortArenasV2();
    /* Ursula's cave (1) goes on below its floor; her own layer (2) shows one Ursula */
    int ursLayer = PortArenasV2() && sBgLatch[bg].valid ? PortUrsulaLayer((const void* const*)sBgLatch[bg].map) : 0;
    /* Jafar's room (1) goes on by its periods; the genie's layer (2) shows one genie */
    int jfLayer = PortJafarWide() && sBgLatch[bg].valid ? PortJafarLayer((const void* const*)sBgLatch[bg].map) : 0;
    /* Marluxia's last fight: the field (BG0) wraps beyond the picture as on the hardware, the body (BG1) once */
    int mxField = PortMarluxiaExtendOn() && (bg == 0 || bg == 1);
    /* Oogie's room goes on as one bay of a long hall */
    int oogieHall = PortArenasV2() && sBgLatch[bg].valid && PortOogieLayer((const void* const*)sBgLatch[bg].map);
    /* Maleficent's hall (1) goes on by its bays; her dragon's layer (2) shows one dragon */
    int mdLayer = 0;
    if (PortArenasV2() && sBgLatch[bg].valid) {
        int b;
        if (PortMaleficentLayer((const void* const*)sBgLatch[bg].map))
            mdLayer = 1;
        else if (PortMaleficentDragonLayer((const void* const*)sBgLatch[bg].map))
            for (b = 0; b < 4; b++)
                if (b != bg && sBgLatch[b].valid && PortMaleficentLayer((const void* const*)sBgLatch[b].map))
                    mdLayer = 2;
    }

    (void)dispcnt;
    if (mosV > 1)
        gy -= ((gy % mosV) + mosV) % mosV;
    py = (gy + vofs) & hmask;
    ty = py >> 3;
    fy = py & 7;
    outside = gy < 0 || gy >= GBA_SCREEN_H;
    if (sSpotlight && bg == 0) {
        /* Extend the authored shade, never the unused VRAM padding. */
        py = ((gy < 0 ? 0 : gy >= GBA_SCREEN_H ? GBA_SCREEN_H - 1 : gy) + vofs) & hmask;
        ty = py >> 3;
        fy = py & 7;
        outside = 0;
    }

    for (x = 0; x < w; x++) {
        int gx = x - gPortScreen.originX - sBgShiftLatched[bg];
        int px, tx, block, entry, tile, pal, fx, fyy, ci;
        const uint16_t* map;

        if (x - gPortScreen.originX < sLeft || x - gPortScreen.originX >= sRight) {
            out[x] = TRANSPARENT;
            continue;
        }
        if (sSpotlight && bg == 0)
            gx = (gx < 0 ? 0 : gx >= GBA_SCREEN_W ? GBA_SCREEN_W - 1 : gx);
        if (mosH > 1)
            gx -= ((gx % mosH) + mosH) % mosH;
        if (gPortScreen.wide && bg == 3 && sBgLatch[3].valid &&
            sPreliminarRecipe[3] && sPreliminarRecipe[3]->v[0] == 102 &&
            (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[3];
            int wx = l->x + gx + (hofs - l->hofs);
            int wy = l->y + gy + (vofs - l->vofs);
            TiledBackdropCoordinates(3, &wx, &wy);
            out[x] = Preliminar102Underlay(chars, bpp8, wx, wy);
            sInWorld[x] = 1;
            continue;
        }
        if (guardPatio && (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs);
            int wy = l->y + gy + (vofs - l->vofs);
            if (PortGuardArmorBackdropPixel(wx, wy, &out[x])) {
                sInWorld[x] = 1;
                continue;
            }
        }
        if (ursLayer && (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
            int cx = l->x + GBA_SCREEN_W / 2 + (hofs - l->hofs), cy = l->y + GBA_SCREEN_H / 2 + (vofs - l->vofs);
            if (ursLayer == 1 && gy >= GBA_SCREEN_H && PortUrsulaFloorPixel(wx, wy, cy, &out[x])) {
                sInWorld[x] = 1;
                continue;
            }
            if (ursLayer == 2 && !PortMaleficentDragonVisible(wx, wy, cx, cy)) { /* the nearest copy only */
                out[x] = TRANSPARENT;
                continue;
            }
        }
        if (mdLayer && (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
            int cx = l->x + GBA_SCREEN_W / 2 + (hofs - l->hofs), cy = l->y + GBA_SCREEN_H / 2 + (vofs - l->vofs);
            if (mdLayer == 1 && PortMaleficentHallPixel(wx, wy, cx, cy, &out[x])) {
                sInWorld[x] = 1;
                continue;
            }
            if (mdLayer == 2 && !PortMaleficentDragonVisible(wx, wy, cx, cy)) {
                out[x] = TRANSPARENT;
                continue;
            }
        }
        if (mxField && (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            if (bg == 0)
                goto picture;
            entry = PortMarluxiaBodyEntry(gx, gy, hofs, vofs);
            if (entry <= 0) { /* beside the body, or a cell the game left empty */
                out[x] = TRANSPARENT;
                continue;
            }
            tile = entry & 0x3FF;
            fx = (entry & 0x400) ? 7 - ((gx + hofs) & 7) : ((gx + hofs) & 7);
            fyy = (entry & 0x800) ? 7 - fy : fy;
            goto pixel;
        }
        if (jfLayer && (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
            int cx = l->x + GBA_SCREEN_W / 2 + (hofs - l->hofs), cy = l->y + GBA_SCREEN_H / 2 + (vofs - l->vofs);
            if (jfLayer == 1 && PortJafarHallPixel(wx, wy, cx, cy, &out[x])) {
                sInWorld[x] = 1;
                continue;
            }
            if (jfLayer == 2 && (!PortJafarGenieVisible(wx, wy, cx, cy) || PortJafarGenieCleared(wx, wy))) {
                out[x] = TRANSPARENT;
                continue;
            }
        }
        if (oogieHall && (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
            int cx = l->x + GBA_SCREEN_W / 2 + (hofs - l->hofs), cy = l->y + GBA_SCREEN_H / 2 + (vofs - l->vofs);
            if (PortOogieHallPixel(wx, wy, cx, cy, &out[x])) {
                sInWorld[x] = 1;
                continue;
            }
        }
        if (tmBoard && gy >= GBA_SCREEN_H) { /* below the picture only: above it the map goes on as it is */
            const BgLatch* l = &sBgLatch[bg];
            if (PortTrickmasterBackdropPixel(l->x + gx + (hofs - l->hofs), l->y + gy + (vofs - l->vofs), &out[x])) {
                sInWorld[x] = 1;
                continue;
            }
        }
        if (sDarksideLayer[bg] && (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
            if (sDarksideLayer[bg] == 1 ? PortDarksideShorePixel(wx, wy, &out[x])
                                        : PortDarksideBodyPixel((const void* const*)l->map, wx, wy,
                                                                l->x + GBA_SCREEN_W / 2, l->y + GBA_SCREEN_H / 2,
                                                                &out[x])) {
                sInWorld[x] = 1;
                continue;
            }
        }
        if (bg == sSinglePanelBg && sBgLatch[bg].valid &&
            (outside || gx < 0 || gx >= GBA_SCREEN_W)) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
            if (wx < 0 || wy < 0 || wx >= 256 || wy >= 256) {
                sInWorld[x] = 1;
                out[x] = SinglePanelColor(l, cnt, wx, wy);
                continue;
            }
        }
        if ((outside || gx < 0 || gx >= GBA_SCREEN_W) && mosH == 1 &&
            !sScreenBg[bg] && !sBgDockedLatched[bg] &&
            (sBgLatch[bg].valid || sBgWorldLatched[bg].valid)) {
            const BgLatch* l = &sBgLatch[bg];
            const BgWorld* wd = &sBgWorldLatched[bg];
            int wx = l->valid ? l->x + gx + (hofs - l->hofs) : wd->x + gx + (hofs - wd->hofs);
            int wy = l->valid ? l->y + gy + (vofs - l->vofs) : wd->y + gy + (vofs - wd->vofs);
            if (sPreliminarRecipe[bg] &&
                ((bg==2 && (sPreliminarRecipe[bg]->v[0]==44 || sPreliminarRecipe[bg]->v[0]==45)) ||
                 sPreliminarRecipe[bg]->v[0]==95 || sPreliminarRecipe[bg]->v[0]==99 || (bg==2 && sPreliminarRecipe[bg]->v[0]==104) ||
                 sPreliminarRecipe[bg]->v[0]==110 ||
                 sPreliminarRecipe[bg]->v[0]==121 || sPreliminarRecipe[bg]->v[0]==122 ||
                 sPreliminarRecipe[bg]->v[0]==124)) {
                int sx,sy;
                if (sPreliminarRecipe[bg]->v[0]==45 && PreliminarFirPixel(wx,wy,&out[x])) {
                    sInWorld[x]=1;
                    continue;
                }
                if (PortPreliminarPixelSource(sPreliminarRecipe[bg],wx,wy,&sx,&sy)) {
                    int index=SinglePanelIndex(l,cnt,sx,sy);
                    out[x]=index>=0 && (index&15)?PLTT16[index]&0x7fff:TRANSPARENT;
                    sInWorld[x]=1;
                    continue;
                }
                goto scalar_world;
            }
            if (sAgrabah109Layer[bg] == 2) {
                if (Agrabah109DecorPixel(l, chars, wx, wy, &out[x])) {
                    sInWorld[x] = 1;
                    continue;
                }
                /* A prop's transparent silhouette can start within a tile. */
                goto scalar_world;
            }
            /* Reflected bands can change direction within a tile: retain the
               scalar sampler there rather than assuming a forward tile run. */
            if (bg == sTiledBackdropBg) goto scalar_world;
            int run = 8 - (wx & 7), we, j;
            if (run > w - x) run = w - x;
            if (run > sRight - (x - gPortScreen.originX)) run = sRight - (x - gPortScreen.originX);
            if (!outside && gx < 0 && run > -gx) run = -gx;
            we = l->valid ? ExtendedWorldEntry(l, wx, wy) : wd->fn(bg, wx >> 3, wy >> 3);
            if (we < 0) {
                for (j = 0; j < run; j++) out[x + j] = sWorldPadding[bg];
                x += run - 1;
                continue;
            }
            /* World extensions sample one tile row at a time. UI glyphs
               keep the scalar path, including its exact registration order. */
            {
                unsigned base = ((cnt >> 2) & 3) * 0x4000;
                unsigned off = (we & 1023) * (bpp8 ? 64 : 32);
                if (bpp8 || !sHdText || base + off >= 0x10000 ||
                    sBgTextTiles[(base + off) / 32].code <= 0x20) {
                    int yy = (we & 0x800) ? 7 - (wy & 7) : wy & 7;
                    int palette = (we >> 12) * 16;
                    off += yy * (bpp8 ? 8 : 4);
                    memset(sInWorld + x, 1, run);
                    if (base + off + (bpp8 ? 8 : 4) > 0x10000) {
                        for (j = 0; j < run; j++) out[x + j] = TRANSPARENT;
                    } else {
                        int xx = (we & 0x400) ? 7 - (wx & 7) : wx & 7;
                        int step = (we & 0x400) ? -1 : 1;
                        const uint8_t* pixels = chars + off;
                        if (bpp8) {
                            for (j = 0; j < run; j++, xx += step) {
                                int c = pixels[xx];
                                out[x + j] = c ? PLTT16[c] & 0x7FFF : TRANSPARENT;
                            }
                        } else {
                            const uint16_t* colors = PLTT16 + palette;
                            for (j = 0; j < run; j++, xx += step) {
                                int c = (pixels[xx >> 1] >> ((xx & 1) * 4)) & 15;
                                out[x + j] = c ? colors[c] & 0x7FFF : TRANSPARENT;
                            }
                        }
                    }
                    x += run - 1;
                    continue;
                }
            }
        }
scalar_world:
        if (outside || gx < 0 || gx >= GBA_SCREEN_W) {
            /* Beyond the hardware picture: only map-block layers continue. */
            const BgLatch* l = &sBgLatch[bg];
            int we;

            if (sScreenBg[bg] || sBgDockedLatched[bg] || (!l->valid && !sBgWorldLatched[bg].valid)) {
                out[x] = TRANSPARENT;
                continue;
            }
            if (!l->valid) {
                const BgWorld* wd = &sBgWorldLatched[bg];
                int wx = wd->x + gx + (hofs - wd->hofs), wy = wd->y + gy + (vofs - wd->vofs);
                /* Adjacent pixels share a map entry. Cache only this row,
                   so HBlank scroll/VRAM changes are never carried forward. */
                if (!cachedValid || cachedTx != (wx >> 3) || cachedTy != (wy >> 3)) {
                    cachedTx = wx >> 3; cachedTy = wy >> 3;
                    cachedEntry = wd->fn(bg, cachedTx, cachedTy);
                    cachedValid = 1;
                }
                we = cachedEntry;
                if (we < 0) {
                    out[x] = TRANSPARENT;
                    continue;
                }
                sInWorld[x] = 1;
                entry = we;
                px = wx & 7;
                fy = wy & 7;
                tile = entry & 0x3FF;
                fx = (entry & 0x400) ? 7 - px : px;
                fyy = (entry & 0x800) ? 7 - fy : fy;
                fy = py & 7;
                goto pixel;
            }
            {
                int wx = l->x + gx + (hofs - l->hofs), wy = l->y + gy + (vofs - l->vofs);
                TiledBackdropCoordinates(bg, &wx, &wy);
                if (!cachedValid || cachedTx != (wx >> 3) || cachedTy != (wy >> 3)) {
                    cachedTx = wx >> 3; cachedTy = wy >> 3;
                    cachedEntry = ExtendedWorldEntry(l, wx, wy);
                    cachedValid = 1;
                }
                we = cachedEntry;
                if (we < 0) {
                    if (gPortScreen.wide && bg == 3 && sPreliminarRecipe[bg] &&
                        sPreliminarRecipe[bg]->v[0] == 102)
                        out[x] = Preliminar102Underlay(chars, bpp8, wx, wy);
                    else
                        out[x] = sWorldPadding[bg];
                    continue;
                }
                px = wx & 7; fy = wy & 7;
            }
            sInWorld[x] = 1;
            entry = we;
            tile = entry & 0x3FF;
            fx = (entry & 0x400) ? 7 - px : px;
            fyy = (entry & 0x800) ? 7 - fy : fy;
            fy = py & 7;
            goto pixel;
        }
    picture:
        px = (gx + hofs) & wmask;
        tx = px >> 3;
        block = 0;
        if (tx >= 32)
            block += 1;
        if (ty >= 32)
            block += (size == 3) ? 2 : 1;
        map = screen + block * 0x400;
        entry = map[(ty & 31) * 32 + (tx & 31)];
        tile = entry & 0x3FF;
        fx = (entry & 0x400) ? 7 - (px & 7) : (px & 7);
        fyy = (entry & 0x800) ? 7 - fy : fy;
    pixel:
        if (bpp8) {
            uint32_t off = tile * 64 + fyy * 8 + fx;
            ci = (((cnt >> 2) & 3) * 0x4000 + off < 0x10000) ? chars[off] : 0;
            out[x] = ci ? (PLTT16[ci] & 0x7FFF) : TRANSPARENT;
        } else {
            uint32_t off = tile * 32 + fyy * 4 + (fx >> 1);
            uint8_t b = (((cnt >> 2) & 3) * 0x4000 + off < 0x10000) ? chars[off] : 0;
            ci = (fx & 1) ? (b >> 4) : (b & 15);
            pal = entry >> 12;
            out[x] = ci ? (PLTT16[pal * 16 + ci] & 0x7FFF) : TRANSPARENT;
            if (sHdText && !(entry & 0xC00)) {
                unsigned offset = ((cnt >> 2) & 3) * 0x4000 + tile * 32;
                if (offset < 0x10000) {
                    const BgTextTile* text = BgTextTileAt(offset);
                    if (text->code > 0x20 && !memcmp(text->source, VRAM8 + offset, 32)) {
                        if ((px & 7) == 0 && ((py & 7) == 0 || gy == 0) && BgTextRoom()) {
                            BgTextGlyph* glyph = BgTextNew();
                            int xx = x - (px & 7), yy = gy - (py & 7);
                            glyph->bg = (uint8_t)bg;
                            glyph->priority = cnt & 3;
                            glyph->screen = sOverlay && sScreenBg[bg];
                            if (glyph->screen) {
                                xx -= gPortScreen.originX;
                                if (sBgSplit[bg]) xx += xx < sBgSplit[bg] ? -PortScreenMarginX() : PortScreenMarginX();
                                xx += (gPortOverlayW - GBA_SCREEN_W) / 2;
                            } else yy += gPortScreen.originY;
                            memset(&glyph->glyph, 0, sizeof(glyph->glyph));
                            glyph->glyph.x = xx; glyph->glyph.y = yy;
                            glyph->glyph.code = text->code;
                            glyph->glyph.style = text->style;
                            glyph->glyph.cap = text->cap;
                            glyph->glyph.base = text->base;
                            /* Numeric BG cells use the same 13/14/15 panel,
                               shadow and face convention as EXP cells. */
                            {
                                /* the face is the lighter of the digit's two
                                   colours in this cell's palette (greyed-out
                                   panels use another palette) */
                                uint16_t a = PLTT16[pal * 16 + text->faceIndex] & 0x7FFF;
                                uint16_t b = PLTT16[pal * 16 + text->shadowIndex] & 0x7FFF;
                                int la = (a & 31) * 2 + ((a >> 5) & 31) * 5 + ((a >> 10) & 31);
                                int lb = (b & 31) * 2 + ((b >> 5) & 31) * 5 + ((b >> 10) & 31);
                                glyph->glyph.face = la >= lb ? a : b;
                                glyph->glyph.shadow = la >= lb ? b : a;
                            }
                        }
                        /* the pixel-art digit gives way to the panel colour, so
                           the layer keeps covering its cell and the occlusion
                           test still finds this panel in front */
                        if (ci == text->shadowIndex || ci == text->faceIndex)
                            out[x] = text->panelIndex ? (PLTT16[pal * 16 + text->panelIndex] & 0x7FFF) : TRANSPARENT;
                    }
                }
            }
        }
        if (out[x] == TRANSPARENT && gPortScreen.wide && bg == 3 &&
            sPreliminarRecipe[bg] && sPreliminarRecipe[bg]->v[0] == 102 &&
            (outside || gx < 0 || gx >= GBA_SCREEN_W) && sBgLatch[bg].valid) {
            const BgLatch* l = &sBgLatch[bg];
            int wx = l->x + gx + (hofs - l->hofs);
            int wy = l->y + gy + (vofs - l->vofs);
            TiledBackdropCoordinates(bg, &wx, &wy);
            out[x] = Preliminar102Underlay(chars, bpp8, wx, wy);
        }
    }
}

/* Affine BG mosaic: the reference point of the first line of each block of
   mosV lines, which the following lines of the block reuse. */
static PPU_TLS int32_t sMosAffX[2], sMosAffY[2];
/* The line after the last one drawn: a frame's first line (the wide view's rows above the GBA picture start inside
   a block) takes its own reference, never the one a previous drawn frame left (with frame skipping, an older
   frame's: the card-break mosaic's top rows, issue #43). */
static PPU_TLS int sMosNextGy[2] = { 0x7FFFFFFF, 0x7FFFFFFF };
static int MosaicBlockStart(int a, int gy, int mosV) {
    int start = ((gy % mosV) + mosV) % mosV == 0 || gy != sMosNextGy[a];
    sMosNextGy[a] = gy + 1;
    return start;
}

/* An affine effect layer shows one copy on the GBA's 240 columns; beyond them only the copies the original picture
   shows are drawn, whether the layer wraps or the wide view repeats the arena sideways (Darkside's smoke was three
   clouds at 32:9). a, b: the texture x at the picture's edges. */
static int AffineCopyShown(int px, int a, int b, int size) {
    /* each texture pixel once: in the copy nearest the middle of the original picture */
    int d = 2 * px - (a + b);
    return d >= -size && d < size;
}
/* Layers that wrap on the GBA only by chance: one copy in the wide view. */
static int AffineSingleCopy(int bg) {
    return bg == 2 && sDarksideLayer[0] == 1;
}

/* Axis-aligned affine stages reuse their horizontal address calculation
   between scanlines. Cache coordinates only: VRAM and palette are still read
   on each line, so HBlank changes never reuse stale tile pixels or colours. */
static void DrawStraightAffine(int bg, int gy, uint16_t cnt, int32_t refX, int32_t refY) {
    static PPU_TLS int keys[2][14], valid[2];
    static PPU_TLS int16_t columns[2][PORT_MAX_W];
    int key[14], a = bg - 2, w = gPortScreen.width;
    int size = 128 << (cnt >> 14), tiles = size >> 3, wrap = (cnt >> 13) & 1;
    int py = refY >> 8, ornaments, single, x, i, lastTile = -1;
    uint16_t colors[8];
    uint16_t* out = sBgLine[bg];
    const uint8_t* chars = VRAM8 + ((cnt >> 2) & 3) * 0x4000;
    const uint8_t* screen = VRAM8 + ((cnt >> 8) & 31) * 0x800;

    if (sBattleView && gPortScreen.wide && ((cnt >> 2) & 3) == 0 &&
        ((cnt >> 8) & 31) == 12 && size >= 512 && gy >= GBA_SCREEN_H && py >= size)
        py = size - 1;
    ornaments = sBattleView && sArenaBands[1] && size >= 512 &&
                ((cnt >> 2) & 3) == 0 && ((cnt >> 8) & 31) == 12 && py >= 0 && py < size;
    if (wrap) py &= size - 1;
    else if (py < 0 || py >= size) {
        for (x = 0; x < w; x++) out[x] = TRANSPARENT;
        return;
    }
    key[0] = refX; key[1] = w; key[2] = gPortScreen.originX;
    key[3] = sLeft; key[4] = sRight; key[5] = cnt;
    single = AffineSingleCopy(bg);
    key[6] = sBattleView; key[7] = ornaments | single << 1;
    for (i = 0; i < 6; i++) key[8 + i] = sArenaBands[i];
    if (!valid[a] || memcmp(keys[a], key, sizeof(key))) {
        for (x = 0; x < w; x++) {
            int gx = x - gPortScreen.originX;
            int px = (refX + 256 * gx) >> 8;
            if (gx < sLeft || gx >= sRight) { columns[a][x] = -1; continue; }
            if (single && (gx < 0 || gx >= GBA_SCREEN_W) &&
                !AffineCopyShown(px, refX >> 8, (refX + 256 * (GBA_SCREEN_W - 1)) >> 8, size)) {
                columns[a][x] = -1;
                continue;
            }
            if (ornaments && (gx < 0 || gx >= GBA_SCREEN_W)) {
                if (px < sArenaBands[0])
                    px = ArenaBandPixel(sArenaBands[0] - 1 - px, sArenaBands[2], sArenaBands[3], 0);
                else if (px >= sArenaBands[1])
                    px = ArenaBandPixel(px - sArenaBands[1], sArenaBands[4], sArenaBands[5], 1);
            }
            if (wrap || ((px < 0 || px >= size) && size >= 512 && (gx < 0 || gx >= GBA_SCREEN_W)))
                px &= size - 1;
            columns[a][x] = (px >= 0 && px < size) ? (int16_t)px : -1;
        }
        memcpy(keys[a], key, sizeof(key)); valid[a] = 1;
    }
    screen += (py >> 3) * tiles;
    for (x = 0; x < w; x++) {
        int px = columns[a][x], tile;
        if (px < 0) { out[x] = TRANSPARENT; continue; }
        sInWorld[x] = 1;
        tile = screen[px >> 3];
        if (tile != lastTile) {
            const uint8_t* pixels = chars + tile * 64 + (py & 7) * 8;
            for (i = 0; i < 8; i++) colors[i] = pixels[i] ? PLTT16[pixels[i]] & 0x7FFF : TRANSPARENT;
            lastTile = tile;
        }
        out[x] = colors[px & 7];
    }
}

static void DrawAffineBg(int bg, int gy, uint16_t dispcnt) {
    int a = bg - 2;
    uint16_t cnt = IO16(0x08 + bg * 2);
    int16_t pa = (int16_t)IO16(0x20 + a * 0x10);
    int16_t pc = (int16_t)IO16(0x24 + a * 0x10);
    const uint8_t* chars = VRAM8 + ((cnt >> 2) & 3) * 0x4000;
    const uint8_t* screen = VRAM8 + ((cnt >> 8) & 31) * 0x800;
    int size = 128 << (cnt >> 14);
    int tiles = size >> 3;
    int wrap = (cnt >> 13) & 1;
    uint16_t* out = sBgLine[bg];
    int x, w = gPortScreen.width;
    int mosaic = (cnt >> 6) & 1;
    int mosH = mosaic ? (IO16(0x4C) & 15) + 1 : 1;
    int mosV = mosaic ? ((IO16(0x4C) >> 4) & 15) + 1 : 1;
    int32_t refX = sAffX[a], refY = sAffY[a];
    int32_t rx, ry;

    (void)dispcnt;
    if (mosV > 1) {
        if (MosaicBlockStart(a, gy, mosV)) {
            sMosAffX[a] = refX;
            sMosAffY[a] = refY;
        }
        refX = sMosAffX[a];
        refY = sMosAffY[a];
    }
    if (pa == 256 && pc == 0 && mosH == 1) {
        DrawStraightAffine(bg, gy, cnt, refX, refY);
        return;
    }
    rx = refX + (int32_t)pa * (-gPortScreen.originX);
    ry = refY + (int32_t)pc * (-gPortScreen.originX);
    for (x = 0; x < w; x++, rx += pa, ry += pc) {
        int gx = x - gPortScreen.originX;
        int px, py;
        int tile, ci;

        if (mosH > 1) { /* every block of mosH pixels shows its first one */
            int gq = gx - ((gx % mosH) + mosH) % mosH;
            px = (refX + (int32_t)pa * gq) >> 8;
            py = (refY + (int32_t)pc * gq) >> 8;
        } else {
            px = rx >> 8;
            py = ry >> 8;
        }

        if (gx < sLeft || gx >= sRight) {
            out[x] = TRANSPARENT;
            continue;
        }
        /* Outside the original picture, continue the bottom of the arena
           in texture space. Composition still handles palette zero, fades,
           masks and sprites normally; no framebuffer colours are guessed. */
        if (sBattleView && gPortScreen.wide && ((cnt >> 2) & 3) == 0 && ((cnt >> 8) & 31) == 12 && size >= 512 &&
            gy >= GBA_SCREEN_H && py >= size)
            py = size - 1;
        /* Continue authored arena ornaments only in the wide margins. The
           original hardware viewport, camera and collision remain exact.
           Reflecting each short side band joins its edge without bringing
           padding or unique centre landmarks into the added view. */
        if (sBattleView && sArenaBands[1] && size >= 512 &&
            ((cnt >> 2) & 3) == 0 && ((cnt >> 8) & 31) == 12 &&
            (gx < 0 || gx >= GBA_SCREEN_W) && py >= 0 && py < size) {
            if (px < sArenaBands[0])
                px = ArenaBandPixel(sArenaBands[0] - 1 - px,
                                    sArenaBands[2], sArenaBands[3], 0);
            else if (px >= sArenaBands[1])
                px = ArenaBandPixel(px - sArenaBands[1],
                                    sArenaBands[4], sArenaBands[5], 1);
        }
        if (AffineSingleCopy(bg) && (gx < 0 || gx >= GBA_SCREEN_W) &&
            !AffineCopyShown(px, refX >> 8, (refX + (int32_t)pa * (GBA_SCREEN_W - 1)) >> 8, size)) {
            out[x] = TRANSPARENT;
            continue;
        }
        if (wrap) {
            px &= size - 1;
            py &= size - 1;
        } else if ((px < 0 || px >= size) && py >= 0 && py < size && size >= 512 && (gx < 0 || gx >= GBA_SCREEN_W)) {
            /* wide margins of a battle arena: the stage repeats sideways
               instead of ending in the void */
            px &= size - 1;
        } else if (px < 0 || py < 0 || px >= size || py >= size) {
            out[x] = TRANSPARENT;
            continue;
        }
        sInWorld[x] = 1;
        tile = screen[(py >> 3) * tiles + (px >> 3)];
        ci = chars[tile * 64 + (py & 7) * 8 + (px & 7)];
        out[x] = ci ? (PLTT16[ci] & 0x7FFF) : TRANSPARENT;
    }
}

static void DrawBitmapBg(int mode, int gy, uint16_t dispcnt) {
    int16_t pa = (int16_t)IO16(0x20);
    int16_t pc = (int16_t)IO16(0x24);
    uint16_t* out = sBgLine[2];
    int x, w = gPortScreen.width;
    int bw = (mode == 5) ? 160 : 240;
    int bh = (mode == 5) ? 128 : 160;
    uint32_t page = (mode != 3 && (dispcnt & 0x10)) ? 0xA000 : 0;
    int mosaic = (IO16(0x0C) >> 6) & 1;
    int mosH = mosaic ? (IO16(0x4C) & 15) + 1 : 1;
    int mosV = mosaic ? ((IO16(0x4C) >> 4) & 15) + 1 : 1;
    int32_t refX = sAffX[0], refY = sAffY[0], rx, ry;

    /* Bitmap modes use BG2's affine engine, including its screen-space
       mosaic. Reuse the first line's reference point within each block. */
    if (mosV > 1) {
        if (MosaicBlockStart(0, gy, mosV)) {
            sMosAffX[0] = refX;
            sMosAffY[0] = refY;
        }
        refX = sMosAffX[0];
        refY = sMosAffY[0];
    }
    rx = refX + (int32_t)pa * (-gPortScreen.originX);
    ry = refY + (int32_t)pc * (-gPortScreen.originX);

    for (x = 0; x < w; x++, rx += pa, ry += pc) {
        int gx = x - gPortScreen.originX;
        int px = rx >> 8, py = ry >> 8;

        if (mosH > 1) {
            int first = gx - ((gx % mosH) + mosH) % mosH;
            px = (refX + (int32_t)pa * first) >> 8;
            py = (refY + (int32_t)pc * first) >> 8;
        }

        if (gx < sLeft || gx >= sRight || px < 0 || py < 0 || px >= bw || py >= bh) {
            out[x] = TRANSPARENT;
            continue;
        }
        if (mode == 4) {
            int ci = VRAM8[page + py * 240 + px];
            out[x] = ci ? (PLTT16[ci] & 0x7FFF) : TRANSPARENT;
        } else {
            out[x] = *(const uint16_t*)(VRAM8 + page + (py * bw + px) * 2) & 0x7FFF;
        }
    }
}

/* ---- sprites --------------------------------------------------------------- */

static const uint8_t sObjSize[3][4][2] = {
    { { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 } },
    { { 16, 8 }, { 32, 8 }, { 32, 16 }, { 64, 32 } },
    { { 8, 16 }, { 8, 32 }, { 16, 32 }, { 32, 64 } },
};

/* Where an HD glyph's coverage is checked: a pixel of the glyph's own ink, the one nearest (4, 6). A fixed (4, 6)
   falls in the gap of letters such as V or T, where the sprite shows whatever lies behind it, so the glyph was
   taken for covered whenever that changed (the level-up message flickered while its effect animated). */
static void GlyphInkPoint(int i, int* dx, int* dy) {
    uint16_t a0 = OAM16[i * 4 + 0], a1 = OAM16[i * 4 + 1], a2 = OAM16[i * 4 + 2];
    int shape = a0 >> 14, w, h, x, y, best = 1 << 30;
    int mapping1d = (IO16(0) >> 6) & 1, tile = a2 & 0x3FF;

    *dx = 4;
    *dy = 6;
    if (shape == 3 || (a0 & 0x2000) || (a0 & 0x100)) /* 8bpp or affine: keep the old point */
        return;
    w = sObjSize[shape][a1 >> 14][0];
    h = sObjSize[shape][a1 >> 14][1];
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            int t = mapping1d ? tile + (y >> 3) * (w >> 3) + (x >> 3) : tile + (y >> 3) * 32 + (x >> 3);
            uint8_t b = VRAM8[0x10000 + ((t * 32 + (y & 7) * 4 + ((x & 7) >> 1)) & 0x7FFF)];
            int sx = (a1 & 0x1000) ? w - 1 - x : x, sy = (a1 & 0x2000) ? h - 1 - y : y;
            int d = (sx - 4) * (sx - 4) + (sy - 6) * (sy - 6);
            if (((x & 1) ? b >> 4 : b & 15) && d < best) {
                best = d;
                *dx = sx;
                *dy = sy;
            }
        }
}

/* Whether the sprite on top at a glyph's ink point hides HD glyph i. Another HD glyph does not: its pixels are
   replaced by the outline font, and the cartridge's proportional text lets neighbouring letter sprites overlap
   (the V of a level-up message covered part of the T next to it, so the T came and went as OAM order changed). */
static int ObjCoversGlyph(uint16_t occ, int prio, int i) {
    return occ < prio * 128 + i && !HdTextGlyph(occ & 127);
}

static void DrawObjs(int gy, uint16_t dispcnt) {
    int i, w = gPortScreen.width;
    /* Event 097 stages characters just beyond the cartridge's
       240x160 camera. Keep the extended backgrounds, but let those actors
       enter and leave through the original frame as authored. */
    int sceneActorFrame = gPortScreen.wide && sEventBackdropProfile == 97 && !sEnvActorFrameOff;
    int mapping1d = (dispcnt >> 6) & 1;
    int bitmapMode = (dispcnt & 7) >= 3;
    uint16_t mosaic = IO16(0x4C);
    int mosH = ((mosaic >> 8) & 15) + 1;
    int mosV = ((mosaic >> 12) & 15) + 1;

    sLineHasObjects = 0;
    for (i = 0; i < w; i++) {
        sObjLine[i] = TRANSPARENT;
        sObjPrio[i] = 4;
        sObjSemi[i] = 0;
        sScrLine[i] = TRANSPARENT;
        sScrPrio[i] = 4;
        sScrIndex[i] = 127;
        sObjWin[i] = 0;
    }
    if (!(dispcnt & 0x1000))
        return;

    for (i = 0; i < 128; i++) {
        uint16_t a0 = OAM16[i * 4 + 0];
        uint16_t a1 = OAM16[i * 4 + 1];
        uint16_t a2 = OAM16[i * 4 + 2];
        int affine = (a0 >> 8) & 1;
        int dbl = (a0 >> 9) & 1;
        int mode = (a0 >> 10) & 3;
        int shape = (a0 >> 14) & 3;
        int size = (a1 >> 14) & 3;
        int ow, oh, bw, bh, ox, oy, line, sx, x;
        int bpp8 = (a0 >> 13) & 1;
        int prio = (a2 >> 10) & 3;
        int tileBase = a2 & 0x3FF;
        int palBank = a2 >> 12;
        int16_t pa = 0x100, pb = 0, pc = 0, pd = 0x100;
        int objMosaic = (a0 >> 12) & 1;

        if (!affine && dbl)
            continue; /* disabled */
        if (shape == 3 || mode == 3)
            continue;
        if (sHudLayer && sOamTag[i].hud)
            continue; /* drawn by PortPpuRenderHud */
        if (HdTextGlyph(i) && sOamTag[i].style != 2)
            continue; /* drawn by port_hdtext.c */
        if (sMainHdPart[i] && !PortHdSpriteKeepsCartridge(sOamHd[i].id))
            continue; /* drawn by port_hdsprites.c over the picture */
        ow = sObjSize[shape][size][0];
        oh = sObjSize[shape][size][1];
        bw = ow << (affine && dbl);
        bh = oh << (affine && dbl);

        /* Decode the 8-bit Y and 9-bit X against the visible span. */
        oy = a0 & 0xFF;
        ox = a1 & 0x1FF;
        if ((sOamExt[i].x & 0x1FF) == ox && (sOamExt[i].y & 0xFF) == oy) {
            ox = sOamExt[i].x;
            oy = sOamExt[i].y;
        } else {
            if (oy >= sBottom)
                oy -= 256;
            if (ox >= sRight)
                ox -= 512;
        }
        if (ox >= sRight || ox + bw <= sLeft)
            continue;

        line = gy - oy;
        /* Reject lines outside the sprite before mosaic quantisation:
           truncating a negative remainder would turn -1 into line zero. */
        if (line < 0 || line >= bh)
            continue;
        if (objMosaic && mosV > 1)
            line -= line % mosV;
        if (bitmapMode && tileBase < 512)
            continue;

        if (affine) {
            const uint16_t* p = OAM16 + ((a1 >> 9) & 31) * 16;
            pa = (int16_t)p[3];
            pb = (int16_t)p[7];
            pc = (int16_t)p[11];
            pd = (int16_t)p[15];
        }

        for (sx = 0; sx < bw; sx++) {
            int gx = ox + sx;
            int fx = gx;
            int tx, ty, tile, ci, col;
            uint32_t off;

            if (gx < sLeft || gx >= sRight)
                continue;
            if (sceneActorFrame && !sOamTag[i].screen && !sOamTag[i].hud &&
                (gx < 0 || gx >= GBA_SCREEN_W || gy < 0 || gy >= GBA_SCREEN_H))
                continue;
            if (sOamTag[i].clipX0 < sOamTag[i].clipX1 &&
                (gx < sOamTag[i].clipX0 || gx >= sOamTag[i].clipX1))
                continue;
            x = gx + gPortScreen.originX;
            if (x < 0 || x >= w)
                continue;
            if (objMosaic && mosH > 1)
                fx = gx - (((gx % mosH) + mosH) % mosH);
            fx -= ox;

            if (affine) {
                int cx = fx - bw / 2, cy = line - bh / 2;
                tx = ((pa * cx + pb * cy) >> 8) + ow / 2;
                ty = ((pc * cx + pd * cy) >> 8) + oh / 2;
                if (tx < 0 || ty < 0 || tx >= ow || ty >= oh)
                    continue;
            } else {
                if (fx < 0 || fx >= ow)
                    continue;
                tx = (a1 & 0x1000) ? ow - 1 - fx : fx;
                ty = (a1 & 0x2000) ? oh - 1 - line : line;
            }

            if (bpp8) {
                if (mapping1d)
                    tile = tileBase + ((ty >> 3) * (ow >> 3) + (tx >> 3)) * 2;
                else
                    tile = tileBase + (ty >> 3) * 32 + (tx >> 3) * 2;
                off = 0x10000 + (tile & 0x3FF) * 32 + (ty & 7) * 8 + (tx & 7);
                ci = VRAM8[off];
                col = ci ? (PLTT16[256 + ci] & 0x7FFF) : -1;
            } else {
                if (mapping1d)
                    tile = tileBase + (ty >> 3) * (ow >> 3) + (tx >> 3);
                else
                    tile = tileBase + (ty >> 3) * 32 + (tx >> 3);
                off = 0x10000 + (tile & 0x3FF) * 32 + (ty & 7) * 4 + ((tx & 7) >> 1);
                ci = (tx & 1) ? (VRAM8[off] >> 4) : (VRAM8[off] & 15);
                col = ci ? (PLTT16[256 + palBank * 16 + ci] & 0x7FFF) : -1;
            }
            if (col < 0)
                continue;
            if (sOamTag[i].header && x < gPortScreen.originX) {
                /* off the GBA screen: the band, not hidden lettering */
                int band = HeaderBandColor(gy, x - gPortScreen.originX); /* headers are drawn at y 0 */
                if (band >= 0)
                    col = band;
            }
            if (HdTextGlyph(i) && sOamTag[i].style == 2 && (ci == 14 || ci == 15))
                col = PLTT16[256 + palBank * 16 + 13] & 0x7FFF; /* Refill old ink with the blue panel, not transparency. */
            if (mode == 2) {
                sObjWin[x] = 1;
                continue;
            }
            if (sOverlay && sOamTag[i].screen) {
                if (sScrLine[x] == TRANSPARENT || prio < sScrPrio[x]) {
                    sScrLine[x] = (uint16_t)col;
                    sScrPrio[x] = (uint8_t)prio;
                    sScrIndex[x] = (uint8_t)i;
                    sScrSemi[x] = (mode == 1);
                }
            } else if (sObjLine[x] == TRANSPARENT || prio < sObjPrio[x]) {
                sLineHasObjects = 1;
                sObjLine[x] = (uint16_t)col;
                sObjPrio[x] = (uint8_t)prio;
                sObjIndex[x] = (uint8_t)i;
                sObjSemi[x] = (mode == 1);
            }
        }
    }
}

/* ---- windows --------------------------------------------------------------- */

static void WinSpan(uint16_t reg, int limit, int lo, int hi, int* a, int* b, int* wrapped) {
    int s = reg >> 8, e = reg & 0xFF;

    *wrapped = 0;
    if (e > limit || s > e) {
        if (s > e && e <= limit) {
            *wrapped = 1; /* covers [s, limit) and [0, e) */
        } else {
            e = limit;
        }
    }
    /* A window spanning the whole original screen spans the wide view too. */
    if (!*wrapped && s == 0 && e >= limit) {
        s = lo;
        e = hi;
    }
    *a = s;
    *b = e;
}

static int InWin(int v, int a, int b, int wrapped, int limit, int lo, int hi) {
    if (wrapped)
        return (v >= a && v < hi) || (v < b && v >= lo) || (a >= limit && v >= limit);
    return v >= a && v < b;
}

static void BuildWindows(int gy, uint16_t dispcnt) {
    int w = gPortScreen.width;
    int x;
    uint16_t winin = IO16(0x48), winout = IO16(0x4A);
    int win0 = (dispcnt >> 13) & 1, win1 = (dispcnt >> 14) & 1, winObj = (dispcnt >> 15) & 1;
    int in0 = 0, in1 = 0;
    int a0 = 0, b0 = 0, w0 = 0, a1 = 0, b1 = 0, w1 = 0;

    if (!win0 && !win1 && !winObj) {
        memset(sWinMask, 0x3F, w);
        return;
    }
    if (win0) {
        int ya, yb, yw;
        WinSpan(IO16(0x44), GBA_SCREEN_H, sTop, sBottom, &ya, &yb, &yw);
        in0 = InWin(gy, ya, yb, yw, GBA_SCREEN_H, sTop, sBottom);
        WinSpan(IO16(0x40), GBA_SCREEN_W, sLeft, sRight, &a0, &b0, &w0);
    }
    if (win1) {
        int ya, yb, yw;
        WinSpan(IO16(0x46), GBA_SCREEN_H, sTop, sBottom, &ya, &yb, &yw);
        in1 = InWin(gy, ya, yb, yw, GBA_SCREEN_H, sTop, sBottom);
        WinSpan(IO16(0x42), GBA_SCREEN_W, sLeft, sRight, &a1, &b1, &w1);
    }
    for (x = 0; x < w; x++) {
        int gx = x - gPortScreen.originX;
        uint8_t m = winout & 0x3F;

        if (winObj && sObjWin[x])
            m = (winout >> 8) & 0x3F;
        if (in1 && InWin(gx, a1, b1, w1, GBA_SCREEN_W, sLeft, sRight))
            m = (winin >> 8) & 0x3F;
        if (in0 && InWin(gx, a0, b0, w0, GBA_SCREEN_W, sLeft, sRight))
            m = winin & 0x3F;
        sWinMask[x] = m;
    }
}

/* ---- composition ----------------------------------------------------------- */

static uint16_t Blend(uint16_t a, uint16_t b, int ea, int eb) {
    int r = ((a & 31) * ea + (b & 31) * eb) >> 4;
    int g = (((a >> 5) & 31) * ea + ((b >> 5) & 31) * eb) >> 4;
    int bl = (((a >> 10) & 31) * ea + ((b >> 10) & 31) * eb) >> 4;

    if (r > 31)
        r = 31;
    if (g > 31)
        g = 31;
    if (bl > 31)
        bl = 31;
    return (uint16_t)(r | (g << 5) | (bl << 10));
}

static uint16_t Brighten(uint16_t c, int ey) {
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;

    r += ((31 - r) * ey) >> 4;
    g += ((31 - g) * ey) >> 4;
    b += ((31 - b) * ey) >> 4;
    return (uint16_t)(r | (g << 5) | (b << 10));
}

static uint16_t Darken(uint16_t c, int ey) {
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;

    r -= (r * ey) >> 4;
    g -= (g * ey) >> 4;
    b -= (b * ey) >> 4;
    return (uint16_t)(r | (g << 5) | (b << 10));
}

static void BuildNarrowSides(int gy, uint16_t dispcnt, const int* bgOn, const int* bgPrio);

/* The approved study: six native pixels, original palette only. White is
   entry31 in both regional title palettes. Use its LIVE value so palette
   fades/brightness remain coherent. Cache only while that palette is stable. */
static void TitleArtFadeLine(int gy, int bg) {
    static const int ramp[7] = {0, 19, 66, 128, 189, 236, 255};
    int x, by;
    if (!sTitleArtFade || gy < 0 || gy >= GBA_SCREEN_H) return;
    by = gy > 153 ? gy - 153 : 0;
    for (x = 0; x < gPortScreen.width; x++) {
        int gx = x - gPortScreen.originX, rx, code, c, index, i, strength, target[3], best, distance;
        if (gx < 0 || gx >= GBA_SCREEN_W) continue;
        rx = gx > 233 ? gx - 233 : 0;
        if (!rx && !by) continue;
        c = sBgLine[bg][x];
        if (c == TRANSPARENT) continue;
        for (index = 0; index < 256 && (sTitlePalette[index] & 0x7FFF) != c; index++) {}
        if (index == 256) continue;
        code = by * 7 + rx;
        if (!sTitleFadeReady[code][index]) {
            strength = 255 - ((255-ramp[rx])*(255-ramp[by]) + 127)/255;
            for (i = 0; i < 3; i++) {
                int a = (c >> (i*5)) & 31, white = (sTitlePalette[31] >> (i*5)) & 31;
                target[i] = a * (255-strength) + white * strength;
            }
            best = 0; distance = 0x7FFFFFFF;
            for (i = 0; i < 256; i++) {
                int p = sTitlePalette[i], r = (p & 31)*255-target[0];
                int g = ((p>>5)&31)*255-target[1], b = ((p>>10)&31)*255-target[2];
                int d = r*r + g*g + b*b;
                if (d < distance) {best = i; distance = d;}
            }
            sTitleFadeColors[code][index] = sTitlePalette[best] & 0x7FFF;
            sTitleFadeReady[code][index] = 1;
        }
        sBgLine[bg][x] = sTitleFadeColors[code][index];
    }
}

static void ComposeLine(int y, uint16_t dispcnt, const int* bgOn, const int* bgPrio, uint32_t* frame, int objects) {
    int w = gPortScreen.width;
    uint32_t* dst = frame + y * w;
    uint16_t bldcnt = IO16(0x50), bldalpha = IO16(0x52);
    int eva = bldalpha & 31, evb = (bldalpha >> 8) & 31;
    int evy = IO16(0x54) & 31, effect = (bldcnt >> 6) & 3;
    uint16_t backdrop = PLTT16[0] & 0x7FFF;
    int order[4], n = 0, p, b, x, docked = 0;
    /* Wide margins where no layer draws: the room's own void colour (a map's
       uniform padding), not the system backdrop. A room can end inside its
       map with nothing past it, which showed black or white beside the void
       the rest of the margin has (#56). The GBA's 240x160 is untouched. */
    uint16_t marginFill = TRANSPARENT;
    int gyLine = y - gPortScreen.originY;
    int collect = objects && (sHudLayer || sHdText || sMainHdAny || PortHd2dActive());
    int scene102Fill = gPortScreen.wide && !sBattleView && sBgLatch[3].valid &&
        sPreliminarRecipe[3] && sPreliminarRecipe[3]->v[0] == 102;
    int row = y * w;

    if (eva > 16) eva = 16;
    if (evb > 16) evb = 16;
    if (evy > 16) evy = 16;
    for (p = 0; p < 4; p++)
        for (b = 0; b < 4; b++)
            if (bgOn[b] && !sScreenBg[b] && bgPrio[b] == p)
                order[n++] = b;
    if (gPortScreen.wide && !sBattleView)
        for (p = n - 1; p >= 0 && marginFill == TRANSPARENT; p--)
            marginFill = sWorldPadding[order[p]];
    if (collect)
        for (b = 0; b < 4; b++)
            if (bgOn[b] && sBgDockedLatched[b]) docked |= 1 << b;

    /* Most extra world scanlines contain only opaque backgrounds. Windows,
       object alpha, docked messages and colour effects retain the full path.
       Keep priority metadata identical for HUD and HD text consumers. */
    if (gPortScreen.wide && !(dispcnt & 0xE000) &&
        (!objects || !sLineHasObjects) && !docked &&
        (effect == 0 || !(bldcnt & 0x3F)) &&
        !(sOverlay && sSpotlight && (sScreenBgMask & 2u))) {
        for (x = 0; x < w; x++) {
            int bg = -1;
            for (b = 0; b < n; b++) {
                int candidate = order[b];
                if (sBgLine[candidate][x] != TRANSPARENT) { bg = candidate; break; }
            }
            int gx = x - gPortScreen.originX;
            int margin = gx < 0 || gx >= GBA_SCREEN_W || gyLine < 0 || gyLine >= GBA_SCREEN_H;
            dst[x] = sColorLut[bg >= 0 ? sBgLine[bg][x] :
                               scene102Fill && margin ? Preliminar102ViewUnderlay(gx, gyLine) :
                               margin && marginFill != TRANSPARENT ? marginFill : sInWorld[x] ? backdrop : 0];
            if (collect) {
                sMsgMask[row + x] = 0;
                sOccBg[row + x] = bg < 0 ? 4 : (uint8_t)bgPrio[bg];
                sOccBgLayer[row + x] = bg < 0 ? 4 : (uint8_t)bg;
                sOccObj[row + x] = 0xFFFFu;
            }
        }
        return;
    }

    for (x = 0; x < w; x++) {
        int gx = x - gPortScreen.originX;
        uint8_t mask = sWinMask[x];
        int margin = gx < 0 || gx >= GBA_SCREEN_W || gyLine < 0 || gyLine >= GBA_SCREEN_H;
        uint16_t bd = margin && marginFill != TRANSPARENT ? marginFill : sInWorld[x] ? backdrop : 0;
        uint16_t top, second = bd, col;
        int first, topLayer, secondLayer = LAYER_BD, bg = -1;
        int obj = objects && (mask & WIN_OBJ) && sObjLine[x] != TRANSPARENT;
        int objFront, blend;

        /* Only clear the world behind a separately composed synthesis UI.
           Fixed views draw that UI in this very pass, so clearing it would
           leave a black screen with only the later HD text visible (#16). */
        if (gx < sLeft || gx >= sRight || (sOverlay && sSpotlight && (sScreenBgMask & 2u))) {
            dst[x] = 0xFF000000u;
            continue;
        }
        /* Find the first visible BG once. Composition and HD occlusion use
           the same priority decision; the second layer is needed only for
           alpha, not for opaque pixels or brightness effects. */
        for (first = 0; first < n; first++) {
            b = order[first];
            if ((mask & WIN_BG(b)) && sBgLine[b][x] != TRANSPARENT) {
                bg = b;
                break;
            }
        }
        if (bg < 0 && scene102Fill && margin)
            bd = Preliminar102ViewUnderlay(gx, gyLine);
        if (collect) {
            int message = 0;
            if (docked)
                for (b = 0; b < 4; b++)
                    if ((docked & (1 << b)) && (mask & WIN_BG(b)) && sBgLine[b][x] != TRANSPARENT)
                        message = 1;
            sMsgMask[row + x] = (uint8_t)message;
            if (!(mask & WIN_OBJ) || (message && sHudLayer)) {
                sOccBg[row + x] = sOccBgLayer[row + x] = 0;
                sOccObj[row + x] = 0;
            } else {
                sOccBg[row + x] = bg < 0 ? 4 : (uint8_t)bgPrio[bg];
                sOccBgLayer[row + x] = bg < 0 ? 4 : (uint8_t)bg;
                sOccObj[row + x] = sObjLine[x] != TRANSPARENT ?
                    (uint16_t)(sObjPrio[x] * 128 + sObjIndex[x]) : 0xFFFFu;
            }
        }
        objFront = obj && (bg < 0 || sObjPrio[x] <= bgPrio[bg]);
        topLayer = objFront ? LAYER_OBJ : bg < 0 ? LAYER_BD : bg;
        top = objFront ? sObjLine[x] : bg < 0 ? bd : sBgLine[bg][x];
        blend = (objFront && sObjSemi[x]) ||
                ((mask & WIN_SFX) && effect == 1 && (bldcnt & (1 << topLayer)));
        if (blend) {
            if (objFront) {
                if (bg >= 0) { second = sBgLine[bg][x]; secondLayer = bg; }
            } else if (bg >= 0) {
                int next = -1;
                for (b = first + 1; b < n; b++) {
                    int k = order[b];
                    if ((mask & WIN_BG(k)) && sBgLine[k][x] != TRANSPARENT) { next = k; break; }
                }
                if (obj && (next < 0 || sObjPrio[x] <= bgPrio[next])) {
                    second = sObjLine[x]; secondLayer = LAYER_OBJ;
                } else if (next >= 0) {
                    second = sBgLine[next][x]; secondLayer = next;
                }
            }
        }
        col = top;
        if (objFront && sObjSemi[x] && (bldcnt & (0x100 << secondLayer))) {
            col = Blend(top, second, eva, evb);
        } else if ((mask & WIN_SFX) && (bldcnt & (1 << topLayer))) {
            switch (effect) {
                case 1:
                    if (bldcnt & (0x100 << secondLayer)) col = Blend(top, second, eva, evb);
                    break;
                case 2: col = Brighten(top, evy); break;
                case 3: col = Darken(top, evy); break;
            }
        }
        dst[x] = sColorLut[col];
    }
}

/* ---- frame ------------------------------------------------------------------ */

static void LatchAffine(int force) {
    int a;

    for (a = 0; a < 2; a++) {
        uint32_t rx = IO32(0x28 + a * 0x10), ry = IO32(0x2C + a * 0x10);
        if (force || rx != sAffLatchX[a]) {
            sAffLatchX[a] = rx;
            sAffX[a] = Sext28(rx);
        }
        if (force || ry != sAffLatchY[a]) {
            sAffLatchY[a] = ry;
            sAffY[a] = Sext28(ry);
        }
    }
}

static void StepAffine(int lines) {
    int a;

    for (a = 0; a < 2; a++) {
        sAffX[a] += (int16_t)IO16(0x22 + a * 0x10) * lines;
        sAffY[a] += (int16_t)IO16(0x26 + a * 0x10) * lines;
    }
}

void PortBgClip(int bg, int on) {
    if ((unsigned)bg < 4) sBgClip[bg] = on;
}

void PortBgSplitBands(int bg, int enabled) {
    if ((unsigned)bg < 4) sBgSplitBands[bg] = enabled;
}

static void SplitBgLine(int b, int gy) {
    static PPU_TLS uint16_t tmp[PORT_MAX_W];
    int w = gPortScreen.width, m = PortScreenMarginX(), split = sBgSplit[b], x;

    memcpy(tmp, sBgLine[b], (size_t)w * 2);
    for (x = 0; x < w; x++) {
        int gx = x - gPortScreen.originX, c;
        if (gx < split - m)
            c = gx + m;
        else if (gx >= split + m)
            c = gx - m;
        else {
            if (sBgSplitBands[b] && (gy < 8 || gy >= 152)) {
                /* Repeat the outermost complete tile of the stat-side band.
                   This excludes OBJ cursors, text and the battle backdrop. */
                c = gPortScreen.originX + GBA_SCREEN_W - 8 + ((gx - split + m) & 7);
                sBgLine[b][x] = (c >= 0 && c < w) ? tmp[c] : TRANSPARENT;
                continue;
            }
            sBgLine[b][x] = TRANSPARENT;
            continue;
        }
        c += gPortScreen.originX;
        sBgLine[b][x] = (c >= 0 && c < w) ? tmp[c] : TRANSPARENT;
    }
}

/* The screen layer's line gy: menu layers only, transparent elsewhere. */
static void ComposeOverlayLine(int gy, uint16_t dispcnt, const int* bgOn, const int* bgPrio) {
    int wo = gPortOverlayW, w = gPortScreen.width;
    int originO = (wo - GBA_SCREEN_W) / 2;
    uint32_t* dst = gPortOverlay + gy * wo;
    uint16_t bldcnt = IO16(0x50), bldalpha = IO16(0x52);
    int eva = ((bldalpha & 31) > 16 ? 16 : (bldalpha & 31)), evb = (((bldalpha >> 8) & 31) > 16 ? 16 : ((bldalpha >> 8) & 31));
    int evy = ((IO16(0x54) & 31) > 16 ? 16 : (IO16(0x54) & 31)), effect = (bldcnt >> 6) & 3;
    int order[4], n = 0, p, b, ox;

    for (p = 0; p < 4; p++)
        for (b = 0; b < 4; b++)
            if (bgOn[b] && sScreenBg[b] && bgPrio[b] == p)
                order[n++] = b;
    for (ox = 0; ox < wo; ox++) {
        int gx = ox - originO, x = gx + gPortScreen.originX, i, found = 0, objDone = 0;
        int topLayer = LAYER_BD, secondLayer = LAYER_BD;
        uint16_t top = 0, second = 0, col;
        uint8_t mask;

        dst[ox] = 0;
        sScreenOccBg[gy * wo + ox] = 0;
        sScreenOccBgLayer[gy * wo + ox] = 0;
        sScreenOccObj[gy * wo + ox] = 0;
        if (x < 0 || x >= w || gx < sLeft || gx >= sRight)
            continue;
        mask = sWinMask[x];
        if ((sHdText || sScreenHdAny) && (mask & WIN_OBJ)) {
            int occ = 4, occLayer = 4;
            for (i = 0; i < n; i++) {
                if ((mask & WIN_BG(order[i])) && sBgLine[order[i]][x] != TRANSPARENT) {
                    occ = bgPrio[order[i]];
                    occLayer = order[i];
                    break;
                }
            }
            sScreenOccBg[gy * wo + ox] = (uint8_t)occ;
            sScreenOccBgLayer[gy * wo + ox] = (uint8_t)occLayer;
            sScreenOccObj[gy * wo + ox] = sScrLine[x] != TRANSPARENT ?
                (uint16_t)(sScrPrio[x] * 128 + sScrIndex[x]) : 0xFFFF;
        }
        for (i = 0; i <= n && found < 2; i++) {
            int prio = (i < n) ? bgPrio[order[i]] : 4;
            if (!objDone && (mask & WIN_OBJ) && sScrLine[x] != TRANSPARENT && sScrPrio[x] <= prio) {
                objDone = 1;
                if (found++ == 0) {
                    top = sScrLine[x];
                    topLayer = LAYER_OBJ;
                } else {
                    second = sScrLine[x];
                    secondLayer = LAYER_OBJ;
                }
                if (found == 2)
                    break;
            }
            if (i < n && (mask & WIN_BG(order[i])) && sBgLine[order[i]][x] != TRANSPARENT) {
                if (found++ == 0) {
                    top = sBgLine[order[i]][x];
                    topLayer = order[i];
                } else {
                    second = sBgLine[order[i]][x];
                    secondLayer = order[i];
                }
            }
        }
        if (found == 0)
            continue;
        col = top;
        {
            uint32_t alpha = 0xFF000000u;
            int blend = (topLayer == LAYER_OBJ && sScrSemi[x]) || ((mask & WIN_SFX) && effect == 1 && (bldcnt & (1 << topLayer)));
            if (blend) {
                if (found == 2 && (bldcnt & (0x100 << secondLayer)))
                    col = Blend(top, second, eva, evb);
                else
                    alpha = (uint32_t)(eva * 16 > 255 ? 255 : eva * 16) << 24; /* over the world below */
            } else if ((mask & WIN_SFX) && (bldcnt & (1 << topLayer))) {
                if (effect == 2)
                    col = Brighten(top, evy);
                else if (effect == 3)
                    col = Darken(top, evy);
            }
            dst[ox] = (sColorLut[col] & 0x00FFFFFFu) | alpha;
        }
    }
}

/* Extend authored title bands over the plain margin, before presentation.
   Sample each uncomposited layer, never the illustration/lettering. Keep
   hardware alpha and palette effects; the title margin is palette white. */
static void BuildTitleBands(int gy, const int* bgOn) {
    int side;
    if (!sTitleBands || gy < 0 || gy >= GBA_SCREEN_H || sEnvTitleBandsOff) return;
    for (side = 0; side < 2; side++) {
        int edge = gPortScreen.originX + (side ? GBA_SCREEN_W - 1 : 0);
        int step = side ? -1 : 1, k, layer = -1;
        uint16_t c = TRANSPARENT, white = PLTT16[31] & 0x7FFF;
        uint16_t ctrl = IO16(0x50), alpha = IO16(0x52);
        int effect = (ctrl >> 6) & 3;
        /* The press-start strip is BG0; its edge is a flat run. */
        if (bgOn[0] && sBgLine[0][edge] != TRANSPARENT) {
            c = sBgLine[0][edge];
            for (k = 0; k < 8 && sBgLine[0][edge + k * step] == c; k++) {}
            if (k == 8) layer = 0;
        }
        /* The brightness footer is an OBJ rooted at the native edge.
           Its authored band begins at y=143; labels remain inside. */
        if (gy >= 143 && sObjLine[edge] != TRANSPARENT) {
            uint16_t obj = sObjLine[edge];
            for (k = 0; k < 8 && sObjLine[edge + k * step] == obj; k++) {}
            if (k == 8) { c = obj; layer = LAYER_OBJ; }
        }
        if (layer < 0) continue;
        if ((layer == LAYER_OBJ && sObjSemi[edge]) ||
            (effect == 1 && (ctrl & (1 << layer))))
            c = Blend(c, white, (alpha & 31) > 16 ? 16 : alpha & 31,
                      ((alpha >> 8) & 31) > 16 ? 16 : (alpha >> 8) & 31);
        else if (ctrl & (1 << layer)) {
            int evy = IO16(0x54) & 31;
            if (evy > 16) evy = 16;
            if (effect == 2) c = Brighten(c, evy);
            if (effect == 3) c = Darken(c, evy);
        }
        sTitleBand[side][gy] = sColorLut[c];
    }
}

static void RenderLine(int y, int gy) {
    uint16_t dispcnt = IO16(0x00);
    int mode = dispcnt & 7;
    int bgOn[4] = { 0, 0, 0, 0 }, bgPrio[4];
    int b, w = gPortScreen.width;

    if (dispcnt & 0x80) {
        uint32_t* dst = gPortFrame + y * w;
        int x;
        for (x = 0; x < w; x++) {
            int gx = x - gPortScreen.originX;
            dst[x] = (gx < sLeft || gx >= sRight) ? 0xFF000000u : 0xFFFFFFFFu;
            gPortMenuBackground[y * w + x] = dst[x];
        }
        return;
    }

    {
        int x, inside = !gPortScreen.wide || (gy >= 0 && gy < GBA_SCREEN_H);
        for (x = 0; x < w; x++) {
            int gx = x - gPortScreen.originX;
            sInWorld[x] = inside && (!gPortScreen.wide || (gx >= 0 && gx < GBA_SCREEN_W));
        }
    }

    for (b = 0; b < 4; b++) {
        bgPrio[b] = IO16(0x08 + b * 2) & 3;
        if (!(dispcnt & (0x100 << b)))
            continue;
        switch (mode) {
            case 0:
                bgOn[b] = 1;
                break;
            case 1:
                bgOn[b] = b <= 2;
                break;
            case 2:
                bgOn[b] = b >= 2;
                break;
            default:
                bgOn[b] = b == 2;
                break;
        }
    }
    if (sTitleLogoBgHd)
        bgOn[2] = 0; /* the title's logo: drawn HD over the picture (QueueMainHdParts) */
    for (b = 0; b < 4; b++) {
        if (!bgOn[b])
            continue;
        if (mode >= 3)
            DrawBitmapBg(mode, gy, dispcnt);
        else if (b >= 2 && (mode == 2 || (mode == 1 && b == 2)))
            DrawAffineBg(b, gy, dispcnt);
        else
            DrawTextBg(b, gy, dispcnt);
        if (sOverlay && sScreenBg[b] && sBgSplit[b])
            SplitBgLine(b, gy);
        else if (sBgClip[b] && !(sOverlay && sScreenBg[b])) {
            /* its wrapped columns would show in the margins */
            int x;
            for (x = 0; x < w; x++)
                if (x - gPortScreen.originX < 0 || x - gPortScreen.originX >= GBA_SCREEN_W)
                    sBgLine[b][x] = TRANSPARENT;
        }
    }
    if (bgOn[1]) TitleArtFadeLine(gy, 1);
    if (bgOn[0] && sTitleArtFadeBg0) TitleArtFadeLine(gy, 0);
    DrawObjs(gy, dispcnt);
    BuildTitleBands(gy, bgOn);
    BuildWindows(gy, dispcnt);
    ComposeLine(y, dispcnt, bgOn, bgPrio, gPortFrame, 1);
    /* The presenter also uses this background in non-menu narrow scenes,
       including the blank frame between modes. Never leave last frame's
       palette/fade in the buffer. */
    if (sSceneNarrow || !gPortScreen.wide)
        ComposeLine(y, dispcnt, bgOn, bgPrio, gPortMenuBackground, 0);
    if (sSceneNarrow)
        BuildNarrowSides(gy, dispcnt, bgOn, bgPrio);
    if (sOverlay && gy >= 0 && gy < GBA_SCREEN_H)
        ComposeOverlayLine(gy, dispcnt, bgOn, bgPrio);
}

/* Full-height GBA scenes (menus shown as the GBA picture with side bars):
   what lies beyond the picture's edges, built from the BG layers themselves
   so the bars are the game's own art in its own pixels and palettes. The
   scenery (the lowest enabled BG) continues mirrored, as the hardware's
   horizontal tile flip draws it; a layer above it continues only where it
   reaches the edge as a long horizontal band (a title bar, a banner, a
   panel), and is transparent elsewhere so the scenery shows; the backdrop is
   underneath. The scene's fade applies to the bars too. */
#define NARROW_SIDE_W 96
#define PLTT16_0() (PPU_PLTT_BASE[0])
static uint32_t sNarrowSide[2][GBA_SCREEN_H][NARROW_SIDE_W];
/* per row: the colour of the layer in front of the scenery at the picture's
   edge, and whether that row was a band (2), only touched the edge (1) or
   showed the scenery (0) */
static uint32_t sNarrowEdge[2][GBA_SCREEN_H];
static uint8_t sNarrowEdgeKind[2][GBA_SCREEN_H];
/* sprite rows at the edge (PortNarrowObjBands): each row's colours and counts */
static int sNarrowObjBands;
#define OBJ_BAND_COLORS 16
static uint16_t sObjBandColor[2][GBA_SCREEN_H][OBJ_BAND_COLORS];
static uint8_t sObjBandCount[2][GBA_SCREEN_H][OBJ_BAND_COLORS];
static uint8_t sObjBandKinds[2][GBA_SCREEN_H], sObjBandTotal[2][GBA_SCREEN_H];

/* A box or title is one block of rows touching the edge; its lettering can
   cut some rows short of a band (the Moogle Shop's points box, the title's
   middle rows), but they belong to it, so a block with any band row carries
   every row on, each in its own edge colour. */
/* 2 when the layer in front of the scenery reaches the edge of row y as a
   long band, 1 when it only touches the edge, 0 when the edge shows the
   scenery itself (GBA-size scenes only; -1 otherwise) */
int PortNarrowEdgeKind(int side, int y) {
    if (!sSceneNarrow || y < 0 || y >= GBA_SCREEN_H)
        return -1;
    return sNarrowEdgeKind[side > 0][y];
}

/* Rows of sprites at the edge (the world select's bands and sliding title plate,
   PortNarrowObjBands): a row of one colour carries on as itself (a band, an
   outline); the others take the colour commonest over their whole run of rows,
   the plate's body rather than its lettering, as the margin shows once the slide
   has ended and the game's own band layer takes over. */
static int ObjBandTop(int side, int y) {
    int j, top = 0;
    for (j = 1; j < sObjBandKinds[side][y]; j++)
        if (sObjBandCount[side][y][j] > sObjBandCount[side][y][top])
            top = j;
    return top;
}

static int ObjBandMixed(int side, int y) {
    return sObjBandKinds[side][y] && sObjBandTotal[side][y] >= 8 &&
           sObjBandCount[side][y][ObjBandTop(side, y)] * 10 < sObjBandTotal[side][y] * 9;
}

static void ObjBandFill(int side, int y, uint16_t c) {
    int d;
    sNarrowEdge[side][y] = sColorLut[c & 0x7FFF];
    sNarrowEdgeKind[side][y] = 2;
    for (d = 0; d < NARROW_SIDE_W; d++)
        sNarrowSide[side][y][d] = sColorLut[c & 0x7FFF];
}

static void FinishObjBands(void) {
    int side, y;
    for (side = 0; side < 2; side++)
        for (y = 0; y < GBA_SCREEN_H; y++) {
            int z = y, k, j, best = -1, kinds = 0;
            uint32_t total[OBJ_BAND_COLORS * 4], rows[OBJ_BAND_COLORS * 4];
            uint16_t cols[OBJ_BAND_COLORS * 4];
            if (!sObjBandKinds[side][y] || sObjBandTotal[side][y] < 8)
                continue;
            if (!ObjBandMixed(side, y)) { /* a band or an outline: its own colour */
                ObjBandFill(side, y, sObjBandColor[side][y][ObjBandTop(side, y)]);
                continue;
            }
            while (z + 1 < GBA_SCREEN_H && ObjBandMixed(side, z + 1)) z++;
            for (k = y; k <= z; k++)
                for (j = 0; j < sObjBandKinds[side][k]; j++) {
                    int m;
                    for (m = 0; m < kinds && cols[m] != sObjBandColor[side][k][j]; m++) {}
                    if (m == kinds) {
                        if (kinds == OBJ_BAND_COLORS * 4)
                            continue;
                        cols[kinds] = sObjBandColor[side][k][j];
                        rows[kinds] = 0;
                        total[kinds++] = 0;
                    }
                    rows[m]++;
                    total[m] += sObjBandCount[side][k][j];
                }
            /* the plate's body runs through every row; its lettering does not */
            for (k = 0; k < kinds; k++)
                if (best < 0 || rows[k] > rows[best] || (rows[k] == rows[best] && total[k] > total[best]))
                    best = k;
            for (k = y; k <= z; k++)
                ObjBandFill(side, k, cols[best]);
            y = z;
        }
}

void PortNarrowSidesFinish(void) {
    int side, y;
    if (sNarrowObjBands)
        FinishObjBands();
    for (side = 0; side < 2; side++)
        for (y = 0; y < GBA_SCREEN_H; y++) {
            int a = y, z = y, band = 0, k, d;
            if (!sNarrowEdgeKind[side][y])
                continue;
            while (z + 1 < GBA_SCREEN_H && sNarrowEdgeKind[side][z + 1]) z++;
            for (k = a; k <= z; k++) band |= sNarrowEdgeKind[side][k] == 2;
            if (band)
                for (k = a; k <= z; k++)
                    if (sNarrowEdgeKind[side][k] == 1)
                        for (d = 0; d < NARROW_SIDE_W; d++)
                            sNarrowSide[side][k][d] = sNarrowEdge[side][k];
            y = z;
        }
}

uint32_t PortNarrowSidePixel(int side, int y, int distance) {
    if (y < 0 || y >= GBA_SCREEN_H)
        return 0;
    if (distance >= NARROW_SIDE_W)
        distance = NARROW_SIDE_W - 1 - (distance % NARROW_SIDE_W);
    return sNarrowSide[side > 0][y][distance < 0 ? 0 : distance];
}

/* World select: while its header slides in, the title and footer bands are
   sprites (the game enables their BG only once the slide ends), so a band of
   sprites at the picture's edge is carried on as well (PortNarrowObjBands). */
void PortNarrowObjBands(int on) {
    sNarrowObjBands = on;
}

static void BuildNarrowSides(int gy, uint16_t dispcnt, const int* bgOn, const int* bgPrio) {
    int side, d, b, scenery = -1, order[4], n = 0, p, flat;
    int x0 = gPortScreen.originX;
    uint16_t bldcnt = IO16(0x50);
    int effect = (bldcnt >> 6) & 3, evy = IO16(0x54) & 31;

    if (gy < 0 || gy >= GBA_SCREEN_H)
        return;
    if (evy > 16)
        evy = 16;
    /* the scenery: the enabled BG drawn last (largest priority, then index) */
    for (b = 0; b < 4; b++)
        if (bgOn[b] && (scenery < 0 || bgPrio[b] > bgPrio[scenery] ||
                        (bgPrio[b] == bgPrio[scenery] && b > scenery)))
            scenery = b;
    for (p = 0; p < 4; p++)
        for (b = 0; b < 4; b++)
            if (bgOn[b] && bgPrio[b] == p)
                order[n++] = b;
    for (side = 0; side < 2; side++) {
        int edge = side ? x0 + GBA_SCREEN_W - 1 : x0, step = side ? -1 : 1;
        uint16_t band[4];
        int bandOk[4];
        for (b = 0; b < 4; b++) {
            int run = 0, k;
            uint16_t c = sBgLine[b][edge];
            bandOk[b] = 0;
            band[b] = c;
            if (!bgOn[b] || b == scenery || c == TRANSPARENT)
                continue;
            for (k = 0; k < 48 && sBgLine[b][edge + k * step] == c; k++)
                run++;
            bandOk[b] = run >= 24;
        }
        /* scenery that is plain next to the edge (two tiles of one colour)
           carries on plain: mirrored, the shapes further in (the Moogle
           Shop's pedestals) would show up again as reflections */
        flat = 0;
        if (scenery >= 0 && sBgLine[scenery][edge] != TRANSPARENT) {
            int k;
            for (k = 0; k < 16 && sBgLine[scenery][edge + k * step] == sBgLine[scenery][edge]; k++) {}
            flat = k == 16;
        }
        sNarrowEdgeKind[side][gy] = 0;
        for (p = 0; p < n && order[p] != scenery; p++) {
            uint16_t c;
            b = order[p];
            if (band[b] == TRANSPARENT)
                continue;
            c = band[b];
            if (effect == 2 && (bldcnt & 0x3F)) c = Brighten(c, evy);
            if (effect == 3 && (bldcnt & 0x3F)) c = Darken(c, evy);
            sNarrowEdge[side][gy] = sColorLut[c & 0x7FFF];
            sNarrowEdgeKind[side][gy] = bandOk[b] ? 2 : 1;
            break;
        }
        for (d = 0; d < NARROW_SIDE_W; d++) {
            uint16_t col = (uint16_t)(PLTT16_0() & 0x7FFF);
            int i, src = flat ? 0 : d < GBA_SCREEN_W ? d : GBA_SCREEN_W - 1;
            for (i = 0; i < n; i++) {
                b = order[i];
                if (b == scenery) {
                    uint16_t c = sBgLine[b][edge + src * step];
                    if (c != TRANSPARENT) { col = c; break; }
                } else if (bandOk[b]) {
                    col = band[b];
                    break;
                }
            }
            if (effect == 2 && (bldcnt & 0x3F)) col = Brighten(col, evy);
            if (effect == 3 && (bldcnt & 0x3F)) col = Darken(col, evy);
            sNarrowSide[side][gy][d] = sColorLut[col & 0x7FFF];
        }
        /* the sprites touching the edge on this row, counted per colour
           (PortNarrowSidesFinish decides the margin's colour for the run) */
        sObjBandKinds[side][gy] = sObjBandTotal[side][gy] = 0;
        if (sNarrowObjBands && sObjLine[edge] != TRANSPARENT) {
            int k, kinds = 0;
            for (k = 0; k < 48; k++) { /* what meets the margin */
                uint16_t c = sObjLine[edge + k * step];
                int j;
                if (c == TRANSPARENT)
                    break;
                for (j = 0; j < kinds && sObjBandColor[side][gy][j] != c; j++) {}
                if (j == kinds) {
                    if (kinds == OBJ_BAND_COLORS)
                        continue;
                    sObjBandColor[side][gy][j] = c;
                    sObjBandCount[side][gy][kinds++] = 0;
                }
                sObjBandCount[side][gy][j]++;
            }
            sObjBandKinds[side][gy] = (uint8_t)kinds;
            sObjBandTotal[side][gy] = (uint8_t)k;
        }
    }
    (void)dispcnt;
}

/* Start of a line: VCOUNT and the V-counter match flag. */
static void LineStart(int line) {
    uint16_t dispstat = IO16(0x04);

    IO16(0x06) = (uint16_t)line;
    IO16(0x04) = (dispstat & ~6) | (((dispstat >> 8) == line) ? 4 : 0);
}

/* End of a line's drawing period. The V-counter interrupt is delivered here
   rather than at the start of the line: handlers typically wait for HBlank
   anyway, and either way their writes take effect from the next line. */
static void HBlank(int line) {
    uint16_t dispstat;

    IO16(0x04) |= 2;
    dispstat = IO16(0x04);
    if ((dispstat & 4) && (dispstat & 0x20))
        PortRaiseIrq(0x0004);
    if (line < GBA_SCREEN_H)
        PortDmaHBlank();
    if (dispstat & 0x10)
        PortRaiseIrq(0x0002);
    IO16(0x04) &= ~2;
}

/* A mode switch asks for its framing before the old picture has gone: the
   change waits for a blank frame of the transition (or half a second), so
   the last frame of the old screen never shows at the new size. */
static int sLastFrameBlank; /* the frame just drawn was blank (UpdateSceneNarrow) */

void PortSceneNarrow(int on) {
    sNarrowWanted = on;
    /* the old screen already faded out (the game waits for its fade before
       changing mode): the new framing can start with the new screen */
    sNarrowWait = sLastFrameBlank ? 31 : 0;
}

static void UpdateSceneNarrow(uint16_t dispcnt) {
    uint16_t bldcnt = IO16(0x50);
    /* forced blank, nothing enabled, or every layer faded fully to black or to
       white (the world select comes in from white: waiting the half second
       there switched the framing in the middle of its header's slide) */
    int faded = ((bldcnt >> 6) & 3) >= 2 && (IO16(0x54) & 31) >= 16 && (bldcnt & 0x1F) == 0x1F;
    int blank = (dispcnt & 0x80) || !(dispcnt & 0x1F00) || faded;

    int lastBlank = sLastFrameBlank; /* the frame before: also a one-colour picture */

    sLastFrameBlank = blank; /* PortPpuRenderFrame sets it too for a one-colour picture */
    if (sSceneNarrow == sNarrowWanted)
        return;
    if (blank || lastBlank || ++sNarrowWait > 30)
        sSceneNarrow = sNarrowWanted;
}

/* The scene is one of the full-screen menus over the field (PortSceneNarrow). */
int PortPpuSceneIsMenu(void) {
    return sSceneNarrow;
}

/* Grow to the active canvas, retaining capacity across menu/world switches.
   Each allocation remains separately aligned. New space is zeroed just as
   the former maximum-sized calloc buffers were; existing pixels survive a
   resize until the next frame renders them. */
void PortCanvasAlloc(void) {
    static size_t capacity;
    size_t n = (size_t)gPortScreen.width * gPortScreen.height;
    uint32_t* frame;
    uint32_t* background;
    uint8_t *bg, *layer, *message;
    uint16_t* obj;
    if (n <= capacity)
        return;
    frame = (uint32_t*)realloc(gPortFrame, n * sizeof(*frame));
    background = (uint32_t*)realloc(gPortMenuBackground, n * sizeof(*background));
    bg = (uint8_t*)realloc(sOccBg, n);
    layer = (uint8_t*)realloc(sOccBgLayer, n);
    message = (uint8_t*)realloc(sMsgMask, n);
    obj = (uint16_t*)realloc(sOccObj, n * sizeof(*obj));
    if (!frame || !background || !bg || !layer || !message || !obj)
        PortFatal("Out of memory for the picture buffers");
    gPortFrame = frame; gPortMenuBackground = background;
    sOccBg = bg; sOccBgLayer = layer; sMsgMask = message; sOccObj = obj;
    memset(frame + capacity, 0, (n - capacity) * sizeof(*frame));
    memset(background + capacity, 0, (n - capacity) * sizeof(*background));
    memset(bg + capacity, 0, n - capacity);
    memset(layer + capacity, 0, n - capacity);
    memset(message + capacity, 0, n - capacity);
    memset(obj + capacity, 0, (n - capacity) * sizeof(*obj));
    capacity = n;
}

/* The "/" of a BG number such as 095/095 is a fixed tile of the panel, not one of the digit tiles the game
   copies in, so it stayed pixel art between HD digits. A cell between two registered digits whose ink runs
   from top right to bottom left is registered as that slash, with the colours of the digit on its left. */
static uint16_t TextBgEntry(int bg, int tx, int ty) {
    uint16_t cnt = IO16(0x08 + bg * 2);
    int size = cnt >> 14, block = (tx >= 32) + (ty >= 32) * (size == 3 ? 2 : 1);
    const uint16_t* map = (const uint16_t*)(VRAM8 + ((cnt >> 8) & 31) * 0x800 + block * 0x800);
    return map[(ty & 31) * 32 + (tx & 31)];
}

static const BgTextTile* RegisteredDigit(unsigned base, uint16_t entry) {
    unsigned offset = base + (entry & 0x3FF) * 32;
    const BgTextTile* tile;
    if ((entry & 0xC00) || offset >= 0x10000) return NULL;
    tile = BgTextTileAt(offset);
    return tile->code >= '0' && tile->code <= '9' && !memcmp(tile->source, VRAM8 + offset, 32) ? tile : NULL;
}

static void RegisterBgSlashes(uint16_t dispcnt) {
    int bg, mode = dispcnt & 7;
    if (!sHdText || mode > 1) return;
    for (bg = 0; bg < (mode == 1 ? 2 : 4); bg++) {
        uint16_t cnt = IO16(0x08 + bg * 2);
        unsigned base = ((cnt >> 2) & 3) * 0x4000;
        int w = (cnt >> 14) & 1 ? 64 : 32, h = (cnt >> 14) & 2 ? 64 : 32, tx, ty;
        if (!(dispcnt & (0x100 << bg)) || (cnt & 0x80)) continue;
        for (ty = 0; ty < h; ty++)
            for (tx = 1; tx + 1 < w; tx++) {
                uint16_t e = TextBgEntry(bg, tx, ty);
                unsigned offset = base + (e & 0x3FF) * 32;
                const BgTextTile *left, *right;
                int y, x, top = -1, bottom = -1, topX = 0, bottomX = 0, ink;
                if ((e & 0xC00) || offset >= 0x10000 || sBgTextTiles[offset / 32].code > 0x20) continue;
                if (!(left = RegisteredDigit(base, TextBgEntry(bg, tx - 1, ty))) ||
                    !(right = RegisteredDigit(base, TextBgEntry(bg, tx + 1, ty))))
                    continue;
                for (y = 0; y < 8; y++) {
                    int sum = 0, count = 0;
                    for (x = 0; x < 8; x++) {
                        uint8_t v = VRAM8[offset + y * 4 + x / 2];
                        ink = (x & 1) ? v >> 4 : v & 15;
                        if (ink == left->faceIndex || ink == left->shadowIndex) { sum += x; count++; }
                    }
                    if (count) {
                        if (top < 0) { top = y; topX = sum / count; }
                        bottom = y; bottomX = sum / count;
                    }
                }
                if (top < 0 || bottom - top < 3 || topX < bottomX + 2) continue;
                PortRegisterBgTextTile((void*)(uintptr_t)(GBA_VRAM + offset), '/',
                                       left->style | left->faceIndex << 8 | left->shadowIndex << 12 | left->panelIndex << 16,
                                       VRAM8 + offset);
            }
    }
}

void PortPpuBeginFrame(void) {
#ifdef __EMSCRIPTEN__
    extern int gPortWebHdReady;
    if(!gPortWebHdReady)gPortConfig.hdText=0;
#endif
    uint16_t dispcnt = IO16(0x00);
    int b, any = 0;
    if (sTitleArtFade && (!sTitlePaletteValid || memcmp(sTitlePalette, PLTT16, sizeof(sTitlePalette)))) {
        memcpy(sTitlePalette, PLTT16, sizeof(sTitlePalette));
        memset(sTitleFadeReady, 0, sizeof(sTitleFadeReady));
        sTitlePaletteValid = 1;
    }
#if defined(__ANDROID__) || defined(PORT_LINUX) /* FreeType HD text (#77) */
    extern int PortPortableHdReady(void);
#endif
    sBgTextCount = 0;
    sEventBackdropProfile = PortEventBackdropProfile();
    sUseSceneProfiles = getenv("KHCOM_NO_SCENE_PROFILES") == NULL;
    {
        const char* v = getenv("KHCOM_SCENE_ACTOR_FRAME");
        sEnvActorFrameOff = v && strcmp(v, "off") == 0;
        v = getenv("KHCOM_HUD_LAYOUT");
        sEnvHudLayoutFrame = v ? atoi(v) : -1;
        sEnvNoGuardPatio = getenv("KHCOM_NO_GUARD_PATIO") != NULL;
        sEnvTitleBandsOff = getenv("KHCOM_TITLE_BANDS_OFF") != NULL;
    }
    sPreliminarV1 = getenv("KHCOM_PRELIMINAR_V1") != NULL;
    (void)PortArenasV2(); /* its switch read here, on the main thread, before the cores draw (they ask it per row) */
    sSinglePanelBg = PortEventBackdropPanel();

    memset(sTitleBand, 0, sizeof(sTitleBand));
    BuildLut();
    UpdateSceneNarrow(dispcnt);
    for (b = 0; b < 4; b++) {
        sBgLatch[b].valid = ValidateBgLatch(b, dispcnt);
        sBgWorldLatched[b].valid = !sBgLatch[b].valid && ValidateBgWorld(b, dispcnt);
        any |= sBgLatch[b].valid | sBgWorldLatched[b].valid;
    }
    /* EU decompresses these maps into RAM; US/JP use cartridge maps directly.
     * Identify their tile data, never an event/mode or a cached RAM address.
     * Recompute to handle exits, reused allocations and state loads alike.
     * Preview is opt-in until visually approved. */
    memset(sAgrabahLayer, 0, sizeof(sAgrabahLayer));
    memset(sPreliminarRecipe, 0, sizeof(sPreliminarRecipe));
    /* Preliminar V1 is opt-in until approved; scenes approved with honours are on for everyone (=0 turns one off) */
    sPreliminarApprovedOnly = !sPreliminarV1;
    {
        int anyApproved = 0;
        for (b = 0; b < (int)(sizeof(kPreliminarApproved)/sizeof(kPreliminarApproved[0])); b++) {
            char var[] = "KHCOM_SCENE000_WIDE";
            var[11] = (char)('0' + kPreliminarApproved[b] / 100);
            var[12] = (char)('0' + kPreliminarApproved[b] / 10 % 10);
            var[13] = (char)('0' + kPreliminarApproved[b] % 10);
            sPreliminarApprovedOn[b] = PortArtOn(var);
            anyApproved |= sPreliminarApprovedOn[b];
        }
        if (!anyApproved) sPreliminarApprovedOnly = 0;
    }
    if ((sPreliminarV1 || sPreliminarApprovedOnly) && !sBattleView) {
        const char* skipValue=getenv("KHCOM_PRELIMINAR_SKIP");
        int skip=skipValue?atoi(skipValue):-1;
        for (b=1; b<4; b++) {
            const BgLatch* l=&sBgLatch[b];
            if (l->valid && l->width <= 4 && l->height <= 4) {
                uint32_t hash=2166136261u;
                int xx,yy;
                for (yy=0; yy<l->height*32; yy++)
                    for (xx=0; xx<l->width*32; xx++)
                        hash=(hash^(uint32_t)MapTile(l,xx,yy))*16777619u;
                sPreliminarRecipe[b]=PortPreliminarFind(hash,b,l->width,l->height);
                if (getenv("KHCOM_PRELIMINAR_LOG"))
                    PortLog("preliminar bg%d %dx%d hash=%08x scene=%d",b,l->width,l->height,hash,
                            sPreliminarRecipe[b]?sPreliminarRecipe[b]->v[0]:-1);
                if (sPreliminarRecipe[b] && sPreliminarRecipe[b]->v[0]==skip)
                    sPreliminarRecipe[b]=NULL;
                if (sPreliminarApprovedOnly && sPreliminarRecipe[b]) {
                    int k, on = 0;
                    for (k = 0; k < (int)(sizeof(kPreliminarApproved)/sizeof(kPreliminarApproved[0])); k++)
                        if (sPreliminarRecipe[b]->v[0] == kPreliminarApproved[k]) on = sPreliminarApprovedOn[k];
                    if (!on) sPreliminarRecipe[b]=NULL;
                }
            }
        }
    }
    if (sPreliminarRecipe[2] && sPreliminarRecipe[2]->v[0]==45)
        PreparePreliminarFir();
    if (sPreliminarV1 || PortArtOn("KHCOM_AGRABAH_WIDE")) {
        for (b = 2; b <= 3; b++) {
            const BgLatch* l = &sBgLatch[b];
            if (l->valid && l->width == 2 && l->height == 2) {
                uint32_t hash = 2166136261u;
                int xx, yy, e;
                for (yy = 0; yy < 64; yy++)
                    for (xx = 0; xx < 64; xx++) {
                        e = MapTile(l, xx, yy);
                        hash = (hash ^ (uint32_t)e) * 16777619u;
                    }
                if (hash == 0x76732a49u) sAgrabahLayer[b] = 3;
                if (hash == 0x30fe052eu) sAgrabahLayer[b] = 2;
            }
        }
    }
    for (b = 0; b < 2; b++)
        sDarksideLayer[b] = sBgLatch[b].valid ? PortDarksideLayer((const void* const*)sBgLatch[b].map) : 0;
    sDarksideLayer[2] = sDarksideLayer[3] = 0;
    memset(sAgrabah109Layer, 0, sizeof(sAgrabah109Layer));
    sAgrabah109ShopPal[0] = sAgrabah109ShopPal[1] = -1;
    if (sPreliminarV1 || PortArtOn("KHCOM_AGRABAH109_WIDE")) {
        for (b = 2; b <= 3; b++) {
            const BgLatch* l = &sBgLatch[b];
            if (l->valid && l->width == 1 && l->height == 1) {
                uint32_t hash = 2166136261u;
                int xx, yy;
                for (yy = 0; yy < 32; yy++)
                    for (xx = 0; xx < 32; xx++)
                        hash = (hash ^ (uint32_t)MapTile(l, xx, yy)) * 16777619u;
                if (hash == 0xc704bad0u) sAgrabah109Layer[b] = 3;
                if (hash == 0xe1940c58u) sAgrabah109Layer[b] = 2;
            }
        }
    }
    if (sAgrabah109Layer[2] && gSpriteWork) {
        for (b = 0; b < 16; b++) {
            const ObjPalette* p = &gSpriteWork->palettes[b];
            if (p->self != p || !(p->node.flags & LIST_NODE_FLAG_ACTIVE) || p->index >= 16) continue;
            if (p->src == (const void*)gMapGmkAgrabah00Palette) sAgrabah109ShopPal[0] = p->index;
            if (p->src == (const void*)gMapGmkAgrabah01Palette) sAgrabah109ShopPal[1] = p->index;
        }
    }
    /* Marluxia's last fight: the wide view all along (not only while its effect layer is on) */
    if (PortMarluxiaFieldOn())
        any = 1;
    /* Large affine stages (battle arenas) are fully in VRAM and extend as is. */
    for (b = 2; b < 4; b++) {
        int mode = dispcnt & 7;
        if ((dispcnt & (0x100 << b)) && (mode == 2 || (mode == 1 && b == 2)) && (IO16(0x08 + b * 2) >> 14) >= 2)
            any = 1;
    }
    /* the door's spotlight (hitting a door, then room synthesis) is a
       GBA-size scene: shown at the window's height like the other menus,
       never as a small picture at the world's zoom */
    {
        const char* aerithNativeFrame = getenv("KHCOM_AERITH_NATIVE_FRAME");
        int keepAerithFrame = sEventBackdropProfile == 9 && aerithNativeFrame &&
                              strcmp(aerithNativeFrame, "on") == 0;
        gPortScreen.wide = any && !sSceneNarrow && !keepAerithFrame &&
                       !(sSpotlight && !(sScreenBgMask & 2u))
                       && (gPortScreen.width > GBA_SCREEN_W || gPortScreen.height > GBA_SCREEN_H)
                       && !(dispcnt & 0x80);
    }
    if (getenv("KHCOM_PPULOG"))
        PortLog("ppu: dispcnt %04x latch %d%d%d%d world %d%d%d%d bgcnt %04x %04x %04x %04x wide %d narrow %d", dispcnt,
                sBgLatch[0].valid, sBgLatch[1].valid, sBgLatch[2].valid, sBgLatch[3].valid, sBgWorldLatched[0].valid,
                sBgWorldLatched[1].valid, sBgWorldLatched[2].valid, sBgWorldLatched[3].valid, IO16(0x08), IO16(0x0A),
                IO16(0x0C), IO16(0x0E), gPortScreen.wide, sSceneNarrow);
    sHudLayer = gPortScreen.wide && gPortHudScale > 0 && gPortHudW >= GBA_SCREEN_W;
    sHdText = gPortConfig.hdText;
#if defined(__ANDROID__) || defined(PORT_LINUX)
    sHdText = sHdText && PortPortableHdReady();
#endif
    RegisterBgSlashes(dispcnt);
    if (sHudLayer || sHdText) {
        memset(sOccBg, 4, (size_t)gPortScreen.width * gPortScreen.height);
        memset(sOccBgLayer, 4, (size_t)gPortScreen.width * gPortScreen.height);
        memset(sOccObj, 0xFF, (size_t)gPortScreen.width * gPortScreen.height * 2);
    }
    sOverlay = gPortScreenLayer;
    for (b = 0; b < 4; b++) {
        int m = dispcnt & 7;
        int arena = (m == 2 || (m == 1 && b == 2)) && b >= 2 && (IO16(0x08 + b * 2) >> 14) >= 2;
        /* Split menus may use the engine's map-block API too; those blocks
           describe UI pages, not a world that should remain under the overlay. */
        sScreenBg[b] = sOverlay && !(sSpotlight && b == 0 && !(sScreenBgMask & 2u)) &&
            ((sScreenBgMask & (1u << b)) || sBgSplit[b] ||
             (!sBgLatch[b].valid && !sBgWorldLatched[b].valid && !arena));
    }
    if (sOverlay)
        memset(gPortOverlay, 0, (size_t)gPortOverlayW * GBA_SCREEN_H * 4);
    for (b = 0; b < 4; b++) sWorldPadding[b] = WorldPadding(b, &sBgLatch[b]);
    FindTiledBackdrop(PortEventTiledBackdrop());
    if (gPortScreen.wide) {
        sLeft = -gPortScreen.originX;
        sRight = gPortScreen.width - gPortScreen.originX;
        sTop = -gPortScreen.originY;
        sBottom = gPortScreen.height - gPortScreen.originY;
    } else {
        sLeft = 0;
        sRight = GBA_SCREEN_W;
        sTop = 0;
        sBottom = GBA_SCREEN_H;
    }
}

/* Draw the visible lines, running the per-line hardware events in between. */
/* ---- rows above and below the GBA picture ----------------------------------------------------------------------
 * They show the state of line 0 (above) or of line 159 (below): no interrupt or DMA of the game runs between them,
 * so with PORT_PPU_THREADS they are drawn on several cores, each with its own line buffers (PPU_TLS). At zoom x1 they
 * are most of the canvas (920 of 1080 rows at 1080p). The result is the one a single core draws, pixel for pixel:
 * each row gets the affine reference it would have had, the BG text glyphs found are merged back in row order, and
 * frames with state carried from row to row (vertical mosaic, the isometric floor, menu headers) stay on one core.
 * KHCOM_PPU_VERIFY=1 draws every threaded batch again on one core and logs any difference; KHCOM_PPU_THREADS=n sets
 * the number of cores (1: one). */
typedef struct OuterRows {
    int y0, y1, below;
    int32_t affX[2], affY[2]; /* the reference point at line 0 (above) or after line 159 (below) */
} OuterRows;

static void OuterRow(const OuterRows* r, int y) {
    int w = gPortScreen.width, gy = y - gPortScreen.originY, a, k;
    if (!r->below) {
        if (gy < sTop) {
            memset(gPortFrame + (size_t)y * w, 0, (size_t)w * 4);
            return;
        }
        k = gy;
    } else {
        if (gy >= sBottom) {
            memset(gPortFrame + (size_t)y * w, 0, (size_t)w * 4);
            memset(gPortMenuBackground + (size_t)y * w, 0, (size_t)w * 4);
            return;
        }
        k = y - r->y0; /* the drawn rows below come first: one affine step each */
    }
    for (a = 0; a < 2; a++) {
        sAffX[a] = r->affX[a] + (int16_t)IO16(0x22 + a * 0x10) * k;
        sAffY[a] = r->affY[a] + (int16_t)IO16(0x26 + a * 0x10) * k;
    }
    RenderLine(y, gy);
}

#ifdef PORT_PPU_THREADS
#define PPU_MAX_THREADS 8
#define PPU_CHUNK 4 /* rows a core takes at a time */
extern void MapWorldTilePrime(void); /* src/map/map_cell.c */
static int sPpuWorkers = -1; /* other cores drawing with this one; -1 not started */
static SDL_Semaphore *sPpuGo, *sPpuDone;
static const OuterRows* sPpuJob; /* NULL: the GBA picture's rows (PpuInnerRow) */
static int sPpuEnd;
static SDL_AtomicInt sPpuNext;
static void PpuInnerRow(int y);
static PendingGlyph sPpuGlyphs[PPU_MAX_THREADS][1024];
static int sPpuGlyphCount[PPU_MAX_THREADS];
static int sPpuVerify, sPpuMismatches;

static void PpuRows(int slot) {
    const OuterRows* r = sPpuJob;
    int y, end, last = sPpuEnd;
    sGlyphPend = sPpuGlyphs[slot];
    sGlyphPendCount = 0;
    for (;;) {
        y = SDL_AddAtomicInt(&sPpuNext, PPU_CHUNK);
        if (y >= last)
            break;
        for (end = y + PPU_CHUNK < last ? y + PPU_CHUNK : last; y < end; y++) {
            sGlyphRow = y;
            if (r)
                OuterRow(r, y);
            else
                PpuInnerRow(y);
        }
    }
    sPpuGlyphCount[slot] = sGlyphPendCount;
    sGlyphPend = NULL;
}

static int SDLCALL PpuWorker(void* data) {
    int slot = (int)(intptr_t)data;
    for (;;) {
        SDL_WaitSemaphore(sPpuGo);
        PpuRows(slot);
        SDL_SignalSemaphore(sPpuDone);
    }
    return 0;
}

static void PpuStart(void) {
    const char* env = SDL_getenv("KHCOM_PPU_THREADS");
    int n = env ? SDL_atoi(env) : SDL_GetNumLogicalCPUCores();
    int i;
    if (n > PPU_MAX_THREADS)
        n = PPU_MAX_THREADS;
    sPpuWorkers = 0;
    sPpuVerify = SDL_getenv("KHCOM_PPU_VERIFY") != NULL;
    if (n < 2 || !(sPpuGo = SDL_CreateSemaphore(0)) || !(sPpuDone = SDL_CreateSemaphore(0)))
        return;
    for (i = 0; i < n - 1; i++) {
        SDL_Thread* th = SDL_CreateThread(PpuWorker, "ppu", (void*)(intptr_t)i);
        if (!th)
            break;
        SDL_DetachThread(th);
        sPpuWorkers++;
    }
    PortLog("ppu: %d cores for the rows beyond the picture%s", sPpuWorkers + 1, sPpuVerify ? " (verified)" : "");
}

static int PpuGlyphOrder(const void* a, const void* b) {
    const PendingGlyph *x = *(const PendingGlyph* const*)a, *y = *(const PendingGlyph* const*)b;
    if (x->row != y->row)
        return x->row < y->row ? -1 : 1;
    return x < y ? -1 : x > y; /* a row is one core's, kept in its order */
}

/* state that a row hands on to the next one: those frames stay on one core */
/* KHCOM_PROFILE: outer-row batches drawn on several cores, and why the others stayed on one */
static int sPpuStats[5]; /* multi-core, of them with the iso floor, one core: vertical mosaic, menu header, too small */

void PortPpuTakeStats(int out[5]) {
    memcpy(out, sPpuStats, sizeof(sPpuStats));
    memset(sPpuStats, 0, sizeof(sPpuStats));
}

static int PpuRowsIndependent(void) {
    int i;
    if (sBgIsoLatched[0] | sBgIsoLatched[1] | sBgIsoLatched[2] | sBgIsoLatched[3])
        sPpuStats[1]++; /* drawn on several cores: IsoPrime fills the tables first */
    if ((IO16(0x4C) >> 4) & 15) {
        sPpuStats[2]++;
        return 0; /* vertical mosaic: affine BGs reuse the reference of a block's first row */
    }
    for (i = 0; i < 128; i++)
        if (sOamTag[i].header) {
            sPpuStats[3]++;
            return 0; /* HeaderPick's once-a-frame choice */
        }
    sPpuStats[0]++;
    return 1;
}

static int GlyphSame(const BgTextGlyph* a, const BgTextGlyph* b) {
    return !memcmp(&a->glyph, &b->glyph, sizeof(a->glyph)) && a->bg == b->bg && a->priority == b->priority &&
           a->screen == b->screen;
}

/* KHCOM_PPU_VERIFY: rows drawn twice, once on several cores and once line by line on this one, must be the same:
   PpuSaveRows keeps what the first drawing left (and takes its glyphs off the list), PpuCheckRows compares */
static uint8_t* sVerifySaved;
static size_t sVerifySavedSize;
static BgTextGlyph sVerifyGlyphs[1024];
static int sVerifyGlyphCount;

static int PpuSaveRows(int y0, int y1, int glyphStart) {
    size_t w = (size_t)gPortScreen.width, off = (size_t)y0 * w, n = (size_t)(y1 - y0) * w;
    size_t need = n * 13; /* frame, background (4 bytes each), three masks (1), sprite occlusion (2) */
    uint8_t* s;
    if (need > sVerifySavedSize) {
        uint8_t* grown = (uint8_t*)realloc(sVerifySaved, need);
        if (!grown)
            return 0;
        sVerifySaved = grown;
        sVerifySavedSize = need;
    }
    s = sVerifySaved;
    memcpy(s, gPortFrame + off, n * 4);
    memcpy(s + n * 4, gPortMenuBackground + off, n * 4);
    memcpy(s + n * 8, sOccBg + off, n);
    memcpy(s + n * 9, sOccBgLayer + off, n);
    memcpy(s + n * 10, sMsgMask + off, n);
    memcpy(s + n * 11, sOccObj + off, n * 2);
    sVerifyGlyphCount = sBgTextCount - glyphStart;
    memcpy(sVerifyGlyphs, sBgTextGlyphs + glyphStart, (size_t)sVerifyGlyphCount * sizeof(*sVerifyGlyphs));
    sBgTextCount = glyphStart;
    return 1;
}

static void PpuCheckRows(int y0, int y1, int glyphStart, const char* what) {
    size_t w = (size_t)gPortScreen.width, rows = (size_t)(y1 - y0), off = (size_t)y0 * w, n = rows * w;
    const uint8_t* s = sVerifySaved;
    int y, bad = -1, i, count = sVerifyGlyphCount;
    for (y = 0; y < (int)rows && bad < 0; y++) {
        size_t o = (size_t)y * w;
        if (memcmp(s + o * 4, gPortFrame + off + o, w * 4) ||
            memcmp(s + n * 4 + o * 4, gPortMenuBackground + off + o, w * 4) ||
            memcmp(s + n * 8 + o, sOccBg + off + o, w) || memcmp(s + n * 9 + o, sOccBgLayer + off + o, w) ||
            memcmp(s + n * 10 + o, sMsgMask + off + o, w) || memcmp(s + n * 11 + o * 2, sOccObj + off + o, w * 2))
            bad = y0 + y;
    }
    if (bad < 0 && sBgTextCount - glyphStart != count)
        bad = -2;
    for (i = 0; bad < 0 && i < count; i++)
        if (!GlyphSame(&sVerifyGlyphs[i], &sBgTextGlyphs[glyphStart + i]))
            bad = -3;
    if (bad != -1 && sPpuMismatches++ < 20)
        PortLog("ppu verify: MISMATCH %s frame %u rows %d-%d (%s %d)", what, (unsigned)PortFrameCount(), y0, y1,
                bad >= 0 ? "row" : "glyphs", bad);
}

static void PpuVerify(const OuterRows* r, int glyphStart) {
    int y;
    if (!PpuSaveRows(r->y0, r->y1, glyphStart))
        return;
    for (y = r->y0; y < r->y1; y++)
        OuterRow(r, y);
    PpuCheckRows(r->y0, r->y1, glyphStart, "margins");
}

/* Rows y0..y1 on every core; the glyphs found are merged into the list in row order. */
static void PpuRunBatch(const OuterRows* job, int y0, int y1) {
    static const PendingGlyph* order[PPU_MAX_THREADS * 1024];
    int i, slot, n = 0;
    if (sBgWorldLatched[1].fn || sBgWorldLatched[2].fn || sBgWorldLatched[3].fn)
        MapWorldTilePrime(); /* its per-room tables, filled before the cores read them */
    IsoPrime();
    sPpuJob = job;
    sPpuEnd = y1;
    SDL_SetAtomicInt(&sPpuNext, y0);
    for (i = 0; i < sPpuWorkers; i++)
        SDL_SignalSemaphore(sPpuGo);
    PpuRows(sPpuWorkers);
    for (i = 0; i < sPpuWorkers; i++)
        SDL_WaitSemaphore(sPpuDone);
    for (slot = 0; slot <= sPpuWorkers; slot++)
        for (i = 0; i < sPpuGlyphCount[slot]; i++)
            order[n++] = &sPpuGlyphs[slot][i];
    if (n > 1)
        qsort(order, (size_t)n, sizeof(*order), PpuGlyphOrder);
    for (i = 0; i < n && sBgTextCount < 1024; i++)
        sBgTextGlyphs[sBgTextCount++] = order[i]->g;
}

/* ---- the GBA picture's rows on several cores ----------------------------------------------------------------------
 * Between the picture's lines the game's line events run (V-counter and HBlank interrupts, HBlank DMA): what they
 * change shows from the next line on. So the frame's line events run first, each line keeping what its drawing
 * reads that they can change (the display registers, the affine reference point, and the palettes and OAM when
 * they differ from the line before), and then the rows are drawn from those copies on every core like the margins'.
 * The result is the line-by-line one, pixel for pixel: KHCOM_PPU_VERIFY=1 also draws each frame line by line and
 * logs any difference. Frames with state carried from row to row stay line by line (PpuRowsIndependent, the title's
 * faded art); a vertical mosaic switched on halfway draws the rows kept so far and goes on line by line.
 * On by default on the Switch (PORT_SWITCH: at 1280x720 the picture is 160 of the canvas's 180 rows), elsewhere with
 * KHCOM_PPU_INNER=1; KHCOM_PPU_INNER=0 turns it off. */
typedef struct PpuLineState {
    uint8_t io[PPU_IO_KEPT];
    int32_t affX[2], affY[2];
    const uint16_t *pltt, *oam;
} PpuLineState;
static PpuLineState sPpuLines[GBA_SCREEN_H];
static uint16_t sPpuPltt[GBA_SCREEN_H][512], sPpuOam[GBA_SCREEN_H][512];
static int sPpuPlttKept, sPpuOamKept, sPpuInnerFrom, sPpuInnerOn = -1, sPpuInnerGlyphs, sPpuInnerFrames, sPpuInnerShared;

static void PpuInnerRow(int y) {
    int gy = y - gPortScreen.originY;
    const PpuLineState* l = &sPpuLines[gy];
    sPpuIoView = l->io;
    sPpuPlttView = l->pltt;
    sPpuOamView = l->oam;
    sAffX[0] = l->affX[0]; sAffX[1] = l->affX[1];
    sAffY[0] = l->affY[0]; sAffY[1] = l->affY[1];
    RenderLine(y, gy);
    sPpuIoView = NULL;
    sPpuPlttView = sPpuOamView = NULL;
}

/* Whether this frame's picture rows wait for the line events (PpuInnerKeep) */
static int PpuInnerBegin(void) {
    if (sPpuInnerOn < 0) {
        const char* env = SDL_getenv("KHCOM_PPU_INNER");
#ifdef PORT_SWITCH
        sPpuInnerOn = !env || SDL_atoi(env) != 0;
#else
        sPpuInnerOn = env && SDL_atoi(env) != 0;
#endif
        if (sPpuInnerOn)
            PortLog("ppu: the picture's rows on several cores too");
    }
    if (sPpuWorkers < 0)
        PpuStart();
    if (sPpuInnerOn && ++sPpuInnerFrames == 600) { /* how often it acts, in the log the consoles' players send */
        PortLog("ppu: the picture's rows on %d cores in %d of the last 600 frames", sPpuWorkers + 1, sPpuInnerShared);
        sPpuInnerFrames = sPpuInnerShared = 0;
    }
    if (!sPpuInnerOn || sPpuWorkers <= 0 || sTitleArtFade || (IO16(0x00) & 0x80) || !PpuRowsIndependent())
        return 0;
    sPpuInnerShared++;
    sPpuPlttKept = sPpuOamKept = -1;
    sPpuInnerFrom = 0;
    sPpuInnerGlyphs = sBgTextCount;
    return 1;
}

static void PpuInnerDraw(int gy1);

/* Line gy, after its LineStart and LatchAffine: keep what its drawing reads. 0: draw this line and the rest now. */
static int PpuInnerKeep(int gy) {
    PpuLineState* l = &sPpuLines[gy];
    if ((IO16(0x4C) >> 4) & 15) { /* a vertical mosaic: the rows carry its reference */
        PpuInnerDraw(gy);
        return 0;
    }
    memcpy(l->io, (const void*)(uintptr_t)GBA_IO, PPU_IO_KEPT);
    l->affX[0] = sAffX[0]; l->affX[1] = sAffX[1];
    l->affY[0] = sAffY[0]; l->affY[1] = sAffY[1];
    if (sPpuPlttKept < 0 || memcmp(sPpuPltt[sPpuPlttKept], (const void*)(uintptr_t)GBA_PLTT, 1024))
        memcpy(sPpuPltt[++sPpuPlttKept], (const void*)(uintptr_t)GBA_PLTT, 1024);
    if (sPpuOamKept < 0 || memcmp(sPpuOam[sPpuOamKept], (const void*)(uintptr_t)GBA_OAM, 1024))
        memcpy(sPpuOam[++sPpuOamKept], (const void*)(uintptr_t)GBA_OAM, 1024);
    l->pltt = sPpuPltt[sPpuPlttKept];
    l->oam = sPpuOam[sPpuOamKept];
    if (sPpuVerify) { /* the line-by-line drawing to compare with */
        sPpuIoView = NULL;
        RenderLine(gy + gPortScreen.originY, gy);
    }
    return 1;
}

/* The kept rows 0..gy1 of the picture, on every core */
static void PpuInnerDraw(int gy1) {
    int32_t affX[2] = { sAffX[0], sAffX[1] }, affY[2] = { sAffY[0], sAffY[1] };
    int y0 = gPortScreen.originY + sPpuInnerFrom, y1 = gPortScreen.originY + gy1, check = 0;
    if (y1 <= y0)
        return;
    if (sPpuVerify)
        check = PpuSaveRows(y0, y1, sPpuInnerGlyphs);
    PpuRunBatch(NULL, y0, y1);
    if (check)
        PpuCheckRows(y0, y1, sPpuInnerGlyphs, "picture");
    sAffX[0] = affX[0]; sAffX[1] = affX[1]; /* this core drew rows too: the line events' reference goes on */
    sAffY[0] = affY[0]; sAffY[1] = affY[1];
    sPpuInnerFrom = gy1;
}
#endif

static void RenderOuterRows(int y0, int y1, int below) {
    OuterRows r;
    int a, drawn = 0;
    if (y1 <= y0)
        return;
    r.y0 = y0;
    r.y1 = y1;
    r.below = below;
    for (a = 0; a < 2; a++) {
        r.affX[a] = sAffX[a];
        r.affY[a] = sAffY[a];
    }
#ifdef PORT_PPU_THREADS
    if (sPpuWorkers < 0)
        PpuStart();
    if (sPpuWorkers > 0 && (size_t)(y1 - y0) * gPortScreen.width < 64 * 1024)
        sPpuStats[4]++;
    if (sPpuWorkers > 0 && (size_t)(y1 - y0) * gPortScreen.width >= 64 * 1024 && PpuRowsIndependent()) {
        int glyphStart = sBgTextCount;
        PpuRunBatch(&r, y0, y1);
        if (sPpuVerify)
            PpuVerify(&r, glyphStart);
    } else
#endif
    {
        int y;
        for (y = y0; y < y1; y++)
            OuterRow(&r, y);
    }
    /* the reference point the single-core loops left: line 0's above, one step per drawn row below */
    if (below)
        for (a = y0; a < y1; a++)
            drawn += a - gPortScreen.originY < sBottom;
    for (a = 0; a < 2; a++) {
        sAffX[a] = r.affX[a] + (int16_t)IO16(0x22 + a * 0x10) * drawn;
        sAffY[a] = r.affY[a] + (int16_t)IO16(0x26 + a * 0x10) * drawn;
    }
}

/* The sprite palette and the blend registers the picture is drawn with: the HD text is presented after the vertical
   blank, when the game has already loaded the next frame's palette, so a fade's text came out a frame ahead of the
   picture (the deck editor's L/R: the words over a black screen, 23/10). */
static uint16_t sTextObjPalette[256], sTextBld[3];
static int sTextLatched;

void PortPpuRenderFrame(void) {
    int gy;
    int w = gPortScreen.width;

    memcpy(sTextObjPalette, PLTT16 + 256, sizeof(sTextObjPalette));
    sTextBld[0] = IO16(0x50);
    sTextBld[1] = IO16(0x52);
    sTextBld[2] = IO16(0x54);
    sTextLatched = 1;
    PortPpuBeginFrame();
    LatchAffine(1);
    QueueMainHdParts();
    ExportSprites();

    /* Lines above the original picture: state of line 0. */
    RenderOuterRows(0, gPortScreen.originY, 0);

#ifdef PORT_PPU_THREADS
    if (PpuInnerBegin()) {
        int kept = 1;
        for (gy = 0; gy < GBA_SCREEN_H; gy++) {
            LineStart(gy);
            LatchAffine(0);
            if (kept)
                kept = PpuInnerKeep(gy);
            if (!kept)
                RenderLine(gy + gPortScreen.originY, gy);
            StepAffine(1);
            HBlank(gy);
        }
        if (kept)
            PpuInnerDraw(GBA_SCREEN_H);
    } else
#endif
    for (gy = 0; gy < GBA_SCREEN_H; gy++) {
        LineStart(gy);
        LatchAffine(0);
        RenderLine(gy + gPortScreen.originY, gy);
        StepAffine(1);
        HBlank(gy);
    }
    if (!sLastFrameBlank) {
        /* the game's own fades rewrite the palettes: a picture of one colour
           (all white or all black) is a blank frame of the transition too */
        const uint32_t* row = gPortFrame + gPortScreen.originY * w + gPortScreen.originX;
        uint32_t first = row[0];
        int x, uniform = 1;
        for (gy = 0; gy < GBA_SCREEN_H && uniform; gy += 2, row += 2 * w)
            for (x = 0; x < GBA_SCREEN_W; x += 2)
                if (row[x] != first) { uniform = 0; break; }
        sLastFrameBlank = uniform;
    }

    /* Lines below the original picture: state after line 159. */
    RenderOuterRows(gPortScreen.originY + GBA_SCREEN_H, gPortScreen.height, 1);
    MainHdOver();
    CaptureMenuTabPixels();
}

/* A skipped frame (frame_skip, the device is late): the visible lines' events reach the game as in a drawn frame
   (V-counter and HBlank interrupts, HBlank DMA), nothing is drawn and the canvas keeps the last picture. */
void PortPpuRunFrameEvents(void) {
    int gy;

    for (gy = 0; gy < GBA_SCREEN_H; gy++) {
        LineStart(gy);
        HBlank(gy);
    }
}

/* Remaining lines of the frame (VBlank period) after the VBlank interrupt. */
void PortPpuVBlankLines(void) {
    int line;

    for (line = GBA_SCREEN_H + 1; line < 228; line++) {
        LineStart(line);
        HBlank(line);
    }
    IO16(0x06) = 0;
}

/* ---- HUD layer ----------------------------------------------------------------
 * HUD sprites (drawn between PortHudBegin/End) are rendered into their own
 * ARGB buffer, gPortHudW x gPortHudH, which the presenter scales by
 * gPortHudScale: larger than the world while staying pixel-exact. Each
 * sprite keeps its distance to the screen edge it is anchored to.
 */
void PortMenuTabBg(int side, int bg, int x, int y) {
    int i = side > 0;
    sMenuPend.tabBg[i] = bg; sMenuPend.tabX[i] = x; sMenuPend.tabY[i] = y;
    sMenuPend.tabFrame = NULL;
}
void PortMenuTabSprite(void* frame, void* tiles, void* palette) {
    sMenuPend.tabFrame = frame; sMenuPend.tabTiles = tiles; sMenuPend.tabPalette = palette;
}

void PortMenuHeaderSource(void* frame, void* tiles, void* palette) {
    sMenuPend.headerFrame = frame;
    sMenuPend.headerTiles = tiles;
    sMenuPend.headerPalette = palette;
    if (!frame) {
        memset(sMenuPend.bands, 0, sizeof(sMenuPend.bands));
        sMenuPend.tabFrame = NULL;
        sMenuPend.tabBg[0] = sMenuPend.tabBg[1] = -1;
        sMenuShown = sMenuPend; /* a new mode: nothing of the old one stays */
        sMenuModeStart = PortFrameCount();
    }
}

static int ObjColor(uint16_t a0, uint16_t a2, int tx, int ty, int ow, int mapping1d) {
    int bpp8 = (a0 >> 13) & 1;
    int tileBase = a2 & 0x3FF;
    int tile, ci;
    uint32_t off;

    if (bpp8) {
        tile = mapping1d ? tileBase + ((ty >> 3) * (ow >> 3) + (tx >> 3)) * 2 : tileBase + (ty >> 3) * 32 + (tx >> 3) * 2;
        off = 0x10000 + (tile & 0x3FF) * 32 + (ty & 7) * 8 + (tx & 7);
        ci = VRAM8[off];
        return ci ? (PLTT16[256 + ci] & 0x7FFF) : -1;
    }
    tile = mapping1d ? tileBase + (ty >> 3) * (ow >> 3) + (tx >> 3) : tileBase + (ty >> 3) * 32 + (tx >> 3);
    off = 0x10000 + (tile & 0x3FF) * 32 + (ty & 7) * 4 + ((tx & 7) >> 1);
    ci = (tx & 1) ? (VRAM8[off] >> 4) : (VRAM8[off] & 15);
    return ci ? (PLTT16[256 + (a2 >> 12) * 16 + ci] & 0x7FFF) : -1;
}

/* Development aid for the HD inventory (KHCOM_HDSPRITE_EXPORT=dir): every sprite drawn, assembled in its frame's
   coordinates with the palette it is shown with, saved once as <dir>/<tiles address>_<frame address>.bmp;
   port/tools/hdsprite_catalog.py names them from the ROM symbols. */
#if defined(PORT_PPU_THREADS) && !defined(PORT_GBA_RELOC) /* the desktop builds, where SDL is at hand */
typedef struct ExportCanvas {
    const void *tiles, *frame;
    uint32_t px[256 * 256]; /* frame coordinates -128..127 */
    int used;
} ExportCanvas;

static void ExportSprites(void) {
    static const char* dir;
    static int checked;
    static const void* (*seen)[2];
    static int seenCount;
    ExportCanvas* canvas;
    int i, n = 0, mapping1d = (IO16(0x00) >> 6) & 1;
    if (!checked) {
        dir = SDL_getenv("KHCOM_HDSPRITE_EXPORT");
        checked = 1;
    }
    if (!dir)
        return;
    if (!seen)
        seen = calloc(8192, sizeof(*seen));
    canvas = calloc(32, sizeof(*canvas));
    if (!seen || !canvas) {
        free(canvas);
        return;
    }
    for (i = 0; i < 128; i++) {
        uint16_t a0 = OAM16[i * 4 + 0], a1 = OAM16[i * 4 + 1], a2 = OAM16[i * 4 + 2];
        int affine = (a0 >> 8) & 1, dbl = (a0 >> 9) & 1, shape = (a0 >> 14) & 3, size = (a1 >> 14) & 3;
        int ow, oh, k, x, y, hf, vf;
        const OamHdPart* part = &sOamHd[i];
        ExportCanvas* c = NULL;
        if ((!affine && dbl) || shape == 3 || !part->tiles || !part->w)
            continue;
        for (k = 0; k < seenCount; k++)
            if (seen[k][0] == part->tiles && seen[k][1] == part->frame)
                break;
        if (k < seenCount)
            continue;
        for (k = 0; k < n; k++)
            if (canvas[k].tiles == part->tiles && canvas[k].frame == part->frame)
                c = &canvas[k];
        if (!c) {
            if (n == 32)
                continue;
            c = &canvas[n++];
            c->tiles = part->tiles;
            c->frame = part->frame;
        }
        ow = sObjSize[shape][size][0];
        oh = sObjSize[shape][size][1];
        /* the part's own flips: the OAM's less the entry's (affine parts have none) */
        hf = affine ? 0 : (((a1 >> 12) & 1) ^ (part->flags & 1));
        vf = affine ? 0 : (((a1 >> 13) & 1) ^ ((part->flags >> 1) & 1));
        for (y = 0; y < oh; y++)
            for (x = 0; x < ow; x++) {
                int col = ObjColor(a0, a2, hf ? ow - 1 - x : x, vf ? oh - 1 - y : y, ow, mapping1d);
                int lx = part->x + x + 128, ly = part->y + y + 128;
                if (col >= 0 && lx >= 0 && ly >= 0 && lx < 256 && ly < 256) {
                    c->px[ly * 256 + lx] = 0xFF000000u | sColorLut[col];
                    c->used = 1;
                }
            }
    }
    for (i = 0; i < n; i++) {
        char path[512];
        SDL_Surface* s;
        if (!canvas[i].used || seenCount == 8192)
            continue;
        seen[seenCount][0] = canvas[i].tiles;
        seen[seenCount][1] = canvas[i].frame;
        seenCount++;
        SDL_snprintf(path, sizeof(path), "%s/%08x_%08x.bmp", dir, (unsigned)(uintptr_t)canvas[i].tiles,
                     (unsigned)(uintptr_t)canvas[i].frame);
        s = SDL_CreateSurfaceFrom(256, 256, SDL_PIXELFORMAT_ARGB8888, canvas[i].px, 256 * 4);
        if (s) {
            SDL_SaveBMP(s, path);
            SDL_DestroySurface(s);
        }
    }
    free(canvas);
}
#else
static void ExportSprites(void) {
}
#endif

/* Colour of the header asset at (tx, y), tx counted from its left edge; -1
   when transparent or not drawable. Neither cursor OAM nor the finished
   framebuffer enters it. */
static int MenuSpriteColor(const uint16_t* frame, const ObjTiles* tiles,
                           const ObjPalette* palette, int y, int col0, int* top, int* bottom) {
    const uint16_t* p = frame;
    int n, i, left = 32767, offset = 0, col = -1;
    if (!p || !tiles || !palette ||
        tiles->self != tiles || palette->self != palette ||
        (IO16(0) & 0x80) || !(IO16(0) & 0x1000))
        return -2;
    n = *p++;
    if (n > 128) return -2;
    for (i = 0; i < n; i++) {
        int x = p[i * 3 + 1] & 511, oy = p[i * 3] & 255, shape = p[i * 3] >> 14;
        if (x >= 256) x -= 512;
        if (x < left) left = x;
        if (oy >= 128) oy -= 256;
        if (top && shape < 3) {
            int oh = sObjSize[shape][p[i * 3 + 1] >> 14][1];
            if (oy < *top) *top = oy;
            if (oy + oh > *bottom) *bottom = oy + oh;
        }
    }
    for (i = 0; i < n; i++) {
        uint16_t a0 = p[i * 3], a1 = p[i * 3 + 1], a2 = p[i * 3 + 2];
        int shape = a0 >> 14, size = a1 >> 14, x = a1 & 511, oy = a0 & 255;
        int ow, oh, tx, ty, tile;
        if (shape >= 3) continue;
        ow = sObjSize[shape][size][0]; oh = sObjSize[shape][size][1];
        if (x >= 256) x -= 512;
        if (oy >= 128) oy -= 256;
        tx = left + col0 - x; ty = y - oy;
        tile = tiles->allocated ? offset : (a2 & 1023);
        offset += (ow * oh / 64) * ((a0 & 0x2000) ? 2 : 1);
        if (tx < 0 || tx >= ow || ty < 0 || ty >= oh) continue;
        if (a1 & 0x1000) tx = ow - 1 - tx;
        if (a1 & 0x2000) ty = oh - 1 - ty;
        a2 = ((tile + tiles->index) & 1023) |
             (((a2 >> 12) + palette->index) << 12);
        col = ObjColor(a0, a2, tx, ty, ow, (IO16(0) & 0x40) != 0);
        if (col >= 0) break;
    }
    return col;
}

/* A repeatable piece of an asset, independent of its animated screen
   position: columns col0 and col0 + 1 from its left end. */
static uint32_t MenuSpritePixel(const uint16_t* frame, const ObjTiles* tiles,
                                const ObjPalette* palette, int y, int phase, int col0) {
    int col = MenuSpriteColor(frame, tiles, palette, y, col0 + (phase & 1), NULL, NULL);
    if (col < 0) return 0;
    if (IO16(0x50) & 0x10) {
        int effect = (IO16(0x50) >> 6) & 3, evy = IO16(0x54) & 31;
        if (evy > 16) evy = 16;
        if (effect == 2) col = Brighten(col, evy);
        if (effect == 3) col = Darken(col, evy);
    }
    return sColorLut[col];
}

/* The header's band: the pair of columns near its left end with the fewest
   colour changes top to bottom, so a title that starts at the very edge
   (US "WORLD CARDS") does not lend a letter stroke to the band. Its colours
   are kept: outside Europe the title is redrawn on a BG once it has slid in
   and its sprite palette is released, while the band must stay. */
static int sHeaderBand[2][32];

static int HeaderPick(void) {
    static const void* key[3];
    static int pick, valid;
    static uint32_t stamp;

    /* once a frame: the asset's tiles and palette arrive after it is first drawn */
    if (stamp != PortFrameCount() || key[0] != sMenuHeaderFrame || key[1] != sMenuHeaderTiles ||
        key[2] != sMenuHeaderPalette) {
        int top = 32767, bottom = -32767, best = 1 << 30, k, j, yy;
        stamp = PortFrameCount();
        if (MenuSpriteColor(sMenuHeaderFrame, sMenuHeaderTiles, sMenuHeaderPalette, 0, 0, &top, &bottom) == -2)
            return sMenuHeaderFrame && key[0] == sMenuHeaderFrame && valid ? pick : -1;
        valid = 0;
        pick = 0;
        /* most opaque first (the band, not its transparent ends or the
           arrow), then fewest changes (no lettering) */
        for (k = 0; k < 96; k += 2) {
            int changes = 0, opaque = 0, score;
            for (j = 0; j < 2; j++) {
                int prev = -3;
                for (yy = top; yy < bottom && yy < 32; yy++) {
                    int c = MenuSpriteColor(sMenuHeaderFrame, sMenuHeaderTiles, sMenuHeaderPalette, yy, k + j, NULL, NULL);
                    if (c >= 0) opaque++;
                    if (yy > top && c != prev) changes++;
                    prev = c;
                }
            }
            score = changes - opaque * 64;
            if (opaque > 0 && score < best) {
                best = score;
                pick = k;
            }
        }
        for (j = 0; j < 2; j++)
            for (yy = 0; yy < 32; yy++)
                sHeaderBand[j][yy] = MenuSpriteColor(sMenuHeaderFrame, sMenuHeaderTiles, sMenuHeaderPalette, yy, pick + j, NULL, NULL);
        key[0] = sMenuHeaderFrame; key[1] = sMenuHeaderTiles; key[2] = sMenuHeaderPalette;
        valid = 1;
    }
    return valid || key[0] == sMenuHeaderFrame ? pick : -1;
}

static int HeaderBandColor(int y, int phase) {
    if (HeaderPick() < 0 || y < 0 || y >= 32)
        return -1;
    return sHeaderBand[phase & 1][y];
}

static uint32_t HeaderPixelRaw(int y, int phase);

/* Is the header on the picture now? Its band must be in the picture's left
   column: while a menu is still coming in (before its title has appeared)
   the margin must not show the band. Checked once a frame, on GBA-size
   pictures only. */
static int HeaderOnScreen(void) {
    static uint32_t checked = ~0u;
    static int shown;
    int yy, opaque = 0, match = 0;
    if (gPortScreen.wide)
        return 1;
    if (checked == PortFrameCount())
        return shown;
    checked = PortFrameCount();
    for (yy = 0; yy < 16 && gPortScreen.originY + yy < gPortScreen.height; yy++) {
        uint32_t a = HeaderPixelRaw(yy, 0) & 0xFFFFFF, b = HeaderPixelRaw(yy, 1) & 0xFFFFFF;
        uint32_t px = gPortFrame[(gPortScreen.originY + yy) * gPortScreen.width + gPortScreen.originX] & 0xFFFFFF;
        if (!a && !b)
            continue;
        opaque++;
        match += px == a || px == b;
    }
    shown = opaque > 0 && match * 2 >= opaque;
    return shown;
}

/* Where the header sprite was drawn (DrawMenuHeader), so its extension in
   the margin travels with it: the margin shows the sprite's own pixels
   where it reaches past the picture's edge, and the band only to its left,
   as if the screen had always been that wide. */
void PortMenuHeaderAt(int x) {
    sMenuPend.headerX = x;
    sMenuPend.headerDrawn = 1;
}

static int HeaderLeft(void) {
    const uint16_t* p = sMenuHeaderFrame;
    int n, i, left = 32767;
    if (!p) return 0;
    n = *p++;
    if (n <= 0 || n > 128) return 0;
    for (i = 0; i < n; i++) {
        int x = p[i * 3 + 1] & 511;
        if (x >= 256) x -= 512;
        if (x < left) left = x;
    }
    return left;
}

uint32_t PortMenuHeaderPixelAt(int y, int d) {
    int c = -1 - d, s;
    if (!sMenuHeaderFrame || !sMenuShown.headerDrawn)
        return PortMenuHeaderPixel(y, d);
    s = c - sMenuShown.headerX - HeaderLeft();
    if (s >= 0)
        return MenuSpritePixel(sMenuHeaderFrame, sMenuHeaderTiles, sMenuHeaderPalette, y, 0, s);
    return HeaderPixelRaw(y, d);
}

uint32_t PortMenuHeaderPixel(int y, int phase) {
    if (!sMenuHeaderFrame || !HeaderOnScreen())
        return 0;
    return HeaderPixelRaw(y, phase);
}

static uint32_t HeaderPixelRaw(int y, int phase) {
    int col;
    if (!sMenuHeaderFrame)
        return 0;
    col = HeaderBandColor(y, phase);
    if (col < 0) return 0;
    if (IO16(0x50) & 0x10) {
        int effect = (IO16(0x50) >> 6) & 3, evy = IO16(0x54) & 31;
        if (evy > 16) evy = 16;
        if (effect == 2) col = Brighten(col, evy);
        if (effect == 3) col = Darken(col, evy);
    }
    return sColorLut[col];
}

void PortMenuBandSource(int band, int y, void* frame, void* tiles, void* palette) {
    if ((unsigned)band >= 2) return;
    sMenuPend.bands[band].frame = frame;
    sMenuPend.bands[band].tiles = tiles;
    sMenuPend.bands[band].palette = palette;
    sMenuPend.bands[band].y = y;
}

uint32_t PortMenuBandPixel(int y, int phase) {
    int i;
    for (i = 0; i < 2; i++) {
        const MenuBandSource* band = &sMenuBands[i];
        uint32_t color = MenuSpritePixel(band->frame, band->tiles, band->palette, y - band->y, phase, 0);
        if (color) return color;
    }
    return 0;
}

static uint32_t MenuTabPixelRaw(int y, int phase, int side) {
    int i = side > 0, bg = sMenuTabBg[i], x, yy, ci, col;
    uint16_t cnt, entry;
    uint32_t map, chr, address;
    if (sMenuTabFrame && side < 0)
        return MenuSpritePixel(sMenuTabFrame, sMenuTabTiles, sMenuTabPalette, y - 16, 0, 0);
    if (bg < 0 || y < 16 || y >= 32 || (IO16(0) & 0x80)) return 0;
    cnt = IO16(8 + bg * 2);
    map = ((cnt >> 8) & 31) * 2048; chr = ((cnt >> 2) & 3) * 16384;
    /* Read the clean end of the authored tab tile, ignoring animated scroll
       and all foreground OAM. It remains the same piece when the hand moves. */
    x = sMenuTabX[i]; yy = sMenuTabY[i] + y - 16;
    entry = ((const uint16_t*)VRAM8)[(map + ((yy >> 3) * 32 + (x >> 3)) * 2) / 2];
    x &= 7; yy &= 7;
    if (entry & 0x400) x = 7 - x;
    if (entry & 0x800) yy = 7 - yy;
    address = chr + (entry & 1023) * ((cnt & 0x80) ? 64 : 32);
    if (cnt & 0x80) {
        ci = VRAM8[address + yy * 8 + x];
        col = ci;
    } else {
        int packed = VRAM8[address + yy * 4 + x / 2];
        ci = (x & 1) ? packed >> 4 : packed & 15;
        col = (entry >> 12) * 16 + ci;
    }
    if (!ci) return 0;
    col = PLTT16[col] & 0x7FFF;
    if (IO16(0x50) & (1 << bg)) {
        int effect = (IO16(0x50) >> 6) & 3, evy = IO16(0x54) & 31;
        if (evy > 16) evy = 16;
        if (effect == 2) col = Brighten(col, evy);
        if (effect == 3) col = Darken(col, evy);
    }
    return sColorLut[col];
}

static void CaptureMenuTabPixels(void) {
    int side, y;
    for (side = 0; side < 2; side++)
        for (y = 0; y < 16; y++)
            sMenuTabPixels[side][y] = MenuTabPixelRaw(y + 16, 0, side ? 1 : -1);
}

uint32_t PortMenuTabPixel(int y, int phase, int side) {
    if (sMenuTabFrame && side < 0)
        return MenuTabPixelRaw(y, phase, side);
    return y >= 16 && y < 32 ? sMenuTabPixels[side > 0][y - 16] : 0;
}

/* The backdrop colour (palette 0), as displayed. */
uint32_t PortPpuBackdrop(void) {
    return sColorLut[PLTT16[0] & 0x7FFF];
}

/* HD sprites on the HUD: where an HD part was queued (its cartridge pixels), and what the sprites in front of one
   draw there, which goes on a layer over the HD parts. */
static uint8_t* sHdCover;
static size_t sHdCoverSize;
static uint32_t* sHudOver;
static size_t sHudOverSize;
static int sHudOverUsed;

const uint32_t* PortPpuHudOver(void) {
    return sHudOverUsed ? sHudOver : NULL;
}

/* Queue OAM slot i (shown at hx, hy on the HUD canvas, bounding box bw x bh) as an HD part. */
static int QueueHdPart(int layer, int i, int hx, int hy, int ow, int oh, int bw, int bh, int affine,
                       int16_t pa, int16_t pb, int16_t pc, int16_t pd, int alpha) {
    const OamHdPart* part = &sOamHd[i];
    float x[4], y[4];
    int dark = 0, white = 0;
    /* the game fades by rewriting palettes (the title, menus): the HD picture follows its part's palette */
    if (part->palette && !((OAM16[i * 4] >> 13) & 1))
        PortPaletteFade(part->palette + part->bank * 16, PLTT16 + 256 + (OAM16[i * 4 + 2] >> 12) * 16, 16, &dark, &white);
    if (part->w != ow || part->h != oh)
        return 0;
    if (affine) {
        /* the hardware maps a screen offset s from the box's centre to texture t = M s + (ow, oh) / 2 */
        float det = (float)pa * pd - (float)pb * pc, cx = hx + bw * 0.5f, cy = hy + bh * 0.5f;
        static const int kCorner[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
        int k;
        if (det == 0)
            return 0;
        for (k = 0; k < 4; k++) {
            float tx = (kCorner[k][0] - 0.5f) * ow, ty = (kCorner[k][1] - 0.5f) * oh;
            x[k] = cx + 256.0f * (pd * tx - pb * ty) / det;
            y[k] = cy + 256.0f * (-pc * tx + pa * ty) / det;
        }
    } else {
        float l = (float)hx, r = (float)(hx + ow), t = (float)hy, b = (float)(hy + oh);
        if (part->flags & 1) { float v = l; l = r; r = v; }
        if (part->flags & 2) { float v = t; t = b; b = v; }
        x[0] = l; y[0] = t; x[1] = r; y[1] = t; x[2] = r; y[2] = b; x[3] = l; y[3] = b;
    }
    return PortHdSpriteQueue(layer, part->id, x, y, part->x, part->y, part->w, part->h, alpha, dark, white);
}

/* HD parts on the game's picture (the title's logo): queued once a frame, before the lines are drawn, and left out
   of the lines (DrawObjs). Drawn over the finished picture, so a part is meant to be in front of everything. */

/* HD pictures the game shows with no OAM entry: a menu title its menu has moved to a BG copy once it slid in, its
   tiles released (PortHdStillSprite). Queued with the next picture and drawn over it whole, in the fade it was given. */
typedef struct StillHd {
    int id;
    int16_t x, y;
    uint8_t screen, dark, white;
} StillHd;
static StillHd sStillHd[4];
static int sStillHdCount;

void PortHdStillSprite(const void* tiles, const void* frame, int x, int y, int dark, int white) {
    int id = PortHdSpriteFind(tiles, frame), i;
    if (!id)
        return;
    for (i = 0; i < sStillHdCount && sStillHd[i].id != id; i++)
        ;
    if (i == sStillHdCount) {
        if (sStillHdCount == (int)(sizeof(sStillHd) / sizeof(sStillHd[0])))
            return;
        sStillHdCount++;
    }
    sStillHd[i].id = id;
    sStillHd[i].x = (int16_t)x;
    sStillHd[i].y = (int16_t)y;
    sStillHd[i].screen = sScreenDepth > 0;
    sStillHd[i].dark = (uint8_t)(dark < 0 ? 0 : dark > 255 ? 255 : dark);
    sStillHd[i].white = (uint8_t)(white < 0 ? 0 : white > 255 ? 255 : white);
}

static void QueueStillHdParts(void) {
    int i;
    for (i = 0; i < sStillHdCount; i++) {
        const StillHd* s = &sStillHd[i];
        int layer = sOverlay && s->screen ? 2 : 0, x0, y0, w, h;
        float ox = layer == 2 ? (gPortOverlayW - GBA_SCREEN_W) / 2.0f : (float)gPortScreen.originX;
        float oy = layer == 2 ? 0.0f : (float)gPortScreen.originY, x[4], y[4];
        PortHdSpriteBox(s->id, &x0, &y0, &w, &h);
        x[0] = x[3] = ox + s->x + x0;
        x[1] = x[2] = x[0] + w;
        y[0] = y[1] = oy + s->y + y0;
        y[2] = y[3] = y[0] + h;
        PortHdSpriteQueue(layer, s->id, x, y, x0, y0, w, h, 255, s->dark, s->white);
    }
    sStillHdCount = 0;
}

static void QueueMainHdParts(void) {
    uint16_t bldcnt = IO16(0x50);
    int i, eva = IO16(0x52) & 31;
    PortHdSpritesBegin(0);
    PortHdSpritesBegin(2);
    memset(sMainHdPart, 0, sizeof(sMainHdPart));
    memset(sMainHdScreen, 0, sizeof(sMainHdScreen));
    sMainHdAny = sScreenHdAny = 0;
    sTitleLogoBgHd = 0;
    if (sTitleLogoBg && (IO16(0x00) & 0x400) && ((IO16(0x00) & 7) == 1 || (IO16(0x00) & 7) == 2)) {
        /* BG2 maps screen (sx, sy) to texture ((X + pa sx + pb sy) / 256, (Y + pc sx + pd sy) / 256): the
           texture's corners go back through the inverse to the canvas */
        int id = PortHdSpriteFind(gTitleLogoBgTiles, NULL);
        int16_t pa = (int16_t)IO16(0x20), pb = (int16_t)IO16(0x22), pc = (int16_t)IO16(0x24), pd = (int16_t)IO16(0x26);
        float det = (float)pa * pd - (float)pb * pc;
        if (id && det != 0) {
            static const int kCorner[4][2] = { { 0, 0 }, { 256, 0 }, { 256, 256 }, { 0, 256 } };
            /* a screen-layer BG (the intro) is drawn on the screen layer's canvas: the GBA's lines, centred */
            int screen = sOverlay && sScreenBg[2];
            float ox = screen ? (gPortOverlayW - GBA_SCREEN_W) / 2.0f : (float)gPortScreen.originX;
            float oy = screen ? 0.0f : (float)gPortScreen.originY;
            uint16_t bld = IO16(0x50);
            int effect = (bld >> 6) & 3, k, alpha = 255, dark = 0, white = 0;
            float x[4], y[4];
            for (k = 0; k < 4; k++) {
                float u = kCorner[k][0] * 256.0f - sAffX[0], v = kCorner[k][1] * 256.0f - sAffY[0];
                x[k] = (pd * u - pb * v) / det + ox;
                y[k] = (-pc * u + pa * v) / det + oy;
            }
            if (effect == 1 && (bld & 0x04))
                alpha = ((IO16(0x52) & 31) > 16 ? 16 : (IO16(0x52) & 31)) * 255 / 16;
            else if (effect == 1 && (bld & 0x400))
                alpha = (((IO16(0x52) >> 8) & 31) > 16 ? 16 : ((IO16(0x52) >> 8) & 31)) * 255 / 16;
            else if ((bld & 0x04) && effect == 2)
                white = ((IO16(0x54) & 31) > 16 ? 16 : (IO16(0x54) & 31)) * 255 / 16;
            else if ((bld & 0x04) && effect == 3)
                dark = ((IO16(0x54) & 31) > 16 ? 16 : (IO16(0x54) & 31)) * 255 / 16;
            {
                int pd2 = 0, pw2 = 0; /* the title fades its palette too */
                PortPaletteFade((const uint16_t*)gTitleLogoBgPalette, PLTT16 + (gBgPaletteBank[2] & 15) * 16, 80, &pd2, &pw2);
                if (pd2 > dark) dark = pd2;
                if (pw2 > white) white = pw2;
            }
            sTitleLogoBgHd = PortHdSpriteQueue(screen ? 2 : 0, id, x, y, 0, 0, 256, 256, alpha, dark, white);
        }
    }
    QueueStillHdParts();
    if (!(IO16(0x00) & 0x1000))
        return;
    for (i = 127; i >= 0; i--) {
        uint16_t a0 = OAM16[i * 4 + 0], a1 = OAM16[i * 4 + 1];
        int affine = (a0 >> 8) & 1, dbl = (a0 >> 9) & 1, mode = (a0 >> 10) & 3;
        int shape = (a0 >> 14) & 3, size = (a1 >> 14) & 3, ow, oh, bw, bh, ox, oy, alpha = 255;
        int16_t pa = 0x100, pb = 0, pc = 0, pd = 0x100;
        if (!sOamHd[i].id || (!affine && dbl) || shape == 3 || mode >= 2 || (sHudLayer && sOamTag[i].hud) ||
            (((a0 >> 12) & 1) && (IO16(0x4C) & 0xFF00)))
            continue; /* a sprite under a real mosaic (size > 1) keeps the cartridge's pixels */
        ow = sObjSize[shape][size][0];
        oh = sObjSize[shape][size][1];
        bw = ow << (affine && dbl);
        bh = oh << (affine && dbl);
        oy = a0 & 0xFF;
        ox = a1 & 0x1FF;
        if ((sOamExt[i].x & 0x1FF) == ox && (sOamExt[i].y & 0xFF) == oy) {
            ox = sOamExt[i].x;
            oy = sOamExt[i].y;
        } else {
            if (oy >= sBottom)
                oy -= 256;
            if (ox >= sRight)
                ox -= 512;
        }
        if (affine) {
            const uint16_t* p = OAM16 + ((a1 >> 9) & 31) * 16;
            pa = (int16_t)p[3];
            pb = (int16_t)p[7];
            pc = (int16_t)p[11];
            pd = (int16_t)p[15];
        }
        /* a semi-transparent sprite blends with the first target's weight */
        if (mode == 1 || (((bldcnt >> 6) & 3) == 1 && (bldcnt & 0x10)))
            alpha = (eva > 16 ? 16 : eva) * 255 / 16;
        if (sOverlay && sOamTag[i].screen) {
            /* a menu's sprite: on the screen layer, the GBA's lines centred in its width */
            int sx = ox + (gPortOverlayW - GBA_SCREEN_W) / 2;
            sMainHdPart[i] = (uint8_t)QueueHdPart(2, i, sx, oy, ow, oh, bw, bh, affine, pa, pb, pc, pd, alpha);
            if (sMainHdPart[i]) {
                sMainHdScreen[i] = 1;
                sMainHdBox[i][0] = (int16_t)sx;
                sMainHdBox[i][1] = (int16_t)oy;
                sMainHdBox[i][2] = (int16_t)bw;
                sMainHdBox[i][3] = (int16_t)bh;
                sScreenHdAny = 1;
            }
            continue;
        }
        sMainHdPart[i] = (uint8_t)QueueHdPart(0, i, ox + gPortScreen.originX, oy + gPortScreen.originY, ow, oh, bw, bh,
                                              affine, pa, pb, pc, pd, alpha);
        if (sMainHdPart[i]) {
            sMainHdBox[i][0] = (int16_t)(ox + gPortScreen.originX);
            sMainHdBox[i][1] = (int16_t)(oy + gPortScreen.originY);
            sMainHdBox[i][2] = (int16_t)bw;
            sMainHdBox[i][3] = (int16_t)bh;
            if (!sMainHdAny && !sHudLayer && !sHdText) {
                /* the lines collect the occupancy (who is in front where) for MainHdOver */
                memset(sOccBg, 4, (size_t)gPortScreen.width * gPortScreen.height);
                memset(sOccBgLayer, 4, (size_t)gPortScreen.width * gPortScreen.height);
                memset(sOccObj, 0xFF, (size_t)gPortScreen.width * gPortScreen.height * 2);
            }
            sMainHdAny = 1;
        }
    }
}

/* What the cartridge draws in front of an HD part of the game's picture (a dialogue over a card, the cursor's
   glove over a room card), or where a window hides the sprites: those pixels of the finished picture go on a
   layer over the HD parts, so the HD picture stays behind them as the cartridge's sprite did. */
static uint32_t* sMainOver;
static size_t sMainOverSize;
static int sMainOverUsed;
static uint32_t sScreenOver[PORT_MAX_W * GBA_SCREEN_H];
static int sScreenOverUsed;

const uint32_t* PortPpuMainOver(void) {
    return sMainOverUsed ? sMainOver : NULL;
}

/* The same for the screen layer's HD parts (menus), gPortOverlayW x 160. */
const uint32_t* PortPpuScreenOver(void) {
    return sScreenOverUsed ? sScreenOver : NULL;
}

/* One layer's parts: where the cartridge has something in front, its final pixel goes on `over`. */
static void HdOverLayer(int screen, int w, int h, const uint8_t* occBg, const uint16_t* occObj, const uint32_t* picture,
                        uint32_t* over, int* used) {
    int i;
    for (i = 0; i < 128; i++) {
        int prio = (OAM16[i * 4 + 2] >> 10) & 3, x0, y0, x1, y1, x, y;
        if (!sMainHdPart[i] || sMainHdScreen[i] != screen)
            continue;
        x0 = sMainHdBox[i][0] < 0 ? 0 : sMainHdBox[i][0];
        y0 = sMainHdBox[i][1] < 0 ? 0 : sMainHdBox[i][1];
        x1 = sMainHdBox[i][0] + sMainHdBox[i][2] > w ? w : sMainHdBox[i][0] + sMainHdBox[i][2];
        y1 = sMainHdBox[i][1] + sMainHdBox[i][3] > h ? h : sMainHdBox[i][1] + sMainHdBox[i][3];
        for (y = y0; y < y1; y++)
            for (x = x0; x < x1; x++) {
                int k = y * w + x;
                uint16_t obj = occObj[k];
                /* 0 / 0: a window hides the sprites here; a BG of a higher priority; a sprite before this one (not
                   one that keeps its cartridge pixels under its own HD part: that part is drawn over this one anyway,
                   and copying its pixels put the small card above's bottom over the cards below, 23/10) */
                if ((occBg[k] == 0 && obj == 0) || occBg[k] < prio ||
                    (obj != 0xFFFFu && obj < prio * 128 + i &&
                     !(sMainHdPart[obj & 127] && PortHdSpriteKeepsCartridge(sOamHd[obj & 127].id)))) {
                    if (!*used) {
                        memset(over, 0, (size_t)w * h * 4);
                        *used = 1;
                    }
                    over[k] = picture[k] | (screen ? 0 : 0xFF000000u);
                }
            }
    }
}

static void MainHdOver(void) {
    int w = gPortScreen.width, h = gPortScreen.height;
    sMainOverUsed = sScreenOverUsed = 0;
    if (sScreenHdAny)
        HdOverLayer(1, gPortOverlayW, GBA_SCREEN_H, sScreenOccBg, sScreenOccObj, gPortOverlay, sScreenOver,
                    &sScreenOverUsed);
    if (!sMainHdAny)
        return;
    if ((size_t)w * h > sMainOverSize) {
        uint32_t* over = realloc(sMainOver, (size_t)w * h * 4);
        if (!over)
            return;
        sMainOver = over;
        sMainOverSize = (size_t)w * h;
    }
    HdOverLayer(0, w, h, sOccBg, sOccObj, gPortFrame, sMainOver, &sMainOverUsed);
}

void PortPpuRenderHud(uint32_t* buf) {
    uint16_t dispcnt = IO16(0x00);
    uint16_t bldcnt = IO16(0x50);
    int effect = (bldcnt >> 6) & 3, evy = IO16(0x54) & 31;
    int mapping1d = (dispcnt >> 6) & 1;
    int w = gPortHudW, h = gPortHudH;
    int i;

    sEnemyHudRect[0] = w; sEnemyHudRect[1] = h;
    PortHdHudSpriteBegin();
    PortHdSpritesBegin(1);
    sHudOverUsed = 0;
    if (gPortConfig.hdCards && (size_t)w * h > sHdCoverSize) {
        uint8_t* cover = realloc(sHdCover, (size_t)w * h);
        uint32_t* over = realloc(sHudOver, (size_t)w * h * 4);
        if (cover) { sHdCover = cover; sHdCoverSize = (size_t)w * h; }
        if (over) { sHudOver = over; sHudOverSize = (size_t)w * h; }
    }
    if (sHdCover && (size_t)w * h <= sHdCoverSize)
        memset(sHdCover, 0, (size_t)w * h);
    sEnemyHudRect[2] = sEnemyHudRect[3] = -1;
    sEnemyLabelRect[0] = w; sEnemyLabelRect[1] = h;
    sEnemyLabelRect[2] = sEnemyLabelRect[3] = -1;
    sEnemyLabelColor = 0;
    sEnemyBarRect[0] = w; sEnemyBarRect[1] = h;
    sPlayerBarRect[0] = w; sPlayerBarRect[1] = h;
    sPlayerFullHp = 0;
    sPlayerFrameRect[0] = w; sPlayerFrameRect[1] = h;
    sPlayerFrameRect[2] = sPlayerFrameRect[3] = -1;
    sPlayerBarRect[2] = sPlayerBarRect[3] = -1;
    memset(sPlayerBarRows, 0, sizeof(sPlayerBarRows));
    if (gPortConfig.hdHud && (size_t)w * 32 > sPlayerBarMaskSize) {
        uint8_t* mask = realloc(sPlayerBarMask, (size_t)w * 32);
        if (mask) { sPlayerBarMask = mask; sPlayerBarMaskSize = (size_t)w * 32; }
    }
    if (sPlayerBarMask && (size_t)w * 32 <= sPlayerBarMaskSize)
        memset(sPlayerBarMask, 0, (size_t)w * 32);
    sEnemyBarRect[2] = sEnemyBarRect[3] = -1;
    memset(sEnemyBarRows, 0, sizeof(sEnemyBarRows));
    sHudDigitCount = 0;
    if (gPortConfig.hdHud && (size_t)w * h > sHudDigitOwnerSize) {
        uint8_t* owners = realloc(sHudDigitOwners, (size_t)w * h);
        if (owners) { sHudDigitOwners = owners; sHudDigitOwnerSize = (size_t)w * h; }
    }
    if (sHudDigitOwners && (size_t)w * h <= sHudDigitOwnerSize)
        memset(sHudDigitOwners, 0, (size_t)w * h);
    if (gPortConfig.hdHud && (size_t)w * h > sEnemyBarMaskSize) {
        uint8_t* mask = realloc(sEnemyBarMask, (size_t)w * h);
        if (mask) { sEnemyBarMask = mask; sEnemyBarMaskSize = (size_t)w * h; }
    }
    if (sEnemyBarMask && (size_t)w * h <= sEnemyBarMaskSize)
        memset(sEnemyBarMask, 0, (size_t)w * h);
    memset(buf, 0, (size_t)w * h * 4);
    if (!sHudLayer || (dispcnt & 0x80) || !(dispcnt & 0x1000))
        return;
    if (evy > 16)
        evy = 16;

    for (i = 127; i >= 0; i--) {
        uint16_t a0 = OAM16[i * 4 + 0], a1 = OAM16[i * 4 + 1], a2 = OAM16[i * 4 + 2];
        int affine = (a0 >> 8) & 1, dbl = (a0 >> 9) & 1, mode = (a0 >> 10) & 3;
        int shape = (a0 >> 14) & 3, size = (a1 >> 14) & 3;
        int ow, oh, bw, bh, ox, oy, xg, yg, hx, hy, sx, sy;
        int16_t pa = 0x100, pb = 0, pc = 0, pd = 0x100;
        const SpriteTag* tag = &sOamTag[i];
        PortHudDigit* digit = NULL;
        uint32_t* rawHud = NULL;
        int hdSprite = 0, hdPart = 0;

        if (!tag->hud || (!affine && dbl) || shape == 3 || mode >= 2 ||
            (HdTextGlyph(i) && tag->style != 2))
            continue;
        ow = sObjSize[shape][size][0];
        oh = sObjSize[shape][size][1];
        if (gPortConfig.hdHud && sHudDigitOwners && (size_t)w * h <= sHudDigitOwnerSize &&
            tag->digit && tag->digit <= 10 && sHudDigitCount < 128) {
            if (!sHudDigits) sHudDigits = calloc(128, sizeof(*sHudDigits));
            if (sHudDigits) {
                digit = &sHudDigits[sHudDigitCount];
                digit->x = w; digit->y = h; digit->w = digit->h = -1;
                digit->code = '0' + tag->digit - 1;
                digit->slot = i;
                digit->color = 0;
            }
        }
        {
            uint32_t* reference = PortHdHudExportBuffer();
            rawHud = reference;
            if (reference) {
                int xx, yy;
                for (yy = 0; yy < oh; yy++) for (xx = 0; xx < ow; xx++) {
                    int c = ObjColor(a0, a2, xx, yy, ow, mapping1d);
                    reference[yy * ow + xx] = c < 0 ? 0 : sColorLut[c];
                }
                PortHdHudExportSprite(ow, oh, tag->hdHudPart, tag->ax, tag->ay == 3 ? (tag->ayq > 0 ? 1 : -1) : tag->ay);
            }
        }
        bw = ow << (affine && dbl);
        bh = oh << (affine && dbl);
        oy = a0 & 0xFF;
        ox = a1 & 0x1FF;
        if ((sOamExt[i].x & 0x1FF) == ox && (sOamExt[i].y & 0xFF) == oy) {
            ox = sOamExt[i].x;
            oy = sOamExt[i].y;
        }
        /* back to GBA-screen coordinates, then onto the HUD canvas */
        xg = ox - (tag->ax == 2 ? 0 : tag->ax) * gPortScreen.originX;
        yg = oy - TagShiftY(tag);
        hx = tag->ax == 2 ? HudWorldX(xg, bw, w)
             : tag->ax < 0 ? xg : (tag->ax > 0 ? w - (GBA_SCREEN_W - xg) : xg + (w - GBA_SCREEN_W) / 2);
        hy = TagHudY(tag, yg, h, HudRaise());
        if (!affine && !tag->hdHudPart && !tag->digit && sHudDigitOwners &&
            (size_t)w*h <= sHudDigitOwnerSize)
            hdSprite = PortHdHudSpriteRegister(rawHud, ow, oh, i, hx, hy,
                ((a1 & 0x1000) ? 1 : 0) | ((a1 & 0x2000) ? 2 : 0), mode == 1 ? 0xA0 : 255);
        {
            if (sEnvHudLayoutFrame >= 0 && (uint32_t)sEnvHudLayoutFrame == PortFrameCount())
                PortLog("hud-layout: frame=%u slot=%d x=%d y=%d w=%d h=%d tile=%d palette=%d part=%d affine=%d",
                    PortFrameCount(), i, hx, hy, bw, bh, a2 & 1023, a2 >> 12, tag->hdHudPart, affine);
        }
        if (affine) {
            const uint16_t* p = OAM16 + ((a1 >> 9) & 31) * 16;
            pa = (int16_t)p[3];
            pb = (int16_t)p[7];
            pc = (int16_t)p[11];
            pd = (int16_t)p[15];
        }
        if (sOamHd[i].id && !hdSprite && sHdCover && (size_t)w * h <= sHdCoverSize &&
            sHudOver && (size_t)w * h <= sHudOverSize && mode != 2)
            hdPart = QueueHdPart(1, i, hx, hy, ow, oh, bw, bh, affine, pa, pb, pc, pd, mode == 1 ? 0xA0 : 255);
        for (sy = 0; sy < bh; sy++) {
            int py = hy + sy;
            if (py < 0 || py >= h)
                continue;
            for (sx = 0; sx < bw; sx++) {
                int px = hx + sx, tx, ty, col, digitInk = 0, barInk = 0;
                uint32_t argb;
                if (px < 0 || px >= w)
                    continue;
                if (affine) {
                    int cx = sx - bw / 2, cy = sy - bh / 2;
                    tx = ((pa * cx + pb * cy) >> 8) + ow / 2;
                    ty = ((pc * cx + pd * cy) >> 8) + oh / 2;
                    if (tx < 0 || ty < 0 || tx >= ow || ty >= oh)
                        continue;
                } else {
                    tx = (a1 & 0x1000) ? ow - 1 - sx : sx;
                    ty = (a1 & 0x2000) ? oh - 1 - sy : sy;
                }
                col = ObjColor(a0, a2, tx, ty, ow, mapping1d);
                if (col < 0)
                    continue;
                digitInk = digit && ((col & 31) >= 22 || ((col >> 5) & 31) >= 22 || ((col >> 10) & 31) >= 22);
                if (gPortConfig.hdHud && (tag->hdHudPart == 4 || tag->hdHudPart == 6) && py < 32 && sPlayerBarMask &&
                    (size_t)w * 32 <= sPlayerBarMaskSize && sHudDigitOwners && (size_t)w * h <= sHudDigitOwnerSize) {
                    int r = col & 31, g = (col >> 5) & 31, b = (col >> 10) & 31;
                    int high = r > g ? r : g, low = r < g ? r : g;
                    if (b > high) high = b;
                    if (b < low) low = b;
                    barInk = high - low > 3;
                }
                if (gPortConfig.hdHud && tag->hdHudPart == 3 && sEnemyBarMask && sHudDigitOwners && py < 32 &&
                    (size_t)w * h <= sEnemyBarMaskSize && (size_t)w * h <= sHudDigitOwnerSize) {
                    int r = col & 31, g = (col >> 5) & 31, b = (col >> 10) & 31;
                    int high = r > g ? r : g, low = r < g ? r : g;
                    if (b > high) high = b;
                    if (b < low) low = b;
                    barInk = high - low > 3;
                }
                if (HdTextGlyph(i) && tag->style == 2 &&
                    (col == (PLTT16[256 + (a2 >> 12) * 16 + 14] & 0x7FFF) ||
                     col == (PLTT16[256 + (a2 >> 12) * 16 + 15] & 0x7FFF)))
                    col = PLTT16[256 + (a2 >> 12) * 16 + 13] & 0x7FFF;
                {
                    /* hidden where the cartridge draws something in front */
                    int cx = ox + sx + gPortScreen.originX, cy = oy + sy + gPortScreen.originY;
                    int prio = (a2 >> 10) & 3;
                    if (cx >= 0 && cy >= 0 && cx < gPortScreen.width && cy < gPortScreen.height) {
                        int k = cy * gPortScreen.width + cx;
                        if (sOccBg[k] < prio || sOccObj[k] < prio * 128 + i)
                            continue;
                    }
                }
                if (gPortConfig.hdHud && sHudDigitOwners && (size_t)w * h <= sHudDigitOwnerSize)
                    sHudDigitOwners[py * w + px] = i + 1;
                if (hdSprite) continue;
                if (hdPart) {
                    sHdCover[py * w + px] = 1;
                    continue;
                }
                if (tag->hdHudPart == 5 && px >= 31 && PortHdHudPlayerFrameReady()) {
                    int r = col & 31, g = (col >> 5) & 31, b = (col >> 10) & 31;
                    int high = r > g ? r : g, low = r < g ? r : g;
                    if (b > high) high = b;
                    if (b < low) low = b;
                    if (high - low <= 3 && high > 0) {
                        if (px < sPlayerFrameRect[0]) sPlayerFrameRect[0] = px;
                        if (py < sPlayerFrameRect[1]) sPlayerFrameRect[1] = py;
                        if (px > sPlayerFrameRect[2]) sPlayerFrameRect[2] = px;
                        if (py > sPlayerFrameRect[3]) sPlayerFrameRect[3] = py;
                        continue;
                    }
                }
                if (gPortConfig.hdHud && (tag->hdHudPart == 1 || tag->hdHudPart == 2)) {
                    int r = col & 31, g = (col >> 5) & 31, b = (col >> 10) & 31;
                    int high = r > g ? r : g, low = r < g ? r : g;
                    if (b > high) high = b;
                    if (b < low) low = b;
                    if (px < sEnemyHudRect[0]) sEnemyHudRect[0] = px;
                    if (py < sEnemyHudRect[1]) sEnemyHudRect[1] = py;
                    if (px > sEnemyHudRect[2]) sEnemyHudRect[2] = px;
                    if (py > sEnemyHudRect[3]) sEnemyHudRect[3] = py;
                    if (tag->hdHudPart == 1 && px >= w - 20 && high - low > 3 && PortHdHudLettersReady()) {
                        uint32_t face = sColorLut[col];
                        if (px < sEnemyLabelRect[0]) sEnemyLabelRect[0] = px;
                        if (py < sEnemyLabelRect[1]) sEnemyLabelRect[1] = py;
                        if (px > sEnemyLabelRect[2]) sEnemyLabelRect[2] = px;
                        if (py > sEnemyLabelRect[3]) sEnemyLabelRect[3] = py;
                        if ((face & 0xFFFFFF) > (sEnemyLabelColor & 0xFFFFFF)) sEnemyLabelColor = face;
                        continue;
                    }
                    /* Replace only the neutral frame. Coloured lettering and
                       lower HP layers retain the cartridge's exact pixels. */
                    /* Native black pixels also clip the live bar beneath
                       the lettering; keep that occlusion exactly intact. */
                    if (high - low <= 3 && high != 0)
                        continue;
                }
                if (bldcnt & 0x10) {
                    if (effect == 2)
                        col = Brighten((uint16_t)col, evy);
                    else if (effect == 3)
                        col = Darken((uint16_t)col, evy);
                }
                argb = sColorLut[col];
                if (barInk) {
                    if (tag->hdHudPart == 4 || tag->hdHudPart == 6) {
                        sPlayerFullHp = tag->hdHudPart == 6;
                        sPlayerBarMask[py * w + px] = 1;
                        sPlayerBarSlot = i;
                        if (px < sPlayerBarRect[0]) sPlayerBarRect[0] = px;
                        if (py < sPlayerBarRect[1]) sPlayerBarRect[1] = py;
                        if (px > sPlayerBarRect[2]) sPlayerBarRect[2] = px;
                        if (py > sPlayerBarRect[3]) sPlayerBarRect[3] = py;
                        sPlayerBarRows[py] = argb;
                        continue;
                    }
                    sEnemyBarMask[py * w + px] = 1;
                    sEnemyBarSlot = i;
                    if (px < sEnemyBarRect[0]) sEnemyBarRect[0] = px;
                    if (py < sEnemyBarRect[1]) sEnemyBarRect[1] = py;
                    if (px > sEnemyBarRect[2]) sEnemyBarRect[2] = px;
                    if (py > sEnemyBarRect[3]) sEnemyBarRect[3] = py;
                    sEnemyBarRows[py] = argb;
                    continue;
                }
                if (gPortConfig.hdHud && sHudDigitOwners && (size_t)w * h <= sHudDigitOwnerSize)
                    sHudDigitOwners[py * w + px] = i + 1;
                if (digitInk) {
                    if (px < digit->x) digit->x = px;
                    if (py < digit->y) digit->y = py;
                    if (px > digit->w) digit->w = px;
                    if (py > digit->h) digit->h = py;
                    if ((argb & 0xFFFFFF) > (digit->color & 0xFFFFFF)) digit->color = argb;
                    continue;
                }
                if (mode == 1)
                    argb = (argb & 0x00FFFFFFu) | 0xA0000000u;
                if (sHdCover && (size_t)w * h <= sHdCoverSize && sHdCover[py * w + px]) {
                    /* in front of an HD part: drawn over it */
                    if (!sHudOverUsed) {
                        memset(sHudOver, 0, (size_t)w * h * 4);
                        sHudOverUsed = 1;
                    }
                    sHudOver[py * w + px] = argb;
                    continue;
                }
                buf[py * w + px] = argb;
            }
        }
        if (digit && digit->w >= digit->x) {
            digit->w -= digit->x - 1; digit->h -= digit->y - 1;
            sHudDigitCount++;
        }
    }
}

/* HD text glyphs of one layer (0 world canvas, 1 HUD canvas, 2 screen layer), front last. */
int PortPpuTextGlyphs(PortTextGlyph* out, int max, int hud) {
    uint16_t dispcnt = IO16(0x00);
    uint16_t bldcnt = sTextLatched ? sTextBld[0] : IO16(0x50);
    int effect = (bldcnt >> 6) & 3, evy = (sTextLatched ? sTextBld[2] : IO16(0x54)) & 31;
    int n = 0, i;

    if (!sHdText || (dispcnt & 0x80))
        return 0;
    if (evy > 16)
        evy = 16;
    /* BG numbers enter before OBJ text, in the same back-to-front order as
       their authored backgrounds. Their panel pixels still obey windows. */
    {
        int priority, bg, j;
        for (priority = 3; priority >= 0; priority--) for (bg = 3; bg >= 0; bg--)
            for (j = 0; j < sBgTextCount && n < max; j++) {
                const BgTextGlyph* source = &sBgTextGlyphs[j];
                const PortTextGlyph* glyph = &source->glyph;
                int layer = source->screen ? 2 : 0;
                int width = source->screen ? gPortOverlayW : gPortScreen.width;
                int height = source->screen ? GBA_SCREEN_H : gPortScreen.height;
                int px = glyph->x + 4, py = glyph->y + 4, at;
                const uint8_t* occBg = source->screen ? sScreenOccBg : sOccBg;
                const uint8_t* occLayer = source->screen ? sScreenOccBgLayer : sOccBgLayer;
                const uint16_t* occObj = source->screen ? sScreenOccObj : sOccObj;
                uint16_t face, shadow;
                if (source->priority != priority || source->bg != bg || layer != hud ||
                    px < 0 || px >= width || py < 0 || py >= height) continue;
                at = py * width + px;
                if ((occBg[at] == 0 && occObj[at] == 0) || occBg[at] < priority ||
                    (occBg[at] == priority && occLayer[at] < bg) ||
                    occObj[at] < (priority + 1) * 128) continue; /* OBJ wins ties with BG. */
                out[n] = *glyph;
                /* a window over part of the cell (a menu opened beside the
                   number): only the columns left uncovered are drawn */
                {
                    int c0 = 4, c1 = 4, row = py * width;
                    #define BG_TEXT_SHOWN(cx) (glyph->x + (cx) >= 0 && glyph->x + (cx) < width &&                         !((occBg[row + glyph->x + (cx)] == 0 && occObj[row + glyph->x + (cx)] == 0) ||                           occBg[row + glyph->x + (cx)] < priority ||                           (occBg[row + glyph->x + (cx)] == priority && occLayer[row + glyph->x + (cx)] < bg) ||                           occObj[row + glyph->x + (cx)] < (priority + 1) * 128))
                    while (c0 > 0 && BG_TEXT_SHOWN(c0 - 1)) c0--;
                    while (c1 < 8 && BG_TEXT_SHOWN(c1)) c1++;
                    #undef BG_TEXT_SHOWN
                    out[n].clip0 = (uint8_t)c0;
                    out[n].clip1 = (uint8_t)(c0 == 0 && c1 == 8 ? 0 : c1);
                }
                face = (uint16_t)glyph->face; shadow = (uint16_t)glyph->shadow;
                if (bldcnt & (1 << bg)) {
                    if (effect == 2) { face = Brighten(face, evy); shadow = Brighten(shadow, evy); }
                    if (effect == 3) { face = Darken(face, evy); shadow = Darken(shadow, evy); }
                }
                out[n].face = sColorLut[face]; out[n].shadow = sColorLut[shadow];
                n++;
            }
    }
    for (i = 127; i >= 0 && n < max && (dispcnt & 0x1000); i--) {
        uint16_t a0 = OAM16[i * 4 + 0], a1 = OAM16[i * 4 + 1], a2 = OAM16[i * 4 + 2];
        const SpriteTag* tag = &sOamTag[i];
        int onHud = sHudLayer && tag->hud;
        int onScreen = sOverlay && tag->screen;
        int layer = onScreen ? 2 : onHud;
        int ox, oy, bank, k;
        uint16_t col[2];

        if (!HdTextGlyph(i) || layer != hud || (a0 & 0x300) == 0x200 || ((a0 >> 10) & 3) >= 2)
            continue;
        oy = a0 & 0xFF;
        ox = a1 & 0x1FF;
        if ((sOamExt[i].x & 0x1FF) == ox && (sOamExt[i].y & 0xFF) == oy) {
            ox = sOamExt[i].x;
            oy = sOamExt[i].y;
        } else {
            if (oy >= sBottom)
                oy -= 256;
            if (ox >= sRight)
                ox -= 512;
        }
        if (onScreen) {
            int inkX, inkY;
            GlyphInkPoint(i, &inkX, &inkY);
            int px = ox + (gPortOverlayW - GBA_SCREEN_W) / 2 + inkX;
            int py = oy + inkY, prio = (a2 >> 10) & 3;
            if (px >= 0 && px < gPortOverlayW && py >= 0 && py < GBA_SCREEN_H) {
                int at = py * gPortOverlayW + px;
                if ((sScreenOccBg[at] == 0 && sScreenOccObj[at] == 0) ||
                    sScreenOccBg[at] < prio || ObjCoversGlyph(sScreenOccObj[at], prio, i))
                    continue;
            }
        }
        if (!onScreen) {
            /* HD glyphs bypass DrawObjs, but must still obey the cartridge's
               background/window and object priority. A covered glyph must
               not be painted over a tutorial or confirmation panel. HUD
               glyphs use their original OAM position for coverage, just as
               PortPpuRenderHud does. Their own docked message window is the
               explicit exception below, keeping field dialogue visible. */
            int inkX, inkY;
            GlyphInkPoint(i, &inkX, &inkY);
            int px = ox + gPortScreen.originX + inkX;
            int py = oy + gPortScreen.originY + inkY;
            int prio = (a2 >> 10) & 3;
            if (px >= 0 && px < gPortScreen.width && py >= 0 && py < gPortScreen.height) {
                int at = py * gPortScreen.width + px;
                /* a message window marks its area as covered (it stays in
                   front of the HUD), but the text on it is its own: the
                   field dialogue must stay visible */
                int message = sHudLayer && sMsgMask[at];
                if (!message && ((sOccBg[at] == 0 && sOccObj[at] == 0) || sOccBg[at] < prio ||
                    ObjCoversGlyph(sOccObj[at], prio, i)))
                    continue;
            }
        }
        if (onScreen) {
            /* the screen layer: GBA lines 0..159, centred in its width */
            out[n].x = ox + (gPortOverlayW - GBA_SCREEN_W) / 2;
            out[n].y = oy;
        } else if (onHud) {
            int w = gPortHudW, h = gPortHudH;
            int xg = ox - (tag->ax == 2 ? 0 : tag->ax) * gPortScreen.originX, yg = oy - TagShiftY(tag);
            out[n].x = tag->ax == 2 ? HudWorldX(xg, 8, w)
                       : tag->ax < 0 ? xg : (tag->ax > 0 ? w - (GBA_SCREEN_W - xg) : xg + (w - GBA_SCREEN_W) / 2);
            out[n].y = TagHudY(tag, yg, h, HudRaise());
        } else {
            if (ox + 16 <= sLeft || ox >= sRight || oy + 16 <= sTop || oy >= sBottom)
                continue;
            out[n].x = ox + gPortScreen.originX;
            out[n].y = oy + gPortScreen.originY;
        }
        /* colour 1 is the face, colour 2 the shadow, as the game fades them */
        bank = a2 >> 12;
        for (k = 0; k < 2; k++) {
            int index = bank * 16 + (tag->style == 2 ? 15 - k : 1 + k);
            col[k] = (sTextLatched ? sTextObjPalette[index] : PLTT16[256 + index]) & 0x7FFF;
            if (bldcnt & 0x10) {
                if (effect == 2)
                    col[k] = Brighten(col[k], evy);
                else if (effect == 3)
                    col[k] = Darken(col[k], evy);
            }
        }
        out[n].code = tag->text;
        out[n].clip0 = out[n].clip1 = 0;
        out[n].style = tag->style;
        out[n].semi = 0;
        if (((a0 >> 10) & 3) == 1) {
            int eva = (sTextLatched ? sTextBld[1] : IO16(0x52)) & 31;
            out[n].semi = (uint8_t)(eva >= 1 && eva <= 16 ? eva * 255 / 16 : 0xA0);
        }
        out[n].face = sColorLut[col[0]];
        out[n].shadow = sColorLut[col[1]];
        n++;
    }
    return n;
}

/* HD-2D: 1 where a sprite is what shows, 2 on message windows (from the HUD
   occupancy buffers, filled while PortHd2dActive()); a window hiding OBJs
   reads as 0 / 0. */
void PortPpuSpriteMask(uint8_t* out) {
    int i, n = gPortScreen.width * gPortScreen.height;

    for (i = 0; i < n; i++) {
        uint16_t obj = sOccObj[i];
        uint8_t bg = sOccBg[i];
        out[i] = sMsgMask[i] ? 2 : (obj != 0xFFFFu && !(obj == 0 && bg == 0) && (obj >> 7) <= bg);
    }
}
