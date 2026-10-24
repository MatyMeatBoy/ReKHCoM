/**
 * mode_worldselect.c
 * World Select Screen
 */

#include "system_state.h"
#include "map_api.h"
#include "msg_api.h"
#include "display.h"
#include "fade.h"
#include "mode_worldselect.h"
#include "sprites_bos5.h"
#include "sprites_worldinspect.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "malloc.h"
#include "songs.h"
#include "world_types.h"
#include "bg_animation_data.h"
#include "card_api.h"
#include "engine_math.h"
#include "game_state.h"
#include "gba/defines.h"
#include "key.h"
#include "m4a_song.h"
#include "map_runtime.h"
#include "mode.h"
#include "obj.h"
#include "obj_api.h"
#include "poo_api.h"
#include "taskpool.h"
#include "types.h"
#include "gba/macro.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "card_ids.h"
#include "card_message_data.h"
#include "event_ids.h"
#include "player_progression_types.h"
#ifdef PLATFORM_PC
#include "port/port_hdsprites.h"
#endif

#ifdef VERSION_EU
static void* sWorldselectBg1Maps[5] = {
    gWorldselectTitleMap,
    gWorldselectTitleFrenchMap,
    gWorldselectTitleGermanMap,
    gWorldselectTitleItalianMap,
    gWorldselectTitleSpanishMap,
};

static void* sWorldselectNameTileData[5] = {
    gWorldselectNameWonderlandTiles,
    gWorldselectNameFrenchTiles,
    gWorldselectNameGermanTiles,
    gWorldselectNameItalianTiles,
    gWorldselectNameSpanishTiles,
};

static void* sWorldselectTitleGfx[5] = {
    gWorldselectTitleFrame0,
    gWorldselectTitleFrenchFrame0,
    gWorldselectTitleGermanFrame0,
    gWorldselectTitleItalianFrame0,
    gWorldselectTitleSpanishFrame0,
};

static void* sWorldselectTitleTileData[5] = {
    gWorldselectTitleTiles,
    gWorldselectTitleFrenchTiles,
    gWorldselectTitleGermanTiles,
    gWorldselectTitleItalianTiles,
    gWorldselectTitleSpanishTiles,
};
#endif

static void* sWorldselectPaletteCycle[30] = {
    gWorldselectCycle00Palette,
    gWorldselectCycle01Palette,
    gWorldselectCycle02Palette,
    gWorldselectCycle03Palette,
    gWorldselectCycle04Palette,
    gWorldselectCycle05Palette,
    gWorldselectCycle06Palette,
    gWorldselectCycle07Palette,
    gWorldselectCycle08Palette,
    gWorldselectCycle09Palette,
    gWorldselectCycle10Palette,
    gWorldselectCycle11Palette,
    gWorldselectCycle12Palette,
    gWorldselectCycle13Palette,
    gWorldselectCycle14Palette,
    gWorldselectCycle15Palette,
    gWorldselectCycle14Palette,
    gWorldselectCycle13Palette,
    gWorldselectCycle12Palette,
    gWorldselectCycle11Palette,
    gWorldselectCycle10Palette,
    gWorldselectCycle09Palette,
    gWorldselectCycle08Palette,
    gWorldselectCycle07Palette,
    gWorldselectCycle06Palette,
    gWorldselectCycle05Palette,
    gWorldselectCycle04Palette,
    gWorldselectCycle03Palette,
    gWorldselectCycle02Palette,
    gWorldselectCycle01Palette,
};

#ifdef VERSION_EU
static const WorldselectTileSizes sWorldselectTitleTileSizes = { { 896, 960, 1024, 1024, 960 } };
#endif

static const WorldselectWorldDef sWorldselectWorldDefs[13] = {
#ifdef VERSION_EU
    { 1, WORLD_AGRABAH, EVENT_107_AGRABAH_E0, -1, gWorldImageAgrabahPalette, gWorldImageAgrabahTiles, gWorldImageAgrabahFrame0, sWorldselectNameTileData, 8192 },
#else
    { 1, WORLD_AGRABAH, EVENT_107_AGRABAH_E0, -1, gWorldImageAgrabahPalette, gWorldImageAgrabahTiles, gWorldImageAgrabahFrame0, gWorldselectNameAgrabahTiles },
#endif
#ifdef VERSION_EU
    { 2, WORLD_ATLANTICA, EVENT_101_ATLANTICA_E0, -1, gWorldImageAtlanticaPalette, gWorldImageAtlanticaTiles, gWorldImageAtlanticaFrame0, sWorldselectNameTileData, 12288 },
#else
    { 2, WORLD_ATLANTICA, EVENT_101_ATLANTICA_E0, -1, gWorldImageAtlanticaPalette, gWorldImageAtlanticaTiles, gWorldImageAtlanticaFrame0, gWorldselectNameAtlanticaTiles },
#endif
#ifdef VERSION_EU
    { 4, WORLD_OLYMPUS_COLISEUM, EVENT_120_COLISEUM_E0, -1, gWorldImageOlympusColiseumPalette, gWorldImageOlympusColiseumTiles, gWorldImageOlympusColiseumFrame0, sWorldselectNameTileData, 6144 },
#else
    { 4, WORLD_OLYMPUS_COLISEUM, EVENT_120_COLISEUM_E0, -1, gWorldImageOlympusColiseumPalette, gWorldImageOlympusColiseumTiles, gWorldImageOlympusColiseumFrame0, gWorldselectNameOlympusColiseumTiles },
#endif
#ifdef VERSION_EU
    { 8, WORLD_WONDERLAND, EVENT_094_WONDERLAND_E0, -1, gWorldImageWonderlandPalette, gWorldImageWonderlandTiles, gWorldImageWonderlandFrame0, sWorldselectNameTileData, 0 },
#else
    { 8, WORLD_WONDERLAND, EVENT_094_WONDERLAND_E0, -1, gWorldImageWonderlandPalette, gWorldImageWonderlandTiles, gWorldImageWonderlandFrame0, gWorldselectNameWonderlandTiles },
#endif
#ifdef VERSION_EU
    { 16, WORLD_MONSTRO, EVENT_074_MONSTORO_E0, -1, gWorldImageMonstroPalette, gWorldImageMonstroTiles, gWorldImageMonstroFrame0, sWorldselectNameTileData, 10240 },
#else
    { 16, WORLD_MONSTRO, EVENT_074_MONSTORO_E0, -1, gWorldImageMonstroPalette, gWorldImageMonstroTiles, gWorldImageMonstroFrame0, gWorldselectNameMonstroTiles },
#endif
#ifdef VERSION_EU
    { 32, WORLD_HALLOWEEN_TOWN, EVENT_087_HALLOWEEN_TOWN_E0, -1, gWorldImageHalloweenTownPalette, gWorldImageHalloweenTownTiles, gWorldImageHalloweenTownFrame0, sWorldselectNameTileData, 14336 },
#else
    { 32, WORLD_HALLOWEEN_TOWN, EVENT_087_HALLOWEEN_TOWN_E0, -1, gWorldImageHalloweenTownPalette, gWorldImageHalloweenTownTiles, gWorldImageHalloweenTownFrame0, gWorldselectNameHalloweenTownTiles },
#endif
#ifdef VERSION_EU
    { 64, WORLD_NEVER_LAND, EVENT_115_NEVERLAND_E0, -1, gWorldImageNeverLandPalette, gWorldImageNeverLandTiles, gWorldImageNeverLandFrame0, sWorldselectNameTileData, 16384 },
#else
    { 64, WORLD_NEVER_LAND, EVENT_115_NEVERLAND_E0, -1, gWorldImageNeverLandPalette, gWorldImageNeverLandTiles, gWorldImageNeverLandFrame0, gWorldselectNameNeverLandTiles },
#endif
#ifdef VERSION_EU
    { 128, WORLD_HOLLOW_BASTION, EVENT_129_HOLLOWBASTION_E0, EVENT_151_RIKU_B12F_E0, gWorldImageHollowBastionPalette, gWorldImageHollowBastionTiles, gWorldImageHollowBastionFrame0, sWorldselectNameTileData, 20480 },
#else
    { 128, WORLD_HOLLOW_BASTION, EVENT_129_HOLLOWBASTION_E0, EVENT_151_RIKU_B12F_E0, gWorldImageHollowBastionPalette, gWorldImageHollowBastionTiles, gWorldImageHollowBastionFrame0, gWorldselectNameHollowBastionTiles },
#endif
#ifdef VERSION_EU
    { 256, WORLD_DESTINY_ISLANDS, EVENT_053_12F_DESTINY_ISLAND_E0, EVENT_177_RIKU_B3F_E0, gWorldImageDestinyIslandsPalette, gWorldImageDestinyIslandsTiles, gWorldImageDestinyIslandsFrame0, sWorldselectNameTileData, 2048 },
#else
    { 256, WORLD_DESTINY_ISLANDS, EVENT_053_12F_DESTINY_ISLAND_E0, EVENT_177_RIKU_B3F_E0, gWorldImageDestinyIslandsPalette, gWorldImageDestinyIslandsTiles, gWorldImageDestinyIslandsFrame0, gWorldselectNameDestinyIslandsTiles },
#endif
#ifdef VERSION_EU
    { 512, WORLD_TRAVERSE_TOWN, EVENT_001_1F_ENTRANCE_PART2, -1, gWorldImageTraverseTownPalette, gWorldImageTraverseTownTiles, gWorldImageTraverseTownFrame0, sWorldselectNameTileData, 4096 },
#else
    { 512, WORLD_TRAVERSE_TOWN, EVENT_001_1F_ENTRANCE_PART2, -1, gWorldImageTraverseTownPalette, gWorldImageTraverseTownTiles, gWorldImageTraverseTownFrame0, gWorldselectNameTraverseTownTiles },
#endif
#ifdef VERSION_EU
    { 2048, WORLD_TWILIGHT_TOWN, EVENT_044_11F_TWILIGHT_TOWN_E0, EVENT_186_RIKU_B2F_E0, gWorldImageTwilightTownPalette, gWorldImageTwilightTownTiles, gWorldImageTwilightTownFrame0, sWorldselectNameTileData, 22528 },
#else
    { 2048, WORLD_TWILIGHT_TOWN, EVENT_044_11F_TWILIGHT_TOWN_E0, EVENT_186_RIKU_B2F_E0, gWorldImageTwilightTownPalette, gWorldImageTwilightTownTiles, gWorldImageTwilightTownFrame0, gWorldselectNameTwilightTownTiles },
#endif
#ifdef VERSION_EU
    { 4096, WORLD_CASTLE_OBLIVION, EVENT_061_13F_ENTRANCE, EVENT_192_RIKU_B1F_E0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, gWorldImageCastleOblivionFrame0, sWorldselectNameTileData, 24576 },
#else
    { 4096, WORLD_CASTLE_OBLIVION, EVENT_061_13F_ENTRANCE, EVENT_192_RIKU_B1F_E0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, gWorldImageCastleOblivionFrame0, gWorldselectNameCastleOblivionTiles },
#endif
#ifdef VERSION_EU
    { 1024, WORLD_100_ACRE_WOOD, EVENT_134_100ACREWOOD_START, -1, gWorldImage100AcreWoodPalette, gWorldImage100AcreWoodTiles, gWorldImage100AcreWoodFrame0, sWorldselectNameTileData, 18432 },
#else
    { 1024, WORLD_100_ACRE_WOOD, EVENT_134_100ACREWOOD_START, -1, gWorldImage100AcreWoodPalette, gWorldImage100AcreWoodTiles, gWorldImage100AcreWoodFrame0, gWorldselectName100AcreWoodTiles },
#endif
};

Mode gModeWorldselect = {
    "mode_worldselect",
    (ModeInitFunc)mode_worldselect_0,
    mode_worldselect_1,
    mode_worldselect_2,
};

enum WorldselectStep {
    WORLDSELECT_STEP_BARS_IN,
    WORLDSELECT_STEP_TITLE_IN,
    WORLDSELECT_STEP_TUTORIAL,
    WORLDSELECT_STEP_SELECT,
    WORLDSELECT_STEP_START_ANIM,
    WORLDSELECT_STEP_TITLE_OUT,
    WORLDSELECT_STEP_BARS_OUT,
    WORLDSELECT_STEP_FADE_OUT,
    WORLDSELECT_STEP_WAIT_FADE,
    WORLDSELECT_STEP_ENTER_WORLD
};

enum WorldselectRotation {
    WORLDSELECT_ROTATION_NONE,
    WORLDSELECT_ROTATION_RIGHT,
    WORLDSELECT_ROTATION_LEFT
};

enum WorldselectNameMode {
    WORLDSELECT_NAME_MODE_IDLE,
    WORLDSELECT_NAME_MODE_SHOW,
    WORLDSELECT_NAME_MODE_HIDE
};

static s16 sWorldselectCursor;
static WorldselectSlot sWorldselectSlots[5];
static s16 sWorldselectWorlds[14];
static u32 sWorldselectRotation;
static s16 sWorldselectSlotCount;
static s16 sWorldselectWorldCount;
static void* sWorldselectCardTiles[2];
static void* sWorldselectCardPalettes[2];
static struct ObjTiles* sWorldselectTitleTiles;
static struct ObjPalette* sWorldselectOverlayPalette;
static struct ObjTiles* sWorldselectFrameTiles;
static u8 sWorldselectBobPhase;
static s16 sWorldselectNameMode;
static s16 sWorldselectNameWidth;
static s16 sWorldselectNameWorld;
static void* sWorldselectNameBuffer;
static s16 sWorldselectStep;
static s16 sWorldselectTimer;
static s32 sWorldselectFrameY[2];
static s32 sWorldselectTitleX;
static TaskPool sWorldselectTaskPool;
static s16 sWorldselectTutorialStep;
static u8 sWorldselectFirstVisit;
static u8 sWorldselectBgAnimActive;
static u8 sWorldselectCancelled;
static s16 sWorldselectPaletteTimer;
static u16 sWorldselectPaletteFrame;

void WorldselectLoadSlotPalette(s16 model, s16 slot) {
    void* src;
    s32 size;

    if (sWorldselectWorldDefs[model].world == WORLD_100_ACRE_WOOD && IsPooAltImageActive()) {
        src = gPooAltImagePalettes;
        size = 0x40;
    } else {
        src = sWorldselectWorldDefs[model].palette;
        size = 0x20;
    }

    sWorldselectSlots[slot].palette = LoadObjPalette(src, size);
}

void WorldselectLoadSlotTiles(s16 model, s16 slot) {
    void* src;

    if (sWorldselectWorldDefs[model].world == WORLD_100_ACRE_WOOD && IsPooAltImageActive()) {
        src = gPooAltImageTiles;
    } else {
        src = sWorldselectWorldDefs[model].tiles;
    }

    sWorldselectSlots[slot].tiles = LoadObjTiles(src, 0x1000);
}

s16 WorldselectSetSlotGfx(s16 model, s16 slot) {
    if (sWorldselectWorldDefs[model].world == WORLD_100_ACRE_WOOD && IsPooAltImageActive()) {
        sWorldselectSlots[slot].gfx = gPooAltImageFrame0;
    } else {
        sWorldselectSlots[slot].gfx = sWorldselectWorldDefs[model].gfx;
    }
}

void WorldselectDrawName(s16 model, s16 width) {
    u8* src;
    u8* srcRight;

    DmaFill16(3, 0, sWorldselectNameBuffer, 0x6C0);

    if (width > 0) {
#ifdef VERSION_EU
        src = ((u8**)sWorldselectWorldDefs[model].nameTiles)[gLanguage];
        src += sWorldselectWorldDefs[model].nameTilesOffset;
#else
        src = sWorldselectWorldDefs[model].nameTiles;
#endif
        DmaCopy16(3, src, (u8*)sWorldselectNameBuffer + (9 - width) * 32, width * 32);
        srcRight = src + (18 - width) * 32;
        DmaCopy16(3, srcRight, (u8*)sWorldselectNameBuffer + 288, width * 32);
        DmaCopy16(3, src + 576, (u8*)sWorldselectNameBuffer + (9 - width) * 32 + 576, width * 32);
        DmaCopy16(3, srcRight + 576, (u8*)sWorldselectNameBuffer + 864, width * 32);
        DmaCopy16(3, src + 1152, (u8*)sWorldselectNameBuffer + (9 - width) * 32 + 1152, width * 32);
        DmaCopy16(3, srcRight + 1152, (u8*)sWorldselectNameBuffer + 1440, width * 32);
    }

    RequestDma3Copy(sWorldselectNameBuffer, (u8*)GetBgCharBase(0) + 1024, 0x6C0);
}

void WorldselectHandleInput() {
    s16 i;
    s16 slot;
    s16 listIndex;
    u8 step;

    switch (sWorldselectRotation) {
    case WORLDSELECT_ROTATION_NONE:
        if (GetKeysPressed() & A_BUTTON) {
            BgAnimInit(2, BGCNT_AFF512x512, BGCNT_256COLOR);
            BgAnimStart(&gBgAnimDefWorldStart, 112, 126);
            SetBgPriority(2, 1);
            gBldCnt |= BLDCNT_TGT2_OBJ;
            sWorldselectBgAnimActive = TRUE;
            m4aSongNumStart(SONG_SYS_WORLDSTART);
            sWorldselectCancelled = FALSE;
            sWorldselectStep = WORLDSELECT_STEP_START_ANIM;
        } else if ((GetKeysPressed() & B_BUTTON) && sWorldselectFirstVisit == 0) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            LoadBgMap(0, gWorldselectGlowMap, sizeof(gWorldselectGlowMap));
            LoadBgMap(1, gWorldselectBarMap, sizeof(gWorldselectBarMap));
            sWorldselectCancelled = TRUE;
            sWorldselectTimer = 16;
            sWorldselectStep = WORLDSELECT_STEP_TITLE_OUT;
        } else if (sWorldselectSlotCount > 1) {
            if (GetKeysHeld() & DPAD_LEFT) {
                slot = sWorldselectCursor - sWorldselectSlotCount / 2;

                while (slot < 0) {
                    slot += sWorldselectSlotCount;
                }

                ReleaseObjPalette(sWorldselectSlots[slot].palette);
                ReleaseObjTiles(sWorldselectSlots[slot].tiles);
                listIndex = sWorldselectSlots[sWorldselectCursor].listIndex - sWorldselectSlotCount / 2;

                while (listIndex < 0) {
                    listIndex += sWorldselectWorldCount;
                }

                sWorldselectSlots[slot].listIndex = listIndex;
                WorldselectLoadSlotPalette(sWorldselectWorlds[listIndex], slot);
                WorldselectLoadSlotTiles(sWorldselectWorlds[listIndex], slot);
                WorldselectSetSlotGfx(sWorldselectWorlds[listIndex], slot);
                sWorldselectCursor--;

                if (sWorldselectCursor < 0) {
                    sWorldselectCursor = sWorldselectSlotCount - 1;
                }

                m4aSongNumStart(SONG_SYS_WORLDSELECT);
                sWorldselectNameMode = WORLDSELECT_NAME_MODE_HIDE;
                sWorldselectRotation = WORLDSELECT_ROTATION_LEFT;
            } else if (GetKeysHeld() & DPAD_RIGHT) {
                slot = sWorldselectCursor + sWorldselectSlotCount / 2;

                while (slot >= sWorldselectSlotCount) {
                    slot -= sWorldselectSlotCount;
                }

                ReleaseObjPalette(sWorldselectSlots[slot].palette);
                ReleaseObjTiles(sWorldselectSlots[slot].tiles);
                listIndex = sWorldselectSlots[sWorldselectCursor].listIndex + sWorldselectSlotCount / 2;

                while (listIndex >= sWorldselectWorldCount) {
                    listIndex -= sWorldselectWorldCount;
                }

                sWorldselectSlots[slot].listIndex = listIndex;
                WorldselectLoadSlotPalette(sWorldselectWorlds[listIndex], slot);
                WorldselectLoadSlotTiles(sWorldselectWorlds[listIndex], slot);
                WorldselectSetSlotGfx(sWorldselectWorlds[listIndex], slot);
                sWorldselectCursor++;

                if (sWorldselectCursor >= sWorldselectSlotCount) {
                    sWorldselectCursor = 0;
                }

                m4aSongNumStart(SONG_SYS_WORLDSELECT);
                sWorldselectNameMode = WORLDSELECT_NAME_MODE_HIDE;
                sWorldselectRotation = WORLDSELECT_ROTATION_RIGHT;
            }
        }

        break;
    case WORLDSELECT_ROTATION_RIGHT:
        if (GetKeysHeld() & DPAD_LEFT) {
            slot = sWorldselectCursor - sWorldselectSlotCount / 2;

            while (slot < 0) {
                slot += sWorldselectSlotCount;
            }

            ReleaseObjPalette(sWorldselectSlots[slot].palette);
            ReleaseObjTiles(sWorldselectSlots[slot].tiles);
            listIndex = sWorldselectSlots[sWorldselectCursor].listIndex - sWorldselectSlotCount / 2;

            while (listIndex < 0) {
                listIndex += sWorldselectWorldCount;
            }

            sWorldselectSlots[slot].listIndex = listIndex;
            WorldselectLoadSlotPalette(sWorldselectWorlds[listIndex], slot);
            WorldselectLoadSlotTiles(sWorldselectWorlds[listIndex], slot);
            WorldselectSetSlotGfx(sWorldselectWorlds[listIndex], slot);
            sWorldselectCursor--;

            if (sWorldselectCursor < 0) {
                sWorldselectCursor = sWorldselectSlotCount - 1;
            }

            m4aSongNumStart(SONG_SYS_WORLDSELECT);
            sWorldselectRotation = WORLDSELECT_ROTATION_LEFT;
        } else {
            for (i = 0; i < sWorldselectSlotCount; i++) {
                sWorldselectSlots[i].angle -= 2;
            }

            if (sWorldselectSlots[sWorldselectCursor].angle <= 128) {
                if (GetKeysHeld() & DPAD_RIGHT) {
                    slot = sWorldselectCursor + sWorldselectSlotCount / 2;

                    while (slot >= sWorldselectSlotCount) {
                        slot -= sWorldselectSlotCount;
                    }

                    ReleaseObjPalette(sWorldselectSlots[slot].palette);
                    ReleaseObjTiles(sWorldselectSlots[slot].tiles);
                    listIndex = sWorldselectSlots[sWorldselectCursor].listIndex + sWorldselectSlotCount / 2;

                    while (listIndex >= sWorldselectWorldCount) {
                        listIndex -= sWorldselectWorldCount;
                    }

                    sWorldselectSlots[slot].listIndex = listIndex;
                    WorldselectLoadSlotPalette(sWorldselectWorlds[listIndex], slot);
                    WorldselectLoadSlotTiles(sWorldselectWorlds[listIndex], slot);
                    WorldselectSetSlotGfx(sWorldselectWorlds[listIndex], slot);
                    sWorldselectCursor++;

                    if (sWorldselectCursor >= sWorldselectSlotCount) {
                        sWorldselectCursor = 0;
                    }

                    m4aSongNumStart(SONG_SYS_WORLDSELECT);
                    sWorldselectRotation = WORLDSELECT_ROTATION_RIGHT;
                } else {
                    step = 128 - sWorldselectSlots[sWorldselectCursor].angle;

                    for (i = 0; i < sWorldselectSlotCount; i++) {
                        sWorldselectSlots[i].angle += step;
                    }

                    sWorldselectNameMode = WORLDSELECT_NAME_MODE_SHOW;
                    sWorldselectNameWorld = sWorldselectWorlds[sWorldselectSlots[sWorldselectCursor].listIndex];
                    sWorldselectRotation = WORLDSELECT_ROTATION_NONE;
                }
            }
        }

        break;
    case WORLDSELECT_ROTATION_LEFT:
        if (GetKeysHeld() & DPAD_RIGHT) {
            slot = sWorldselectCursor + sWorldselectSlotCount / 2;

            while (slot >= sWorldselectSlotCount) {
                slot -= sWorldselectSlotCount;
            }

            ReleaseObjPalette(sWorldselectSlots[slot].palette);
            ReleaseObjTiles(sWorldselectSlots[slot].tiles);
            listIndex = sWorldselectSlots[sWorldselectCursor].listIndex + sWorldselectSlotCount / 2;

            while (listIndex >= sWorldselectWorldCount) {
                listIndex -= sWorldselectWorldCount;
            }

            sWorldselectSlots[slot].listIndex = listIndex;
            WorldselectLoadSlotPalette(sWorldselectWorlds[listIndex], slot);
            WorldselectLoadSlotTiles(sWorldselectWorlds[listIndex], slot);
            WorldselectSetSlotGfx(sWorldselectWorlds[listIndex], slot);
            sWorldselectCursor++;

            if (sWorldselectCursor >= sWorldselectSlotCount) {
                sWorldselectCursor = 0;
            }

            m4aSongNumStart(SONG_SYS_WORLDSELECT);
            sWorldselectRotation = WORLDSELECT_ROTATION_RIGHT;
        } else {
            for (i = 0; i < sWorldselectSlotCount; i++) {
                sWorldselectSlots[i].angle += 2;
            }

            if ((s8)sWorldselectSlots[sWorldselectCursor].angle < 0) {
                if (GetKeysHeld() & DPAD_LEFT) {
                    slot = sWorldselectCursor - sWorldselectSlotCount / 2;

                    while (slot < 0) {
                        slot += sWorldselectSlotCount;
                    }

                    ReleaseObjPalette(sWorldselectSlots[slot].palette);
                    ReleaseObjTiles(sWorldselectSlots[slot].tiles);
                    listIndex = sWorldselectSlots[sWorldselectCursor].listIndex - sWorldselectSlotCount / 2;

                    while (listIndex < 0) {
                        listIndex += sWorldselectWorldCount;
                    }

                    sWorldselectSlots[slot].listIndex = listIndex;
                    WorldselectLoadSlotPalette(sWorldselectWorlds[listIndex], slot);
                    WorldselectLoadSlotTiles(sWorldselectWorlds[listIndex], slot);
                    WorldselectSetSlotGfx(sWorldselectWorlds[listIndex], slot);
                    sWorldselectCursor--;

                    if (sWorldselectCursor < 0) {
                        sWorldselectCursor = sWorldselectSlotCount - 1;
                    }

                    m4aSongNumStart(SONG_SYS_WORLDSELECT);
                    sWorldselectRotation = WORLDSELECT_ROTATION_LEFT;
                } else {
                    step = sWorldselectSlots[sWorldselectCursor].angle + 128;

                    for (i = 0; i < sWorldselectSlotCount; i++) {
                        sWorldselectSlots[i].angle -= step;
                    }

                    sWorldselectNameMode = WORLDSELECT_NAME_MODE_SHOW;
                    sWorldselectNameWorld = sWorldselectWorlds[sWorldselectSlots[sWorldselectCursor].listIndex];
                    sWorldselectRotation = WORLDSELECT_ROTATION_NONE;
                }
            }
        }

        break;
    }
}

void WorldselectDraw() {
    s16 i;
    ObjAffine* sprite;
    s16 x;
    s16 y;
    u8 ang;
    u8 facing;
    s32 bob;
    s32 ringY;
    s32 scale;
    s32 scaleX;
    s32 bobY;
    s16 angle;
    void* anim;
    void* tiles;
    void* pal;

#ifdef PLATFORM_PC
    /* the title's HD banner: the sprite stays over the BG copy the screen switches to once it has slid in */
    if (sWorldselectStep >= WORLDSELECT_STEP_TUTORIAL && sWorldselectStep <= WORLDSELECT_STEP_START_ANIM &&
        sWorldselectTitleTiles != NULL &&
        PortHdSpriteFind(sWorldselectTitleTiles->src,
#ifdef VERSION_EU
                         sWorldselectTitleGfx[gLanguage]
#else
                         gWorldselectTitleFrame0
#endif
                         ))
        DrawSprite(0, 0,
#ifdef VERSION_EU
                   sWorldselectTitleGfx[gLanguage],
#else
                   gWorldselectTitleFrame0,
#endif
                   sWorldselectTitleTiles, sWorldselectOverlayPalette, NULL, SPRITE_PRIORITY(0), 0x3E8); /* over BG1 */
#endif
    if (sWorldselectStep < WORLDSELECT_STEP_TUTORIAL || sWorldselectStep > WORLDSELECT_STEP_START_ANIM) {
        DrawSprite(sWorldselectTitleX >> 8, 0,
#ifdef VERSION_EU
                      sWorldselectTitleGfx[gLanguage],
#else
                      gWorldselectTitleFrame0,
#endif
                      sWorldselectTitleTiles, sWorldselectOverlayPalette, NULL, SPRITE_PRIORITY(1),
                      0x3E8);
        DrawSprite(120, sWorldselectFrameY[0] >> 8, gWorldselectBarFrame0, sWorldselectFrameTiles, sWorldselectOverlayPalette, NULL,
                      SPRITE_PRIORITY(1), 0x3E9);
        DrawSprite(120, sWorldselectFrameY[1] >> 8, gWorldselectBarFrame1, sWorldselectFrameTiles, sWorldselectOverlayPalette, NULL,
                      SPRITE_PRIORITY(3), 0xBBA);
    }

    for (i = 0; i < sWorldselectSlotCount; i++) {
        ang = sWorldselectSlots[i].angle;
        bob = -COS(256 / sWorldselectSlotCount * i + sWorldselectBobPhase);
        facing = (bob * 3 >> 7) + ang;

        if ((u8)(facing - 62) > 2 && (u8)(facing + 64) > 2) {
            ringY = -gSineTable[ang + 64] * 5 >> 5;
            scale = -25600 / (ringY - 140);
            scaleX = -gSineTable[facing + 64] * scale >> 8;
            angle = ang;
            x = (gSineTable[angle] * 5 >> 4) + 120;
            bobY = ((scale << 3) * bob >> 16) + 64;
            y = ringY + bobY;

            if ((u8)(facing - 121) <= 14) {
                sprite = NULL;
                tiles = sWorldselectCardTiles[0];
                pal = sWorldselectCardPalettes[0];
                anim = gWorldselectCardFrame0;

                if (sWorldselectStep > WORLDSELECT_STEP_SELECT && !sWorldselectCancelled) {
                    BgAnimSetPosition(x - 1, y - 5);
                }
            } else if (facing <= 61) {
                sprite = NULL;
                tiles = sWorldselectCardTiles[1];
                pal = sWorldselectCardPalettes[1];
                anim = gWorldselectCardFlipFrames[(62 - facing) / 13];
            } else if (facing > 194) {
                sprite = NULL;
                tiles = sWorldselectCardTiles[1];
                pal = sWorldselectCardPalettes[1];
                anim = gWorldselectCardFlipFrames[(facing - 194) / 13];
            } else {
                sprite = AllocObjAffine(0, scaleX, scale, FALSE);
                tiles = sWorldselectCardTiles[0];
                pal = sWorldselectCardPalettes[0];
                anim = gWorldselectCardFrame0;
            }

            DrawSprite(x, y, anim, tiles, pal, sprite, SPRITE_PRIORITY(2),
                          ang > 128 ? (u16)(ang * 2 + 0x6D1) : (u16)((128 - ang) * 2 + 0x7D1));

            if ((u8)(facing - 65) <= 126) {
                DrawSprite(x, y, sWorldselectSlots[i].gfx, sWorldselectSlots[i].tiles,
                              sWorldselectSlots[i].palette, sprite, SPRITE_PRIORITY(2),
                              ang > 128 ? (u16)(ang * 2 + 0x6D0)
                                        : (u16)((128 - ang) * 2 + 0x7D0));
            }
        }
    }

    switch (sWorldselectNameMode) {
    case WORLDSELECT_NAME_MODE_SHOW:
        WorldselectDrawName(sWorldselectNameWorld, sWorldselectNameWidth);

        if (sWorldselectNameWidth <= 8) {
            sWorldselectNameWidth++;
        } else {
            sWorldselectNameMode = WORLDSELECT_NAME_MODE_IDLE;
        }

        break;
    case WORLDSELECT_NAME_MODE_HIDE:
        WorldselectDrawName(sWorldselectNameWorld, sWorldselectNameWidth);

        if (sWorldselectNameWidth > 0) {
            sWorldselectNameWidth--;
        } else {
            sWorldselectNameMode = WORLDSELECT_NAME_MODE_IDLE;
        }

        break;
    }

    if (sWorldselectBgAnimActive) {
        BgAnimUpdate();
    }

    TaskPoolDraw(&sWorldselectTaskPool);
}

void WorldselectSetBgMode0() {
    SetBgMode0();
    SetupBg(0, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 1, 30, 14);
    SetBgPriority(0, 3);
    SetBgPriority(1, 1);
    SetBgPriority(2, 0);
}

void WorldselectSetBgMode1() {
    SetBgMode1();
    SetupBg(0, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 1, 30, 10);
    SetBgPriority(0, 3);
    SetBgPriority(1, 0);
    SetBgPriority(2, 2);
    gBldCnt = (BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1);
    gBldAlpha = BLDALPHA_BLEND(16, 16);
}

void WorldselectCyclePalette() {
    sWorldselectPaletteTimer++;

    if (sWorldselectPaletteTimer > 6) {
        sWorldselectPaletteTimer = 0;
        sWorldselectPaletteFrame++;

        if (sWorldselectPaletteFrame > 29) {
            sWorldselectPaletteFrame = 0;
        }

        LoadPalette(sWorldselectPaletteCycle[(s16)sWorldselectPaletteFrame], (void*)(BG_PLTT + 2 * PLTT_SIZE_4BPP), 32);
    }
}

void mode_worldselect_0() {
    s16 i;
    s16 j;
    void** bufferPtr;

    SpriteReset();
    sWorldselectFirstVisit = (gGameState.progression.tutorialFlags ^ TUTORIAL_FLAG_WORLDSELECT) & TUTORIAL_FLAG_WORLDSELECT;
    sWorldselectBgAnimActive = FALSE;
    sWorldselectCancelled = FALSE;
    FadeStartIn(FADE_MODE_ADD_WHITE, 16);

    if (sWorldselectFirstVisit != 0) {
        WorldselectSetBgMode0();
    } else {
        WorldselectSetBgMode1();
    }

    sWorldselectCursor = 0;
    sWorldselectRotation = WORLDSELECT_ROTATION_NONE;
    j = 0;

    for (i = 0; i <= 12; i++) {
        if (gGameState.availableWorlds & sWorldselectWorldDefs[i].worldBit) {
            sWorldselectWorlds[j] = i;
            j++;
        }
    }

    sWorldselectWorldCount = j;
    sWorldselectSlotCount = j > 5 ? 5 : j;
    j = 0;

    for (i = 0; i < sWorldselectSlotCount; i++, j++) {
        if (j >= sWorldselectWorldCount) {
            j = 0;
        }

        if (i == sWorldselectSlotCount - 1 && sWorldselectSlotCount > 2) {
            j = sWorldselectWorldCount - 1;
        }

        sWorldselectSlots[i].listIndex = j;
        sWorldselectSlots[i].angle = 256 / sWorldselectSlotCount * i - 128;
        WorldselectLoadSlotPalette(sWorldselectWorlds[j], i);
        WorldselectLoadSlotTiles(sWorldselectWorlds[j], i);
        WorldselectSetSlotGfx(sWorldselectWorlds[j], i);
    }

    sWorldselectBobPhase = 0;
    sWorldselectNameMode = WORLDSELECT_NAME_MODE_SHOW;
    sWorldselectNameWidth = 0;
    sWorldselectNameWorld = sWorldselectWorlds[sWorldselectSlots[sWorldselectCursor].listIndex];
    bufferPtr = &sWorldselectNameBuffer;
    *bufferPtr = EwramAlloc(0x6C0);
    sWorldselectPaletteTimer = 0;
    sWorldselectPaletteFrame = 0;
    sWorldselectStep = WORLDSELECT_STEP_BARS_IN;
    sWorldselectTimer = 16;
    sWorldselectFrameY[0] = -2048;
    sWorldselectFrameY[1] = 0xA800;
    sWorldselectTitleX = -32768;
    LoadBgPalette(0, gWorldselectBgPalettes, sizeof(gWorldselectBgPalettes));
    LoadBgTiles(0, gWorldselectBgTiles, sizeof(gWorldselectBgTiles));
    WorldselectDrawName(sWorldselectNameWorld, sWorldselectNameWidth);
    LoadBgMap(0, gWorldselectGlowMap, sizeof(gWorldselectGlowMap));
    LoadBgMap(1, gWorldselectBarMap, sizeof(gWorldselectBarMap));

    if (sWorldselectFirstVisit == 0) {
        BgAnimInit(2, BGCNT_AFF512x512, BGCNT_256COLOR);
        BgAnimStart(&gBgAnimDefWorldSelect, 120, 110);
        BgAnimSetLoopStartFrame(0);
        sWorldselectBgAnimActive = TRUE;
    }

    sWorldselectCardPalettes[0] = LoadObjPalette(gWorldselectCardPalette, sizeof(gWorldselectCardPalette));
    sWorldselectCardTiles[0] = LoadObjTiles(gWorldselectCardTiles, sizeof(gWorldselectCardTiles));
    sWorldselectCardPalettes[1] = LoadObjPalette(gWorldselectCardFlipPalette, sizeof(gWorldselectCardFlipPalette));
    sWorldselectCardTiles[1] = LoadObjTiles(gWorldselectCardFlipTiles, sizeof(gWorldselectCardFlipTiles));
    sWorldselectOverlayPalette = LoadObjPalette(gWorldselectOverlayPalette, sizeof(gWorldselectOverlayPalette));
#ifdef VERSION_EU
    sWorldselectTitleTiles = LoadObjTiles(sWorldselectTitleTileData[gLanguage], sWorldselectTitleTileSizes.sizes[gLanguage]);
#else
    sWorldselectTitleTiles = LoadObjTiles(gWorldselectTitleTiles, sizeof(gWorldselectTitleTiles));
#endif
    sWorldselectFrameTiles = LoadObjTiles(gWorldselectBarTiles, sizeof(gWorldselectBarTiles));
    TaskPoolInit(&sWorldselectTaskPool, 1);
    EnableBg(0);
    EnableBg(1);

    if (sWorldselectFirstVisit != 0) {
        DisableBg(2);
    } else {
        EnableBg(2);
    }
}

void mode_worldselect_1() {
    s16 world;
    s16 eventId;

    UpdatePlayTime();
    sWorldselectBobPhase += 2;

    switch (sWorldselectStep) {
    case WORLDSELECT_STEP_BARS_IN:
        ApproachValue(&sWorldselectFrameY[0], 0, sWorldselectTimer);
        ApproachValue(&sWorldselectFrameY[1], 0x9800, sWorldselectTimer);
        sWorldselectTimer--;

        if (sWorldselectTimer <= 0) {
            sWorldselectTimer = 16;
            sWorldselectStep = WORLDSELECT_STEP_TITLE_IN;
        }

        break;
    case WORLDSELECT_STEP_TITLE_IN:
        ApproachValue(&sWorldselectTitleX, 0, sWorldselectTimer);
        sWorldselectTimer--;

        if (sWorldselectTimer <= 0) {
            if (sWorldselectFirstVisit != 0) {
                sWorldselectTutorialStep = 0;
                CreateCardMessageTask(&sWorldselectTaskPool, 2, CARD_MSG_WORLDSELECT_TUTORIAL_0);
                sWorldselectStep = WORLDSELECT_STEP_TUTORIAL;
            } else {
                sWorldselectStep = WORLDSELECT_STEP_SELECT;
            }

            LoadBgMap(0, gWorldselectGlowLineMap, sizeof(gWorldselectGlowLineMap));
#ifdef VERSION_EU
            LoadBgMap(1, sWorldselectBg1Maps[gLanguage], 0x500);
#else
            LoadBgMap(1, gWorldselectTitleMap, sizeof(gWorldselectTitleMap));
#endif
        }

        break;
    case WORLDSELECT_STEP_TUTORIAL:
        if (!IsMessageWindowOpen()) {
            if (sWorldselectTutorialStep == 0) {
                CreateCardMessageTask(&sWorldselectTaskPool, 2, CARD_MSG_WORLDSELECT_TUTORIAL_1);
                sWorldselectTutorialStep++;
            } else {
                gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_WORLDSELECT;
                WorldselectSetBgMode1();
                BgAnimInit(2, BGCNT_AFF512x512, BGCNT_256COLOR);
                BgAnimStart(&gBgAnimDefWorldSelect, 120, 110);
                BgAnimSetLoopStartFrame(0);
                sWorldselectBgAnimActive = TRUE;
                sWorldselectStep = WORLDSELECT_STEP_SELECT;
            }
        }

        break;
    case WORLDSELECT_STEP_SELECT:
        WorldselectHandleInput();
        break;
    case WORLDSELECT_STEP_START_ANIM:
        if (BgAnimIsStopped()) {
            LoadBgMap(0, gWorldselectGlowMap, sizeof(gWorldselectGlowMap));
            LoadBgMap(1, gWorldselectBarMap, sizeof(gWorldselectBarMap));
            sWorldselectTimer = 16;
            sWorldselectStep = WORLDSELECT_STEP_TITLE_OUT;
        }

        break;
    case WORLDSELECT_STEP_TITLE_OUT:
        ApproachValue(&sWorldselectTitleX, -32768, sWorldselectTimer);
        sWorldselectTimer--;

        if (sWorldselectTimer <= 0) {
            sWorldselectTimer = 16;
            sWorldselectStep = WORLDSELECT_STEP_BARS_OUT;
        }

        break;
    case WORLDSELECT_STEP_BARS_OUT:
        ApproachValue(&sWorldselectFrameY[0], -2048, sWorldselectTimer);
        ApproachValue(&sWorldselectFrameY[1], 0xA800, sWorldselectTimer);
        sWorldselectTimer--;

        if (sWorldselectTimer <= 0) {
            sWorldselectStep = WORLDSELECT_STEP_FADE_OUT;
        }

        break;
    case WORLDSELECT_STEP_FADE_OUT:
        FadeLock();

        if (sWorldselectCancelled) {
            FadeStartOut(FADE_MODE_BLACK, 16);
        } else {
            FadeStartOut(FADE_MODE_ADD_WHITE, 16);
        }

        sWorldselectStep = WORLDSELECT_STEP_WAIT_FADE;
        break;
    case WORLDSELECT_STEP_WAIT_FADE:
        if (!FadeIsActive()) {
            if (sWorldselectCancelled) {
                RequestMapMode();
            } else {
                sWorldselectTimer = 60;
                sWorldselectStep = WORLDSELECT_STEP_ENTER_WORLD;
            }
        }

        break;
    case WORLDSELECT_STEP_ENTER_WORLD:
        sWorldselectTimer--;

        if (sWorldselectTimer <= 0) {
            world = sWorldselectWorldDefs[sWorldselectWorlds[sWorldselectSlots[sWorldselectCursor].listIndex]].world;

            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                eventId = sWorldselectWorldDefs[sWorldselectWorlds[sWorldselectSlots[sWorldselectCursor].listIndex]].eventId;
            } else {
                eventId = sWorldselectWorldDefs[sWorldselectWorlds[sWorldselectSlots[sWorldselectCursor].listIndex]].rikuEventId;
            }

            gGameState.availableWorlds &=
                ~sWorldselectWorldDefs[sWorldselectWorlds[sWorldselectSlots[sWorldselectCursor].listIndex]].worldBit;
            SetFloorWorld(world);

            if (eventId >= 0) {
                RequestEventMode(eventId);
            } else {
                if (gGameState.flags & GAME_FLAG_RIKU) {
                    AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_BEGINNINGS, 1));
                }

                EnterFloorWorld();
                RequestMapMode();
            }
        }

        break;
    }

    WorldselectCyclePalette();

    if (FadeIsActive()) {
        FadeGetAmount();
    }

    TaskPoolUpdate(&sWorldselectTaskPool);
    WorldselectDraw();
}

void mode_worldselect_2() {
    s16 i;

    EwramFree(sWorldselectNameBuffer);

    for (i = 0; i < sWorldselectSlotCount; i++) {
        ReleaseObjPalette(sWorldselectSlots[i].palette);
        ReleaseObjTiles(sWorldselectSlots[i].tiles);
    }

    for (i = 0; i < 2; i++) {
        ReleaseObjPalette(sWorldselectCardPalettes[i]);
        ReleaseObjTiles(sWorldselectCardTiles[i]);
    }

    ReleaseObjPalette(sWorldselectOverlayPalette);
    ReleaseObjTiles(sWorldselectTitleTiles);
    ReleaseObjTiles(sWorldselectFrameTiles);
    TaskPoolDestroy(&sWorldselectTaskPool);
}
