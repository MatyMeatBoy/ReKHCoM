/**
 * mode_worldwarp.c
 * World Warp Screen
 */

#include "gba/keys.h"
#include "key.h"
#include "monsgage.h"
#include "types.h"
#include "jiminy_data.h"
#include "system_state.h"
#include "map_api.h"
#include "mode_worldwarp.h"
#include "sprites_worldinspect.h"
#include "world_types.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include "common_text.h"
#include "anim.h"
#include "display.h"
#include "engine_math.h"
#include "game_state.h"
#include "jiminy_inline_text_data.h"
#include "m4a_song.h"
#include "map_runtime.h"
#include "mode.h"
#include "obj.h"
#include "obj_api.h"
#include "text.h"
#include "text_types.h"
#include "gba/macro.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "macros.h"
#ifdef PLATFORM_PC
#include "engine.h"
#include "port/port_hdsprites.h"
#endif

enum WorldWarpState {
    WORLD_WARP_STATE_BARS_IN,
    WORLD_WARP_STATE_TITLE_IN,
    WORLD_WARP_STATE_SELECT,
    WORLD_WARP_STATE_TITLE_OUT,
    WORLD_WARP_STATE_BARS_OUT,
    WORLD_WARP_STATE_EXIT
};

static s16 sWorldWarpCursor;
static s16 sWorldWarpFloorCount;
static s16 sWorldWarpFloorWorlds[13];
static s16 sWorldWarpTarget;
static void* sWorldWarpTilemap;
static struct ObjTiles* sWorldWarpBarTiles;
static struct ObjPalette* sWorldWarpBarPalette;
static struct ObjTiles* sWorldWarpHighlightTiles;
static struct ObjPalette* sWorldWarpHighlightPalette;
static AnimState sWorldWarpHighlightAnim;
static AnimState sWorldWarpArrowAnim;
static struct ObjTiles* sWorldWarpCursorTiles;
static struct ObjPalette* sWorldWarpCursorPalette;
static AnimState sWorldWarpCursorAnim;
static void* sWorldWarpIconTiles[14];
static void* sWorldWarpIconPalettes[14];
static void* sWorldWarpIconSprites[14];
#ifdef VERSION_EU
static TextSlot sWorldWarpCurrentName[48];
#else
static TextSlot sWorldWarpCurrentName[24];
#endif
static u8 sWorldWarpCurrentNameCount;
#ifdef VERSION_EU
static TextSlot sWorldWarpSelectedName[48];
#else
static TextSlot sWorldWarpSelectedName[24];
#endif
static u8 sWorldWarpSelectedNameCount;
static s16 sWorldWarpState;
static s16 sWorldWarpSteps;
static s32 sWorldWarpBarY[2];
static s32 sWorldWarpBarX;
static s32 sWorldWarpCursorX;
static s32 sWorldWarpCursorY;

static const WarpRect sWarpRects[4] = {
    {2, 1, 20, 2},
    {2, 5, 0, 13},
    {2, 5, 3, 13},
    {4, 6, 6, 13},
};

static WarpIcon sWarpIcons[13] = {
    {7, 8, 1, 3, 22, 16, 0, 21, 18, {0, 0}},
    {6, 9, 2, 0, 15, 16, 0, 14, 18, {0, 0}},
    {5, 10, 3, 1, 8, 16, 0, 7, 18, {0, 0}},
    {4, 12, 0, 2, 1, 16, 1, 0, 14, {0, 0}},
    {11, 3, 7, 5, 1, 12, 0, 7, 14, {0, 0}},
    {10, 2, 4, 6, 8, 12, 0, 14, 14, {0, 0}},
    {9, 1, 5, 7, 15, 12, 0, 21, 14, {0, 0}},
    {8, 0, 6, 4, 22, 12, 2, 28, 10, {0, 0}},
    {0, 7, 9, 11, 22, 8, 0, 21, 10, {0, 0}},
    {1, 6, 10, 8, 15, 8, 0, 14, 10, {0, 0}},
    {2, 5, 11, 9, 8, 8, 0, 7, 10, {0, 0}},
    {12, 4, 8, 10, 1, 8, 3, 0, 5, {0, 0}},
    {3, 11, 12, 12, 3, 3, 0, 0, 0, {0, 0}},
};

static WorldSelectDef sWorldSelectDefs[14] = {
    {NULL, 0, NULL, 0, NULL, NULL},
    {gWorldIconAgrabahPalette, sizeof(gWorldIconAgrabahPalette), gWorldIconAgrabahTiles, sizeof(gWorldIconAgrabahTiles), gWorldIconAgrabahFrame0, LOCALIZED(gWorldNameAgrabah)},
    {gWorldIconAtlanticaPalette, sizeof(gWorldIconAtlanticaPalette), gWorldIconAtlanticaTiles, sizeof(gWorldIconAtlanticaTiles), gWorldIconAtlanticaFrame0, LOCALIZED(gWorldNameAtlantica)},
    {gWorldIconOlympusColiseumPalette, sizeof(gWorldIconOlympusColiseumPalette), gWorldIconOlympusColiseumTiles, sizeof(gWorldIconOlympusColiseumTiles), gWorldIconOlympusColiseumFrame0, LOCALIZED(gWorldNameOlympusColiseum)},
    {gWorldIconWonderlandPalette, sizeof(gWorldIconWonderlandPalette), gWorldIconWonderlandTiles, sizeof(gWorldIconWonderlandTiles), gWorldIconWonderlandFrame0, LOCALIZED(gWorldNameWonderland)},
    {gWorldIconMonstroPalette, sizeof(gWorldIconMonstroPalette), gWorldIconMonstroTiles, sizeof(gWorldIconMonstroTiles), gWorldIconMonstroFrame0, LOCALIZED(gWorldNameMonstro)},
    {gWorldIconHalloweenTownPalette, sizeof(gWorldIconHalloweenTownPalette), gWorldIconHalloweenTownTiles, sizeof(gWorldIconHalloweenTownTiles), gWorldIconHalloweenTownFrame0, LOCALIZED(gWorldNameHalloweenTown)},
    {gWorldIconNeverLandPalette, sizeof(gWorldIconNeverLandPalette), gWorldIconNeverLandTiles, sizeof(gWorldIconNeverLandTiles), gWorldIconNeverLandFrame0, LOCALIZED(gWorldNameNeverLand)},
    {gWorldIconHollowBastionPalette, sizeof(gWorldIconHollowBastionPalette), gWorldIconHollowBastionTiles, sizeof(gWorldIconHollowBastionTiles), gWorldIconHollowBastionFrame0, LOCALIZED(gWorldNameHollowBastion)},
    {gWorldIconDestinyIslandsPalette, sizeof(gWorldIconDestinyIslandsPalette), gWorldIconDestinyIslandsTiles, sizeof(gWorldIconDestinyIslandsTiles), gWorldIconDestinyIslandsFrame0, LOCALIZED(gWorldNameDestinyIslands)},
    {gWorldIconTraverseTownPalette, sizeof(gWorldIconTraverseTownPalette), gWorldIconTraverseTownTiles, sizeof(gWorldIconTraverseTownTiles), gWorldIconTraverseTownFrame0, LOCALIZED(gWorldNameTraverseTown)},
    {gWorldIconTwilightTownPalette, sizeof(gWorldIconTwilightTownPalette), gWorldIconTwilightTownTiles, sizeof(gWorldIconTwilightTownTiles), gWorldIconTwilightTownFrame0, LOCALIZED(gWorldNameTwilightTown)},
    {gWorldIconCastleOblivionPalette, sizeof(gWorldIconCastleOblivionPalette), gWorldIconCastleOblivionTiles, sizeof(gWorldIconCastleOblivionTiles), gWorldIconCastleOblivionFrame0, LOCALIZED(gWorldNameCastleOblivion)},
    {gWorldIcon100AcreWoodPalette, sizeof(gWorldIcon100AcreWoodPalette), gWorldIcon100AcreWoodTiles, sizeof(gWorldIcon100AcreWoodTiles), gWorldIcon100AcreWoodFrame0, LOCALIZED(gWorldName100AcreWood)},
};

void WorldWarpSetTilemapRectPalette(u8 pal, u16 w, s16 h, u16* map, s16 x, s16 y) {
    s16 i;
    s16 j;
    s16 n;
    u16 palBits;

    n = w;
    palBits = pal << 12;
    map += x + y * 32;

    for (j = 0; j < h; j++) {
        for (i = 0; i < n; i++) {
            *map = (*map & 0xFFF) | palBits;
            map++;
        }

        map += 32 - n;
    }
}

void WorldWarpCopyTilemapRect(s16 w, s16 h, u16* src, s16 sx, s16 sy, u16* dst, s16 dx, s16 dy) {
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

u8 WorldWarpLoadCurrentName(s16 world) {
    if (world <= 0) {
        return 0;
    }

#ifdef VERSION_EU
    return LoadTextSlots(GetLocalizedString(sWorldSelectDefs[world].name), sWorldWarpCurrentName);
#else
    return LoadTextSlots(sWorldSelectDefs[world].name, sWorldWarpCurrentName);
#endif
}

u8 WorldWarpLoadSelectedName(s16 world) {
    if (world <= 0) {
        return 0;
    }

#ifdef VERSION_EU
    return LoadTextSlots(GetLocalizedString(sWorldSelectDefs[world].name), sWorldWarpSelectedName);
#else
    return LoadTextSlots(sWorldSelectDefs[world].name, sWorldWarpSelectedName);
#endif
}

void WorldWarpLoadFloorTiles(s16 floor, u8* tiles, void* dst) {
    RequestDma3Copy(tiles + floor * 256, dst, 0x100);
}

u16 WorldWarpReadMenuKeys() {
    s32 keys;

    keys = GetKeysPressed() & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON);
    keys |= GetKeysRepeat() & (DPAD_ANY | L_BUTTON | R_BUTTON);
    return keys;
}

void WorldWarpHandleInput() {
    s16 prev;
    u16 keys;

    prev = sWorldWarpCursor;
    keys = WorldWarpReadMenuKeys();

    if (keys & A_BUTTON) {
        if (sWorldWarpCursor == gGameState.floor) {
            sWorldWarpTarget = -1;
            LoadBgMap(0, gWorldWarpBgMap, sizeof(gWorldWarpBgMap));
            sWorldWarpSteps = 16;
            sWorldWarpState = WORLD_WARP_STATE_TITLE_OUT;
            m4aSongNumStart(SONG_SYS_CLOSE);
        } else {
            if (sWorldWarpFloorWorlds[sWorldWarpCursor] >= 0) {
                sWorldWarpTarget = sWorldWarpCursor;
                LoadBgMap(0, gWorldWarpBgMap, sizeof(gWorldWarpBgMap));
                sWorldWarpSteps = 16;
                sWorldWarpState = WORLD_WARP_STATE_TITLE_OUT;
                m4aSongNumStart(SONG_SYS_WORLDSTART);
            } else {
                m4aSongNumStart(SONG_SYS_BEEP);
            }
        }
    } else if (keys & B_BUTTON) {
        sWorldWarpTarget = -1;
        LoadBgMap(0, gWorldWarpBgMap, sizeof(gWorldWarpBgMap));
        sWorldWarpSteps = 16;
        sWorldWarpState = WORLD_WARP_STATE_TITLE_OUT;
        m4aSongNumStart(SONG_SYS_CLOSE);
    } else if (keys & DPAD_UP) {
        do {
            sWorldWarpCursor = sWarpIcons[sWorldWarpCursor].up;

            if (sWorldWarpCursor == prev) {
                break;
            }
        } while (sWorldWarpFloorWorlds[sWorldWarpCursor] == -1);
    } else if (keys & DPAD_DOWN) {
        do {
            sWorldWarpCursor = sWarpIcons[sWorldWarpCursor].down;

            if (sWorldWarpCursor == prev) {
                break;
            }
        } while (sWorldWarpFloorWorlds[sWorldWarpCursor] == -1);
    } else if (keys & DPAD_LEFT) {
        do {
            sWorldWarpCursor = sWarpIcons[sWorldWarpCursor].left;

            if (sWorldWarpCursor == prev) {
                break;
            }
        } while (sWorldWarpFloorWorlds[sWorldWarpCursor] == -1);
    } else if (keys & DPAD_RIGHT) {
        do {
            sWorldWarpCursor = sWarpIcons[sWorldWarpCursor].right;

            if (sWorldWarpCursor == prev) {
                break;
            }
        } while (sWorldWarpFloorWorlds[sWorldWarpCursor] == -1);
    }

    if (sWorldWarpCursor != prev) {
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
            } else {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
            }

            break;
        case LANGUAGE_FRENCH:
            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorFrenchTiles, (u8*)GetBgCharBase(0) + 0x20);
            } else {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorFrenchTiles, (u8*)GetBgCharBase(0) + 0x20);
            }

            break;
        case LANGUAGE_SPANISH:
            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorSpanishTiles, (u8*)GetBgCharBase(0) + 0x20);
            } else {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorSpanishTiles, (u8*)GetBgCharBase(0) + 0x20);
            }

            break;
        case LANGUAGE_ITALIAN:
            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorItalianTiles, (u8*)GetBgCharBase(0) + 0x20);
            } else {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorItalianTiles, (u8*)GetBgCharBase(0) + 0x20);
            }

            break;
        case LANGUAGE_GERMAN:
        default:
            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorGermanTiles, (u8*)GetBgCharBase(0) + 0x20);
            } else {
                WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorGermanTiles, (u8*)GetBgCharBase(0) + 0x20);
            }

            break;
        }
#else
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
        } else {
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
        }
#endif

        sWorldWarpSelectedNameCount = WorldWarpLoadSelectedName(sWorldWarpFloorWorlds[sWorldWarpCursor]);
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void WorldWarpDraw() {
    s32 i;
#ifdef VERSION_EU
    void* titleSprite;
    void* topBarSprite;
    void* bottomBarSprite;

    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        titleSprite = gWorldWarpBarFrame0;
        topBarSprite = gWorldWarpBarFrame1;
        bottomBarSprite = gWorldWarpBarFrame2;
        break;
    case LANGUAGE_FRENCH:
        titleSprite = gWorldWarpBarFrenchFrame0;
        topBarSprite = gWorldWarpBarFrenchFrame1;
        bottomBarSprite = gWorldWarpBarFrenchFrame2;
        break;
    case LANGUAGE_SPANISH:
        titleSprite = gWorldWarpBarSpanishFrame0;
        topBarSprite = gWorldWarpBarSpanishFrame1;
        bottomBarSprite = gWorldWarpBarSpanishFrame2;
        break;
    case LANGUAGE_ITALIAN:
        titleSprite = gWorldWarpBarItalianFrame0;
        topBarSprite = gWorldWarpBarItalianFrame1;
        bottomBarSprite = gWorldWarpBarItalianFrame2;
        break;
    case LANGUAGE_GERMAN:
    default:
        titleSprite = gWorldWarpBarGermanFrame0;
        topBarSprite = gWorldWarpBarGermanFrame1;
        bottomBarSprite = gWorldWarpBarGermanFrame2;
        break;
    }

    DrawMenuHeader(sWorldWarpBarX >> 8, 0, titleSprite, sWorldWarpBarTiles, sWorldWarpBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB8);

    if (sWorldWarpState != WORLD_WARP_STATE_SELECT) {
        DrawSprite(0x80, sWorldWarpBarY[0] >> 8, topBarSprite, sWorldWarpBarTiles, sWorldWarpBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB9);
        DrawSprite(0x80, sWorldWarpBarY[1] >> 8, bottomBarSprite, sWorldWarpBarTiles, sWorldWarpBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB9);
    }
#else
    if (sWorldWarpState != WORLD_WARP_STATE_SELECT) {
        DrawMenuHeader(sWorldWarpBarX >> 8, 0, gWorldWarpBarFrame0, sWorldWarpBarTiles, sWorldWarpBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB8);
        DrawSprite(0x80, sWorldWarpBarY[0] >> 8, gWorldWarpBarFrame1, sWorldWarpBarTiles, sWorldWarpBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB9);
        DrawSprite(0x80, sWorldWarpBarY[1] >> 8, gWorldWarpBarFrame2, sWorldWarpBarTiles, sWorldWarpBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB9);
    }
#ifdef PLATFORM_PC
    /* this version stops drawing the title sprite while selecting: the BG copy stays, the HD banner goes over it */
    else {
        int level = (int)(gFadeWork->amount >> 8) * 255 / 31;
        PortHdStillSprite(gWorldWarpBarTiles, gWorldWarpBarFrame0, sWorldWarpBarX >> 8, 0,
                          gFadeWork->mode == FADE_MODE_BLACK ? level : 0,
                          gFadeWork->mode == FADE_MODE_WHITE || gFadeWork->mode == FADE_MODE_ADD_WHITE ? level : 0);
    }
#endif
#endif

    if (sWorldWarpState == WORLD_WARP_STATE_SELECT) {
        DrawSprite(sWarpIcons[sWorldWarpCursor].x * 8 + 22,
            sWarpIcons[sWorldWarpCursor].y * 8 + 12,
            AnimUpdate(&sWorldWarpHighlightAnim), sWorldWarpHighlightTiles, sWorldWarpHighlightPalette, NULL, SPRITE_PRIORITY(2), 0x898);
        ApproachValueHalf(&sWorldWarpCursorX, (sWarpIcons[sWorldWarpCursor].x << 11) + 0x2000);
        ApproachValueHalf(&sWorldWarpCursorY, (sWarpIcons[sWorldWarpCursor].y << 11) + 0xFFFFFA00);
        DrawSprite(sWorldWarpCursorX >> 8, sWorldWarpCursorY >> 8, AnimUpdate(&sWorldWarpCursorAnim),
            sWorldWarpCursorTiles, sWorldWarpCursorPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);
    }

    for (i = 0; i <= 12; i++) {
        if (sWorldWarpIconSprites[i] != NULL) {
            DrawSprite(sWarpIcons[i].x * 8 + 16, sWarpIcons[i].y * 8 + 16,
                sWorldWarpIconSprites[i], sWorldWarpIconTiles[i], sWorldWarpIconPalettes[i], NULL, SPRITE_PRIORITY(2), i + 0x834);
        }
    }

    if (sWorldWarpIconSprites[gGameState.floor] != NULL) {
#ifdef VERSION_EU
        DrawSprite(0x66, 0x10, sWorldWarpIconSprites[gGameState.floor], sWorldWarpIconTiles[gGameState.floor],
            sWorldWarpIconPalettes[gGameState.floor], NULL, 0, 2);
#else
        DrawSprite(0x70, 0x10, sWorldWarpIconSprites[gGameState.floor], sWorldWarpIconTiles[gGameState.floor],
            sWorldWarpIconPalettes[gGameState.floor], NULL, 0, 2);
#endif
    }

    if (sWorldWarpCurrentNameCount != 0) {
#ifdef VERSION_EU
        DrawTextSlots(0x78, 0x0C, sWorldWarpCurrentName, sWorldWarpHighlightPalette, 0, sWorldWarpCurrentNameCount);
#else
        DrawTextSlots(0x80, 0x0C, sWorldWarpCurrentName, sWorldWarpHighlightPalette, 0, sWorldWarpCurrentNameCount);
#endif
    }

    DrawSprite(0xB0, 0x1A, AnimUpdate(&sWorldWarpArrowAnim), sWorldWarpHighlightTiles, sWorldWarpHighlightPalette, NULL, 0, 2);

    if (sWorldWarpIconSprites[sWorldWarpCursor] != NULL) {
#ifdef VERSION_EU
        DrawSprite(0x66, 0x30, sWorldWarpIconSprites[sWorldWarpCursor], sWorldWarpIconTiles[sWorldWarpCursor],
            sWorldWarpIconPalettes[sWorldWarpCursor], NULL, 0, 2);
#else
        DrawSprite(0x70, 0x30, sWorldWarpIconSprites[sWorldWarpCursor], sWorldWarpIconTiles[sWorldWarpCursor],
            sWorldWarpIconPalettes[sWorldWarpCursor], NULL, 0, 2);
#endif
    }

    if (sWorldWarpSelectedNameCount != 0) {
#ifdef VERSION_EU
        DrawTextSlots(0x78, 0x2C, sWorldWarpSelectedName, sWorldWarpCursorPalette, 0, sWorldWarpSelectedNameCount);
#else
        DrawTextSlots(0x80, 0x2C, sWorldWarpSelectedName, sWorldWarpCursorPalette, 0, sWorldWarpSelectedNameCount);
#endif
    }
}

void mode_worldwarp_0() {
    s32 i;
    void** tilemapPtr;

    sWorldWarpFloorCount = GetProgressFloor() + 1;
    tilemapPtr = &sWorldWarpTilemap;
    *tilemapPtr = EwramAlloc(0x500);
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
    sWorldWarpCursor = gGameState.floor;
    sWorldWarpTarget = -1;

    for (i = 0; i <= 12; i++) {
        if (i < sWorldWarpFloorCount) {
            sWorldWarpFloorWorlds[i] = gGameState.floors[i].world;
        } else {
            sWorldWarpFloorWorlds[i] = -1;
        }
    }

    sWorldWarpState = WORLD_WARP_STATE_BARS_IN;
    sWorldWarpSteps = 16;
    sWorldWarpBarY[0] = -0x800;
    sWorldWarpBarY[1] = 0xA800;
    sWorldWarpBarX = -0x8000;
    sWorldWarpCursorX = (sWarpIcons[sWorldWarpCursor].x << 11) + 0x2000;
    sWorldWarpCursorY = (sWarpIcons[sWorldWarpCursor].y << 11) - 0x600;
    LoadBgPalette(0, gWorldWarpPalettes, sizeof(gWorldWarpPalettes));

    for (i = 0; i <= 12; i++) {
        if (sWorldWarpFloorWorlds[i] > 0) {
            sWorldWarpIconPalettes[i] = LoadObjPalette(sWorldSelectDefs[sWorldWarpFloorWorlds[i]].palette, sWorldSelectDefs[sWorldWarpFloorWorlds[i]].paletteSize);
            sWorldWarpIconTiles[i] = LoadObjTiles(sWorldSelectDefs[sWorldWarpFloorWorlds[i]].tiles, sWorldSelectDefs[sWorldWarpFloorWorlds[i]].tilesSize);
            sWorldWarpIconSprites[i] = sWorldSelectDefs[sWorldWarpFloorWorlds[i]].sprite;
        } else {
            sWorldWarpIconPalettes[i] = NULL;
            sWorldWarpIconTiles[i] = NULL;
            sWorldWarpIconSprites[i] = NULL;
        }
    }

    LoadBgTiles(0, gWorldWarpTiles, sizeof(gWorldWarpTiles));

#ifdef VERSION_EU
    if (gGameState.flags & GAME_FLAG_RIKU) {
        switch (gLanguage) {
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gWorldWarpRikuFloorNumberFrenchTiles, (u8*)GetBgCharBase(0) + 0x6400, sizeof(gWorldWarpRikuFloorNumberFrenchTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gWorldWarpRikuFloorNumberSpanishTiles, (u8*)GetBgCharBase(0) + 0x6400, sizeof(gWorldWarpRikuFloorNumberSpanishTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gWorldWarpRikuFloorNumberItalianTiles, (u8*)GetBgCharBase(0) + 0x6400, sizeof(gWorldWarpRikuFloorNumberItalianTiles));
            break;
        case LANGUAGE_GERMAN:
            // @bug Reads past the end of gWorldWarpRikuFloorNumberGermanTiles.
            RequestDma3Copy(gWorldWarpRikuFloorNumberGermanTiles, (u8*)GetBgCharBase(0) + 0x6400, 0x800);
            break;
        }
    } else {
        switch (gLanguage) {
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gWorldWarpFloorNumberFrenchTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldWarpFloorNumberFrenchTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gWorldWarpFloorNumberSpanishTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldWarpFloorNumberSpanishTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gWorldWarpFloorNumberItalianTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldWarpFloorNumberItalianTiles));
            break;
        case LANGUAGE_GERMAN:
            RequestDma3Copy(gWorldWarpFloorNumberGermanTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldWarpFloorNumberGermanTiles));
            break;
        }
    }
#endif

    LoadBgMap(0, gWorldWarpBgMap, sizeof(gWorldWarpBgMap));
    DmaCopy16(3, gWorldWarpFloorMap, sWorldWarpTilemap, sizeof(gWorldWarpFloorMap));

    for (i = 0; i <= 12; i++) {
        if (sWorldWarpFloorWorlds[i] >= 0) {
            WorldWarpCopyTilemapRect(7, 4, gWorldWarpFloorPartsMap, 0, 0, sWorldWarpTilemap, sWarpIcons[i].x, sWarpIcons[i].y);
        }
    }

    for (i = 0; i < sWorldWarpFloorCount - 1; i++) {
        if (sWorldWarpFloorWorlds[i] >= 0 && sWorldWarpFloorWorlds[i + 1] >= 0) {
            WorldWarpCopyTilemapRect(sWarpRects[sWarpIcons[i].rect].width, sWarpRects[sWarpIcons[i].rect].height,
                gWorldWarpFloorPartsMap, sWarpRects[sWarpIcons[i].rect].x,
                sWarpRects[sWarpIcons[i].rect].y, sWorldWarpTilemap,
                sWarpIcons[i].x2, sWarpIcons[i].y2);
        }
    }

    for (i = 0; i < sWorldWarpFloorCount; i++) {
        if (sWorldWarpFloorWorlds[i] >= 0) {
            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                if (i <= 8) {
                    WorldWarpCopyTilemapRect(3, 1, gWorldWarpFloorPartsMap, i * 3, 9, sWorldWarpTilemap,
                        sWarpIcons[i].x + 3, sWarpIcons[i].y + 2);
                } else {
                    WorldWarpCopyTilemapRect(4, 1, gWorldWarpFloorPartsMap, (i - 9) * 4, 10, sWorldWarpTilemap,
                        sWarpIcons[i].x + 3, sWarpIcons[i].y + 2);
                }
            } else if (i <= 2) {
                WorldWarpCopyTilemapRect(4, 1, gWorldWarpFloorPartsMap, i * 4, 21, sWorldWarpTilemap,
                    sWarpIcons[i].x + 3, sWarpIcons[i].y + 2);
            } else {
                WorldWarpCopyTilemapRect(3, 1, gWorldWarpFloorPartsMap, (i - 3) * 3, 22, sWorldWarpTilemap,
                    sWarpIcons[i].x + 3, sWarpIcons[i].y + 2);
            }
        }
    }

    for (i = 0; i < sWorldWarpFloorCount; i++) {
        if (sWorldWarpCursor == i) {
            WorldWarpSetTilemapRectPalette(3, 7, 4, sWorldWarpTilemap, sWarpIcons[i].x, sWarpIcons[i].y);
        } else {
            WorldWarpSetTilemapRectPalette(2, 7, 4, sWorldWarpTilemap, sWarpIcons[i].x, sWarpIcons[i].y);
        }
    }

    LoadBgMap(1, sWorldWarpTilemap, 0x500);

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpCurrentFloorTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
        } else {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpRikuCurrentFloorTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
        }

        break;
    case LANGUAGE_FRENCH:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpCurrentFloorFrenchTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorFrenchTiles, (u8*)GetBgCharBase(0) + 0x20);
        } else {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpRikuCurrentFloorFrenchTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorFrenchTiles, (u8*)GetBgCharBase(0) + 0x20);
        }

        break;
    case LANGUAGE_SPANISH:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpCurrentFloorSpanishTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorSpanishTiles, (u8*)GetBgCharBase(0) + 0x20);
        } else {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpRikuCurrentFloorSpanishTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorSpanishTiles, (u8*)GetBgCharBase(0) + 0x20);
        }

        break;
    case LANGUAGE_ITALIAN:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpCurrentFloorItalianTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorItalianTiles, (u8*)GetBgCharBase(0) + 0x20);
        } else {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpRikuCurrentFloorItalianTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorItalianTiles, (u8*)GetBgCharBase(0) + 0x20);
        }

        break;
    case LANGUAGE_GERMAN:
    default:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpCurrentFloorGermanTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorGermanTiles, (u8*)GetBgCharBase(0) + 0x20);
        } else {
            WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpRikuCurrentFloorGermanTiles, (u8*)GetBgCharBase(0) + 0x120);
            WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorGermanTiles, (u8*)GetBgCharBase(0) + 0x20);
        }

        break;
    }
#else
    if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
        WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpCurrentFloorTiles, (u8*)GetBgCharBase(0) + 0x120);
        WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
    } else {
        WorldWarpLoadFloorTiles(gGameState.floor, gWorldWarpRikuCurrentFloorTiles, (u8*)GetBgCharBase(0) + 0x120);
        WorldWarpLoadFloorTiles(sWorldWarpCursor, gWorldWarpRikuSelectedFloorTiles, (u8*)GetBgCharBase(0) + 0x20);
    }
#endif

    sWorldWarpBarPalette = LoadObjPalette(gWorldWarpBarPalette, sizeof(gWorldWarpBarPalette));

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        sWorldWarpBarTiles = LoadObjTiles(gWorldWarpBarTiles, sizeof(gWorldWarpBarTiles));
        break;
    case LANGUAGE_FRENCH:
        sWorldWarpBarTiles = LoadObjTiles(gWorldWarpBarFrenchTiles, sizeof(gWorldWarpBarFrenchTiles));
        break;
    case LANGUAGE_SPANISH:
        sWorldWarpBarTiles = LoadObjTiles(gWorldWarpBarSpanishTiles, sizeof(gWorldWarpBarSpanishTiles));
        break;
    case LANGUAGE_ITALIAN:
        sWorldWarpBarTiles = LoadObjTiles(gWorldWarpBarItalianTiles, sizeof(gWorldWarpBarItalianTiles));
        break;
    case LANGUAGE_GERMAN:
    default:
        sWorldWarpBarTiles = LoadObjTiles(gWorldWarpBarGermanTiles, sizeof(gWorldWarpBarGermanTiles));
        break;
    }
#else
    sWorldWarpBarTiles = LoadObjTiles(gWorldWarpBarTiles, sizeof(gWorldWarpBarTiles));
#endif
    sWorldWarpHighlightPalette = LoadObjPalette(gWorldWarpHighlightPalette, sizeof(gWorldWarpHighlightPalette));
    sWorldWarpHighlightTiles = LoadObjTiles(gWorldWarpHighlightTiles, sizeof(gWorldWarpHighlightTiles));
    AnimInit(&sWorldWarpHighlightAnim, gWorldWarpHighlightAnims, gWorldWarpHighlightFrames);
    AnimStart(&sWorldWarpHighlightAnim, 0, ANIM_FLAG_LOOP);
    AnimInit(&sWorldWarpArrowAnim, gWorldWarpHighlightAnims, gWorldWarpHighlightFrames);
    AnimStart(&sWorldWarpArrowAnim, 1, ANIM_FLAG_LOOP);
    sWorldWarpCursorPalette = LoadObjPalette(gWorldWarpCursorPalette, sizeof(gWorldWarpCursorPalette));
    sWorldWarpCursorTiles = LoadObjTiles(gWorldWarpCursorTiles, sizeof(gWorldWarpCursorTiles));
    AnimInit(&sWorldWarpCursorAnim, gWorldWarpCursorAnims, gWorldWarpCursorFrames);
    AnimStart(&sWorldWarpCursorAnim, 0, ANIM_FLAG_LOOP);
    InitTextSlots(sWorldWarpCurrentName, ARRAY_COUNT(sWorldWarpCurrentName));
    InitTextSlots(sWorldWarpSelectedName, ARRAY_COUNT(sWorldWarpSelectedName));
    sWorldWarpCurrentNameCount = WorldWarpLoadCurrentName(sWorldWarpFloorWorlds[gGameState.floor]);
    sWorldWarpSelectedNameCount = WorldWarpLoadSelectedName(sWorldWarpFloorWorlds[sWorldWarpCursor]);
    EnableBg(0);
    EnableBg(1);
    DisableBg(2);
    DisableBg(3);
}

void mode_worldwarp_1() {
    UpdatePlayTime();

    switch (sWorldWarpState) {
    case WORLD_WARP_STATE_BARS_IN:
        ApproachValue(&sWorldWarpBarY[0], 0, sWorldWarpSteps);
        ApproachValue(&sWorldWarpBarY[1], 0x9800, sWorldWarpSteps);

        if (--sWorldWarpSteps <= 0) {
            sWorldWarpSteps = 16;
            sWorldWarpState = WORLD_WARP_STATE_TITLE_IN;
        }

        break;
    case WORLD_WARP_STATE_TITLE_IN:
        ApproachValue(&sWorldWarpBarX, 0, sWorldWarpSteps);

        if (--sWorldWarpSteps <= 0) {
            LoadBgMap(0, gWorldWarpBgHeaderMap, sizeof(gWorldWarpBgHeaderMap));
            sWorldWarpState = WORLD_WARP_STATE_SELECT;
        }

        break;
    case WORLD_WARP_STATE_SELECT:
        WorldWarpHandleInput();
        break;
    case WORLD_WARP_STATE_TITLE_OUT:
        ApproachValue(&sWorldWarpBarX, -0x8000, sWorldWarpSteps);

        if (--sWorldWarpSteps <= 0) {
            sWorldWarpSteps = 16;
            sWorldWarpState = WORLD_WARP_STATE_BARS_OUT;
        }

        break;
    case WORLD_WARP_STATE_BARS_OUT:
        ApproachValue(&sWorldWarpBarY[0], -0x800, sWorldWarpSteps);
        ApproachValue(&sWorldWarpBarY[1], 0xA800, sWorldWarpSteps);

        if (--sWorldWarpSteps <= 0) {
            sWorldWarpSteps = 16;
            FadeStartOut(FADE_MODE_BLACK, 16);
            sWorldWarpState = WORLD_WARP_STATE_EXIT;
        }

        break;
    case WORLD_WARP_STATE_EXIT:
        if (!FadeIsActive()) {
            if (sWorldWarpTarget >= 0) {
                WarpToFloor(sWorldWarpTarget);
            } else {
                RequestMapMode();
            }
        }

        break;
    }

    WorldWarpDraw();
}

void mode_worldwarp_2() {
    s32 i;

    ReleaseObjPalette(sWorldWarpBarPalette);
    ReleaseObjTiles(sWorldWarpBarTiles);
    ReleaseObjPalette(sWorldWarpHighlightPalette);
    ReleaseObjTiles(sWorldWarpHighlightTiles);
    ReleaseObjPalette(sWorldWarpCursorPalette);
    ReleaseObjTiles(sWorldWarpCursorTiles);

    for (i = 0; i <= 12; i++) {
        if (sWorldWarpIconPalettes[i] != NULL) {
            ReleaseObjPalette(sWorldWarpIconPalettes[i]);
        }

        if (sWorldWarpIconTiles[i] != NULL) {
            ReleaseObjTiles(sWorldWarpIconTiles[i]);
        }
    }

    FreeTextSlots(sWorldWarpCurrentName, ARRAY_COUNT(sWorldWarpCurrentName));
    FreeTextSlots(sWorldWarpSelectedName, ARRAY_COUNT(sWorldWarpSelectedName));
    EwramFree(sWorldWarpTilemap);
}

Mode gModeWorldwarp = {
    "mode_worldwarp",
    (ModeInitFunc)mode_worldwarp_0,
    mode_worldwarp_1,
    mode_worldwarp_2,
};
