/**
 * mode_worldinspect.c
 * World List Screen
 */

#include "types.h"
#ifdef PLATFORM_PC
#include "engine.h"
#include "port/port_hdsprites.h"
#endif
#include "worldinspect_data.h"
#include "registration_data.h"
#include "system_state.h"
#include "map_api.h"
#include "engine_math.h"
#include "m4a_song.h"
#include "text.h"
#include "monsgage.h"
#include "fade.h"
#include "obj_api.h"
#include "display.h"
#include "malloc.h"
#include "mode_worldinspect.h"
#include "game_state.h"
#include "gba/keys.h"
#include "anim.h"
#include "card_description_data.h"
#include "sprites_bos5.h"
#include "sprites_worldinspect.h"
#include "gba/io_reg.h"
#include "songs.h"
#include "common_text.h"
#include "world_types.h"
#include "jiminy_data.h"
#include "jiminy_inline_text_data.h"
#include "key.h"
#include "mode.h"
#include "obj.h"
#include "poo_api.h"
#include "text_types.h"
#include "gba/macro.h"
#include "sprite_palettes.h"
#include <stddef.h>
#include "macros.h"

enum WorldInspectState {
    WORLD_INSPECT_STATE_BARS_IN,
    WORLD_INSPECT_STATE_TITLE_IN,
    WORLD_INSPECT_STATE_SELECT,
    WORLD_INSPECT_STATE_TITLE_OUT,
    WORLD_INSPECT_STATE_BARS_OUT,
    WORLD_INSPECT_STATE_EXIT
};

static s16 sWorldInspectCursor;
static s16 sWorldInspectFloorCount;
static s16 sWorldInspectWorlds[12];

static const WorldinspectConn sWorldinspectConns[3] = {
    { 2, 1, 20, 2 },
    { 2, 5, 0, 13 },
    { 2, 5, 3, 13 },
};

static s16 sWorldInspectDetailOpen;
static u8 sWorldInspectBobPhase;
static struct ObjPalette* sWorldInspectBarPalette;
static struct ObjTiles* sWorldInspectBarTiles;
static struct ObjPalette* sWorldInspectHighlightPalette;
static struct ObjTiles* sWorldInspectHighlightTiles;
static AnimState sWorldInspectHighlightAnim;
static struct ObjPalette* sWorldInspectCursorPalette;
static struct ObjTiles* sWorldInspectCursorTiles;
static AnimState sWorldInspectCursorAnim;
static void* sWorldInspectIconPalettes[12];
static void* sWorldInspectIconTiles[12];
static void* sWorldInspectIconSprites[12];
#ifdef VERSION_EU
static TextSlot sWorldInspectNameText[48];
#else
static TextSlot sWorldInspectNameText[24];
#endif
static u8 sWorldInspectNameTextCount;
#ifdef VERSION_EU
static TextSlot sWorldInspectDescText[120];
#else
static TextSlot sWorldInspectDescText[60];
#endif
static u8 sWorldInspectDescTextCount;
static void* sWorldInspectDetailPalettes[2];
static void* sWorldInspectDetailTiles[2];
static void* sWorldInspectDetailSprites[2];
static s16 sWorldInspectState;
static s16 sWorldInspectSteps;
static s32 sWorldInspectBarY[2];
static s32 sWorldInspectBarX;
static void* sWorldInspectTilemap;
static s32 sWorldInspectCursorX;
static s32 sWorldInspectCursorY;
static u8 sWorldInspectReturnToMenu;

static WorldinspectNav sWorldinspectNavs[12] = {
    { 7, 8, 1, 3, 22, 14, 0, 21, 16, { 0, 0 } },
    { 6, 9, 2, 0, 15, 14, 0, 14, 16, { 0, 0 } },
    { 5, 10, 3, 1, 8, 14, 0, 7, 16, { 0, 0 } },
    { 4, 11, 0, 2, 1, 14, 1, 0, 12, { 0, 0 } },
    { 11, 3, 7, 5, 1, 10, 0, 7, 12, { 0, 0 } },
    { 10, 2, 4, 6, 8, 10, 0, 14, 12, { 0, 0 } },
    { 9, 1, 5, 7, 15, 10, 0, 21, 12, { 0, 0 } },
    { 8, 0, 6, 4, 22, 10, 2, 28, 8, { 0, 0 } },
    { 0, 7, 9, 11, 22, 6, 0, 21, 8, { 0, 0 } },
    { 1, 6, 10, 8, 15, 6, 0, 14, 8, { 0, 0 } },
    { 2, 5, 11, 9, 8, 6, 0, 7, 8, { 0, 0 } },
    { 3, 4, 8, 10, 1, 6, 0, 0, 8, { 0, 0 } },
};

static WorldinspectMsg sWorldinspectMsgs[14] = {
    {
        0, 0, NULL, 0, NULL, 0, NULL, NULL, 0, NULL, 0, NULL,
        NULL,
        0, 0,
    },
    {
        1, WORLD_AGRABAH, gWorldIconAgrabahPalette, sizeof(gWorldIconAgrabahPalette), gWorldIconAgrabahTiles, sizeof(gWorldIconAgrabahTiles), gWorldIconAgrabahFrame0, gWorldImageAgrabahPalette, sizeof(gWorldImageAgrabahPalette), gWorldImageAgrabahTiles, sizeof(gWorldImageAgrabahTiles), gWorldImageAgrabahFrame0,
        LOCALIZED(gWorldNameAgrabah),
        0, 0,
    },
    {
        2, WORLD_ATLANTICA, gWorldIconAtlanticaPalette, sizeof(gWorldIconAtlanticaPalette), gWorldIconAtlanticaTiles, sizeof(gWorldIconAtlanticaTiles), gWorldIconAtlanticaFrame0, gWorldImageAtlanticaPalette, sizeof(gWorldImageAtlanticaPalette), gWorldImageAtlanticaTiles, sizeof(gWorldImageAtlanticaTiles), gWorldImageAtlanticaFrame0,
        LOCALIZED(gWorldNameAtlantica),
        2, 2,
    },
    {
        4, WORLD_OLYMPUS_COLISEUM, gWorldIconOlympusColiseumPalette, sizeof(gWorldIconOlympusColiseumPalette), gWorldIconOlympusColiseumTiles, sizeof(gWorldIconOlympusColiseumTiles), gWorldIconOlympusColiseumFrame0, gWorldImageOlympusColiseumPalette, sizeof(gWorldImageOlympusColiseumPalette), gWorldImageOlympusColiseumTiles, sizeof(gWorldImageOlympusColiseumTiles), gWorldImageOlympusColiseumFrame0,
        LOCALIZED(gWorldNameOlympusColiseum),
        6, 6,
    },
    {
        8, WORLD_WONDERLAND, gWorldIconWonderlandPalette, sizeof(gWorldIconWonderlandPalette), gWorldIconWonderlandTiles, sizeof(gWorldIconWonderlandTiles), gWorldIconWonderlandFrame0, gWorldImageWonderlandPalette, sizeof(gWorldImageWonderlandPalette), gWorldImageWonderlandTiles, sizeof(gWorldImageWonderlandTiles), gWorldImageWonderlandFrame0,
        LOCALIZED(gWorldNameWonderland),
        4, 4,
    },
    {
        16, WORLD_MONSTRO, gWorldIconMonstroPalette, sizeof(gWorldIconMonstroPalette), gWorldIconMonstroTiles, sizeof(gWorldIconMonstroTiles), gWorldIconMonstroFrame0, gWorldImageMonstroPalette, sizeof(gWorldImageMonstroPalette), gWorldImageMonstroTiles, sizeof(gWorldImageMonstroTiles), gWorldImageMonstroFrame0,
        LOCALIZED(gWorldNameMonstro),
        3, 3,
    },
    {
        32, WORLD_HALLOWEEN_TOWN, gWorldIconHalloweenTownPalette, sizeof(gWorldIconHalloweenTownPalette), gWorldIconHalloweenTownTiles, sizeof(gWorldIconHalloweenTownTiles), gWorldIconHalloweenTownFrame0, gWorldImageHalloweenTownPalette, sizeof(gWorldImageHalloweenTownPalette), gWorldImageHalloweenTownTiles, sizeof(gWorldImageHalloweenTownTiles), gWorldImageHalloweenTownFrame0,
        LOCALIZED(gWorldNameHalloweenTown),
        5, 5,
    },
    {
        64, WORLD_NEVER_LAND, gWorldIconNeverLandPalette, sizeof(gWorldIconNeverLandPalette), gWorldIconNeverLandTiles, sizeof(gWorldIconNeverLandTiles), gWorldIconNeverLandFrame0, gWorldImageNeverLandPalette, sizeof(gWorldImageNeverLandPalette), gWorldImageNeverLandTiles, sizeof(gWorldImageNeverLandTiles), gWorldImageNeverLandFrame0,
        LOCALIZED(gWorldNameNeverLand),
        1, 1,
    },
    {
        128, WORLD_HOLLOW_BASTION, gWorldIconHollowBastionPalette, sizeof(gWorldIconHollowBastionPalette), gWorldIconHollowBastionTiles, sizeof(gWorldIconHollowBastionTiles), gWorldIconHollowBastionFrame0, gWorldImageHollowBastionPalette, sizeof(gWorldImageHollowBastionPalette), gWorldImageHollowBastionTiles, sizeof(gWorldImageHollowBastionTiles), gWorldImageHollowBastionFrame0,
        LOCALIZED(gWorldNameHollowBastion),
        7, 7,
    },
    {
        256, WORLD_DESTINY_ISLANDS, gWorldIconDestinyIslandsPalette, sizeof(gWorldIconDestinyIslandsPalette), gWorldIconDestinyIslandsTiles, sizeof(gWorldIconDestinyIslandsTiles), gWorldIconDestinyIslandsFrame0, gWorldImageDestinyIslandsPalette, sizeof(gWorldImageDestinyIslandsPalette), gWorldImageDestinyIslandsTiles, sizeof(gWorldImageDestinyIslandsTiles), gWorldImageDestinyIslandsFrame0,
        LOCALIZED(gWorldNameDestinyIslands),
        8, 8,
    },
    {
        512, WORLD_TRAVERSE_TOWN, gWorldIconTraverseTownPalette, sizeof(gWorldIconTraverseTownPalette), gWorldIconTraverseTownTiles, sizeof(gWorldIconTraverseTownTiles), gWorldIconTraverseTownFrame0, gWorldImageTraverseTownPalette, sizeof(gWorldImageTraverseTownPalette), gWorldImageTraverseTownTiles, sizeof(gWorldImageTraverseTownTiles), gWorldImageTraverseTownFrame0,
        LOCALIZED(gWorldNameTraverseTown),
        9, 9,
    },
    {
        2048, WORLD_TWILIGHT_TOWN, gWorldIconTwilightTownPalette, sizeof(gWorldIconTwilightTownPalette), gWorldIconTwilightTownTiles, sizeof(gWorldIconTwilightTownTiles), gWorldIconTwilightTownFrame0, gWorldImageTwilightTownPalette, sizeof(gWorldImageTwilightTownPalette), gWorldImageTwilightTownTiles, sizeof(gWorldImageTwilightTownTiles), gWorldImageTwilightTownFrame0,
        LOCALIZED(gWorldNameTwilightTown),
        10, 10,
    },
    {
        4096, WORLD_CASTLE_OBLIVION, gWorldIconCastleOblivionPalette, sizeof(gWorldIconCastleOblivionPalette), gWorldIconCastleOblivionTiles, sizeof(gWorldIconCastleOblivionTiles), gWorldIconCastleOblivionFrame0, gWorldImageCastleOblivionPalette, sizeof(gWorldImageCastleOblivionPalette), gWorldImageCastleOblivionTiles, sizeof(gWorldImageCastleOblivionTiles), gWorldImageCastleOblivionFrame0,
        LOCALIZED(gWorldNameCastleOblivion),
        11, 13,
    },
    {
        1024, WORLD_100_ACRE_WOOD, gWorldIcon100AcreWoodPalette, sizeof(gWorldIcon100AcreWoodPalette), gWorldIcon100AcreWoodTiles, sizeof(gWorldIcon100AcreWoodTiles), gWorldIcon100AcreWoodFrame0, gWorldImage100AcreWoodPalette, sizeof(gWorldImage100AcreWoodPalette), gWorldImage100AcreWoodTiles, sizeof(gWorldImage100AcreWoodTiles), gWorldImage100AcreWoodFrame0,
        LOCALIZED(gWorldName100AcreWood),
        12, 12,
    },
};

void WorldInspectSetTilemapRectPalette(u8 pal, u16 w, s16 h, u16* map, s16 x, s16 y) {
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

void WorldInspectCopyTilemapRect(s16 w, s16 h, u16* src, s16 sx, s16 sy, u16* dst, s16 dx, s16 dy) {
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

u8 WorldInspectLoadName(s16 world) {
#ifdef VERSION_EU
    u8 count = 0;

    if (world != 0) {
        count = LoadTextSlots(GetLocalizedString(sWorldinspectMsgs[world].text), sWorldInspectNameText);
    }

    return count;
#else
    if (world == 0) {
        return 0;
    }

    return LoadTextSlots(sWorldinspectMsgs[world].text, sWorldInspectNameText);
#endif
}

u8 WorldInspectLoadDesc(s16 world) {
    CardDescriptionText** descs;
    CardDescriptionText** desc;
    u16 descId;

    if (world != 0) {
        descs = gWorldDescriptions;

        if (gGameState.flags & GAME_FLAG_RIKU) {
            descId = sWorldinspectMsgs[world].descId2;
        } else {
            descId = sWorldinspectMsgs[world].descId;
        }

        desc = &descs[descId];

#ifdef VERSION_EU
        {
            const u8** langs = (*desc)->strings;

            return LoadTextSlots((void*)langs[gLanguage], sWorldInspectDescText);
        }
#else
        return LoadTextSlots((void*)*desc, sWorldInspectDescText);
#endif
    }

    return 0;
}

void WorldInspectLoadFloorTiles(s16 index) {
    u8* src;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
#endif
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            src = gWorldInspectFloorTiles;
        } else {
            src = gWorldInspectRikuFloorTiles;
        }

#ifdef VERSION_EU
        break;
    case LANGUAGE_FRENCH:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            src = gWorldInspectFloorFrenchTiles;
        } else {
            src = gWorldInspectRikuFloorFrenchTiles;
        }

        break;
    case LANGUAGE_SPANISH:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            src = gWorldInspectFloorSpanishTiles;
        } else {
            src = gWorldInspectRikuFloorSpanishTiles;
        }

        break;
    case LANGUAGE_ITALIAN:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            src = gWorldInspectFloorItalianTiles;
        } else {
            src = gWorldInspectRikuFloorItalianTiles;
        }

        break;
    case LANGUAGE_GERMAN:
    default:
        if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
            src = gWorldInspectFloorGermanTiles;
        } else {
            src = gWorldInspectRikuFloorGermanTiles;
        }

        break;
    }
#endif

    if (index < sWorldInspectFloorCount) {
        src += index * 256;
    } else {
        src += 0xD00;
    }

    RequestDma3Copy(src, (u8*)GetBgCharBase(0) + 32, 0x100);
}

s32 WorldInspectReadMenuKeys() {
    s32 keys;

    keys = GetKeysPressed() & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON);
    keys |= GetKeysRepeat() & (DPAD_ANY | L_BUTTON | R_BUTTON);
    return keys;
}

void WorldInspectHandleInput() {
    u16 keys;
    s16 old;
    s16 i;

    old = sWorldInspectCursor;
    keys = WorldInspectReadMenuKeys();

    if (keys & A_BUTTON) {
        if (sWorldInspectWorlds[sWorldInspectCursor] != 0) {
#ifdef VERSION_EU
            ReleaseObjPalette(sWorldInspectBarPalette);
            sWorldInspectBarPalette = NULL;
#endif
            sWorldInspectDetailPalettes[0] = LoadObjPalette(gWorldselectCardPalette, sizeof(gWorldselectCardPalette));
            sWorldInspectDetailTiles[0] = LoadObjTiles(gWorldselectCardTiles, sizeof(gWorldselectCardTiles));
            sWorldInspectDetailSprites[0] = gWorldselectCardFrame0;

            if (sWorldInspectCursor <= 9) {
                for (i = 10; i < 12; i++) {
                    if (sWorldInspectIconPalettes[i] != NULL) {
                        ReleaseObjPalette(sWorldInspectIconPalettes[i]);
                        sWorldInspectIconPalettes[i] = NULL;
                    }
                }
            } else {
                for (i = 4; i < 6; i++) {
                    if (sWorldInspectIconPalettes[i] != NULL) {
                        ReleaseObjPalette(sWorldInspectIconPalettes[i]);
                        sWorldInspectIconPalettes[i] = NULL;
                    }
                }
            }

            if (sWorldInspectWorlds[sWorldInspectCursor] == WORLD_100_ACRE_WOOD && IsPooAltImageActive()) {
                sWorldInspectDetailPalettes[1] = LoadObjPalette(gPooAltImagePalettes, sizeof(gPooAltImagePalettes));
                sWorldInspectDetailTiles[1] = LoadObjTiles(gPooAltImageTiles, sizeof(gPooAltImageTiles));
                sWorldInspectDetailSprites[1] = gPooAltImageFrame0;
            } else {
                sWorldInspectDetailPalettes[1] =
                    LoadObjPalette(sWorldinspectMsgs[sWorldInspectWorlds[sWorldInspectCursor]].palette2,
                                   sWorldinspectMsgs[sWorldInspectWorlds[sWorldInspectCursor]].paletteSize2);
                sWorldInspectDetailTiles[1] =
                    LoadObjTiles(sWorldinspectMsgs[sWorldInspectWorlds[sWorldInspectCursor]].tiles2,
                                 sWorldinspectMsgs[sWorldInspectWorlds[sWorldInspectCursor]].tilesSize2);
                sWorldInspectDetailSprites[1] = sWorldinspectMsgs[sWorldInspectWorlds[sWorldInspectCursor]].sprite2;
            }

            sWorldInspectDescTextCount = WorldInspectLoadDesc(sWorldInspectWorlds[sWorldInspectCursor]);
            gBldCnt = (BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2);
            gBldAlpha = BLDALPHA_BLEND(8, 8);
            EnableBg(2);
            EnableBg(3);
            m4aSongNumStart(SONG_SYS_KETTEI);
            sWorldInspectDetailOpen = TRUE;
        }
    } else if (keys & B_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        LoadBgMap(0, gWorldInspectBgMap, sizeof(gWorldInspectBgMap));
#ifndef VERSION_EU
        sWorldInspectBarPalette = LoadObjPalette(gWorldInspectBarPalette, sizeof(gWorldInspectBarPalette));
#endif
        sWorldInspectReturnToMenu = TRUE;
        sWorldInspectSteps = 16;
        sWorldInspectState = WORLD_INSPECT_STATE_TITLE_OUT;
    } else if (keys & START_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        LoadBgMap(0, gWorldInspectBgMap, sizeof(gWorldInspectBgMap));
#ifndef VERSION_EU
        sWorldInspectBarPalette = LoadObjPalette(gWorldInspectBarPalette, sizeof(gWorldInspectBarPalette));
#endif
        sWorldInspectReturnToMenu = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 16);
        sWorldInspectState = WORLD_INSPECT_STATE_EXIT;
    } else if (keys & DPAD_UP) {
        while (1) {
            sWorldInspectCursor = sWorldinspectNavs[sWorldInspectCursor].up;

            if (sWorldInspectCursor == old) {
                break;
            }

            if (sWorldInspectWorlds[sWorldInspectCursor] != 0) {
                break;
            }
        }
    } else if (keys & DPAD_DOWN) {
        while (1) {
            sWorldInspectCursor = sWorldinspectNavs[sWorldInspectCursor].down;

            if (sWorldInspectCursor == old) {
                break;
            }

            if (sWorldInspectWorlds[sWorldInspectCursor] != 0) {
                break;
            }
        }
    } else if (keys & DPAD_LEFT) {
        while (1) {
            sWorldInspectCursor = sWorldinspectNavs[sWorldInspectCursor].left;

            if (sWorldInspectCursor == old) {
                break;
            }

            if (sWorldInspectWorlds[sWorldInspectCursor] != 0) {
                break;
            }
        }
    } else if (keys & DPAD_RIGHT) {
        while (1) {
            sWorldInspectCursor = sWorldinspectNavs[sWorldInspectCursor].right;

            if (sWorldInspectCursor == old) {
                break;
            }

            if (sWorldInspectWorlds[sWorldInspectCursor] != 0) {
                break;
            }
        }
    }

    if (sWorldInspectCursor != old) {
        WorldInspectLoadFloorTiles(sWorldInspectCursor);
        sWorldInspectNameTextCount = WorldInspectLoadName(sWorldInspectWorlds[sWorldInspectCursor]);
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void WorldInspectHandleDetailInput() {
    u16 keys;
    s32 i;

    keys = WorldInspectReadMenuKeys();

    if (keys & (B_BUTTON | START_BUTTON)) {
        m4aSongNumStart(SONG_SYS_CLOSE);

        for (i = 0; i < 2; i++) {
            ReleaseObjPalette(sWorldInspectDetailPalettes[i]);
            ReleaseObjTiles(sWorldInspectDetailTiles[i]);
        }

        if (sWorldInspectCursor <= 9) {
            for (i = 10; i < 12; i++) {
                if (sWorldInspectWorlds[i] != 0) {
                    sWorldInspectIconPalettes[i] = LoadObjPalette(sWorldinspectMsgs[sWorldInspectWorlds[i]].palette,
                                                      sWorldinspectMsgs[sWorldInspectWorlds[i]].paletteSize);
                }
            }
        } else {
            for (i = 4; i < 6; i++) {
                if (sWorldInspectWorlds[i] != 0) {
                    sWorldInspectIconPalettes[i] = LoadObjPalette(sWorldinspectMsgs[sWorldInspectWorlds[i]].palette,
                                                      sWorldinspectMsgs[sWorldInspectWorlds[i]].paletteSize);
                }
            }
        }

        gBldCnt = 0;
        DisableBg(2);
        DisableBg(3);
        sWorldInspectReturnToMenu = TRUE;
        sWorldInspectDetailOpen = FALSE;

#ifdef VERSION_EU
        sWorldInspectBarPalette = LoadObjPalette(gWorldInspectBarPalette, sizeof(gWorldInspectBarPalette));
#endif

        if (keys & START_BUTTON) {
            LoadBgMap(0, gWorldInspectBgMap, sizeof(gWorldInspectBgMap));
#ifndef VERSION_EU
            sWorldInspectBarPalette = LoadObjPalette(gWorldInspectBarPalette, sizeof(gWorldInspectBarPalette));
#endif
            sWorldInspectReturnToMenu = FALSE;
            FadeStartOut(FADE_MODE_BLACK, 16);
            sWorldInspectState = WORLD_INSPECT_STATE_EXIT;
        }
    }
}

void WorldInspectDraw() {
    s32 i;
    u16 prio;
#ifdef VERSION_EU
    void* titleSprite;
    void* topBarSprite;
    void* bottomBarSprite;

    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        titleSprite = gWorldInspectBarFrame0;
        topBarSprite = gWorldInspectBarFrame1;
        bottomBarSprite = gWorldInspectBarFrame2;
        break;
    case LANGUAGE_FRENCH:
        titleSprite = gWorldInspectBarFrenchFrame0;
        topBarSprite = gWorldInspectBarFrenchFrame1;
        bottomBarSprite = gWorldInspectBarFrenchFrame2;
        break;
    case LANGUAGE_SPANISH:
        titleSprite = gWorldInspectBarSpanishFrame0;
        topBarSprite = gWorldInspectBarSpanishFrame1;
        bottomBarSprite = gWorldInspectBarSpanishFrame2;
        break;
    case LANGUAGE_ITALIAN:
        titleSprite = gWorldInspectBarItalianFrame0;
        topBarSprite = gWorldInspectBarItalianFrame1;
        bottomBarSprite = gWorldInspectBarItalianFrame2;
        break;
    case LANGUAGE_GERMAN:
    default:
        titleSprite = gWorldInspectBarGermanFrame0;
        topBarSprite = gWorldInspectBarGermanFrame1;
        bottomBarSprite = gWorldInspectBarGermanFrame2;
        break;
    }
#endif

#ifdef VERSION_EU
    if (sWorldInspectBarPalette != NULL) {
#else
    if (sWorldInspectState != WORLD_INSPECT_STATE_SELECT) {
#endif
        DrawMenuHeader(sWorldInspectBarX >> 8, 0,
#ifdef VERSION_EU
                      titleSprite,
#else
                      gWorldInspectBarFrame0,
#endif
                      sWorldInspectBarTiles, sWorldInspectBarPalette, NULL,
                      SPRITE_PRIORITY(3), 3000);
#ifdef VERSION_EU
    }

    if (sWorldInspectState != WORLD_INSPECT_STATE_SELECT) {
#endif
        DrawSprite(112, sWorldInspectBarY[0] >> 8,
#ifdef VERSION_EU
                      topBarSprite,
#else
                      gWorldInspectBarFrame1,
#endif
                      sWorldInspectBarTiles, sWorldInspectBarPalette, NULL,
                      SPRITE_PRIORITY(3), 3001);
        DrawSprite(112, sWorldInspectBarY[1] >> 8,
#ifdef VERSION_EU
                      bottomBarSprite,
#else
                      gWorldInspectBarFrame2,
#endif
                      sWorldInspectBarTiles, sWorldInspectBarPalette, NULL,
                      SPRITE_PRIORITY(3), 3001);
    }
#if defined(PLATFORM_PC) && !defined(VERSION_EU)
    /* this version stops drawing the title sprite while selecting: the BG copy stays, the HD banner goes over it */
    else {
        int level = (int)(gFadeWork->amount >> 8) * 255 / 31;
        PortHdStillSprite(gWorldInspectBarTiles, gWorldInspectBarFrame0, sWorldInspectBarX >> 8, 0,
                          gFadeWork->mode == FADE_MODE_BLACK ? level : 0,
                          gFadeWork->mode == FADE_MODE_WHITE || gFadeWork->mode == FADE_MODE_ADD_WHITE ? level : 0);
    }
#endif

    prio = SPRITE_PRIORITY(1);

    if (sWorldInspectDetailOpen == TRUE) {
        prio |= SPRITE_FLAG_BLEND;
    }

    if (sWorldInspectState == WORLD_INSPECT_STATE_SELECT) {
        DrawSprite(sWorldinspectNavs[sWorldInspectCursor].x * 8 + 22,
                      sWorldinspectNavs[sWorldInspectCursor].y * 8 + 12,
                      AnimUpdate(&sWorldInspectHighlightAnim), sWorldInspectHighlightTiles, sWorldInspectHighlightPalette, NULL, prio, 2013);
        ApproachValueHalf(&sWorldInspectCursorX,
                      (sWorldinspectNavs[sWorldInspectCursor].x << 11) + 0x2000);
        ApproachValueHalf(&sWorldInspectCursorY,
                      (sWorldinspectNavs[sWorldInspectCursor].y << 11) + 0xFFFFFA00);
        DrawSprite(sWorldInspectCursorX >> 8, sWorldInspectCursorY >> 8, AnimUpdate(&sWorldInspectCursorAnim),
                      sWorldInspectCursorTiles, sWorldInspectCursorPalette, NULL, prio, 2000);
    }

    for (i = 0; i < 12; i++) {
        if (sWorldInspectIconSprites[i] != NULL && sWorldInspectIconPalettes[i] != NULL) {
            DrawSprite(sWorldinspectNavs[i].x * 8 + 16, sWorldinspectNavs[i].y * 8 + 16,
                          sWorldInspectIconSprites[i], sWorldInspectIconTiles[i], sWorldInspectIconPalettes[i], NULL, prio,
                          i + 2001);
        }
    }

    if (sWorldInspectIconSprites[sWorldInspectCursor] != NULL) {
        DrawSprite(
#ifdef VERSION_EU
                      102,
#else
                      112,
#endif
                      32, sWorldInspectIconSprites[sWorldInspectCursor], sWorldInspectIconTiles[sWorldInspectCursor],
                      sWorldInspectIconPalettes[sWorldInspectCursor], NULL, 0, 0);
    }

    if (sWorldInspectNameTextCount != 0) {
        DrawTextSlots(
#ifdef VERSION_EU
                      120,
#else
                      128,
#endif
                      28, sWorldInspectNameText, sWorldInspectCursorPalette, 1, sWorldInspectNameTextCount);
    }

    if (sWorldInspectDetailOpen == TRUE) {
        if (sWorldInspectDescTextCount != 0) {
            DrawTextSlots(97, 56, sWorldInspectDescText, sWorldInspectHighlightPalette, 0, sWorldInspectDescTextCount);
        }

        DrawSprite(47, (-gSineTable[sWorldInspectBobPhase + 0x40] >> 5) + 84, sWorldInspectDetailSprites[0],
                      sWorldInspectDetailTiles[0], sWorldInspectDetailPalettes[0], NULL, 0, 1);
        DrawSprite(47, (-gSineTable[sWorldInspectBobPhase + 0x40] >> 5) + 84, sWorldInspectDetailSprites[1],
                      sWorldInspectDetailTiles[1], sWorldInspectDetailPalettes[1], NULL, 0, 0);
    }
}

void mode_worldinspect_0() {
#ifdef PLATFORM_PC
    PortSideStyle(PORT_SIDE_BLUR_BANDS); /* blurred reflection, its bars drawn on */
#endif
    s16 i;
    s32 floor;
    s16 id;
    void** tilemapPtr;
    u32 floorKeep;
    s16 slot;

    tilemapPtr = &sWorldInspectTilemap;
    *tilemapPtr = EwramAlloc(0x500);
    SpriteReset();
    FadeStartIn(FADE_MODE_BLACK, 16);
    SetBgMode0();
    SetupBg(0, 0, 0x1C, 0);
    SetupBg(1, 0, 0x1D, 0);
    SetupBg(2, 0, 0x1E, 0);
    SetupBg(3, 0, 0x1F, 0);
    SetBgPriority(0, 3);
    SetBgPriority(1, 2);
    SetBgPriority(2, 1);
    SetBgPriority(3, 0);
    floor = gGameState.floor;
    floorKeep = (u16)floor;
    sWorldInspectCursor = gGameState.floor > 11 ? 11 : floor;

    sWorldInspectFloorCount = 0;

    for (i = 0; i <= 11; i++) {
        sWorldInspectWorlds[i] = gGameState.floors[i].world;

        if (sWorldInspectWorlds[i] != 0) {
            sWorldInspectFloorCount = i + 1;
        }
    }

    for (i = 0, slot = sWorldInspectFloorCount; i <= 13; i++) {
        if ((gGameState.availableWorlds & sWorldinspectMsgs[i].flags) != 0) {
#ifdef PLATFORM_PC
            /* The list is full: on PC the next variable is not the GBA's. */
            if (slot > 11) {
                break;
            }
#endif
            // @bug Writes past the end of sWorldInspectWorlds when the floors fill it.
            sWorldInspectWorlds[slot++] = sWorldinspectMsgs[i].world;

            if (slot > 11) {
                break;
            }
        }
    }

    sWorldInspectDetailOpen = FALSE;
    sWorldInspectBobPhase = 0;
    sWorldInspectCursorX = (sWorldinspectNavs[sWorldInspectCursor].x << 11) + 0x2000;
    sWorldInspectCursorY = (sWorldinspectNavs[sWorldInspectCursor].y << 11) - 0x600;
    sWorldInspectState = WORLD_INSPECT_STATE_BARS_IN;
    sWorldInspectSteps = 16;
    sWorldInspectBarY[0] = -0x800;
    sWorldInspectBarY[1] = 0xA800;
    sWorldInspectBarX = -0x8000;
    LoadBgPalette(0, gWorldInspectBgPalettes, sizeof(gWorldInspectBgPalettes));

    for (i = 0; i <= 11; i++) {
        if (sWorldInspectWorlds[i] != 0) {
            id = sWorldInspectWorlds[i];
            sWorldInspectIconPalettes[i] = LoadObjPalette(sWorldinspectMsgs[id].palette, sWorldinspectMsgs[id].paletteSize);
            id = sWorldInspectWorlds[i];
            sWorldInspectIconTiles[i] = LoadObjTiles(sWorldinspectMsgs[id].tiles, sWorldinspectMsgs[id].tilesSize);
            id = sWorldInspectWorlds[i];
            sWorldInspectIconSprites[i] = sWorldinspectMsgs[id].sprite;
        } else {
            sWorldInspectIconPalettes[i] = NULL;
            sWorldInspectIconTiles[i] = NULL;
            sWorldInspectIconSprites[i] = NULL;
        }
    }

    LoadBgTiles(0, gWorldInspectTiles, sizeof(gWorldInspectTiles));

#ifdef VERSION_EU
    if (gGameState.flags & GAME_FLAG_RIKU) {
        switch (gLanguage) {
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gWorldInspectRikuFloorNumberFrenchTiles, (u8*)GetBgCharBase(0) + 0x6400, sizeof(gWorldInspectRikuFloorNumberFrenchTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gWorldInspectRikuFloorNumberSpanishTiles, (u8*)GetBgCharBase(0) + 0x6400, sizeof(gWorldInspectRikuFloorNumberSpanishTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gWorldInspectRikuFloorNumberItalianTiles, (u8*)GetBgCharBase(0) + 0x6400, sizeof(gWorldInspectRikuFloorNumberItalianTiles));
            break;
        case LANGUAGE_GERMAN:
            // @bug Reads past the end of gWorldInspectRikuFloorNumberGermanTiles.
            RequestDma3Copy(gWorldInspectRikuFloorNumberGermanTiles, (u8*)GetBgCharBase(0) + 0x6400, 0x800);
            break;
        }
    } else {
        switch (gLanguage) {
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gWorldInspectFloorNumberFrenchTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldInspectFloorNumberFrenchTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gWorldInspectFloorNumberSpanishTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldInspectFloorNumberSpanishTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gWorldInspectFloorNumberItalianTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldInspectFloorNumberItalianTiles));
            break;
        case LANGUAGE_GERMAN:
            RequestDma3Copy(gWorldInspectFloorNumberGermanTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gWorldInspectFloorNumberGermanTiles));
            break;
        }
    }
#endif

    LoadBgMap(0, gWorldInspectBgMap, sizeof(gWorldInspectBgMap));
    DmaCopy16(3, gWorldInspectFloorMap, sWorldInspectTilemap, sizeof(gWorldInspectFloorMap));

    for (i = 0; i <= 11; i++) {
        if (sWorldInspectWorlds[i] != 0) {
            WorldInspectCopyTilemapRect(7, 4, gWorldInspectFloorPartsMap, 0, 0, sWorldInspectTilemap, sWorldinspectNavs[i].x, sWorldinspectNavs[i].y);
        }
    }

    for (i = 0; i < sWorldInspectFloorCount - 1; i++) {
        if (sWorldInspectWorlds[i] != 0 && sWorldInspectWorlds[i + 1] != 0) {
            const WorldinspectConn* conn = sWorldinspectConns;
            id = sWorldinspectNavs[i].rect;
            WorldInspectCopyTilemapRect(conn[id].width, conn[id].height, gWorldInspectFloorPartsMap, conn[id].x, conn[id].y, sWorldInspectTilemap, sWorldinspectNavs[i].x2, sWorldinspectNavs[i].y2);
        }
    }

    for (i = 0; i < sWorldInspectFloorCount; i++) {
        if (sWorldInspectWorlds[i] != 0) {
            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                if (i <= 8) {
                    WorldInspectCopyTilemapRect(3, 1, gWorldInspectFloorPartsMap, i * 3, 9, sWorldInspectTilemap, sWorldinspectNavs[i].x + 3, sWorldinspectNavs[i].y + 2);
                } else {
                    WorldInspectCopyTilemapRect(4, 1, gWorldInspectFloorPartsMap, (i - 9) * 4, 10, sWorldInspectTilemap, sWorldinspectNavs[i].x + 3, sWorldinspectNavs[i].y + 2);
                }
            } else {
                if (i <= 2) {
                    WorldInspectCopyTilemapRect(4, 1, gWorldInspectFloorPartsMap, i * 4, 0x15, sWorldInspectTilemap, sWorldinspectNavs[i].x + 3, sWorldinspectNavs[i].y + 2);
                } else {
                    WorldInspectCopyTilemapRect(3, 1, gWorldInspectFloorPartsMap, (i - 3) * 3, 0x16, sWorldInspectTilemap, sWorldinspectNavs[i].x + 3, sWorldinspectNavs[i].y + 2);
                }
            }
        }
    }

    for (i = 0; i < sWorldInspectFloorCount; i++) {
        if ((s16)floorKeep == i) {
            WorldInspectSetTilemapRectPalette(3, 7, 4, sWorldInspectTilemap, sWorldinspectNavs[i].x, sWorldinspectNavs[i].y);
        } else {
            WorldInspectSetTilemapRectPalette(2, 7, 4, sWorldInspectTilemap, sWorldinspectNavs[i].x, sWorldinspectNavs[i].y);
        }
    }

    LoadBgMap(1, sWorldInspectTilemap, 0x500);
    LoadBgMap(2, gWorldInspectShadeMap, sizeof(gWorldInspectShadeMap));
    LoadBgMap(3, gWorldInspectDetailMap, sizeof(gWorldInspectDetailMap));
    WorldInspectLoadFloorTiles(sWorldInspectCursor);
    sWorldInspectHighlightPalette = LoadObjPalette(gWorldInspectHighlightPalette, sizeof(gWorldInspectHighlightPalette));
    sWorldInspectHighlightTiles = LoadObjTiles(gWorldInspectHighlightTiles, sizeof(gWorldInspectHighlightTiles));
    AnimInit(&sWorldInspectHighlightAnim, gWorldInspectHighlightAnims, gWorldInspectHighlightFrames);
    AnimStart(&sWorldInspectHighlightAnim, 0, ANIM_FLAG_LOOP);
    sWorldInspectCursorPalette = LoadObjPalette(gWorldInspectCursorPalette, sizeof(gWorldInspectCursorPalette));
    sWorldInspectCursorTiles = LoadObjTiles(gWorldInspectCursorTiles, sizeof(gWorldInspectCursorTiles));
    AnimInit(&sWorldInspectCursorAnim, gWorldInspectCursorAnims, gWorldInspectCursorFrames);
    AnimStart(&sWorldInspectCursorAnim, 0, ANIM_FLAG_LOOP);
    InitTextSlots(sWorldInspectNameText, ARRAY_COUNT(sWorldInspectNameText));
    sWorldInspectNameTextCount = WorldInspectLoadName(sWorldInspectWorlds[sWorldInspectCursor]);
    InitTextSlots(sWorldInspectDescText, ARRAY_COUNT(sWorldInspectDescText));
    sWorldInspectBarPalette = LoadObjPalette(gWorldInspectBarPalette, sizeof(gWorldInspectBarPalette));

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        sWorldInspectBarTiles = LoadObjTiles(gWorldInspectBarTiles, sizeof(gWorldInspectBarTiles));
        break;
    case LANGUAGE_FRENCH:
        sWorldInspectBarTiles = LoadObjTiles(gWorldInspectBarFrenchTiles, sizeof(gWorldInspectBarFrenchTiles));
        break;
    case LANGUAGE_SPANISH:
        sWorldInspectBarTiles = LoadObjTiles(gWorldInspectBarSpanishTiles, sizeof(gWorldInspectBarSpanishTiles));
        break;
    case LANGUAGE_ITALIAN:
        sWorldInspectBarTiles = LoadObjTiles(gWorldInspectBarItalianTiles, sizeof(gWorldInspectBarItalianTiles));
        break;
    case LANGUAGE_GERMAN:
    default:
        sWorldInspectBarTiles = LoadObjTiles(gWorldInspectBarGermanTiles, sizeof(gWorldInspectBarGermanTiles));
        break;
    }
#else
    sWorldInspectBarTiles = LoadObjTiles(gWorldInspectBarTiles, sizeof(gWorldInspectBarTiles));
#endif
    EnableBg(0);
    EnableBg(1);
    DisableBg(2);
    DisableBg(3);
}

void mode_worldinspect_1() {
    UpdatePlayTime();
    sWorldInspectBobPhase += 2;

    switch (sWorldInspectState) {
    case WORLD_INSPECT_STATE_BARS_IN:
        ApproachValue(&sWorldInspectBarY[0], 0, sWorldInspectSteps);
        ApproachValue(&sWorldInspectBarY[1], 0x9800, sWorldInspectSteps);
        sWorldInspectSteps--;

        if (sWorldInspectSteps <= 0) {
            sWorldInspectSteps = 16;
            sWorldInspectState = WORLD_INSPECT_STATE_TITLE_IN;
        }

        break;
    case WORLD_INSPECT_STATE_TITLE_IN:
        ApproachValue(&sWorldInspectBarX, 0, sWorldInspectSteps);
        sWorldInspectSteps--;

        if (sWorldInspectSteps <= 0) {
            LoadBgMap(0, gWorldInspectBgHeaderMap, sizeof(gWorldInspectBgHeaderMap));
#ifndef VERSION_EU
            ReleaseObjPalette(sWorldInspectBarPalette);
#endif
            sWorldInspectState = WORLD_INSPECT_STATE_SELECT;
        }

        break;
    case WORLD_INSPECT_STATE_SELECT:
        switch (sWorldInspectDetailOpen) {
        case FALSE:
            WorldInspectHandleInput();
            break;
        case TRUE:
            WorldInspectHandleDetailInput();
            break;
        }

        break;
    case WORLD_INSPECT_STATE_TITLE_OUT:
        ApproachValue(&sWorldInspectBarX, -0x8000, sWorldInspectSteps);
        sWorldInspectSteps--;

        if (sWorldInspectSteps <= 0) {
            sWorldInspectSteps = 16;
            sWorldInspectState = WORLD_INSPECT_STATE_BARS_OUT;
        }

        break;
    case WORLD_INSPECT_STATE_BARS_OUT:
        ApproachValue(&sWorldInspectBarY[0], -0x800, sWorldInspectSteps);
        ApproachValue(&sWorldInspectBarY[1], 0xA800, sWorldInspectSteps);
        sWorldInspectSteps--;

        if (sWorldInspectSteps <= 0) {
            FadeStartOut(FADE_MODE_BLACK, 16);
            sWorldInspectState = WORLD_INSPECT_STATE_EXIT;
        }

        break;
    case WORLD_INSPECT_STATE_EXIT:
        if (!FadeIsActive()) {
            ReturnToMap(sWorldInspectReturnToMenu);
        }

        break;
    }

    WorldInspectDraw();
}

void mode_worldinspect_2() {
    s32 i;

    ReleaseObjPalette(sWorldInspectBarPalette);
    ReleaseObjTiles(sWorldInspectBarTiles);
    ReleaseObjPalette(sWorldInspectHighlightPalette);
    ReleaseObjTiles(sWorldInspectHighlightTiles);
    ReleaseObjPalette(sWorldInspectCursorPalette);
    ReleaseObjTiles(sWorldInspectCursorTiles);

    for (i = 0; i < 12; i++) {
        if (sWorldInspectIconPalettes[i]) {
            ReleaseObjPalette(sWorldInspectIconPalettes[i]);
        }

        if (sWorldInspectIconTiles[i]) {
            ReleaseObjTiles(sWorldInspectIconTiles[i]);
        }
    }

    FreeTextSlots(sWorldInspectNameText, ARRAY_COUNT(sWorldInspectNameText));
    FreeTextSlots(sWorldInspectDescText, ARRAY_COUNT(sWorldInspectDescText));
    EwramFree(sWorldInspectTilemap);
}

Mode gModeWorldinspect = {
    "mode_worldinspect",
    (ModeInitFunc)mode_worldinspect_0,
    mode_worldinspect_1,
    mode_worldinspect_2,
};
