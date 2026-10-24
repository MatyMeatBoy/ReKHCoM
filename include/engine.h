#ifndef GUARD_ENGINE_H
#define GUARD_ENGINE_H

#include "types.h"
#include "obj.h"
#include "macros.h"

struct SpriteWork;

typedef struct ObjTileListEntry {
    u16 attr0;
    u16 attr1;
    u16 tile;
} ObjTileListEntry;

typedef struct Dma3Request {
    const void* src;
    void* dst;
    u16 size;
} Dma3Request;

#define BG_ENTRY_COUNT 4
typedef struct BgEntry {
    u8 dirty;
    void* const* map;
    u8 width;
    u8 height;
    u16 x;
    u16 y;
#ifdef VERSION_EU
    void** decompressedMap;
#endif
} BgEntry;

#ifdef PLATFORM_PC
int PortRikuAvailable(int soraCleared);
void PortLatchBgMaps(const BgEntry* entries);
void PortOamSetPosition(int index, int x, int y);
/* Wide view: how far the picture extends left/right of the GBA screen. */
int PortWideMarginX(void);
/* Horizontal display offset for a background / for sprites being drawn,
   used to dock screen-edge UI (message windows) to the wide view's edges. */
void PortSetBgShiftX(int bg, int shift);
void PortDockBg(int bg, int shift);
extern int gPortSpriteShiftX;
extern int gPortSpriteShiftY;
/* Clip subsequently queued sprites to logical GBA X coordinates. x0==x1 ends
 * the scope. Docked dialogue portraits use their window's shifted viewport. */
void PortSpriteClipX(int x0, int x1);
int PortWideMarginY(void);
/* Anchor what is drawn until PortHudEnd() to a side of the wide view:
   ax/ay are -1 (left/top), 0 (stay) or 1 (right/bottom). */
void PortHudBegin(int ax, int ay);
/* Same, choosing the side from where (x, y) lies on the GBA screen: for
   pointers and highlights that refer to an anchored HUD element. */
void PortHudBeginAt(int x, int y);
/* Deck cards that rise to the stock: vertical anchor follows their height. */
void PortHudBeginFollowY(int ax);
int PortSpriteShiftY(int entry, int y);
/* Bookkeeping for the HUD layer: the sprite entry just queued, and which
   entry an OAM slot was built from. */
void PortSpriteTag(int entry);
void PortOamSetSource(int oamIndex, int entry);
/* The title's logo is on BG2 (title, end of the credits): with the HD title it is drawn HD (port_ppu.c). */
void PortTitleLogoBg(int on);
/* HD sprites: the ROM tiles and frame an OAM slot shows, and the part's box in
   the frame's own coordinates (before the entry's flips and affine). */
void PortOamSetHdPart(int oamIndex, const void* tiles, const void* frame, int x, int y, int w, int h, int flags,
                      const void* palette, int bank);
/* Start of a new OAM list: forget which entry each slot came from. */
void PortOamTagsReset(void);
/* HD text: the pixel-font character of the sprites being drawn (0: none). */
extern int gPortTextChar;
/* Set while a menu header is drawn: its pixels left of the GBA screen show
   the band of the asset instead of what the cartridge kept off screen. */
extern int gPortHeaderDraw;
/* 0 menu/dialogue, 1 movie subtitles, 2 compact numeric labels. */
extern int gPortTextStyle;
/* A 4bpp numeric BG cell; source fingerprint prevents stale tile metadata.
   Explicit face/shadow/panel indices in style bits 8..11/12..15/16..19;
   without explicit face bits, infer the existing panel's indices. */
void PortRegisterBgTextTile(void* address, int code, int style, const void* source32);
void PortClearBgTextTiles(void);
void PortHudEnd(void);
/* Experimental Re:Chain of Memories options: percentage applied to damage
   taken by the player (player != 0) or by everyone else; whether event
   dialogue types without its per-character delay. */
int PortScaleDamage(int damage, int player);
/* A new game: the options overlay asks how to start it (version, difficulty,
   HD text, QoL buttons) before the game goes on. */
void PortUiOpenNewGame(void);
int PortFastText(void);
/* One-button dodge roll: the d-pad direction to roll toward, or 0. */
int PortTakeDodge(int held, int facingLeft);
/* Return-stocked-cards button: 1 once per completed hold. */
int PortTakeUnstock(void);
/* Opening movie: show or hide the "A again skips" prompt. */
void PortUiSkipPrompt(int visible);
/* This frame shows a GBA-size screen (menus, the battle start): scale the
   160 lines to the window's height. Called every frame it applies. */
void PortFitScreen(void);
/* Menus drawn over the world: their sprites and screen layers go on their
   own layer, scaled to the window's height, while the world keeps its scale.
   PortScreenMarginX() is that layer's margin beyond the GBA screen. */
void PortScreenBegin(void);
void PortScreenEnd(void);
/* Explicit UI ownership for menus using the map-block API. */
void PortScreenBg(int bg, int on);
int PortScreenMarginX(void);
void PortBgSplit(int bg, int column);
void PortBgSplitBands(int bg, int enabled);
/* A UI layer drawn on the world: kept to the GBA columns, so what it hides
   past the right edge does not wrap into the margins. Cleared with the battle view. */
void PortBgClip(int bg, int on);
/* Battles: world-map layers repeat sideways past their edge. */
void PortSetBattleView(int on);
/* Marluxia's last fight (bos_lst*.c), presentation only: its field is a screen background that wraps like the
   hardware's, and the body's map as built before the game clears its rows off the screen (the wide view shows one
   body, whole). Cleared with the battle view. */
void PortMarluxiaField(int on);
void PortMarluxiaBodyMap(const void* map);
/* Authored arena bounds and repeatable side bands; screen margins only. */
void PortSetBattleArenaBands(int left, int right, int leftStart, int leftWidth,
                             int rightStart, int rightWidth);
/* GBA-size screens: how the side bars beyond them are filled. */
enum { PORT_SIDE_STRETCH, PORT_SIDE_COLOR, PORT_SIDE_TEXTURE, PORT_SIDE_BLUR, PORT_SIDE_MENU, PORT_SIDE_PATTERN, PORT_SIDE_PIXEL_FADE, PORT_SIDE_EDGE_FADE,
       PORT_SIDE_BLUR_BANDS, PORT_SIDE_GROUND_BANDS, PORT_SIDE_MOVIE,
       PORT_SIDE_SCENE_BANDS, /* the PPU's own margin (scenery carried on) with the frame's bars drawn on */
       PORT_SIDE_GROUND_SHEETS /* the journal's open book: GROUND_BANDS with each side's own ground (23/10) */ };
void PortSideStyle(int style);
/* Only the illustrated title BG; never text, logo, sprites or other modes. */
void PortTitleArtFade(int enabled);
void PortTitleArtFadeFirst(int enabled);
void PortTitleBands(int enabled);
void PortMenuPanelSide(int side);
/* Status has authored BG panels; foreground badges must never fill margins. */
int PortMenuBackgroundOnly(void);
void PortMenuHeaderSource(void* frame, void* tiles, void* palette);
void PortMenuHeaderAt(int x);
void PortMenuBandSource(int band, int y, void* frame, void* tiles, void* palette);
void PortSetSpotlight(int active);
void PortMenuTabBg(int side, int bg, int x, int y);
void PortMenuTabSprite(void* frame, void* tiles, void* palette);
/* Full-screen menus: shown as the GBA picture (full height, side bars). */
void PortSceneNarrow(int on);
void PortNarrowObjBands(int on);
#define PORT_MENU_LEFT(x) ((x) - PortScreenMarginX())
#define PORT_MENU_RIGHT(x) ((x) + PortScreenMarginX())
/* A band across the GBA screen (menu title bars): drawn twice, moved into
   both margins of the wide view, so it reaches its edges. */
#define PORT_BAND(draw)                \
    do {                               \
        int m_ = PortScreenMarginX();  \
        gPortSpriteShiftX -= m_;       \
        draw;                          \
        gPortSpriteShiftX += m_;       \
        if (m_ > 0) {                  \
            draw;                      \
            gPortSpriteShiftX += m_;   \
            draw;                      \
            gPortSpriteShiftX -= m_;   \
        }                              \
    } while (0)
/* Wide view for tilemaps the game streams into VRAM from its own world data:
   fn returns the screen entry of world tile (tx, ty) of a background whose
   scroll registers show world pixel (originX, originY) at the screen's
   top-left. NULL clears it. */
typedef int (*PortWorldTileFunc)(int bg, int tx, int ty);
void PortSetBgWorld(int bg, PortWorldTileFunc fn, int originX, int originY);
/* Event scenes: continue the map past its edges along the isometric diagonal. */
void PortSetBgIsoExtend(int bg, int enabled);
#endif

typedef struct BgWork {
    BgEntry entries[BG_ENTRY_COUNT];
} BgWork;

typedef struct Dma3Pending {
    void* dst;
    u16 size;
} Dma3Pending;

typedef struct Dma3Blit {
    void* src;
    void* dst;
    u8 srcX;
    u8 srcY;
    u8 dstX;
    u8 dstY;
    u8 width;
    u8 height;
} Dma3Blit;

typedef struct Dma3Fill {
    void* src;
    void* dst;
    u8 x;
    u8 y;
    u8 vertical;
} Dma3Fill;

typedef struct Dma3Queue {
    Dma3Request requests[256];
    Dma3Blit blits[64];
    Dma3Fill fills[8];
    void (*callbacks[8])();
    Dma3Pending pending[4];
#ifdef VERSION_EU
    Dma3Request lz77Requests[32];
#endif
    vu16 requestCount;
    vu16 blitCount;
    vu16 fillCount;
    vu16 callbackCount;
    vu16 count;
    vu16 lz77RequestCount;
    u32 transferredBytes;
} Dma3Queue;

typedef struct SpriteWork SpriteWork;
extern SpriteWork* gSpriteWork;
extern Dma3Queue* gDma3Requests;

enum FadeFlag {
    FADE_FLAG_ACTIVE = 0x1,
    FADE_FLAG_LOCKED = 0x2,
    FADE_FLAG_PAUSED = 0x4
};

typedef struct FadeWork {
    PaletteSlot slots[32];
    u32 amount;
    u32 target;
    u32 lastAmount;
    u16 timer;
    u32 mode;
    u16 flags;
} FadeWork;

STATIC_ASSERT(sizeof(FadeWork) == 0x598, FadeWorkSize);
STATIC_ASSERT(sizeof(PaletteSlot) == 0x2C, PaletteSlotSize);

typedef struct Spline2D {
    s16 pointCount;
    s32* intervals;
    s32* scratch;
    s32* knots;
    s32* xCoefficients;
    s32* yCoefficients;
    s32* xValues;
    s32* yValues;
} Spline2D;

STATIC_ASSERT(sizeof(Spline2D) == 0x20, Spline2DSize);

void SplineBuildAxisCoefficients(Spline2D* spline, s32* knots, s32* values, s32* coefficients);
s32 SplineEvaluateAxis(s16* pointCount, s32 position, s32* knots, s32* values, s32* coefficients);
void SplineInit2D(Spline2D* spline, s32* xValues, s32* yValues, s16 pointCount);
void SplineEvaluate2D(Spline2D* spline, s32 position, s32* outX, s32* outY);
void SplineFreeBuffers(Spline2D* spline);

u8 DrawSpriteAllocatedTiles(s16 x, s16 y, void* sprite, void* obj, void* palette, ObjAffine* affine, u16 flags, u16 priority);
u8 DrawSpriteFrameTiles(s16 x, s16 y, void* obj, void* palette, ObjAffine* affine, u16 flags, u16 priority);
void ReleaseSharedObjTiles(ObjTiles* tiles);
void ReleaseAllocatedObjTiles(ObjTiles* tiles);
void ReleaseSpriteFrameTiles(ObjTiles* tiles);
void ReleaseObjPaletteRef(ObjPalette* palette);
void SetSpriteMosaicEnabled(u8 enabled);
void VTransFree();
void BgFree();
void SetBgMosaic(s32 bg, u8 on);
void FadeFree();
u8 GetBgScrollX(u32 bg);
u8 GetBgScrollY(u32 bg);
void InitDynamicObjTilesAtSlot(ObjTiles* tiles, u16 slot, u16 size, void* src);
void* GetBgMapBlock(BgEntry* entry, u16 x, u16 y);
void SetSpriteOamUpdatesPaused(u8 paused);
void SetBgMosaicSize(u8 x, u8 y);
s32 Lerp8(s32 from, s32 to, s32 weight);

void VTransInit();
void BgInit();
void FadeInit();
void InitDisplayRegs();

#endif /* GUARD_ENGINE_H */
