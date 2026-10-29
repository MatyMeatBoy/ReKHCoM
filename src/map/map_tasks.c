/**
 * map_tasks.c
 * Field Enemies and Gimmicks
 */

#include "monsgage.h"
#include "map_tasks.h"
#include "sprites_btl.h"
#include "sprites_emy.h"
#include "sprites_evt.h"
#include "sprites_map.h"
#include "sprites_map_tasks.h"
#include "sprites_card_pictures.h"
#include "system_state.h"
#include "gba/keys.h"
#include "engine_math.h"
#include "map_runtime.h"
#include "fade.h"
#include "songs.h"
#include <stdlib.h>
#include "world_types.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "btl_collision.h"
#include "card.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "display.h"
#include "field_state.h"
#include "fld_types.h"
#include "game_state.h"
#include "key.h"
#include "m4a_song.h"
#include "map.h"
#include "map_api.h"
#include "map_types.h"
#include "mode.h"
#include "msg_api.h"
#include "obj.h"
#include "obj_api.h"
#include "player_progression_types.h"
#include "registration_data.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "text.h"
#include "types.h"
#include <stddef.h>
#include "lockon.h"
#include "map_room_tables.h"
#include "sprite_palettes.h"
#include "card_message_data.h"
#include "gba/defines.h"
#include "macros.h"

static const AnimDef sMapEnm00AnimDefs[10] = {
    { gEmy00L06Frames, gEmy00L06Anims, gEmy00L06Tiles, 0 },
    { gEmy00L06Frames, gEmy00L06Anims, gEmy00L06Tiles, 0 },
    { gEmy00L00BackFrames, gEmy00L00BackAnims, gEmy00L00BackTiles, 0 },
    { gEmy00L00Frames, gEmy00L00Anims, gEmy00L00Tiles, 0 },
    { gEmy00L02BackFrames, gEmy00L02BackAnims, gEmy00L02BackTiles, 0 },
    { gEmy00L02Frames, gEmy00L02Anims, gEmy00L02Tiles, 0 },
    { gEmy00L07BackFrames, gEmy00L07BackAnims, gEmy00L07BackTiles, 0 },
    { gEmy00L07Frames, gEmy00L07Anims, gEmy00L07Tiles, 0 },
    { gEmy00L09Frames, gEmy00L09Anims, gEmy00L09Tiles, 0 },
    { gEmy00L09Frames, gEmy00L09Anims, gEmy00L09Tiles, 0 },
};

const AnimDef gMapEnm00AnimDef10 = {
    gEmy00PeekFrames, gEmy00PeekAnims, gEmy00PeekTiles, 1,
};

const AnimDef gMapEnm00AnimDef11 = {
    gEmy00PeekFrames, gEmy00PeekAnims, gEmy00PeekTiles, 1,
};

const AnimDef gMapEnm00AnimDef12 = {
    gEmy00PeekFrames, gEmy00PeekAnims, gEmy00PeekTiles, 0,
};

const AnimDef gMapEnm00AnimDef13 = {
    gEmy00PeekFrames, gEmy00PeekAnims, gEmy00PeekTiles, 0,
};

const AnimDef gMapEnm00AnimDef14 = {
    gEmy00PeekFrames, gEmy00PeekAnims, gEmy00PeekTiles, 2,
};

const AnimDef gMapEnm00AnimDef15 = {
    gEmy00PeekFrames, gEmy00PeekAnims, gEmy00PeekTiles, 2,
};

const MapEnmDef gMapEnm00Def = {
    sMapEnm00AnimDefs, gEmy00Palette,
    32, 8, 16,
    &gTaskDescMapEnm00, MAP_ENM_DEF_FLAG_NO_SHADOW,
};

TaskDesc gTaskDescMapEnm00 = {
    "Task_MapEnm00",
    (TaskInitFunc)Task_MapEnm00_0,
    (TaskUpdateFunc)Task_MapEnm00_1,
    (TaskDrawFunc)Task_MapEnm00_2,
    (TaskDestroyFunc)Task_MapEnm00_3,
    sizeof(MapEnmWork),
};

static const AnimDef sMapEnm01AnimDefs[6] = {
    { gEmy01L00Frames, gEmy01L00Anims, gEmy01L00Tiles, 0 },
    { gEmy01L00Frames, gEmy01L00Anims, gEmy01L00Tiles, 0 },
    { gEmy01L00Frames, gEmy01L00Anims, gEmy01L00Tiles, 2 },
    { gEmy01L00Frames, gEmy01L00Anims, gEmy01L00Tiles, 2 },
    { gEmy01L00Frames, gEmy01L00Anims, gEmy01L00Tiles, 3 },
    { gEmy01L00Frames, gEmy01L00Anims, gEmy01L00Tiles, 3 },
};

const MapEnmDef gMapEnm01Def = {
    sMapEnm01AnimDefs, gEmy01Palette,
    17, 16, 16,
    &gTaskDescMapEnm01, MAP_ENM_DEF_FLAG_AIRBORNE,
};

TaskDesc gTaskDescMapEnm01 = {
    "Task_MapEnm01",
    (TaskInitFunc)Task_MapEnm01_0,
    (TaskUpdateFunc)Task_MapEnm01_1,
    (TaskDrawFunc)Task_MapEnm01_2,
    (TaskDestroyFunc)Task_MapEnm01_3,
    sizeof(MapEnm01Work),
};

static const AnimDef sMapEnm02AnimDefs[2] = {
    { gEmy3800Frames, gEmy3800Anims, gEmy3800Tiles, 0 },
    { gEmy3800Frames, gEmy3800Anims, gEmy3800Tiles, 0 },
};

const AnimDef gMapEnm02AnimDef2 = {
    gEmy3801Frames, gEmy3801Anims, gEmy3801Tiles, 0,
};

const AnimDef gMapEnm02AnimDef3 = {
    gEmy3801Frames, gEmy3801Anims, gEmy3801Tiles, 0,
};

const MapEnmDef gMapEnm02Def = {
    sMapEnm02AnimDefs, gEmy38Palette,
    106, 48, 36,
    &gTaskDescMapEnm02, MAP_ENM_DEF_FLAG_GUARD,
};

TaskDesc gTaskDescMapEnm02 = {
    "Task_MapEnm02",
    (TaskInitFunc)Task_MapEnm02_0,
    (TaskUpdateFunc)Task_MapEnm02_1,
    (TaskDrawFunc)Task_MapEnm02_2,
    (TaskDestroyFunc)Task_MapEnm02_3,
    sizeof(MapEnmWork),
};

static const AnimDef sMapEnm03AnimDefs[4] = {
    { gEmy2900Frames, gEmy2900Anims, gEmy2900Tiles, 0 },
    { gEmy2900Frames, gEmy2900Anims, gEmy2900Tiles, 0 },
    { gEmy2900Frames, gEmy2900Anims, gEmy2900Tiles, 0 },
    { gEmy2900Frames, gEmy2900Anims, gEmy2900Tiles, 0 },
};

const AnimDef gMapEnm03AnimDef4 = {
    gEmy2902Frames, gEmy2902Anims, gEmy2902Tiles, 0,
};

const AnimDef gMapEnm03AnimDef5 = {
    gEmy2902Frames, gEmy2902Anims, gEmy2902Tiles, 0,
};

const MapEnmDef gMapEnm03Def = {
    sMapEnm03AnimDefs, gEmy29Palette,
    73, 56, 36,
    &gTaskDescMapEnm03, MAP_ENM_DEF_FLAG_AIRBORNE | MAP_ENM_DEF_FLAG_GUARD,
};

TaskDesc gTaskDescMapEnm03 = {
    "Task_MapEnm03",
    (TaskInitFunc)Task_MapEnm03_0,
    (TaskUpdateFunc)Task_MapEnm03_1,
    (TaskDrawFunc)Task_MapEnm03_2,
    (TaskDestroyFunc)Task_MapEnm03_3,
    sizeof(MapEnm03Work),
};

static const AnimDef sMapEnm04AnimDefs[6] = {
    { gEmy0600BackFrames, gEmy0600BackAnims, gEmy0600BackTiles, 0 },
    { gEmy0600Frames, gEmy0600Anims, gEmy0600Tiles, 0 },
    { gEmy0600BackFrames, gEmy0600BackAnims, gEmy0600BackTiles, 0 },
    { gEmy0600Frames, gEmy0600Anims, gEmy0600Tiles, 0 },
    { gEmy0603Frames, gEmy0603Anims, gEmy0603Tiles, 0 },
    { gEmy0603Frames, gEmy0603Anims, gEmy0603Tiles, 0 },
};

const MapEnmDef gMapEnm04Def = {
    sMapEnm04AnimDefs, gEmy06Palette,
    34, 16, 12,
    &gTaskDescMapEnm04, MAP_ENM_DEF_FLAG_AIRBORNE,
};

TaskDesc gTaskDescMapEnm04 = {
    "Task_MapEnm04",
    (TaskInitFunc)Task_MapEnm04_0,
    (TaskUpdateFunc)Task_MapEnm04_1,
    (TaskDrawFunc)Task_MapEnm04_2,
    (TaskDestroyFunc)Task_MapEnm04_3,
    sizeof(MapEnm01Work),
};

static const AnimDef sMapEnm05AnimDefs[8] = {
    { gEmy07Fl01Frames, gEmy07Fl01Anims, gEmy07Fl01Tiles, 0 },
    { gEmy07Fl01Frames, gEmy07Fl01Anims, gEmy07Fl01Tiles, 0 },
    { gEmy07Fl00Frames, gEmy07Fl00Anims, gEmy07Fl00Tiles, 0 },
    { gEmy07Fl00Frames, gEmy07Fl00Anims, gEmy07Fl00Tiles, 0 },
    { gEmy07Fl02Frames, gEmy07Fl02Anims, gEmy07Fl02Tiles, 0 },
    { gEmy07Fl02Frames, gEmy07Fl02Anims, gEmy07Fl02Tiles, 0 },
    { gEmy07Fl03Frames, gEmy07Fl03Anims, gEmy07Fl03Tiles, 0 },
    { gEmy07Fl03Frames, gEmy07Fl03Anims, gEmy07Fl03Tiles, 0 },
};

const MapEnmDef gMapEnm05Def = {
    sMapEnm05AnimDefs, gEmy07Palette,
    26, 16, 12,
    &gTaskDescMapEnm05, MAP_ENM_DEF_FLAG_SPAWN_ANYWHERE,
};

TaskDesc gTaskDescMapEnm05 = {
    "Task_MapEnm05",
    (TaskInitFunc)Task_MapEnm05_0,
    (TaskUpdateFunc)Task_MapEnm05_1,
    (TaskDrawFunc)Task_MapEnm05_2,
    (TaskDestroyFunc)Task_MapEnm05_3,
    sizeof(MapEnmWork),
};

static const AnimDef sMapEnm06AnimDefs[8] = {
    { gEmy07Fl01Frames, gEmy07Fl01Anims, gEmy07Fl01Tiles, 0 },
    { gEmy07Fl01Frames, gEmy07Fl01Anims, gEmy07Fl01Tiles, 0 },
    { gEmy07Fl00Frames, gEmy07Fl00Anims, gEmy07Fl00Tiles, 0 },
    { gEmy07Fl00Frames, gEmy07Fl00Anims, gEmy07Fl00Tiles, 0 },
    { gEmy07Fl02Frames, gEmy07Fl02Anims, gEmy07Fl02Tiles, 0 },
    { gEmy07Fl02Frames, gEmy07Fl02Anims, gEmy07Fl02Tiles, 0 },
    { gEmy07Fl03Frames, gEmy07Fl03Anims, gEmy07Fl03Tiles, 0 },
    { gEmy07Fl03Frames, gEmy07Fl03Anims, gEmy07Fl03Tiles, 0 },
};

const MapEnmDef gMapEnm06Def = {
    sMapEnm06AnimDefs, gEmy07bPalette,
    26, 16, 12,
    &gTaskDescMapEnm06, MAP_ENM_DEF_FLAG_SPAWN_ANYWHERE,
};

TaskDesc gTaskDescMapEnm06 = {
    "Task_MapEnm06",
    (TaskInitFunc)Task_MapEnm06_0,
    (TaskUpdateFunc)Task_MapEnm06_1,
    (TaskDrawFunc)Task_MapEnm06_2,
    (TaskDestroyFunc)Task_MapEnm06_3,
    sizeof(MapEnmWork),
};

static u8 sMapDbgCursorString[] = "_";

TaskDesc gTaskDescMapDbg = {
    "Task_MapDbg",
    (TaskInitFunc)Task_MapDbg_0,
    (TaskUpdateFunc)Task_MapDbg_1,
    (TaskDrawFunc)Task_MapDbg_2,
    (TaskDestroyFunc)Task_MapDbg_3,
    sizeof(MapDbgWork),
};

TaskDesc gTaskDescMapGmkJump = {
    "Task_MapGmk_Jump",
    (TaskInitFunc)Task_MapGmk_Jump_0,
    (TaskUpdateFunc)Task_MapGmk_Jump_1,
    (TaskDrawFunc)Task_MapGmk_Jump_2,
    (TaskDestroyFunc)Task_MapGmk_Jump_3,
    sizeof(MapGmkJumpWork),
};

static TaskDesc sTaskDescMapGmkEnm = {
    "Task_MapGmk_Enm",
    (TaskInitFunc)Task_MapGmk_Enm_0,
    (TaskUpdateFunc)Task_MapGmk_Enm_1,
    (TaskDrawFunc)Task_MapGmk_Enm_2,
    (TaskDestroyFunc)Task_MapGmk_Enm_3,
    sizeof(MapGmkEnmWork),
};

TaskDesc gTaskDescMapGmkDmy = {
    "Task_MapGmk_Dmy",
    (TaskInitFunc)Task_MapGmk_Dmy_0,
    (TaskUpdateFunc)Task_MapGmk_Dmy_1,
    (TaskDrawFunc)Task_MapGmk_Dmy_2,
    (TaskDestroyFunc)Task_MapGmk_Dmy_3,
    sizeof(MapGmkDmyWork),
};

TaskDesc gTaskDescMapGmkTutorial = {
    "Task_MapGmk_Tutorial",
    (TaskInitFunc)Task_MapGmk_Tutorial_0,
    (TaskUpdateFunc)Task_MapGmk_Tutorial_1,
    (TaskDrawFunc)Task_MapGmk_Tutorial_2,
    (TaskDestroyFunc)Task_MapGmk_Tutorial_3,
    sizeof(MapGmkTutorialWork),
};

static TaskDesc sTaskDescMapGmkSpider = {
    "Task_MapGmk_Spider",
    (TaskInitFunc)Task_MapGmk_Spider_0,
    (TaskUpdateFunc)Task_MapGmk_Spider_1,
    (TaskDrawFunc)Task_MapGmk_Spider_2,
    (TaskDestroyFunc)Task_MapGmk_Spider_3,
    sizeof(MapGmkSpiderWork),
};

TaskDesc gTaskDescMapGmkGP00 = {
    "Task_MapGmk_GP00",
    (TaskInitFunc)Task_MapGmk_GP00_0,
    (TaskUpdateFunc)Task_MapGmk_GP00_1,
    (TaskDrawFunc)Task_MapGmk_GP00_2,
    (TaskDestroyFunc)Task_MapGmk_GP00_3,
    sizeof(MapGmkGpWork),
};

TaskDesc gTaskDescMapGmkGP01 = {
    "Task_MapGmk_GP01",
    (TaskInitFunc)Task_MapGmk_GP01_0,
    (TaskUpdateFunc)Task_MapGmk_GP01_1,
    (TaskDrawFunc)Task_MapGmk_GP01_2,
    (TaskDestroyFunc)Task_MapGmk_GP01_3,
    sizeof(MapGmkGp1Work),
};

TaskDesc gTaskDescMapGmkGP02 = {
    "Task_MapGmk_GP02",
    (TaskInitFunc)Task_MapGmk_GP02_0,
    (TaskUpdateFunc)Task_MapGmk_GP02_1,
    (TaskDrawFunc)Task_MapGmk_GP02_2,
    (TaskDestroyFunc)Task_MapGmk_GP02_3,
    sizeof(MapGmkGpWork),
};

TaskDesc gTaskDescMapGmkGP03 = {
    "Task_MapGmk_GP03",
    (TaskInitFunc)Task_MapGmk_GP03_0,
    (TaskUpdateFunc)Task_MapGmk_GP03_1,
    (TaskDrawFunc)Task_MapGmk_GP03_2,
    (TaskDestroyFunc)Task_MapGmk_GP03_3,
    sizeof(MapGmkGpWork),
};

TaskDesc gTaskDescMapGmkGP04 = {
    "Task_MapGmk_GP04",
    (TaskInitFunc)Task_MapGmk_GP04_0,
    (TaskUpdateFunc)Task_MapGmk_GP04_1,
    (TaskDrawFunc)Task_MapGmk_GP04_2,
    (TaskDestroyFunc)Task_MapGmk_GP04_3,
    sizeof(MapGmkGpWork),
};

TaskDesc gTaskDescMapGmkGP05 = {
    "Task_MapGmk_GP05",
    (TaskInitFunc)Task_MapGmk_GP05_0,
    (TaskUpdateFunc)Task_MapGmk_GP05_1,
    (TaskDrawFunc)Task_MapGmk_GP05_2,
    (TaskDestroyFunc)Task_MapGmk_GP05_3,
    sizeof(MapGmkGpWork),
};

TaskDesc gTaskDescMapGmkGP06 = {
    "Task_MapGmk_GP06",
    (TaskInitFunc)Task_MapGmk_GP06_0,
    (TaskUpdateFunc)Task_MapGmk_GP06_1,
    (TaskDrawFunc)Task_MapGmk_GP06_2,
    (TaskDestroyFunc)Task_MapGmk_GP06_3,
    sizeof(MapGmkGpWork),
};

TaskDesc gTaskDescMapGmkGP07 = {
    "Task_MapGmk_GP07",
    (TaskInitFunc)Task_MapGmk_GP07_0,
    (TaskUpdateFunc)Task_MapGmk_GP07_1,
    (TaskDrawFunc)Task_MapGmk_GP07_2,
    (TaskDestroyFunc)Task_MapGmk_GP07_3,
    sizeof(MapGmkGp07Work),
};

TaskDesc gTaskDescMapGmkGP08 = {
    "Task_MapGmk_GP08",
    (TaskInitFunc)Task_MapGmk_GP08_0,
    (TaskUpdateFunc)Task_MapGmk_GP08_1,
    (TaskDrawFunc)Task_MapGmk_GP08_2,
    (TaskDestroyFunc)Task_MapGmk_GP08_3,
    sizeof(MapGmkGp08Work),
};

TaskDesc gTaskDescMapGmkGP09 = {
    "Task_MapGmk_GP09",
    (TaskInitFunc)Task_MapGmk_GP09_0,
    (TaskUpdateFunc)Task_MapGmk_GP09_1,
    (TaskDrawFunc)Task_MapGmk_GP09_2,
    (TaskDestroyFunc)Task_MapGmk_GP09_3,
    sizeof(MapGmkGp09Work),
};

TaskDesc gTaskDescMapGmk00 = {
    "Task_MapGmk00",
    (TaskInitFunc)Task_MapGmk00_0,
    (TaskUpdateFunc)Task_MapGmk00_1,
    (TaskDrawFunc)Task_MapGmk00_2,
    (TaskDestroyFunc)Task_MapGmk00_3,
    sizeof(MapGmk00Work),
};

/*
 * Field enemies are removed once they leave the screen (plus a small margin).
 * The PC wide view shows more than 240x160, so the bounds follow the visible
 * area; otherwise enemies vanish in plain sight and respawn elsewhere. The
 * debug option "GBA map logic" keeps the cartridge's 240x160 bounds.
 */
#ifdef PLATFORM_PC
int PortGbaMapLogic(void);
int PortGbaMapFade(void);
#define MAP_ENM_FADE_FRAMES 24
#define MAP_ENM_VIEW(view, gba) (PortGbaMapLogic() ? (gba) : (view))
#else
#define MAP_ENM_VIEW(view, gba) (gba)
#endif
#define MAP_ENM_OFF_LEFT (gFieldState->x + MAP_ENM_VIEW(VIEW_LEFT, 0) * 0x100 - 0x1800)
#define MAP_ENM_OFF_RIGHT (gFieldState->x + MAP_ENM_VIEW(VIEW_RIGHT - DISPLAY_WIDTH, 0) * 0x100 + 0x10800)
#define MAP_ENM_OFF_TOP (gFieldState->y + MAP_ENM_VIEW(VIEW_TOP, 0) * 0x100 - 0x800)
#define MAP_ENM_OFF_BOTTOM (gFieldState->y + MAP_ENM_VIEW(VIEW_BOTTOM - DISPLAY_HEIGHT, 0) * 0x100 + 0xC000)

s32 MapEnm00CheckOffscreen(MapEnmWork* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (work->obj.fieldPosition.x < MAP_ENM_OFF_LEFT || work->obj.fieldPosition.x > MAP_ENM_OFF_RIGHT ||
        pos->y + pos->z < MAP_ENM_OFF_TOP || pos->y + pos->z > MAP_ENM_OFF_BOTTOM) {
#ifdef PLATFORM_PC
        /* GBA logic (Improved): it leaves at the cartridge's distance, but fades out first (MapEnmDraw). */
        if (PortGbaMapFade() && work->unk_D2 < MAP_ENM_FADE_FRAMES) {
            if (work->unk_D2 == 0) {
                ColliderSetDisabled(&work->collider, TRUE);
            }

            work->unk_D2++;
            return FALSE;
        }
#endif
        work->update = NULL;
        ColliderSetDisabled(&work->collider, TRUE);
        return TRUE;
    }

    return FALSE;
}

void MapEnm00Move(MapEnmWork* work, s32 accel, s32 maxSpeed) {
    FldObj* obj = &work->obj;

    if (work->flags & MAP_ENM_FLAG_SLOW) {
        accel /= 5;
        maxSpeed /= 5;
    }

    work->obj.fieldPosition.x += gSineTable[obj->angle] * obj->speed >> 8;
    obj->fieldPosition.y += -gSineTable[obj->angle + 64] * obj->speed >> 8;
    obj->speed += accel;

    if (obj->speed > maxSpeed) {
        obj->speed = maxSpeed;
    }
}

void MapEnm00CheckBlocked(MapEnmWork* work, s32 x, s32 y) {
    FldPos* pos = &work->obj.fieldPosition;

    if (IsFldPosBlocked(pos) != 0 || GetFldPosGround(pos) != pos->z) {
        work->obj.fieldPosition.x = x;
        pos->y = y;
        work->update = MapEnm00Vanish;
        ColliderSetDisabled(&work->collider, TRUE);
    }
}

s32 MapEnm00SpotPlayer(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    u8 ang;

    if (gFieldState->actor.fieldPosition.ground != obj->fieldPosition.ground) {
        return FALSE;
    }

    ang = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);

    if (abs(GetAngleDiff(ang, obj->angle)) > 0x18) {
        return FALSE;
    }

    obj->angle = ang;
    return TRUE;
}

void MapEnm00Appear(MapEnmWork* work) {
    MapEnmSetAnim(work, 0, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        if (work->flags & MAP_ENM_FLAG_AGGRESSIVE) {
            work->update = MapEnm00Pursue;
        } else {
            work->update = MapEnm00Idle;
        }

        work->timer = 0;
        ColliderSetDisabled(&work->collider, FALSE);
    } else {
        MapEnmUpdateAnim(work);

        if (work->colliderDelay > 0) {
            work->colliderDelay--;

            if (work->colliderDelay <= 0) {
                ColliderSetDisabled(&work->collider, FALSE);
            }
        }
    }
}

void MapEnm00Idle(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    s32 x;
    s32 y;

    MapEnmSetAnim(work, 1, 1);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    x = work->obj.fieldPosition.x;
    y = obj->fieldPosition.y;

    if ((u8)MapEnm00SpotPlayer(work)) {
        work->timer = 0;
        work->update = MapEnm00Chase;
    } else if (GetRandom() % 80 == 0) {
        switch (GetRandom() % 4) {
        case 0:
            obj->angle = FLD_ANGLE_DOWN_LEFT;
            break;
        case 1:
            obj->angle = FLD_ANGLE_DOWN_RIGHT;
            break;
        case 2:
            obj->angle = FLD_ANGLE_UP_LEFT;
            break;
        default:
            obj->angle = FLD_ANGLE_UP_RIGHT;
            break;
        }

        work->timer = 0;
        work->update = MapEnm00Wander;
    }

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm00Hit;
    } else {
        MapEnmCheckContact(work);

        if (!(u8)MapEnm00CheckOffscreen(work)) {
            MapEnm00CheckBlocked(work, x, y);
        }
    }
}

void MapEnm00Wander(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    s32 x;
    s32 y;

    MapEnmSetAnim(work, 2, 1);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    x = work->obj.fieldPosition.x;
    y = obj->fieldPosition.y;

    if ((u8)MapEnm00SpotPlayer(work)) {
        work->timer = 0;
        work->update = MapEnm00Chase;
    } else if (GetRandom() % 80 == 0) {
        obj->speed = 0;
        work->update = MapEnm00Idle;
    } else {
        MapEnm00Move(work, 12, 0x80);
    }

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm00Hit;
    } else {
        MapEnmCheckContact(work);

        if (!(u8)MapEnm00CheckOffscreen(work)) {
            MapEnm00CheckBlocked(work, x, y);
        }
    }
}

void MapEnm00Chase(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    s32 x;
    s32 y;

    MapEnmSetAnim(work, 2, 1);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    x = obj->fieldPosition.x;
    y = obj->fieldPosition.y;

    if (work->timer % 8 == 0) {
        if (!(u8)MapEnm00SpotPlayer(work)) {
            obj->speed = 0;
            work->update = MapEnm00Idle;
        }
    }

    MapEnm00Move(work, 12, 0x180);
    work->timer++;

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm00Hit;
    } else {
        MapEnmCheckContact(work);

        if (!(u8)MapEnm00CheckOffscreen(work)) {
            MapEnm00CheckBlocked(work, x, y);
        }
    }
}

void MapEnm00Pursue(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    s32 x;
    s32 y;

    MapEnmSetAnim(work, 2, 1);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    x = obj->fieldPosition.x;
    y = obj->fieldPosition.y;

    if (work->timer % 8 == 0) {
        obj->angle = GetAngle(x, y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    MapEnm00Move(work, 25, 0x200);

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm00Hit;
    } else {
        MapEnmCheckContact(work);
        MapEnm00CheckBlocked(work, x, y);
        work->timer++;
    }
}

void MapEnm00Vanish(MapEnmWork* work) {
    work->flags |= MAP_ENM_FLAG_REMOVED;
    MapEnmSetAnim(work, 3, 0);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->update = NULL;
    }
}

void MapEnm00Hit(MapEnmWork* work) {
    MapEnmSetAnim(work, 4, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->flags |= MAP_ENM_FLAG_FIRST_STRIKE;
        MapEnmStartBattle(work);
    } else {
        MapEnmUpdateAnim(work);
    }
}

void MapEnm00Stand(MapEnmWork* work) {
    MapEnmSetAnim(work, 1, 0);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolUpdate(&work->tasks);

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm00Hit;
    } else {
        MapEnmCheckContact(work);
    }
}

void Task_MapEnm00_0(MapEnmWork* work, MapEnmArgs* arg) {
    MapEnmInit(work, arg);

    if (work->update == NULL) {
        if (work->flags & MAP_ENM_FLAG_ASLEEP) {
            work->update = MapEnm00Stand;
            MapEnmSetAnim(work, 1, 0);
            work->gfx = AnimGetGfx(&work->anim);
            ColliderSetDisabled(&work->collider, FALSE);
        } else {
            work->update = MapEnm00Appear;
            MapEnmSetAnim(work, 0, 0);
            work->gfx = AnimGetGfx(&work->anim);
            ColliderSetDisabled(&work->collider, TRUE);
        }
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    work->timer = 0;
}

s32 Task_MapEnm00_1(MapEnmWork* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapEnmSaveToCache(work);
        return 0;
    }

    if ((gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) && work->update != MapEnm00Hit) {
        return 1;
    }

    if (work->update != NULL) {
        work->update(work);

        if (work->update != NULL) {
            ColliderSetPosition(&work->collider, pos->x, pos->y, pos->z);
            return 1;
        }
    }

    return 0;
}

void Task_MapEnm00_2(MapEnmWork* work) {
    MapEnmDraw(work);
}

void Task_MapEnm00_3(MapEnmWork* work) {
    MapEnmDestroy(work);
}

#ifdef PLATFORM_PC
/* A fading flyer holds still, without a collider, until the fade ends; then it leaves as on the GBA. */
static s32 MapEnmFlyerFade(MapEnmWork* work) {
    if (++work->unk_D2 >= MAP_ENM_FADE_FRAMES) {
        work->update = NULL;
        return 0;
    }

    return 1;
}
#endif

void MapEnm01CheckOffscreen(MapEnm01Work* work) {
    if (work->enm.obj.fieldPosition.x < MAP_ENM_OFF_LEFT || work->enm.obj.fieldPosition.x > MAP_ENM_OFF_RIGHT) {
        if (work->wasOnScreen) {
#ifdef PLATFORM_PC
            /* GBA logic (Improved): fade out first; Task_MapEnm01_1 finishes it (MapEnmFlyerFade). */
            if (PortGbaMapFade()) {
                work->enm.unk_D2 = 1;
                ColliderSetDisabled(&work->enm.collider, TRUE);
                return;
            }
#endif
            work->enm.update = NULL;
        }
    } else if (!work->wasOnScreen) {
        work->wasOnScreen = TRUE;
    }
}

void MapEnm01UpdateHover(MapEnmWork* work, u8 moving) {
    s32* fields = &work->obj.fieldPosition.x;
    s32 prevZ = fields[2];
    s32 hoverZ;

    switch (moving) {
    case TRUE:
        hoverZ = work->targetZ + SIN(gFrameCounter) * 10;
        break;
    case FALSE:
    default:
        hoverZ = work->targetZ + SIN(gFrameCounter * 2) * 12;
        break;
    }

    if (work->flags & MAP_ENM_FLAG_SLOW) {
        fields[2] += (hoverZ - fields[2]) / 80;
    } else {
        fields[2] += (hoverZ - fields[2]) >> 4;
    }

    if (fields[3] < fields[2]) {
        fields[2] = prevZ;
        work->targetZ = prevZ - 0x1C00;
    }
}

void MapEnm01PickTarget(MapEnmWork* work, u8 atPlayer) {
    s32 leftOffset;
    s32 rightOffset;
    s32 yOffset;
    s32 zOffset;

    if (atPlayer) {
        work->targetX = gFieldState->actor.fieldPosition.x;
        work->targetY = gFieldState->actor.fieldPosition.y;
        work->targetZ = gFieldState->actor.fieldPosition.z - 0x1000;
    } else {
        work->targetX = gFieldState->actor.fieldPosition.x;
        work->targetY = gFieldState->actor.fieldPosition.y;
        work->targetZ = gFieldState->actor.fieldPosition.z;

        if (GetRandom() % 2) {
            leftOffset = GetRandom() % 65 * 256;
            leftOffset += 0x2000;
            work->targetX -= leftOffset;
        } else {
            rightOffset = GetRandom() % 65 * 256;
            rightOffset += 0x2000;
            work->targetX += rightOffset;
        }

        yOffset = GetRandom() % 121 * 256;
        yOffset -= 0x3C00;
        work->targetY += yOffset;
        zOffset = GetRandom() % 49 * 256;
        zOffset += 0x1000;
        work->targetZ -= zOffset;
    }
}

void MapEnm01Idle(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldObj* obj = &work->obj;

    MapEnmSetAnim(work, 0, 3);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    MapEnm01UpdateHover(work, FALSE);

    if (GetRandom() % 20 == 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    if (obj->fieldPosition.z < gFieldState->actor.fieldPosition.z - 0x4000) {
        MapEnm01PickTarget(work, FALSE);
        work->timer = 0;
        obj->speed = 0;
        work->update = MapEnm01Fly;
    } else if (GetRandom() % 130 == 0) {
        if (GetRandom() % 2 != 0) {
            MapEnm01PickTarget(work, FALSE);
        } else {
            MapEnm01PickTarget(work, TRUE);
        }

        enm->timer = 0;
        obj->speed = 0;
        enm->update = MapEnm01Fly;
    }

    if ((u8)MapEnmCheckAttacked(enm)) {
        enm->update = MapEnm01Hit;
    } else {
        MapEnmCheckContact(enm);
        MapEnm01CheckOffscreen((MapEnm01Work*)work);
    }
}

void MapEnm01Fly(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    FldPos prevPos;
    s32 dx;
    s32 dy;
    s32 step;
    s32 ground;
    u32 lim;

    MapEnmSetAnim(work, 1, 3);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    MapEnm01UpdateHover(work, TRUE);
    prevPos = work->obj.fieldPosition;

    if (GetRandom() % 20 != 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    dx = work->targetX;
    dy = work->targetY;

    if (work->flags & MAP_ENM_FLAG_SLOW) {
        lim = 0x140;
        obj->speed += 10;

        if (obj->speed > 76) {
            obj->speed = 76;
        }
    } else {
        lim = 64;
        obj->speed += 51;

        if (obj->speed > 0x180) {
            obj->speed = 0x180;
        }
    }

    step = (dx - obj->fieldPosition.x) >> 5;

    if (step > obj->speed) {
        step = obj->speed;
    } else if (step < -obj->speed) {
        step = -obj->speed;
    }

    obj->fieldPosition.x += step;
    step = (dy - obj->fieldPosition.y) >> 5;

    if (step > obj->speed) {
        step = obj->speed;
    } else if (step < -obj->speed) {
        step = -obj->speed;
    }

    obj->fieldPosition.y += step;

    if (work->timer > lim) {
        work->update = MapEnm01Idle;
    } else {
        work->timer++;
    }

    ground = GetFldPosGround(&obj->fieldPosition);

    if (ground < obj->fieldPosition.z) {
        obj->fieldPosition = prevPos;
        work->targetY = obj->fieldPosition.y + 0x1000;
    } else if (ground == MAP_Z_BOTTOM) {
        obj->fieldPosition = prevPos;
        work->targetY = obj->fieldPosition.y - 0x1000;
    } else {
        obj->fieldPosition.ground = ground;
    }

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm01Hit;
    } else {
        MapEnmCheckContact(work);
    }
}

void MapEnm01Hit(MapEnmWork* work) {
    MapEnmSetAnim(work, 2, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->flags |= MAP_ENM_FLAG_FIRST_STRIKE;
        MapEnmStartBattle(work);
    } else {
        MapEnmUpdateAnim(work);
    }
}

void MapEnm01Stand(MapEnmWork* work) {
    MapEnmSetAnim(work, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolUpdate(&work->tasks);

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm01Hit;
    } else {
        MapEnmCheckContact(work);
    }
}

void Task_MapEnm01_0(MapEnmWork* work, MapEnmArgs* arg) {
    MapEnm01Work* enm01 = (MapEnm01Work*)work;

    MapEnmInit(work, arg);

    if (work->flags & MAP_ENM_FLAG_ASLEEP) {
        work->update = MapEnm01Stand;
        MapEnmSetAnim(work, 0, 0);
        work->gfx = AnimGetGfx(&work->anim);
        ColliderSetDisabled(&work->collider, FALSE);
    } else {
        work->update = MapEnm01Idle;
        MapEnmSetAnim(work, 0, 1);
        work->gfx = AnimGetGfx(&work->anim);
        ColliderSetDisabled(&work->collider, FALSE);
    }

    enm01->enm.timer = 0;
    ((MapEnm01Work*)work)->wasOnScreen = FALSE;
}

s32 Task_MapEnm01_1(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldPos* pos = &work->obj.fieldPosition;

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapEnmSaveToCache(work);
        return 0;
    }

    if ((gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) && work->update != MapEnm01Hit) {
        return 1;
    }

#ifdef PLATFORM_PC
    if (work->unk_D2 != 0) {
        return MapEnmFlyerFade(work);
    }
#endif

    if (work->update != NULL) {
        (work->update)(enm);

        if (work->update != NULL) {
            ColliderSetPosition(&work->collider, pos->x, pos->y, pos->z);
            return 1;
        }
    }

    return 0;
}

void Task_MapEnm01_2(MapEnmWork* work) {
    MapEnmDraw(work);
}

void Task_MapEnm01_3(MapEnmWork* work) {
    MapEnmDestroy(work);
}

void MapEnm02Idle(MapEnmWork* work) {
    MapEnmSetAnim(work, 0, 1);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);

    if (IsHitByMapAttack(&work->obj.fieldPosition, work->radius, work->height)) {
        m4aSongNumStart(SONG_SYS_FIELD_ATT00);
        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        gMapRoomState->flags |= ROOM_FLAG_ENEMY_STRUCK;
        MapEnmStartBattle(work);
    }

    MapEnmCheckContact(work);
}

void Task_MapEnm02_0(MapEnmWork* work, MapEnmArgs* arg) {
    MapEnmInit(work, arg);
    work->update = MapEnm02Idle;
    MapEnmSetAnim(work, 0, 1);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderSetDisabled(&work->collider, FALSE);
}

s32 Task_MapEnm02_1(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldPos* pos = &enm->obj.fieldPosition;

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapEnmSaveToCache(work);
        return 0;
    }

    if (enm->update != NULL) {
        (enm->update)(enm);

        if (enm->update != NULL) {
            ColliderSetPosition(&enm->collider, pos->x, pos->y, pos->z);
            return 1;
        }
    }

    return 0;
}

void Task_MapEnm02_2(MapEnmWork* work) {
    MapEnmDraw(work);
}

void Task_MapEnm02_3(MapEnmWork* work) {
    MapEnmDestroy(work);
}

void MapEnm03UpdateHover(MapEnmWork* work, u8 moving) {
    s32* fields = &work->obj.fieldPosition.x;
    s32 prevZ = fields[2];
    s32 hoverZ;

    switch (moving) {
    case TRUE:
        hoverZ = work->targetZ + SIN(gFrameCounter) * 10;
        break;
    case FALSE:
    default:
        hoverZ = work->targetZ + SIN(gFrameCounter * 2) * 12;
        break;
    }

    if (work->flags & MAP_ENM_FLAG_SLOW) {
        fields[2] += (hoverZ - fields[2]) / 80;
    } else {
        fields[2] += (hoverZ - fields[2]) >> 4;
    }

    if (fields[3] < fields[2]) {
        fields[2] = prevZ;
        work->targetZ = prevZ - 0x1C00;
    }
}

s32 MapEnm03MoveToTarget(MapEnmWork* work) {
    s32* fields = &work->obj.fieldPosition.x;
    s32 dx;
    s32 dy;
    s32 lim;

    fields[4] += 0x100;

    if (fields[4] > 0x500) {
        fields[4] = 0x500;
    }

    dx = (work->targetX - work->obj.fieldPosition.x) / 32;
    lim = fields[4];

    if (dx > lim) {
        dx = lim;
    } else if (dx < -lim) {
        dx = -lim;
    }

    fields[0] += dx;

    dy = (work->targetY - fields[1]) / 32;

    if (dy > lim) {
        dy = lim;
    } else if (dy < -lim) {
        dy = -lim;
    }

    fields[1] += dy;

    if (work->timer > 64) {
        return TRUE;
    }

    work->timer++;
    return FALSE;
}

s32 MapEnm03IsPlayerNearHome(MapEnm03Work* work, s32 lim) {
    s32 dx;
    s32 dy;

    dx = work->home.x - gFieldState->actor.fieldPosition.x;

    if (dx < 0) {
        dx = gFieldState->actor.fieldPosition.x - work->home.x;
    }

    dy = work->home.y - gFieldState->actor.fieldPosition.y;

    if (dy < 0) {
        dy = gFieldState->actor.fieldPosition.y - work->home.y;
    }

    if (dx > 0x8000 || dy > 0x8000) {
        return FALSE;
    }

    return Sqrt8((dx * dx >> 8) + (dy * dy >> 8)) < lim ? TRUE : FALSE;
}

void MapEnm03Guard(MapEnmWork* work) {
    FldObj* obj = &work->obj;

    MapEnmSetAnim(work, 0, 3);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    MapEnm03UpdateHover(work, FALSE);

    if (GetRandom() % 20 == 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    if ((u8)MapEnm03IsPlayerNearHome((MapEnm03Work*)work, 0x6000) && obj->fieldPosition.ground == gFieldState->actor.fieldPosition.ground) {
        work->targetX = gFieldState->actor.fieldPosition.x;
        work->targetY = gFieldState->actor.fieldPosition.y;
        work->targetZ = gFieldState->actor.fieldPosition.ground - 0x1000;
        work->timer = 0;
        obj->speed = 0;
        work->update = MapEnm03Charge;
    }

    MapEnmCheckContact(work);
}

void MapEnm03Charge(MapEnmWork* work) {
    MapEnm03Work* enm03 = (MapEnm03Work*)work;
    FldObj* obj = &work->obj;
    FldPos prevPos;
    s32 ground;

    MapEnmSetAnim(work, 1, 3);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    MapEnm03UpdateHover(work, TRUE);
    prevPos = obj->fieldPosition;

    if (GetRandom() % 20 != 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    if ((u8)MapEnm03IsPlayerNearHome((MapEnm03Work*)work, 0x6000) && obj->fieldPosition.ground == gFieldState->actor.fieldPosition.ground) {
        work->targetX = gFieldState->actor.fieldPosition.x;
        work->targetY = gFieldState->actor.fieldPosition.y;
    }

    if ((u8)MapEnm03MoveToTarget(&enm03->enm)) {
        work->targetX = enm03->home.x;
        work->targetY = enm03->home.y;
        work->targetZ = enm03->home.z;
        work->timer = 0;
        obj->speed = 0;
        work->update = MapEnm03Return;
    }

    ground = GetFldPosGround(&obj->fieldPosition);

    if (ground < obj->fieldPosition.z) {
        obj->fieldPosition = prevPos;
        work->targetY = obj->fieldPosition.y + 0x1000;
    } else if (ground == MAP_Z_BOTTOM) {
        obj->fieldPosition = prevPos;
        work->targetY = obj->fieldPosition.y - 0x1000;
    } else {
        obj->fieldPosition.ground = ground;
    }

    MapEnmCheckContact(work);
}

void MapEnm03Return(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    FldPos save;
    s32 ground;

    MapEnmSetAnim(work, 1, 3);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    MapEnm03UpdateHover(work, TRUE);
    save = obj->fieldPosition;

    if (GetRandom() % 20 != 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    if ((u8)MapEnm03MoveToTarget(work)) {
        work->timer = 0;
        obj->speed = 0;
        work->update = MapEnm03Guard;
    }

    ground = GetFldPosGround(&obj->fieldPosition);

    if (ground < obj->fieldPosition.z) {
        obj->fieldPosition = save;
        work->targetY = obj->fieldPosition.y + 0x1000;
    } else if (ground == MAP_Z_BOTTOM) {
        obj->fieldPosition = save;
        work->targetY = obj->fieldPosition.y - 0x1000;
    } else {
        obj->fieldPosition.ground = ground;
    }

    MapEnmCheckContact(work);
}

void Task_MapEnm03_0(MapEnmWork* work, MapEnmArgs* arg) {
    MapEnmInit(work, arg);
    work->update = MapEnm03Guard;
    MapEnmSetAnim(work, 0, 1);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderSetDisabled(&work->collider, FALSE);
    work->timer = 0;
    ((MapEnm03Work*)work)->home = work->obj.fieldPosition;
}

s32 Task_MapEnm03_1(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldPos* pos = &enm->obj.fieldPosition;

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapEnmSaveToCache(work);
        return 0;
    }

    if (enm->update != NULL) {
        (enm->update)(enm);

        if (enm->update != NULL) {
            ColliderSetPosition(&enm->collider, pos->x, pos->y, pos->z);
            return 1;
        }
    }

    return 0;
}

void Task_MapEnm03_2(MapEnmWork* work) {
    MapEnmDraw(work);
}

void Task_MapEnm03_3(MapEnmWork* work) {
    MapEnmDestroy(work);
}

void MapEnm04CheckOffscreen(MapEnm01Work* work) {
    if (work->enm.obj.fieldPosition.x < MAP_ENM_OFF_LEFT || work->enm.obj.fieldPosition.x > MAP_ENM_OFF_RIGHT) {
        if (work->wasOnScreen) {
#ifdef PLATFORM_PC
            /* GBA logic (Improved): fade out first; Task_MapEnm04_1 finishes it (MapEnmFlyerFade). */
            if (PortGbaMapFade()) {
                work->enm.unk_D2 = 1;
                ColliderSetDisabled(&work->enm.collider, TRUE);
                return;
            }
#endif
            work->enm.update = NULL;
        }
    } else if (!work->wasOnScreen) {
        work->wasOnScreen = TRUE;
    }
}

void MapEnm04UpdateHover(MapEnmWork* work, u8 moving) {
    s32* fields = &work->obj.fieldPosition.x;
    s32 prevZ = fields[2];
    s32 hoverZ;

    switch (moving) {
    case TRUE:
        hoverZ = work->targetZ + SIN(gFrameCounter) * 10;
        break;
    case FALSE:
    default:
        hoverZ = work->targetZ + SIN(gFrameCounter * 2) * 12;
        break;
    }

    if (work->flags & MAP_ENM_FLAG_SLOW) {
        fields[2] += (hoverZ - fields[2]) / 80;
    } else {
        fields[2] += (hoverZ - fields[2]) >> 4;
    }

    if (fields[3] < fields[2]) {
        fields[2] = prevZ;
        work->targetZ = prevZ - 0x1C00;
    }
}

void MapEnm04PickTarget(MapEnmWork* work, u8 atPlayer) {
    if (atPlayer) {
        work->targetX = gFieldState->actor.fieldPosition.x;
        work->targetY = gFieldState->actor.fieldPosition.y;
        work->targetZ = gFieldState->actor.fieldPosition.z - 0x1000;
    } else {
        work->targetX = gFieldState->actor.fieldPosition.x;
        work->targetY = gFieldState->actor.fieldPosition.y;
        work->targetZ = gFieldState->actor.fieldPosition.z;

        if (GetRandom() % 2) {
            s32 offset = GetRandom() % 65 * 256 + 0x2000;

            work->targetX -= offset;
        } else {
            s32 offset = GetRandom() % 65 * 256 + 0x2000;

            work->targetX += offset;
        }

        {
            s32 offset = GetRandom() % 121 * 256 - 0x3C00;

            work->targetY += offset;
        }

        {
            s32 offset = GetRandom() % 49 * 256 + 0x1000;

            work->targetZ -= offset;
        }
    }
}

void MapEnm04Idle(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldObj* obj = &work->obj;
    FldPos prevPos;

    MapEnmSetAnim(work, 0, 3);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    prevPos = work->obj.fieldPosition;
    MapEnm04UpdateHover(work, FALSE);

    if (GetRandom() % 20 == 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    if (obj->fieldPosition.z < gFieldState->actor.fieldPosition.z - 0x4000) {
        MapEnm04PickTarget(work, FALSE);
        work->timer = 0;
        obj->speed = 0;
        work->update = MapEnm04Fly;
    } else if (GetRandom() % 130 == 0) {
        if (GetRandom() % 2) {
            MapEnm04PickTarget(work, FALSE);
        } else {
            MapEnm04PickTarget(work, TRUE);
        }

        enm->timer = 0;
        obj->speed = 0;
        enm->update = MapEnm04Fly;
    }

    if ((u8)MapEnmCheckAttacked(enm)) {
        enm->update = MapEnm04Hit;
    } else {
        MapEnmCheckContact(enm);
        MapEnm04CheckOffscreen((MapEnm01Work*)work);
    }
}

void MapEnm04Fly(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    FldPos prevPos;
    s32 dx;
    s32 dy;
    s32 step;
    s32 ground;
    u32 lim;

    MapEnmSetAnim(work, 1, 3);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);
    MapEnm04UpdateHover(work, TRUE);
    prevPos = work->obj.fieldPosition;

    if (GetRandom() % 20 != 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    dx = work->targetX;
    dy = work->targetY;

    if (work->flags & MAP_ENM_FLAG_SLOW) {
        lim = 0x140;
        obj->speed += 10;

        if (obj->speed > 76) {
            obj->speed = 76;
        }
    } else {
        lim = 64;
        obj->speed += 51;

        if (obj->speed > 0x180) {
            obj->speed = 0x180;
        }
    }

    step = (dx - obj->fieldPosition.x) >> 5;

    if (step > obj->speed) {
        step = obj->speed;
    } else if (step < -obj->speed) {
        step = -obj->speed;
    }

    obj->fieldPosition.x += step;
    step = (dy - obj->fieldPosition.y) >> 5;

    if (step > obj->speed) {
        step = obj->speed;
    } else if (step < -obj->speed) {
        step = -obj->speed;
    }

    obj->fieldPosition.y += step;

    if (work->timer > lim) {
        work->update = MapEnm04Idle;
    } else {
        work->timer++;
    }

    ground = GetFldPosGround(&obj->fieldPosition);

    if (ground < obj->fieldPosition.z) {
        obj->fieldPosition = prevPos;
        work->targetY = obj->fieldPosition.y + 0x1000;
    } else if (ground == MAP_Z_BOTTOM) {
        obj->fieldPosition = prevPos;
        work->targetY = obj->fieldPosition.y - 0x1000;
    } else {
        obj->fieldPosition.ground = ground;
    }

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm04Hit;
    } else {
        MapEnmCheckContact(work);
    }
}

void MapEnm04Hit(MapEnmWork* work) {
    MapEnmSetAnim(work, 2, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->flags |= MAP_ENM_FLAG_FIRST_STRIKE;
        MapEnmStartBattle(work);
    } else {
        MapEnmUpdateAnim(work);
    }
}

void MapEnm04Stand(MapEnmWork* work) {
    MapEnmSetAnim(work, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolUpdate(&work->tasks);

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm04Hit;
    } else {
        MapEnmCheckContact(work);
    }
}

void Task_MapEnm04_0(MapEnmWork* work, MapEnmArgs* arg) {
    MapEnm01Work* enm01 = (MapEnm01Work*)work;

    MapEnmInit(work, arg);

    if (work->flags & MAP_ENM_FLAG_ASLEEP) {
        work->update = MapEnm04Stand;
        MapEnmSetAnim(work, 0, 0);
        work->gfx = AnimGetGfx(&work->anim);
        ColliderSetDisabled(&work->collider, FALSE);
    } else {
        work->update = MapEnm04Idle;
        MapEnmSetAnim(work, 0, 1);
        work->gfx = AnimGetGfx(&work->anim);
        ColliderSetDisabled(&work->collider, FALSE);
    }

    enm01->enm.timer = 0;
    ((MapEnm01Work*)work)->wasOnScreen = FALSE;
}

s32 Task_MapEnm04_1(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldPos* pos = &work->obj.fieldPosition;

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapEnmSaveToCache(work);
        return 0;
    }

    if ((gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) && work->update != MapEnm04Hit) {
        return 1;
    }

#ifdef PLATFORM_PC
    if (work->unk_D2 != 0) {
        return MapEnmFlyerFade(work);
    }
#endif

    if (work->update != NULL) {
        (work->update)(enm);

        if (work->update != NULL) {
            ColliderSetPosition(&work->collider, pos->x, pos->y, pos->z);
            return 1;
        }
    }

    return 0;
}

void Task_MapEnm04_2(MapEnmWork* work) {
    MapEnmDraw(work);
}

void Task_MapEnm04_3(MapEnmWork* work) {
    MapEnmDestroy(work);
}

void MapEnm05Appear(MapEnmWork* work) {
    MapEnmWork* enm = work;

    MapEnmSetAnim(work, 0, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        ColliderSetDisabled(&work->collider, FALSE);
        work->timer = GetRandom() % 121 + 60;
        work->update = MapEnm05Idle;
    } else {
        MapEnmUpdateAnim(enm);

        if (enm->colliderDelay > 0) {
            enm->colliderDelay--;

            if (enm->colliderDelay <= 0) {
                ColliderSetDisabled(&enm->collider, FALSE);
            }
        }
    }
}

void MapEnm05Idle(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldObj* obj = &work->obj;

    MapEnmSetAnim(work, 1, 1);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);

    if (GetRandom() % 20 == 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm05Hit;
    } else {
        MapEnmCheckContact(work);

        if (work->timer != 0) {
            work->timer--;
        } else {
            ColliderSetDisabled(&enm->collider, TRUE);
            enm->update = MapEnm05Vanish;
        }
    }
}

void MapEnm05Vanish(MapEnmWork* work) {
    work->flags |= MAP_ENM_FLAG_REMOVED;
    MapEnmSetAnim(work, 2, 0);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->update = NULL;
    }
}

void MapEnm05Hit(MapEnmWork* work) {
    MapEnmSetAnim(work, 3, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->flags |= MAP_ENM_FLAG_FIRST_STRIKE;
        MapEnmStartBattle(work);
    } else {
        MapEnmUpdateAnim(work);
    }
}

void Task_MapEnm05_0(MapEnmWork* work, MapEnmArgs* arg) {
    MapEnmInit(work, arg);

    if (work->update == NULL) {
        work->update = MapEnm05Appear;
        MapEnmSetAnim(work, 0, 0);
        work->gfx = AnimGetGfx(&work->anim);
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    work->timer = 0;
}

s32 Task_MapEnm05_1(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldPos* pos = &work->obj.fieldPosition;

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapEnmSaveToCache(work);
        return 0;
    }

    if ((gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) && work->update != MapEnm05Hit) {
        return 1;
    }

    if (work->update != NULL) {
        (work->update)(enm);

        if (work->update != NULL) {
            ColliderSetPosition(&work->collider, pos->x, pos->y, pos->z);
            return 1;
        }
    }

    return 0;
}

void Task_MapEnm05_2(MapEnmWork* work) {
    MapEnmDraw(work);
}

void Task_MapEnm05_3(MapEnmWork* work) {
    MapEnmDestroy(work);
}

void MapEnm06Appear(MapEnmWork* work) {
    MapEnmWork* enm = work;

    MapEnmSetAnim(work, 0, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        ColliderSetDisabled(&work->collider, FALSE);
        work->timer = GetRandom() % 121 + 60;
        work->update = MapEnm06Idle;
    } else {
        MapEnmUpdateAnim(enm);

        if (enm->colliderDelay > 0) {
            enm->colliderDelay--;

            if (enm->colliderDelay <= 0) {
                ColliderSetDisabled(&enm->collider, FALSE);
            }
        }
    }
}

void MapEnm06Idle(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldObj* obj = &work->obj;

    MapEnmSetAnim(work, 1, 1);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);

    if (GetRandom() % 20 == 0) {
        obj->angle = GetAngle(work->obj.fieldPosition.x, obj->fieldPosition.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
    }

    if ((u8)MapEnmCheckAttacked(work)) {
        work->update = MapEnm06Hit;
    } else {
        MapEnmCheckContact(work);

        if (work->timer != 0) {
            work->timer--;
        } else {
            ColliderSetDisabled(&enm->collider, TRUE);
            enm->update = MapEnm06Vanish;
        }
    }
}

void MapEnm06Vanish(MapEnmWork* work) {
    work->flags |= MAP_ENM_FLAG_REMOVED;
    MapEnmSetAnim(work, 2, 0);
    MapEnmUpdateAnim(work);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->update = NULL;
    }
}

void MapEnm06Hit(MapEnmWork* work) {
    MapEnmSetAnim(work, 3, 0);
    TaskPoolUpdate(&work->tasks);

    if (AnimIsFinished(&work->anim)) {
        work->flags |= MAP_ENM_FLAG_FIRST_STRIKE;
        MapEnmStartBattle(work);
    } else {
        MapEnmUpdateAnim(work);
    }
}

void Task_MapEnm06_0(MapEnmWork* work, MapEnmArgs* arg) {
    MapEnmInit(work, arg);

    if (work->update == NULL) {
        work->update = MapEnm06Appear;
        MapEnmSetAnim(work, 0, 0);
        work->gfx = AnimGetGfx(&work->anim);
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    work->timer = 0;
}

s32 Task_MapEnm06_1(MapEnmWork* work) {
    MapEnmWork* enm = work;
    FldPos* pos = &work->obj.fieldPosition;

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapEnmSaveToCache(work);
        return 0;
    }

    if ((gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) && work->update != MapEnm06Hit) {
        return 1;
    }

    if (work->update != NULL) {
        (work->update)(enm);

        if (work->update != NULL) {
            ColliderSetPosition(&work->collider, pos->x, pos->y, pos->z);
            return 1;
        }
    }

    return 0;
}

void Task_MapEnm06_2(MapEnmWork* work) {
    MapEnmDraw(work);
}

void Task_MapEnm06_3(MapEnmWork* work) {
    MapEnmDestroy(work);
}

s32 GetMapRoomDebugCode(MapFloorRoom* floorRoom) {
    return (gGameState.floor << 28) + (floorRoom->roomType << 20) + (floorRoom->cardValue << 16) + (gMapFloorState.room << 8) + (gMapFloorState.eventStep << 4) + gMapFloorState.world;
}

void MapDbgWaitInput(MapDbgWork* work) {
    if ((GetKeysHeld() & (L_BUTTON | R_BUTTON)) == (L_BUTTON | R_BUTTON)) {
        if (GetKeysPressed() & SELECT_BUTTON) {
            *work->editing = 1;
            RequestFieldResume();
            ModeRequest(&gModeDebflag, 1);
        }

#ifndef VERSION_EU
        if (GetKeysPressed() & START_BUTTON) {
            work->visible = TRUE;
            *work->editing = 1;
            work->update = MapDbgEditSeed;
        }
#endif
    }
}

void MapDbgEditSeed(MapDbgWork* work) {
    MapFloorRoom* floorRoom;
    s32 step;
    s32 i;

    step = 1;

    for (i = work->seedCursor; i > 0; i--) {
        step <<= 4;
    }

    floorRoom = GetMapFloorRoom(gMapFloorState.room);

    if (GetKeysRepeat() & DPAD_UP) {
        floorRoom->seed += step;
    }

    if (GetKeysRepeat() & DPAD_DOWN) {
        floorRoom->seed -= step;
    }

    if (GetKeysPressed() & DPAD_LEFT) {
        work->seedCursor = work->seedCursor == 7 ? 0 : work->seedCursor + 1;
    }

    if (GetKeysPressed() & DPAD_RIGHT) {
        work->seedCursor = work->seedCursor == 0 ? 7 : work->seedCursor - 1;
    }

    work->seedTextLength = FormatSmallFontHex(floorRoom->seed, work->seedText);

    if (GetKeysPressed() & SELECT_BUTTON) {
        if (++gMapFloorState.world > WORLD_CASTLE_OBLIVION) {
            gMapFloorState.world = 0;
        }

        work->codeTextLength = FormatSmallFontHex(GetMapRoomDebugCode(floorRoom), work->codeText);
    }

    if (GetKeysPressed() & (L_BUTTON | R_BUTTON)) {
        work->update = MapDbgEditWorld;
    } else if (GetKeysPressed() & (A_BUTTON | B_BUTTON | START_BUTTON)) {
        *work->editing = 0;
        work->update = MapDbgWaitInput;
    }
}

void MapDbgEditWorld(MapDbgWork* work) {
    MapFloorRoom* floorRoom = GetMapFloorRoom(gMapFloorState.room);

    if ((GetKeysRepeat() & DPAD_UP) && work->codeCursor == 0) {
        gMapFloorState.world = gMapFloorState.world < WORLD_CASTLE_OBLIVION ? gMapFloorState.world + 1 : 0;
    }

    if ((GetKeysRepeat() & DPAD_DOWN) && work->codeCursor == 0) {
        gMapFloorState.world = gMapFloorState.world != 0 ? gMapFloorState.world - 1 : WORLD_CASTLE_OBLIVION;
    }

    work->codeTextLength = FormatSmallFontHex(GetMapRoomDebugCode(floorRoom), work->codeText);

    if (GetKeysPressed() & (L_BUTTON | R_BUTTON)) {
        work->update = MapDbgEditSeed;
    }

    if (GetKeysPressed() & (A_BUTTON | B_BUTTON | START_BUTTON)) {
        *work->editing = 0;
        work->update = MapDbgWaitInput;
    }
}

void Task_MapDbg_0(MapDbgWork* work, u8* editing) {
#ifndef VERSION_EU
    MapFloorRoom* floorRoom;
#endif
    work->visible = FALSE;
    work->editing = editing;
    *editing = 0;
    work->update = MapDbgWaitInput;
    work->seedCursor = 0;
    work->codeCursor = 0;
#ifdef VERSION_EU
    GetMapFloorRoom(gMapFloorState.room);
#else
    floorRoom = GetMapFloorRoom(gMapFloorState.room);
    work->tiles = LoadSmallFontTiles();
    work->palette = LoadSmallFontPalette();
    work->seedTextLength = FormatSmallFontHex(floorRoom->seed, work->seedText);
    work->codeTextLength = FormatSmallFontHex(GetMapRoomDebugCode(floorRoom), work->codeText);
    work->cursorTextLength = EncodeSmallFontString(sMapDbgCursorString, &work->cursorText);
#endif
}

s32 Task_MapDbg_1(MapDbgWork* work) {
    work->update(work);
    return 1;
}

void Task_MapDbg_2(MapDbgWork* work) {
#ifndef VERSION_EU
    if (work->visible) {
        DrawSmallFontString(DISPLAY_WIDTH - work->seedTextLength * 8, 0x8E, work->seedText, work->tiles, work->palette, 0, work->seedTextLength);
        DrawSmallFontString(DISPLAY_WIDTH - work->codeTextLength * 8, 0x96, work->codeText, work->tiles, work->palette, 0, work->codeTextLength);

        if (*work->editing != 0) {
            if (work->update == MapDbgEditSeed) {
                DrawSmallFontString(DISPLAY_WIDTH - (work->seedCursor + 1) * 8, 0x90, &work->cursorText, work->tiles, work->palette, 0, work->cursorTextLength);
            } else {
                DrawSmallFontString(DISPLAY_WIDTH - (work->codeCursor + 1) * 8, 0x98, &work->cursorText, work->tiles, work->palette, 0, work->cursorTextLength);
            }
        }
    }
#endif
}

void Task_MapDbg_3(MapDbgWork* work) {
#ifndef VERSION_EU
    FreeSmallFontResources(work->tiles, work->palette);
#endif
}

enum MapGmkJumpState {
    MAP_GMK_JUMP_STATE_WAIT_STEP,
    MAP_GMK_JUMP_STATE_WAIT_JUMP,
    MAP_GMK_JUMP_STATE_LAUNCH
};

void MapGmkJumpWaitStep(MapGmkJumpWork* work) {
    if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER) && (work->collider.standFlags & COLLIDER_STAND_STOOD_ON)) {
        gMapRoomState->jumpGmkHeight = work->jumpHeight;
        gMapRoomState->jumpGmkAngle = work->obj.angle;
        work->update = MapGmkJumpWaitJump;
        work->state = MAP_GMK_JUMP_STATE_WAIT_JUMP;
        AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
    } else {
        AnimUpdate(&work->anim);
    }
}

void MapGmkJumpWaitJump(MapGmkJumpWork* work) {
    if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
        gMapRoomState->jumpGmkHeight = work->jumpHeight;
        gMapRoomState->jumpGmkAngle = work->obj.angle;
    } else if (gFieldState->actor.fieldPosition.z != gFieldState->actor.fieldPosition.ground) {
        work->update = MapGmkJumpLaunch;
        work->state = MAP_GMK_JUMP_STATE_LAUNCH;
        AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
    } else {
        work->update = MapGmkJumpWaitStep;
        work->state = MAP_GMK_JUMP_STATE_WAIT_STEP;
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    }
}

void MapGmkJumpLaunch(MapGmkJumpWork* work) {
    if (AnimIsFinished(&work->anim)) {
        work->update = MapGmkJumpWaitStep;
        work->state = MAP_GMK_JUMP_STATE_WAIT_STEP;
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    } else {
        AnimUpdate(&work->anim);
    }
}

void Task_MapGmk_Jump_0(MapGmkJumpWork* work, MapPlatform* arg) {
    FldObj* obj = &work->obj;
    AnimState* anim;

#ifdef PLATFORM_PC
    /* Task work is reused without clearing. A spring lies flat; stale height
     * otherwise makes FldObjIsOutOfView disable its collider on screen. */
    obj->height = 0;
#endif
    obj->fieldPosition.x = arg->x << 13;
    obj->fieldPosition.y = arg->y << 12;
    obj->fieldPosition.z = 0;
    obj->fieldPosition.z = work->obj.fieldPosition.ground = GetFldPosFloor(&obj->fieldPosition);
    obj->fieldPosition.y -= work->obj.fieldPosition.ground;

    switch (arg->spotType) {
    case MAP_CELL_TYPE_LEFT_WALL_TOP:
        obj->angle = FLD_ANGLE_UP_LEFT;
        break;
    case MAP_CELL_TYPE_RIGHT_WALL_TOP:
        obj->angle = FLD_ANGLE_UP_RIGHT;
        break;
    case MAP_CELL_TYPE_FLOOR:
    default:
        obj->angle = FLD_ANGLE_UP;
        break;
    }

    work->jumpHeight = arg->spotLowerZ - arg->spotUpperZ;
    work->palette = LoadObjPalette(gMapGmkJumpPalette, sizeof(gMapGmkJumpPalette));
    work->tiles = LoadObjTiles(gMapGmkJumpTiles, sizeof(gMapGmkJumpTiles));
    anim = &work->anim;
    AnimInit(anim, gMapGmkJumpAnims, gMapGmkJumpFrames);
    work->state = MAP_GMK_JUMP_STATE_WAIT_STEP;
    AnimStart(anim, 0, ANIM_FLAG_LOOP);
    work->update = MapGmkJumpWaitStep;
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, 16, 0);
    ColliderSetPosition(&work->collider, obj->fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
}

s32 Task_MapGmk_Jump_1(MapGmkJumpWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        work->update(work);
    }

    return 1;
}

void Task_MapGmk_Jump_2(MapGmkJumpWork* work) {
    u16 priority;
    s32 pixelY;
    s16 x;
    s16 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_Jump_3(MapGmkJumpWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmkEnmRise(MapGmkEnmWork* work) {
    if (work->timer != 0) {
        work->gfx = AnimUpdate(&work->anim);
        ApproachValue(&work->obj.fieldPosition.z, work->targetZ, work->timer);
        work->timer--;
    } else {
        gMapRoomState->flags |= ROOM_FLAG_START_BATTLE;
        gFieldState->flags |= FIELD_FLAG_FREEZE_ENEMIES;
        gMapRoomState->battleId = GetRandomBattleId();
        work->update = NULL;
    }

    return 1;
}

void Task_MapGmk_Enm_0(MapGmkEnmWork* work, FldPos* arg) {
    FldPos* pos = &work->obj.fieldPosition;
    u8 flip;

    work->obj.fieldPosition = *arg;
    work->obj.fieldPosition.y += 0x800;
    work->obj.fieldPosition.z -= 0x1000;
    work->obj.height = 16;

    if (gMapFloorState.world != WORLD_ATLANTICA) {
        work->tiles = AllocObjTiles(0x220, gEmy01L00Tiles);
        work->palette = LoadObjPalette(gEmy01Palette, sizeof(gEmy01Palette));
        AnimInit(&work->anim, gEmy01L00Anims, gEmy01L00Frames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(&work->anim);
    } else {
        work->tiles = AllocObjTiles(0x440, gEmy0600Tiles);
        work->palette = LoadObjPalette(gEmy06Palette, sizeof(gEmy06Palette));
        AnimInit(&work->anim, gEmy0600Anims, gEmy0600Frames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(&work->anim);
    }

    work->update = MapGmkEnmRise;
    flip = FALSE;

    if (gFieldState->actor.fieldPosition.x >= pos->x) {
        flip = TRUE;
    }

    work->flipX = flip;
    work->timer = 8;
    work->targetZ = pos->z - 0x1000;
}

u8 Task_MapGmk_Enm_1(MapGmkEnmWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_Enm_2(MapGmkEnmWork* work) {
    u16 flags;
    s32 pixelY;
    s32 x;
    s32 y;
    s32 flip;

    flip = work->flipX;
    flags = SPRITE_PRIORITY(2);

    if (flip) {
        flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
    }

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, -0x1004 - pixelY * 4);
}

void Task_MapGmk_Enm_3(MapGmkEnmWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void Task_MapGmk_Dmy_0(MapGmkDmyWork* work) {
    work->tiles = AllocObjTiles(MapGmkGetFreeTiles() << 5, NULL);
}

s32 Task_MapGmk_Dmy_1() {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    return 1;
}

void Task_MapGmk_Dmy_2(MapGmkDmyWork* work) {
}

void Task_MapGmk_Dmy_3(MapGmkDmyWork* work) {
    if (work->tiles != NULL) {
        ReleaseObjTiles(work->tiles);
    }
}

u8 MapGmkTutorialWaitHit(MapGmkTutorialWork* work) {
    if (IsHitByMapAttack(&work->obj.fieldPosition, 0, 8)) {
        if ((gFieldState->flags & FIELD_FLAG_PLAYER_JUMPING) == 0 && gFieldState->actor.fieldPosition.z == gFieldState->actor.fieldPosition.ground) {
            TaskPool* pool = &work->tasks;

            TaskCreate(pool, &gTaskDescMapSpark, &work->obj);
            m4aSongNumStart(SONG_SND_220);
            gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
            gFieldState->lockonTarget = &work->obj;
            gMapRoomState->door = &work->obj;
            work->update = MapGmkTutorialWaitCard;
            gMapRoomState->doorRoom = 0;
            gMapRoomState->doorSide = MAP_DOOR_SIDE_UP_RIGHT;
            FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
            TaskCreate(pool, &gTaskDescRoomcreate, NULL);
        }
    }

    return 1;
}

u8 MapGmkTutorialWaitCard(MapGmkTutorialWork* work) {
    void* card = GetSelectedMapCard();

    if (card != NULL) {
        CreateMapRoom(gMapFloorDefs[0].entryRoom, card);
        work->update = MapGmkTutorialWaitOpen;
    }

    if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) == 0) {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        FadeSetPaletteExcluded(work->palette->index + 16, FALSE);
        work->update = MapGmkTutorialWaitHit;
    }

    return 1;
}

u8 MapGmkTutorialWaitOpen(MapGmkTutorialWork* work) {
    if (gFieldState->flags & FIELD_FLAG_DOOR_OPENED) {
        UpdateSpriteFrameTiles(work->tiles, gMapDoorSide0Frame0, gMapDoorTraverseTownSide0OpenTiles);
        work->opened = TRUE;
        work->update = MapGmkTutorialIdle;
    }

    return 1;
}

u8 MapGmkTutorialIdle(MapGmkTutorialWork* work) {
    return 1;
}

void Task_MapGmk_Tutorial_0(MapGmkTutorialWork* work) {
    work->obj.fieldPosition.x = 0x19000;
    work->obj.fieldPosition.y = 0xAA00;
    work->obj.fieldPosition.z = 0;
    work->obj.fieldPosition.ground = GetFldPosFloor(&work->obj.fieldPosition);
    work->obj.fieldPosition.z = work->obj.fieldPosition.ground;
    work->obj.fieldPosition.y -= work->obj.fieldPosition.ground;
    work->obj.angle = FLD_ANGLE_DOWN_LEFT;
    work->obj.height = 32;
    work->palette = LoadObjPalette(gMapDoorTraverseTownPalette, sizeof(gMapDoorTraverseTownPalette));
    work->tiles = AllocSpriteFrameTiles(0x400);
    UpdateSpriteFrameTiles(work->tiles, gMapDoorSide0Frame0, gMapDoorTraverseTownSide0ClosedTiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, 16, 0);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, work->obj.fieldPosition.y, work->obj.fieldPosition.z);
    work->opened = FALSE;
    work->update = MapGmkTutorialWaitHit;
    TaskPoolInit(&work->tasks, 2);
}

s32 Task_MapGmk_Tutorial_1(MapGmkTutorialWork* work) {
    if (work->opened && (work->collider.standFlags & COLLIDER_STAND_STOOD_ON) && work->collider.otherType == COLLIDER_TYPE_PLAYER) {
        gMapRoomState->flags |= ROOM_FLAG_ENTER_WORLD;
    }

    if (work->update != NULL) {
        if (work->update(work) == 0) {
            return 0;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void Task_MapGmk_Tutorial_2(MapGmkTutorialWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0xFE4 - pixelY * 4;
    DrawSprite(x, y, NULL, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
    TaskPoolDraw(&work->tasks);
}

void Task_MapGmk_Tutorial_3(MapGmkTutorialWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
}

u8 MapGmkSpiderStartBattle(MapGmkSpiderWork* work) {
    AnimState* anim = &work->anim;

    if (AnimIsFinished(anim)) {
        gMapRoomState->flags |= ROOM_FLAG_START_BATTLE;
        gFieldState->flags |= FIELD_FLAG_FREEZE_ENEMIES;
        gMapRoomState->battleId = GetRandom() % 3 + 125;
        work->update = NULL;
    } else {
        work->gfx = AnimUpdate(anim);
    }

    return 1;
}

void Task_MapGmk_Spider_0(MapGmkSpiderWork* work, MapGmkPlacement* arg) {
    u8 flip;

    work->obj.fieldPosition = arg->pos;
    work->obj.height = 24;
    work->tiles = AllocObjTiles(0x720, gEmy2103Tiles);
    work->palette = LoadObjPalette(gEmy21Palette, sizeof(gEmy21Palette));
    AnimInit(&work->anim, gEmy2103Anims, gEmy2103Frames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    SetObjTileSource(work->tiles, gEmy2103Tiles);
    work->update = MapGmkSpiderStartBattle;
    flip = FALSE;

    if (gFieldState->actor.fieldPosition.x >= work->obj.fieldPosition.x) {
        flip = TRUE;
    }

    work->flipX = flip;
}

u8 Task_MapGmk_Spider_1(MapGmkSpiderWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_Spider_2(MapGmkSpiderWork* work) {
    u16 flags;
    u16 priority;
    s32 flip;
    s32 pixelY;
    s32 x;
    s32 y;

    flip = work->flipX;
    flags = SPRITE_PRIORITY(2);

    if (flip) {
        flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
    }

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, priority);
}

void Task_MapGmk_Spider_3(MapGmkSpiderWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

u8 MapGmkGp00WaitHit(MapGmkGpWork* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (IsHitByMapAttack(pos, 8, 8)) {
        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);

        if (!(work->placement->flags & GMK_FLAG_USED)) {
            work->placement->flags |= GMK_FLAG_USED;
            DropMapGmkPrize(pos);
        }

        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        work->timer = 20;
        work->update = MapGmkGp00HitDelay;
    }

    return 1;
}

u8 MapGmkGp00HitDelay(MapGmkGpWork* work) {
    if (work->timer != 0) {
        work->timer--;
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        work->update = MapGmkGp00WaitHit;
    }

    return 1;
}

void Task_MapGmk_GP00_0(MapGmkGpWork* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = LoadObjTiles(def->tiles, def->tilesSize);
    work->palette = LoadObjPalette(def->palette, 32);
    AnimInit(&work->anim, def->anims, def->gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->timer = 0;
    work->update = MapGmkGp00WaitHit;
}

u8 Task_MapGmk_GP00_1(MapGmkGpWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP00_2(MapGmkGpWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_GP00_3(MapGmkGpWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmkGp01WaitHit(MapGmkGp1Work* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (!IsHitByMapAttack(pos, 8, 8)) {
        work->gfx = AnimUpdate(&work->anim);
    } else {
        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);
        DropMapGmkPrize(&work->obj.fieldPosition);
        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        work->placement->flags |= GMK_FLAG_DESTROYED;
        ColliderSetDisabled(&work->collider, TRUE);
        AnimStart(&work->anim, 1, 0);
        work->update = MapGmkGp01Break;
    }

    return 1;
}

u8 MapGmkGp01Break(MapGmkGp1Work* work) {
    AnimState* anim = &work->anim;

    if (!AnimIsFinished(anim)) {
        work->gfx = AnimUpdate(anim);
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        work->visible = FALSE;
        work->update = NULL;
    }

    return 1;
}

void Task_MapGmk_GP01_0(MapGmkGp1Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;
    AnimState* anim;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    anim = &work->anim;
    AnimInit(anim, def->anims, def->gfxTable);
    AnimStart(anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->visible = TRUE;
    work->update = MapGmkGp01WaitHit;
}

u8 Task_MapGmk_GP01_1(MapGmkGp1Work* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP01_2(MapGmkGp1Work* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    if (work->visible) {
        x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
        pixelY = work->obj.fieldPosition.y >> 8;
        y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
        priority = -0x1004 - pixelY * 4;
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
    }
}

void Task_MapGmk_GP01_3(MapGmkGp1Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmkGp02WaitHit(MapGmkGpWork* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (IsHitByMapAttack(pos, 8, 8)) {
        MapGmkPlacement* placement;

        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);
        placement = work->placement;

        if ((placement->flags & GMK_FLAG_USED) == 0) {
            placement->flags |= GMK_FLAG_USED;
            DropMapGmkPrize(pos);
        }

        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        AnimStart(&work->anim, 1, 0);
        work->update = MapGmkGp02HitAnim;
    }

    return 1;
}

u8 MapGmkGp02HitAnim(MapGmkGpWork* work) {
    if (!AnimIsFinished(&work->anim)) {
        work->gfx = AnimUpdate(&work->anim);
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        AnimStart(&work->anim, 0, 0);
        work->gfx = AnimGetGfx(&work->anim);
        work->update = MapGmkGp02WaitHit;
    }

    return 1;
}

void Task_MapGmk_GP02_0(MapGmkGpWork* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;
    AnimState* anim;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    anim = &work->anim;
    AnimInit(anim, def->anims, def->gfxTable);
    AnimStart(anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->update = MapGmkGp02WaitHit;
}

u8 Task_MapGmk_GP02_1(MapGmkGpWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP02_2(MapGmkGpWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_GP02_3(MapGmkGpWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmkGp03WaitHit(MapGmkGpWork* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (IsHitByMapAttack(pos, 8, 8)) {
        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);
        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        AnimStart(&work->anim, 1, 0);
        work->update = MapGmkGp03HitAnim;
    }

    return 1;
}

u8 MapGmkGp03HitAnim(MapGmkGpWork* work) {
    AnimState* anim = &work->anim;

    if (AnimIsFinished(anim)) {
        MapGmkPlacement* placement = work->placement;

        if ((placement->flags & GMK_FLAG_USED) == 0) {
            placement->flags |= GMK_FLAG_USED;
            DropMapGmkPrize(&work->obj.fieldPosition);
        }

        AnimStart(anim, 2, 0);
        work->update = MapGmkGp03EndAnim;
    } else {
        work->gfx = AnimUpdate(anim);
    }

    return 1;
}

u8 MapGmkGp03EndAnim(MapGmkGpWork* work) {
    if (!AnimIsFinished(&work->anim)) {
        work->gfx = AnimUpdate(&work->anim);
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        AnimStart(&work->anim, 0, 0);
        work->gfx = AnimGetGfx(&work->anim);
        work->update = MapGmkGp03WaitHit;
    }

    return 1;
}

void Task_MapGmk_GP03_0(MapGmkGpWork* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;
    AnimState* anim;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    anim = &work->anim;
    AnimInit(anim, def->anims, def->gfxTable);
    AnimStart(anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->update = MapGmkGp03WaitHit;
}

u8 Task_MapGmk_GP03_1(MapGmkGpWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP03_2(MapGmkGpWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_GP03_3(MapGmkGpWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmkGp04WaitHit(MapGmkGpWork* work) {
    FldPos* pos = &work->obj.fieldPosition;
    AnimState* anim = &work->anim;

    work->gfx = AnimUpdate(anim);

    if ((work->placement->flags & GMK_FLAG_USED) == 0 && IsHitByMapAttack(pos, 8, 8)) {
        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);
        DropMapGmkPrize(pos);
        work->placement->flags |= GMK_FLAG_USED;
        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        AnimStart(anim, 1, ANIM_FLAG_LOOP);
        work->timer = 20;
        work->update = MapGmkGp04HitDelay;
    }

    return 1;
}

u8 MapGmkGp04HitDelay(MapGmkGpWork* work) {
    work->gfx = AnimUpdate(&work->anim);

    if (work->timer != 0) {
        work->timer--;
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        work->update = NULL;
    }

    return 1;
}

void Task_MapGmk_GP04_0(MapGmkGpWork* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    AnimInit(&work->anim, def->anims, def->gfxTable);

    if (work->placement->flags & GMK_FLAG_USED) {
        AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
    } else {
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    }

    work->gfx = AnimGetGfx(&work->anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, obj->fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->update = MapGmkGp04WaitHit;
}

u8 Task_MapGmk_GP04_1(MapGmkGpWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP04_2(MapGmkGpWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_GP04_3(MapGmkGpWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmkGp05WaitHit(MapGmkGpWork* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (!(work->placement->flags & GMK_FLAG_USED) && IsHitByMapAttack(pos, 8, 8)) {
        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);
        DropMapGmkPrize(pos);
        work->placement->flags |= GMK_FLAG_USED;
        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        AnimStart(&work->anim, 1, 0);
        work->update = MapGmkGp05HitAnim;
    }

    return 1;
}

u8 MapGmkGp05HitAnim(MapGmkGpWork* work) {
    AnimState* anim = &work->anim;

    if (!AnimIsFinished(anim)) {
        work->gfx = AnimUpdate(anim);
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        AnimStart(anim, 2, 0);
        work->gfx = AnimGetGfx(anim);
        work->update = NULL;
    }

    return 1;
}

void Task_MapGmk_GP05_0(MapGmkGpWork* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    AnimInit(&work->anim, def->anims, def->gfxTable);

    if (work->placement->flags & GMK_FLAG_USED) {
        AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
    } else {
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    }

    work->gfx = AnimGetGfx(&work->anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, obj->fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->update = MapGmkGp05WaitHit;
}

u8 Task_MapGmk_GP05_1(MapGmkGpWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP05_2(MapGmkGpWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_GP05_3(MapGmkGpWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmkGp06WaitHit(MapGmkGpWork* work) {
    FldPos* pos = &work->obj.fieldPosition;
    AnimState* anim = &work->anim;

    work->gfx = AnimUpdate(anim);

    if (IsHitByMapAttack(pos, 8, 8)) {
        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);

        if ((work->placement->flags & GMK_FLAG_USED) == 0) {
            work->placement->flags |= GMK_FLAG_USED;
            DropMapGmkPrize(pos);
        }

        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;

        if (work->placement->flags & GMK_FLAG_TOGGLED) {
            work->placement->flags &= ~GMK_FLAG_TOGGLED;
            AnimStart(anim, 0, ANIM_FLAG_LOOP);
        } else {
            work->placement->flags |= GMK_FLAG_TOGGLED;
            AnimStart(anim, 1, ANIM_FLAG_LOOP);
        }

        work->timer = 20;
        work->update = MapGmkGp06HitDelay;
    }

    return 1;
}

u8 MapGmkGp06HitDelay(MapGmkGpWork* work) {
    work->gfx = AnimUpdate(&work->anim);

    if (work->timer != 0) {
        work->timer--;
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        work->update = MapGmkGp06WaitHit;
    }

    return 1;
}

void Task_MapGmk_GP06_0(MapGmkGpWork* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    AnimInit(&work->anim, def->anims, def->gfxTable);

    if (work->placement->flags & GMK_FLAG_TOGGLED) {
        AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
    } else {
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    }

    work->gfx = AnimGetGfx(&work->anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, obj->fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->update = MapGmkGp06WaitHit;
}

u8 Task_MapGmk_GP06_1(MapGmkGpWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP06_2(MapGmkGpWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_GP06_3(MapGmkGpWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

s32 MapGmkGp07WaitStep(MapGmkGp07Work* work) {
    AnimState* anim = &work->anim;

    work->gfx = AnimUpdate(anim);

    if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
        if (work->collider.standFlags & COLLIDER_STAND_STOOD_ON) {
            if (!(work->placement->flags & GMK_FLAG_USED)) {
                work->placement->flags |= GMK_FLAG_USED;
                DropMapGmkPrize(&work->obj.fieldPosition);
            }

            AnimStart(anim, 1, 0);
            work->gfx = AnimGetGfx(anim);
            work->update = MapGmkGp07WaitStepOff;
        }
    }

    return 1;
}

s32 MapGmkGp07WaitStepOff(MapGmkGp07Work* work) {
    work->gfx = AnimUpdate(&work->anim);

    if (!ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
        AnimStart(&work->anim, 2, 0);
        work->update = MapGmkGp07EndAnim;
    }

    return 1;
}

s32 MapGmkGp07EndAnim(MapGmkGp07Work* work) {
    AnimState* anim = &work->anim;

    if (AnimIsFinished(anim)) {
        AnimStart(anim, 0, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(anim);
        work->update = MapGmkGp07WaitStep;
    } else {
        work->gfx = AnimUpdate(anim);
    }

    return 1;
}

void Task_MapGmk_GP07_0(MapGmkGp07Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    AnimInit(&work->anim, def->anims, def->gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->update = MapGmkGp07WaitStep;
}

u8 Task_MapGmk_GP07_1(MapGmkGp07Work* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP07_2(MapGmkGp07Work* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk_GP07_3(MapGmkGp07Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

s32 MapGmkGp08WaitHit(MapGmkGp08Work* work) {
    FldPos* pos = &work->obj.fieldPosition;
    AnimState* anim;

    if (IsHitByMapAttack(pos, 8, 8)) {
        m4aSongNumStart(work->hitSong);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);

        if (!(work->placement->flags & GMK_FLAG_USED)) {
            work->placement->flags |= GMK_FLAG_USED;
            DropMapGmkPrize(pos);
        }

        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        anim = &work->anim;
        AnimStart(anim, 1, 0);
        work->gfx2 = AnimGetGfx(anim);
        work->overlayVisible = TRUE;
        work->update = MapGmkGp08HitAnim;
    }

    return 1;
}

s32 MapGmkGp08HitAnim(MapGmkGp08Work* work) {
    AnimState* anim = &work->anim;

    if (!AnimIsFinished(anim)) {
        work->gfx2 = AnimUpdate(anim);
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        work->overlayVisible = FALSE;
        work->update = MapGmkGp08WaitHit;
    }

    return 1;
}

void Task_MapGmk_GP08_0(MapGmkGp08Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = LoadObjTiles(def->tiles, def->tilesSize);
    work->palette = LoadObjPalette(def->palette, 32);
    AnimInit(&work->anim, def->anims, def->gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->hitSong = def->hitSong;
    work->overlayVisible = FALSE;
    work->update = MapGmkGp08WaitHit;
}

u8 Task_MapGmk_GP08_1(MapGmkGp08Work* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP08_2(MapGmkGp08Work* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);

    if (work->overlayVisible) {
        DrawSprite(x, y, work->gfx2, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority - 1);
    }
}

void Task_MapGmk_GP08_3(MapGmkGp08Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

s32 MapGmkGp09WaitStep(MapGmkGp09Work* work) {
    AnimState* anim = &work->anim;

    work->gfx2 = AnimUpdate(anim);

    if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
        if (work->collider.standFlags & COLLIDER_STAND_STOOD_ON) {
            if (!(work->placement->flags & GMK_FLAG_USED)) {
                work->placement->flags |= GMK_FLAG_USED;
                DropMapGmkPrize(&work->obj.fieldPosition);
            }

            AnimStart(anim, 1, 0);
            work->gfx2 = AnimGetGfx(anim);
            work->overlayVisible = TRUE;
            work->update = MapGmkGp09StepAnim;
        }
    }

    return 1;
}

s32 MapGmkGp09StepAnim(MapGmkGp09Work* work) {
    if (AnimIsFinished(&work->anim)) {
        work->overlayVisible = FALSE;
        work->update = MapGmkGp09WaitStepOff;
    } else {
        work->gfx2 = AnimUpdate(&work->anim);
    }

    return 1;
}

s32 MapGmkGp09WaitStepOff(MapGmkGp09Work* work) {
    if (!ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
        work->update = MapGmkGp09WaitStep;
    }

    return 1;
}

void Task_MapGmk_GP09_0(MapGmkGp09Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = LoadObjTiles(def->tiles, def->tilesSize);
    work->palette = LoadObjPalette(def->palette, 32);
    AnimInit(&work->anim, def->anims, def->gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->overlayVisible = FALSE;
    work->update = MapGmkGp09WaitStep;
}

u8 Task_MapGmk_GP09_1(MapGmkGp09Work* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_GP09_2(MapGmkGp09Work* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);

    if (work->overlayVisible) {
        DrawSprite(x, y, work->gfx2, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority - 1);
    }
}

void Task_MapGmk_GP09_3(MapGmkGp09Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

void Task_MapGmk00_0(MapGmk00Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;
    AnimState* anim;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = LoadObjTiles(def->tiles, def->tilesSize);
    work->palette = LoadObjPalette(def->palette, 32);
    anim = &work->anim;
    AnimInit(anim, def->anims, def->gfxTable);
    AnimStart(anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);

    if (FldObjIsOutOfView(obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    }

    work->radius = def->radius;
    work->stoodOn = FALSE;
    work->visible = TRUE;
    work->unk_0C6 = 0;
}

u8 Task_MapGmk00_1(MapGmk00Work* work) {
    FldPos* pos = &work->obj.fieldPosition;
    u16 standing;

    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (!(work->placement->flags & GMK_FLAG_USED)) {
        standing = work->collider.standFlags & COLLIDER_STAND_STOOD_ON;

        if (standing != 0) {
            if (work->stoodOn != TRUE) {
                work->stoodOn = TRUE;
                work->placement->flags |= GMK_FLAG_USED;
                DropMapGmkPrize(pos);
            }
        } else {
            work->stoodOn = FALSE;
        }
    }

    return 1;
}

void Task_MapGmk00_2(MapGmk00Work* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    if (work->visible) {
        x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
        pixelY = work->obj.fieldPosition.y >> 8;
        y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
        priority = -0x1004 - pixelY * 4;
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
    }
}

void Task_MapGmk00_3(MapGmk00Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

u8 MapGmk01WaitHit(MapGmk01Work* work) {
    if (IsHitByMapAttack(&work->obj.fieldPosition, 8, 8)) {
        AnimState* anim;

        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        m4aSongNumStart(SONG_SYS_TRESURE);
        anim = &work->anim;
        AnimStart(anim, 1, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(anim);
        SetObjTileSource(work->tiles, gMapGmk01Tiles);
        work->update = MapGmk01Open;
    }

    return 1;
}

u8 MapGmk01Open(MapGmk01Work* work) {
    FldPos* pos = &work->obj.fieldPosition;

    if (work->placement->flags & GMK_FLAG_HAS_ENEMY) {
        gMapRoomState->flags |= ROOM_FLAG_ENEMY_STRUCK;
        work->placement->flags |= GMK_FLAG_USED;
        TaskCreate(&gFieldState->tasks, &sTaskDescMapGmkEnm, pos);
        work->update = NULL;
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_NO_RANDOM_PRIZE;
        CreateWorldPrize(gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y, gFieldState->actor.fieldPosition.z);
        GetMapFloorRoom(gMapFloorState.room)->flags |= FLOOR_ROOM_FLAG_CHEST_OPENED;
        work->placement->flags |= GMK_FLAG_USED;
        work->timer = 20;
        work->update = MapGmk01WaitPrize;
    }

    return 1;
}

u8 MapGmk01WaitPrize(MapGmk01Work* work) {
    if (work->timer != 0) {
        if (!IsMessageWindowOpen()) {
            work->timer--;
        }
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        work->update = NULL;
    }

    return 1;
}

void Task_MapGmk01_0(MapGmk01Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;
    AnimState* anim;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    obj->height = def->height;
    work->tiles = AllocObjTiles(0x320, gMapGmk01Tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    anim = &work->anim;
    AnimInit(anim, def->anims, def->gfxTable);

    if (work->placement->flags & GMK_FLAG_USED) {
        AnimStart(anim, 1, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(anim);
        work->update = NULL;
    } else {
        gMapRoomState->flags |= ROOM_FLAG_NO_RANDOM_PRIZE;
        AnimStart(anim, 0, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(anim);
        work->update = MapGmk01WaitHit;
    }

    SetObjTileSource(work->tiles, gMapGmk01Tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, obj->fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
}

u8 Task_MapGmk01_1(MapGmk01Work* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk01_2(MapGmk01Work* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = work->obj.fieldPosition.y >> 8;
    y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk01_3(MapGmk01Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

void MapGmkBarrelDropPrizes(FldPos* pos) {
    u16 roll;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        roll = GetRandom() % 10000;

        if (roll < 6000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_SMALL, 5, pos->x, pos->y, pos->z);
        } else if (roll < 10000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_LARGE, 3, pos->x, pos->y, pos->z);
        }
    } else {
        roll = GetRandom() % 10000;

        if (roll < 3000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_SMALL, 5, pos->x, pos->y, pos->z);
        } else if (roll < 5000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_LARGE, 3, pos->x, pos->y, pos->z);
        } else if (roll < 8000) {
            CreateMapPrizeTasks(MAP_PRIZE_MOOGLE_POINTS_SMALL, 5, pos->x, pos->y, pos->z);
        } else {
            CreateMapPrizeTasks(MAP_PRIZE_MOOGLE_POINTS_LARGE, 5, pos->x, pos->y, pos->z);
        }
    }
}

u8 MapGmkBarrelWaitHit(MapGmkBarrelWork* work) {
    FldPos* pos = &work->obj.fieldPosition;
    u16 roll;

    if (IsHitByMapAttack(pos, 8, 8)) {
        roll = GetRandom() % 10000;

        if (roll <= 1499) {
            m4aSongNumStart(SONG_SYS_OBJ_BREAK);
            TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);
            gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
            gMapRoomState->flags |= ROOM_FLAG_ENEMY_STRUCK;
            work->placement->flags |= GMK_FLAG_DESTROYED;
            TaskCreate(&gFieldState->tasks, &sTaskDescMapGmkSpider, work->placement);
            return 0;
        }

        m4aSongNumStart(SONG_SYS_OBJ_BREAK);
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSpark, pos);

        if (roll <= 5999) {
            if (TryCreateRandomPrzCard(FALSE, pos->x, pos->y, pos->z) != TRUE) {
                MapGmkBarrelDropPrizes(pos);
            }
        } else if (roll <= 9999) {
            MapGmkBarrelDropPrizes(pos);
        }

        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        work->placement->flags |= GMK_FLAG_DESTROYED;
        ColliderSetDisabled(&work->collider, TRUE);
        AnimStart(&work->anim, 1, 0);
        work->update = MapGmkBarrelBreak;
        return 1;
    }

    if (FldObjIsOutOfView(&work->obj)) {
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    return 1;
}

u8 MapGmkBarrelBreak(MapGmkBarrelWork* work) {
    AnimState* anim = &work->anim;

    if (!AnimIsFinished(anim)) {
        work->gfx = AnimUpdate(anim);
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        work->visible = FALSE;
        work->update = NULL;
    }

    return 1;
}

void Task_MapGmk_Barrel_0(MapGmkBarrelWork* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;
    AnimState* anim;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    work->obj.fieldPosition.x += def->offsetX << 8;
    obj->fieldPosition.y += def->offsetY << 8;
    obj->fieldPosition.z += def->offsetZ << 8;
    obj->height = def->height;
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    work->palette = LoadObjPalette(def->palette, 32);
    anim = &work->anim;
    AnimInit(anim, def->anims, def->gfxTable);
    AnimStart(anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(anim);
    SetObjTileSource(work->tiles, def->tiles);
    ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, def->radius, def->height);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    work->visible = TRUE;
    work->update = MapGmkBarrelWaitHit;
}

u8 Task_MapGmk_Barrel_1(MapGmkBarrelWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (work->update != NULL) {
        return work->update(work);
    }

    return 1;
}

void Task_MapGmk_Barrel_2(MapGmkBarrelWork* work) {
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    if (work->visible) {
        x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
        pixelY = work->obj.fieldPosition.y >> 8;
        y = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);
        priority = -0x1004 - pixelY * 4;
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
    }
}

void Task_MapGmk_Barrel_3(MapGmkBarrelWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

void MapGmk04CheckTalk(MapGmk04Work* work) {
    if ((gMapRoomState->flags & ROOM_FLAG_TUTORIAL_ACTIVE) == 0 && (u8)IsFldObjTalkTarget(&work->obj) && (GetKeysPressed() & A_BUTTON)) {
        m4aSongNumStart(SONG_SYS_KETTEI);
        gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSave, NULL);
        work->update = NULL;
    }
}

void MapGmk04CheckFirstTalk(MapGmk04Work* work) {
    u32 state = gMapRoomState->flags;

    if (state & ROOM_FLAG_TUTORIAL_ACTIVE) {
        gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
        CreateCardMessageTask(&work->tasks, 0, CARD_MSG_QUICK_SAVE_TUTORIAL);
        work->update = MapGmk04WaitFirstTalkEnd;
    } else if (gFieldState->lockonTarget == &work->obj) {
        gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
        gMapRoomState->flags = state | ROOM_FLAG_TUTORIAL_ACTIVE;
        CreateCardMessageTask(&work->tasks, 0, CARD_MSG_SAVE_POINT_TUTORIAL);
        work->update = MapGmk04WaitMessage;
    }
}

void MapGmk04WaitMessage(MapGmk04Work* work) {
    if (!IsMessageWindowOpen()) {
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSave, NULL);
        work->update = NULL;
    }
}

void MapGmk04WaitFirstTalkEnd(MapGmk04Work* work) {
    if (!IsMessageWindowOpen()) {
        gMapRoomState->flags &= ~ROOM_FLAG_TUTORIAL_ACTIVE;
        gFieldState->flags &= ~FIELD_FLAG_FREEZE_PLAYER;
        gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_SAVE_POINT;
        work->update = MapGmk04CheckTalk;
    }
}

#ifdef PLATFORM_PC
void PortBotSavePoint(FldPos* pos);
#endif
void Task_MapGmk04_0(MapGmk04Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    work->placement = arg;
    obj->fieldPosition = arg->pos;
    obj->height = def->height;
    obj->kind = FLD_OBJ_KIND_FIXTURE;

    if (gGameState.progression.tutorialFlags & TUTORIAL_FLAG_SAVE_POINT) {
        work->update = MapGmk04CheckTalk;
    } else {
        work->update = MapGmk04CheckFirstTalk;
    }

    work->palette = LoadObjPalette(def->palette, 32);
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    AnimInit(&work->anim, def->anims, def->gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_NPC, 24, 24);
    ColliderSetPosition(&work->collider, obj->fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    TaskPoolInit(&work->tasks, 1);
    FldObjRegister(obj);
#ifdef PLATFORM_PC
    PortBotSavePoint(&obj->fieldPosition); /* port_testbot.c */
#endif
}

s32 Task_MapGmk04_1(MapGmk04Work* work) {
    TaskPoolUpdate(&work->tasks);

    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    work->gfx = AnimUpdate(&work->anim);

    if (work->update != NULL) {
        work->update(work);

        if (work->update != NULL) {
            return 1;
        }
    }

    return 0;
}

void Task_MapGmk04_2(MapGmk04Work* work) {
    FldPos* pos = &work->obj.fieldPosition;
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    TaskPoolDraw(&work->tasks);
    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = pos->y >> 8;
    y = pixelY + (pos->z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk04_3(MapGmk04Work* work) {
#ifdef PLATFORM_PC
    PortBotSavePoint(NULL);
#endif
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
    FldObjUnregister(&work->obj);
}

void MapGmk05CheckTalk(MapGmk05Work* work) {
    if (work->targeted && (GetKeysPressed() & A_BUTTON)) {
        gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
        RequestFieldResume();
        FadeStartOut(FADE_MODE_BLACK, 16);
        m4aSongNumStart(SONG_SYS_MOUGURI);
        work->update = MapGmk05EnterShop;
    }
}

void MapGmk05EnterShop(MapGmk05Work* work) {
    MapFloorRoom* floorRoom;

    if (FadeIsActive()) {
        return;
    }

    floorRoom = GetMapFloorRoom(gMapFloorState.room);

    if (floorRoom->flags & FLOOR_ROOM_FLAG_SHOP_VISITED) {
        ModeRequest(&gModeMsTop, 0);
    } else {
        floorRoom->flags |= FLOOR_ROOM_FLAG_SHOP_VISITED;
        ModeRequest(&gModeMsTop, 1);
    }

    work->update = NULL;
}

void Task_MapGmk05_0(MapGmk05Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;
    AnimState* anim;

    obj->fieldPosition = arg->pos;
    obj->height = def->height;
    obj->kind = FLD_OBJ_KIND_NPC;
    work->update = MapGmk05CheckTalk;
    work->palette = LoadObjPalette(def->palette, 32);
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    anim = &work->anim;
    AnimInit(anim, def->anims, def->gfxTable);
    AnimStart(anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_NPC, 16, 24);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    FldObjRegister(obj);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescFldShadow, obj);
    work->targeted = FALSE;
    TaskPoolInit(&work->tasks2, 1);
    TaskCreate(&work->tasks2, &gTaskDescMapTalk, obj);
}

s32 Task_MapGmk05_1(MapGmk05Work* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    TaskPoolUpdate(&work->tasks);
    TaskPoolUpdate(&work->tasks2);
    work->gfx = AnimUpdate(&work->anim);
    work->targeted = IsFldObjTalkTarget(&work->obj);

    if (work->update != NULL) {
        work->update(work);

        if (work->update != NULL) {
            return 1;
        }
    }

    return 0;
}

void Task_MapGmk05_2(MapGmk05Work* work) {
    FldObj* obj = &work->obj;
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    pixelY = obj->fieldPosition.y >> 8;
    priority = -0x1004 - pixelY * 4;
    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    y = pixelY + (obj->fieldPosition.z >> 8) - (gFieldState->y >> 8);
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
    obj->shadowZ = obj->fieldPosition.ground;
    obj->shadowPriority = priority + 1;
    TaskPoolDraw(&work->tasks);

    if (work->targeted) {
        TaskPoolDraw(&work->tasks2);
    }
}

void Task_MapGmk05_3(MapGmk05Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
    FldObjUnregister(&work->obj);
    TaskPoolDestroy(&work->tasks);
    TaskPoolDestroy(&work->tasks2);
}

void MapGmk06CheckTalk(MapGmk06Work* work) {
    if (gFieldState->lockonTarget == &work->obj && (gGameState.progression.tutorialFlags & TUTORIAL_FLAG_WARP_POINT) == 0) {
        gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
        gMapRoomState->flags |= ROOM_FLAG_TUTORIAL_ACTIVE;
        CreateCardMessageTask(&work->tasks, 0, CARD_MSG_WARP_POINT_TUTORIAL);
        gFieldState->lockonDelay = 30;
        work->update = MapGmk06WaitMessage;
    } else if ((u8)IsFldObjTalkTarget(&work->obj) && (GetKeysPressed() & A_BUTTON)) {
        m4aSongNumStart(SONG_SYS_KETTEI);
        gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
        RequestFieldResume();
        FadeStartOut(FADE_MODE_BLACK, 16);
        work->update = MapGmk06EnterWorldWarp;
    }
}

void MapGmk06EnterWorldWarp(MapGmk06Work* work) {
    if (!FadeIsActive()) {
        ModeRequest(&gModeWorldwarp, 0);
        work->update = NULL;
    }
}

void MapGmk06WaitMessage(MapGmk06Work* work) {
    if (!IsMessageWindowOpen()) {
        gFieldState->flags &= ~FIELD_FLAG_FREEZE_PLAYER;
        gMapRoomState->flags &= ~ROOM_FLAG_TUTORIAL_ACTIVE;
        gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_WARP_POINT;
        work->update = MapGmk06CheckTalk;
    } else {
        gFieldState->lockonDelay = 30;
    }
}

void Task_MapGmk06_0(MapGmk06Work* work, MapGmkPlacement* arg) {
    FldObj* obj = &work->obj;
    const MapGmkDef* def = arg->def;

    obj->fieldPosition = arg->pos;
    obj->height = def->height;
    obj->kind = FLD_OBJ_KIND_FIXTURE;
    work->update = MapGmk06CheckTalk;
    work->palette = LoadObjPalette(def->palette, 32);
    work->tiles = AllocObjTiles(def->tilesSize, def->tiles);
    AnimInit(&work->anim, def->anims, def->gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_NPC, 24, 24);
    ColliderSetPosition(&work->collider, work->obj.fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    TaskPoolInit(&work->tasks, 1);
    FldObjRegister(obj);
}

s32 Task_MapGmk06_1(MapGmk06Work* work) {
    TaskPoolUpdate(&work->tasks);

    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    work->gfx = AnimUpdate(&work->anim);

    if (work->update != NULL) {
        work->update(work);

        if (work->update != NULL) {
            return 1;
        }
    }

    return 0;
}

void Task_MapGmk06_2(MapGmk06Work* work) {
    FldPos* pos = &work->obj.fieldPosition;
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;

    TaskPoolDraw(&work->tasks);
    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    pixelY = pos->y >> 8;
    y = pixelY + (pos->z >> 8) - (gFieldState->y >> 8);
    priority = -0x1004 - pixelY * 4;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void Task_MapGmk06_3(MapGmk06Work* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
    FldObjUnregister(&work->obj);
}

void MapPrizeBounce(MapPrizeWork* work) {
    work->vz += 0x38;
    work->z += work->vz;
    work->x += gSineTable[work->angle] * work->speed >> 8;
    work->y += -gSineTable[work->angle + 64] * work->speed >> 8;

    if (IsFldPosBlocked((FldPos*)work) != 0) {
        work->angle = work->angle + (100 + GetRandom() % 57);
    } else {
        work->ground = GetFldPosGround((FldPos*)work);
    }

    if (work->z > work->ground) {
        work->z = work->ground;
        work->vz = -(GetRandom() % 0x181 + 0x180);
    }

    if (work->collider.colliding) {
        u16 maxHp;

        switch (work->kind) {
        case MAP_PRIZE_MOOGLE_POINTS_SMALL:
        case MAP_PRIZE_MOOGLE_POINTS_LARGE:
            m4aSongNumStart(SONG_SYS_POWER_GET);
            gGameState.progression.mooglePoints += work->amount;

            if (gGameState.progression.mooglePoints > 99999) {
                gGameState.progression.mooglePoints = 99999;
            }
#ifdef PLATFORM_PC
            {
                extern void PortMoogleNotice(unsigned amount, unsigned total);
                PortMoogleNotice(work->amount, gGameState.progression.mooglePoints);
            }
#endif

            break;
        case MAP_PRIZE_HP_SMALL:
        case MAP_PRIZE_HP_LARGE:
        default:
            m4aSongNumStart(SONG_SYS_POWER_GET);
            gGameState.hp += work->amount;
            maxHp = gGameState.progression.maxHp;

            if (gGameState.hp > (s16)maxHp) {
                gGameState.hp = maxHp;
            }

            break;
        }

        work->update = MapPrizeCollect;
        work->timer = 0;
        work->angle = GetAngle(gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y, work->x, work->y);
        work->collected = TRUE;
        work->visible = TRUE;
        work->angleStep = GetRandom() % 6 + 5;
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetPosition(&work->collider, work->x, work->y, work->z);

        if (work->timer == 20) {
            ColliderSetDisabled(&work->collider, FALSE);
        }

        if (work->timer > 420) {
                work->visible = !work->visible ? TRUE : FALSE;
        }

        if (work->timer++ > 480) {
            work->update = NULL;
        }
    }
}

void MapPrizeCollect(MapPrizeWork* work) {
    s32 x;
    s32 y;
    s32 z;
    s32 offset;
    FieldState* field = gFieldState;

    offset = gSineTable[work->angle] * 32;
    x = field->actor.fieldPosition.x + (offset * work->scale >> 8);
    offset = -gSineTable[work->angle + 64] * 22;
    y = field->actor.fieldPosition.y + (offset * work->scale >> 8);
    z = field->actor.fieldPosition.z - (work->timer / 2 << 8);
    work->angle += work->angleStep;
    work->x += (x - work->x) >> 2;
    work->y += (y - work->y) >> 2;
    work->z += (z - work->z) >> 2;
    work->ground = FieldFloorAt(work->x, work->y, work->z);
    work->scale -= 2;

    if (work->timer > 60) {
        work->update = NULL;
    } else {
        work->timer++;
    }
}

void Task_MapPrize_0(MapPrizeWork* work, MapPrizeArgs* arg) {
    work->x = arg->x;
    work->y = arg->y;
    work->z = arg->z;
    work->ground = 0;
    FldPosInitGround((FldPos*)work);
    work->vz = -(GetRandom() % 0x301 + 0x200);
    work->speed = GetRandom() % 155 + 153;
    work->angle = GetRandom();
    work->tiles = LoadObjTiles(gMapPrizeTiles, sizeof(gMapPrizeTiles));
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    work->kind = arg->id;

    switch (work->kind) {
    case MAP_PRIZE_MOOGLE_POINTS_LARGE:
        work->gfx = gMapPrizeFrame3;
        work->amount = 10;
        break;
    case MAP_PRIZE_MOOGLE_POINTS_SMALL:
        work->gfx = gMapPrizeFrame2;
        work->amount = 4;
        break;
    case MAP_PRIZE_HP_LARGE:
        work->gfx = gMapPrizeFrame1;
        work->amount = gGameState.progression.maxHp / 20;
        break;
    case MAP_PRIZE_HP_SMALL:
    default:
        work->gfx = gMapPrizeFrame0;
        work->amount = gGameState.progression.maxHp * 3 / 100;
        break;
    }

    work->gfx2 = gMapPrizeFrame4;
    work->collected = FALSE;
    work->visible = TRUE;
    work->timer = 0;
    work->update = MapPrizeBounce;
    work->scale = Q_8_8(1);
    ColliderInit(&work->collider, COLLIDER_TYPE_PICKUP, 16, 50);
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    ColliderSetDisabled(&work->collider, TRUE);
}

s32 Task_MapPrize_1(MapPrizeWork* work) {
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (work->update == NULL) {
        return 0;
    }

    work->update(work);

    if (work->update == NULL) {
        return 0;
    }

    return 1;
}

void Task_MapPrize_2(MapPrizeWork* work) {
    s16 x;
    s16 y;
    ObjAffine* aff;

    if (work->visible) {
        x = (work->x >> 8) - (gFieldState->x >> 8);
        y = (work->y >> 8) + (work->z >> 8) - (gFieldState->y >> 8);

        if (work->scale != Q_8_8(1)) {
            aff = AllocObjAffine(0, work->scale, work->scale, FALSE);
        } else {
            aff = NULL;
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, aff, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);

        if (!work->collected) {
            DrawSprite(x, (work->y >> 8) + (work->ground >> 8) - (gFieldState->y >> 8), work->gfx2, work->tiles, work->palette, aff, SPRITE_PRIORITY(2), 0xFFFF);
        }
    }
}

void Task_MapPrize_3(MapPrizeWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

void MapPrzCardUpdateScale(MapPrzCardWork* work) {
    work->scaleX = -COS(work->phaseX + 0x80) * work->scale >> 8;
    work->scaleY = -COS(work->phaseY + 0x80) * work->scale >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }
}

void MapPrzCardAimAtCenter(MapPrzCardWork* work) {
    s32 dx;
    s32 dy;

    dx = 0x7800;
    dy = 0x5000;
    dx -= work->posX;
    dy -= work->posY;
    work->distance = NormalizeVector2D8(&dx, &dy);
    work->dirX = -dx;
    work->dirY = -dy;
    work->speed = 0x300;
    work->vz = 2;
}

void MapPrzCardBounce(MapPrzCardWork* work) {
    FldPos prevPos = *(FldPos*)work;
    s32 nx;
    s32 ny;

    work->vz += 0x38;
    work->posZ += work->vz;
    work->posX += gSineTable[work->angle] * work->speed >> 8;
    work->posY += -gSineTable[work->angle + 64] * work->speed >> 8;

    if (IsFldPosBlocked((FldPos*)work) != 0) {
        work->angle = work->angle + (112 + GetRandom() % 33);
        work->posX = prevPos.x;
        work->posY = prevPos.y;
    } else {
        work->ground = GetFldPosGround((FldPos*)work);
    }

    if (work->posZ - 0x800 > work->ground) {
        work->posZ = work->ground - 0x800;
        work->vz = -(work->vz * 217 >> 8);

        if (work->vz > -0x200) {
            work->vz = -0x200;
        }
    }

    if (work->collider.colliding) {
        work->collected = TRUE;
        m4aSongNumStart(SONG_SYS_ITEMGET);
        ObtainCard(work->cardId);

        if (!work->worldPrize) {
            MapFloorRoom* floorRoom = GetMapFloorRoom(gMapFloorState.room);

            if (floorRoom->przCardsLeft != 0) {
                floorRoom->przCardsLeft--;
            }
        }

        nx = (work->posX >> 8) - (gFieldState->x >> 8);
        ny = (work->posY >> 8) + (work->posZ >> 8) - (gFieldState->y >> 8);
        work->posX = (s16)nx << 8;
        work->posY = (s16)ny << 8;
        ColliderSetDisabled(&work->collider, TRUE);
        work->priority = 50;
        MapPrzCardAimAtCenter(work);
        work->spriteFlags = 0;
        work->update = MapPrzCardFlyToCenter;
    } else {
        work->x = (work->posX >> 8) - (gFieldState->x >> 8);
        work->y = (work->posY >> 8) + (work->posZ >> 8) - (gFieldState->y >> 8);
        work->priority = -0x1004 - (work->posY >> 8) * 4;
        MapPrzCardUpdateScale(work);
        work->phaseX += 2;
        ColliderSetPosition(&work->collider, work->posX, work->posY, work->posZ);

        if (work->timer == 20) {
            ColliderSetDisabled(&work->collider, FALSE);
        }

        if (work->timer <= 59) {
            work->timer++;
        }
    }
}

void MapPrzCardFlyToCenter(MapPrzCardWork* work) {
    s32 dx;
    s32 dy;
    s32 x;
    s32 y;

    if (work->speed < 0) {
        dx = 0x7800 - work->posX;
        dy = 0x5000 - work->posY;
        NormalizeVector2D8(&dx, &dy);
        work->dirX = -dx;
        work->dirY = -dy;

        if (work->distance <= 0x7FF) {
            work->rotation = 0;
            work->timer = 0;
            work->update = MapPrzCardShowName;
            TaskCreate(&work->tasks, &gTaskDescMapMsg, LANGSEL(gCardDefs[work->cardId].name));
        }
    }

    work->posX += work->dirX * work->speed >> 8;
    work->posY += work->dirY * work->speed >> 8;
    work->rotation += 32;
    work->phaseY += (64 - work->phaseY) >> 4;
    work->phaseX = 0;
    work->distance = VectorLength2D(0x7800 - work->posX, 0x5000 - work->posY);
    work->speed -= work->vz;
    work->vz += 2;
    work->scale += 3;

    if (work->scale > Q_8_8(1)) {
        work->scale = Q_8_8(1);
    }

    x = work->posX >> 8;
    work->x = x;
    y = work->posY >> 8;
    work->y = y;
    MapPrzCardUpdateScale(work);
}

void MapPrzCardShowName(MapPrzCardWork* work) {
    s32 x;
    s32 y;

    work->posX = 0x7800;
    work->posY = 0x5800;
    work->rotation = 0;
    work->phaseY = 0;
    work->scale += 2;

    if (work->scale > Q_8_8(1)) {
        work->scale = Q_8_8(1);
    }

    x = work->posX >> 8;
    work->x = x;
    y = work->posY >> 8;
    work->y = y;
    MapPrzCardUpdateScale(work);
    work->timer++;

    if (work->timer == 30) {
        work->timer = 0;
        work->update = MapPrzCardShrink;
    }

    TaskPoolUpdate(&work->tasks);
}

void MapPrzCardShrink(MapPrzCardWork* work) {
    s32 x;
    s32 y;

    work->rotation += 32;
    x = (gFieldState->actor.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    y = (gFieldState->actor.fieldPosition.y >> 8) + (gFieldState->actor.fieldPosition.z >> 8) - (gFieldState->y >> 8);
    work->x += ((s16)x - work->x) >> 3;
    work->y += ((s16)y - work->y) >> 3;
    work->scaleX -= 10;
    work->scaleY -= 10;

    if (work->scaleX <= 10) {
        work->update = NULL;
    }
}

void Task_MapPrzCard_0(MapPrzCardWork* work, MapPrizeArgs* arg) {
    const CardDef* def;
    const CardBack* back;

    gMapRoomState->flags |= ROOM_FLAG_PRIZE_CARD_ACTIVE;
    work->cardId = arg->id;
    def = &gCardDefs[work->cardId];
    work->tiles = LoadObjTiles(def->tiles, 0x300);
    work->palette = LoadObjPalette(def->palette, 32);
    work->stat = *(CardStat*)&def->kind;
    back = &gCardBacks[work->stat.category];
    work->palette2 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles2 = LoadObjTiles(back->tiles, 0x280);
    work->tiles3 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    work->palette3 = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    work->tiles4 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->posX = arg->x;
    work->posY = arg->y;
    work->posZ = arg->z;
    work->ground = 0;
    FldPosInitGround((FldPos*)work);
    work->vz = -(GetRandom() % 129 + 0x300);
    work->speed = GetRandom() % 129 + 128;
    work->angle = GetRandom();
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->scale = Q_8_8(0.5);
    work->rotation = 24;
    work->phaseY = 0;
    work->phaseX = 0;
    work->worldPrize = arg->worldPrize;
    ColliderInit(&work->collider, COLLIDER_TYPE_PICKUP, 30, 10);
    ColliderSetPosition(&work->collider, work->posX, work->posY, work->posZ);

    if (work->worldPrize) {
        ColliderSetDisabled(&work->collider, FALSE);
    } else {
        ColliderSetDisabled(&work->collider, TRUE);
    }

    work->spriteFlags = SPRITE_PRIORITY(2);
    work->timer = 0;
    work->update = MapPrzCardBounce;
    work->collected = FALSE;
    TaskPoolInit(&work->tasks, 1);
}

#ifdef PLATFORM_PC
void PortBotPrize(int x, int y, int z);
#endif
s32 Task_MapPrzCard_1(MapPrzCardWork* work) {
#ifdef PLATFORM_PC
    if (!work->collected)
        PortBotPrize(work->posX >> 8, work->posY >> 8, work->ground >> 8); /* port_testbot.c */
#endif
    if ((u8)IsMapInterrupted()) {
        return 0;
    }

    if (work->update == NULL) {
        return 0;
    }

    work->update(work);

    if (work->update == NULL) {
        return 0;
    }

    return 1;
}

void Task_MapPrzCard_2(MapPrzCardWork* work) {
    const CardDef* def;
    const CardBack* back;
    void* digitGfx;
    ObjAffine* affine;
    s16 x;
    s16 y;
    s16 shadowScale;

    if (work->scaleX == Q_8_8(1) && work->scaleY == Q_8_8(1) && work->rotation == 0) {
        affine = NULL;
    } else {
        affine = AllocObjAffine(work->rotation, work->scaleX, work->scaleY, TRUE);
    }

    def = &gCardDefs[work->cardId];
    DrawSprite(work->x, work->y - 8, def->gfx, work->tiles, work->palette,
        affine, work->spriteFlags, work->priority + 1);
    back = &gCardBacks[work->stat.category];
    DrawSprite(work->x, work->y - 8, back->gfx, work->tiles2,
        work->palette2, affine, work->spriteFlags, work->priority);

    if (work->stat.category != CARD_CATEGORY_ENEMY) {
        digitGfx = gCardValueDigitFrames[work->stat.value];
        DrawSprite(work->x, work->y - 8, digitGfx, work->tiles3,
            work->palette2, affine, work->spriteFlags, work->priority - 1);
    }

    if (!work->collected) {
        x = (work->posX >> 8) - (gFieldState->x >> 8);
        y = (work->posY >> 8) + (work->ground >> 8) - (gFieldState->y >> 8);
        shadowScale = Q_8_8(0.8) - ((work->ground - work->posZ) >> 7);

        if (shadowScale <= 2) {
            shadowScale = 2;
        }

        DrawSprite(x, y, gBtlShadowFrames[0], work->tiles4, work->palette3,
            AllocObjAffine(0, shadowScale, shadowScale, FALSE), SPRITE_PRIORITY(2), work->priority + 2);
    }

    TaskPoolDraw(&work->tasks);
}

void Task_MapPrzCard_3(MapPrzCardWork* work) {
    FadeSetPaletteExcluded(work->palette2->index + 0x10, FALSE);
    FadeSetPaletteExcluded(work->palette->index + 0x10, FALSE);
    ColliderUnregister(&work->collider);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette3);
    TaskPoolDestroy(&work->tasks);
    gMapRoomState->flags &= ~ROOM_FLAG_PRIZE_CARD_ACTIVE;
}

void MapPrzStockShowMessage(MapPrzStockWork* work) {
    CreateCardMessageTask(&work->tasks, 0, work->stock[1]);
    work->update = MapPrzStockWaitMessage;
}

void MapPrzStockWaitMessage(MapPrzStockWork* work) {
    if (!IsMessageWindowOpen()) {
        work->update = NULL;
    }
}

void Task_MapPrzStock_0(MapPrzStockWork* work, u16* stock) {
    work->stock = stock;
    gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
    gFieldState->flags |= FIELD_FLAG_FREEZE_ENEMIES;
    work->update = MapPrzStockShowMessage;
    TaskPoolInit(&work->tasks, 1);
}

s32 Task_MapPrzStock_1(MapPrzStockWork* work) {
    TaskPoolUpdate(&work->tasks);

    if (work->update != NULL) {
        work->update(work);

        if (work->update != NULL) {
            return 1;
        }
    }

    return 0;
}

void Task_MapPrzStock_2(MapPrzStockWork* work) {
    TaskPoolDraw(&work->tasks);
}

void Task_MapPrzStock_3(MapPrzStockWork* work) {
    TaskPoolDestroy(&work->tasks);
    gFieldState->flags &= ~FIELD_FLAG_FREEZE_PLAYER;
    gFieldState->flags &= ~FIELD_FLAG_FREEZE_ENEMIES;
}

void MapMsgInit(MapMsgWork* work, void* text) {
    LoadBgTiles(0, gMapMsgWinTiles, sizeof(gMapMsgWinTiles));
    LoadBgMap(0, gMapMsgWinMap, sizeof(gMapMsgWinMap));
    SetBgScroll(0, 0, (u16)-46);
    LoadPalette(gCard00Palette, (void*)(BG_PLTT + 15 * PLTT_SIZE_4BPP), sizeof(gCard00Palette));
    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    work->textSlotCount = LoadTextSlots(text, work->textSlots);
    work->palette = LoadTextPalette(1);
    FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    work->textX = (DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) / 2;
    work->timer = 0;
}

s32 Task_MapMsg_1(MapMsgWork* work) {
    return 1;
}

void MapMsgDraw(MapMsgWork* work) {
    DrawTextSlots(work->textX, 120, work->textSlots, work->palette, 50, work->textSlotCount);
}

void MapMsgDestroy(MapMsgWork* work) {
    if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) == 0) {
        DisableBg(0);
    }

    FadeSetPaletteExcluded(work->palette->index + 0x10, FALSE);
    ReleaseObjPalette(work->palette);
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
}

s32 Task_MapMsg2_1(MapMsgWork* work) {
    work->timer++;

    if (work->timer == 120) {
        return 0;
    }

    return 1;
}

void Task_MapSpark_0(MapSparkWork* work, FldObj* obj) {
    AnimState* anim;

    work->obj = obj;
    work->tiles = AllocObjTiles(0x200, gMapSparkTiles);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    anim = &work->anim;
    AnimInit(anim, gMapSparkAnims, gMapSparkFrames);

    if (gGameState.flags & GAME_FLAG_RIKU) {
        AnimStart(anim, 1, ANIM_FLAG_LOOP);
    } else {
        AnimStart(anim, 0, ANIM_FLAG_LOOP);
    }
}

s32 Task_MapSpark_1(MapSparkWork* work) {
    AnimUpdate(&work->anim);

    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    return 1;
}

void Task_MapSpark_2(MapSparkWork* work) {
    FldObj* obj = work->obj;
    s32 height;
    u16 x;
    u16 y;

    if (obj->height <= 32) {
        height = obj->height << 8;
    } else {
        height = 0x2000;
    }

    x = (obj->fieldPosition.x >> 8) - (gFieldState->x >> 8);
    y = (obj->fieldPosition.y >> 8) + ((obj->fieldPosition.z - height) >> 8) - (gFieldState->y >> 8);
    DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0x50);
}

void Task_MapSpark_3(MapSparkWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void Task_MapTalk_0(MapTalkWork* work, FldObj* obj) {
    work->obj = obj;
    work->tiles = AllocObjTiles(0x200, &gMapSparkTiles[0x1028]);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    AnimInit(&work->anim, gMapTalkAnims, gMapTalkFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->playerOnRight = FALSE;
}

s32 Task_MapTalk_1(MapTalkWork* work) {
    FldObj* obj = work->obj;
    AnimState* anim = &work->anim;

    AnimUpdate(anim);

    if (gFieldState->actor.fieldPosition.x < obj->fieldPosition.x) {
        work->playerOnRight = FALSE;
        AnimStart(anim, 0, ANIM_FLAG_LOOP);
    } else {
        work->playerOnRight = TRUE;
        AnimStart(anim, 1, ANIM_FLAG_LOOP);
    }

    return 1;
}

void Task_MapTalk_2(MapTalkWork* work) {
    FldObj* obj = work->obj;
    u16 x;
    u16 y;

    if (work->playerOnRight) {
        x = (obj->fieldPosition.x >> 8) - (gFieldState->x >> 8) - 16;
    } else {
        x = (obj->fieldPosition.x >> 8) - (gFieldState->x >> 8) + 16;
    }

    y = (obj->fieldPosition.y >> 8) + ((obj->fieldPosition.z - (obj->height << 8)) >> 8) - (gFieldState->y >> 8);
    DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0x50);
}

void Task_MapTalk_3(MapTalkWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

const MapGmkDef gMapGmk01Def = {
    gMapGmk01Palette, gMapGmk01Tiles, 0x200, gMapGmk01Frames, gMapGmk01Anims,
    TRUE, 13, 0, 0, 0, 16, 16, SONG_SYS_TRESURE, &gTaskDescMapGmk01,
};

TaskDesc gTaskDescMapGmk01 = {
    "Task_MapGmk01",
    (TaskInitFunc)Task_MapGmk01_0,
    (TaskUpdateFunc)Task_MapGmk01_1,
    (TaskDrawFunc)Task_MapGmk01_2,
    (TaskDestroyFunc)Task_MapGmk01_3,
    sizeof(MapGmk01Work),
};

const MapGmkDef gMapGmkBarrelDef = {
    gMapGmkBarrelPalette, gMapGmkBarrelTiles, 0x400, gMapGmkBarrelFrames, gMapGmkBarrelAnims,
    TRUE, 0, 0, 0, 0, 12, 24, SONG_SYS_OBJ_BREAK, &gTaskDescMapGmkBarrel,
};

TaskDesc gTaskDescMapGmkBarrel = {
    "Task_MapGmk_Barrel",
    (TaskInitFunc)Task_MapGmk_Barrel_0,
    (TaskUpdateFunc)Task_MapGmk_Barrel_1,
    (TaskDrawFunc)Task_MapGmk_Barrel_2,
    (TaskDestroyFunc)Task_MapGmk_Barrel_3,
    sizeof(MapGmkBarrelWork),
};

const MapGmkDef gMapGmk04Def = {
    gMapGmk04Palette, gMapGmk04Tiles, 0x400, gMapGmk04Frames, gMapGmk04Anims,
    TRUE, 13, 0, 0, 0, 24, 62, SONG_SYS_KETTEI, &gTaskDescMapGmk04,
};

TaskDesc gTaskDescMapGmk04 = {
    "Task_MapGmk04",
    (TaskInitFunc)Task_MapGmk04_0,
    (TaskUpdateFunc)Task_MapGmk04_1,
    (TaskDrawFunc)Task_MapGmk04_2,
    (TaskDestroyFunc)Task_MapGmk04_3,
    sizeof(MapGmk04Work),
};

const MapGmkDef gMapGmk05Def = {
    gMoguPalette, gMoguFl00Tiles, 0x100, gMoguFl00Frames, gMoguFl00Anims,
    TRUE, 13, 0, 0, 0, 16, 24, SONG_SYS_MOUGURI, &gTaskDescMapGmk05,
};

TaskDesc gTaskDescMapGmk05 = {
    "Task_MapGmk05",
    (TaskInitFunc)Task_MapGmk05_0,
    (TaskUpdateFunc)Task_MapGmk05_1,
    (TaskDrawFunc)Task_MapGmk05_2,
    (TaskDestroyFunc)Task_MapGmk05_3,
    sizeof(MapGmk05Work),
};

const MapGmkDef gMapGmk06Def = {
    gMapGmk06Palette, gMapGmk06Tiles, 0x400, gMapGmk06Frames, gMapGmk06Anims,
    TRUE, 13, 0, 0, 0, 24, 54, SONG_SYS_KETTEI, &gTaskDescMapGmk06,
};

TaskDesc gTaskDescMapGmk06 = {
    "Task_MapGmk06",
    (TaskInitFunc)Task_MapGmk06_0,
    (TaskUpdateFunc)Task_MapGmk06_1,
    (TaskDrawFunc)Task_MapGmk06_2,
    (TaskDestroyFunc)Task_MapGmk06_3,
    sizeof(MapGmk06Work),
};

TaskDesc gTaskDescMapPrize = {
    "Task_MapPrize",
    (TaskInitFunc)Task_MapPrize_0,
    (TaskUpdateFunc)Task_MapPrize_1,
    (TaskDrawFunc)Task_MapPrize_2,
    (TaskDestroyFunc)Task_MapPrize_3,
    sizeof(MapPrizeWork),
};

TaskDesc gTaskDescMapPrzCard = {
    "Task_MapPrzCard",
    (TaskInitFunc)Task_MapPrzCard_0,
    (TaskUpdateFunc)Task_MapPrzCard_1,
    (TaskDrawFunc)Task_MapPrzCard_2,
    (TaskDestroyFunc)Task_MapPrzCard_3,
    sizeof(MapPrzCardWork),
};

TaskDesc gTaskDescMapPrzStock = {
    "Task_MapPrzStock",
    (TaskInitFunc)Task_MapPrzStock_0,
    (TaskUpdateFunc)Task_MapPrzStock_1,
    (TaskDrawFunc)Task_MapPrzStock_2,
    (TaskDestroyFunc)Task_MapPrzStock_3,
    sizeof(MapPrzStockWork),
};

TaskDesc gTaskDescMapMsg = {
    "Task_MapMsg",
    (TaskInitFunc)MapMsgInit,
    (TaskUpdateFunc)Task_MapMsg_1,
    (TaskDrawFunc)MapMsgDraw,
    (TaskDestroyFunc)MapMsgDestroy,
    sizeof(MapMsgWork),
};

TaskDesc gTaskDescMapMsg2 = {
    "Task_MapMsg2",
    (TaskInitFunc)MapMsgInit,
    (TaskUpdateFunc)Task_MapMsg2_1,
    (TaskDrawFunc)MapMsgDraw,
    (TaskDestroyFunc)MapMsgDestroy,
    sizeof(MapMsgWork),
};

TaskDesc gTaskDescMapSpark = {
    "Task_MapSpark",
    (TaskInitFunc)Task_MapSpark_0,
    (TaskUpdateFunc)Task_MapSpark_1,
    (TaskDrawFunc)Task_MapSpark_2,
    (TaskDestroyFunc)Task_MapSpark_3,
    sizeof(MapSparkWork),
};

TaskDesc gTaskDescMapTalk = {
    "Task_MapTalk",
    (TaskInitFunc)Task_MapTalk_0,
    (TaskUpdateFunc)Task_MapTalk_1,
    (TaskDrawFunc)Task_MapTalk_2,
    (TaskDestroyFunc)Task_MapTalk_3,
    sizeof(MapTalkWork),
};
