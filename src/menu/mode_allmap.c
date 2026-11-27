/**
 * mode_allmap.c
 * Floor Map Screen
 */

#include "macros.h"
#include "mode_allmap.h"
#include "sprites_allmap.h"
#include "poo.h"
#include "system_state.h"
#include "gba/io_reg.h"
#include "allmap_api.h"
#include "malloc.h"
#include "fade.h"
#include <string.h>
#include "allmap_types.h"
#include "anim.h"
#include "display.h"
#include "game_state.h"
#include "gba/defines.h"
#include "gba/macro.h"
#include "intr.h"
#include "m4a_catalog_data.h"
#include "m4a_song.h"
#include "map_api.h"
#include "map_runtime.h"
#include "map_types.h"
#include "mode.h"
#include "obj_api.h"
#include "registration_data.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "allmap.h"
#include "default_bg_map.h"
#include "sprite_palettes.h"
#include "mode_allmap_api.h"

static const PooBgSet sAllmapWorldBgs[15] = {
    { gAllmapWorldBgWonderlandMap, gAllmapWorldBgWonderlandTiles, gAllmapWorldBgWonderlandPalette },
    { gAllmapWorldBgAgrabahMap, gAllmapWorldBgAgrabahTiles, gAllmapWorldBgAgrabahPalette },
    { gAllmapWorldBgAtlanticaMap, gAllmapWorldBgAtlanticaTiles, gAllmapWorldBgAtlanticaPalette },
    { gAllmapWorldBgOlympusColiseumMap, gAllmapWorldBgOlympusColiseumTiles, gAllmapWorldBgOlympusColiseumPalette },
    { gAllmapWorldBgWonderlandMap, gAllmapWorldBgWonderlandTiles, gAllmapWorldBgWonderlandPalette },
    { gAllmapWorldBgMonstroMap, gAllmapWorldBgMonstroTiles, gAllmapWorldBgMonstroPalette },
    { gAllmapWorldBgHalloweenTownMap, gAllmapWorldBgHalloweenTownTiles, gAllmapWorldBgHalloweenTownPalette },
    { gAllmapWorldBgNeverLandMap, gAllmapWorldBgNeverLandTiles, gAllmapWorldBgNeverLandPalette },
    { gAllmapWorldBgHollowBastionMap, gAllmapWorldBgHollowBastionTiles, gAllmapWorldBgHollowBastionPalette },
    { gAllmapWorldBgDestinyIslandsMap, gAllmapWorldBgDestinyIslandsTiles, gAllmapWorldBgDestinyIslandsPalette },
    { gAllmapWorldBgTraverseTownMap, gAllmapWorldBgTraverseTownTiles, gAllmapWorldBgTraverseTownPalette },
    { gAllmapWorldBgTwilightTownMap, gAllmapWorldBgTwilightTownTiles, gAllmapWorldBgTwilightTownPalette },
    { gAllmapWorldBgCastleOblivionMap, gAllmapWorldBgCastleOblivionTiles, gAllmapWorldBgCastleOblivionPalette },
    { gAllmapWorldBg100AcreWoodMap, gAllmapWorldBg100AcreWoodTiles, gAllmapWorldBg100AcreWoodPalette },
    { gAllmapWorldBgWonderlandMap, gAllmapWorldBgWonderlandTiles, gAllmapWorldBgWonderlandPalette },
};

static const PooPalStep sAllmapPalSteps[9] = {
    { 0, 40 },
    { 1, 8 },
    { 2, 8 },
    { 3, 8 },
    { 4, 15 },
    { 3, 8 },
    { 2, 8 },
    { 1, 8 },
    { POO_PAL_STEP_END, 255 },
};

#ifdef VERSION_EU
u8* gAllmapFloorTilesByLanguage[5] = {
    gAllmapFloorTiles,
    gAllmapFloorFrenchTiles,
    gAllmapFloorGermanTiles,
    gAllmapFloorItalianTiles,
    gAllmapFloorSpanishTiles,
};

u8* gAllmapRikuFloorTilesByLanguage[5] = {
    gAllmapRikuFloorTiles,
    gAllmapRikuFloorFrenchTiles,
    gAllmapRikuFloorGermanTiles,
    gAllmapRikuFloorItalianTiles,
    gAllmapRikuFloorSpanishTiles,
};
#endif

Mode gModeAllmap = {
    "mode_allmap",
    mode_allmap_0,
    mode_allmap_1,
    mode_allmap_2,
};

static const AllmapRoomOrder sAllmapRoomOrder = {{
    0, 4, 2, 5, 3, 10, 9, 13, 1, 7, 8, 11, 6, 14, 12, 15,
}};

static const AllmapRoomDirs sAllmapRoomDirs = {{1, 2, 3, 0}};

u16* gAllmapBg0MapBlocks[8] EWRAM_COMMON(16);
u32 gAllmapModeState EWRAM_COMMON(4);
TaskPool gAllmapTaskPool EWRAM_COMMON(16);
u16* gAllmapBg1Map EWRAM_COMMON(4);
u16 gAllmapCursorDropTimer EWRAM_COMMON(4);
u16* gAllmapBg1MapBlocks[8] EWRAM_COMMON(16);
u16* gAllmapBg0Map EWRAM_COMMON(4);
u16 gAllmapScrollInTimer EWRAM_COMMON(4);

static u16 sAllmapPalTimer;
static u16 sAllmapPalStep;
static s16 sAllmapBlendTimer;
static u8 sAllmapPalette10Copy[0x40];
static u8 sAllmapReturnToMenu;
static u8 sAllmapLowerBgm;

void AllmapVCountCallback() {
    while ((REG_DISPSTAT & DISPSTAT_HBLANK) == 0) {
    }

    REG_BG2CNT &= ~BGCNT_PRIORITY_MASK;
    REG_BG2CNT |= BGCNT_PRIORITY(2);
    REG_BG2HOFS = 0;
}

void AllmapAllocBgMaps() {
    u32 i;
    u16 j;
    u16 k;

    gAllmapBg0Map = EwramAlloc(0x2000 * sizeof(u16));
    gAllmapBg1Map = EwramAlloc(0x2000 * sizeof(u16));

    for (i = 0; i < 0x2000; i++) {
        gAllmapBg0Map[i] = 0;
        gAllmapBg1Map[i] = 0;
    }

    for (j = 0; j < 4; j++) {
        for (k = 0; k < 2; k++) {
            gAllmapBg0MapBlocks[j * 2 + k] = gAllmapBg0Map + (j * 2 + k) * 0x400;
            gAllmapBg1MapBlocks[j * 2 + k] = gAllmapBg1Map + (j * 2 + k) * 0x400;
        }
    }
}

void AllmapDimPalette10() {
    s32 i;

    for (i = 0; i < 32; i++) {
        FadeSetPaletteExcluded(i, TRUE);
    }

    FadeSetPaletteExcluded(10, FALSE);
    FadeToAmount(FADE_MODE_BLACK, 16, 16);
}

void AllmapSetBlend(s16 alpha) {
    SetBlendAlpha(alpha, 16 - alpha);
}

void AllmapCyclePalette() {
    PooPalStep steps[9];

    memcpy(steps, sAllmapPalSteps, sizeof(steps));
    sAllmapPalTimer++;

    if (sAllmapPalTimer < steps[sAllmapPalStep].duration) {
        return;
    }

    sAllmapPalTimer = 0;
    sAllmapPalStep++;

    if (steps[sAllmapPalStep].palette == POO_PAL_STEP_END) {
        sAllmapPalStep = 0;
    }

    LoadPalette(gAllmapRoomPalettes[2 + steps[sAllmapPalStep].palette], (void*)(BG_PLTT + 2 * PLTT_SIZE_4BPP), sizeof(gAllmapRoomPalettes[0]));
}

void AllmapLoadWorldBg() {
    RequestDma3Copy(sAllmapWorldBgs[gGameState.world].map, (u8*)GetBgScreenBase(2) + 0x200, 0x300);
    RequestDma3Copy(sAllmapWorldBgs[gGameState.world].tiles, (u8*)GetBgCharBase(2) + 0x2000, 0x2000);
    LoadPalette(sAllmapWorldBgs[gGameState.world].palette, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), 0x20);
}

void AllmapLoadFloorTiles() {
    u8* src;
    void* dst;

    dst = (u8*)GetBgCharBase(2) + 0x20;

    if ((gGameState.flags & GAME_FLAG_RIKU) != 0) {
#ifdef VERSION_EU
        src = gAllmapRikuFloorTilesByLanguage[gLanguage] + gGameState.floor * 0x140;
#else
        src = &gAllmapRikuFloorTiles[gGameState.floor * 0x140];
#endif
    } else {
#ifdef VERSION_EU
        src = gAllmapFloorTilesByLanguage[gLanguage] + gGameState.floor * 0x140;
#else
        src = &gAllmapFloorTiles[gGameState.floor * 0x140];
#endif
    }

    RequestDma3Copy(src, dst, 0x140);
    dst = (u8*)GetBgScreenBase(2) + 0x480;
    src = gAllmapFloorTitleMap;
    RequestDma3Copy(src, dst, 10);
    dst = (u8*)GetBgScreenBase(2) + 0x4C0;
    src += 0x40;
    RequestDma3Copy(src, dst, 10);
}

void mode_allmap_0(s32 lowerBgm) {
    sAllmapLowerBgm = 0;

    if (lowerBgm == 1) {
        sAllmapLowerBgm = lowerBgm;
    }

    SetObjPaletteRange(0, 14);
    ClearStockMesDispWork();
    SetBgMode0();
    SetupBg(3, 1, 28, 8);
    SetBgPriority(3, 3);
    LoadBgTiles(3, gAllmapBackdropTiles, sizeof(gAllmapBackdropTiles));
    LoadBgPalette(3, gAllmapBgPalettes, sizeof(gAllmapBgPalettes));
    LoadBgMap(3, gAllmapBackdropMap, sizeof(gAllmapBackdropMap));
    SetupBg(2, 1, 29, 8);
    SetBgPriority(2, 0);
    LoadBgMap(2, gDefaultBgMap, 0x200);
    AllmapLoadWorldBg();
    AllmapLoadFloorTiles();
    SetupBg(0, 0, 26, 0);
    SetBgPriority(0, 2);
    LoadBgTiles(0, gAllmapRoomTiles, sizeof(gAllmapRoomTiles));
    LoadBgPalette(0, gAllmapRoomPalettes, sizeof(gAllmapRoomPalettes));
    AllmapAllocBgMaps();
    SetBgMapBlocks(0, gAllmapBg0MapBlocks, 2, 4);
    SetupBg(1, 0, 27, 0);
    SetBgPriority(1, 2);
    SetBgMapBlocks(1, gAllmapBg1MapBlocks, 2, 4);
    TaskPoolInit(&gAllmapTaskPool, 1);
    TaskCreate(&gAllmapTaskPool, &gTaskDescAllmapBar, NULL);
    gAllmapModeState = ALLMAP_MODE_STATE_FADE;
    InitAllmap();
    REG_IME = 0;
    REG_IE |= INTR_FLAG_VCOUNT;
    REG_DISPSTAT &= 0xFF;
    REG_DISPSTAT |= DISPSTAT_VCOUNT_SETTING(80) | DISPSTAT_VCOUNT_INTR;
    SetVCountCallback(AllmapVCountCallback);
    REG_IME = 1;
    FadeStartIn(FADE_MODE_BLACK, 16);

    if (sAllmapLowerBgm != 0) {
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFF, 0x80);
    }

    gAllmapScrollInTimer = 30;
    gAllmapCursorDropTimer = 30;
    sAllmapPalTimer = 0;
    sAllmapPalStep = 0;
}

void AllmapFreezePalette10() {
    FadeSetPaletteExcluded(10, TRUE);
    CpuCopy16((void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), sAllmapPalette10Copy, 32);
    LoadPalette(sAllmapPalette10Copy, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), 32);
}

void mode_allmap_1() {
    UpdatePlayTime();
    TaskPoolUpdate(&gAllmapTaskPool);
    TaskPoolDraw(&gAllmapTaskPool);

    if (gAllmapModeState == ALLMAP_MODE_STATE_FADE && !FadeIsActive()) {
        if (gAllmapScrollInTimer != 0 && gAllmapCursorDropTimer != 0) {
            gAllmapModeState = ALLMAP_MODE_STATE_BAR_SLIDE;
            sAllmapBlendTimer = 16;
        } else {
            ReturnToMap(sAllmapReturnToMenu);
        }
    }

    if (gAllmapModeState == ALLMAP_MODE_STATE_INTRO) {
        if (gAllmapScrollInTimer != 0) {
            gAllmapScrollInTimer--;
        }

        if (gAllmapCursorDropTimer != 0) {
            gAllmapCursorDropTimer--;
        }

        if (sAllmapBlendTimer > 0) {
            if (sAllmapBlendTimer == 16) {
                AllmapDimPalette10();
            }

            sAllmapBlendTimer--;
            AllmapSetBlend(sAllmapBlendTimer);

            if (sAllmapBlendTimer == 0) {
                AllmapFreezePalette10();
            }
        }

        if (gAllmapScrollInTimer == 0 && gAllmapCursorDropTimer == 0) {
            gAllmapModeState = ALLMAP_MODE_STATE_ACTIVE;
        }
    }

    AllmapCyclePalette();

    if (gAllmapModeState == ALLMAP_MODE_STATE_INTRO || gAllmapModeState == ALLMAP_MODE_STATE_ACTIVE) {
        EnableBg(0);
        EnableBg(1);
        UpdateAllmap();
    } else {
        DisableBg(0);
        DisableBg(1);
    }
#ifdef PLATFORM_PC
    {
        /* GA36 4:3, a floor larger than the view: the header goes to the panel's top (port_main.c,
           DrawAllmapAnchored) and covers the rooms that reach it, as the footer does. The windows leave the rooms
           (BG0, BG1) out of the header's rows, so nothing of them is moved up with it; window 0 (its left part)
           also the cursor's sprites, window 1 (the rest) keeps the sprites of the room's name. */
        extern int gPortAllmapCoverRows;
        extern int PortGa36AllmapCoverRows(int* header, int* footer);
        static u8 sCovering;
        int header = gPortAllmapCoverRows, footer = 0;
        /* GA36 at its 2x zoom: the footer's rows too, window 1 wrapping from them round to the header's */
        PortGa36AllmapCoverRows(&header, &footer);
        if (header > 0) {
            gWin0H = WIN_RANGE(0, 128);
            gWin1H = WIN_RANGE(0, DISPLAY_WIDTH);
            gWin0V = WIN_RANGE(0, header);
            gWin1V = footer > header ? WIN_RANGE(footer, header) : WIN_RANGE(0, header);
            gWinIn = WININ_WIN0_BG2 | WININ_WIN0_BG3 | WININ_WIN0_CLR |
                     ((WININ_WIN0_BG2 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR) << 8);
            gWinOut = (gWinOut & 0xFF00) | 0x3F;
            gDispCnt |= DISPCNT_WIN0_ON | DISPCNT_WIN1_ON;
            sCovering = 1;
        } else if (sCovering) {
            gDispCnt &= ~(DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
            sCovering = 0;
        }
    }
#endif
}

void mode_allmap_2() {
#ifdef PLATFORM_PC
    {
        extern int gPortAllmapCoverRows;
        gPortAllmapCoverRows = 0;
        gDispCnt &= ~(DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    }
#endif
    DestroyAllmap();
    TaskPoolDestroy(&gAllmapTaskPool);
    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_VCOUNT;
    REG_DISPSTAT &= ~DISPSTAT_VCOUNT_INTR;
    REG_IME = 1;
    ResetVCountCallback();

    if (sAllmapLowerBgm != 0) {
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFF, 0x100);
    }

    EwramFree(gAllmapBg0Map);
    EwramFree(gAllmapBg1Map);
}

void SetAllmapReturnToMenu(u8 returnToMenu) {
    sAllmapReturnToMenu = returnToMenu;
}

u8 AllmapDoorLeadsToHall(u8 room, u8 side) {
    u8* links = GetMapRoomLinks(room);

    if ((u8)(links[side] + 3) <= 1) {
        return TRUE;
    }

    return FALSE;
}

u8 AllmapDoorExists(u8 room, u8 side) {
    u16 flags = GetMapDoorFlags(room, side);

    if (flags == 0 || (flags & DOOR_FLAG_SEALED) != 0) {
        return FALSE;
    }

    return TRUE;
}

u8 AllmapDoorIsOpen(u8 room, u8 side) {
    u16 flags = GetMapDoorFlags(room, side);

    if ((flags & DOOR_FLAG_OPEN) != 0) {
        return TRUE;
    }

    return FALSE;
}

s32 SetupAllmapRoomDoors(AllmapRoomWork* work) {
    AllmapRoomOrder order = sAllmapRoomOrder;
    AllmapRoomDirs dirs = sAllmapRoomDirs;
    u32 mask;
    u8 i;

    for (i = 0; i < ARRAY_COUNT(work->gfx); i++) {
        work->gfx[i] = NULL;
        work->tiles2[i] = NULL;
    }

    if (GetEventRoomKind(work->room) == EVENT_DOOR_EVENT_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_BOSS_ROOM) {
        if (TestAllmapRoomFlag(work->room, FLOOR_ROOM_FLAG_EVENT_DONE) != 0) {
            return 17;
        }

        if (TestAllmapRoomFlag(work->room, FLOOR_ROOM_FLAG_VISITED) == 0) {
            return 1;
        }
    } else if (TestAllmapRoomFlag(work->room, FLOOR_ROOM_FLAG_VISITED) == 0) {
        return 0;
    }

    mask = 0;

    for (i = 0; i < ARRAY_COUNT(work->anim); i++) {
        if (!AllmapDoorExists(work->room, i)) {
            continue;
        }

        mask += 1 << i;

        if (AllmapDoorLeadsToHall(work->room, i)) {
            AnimInit(&work->anim[i], gAllmapHallDoorAnims, gAllmapHallDoorFrames);
            AnimStart(&work->anim[i], dirs.animIds[i], ANIM_FLAG_LOOP);
            work->gfx[i] = AnimGetGfx(&work->anim[i]);

            if (!work->asSprite) {
                work->tiles2[i] = LoadObjTiles(gAllmapHallDoorTiles, sizeof(gAllmapHallDoorTiles));
            } else {
                work->tiles2[i] = AllocObjTiles(GetMaxSpriteTileBytes(gAllmapHallDoorFrames, ARRAY_COUNT(gAllmapHallDoorFrames)), gAllmapHallDoorTiles);
            }
        } else if (!AllmapDoorIsOpen(work->room, i)) {
            AnimInit(&work->anim[i], gAllmapClosedDoorAnims, gAllmapClosedDoorFrames);
            AnimStart(&work->anim[i], dirs.animIds[i], ANIM_FLAG_LOOP);
            work->gfx[i] = AnimGetGfx(&work->anim[i]);

            if (!work->asSprite) {
                work->tiles2[i] = LoadObjTiles(gAllmapClosedDoorTiles, sizeof(gAllmapClosedDoorTiles));
            } else {
                work->tiles2[i] = AllocObjTiles(GetMaxSpriteTileBytes(gAllmapClosedDoorFrames, ARRAY_COUNT(gAllmapClosedDoorFrames)), gAllmapClosedDoorTiles);
            }
        }
    }

    return order.shapes[mask] + 1;
}
