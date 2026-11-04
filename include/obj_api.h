#ifndef GUARD_OBJ_API_H
#define GUARD_OBJ_API_H

#include "obj.h"
#include "types.h"

struct ObjTiles;
struct ObjPalette;

struct ObjTiles* LoadObjTiles(const void* src, u16 size);
struct ObjTiles* AllocObjTiles(u16 size, const void* owner);
void ReleaseObjTiles(void* tiles);
struct ObjPalette* LoadObjPalette(const void* src, u16 size);
struct ObjPalette* AllocObjPalette(u16 size);
void ReleaseObjPalette(struct ObjPalette* palette);
u8 DrawSprite(s16 x, s16 y, void* sprite, void* tiles, void* palette, ObjAffine* affine, u16 flags, u16 priority);
#ifdef PLATFORM_PC
u8 DrawMenuHeader(s16 x, s16 y, void* sprite, void* tiles, void* palette, ObjAffine* affine, u16 flags, u16 priority);
u8 DrawMenuTab(s16 x, s16 y, void* sprite, void* tiles, void* palette, ObjAffine* affine, u16 flags, u16 priority);
#else
#define DrawMenuHeader DrawSprite
#define DrawMenuTab DrawSprite
#endif
void DrawSpriteUnsorted(s16 x, s16 y, void* sprite, void* tiles, void* palette, u16 flags);
ObjAffine* AllocObjAffine(u8 angle, s32 sx, s32 sy, u8 doubleSize);
ObjAffine* AllocObjAffineAngle(u8 angle, u8 doubleSize);
u8 CanAllocObjTiles(u16 count);
void SpriteReset();
void SetObjTileSource(struct ObjTiles* tiles, const void* src);
u16 GetMaxSpriteTileBytes(void** sprites, u16 count);
u16 GetSpriteTileBytes(u16* sprite);
struct ObjTiles* AllocSpriteFrameTiles(u16 size);
u8 UpdateSpriteFrameTiles(struct ObjTiles* tiles, u16* sprite, void* src);
/* PORT_TILES_OK(t): tiles a full pool did not give (NULL) are not copied to: the GBA DMAs from what its BIOS reads
   and draws nothing of it (DrawSprite skips NULL); the PC would read through NULL (OptiPort v2.8, issue #564). */
#ifdef PLATFORM_PC
#define PORT_TILES_OK(t) ((t) != NULL)
#else
#define PORT_TILES_OK(t) 1
#endif

void InitObjTilesAtSlot(struct ObjTiles* tiles, u16 slot, void* src, u16 size);
void InitObjPaletteAtSlot(struct ObjPalette* palette, u16 slot, void* src, u16 size);
void UpdateAllocatedObjPalette(struct ObjPalette* palette, void* src);
u8 CanAllocObjPalette(u16 count);
void UpdateSpriteOam();
u8 IsRectOutsideScreen(s16 x, s16 y, s32 topExtent, s32 bottomExtent, s32 leftExtent, s32 rightExtent);
void SetObjPaletteRange(u16 start, u16 count);
u16 GetObjTileCount(u16 attr0, u16 attr1);

#endif
