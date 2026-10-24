/**
 * engine.c
 * Graphics Engine
 */

#include "macros.h"
#include "engine_math.h"
#include "fade.h"
#include "pallet.h"
#include "display.h"
#include <stdlib.h>
#include "obj_api.h"
#include "listpool.h"
#include "anim.h"
#include "gba/syscall.h"
#include "malloc.h"
#include "engine.h"
#include "sprite.h"
#include "types.h"
#include "system_state.h"
#include "gba/defines.h"
#include "gba/io_reg.h"
#include "gba/macro.h"
#include "gba/oam.h"
#include "obj.h"
#include <stddef.h>

u16 gLastBackdropColor IWRAM_DATA(4);
u8 gBgPaletteBank[4] IWRAM_DATA(4);
struct BgWork* gBgWork IWRAM_DATA(4);
u16 gBackdropColor IWRAM_DATA(4);
u16 gBg2Cnt IWRAM_DATA(4);
u16 gBg3PB IWRAM_DATA(4);
u16 gBg3VOfs IWRAM_DATA(4);
u16 gBg3PA IWRAM_DATA(4);
u16 gWin1V IWRAM_DATA(4);
u16 gWin1H IWRAM_DATA(4);
u32 gBg2Y IWRAM_DATA(4);
u16 gBg2PB IWRAM_DATA(4);
u16 gBg0VOfs IWRAM_DATA(4);
vu16 gDispCnt IWRAM_DATA(4);
u16 gBg3PC IWRAM_DATA(4);
u16 gBg1Cnt IWRAM_DATA(4);
u16 gBg0Cnt IWRAM_DATA(4);
u16 gBg2PA IWRAM_DATA(4);
u16 gBg2PC IWRAM_DATA(4);
u16 gWin0V IWRAM_DATA(4);
u16 gBldY IWRAM_DATA(4);
u16 gBg2HOfs IWRAM_DATA(4);
u32 gBg3X IWRAM_DATA(4);
vu16 gMosaic IWRAM_DATA(4);
u32 gBg2X IWRAM_DATA(4);
u16 gWin0H IWRAM_DATA(4);
u16 gBg2VOfs IWRAM_DATA(4);
u16 gBg1HOfs IWRAM_DATA(4);
u16 gBg3Cnt IWRAM_DATA(4);
u16 gBg3PD IWRAM_DATA(4);
u16 gBg0HOfs IWRAM_DATA(4);
u16 gWinOut IWRAM_DATA(4);
u16 gBg2PD IWRAM_DATA(4);
u16 gWinIn IWRAM_DATA(4);
u16 gBldCnt IWRAM_DATA(4);
u16 gBg1VOfs IWRAM_DATA(4);
u16 gBg3HOfs IWRAM_DATA(4);
u32 gBg3Y IWRAM_DATA(4);
vu16 gBldAlpha IWRAM_DATA(4);
struct FadeWork* gFadeWork IWRAM_DATA(4);

static vu16* sBgControl[4] = { &gBg0Cnt, &gBg1Cnt, &gBg2Cnt, &gBg3Cnt };

const s16 gSineTable[320] = {
        0,     6,    12,    18,    25,    31,    37,    43,    49,    56,    62,    68,
       74,    80,    86,    92,    97,   103,   109,   115,   120,   126,   131,   136,
      142,   147,   152,   157,   162,   167,   171,   176,   181,   185,   189,   193,
      197,   201,   205,   209,   212,   216,   219,   222,   225,   228,   231,   234,
      236,   238,   241,   243,   244,   246,   248,   249,   251,   252,   253,   254,
      254,   255,   255,   255,   256,   255,   255,   255,   254,   254,   253,   252,
      251,   249,   248,   246,   244,   243,   241,   238,   236,   234,   231,   228,
      225,   222,   219,   216,   212,   209,   205,   201,   197,   193,   189,   185,
      181,   176,   171,   167,   162,   157,   152,   147,   142,   136,   131,   126,
      120,   115,   109,   103,    97,    92,    86,    80,    74,    68,    62,    56,
       49,    43,    37,    31,    25,    18,    12,     6,     0,    -6,   -12,   -18,
      -25,   -31,   -37,   -43,   -49,   -56,   -62,   -68,   -74,   -80,   -86,   -92,
      -97,  -103,  -109,  -115,  -120,  -126,  -131,  -136,  -142,  -147,  -152,  -157,
     -162,  -167,  -171,  -176,  -181,  -185,  -189,  -193,  -197,  -201,  -205,  -209,
     -212,  -216,  -219,  -222,  -225,  -228,  -231,  -234,  -236,  -238,  -241,  -243,
     -244,  -246,  -248,  -249,  -251,  -252,  -253,  -254,  -254,  -255,  -255,  -255,
     -256,  -255,  -255,  -255,  -254,  -254,  -253,  -252,  -251,  -249,  -248,  -246,
     -244,  -243,  -241,  -238,  -236,  -234,  -231,  -228,  -225,  -222,  -219,  -216,
     -212,  -209,  -205,  -201,  -197,  -193,  -189,  -185,  -181,  -176,  -171,  -167,
     -162,  -157,  -152,  -147,  -142,  -136,  -131,  -126,  -120,  -115,  -109,  -103,
      -97,   -92,   -86,   -80,   -74,   -68,   -62,   -56,   -49,   -43,   -37,   -31,
      -25,   -18,   -12,    -6,     0,     6,    12,    18,    25,    31,    37,    43,
       49,    56,    62,    68,    74,    80,    86,    92,    97,   103,   109,   115,
      120,   126,   131,   136,   142,   147,   152,   157,   162,   167,   171,   176,
      181,   185,   189,   193,   197,   201,   205,   209,   212,   216,   219,   222,
      225,   228,   231,   234,   236,   238,   241,   243,   244,   246,   248,   249,
      251,   252,   253,   254,   254,   255,   255,   255,
};

static const u8 sVTransHeapName[] = "VTRANS";

static const u8 sBgHeapName[] = "BG";

static const u8 sFadeHeapName[8] = "FADE";

static u32 sMosaicSize;
static u32 sMosaicTarget;
static u16 sMosaicTimer;
static u8 sMosaicActive;

u8 DrawSpriteAllocatedTiles(s16 x, s16 y, void* sprite, void* obj, void* palette, ObjAffine* affine, u16 flags, u16 priority) {
    SpriteWork* work;
    SpriteWork* sortWork;
    u16 n;
    s32 i;
    u16 tileCount;
    u16 base;

    if (palette == NULL || sprite == NULL) {
        return FALSE;
    }

    work = gSpriteWork;
#ifdef PLATFORM_PC
    work->entries[work->entryCount].x = x + gPortSpriteShiftX;
    PortSpriteTag(work->entryCount);
#else
    work->entries[work->entryCount].x = x;
#endif
#ifdef PLATFORM_PC
    work->entries[work->entryCount].y = y + PortSpriteShiftY(work->entryCount, y);
#else
    work->entries[work->entryCount].y = y;
#endif
    work->entries[work->entryCount].tiles = obj;
    work->entries[work->entryCount].palette = palette;
    work->entries[work->entryCount].affine = affine;
    work->entries[work->entryCount].flags = flags;
    work->entries[work->entryCount].priority = priority;
    work->entries[work->entryCount].sprite = sprite;

    if (((ObjTiles*)obj)->sprite != sprite) {
        ((ObjTiles*)obj)->sprite = sprite;
        n = *(u16*)sprite;
        sprite = (u16*)sprite + 1;
        base = 0;

        if (n != 0) {
            i = n;

            do {
                tileCount = GetObjTileCount(((ObjTileListEntry*)sprite)->attr0, ((ObjTileListEntry*)sprite)->attr1);
                RequestDma3Copy(((ObjTiles*)obj)->src + ((((ObjTileListEntry*)sprite)->tile & OAM_TILE_MASK) << 5), (void*)(((((ObjTiles*)obj)->index + base) << 5) + OBJ_VRAM0), tileCount << 5);
                base += tileCount;
                sprite = (u16*)sprite + 3;
            } while (--i);
        }
    }

    sortWork = gSpriteWork;
    sortWork->sortPtrs[sortWork->entryCount] = &sortWork->entries[sortWork->entryCount];
    sortWork->entryCount += 1;
    return TRUE;
}

u8 DrawSpriteFrameTiles(s16 x, s16 y, void* obj, void* palette, ObjAffine* affine, u16 flags, u16 priority) {
    SpriteWork* work;

    if (palette == NULL || ((ObjTiles*)obj)->src == NULL) {
        return FALSE;
    }

    {
        work = gSpriteWork;
#ifdef PLATFORM_PC
        work->entries[work->entryCount].x = x + gPortSpriteShiftX;
        PortSpriteTag(work->entryCount);
#else
        work->entries[work->entryCount].x = x;
#endif
#ifdef PLATFORM_PC
        work->entries[work->entryCount].y = y + PortSpriteShiftY(work->entryCount, y);
#else
        work->entries[work->entryCount].y = y;
#endif
        work->entries[work->entryCount].tiles = obj;
        work->entries[work->entryCount].palette = palette;
        work->entries[work->entryCount].affine = affine;
        work->entries[work->entryCount].flags = flags;
        work->entries[work->entryCount].priority = priority;
        work->entries[work->entryCount].sprite = ((ObjTiles*)obj)->sprite;
        work->sortPtrs[work->entryCount] = &work->entries[work->entryCount];
        work->entryCount += 1;
    }

    return TRUE;
}

enum ObjTilesType {
    OBJ_TILES_TYPE_SHARED,
    OBJ_TILES_TYPE_ALLOCATED,
    OBJ_TILES_TYPE_FRAME
};

#ifdef PLATFORM_PC
u8 DrawMenuHeader(s16 x, s16 y, void* sprite, void* obj, void* palette, ObjAffine* affine, u16 flags, u16 priority) {
    u8 r;
    PortMenuHeaderSource(sprite, obj, palette);
    PortMenuHeaderAt(x);
    gPortHeaderDraw = 1;
    r = DrawSprite(x, y, sprite, obj, palette, affine, flags, priority);
    gPortHeaderDraw = 0;
    return r;
}
u8 DrawMenuTab(s16 x, s16 y, void* sprite, void* obj, void* palette, ObjAffine* affine, u16 flags, u16 priority) {
    PortMenuTabSprite(sprite, obj, palette);
    return DrawSprite(x, y, sprite, obj, palette, affine, flags, priority);
}
#endif

u8 DrawSprite(s16 x, s16 y, void* sprite, void* obj, void* palette, ObjAffine* affine, u16 flags, u16 priority) {
    if (gSpriteWork->entryCount <= 127 && obj != NULL) {
        switch (((ObjTiles*)obj)->type) {
        case OBJ_TILES_TYPE_SHARED:
            return DrawSpriteSharedTiles(x, y, sprite, obj, palette, affine, flags, priority);
        case OBJ_TILES_TYPE_ALLOCATED:
            return DrawSpriteAllocatedTiles(x, y, sprite, obj, palette, affine, flags, priority);
        case OBJ_TILES_TYPE_FRAME:
            return DrawSpriteFrameTiles(x, y, obj, palette, affine, flags, priority);
        }
    }

    return FALSE;
}

void DrawSpriteUnsorted(s16 x, s16 y, void* sprite, void* tiles, void* palette, u16 flags) {
    SpriteWork* work;
    u32 z;

    work = gSpriteWork;

    if (work->entryCount > 0x7F) {
        return;
    }

#ifdef PLATFORM_PC
    work->entries[work->entryCount].x = x + gPortSpriteShiftX;
    PortSpriteTag(work->entryCount);
#else
    work->entries[work->entryCount].x = x;
#endif
    z = 0;
#ifdef PLATFORM_PC
    work->entries[work->entryCount].y = y + PortSpriteShiftY(work->entryCount, y);
#else
    work->entries[work->entryCount].y = y;
#endif
    work->entries[work->entryCount].tiles = tiles;
    work->entries[work->entryCount].palette = palette;
    work->entries[work->entryCount].affine = NULL;
    work->entries[work->entryCount].flags = flags;
    work->entries[work->entryCount].priority = z;
    work->entries[work->entryCount].sprite = sprite;
    work->sortPtrs[work->entryCount] = &work->entries[work->entryCount];
    work->entryCount += 1;
    work->sortLo += 1;
}

void DrawSpriteUnsortedAffine(u16 x, u16 y, void* sprite, void* tiles, void* palette, ObjAffine* affine, u16 flags) {
    SpriteWork* work;
    u32 z;

    work = gSpriteWork;

    if (work->entryCount > 0x7F) {
        return;
    }

#ifdef PLATFORM_PC
    work->entries[work->entryCount].x = x + gPortSpriteShiftX;
    PortSpriteTag(work->entryCount);
#else
    work->entries[work->entryCount].x = x;
#endif
    z = 0;
#ifdef PLATFORM_PC
    work->entries[work->entryCount].y = y + PortSpriteShiftY(work->entryCount, y);
#else
    work->entries[work->entryCount].y = y;
#endif
    work->entries[work->entryCount].tiles = tiles;
    work->entries[work->entryCount].palette = palette;
    work->entries[work->entryCount].affine = affine;
    work->entries[work->entryCount].flags = flags;
    work->entries[work->entryCount].priority = z;
    work->entries[work->entryCount].sprite = sprite;
    work->sortPtrs[work->entryCount] = &work->entries[work->entryCount];
    work->entryCount += 1;
    work->sortLo += 1;
}

ObjTiles* LoadObjTiles(const void* src, u16 size) {
    ObjTiles* node;
    ObjTiles* cur;
    ObjTiles* next;
    s32 avail;
    s16 gap;

    if (size == 0) {
        return NULL;
    }

    if (src == NULL) {
        return NULL;
    }

    cur = ListPoolFirst(&gSpriteWork->tilePool);

    while (cur != NULL) {
        if (cur->src == src && !cur->allocated) {
            cur->refCount++;
            return cur;
        }

        cur = ListPoolNext(&cur->node);
    }

    node = ListPoolFirstFree(&gSpriteWork->tilePool);

    if (node == NULL) {
        return NULL;
    }

    node->type = OBJ_TILES_TYPE_SHARED;
    node->count = size / 32;
    node->src = src;
    node->refCount = 0;
    node->sprite = NULL;
    node->allocated = FALSE;
    node->self = node;
    cur = ListPoolFirst(&gSpriteWork->tilePool);

    if (cur == NULL) {
        node->index = gSpriteWork->tilePool.rangeStart;
        RequestDma3Copy(src, (void*)(OBJ_VRAM0 + node->index * TILE_SIZE_4BPP), size);
        ListPoolActivate(&node->node, &gSpriteWork->tilePool);
        return node;
    }

    node->index = gSpriteWork->tilePool.rangeStart;
    avail = cur->index - gSpriteWork->tilePool.rangeStart;

    if (node->count <= (s16)avail) {
        RequestDma3Copy(src, (void*)(OBJ_VRAM0 + node->index * TILE_SIZE_4BPP), size);
        ListPoolActivateBefore(&node->node, &gSpriteWork->tilePool, &cur->node);
        return node;
    }

    for (;;) {
        if (cur == NULL) {
            break;
        }

        next = ListPoolNext(&cur->node);
        node->index = cur->index + cur->count;

        if (node->index + node->count > gSpriteWork->tilePool.rangeEnd) {
            break;
        }

        if (next != NULL) {
            gap = next->index - node->index;
        } else {
            gap = gSpriteWork->tilePool.rangeEnd - node->index;
        }

        if (node->count <= gap) {
            RequestDma3Copy(src, (void*)(OBJ_VRAM0 + node->index * TILE_SIZE_4BPP), size);
            ListPoolActivateAfter(&node->node, &gSpriteWork->tilePool, &cur->node);
            return node;
        }

        cur = next;
    }

    return NULL;
}

void ReleaseSharedObjTiles(ObjTiles* tiles) {
    if ((s16)tiles->refCount > 0) {
        tiles->refCount -= 1;
    } else {
        tiles->self = NULL;
        ListPoolRelease(&tiles->node, &gSpriteWork->tilePool);
    }
}

void ReleaseAllocatedObjTiles(ObjTiles* tiles) {
    tiles->self = NULL;
    ListPoolRelease(&tiles->node, &gSpriteWork->tilePool);
}

void ReleaseSpriteFrameTiles(ObjTiles* tiles) {
    tiles->self = NULL;
    ListPoolRelease(&tiles->node, &gSpriteWork->tilePool);
}

void ReleaseObjTiles(void* tiles) {
    ObjTiles* objTiles = tiles;
    ObjTiles* self;

    if (objTiles == NULL) {
        return;
    }

    self = objTiles->self;

    if (self != objTiles) {
        return;
    }

    switch (self->type) {
    case OBJ_TILES_TYPE_SHARED:
        ReleaseSharedObjTiles(self);
        break;
    case OBJ_TILES_TYPE_ALLOCATED:
        ReleaseAllocatedObjTiles(self);
        break;
    case OBJ_TILES_TYPE_FRAME:
        ReleaseSpriteFrameTiles(self);
        break;
    }
}

ObjTiles* AllocObjTiles(u16 size, const void* owner) {
    ObjTiles* node;
    ObjTiles* cur;
    ObjTiles* next;
    s32 avail;
    s16 gap;

    if (size == 0) {
        return NULL;
    }

    node = ListPoolFirstFree(&gSpriteWork->tilePool);

    if (node == NULL) {
        return NULL;
    }

    node->type = OBJ_TILES_TYPE_ALLOCATED;
    node->count = size / 32;
    node->src = owner;
    node->refCount = 0;
    node->sprite = NULL;
    node->allocated = TRUE;
    node->self = node;
    cur = ListPoolFirst(&gSpriteWork->tilePool);

    if (cur == NULL) {
        node->index = gSpriteWork->tilePool.rangeStart;
        ListPoolActivate(&node->node, &gSpriteWork->tilePool);
        return node;
    }

    node->index = gSpriteWork->tilePool.rangeStart;
    avail = cur->index - gSpriteWork->tilePool.rangeStart;

    if (node->count <= (s16)avail) {
        ListPoolActivateBefore(&node->node, &gSpriteWork->tilePool, &cur->node);
        return node;
    }

    for (;;) {
        if (cur == NULL) {
            break;
        }

        next = ListPoolNext(&cur->node);
        node->index = cur->index + cur->count;

        if (node->index + node->count > gSpriteWork->tilePool.rangeEnd) {
            break;
        }

        if (next != NULL) {
            gap = next->index - node->index;
        } else {
            gap = gSpriteWork->tilePool.rangeEnd - node->index;
        }

        if (node->count <= gap) {
            ListPoolActivateAfter(&node->node, &gSpriteWork->tilePool, &cur->node);
            return node;
        }

        cur = next;
    }

    return NULL;
}

void SetObjTileSource(ObjTiles* tiles, const void* src) {
    // @bug Callers pass NULL when the tiles failed to allocate (NULL write).
    BIOS_PTR(tiles)->src = src;
}

enum ObjPaletteType {
    OBJ_PALETTE_TYPE_SHARED,
    OBJ_PALETTE_TYPE_ALLOCATED = 2
};

ObjPalette* LoadObjPalette(const void* src, u16 size) {
    ObjPalette* node;
    ObjPalette* cur;
    ObjPalette* next;
    s32 avail;
    s16 gap;

    if (size == 0) {
        return NULL;
    }

    if (src == NULL) {
        return NULL;
    }

    for (cur = ListPoolFirst(&gSpriteWork->palettePool); cur != NULL; cur = ListPoolNext(&cur->node)) {
        if (cur->src == src) {
            cur->refCount++;
            return cur;
        }
    }

    node = ListPoolFirstFree(&gSpriteWork->palettePool);

    if (node == NULL) {
        return NULL;
    }

    node->type = OBJ_PALETTE_TYPE_SHARED;
    node->count = size / 32;
    node->src = src;
    node->refCount = 0;
    node->self = node;
    cur = ListPoolFirst(&gSpriteWork->palettePool);

    if (cur == NULL) {
        node->index = gSpriteWork->palettePool.rangeStart;
        LoadPalette(src, (void*)(OBJ_PLTT + node->index * PLTT_SIZE_4BPP), size);
        ListPoolActivate(&node->node, &gSpriteWork->palettePool);
        return node;
    }

    node->index = gSpriteWork->palettePool.rangeStart;
    avail = cur->index - gSpriteWork->palettePool.rangeStart;

    if (node->count <= (s16)avail) {
        LoadPalette(src, (void*)(OBJ_PLTT + node->index * PLTT_SIZE_4BPP), size);
        ListPoolActivateBefore(&node->node, &gSpriteWork->palettePool, &cur->node);
        return node;
    }

    for (;;) {
        if (cur == NULL) {
            break;
        }

        next = ListPoolNext(&cur->node);
        node->index = cur->index + cur->count;

        if (node->index + node->count > gSpriteWork->palettePool.rangeEnd) {
            break;
        }

        if (next != NULL) {
            gap = next->index - node->index;
        } else {
            gap = gSpriteWork->palettePool.rangeEnd - node->index;
        }

        if (node->count <= gap) {
            LoadPalette(src, (void*)(OBJ_PLTT + node->index * PLTT_SIZE_4BPP), size);
            ListPoolActivateAfter(&node->node, &gSpriteWork->palettePool, &cur->node);
            return node;
        }

        cur = next;
    }

    return NULL;
}

void LoadObjPaletteBank(u16 bank, void* src) {
    LoadPalette(src, (void*)(OBJ_PLTT + bank * PLTT_SIZE_4BPP), 32);
}

void ReleaseObjPaletteRef(ObjPalette* palette) {
    if ((s16)palette->refCount > 0) {
        palette->refCount -= 1;
    } else {
        palette->self = NULL;
        FadeClearPaletteSlot(palette->index + 0x10);
        ListPoolRelease(&palette->node, &gSpriteWork->palettePool);
    }
}

void ReleaseObjPalette(ObjPalette* palette) {
    if (palette != NULL && palette->self == palette) {
        ReleaseObjPaletteRef(palette);
    }
}

ObjAffine* AllocObjAffineAngle(u8 angle, u8 doubleSize) {
    ObjAffine* affine;
    s32 sin;
    s32 cos;

    if (gSpriteWork->affineCount < ARRAY_COUNT(gSpriteWork->affine) && angle != 0) {
        sin = gSineTable[angle];
        cos = gSineTable[angle + 0x40];
        affine = &gSpriteWork->affine[gSpriteWork->affineCount];
        affine->pa = cos;
        affine->pb = sin;
        affine->pc = -sin;
        affine->pd = cos;
        affine->index = gSpriteWork->affineCount;
        affine->doubleSize = doubleSize;
        affine->sx = Q_8_8(1);
        affine->sy = Q_8_8(1);
        affine->angle = angle;
        gSpriteWork->affineCount += 1;
        return affine;
    }

    return NULL;
}

ObjAffine* AllocObjAffine(u8 angle, s32 sx, s32 sy, u8 doubleSize) {
    ObjAffine* affine;
    s32 sin;
    s32 cos;

    if (gSpriteWork->affineCount > 0x1F || (angle == 0 && sx == Q_8_8(1) && sy == sx)) {
        return NULL;
    }

    sin = gSineTable[angle];
    cos = gSineTable[angle + 0x40];
    affine = &gSpriteWork->affine[gSpriteWork->affineCount];
    // @bug sx or sy can be 0 (division by zero).
    affine->pa = DIV0(cos << 8, sx);
    affine->pb = DIV0(sin << 8, sy);
    affine->pc = DIV0(-sin << 8, sx);
    affine->pd = DIV0(cos << 8, sy);
    affine->index = gSpriteWork->affineCount;
    affine->doubleSize = doubleSize;
    affine->sx = sx;
    affine->sy = sy;
    affine->angle = angle;
    gSpriteWork->affineCount += 1;

    return affine;
}

ObjAffine* AllocObjAffineScaleFirst(u8 angle, s32 sx, s32 sy, u8 doubleSize) {
    ObjAffine* affine;
    s32 sin;
    s32 cos;

    if (gSpriteWork->affineCount > 0x1F || (angle == 0 && sx == Q_8_8(1) && sy == sx)) {
        return NULL;
    }

    sin = gSineTable[angle];
    cos = gSineTable[angle + 0x40];
    affine = &gSpriteWork->affine[gSpriteWork->affineCount];
    // @bug sx or sy can be 0 (division by zero).
    affine->pa = DIV0(cos << 8, sx);
    affine->pb = DIV0(sin << 8, sx);
    affine->pc = DIV0(-sin << 8, sy);
    affine->pd = DIV0(cos << 8, sy);
    affine->index = gSpriteWork->affineCount;
    affine->doubleSize = doubleSize;
    affine->sx = sx;
    affine->sy = sy;
    affine->angle = angle;
    gSpriteWork->affineCount += 1;

    return affine;
}

ObjAffine* AllocObjAffineMatrix(u16 pa, u16 pb, u16 pc, u16 pd, u8 doubleSize) {
    ObjAffine* affine;
    u32 z;

    if (gSpriteWork->affineCount > 0x1F) {
        return NULL;
    }

    affine = &gSpriteWork->affine[gSpriteWork->affineCount];
    z = 0;
    affine->pa = pa;
    affine->pb = pb;
    affine->pc = pc;
    affine->pd = pd;
    affine->index = gSpriteWork->affineCount;
    affine->doubleSize = doubleSize;
    affine->sx = Q_8_8(1);
    affine->sy = Q_8_8(1);
    affine->angle = z;
    gSpriteWork->affineCount += 1;

    return affine;
}

void SortSprites() {
    if (gSpriteWork->entryCount > 1) {
        SortSpriteEntries(gSpriteWork->sortPtrs, gSpriteWork->sortLo,
                      gSpriteWork->entryCount - 1);
    }

    gSpriteWork->sortLo = 0;
}

#define ENGINE_SET_SQUARE_SIZE(w, h, size) \
    do { \
        *(w) = (size); \
        *(h) = (size); \
    } while (0)
static inline void EngineObjSize(u16 attr0, u16 attr1, u16* w, u16* h) {
    switch ((((u32)attr1 << 16) | attr0) & OAM_SHAPE_SIZE(OAM_SHAPE_MASK, 3)) {
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 0):
        ENGINE_SET_SQUARE_SIZE(w, h, 8);
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 1):
        ENGINE_SET_SQUARE_SIZE(w, h, 16);
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 2):
        ENGINE_SET_SQUARE_SIZE(w, h, 32);
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 3):
        ENGINE_SET_SQUARE_SIZE(w, h, 64);
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 0):
        *w = 16;
        *h = 8;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 1):
        *w = 32;
        *h = 8;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 2):
        *w = 32;
        *h = 16;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 3):
        *w = 64;
        *h = 32;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 0):
        *w = 8;
        *h = 16;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 1):
        *w = 8;
        *h = 32;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 2):
        *w = 16;
        *h = 32;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 3):
        *w = 32;
        *h = 64;
        break;
    default:
        *w = 0;
        *h = 0;
        break;
    }
}

#undef ENGINE_SET_SQUARE_SIZE
void UpdateSpriteOam() {
    SpriteEntry** entries;
    SpriteEntry* entry;
    ObjAffine* affine;
    s32 emitted;
    s32 i;
    s32 j;
    s32 count;
    u16* parts;
    u16 partCount;
    u16* oam;
    u16 tileOffset;
    u8 mosaic;
    u16 attr0;
    u16 attr1;
    u16 attr2;
    s16 width;
    s16 height;
    s16 x;
    s16 y;
    s32 xx;
    s32 yy;
    s32 cosIndex;
    s32 sinIndex;
    u16 palette;
    ObjTiles* tiles;
    u32 flip;
    u32 flags;
    s16 yMask = OAM_Y_MASK;
#ifdef PLATFORM_PC
    s16 partX = 0, partY = 0, partW = 0, partH = 0;
#endif

    if (gSpriteWork->oamUpdatesPaused) {
        return;
    }
#ifdef PLATFORM_PC
    PortOamTagsReset();
#endif

    oam = (u16*)OAM;

    for (i = 0; i < gSpriteWork->affineCount; i++) {
        oam += 3;
        *oam = gSpriteWork->affine[i].pa;
        oam += 4;
        *oam = gSpriteWork->affine[i].pb;
        oam += 4;
        *oam = gSpriteWork->affine[i].pc;
        oam += 4;
        *oam = gSpriteWork->affine[i].pd;
        oam++;
    }

    gSpriteWork->affineCount = 0;
    emitted = 0;
    oam = (u16*)OAM;
    count = gSpriteWork->entryCount;
    entries = gSpriteWork->sortPtrs;
    mosaic = gSpriteWork->mosaicEnabled;

    for (i = 0; i < count; i++) {
        entry = entries[i];
        parts = entry->sprite;
        affine = entry->affine;
        partCount = *parts++;
        tileOffset = 0;

        if (mosaic && (entry->flags & SPRITE_FLAG_NO_MOSAIC) == 0) {
            entry->flags |= SPRITE_FLAG_MOSAIC;
        }

        for (j = 0; j < partCount; j++) {
            attr0 = *parts++;
            attr1 = *parts++;
            attr2 = *parts++;
            y = attr0 & OAM_Y_MASK;

            if (y & 0x80) {
                y = yMask ^ y;
                y = 0xFFFF ^ y;
            }

            x = attr1 & OAM_X_MASK;

            if (x & 0x100) {
                x = OAM_X_MASK ^ x;
                x = 0xFFFF ^ x;
            }

            EngineObjSize(attr0, attr1, &width, &height);
#ifdef PLATFORM_PC
            partX = x;
            partY = y;
            partW = width;
            partH = height;
#endif

            if (affine != NULL) {
                x += width >> 1;
                y += height >> 1;

                if (affine->angle != 0) {
                    s32 term;
                    term = gSineTable[cosIndex = (s16)((sinIndex = -affine->angle) + 64) & 255] * x;
                    sinIndex &= 255;
                    xx = term + gSineTable[sinIndex] * y;
                    yy = gSineTable[cosIndex + 64] * x + gSineTable[sinIndex + 64] * y;
                    xx = (affine->sx * xx) >> 8;
                    yy = (affine->sy * yy) >> 8;
                } else {
                    xx = affine->sx * x;
                    yy = affine->sy * y;
                }

                x = xx >> 8;
                y = yy >> 8;
                x -= width >> 1;
                y -= height >> 1;

                if (affine->doubleSize) {
                    x -= width >> 1;
                    y -= height >> 1;
                    width <<= 1;
                    height <<= 1;
                }

                if (affine->doubleSize) {
                    attr0 |= OAM_AFFINE | OAM_DOUBLE_SIZE;
                } else {
                    attr0 |= OAM_AFFINE;
                }

                attr1 |= affine->index << 9;
            } else {
                flags = entry->flags;
                flip = flags & SPRITE_FLAG_VFLIP;

                if (flip) {
                    attr1 ^= flip << 12;
                    y = -y - height;
                }

                flip = flags & SPRITE_FLAG_HFLIP;

                if (flip) {
                    attr1 ^= flip << 12;
                    x = -x - width;
                }
            }

            x += (s16)entry->x;
            y += (s16)entry->y;

#ifdef PLATFORM_PC
            if (x > VIEW_RIGHT - 1 || x <= VIEW_LEFT - width || y > VIEW_BOTTOM - 1 || y <= VIEW_TOP - height) {
#else
            if (x > DISPLAY_WIDTH - 1 || x <= -width || y > DISPLAY_HEIGHT - 1 || y <= -height) {
#endif
                if (((ObjTiles*)entry->tiles)->allocated) {
                    tileOffset += GetObjTileCount(attr0, attr1);
                }

                continue;
            }

            oam[0] = (attr0 & ~OAM_Y_MASK) | (y & OAM_Y_MASK);
            oam[1] = (attr1 & ~OAM_X_MASK) | (x & OAM_X_MASK);
#ifdef PLATFORM_PC
            /* Full-range position for the wide view (OAM keeps 9/8 bits). */
            PortOamSetPosition(emitted, x, y);
            PortOamSetSource(emitted, (int)(entry - gSpriteWork->entries));
            PortOamSetHdPart(emitted, ((ObjTiles*)entry->tiles)->src, entry->sprite, partX, partY, partW, partH,
                             affine != NULL ? 0 : entry->flags, ((ObjPalette*)entry->palette)->src, attr2 >> 12);
#endif
            tiles = entry->tiles;

            if (tiles->allocated) {
                palette = (attr2 >> 12) + ((ObjPalette*)entry->palette)->index;
                oam[2] = (attr2 & SPRITE_PRIORITY_MASK) | (tileOffset + tiles->index) | (palette << 12);
                tileOffset += GetObjTileCount(oam[0], oam[1]);
            } else {
                palette = (attr2 >> 12) + ((ObjPalette*)entry->palette)->index;
                oam[2] = ((attr2 & ~OAM_PALETTE_MASK) + tiles->index) | (palette << 12);
            }

            oam[0] |= (entry->flags & SPRITE_FLAG_MOSAIC) << 9;
            oam[0] |= (entry->flags & SPRITE_FLAG_BLEND) << 8;
            oam[2] |= entry->flags & SPRITE_PRIORITY_MASK;
            oam += 4;
            emitted++;
        }
    }

    for (i = emitted; i < 128; i++) {
        *oam = OAM_DISABLE;
        oam += 4;
    }

    gSpriteWork->entryCount = 0;
}

void SetSpriteMosaicEnabled(u8 enabled) {
    gSpriteWork->mosaicEnabled = enabled;
}

void SetObjMosaicSize(u8 x, u8 y) {
    x &= 0xF;
    y &= 0xF;
    gMosaic = (gMosaic & 0xFF) | (x << 8) | (y << 12);
}

void SetSpriteOamUpdatesPaused(u8 paused) {
    gSpriteWork->oamUpdatesPaused = paused;
}

u16 GetMaxSpriteTileBytes(void** sprites, u16 n) {
    u16* sprite;
    u16 count;
    u16 sum;
    u16 max;
    u16 i;
    u16 j;

    max = 0;

    for (i = 0; i < n; i++) {
        sprite = sprites[i];
        count = sprite[0];
        sprite++;
        sum = 0;

        for (j = 0; j < count; j++) {
            sum += GetObjTileCount(sprite[0], sprite[1]);
            sprite += 3;
        }

        if (max < sum) {
            max = sum;
        }
    }

    return max * 32;
}

u16 GetSpriteTileBytes(u16* sprite) {
    u16 count = *sprite++;
    u16 total = 0;
    u16 i;

    for (i = 0; i < count; i++) {
        total += GetObjTileCount(sprite[0], sprite[1]);
        sprite += 3;
    }

    return total << 5;
}

u8 IsRectOutsideScreen(s16 x, s16 y, s32 topExtent, s32 bottomExtent, s32 leftExtent, s32 rightExtent) {
    u16 top = topExtent;
    u16 bottom = bottomExtent;
    u16 left = leftExtent;
    s16 right = rightExtent;

#ifdef PLATFORM_PC
    if (x + right < VIEW_LEFT || x - (s16)left > VIEW_RIGHT || y + (s16)bottom < VIEW_TOP || y - (s16)top > VIEW_BOTTOM) {
        return TRUE;
    }
#else
    if (x + right < 0) {
        return TRUE;
    }

    if (x - (s16)left > DISPLAY_WIDTH) {
        return TRUE;
    }

    if (y + (s16)bottom < 0) {
        return TRUE;
    }

    if (y - (s16)top > DISPLAY_HEIGHT) {
        return TRUE;
    }
#endif

    return FALSE;
}

u8 IsSpriteOutsideScreen(u16* oam, s16 x, s16 y) {
    u16 i;
    u16 n;
    u16 attr0;
    u16 attr1;
    s16 dx;
    s16 dy;
    s16 raw;
    s16 w;
    s16 h;

    n = *oam++;

    for (i = 0; i < n; i++) {
        attr0 = *oam++;
        attr1 = *oam;
        oam += 2;
        dx = attr1 & OAM_X_MASK;
        raw = dx;

        if (raw & 0x100) {
            dx = raw ^ OAM_X_MASK;
            dx = ~dx;
        }

        dy = attr0 & OAM_Y_MASK;
        raw = dy;

        if (raw & 0x80) {
            dy = raw ^ OAM_Y_MASK;
            dy = ~dy;
        }

        // @bug Each part is offset from the previous one, not the sprite's position (never called).
        x += dx;
        y += dy;
        EngineObjSize(attr0, attr1, &w, &h);

        if (x <= DISPLAY_WIDTH - 1 && x > -w && y <= DISPLAY_HEIGHT - 1 && y > -h) {
            return FALSE;
        }
    }

    return TRUE;
}

void InitObjTilesAtSlot(ObjTiles* tiles, u16 slot, void* src, u16 size) {
    if (slot + (size >> 5) <= 0x400) {
        tiles->type = OBJ_TILES_TYPE_SHARED;
        tiles->count = size >> 5;
        tiles->src = src;
        tiles->refCount = 0;
        tiles->sprite = NULL;
        tiles->allocated = FALSE;
        tiles->index = slot;
        RequestDma3Copy(src, (void*)(OBJ_VRAM0 + tiles->index * TILE_SIZE_4BPP), size);
    }
}

void InitDynamicObjTilesAtSlot(ObjTiles* tiles, u16 slot, u16 size, void* src) {
    if (slot + (size >> 5) <= 0x400) {
        tiles->type = OBJ_TILES_TYPE_ALLOCATED;
        tiles->count = size >> 5;
        tiles->src = src;
        tiles->refCount = 0;
        tiles->sprite = NULL;
        tiles->allocated = TRUE;
        tiles->index = slot;
    }
}

void InitObjPaletteAtSlot(ObjPalette* palette, u16 slot, void* src, u16 size) {
    if (slot + (size >> 5) <= 0x10) {
        palette->type = OBJ_PALETTE_TYPE_SHARED;
        palette->count = size >> 5;
        palette->src = src;
        palette->refCount = 0;
        palette->index = slot;
        RequestDma3Copy(src, (void*)(OBJ_PLTT + palette->index * PLTT_SIZE_4BPP), size);
    }
}

ObjTiles* AllocSpriteFrameTiles(u16 size) {
    ObjTiles* tiles = AllocObjTiles(size, NULL);

    if (tiles != NULL) {
        tiles->type = OBJ_TILES_TYPE_FRAME;
    }

    return tiles;
}

u8 UpdateSpriteFrameTiles(ObjTiles* tiles, u16* sprite, void* src) {
    u16 count;
    s32 j;
    u16 tileOffset;
    u16 tileCount;

    if (sprite != NULL && src != NULL && tiles->type == OBJ_TILES_TYPE_FRAME) {
        if (tiles->sprite != sprite || tiles->src != src) {
            tiles->src = src;
            tiles->sprite = sprite;
            count = *sprite;
            sprite++;
            tileOffset = 0;

            if (count != 0) {
                j = count;

                do {
                    tileCount = GetObjTileCount(sprite[0], sprite[1]);
                    RequestDma3Copy(tiles->src + ((sprite[2] & OAM_TILE_MASK) << 5),
                                    (void*)(((tiles->index + tileOffset) << 5) + OBJ_VRAM0), tileCount * 32);
                    tileOffset = tileOffset + tileCount;
                    sprite += 3;
                    j--;
                } while (j != 0);
            }

            return TRUE;
        }
    }

    return FALSE;
}

ObjPalette* AllocObjPalette(u16 size) {
    ObjPalette* node;
    ObjPalette* cur;
    ObjPalette* next;
    s32 avail;
    s16 gap;

    node = ListPoolFirstFree(&gSpriteWork->palettePool);

    if (node == NULL) {
        return NULL;
    }

    node->type = OBJ_PALETTE_TYPE_ALLOCATED;
    node->count = size / 32;
    node->src = NULL;
    node->refCount = 0;
    node->self = node;
    cur = ListPoolFirst(&gSpriteWork->palettePool);

    if (cur == NULL) {
        node->index = gSpriteWork->palettePool.rangeStart;
        ListPoolActivate(&node->node, &gSpriteWork->palettePool);
        return node;
    }

    node->index = gSpriteWork->palettePool.rangeStart;
    avail = cur->index - gSpriteWork->palettePool.rangeStart;

    if (node->count <= (s16)avail) {
        ListPoolActivateBefore(&node->node, &gSpriteWork->palettePool, &cur->node);
        return node;
    }

    for (;;) {
        if (cur == NULL) {
            break;
        }

        next = ListPoolNext(&cur->node);
        node->index = cur->index + cur->count;

        if (node->index + node->count > gSpriteWork->palettePool.rangeEnd) {
            break;
        }

        if (next != NULL) {
            gap = next->index - node->index;
        } else {
            gap = gSpriteWork->palettePool.rangeEnd - node->index;
        }

        if (node->count <= gap) {
            ListPoolActivateAfter(&node->node, &gSpriteWork->palettePool, &cur->node);
            return node;
        }

        cur = next;
    }

    return NULL;
}

void UpdateAllocatedObjPalette(ObjPalette* palette, void* src) {
    if (palette->type == OBJ_PALETTE_TYPE_ALLOCATED) {
        LoadPalette(src, (void*)(OBJ_PLTT + palette->index * PLTT_SIZE_4BPP), palette->count << 5);
    }
}

u8 CanAllocObjTiles(u16 count) {
    ObjTiles* cur;
    ObjTiles* next;
    u16 pos;
    s16 gap;

    cur = ListPoolFirst(&gSpriteWork->tilePool);

    if (cur == NULL) {
        return TRUE;
    }

    pos = gSpriteWork->tilePool.rangeStart;

    if (count <= (s16)(cur->index - pos)) {
        return TRUE;
    }

    for (;;) {
        if (cur == NULL) {
            break;
        }

        next = ListPoolNext(&cur->node);
        pos = cur->index + cur->count;

        if ((s16)pos + count > gSpriteWork->tilePool.rangeEnd) {
            break;
        }

        if (next != NULL) {
            gap = next->index - pos;
        } else {
            gap = gSpriteWork->tilePool.rangeEnd - pos;
        }

        if (count <= gap) {
            return TRUE;
        }

        cur = next;
    }

    return FALSE;
}

u8 CanAllocObjPalette(u16 count) {
    ObjPalette* cur;
    ObjPalette* next;
    u16 pos;
    s16 gap;

    cur = ListPoolFirst(&gSpriteWork->palettePool);

    if (cur == NULL) {
        return TRUE;
    }

    pos = gSpriteWork->palettePool.rangeStart;

    if (count <= (s16)(cur->index - pos)) {
        return TRUE;
    }

    for (;;) {
        if (cur == NULL) {
            break;
        }

        next = ListPoolNext(&cur->node);
        pos = cur->index + cur->count;

        if ((s16)pos + count > gSpriteWork->palettePool.rangeEnd) {
            break;
        }

        if (next != NULL) {
            gap = next->index - pos;
        } else {
            gap = gSpriteWork->palettePool.rangeEnd - pos;
        }

        if (count <= gap) {
            return TRUE;
        }

        cur = next;
    }

    return FALSE;
}

void GetObjSize(u16 attr0, u16 attr1, u16* w, u16* h) {
    switch (((attr1 << 16) | attr0) & OAM_SHAPE_SIZE(OAM_SHAPE_MASK, 3)) {
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 0):
        *w = 8;
        *h = 8;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 1):
        *w = 16;
        *h = 16;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 2):
        *w = 32;
        *h = 32;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_SQUARE, 3):
        *w = 64;
        *h = 64;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 0):
        *w = 16;
        *h = 8;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 1):
        *w = 32;
        *h = 8;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 2):
        *w = 32;
        *h = 16;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_HORIZONTAL, 3):
        *w = 64;
        *h = 32;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 0):
        *w = 8;
        *h = 16;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 1):
        *w = 8;
        *h = 32;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 2):
        *w = 16;
        *h = 32;
        break;
    case OAM_SHAPE_SIZE(OAM_SHAPE_VERTICAL, 3):
        *w = 32;
        *h = 64;
        break;
    default:
        *w = 0;
        *h = 0;
        break;
    }
}

s32 Sqrt8(s32 value) {
    s32 x;
    s32 prev;

    if (value > 0) {
        x = 0x100;

        if (value > 0x100) {
            x = value;
        }

        do {
            prev = x;
            x = ((value << 8) / prev + prev) / 2;
        } while (x < prev);

        return prev;
    }

    return 0;
}

void SplineBuildAxisCoefficients(Spline2D* spline, s32* knots, s32* values, s32* coefficients) {
    s32* intervals;
    s32* scratch;
    s32 n;
    s32 i;
    s32 ratio;

    n = spline->pointCount;
    intervals = spline->intervals;
    scratch = spline->scratch;
    coefficients[0] = 0;
    coefficients[n - 1] = 0;

    for (i = 0; i < n - 1; i++) {
        intervals[i] = knots[i + 1] - knots[i];
        scratch[i + 1] = ((values[i + 1] - values[i]) << 8) / intervals[i];
    }

    coefficients[1] = (scratch[2] - scratch[1]) - ((intervals[0] * coefficients[0]) >> 8);
    scratch[1] = (knots[2] - knots[0]) << 1;

    for (i = 1; i < n - 2; i++) {
        ratio = (intervals[i] << 8) / scratch[i];
        coefficients[i + 1] = (scratch[i + 2] - scratch[i + 1]) - ((coefficients[i] * ratio) >> 8);
        scratch[i + 1] = ((knots[i + 2] - knots[i]) << 1) - ((ratio * intervals[i]) >> 8);
    }

    coefficients[n - 2] -= (intervals[n - 2] * coefficients[n - 1]) >> 8;

    for (i = n - 2; i > 0; i--) {
        coefficients[i] = ((coefficients[i] - ((intervals[i] * coefficients[i + 1]) >> 8)) << 8) / scratch[i];
    }
}

s32 SplineEvaluateAxis(s16* pointCount, s32 position, s32* knots, s32* values, s32* coefficients) {
    s32 lo;
    s32 hi;
    s32 mid;
    s32 dx;
    s32 offset;
    s32 coef0;
    s32 coef1;
    s32 term;
    s32 count;

    count = *pointCount;
    lo = 0;
    hi = count - 1;

    while (lo < hi) {
        mid = (lo + hi) / 2;

        if (knots[mid] < position) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo > 0) {
        lo--;
    }

    dx = knots[lo + 1] - knots[lo];
    offset = position - knots[lo];
    coef1 = coefficients[lo + 1];
    coef0 = coefficients[lo];
    term = (((offset * (coef1 - coef0)) >> 8) << 8) / dx;
    term = (offset * (term + coef0 * 3)) >> 8;
    term += ((values[lo + 1] - values[lo]) << 8) / dx - ((dx * (coef0 * 2 + coef1)) >> 8);
    return ((offset * term) >> 8) + values[lo];
}

void SplineInit2D(Spline2D* spline, s32* xValues, s32* yValues, s16 pointCount) {
    s32 i;
    s32 len;
    s32* knots;
    s32* xCoefficients;
    s32* yCoefficients;
    s32 dx;
    s32 dy;
    s32 size;

    size = pointCount * 4;
    len = 0;
    spline->pointCount = pointCount;
    spline->intervals = EwramAlloc(size);
    spline->scratch = EwramAlloc(size);
    spline->knots = EwramAlloc(size);
    spline->xCoefficients = EwramAlloc(size);
    spline->yCoefficients = EwramAlloc(size);
    spline->xValues = xValues;
    spline->yValues = yValues;
    knots = spline->knots;
    xCoefficients = spline->xCoefficients;
    yCoefficients = spline->yCoefficients;
    knots[0] = len;

    for (i = 1; i < pointCount; i++) {
        dx = xValues[i] - xValues[i - 1];
        dy = yValues[i] - yValues[i - 1];
        knots[i] = knots[i - 1] + Sqrt8(((dx * dx) >> 8) + ((dy * dy) >> 8));
    }

    for (i = 1; i < pointCount; i++) {
        knots[i] = (knots[i] << 8) / knots[pointCount - 1];
    }

    SplineBuildAxisCoefficients(spline, knots, xValues, xCoefficients);
    SplineBuildAxisCoefficients(spline, knots, yValues, yCoefficients);
}

void SplineEvaluate2D(Spline2D* spline, s32 position, s32* outX, s32* outY) {
    *outX = SplineEvaluateAxis(&spline->pointCount, position, spline->knots, spline->xValues, spline->xCoefficients);
    *outY = SplineEvaluateAxis(&spline->pointCount, position, spline->knots, spline->yValues, spline->yCoefficients);
}

void SplineFreeBuffers(Spline2D* spline) {
    EwramFree(spline->intervals);
    EwramFree(spline->scratch);
    EwramFree(spline->knots);
    EwramFree(spline->xCoefficients);
    EwramFree(spline->yCoefficients);
}

void InitDisplayRegs() {
    gDispCnt = DISPCNT_OBJ_1D_MAP;
    gMosaic = 0;
    gBldCnt = 0;
    gBldAlpha = 0;
    gBldY = 0;
    gWin0H = 0;
    gWin1H = 0;
    gWin0V = 0;
    gWin1V = 0;
    gWinIn = 0;
    gWinOut = 0;
    gBg0Cnt = 0;
    gBg1Cnt = 0;
    gBg2Cnt = 0;
    gBg3Cnt = 0;
    gBg0HOfs = 0;
    gBg0VOfs = 0;
    gBg1HOfs = 0;
    gBg1VOfs = 0;
    gBg2HOfs = 0;
    gBg2VOfs = 0;
    gBg3HOfs = 0;
    gBg3VOfs = 0;
    gBg2PA = 0x100;
    gBg2PB = 0;
    gBg2PC = 0;
    gBg2PD = 0x100;
    gBg2X = 0;
    gBg2Y = 0;
    gBg3PA = 0x100;
    gBg3PB = 0;
    gBg3PC = 0;
    gBg3PD = 0x100;
    gBg3X = 0;
    gBg3Y = 0;
}

void CommitDisplayRegs() {
    REG_MOSAIC = gMosaic;
    REG_BLDCNT = gBldCnt;
    REG_BLDALPHA = gBldAlpha;
    REG_BLDY = gBldY;
    REG_WIN0H = gWin0H;
    REG_WIN1H = gWin1H;
    REG_WIN0V = gWin0V;
    REG_WIN1V = gWin1V;
    REG_WININ = gWinIn;
    REG_WINOUT = gWinOut;
    REG_BG0CNT = gBg0Cnt;
    REG_BG1CNT = gBg1Cnt;
    REG_BG2CNT = gBg2Cnt;
    REG_BG3CNT = gBg3Cnt;
    REG_BG0HOFS = gBg0HOfs;
    REG_BG0VOFS = gBg0VOfs;
    REG_BG1HOFS = gBg1HOfs;
    REG_BG1VOFS = gBg1VOfs;
    REG_BG2HOFS = gBg2HOfs;
    REG_BG2VOFS = gBg2VOfs;
    REG_BG3HOFS = gBg3HOfs;
    REG_BG3VOFS = gBg3VOfs;
    REG_BG2PA = gBg2PA;
    REG_BG2PB = gBg2PB;
    REG_BG2PC = gBg2PC;
    REG_BG2PD = gBg2PD;
    REG_BG2X = gBg2X;
    REG_BG2Y = gBg2Y;
    REG_BG3PA = gBg3PA;
    REG_BG3PB = gBg3PB;
    REG_BG3PC = gBg3PC;
    REG_BG3PD = gBg3PD;
    REG_BG3X = gBg3X;
    REG_BG3Y = gBg3Y;
    REG_DISPCNT = gDispCnt;
    *(vu16*)PLTT = gBackdropColor;
#ifdef PLATFORM_PC
    /* Map positions that go with the scroll registers just written. */
    PortLatchBgMaps(gBgWork->entries);
#endif
}

void VTransInit() {
    SetIwramHeapName(sVTransHeapName);
    gDma3Requests = IwramAlloc(sizeof(Dma3Queue));
    CpuFill32(0, gDma3Requests, sizeof(Dma3Queue));
}

void VTransFree() {
    IwramFree(gDma3Requests);
}

void VTransReset() {
    Dma3Queue* queue = gDma3Requests;

    queue->requestCount = 0;
    queue->blitCount = 0;
    queue->fillCount = 0;
    queue->callbackCount = 0;
    queue->count = 0;
#ifdef VERSION_EU
    queue->lz77RequestCount = 0;
#endif
    queue->transferredBytes = 0;
}

u8 RequestDma3Copy(const void* src, void* dst, u16 size) {
    Dma3Queue* queue;

    if (size == 0) {
        return FALSE;
    }

    queue = gDma3Requests;

    if (queue->requestCount > 255) {
        return FALSE;
    }

    if ((gSystemFlags & SYSTEM_FLAG_DMA3_IMMEDIATE) == 0) {
        queue->requests[queue->requestCount].src = src;
        queue->requests[queue->requestCount].dst = dst;
        queue->requests[queue->requestCount].size = size;
        queue->requestCount = queue->requestCount + 1;
    } else {
        DmaCopy16(3, src, dst, size);
    }

    return TRUE;
}

#ifdef VERSION_EU
u8 RequestLz77UnCompVram(void* src, void* dst) {
    Dma3Queue* queue = gDma3Requests;
    u16 flags;

    if (queue->lz77RequestCount > 31) {
        return FALSE;
    }

    flags = gSystemFlags & SYSTEM_FLAG_DMA3_IMMEDIATE;

    if (flags == 0) {
        queue->lz77Requests[queue->lz77RequestCount].src = src;
        queue->lz77Requests[queue->lz77RequestCount].dst = dst;
        queue->lz77Requests[queue->lz77RequestCount].size = flags;
        queue->lz77RequestCount++;
    } else {
        LZ77UnCompVram(src, dst);
    }

    return TRUE;
}
#endif

u8 RequestDma3Clear(void* dst, u16 size) {
    Dma3Queue* queue = gDma3Requests;

    if (queue->count > 3) {
        return FALSE;
    }

    queue->pending[queue->count].dst = dst;
    queue->pending[queue->count].size = size;
    queue->count = queue->count + 1;

    return TRUE;
}

u8 RequestTilemapRectCopy(void* src, void* dst, u8 x, u8 y, u8 dstX, u8 dstY, s8 sw, s8 sh) {
    if (gDma3Requests->blitCount > 63) {
        return FALSE;
    }

    if (sw <= 0 || sh <= 0) {
        return FALSE;
    }

    gDma3Requests->blits[gDma3Requests->blitCount].src = src;
    gDma3Requests->blits[gDma3Requests->blitCount].dst = dst;
    gDma3Requests->blits[gDma3Requests->blitCount].srcX = x;
    gDma3Requests->blits[gDma3Requests->blitCount].srcY = y;
    gDma3Requests->blits[gDma3Requests->blitCount].dstX = dstX;
    gDma3Requests->blits[gDma3Requests->blitCount].dstY = dstY;
    gDma3Requests->blits[gDma3Requests->blitCount].width = sw;
    gDma3Requests->blits[gDma3Requests->blitCount].height = sh;
    gDma3Requests->blitCount = gDma3Requests->blitCount + 1;
    return TRUE;
}

u8 RequestTilemapStripCopy(void* src, void* dst, u8 x, u8 y, u8 vertical) {
    if (gDma3Requests->fillCount > 7) {
        return FALSE;
    }

    gDma3Requests->fills[gDma3Requests->fillCount].src = src;
    gDma3Requests->fills[gDma3Requests->fillCount].dst = dst;
    gDma3Requests->fills[gDma3Requests->fillCount].x = x & 0x1F;
    gDma3Requests->fills[gDma3Requests->fillCount].y = y & 0x1F;
    gDma3Requests->fills[gDma3Requests->fillCount].vertical = vertical;
    gDma3Requests->fillCount = gDma3Requests->fillCount + 1;

    return TRUE;
}

u8 QueueVTransCallback(void (*callback)()) {
    Dma3Queue* queue = gDma3Requests;

    if (queue->callbackCount > 7) {
        return FALSE;
    }

    queue->callbacks[queue->callbackCount] = callback;
    queue->callbackCount = queue->callbackCount + 1;

    return TRUE;
}

u32 GetVTransTransferredBytes() {
    Dma3Queue* queue = gDma3Requests;

    return queue->transferredBytes;
}

void FlushDma3Queue() {
    Dma3Queue* queue;
    Dma3Request* req;
    Dma3Blit* blits;
    Dma3Fill* fills;
    Dma3Fill* fill;
    s32 sx;
    s32 dx;
    s32 sy;
    s32 dy;
    Dma3Pending* pend;
    void (**cb)();
    u16 n;
    s32 i;
    s32 mask;
    s32 row;
    s32 col;
#ifdef VERSION_EU
    Dma3Request* compressed;
#endif

    queue = gDma3Requests;
    req = queue->requests;
    blits = queue->blits;
    fills = queue->fills;
    cb = queue->callbacks;
    pend = queue->pending;
#ifdef VERSION_EU
    compressed = queue->lz77Requests;
#endif
    queue->transferredBytes = 0;
    n = queue->callbackCount;

    for (i = 0; i < n; i++) {
        cb[i]();
    }

    gDma3Requests->callbackCount = 0;
    n = gDma3Requests->requestCount;

    for (i = 0; i < n; i++) {
        DmaCopy16(3, req[i].src, req[i].dst, req[i].size);
        gDma3Requests->transferredBytes += req[i].size;
    }

    gDma3Requests->requestCount = 0;
#ifdef VERSION_EU
    n = gDma3Requests->lz77RequestCount;

    for (i = 0; i < n; i++) {
        LZ77UnCompVram(compressed[i].src, compressed[i].dst);
    }

    gDma3Requests->lz77RequestCount = 0;
#endif
    n = gDma3Requests->fillCount;

    for (i = 0; i < n; i++) {
        mask = 31;

        if (fills[i].vertical) {
            for (row = 0; row < 32; row++) {
                fill = &fills[i];
                dy = ((fill->y + row) & mask) << 5;
                ((u16*)fill->dst)[dy + fill->x] = ((u16*)fill->src)[row];
            }
        } else {
            for (col = 0; col < 32; col++) {
                fill = &fills[i];
                sx = (fill->x + col) & mask;
                ((u16*)fill->dst)[(fill->y << 5) + sx] = ((u16*)fill->src)[col];
            }
        }
    }

    gDma3Requests->fillCount = 0;
    n = gDma3Requests->blitCount;

    for (i = 0; i < n; i++) {
        for (row = 0; row < blits[i].height; row++) {
            sy = ((blits[i].srcY + row) & 31) << 5;
            dy = ((blits[i].dstY + row) & 31) << 5;

            for (col = 0; col < blits[i].width; col++) {
                sx = (blits[i].srcX + col) & 31;
                dx = (blits[i].dstX + col) & 31;
                ((u16*)blits[i].dst)[dx + dy] = ((u16*)blits[i].src)[sx + sy];
            }
        }
    }

    gDma3Requests->blitCount = 0;
    n = gDma3Requests->count;

    for (i = 0; i < n; i++) {
        DmaFill16(3, 0, pend[i].dst, pend[i].size);
        gDma3Requests->transferredBytes += pend[i].size;
    }

    gDma3Requests->count = 0;
}

void FlushDma3QueueWithCpu() {
    Dma3Queue* queue;
    Dma3Request* req;
    Dma3Blit* blits;
    Dma3Fill* fills;
    Dma3Fill* fill;
    s32 sy;
    s32 dy;
    Dma3Pending* pend;
    void (**cb)();
    void (**callback)();
    u16 n;
    s32 i;
    s32 mask;
    s32 row;
    s32 col;
    s32 sourceIndex;
#ifdef VERSION_EU
    Dma3Request* compressed;
    Dma3Request* currentCompressed;
#endif

    queue = gDma3Requests;
    req = queue->requests;
    blits = queue->blits;
    fills = queue->fills;
    cb = queue->callbacks;
    pend = queue->pending;
#ifdef VERSION_EU
    compressed = queue->lz77Requests;
#endif
    queue->transferredBytes = 0;
    n = queue->callbackCount;

    if (n != 0) {
        callback = cb;
        i = n;

        do {
            (*callback++)();
        } while (--i);
    }

    gDma3Requests->callbackCount = 0;
    n = gDma3Requests->requestCount;

    if (n != 0) {
        i = n;

        do {
            CpuCopy16(req->src, req->dst, req->size);
            gDma3Requests->transferredBytes += req->size;
            req++;
        } while (--i);
    }

    gDma3Requests->requestCount = 0;
#ifdef VERSION_EU
    n = gDma3Requests->lz77RequestCount;

    if (n != 0) {
        currentCompressed = compressed;
        i = n;

        do {
            LZ77UnCompVram(currentCompressed->src, currentCompressed->dst);
            currentCompressed++;
        } while (--i);
    }

    gDma3Requests->lz77RequestCount = 0;
#endif
    n = gDma3Requests->fillCount;

    for (i = 0; i < n; i++) {
        mask = 31;

        if (fills[i].vertical) {
            row = 0;

            for (; row < 32; row++) {
                fill = &fills[i];
                dy = ((fill->y + row) & mask) << 5;
                ((u16*)fill->dst)[dy + fill->x] = ((u16*)fill->src)[row];
            }
        } else {
            col = 0;

            for (; col < 32; col++) {
                fill = &fills[i];
                sourceIndex = (fill->x + col) & mask;
                ((u16*)fill->dst)[(fill->y << 5) + sourceIndex] = ((u16*)fill->src)[col];
            }
        }
    }

    gDma3Requests->fillCount = 0;
    n = gDma3Requests->blitCount;

    for (i = 0; i < n; i++) {
        for (row = 0; row < blits[i].height; row++) {
            sy = ((blits[i].srcY + row) & 31) << 5;
            dy = ((blits[i].dstY + row) & 31) << 5;

            for (col = 0; col < blits[i].width; col++) {
                sourceIndex = (blits[i].srcX + col) & 31;
                ((u16*)blits[i].dst)[((blits[i].dstX + col) & 31) + dy] = ((u16*)blits[i].src)[sourceIndex + sy];
            }
        }
    }

    gDma3Requests->blitCount = 0;
    n = gDma3Requests->count;

    for (i = 0; i < n; i++) {
        CpuFill16(0, pend[i].dst, pend[i].size);
        gDma3Requests->transferredBytes += pend[i].size;
    }

    gDma3Requests->count = 0;
}

void BgInit() {
    BgWork** bgWork;

    SetIwramHeapName(sBgHeapName);
    bgWork = &gBgWork;
    *bgWork = IwramAlloc(sizeof(BgWork));
    CpuFill32(0, *bgWork, sizeof(BgWork));
}

void BgFree() {
    IwramFree(gBgWork);
}

void* GetBgMapBlock(BgEntry* entry, u16 x, u16 y) {
    u8 col = (x >> 8) % entry->width;
    u8 row = (y >> 8) % entry->height;

    return entry->map[entry->width * row + col];
}

void CopyBgMapRect(u16 x, u16 y, BgEntry* entry, void* dst, u8 sx, u8 sy, u8 w, u8 h) {
    u8 tx;
    u8 ty;
    s8 w1;
    s8 w2;
    s8 h1;
    s8 h2;
    u16 x2;
    u16 y2;
    u8 sx2;
    u8 sy2;
    s32 ox;
    s32 oy;

    tx = (x & 0xFF) >> 3;
    ty = (y & 0xFF) >> 3;
    w1 = 32 - tx;

    if (w1 >= w) {
        w1 = w;
        w2 = 0;
    } else {
        w2 = w - w1;
    }

    h1 = 32 - ty;

    if (h1 >= h) {
        h1 = h;
        h2 = 0;
    } else {
        h2 = h - h1;
    }

    RequestTilemapRectCopy(GetBgMapBlock(entry, x, y), dst, tx, ty, sx, sy, w1, h1);
    x2 = x + 256;
    RequestTilemapRectCopy(GetBgMapBlock(entry, x2, y), dst, 0, ty, sx2 = sx - (ox = tx - 32), sy, w2, h1);
    y2 = y + 256;
    RequestTilemapRectCopy(GetBgMapBlock(entry, x, y2), dst, tx, 0, sx, sy2 = sy - (oy = ty - 32), w1, h2);
    RequestTilemapRectCopy(GetBgMapBlock(entry, x2, y2), dst, 0, 0, sx2, sy2, w2, h2);
}

void BgReset() {
#ifdef PLATFORM_PC
    {
        s32 b;
        for (b = 0; b < 4; b++) {
            PortSetBgShiftX(b, 0);
            PortSetBgWorld(b, NULL, 0, 0);
            PortSetBgIsoExtend(b, 0);
        }
    }
#endif
#ifdef VERSION_EU
    CpuFill32(0, gBgWork, sizeof(BgWork));
#endif
    gBackdropColor = 0;
    DisableBg(0);
    DisableBg(1);
    DisableBg(2);
    DisableBg(3);
    SetBgMosaicSize(0, 0);
    gBldCnt = 0;
}

void SetBgMode0() {
    s32 i;

    gDispCnt = gDispCnt & ~DISPCNT_MODE_MASK;
    gBg0Cnt = 0;
    gBg1Cnt = BGCNT_PRIORITY(1);
    gBg2Cnt = BGCNT_PRIORITY(2);
    gBg3Cnt = BGCNT_PRIORITY(3);
    SetupBg(0, 0, 7, 0);
    SetupBg(1, 1, 15, 4);
    SetupBg(2, 2, 23, 8);
    SetupBg(3, 3, 31, 12);
    SetBgScroll(0, 0, 0);
    SetBgScroll(1, 0, 0);
    SetBgScroll(2, 0, 0);
    SetBgScroll(3, 0, 0);

    for (i = 0; i <= 3; i++) {
        gBgWork->entries[i].map = NULL;
    }
}

void SetBgMode1() {
    s32 i;

    gDispCnt = (gDispCnt & ~DISPCNT_MODE_MASK) | DISPCNT_MODE_1;
    gBg0Cnt = 0;
    gBg1Cnt = BGCNT_PRIORITY(1);
    gBg2Cnt = (BGCNT_PRIORITY(2) | BGCNT_256COLOR);
    SetupBg(0, 0, 7, 0);
    SetupBg(1, 1, 15, 0);
    SetupBg(2, 2, 23, 0);
    SetBgScroll(0, 0, 0);
    SetBgScroll(1, 0, 0);
    SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), 0, 0);

    for (i = 0; i <= 3; i++) {
        gBgWork->entries[i].map = NULL;
    }
}

void SetBgMode2() {
    s32 i;

    gDispCnt = (gDispCnt & ~DISPCNT_MODE_MASK) | DISPCNT_MODE_2;
    gBg2Cnt = (BGCNT_256COLOR | BGCNT_WRAP | BGCNT_AFF256x256);
    gBg3Cnt = (BGCNT_PRIORITY(1) | BGCNT_256COLOR | BGCNT_AFF256x256);
    SetupBg(2, 0, 15, 0);
    SetupBg(3, 2, 31, 0);
    SetBgAffine(2, 0, Q_8_8(1), Q_8_8(1), 0, 0);
    SetBgAffine(3, 0, Q_8_8(1), Q_8_8(1), 0, 0);

    for (i = 0; i <= 3; i++) {
        gBgWork->entries[i].map = NULL;
    }
}

void SetBgMode3() {
    gDispCnt = (gDispCnt & ~DISPCNT_MODE_MASK) | DISPCNT_MODE_3;
    SetBgScroll(2, 0, 0);
}

void EnableBg(s32 bg) {
    switch ((u32)bg) {
    case 0:
        gDispCnt |= DISPCNT_BG0_ON;
        break;
    case 1:
        gDispCnt |= DISPCNT_BG1_ON;
        break;
    case 2:
        gDispCnt |= DISPCNT_BG2_ON;
        break;
    case 3:
        gDispCnt |= DISPCNT_BG3_ON;
        break;
    }
}

void DisableBg(s32 bg) {
    switch ((u32)bg) {
    case 0:
        gDispCnt &= ~DISPCNT_BG0_ON;
        break;
    case 1:
        gDispCnt &= ~DISPCNT_BG1_ON;
        break;
    case 2:
        gDispCnt &= ~DISPCNT_BG2_ON;
        break;
    case 3:
        gDispCnt &= ~DISPCNT_BG3_ON;
        break;
    }
}

void SetupBg(s32 bg, u8 charBase, u8 screenBase, u8 palette) {
    vu16* control = sBgControl[bg];

#ifdef PLATFORM_PC
    /* a new owner sets the layer up: no docking left over from the last one */
    PortSetBgShiftX(bg, 0);
#endif
    *control = (*control & ~BGCNT_CHARBASE_MASK) | BGCNT_CHARBASE(charBase);
    *control = (*control & ~BGCNT_SCREENBASE_MASK) | BGCNT_SCREENBASE(screenBase);
    gBgPaletteBank[bg] = palette;
}

void LoadBgTiles(s32 bg, void* src, u16 size) {
    EnableBg(bg);
    RequestDma3Copy(src, GetBgCharBase(bg), size);
}

void LoadBgPalette(s32 bg, void* src, u16 size) {
    EnableBg(bg);
    LoadPalette(src, (void*)((gBgPaletteBank[bg] << 5) + PLTT), size);
}

void LoadBgMap(s32 bg, const void* src, u16 size) {
    EnableBg(bg);
    RequestDma3Copy(src, GetBgScreenBase(bg), size);
}

void* GetBgCharBase(s32 bg) {
    return (void*)(((*sBgControl[bg] & BGCNT_CHARBASE_MASK) << 12) + VRAM);
}

void* GetBgScreenBase(s32 bg) {
    return (void*)(((*sBgControl[bg] & BGCNT_SCREENBASE_MASK) << 3) + VRAM);
}

void SetBgMapBlocks(s32 bg, const void* src, u8 w, u8 h) {
#ifdef PLATFORM_PC
    PortSetBgShiftX(bg, 0);
    PortSetBgWorld(bg, NULL, 0, 0);
    PortSetBgIsoExtend(bg, 0);
#endif
    if (gDispCnt & DISPCNT_MODE_MASK) {
        if (bg == 2 || bg == 3) {
            return;
        }
    }

    EnableBg(bg);
    gBgWork->entries[bg].map = src;
    gBgWork->entries[bg].width = w;
    gBgWork->entries[bg].height = h;
    gBgWork->entries[bg].x = 0;
    gBgWork->entries[bg].y = 0;
    gBgWork->entries[bg].dirty = TRUE;
}

void RedrawBgMapAt(s32 bg, u16 x, u16 y) {
    BgEntry* entry = &gBgWork->entries[bg];

    if (entry->map == NULL) {
        return;
    }

    entry->x = x;
    entry->y = y;
    CopyBgMapRect(x, y, entry, (void*)(((*sBgControl[bg] & BGCNT_SCREENBASE_MASK) << 3) + VRAM), 0, 0, 0x1F, 0x15);
    SetBgScroll(bg, x & 7, y & 7);
    entry->dirty = FALSE;
}

void ScrollBgMapTo(s32 bg, u16 x, u16 y) {
    BgEntry* entry;
    s8 dx;
    s8 dy;
    u32 sx;
    u32 sy;
    u8 tx;
    u8 ty;
    u8 cx;
    u8 cy;
    void* dst;

    entry = &gBgWork->entries[bg];

    if (entry->map == NULL) {
        return;
    }

    if (entry->dirty) {
        RedrawBgMapAt(bg, x, y);
        return;
    }

    dx = (x >> 3) - (entry->x >> 3);
    dy = (y >> 3) - (entry->y >> 3);

    if (abs(dx) > 29 || abs(dy) > 19) {
        RedrawBgMapAt(bg, x, y);
        return;
    }

    sx = GetBgScrollX(bg);
    sy = GetBgScrollY(bg);
    SetBgScroll(bg, (u16)(sx + (x - entry->x)), (u16)(sy + (y - entry->y)));

    if (dx == 0 && dy == 0) {
        entry->x = x;
        entry->y = y;
        return;
    }

    dst = (void*)(((*sBgControl[bg] & BGCNT_SCREENBASE_MASK) << 3) + VRAM);
    tx = sx >> 3;
    ty = sy >> 3;
    cx = GetBgScrollX(bg) >> 3;
    cy = GetBgScrollY(bg) >> 3;

    if (dx > 0) {
        if (dx > 31) {
            dx = 31;
        }

        CopyBgMapRect(entry->x + 248, y, entry, dst, tx + 31, cy, dx, 21);
    } else if (dx < 0) {
        dx = -dx;

        if (dx > 31) {
            dx = 31;
        }

        CopyBgMapRect(entry->x - (dx << 3), y, entry, dst, tx - dx, cy, dx, 21);
    }

    if (dy > 0) {
        if (dy > 21) {
            dy = 21;
        }

        CopyBgMapRect(x, entry->y + 168, entry, dst, cx, ty + 21, 31, dy);
    } else if (dy < 0) {
        dy = -dy;

        if (dy > 21) {
            dy = 21;
        }

        CopyBgMapRect(x, entry->y - (dy << 3), entry, dst, cx, ty - dy, 31, dy);
    }

    entry->x = x;
    entry->y = y;
}

u16 GetBgMapX(s32 bg) {
    BgEntry* entry = &gBgWork->entries[bg];

    if (entry->map == NULL) {
        return 0;
    }

    return entry->x;
}

u16 GetBgMapY(s32 bg) {
    BgEntry* entry = &gBgWork->entries[bg];

    if (entry->map == NULL) {
        return 0;
    }

    return entry->y;
}

void SetBgMosaic(s32 bg, u8 on) {
    if (on) {
        *sBgControl[bg] |= BGCNT_MOSAIC;
    } else {
        *sBgControl[bg] &= ~BGCNT_MOSAIC;
    }
}

void SetBgMosaicSize(u8 x, u8 y) {
    x &= 0xF;
    y &= 0xF;
    gMosaic = (gMosaic & 0xFF00) | x | (y << 4);
}

void SetBgScroll(s32 bg, s32 x, s32 y) {
    x &= 0x1FF;
    y &= 0x1FF;

    switch ((u32)bg) {
    case 0:
        gBg0HOfs = x;
        gBg0VOfs = y;
        break;
    case 1:
        gBg1HOfs = x;
        gBg1VOfs = y;
        break;
    case 2:
        gBg2HOfs = x;
        gBg2VOfs = y;
        break;
    case 3:
        gBg3HOfs = x;
        gBg3VOfs = y;
        break;
    }
}

u8 GetBgScrollX(u32 bg) {
    u16 scroll;

    switch (bg) {
    case 0:
        scroll = gBg0HOfs;
        break;
    case 1:
        scroll = gBg1HOfs;
        break;
    case 2:
        scroll = gBg2HOfs;
        break;
    case 3:
        scroll = gBg3HOfs;
        break;
    default:
        return 0;
    }

    return scroll;
}

u8 GetBgScrollY(u32 bg) {
    u16 scroll;

    switch (bg) {
    case 0:
        scroll = gBg0VOfs;
        break;
    case 1:
        scroll = gBg1VOfs;
        break;
    case 2:
        scroll = gBg2VOfs;
        break;
    case 3:
        scroll = gBg3VOfs;
        break;
    default:
        return 0;
    }

    return scroll;
}

void SetBgPriority(s32 bg, u16 priority) {
    vu16* control = sBgControl[bg];

    *control &= ~BGCNT_PRIORITY_MASK;
    *control |= priority;
}

void SetBgSize(s32 bg, u16 size) {
    vu16* control = sBgControl[bg];

    *control &= ~BGCNT_SIZE_MASK;
    *control |= size;
}

void SetBgColorMode(s32 bg, u16 colorMode) {
    if (colorMode == BGCNT_256COLOR) {
        vu16* control = sBgControl[bg];

        *control &= 0xFFFF;
        *control |= BGCNT_256COLOR;
    } else {
        vu16* control = sBgControl[bg];

        *control &= ~BGCNT_256COLOR;
        *control |= 0;
    }
}

void SetBgOverflow(s32 bg, u8 on) {
    if (on) {
        *sBgControl[bg] |= BGCNT_WRAP;
    } else {
        *sBgControl[bg] &= ~BGCNT_WRAP;
    }
}

void SetBgAffine(s32 bg, u8 rot, s32 sx, s32 sy, s32 dx, s32 dy) {
    BgAffineSrcData src;
    BgAffineDstData dst;

    src.texX = 0;
    src.texY = 0;
    src.scrX = DISPLAY_WIDTH / 2;
    src.scrY = DISPLAY_HEIGHT / 2;
    src.sx = 0x10000 / sx;
    src.sy = 0x10000 / sy;
    src.alpha = -rot << 8;
    BgAffineSet(&src, &dst, 1);

    switch (bg) {
    case 2:
        gBg2PA = dst.pa;
        gBg2PB = dst.pb;
        gBg2PC = dst.pc;
        gBg2PD = dst.pd;
        gBg2X = dst.dx + dx;
        gBg2Y = dst.dy + dy;
        break;
    case 3:
        gBg3PA = dst.pa;
        gBg3PB = dst.pb;
        gBg3PC = dst.pc;
        gBg3PD = dst.pd;
        gBg3X = dst.dx + dx;
        gBg3Y = dst.dy + dy;
        break;
    }
}

void SetBackdropColor(u32 r, u32 g, u32 b) {
    u8 red = r;
    u8 green = g;
    u8 blue = b;

    green &= 0x1F;
    blue &= 0x1F;
    gLastBackdropColor = (blue << 10) | (green << 5) | (red & 0x1F);
    gBackdropColor = gLastBackdropColor;
}

void SetBgBlend(s32 bg, u16 target2, u16 target1) {
    switch ((u32)bg) {
    case 0:
        gBldCnt = (BLDCNT_TGT1_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
        break;
    case 1:
        gBldCnt = (BLDCNT_TGT1_BG1 | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
        break;
    case 2:
        gBldCnt = (BLDCNT_TGT1_BG2 | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
        break;
    default:
        gBldCnt = (BLDCNT_TGT1_BG3 | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_OBJ);
        break;
    }

    gBldCnt |= BLDCNT_EFFECT_BLEND;
    gBldAlpha = (target2 << 8) | target1;
}

void SetBlendAlpha(u16 target2, u16 target1) {
    gBldAlpha = (target2 << 8) | target1;
}

#ifdef VERSION_EU
void LoadBgTilesLz77(s32 bg, void* src) {
    EnableBg(bg);
    RequestLz77UnCompVram(src, GetBgCharBase(bg));
}

void LoadBgMapLz77(s32 bg, void* src) {
    EnableBg(bg);
    RequestLz77UnCompVram(src, GetBgScreenBase(bg));
}

u32 Lz77GetUncompSize(u32* src) {
    return *src >> 8;
}

u8 SetBgMapBlocksLz77(s32 bg, const void* src, u8 w, u8 h) {
    BgEntry* entry;
    s32 count;
    s32 i;

#ifdef PLATFORM_PC
    PortSetBgIsoExtend(bg, 0);
#endif

    if ((gDispCnt & DISPCNT_MODE_MASK) != 0 && (bg == 2 || bg == 3)) {
        return FALSE;
    }

    entry = &gBgWork->entries[bg];

    if (entry->decompressedMap != NULL) {
        return FALSE;
    }

    count = w * h;
    entry->decompressedMap = EwramAlloc(count * sizeof(void*));

    if (entry->decompressedMap == NULL) {
        return FALSE;
    }

    for (i = 0; i < count; i++) {
        entry->decompressedMap[i] = EwramAlloc(Lz77GetUncompSize(((u32**)src)[i]));

        if (entry->decompressedMap[i] == NULL) {
            return FALSE;
        }

        LZ77UnCompWram(((u32**)src)[i], entry->decompressedMap[i]);
    }

    EnableBg(bg);
    entry->map = entry->decompressedMap;
    entry->width = w;
    entry->height = h;
    entry->x = 0;
    entry->y = 0;
    entry->dirty = TRUE;
    return TRUE;
}

void FreeBgDecompressedMap(s32 bg) {
    BgEntry* entry = &gBgWork->entries[bg];
    s32 count;
    s32 i;

    if (entry->decompressedMap != NULL) {
        count = entry->width * entry->height;

        for (i = 0; i < count; i++) {
            EwramFree(entry->decompressedMap[i]);
        }

        EwramFree(entry->decompressedMap);
        entry->decompressedMap = NULL;
    }
}
#endif

s16 GetAngleDiff(s32 target, s32 angle) {
    s32 to = target & 0xFF;
    s32 from = angle & 0xFF;
    s32 diff = to - from;

    if (diff <= -0x80) {
        return (to + 0x100) - from;
    }

    if (diff > 0x7F) {
        return (to + 0xFFFFFF00) - from;
    }

    return diff;
}

s32 GetAngleDiff16(s32 target, s32 angle) {
    s32 to = target & 0xFFFF;
    s32 from = angle & 0xFFFF;
    s32 diff = to - from;
    s32 wrap;

    if (diff <= -0x8000) {
        return (to + 0x10000) - from;
    }

    wrap = 0x10000;

    if (diff > 0x7FFF) {
        return (to - wrap) - from;
    }

    return diff;
}

void ApproachAngle(u16* value, u16 target, u16 shift) {
    s16 diff;
    u16 step;

    if (*value == target) {
        return;
    }

    diff = GetAngleDiff((s16)target, (s16)*value);

    if (diff == 0) {
        return;
    }

    step = diff >> shift;
    *value = step + *value;
}

void ApproachAngle16(u16* value, u16 target, u16 shift) {
    s32 diff = GetAngleDiff16(target, *value);

    if (diff != 0) {
        *value = (diff >> shift) + *value;
    }
}

void ApproachValue(s32* value, s32 target, u16 steps) {
    s32 cur;
    s32 delta;

    cur = *value;
    delta = target - cur;

    if (steps == 0) {
        steps = 1;
    }

    *value = cur + delta / steps;
}

s32 GetHalfStepDivisor(u16 steps) {
    steps >>= 1;

    if (steps == 0) {
        steps = 1;
    }

    return steps;
}

void ApproachValueHalfSteps(s32* value, s32 target, u16 steps) {
    s32 delta = target - *value;

    *value += delta / GetHalfStepDivisor(steps);
}

s32 Lerp8(s32 from, s32 to, s32 weight) {
    return (from * (0x100 - weight) >> 8) + (to * weight >> 8);
}

void AnimInit(AnimState* anim, void* anims, void* gfxTable) {
    anim->gfxTable = gfxTable;
    anim->anims = anims;
    anim->frames = NULL;
}

void AnimChangeWithTables(AnimState* anim, u16 animId, u16 flags, void* anims, void* gfxTable) {
    if (anim->gfxTable != gfxTable || anim->anims != anims || anim->animId != animId) {
        anim->gfxTable = gfxTable;
        anim->anims = anims;
        AnimStart(anim, animId, flags);
    }
}

void AnimStart(AnimState* anim, u16 animId, u16 flags) {
    AnimHeader* header = anim->anims[animId];

    anim->frameCount = header->frameCount;

    if (anim->frameCount == 0) {
        anim->frames = NULL;
        return;
    }

    anim->frames = header->frames;

    if ((flags & ANIM_FLAG_KEEP_FRAME) == 0) {
        anim->timer = 0;

        if (flags & ANIM_FLAG_RANDOM_START) {
            anim->frame = GetRandom() % anim->frameCount;
        } else {
            anim->frame = 0;
        }
    }

    anim->flags = flags;
    anim->animId = animId;
}

void AnimChange(AnimState* anim, u16 id, u16 flags) {
    AnimHeader* header;

    if (anim->animId == id) {
        return;
    }

    header = anim->anims[id];
    anim->frameCount = header->frameCount;

    if (anim->frameCount == 0) {
        anim->frames = NULL;
        return;
    }

    anim->frames = header->frames;

    if ((flags & ANIM_FLAG_KEEP_FRAME) == 0) {
        anim->timer = 0;

        if (flags & ANIM_FLAG_RANDOM_START) {
            anim->frame = GetRandom() % anim->frameCount;
        } else {
            anim->frame = 0;
        }
    }

    anim->flags = flags;
    anim->animId = id;
}

void* AnimUpdate(AnimState* anim) {
    void* gfx = AnimGetGfx(anim);
    AnimFrame* frames = anim->frames;
    u16 index;

    if (frames == NULL) {
        return NULL;
    }

    anim->timer++;
    index = anim->frame;

    if (anim->timer >= frames[index].duration) {
        anim->frame = index + 1;
        anim->timer = 0;

        if (anim->frame >= anim->frameCount) {
            if (anim->flags & ANIM_FLAG_LOOP) {
                anim->frame = 0;
            } else {
                anim->frame = index;
            }

            anim->flags |= ANIM_FLAG_FINISHED;
        }
    }

    return gfx;
}

u8 AnimIsFrameEnding(AnimState* anim) {
    if (anim->frames == NULL) {
        return FALSE;
    }

    if (!(anim->flags & ANIM_FLAG_LOOP)) {
        if (anim->flags & ANIM_FLAG_FINISHED) {
            return FALSE;
        }
    }

    if (anim->timer + 1 >= anim->frames[anim->frame].duration) {
        return TRUE;
    }

    return FALSE;
}

void* AnimGetGfx(AnimState* anim) {
    void* result;

    if (anim->frames != NULL) {
        result = anim->gfxTable[anim->frames[anim->frame].gfxIndex];
    } else {
        result = NULL;
    }

    return result;
}

u8 AnimIsFinished(AnimState* anim) {
    if (anim->flags & ANIM_FLAG_FINISHED) {
        return TRUE;
    }

    return FALSE;
}

u16 AnimGetId(AnimState* anim) {
    return anim->animId;
}

u16 AnimGetFrame(AnimState* anim) {
    return anim->frame;
}

u16 AnimGetGfxIndex(AnimState* anim) {
    return anim->frames[anim->frame].gfxIndex;
}

void AnimSetFrame(AnimState* anim, u16 frame) {
    if (frame < anim->frameCount) {
        anim->frame = frame;
        anim->timer = 0;
        anim->flags &= ~ANIM_FLAG_FINISHED;
    }
}

void AnimReset(AnimState* anim) {
    anim->frame = 0;
    anim->timer = 0;
    anim->flags &= ~ANIM_FLAG_FINISHED;
}

void FadeInit() {
    SetIwramHeapName(sFadeHeapName);
    gFadeWork = IwramAlloc(sizeof(FadeWork));
    CpuFill32(0, gFadeWork, sizeof(FadeWork));
}

void FadeFree() {
    IwramFree(gFadeWork);
}

void FadeReset() {
    CpuFill32(0, gFadeWork, sizeof(FadeWork));
}

void LoadPalette(const void* src, void* dst, u16 size) {
    PaletteSlot* base;
    s32 idx;
    s32 count;
    s32 i;

    base = gFadeWork->slots;
    count = size / 32;
    idx = ((s32)dst - PLTT) / 32;
    src = LoadPaletteWithEffect(src, dst, size);

    for (i = 0; i < count; i++) {
        // @bug Writes past the fade slots when dst is outside palette RAM.
        base[idx + i].src = (u8*)src + i * 32;
        base[idx + i].dst = (u8*)dst + i * 32;
        base[idx + i].dirty = TRUE;
    }
}

void FadeClearPaletteSlot(u16 slot) {
    PaletteSlot* entry = gFadeWork->slots;

    entry += slot;
    entry->src = NULL;
}

void FadeUpdate() {
    s32 i;
    s32 j;
    s32 changed;
    u16 amount;
    PaletteSlot* slot;
    u16* src;
    u16* dst;
    s16 r;
    s16 g;
    s16 b;
    s16 gray;
    s16 red;
    s16 green;
    s16 blue;
    u16 color;

    if (gFadeWork->target != 0 || gFadeWork->amount != 0) {
        changed = gFadeWork->amount != gFadeWork->lastAmount;
        amount = gFadeWork->amount >> 8;

        for (i = 0; i < 32; i++) {
            slot = &gFadeWork->slots[i];
            src = slot->src;

            if (src == NULL) {
                continue;
            }

            if (slot->excluded && (gFadeWork->flags & FADE_FLAG_LOCKED) == 0) {
                continue;
            }

            if (slot->dirty) {
                slot->dirty = FALSE;
            } else if (!changed) {
                continue;
            }

            dst = slot->buffer;

            for (j = 0; j < 16; j++) {
                color = *src++;
                r = color & 31;
                g = (color >> 5) & 31;
                b = (color >> 10) & 31;

                switch (gFadeWork->mode) {
                case FADE_MODE_BLACK:
                    r -= amount;
                    g -= amount;
                    b -= amount;

                    if (r < 0) r = 0;

                    if (g < 0) g = 0;

                    if (b < 0) b = 0;

                    break;
                case FADE_MODE_WHITE:
                    if (r < amount) r = amount;

                    if (g < amount) g = amount;

                    if (b < amount) b = amount;

                    break;
                case FADE_MODE_RED:
                    r += amount;
                    g -= amount;
                    b -= amount;

                    if (r > 31) r = 31;

                    if (g < 0) g = 0;

                    if (b < 0) b = 0;

                    break;
                case FADE_MODE_GREEN:
                    r -= amount;
                    g += amount;
                    b -= amount;

                    if (r < 0) r = 0;

                    if (g > 31) g = 31;

                    if (b < 0) b = 0;

                    break;
                case FADE_MODE_BLUE:
                    r -= amount;
                    g -= amount;
                    b += amount;

                    if (r < 0) r = 0;

                    if (g < 0) g = 0;

                    if (b > 31) b = 31;

                    break;
                case FADE_MODE_ADD_WHITE:
                    r += amount;
                    g += amount;
                    b += amount;

                    if (r > 31) r = 31;

                    if (g > 31) g = 31;

                    if (b > 31) b = 31;

                    break;
                case FADE_MODE_GRAY:
                    gray = ((r + g + b) >> 2) * amount;
                    r = (gray + r * (31 - amount)) >> 5;
                    g = (gray + g * (31 - amount)) >> 5;
                    b = (gray + b * (31 - amount)) >> 5;
                    break;
                case FADE_MODE_WHITE_BLEND:
                    red = (31 - r) * amount;
                    green = (31 - g) * amount;
                    blue = (31 - b) * amount;
                    r = (red + r * (31 - amount)) / 31;
                    g = (green + g * (31 - amount)) / 31;
                    b = (blue + b * (31 - amount)) / 31;
                    break;
                case FADE_MODE_CONTRAST:
                    gray = 31 * amount;

                    if ((r + g + b) / 3 > 12) {
                        r = (gray + r * (31 - amount)) / 31;
                        g = (gray + g * (31 - amount)) / 31;
                        b = (gray + b * (31 - amount)) / 31;
                    } else {
                        r = r * (31 - amount) / 31;
                        g = g * (31 - amount) / 31;
                        b = b * (31 - amount) / 31;
                    }

                    break;
                case FADE_MODE_DARK_MAGENTA:
                    r -= amount;
                    g -= amount * 2;
                    b -= amount;

                    if (r < 0) r = 0;

                    if (g < 0) g = 0;

                    if (b < 0) b = 0;

                    break;
                case FADE_MODE_DARK_RED:
                    r -= amount;
                    g -= amount * 2;
                    b -= amount * 2;

                    if (r < 0) r = 0;

                    if (g < 0) g = 0;

                    if (b < 0) b = 0;

                    break;
                }

                *dst++ = b * 1024 | g * 32 | r;
            }

            RequestDma3Copy(slot->buffer, slot->dst, sizeof(slot->buffer));
        }
    }

    gFadeWork->lastAmount = gFadeWork->amount;

    if (gFadeWork->timer != 0) {
        if ((gFadeWork->flags & FADE_FLAG_PAUSED) == 0) {
            ApproachValue((s32*)&gFadeWork->amount, gFadeWork->target, gFadeWork->timer);
            gFadeWork->timer--;
        }

        if (gFadeWork->timer == 0 && gFadeWork->amount == 0) {
            for (i = 0; i < 32; i++) {
                slot = &gFadeWork->slots[i];
                RequestDma3Copy(slot->src, slot->dst, 32);
            }
        }
    } else {
        gFadeWork->flags = 0;
    }
}

void FadeStartIn(s32 mode, u16 frames) {
    FadeWork* base = gFadeWork;
    u32 z;

    if (base->flags & FADE_FLAG_LOCKED) {
        if (base->flags & FADE_FLAG_ACTIVE) {
            return;
        }
    }

    z = 0;
    base->flags = FADE_FLAG_ACTIVE;
    base->timer = frames;
    base->amount = 0x1F00;
    base->target = z;
    base->lastAmount = z;
    base->mode = mode;
}

void FadeStartOut(s32 mode, u16 frames) {
    FadeWork* base = gFadeWork;
    u32 z;

    if (base->flags & FADE_FLAG_LOCKED) {
        if (base->flags & FADE_FLAG_ACTIVE) {
            return;
        }
    }

    z = 0;
    base->flags = FADE_FLAG_ACTIVE;
    base->timer = frames;
    base->amount = z;
    base->target = 0x1F00;
    base->lastAmount = z;
    base->mode = mode;
}

void FadeToOriginal(s32 mode, u16 frames) {
    FadeWork* base = gFadeWork;
    u32 z;

    if (base->flags & FADE_FLAG_LOCKED) {
        if (base->flags & FADE_FLAG_ACTIVE) {
            return;
        }
    }

    z = 0;
    base->flags = FADE_FLAG_ACTIVE;
    base->timer = frames;
    base->target = z;
    base->mode = mode;
}

void FadeToAmount(s32 mode, u16 amount, u16 frames) {
    FadeWork* base = gFadeWork;

    if (base->flags & FADE_FLAG_LOCKED) {
        if (base->flags & FADE_FLAG_ACTIVE) {
            return;
        }
    }

    base->flags = FADE_FLAG_ACTIVE;
    base->timer = frames;
    base->target = amount << 8;
    base->mode = mode;
}

void FadeFromAmount(s32 mode, u16 amount, u16 frames) {
    FadeWork* base = gFadeWork;
    u32 z;

    if (base->flags & FADE_FLAG_LOCKED) {
        if (base->flags & FADE_FLAG_ACTIVE) {
            return;
        }
    }

    z = 0;
    base->flags = FADE_FLAG_ACTIVE;
    base->timer = frames;
    base->amount = amount << 8;
    base->lastAmount = z;
    base->target = z;
    base->mode = mode;
}

void FadeSetPaletteExcluded(u16 slot, u8 value) {
    PaletteSlot* entry;

    if (slot > 0x1F) {
        return;
    }

    entry = gFadeWork->slots;
    entry += slot;
    entry->excluded = value;
}

u8 FadeIsActive() {
    if (gFadeWork->flags & FADE_FLAG_ACTIVE) {
        return TRUE;
    }

    return FALSE;
}

u16 FadeGetColor() {
    switch (gFadeWork->mode) {
    case FADE_MODE_WHITE:
    case FADE_MODE_ADD_WHITE:
        return 0x7FFF;
    case FADE_MODE_RED:
        return 0x1F;
    case FADE_MODE_BLUE:
        return 0x7C00;
    case FADE_MODE_GREEN:
        return 0x3E0;
    case FADE_MODE_BLACK:
    default:
        return 0;
    }
}

u16 FadeGetAmount() {
    return gFadeWork->amount >> 8;
}

void FadeLock() {
    u16 flags = gFadeWork->flags | FADE_FLAG_LOCKED;

    gFadeWork->flags = flags;
}

void FadeSetPaused(u8 on) {
    if (on) {
        u16 flags = gFadeWork->flags | FADE_FLAG_PAUSED;

        gFadeWork->flags = flags;
    } else {
        gFadeWork->flags &= ~FADE_FLAG_PAUSED;
    }
}

void MosaicReset() {
    sMosaicSize = 0;
    sMosaicTarget = 0;
    sMosaicTimer = 0;
    sMosaicActive = FALSE;
}

void MosaicUpdate() {
    s16 amount;
    u8 size;

    if (sMosaicTimer != 0) {
        ApproachValue((s32*)&sMosaicSize, sMosaicTarget, sMosaicTimer--);
        amount = sMosaicSize >> 8;
        size = amount;
        SetBgMosaicSize(size, size);
        SetObjMosaicSize(size, size);
    } else if (sMosaicActive) {
        sMosaicActive = FALSE;
        SetSpriteMosaicEnabled(FALSE);
    }
}

void MosaicStartIn(u16 frames, u16 size) {
    sMosaicTimer = frames;
    sMosaicSize = size << 8;
    sMosaicTarget = 0;
    sMosaicActive = TRUE;
    SetBgMosaic(0, TRUE);
    SetBgMosaic(1, TRUE);
    SetBgMosaic(2, TRUE);
    SetBgMosaic(3, TRUE);
    SetSpriteMosaicEnabled(TRUE);
}

void MosaicStartOut(u16 frames, u16 size) {
    sMosaicTimer = frames;
    sMosaicSize = 0;
    sMosaicTarget = size << 8;
    sMosaicActive = TRUE;
    SetBgMosaic(0, TRUE);
    SetBgMosaic(1, TRUE);
    SetBgMosaic(2, TRUE);
    SetBgMosaic(3, TRUE);
    SetSpriteMosaicEnabled(TRUE);
}

u8 MosaicIsActive() {
    return sMosaicActive;
}

Dma3Queue* gDma3Requests IWRAM_COMMON(4);
