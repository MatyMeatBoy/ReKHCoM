/**
 * btl_map.c
 * Battle Background Map
 */

#include "btl.h"
#include "battle_backgrounds.h"
#include "btl_api.h"
#include "battle_work.h"
#include "display.h"
#include "engine_math.h"
#include "game_state.h"
#include "taskpool.h"
#include "types.h"
#include "event_backgrounds.h"
#include <stddef.h>
#include "battle_ids.h"
#include "gba/io_reg.h"
#include "gba/defines.h"

#ifdef PLATFORM_PC
#include "engine.h"
/* Repeat only authored side ornaments. The camera, objects, collision and
 * 240x160 canvas retain their cartridge behavior; the PPU owns sampling. */
#define BTL_ARENA_BANDS(lw, rw) PortSetBattleArenaBands(48, 464, 48, (lw), 464 - (rw), (rw))
#define BTL_HOOK_BANDS() PortSetBattleArenaBands(24, 488, 24, 64, 24, 64)
#else
#define BTL_ARENA_BANDS(lw, rw) ((void)0)
#define BTL_HOOK_BANDS() ((void)0)
#endif

#define BTL_MAP_VIEW_Y_MIN 144
#define BTL_MAP_VIEW_Y_MAX_EXTRA 32

static u8 sBtlMapShakeActive;
static u16 sBtlMapShakeStep;
static s32 sBtlMapShakeOffset;

static const s8 sBtlMapShakePattern[32] = {
    4, 4, 4, 4, -4, -4, -4, -4, 3, 3, 3, 3, -3, -3, -3, -3, 2, 2, 2, 2, -2, -2, -2, -2, 1, 1, 1, 1, -1, -1, -1, -1,
};

void task_btl_map_0(BtlMapWork* work) {
    SetBgSize(gBtlWork->mapBg, BGCNT_AFF512x512);

    if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
        switch (gBtlWork->battleId) {
        case BATTLE_TUTORIAL_0:
        case BATTLE_TUTORIAL_1:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgTraverseTownTiles, sizeof(gBtlBgTraverseTownTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgTraverseTownPalette, sizeof(gBtlBgTraverseTownPalette));
            BTL_ARENA_BANDS(96, 96);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgTraverseTownMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgTraverseTownMap, sizeof(gBtlBgTraverseTownMap));
#endif
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_ANSEM_2:
#ifdef VERSION_EU
            LoadBgTilesLz77(gBtlWork->mapBg, gBtlBgAnsem2Tiles);
#else
            LoadBgTiles(gBtlWork->mapBg, gBtlBgAnsem2Tiles, sizeof(gBtlBgAnsem2Tiles));
#endif
            LoadBgPalette(gBtlWork->mapBg, gBtlBgAnsem2Palette, sizeof(gBtlBgAnsem2Palette));
            BTL_ARENA_BANDS(64, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgAnsem2Map);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgAnsem2Map, sizeof(gBtlBgAnsem2Map));
#endif
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_HADES:
#ifdef VERSION_EU
            LoadBgTilesLz77(gBtlWork->mapBg, gBtlBgHadesTiles);
#else
            LoadBgTiles(gBtlWork->mapBg, gBtlBgHadesTiles, sizeof(gBtlBgHadesTiles));
#endif
            LoadBgPalette(gBtlWork->mapBg, gBtlBgHadesPalette, sizeof(gBtlBgHadesPalette));
            BTL_ARENA_BANDS(64, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgHadesMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgHadesMap, sizeof(gBtlBgHadesMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_HOOK:
#ifdef VERSION_EU
            LoadBgTilesLz77(gBtlWork->mapBg, gBtlBgHookTiles);
#else
            LoadBgTiles(gBtlWork->mapBg, gBtlBgHookTiles, sizeof(gBtlBgHookTiles));
#endif
            LoadBgPalette(gBtlWork->mapBg, gBtlBgHookPalette, sizeof(gBtlBgHookPalette));
            BTL_HOOK_BANDS();
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgHookMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgHookMap, sizeof(gBtlBgHookMap));
#endif
            gBtlWork->fadeAmount = 9;
            break;
        case BATTLE_CLOUD:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgOlympusColiseumTiles, sizeof(gBtlBgOlympusColiseumTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgOlympusColiseumPalette, sizeof(gBtlBgOlympusColiseumPalette));
            BTL_ARENA_BANDS(72, 72);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgOlympusColiseumMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgOlympusColiseumMap, sizeof(gBtlBgOlympusColiseumMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_RIKU_6:
        case BATTLE_VEXEN_2:
#ifdef VERSION_EU
            LoadBgTilesLz77(gBtlWork->mapBg, gBtlBgMansionTiles);
#else
            LoadBgTiles(gBtlWork->mapBg, gBtlBgMansionTiles, sizeof(gBtlBgMansionTiles));
#endif
            LoadBgPalette(gBtlWork->mapBg, gBtlBgMansionPalette, sizeof(gBtlBgMansionPalette));
            BTL_ARENA_BANDS(64, 88);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgMansionMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgMansionMap, sizeof(gBtlBgMansionMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_MARLUXIA:
#ifdef VERSION_EU
            LoadBgTilesLz77(gBtlWork->mapBg, gBtlBgMarluxiaTiles);
#else
            LoadBgTiles(gBtlWork->mapBg, gBtlBgMarluxiaTiles, sizeof(gBtlBgMarluxiaTiles));
#endif
            LoadBgPalette(gBtlWork->mapBg, gBtlBgMarluxiaPalette, sizeof(gBtlBgMarluxiaPalette));
            BTL_ARENA_BANDS(72, 80);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgMarluxiaMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgMarluxiaMap, sizeof(gBtlBgMarluxiaMap));
#endif
            gBtlWork->fadeAmount = 20;
            break;
        default:
#ifdef VERSION_EU
            LoadBgTilesLz77(gBtlWork->mapBg, gBtlBgHumTiles);
#else
            LoadBgTiles(gBtlWork->mapBg, gBtlBgHumTiles, sizeof(gBtlBgHumTiles));
#endif
            LoadBgPalette(gBtlWork->mapBg, gBtlBgHumPalette, sizeof(gBtlBgHumPalette));
            BTL_ARENA_BANDS(96, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgHumMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgHumMap, sizeof(gBtlBgHumMap));
#endif
            gBtlWork->fadeAmount = 20;
            break;
        }
    } else if (gBtlWork->battleId == BATTLE_CARD_SOLDIERS) {
        LoadBgTiles(gBtlWork->mapBg, gBtlBgGardenTiles, sizeof(gBtlBgGardenTiles));
        LoadBgPalette(gBtlWork->mapBg, gBtlBgGardenPalette, sizeof(gBtlBgGardenPalette));
        BTL_ARENA_BANDS(64, 72);
#ifdef VERSION_EU
        LoadBgMapLz77(gBtlWork->mapBg, gBtlBgGardenMap);
#else
        LoadBgMap(gBtlWork->mapBg, gBtlBgGardenMap, sizeof(gBtlBgGardenMap));
#endif
        gBtlWork->fadeAmount = 10;
    } else {
        switch (gGameState.battleStage) {
        case BATTLE_STAGE_WONDERLAND:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgWonderlandTiles, sizeof(gBtlBgWonderlandTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgWonderlandPalette, sizeof(gBtlBgWonderlandPalette));
            BTL_ARENA_BANDS(96, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgWonderlandMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgWonderlandMap, sizeof(gBtlBgWonderlandMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_STAGE_GARDEN:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgGardenTiles, sizeof(gBtlBgGardenTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgGardenPalette, sizeof(gBtlBgGardenPalette));
            BTL_ARENA_BANDS(64, 72);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgGardenMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgGardenMap, sizeof(gBtlBgGardenMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_STAGE_AGRABAH:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgAgrabahTiles, sizeof(gBtlBgAgrabahTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgAgrabahPalette, sizeof(gBtlBgAgrabahPalette));
            BTL_ARENA_BANDS(72, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgAgrabahMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgAgrabahMap, sizeof(gBtlBgAgrabahMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_STAGE_ATLANTICA:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgAtlanticaTiles, sizeof(gBtlBgAtlanticaTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgAtlanticaPalette, sizeof(gBtlBgAtlanticaPalette));
            BTL_ARENA_BANDS(64, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgAtlanticaMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgAtlanticaMap, sizeof(gBtlBgAtlanticaMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_STAGE_MONSTRO:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgMonstroTiles, sizeof(gBtlBgMonstroTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgMonstroPalette, sizeof(gBtlBgMonstroPalette));
            BTL_ARENA_BANDS(96, 96);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgMonstroMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgMonstroMap, sizeof(gBtlBgMonstroMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_STAGE_HALLOWEEN_TOWN:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgHalloweenTownTiles, sizeof(gBtlBgHalloweenTownTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgHalloweenTownPalette, sizeof(gBtlBgHalloweenTownPalette));
            BTL_ARENA_BANDS(64, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgHalloweenTownMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgHalloweenTownMap, sizeof(gBtlBgHalloweenTownMap));
#endif
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_STAGE_NEVER_LAND:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgNeverLandTiles, sizeof(gBtlBgNeverLandTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgNeverLandPalette, sizeof(gBtlBgNeverLandPalette));
            BTL_ARENA_BANDS(96, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgNeverLandMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgNeverLandMap, sizeof(gBtlBgNeverLandMap));
#endif
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_STAGE_DESTINY_ISLANDS:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgDestinyIslandsTiles, sizeof(gBtlBgDestinyIslandsTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgDestinyIslandsPalette, sizeof(gBtlBgDestinyIslandsPalette));
            BTL_ARENA_BANDS(64, 80);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgDestinyIslandsMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgDestinyIslandsMap, sizeof(gBtlBgDestinyIslandsMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_STAGE_HOLLOW_BASTION:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgHollowBastionTiles, sizeof(gBtlBgHollowBastionTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgHollowBastionPalette, sizeof(gBtlBgHollowBastionPalette));
            BTL_ARENA_BANDS(96, 96);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgHollowBastionMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgHollowBastionMap, sizeof(gBtlBgHollowBastionMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        case BATTLE_STAGE_TRAVERSE_TOWN:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgTraverseTownTiles, sizeof(gBtlBgTraverseTownTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgTraverseTownPalette, sizeof(gBtlBgTraverseTownPalette));
            BTL_ARENA_BANDS(96, 96);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgTraverseTownMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgTraverseTownMap, sizeof(gBtlBgTraverseTownMap));
#endif
            gBtlWork->fadeAmount = 5;
            break;
        case BATTLE_STAGE_CASTLE_OBLIVION:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgCastleOblivionTiles, sizeof(gBtlBgCastleOblivionTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgCastleOblivionPalette, sizeof(gBtlBgCastleOblivionPalette));
            BTL_ARENA_BANDS(64, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgCastleOblivionMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgCastleOblivionMap, sizeof(gBtlBgCastleOblivionMap));
#endif
            gBtlWork->fadeAmount = 20;
            break;
        case BATTLE_STAGE_TWILIGHT_TOWN:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgTwilightTownTiles, sizeof(gBtlBgTwilightTownTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgTwilightTownPalette, sizeof(gBtlBgTwilightTownPalette));
            BTL_ARENA_BANDS(64, 64);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgTwilightTownMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgTwilightTownMap, sizeof(gBtlBgTwilightTownMap));
#endif
            gBtlWork->fadeAmount = 10;
            break;
        default:
            LoadBgTiles(gBtlWork->mapBg, gBtlBgOlympusColiseumTiles, sizeof(gBtlBgOlympusColiseumTiles));
            LoadBgPalette(gBtlWork->mapBg, gBtlBgOlympusColiseumPalette, sizeof(gBtlBgOlympusColiseumPalette));
            BTL_ARENA_BANDS(72, 72);
#ifdef VERSION_EU
            LoadBgMapLz77(gBtlWork->mapBg, gBtlBgOlympusColiseumMap);
#else
            LoadBgMap(gBtlWork->mapBg, gBtlBgOlympusColiseumMap, sizeof(gBtlBgOlympusColiseumMap));
#endif
            gBtlWork->fadeAmount = 11;
            break;
        }
    }

    gBtlWork->scale = Q_8_8(1);
    gBtlWork->zoomScale = Q_8_8(1);
    gBtlWork->x = 0x10000;
    gBtlWork->y = 0x16000;
    gBtlWork->viewX = 0x10000;
    gBtlWork->viewY = 0x16000;
    gBtlWork->x2 = 0x10000;
    gBtlWork->y2 = 0x16000;
    gBtlWork->zoomX = 0x10000;
    gBtlWork->zoomY = 0x16000;
    gBtlWork->zoomSteps = 0;
    gBtlWork->rotation = 0;
    work->xMin = gBtlWork->xMin << 8;
    work->xMax = gBtlWork->xMax << 8;
    work->yMin = gBtlWork->yMin << 8;
    work->yMax = (gBtlWork->yMax + BTL_MAP_VIEW_Y_MAX_EXTRA) << 8;
    BtlMapResetShake();
    SetBgAffine(gBtlWork->mapBg, gBtlWork->rotation, gBtlWork->scale,
                gBtlWork->scale, gBtlWork->viewX,
                gBtlWork->viewY + 0x2800);
}

void BtlMapResetShake() {
    sBtlMapShakeActive = FALSE;
    sBtlMapShakeStep = 0;
    sBtlMapShakeOffset = 0;
}

void BtlMapStartShake() {
    sBtlMapShakeActive = TRUE;
    sBtlMapShakeStep = 0;
    sBtlMapShakeOffset = 0;
}

void BtlMapUpdateShake() {
    if (sBtlMapShakeActive) {
        sBtlMapShakeOffset += ((sBtlMapShakePattern[(s16)sBtlMapShakeStep] << 12) - sBtlMapShakeOffset) >> 3;
        sBtlMapShakeStep++;

        if (sBtlMapShakeStep > 0x1F) {
            sBtlMapShakeActive = FALSE;
            sBtlMapShakeOffset = 0;
            gBtlWork->rotation = 0;
        }
    }
}

s32 BtlMapGetShake() {
    return sBtlMapShakeOffset;
}

void BtlMapSetCameraTarget(s32 x, s32 y) {
    gBtlWork->x2 = x;
    gBtlWork->y2 = y;
}

void BtlMapFollowPosition(s32 px, s32 py, s32 pz) {
    s32 x = (px + 0x10000) >> 1;
    s32 y = (py + 0x14400) >> 1;

    if (px - x > 0x3000) {
        x = px - 0x3000;
    } else if (x - px > 0x3000) {
        x = px + 0x3000;
    }

    if (py - y > 0x3000) {
        y = py - 0x3000;
    } else if (y - py > 0x3000) {
        y = py + 0x3000;
    }

    gBtlWork->x2 = x;
    gBtlWork->y2 = y + pz;
}

s32 task_btl_map_1(BtlMapWork* work) {
    s32 dx;
    s32 dy;

    BtlMapUpdateShake();

    if (gBtlWork->zoomSteps > 0) {
        ApproachValueHalfSteps(&gBtlWork->scale, gBtlWork->zoomScale, gBtlWork->zoomSteps);
        ApproachValueHalfSteps(&gBtlWork->x, gBtlWork->zoomX, gBtlWork->zoomSteps);
        ApproachValueHalfSteps(&gBtlWork->y, gBtlWork->zoomY, gBtlWork->zoomSteps);

        if (gBtlWork->zoomScale == Q_8_8(1)) {
            ApproachValueHalfSteps(&work->xMin, gBtlWork->xMin << 8, gBtlWork->zoomSteps);
            ApproachValueHalfSteps(&work->xMax, gBtlWork->xMax << 8, gBtlWork->zoomSteps);
            ApproachValueHalfSteps(&work->yMin, gBtlWork->yMin << 8, gBtlWork->zoomSteps);
            ApproachValueHalfSteps(&work->yMax, (gBtlWork->yMax + BTL_MAP_VIEW_Y_MAX_EXTRA) << 8, gBtlWork->zoomSteps);
        } else if (gBtlWork->zoomScale > Q_8_8(1)) {
            ApproachValueHalfSteps(&work->xMin, 0x3000, gBtlWork->zoomSteps);
            ApproachValueHalfSteps(&work->xMax, 0x1D000, gBtlWork->zoomSteps);
            ApproachValueHalfSteps(&work->yMin, 0x9000, gBtlWork->zoomSteps);
            ApproachValueHalfSteps(&work->yMax, 0x1E800, gBtlWork->zoomSteps);
        }

        gBtlWork->zoomSteps--;
    } else if (gBtlWork->scale == Q_8_8(1)) {
        dx = (gBtlWork->x2 - gBtlWork->x) >> 3;
        dy = (gBtlWork->y2 - gBtlWork->y) >> 3;

        if (dx > 0x400) {
            dx = 0x400;
        } else if (dx < -0x400) {
            dx = -0x400;
        }

        if (dy > 0x400) {
            dy = 0x400;
        } else if (dy < -0x400) {
            dy = -0x400;
        }

        gBtlWork->x += dx;
        gBtlWork->y += dy;
    }

    gBtlWork->viewX = gBtlWork->x;
    gBtlWork->viewY = gBtlWork->y;

    if (sBtlMapShakeActive) {
        gBtlWork->rotation = (sBtlMapShakeOffset >> 8) / 3;
    }

    if (gBtlWork->viewX - ((DISPLAY_WIDTH / 2) << 8) < work->xMin) {
        gBtlWork->viewX = work->xMin + ((DISPLAY_WIDTH / 2) << 8);
    } else if (gBtlWork->viewX + ((DISPLAY_WIDTH / 2) << 8) > work->xMax) {
        gBtlWork->viewX = work->xMax - ((DISPLAY_WIDTH / 2) << 8);
    }

    if (gBtlWork->viewY - ((DISPLAY_HEIGHT / 2) << 8) < (BTL_MAP_VIEW_Y_MIN << 8)) {
        gBtlWork->viewY = (BTL_MAP_VIEW_Y_MIN + DISPLAY_HEIGHT / 2) << 8;
    } else if (gBtlWork->viewY + ((DISPLAY_HEIGHT / 2) << 8) > work->yMax) {
        gBtlWork->viewY = work->yMax - ((DISPLAY_HEIGHT / 2) << 8);
    }

    SetBgAffine(gBtlWork->mapBg, gBtlWork->rotation, gBtlWork->scale,
                  gBtlWork->scale, gBtlWork->viewX,
                  gBtlWork->viewY + 0x2800);
#ifdef PLATFORM_PC
    {
        extern int PortBtlCamTrace(void);
        extern void PortLog(const char* fmt, ...);
        if (PortBtlCamTrace())
            PortLog("btlcam bg %d scale %d view %x,%x target %x,%x lim x %x-%x y %x-%x", gBtlWork->mapBg,
                    gBtlWork->scale, (unsigned)gBtlWork->viewX, (unsigned)gBtlWork->viewY, (unsigned)gBtlWork->x2,
                    (unsigned)gBtlWork->y2, (unsigned)work->xMin, (unsigned)work->xMax, (unsigned)work->yMin,
                    (unsigned)work->yMax);
    }
#endif
    return 1;
}

TaskDesc gTaskDescBtlMap = {
    "task_btl_map",
    (TaskInitFunc)task_btl_map_0,
    (TaskUpdateFunc)task_btl_map_1,
    NULL,
    NULL,
    sizeof(BtlMapWork),
};
