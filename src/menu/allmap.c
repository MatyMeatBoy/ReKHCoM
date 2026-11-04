/**
 * allmap.c
 * Floor Map Screen Tasks
 */

#include "macros.h"
#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "registration_data.h"
#include "system_state.h"
#include "map_api.h"
#include "obj_api.h"
#include "allmap.h"
#include "gba/keys.h"
#include "allmap_api.h"
#include "sprites_allmap.h"
#include "sprites_card_pictures.h"
#include "engine_math.h"
#include "card_api.h"
#include "malloc.h"
#include "fade.h"
#include "mode_allmap_api.h"
#include "map_runtime.h"
#include "songs.h"
#include <string.h>
#include "allmap_types.h"
#include "anim.h"
#include "card_ui_types.h"
#include "display.h"
#include "field_state.h"
#include "key.h"
#include "m4a_song.h"
#include "map_types.h"
#include "obj.h"
#include "poo_api.h"
#include "taskpool.h"
#include "text.h"
#include "types.h"
#include <stddef.h>
#include "default_bg_map.h"
#include "sprite_palettes.h"
#include "gba/defines.h"
#include "card_ids.h"

#define sAllmapState ((AllmapState*)gSharedModeWork)

u8 gAllmapCursorRoom EWRAM_COMMON(4);
s16 gAllmapCameraY EWRAM_COMMON(4);
s16 gAllmapCameraX EWRAM_COMMON(4);

static TaskDesc sTaskDescAllmapRoom = {
    "task_allmap_room",
    (TaskInitFunc)task_allmap_room_0,
    (TaskUpdateFunc)task_allmap_room_1,
    (TaskDrawFunc)task_allmap_room_2,
    (TaskDestroyFunc)task_allmap_room_3,
    sizeof(AllmapRoomWork),
};

static TaskDesc sTaskDescAllmapCursor = {
    "task_allmap_cursor",
    (TaskInitFunc)task_allmap_cursor_0,
    (TaskUpdateFunc)task_allmap_cursor_1,
    (TaskDrawFunc)task_allmap_cursor_2,
    (TaskDestroyFunc)task_allmap_cursor_3,
    sizeof(AllmapCursorWork),
};

static TaskDesc sTaskDescAllmapRoomname = {
    "task_allmap_roomname",
    (TaskInitFunc)task_allmap_roomname_0,
    (TaskUpdateFunc)task_allmap_roomname_1,
    (TaskDrawFunc)task_allmap_roomname_2,
    (TaskDestroyFunc)task_allmap_roomname_3,
    sizeof(AllmapRoomnameWork),
};

#ifdef VERSION_EU
static void* sAllmapBarSprites[5] = {
    gAllmapBarTitleFrame0,
    gAllmapBarTitleFrame1,
    gAllmapBarTitleFrame4,
    gAllmapBarTitleFrame3,
    gAllmapBarTitleFrame2,
};

static void* sAllmapBarBgMapsByLanguage[5] = {
    gAllmapBarBgEnglishMap,
    gAllmapBarBgFrenchMap,
    gAllmapBarBgGermanMap,
    gAllmapBarBgItalianMap,
    gAllmapBarBgSpanishMap,
};
#endif

TaskDesc gTaskDescAllmapBar = {
    "task_allmap_bar",
    (TaskInitFunc)task_allmap_bar_0,
    (TaskUpdateFunc)task_allmap_bar_1,
    (TaskDrawFunc)task_allmap_bar_2,
    (TaskDestroyFunc)task_allmap_bar_3,
    sizeof(AllmapBarWork),
};

static const s16 sAllmapDoorCardOffsets[4][2] = {
    {-24, 48},
    {56, 48},
    {56, -19},
    {-24, -19},
};

const s16 gUnk_096FDC20[4][2] = {
    {16, -16},
    {-16, -16},
    {-16, 16},
    {16, 16},
};

static const u16 sAllmapDirDeltas[4][2] = {
    {0xFFFF, 1},
    {1, 1},
    {1, 0xFFFF},
    {0xFFFF, 0xFFFF},
};

static const u32 sAllmapReverseDoors[4] = {
    MAP_DOOR_SIDE_UP_RIGHT,
    MAP_DOOR_SIDE_UP_LEFT,
    MAP_DOOR_SIDE_DOWN_LEFT,
    MAP_DOOR_SIDE_DOWN_RIGHT,
};

static const s16 sAllmapKeyCardX[4][4] = {
    {120, 0, 0, 0},
    {104, 136, 0, 0},
    {88, 120, 152, 0},
    {72, 104, 136, 168},
};

static s32 sAllmapDoorOffsetX;
static s32 sAllmapDoorOffsetY;
static s32 sAllmapCameraFixedX;
static s32 sAllmapCameraFixedY;

s32 GetAllmapRoomPaletteOffset(u8 room) {
    u8 eventRoom = 0;

    if (GetEventRoomKind(room) == EVENT_DOOR_EVENT_ROOM || GetEventRoomKind(room) == EVENT_DOOR_BOSS_ROOM) {
        eventRoom = 1;
    }

    return eventRoom << 5;
}

void task_allmap_room_0(AllmapRoomWork* work, AllmapRoomArg* arg) {
    void* pal;

    work->x = arg->x;
    work->y = arg->y;
    work->dropY = -arg->y << 8;
    work->dropTargetY = arg->y << 8;
    work->room = arg->room;
    work->asSprite = arg->asSprite;
    work->shape = SetupAllmapRoomDoors(work);

    if (!work->asSprite) {
        work->tiles = LoadObjTiles(gAllmapRoomTiles, sizeof(gAllmapRoomTiles));
        work->gfx2 = NULL;
    } else {
        work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gAllmapRoomFrames, 17), gAllmapRoomTiles);
        work->gfx2 = gAllmapRoomFrames[work->shape];
    }

    if (!work->asSprite && work->room == gMapFloorState.room) {
        pal = gAllmapRoomPalettes[2];
    } else {
        pal = (u8*)gAllmapRoomPalettes + GetAllmapRoomPaletteOffset(work->room);
    }

    work->palette = LoadObjPalette(pal, 32);
}

s32 task_allmap_room_1() {
    return 1;
}

void task_allmap_room_2(AllmapRoomWork* work) {
    s32 i;
    s16 x;
    s16 y;
    u16 flags;
    u16 priority;

    if (!work->asSprite) {
        x = work->x * 24 - gAllmapCameraX;
        y = work->y * 24 - gAllmapCameraY;
        flags = SPRITE_PRIORITY(2);
        priority = -4100 - work->y * 4;
    } else {
        if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) != 0) {
            return;
        }

        x = work->x;
        y = work->y;
        flags = 0;
        priority = 80;
    }

    if (x < -32 || x > DISPLAY_WIDTH) {
        return;
    }

    if (y < -32) {
        return;
    }

    if (y > DISPLAY_HEIGHT) {
        return;
    }

#ifdef PLATFORM_PC
    /* Only the field diagram is a HUD element; the full map is unchanged. */
    if (work->asSprite) {
        PortHudBegin(1, -1);
        /* a little breathing room from the corner, at the HUD's own scale */
        x -= 6;
        y += 6;
    }
#endif
    if (work->gfx2 != NULL) {
        DrawSprite(x, y, work->gfx2, work->tiles, work->palette, NULL, flags, priority);
    }

    for (i = 0; i < 4; i++) {
        if (work->gfx[i] != NULL) {
            work->gfx[i] = AnimUpdate(&work->anim[i]);
            DrawSprite(x, y, work->gfx[i], work->tiles2[i], work->palette, NULL, flags, i - 4 + priority);
        }
    }
#ifdef PLATFORM_PC
    if (work->asSprite)
        PortHudEnd();
#endif
}

void task_allmap_room_3(AllmapRoomWork* work) {
    s32 i;

    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);

    for (i = 0; i < 4; i++) {
        if (work->tiles2[i] != NULL) {
            ReleaseObjTiles(work->tiles2[i]);
        }
    }
}

void* CreateAllmapRoomTask(TaskPool* pool) {
    AllmapRoomArg arg;

    arg.x = 208;
    arg.y = 0;
    arg.room = gMapFloorState.room;
    arg.asSprite = TRUE;
    return TaskCreate(pool, &sTaskDescAllmapRoom, &arg);
}

u8 AllmapDoorHasCardInfo(u8 room, u8 side) {
    if (GetEventRoomKind(room) != EVENT_DOOR_NONE) {
        if (TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_EVENT_DONE) != 0) {
            return FALSE;
        }
    }

    if (TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_VISITED) != 0) {
        return AllmapDoorExists(room, side);
    }

    return FALSE;
}

u8 AllmapDoorHasKeyInfo(u8 room, u8 side) {
    if (GetEventRoomKind(room) == EVENT_DOOR_NONE) {
        if (TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_VISITED) == 0) {
            return FALSE;
        }
    } else {
        if (TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_EVENT_DONE) != 0) {
            return FALSE;
        }

        if (TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_VISITED) == 0) {
            return FALSE;
        }
    }

    if (AllmapDoorExists(room, side)) {
        return AllmapDoorIsOpen(room, side) == FALSE;
    }

    return FALSE;
}

void task_allmap_cursor_0(AllmapCursorWork* work, AllmapCursorPos* arg) {
    work->pos = *arg;
    work->screenX = work->pos.x * 24 + 16 - gAllmapCameraX;
    work->screenY = work->pos.y * 24 + 11 - gAllmapCameraY;
    work->dropY = -work->screenY << 8;
    work->dropTargetY = work->screenY << 8;
    work->x = work->drawX = work->screenX << 8;
    work->y = work->drawY = work->screenY << 8;
    work->tiles = LoadObjTiles(gAllmapCursorTiles, sizeof(gAllmapCursorTiles));
    work->palette = LoadObjPalette(gAllmapObjPalette, sizeof(gAllmapObjPalette));
    AnimInit(&work->anim, gAllmapCursorAnims, gAllmapCursorFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->moveSteps = 0;
}

s32 task_allmap_cursor_1(AllmapCursorWork* work) {
    s32 x;
    s32 y;

    if (gAllmapModeState == ALLMAP_MODE_STATE_INTRO) {
        if (gAllmapCursorDropTimer > 6) {
            ApproachValue(&work->dropY, work->dropTargetY, gAllmapCursorDropTimer - 7);
        } else if (gAllmapCursorDropTimer > 3) {
            ApproachValueHalfSteps(&work->dropY, work->dropTargetY - 0x800, gAllmapCursorDropTimer - 3);
        } else {
            ApproachValueHalfSteps(&work->dropY, work->dropTargetY, gAllmapCursorDropTimer);
        }
    }

    if (gAllmapModeState != ALLMAP_MODE_STATE_ACTIVE) {
        work->gfx = gAllmapCursorFrames[0];
        return 1;
    }

    work->gfx = AnimUpdate(&work->anim);
    x = (work->pos.x * 24 + 16 - gAllmapCameraX) << 8;
    y = (work->pos.y * 24 + 11 - gAllmapCameraY) << 8;

    if (x != work->x || y != work->y) {
        work->x = x;
        work->y = y;
        work->moveSteps = 4;
    }

    if (work->moveSteps != 0) {
        ApproachValue(&work->drawX, work->x, work->moveSteps);
        ApproachValue(&work->drawY, work->y, work->moveSteps);
        work->moveSteps--;
    }

    return 1;
}

void task_allmap_cursor_2(AllmapCursorWork* work) {
    s16 x;
    s16 y;

    if (IsStockMesDispActive()) {
        return;
    }

    if (gAllmapModeState != ALLMAP_MODE_STATE_ACTIVE) {
        x = work->screenX;
        y = work->dropY >> 8;
    } else {
        x = work->drawX >> 8;
        y = work->drawY >> 8;
    }

    DrawSprite(x - 15, y - 22, work->gfx, work->tiles, work->palette, NULL, 0, 49);
}

void task_allmap_cursor_3(AllmapCursorWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

s16 AllmapDrawRoomnameFrame(u16 width) {
    u8* base;
    u16* src;
    s16 left;
    s16 tileOffset;
    u8* dst;

    left = 120 - width;

    if (left < 0) {
        left = 0;
    }

    tileOffset = left / 8;
    base = GetBgScreenBase(2);
    dst = base + 28;
    src = gAllmapRoomnameFrameMap - tileOffset;
    RequestDma3Copy(src, dst, 32);
    dst = base + 92;
    src += 32;
    RequestDma3Copy(src, dst, 32);
    dst = base + 156;
    src += 32;
    RequestDma3Copy(src, dst, 32);
    return left - left % 8 / 2;
}

s32 GetAllmapRoomnamePaletteOffset(u8 nameId) {
    switch (GetRoomCardBackIndex(nameId)) {
    case MAP_CARD_COLOR_GREEN:
        return 64;
    case MAP_CARD_COLOR_RED:
        return 96;
    case MAP_CARD_COLOR_GOLD:
        return 0;
    case MAP_CARD_COLOR_NONE:
    case MAP_CARD_COLOR_BLUE:
    default:
        return 32;
    }
}

void task_allmap_roomname_0(AllmapRoomnameWork* work, u8* nameId) {
    u16 pal;

    InitTextSlots(work->textSlots, ALLMAP_ROOMNAME_TEXT_SLOTS);
    work->textSlotCount = LoadTextSlots(GetRoomName(nameId[0]), work->textSlots);
    pal = GetAllmapRoomnamePaletteOffset(nameId[0]);
    work->palette = LoadObjPalette(gAllmapRoomnamePalettes + pal, 32);
    LoadPalette(gAllmapRoomnameBgPalettes + pal, (void*)(BG_PLTT + 11 * PLTT_SIZE_4BPP), 32);
    work->x = AllmapDrawRoomnameFrame(GetTextSlotsWidth(work->textSlots, work->textSlotCount));
}

s32 task_allmap_roomname_1() {
    return 1;
}

void task_allmap_roomname_2(AllmapRoomnameWork* work) {
    DrawTextSlots(work->x + 117, 3, work->textSlots, work->palette, 50, work->textSlotCount);
}

void task_allmap_roomname_3(AllmapRoomnameWork* work) {
    FreeTextSlots(work->textSlots, ALLMAP_ROOMNAME_TEXT_SLOTS);
    ReleaseObjPalette(work->palette);
}

void AllmapClearRoomnameFrame() {
    u8* base;
    u16* src;
    u8* dst;

    base = GetBgScreenBase(2);
    dst = base + 28;
    src = gDefaultBgMap;
    RequestDma3Copy(src, dst, 32);
    dst = base + 92;
    src += 32;
    RequestDma3Copy(src, dst, 32);
    dst = base + 156;
    src += 32;
    RequestDma3Copy(src, dst, 32);
}

enum AllmapBarState {
    ALLMAP_BAR_STATE_BARS_IN,
    ALLMAP_BAR_STATE_TITLE_IN,
    ALLMAP_BAR_STATE_IDLE,
    ALLMAP_BAR_STATE_TITLE_OUT,
    ALLMAP_BAR_STATE_BARS_OUT
};

void AllmapBarStartClose(AllmapBarWork* work) {
    work->closing = TRUE;

    if (work->state == ALLMAP_BAR_STATE_BARS_IN) {
        work->state = ALLMAP_BAR_STATE_BARS_OUT;
    } else {
        work->state = ALLMAP_BAR_STATE_TITLE_OUT;
    }

    if (work->steps == 0) {
        work->steps = 16;
    }

    LoadBgMap(3, gAllmapBackdropMap, sizeof(gAllmapBackdropMap));
    work->targetY = -0x800;
    work->targetY2 = 0xA000;
    work->targetX = -0x8000;
}

void task_allmap_bar_0(AllmapBarWork* work) {
    gStockMesDispWork = work;
    work->tiles = LoadObjTiles(gAllmapBarTitleTiles, sizeof(gAllmapBarTitleTiles));
    work->tiles2 = LoadObjTiles(gAllmapBarBandTiles, sizeof(gAllmapBarBandTiles));
    work->palette = LoadObjPalette(gAllmapObjPalette, sizeof(gAllmapObjPalette));
    work->steps = 16;
    work->state = ALLMAP_BAR_STATE_BARS_IN;
    work->y = -0x800;
    work->y2 = 0xA000;
    work->x = -0x8000;
    work->targetY = 0;
    work->targetY2 = 0x9800;
    work->targetX = 0;
    work->closing = FALSE;
    work->fadeStarted = FALSE;
}

void AllmapBarFadeOut(AllmapBarWork* work) {
    FadeStartOut(FADE_MODE_BLACK, 16);
    FadeLock();
}

s32 task_allmap_bar_1(AllmapBarWork* work) {
    s32 i;

    switch (work->state) {
    case ALLMAP_BAR_STATE_BARS_IN:
        ApproachValue(&work->y, work->targetY, work->steps);
        ApproachValue(&work->y2, work->targetY2, work->steps);
        work->steps--;

        if (work->steps == 0) {
            work->steps = 16;
            work->state = ALLMAP_BAR_STATE_TITLE_IN;
        }

        break;
    case ALLMAP_BAR_STATE_TITLE_IN:
        ApproachValue(&work->x, work->targetX, work->steps);
        work->steps--;

        if (work->steps == 0) {
#ifdef VERSION_EU
            LoadBgMap(3, sAllmapBarBgMapsByLanguage[gLanguage], 0x500);
#else
            LoadBgMap(3, gAllmapBarBgMap, 0x500);
#endif
            work->state = ALLMAP_BAR_STATE_IDLE;
            gAllmapModeState = ALLMAP_MODE_STATE_INTRO;
        }

        break;
    case ALLMAP_BAR_STATE_TITLE_OUT:
        ApproachValue(&work->x, work->targetX, work->steps);
        work->steps--;

        if (work->steps == 0) {
            work->steps = 16;
            work->state = ALLMAP_BAR_STATE_BARS_OUT;
        }

        break;
    case ALLMAP_BAR_STATE_BARS_OUT:
        if (!FadeIsActive() && !work->fadeStarted) {
            for (i = 0; i < 32; i++) {
                FadeSetPaletteExcluded(i, FALSE);
            }

            AllmapBarFadeOut(work);
            work->fadeStarted = TRUE;
        }

        ApproachValue(&work->y, work->targetY, work->steps);
        ApproachValue(&work->y2, work->targetY2, work->steps);
        work->steps--;

        if (work->steps == 0) {
            gAllmapModeState = ALLMAP_MODE_STATE_FADE;
            return 0;
        }

        break;
    case ALLMAP_BAR_STATE_IDLE:
        if (work->closing) {
            break;
        }

        if (gAllmapModeState != ALLMAP_MODE_STATE_ACTIVE) {
            break;
        }

        if ((GetKeysPressed() & START_BUTTON) != 0) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            AllmapBarFadeOut(work);
            gAllmapModeState = ALLMAP_MODE_STATE_FADE;
            AllmapClearRoomnameFrame();
            SetAllmapReturnToMenu(FALSE);
            return 0;
        }

        if ((GetKeysPressed() & B_BUTTON) != 0 && !IsStockMesDispActive()) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            AllmapBarStartClose(work);
            gAllmapModeState = ALLMAP_MODE_STATE_BAR_SLIDE;
            AllmapClearRoomnameFrame();
            SetAllmapReturnToMenu(TRUE);
        }

        break;
    }

    return 1;
}

void task_allmap_bar_2(AllmapBarWork* work) {
#ifdef PLATFORM_PC
    /* The same authored ornamental pieces continue into the wide margins,
       at their actual animated Y, before the final BG replaces them. */
    PortMenuBandSource(0, work->y >> 8,
        work->state == ALLMAP_BAR_STATE_IDLE ? NULL : gAllmapBarBandFrame0, work->tiles2, work->palette);
    PortMenuBandSource(1, work->y2 >> 8,
        work->state == ALLMAP_BAR_STATE_IDLE ? NULL : gAllmapBarBandFrame1, work->tiles2, work->palette);
#endif
    if (work->state == ALLMAP_BAR_STATE_IDLE) {
        return;
    }

#ifdef VERSION_EU
    DrawMenuHeader(work->x >> 8, 0, sAllmapBarSprites[gLanguage], work->tiles, work->palette, NULL,
        SPRITE_PRIORITY(3), 1000);
#else
    DrawMenuHeader(work->x >> 8, 0, gAllmapBarTitleFrame0, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 1000);
#endif
    DrawSprite(128, work->y >> 8, gAllmapBarBandFrame0, work->tiles2, work->palette, NULL, SPRITE_PRIORITY(3), 1001);
    DrawSprite(128, work->y2 >> 8, gAllmapBarBandFrame1, work->tiles2, work->palette, NULL, SPRITE_PRIORITY(3), 1002);
}

void task_allmap_bar_3(AllmapBarWork* work) {
#ifdef PLATFORM_PC
    PortMenuBandSource(0, 0, NULL, NULL, NULL);
    PortMenuBandSource(1, 0, NULL, NULL, NULL);
#endif
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette);
}

u8 AllmapHasDoorInfo(AllmapCursorPos pos) {
    AllmapCursorPos neighbor;
    s32 i;
    u8 room;
    u8 hasInfo;

    for (i = 0; i < 4; i++) {
        neighbor.x = pos.x + sAllmapDirDeltas[i][0];
        neighbor.y = pos.y + sAllmapDirDeltas[i][1];
        room = GetAllmapRoomAt(neighbor);

        if (room != MAP_ROOM_NONE) {
            if (GetEventRoomKind(GetAllmapRoomAt(pos)) == EVENT_DOOR_HIDDEN_CHAMBER) {
                hasInfo = AllmapDoorHasKeyInfo(room, sAllmapReverseDoors[i]);
            } else {
                hasInfo = AllmapDoorHasCardInfo(room, sAllmapReverseDoors[i]);
            }

            if (hasInfo) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void AllmapDoorinfoLoadDoors(AllmapDoorinfoWork* work) {
    s32 i;
    AllmapCursorPos pos;
    u8 room;
    u8 value;

    work->count = 0;

    for (i = 0; i < 4; i++) {
        pos.x = work->pos.x + sAllmapDirDeltas[i][0];
        pos.y = work->pos.y + sAllmapDirDeltas[i][1];
        room = GetAllmapRoomAt(pos);

        if (room != MAP_ROOM_NONE && AllmapDoorHasCardInfo(room, sAllmapReverseDoors[i])) {
            value = GetMapRoomCardValue(room) + 1;

            if (value == 10) {
                value = 0;
            }

            work->doors[i].sprite.tiles = AllocKeyValueTiles(value);
            work->doors[i].sprite.palette = LoadObjPalette(gDoorCardPalette, sizeof(gDoorCardPalette));
            work->doors[i].sprite.gfx = NULL;
            work->doors[i].sprite.tiles2 = LoadObjTiles(gCardOutlineWhiteTiles, sizeof(gCardOutlineWhiteTiles));
            work->doors[i].sprite.palette2 = LoadObjPalette(gDoorCardPalette, sizeof(gDoorCardPalette));
            work->doors[i].sprite.gfx2 = gCardOutlineWhiteFrames[0];
            FadeSetPaletteExcluded(BIOS_PTR(work->doors[i].sprite.palette)->index + 16, TRUE);
            FadeSetPaletteExcluded(BIOS_PTR(work->doors[i].sprite.palette2)->index + 16, TRUE);
            work->gfx2[i] = gAllmapDoorinfoArrowFrames[i];
            work->count++;
        } else {
            work->gfx2[i] = NULL;
        }
    }

    if (work->count != 0) {
        work->tiles = LoadObjTiles(gAllmapDoorinfoArrowTiles, sizeof(gAllmapDoorinfoArrowTiles));
        work->palette = LoadObjPalette(gAllmapObjPalette, sizeof(gAllmapObjPalette));
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
        work->tiles2 = LoadObjTiles(gAllmapRoomTiles, sizeof(gAllmapRoomTiles));
        work->gfx = gAllmapRoomFrames[0];
        InitObjPaletteAtSlot(work->palette2, 15, gAllmapRoomPalettes[0], sizeof(gAllmapRoomPalettes[0]));
        FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 16, TRUE);
    }
}

s32 GetAllmapKeyCardX(u16 count, s32 index) {
    s16 keyCardX[4][4];

    memcpy(keyCardX, sAllmapKeyCardX, sizeof(keyCardX));
    return keyCardX[count - 1][index] << 8;
}

void AllmapDoorinfoLoadKeys(AllmapDoorinfoWork* work) {
    s32 i;
    AllmapCursorPos pos;
    u8 room;
    EventKeyCard* card;

    for (i = 0; i < 4; i++) {
        pos.x = work->pos.x + sAllmapDirDeltas[i][0];
        pos.y = work->pos.y + sAllmapDirDeltas[i][1];
        room = GetAllmapRoomAt(pos);

        if (room != MAP_ROOM_NONE && AllmapDoorHasKeyInfo(room, sAllmapReverseDoors[i])) {
            break;
        }
    }

    if (i == 4) {
        work->count = 0;
    } else {
        SelectEventDoor(work->room, sAllmapReverseDoors[i]);
        work->count = CountRemainingEventKeys();
    }

    for (i = 0; i < work->count; i++) {
        card = &work->doors[i];
        InitEventKeyCard(card, GetEventKey(i));
        SetLayeredCardSpritePos(GetAllmapKeyCardX(work->count, i), 0x6800, &card->sprite);

        if (work->doors[i].sprite.palette != NULL) {
            FadeSetPaletteExcluded(BIOS_PTR(work->doors[i].sprite.palette)->index + 16, TRUE);
        }

        if (work->doors[i].sprite.palette2 != NULL) {
            FadeSetPaletteExcluded(BIOS_PTR(work->doors[i].sprite.palette2)->index + 16, TRUE);
        }

        if (work->doors[i].sprite.palette3 != NULL) {
            FadeSetPaletteExcluded(BIOS_PTR(work->doors[i].sprite.palette3)->index + 16, TRUE);
        }
    }

    work->tiles2 = LoadObjTiles(gAllmapRoomTiles, sizeof(gAllmapRoomTiles));

    if (GetEventRoomKind(work->room) == EVENT_DOOR_EVENT_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_BOSS_ROOM) {
        work->gfx = gAllmapRoomFrames[1];
        InitObjPaletteAtSlot(work->palette2, 15, gAllmapRoomPalettes[1], sizeof(gAllmapRoomPalettes[0]));
    } else {
        work->gfx = gAllmapRoomFrames[0];
        InitObjPaletteAtSlot(work->palette2, 15, gAllmapRoomPalettes[0], sizeof(gAllmapRoomPalettes[0]));
    }

    FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 16, TRUE);
}

void task_allmap_doorinfo_0(AllmapDoorinfoWork* work, AllmapCursorPos* arg) {
    s32 i;

    for (i = 0; i < 32; i++) {
        FadeSetPaletteExcluded(i, FALSE);
    }

    work->palette2 = EwramAlloc(sizeof(ObjPalette));
    work->pos = *arg;
    work->room = GetAllmapRoomAt(*arg);
    work->roomX = work->pos.x * 24 - gAllmapCameraX;
    work->roomY = work->pos.y * 24 - gAllmapCameraY;
    work->targetX = 0x6800;

    if (GetEventRoomKind(work->room) == EVENT_DOOR_EVENT_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_BOSS_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_HIDDEN_CHAMBER) {
        work->targetY = 0x2100;
        AllmapDoorinfoLoadKeys(work);
    } else {
        work->targetY = 0x4200;
        AllmapDoorinfoLoadDoors(work);
    }

    work->steps = 8;
    work->x = work->roomX << 8;
    work->y = work->roomY << 8;
    work->closing = FALSE;
    FadeToAmount(FADE_MODE_BLACK, 14, 8);
}

s32 task_allmap_doorinfo_1(AllmapDoorinfoWork* work) {
    if ((GetKeysPressed() & B_BUTTON) != 0 && !work->closing) {
        work->closing = TRUE;
        m4aSongNumStart(SONG_SYS_CLOSE);
        work->steps = 8 - work->steps;
        work->targetX = work->roomX << 8;
        work->targetY = work->roomY << 8;
        FadeToOriginal(FADE_MODE_BLACK, 8);
    }

    if (work->steps != 0) {
        ApproachValue(&work->x, work->targetX, work->steps);
        ApproachValue(&work->y, work->targetY, work->steps);
        work->steps--;
    }

    if (work->closing && work->steps == 0) {
        return 0;
    }

    return 1;
}

void AllmapDoorinfoDrawDoors(AllmapDoorinfoWork* work) {
    s32 i;

    if (gAllmapModeState == ALLMAP_MODE_STATE_FADE) {
        return;
    }

    for (i = 0; i < 4; i++) {
        if (work->gfx2[i] != NULL && work->steps == 0) {
            DrawSprite(work->x >> 8, work->y >> 8, work->gfx2[i], work->tiles, work->palette, NULL, 0, i + 51);
            sAllmapDoorOffsetX = sAllmapDoorCardOffsets[i][0];
            sAllmapDoorOffsetY = sAllmapDoorCardOffsets[i][1];
            DrawSprite(sAllmapDoorOffsetX + (work->x >> 8), sAllmapDoorOffsetY + (work->y >> 8), work->doors[i].sprite.gfx, work->doors[i].sprite.tiles, work->doors[i].sprite.palette, NULL, 0, i + 40);
            DrawSprite((work->x >> 8) + sAllmapDoorOffsetX, (work->y >> 8) + sAllmapDoorOffsetY, work->doors[i].sprite.gfx2, work->doors[i].sprite.tiles2, work->doors[i].sprite.palette2, NULL, 0, i + 30);
        }
    }

    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles2, work->palette2, NULL, 0, 20);
}

void AllmapDoorinfoDrawKeys(AllmapDoorinfoWork* work) {
    s32 i;

    if (work->steps == 0) {
        for (i = 0; i < work->count; i++) {
            DrawLayeredCardSprite(&work->doors[i].sprite, 0);
        }
    }

    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles2, work->palette2, NULL, 0, 20);
}

void task_allmap_doorinfo_2(AllmapDoorinfoWork* work) {
    if (GetEventRoomKind(work->room) == EVENT_DOOR_EVENT_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_BOSS_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_HIDDEN_CHAMBER) {
        AllmapDoorinfoDrawKeys(work);
    } else {
        AllmapDoorinfoDrawDoors(work);
    }
}

void task_allmap_doorinfo_3(AllmapDoorinfoWork* work) {
    s32 i;

    if (!work->closing) {
        FadeToOriginal(FADE_MODE_BLACK, 8);
    }

    ReleaseObjTiles(work->tiles2);

    if (GetEventRoomKind(work->room) == EVENT_DOOR_EVENT_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_BOSS_ROOM || GetEventRoomKind(work->room) == EVENT_DOOR_HIDDEN_CHAMBER) {
        for (i = 0; i < work->count; i++) {
            ReleaseLayeredCardSprite(&work->doors[i].sprite);
        }
    } else {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);

        for (i = 0; i < 4; i++) {
            if (work->gfx2[i] != NULL) {
                ReleaseObjTiles(work->doors[i].sprite.tiles);
                ReleaseObjPalette(work->doors[i].sprite.palette);
                ReleaseObjTiles(work->doors[i].sprite.tiles2);
                ReleaseObjPalette(work->doors[i].sprite.palette2);
            }
        }
    }

    for (i = 0; i < 32; i++) {
        FadeSetPaletteExcluded(i, TRUE);
    }

    FadeSetPaletteExcluded(10, FALSE);
    EwramFree(work->palette2);
}

void task_allmap_pusha_0(AllmapPushaWork* work, AllmapCursorWork* arg) {
    gStockMesDispWork = work;
    work->cursor = arg;
    work->x = arg->pos.x * 24 - gAllmapCameraX;
    work->y = arg->pos.y * 24 - gAllmapCameraY;
    work->tiles = LoadObjTiles(gAllmapPushaTiles, sizeof(gAllmapPushaTiles));
    work->palette = LoadObjPalette(gAllmapObjPalette, sizeof(gAllmapObjPalette));
    work->gfx = gAllmapPushaFrame0;
    work->angle = 0;
    TaskPoolInit(&work->tasks, 1);
    work->task = NULL;
}

s32 task_allmap_pusha_1(AllmapPushaWork* work) {
    if (!IsStockMesDispActive() && (GetKeysPressed() & A_BUTTON) != 0) {
        m4aSongNumStart(SONG_SYS_KETTEI);
        work->task = TaskCreate(&work->tasks, &gTaskDescAllmapDoorinfo, &work->cursor->pos);
    }

    work->y2 = gSineTable[(u8)work->angle] >> 8;
    work->angle += 16;
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_allmap_pusha_2(AllmapPushaWork* work) {
    if (IsStockMesDispActive()) {
        TaskPoolDraw(&work->tasks);
    } else {
        work->x = work->cursor->pos.x * 24 - gAllmapCameraX;
        work->y = work->cursor->pos.y * 24 - gAllmapCameraY;
        DrawSprite(work->x, work->y - work->y2 + 2, work->gfx, work->tiles, work->palette, NULL, 0, 48);
    }
}

void task_allmap_pusha_3(AllmapPushaWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
    ClearStockMesDispWork();
}

u8 IsStockMesDispActive() {
    // @bug The map's title bar also registers itself here (task_allmap_bar_0); read as a
    // pusha, its steps (16) is the task pointer, so the flags are read at 0x1A (BIOS area).
    if (gStockMesDispWork == NULL || !IsTaskActive(BIOS_LOW_PTR(((AllmapPushaWork*)gStockMesDispWork)->task))) {
        return FALSE;
    }

    return TRUE;
}

void ClearStockMesDispWork() {
    gStockMesDispWork = NULL;
}

void AllmapDrawRoomTiles(s16 x, s16 y, s32 shape, u8 selected) {
    AllmapCursorPos pos;
    u16* map;
    s16 tileX;
    s16 tileY;
    s32 i;
    s32 j;
    u8 room;
    u16 tile;
    s16 ofs;

    if ((x & 1) != 0) {
        map = gAllmapBg0Map;
    } else {
        map = gAllmapBg1Map;
    }

    tileX = (x * 24 - sAllmapState->originX) / 8;
    tileY = (y * 24 - sAllmapState->originY) / 8;
    pos.x = x;
    pos.y = y;
    room = GetAllmapRoomAt(pos);
    tile = shape * 16;

    if (TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_EVENT_DONE) != 0 || TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_VISITED) != 0 || AllmapHasDoorInfo(pos)) {
        if (selected) {
            tile += 0x2000;
        } else if (shape == 1) {
            tile += 0x1000;
        } else if (shape == 17) {
            tile += 0xF000;
        }
    } else if (shape == 1 || shape == 17) {
        tile += 0xF000;
    } else {
        tile += 0xE000;
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            ofs = (tileY + i) / 32 * 2048 + (tileY + i) % 32 * 32 + (tileX + j) / 32 * 1024 + (tileX + j) % 32;
            map[ofs] = tile;
            tile++;
        }
    }
}

void InitAllmap() {
    s32 i;
    s32 j;
    AllmapCursorPos arg;
    AllmapRoomWork* room;
    AllmapRoomWork* cursorRoom;
    void** state = &gSharedModeWork;

    *state = EwramAlloc(sizeof(AllmapState));
    sAllmapState->lastRoom = MAP_ROOM_NONE;
    sAllmapState->moveSpeed = 0x400;
    gAllmapCursorRoom = gMapFloorState.room;
    TaskPoolInit(&sAllmapState->tasks, 35);
    sAllmapState->pushaTask = NULL;
    sAllmapState->roomnameTask = NULL;

    for (i = 0; i < 32; i++) {
        sAllmapState->roomTasks[i] = NULL;
    }

    sAllmapState->minX = sAllmapState->maxX = 32;
    sAllmapState->minY = sAllmapState->maxY = 32;
    AllmapAddRoom(0, 32, 32);
    sAllmapState->scrollX = 0;
    sAllmapState->scrollY = 0;
    AllmapSetBounds(sAllmapState->minX, sAllmapState->maxX, sAllmapState->minY, sAllmapState->maxY);
    sAllmapState->introScrollY = (sAllmapState->scrollY + DISPLAY_HEIGHT) << 8;
    sAllmapState->introTargetY = sAllmapState->scrollY << 8;
    gAllmapCameraX = sAllmapState->originX + sAllmapState->scrollX;
    gAllmapCameraY = sAllmapState->originY + sAllmapState->scrollY;
    sAllmapCameraFixedX = (sAllmapState->originX + sAllmapState->scrollX) << 8;
    sAllmapCameraFixedY = (sAllmapState->originY + sAllmapState->scrollY) << 8;

    for (j = 0; j < 32; j++) {
        if (IsTaskActive(sAllmapState->roomTasks[j])) {
            room = sAllmapState->roomTasks[j]->work;
            AllmapDrawRoomTiles(room->x, room->y, room->shape, j == gAllmapCursorRoom);
        }
    }

    RedrawBgMapAt(0, sAllmapState->scrollX - sAllmapState->originX % 8, sAllmapState->scrollY - sAllmapState->originY % 8);
    RedrawBgMapAt(1, sAllmapState->scrollX - sAllmapState->originX % 8, sAllmapState->scrollY - sAllmapState->originY % 8);
    cursorRoom = sAllmapState->roomTasks[gAllmapCursorRoom]->work;
    arg.x = cursorRoom->x;
    arg.y = cursorRoom->y;
    sAllmapState->cursorTask = TaskCreate(&sAllmapState->tasks, &sTaskDescAllmapCursor, &arg);
    AllmapInitDropOffsets();
}

void AllmapUpdateCamera(AllmapState* state) {
    s32 targetX;
    s32 targetY;
    s32 dx;
    s32 dy;
    s32 prevX;
    s32 prevY;

    targetX = (state->originX + state->scrollX) << 8;
    targetY = (state->originY + state->scrollY) << 8;
    dx = (targetX - sAllmapCameraFixedX) >> 3;
    dy = (targetY - sAllmapCameraFixedY) >> 3;

    if (dx > 0x800) {
        dx = 0x800;
    } else if (dx < -0x800) {
        dx = -0x800;
    }

    if (dy > 0x800) {
        dy = 0x800;
    } else if (dy < -0x800) {
        dy = -0x800;
    }

    prevX = sAllmapCameraFixedX;
    prevY = sAllmapCameraFixedY;
    sAllmapCameraFixedX += dx;
    sAllmapCameraFixedY += dy;

    if ((prevX - sAllmapCameraFixedX >= 0 ? prevX - sAllmapCameraFixedX : sAllmapCameraFixedX - prevX) <= 7) {
        sAllmapCameraFixedX = targetX;
    }

    if ((prevY - sAllmapCameraFixedY >= 0 ? prevY - sAllmapCameraFixedY : sAllmapCameraFixedY - prevY) <= 7) {
        sAllmapCameraFixedY = targetY;
    }

    gAllmapCameraX = sAllmapCameraFixedX >> 8;
    gAllmapCameraY = sAllmapCameraFixedY >> 8;
}

void UpdateAllmap() {
    s16 x;
    s16 y;

    if (!FadeIsActive() && gAllmapModeState == ALLMAP_MODE_STATE_ACTIVE) {
        AllmapHandleInput();
    }

    if (gAllmapModeState == ALLMAP_MODE_STATE_INTRO) {
        if (gAllmapScrollInTimer > 6) {
            ApproachValue(&sAllmapState->introScrollY, sAllmapState->introTargetY - 0x200, gAllmapScrollInTimer - 7);
        } else if (gAllmapScrollInTimer & 1) {
            ApproachValue(&sAllmapState->introScrollY, sAllmapState->introTargetY, 1);
        } else {
            ApproachValue(&sAllmapState->introScrollY, sAllmapState->introTargetY - 0x200, 1);
        }

        sAllmapState->scrollY = sAllmapState->introScrollY >> 8;
    }

    if (gAllmapModeState == ALLMAP_MODE_STATE_ACTIVE) {
        AllmapUpdateCamera(sAllmapState);
    } else {
        gAllmapCameraX = sAllmapState->originX + sAllmapState->scrollX;
        gAllmapCameraY = sAllmapState->originY + sAllmapState->scrollY;
        sAllmapCameraFixedX = (sAllmapState->originX + sAllmapState->scrollX) << 8;
        sAllmapCameraFixedY = (sAllmapState->originY + sAllmapState->scrollY) << 8;
    }

    x = gAllmapCameraX - sAllmapState->originX;
    y = gAllmapCameraY - sAllmapState->originY;
    ScrollBgMapTo(0, x - sAllmapState->originX % 8, y - sAllmapState->originY % 8);
    ScrollBgMapTo(1, x - sAllmapState->originX % 8, y - sAllmapState->originY % 8);
    TaskPoolUpdate(&sAllmapState->tasks);
    TaskPoolDraw(&sAllmapState->tasks);
}

void DestroyAllmap() {
    TaskPoolDestroy(&sAllmapState->tasks);
    EwramFree(gSharedModeWork);
}

u16 GetAllmapMoveSpeed() {
    if ((GetKeysHeld() & R_BUTTON) != 0) {
        return sAllmapState->moveSpeed >> 7;
    }

    return sAllmapState->moveSpeed >> 8;
}

void AllmapInitDropOffsets() {
    AllmapRoomWork* room;
    AllmapCursorWork* cursor;
    s32 base;
    u8 i;

    base = (sAllmapState->maxY * 24 - gAllmapCameraY) << 9;

    for (i = 0; i < ARRAY_COUNT(sAllmapState->roomTasks); i++) {
        if (IsTaskActive(sAllmapState->roomTasks[i])) {
            room = sAllmapState->roomTasks[i]->work;
            room->dropTargetY = (room->y * 24 - gAllmapCameraY) << 8;
            room->dropY = room->dropTargetY - base;
        }
    }

    if (IsTaskActive(sAllmapState->cursorTask)) {
        cursor = sAllmapState->cursorTask->work;
        cursor->dropY = cursor->dropTargetY - base;
    }
}

s32 GetAllmapRoomAt(AllmapCursorPos pos) {
    AllmapRoomWork* room;
    u8 i;

    for (i = 0; i < ARRAY_COUNT(sAllmapState->roomTasks); i++) {
        if (IsTaskActive(sAllmapState->roomTasks[i])) {
            room = sAllmapState->roomTasks[i]->work;

            if (pos.x == room->x && pos.y == room->y) {
                return i;
            }
        }
    }

    return MAP_ROOM_NONE;
}

void AllmapCenterOnRoom() {
    AllmapRoomWork* room;

    room = sAllmapState->roomTasks[gAllmapCursorRoom]->work;
    sAllmapState->scrollY = room->y * 24 - sAllmapState->originY - 69;

    if (sAllmapState->scrollY < 0 || sAllmapState->height <= DISPLAY_HEIGHT - 1) {
        sAllmapState->scrollY = 0;
    } else if (sAllmapState->scrollY > sAllmapState->height - DISPLAY_HEIGHT) {
        sAllmapState->scrollY = sAllmapState->height - DISPLAY_HEIGHT;
    }

    if ((s16)(room->y * 24 - sAllmapState->originY) <= 15) {
        sAllmapState->scrollY -= 16;
    }

    sAllmapState->scrollX = room->x * 24 - sAllmapState->originX - 104;

    if (sAllmapState->scrollX < 0 || sAllmapState->width <= DISPLAY_WIDTH - 1) {
        sAllmapState->scrollX = 0;
    } else if (sAllmapState->scrollX > sAllmapState->width - DISPLAY_WIDTH) {
        sAllmapState->scrollX = sAllmapState->width - DISPLAY_WIDTH;
    }
}

void AllmapHandleInput() {
    AllmapCursorWork* cursor;
    AllmapCursorPos pos;
    MapFloorRoom* floorRoom;
    u8 moved;
    u8 room;

    moved = FALSE;
    cursor = sAllmapState->cursorTask->work;
    pos = cursor->pos;

    switch (GetKeysRepeat()) {
    case DPAD_UP:
        pos.x++;
        pos.y--;
        moved = TRUE;
        break;
    case DPAD_RIGHT:
        pos.x++;
        pos.y++;
        moved = TRUE;
        break;
    case DPAD_LEFT:
        pos.x--;
        pos.y--;
        moved = TRUE;
        break;
    case DPAD_DOWN:
        pos.x--;
        pos.y++;
        moved = TRUE;
        break;
    }

    room = GetAllmapRoomAt(pos);

    if (room == MAP_ROOM_NONE) {
        return;
    }

    cursor->pos = pos;

    if (sAllmapState->lastRoom == room) {
        return;
    }

    sAllmapState->lastRoom = room;
    gAllmapCursorRoom = room;

    if (moved) {
        m4aSongNumStart(SONG_SYS_CLICK);
        AllmapCenterOnRoom();
    }

    if (IsTaskActive(sAllmapState->roomnameTask)) {
        TaskKill(&sAllmapState->tasks, sAllmapState->roomnameTask);
    }

    floorRoom = GetMapFloorRoom(room);

    if (floorRoom->nameId != ROOM_NAME_UNKNOWN_PLACE && (TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_VISITED) != 0 || TestAllmapRoomFlag(room, FLOOR_ROOM_FLAG_EVENT_DONE) != 0)) {
        sAllmapState->roomnameTask = TaskCreate(&sAllmapState->tasks, &sTaskDescAllmapRoomname, &floorRoom->nameId);
    } else {
        sAllmapState->roomnameTask = NULL;
        AllmapClearRoomnameFrame();
    }

    if (IsTaskActive(sAllmapState->pushaTask)) {
        TaskKill(&sAllmapState->tasks, sAllmapState->pushaTask);
    }

    if (AllmapHasDoorInfo(cursor->pos)) {
        sAllmapState->pushaTask = TaskCreate(&sAllmapState->tasks, &gTaskDescAllmapPusha, cursor);
    }
}

void AllmapAddRoom(u8 id, u16 x, u16 y) {
    AllmapRoomArg arg;
    u8* links;
    u8 room;

    links = GetMapRoomLinks(id);

    if (IsTaskActive(sAllmapState->roomTasks[id])) {
        return;
    }

    if (sAllmapState->maxX < x) {
        sAllmapState->maxX = x;
    }

    if (sAllmapState->minX > x) {
        sAllmapState->minX = x;
    }

    if (sAllmapState->maxY < y) {
        sAllmapState->maxY = y;
    }

    if (sAllmapState->minY > y) {
        sAllmapState->minY = y;
    }

    arg.x = x;
    arg.y = y;
    arg.room = id;
    arg.asSprite = FALSE;
    sAllmapState->roomTasks[id] = TaskCreate(&sAllmapState->tasks, &sTaskDescAllmapRoom, &arg);

    room = links[MAP_DOOR_SIDE_UP_RIGHT];

    if ((u8)(room + 3) > 2) {
        AllmapAddRoom(room, x + 1, y - 1);
    }

    room = links[MAP_DOOR_SIDE_DOWN_LEFT];

    if ((u8)(room + 3) > 2) {
        AllmapAddRoom(room, x - 1, y + 1);
    }

    room = links[MAP_DOOR_SIDE_DOWN_RIGHT];

    if ((u8)(room + 3) > 2) {
        AllmapAddRoom(room, x + 1, y + 1);
    }

    room = links[MAP_DOOR_SIDE_UP_LEFT];

    if ((u8)(room + 3) > 2) {
        AllmapAddRoom(room, x - 1, y - 1);
    }
}

void AllmapSetBounds(u16 minX, u16 maxX, u16 minY, u16 maxY) {
    u16 dx;
    u16 dy;

    sAllmapState->height = (maxY - minY) * 24 + 32;

    if (sAllmapState->height <= DISPLAY_HEIGHT - 1) {
        dy = (DISPLAY_HEIGHT - sAllmapState->height) / 2;
    } else {
        dy = 0;
    }

    sAllmapState->width = (maxX - minX) * 24 + 32;

    if (sAllmapState->width <= DISPLAY_WIDTH - 1) {
        dx = (DISPLAY_WIDTH - sAllmapState->width) / 2;
    } else {
        dx = 0;
    }

    sAllmapState->originX = minX * 24 - dx;
    sAllmapState->originY = minY * 24 - dy;
    AllmapCenterOnRoom();
}

u8 TestAllmapRoomFlag(u8 room, u16 flag) {
    return *(u8*)GetMapFloorRoom(room) & flag;
}

void* GetAllmapRoomWork(u8 room) {
    return sAllmapState->roomTasks[room]->work;
}

TaskDesc gTaskDescAllmapDoorinfo = {
    "task_allmap_doorinfo",
    (TaskInitFunc)task_allmap_doorinfo_0,
    (TaskUpdateFunc)task_allmap_doorinfo_1,
    (TaskDrawFunc)task_allmap_doorinfo_2,
    (TaskDestroyFunc)task_allmap_doorinfo_3,
    sizeof(AllmapDoorinfoWork),
};

TaskDesc gTaskDescAllmapPusha = {
    "task_allmap_pusha",
    (TaskInitFunc)task_allmap_pusha_0,
    (TaskUpdateFunc)task_allmap_pusha_1,
    (TaskDrawFunc)task_allmap_pusha_2,
    (TaskDestroyFunc)task_allmap_pusha_3,
    sizeof(AllmapPushaWork),
};
