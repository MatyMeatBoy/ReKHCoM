/**
 * mode_ms.c
 * Moogle Card Pack Shop
 */

#include "registration_data.h"
#include "system_state.h"
#include "map_api.h"
#include "display.h"
#include "text.h"
#include "monsgage.h"
#include "mode_ms.h"
#include "gba/keys.h"
#include "sprites_moogle_shop.h"
#include "sprites_card_pictures.h"
#include "card_ids.h"
#include "ms_types.h"
#include "mode_ms_top_api.h"
#include "malloc.h"
#include "fade.h"
#include "player_progression.h"
#include "songs.h"
#include "anim.h"
#include "card.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "engine_math.h"
#include "game_state.h"
#include "gba/defines.h"
#include "gba/macro.h"
#include "key.h"
#include "m4a_song.h"
#include "map_types.h"
#include "mode.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "text_types.h"
#include "types.h"
#include "sprite_palettes.h"
#include "macros.h"

#ifdef VERSION_EU
static void* sMoogleShopBgMapsByLanguage[5] = {
    gMoogleShopBgMap,
    gMoogleShopBgFrenchMap,
    gMoogleShopBgGermanMap,
    gMoogleShopBgItalianMap,
    gMoogleShopBgSpanishMap,
};

static u16* sMooglePackAttackMagicMapsByLanguage[5] = {
    gMooglePackAttackMagicMap,
    gMooglePackAttackMagicMap,
    gMooglePackAttackMagicGermanMap,
    gMooglePackAttackMagicItalianMap,
    gMooglePackAttackMagicSpanishMap,
};

static u16* sMooglePackItemMixedMapsByLanguage[5] = {
    gMooglePackItemMixedMap,
    gMooglePackItemMixedMap,
    gMooglePackItemMixedGermanMap,
    gMooglePackItemMixedItalianMap,
    gMooglePackItemMixedSpanishMap,
};
#endif

static const MooglePackMenuEntry sMooglePackMenuEntries[4] = {
#ifdef VERSION_EU
    {MOOGLE_PACK_ENTRY_NONE, 2, MOOGLE_PACK_ENTRY_ROW_LIST, 1, 67, 16, gMooglePackSelectionTopLeftMap, sizeof(gMooglePackSelectionTopLeftMap), 5, 3, 40, 24, {{sMooglePackAttackMagicMapsByLanguage, 0, 0}, {sMooglePackAttackMagicMapsByLanguage, 0, 16}, {sMooglePackItemMixedMapsByLanguage, 0, 0}, {sMooglePackItemMixedMapsByLanguage, 0, 16}}},
    {MOOGLE_PACK_ENTRY_NONE, 3, 0, MOOGLE_PACK_ENTRY_NONE, 163, 16, gMooglePackSelectionTopRightMap, sizeof(gMooglePackSelectionTopRightMap), 17, 3, 136, 24, {{sMooglePackAttackMagicMapsByLanguage, 12, 0}, {sMooglePackAttackMagicMapsByLanguage, 12, 16}, {sMooglePackItemMixedMapsByLanguage, 12, 0}, {sMooglePackItemMixedMapsByLanguage, 12, 16}}},
    {0, MOOGLE_PACK_ENTRY_NONE, MOOGLE_PACK_ENTRY_ROW_LIST, 3, 67, 80, gMooglePackSelectionBottomLeftMap, sizeof(gMooglePackSelectionBottomLeftMap), 5, 11, 40, 88, {{sMooglePackAttackMagicMapsByLanguage, 0, 8}, {sMooglePackAttackMagicMapsByLanguage, 0, 24}, {sMooglePackItemMixedMapsByLanguage, 0, 8}, {sMooglePackItemMixedMapsByLanguage, 0, 24}}},
    {1, MOOGLE_PACK_ENTRY_NONE, 2, MOOGLE_PACK_ENTRY_NONE, 163, 80, gMooglePackSelectionBottomRightMap, sizeof(gMooglePackSelectionBottomRightMap), 17, 11, 136, 88, {{sMooglePackAttackMagicMapsByLanguage, 12, 8}, {sMooglePackAttackMagicMapsByLanguage, 12, 24}, {sMooglePackItemMixedMapsByLanguage, 12, 8}, {sMooglePackItemMixedMapsByLanguage, 12, 24}}},
#else
    {MOOGLE_PACK_ENTRY_NONE, 2, MOOGLE_PACK_ENTRY_ROW_LIST, 1, 67, 16, gMooglePackSelectionTopLeftMap, sizeof(gMooglePackSelectionTopLeftMap), 5, 3, 40, 24, {{gMooglePackAttackMagicMap, 0, 0}, {gMooglePackAttackMagicMap, 0, 16}, {gMooglePackItemMixedMap, 0, 0}, {gMooglePackItemMixedMap, 0, 16}}},
    {MOOGLE_PACK_ENTRY_NONE, 3, 0, MOOGLE_PACK_ENTRY_NONE, 163, 16, gMooglePackSelectionTopRightMap, sizeof(gMooglePackSelectionTopRightMap), 17, 3, 136, 24, {{gMooglePackAttackMagicMap, 12, 0}, {gMooglePackAttackMagicMap, 12, 16}, {gMooglePackItemMixedMap, 12, 0}, {gMooglePackItemMixedMap, 12, 16}}},
    {0, MOOGLE_PACK_ENTRY_NONE, MOOGLE_PACK_ENTRY_ROW_LIST, 3, 67, 80, gMooglePackSelectionBottomLeftMap, sizeof(gMooglePackSelectionBottomLeftMap), 5, 11, 40, 88, {{gMooglePackAttackMagicMap, 0, 8}, {gMooglePackAttackMagicMap, 0, 24}, {gMooglePackItemMixedMap, 0, 8}, {gMooglePackItemMixedMap, 0, 24}}},
    {1, MOOGLE_PACK_ENTRY_NONE, 2, MOOGLE_PACK_ENTRY_NONE, 163, 80, gMooglePackSelectionBottomRightMap, sizeof(gMooglePackSelectionBottomRightMap), 17, 11, 136, 88, {{gMooglePackAttackMagicMap, 12, 8}, {gMooglePackAttackMagicMap, 12, 24}, {gMooglePackItemMixedMap, 12, 8}, {gMooglePackItemMixedMap, 12, 24}}},
#endif
};

static const MooglePackSpriteDef sMooglePackSpriteDefs[4] = {
    {gMooglePackTier0Palette, sizeof(gMooglePackTier0Palette), gMooglePackTier0Tiles, sizeof(gMooglePackTier0Tiles), gMooglePackTier0Frame0, 10, 12},
    {gMooglePackTier1Palette, sizeof(gMooglePackTier1Palette), gMooglePackTier1Tiles, sizeof(gMooglePackTier1Tiles), gMooglePackTier1Frame0, 12, 13},
    {gMooglePackTier2Palette, sizeof(gMooglePackTier2Palette), gMooglePackTier2Tiles, sizeof(gMooglePackTier2Tiles), gMooglePackTier2Frame0, 12, 13},
    {gMooglePackTier3Palette, sizeof(gMooglePackTier3Palette), gMooglePackTier3Tiles, sizeof(gMooglePackTier3Tiles), gMooglePackTier3Frame0, 9, 7},
};

static const u16 sMoogleCardValueWeights[10] = {
    5,
    15,
    15,
    16,
    14,
    10,
    10,
    6,
    5,
    4,
};

static const MooglePackCardDef sMoogleAttackPackCards[17] = {
    {0, 0, {0, 0, 0, 0}},
    {CARD_ID(CARD_THREE_WISHES, 0), 1, {20, 14, 4, 4}},
    {CARD_ID(CARD_PUMPKINHEAD, 0), 3, {20, 14, 4, 4}},
    {CARD_ID(CARD_WISHING_STAR, 0), 5, {20, 14, 4, 4}},
    {CARD_ID(CARD_LADY_LUCK, 0), 10, {20, 14, 4, 4}},
    {CARD_ID(CARD_OLYMPIA, 0), 8, {20, 14, 4, 4}},
    {CARD_ID(CARD_METAL_CHOCOBO, 0), 7, {0, 5, 10, 6}},
    {CARD_ID(CARD_CRABCLAW, 0), 2, {0, 5, 10, 6}},
    {CARD_ID(CARD_FAIRY_HARP, 0), 4, {0, 5, 10, 6}},
    {CARD_ID(CARD_LIONHEART, 0), 9, {0, 5, 10, 6}},
    {CARD_ID(CARD_SPELLBINDER, 0), 6, {0, 5, 10, 6}},
    {CARD_ID(CARD_DIVINE_ROSE, 0), 11, {0, 5, 10, 6}},
    {CARD_ID(CARD_OATHKEEPER, 0), 12, {0, 0, 5, 10}},
    {CARD_ID(CARD_OBLIVION, 0), 13, {0, 0, 5, 10}},
    {CARD_ID(CARD_DIAMOND_DUST, 0), 15, {0, 0, 5, 10}},
    {CARD_ID(CARD_ONE_WINGED_ANGEL, 0), 16, {0, 0, 5, 10}},
    {CARD_ID(CARD_ULTIMA_WEAPON, 0), 14, {0, 0, 0, 4}},
};

static const MooglePackCardDef sMoogleMagicPackCards[14] = {
    {CARD_ID(CARD_FIRE, 0), 17, {15, 10, 5, 5}},
    {CARD_ID(CARD_BLIZZARD, 0), 18, {15, 10, 5, 5}},
    {CARD_ID(CARD_THUNDER, 0), 19, {15, 10, 5, 5}},
    {CARD_ID(CARD_GRAVITY, 0), 21, {0, 5, 10, 5}},
    {CARD_ID(CARD_STOP, 0), 22, {0, 5, 10, 5}},
    {CARD_ID(CARD_AERO, 0), 23, {0, 5, 10, 5}},
    {CARD_ID(CARD_CURE, 0), 20, {10, 5, 0, 0}},
    {CARD_ID(CARD_SIMBA, 0), 24, {20, 5, 5, 5}},
    {CARD_ID(CARD_GENIE, 0), 25, {10, 20, 8, 10}},
    {CARD_ID(CARD_BAMBI, 0), 26, {0, 0, 8, 10}},
    {CARD_ID(CARD_DUMBO, 0), 27, {10, 20, 8, 10}},
    {CARD_ID(CARD_TINKER_BELL, 0), 28, {0, 0, 8, 10}},
    {CARD_ID(CARD_MUSHU, 0), 29, {0, 0, 8, 10}},
    {CARD_ID(CARD_CLOUD, 0), 30, {5, 5, 10, 15}},
};

static const MooglePackCardDef sMoogleItemPackCards[7] = {
    {CARD_ID(CARD_POTION, 0), 31, {50, 40, 0, 0}},
    {CARD_ID(CARD_ETHER, 0), 34, {50, 40, 20, 15}},
    {CARD_ID(CARD_HI_POTION, 0), 32, {0, 10, 25, 20}},
    {CARD_ID(CARD_MEGA_ETHER, 0), 35, {0, 10, 25, 20}},
    {CARD_ID(CARD_MEGA_POTION, 0), 33, {0, 0, 15, 20}},
    {CARD_ID(CARD_ELIXIR, 0), 36, {0, 0, 15, 15}},
    {CARD_ID(CARD_MEGALIXIR, 0), 37, {0, 0, 0, 10}},
};

static const MooglePackCardTable sMooglePackCardTables[3] = {
    {sMoogleAttackPackCards, ARRAY_COUNT(sMoogleAttackPackCards)},
    {sMoogleMagicPackCards, ARRAY_COUNT(sMoogleMagicPackCards)},
    {sMoogleItemPackCards, ARRAY_COUNT(sMoogleItemPackCards)},
};

static const s16 sMooglePackTiers[13][4][4] = {
    {{0, 0, -1, -1}, {0, 0, -1, -1}, {0, 0, -1, -1}, {0, 0, -1, -1}},
    {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}},
    {{0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}},
    {{0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}},
    {{0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}, {0, 1, 1, 2}},
    {{0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}},
    {{0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}},
    {{0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}},
};

static const s16 sMooglePackPrices[4][4] = {
    {100, 200, 300, 500},
    {200, 250, 270, 300},
    {150, 200, 300, 350},
    {150, 200, 300, 400},
};

enum MooglePackCardState {
    MOOGLE_PACK_CARD_STATE_WAIT,
    MOOGLE_PACK_CARD_STATE_DEAL,
    MOOGLE_PACK_CARD_STATE_SPIN,
    MOOGLE_PACK_CARD_STATE_FLIP,
    MOOGLE_PACK_CARD_STATE_RISE,
    MOOGLE_PACK_CARD_STATE_SPIN_PREMIUM,
    MOOGLE_PACK_CARD_STATE_FLIP_PREMIUM,
    MOOGLE_PACK_CARD_STATE_RISE_PREMIUM,
    MOOGLE_PACK_CARD_STATE_REVEALED,
    MOOGLE_PACK_CARD_STATE_BROWSE
};

enum MoogleShopState {
    MOOGLE_SHOP_STATE_FADE_IN,
    MOOGLE_SHOP_STATE_SOLD_OUT,
    MOOGLE_SHOP_STATE_SELECT_ROW,
    MOOGLE_SHOP_STATE_SELECT_PACK,
    MOOGLE_SHOP_STATE_OPEN_PACK,
    MOOGLE_SHOP_STATE_EXIT
};

static MooglePackCardWork sMooglePackCards[5];
static struct ObjPalette* sMooglePackCard00Palette;
static struct ObjTiles* sMooglePackValueTiles;
static struct ObjPalette* sMooglePackPremiumValuePalette;
static struct ObjTiles* sMooglePackPremiumValueTiles;
static struct ObjPalette* sMooglePackCategoryPalette;
static TextSlot* sMooglePackNameText;
static u8 sMooglePackNameTextCount;
static TextSlot* sMooglePackDescText;
static u8 sMooglePackDescTextCount;
static struct ObjTiles* sMooglePackPremiumTiles;
static AnimState sMooglePackPremiumAnim;
static TaskPool sMooglePackHosiTasks[5];
static s32 sMooglePackCursorX;
static s32 sMooglePackCursorY;
static struct ObjTiles* sMooglePackCursorTiles;
static struct ObjPalette* sMooglePackCursorPalette;
static AnimState sMooglePackCursorAnim;
static u8 sMoogleShopHasPacks;
static s16 sMoogleShopState;
static s16 sMoogleShopRowCursor;
static s16 sMoogleShopRowCategory[4];
static s16 sMoogleShopPackCursor;
static s16 sMoogleShopPacks[4][4][2];
static u16 sMooglePackCardIds[5];
static s16 sMooglePackCardCursor;
static u16 sMooglePackBoughtFlags[32];
static u16 sMoogleFreePackFlags[2];
static struct ObjTiles* sMoogleShopCursorTiles;
static struct ObjPalette* sMoogleShopCursorPalette;
static AnimState sMoogleShopCursorAnim;
static void* sMooglePackTiles[4];
static void* sMooglePackPalettes[4];
static void* sMooglePackSprites[4];
static u16* sMoogleShopTilemap;
static s32 sMoogleShopCursorX;
static s32 sMoogleShopCursorY;
static u8 sMoogleShopBackToTop;

void MoogleShopClearFlags() {
    s32 i;

    for (i = 0; i < 32; i++) {
        sMooglePackBoughtFlags[i] = 0;
    }

    for (i = 0; i < 2; i++) {
        sMoogleFreePackFlags[i] = 0;
    }
}

void MoogleShopSaveFlags(void* dst) {
    u16* buf = dst;
    s32 i;

    for (i = 0; i < 32; i++) {
        buf[i] = sMooglePackBoughtFlags[i];
    }

    for (i = 0; i < 2; i++) {
        buf[i + 32] = sMoogleFreePackFlags[i];
    }
}

void MoogleShopLoadFlags(void* src) {
    u16* buf = src;
    s32 i;

    for (i = 0; i < 32; i++) {
        sMooglePackBoughtFlags[i] = buf[i];
    }

    for (i = 0; i < 2; i++) {
        sMoogleFreePackFlags[i] = buf[i + 32];
    }
}

void SetMooglePackBought(u16 room, u16 category, u16 pack) {
    u16 bit;
    bit = room * 16 + category * 4 + pack;

    if (bit <= 0x1FF) {
        sMooglePackBoughtFlags[bit >> 4] |= 1 << (bit & 15);
    }
}

void ClearMooglePackBought(u16 room, u16 category, u16 pack) {
    u16 bit;
    bit = room * 16 + category * 4 + pack;

    if (bit <= 0x1FF) {
        sMooglePackBoughtFlags[bit >> 4] &= ~(1 << (bit & 15));
    }
}

u8 IsMooglePackBought(u16 room, u16 category, u16 pack) {
    u16 bit;
    bit = room * 16 + category * 4 + pack;

    if (bit <= 0x1FF) {
        return sMooglePackBoughtFlags[bit >> 4] >> (bit & 15) & 1;
    }

    return 0;
}

void SetMoogleFreePackFlag(u16 room) {
    if (room <= 31) {
        sMoogleFreePackFlags[room >> 4] |= 1 << (room & 15);
    }
}

void ClearMoogleFreePackFlag(u16 room) {
    if (room <= 31) {
        sMoogleFreePackFlags[room >> 4] &= ~(1 << (room & 15));
    }
}

u8 GetMoogleFreePackFlag(u16 room) {
    if (room <= 31) {
        return sMoogleFreePackFlags[room >> 4] >> (room & 15) & 1;
    }

    return 0;
}

void ClearMoogleRoomFlags() {
    s16 i;
    s16 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            ClearMooglePackBought(gMapFloorState.room, i, j);
        }
    }

    ClearMoogleFreePackFlag(gMapFloorState.room);
}

u8 BuildMooglePackList(s16 floor) {
    s16 i;
    s16 k;
    s16 packs;
    s16 rows;
    s16 tier;
    s32 hasPacks;

    for (i = 0; i < 4; i++) {
        sMoogleShopRowCategory[i] = -1;

        for (k = 0; k < 4; k++) {
            sMoogleShopPacks[i][k][0] = -1;
        }
    }

    rows = 0;

    for (i = 0; i < 4; i++) {
        packs = 0;

        for (k = 0; k < 4; k++) {
            tier = sMooglePackTiers[floor][i][k];

            if (IsMooglePackBought(gMapFloorState.room, i, k) == 0) {
                if (tier >= 0) {
                    sMoogleShopPacks[rows][packs][0] = k;
                    sMoogleShopPacks[rows][packs][1] = tier;
                    packs++;
                }
            }
        }

        if (packs > 0) {
            sMoogleShopRowCategory[rows] = i;
            rows++;
        }
    }

    hasPacks = FALSE;

    if (rows > 0) {
        hasPacks = TRUE;
    }

    return hasPacks;
}

void MoogleShopCopyTilemapRect(u16 w, s16 h, u16* src, s16 sx, s16 sy, u16* dst, s16 dx, s16 dy) {
    s16 i;
    s16 j;
    s16 n;

    n = w;
    src += sx + sy * 32;
    dst += dx + dy * 32;

    for (j = 0; j < h; j++) {
        for (i = 0; i < n; i++) {
            *dst++ = *src++;
        }

        src += 32 - n;
        dst += 32 - n;
    }
}

void DrawMoogleShopPacks(s16 row) {
    s32 j;

    DmaFill16(3, 0, sMoogleShopTilemap, 0x500);

    for (j = 0; j < 4; j++) {
        if (sMoogleShopPacks[row][j][0] >= 0) {
            LoadDecimalDigitTiles(sMooglePackPrices[sMoogleShopRowCategory[row]][sMoogleShopPacks[row][j][1]], gMooglePackPriceDigitTiles,
                (u8*)GetBgCharBase(2) + (j * 0xC0 + 0xC0), 0x40, 3);
            MoogleShopCopyTilemapRect(12, 8, LANGSTR(sMooglePackMenuEntries[j].packTilemaps[sMoogleShopRowCategory[row]].tilemap),
                sMooglePackMenuEntries[j].packTilemaps[sMoogleShopRowCategory[row]].srcX,
                sMooglePackMenuEntries[j].packTilemaps[sMoogleShopRowCategory[row]].srcY, sMoogleShopTilemap,
                sMooglePackMenuEntries[j].tilemapX, sMooglePackMenuEntries[j].tilemapY);
        }
    }

    LoadBgMap(2, sMoogleShopTilemap, 0x500);
}

s32 MoogleShopReadMenuKeys() {
    s32 keys;

    keys = GetKeysPressed() & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON);
    return keys | (GetKeysRepeat() & (DPAD_ANY | R_BUTTON | L_BUTTON));
}

void InitMooglePackOpening(s16 x, s16 y) {
    s16 i;
    u16 id;
    TextSlot** textPtr;

    for (i = 0; i < 5; i++) {
        id = sMooglePackCardIds[i];

        if (id & CARD_FLAG_PREMIUM) {
            sMooglePackCards[i].premium = TRUE;
        } else {
            sMooglePackCards[i].premium = FALSE;
        }

        sMooglePackCards[i].revealed = FALSE;
        id &= CARD_ID_MASK;
        sMooglePackCards[i].palette = LoadObjPalette(gCardDefs[id].palette, 0x20);
        FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCards[i].palette)->index + 0x10, TRUE);
        sMooglePackCards[i].tiles = LoadObjTiles(gCardDefs[id].tiles, 0x200);
        sMooglePackCards[i].gfx = gCardDefs[id].gfx;
        sMooglePackCards[i].palette2 = LoadObjPalette(gMooglePackCardSpinPalettes + gCardDefs[id].category * 16, 0x20);
        FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCards[i].palette2)->index + 0x10, TRUE);
        sMooglePackCards[i].tiles2 = LoadObjTiles(gMooglePackCardSpinTiles, sizeof(gMooglePackCardSpinTiles));
        sMooglePackCards[i].backSprite = NULL;
        AnimInit(&sMooglePackCards[i].anim, gMooglePackCardSpinAnims, gMooglePackCardSpinFrames);
        AnimStart(&sMooglePackCards[i].anim, 0, ANIM_FLAG_LOOP);
        sMooglePackCards[i].x = x << 8;
        sMooglePackCards[i].y = y << 8;
        sMooglePackCards[i].scale = 2;
        sMooglePackCards[i].flipAngle = 0;
        sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_WAIT;
        sMooglePackCards[i].timer = 0;
    }

    sMooglePackCard00Palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCard00Palette)->index + 0x10, TRUE);
    sMooglePackValueTiles = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    sMooglePackPremiumValuePalette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackPremiumValuePalette)->index + 0x10, TRUE);
    sMooglePackPremiumValueTiles = LoadObjTiles(gCardPremiumValueDigitTiles, sizeof(gCardPremiumValueDigitTiles));
    sMooglePackCursorPalette = LoadObjPalette(gMoogleShopCursorPalette, sizeof(gMoogleShopCursorPalette));
    sMooglePackCursorTiles = LoadObjTiles(gMoogleShopCursorTiles, sizeof(gMoogleShopCursorTiles));
    AnimInit(&sMooglePackCursorAnim, gMoogleShopCursorAnims, gMoogleShopCursorFrames);
    AnimStart(&sMooglePackCursorAnim, 0, ANIM_FLAG_LOOP);
    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCursorPalette)->index + 0x10, TRUE);
    sMooglePackCategoryPalette = LoadObjPalette(gMooglePackCategoryPalettes, 0x20);
    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCategoryPalette)->index + 0x10, TRUE);
    textPtr = &sMooglePackNameText;
    *textPtr = EwramAlloc(0x24 * sizeof(TextSlot));
    InitTextSlots(sMooglePackNameText, 0x24);
    textPtr = &sMooglePackDescText;
    *textPtr = EwramAlloc(0x5A * sizeof(TextSlot));
    InitTextSlots(sMooglePackDescText, 0x5A);
    sMooglePackPremiumTiles = LoadObjTiles(gCardPremiumTiles, sizeof(gCardPremiumTiles));
    AnimInit(&sMooglePackPremiumAnim, gCardPremiumAnims, gCardPremiumFrames);
    AnimStart(&sMooglePackPremiumAnim, 0, ANIM_FLAG_LOOP);

    for (i = 0; i < 5; i++) {
        TaskPoolInit(&sMooglePackHosiTasks[i], 8);
    }

    sMooglePackCards[0].timer = 15;
}

void ReleaseMooglePackOpening() {
    s16 i;

    for (i = 0; i < 5; i++) {
        FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCards[i].palette)->index + 0x10, FALSE);
        ReleaseObjPalette(sMooglePackCards[i].palette);
        ReleaseObjTiles(sMooglePackCards[i].tiles);
        FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCards[i].palette2)->index + 0x10, FALSE);
        ReleaseObjPalette(sMooglePackCards[i].palette2);
        ReleaseObjTiles(sMooglePackCards[i].tiles2);
    }

    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCursorPalette)->index + 0x10, FALSE);
    ReleaseObjPalette(sMooglePackCursorPalette);
    ReleaseObjTiles(sMooglePackCursorTiles);
    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCategoryPalette)->index + 0x10, FALSE);
    ReleaseObjPalette(sMooglePackCategoryPalette);
    FreeTextSlots(sMooglePackNameText, 0x24);
    EwramFree(sMooglePackNameText);
    FreeTextSlots(sMooglePackDescText, 0x5A);
    EwramFree(sMooglePackDescText);
    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCard00Palette)->index + 0x10, FALSE);
    ReleaseObjPalette(sMooglePackCard00Palette);
    ReleaseObjTiles(sMooglePackValueTiles);
    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackPremiumValuePalette)->index + 0x10, FALSE);
    ReleaseObjPalette(sMooglePackPremiumValuePalette);
    ReleaseObjTiles(sMooglePackPremiumValueTiles);
    ReleaseObjTiles(sMooglePackPremiumTiles);

    for (i = 0; i < 5; i++) {
        TaskPoolDestroy(&sMooglePackHosiTasks[i]);
    }
}

void DrawMooglePackOpening() {
    s16 i;
    ObjAffine* affine;
    void* obj;
    s32 scaleX;
    void* anim;

    anim = AnimUpdate(&sMooglePackPremiumAnim);

    for (i = 0; i < 5; i++) {
        if (!sMooglePackCards[i].revealed) {
            scaleX = sMooglePackCards[i].scale;
            affine = AllocObjAffine(0, scaleX, scaleX, FALSE);
            obj = AnimUpdate(&sMooglePackCards[i].anim);
        } else {
            scaleX = sMooglePackCards[i].scale * -COS(sMooglePackCards[i].flipAngle) >> 8;
            affine = AllocObjAffine(0, scaleX, sMooglePackCards[i].scale, FALSE);
            obj = sMooglePackCards[i].backSprite;
        }

        if (scaleX != 0) {
            DrawSprite(sMooglePackCards[i].x >> 8, sMooglePackCards[i].y >> 8, obj, sMooglePackCards[i].tiles2, sMooglePackCards[i].palette2, affine, 0, 0x50);

            if (sMooglePackCards[i].revealed) {
                DrawSprite(sMooglePackCards[i].x >> 8, sMooglePackCards[i].y >> 8, sMooglePackCards[i].gfx, sMooglePackCards[i].tiles, sMooglePackCards[i].palette, affine, 0, 0x58);

                if (sMooglePackCards[i].premium) {
                    DrawSprite(sMooglePackCards[i].x >> 8, sMooglePackCards[i].y >> 8, gCardPremiumValueDigitFrames[gCardDefs[sMooglePackCardIds[i] & CARD_ID_MASK].value], sMooglePackPremiumValueTiles, sMooglePackPremiumValuePalette, affine, 0, 0x48);
                    DrawSprite(sMooglePackCards[i].x >> 8, sMooglePackCards[i].y >> 8, anim, sMooglePackPremiumTiles, sMooglePackCard00Palette, affine, 0, 0x40);
                } else {
                    DrawSprite(sMooglePackCards[i].x >> 8, sMooglePackCards[i].y >> 8, gCardValueDigitFrames[gCardDefs[sMooglePackCardIds[i] & CARD_ID_MASK].value], sMooglePackValueTiles, sMooglePackCard00Palette, affine, 0, 0x48);
                }
            }
        }

        if (sMooglePackCards[i].state == MOOGLE_PACK_CARD_STATE_BROWSE) {
            DrawTextSlots(0x30, 0x63, sMooglePackNameText, sMooglePackCategoryPalette, 0, sMooglePackNameTextCount);
            DrawTextSlots(0x31, 0x72, sMooglePackDescText, sMooglePackCursorPalette, 0, sMooglePackDescTextCount);
            ApproachValueHalf(&sMooglePackCursorX, sMooglePackCards[sMooglePackCardCursor].x - 0x1000);
            ApproachValueHalf(&sMooglePackCursorY, sMooglePackCards[sMooglePackCardCursor].y - 0x2000);
            DrawSprite(sMooglePackCursorX >> 8, sMooglePackCursorY >> 8, AnimUpdate(&sMooglePackCursorAnim), sMooglePackCursorTiles, sMooglePackCursorPalette, NULL, 0, 0);
        }
    }

    for (i = 0; i < 5; i++) {
        TaskPoolDraw(&sMooglePackHosiTasks[i]);
    }
}

u8 UpdateMooglePackOpening(u16 freePack) {
    MsShopHosiArg premiumSparkle;
    MsShopHosiArg revealedSparkle;
    MsShopHosiArg browseSparkle;
    u8 result;
    s16 i;
    s16 j;
    s16 k;
    s16 old;
    u16 keys;
    u16 delta;
    s32 premiumTick;
    s32 revealedTick;
    s32 browseTick;

    result = TRUE;

    for (i = 0; i < 5; i++) {
        switch (sMooglePackCards[i].state) {
        case MOOGLE_PACK_CARD_STATE_WAIT:
            if (!FadeIsActive()) {
                if (sMooglePackCards[i].timer != 0) {
                    sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_DEAL;
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_DEAL:
            ApproachValue(&sMooglePackCards[i].x, i * 10240 + 0x2800, sMooglePackCards[i].timer);
            ApproachValue(&sMooglePackCards[i].y, 0x6400, sMooglePackCards[i].timer);
            ApproachValue(&sMooglePackCards[i].scale, Q_8_8(1), sMooglePackCards[i].timer);

            if (--sMooglePackCards[i].timer == 0) {
                if (sMooglePackCards[i].premium) {
                    sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_SPIN_PREMIUM;
                } else {
                    sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_SPIN;
                }

                if (i <= 3) {
                    sMooglePackCards[i + 1].timer = 15;
                } else {
                    for (j = 0; j < 5; j++) {
                        if (sMooglePackCards[j].state == MOOGLE_PACK_CARD_STATE_SPIN) {
                            sMooglePackCards[j].timer = 8;
                            break;
                        }
                    }

                    if (j == 5) {
                        sMooglePackCards[0].timer = 8;
                    }
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_SPIN:
            if (sMooglePackCards[i].timer != 0) {
                if (AnimGetFrame(&sMooglePackCards[i].anim) == 3 || AnimGetFrame(&sMooglePackCards[i].anim) == 8) {
                    ReleaseObjPalette(sMooglePackCards[i].palette2);
                    ReleaseObjTiles(sMooglePackCards[i].tiles2);
                    sMooglePackCards[i].palette2 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
                    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCards[i].palette2)->index + 0x10, TRUE);
                    sMooglePackCards[i].tiles2 = LoadObjTiles(gCardBacks[gCardDefs[sMooglePackCardIds[i] & CARD_ID_MASK].category].tiles, 0x300);
                    sMooglePackCards[i].backSprite = gCardBacks[gCardDefs[sMooglePackCardIds[i] & CARD_ID_MASK].category].gfx;
                    sMooglePackCards[i].flipAngle = 0x40;
                    sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_FLIP;
                    sMooglePackCards[i].revealed = TRUE;
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_FLIP:
            delta = 0x80 - sMooglePackCards[i].flipAngle;
            sMooglePackCards[i].flipAngle += delta / sMooglePackCards[i].timer;

            if (--sMooglePackCards[i].timer == 0) {
                sMooglePackCards[i].timer = 5;
                sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_RISE;
            }

            break;
        case MOOGLE_PACK_CARD_STATE_RISE:
            ApproachValue(&sMooglePackCards[i].y, 0x4600, sMooglePackCards[i].timer);

            if (--sMooglePackCards[i].timer == 0) {
                m4aSongNumStart(SONG_SYS_KAIHUKU);
                sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_REVEALED;

                for (j = 0; j < 5; j++) {
                    if (sMooglePackCards[j].state == MOOGLE_PACK_CARD_STATE_SPIN) {
                        sMooglePackCards[j].timer = 8;
                        break;
                    }
                }

                if (j == 5) {
                    for (j = 0; j < 5; j++) {
                        if (sMooglePackCards[j].state == MOOGLE_PACK_CARD_STATE_SPIN_PREMIUM) {
                            sMooglePackCards[j].timer = 8;
                            break;
                        }
                    }

                    if (j == 5) {
                        sMooglePackCards[0].timer = 60;
                    }
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_SPIN_PREMIUM:
            if (sMooglePackCards[i].timer != 0) {
                if (AnimGetFrame(&sMooglePackCards[i].anim) == 3 || AnimGetFrame(&sMooglePackCards[i].anim) == 8) {
                    ReleaseObjPalette(sMooglePackCards[i].palette2);
                    ReleaseObjTiles(sMooglePackCards[i].tiles2);
                    sMooglePackCards[i].palette2 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
                    FadeSetPaletteExcluded(BIOS_PTR(sMooglePackCards[i].palette2)->index + 0x10, TRUE);
                    sMooglePackCards[i].tiles2 = LoadObjTiles(gCardBacks[gCardDefs[sMooglePackCardIds[i] & CARD_ID_MASK].category].tiles, 0x300);
                    sMooglePackCards[i].backSprite = gCardBacks[gCardDefs[sMooglePackCardIds[i] & CARD_ID_MASK].category].gfx;
                    sMooglePackCards[i].flipAngle = 0x40;
                    sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_FLIP_PREMIUM;
                    sMooglePackCards[i].revealed = TRUE;
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_FLIP_PREMIUM:
            delta = 0x80 - sMooglePackCards[i].flipAngle;
            sMooglePackCards[i].flipAngle += delta / sMooglePackCards[i].timer;

            if (--sMooglePackCards[i].timer == 0) {
                sMooglePackCards[i].timer = 5;
                sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_RISE_PREMIUM;

                for (k = 0; k < 8; k++) {
                    premiumSparkle.x = sMooglePackCards[i].x >> 8;
                    premiumSparkle.y = sMooglePackCards[i].y >> 8;
                    premiumSparkle.palette = sMooglePackCategoryPalette;
                    premiumSparkle.angle = GetRandom() % 96 - 48;
                    premiumSparkle.speed = GetRandom() % 256 + 0x1C0;
                    TaskCreate(&sMooglePackHosiTasks[i], &gTaskDescMsShopHosi, &premiumSparkle);
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_RISE_PREMIUM:
            ApproachValue(&sMooglePackCards[i].y, 0x4600, sMooglePackCards[i].timer);
            premiumTick = gFrameCounter & 0x1F;

            if (premiumTick == 0) {
                premiumSparkle.x = (sMooglePackCards[i].x >> 8) + GetRandom() % 32 - 16;
                premiumSparkle.y = (sMooglePackCards[i].y >> 8) + GetRandom() % 32 - 16;
                premiumSparkle.palette = sMooglePackCategoryPalette;
                premiumSparkle.angle = 0x80;
                premiumSparkle.speed = premiumTick;
                TaskCreate(&sMooglePackHosiTasks[i], &gTaskDescMsShopHosi, &premiumSparkle);
            }

            if (--sMooglePackCards[i].timer == 0) {
                m4aSongNumStart(SONG_SYS_KAIHUKU);
                sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_REVEALED;

                for (j = 0; j < 5; j++) {
                    if (sMooglePackCards[j].state == MOOGLE_PACK_CARD_STATE_SPIN_PREMIUM) {
                        sMooglePackCards[j].timer = 8;
                        break;
                    }
                }

                if (j == 5) {
                    sMooglePackCards[0].timer = 60;
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_REVEALED:
            if (sMooglePackCards[i].premium) {
                revealedTick = gFrameCounter & 0x1F;

                if (revealedTick == 0) {
                    revealedSparkle.x = (sMooglePackCards[i].x >> 8) + GetRandom() % 32 - 16;
                    revealedSparkle.y = (sMooglePackCards[i].y >> 8) + GetRandom() % 32 - 16;
                    revealedSparkle.palette = sMooglePackCategoryPalette;
                    revealedSparkle.angle = 0x80;
                    revealedSparkle.speed = revealedTick;
                    TaskCreate(&sMooglePackHosiTasks[i], &gTaskDescMsShopHosi, &revealedSparkle);
                }
            }

            if (sMooglePackCards[i].timer != 0) {
                if (--sMooglePackCards[i].timer == 0) {
                    if (freePack & 1) {
                        SetupBg(3, 0, 31, 0);
                        SetBgScroll(3, 0, 0);
                        // @bug Reads past the end of gMoogleShopBgTiles.
                        RequestDma3Copy(gMoogleShopBgTiles + 0x5800, (u8*)GetBgCharBase(3) + 0x5800, 0x1400);
                        LoadBgMap(3, gMooglePackCardInfoMap, sizeof(gMooglePackCardInfoMap));
                    }

                    sMooglePackCardCursor = 0;
                    sMooglePackCursorX = sMooglePackCards[0].x - 0x1000;
                    sMooglePackCursorY = sMooglePackCards[0].y - 0x2000;
                    LoadPalette(gMooglePackCardInfoPalettes + gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].category * 16, (void*)(BG_PLTT + 13 * PLTT_SIZE_4BPP), 0x20);
                    sMooglePackNameTextCount = LoadTextSlots(LANGSEL(gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].name), sMooglePackNameText);
                    sMooglePackDescTextCount = LoadTextSlots((void*)LANGSTR(gCardKindDescriptions[gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].kind]), sMooglePackDescText);
                    LoadObjPaletteBank(BIOS_PTR(sMooglePackCategoryPalette)->index, gMooglePackCategoryPalettes + gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].category * 16);
                    EnableBg(3);
                    sMooglePackCards[i].state = MOOGLE_PACK_CARD_STATE_BROWSE;
                }
            }

            break;
        case MOOGLE_PACK_CARD_STATE_BROWSE:
            if (sMooglePackCards[i].premium) {
                browseTick = gFrameCounter & 0x1F;

                if (browseTick == 0) {
                    browseSparkle.x = (sMooglePackCards[i].x >> 8) + GetRandom() % 32 - 16;
                    browseSparkle.y = (sMooglePackCards[i].y >> 8) + GetRandom() % 32 - 16;
                    browseSparkle.palette = sMooglePackCategoryPalette;
                    browseSparkle.angle = 0x80;
                    browseSparkle.speed = browseTick;
                    TaskCreate(&sMooglePackHosiTasks[i], &gTaskDescMsShopHosi, &browseSparkle);
                }
            }

            keys = MoogleShopReadMenuKeys();
            old = sMooglePackCardCursor;

            if (keys & (A_BUTTON | B_BUTTON)) {
                m4aSongNumStart(SONG_SYS_CLOSE);
                result = FALSE;
            } else if (keys & DPAD_LEFT) {
                sMooglePackCardCursor--;

                if (sMooglePackCardCursor < 0) {
                    sMooglePackCardCursor = 4;
                }
            } else if (keys & DPAD_RIGHT) {
                sMooglePackCardCursor++;

                if (sMooglePackCardCursor > 4) {
                    sMooglePackCardCursor = 0;
                }
            }

            if (sMooglePackCardCursor != old) {
                LoadPalette(gMooglePackCardInfoPalettes + gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].category * 16, (void*)(BG_PLTT + 13 * PLTT_SIZE_4BPP), 0x20);
                sMooglePackNameTextCount = LoadTextSlots(LANGSEL(gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].name), sMooglePackNameText);
                sMooglePackDescTextCount = LoadTextSlots((void*)LANGSTR(gCardKindDescriptions[gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].kind]), sMooglePackDescText);
                LoadObjPaletteBank(BIOS_PTR(sMooglePackCategoryPalette)->index, gMooglePackCategoryPalettes + gCardDefs[sMooglePackCardIds[sMooglePackCardCursor] & CARD_ID_MASK].category * 16);
                m4aSongNumStart(SONG_SYS_CLICK);
            }

            break;
        }
    }

    for (i = 0; i < 5; i++) {
        TaskPoolUpdate(&sMooglePackHosiTasks[i]);
    }

    return result;
}

void DrawMoogleShopCategoryLabels(s16 row) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (sMoogleShopRowCategory[i] >= 0) {
            RequestTilemapRectCopy(gMoogleShopCategoryLabelsMap, GetBgScreenBase(0), sMoogleShopRowCategory[i] * 6, i == row ? 0 : 3, 0, i * 3 + 3, 6, 3);
        }
    }
}

void LoadMooglePackSelectionTilemap(s16 pack) {
    LoadBgMap(1, sMooglePackMenuEntries[pack].selectionTilemap, sMooglePackMenuEntries[pack].selectionTilemapSize);
}

void MoogleShopHandleSoldOutInput() {
    u16 keys;
    keys = MoogleShopReadMenuKeys();

    if ((keys & A_BUTTON) == 0) {
        if (keys & B_BUTTON) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            FadeStartOut(FADE_MODE_BLACK, 16);
            sMoogleShopBackToTop = TRUE;
            sMoogleShopState = MOOGLE_SHOP_STATE_EXIT;
        } else if (keys & START_BUTTON) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            FadeStartOut(FADE_MODE_BLACK, 16);
            sMoogleShopBackToTop = FALSE;
            sMoogleShopState = MOOGLE_SHOP_STATE_EXIT;
        }
    }
}

void MoogleShopHandleRowInput() {
    u16 keys;
    s16 old;
    s16 i;

    old = sMoogleShopRowCursor;
    keys = MoogleShopReadMenuKeys();

    if ((keys & A_BUTTON) == 0) {
        if (keys & B_BUTTON) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            FadeStartOut(FADE_MODE_BLACK, 16);
            sMoogleShopBackToTop = TRUE;
            sMoogleShopState = MOOGLE_SHOP_STATE_EXIT;
        } else if (keys & START_BUTTON) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            FadeStartOut(FADE_MODE_BLACK, 16);
            sMoogleShopBackToTop = FALSE;
            sMoogleShopState = MOOGLE_SHOP_STATE_EXIT;
        } else if (keys & DPAD_UP) {
            sMoogleShopRowCursor--;

            if (sMoogleShopRowCursor < 0) {
                i = 3;

                if (sMoogleShopRowCategory[i] < 0) {
                    do {
                        i--;

                        if (i <= 0) {
                            break;
                        }
                    } while (sMoogleShopRowCategory[i] < 0);
                }

                sMoogleShopRowCursor = i;
            }
        } else if (keys & DPAD_DOWN) {
            sMoogleShopRowCursor++;

            if (sMoogleShopRowCursor > 3) {
                sMoogleShopRowCursor = 0;
            } else if (sMoogleShopRowCategory[sMoogleShopRowCursor] < 0) {
                sMoogleShopRowCursor = 0;
            }
        } else if (keys & DPAD_RIGHT) {
            sMoogleShopState = MOOGLE_SHOP_STATE_SELECT_PACK;
            sMoogleShopPackCursor = 0;
            LoadMooglePackSelectionTilemap(0);
            m4aSongNumStart(SONG_SYS_CLICK);
        }
    }

    if (sMoogleShopRowCursor == -1) {
        sMoogleShopRowCursor = old;
    }

    if (sMoogleShopRowCursor != old) {
        DrawMoogleShopCategoryLabels(sMoogleShopRowCursor);
        DrawMoogleShopPacks(sMoogleShopRowCursor);
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

u16 RollMoogleCardValue() {
    s16 i;
    u16 roll;
    u16 acc;

    roll = GetRandom() % 100;
    i = 0;
    acc = sMoogleCardValueWeights[0];

    if (roll >= acc) {
        do {
            i++;

            if (i > 9) {
                break;
            }

            acc += sMoogleCardValueWeights[i];
        } while (roll >= acc);
    }

    return i % 10;
}

void RollMooglePackCards(s16 category, s16 tier) {
    s16 lo;
    s16 hi;
    s16 j;
    s16 m;
    s16 count;
    s16 capacity;
    u16 total;
    u16 rnd;
    u16 acc;
    u16 id;
    const MooglePackCardDef** list;
    const MooglePackCardDef* cards;
    s16 n;

    id = 0;

    if (category <= 2) {
        lo = category;
        hi = category + 1;
    } else {
        lo = 0;
        hi = 3;
    }

    total = 0;
    capacity = 0;

    for (j = lo; j < hi; j++) {
        capacity += sMooglePackCardTables[j].count;
    }

    list = EwramAlloc(capacity * sizeof(*list));
    count = 0;

    for (j = lo; j < hi; j++) {
        cards = sMooglePackCardTables[j].cards;
        n = sMooglePackCardTables[j].count;

        for (m = 0; m < n; m++) {
            if (cards[m].weights[tier] != 0) {
                total += cards[m].weights[tier];
                list[count] = &cards[m];
                count++;
            }
        }
    }

    for (j = 0; j < 5; j++) {
        rnd = GetRandom() % total;
        acc = 0;

        for (m = 0; m < count; m++) {
            acc += list[m]->weights[tier];

            if (rnd < acc) {
                if (IsCardKindObtained(list[m]->unlockFlag)) {
                    id = list[m]->cardId;
                } else {
                    switch (gCardDefs[list[m]->cardId].category) {
                    case 0:
                        id = CARD_ID(CARD_KINGDOM_KEY, 0);
                        break;
                    case 1:
                        id = CARD_ID(CARD_CURE, 0);
                        break;
                    case 2:
                        id = CARD_ID(CARD_POTION, 0);
                        break;
                    }
                }

                id += RollMoogleCardValue();

                if (gCardDefs[id].category <= CARD_CATEGORY_MAGIC) {
                    if (GetRandom() % 100 <= 9) {
                        id |= CARD_FLAG_PREMIUM;
                    }
                }

                sMooglePackCardIds[j] = id;

                if (ObtainCard(id) < 0) {
                    AddMooglePoints(GetCardMooglePointValue(id));
                }

                break;
            }
        }
    }

    EwramFree(list);
}

void MoogleShopHandlePackInput() {
    u16 keys;
    s16 old;

    old = sMoogleShopPackCursor;
    keys = MoogleShopReadMenuKeys();

    if (keys & A_BUTTON) {
        if (sMoogleShopPacks[sMoogleShopRowCursor][sMoogleShopPackCursor][0] >= 0 &&
            SpendMooglePoints(sMooglePackPrices[sMoogleShopRowCategory[sMoogleShopRowCursor]][sMoogleShopPacks[sMoogleShopRowCursor][sMoogleShopPackCursor][1]])) {
            RollMooglePackCards(sMoogleShopRowCategory[sMoogleShopRowCursor], sMoogleShopPacks[sMoogleShopRowCursor][sMoogleShopPackCursor][1]);
            InitMooglePackOpening(sMoogleShopPackCursor % 2 * 96 + 72, sMoogleShopPackCursor / 2 * 64 + 48);
            FadeSetPaletteExcluded(13, TRUE);
            FadeToAmount(FADE_MODE_BLACK, 16, 8);
            m4aSongNumStart(SONG_SYS_KETTEI);
            sMoogleShopState = MOOGLE_SHOP_STATE_OPEN_PACK;
        } else {
            m4aSongNumStart(SONG_SYS_BEEP);
        }
    } else if (keys & B_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        FadeStartOut(FADE_MODE_BLACK, 16);
        sMoogleShopBackToTop = TRUE;
        sMoogleShopState = MOOGLE_SHOP_STATE_EXIT;
    } else if (keys & START_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        FadeStartOut(FADE_MODE_BLACK, 16);
        sMoogleShopBackToTop = FALSE;
        sMoogleShopState = MOOGLE_SHOP_STATE_EXIT;
    } else if (keys & DPAD_UP) {
        sMoogleShopPackCursor = sMooglePackMenuEntries[sMoogleShopPackCursor].upEntry;
    } else if (keys & DPAD_DOWN) {
        sMoogleShopPackCursor = sMooglePackMenuEntries[sMoogleShopPackCursor].downEntry;
    } else if (keys & DPAD_LEFT) {
        sMoogleShopPackCursor = sMooglePackMenuEntries[sMoogleShopPackCursor].leftEntry;
    } else if (keys & DPAD_RIGHT) {
        sMoogleShopPackCursor = sMooglePackMenuEntries[sMoogleShopPackCursor].rightEntry;
    }

    if (sMoogleShopPackCursor == MOOGLE_PACK_ENTRY_NONE) {
        sMoogleShopPackCursor = old;
    } else if (sMoogleShopPackCursor == MOOGLE_PACK_ENTRY_ROW_LIST) {
        sMoogleShopPackCursor = old;
        sMoogleShopState = MOOGLE_SHOP_STATE_SELECT_ROW;
        DisableBg(1);
        m4aSongNumStart(SONG_SYS_CLICK);
    } else if (sMoogleShopPacks[sMoogleShopRowCursor][sMoogleShopPackCursor][0] < 0) {
        sMoogleShopPackCursor = old;
    }

    if (sMoogleShopPackCursor != old) {
        LoadMooglePackSelectionTilemap(sMoogleShopPackCursor);
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void MoogleShopDraw() {
    s32 i;
    s16 tier;

    switch (sMoogleShopState) {
    case MOOGLE_SHOP_STATE_SELECT_ROW:
        if (sMoogleShopHasPacks) {
            ApproachValueHalf(&sMoogleShopCursorX, 0x400);
            ApproachValueHalf(&sMoogleShopCursorY, sMoogleShopRowCursor * 6144 + 0x800);
            DrawSprite(sMoogleShopCursorX >> 8, sMoogleShopCursorY >> 8, AnimUpdate(&sMoogleShopCursorAnim), sMoogleShopCursorTiles, sMoogleShopCursorPalette, NULL, SPRITE_PRIORITY(1), 0x3E8);
        }

        break;
    case MOOGLE_SHOP_STATE_SELECT_PACK:
        ApproachValueHalf(&sMoogleShopCursorX, sMooglePackMenuEntries[sMoogleShopPackCursor].cursorX << 8);
        ApproachValueHalf(&sMoogleShopCursorY, sMooglePackMenuEntries[sMoogleShopPackCursor].cursorY << 8);
        DrawSprite(sMoogleShopCursorX >> 8, sMoogleShopCursorY >> 8, AnimUpdate(&sMoogleShopCursorAnim), sMoogleShopCursorTiles, sMoogleShopCursorPalette, NULL, SPRITE_PRIORITY(1), 0x3E8);
        break;
    case MOOGLE_SHOP_STATE_OPEN_PACK:
        DrawMooglePackOpening();
        break;
    }

    for (i = 0; i < 4; i++) {
        if (sMoogleShopPacks[sMoogleShopRowCursor][i][0] >= 0) {
            tier = sMoogleShopPacks[sMoogleShopRowCursor][i][1];
            DrawSprite(sMooglePackMenuEntries[i].spriteX + sMooglePackSpriteDefs[tier].xOffset, sMooglePackMenuEntries[i].spriteY + sMooglePackSpriteDefs[tier].yOffset, sMooglePackSprites[tier], sMooglePackTiles[tier], sMooglePackPalettes[tier], NULL, SPRITE_PRIORITY(1), 0x3F2);
        }
    }
}

void mode_ms_shop_0() {
    s16 i;
    s32 size;
    u16** tilemapPtr;

    tilemapPtr = &sMoogleShopTilemap;
    size = 0x500;
    *tilemapPtr = EwramAlloc(size);
    SpriteReset();
    FadeStartIn(FADE_MODE_BLACK, 16);
    SetBgMode0();
    SetupBg(0, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 0, 30, 0);
    SetupBg(3, 0, 31, 0);
    SetBgPriority(0, 3);
    SetBgPriority(1, 2);
    SetBgPriority(2, 1);
    SetBgPriority(3, 0);
    sMoogleShopState = MOOGLE_SHOP_STATE_FADE_IN;
    sMoogleShopRowCursor = 0;
    sMoogleShopPackCursor = 0;
    sMoogleShopHasPacks = BuildMooglePackList(gGameState.floor);
    LoadBgPalette(0, gMoogleShopBgPalette, sizeof(gMoogleShopBgPalette));
    LoadBgTiles(0, gMoogleShopBgTiles, sizeof(gMoogleShopBgTiles));
    LoadDecimalDigitTiles(GetMooglePoints(), gMoogleShopPointsDigitTiles, (u8*)GetBgCharBase(0) + 0x20, 0x20, 5);
#ifdef VERSION_EU
    LoadBgMap(0, sMoogleShopBgMapsByLanguage[gLanguage], size);
#else
    LoadBgMap(0, gMoogleShopBgMap, size);
#endif
    DrawMoogleShopCategoryLabels(sMoogleShopRowCursor);

    if (sMoogleShopHasPacks) {
        LoadMooglePackSelectionTilemap(sMoogleShopPackCursor);
    }

    DrawMoogleShopPacks(sMoogleShopRowCursor);
    LoadBgMap(3, gMooglePackCardInfoMap, size);
    sMoogleShopCursorPalette = LoadObjPalette(gMoogleShopCursorPalette, sizeof(gMoogleShopCursorPalette));
    sMoogleShopCursorTiles = LoadObjTiles(gMoogleShopCursorTiles, sizeof(gMoogleShopCursorTiles));
    AnimInit(&sMoogleShopCursorAnim, gMoogleShopCursorAnims, gMoogleShopCursorFrames);
    AnimStart(&sMoogleShopCursorAnim, 0, ANIM_FLAG_LOOP);

    for (i = 0; i < 4; i++) {
        sMooglePackPalettes[i] = LoadObjPalette(sMooglePackSpriteDefs[i].palette, sMooglePackSpriteDefs[i].paletteSize);
        sMooglePackTiles[i] = LoadObjTiles(sMooglePackSpriteDefs[i].tiles, sMooglePackSpriteDefs[i].tilesSize);
        sMooglePackSprites[i] = sMooglePackSpriteDefs[i].sprite;
    }

    EnableBg(0);
    DisableBg(1);
    EnableBg(2);
    DisableBg(3);
}

void mode_ms_shop_1() {
    UpdatePlayTime();

    switch (sMoogleShopState) {
    case MOOGLE_SHOP_STATE_FADE_IN:
        if (!FadeIsActive()) {
            if (sMoogleShopHasPacks) {
                sMoogleShopCursorX = 0x400;
                sMoogleShopCursorY = sMoogleShopRowCursor * 6144 + 0x800;
                sMoogleShopState = MOOGLE_SHOP_STATE_SELECT_ROW;
            } else {
                sMoogleShopState = MOOGLE_SHOP_STATE_SOLD_OUT;
            }
        }

        break;
    case MOOGLE_SHOP_STATE_SOLD_OUT:
        MoogleShopHandleSoldOutInput();
        break;
    case MOOGLE_SHOP_STATE_SELECT_ROW:
        MoogleShopHandleRowInput();
        break;
    case MOOGLE_SHOP_STATE_SELECT_PACK:
        MoogleShopHandlePackInput();
        break;
    case MOOGLE_SHOP_STATE_OPEN_PACK:
        if (!UpdateMooglePackOpening(0)) {
            ReleaseMooglePackOpening();
            SetMooglePackBought(gMapFloorState.room, sMoogleShopRowCategory[sMoogleShopRowCursor], sMoogleShopPacks[sMoogleShopRowCursor][sMoogleShopPackCursor][0]);
            sMoogleShopHasPacks = BuildMooglePackList(gGameState.floor);
#ifdef VERSION_EU
            LoadBgMap(0, sMoogleShopBgMapsByLanguage[gLanguage], 0x500);
#else
            LoadBgMap(0, gMoogleShopBgMap, sizeof(gMoogleShopBgMap));
#endif

            if (sMoogleShopRowCursor > 0) {
                if (sMoogleShopRowCategory[sMoogleShopRowCursor] < 0) {
                    do {
                        sMoogleShopRowCursor--;

                        if (sMoogleShopRowCursor <= 0) {
                            break;
                        }
                    } while (sMoogleShopRowCategory[sMoogleShopRowCursor] < 0);
                }
            }

            DrawMoogleShopCategoryLabels(sMoogleShopRowCursor);
            DrawMoogleShopPacks(sMoogleShopRowCursor);

            if (sMoogleShopHasPacks) {
                for (; sMoogleShopPackCursor > 0 && sMoogleShopPacks[sMoogleShopRowCursor][sMoogleShopPackCursor][0] < 0; sMoogleShopPackCursor--) {
                }

                LoadMooglePackSelectionTilemap(sMoogleShopPackCursor);
            } else {
                DisableBg(1);
            }

            LoadDecimalDigitTiles(GetMooglePoints(), gMoogleShopPointsDigitTiles, (u8*)GetBgCharBase(0) + 0x20, 0x20, 5);
            DisableBg(3);
            FadeToOriginal(FADE_MODE_BLACK, 8);
            sMoogleShopState = sMoogleShopHasPacks ? MOOGLE_SHOP_STATE_SELECT_PACK : MOOGLE_SHOP_STATE_SOLD_OUT;
        }

        break;
    case MOOGLE_SHOP_STATE_EXIT:
        if (!FadeIsActive()) {
            if (sMoogleShopBackToTop) {
                ModeRequest(&gModeMsTop, 2);
            } else {
                RequestMapMode();
            }
        }

        break;
    }

    MoogleShopDraw();
}

void mode_ms_shop_2() {
    s32 i;

    ReleaseObjPalette(sMoogleShopCursorPalette);
    ReleaseObjTiles(sMoogleShopCursorTiles);

    for (i = 0; i < 4; i++) {
        ReleaseObjPalette(sMooglePackPalettes[i]);
        ReleaseObjTiles(sMooglePackTiles[i]);
    }

    EwramFree(sMoogleShopTilemap);
}

Mode gModeMsShop = {
    "mode_ms_shop",
    (ModeInitFunc)mode_ms_shop_0,
    mode_ms_shop_1,
    mode_ms_shop_2,
};
