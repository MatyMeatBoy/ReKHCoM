/**
 * map.c
 * Field Map Modes and Menus
 */

#ifdef PLATFORM_PC
#include "engine.h"
extern int PortSaveSlotSelectEnabled(void);
void PortLog(const char* fmt, ...);
#endif
#include "system_state.h"
#include "monsgage.h"
#include "map.h"
#include "map_spawn_data.h"
#include "sprites_btl.h"
#include "sprites_evt.h"
#include "sprites_map.h"
#include "sprites_sora.h"
#include "world_types.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "engine_math.h"
#include "mode_pooh_api.h"
#include "map_runtime.h"
#include "malloc.h"
#include "fade.h"
#include "player_progression.h"
#include "status_api.h"
#include "allmap_api.h"
#include "songs.h"
#include "map_fixed_data.h"
#include "jiminy_data.h"
#include "common_text.h"
#include <string.h>
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "btl_collision.h"
#include "card_api.h"
#include "display.h"
#include "field_state.h"
#include "fld_types.h"
#include "game_state.h"
#include "gba/defines.h"
#include "key.h"
#include "listpool.h"
#include "m4a_catalog_data.h"
#include "m4a_song.h"
#include "map_api.h"
#include "map_enemy_data.h"
#include "map_room_data.h"
#include "map_room_types.h"
#include "map_types.h"
#include "mode.h"
#include "mode_battle_data.h"
#include "msg_api.h"
#include "obj.h"
#include "obj_api.h"
#include "player_progression_types.h"
#include "registration_data.h"
#include "save.h"
#include "save_api.h"
#include "save_types.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "text.h"
#include "text_types.h"
#include "types.h"
#include <stddef.h>
#include "card_map_anim.h"
#include "card_deckmenu2.h"
#include "map_room_tables.h"
#include "sprite_palettes.h"
#include "battle_ids.h"
#include "card_ids.h"
#include "card_message_data.h"
#include "event_ids.h"
#include "mode_movie.h"
#include "macros.h"
#include "mode_sio_api.h"
#include "card_ui_types.h"
#ifdef PLATFORM_PC
/* number tiles registered as HD text, as the deck menus do (card_deckmenu2.c) */
void PortRegisterBgTextTile(void* address, int code, int style, const void* source32);
#define PORT_BG_DIGIT(dst, d, src) PortRegisterBgTextTile((void*)(dst), '0' + (d), 2, (const void*)(src))
#else
#define PORT_BG_DIGIT(dst, d, src) ((void)0)
#endif


#ifndef PORT_MENU_LEFT
#define PORT_MENU_LEFT(x) (x)
#define PORT_MENU_RIGHT(x) (x)
#endif
#ifndef PORT_BAND
#define PORT_BAND(draw) draw /* GBA: one band */
#endif

extern u8 gSoraWorldBattleBase[];
extern u8 gRikuWorldBattleBase[];
extern u8 (*gMapGmkSpotFuncs[])(FldPos*);
extern u8 (*gMapAnmCmds[])(MapAnmSlot*);
extern u8 gWorldEntryEvents[];

static u8 sMapEnmCount;
static u8 sMapEnmTileCount;
static u8 sMapEnmSpawnTimer;
static FldPos sMapEnmSpawnPositions[3];
static u8 sMapGmkCount;
static u8 sMapGmkPaletteCount;
static u16 sMapGmkTileCount;
static const EventKeyList* sEventKeyList;
static EventKey sEventKey;
static EventKeyProgress* sEventKeyProgress;
static ModeFunc sMapDbgUpdate;
static TaskPool sMapDbgTasks;
static u8 sMapDbgEditing;
static void* sMapDbgAllmapRoomTask;
static u32 sUnk_02034FAC;
static ModeFunc sMapFldUpdate;
static Task* sMapFldWorldLogoTask;
static void* sMapFldAllmapRoomTask;
static u8 sUnk_02034FBC;
static u8 sUnk_02034FC0[0x14];
static ModeFunc sMapFixUpdate;
static u8 sMapFixEventDelay;
static NewGameSlotMenuWork* sNewGameSlotMenuWork;
static LoadGameMenuWork* sLoadGameMenuWork;
static MenuMsgWork* sMenuMsgWork;
#ifdef PLATFORM_PC
static u8 sMapSaveTargetSlot;

/* test harness (main.c gmkall): how many gimmicks the room placed */
u8 PortMapGmkCount(void) {
    return sMapGmkCount;
}
#endif

void MapEnmPlaceInView(MapEnmArgs* arg) {
    FldPos* pos = &arg->pos;
    s32 ground;

    MapPickFreeFloorPosInView(pos, &pos->y);
    pos->z = 0;
    ground = GetFldPosFloor(pos);
    pos->ground = ground;
    pos->y -= ground;
    pos->z = ground;
}

void MapEnmPlaceInViewAbove(MapEnmArgs* arg) {
    FldPos* pos = &arg->pos;
    s32 ground;

    MapPickFreeFloorPosInView(pos, &pos->y);
    pos->z = 0;
    ground = GetFldPosFloor(pos);
    pos->ground = ground;
    pos->y -= ground;
    pos->z = -0xA000;
}

s32 MapEnmPlaceInRoom(MapEnmArgs* arg) {
    FldPos* pos = &arg->pos;
    s32 ground;
    s32 i;

    if (MapPickFreeFloorPos(pos, &pos->y)) {
        pos->z = 0;
        ground = GetFldPosFloor(pos);
        pos->ground = ground;
        pos->z = ground;
        pos->y -= ground;

        for (i = 0; i < sMapEnmCount; i++) {
            if (sMapEnmSpawnPositions[i].x >> 8 == pos->x >> 8 && sMapEnmSpawnPositions[i].y >> 8 == pos->y >> 8) {
                return FALSE;
            }
        }

        return TRUE;
    }

    return FALSE;
}

u8 MapEnmPlaceAtStairs(MapEnmArgs* arg) {
    FldPos* pos = &arg->pos;
    MapPlatform* platform = GetMapPlatform(1);
    u16 wd = platform->right - platform->left - 2;
    u16 ht = gMapRoomState->bottomRow - gMapRoomState->topRow - 2;
    s32 i;

    for (i = 0; i < ht; i++) {
        s16 y = gMapRoomState->topRow + i;
        s32 j;

        for (j = 0; j < wd; j++) {
            s32 x = (s16)(platform->left + j);
            MapCell* cell = MapCellAt(x, y);

            if (cell->lowerZ == platform->z && (cell->flags & MAP_CELL_FLAG_STAIRS)) {
                s32 ground;
                s32 coord;

                if (cell->type == MAP_CELL_TYPE_LEFT_WALL_BASE) {
                    arg->angle = FLD_ANGLE_DOWN_RIGHT;
                    coord = (x << 13) + 0x1800;
                } else if (cell->type == MAP_CELL_TYPE_RIGHT_WALL_BASE) {
                    arg->angle = FLD_ANGLE_DOWN_LEFT;
                    coord = (x << 13) + 0x800;
                } else {
                    continue;
                }

                pos->x = coord;
                coord = y << 12;
                pos->y = coord + 0x1800;
                pos->z = 0;
                ground = GetFldPosFloor(pos);
                pos->ground = ground;
                pos->z = ground;
                pos->y -= ground;
                return TRUE;
            }
        }
    }

    return FALSE;
}

u8 MapEnmPlaceAboveGmk01(MapEnmArgs* arg) {
    FldPos* pos = &arg->pos;
    MapPlatform* platform = GetMapPlatform(0);
    u16 wd = platform->right - platform->left - 2;
    u16 ht = gMapRoomState->bottomRow - gMapRoomState->topRow - 2;
    s32 i;

    for (i = 0; i < ht; i++) {
        s16 y = gMapRoomState->topRow + i;
        s32 j;

        for (j = 0; j < wd; j++) {
            s32 x = (s16)(platform->left + j);

            if (MapCellAt(x, y)->flags & MAP_CELL_FLAG_CHEST) {
                s32 ground;

                pos->x = x << 13;
                pos->y = y << 12;
                pos->z = 0;
                ground = GetFldPosFloor(pos);
                pos->ground = ground;
                pos->y -= ground;
                pos->z = ground - 0x2000;
                return TRUE;
            }
        }
    }

    return FALSE;
}

void MapEnmSetupArgs(MapEnmArgs* arg, const MapEnmDef* def) {
    if (def->flags & MAP_ENM_DEF_FLAG_SPAWN_ANYWHERE) {
        MapEnmPlaceInRoom(arg);
    } else if (def->flags & MAP_ENM_DEF_FLAG_AIRBORNE) {
        MapEnmPlaceInViewAbove(arg);
    } else {
        MapEnmPlaceInView(arg);
    }

    switch (GetRandom() % 4) {
    case 0:
        arg->angle = FLD_ANGLE_DOWN_LEFT;
        break;
    case 1:
        arg->angle = FLD_ANGLE_DOWN_RIGHT;
        break;
    case 2:
        arg->angle = FLD_ANGLE_UP_LEFT;
        break;
    default:
        arg->angle = FLD_ANGLE_UP_RIGHT;
        break;
    }

    arg->speed = 0;
    arg->def = def;
    arg->update = NULL;
}

enum MapEnmPlace {
    MAP_ENM_PLACE_ROOM,
    MAP_ENM_PLACE_STAIRS = 2,
    MAP_ENM_PLACE_CHEST
};

void MapEnmSpawnFixed(MapEnmArgs* arg, u8 kind, u8 place) {
    const u8* shape;
    MapFloorRoom* floorRoom;
    const MapEnmDef* def;
    u8 ok;

    shape = gMapRoomShapes[gMapRoomState->roomType];
    floorRoom = GetMapFloorRoom(gMapFloorState.room);

    if (sMapEnmCount >= shape[1]) {
        return;
    }

    if (floorRoom->enemiesLeft - sMapEnmCount <= 0) {
        return;
    }

    def = gMapEnmDefs[kind];

    switch (place) {
    case MAP_ENM_PLACE_STAIRS:
        ok = MapEnmPlaceAtStairs(arg);
        break;
    case MAP_ENM_PLACE_CHEST:
        ok = MapEnmPlaceAboveGmk01(arg);
        break;
    case MAP_ENM_PLACE_ROOM:
    default:
        ok = MapEnmPlaceInRoom(arg);
        arg->angle = GetAngle(arg->pos.x, arg->pos.y, gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
        break;
    }

    if (ok) {
        sMapEnmSpawnPositions[sMapEnmCount] = arg->pos;
        arg->speed = 0;
        arg->def = def;
        arg->update = NULL;
        TaskCreate(&gFieldState->tasks4, def->desc, arg);
    }
}

void MapEnmApplyRoomFlags(MapEnmWork* work) {
    switch (gMapRoomState->roomType) {
    case ROOM_TYPE_LOOMING_DARKNESS:
        work->flags |= MAP_ENM_FLAG_AGGRESSIVE;
        break;
    case ROOM_TYPE_SLEEPING_DARKNESS:
        work->flags |= MAP_ENM_FLAG_ASLEEP;
        work->flags |= MAP_ENM_FLAG_PERSISTENT;
        break;
    case ROOM_TYPE_STAGNANT_SPACE:
        work->flags |= MAP_ENM_FLAG_SLOW;
        break;
    case ROOM_TYPE_WHITE_ROOM:
        work->flags |= MAP_ENM_FLAG_WHITE_MUSHROOM;
        break;
    case ROOM_TYPE_BLACK_ROOM:
        work->flags |= MAP_ENM_FLAG_BLACK_FUNGUS;
        break;
    }
}

void MapEnmSetAnim(MapEnmWork* work, u8 index, u16 flags) {
    const AnimDef* animDef = work->def->animDef;

    switch (work->obj.angle >> 6) {
    case 0:
        animDef += index * 2;
        work->flags |= MAP_ENM_FLAG_HFLIP;
        break;
    case 1:
        animDef += index * 2 + 1;
        work->flags |= MAP_ENM_FLAG_HFLIP;
        break;
    case 2:
        animDef += index * 2 + 1;
        work->flags &= ~MAP_ENM_FLAG_HFLIP;
        break;
    default:
        animDef += index * 2;
        work->flags &= ~MAP_ENM_FLAG_HFLIP;
        break;
    }

    AnimChangeWithTables(&work->anim, animDef->animId, flags, animDef->anims, animDef->gfxTable);
    SetObjTileSource(work->tiles, animDef->tiles);
}

void MapEnmUpdateAnim(MapEnmWork* work) {
    if (gFieldState->flags & FIELD_FLAG_ENEMY_FRAME_CHANGED) {
        if (AnimIsFrameEnding(&work->anim)) {
            return;
        }
    } else {
        if (AnimIsFrameEnding(&work->anim)) {
            gFieldState->flags |= FIELD_FLAG_ENEMY_FRAME_CHANGED;
        }
    }

    work->gfx = AnimUpdate(&work->anim);
}

u8 GetRandomBattleId() {
    const u8* shape = gMapRoomShapes[gMapRoomState->roomType];
    u8 battleIndex = shape[3] + GetRandom() % (shape[4] - shape[3] + 1);

    if (gGameState.flags & GAME_FLAG_RIKU) {
        return battleIndex + gRikuWorldBattleBase[gMapFloorState.world];
    }

    return battleIndex + gSoraWorldBattleBase[gMapFloorState.world];
}

void MapEnmStartBattle(MapEnmWork* work) {
    gGameState.flags |= GAME_FLAG_MAP_ENEMY_BATTLE;
    ColliderSetDisabled(&work->collider, TRUE);
    gMapRoomState->flags |= ROOM_FLAG_START_BATTLE;
    gFieldState->flags |= FIELD_FLAG_FREEZE_ENEMIES;
    work->flags |= MAP_ENM_FLAG_REMOVED;

    if (work->flags & MAP_ENM_FLAG_FIRST_STRIKE) {
        gGameState.flags |= GAME_FLAG_FIRST_STRIKE;
    }

    if (work->flags & MAP_ENM_FLAG_WHITE_MUSHROOM) {
        gMapRoomState->battleId = GetRandom() % 3 + 128;
    } else if (work->flags & MAP_ENM_FLAG_BLACK_FUNGUS) {
        gMapRoomState->battleId = GetRandom() % 3 + 131;
    } else {
        gMapRoomState->battleId = GetRandomBattleId();
    }
}

void MapEnmCheckContact(MapEnmWork* work) {
    if (work->collider.colliding) {
        if ((gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) == 0 && ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
            MapEnmStartBattle(work);
            return;
        }

        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_SOLID)) {
            work->obj.fieldPosition.x += work->collider.pushX;
            work->obj.fieldPosition.y += work->collider.pushY;
        }
    }
}

s32 MapEnmCheckAttacked(MapEnmWork* work) {
    if (IsHitByMapAttack(&work->obj.fieldPosition, work->radius / 2, work->height / 2)) {
        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        gMapRoomState->flags |= ROOM_FLAG_ENEMY_STRUCK;
        TaskCreate(&work->tasks, &gTaskDescMapSpark, &work->obj);

        if (gGameState.flags & GAME_FLAG_RIKU) {
            m4aSongNumStart(SONG_SND_228);
        } else {
            m4aSongNumStart(SONG_SYS_FIELD_ATT00);
        }

        return TRUE;
    }

    return FALSE;
}

void MapEnmSaveToCache(MapEnmWork* work) {
    MapEnmCache* cache = ListPoolFirstFree(&gGameState.enemyCachePool);

    if (cache != NULL) {
        cache->def = work->def;
        cache->update = work->update;
        cache->pos = work->obj.fieldPosition;
        cache->angle = work->obj.angle;
        cache->speed = work->obj.speed;
        ListPoolActivate(&cache->node, &gGameState.enemyCachePool);
    }
}

void MapEnmRestoreFromCache() {
    MapEnmCache* cache;
    MapEnmArgs args;
    const MapEnmDef* def;
    s32 i;

    cache = ListPoolFirst(&gGameState.enemyCachePool);

    while (cache != NULL) {
        def = cache->def;
        args.def = def;
        args.update = cache->update;
        args.pos = cache->pos;
        args.angle = cache->angle;
        args.speed = cache->speed;
        TaskCreate(&gFieldState->tasks4, def->desc, &args);
        cache = ListPoolNext(&cache->node);
    }

    ListPoolInit(&gGameState.enemyCachePool);

    for (i = 0; i < 3; i++) {
        ListPoolAddFree(&gGameState.enemyCache[i].node, &gGameState.enemyCachePool, &gGameState.enemyCache[i]);
    }
}

void MapEnmSpawnRoomSet() {
    MapEnmArgs args;
    s32 i;

    switch (gMapRoomState->roomType) {
    case ROOM_TYPE_GUARDED_TROVE:
        MapEnmSpawnFixed(&args, 3, MAP_ENM_PLACE_CHEST);
        MapEnmSpawnFixed(&args, 2, MAP_ENM_PLACE_STAIRS);
        break;
    case ROOM_TYPE_SLEEPING_DARKNESS:
        for (i = 0; i < 3; i++) {
            if (gMapFloorState.world == WORLD_ATLANTICA) {
                MapEnmSpawnFixed(&args, 4, MAP_ENM_PLACE_ROOM);
            } else if (GetRandom() % 2) {
                MapEnmSpawnFixed(&args, 0, MAP_ENM_PLACE_ROOM);
            } else {
                MapEnmSpawnFixed(&args, 1, MAP_ENM_PLACE_ROOM);
            }
        }

        break;
    }
}

void MapEnmInitRoom() {
    MapEnmCache* cache;
    MapEnmArgs args;
    const MapEnmDef* def;
    MapFloorRoom* floorRoom;
    s32 i;

    sMapEnmCount = 0;
    sMapEnmTileCount = 0;
    sMapEnmSpawnTimer = 46;

    if (gGameState.fieldResume) {
        cache = ListPoolFirst(&gGameState.enemyCachePool);

        while (cache != NULL) {
            def = cache->def;
            args.def = def;
            args.update = cache->update;
            args.pos = cache->pos;
            args.angle = cache->angle;
            args.speed = cache->speed;
            TaskCreate(&gFieldState->tasks4, def->desc, &args);
            cache = ListPoolNext(&cache->node);
        }

        if (gGameState.flags & GAME_FLAG_MAP_ENEMY_BATTLE) {
            gGameState.flags &= ~GAME_FLAG_MAP_ENEMY_BATTLE;

            if ((gGameState.flags & GAME_FLAG_BATTLE_NOT_WON) == 0) {
                floorRoom = GetMapFloorRoom(gMapFloorState.room);

                if (floorRoom->enemiesLeft != 0) {
                    floorRoom->enemiesLeft--;
                }
            }
        }
    } else {
        MapEnmSpawnRoomSet();
    }

    ListPoolInit(&gGameState.enemyCachePool);

    for (i = 0; i < 3; i++) {
        ListPoolAddFree(&gGameState.enemyCache[i].node, &gGameState.enemyCachePool, &gGameState.enemyCache[i]);
    }
}

void MapEnmUpdateSpawner() {
    const u8* shape;
    MapFloorRoom* floorRoom;
    const MapEnmDef* def;
    MapEnmArgs args;

    shape = gMapRoomShapes[gMapRoomState->roomType];
    gFieldState->flags &= ~FIELD_FLAG_ENEMY_FRAME_CHANGED;

    if (sMapEnmSpawnTimer != 0) {
        sMapEnmSpawnTimer--;
        return;
    }

    floorRoom = GetMapFloorRoom(gMapFloorState.room);

    if (sMapEnmCount >= shape[1]) {
        return;
    }

    if (floorRoom->enemiesLeft - sMapEnmCount <= 0) {
        return;
    }

    if (gFieldState->flags & (FIELD_FLAG_FREEZE_ENEMIES | FIELD_FLAG_NO_ENEMY_SPAWN | FIELD_FLAG_ROOM_CREATE)) {
        return;
    }

    if (GetRandom() % 10000 <= 7999) {
        return;
    }

    switch (gMapRoomState->roomType) {
    case ROOM_TYPE_WHITE_ROOM:
        def = gMapEnmDefs[5];
        break;
    case ROOM_TYPE_BLACK_ROOM:
        def = gMapEnmDefs[6];
        break;
    default:
        if (gMapFloorState.world == WORLD_ATLANTICA) {
            def = gMapEnmDefs[4];
        } else if (GetRandom() % 3) {
            def = gMapEnmDefs[0];
        } else {
            def = gMapEnmDefs[1];
        }

        break;
    }

    if (sMapEnmTileCount + def->tileCount > 256) {
        return;
    }

    sMapEnmSpawnTimer = 30;
    MapEnmSetupArgs(&args, def);
    TaskCreate(&gFieldState->tasks4, def->desc, &args);
}

void MapEnmInit(MapEnmWork* work, MapEnmArgs* arg) {
    FldObj* obj = &work->obj;
    const MapEnmDef* def = arg->def;

    work->def = def;
    work->update = arg->update;
    work->flags = 0;
    work->colliderDelay = 30;
    obj->fieldPosition = arg->pos;
    obj->angle = arg->angle;
    obj->speed = arg->speed;
    obj->height = def->height;
    obj->unk_34 = 0;
    obj->kind = FLD_OBJ_KIND_ENEMY;
    work->radius = def->radius;
    work->height = def->height;
    work->timer = 0;
    work->unk_D2 = 0;
    work->targetX = obj->fieldPosition.x;
    work->targetY = obj->fieldPosition.y;
    work->targetZ = obj->fieldPosition.z;
    sMapEnmCount++;
    sMapEnmTileCount += def->tileCount;
    work->tiles = AllocObjTiles(def->tileCount * 32, NULL);
    work->palette = LoadObjPalette(def->palette, 32);
    work->gfx = NULL;
    AnimInit(&work->anim, NULL, NULL);
    TaskPoolInit(&work->tasks, 2);

    if ((def->flags & MAP_ENM_DEF_FLAG_NO_SHADOW) == 0) {
        TaskCreate(&work->tasks, &gTaskDescFldShadow, obj);
    }

    if (def->flags & MAP_ENM_DEF_FLAG_GUARD) {
        work->flags |= MAP_ENM_FLAG_PERSISTENT;
        ColliderInit(&work->collider, COLLIDER_TYPE_ENEMY_NO_ENEMY_COLLISION, def->radius, def->height);
    } else {
        ColliderInit(&work->collider, COLLIDER_TYPE_ENEMY, def->radius, def->height);
    }

    ColliderSetPosition(&work->collider, obj->fieldPosition.x, obj->fieldPosition.y, obj->fieldPosition.z);
    ColliderSetDisabled(&work->collider, TRUE);
    MapEnmApplyRoomFlags(work);
}

void MapEnmDraw(MapEnmWork* work) {
    FldObj* obj = &work->obj;
    u16 flags;
    u16 priority;
    s32 pixelY;
    s32 x;
    s32 y;
    s32 z;
    s32 drawFlags;

    if (work->gfx == NULL) {
        return;
    }

    drawFlags = work->flags & MAP_ENM_FLAG_HFLIP;
    flags = SPRITE_PRIORITY(2);

    if (drawFlags) {
        flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
    }

    pixelY = obj->fieldPosition.y >> 8;
    priority = -0x1004 - pixelY * 4;
    obj->shadowZ = obj->fieldPosition.ground;
    obj->shadowPriority = priority + 1;
    z = 0;
    x = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    drawFlags = flags;
    y = pixelY + (obj->fieldPosition.z >> 8) - (gFieldState->y >> 8);
#ifdef PLATFORM_PC
    if (work->unk_D2 != 0) {
        /* fading out (GBA logic, Improved): semi-transparent sprite over the background */
        s32 eva = 16 - work->unk_D2 * 16 / 24;

        if (eva < 0) {
            eva = 0;
        }

        drawFlags |= SPRITE_FLAG_BLEND;
        gBldCnt = BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_BD;
        gBldAlpha = BLDALPHA_BLEND(eva, 16 - eva);
    }
#endif
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, drawFlags, priority);
    TaskPoolDraw(&work->tasks);
}

void MapEnmDestroy(MapEnmWork* work) {
    MapEnmCache* cache;

    if (gGameState.fieldResume && (work->flags & MAP_ENM_FLAG_REMOVED) == 0 &&
        ((gMapRoomState->flags & ROOM_FLAG_START_BATTLE) == 0 || (work->flags & MAP_ENM_FLAG_PERSISTENT))) {
        cache = ListPoolFirstFree(&gGameState.enemyCachePool);

        if (cache != NULL) {
            cache->def = work->def;
            cache->update = work->update;
            cache->pos = work->obj.fieldPosition;
            cache->angle = work->obj.angle;
            cache->speed = work->obj.speed;
            ListPoolActivate(&cache->node, &gGameState.enemyCachePool);
        }
    }

    sMapEnmCount--;
    sMapEnmTileCount -= work->def->tileCount;
    ColliderUnregister(&work->collider);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}

u8 GetRandomMapGmkIndex(u8 slot) {
    u8 index;

    switch (gMapFloorState.world) {
    case WORLD_AGRABAH:
        index = GetRandom() % 12 + 2;
        break;
    case WORLD_ATLANTICA:
        index = GetRandom() % 2;
        break;
    case WORLD_WONDERLAND:
        index = GetRandom() % 3 + 14;
        break;
    case WORLD_MONSTRO:
        index = GetRandom() % 5 + 17;
        break;
    case WORLD_OLYMPUS_COLISEUM:
        index = GetRandom() % 5 + 35;
        break;
    case WORLD_HOLLOW_BASTION:
        index = GetRandom() % 5 + 40;
        break;
    case WORLD_NEVER_LAND:
        index = GetRandom() % 7 + 45;
        break;
    case WORLD_DESTINY_ISLANDS:
        index = GetRandom() % 5 + 52;
        break;
    case WORLD_TRAVERSE_TOWN:
        index = GetRandom() % 2 + 57;
        break;
    case WORLD_CASTLE_OBLIVION:
        index = GetRandom() % 2 + 63;
        break;
    case WORLD_HALLOWEEN_TOWN:
        if (slot <= 7) {
            switch (GetRandom() % 5) {
            case 2:
            case 3:
                index = GetRandom() % 3 != 0 ? 31 : 34;
                break;
            case 0:
            case 1:
                index = GetRandom() % 3 != 0 ? 30 : 33;
                break;
            default:
                index = GetRandom() % 3 != 0 ? 29 : 32;
                break;
            }
        } else {
            index = GetRandom() % 7 + 22;
        }

        break;
    default:
        index = GetRandom() % 3 + 59;
        break;
    }

    return index;
}

MapCell* MapCellAtPos(s32 x, s32 y) {
    u16 cellX = x / 0x2000;
    u16 cellY = y / 0x1000;
    return MapCellAt(cellX, cellY);
}

s32 MapGmkIsAreaSparse(s16 x, s16 y) {
    s32 i;
    u8 n;
    s16 cx;
    s16 cy;

    n = 0;

    for (i = 0; i < sMapGmkCount; i++) {
        cx = (gMapGmkPlacements[i].pos.x >> 8) / 32;
        cy = ((gMapGmkPlacements[i].pos.y + gMapGmkPlacements[i].pos.z) >> 8) / 16;

        if (cx > x - 9 && cx < x + 9 && cy > y - 11 && cy < y + 11) {
            n++;

            if (n > 2) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

u8 MapCellIsFreeOfType(s16 x, s16 y, u8 type) {
    MapCell* cell = MapCellAt(x, y);

    if (cell != NULL && cell->lowerZ != MAP_Z_BOTTOM && cell->type == type && (cell->flags & (MAP_CELL_FLAG_STAIRS | MAP_CELL_FLAG_JUMP_PAD | MAP_CELL_FLAG_GMK_RESERVED | MAP_CELL_FLAG_KEEP_CLEAR)) == 0) {
        return TRUE;
    }

    return FALSE;
}

s32 MapAreaIsFreeOfType(s16 x, s16 y, u8 w, u8 h, u8 type) {
    s32 i;
    s32 j;

    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            if (!MapCellIsFreeOfType(x + i, y + j, type)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

s32 MapCellHeightExceeds(s16 x, s16 y, u8 limit) {
    u16 height;
    MapCell* cell = MapCellAt(x, y);
    height = (cell->lowerZ - cell->upperZ) >> 8;
    return height > (limit << 4);
}

void MapReserveArea(s16 x, s16 y, u8 w, u8 h) {
    s32 i;
    s32 j;

    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            MapCellAt(x + i, y + j)->flags |= MAP_CELL_FLAG_GMK_RESERVED;
        }
    }
}

s16 MapRowsToWallBase(s16 x, s16 y) {
    u16 n = gMapRoomState->rows - y;
    s32 i;

    for (i = 0; i < n; i++) {
        MapCell* cell = MapCellAt(x, y + i);

        if (cell->flags & MAP_CELL_FLAG_GMK_RESERVED) {
            return 0;
        }

        if (cell->type == MAP_CELL_TYPE_FLOOR || cell->type == MAP_CELL_TYPE_LEFT_WALL_BASE ||
            cell->type == MAP_CELL_TYPE_BACK_WALL_BASE || cell->type == MAP_CELL_TYPE_RIGHT_WALL_BASE) {
            return i;
        }
    }

    return 0;
}

s32 MapWallFaceIsUnreserved(s16 x, s16 y, u16 width) {
    s32 i;
    u16 height;
    s32 j;
    MapCell* cell;
    s32 mask;

    height = gMapRoomState->rows - y;

    for (j = 0; j < width; j++) {
        for (i = 0; i < height; i++) {
            // @bug The upward scan can pass row zero: the GBA reads BIOS at NULL.
            cell = BIOS_PTR(MapCellAt(x + j, y - i));

            if (cell->flags & MAP_CELL_FLAG_GMK_RESERVED) {
                return FALSE;
            }

            if (cell->type < MAP_CELL_TYPE_BACK_WALL_FACE || cell->type > MAP_CELL_TYPE_RIGHT_WALL_FACE) {
                break;
            }
        }
    }

    return TRUE;
}

u8 MapGmkFindFloor2x2(FldPos* pos) {
    u16 w = gMapRoomState->cols - 2;
    u16 h = gMapRoomState->bottomRow - gMapRoomState->topRow - 2;
    s16 rx = GetRandom() % w;
    s16 ry = GetRandom() % h;
    s32 i;
    s32 j;

    for (j = 0; j < h; j++) {
        s16 sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && (u8)MapAreaIsFreeOfType(rx, sy, 2, 2, MAP_CELL_TYPE_FLOOR)) {
                MapReserveArea(rx, sy, 2, 2);
                FldPosPlaceAtCell(pos, rx, sy, 2, 2);
                return 1;
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindLeftWallBase2x3(FldPos* pos) {
    s16 ry;
    s16 rx;
    s16 sy;
    u16 h;
    u16 w;
    s32 i;
    s32 j;

    w = gMapRoomState->cols - 2;
    h = gMapRoomState->bottomRow - gMapRoomState->topRow - 3;
    rx = GetRandom() % w;
    ry = GetRandom() % h;

    for (j = 0; j < h; j++) {
        sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && MapCellIsFreeOfType(rx, sy, MAP_CELL_TYPE_LEFT_WALL_FACE)) {
                s32 y1 = (s16)(sy + 1);

                if (MapCellIsFreeOfType(rx, y1, MAP_CELL_TYPE_LEFT_WALL_BASE)) {
                    s32 y2 = (s16)(sy + 2);

                    if (MapCellIsFreeOfType(rx, y2, MAP_CELL_TYPE_FLOOR)) {
                        s32 x1 = (s16)(rx + 1);

                        if (MapCellIsFreeOfType(x1, sy, MAP_CELL_TYPE_LEFT_WALL_BASE) &&
                            MapCellIsFreeOfType(x1, y1, MAP_CELL_TYPE_FLOOR) &&
                            MapCellIsFreeOfType(x1, y2, MAP_CELL_TYPE_FLOOR) && (u8)MapCellHeightExceeds(rx, sy, 3)) {
                            MapReserveArea(rx, sy, 2, 3);
                            FldPosPlaceAtCell(pos, rx, sy, 2, 3);
                            return 1;
                        }
                    }
                }
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindLeftWallBase1x4(FldPos* pos) {
    s16 x;
    s16 y;
    s16 cy;
    u16 colRange;
    u16 rowRange;
    s32 i;
    s32 j;

    colRange = gMapRoomState->cols - 1;
    rowRange = gMapRoomState->bottomRow - gMapRoomState->topRow - 4;
    x = GetRandom() % colRange;
    y = GetRandom() % rowRange;

    for (i = 0; i < rowRange; i++) {
        cy = gMapRoomState->topRow + y;

        for (j = 0; j < colRange; j++) {
            if ((u8)MapGmkIsAreaSparse(x, cy)) {
                if (MapCellIsFreeOfType(x, cy, MAP_CELL_TYPE_LEFT_WALL_BASE)) {
                    if (MapCellIsFreeOfType(x, cy + 1, MAP_CELL_TYPE_FLOOR)) {
                        if (MapCellIsFreeOfType(x, cy + 2, MAP_CELL_TYPE_FLOOR)) {
                            if (MapCellIsFreeOfType(x, cy + 3, MAP_CELL_TYPE_FLOOR)) {
                                if ((u8)MapCellHeightExceeds(x, cy, 3)) {
                                    MapReserveArea(x, cy, 1, 4);
                                    FldPosPlaceAtCell(pos, x, cy, 1, 4);
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }

            x++;
            x %= colRange;
        }

        y = (y != 0 ? y : rowRange) - 1;
    }

    return 0;
}

u8 MapGmkFindRightWallBase2x3(FldPos* pos) {
    s16 ry;
    s16 rx;
    s16 sy;
    u16 h;
    u16 w;
    s32 i;
    s32 j;

    w = gMapRoomState->cols - 2;
    h = gMapRoomState->bottomRow - gMapRoomState->topRow - 3;
    rx = GetRandom() % w;
    ry = GetRandom() % h;

    for (j = 0; j < h; j++) {
        sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && MapCellIsFreeOfType(rx, sy, MAP_CELL_TYPE_RIGHT_WALL_BASE)) {
                s32 y1 = (s16)(sy + 1);

                if (MapCellIsFreeOfType(rx, y1, MAP_CELL_TYPE_FLOOR)) {
                    s32 y2 = (s16)(sy + 2);

                    if (MapCellIsFreeOfType(rx, y2, MAP_CELL_TYPE_FLOOR)) {
                        s32 x1 = (s16)(rx + 1);

                        if (MapCellIsFreeOfType(x1, sy, MAP_CELL_TYPE_RIGHT_WALL_FACE) &&
                            MapCellIsFreeOfType(x1, y1, MAP_CELL_TYPE_RIGHT_WALL_BASE) &&
                            MapCellIsFreeOfType(x1, y2, MAP_CELL_TYPE_FLOOR) && (u8)MapCellHeightExceeds(rx, sy, 3)) {
                            MapReserveArea(rx, sy, 2, 3);
                            FldPosPlaceAtCell(pos, rx, sy, 2, 3);
                            return 1;
                        }
                    }
                }
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindLeftWallFace3x3(FldPos* pos) {
    s16 ry;
    s16 rx;
    s16 sy;
    u16 h;
    u16 w;
    s32 i;
    s32 j;

    w = gMapRoomState->cols - 3;
    h = gMapRoomState->bottomRow - gMapRoomState->topRow - 3;
    rx = GetRandom() % w;
    ry = GetRandom() % h;

    for (j = 0; j < h; j++) {
        sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && (u8)MapAreaIsFreeOfType(rx, sy, 3, 3, MAP_CELL_TYPE_LEFT_WALL_FACE)) {
                u16 leftRows;
                u16 rightRows;

                if (!(u8)MapWallFaceIsUnreserved(rx, sy, 3)) {
                    continue;
                }

                leftRows = MapRowsToWallBase(rx, sy + 2);
                rightRows = MapRowsToWallBase(rx + 2, sy);

                if (leftRows == rightRows && leftRows > 8) {
                    MapReserveArea(rx, sy, 3, 3);
                    FldPosPlaceAtCell(pos, rx, leftRows + sy, 3, 3);
                    pos->z -= leftRows << 12;
                    return 1;
                }
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindRightWallFace3x3(FldPos* pos) {
    s16 ry;
    s16 rx;
    s16 sy;
    u16 h;
    u16 w;
    s32 i;
    s32 j;

    w = gMapRoomState->cols - 3;
    h = gMapRoomState->bottomRow - gMapRoomState->topRow - 3;
    rx = GetRandom() % w;
    ry = GetRandom() % h;

    for (j = 0; j < h; j++) {
        sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && (u8)MapAreaIsFreeOfType(rx, sy, 3, 3, MAP_CELL_TYPE_RIGHT_WALL_FACE)) {
                u16 leftRows;
                u16 rightRows;

                if (!(u8)MapWallFaceIsUnreserved(rx, sy, 3)) {
                    continue;
                }

                leftRows = MapRowsToWallBase(rx, sy);
                rightRows = MapRowsToWallBase(rx + 2, sy + 2);

                if (leftRows == rightRows && leftRows > 8) {
                    MapReserveArea(rx, sy, 3, 3);
                    FldPosPlaceAtCell(pos, rx, leftRows + sy, 3, 3);
                    pos->z -= leftRows << 12;
                    return 1;
                }
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindLeftWallTop2x2(FldPos* pos) {
    s16 ry;
    s16 rx;
    s16 sy;
    u16 h;
    u16 w;
    s32 i;
    s32 j;

    w = gMapRoomState->cols - 2;
    h = gMapRoomState->bottomRow - gMapRoomState->topRow - 2;
    rx = GetRandom() % w;
    ry = GetRandom() % h;

    for (j = 0; j < h; j++) {
        sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && MapCellIsFreeOfType(rx, sy, MAP_CELL_TYPE_FLOOR)) {
                if (MapCellIsFreeOfType(rx, sy + 1, MAP_CELL_TYPE_LEFT_WALL_TOP)) {
                    if (MapCellIsFreeOfType(rx + 1, sy, MAP_CELL_TYPE_LEFT_WALL_TOP) &&
                        MapCellIsFreeOfType(rx + 1, sy + 1, MAP_CELL_TYPE_LEFT_WALL_FACE)) {
                        u16 leftRows = MapRowsToWallBase(rx, sy + 1);
                        u16 rightRows = MapRowsToWallBase(rx + 1, sy);

                        if (leftRows == rightRows && leftRows > 8) {
                            MapReserveArea(rx, sy, 2, 2);
                            FldPosPlaceAtCell(pos, rx, leftRows + sy, 2, 2);
                            pos->z -= leftRows << 12;
                            return 1;
                        }
                    }
                }
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindBackWallTop1x2(FldPos* pos) {
    s16 x;
    s16 y;
    s16 cy;
    u16 colRange;
    u16 rowRange;
    u16 wallRows;
    s32 i;
    s32 j;

    colRange = gMapRoomState->cols - 1;
    rowRange = gMapRoomState->bottomRow - gMapRoomState->topRow - 2;
    x = GetRandom() % colRange;
    y = GetRandom() % rowRange;

    for (i = 0; i < rowRange; i++) {
        cy = gMapRoomState->topRow + y;

        for (j = 0; j < colRange; j++) {
            if ((u8)MapGmkIsAreaSparse(x, cy)) {
                if (MapCellIsFreeOfType(x, cy, MAP_CELL_TYPE_BACK_WALL_TOP)) {
                    if (MapCellIsFreeOfType(x, cy + 1, MAP_CELL_TYPE_BACK_WALL_FACE)) {
                        wallRows = MapRowsToWallBase(x, cy);

                        if (wallRows > 8) {
                            MapReserveArea(x, cy, 1, 2);
                            FldPosPlaceAtCell(pos, x, wallRows + cy, 1, 2);
                            pos->z -= wallRows << 12;
                            return 1;
                        }
                    }
                }
            }

            x++;
            x %= colRange;
        }

        y = (y != 0 ? y : rowRange) - 1;
    }

    return 0;
}

u8 MapGmkFindRightWallTop2x2(FldPos* pos) {
    s16 ry;
    s16 rx;
    s16 sy;
    u16 h;
    u16 w;
    s32 i;
    s32 j;

    w = gMapRoomState->cols - 2;
    h = gMapRoomState->bottomRow - gMapRoomState->topRow - 2;
    rx = GetRandom() % w;
    ry = GetRandom() % h;

    for (j = 0; j < h; j++) {
        sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && MapCellIsFreeOfType(rx, sy, MAP_CELL_TYPE_RIGHT_WALL_TOP)) {
                s32 y1 = (s16)(sy + 1);

                if (MapCellIsFreeOfType(rx, y1, MAP_CELL_TYPE_RIGHT_WALL_FACE)) {
                    if (MapCellIsFreeOfType(rx + 1, sy, MAP_CELL_TYPE_FLOOR) &&
                        MapCellIsFreeOfType(rx + 1, y1, MAP_CELL_TYPE_RIGHT_WALL_TOP)) {
                        u16 leftRows = MapRowsToWallBase(rx, sy);
                        u16 rightRows = MapRowsToWallBase(rx + 1, y1);

                        if (leftRows == rightRows && leftRows > 8) {
                            MapReserveArea(rx, sy, 2, 2);
                            FldPosPlaceAtCell(pos, rx, leftRows + sy, 2, 2);
                            pos->z -= leftRows << 12;
                            return 1;
                        }
                    }
                }
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindBackWallBase2x3(FldPos* pos) {
    s16 ry;
    s16 rx;
    s16 sy;
    u16 h;
    u16 w;
    s32 i;
    s32 j;

    w = gMapRoomState->cols - 2;
    h = gMapRoomState->bottomRow - gMapRoomState->topRow - 3;
    rx = GetRandom() % w;
    ry = GetRandom() % h;

    for (j = 0; j < h; j++) {
        sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && MapCellIsFreeOfType(rx, sy, MAP_CELL_TYPE_BACK_WALL_BASE)) {
                s32 y1 = (s16)(sy + 1);

                if (MapCellIsFreeOfType(rx, y1, MAP_CELL_TYPE_FLOOR)) {
                    s32 y2 = (s16)(sy + 2);

                    if (MapCellIsFreeOfType(rx, y2, MAP_CELL_TYPE_FLOOR)) {
                        s32 x1 = (s16)(rx + 1);

                        if (MapCellIsFreeOfType(x1, sy, MAP_CELL_TYPE_BACK_WALL_BASE) &&
                            MapCellIsFreeOfType(x1, y1, MAP_CELL_TYPE_FLOOR) &&
                            MapCellIsFreeOfType(x1, y2, MAP_CELL_TYPE_FLOOR) && (u8)MapCellHeightExceeds(rx, sy, 3)) {
                            MapReserveArea(rx, sy, 2, 3);
                            FldPosPlaceAtCell(pos, rx, sy, 2, 3);
                            return 1;
                        }
                    }
                }
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindFloor3x3(FldPos* pos) {
    u16 w = gMapRoomState->cols - 3;
    u16 h = gMapRoomState->bottomRow - gMapRoomState->topRow - 3;
    s16 rx = GetRandom() % w;
    s16 ry = GetRandom() % h;
    s32 i;
    s32 j;

    for (j = 0; j < h; j++) {
        s16 sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && (u8)MapAreaIsFreeOfType(rx, sy, 3, 3, MAP_CELL_TYPE_FLOOR)) {
                MapReserveArea(rx, sy, 3, 3);
                FldPosPlaceAtCell(pos, rx, sy, 3, 3);
                return 1;
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindFloor4x4(FldPos* pos) {
    u16 w = gMapRoomState->cols - 4;
    u16 h = gMapRoomState->bottomRow - gMapRoomState->topRow - 4;
    s16 rx = GetRandom() % w;
    s16 ry = GetRandom() % h;
    s32 i;
    s32 j;

    for (j = 0; j < h; j++) {
        s16 sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && (u8)MapAreaIsFreeOfType(rx, sy, 4, 4, MAP_CELL_TYPE_FLOOR)) {
                MapReserveArea(rx, sy, 4, 4);
                FldPosPlaceAtCell(pos, rx, sy, 4, 4);
                return 1;
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindFloor5x5(FldPos* pos) {
    u16 w = gMapRoomState->cols - 5;
    u16 h = gMapRoomState->bottomRow - gMapRoomState->topRow - 5;
    s16 rx = GetRandom() % w;
    s16 ry = GetRandom() % h;
    s32 i;
    s32 j;

    for (j = 0; j < h; j++) {
        s16 sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            if ((u8)MapGmkIsAreaSparse(rx, sy) && (u8)MapAreaIsFreeOfType(rx, sy, 5, 5, MAP_CELL_TYPE_FLOOR)) {
                MapReserveArea(rx, sy, 5, 5);
                FldPosPlaceAtCell(pos, rx, sy, 5, 5);
                return 1;
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (ry != 0 ? ry : h) - 1;
    }

    return 0;
}

u8 MapGmkFindBaseFloor2x2(FldPos* pos) {
    MapPlatform* platform = GetMapPlatform(0);
    u16 w = platform->right - platform->left - 2;
    u16 h = gMapRoomState->bottomRow - gMapRoomState->topRow - 2;
    s16 rx = GetRandom() % w;
    s16 ry = GetRandom() % h;
    s32 i;
    s32 j;

    for (j = 0; j < h; j++) {
        s16 sy = gMapRoomState->topRow + ry;

        for (i = 0; i < w; i++) {
            s32 sx = (s16)(platform->left + rx);

            if ((u8)MapAreaIsFreeOfType(sx, sy, 2, 2, MAP_CELL_TYPE_FLOOR) && platform->z == MapCellAt(sx, sy)->lowerZ) {
                MapReserveArea(sx, sy, 2, 2);
                FldPosPlaceAtCell(pos, sx, sy, 2, 2);
                return 1;
            }

            rx = (s16)(rx + 1) % w;
        }

        ry = (s16)(ry + 1) % h;
    }

    return 0;
}

u8 MapGmkFindSpot(FldPos* pos, u8 spotFinder) {
    return gMapGmkSpotFuncs[spotFinder](pos);
}

s32 MapGmkIsPaletteUnused(void* palette) {
    s32 i;

    for (i = 0; i < sMapGmkCount; i++) {
        if (gMapGmkPlacements[i].def->palette == palette) {
            return FALSE;
        }
    }

    return TRUE;
}

s32 MapGmkNeedsTiles(u8 ownTiles, const void* tiles) {
    s32 i;

    if (ownTiles) {
        return TRUE;
    }

    for (i = 0; i < sMapGmkCount; i++) {
        if (gMapGmkPlacements[i].def->tiles == tiles) {
            return FALSE;
        }
    }

    return TRUE;
}

void MapGmkReserveJump() {
    s32 i;

    for (i = 0; i < 12; i++) {
        MapPlatform* platform = GetMapPlatform(i);

        if (!platform->hasStairs && platform->spotLowerZ != MAP_Z_BOTTOM) {
            sMapGmkTileCount += 0x4C;
            sMapGmkPaletteCount++;
            break;
        }
    }
}

void MapGmkPlaceGmk01() {
    FldPos pos;
    MapFloorRoom* floorRoom;
    const MapGmkDef* def;
    s32 i;

    floorRoom = GetMapFloorRoom(gMapFloorState.room);

    if (gMapRoomState->roomType == ROOM_TYPE_FALSE_BOUNTY) {
        u8* count;

        def = &gMapGmk01Def;
        count = &sMapGmkCount;

        for (i = 1; i >= 0; i--) {
            s32 size;

            MapGmkFindSpot(&pos, def->spotFinder);

            if (floorRoom->flags & FLOOR_ROOM_FLAG_CHEST_OPENED) {
                gMapGmkPlacements[*count].flags = (GMK_FLAG_USED | GMK_FLAG_HAS_ENEMY);
            } else {
                gMapGmkPlacements[*count].flags = GMK_FLAG_HAS_ENEMY;
            }

            gMapGmkPlacements[*count].def = def;
            gMapGmkPlacements[*count].pos = pos;
            sMapGmkTileCount += (size = def->tilesSize) / 32;
            sMapGmkPaletteCount++;
            (*count)++;
        }
    }

    if (gMapRoomState->roomType == ROOM_TYPE_GUARDED_TROVE || gMapRoomState->roomType == ROOM_TYPE_CALM_BOUNTY ||
        gMapRoomState->roomType == ROOM_TYPE_FALSE_BOUNTY || gMapRoomState->roomType == ROOM_TYPE_HIDDEN_CHAMBER) {
        MapCell* cell;
        u16 opened;

        def = &gMapGmk01Def;
        MapGmkFindSpot(&pos, def->spotFinder);
        opened = floorRoom->flags & FLOOR_ROOM_FLAG_CHEST_OPENED;

        if (opened != 0) {
            gMapGmkPlacements[sMapGmkCount].flags = GMK_FLAG_USED;
        } else {
            gMapGmkPlacements[sMapGmkCount].flags = 0;
        }

        gMapGmkPlacements[sMapGmkCount].def = def;
        gMapGmkPlacements[sMapGmkCount].pos = pos;
        sMapGmkTileCount += def->tilesSize >> 5;
        sMapGmkPaletteCount++;
        sMapGmkCount++;
        cell = MapCellAtPos(pos.x, pos.y + pos.z);
        cell->flags |= MAP_CELL_FLAG_CHEST;
    }
}

void MapGmkPlaceGmk04() {
    FldPos pos;

    if (gMapRoomState->roomType == ROOM_TYPE_MOMENTS_REPRIEVE || gMapRoomState->roomType == ROOM_TYPE_EXIT_ROOM) {
        MapGmkFindSpot(&pos, gMapGmk04Def.spotFinder);
        gMapGmkPlacements[sMapGmkCount].flags = 0;
        gMapGmkPlacements[sMapGmkCount].def = &gMapGmk04Def;
        gMapGmkPlacements[sMapGmkCount].pos = pos;
        sMapGmkTileCount += gMapGmk04Def.tilesSize >> 5;
        sMapGmkPaletteCount++;
        sMapGmkCount++;
    }
}

void MapGmkPlaceMoogle() {
    FldPos pos;

    if (gMapRoomState->roomType == ROOM_TYPE_MOOGLE_ROOM) {
        MapGmkFindSpot(&pos, gMapGmk05Def.spotFinder);
        gMapGmkPlacements[sMapGmkCount].flags = 0;
        gMapGmkPlacements[sMapGmkCount].def = &gMapGmk05Def;
        gMapGmkPlacements[sMapGmkCount].pos = pos;
        sMapGmkTileCount += gMapGmk05Def.tilesSize >> 5;
        sMapGmkPaletteCount++;
        sMapGmkCount++;
    }
}

void MapGmkPlaceWorldGimmicks() {
    FldPos pos;
    const MapGmkDef* def;
    s32 i;
    s32 newPalette;

    if (gMapRoomState->roomType == ROOM_TYPE_MOMENTS_REPRIEVE || gMapRoomState->roomType == ROOM_TYPE_CALM_BOUNTY ||
        gMapRoomState->roomType == ROOM_TYPE_MOOGLE_ROOM || gMapRoomState->roomType == ROOM_TYPE_HIDDEN_CHAMBER ||
        gMapRoomState->roomType == ROOM_TYPE_EXIT_ROOM) {
        return;
    }

    for (i = sMapGmkCount; i <= 15; i++) {
        switch (gMapFloorState.world) {
        case WORLD_TRAVERSE_TOWN:
            def = &gWorldMapGmkDefs[0];
            break;
        case WORLD_WONDERLAND:
            def = &gWorldMapGmkDefs[1];
            break;
        case WORLD_ATLANTICA:
            def = &gWorldMapGmkDefs[2];
            break;
        case WORLD_HALLOWEEN_TOWN:
            def = &gWorldMapGmkDefs[3];
            break;
        case WORLD_HOLLOW_BASTION:
            def = GetRandom() % 2 ? &gWorldMapGmkDefs[4] : &gWorldMapGmkDefs[5];
            break;
        case WORLD_CASTLE_OBLIVION:
            def = &gWorldMapGmkDefs[6];
            break;
        default:
            def = &gMapGmkBarrelDef;
            break;
        }

        if (sMapGmkTileCount + def->tilesSize / 32 > 0x200) {
            return;
        }

        newPalette = (u8)MapGmkIsPaletteUnused(def->palette);

        if (newPalette && sMapGmkPaletteCount > 5) {
            return;
        }

        if (MapGmkFindSpot(&pos, def->spotFinder) == 0) {
            return;
        }

        gMapGmkPlacements[sMapGmkCount].flags = 0;
        gMapGmkPlacements[sMapGmkCount].def = def;
        gMapGmkPlacements[sMapGmkCount].pos = pos;
        sMapGmkTileCount += def->tilesSize >> 5;
        sMapGmkCount++;

        if (newPalette) {
            sMapGmkPaletteCount++;
        }
    }
}

void MapGmkPlaceRandomGimmicks() {
    s32 i;

    for (i = sMapGmkCount; i < 16; i++) {
        FldPos pos;
        const MapGmkDef* def = &gMapGmkDefs[GetRandomMapGmkIndex(i)];
        u8 needsTiles = MapGmkNeedsTiles(def->ownTiles, def->tiles);
        u8 newPalette;

        if (needsTiles) {
            if ((def->tilesSize >> 5) + sMapGmkTileCount > 512) {
                continue;
            }
        }

        newPalette = MapGmkIsPaletteUnused(def->palette);

        if (newPalette) {
            if (sMapGmkPaletteCount > 5) {
                continue;
            }
        }

        if (MapGmkFindSpot(&pos, def->spotFinder) == 0) {
            continue;
        }

        gMapGmkPlacements[sMapGmkCount].flags = 0;
        gMapGmkPlacements[sMapGmkCount].def = def;
        gMapGmkPlacements[sMapGmkCount].pos = pos;
        sMapGmkCount++;

        if (needsTiles) {
            sMapGmkTileCount += def->tilesSize >> 5;
        }

        if (newPalette) {
            sMapGmkPaletteCount++;
        }
    }
}

u8 FldObjIsOutOfView(FldObj* obj) {
    s32 lim = gFieldState->actor.fieldPosition.y + gFieldState->actor.fieldPosition.z + 0x4000 + (obj->height << 8);

    if (obj->fieldPosition.x < gFieldState->x || obj->fieldPosition.x > gFieldState->x + 0xF000 ||
        obj->fieldPosition.y + obj->fieldPosition.z < gFieldState->y || obj->fieldPosition.y + obj->fieldPosition.z > lim) {
        return TRUE;
    }

    return FALSE;
}

u16 MapGmkGetFreeTiles() {
    return 512 - sMapGmkTileCount;
}

void CreateRandomMapPrizes(s32 x, s32 y, s32 z) {
    u16 roll;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        roll = GetRandom() % 10000;

        if (roll < 2500) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_SMALL, 2, x, y, z);
        } else if (roll < 6500) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_SMALL, 5, x, y, z);
        } else if (roll < 9000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_LARGE, 3, x, y, z);
        } else {
            CreateMapPrizeTasks(MAP_PRIZE_HP_LARGE, 5, x, y, z);
        }
    } else {
        roll = GetRandom() % 10000;

        if (roll < 2000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_SMALL, 2, x, y, z);
        } else if (roll < 4000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_SMALL, 5, x, y, z);
        } else if (roll < 6000) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_LARGE, 3, x, y, z);
        } else if (roll < 6500) {
            CreateMapPrizeTasks(MAP_PRIZE_HP_LARGE, 5, x, y, z);
        } else if (roll < 8000) {
            CreateMapPrizeTasks(MAP_PRIZE_MOOGLE_POINTS_SMALL, 5, x, y, z);
        } else {
            CreateMapPrizeTasks(MAP_PRIZE_MOOGLE_POINTS_LARGE, 5, x, y, z);
        }
    }
}

void DropMapGmkPrize(FldPos* pos) {
    u16 roll = GetRandom() % 10000;

    if (roll <= 0x5DB) {
        if (TryCreateRandomPrzCard(FALSE, pos->x, pos->y, pos->z) != TRUE) {
            CreateRandomMapPrizes(pos->x, pos->y, pos->z);
        }
    } else if (roll <= 0x1D4B) {
        CreateRandomMapPrizes(pos->x, pos->y, pos->z);
    }
}

void MapGmkInitRoom() {
    if (!gGameState.fieldResume) {
        gMapGmkPlacements = EwramAlloc(sizeof(MapGmkPlacement) * 16);
        sMapGmkCount = 0;
        sMapGmkPaletteCount = 0;
        sMapGmkTileCount = 0;
        MapGmkReserveJump();
        MapGmkPlaceGmk01();
        MapGmkPlaceGmk04();
        MapGmkPlaceMoogle();
        MapGmkPlaceRandomGimmicks();
        MapGmkPlaceWorldGimmicks();
    }

    MapGmkCreateTasks();
}

void MapGmkCreateTasks() {
    s32 i;
    MapPlatform* platform;
    const MapGmkDef* def;

    for (i = 0; i < 12; i++) {
        platform = GetMapPlatform(i);

        if (!platform->hasStairs && platform->spotLowerZ != MAP_Z_BOTTOM) {
            TaskCreate(&gFieldState->tasks, &gTaskDescMapGmkJump, platform);
        }
    }

    for (i = 0; i < sMapGmkCount; i++) {
        def = gMapGmkPlacements[i].def;

        if ((gMapGmkPlacements[i].flags & GMK_FLAG_DESTROYED) == 0) {
            TaskCreate(&gFieldState->tasks, def->desc, &gMapGmkPlacements[i]);
        }
    }

    TaskCreate(&gFieldState->tasks, &gTaskDescMapGmkDmy, NULL);
}

void MapGmkFree() {
    if (!gGameState.fieldResume) {
        EwramFree(gMapGmkPlacements);
    }
}

const u8* GetCellMaskBlock(void* maskTable, u16 x, u16 y) {
    u8* blockIds = maskTable;

    return gCellMasks[blockIds[(u8)(x >> 3) + (u8)(y >> 3) * 4]];
}

void* GetCellMaskTable(u8 type) {
    s32 i;

    switch (type) {
    case MAP_CELL_TYPE_BACK_WALL_TOP:
    case MAP_CELL_TYPE_BACK_WALL_BASE:
    case MAP_CELL_TYPE_BACK_WALL_FACE:
    case MAP_CELL_TYPE_LEFT_WALL_FACE:
    case MAP_CELL_TYPE_RIGHT_WALL_FACE:
        i = 1;
        break;
    case MAP_CELL_TYPE_LEFT_WALL_TOP:
        i = 2;
        break;
    case MAP_CELL_TYPE_RIGHT_WALL_TOP:
        i = 3;
        break;
    case MAP_CELL_TYPE_LEFT_WALL_BASE:
        i = 4;
        break;
    case MAP_CELL_TYPE_RIGHT_WALL_BASE:
        i = 5;
        break;
    default:
        i = 0;
        break;
    }

    return &gCellMasks[i + 10];
}

u8 MapCellMaskBitAt(MapCell* cell, s32 x, s32 y) {
    u16 cx;
    u16 cy;
    u8 bx;
    u8 by;
    const u8* mask;

    if (cell == NULL) {
        return 1;
    }

    cx = (x >> 8) % 32;
    cy = (y >> 8) % 16;
    mask = GetCellMaskBlock(cell->maskTable, cx, cy);
    bx = cx & 7;
    by = cy & 7;
    return (mask[by] >> (7 - bx)) & 1;
}

void MapPlaceLayer1DecorPiece(s16 x, s16 y, const u8* pieces, u16* base) {
    s32 i;
    s32 j;
    s32 off;
    u8 roll;
    MapCell* cell;

    roll = GetRandom() % 100;

    while (roll >= pieces[0]) {
        pieces += 8;
    }

    for (j = 0; j < pieces[3]; j++) {
        for (i = 0; i < pieces[4]; i++) {
            switch (pieces[5]) {
            case 1:
                cell = MapCellAt(x + j, y + i + (pieces[3] - 1 - j));
                break;
            case 2:
                cell = MapCellAt(x + j, y + i + j);
                break;
            case 0:
            default:
                cell = MapCellAt(x + j, y + i);
                break;
            }

            off = (pieces[2] + i) * 64 + (pieces[1] + j) * 4;
            // @bug cell is NULL where the pattern reaches past the room edge (NULL write).
            BIOS_PTR(cell)->bg2Piece = 50;
            BIOS_PTR(cell)->bg2Map = base + off;
        }
    }
}

u8 MapPatternFits(s16 x, s16 y, const MapCellPattern* pattern) {
    MapCell* cell;
    s32 flags;

    while (pattern->bg3Piece != MAP_PATTERN_END) {
        // @bug MapCellAt returns NULL past the room edge (NULL read).
        cell = BIOS_PTR(MapCellAt(x + pattern->dx, y + pattern->dy));

        if (cell->lowerZ == MAP_Z_BOTTOM) {
            return FALSE;
        }

        if (pattern->bg3Piece != 7 && cell->bg3Piece != pattern->bg3Piece) {
            return FALSE;
        }

        if (cell->bg2Piece != pattern->bg2Piece) {
            return FALSE;
        }

        flags = 0x520;

        if ((cell->flags & flags) != 0) {
            return FALSE;
        }

        flags = cell->flags & (MAP_CELL_FLAG_EDGE_LEFT | MAP_CELL_FLAG_EDGE_RIGHT);
        flags = flags & ~pattern->edgeIgnoreMask;

        if (flags != pattern->edgeFlags) {
            return FALSE;
        }

        pattern++;
    }

    return TRUE;
}

void MapApplyLayer1DecorRule(MapDecorRule* rule) {
    u16 w = gMapRoomState->cols - rule->width + 1;
    u16 h = gMapRoomState->bottomRow - gMapRoomState->topRow + 7;
    s16 y0 = gMapRoomState->topRow - 7;
    s32 i;

    for (i = 0; i < h; i++) {
        s16 y = y0 + i;
        s16 j;

        for (j = 0; j < w; j++) {
            if (MapPatternFits(j, y, (const MapCellPattern*)rule->pattern)) {
                if (GetRandom() % 100 < rule->chance) {
                    MapPlaceLayer1DecorPiece(j, y, rule->pieces, rule->tilemap);
                }
            }
        }
    }
}

void MapApplyLayer1DecorRules(MapDecorRule* rules) {
    if (rules != NULL) {
        while (rules->pattern != NULL) {
            MapApplyLayer1DecorRule(rules);
            rules++;
        }
    }
}

void MapPlaceLayer2DecorPiece(s16 x, s16 y, const u8* pieces, u16* base) {
    s32 i;
    s32 j;
    s32 off;
    u8 roll;
    MapCell* cell;

    roll = GetRandom() % 100;

    while (roll >= pieces[0]) {
        pieces += 8;
    }

    for (j = 0; j < pieces[3]; j++) {
        for (i = 0; i < pieces[4]; i++) {
            cell = MapCellAt(x + j, y + i);
            off = (pieces[2] + i) * 64 + (pieces[1] + j) * 4;
            // @bug cell is NULL where the pattern reaches past the room edge (NULL write).
            BIOS_PTR(cell)->bg1Piece = 38;
            BIOS_PTR(cell)->bg1Map = base + off;
        }
    }
}

u8 MapDecorCheckFits(s16 x, s16 y, const u8* pattern) {
    while (pattern[0] != MAP_PATTERN_END) {
        if (MapCellAt(pattern[0] + x, pattern[1] + y)->bg1Piece != pattern[2]) {
            return FALSE;
        }

        pattern += 4;
    }

    return TRUE;
}

void MapApplyLayer2DecorRule(MapDecorRule* rule) {
    s16 i;
    s16 j;
    u16 w;
    u16 h;

    w = gMapRoomState->cols - rule->width + 1;
    h = gMapRoomState->rows - rule->height + 1;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            if (MapDecorCheckFits(i, j, rule->pattern)) {
                if (GetRandom() % 100 < rule->chance) {
                    MapPlaceLayer2DecorPiece(i, j, rule->pieces, rule->tilemap);
                }
            }
        }
    }
}

void MapApplyLayer2DecorRules(MapDecorRule* rules) {
    if (rules != NULL) {
        while (rules->pattern != NULL) {
            MapApplyLayer2DecorRule(rules);
            rules++;
        }
    }
}

void MapApplyRoomDecor() {
    if (!gGameState.fieldResume) {
        MapRoomDef* roomDef = gMapRoomDefs[gMapFloorState.world];

        MapApplyLayer1DecorRules(roomDef->layer1DecorRules);
        MapApplyLayer2DecorRules(roomDef->layer2DecorRules);
    }
}

void MapAnmSetupSlot(MapAnmSlot* slot, const MapAnmEntry* entry) {
    slot->tiles = entry->tiles;
    slot->frameSize = entry->frameSize;
    slot->dest += entry->tileOffset << 5;
    slot->timer = 0;
    slot->script = entry->script;
    slot->scriptPos = entry->script;
}

void MapAnmStepScript(MapAnmSlot* slot) {
    s16* cmd;
    u8 (*handler)(MapAnmSlot*);

    if (slot->script == NULL) {
        return;
    }

    do {
        cmd = slot->scriptPos;

        if (*cmd & 0x8000) {
            handler = gMapAnmCmds[*(u8*)cmd];
        } else {
            handler = MapAnmCmdFrame;
        }
    } while (handler(slot));
}

void MapAnmFlushSlot(MapAnmSlot* slot) {
    if (slot->pending != NULL) {
        RequestDma3Copy(slot->pending, slot->dest, slot->frameSize);
        slot->pending = NULL;
    }
}

void MapAnmUpdateSlot(MapAnmSlot* slot) {
    MapAnmStepScript(slot);
    MapAnmFlushSlot(slot);
}

void MapAnmResetSlot(MapAnmSlot* slot) {
    slot->tiles = NULL;
    slot->dest = (u8*)GetBgCharBase(2) + 0x7800;
    slot->pending = NULL;
    slot->timer = 0;
    slot->script = NULL;
    slot->scriptPos = NULL;
}

u8 MapAnmCmdEnd(MapAnmSlot* slot) {
    slot->script = NULL;
    slot->scriptPos = NULL;
    return 0;
}

u8 MapAnmCmdFrame(MapAnmSlot* slot) {
    MapAnmSlot* anmSlot = slot;
    s16* cmd;
    s16 duration;

    if (slot->timer == 0) {
        slot->pending = (u8*)slot->tiles + slot->frameSize * slot->scriptPos[1];
    }

    cmd = anmSlot->scriptPos;
    duration = *(u16*)cmd & 0x7FFF;

    if (duration != 0) {
        anmSlot->timer++;

        if (anmSlot->timer >= duration) {
            anmSlot->timer = 0;
            anmSlot->scriptPos = cmd + 2;
        }
    }

    return 0;
}

u8 MapAnmCmdLoop(MapAnmSlot* slot) {
    slot->scriptPos = slot->script;
    return 1;
}

u8 IsEventDoor(u8 room, u8 side) {
    MapEventDoor* door;

    if ((s32)gMapRoomState->flags < 0) {
        return FALSE;
    }

    door = GetMapEventDoor(0);

    while (door->kind != EVENT_DOOR_END) {
        if (door->keyList != EVENT_KEY_LIST_NONE && door->room == room && door->side == side) {
            return TRUE;
        }

        door++;
    }

    return FALSE;
}

u8 SelectEventDoor(u8 room, u8 side) {
    MapEventDoor* door;
    u8 i;

    if ((s32)gMapRoomState->flags < 0) {
        return FALSE;
    }

    i = 0;
    door = GetMapEventDoor(0);

    while (door->kind != EVENT_DOOR_END) {
        if (door->keyList != EVENT_KEY_LIST_NONE && door->room == room && door->side == side) {
            sEventKeyList = &gEventKeyLists[door->keyList];
            sEventKeyProgress = &gMapFloorState.eventKeyProgress[i];
            return TRUE;
        }

        i++;
        door++;
    }

    return FALSE;
}

u8 CountRemainingEventKeys() {
    return sEventKeyList->count - sEventKeyProgress->paid;
}

EventKey* GetEventKey(u8 index) {
    EventKey* keys = &sEventKeyList->keys[sEventKeyProgress->paid];
    EventKey* key = &keys[index];

    sEventKey = *key;

    if (index == 0 && key->rule == EVENT_KEY_RULE_TOTAL && sEventKeyProgress->remaining != 0) {
        sEventKey.value = sEventKeyProgress->remaining;
    }

    return &sEventKey;
}

u8 DoorAcceptsMapCard(MapCardAttributes* card) {
    EventKey* key;
    u8 roomValue;

    if (!IsEventDoor(gMapRoomState->doorRoom, gMapRoomState->doorSide)) {
        if (card->kind > MAP_CARD_BLACK_ROOM) {
            return 0;
        }

        roomValue = GetMapRoomCardValue(gMapFloorState.room);

        if (card->value == 0) {
            return 1;
        }

        return card->value > roomValue;
    }

    key = GetEventKey(0);

    if (key->kind != MAP_CARD_NONE) {
        if (key->kind != card->kind) {
            return 0;
        }
    } else if (card->kind > MAP_CARD_BLACK_ROOM) {
        return 0;
    }

    if (key->color != MAP_CARD_COLOR_NONE && key->color != card->color) {
        return 0;
    }

    switch (key->rule) {
    case EVENT_KEY_RULE_AT_LEAST:
        return card->value >= key->value;
    case EVENT_KEY_RULE_AT_MOST:
        return card->value <= key->value;
    case EVENT_KEY_RULE_EXACT:
        return card->value == key->value;
    case EVENT_KEY_RULE_TOTAL:
        return card->value != 0;
    }

    return 1;
}

s32 PayEventKey(MapCardAttributes* card) {
    if (GetEventKey(0)->rule == EVENT_KEY_RULE_TOTAL) {
        if (sEventKey.value > card->value) {
            sEventKey.value -= card->value;
            sEventKeyProgress->remaining = sEventKey.value;
            return FALSE;
        }

        sEventKeyProgress->remaining = 0;
    }

    sEventKeyProgress->paid++;
    return TRUE;
}

const PrizeEntry* PickRandomPrzCard(u8 worldPrize) {
    u16 roll = GetRandom() % 10000;
    PrzCardChance** chanceTables = gWorldPrzCardChances;
    PrzCardChance* chance = chanceTables[gGameState.world];

    while (chance->cardIndex != 41) {
        const PrizeEntry* prize = &gPrzCardKinds[chance->cardIndex];
        u16 weight = worldPrize ? chance->weight2 : chance->weight;

        if (roll < weight) {
            if (IsCardKindObtained(prize->unk_00[0])) {
                return prize;
            }

            if (chance->cardIndex <= 16) {
                return &gPrzCardKinds[0];
            }

            if (chance->cardIndex <= 30) {
                return &gPrzCardKinds[20];
            }

            return &gPrzCardKinds[31];
        }

        roll -= weight;
        chance++;
    }

    return NULL;
}

u8 RollCardValue() {
    u16 acc = 0;
    u16 roll = GetRandom() % 10000;
    s32 i;
    const u16* weights = gCardValueWeights;

    for (i = 0; i < 10; i++) {
        acc += weights[i];

        if (roll < acc) {
            return i;
        }
    }

    return 0;
}

s32 CreateMapPrzCardTask(const PrizeEntry* prize, u8 worldPrize, s32 x, s32 y, s32 z) {
    MapPrizeArgs args;

    args.worldPrize = worldPrize;
    args.x = x;
    args.y = y;
    args.z = z;
    args.id = prize->unk_02;

    if (args.id <= 0x1B8) {
        args.id += RollCardValue();
    }

    if (CountCardsById(args.id) <= 0x62) {
        TaskCreate(&gFieldState->tasks5, &gTaskDescMapPrzCard, &args);
        return TRUE;
    }

    return FALSE;
}

u8 TryCreateRandomPrzCard(u8 worldPrize, s32 x, s32 y, s32 z) {
    const PrizeEntry* prize;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        return FALSE;
    }

    if (IsCardCollectionFull()) {
        return FALSE;
    }

    if (gMapRoomState->flags & ROOM_FLAG_NO_RANDOM_PRIZE) {
        return FALSE;
    }

    if (gMapRoomState->flags & ROOM_FLAG_PRIZE_CARD_ACTIVE) {
        return FALSE;
    }

    if (GetMapFloorRoom(gMapFloorState.room)->przCardsLeft == 0) {
        return FALSE;
    }

    prize = PickRandomPrzCard(worldPrize);

    if (prize == NULL) {
        return FALSE;
    }

    return CreateMapPrzCardTask(prize, FALSE, x, y, z);
}

void CreateMapPrizeTasks(u8 id, u8 count, s32 x, s32 y, s32 z) {
    MapPrizeArgs args;
    s32 i;

    args.x = x;
    args.y = y;
    args.z = z;
    args.id = id;

    for (i = 0; i < count; i++) {
        TaskCreate(&gFieldState->tasks3, &gTaskDescMapPrize, &args);
    }
}

void CreateWorldPrize(s32 x, s32 y, s32 z) {
    PrizeEntry* entry = gWorldPrizeLists[gGameState.world];
    const PrizeEntry* prize;

    for (; entry->unk_00[0] != 4; entry++) {
        switch (entry->unk_00[0]) {
        case 0:
            if (gMapRoomState->roomType == ROOM_TYPE_HIDDEN_CHAMBER) {
                break;
            }

            prize = &gPrzCardKinds[entry->unk_00[1]];

            if (IsCardKindObtained(prize->unk_00[0]) == TRUE) {
                break;
            }

            SetCardKindObtained(prize->unk_00[0]);
            CreateMapPrzCardTask(prize, TRUE, x, y, z);
            return;
        case 1:
            if (gMapRoomState->roomType == ROOM_TYPE_HIDDEN_CHAMBER) {
                break;
            }

            prize = &gPrzStocks[entry->unk_00[1]];

            if (IsStockLearned(prize->unk_00[0]) == TRUE) {
                break;
            }

            LearnStock(prize->unk_00[0]);
            TaskCreate(&gFieldState->tasks5, &gTaskDescMapPrzStock, prize);
            return;
        case 2:
            if (gMapRoomState->roomType != ROOM_TYPE_HIDDEN_CHAMBER) {
                break;
            }

            prize = &gPrzCardKinds[entry->unk_00[1]];

            if (IsCardKindObtained(prize->unk_00[0]) == TRUE) {
                break;
            }

            SetCardKindObtained(prize->unk_00[0]);
            CreateMapPrzCardTask(prize, TRUE, x, y, z);
            gMapFloorState.flags |= FLOOR_FLAG_CHAMBER_PRIZE_TAKEN;
            return;
        case 3:
            if (gMapRoomState->roomType != ROOM_TYPE_HIDDEN_CHAMBER) {
                break;
            }

            prize = &gPrzStocks[entry->unk_00[1]];

            if (IsStockLearned(prize->unk_00[0]) != TRUE) {
                LearnStock(prize->unk_00[0]);
                TaskCreate(&gFieldState->tasks5, &gTaskDescMapPrzStock, prize);
                gMapFloorState.flags |= FLOOR_FLAG_CHAMBER_PRIZE_TAKEN;
                return;
            }

            break;
        }
    }

    if ((gGameState.flags & GAME_FLAG_RIKU_CLEAR) && gGameState.world == WORLD_CASTLE_OBLIVION && IsCardKindObtained(OBTAINED_CARD_ONE_WINGED_ANGEL)) {
        prize = &gPrzCardKinds[14];

        if (IsCardKindObtained(prize->unk_00[0]) != TRUE) {
            SetCardKindObtained(prize->unk_00[0]);
            CreateMapPrzCardTask(prize, TRUE, x, y, z);
            return;
        }

        prize += 24;

        if (IsCardKindObtained(prize->unk_00[0]) != TRUE) {
            SetCardKindObtained(prize->unk_00[0]);
            CreateMapPrzCardTask(prize, TRUE, x, y, z);
            return;
        }

        prize++;

        if (IsCardKindObtained(prize->unk_00[0]) != TRUE) {
            SetCardKindObtained(prize->unk_00[0]);
            CreateMapPrzCardTask(prize, TRUE, x, y, z);
            return;
        }
    }

    prize = PickRandomPrzCard(TRUE);

    if (prize != NULL) {
        CreateMapPrzCardTask(prize, TRUE, x, y, z);
    }
}

u8 AreWorldPrizesCollected() {
    s32 i;
    u8* entry;

    for (i = 1; i <= 11; i++) {
        PrizeEntry** lists = gWorldPrizeLists;

        entry = (u8*)lists[i];

        while (entry[0] != 4) {
            switch (entry[0]) {
            case 2:
                if (IsCardKindObtained((gPrzCardKinds + entry[1])->unk_00[0]) != TRUE) {
                    return FALSE;
                }

                break;
            case 3:
                if (IsStockLearned((gPrzStocks + entry[1])->unk_00[0]) != TRUE) {
                    return FALSE;
                }

                break;
            }

            entry += 4;
        }
    }

    return TRUE;
}

void CopyMapProgress(MapProgress* progress) {
    s32 i;
    u32* src;
    u32* dst;

    progress->world = gGameState.world;
    progress->floor = gGameState.floor;
    memcpy(progress->floorState, &gMapFloorState, sizeof(progress->floorState));
    src = (u32*)gGameState.floors;
    dst = (u32*)progress->floors;

    for (i = 12; i >= 0; i--) {
        *dst++ = *src++;
    }
}

void RestoreMapProgress(MapProgress* progress) {
    s32 i;
    u32* src;
    u32* dst;

    gGameState.world = progress->world;
    gGameState.floor = progress->floor;
    memcpy(&gMapFloorState, progress->floorState, sizeof(MapFloorState));
    src = (u32*)progress->floors;
    dst = (u32*)gGameState.floors;

    for (i = 12; i >= 0; i--) {
        *dst++ = *src++;
    }
}

void MapDbgSetUpdate(ModeFunc update) {
    sMapDbgUpdate = update;
}

void MapDbgSetUpdateAndRun(ModeFunc update) {
    MapDbgSetUpdate(update);
    sMapDbgUpdate();
}

void MapDbgFreeCameraInput() {
    s32 y = 0;
    s32 x = 0;
    u16 bg1Mask = DISPCNT_BG1_ON;
    u16 objMask = DISPCNT_OBJ_ON;

    if (GetKeysHeld() & DPAD_LEFT) {
        x = -1024;
    }

    if (GetKeysHeld() & DPAD_RIGHT) {
        x = 1024;
    }

    if (GetKeysHeld() & DPAD_UP) {
        y = -1024;
    }

    if (GetKeysHeld() & DPAD_DOWN) {
        y = 1024;
    }

    MapMoveCameraTarget(x, y);

    if (GetKeysPressed() & A_BUTTON) {
        gDispCnt = (gDispCnt & ~DISPCNT_BG1_ON) | (bg1Mask & ~gDispCnt);
    }

    if (GetKeysPressed() & B_BUTTON) {
        gDispCnt = (gDispCnt & ~DISPCNT_OBJ_ON) | (objMask & ~gDispCnt);
    }
}

void MapDbgMain() {
    if (gMapRoomState->flags & ROOM_FLAG_START_BATTLE) {
        gMapRoomState->flags &= ~ROOM_FLAG_START_BATTLE;
        gMapRoomState->flags &= ~ROOM_FLAG_ENEMY_STRUCK;
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
    }

    if (gFieldState->flags & FIELD_FLAG_EXIT_ROOM) {
        FadeStartOut(FADE_MODE_BLACK, 16);
        MapDbgSetUpdateAndRun(MapDbgExitRoom);
        return;
    }

    if (gMapRoomState->flags & ROOM_FLAG_SAVE_MENU_OPEN) {
        TaskKill(&gFieldState->tasks, sMapDbgAllmapRoomTask);
        sMapDbgAllmapRoomTask = NULL;
        MapDbgSetUpdateAndRun(MapDbgWaitMenu);
        return;
    }

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        TaskKill(&gFieldState->tasks, sMapDbgAllmapRoomTask);
        sMapDbgAllmapRoomTask = NULL;
        MapDbgSetUpdateAndRun(MapDbgWaitRoomCreate);
        return;
    }

    if (sMapDbgEditing != 0) {
        MapDbgSetUpdateAndRun(MapDbgWaitEdit);
        return;
    }

    UpdateMapField();
    DrawMapField();
    ColliderUpdateAll();
    MapEnmUpdateSpawner();

    if ((GetKeysHeld() & (L_BUTTON | R_BUTTON)) == (L_BUTTON | R_BUTTON)) {
        return;
    }

    if (GetKeysPressed() & START_BUTTON) {
        MapDbgSetUpdate(MapDbgFreeCameraMode);
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        ModeRequest(&gModeMapChk, 0);
    }
}

void MapDbgExitRoom() {
    DrawMapField();

    if (!FadeIsActive()) {
        SetCurrentMapRoom(gMapRoomState->doorRoom, gMapRoomState->doorSide);

        if (gMapRoomState->doorRoom != MAP_ROOM_EXIT_HALL && gMapRoomState->doorRoom != MAP_ROOM_ENTRANCE_HALL) {
            ModeRequest(&gModeMapDbg, 0);
        } else {
            RequestMapMode();
        }
    }
}

void MapDbgFreeCameraMode() {
    if (sMapDbgEditing != 0) {
        MapDbgSetUpdateAndRun(MapDbgWaitEdit);
        return;
    }

    MapDbgFreeCameraInput();
    TaskPoolUpdate(&gFieldState->tasks);
    DrawMapField();

    if ((GetKeysHeld() & (L_BUTTON | R_BUTTON)) == (L_BUTTON | R_BUTTON)) {
        return;
    }

    if (GetKeysPressed() & START_BUTTON) {
        MapDbgSetUpdate(MapDbgMain);
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        ModeRequest(&gModeMapChk, 0);
    }
}

void MapDbgWaitEdit() {
    DrawMapField();

    if (sMapDbgEditing == 0) {
        ModeRequest(&gModeMapDbg, 0);
    }
}

void MapDbgWaitMenu() {
    if ((gFieldState->flags & FIELD_FLAG_MENU_OPEN) == 0 && (gMapRoomState->flags & ROOM_FLAG_SAVE_MENU_OPEN) == 0) {
        sMapDbgAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
        MapGmkCreateTasks();
        MapDbgSetUpdateAndRun(MapDbgMain);
    } else {
        UpdateMapField();
        DrawMapField();
    }
}

void MapDbgWaitRoomCreate() {
    if (gFieldState->flags & FIELD_FLAG_EXIT_ROOM) {
        FadeStartOut(FADE_MODE_BLACK, 16);
        MapDbgSetUpdateAndRun(MapDbgExitRoom);
    } else if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) == 0) {
        gBldCnt = 0;
        SetBgPriority(0, 0);
        sMapDbgAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
        MapGmkCreateTasks();
        MapEnmRestoreFromCache();
        MapDbgSetUpdateAndRun(MapDbgMain);
    } else {
        UpdateMapField();
        DrawMapField();
    }
}

void Mode_MapDbg_0() {
    MapRoomDef* roomDef;

    gFieldState = EwramAlloc(sizeof(FieldState));
    gMapRoomState = EwramAlloc(sizeof(MapRoomState));
    UpdateGameWorld();
    SetBgMode0();
    SetupBg(3, 0, 28, 0);
    SetupBg(2, 0, 29, 0);
    SetupBg(1, 2, 30, 0);
    SetupBg(0, 3, 31, 14);
    SetBgPriority(3, 3);
    SetBgPriority(2, 3);
    SetBgPriority(1, 1);
    SetBgPriority(0, 0);
    SetBackdropColor(6, 31, 31);
    SetBlendAlpha(6, 10);
    InitFieldState();
    ColliderPoolsInit();
    CreateMapRndTask();
    MapGmkInitRoom();
    MapEnmInitRoom();
    MapApplyRoomDecor();
    SpawnMapPlayer();

    roomDef = gMapRoomDefs[gMapFloorState.world];
    TaskCreate(&gFieldState->tasks, &gTaskDescLockon, NULL);
    TaskCreate(&gFieldState->tasks, &gTaskDescMapAnm, roomDef->tileAnims);
    sMapDbgAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
    MapDbgSetUpdate(MapDbgMain);

    if (gGameState.fieldResume) {
        MapSetCameraTarget(gGameState.fieldPosition.x, gGameState.fieldPosition.y + gGameState.fieldPosition.z);
    } else {
        MapSetCameraTarget(gFieldState->spawnX, gFieldState->spawnY);
    }

    MapSnapCamera();
    ClearFieldResume();
    SeedRandom(gFrameCounter);
    m4aSongNumStartOrContinue(roomDef->song);
    TaskPoolInit(&sMapDbgTasks, 1);
    TaskCreate(&sMapDbgTasks, &gTaskDescMapDbg, &sMapDbgEditing);
    TaskCreate(&gFieldState->tasks, &gTaskDescMapDmg, NULL);
    FadeStartIn(FADE_MODE_BLACK, 16);
}

void Mode_MapDbg_1() {
    TaskPoolUpdate(&sMapDbgTasks);
    TaskPoolDraw(&sMapDbgTasks);
    sMapDbgUpdate();
    UpdatePlayTime();
}

void Mode_MapDbg_2() {
    DestroyMapField();
    MapGmkFree();
    EwramFree(gFieldState);
    EwramFree(gMapRoomState);
    TaskPoolDestroy(&sMapDbgTasks);
}

void MapFldSetUpdate(ModeFunc update) {
    sMapFldUpdate = update;
}

void MapFldSetUpdateAndRun(ModeFunc update) {
    MapFldSetUpdate(update);
    sMapFldUpdate();
}

void MapFldCreateWorldLogo() {
    switch (gMapFloorState.world) {
    case WORLD_ATLANTICA:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)2);
        break;
    case WORLD_HALLOWEEN_TOWN:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)6);
        break;
    case WORLD_MONSTRO:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)5);
        break;
    case WORLD_NEVER_LAND:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)7);
        break;
    case WORLD_OLYMPUS_COLISEUM:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)3);
        break;
    case WORLD_HOLLOW_BASTION:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)8);
        break;
    case WORLD_DESTINY_ISLANDS:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)9);
        break;
    case WORLD_AGRABAH:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)1);
        break;
    case WORLD_TRAVERSE_TOWN:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)10);
        break;
    case WORLD_TWILIGHT_TOWN:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)11);
        break;
    case WORLD_CASTLE_OBLIVION:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)12);
        break;
    default:
        sMapFldWorldLogoTask = TaskCreate(&gMapRoomState->tasks, &gTaskDescWLogo, (void*)4);
        break;
    }
}

void MapFldDestroyAllmapRoom() {
    if (sMapFldAllmapRoomTask != NULL) {
        TaskKill(&gFieldState->tasks, sMapFldAllmapRoomTask);
        sMapFldAllmapRoomTask = NULL;
    }
}

void StartWorldBossBattle() {
    switch (gGameState.world) {
    case WORLD_TRAVERSE_TOWN:
        ModeRequest(&gModeBattle, BATTLE_GUARD_ARMOR);
        break;
    case WORLD_AGRABAH:
        ModeRequest(&gModeBattle, BATTLE_JAFAR);
        break;
    case WORLD_MONSTRO:
        ModeRequest(&gModeBattle, BATTLE_PARASITE_CAGE);
        break;
    case WORLD_NEVER_LAND:
        ModeRequest(&gModeBattle, BATTLE_HOOK);
        break;
    case WORLD_HALLOWEEN_TOWN:
        ModeRequest(&gModeBattle, BATTLE_OOGIE_BOOGIE);
        break;
    case WORLD_ATLANTICA:
        ModeRequest(&gModeBattle, BATTLE_URSULA);
        break;
    case WORLD_WONDERLAND:
        ModeRequest(&gModeBattle, BATTLE_TRICKMASTER);
        break;
    case WORLD_OLYMPUS_COLISEUM:
        ModeRequest(&gModeBattle, BATTLE_HADES);
        break;
    }
}

void MapFldShowWorldLogo() {
    u8 active = IsTaskActive(sMapFldWorldLogoTask);

    if (active) {
        TaskPoolUpdate(&gMapRoomState->tasks);
        TaskPoolDraw(&gMapRoomState->tasks);
        TaskPoolUpdate(&gFieldState->tasks);
        DrawMapField();
    } else {
        u16 flags = gMapFloorState.flags | FLOOR_FLAG_LOGO_SHOWN;
        gMapFloorState.flags = flags;
        sMapFldWorldLogoTask = NULL;
        sMapFldAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
        gFieldState->flags &= ~FIELD_FLAG_NO_ENEMY_SPAWN;
        gFieldState->flags &= ~FIELD_FLAG_NO_LOCKON;
        MapFldSetUpdateAndRun(MapFldMain);
    }
}

void MapFldMain() {
    if (gMapRoomState->flags & ROOM_FLAG_START_BATTLE) {
        FadeStartOut(FADE_MODE_BLACK, 16);
        FadeLock();
        MapFldSetUpdateAndRun(MapFldStartBattle);
        return;
    }

    if (gFieldState->flags & FIELD_FLAG_EXIT_ROOM) {
        FadeStartOut(FADE_MODE_BLACK, 16);
        FadeLock();
        MapFldSetUpdateAndRun(MapFldExitRoom);
        return;
    }

    if (!FadeIsActive() && (gGameState.progression.tutorialFlags & TUTORIAL_FLAG_ROOM_CREATE) != 0 &&
        (gFieldState->flags & (FIELD_FLAG_FREEZE_PLAYER | FIELD_FLAG_ROOM_CREATE)) == 0 && (gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) == 0) {
        if (GetKeysPressed() & SELECT_BUTTON) {
            m4aSongNumStart(SONG_SYS_CANSEL);
            FadeStartOut(FADE_MODE_BLACK, 16);
            FadeLock();
            MapFldSetUpdateAndRun(MapFldOpenAllmap);
            return;
        }

        if (GetKeysPressed() & START_BUTTON) {
            MapFldDestroyAllmapRoom();
            TaskCreate(&gFieldState->tasks, &gTaskDescMapMenu, NULL);
            MapFldSetUpdateAndRun(MapFldWaitMenu);
            return;
        }
    }

    if (gMapRoomState->flags & ROOM_FLAG_SAVE_MENU_OPEN) {
        MapFldDestroyAllmapRoom();
        MapFldSetUpdateAndRun(MapFldWaitMenu);
        return;
    }

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapFldDestroyAllmapRoom();
        MapFldSetUpdateAndRun(MapFldWaitRoomCreate);
        return;
    }

    UpdateMapField();
    DrawMapField();
    ColliderUpdateAll();
    MapEnmUpdateSpawner();
}

void MapFldExitRoom() {
    u8 fading;
    u8* roomEvent;
    MapEventDoor* door;

    DrawMapField();
    fading = FadeIsActive();

    if (fading) {
        return;
    }

    if (gMapRoomState->doorRoom == MAP_ROOM_ENTRANCE_HALL) {
        EnterEntranceHall();
        return;
    }

    if (gMapRoomState->doorRoom == MAP_ROOM_EXIT_HALL) {
        EnterExitHall();
        return;
    }

    roomEvent = GetMapRoomEvent(gMapFloorState.eventStep);

    if (roomEvent[0] == MAP_ROOM_EVENT_END) {
        SetCurrentMapRoom(gMapRoomState->doorRoom, gMapRoomState->doorSide);
        ModeRequest(&gModeMapFld, 0);
        return;
    }

    door = GetMapEventDoor(roomEvent[0]);

    if (door->room != gMapRoomState->doorRoom || door->side != gMapRoomState->doorSide) {
        SetCurrentMapRoom(gMapRoomState->doorRoom, gMapRoomState->doorSide);
        ModeRequest(&gModeMapFld, 0);
        return;
    }

    gGameState.roomEffect = fading;

    switch (door->kind) {
    case EVENT_DOOR_EVENT_ROOM:
    case EVENT_DOOR_WALK_IN:
        if (roomEvent[1] == EVENT_081_MONSTORO_E3 && (gGameState.flags & GAME_FLAG_MONSGAGE_BATTLE)) {
            RequestEventMode(EVENT_085_MONSTORO_E3_RETRY);
        } else {
            RequestEventMode(roomEvent[1]);
        }

        break;
    case EVENT_DOOR_HIDDEN_CHAMBER:
        SetCurrentMapRoom(gMapRoomState->doorRoom, gMapRoomState->doorSide);
        ModeRequest(&gModeMapFld, 0);
        break;
    case EVENT_DOOR_BOSS_ROOM:
        StartWorldBossBattle();
        break;
    }
}

void MapFldStartBattle() {
    DrawMapField();

    if (!FadeIsActive()) {
        RequestFieldResume();

        if (gGameState.flags & GAME_FLAG_RIKU) {
            if (gGameState.progression.tutorialFlags & TUTORIAL_FLAG_RIKU_BATTLE) {
                ModeRequest(&gModeBattle, gMapRoomState->battleId);
            } else {
                ModeRequest(&gModeRikuBtlTutorial, gMapRoomState->battleId);
            }
        } else {
            ModeRequest(&gModeBattle, gMapRoomState->battleId);
        }
    }
}

void MapFldOpenAllmap() {
    DrawMapField();

    if (!FadeIsActive()) {
        RequestFieldResume();
        ModeRequest(&gModeAllmap, 1);
    }
}

void MapFldWaitMenu() {
    if ((gFieldState->flags & FIELD_FLAG_MENU_OPEN) == 0 && (gMapRoomState->flags & ROOM_FLAG_SAVE_MENU_OPEN) == 0) {
        SetupBg(0, 3, 31, 14);
        SetBgPriority(0, 0);
        MapGmkCreateTasks();
        sMapFldAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
        MapFldSetUpdateAndRun(MapFldMain);
    } else {
        UpdateMapField();
        DrawMapField();
    }
}

void MapFldWaitRoomCreate() {
    u16 tutorialFlags;

    if (gFieldState->flags & FIELD_FLAG_EXIT_ROOM) {
        FadeStartOut(FADE_MODE_BLACK, 16);
        FadeLock();
        MapFldSetUpdateAndRun(MapFldExitRoom);

        if ((gGameState.progression.tutorialFlags & TUTORIAL_FLAG_ROOM_CREATE) == 0) {
            tutorialFlags = gGameState.progression.tutorialFlags | TUTORIAL_FLAG_ROOM_CREATE;
            gGameState.progression.tutorialFlags = tutorialFlags;
        }
    } else if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) == 0) {
        gBldCnt = 0;
        SetBgPriority(0, 0);
        MapGmkCreateTasks();
        MapEnmRestoreFromCache();
        sMapFldAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
        MapFldSetUpdateAndRun(MapFldMain);
    } else {
        UpdateMapField();
        DrawMapField();
    }
}

void MapFldRestart() {
    DrawMapField();

    if (sUnk_02034FBC == 0) {
        ModeRequest(&gModeMapFld, 0);
    }
}

void Mode_MapFld_0() {
    MapRoomDef* roomDef;

    if ((gMapFloorState.flags & FLOOR_FLAG_LOGO_SHOWN) && !gGameState.fieldResume) {
        switch (gMapFloorState.entrySide) {
        case MAP_DOOR_SIDE_UP_RIGHT:
            gGameState.fieldAngle = FLD_ANGLE_DOWN_LEFT;
            break;
        case MAP_DOOR_SIDE_DOWN_LEFT:
            gGameState.fieldAngle = FLD_ANGLE_UP_RIGHT;
            break;
        case MAP_DOOR_SIDE_DOWN_RIGHT:
            gGameState.fieldAngle = FLD_ANGLE_UP_LEFT;
            break;
        case MAP_DOOR_SIDE_UP_LEFT:
            gGameState.fieldAngle = FLD_ANGLE_DOWN_RIGHT;
            break;
        }

        StartFieldTransition();
    }

    gFieldState = EwramAlloc(sizeof(FieldState));
    gMapRoomState = EwramAlloc(sizeof(MapRoomState));
    sMapFldWorldLogoTask = NULL;
    sMapFldAllmapRoomTask = NULL;
    UpdateGameWorld();
    SetBgMode0();
    SetupBg(3, 0, 28, 0);
    SetupBg(2, 0, 29, 0);
    SetupBg(1, 2, 30, 0);
    SetupBg(0, 3, 31, 14);
    SetBgPriority(3, 3);
    SetBgPriority(2, 3);
    SetBgPriority(1, 1);
    SetBgPriority(0, 0);
    SetBackdropColor(6, 31, 31);
    InitFieldState();
    ColliderPoolsInit();
    CreateMapRndTask();
    MapGmkInitRoom();
    MapEnmInitRoom();
    MapApplyRoomDecor();
    SpawnMapPlayer();

    roomDef = gMapRoomDefs[gMapFloorState.world];
    TaskCreate(&gFieldState->tasks, &gTaskDescLockon, NULL);
    TaskCreate(&gFieldState->tasks, &gTaskDescMapAnm, roomDef->tileAnims);

    if ((gGameState.progression.tutorialFlags & TUTORIAL_FLAG_MAP) == 0) {
        TaskCreate(&gFieldState->tasks, &gTaskDescMapTutorial, NULL);
    }

    if ((gMapFloorState.flags & FLOOR_FLAG_LOGO_SHOWN) == 0) {
        MapFldSetUpdate(MapFldShowWorldLogo);
        gFieldState->flags |= FIELD_FLAG_NO_ENEMY_SPAWN;
        gFieldState->flags |= FIELD_FLAG_NO_LOCKON;
        MapSetCameraTarget(gFieldState->spawnX, gFieldState->spawnY);
        MapFldCreateWorldLogo();
    } else if (gGameState.fieldResume) {
        MapSetCameraTarget(gGameState.fieldPosition.x, gGameState.fieldPosition.y + gGameState.fieldPosition.z);

        if ((s8)gGameState.mapMenuCursor != -1) {
            gDispCnt &= ~DISPCNT_OBJ_ON;
            TaskCreate(&gFieldState->tasks, &gTaskDescMapMenu, NULL);
            MapFldSetUpdate(MapFldWaitMenu);
        } else {
            sMapFldAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
            MapFldSetUpdate(MapFldMain);
        }
    } else {
        sMapFldAllmapRoomTask = CreateAllmapRoomTask(&gFieldState->tasks);
        MapSetCameraTarget(gFieldState->spawnX, gFieldState->spawnY);
        MapFldSetUpdate(MapFldMain);
    }

    MapSnapCamera();
    ClearFieldResume();
    SeedRandom(gFrameCounter);
    m4aSongNumStartOrContinue(roomDef->song);
    FadeStartIn(FADE_MODE_BLACK, 16);
}

void Mode_MapFld_1() {
    sMapFldUpdate();
    UpdatePlayTime();
}

void Mode_MapFld_2() {
    DestroyMapField();
    MapGmkFree();
    EwramFree(gFieldState);
    EwramFree(gMapRoomState);
}

void MapFixSetUpdate(ModeFunc update) {
    sMapFixUpdate = update;
}

void MapFixSetUpdateAndRun(ModeFunc update) {
    MapFixSetUpdate(update);
    sMapFixUpdate();
}

MapFixedDef* GetMapFixedDef() {
    if (gGameState.flags & GAME_FLAG_RIKU) {
        if (gMapFloorState.room == MAP_ROOM_EXIT_HALL) {
            return gMapFixedDefs[2];
        }

        if (gMapFloorState.room == MAP_ROOM_ENTRANCE_HALL) {
            if (gGameState.floor != 0) {
                return gMapFixedDefs[1];
            }

            return gMapFixedDefs[5];
        }
    }

    if (gMapFloorState.room == MAP_ROOM_EXIT_HALL) {
        if (gGameState.floor != 12) {
            return gMapFixedDefs[2];
        }

        return gMapFixedDefs[3];
    }

    if (gMapFloorState.room == MAP_ROOM_ENTRANCE_HALL) {
        if (gGameState.floor != 0) {
            return gMapFixedDefs[1];
        }

        return gMapFixedDefs[0];
    }

    return gMapFixedDefs[4];
}

void MapFixCreateCharaTasks() {
    if (gMapFloorState.room == MAP_ROOM_TUTORIAL) {
        return;
    }

    if (gGameState.flags & GAME_FLAG_RIKU) {
        switch (gMapFloorState.progress) {
        case 20:
        case 22:
        case 23:
        case 24:
            TaskCreate(&gFieldState->tasks, &gTaskDescMapMickey, NULL);
            break;
        }

        return;
    }

    if (gGameState.progression.friendFlags & FRIEND_FLAG_DONALD_DUCK) {
        TaskCreate(&gFieldState->tasks, &gTaskDescMapDonald, NULL);
    }

    if (gGameState.progression.friendFlags & FRIEND_FLAG_GOOFY) {
        TaskCreate(&gFieldState->tasks, &gTaskDescMapGoofy, NULL);
    }

    if (gMapFloorState.room != MAP_ROOM_EXIT_HALL) {
        return;
    }

    switch (gMapFloorState.progress) {
    case 23:
    case 24:
        if (gGameState.floor == 11) {
            TaskCreate(&gFieldState->tasks, &gTaskDescMapNamine, NULL);
            TaskCreate(&gFieldState->tasks, &gTaskDescMapNiseriku, NULL);
        }

        break;
    case 25:
    case 26:
        if (gGameState.floor == 11 && gGameState.floors[12].eventStep == 0) {
            TaskCreate(&gFieldState->tasks, &gTaskDescMapNamine, NULL);
            TaskCreate(&gFieldState->tasks, &gTaskDescMapNiseriku, NULL);
        }

        break;
    case 27:
        if (gGameState.floor == 12) {
            TaskCreate(&gFieldState->tasks, &gTaskDescMapNamine, NULL);
            TaskCreate(&gFieldState->tasks, &gTaskDescMapNiseriku, NULL);
        }

        break;
    }
}

u8 GetWorldEntryEventId() {
    if (gMapFloorState.flags & FLOOR_FLAG_ENTRY_EVENT_DONE) {
        return MAP_EVENT_NONE;
    }

    if (gGameState.flags & GAME_FLAG_RIKU) {
        switch (gMapFloorState.world) {
        case WORLD_HOLLOW_BASTION:
            return EVENT_151_RIKU_B12F_E0;
        case WORLD_DESTINY_ISLANDS:
            return EVENT_177_RIKU_B3F_E0;
        case WORLD_TWILIGHT_TOWN:
            return EVENT_186_RIKU_B2F_E0;
        case WORLD_CASTLE_OBLIVION:
            return EVENT_192_RIKU_B1F_E0;
        }

        return MAP_EVENT_NONE;
    }

    return gWorldEntryEvents[gMapFloorState.world];
}

u8 GetFloorEventId() {
    if (gMapFloorState.flags & FLOOR_FLAG_CLEARED) {
        return MAP_EVENT_NONE;
    }

    if (gGameState.flags & GAME_FLAG_RIKU) {
        return gRikuFloorEvents[gGameState.floor];
    }

    return gSoraFloorEvents[gGameState.floor];
}

void MapFixMain() {
    if (gMapRoomState->flags & ROOM_FLAG_WALK_OUT) {
        MapFixSetUpdateAndRun(MapFixWaitWalkOut);
        return;
    }

    if (gMapRoomState->flags & (ROOM_FLAG_ENTER_WORLD | ROOM_FLAG_EXIT_NEXT_FLOOR | ROOM_FLAG_EXIT_PREV_FLOOR)) {
        FadeStartOut(FADE_MODE_BLACK, 16);

        if (gMapFloorState.room == MAP_ROOM_ENTRANCE_HALL) {
            MapFixSetUpdateAndRun(MapFixLeaveEntranceHall);
            return;
        }

        if (gMapFloorState.room == MAP_ROOM_EXIT_HALL) {
            MapFixSetUpdateAndRun(MapFixLeaveExitHall);
            return;
        }

        MapFixSetUpdateAndRun(MapFixEnterMapFld);
        return;
    }

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        MapFixSetUpdateAndRun(MapFixWaitRoomCreate);
        return;
    }

    if (gMapRoomState->flags & ROOM_FLAG_SAVE_MENU_OPEN) {
        MapFixSetUpdateAndRun(MapFixWaitMenu);
    } else if (!FadeIsActive() && (gGameState.progression.tutorialFlags & TUTORIAL_FLAG_ROOM_CREATE) != 0 &&
               (gFieldState->flags & (FIELD_FLAG_FREEZE_PLAYER | FIELD_FLAG_ROOM_CREATE)) == 0 && (gMapRoomState->flags & ROOM_FLAG_ENEMY_STRUCK) == 0 &&
               (GetKeysPressed() & START_BUTTON) != 0) {
        TaskCreate(&gFieldState->tasks, &gTaskDescMapMenu, NULL);
        MapFixSetUpdateAndRun(MapFixWaitMenu);
    } else {
        UpdateMapField();
        DrawMapField();
        ColliderUpdateAll();
    }
}

void MapFixEnterMapFld() {
    DrawMapField();

    if (!FadeIsActive()) {
        EnterFloorWorld();
        ModeRequest(&gModeMapFld, 0);
    }
}

void MapFixLeaveEntranceHall() {
    DrawMapField();

    if (FadeIsActive()) {
        return;
    }

    if (gMapRoomState->flags & ROOM_FLAG_ENTER_WORLD) {
        if (gGameState.floor == GetProgressFloor()) {
            gGameState.flags &= ~GAME_FLAG_FRIENDS_SAVED;
            gGameState.progression.friendFlags = (gGameState.progression.friendFlags & ~FRIEND_FLAGS_WORLD) | gGameState.progression.savedFriendFlags;
        }

        if (gMapFloorState.world == 0) {
            gMapFloorState.entrySide = MAP_DOOR_SIDE_UP_RIGHT;
            ModeRequest(&gModeWorldselect, 0);
        } else if (gMapFloorState.world == WORLD_100_ACRE_WOOD) {
            RequestEventMode(EVENT_135_100ACREWOOD_START_RETRY);
        } else if (GetWorldEntryEventId() != MAP_EVENT_NONE) {
            sMapFixEventDelay = 60;
            MapFixSetUpdateAndRun(MapFixWaitWorldEvent);
        } else {
            EnterFloorWorld();
            RequestMapMode();
        }
    } else {
        StoreMapFloorState();
        GoToPreviousFloor();
        RequestMapMode();
    }
}

void MapFixLeaveExitHall() {
    u8 eventId;
    u16 flags;

    DrawMapField();

    if (FadeIsActive()) {
        return;
    }

    if (gMapRoomState->flags & ROOM_FLAG_ENTER_WORLD) {
        if (gMapFloorState.world != WORLD_100_ACRE_WOOD) {
            EnterFloorWorld();
            ModeRequest(&gModeMapFld, 0);
        } else {
            gMapFloorState.entrySide = MAP_DOOR_SIDE_DOWN_LEFT;
            ModeRequest(&gModePooh, 2);
        }

        return;
    }

    eventId = GetFloorEventId();

    if (eventId != MAP_EVENT_NONE) {
        RequestEventMode(eventId);
        return;
    }

    if ((gMapFloorState.flags & FLOOR_FLAG_CLEARED) == 0) {
        flags = gMapFloorState.flags | FLOOR_FLAG_CLEARED;
        gMapFloorState.flags = flags;
        SetFloorJiminyFlags();
        gMapFloorState.progress++;
    }

    StoreMapFloorState();
    GoToNextFloor();
    RequestMapMode();
}

void MapFixWaitMenu() {
    if ((gFieldState->flags & FIELD_FLAG_MENU_OPEN) == 0 && (gMapRoomState->flags & ROOM_FLAG_SAVE_MENU_OPEN) == 0) {
        SetupBg(0, 3, 31, 14);
        SetBgPriority(0, 0);
        MapFixCreateGimmicks(GetMapFixedDef()->gimmicks);
        MapFixSetUpdateAndRun(MapFixMain);
    } else {
        UpdateMapField();
        DrawMapField();
    }
}

void MapFixWaitWalkOut() {
    if ((gMapRoomState->flags & ROOM_FLAG_WALK_OUT) == 0) {
        FadeStartOut(FADE_MODE_ADD_WHITE, 60);
        FadeLock();
        gMapRoomState->flags |= ROOM_FLAG_ENTER_WORLD;
        MapFixSetUpdateAndRun(MapFixLeaveEntranceHall);
    } else {
        UpdateMapField();
        DrawMapField();
    }
}

void MapFixWaitWorldEvent() {
    if (sMapFixEventDelay != 0) {
        sMapFixEventDelay--;
    } else {
        RequestEventMode(GetWorldEntryEventId());
    }
}

void MapFixWaitRoomCreate() {
    if (gFieldState->flags & FIELD_FLAG_EXIT_ROOM) {
        FadeStartOut(FADE_MODE_BLACK, 16);
        MapFixSetUpdateAndRun(MapFixEnterMapFld);
    } else if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) == 0) {
        gBldCnt = 0;
        SetBgPriority(0, 0);
        MapFixCreateGimmicks(GetMapFixedDef()->gimmicks);
        MapFixSetUpdateAndRun(MapFixMain);
    } else {
        UpdateMapField();
        DrawMapField();
        ColliderUpdateAll();
    }
}

void Mode_MapFix_0() {
    MapFixedDef* fixedDef;
    u16 flags;

    gFieldState = EwramAlloc(sizeof(FieldState));
    gMapRoomState = EwramAlloc(sizeof(MapRoomState));
    gMapGmkPlacements = EwramAlloc(sizeof(MapGmkPlacement) * 16);
    UpdateGameWorld();
    SetBgMode0();

    if (gMapFloorState.room == MAP_ROOM_ENTRANCE_HALL) {
        SetupBg(3, 0, 28, 0);
        SetupBg(2, 0, 29, 0);
        SetupBg(1, 0, 30, 0);
        SetupBg(0, 3, 31, 14);
    } else {
        SetupBg(3, 0, 28, 0);
        SetupBg(2, 0, 29, 0);
        SetupBg(1, 2, 30, 0);
        SetupBg(0, 3, 31, 14);
    }

    SetBgPriority(3, 3);
    SetBgPriority(2, 3);
    SetBgPriority(1, 1);
    SetBgPriority(0, 0);
    SetBackdropColor(0, 0, 0);
    InitFieldState();
    ColliderPoolsInit();
    gMapRoomState->flags |= ROOM_FLAG_FIXED_ROOM;
    gMapRoomState->nameId = ROOM_NAME_UNKNOWN_PLACE;
    gMapRoomState->roomType = ROOM_TYPE_PLAIN;

    fixedDef = GetMapFixedDef();
    TaskCreate(&gFieldState->tasks, &gTaskDescMapFix, fixedDef);
    MapFixCreateGimmicks(fixedDef->gimmicks);

    if (gGameState.flags & GAME_FLAG_RIKU) {
        TaskCreate(&gFieldState->tasks2, &gTaskDescFldRiku, NULL);
    } else {
        TaskCreate(&gFieldState->tasks2, &gTaskDescFldSora, NULL);
    }

    TaskCreate(&gFieldState->tasks, &gTaskDescLockon, NULL);

    if (gMapFloorState.flags & FLOOR_FLAG_SHOW_FLOOR_NAME) {
        flags = gMapFloorState.flags & ~FLOOR_FLAG_SHOW_FLOOR_NAME;
        gMapFloorState.flags = flags;
        TaskCreate(&gFieldState->tasks, &gTaskDescMapFloor, NULL);
    }

    if (gMapFloorState.room == MAP_ROOM_TUTORIAL) {
        TaskCreate(&gFieldState->tasks, &gTaskDescMapGmkTutorial, NULL);
    }

    MapFixCreateCharaTasks();

    if (gGameState.fieldResume) {
        MapSetCameraTarget(gGameState.fieldPosition.x, gGameState.fieldPosition.y + gGameState.fieldPosition.z);

        if ((s8)gGameState.mapMenuCursor != -1) {
            gDispCnt &= ~DISPCNT_OBJ_ON;
            TaskCreate(&gFieldState->tasks, &gTaskDescMapMenu, NULL);
            MapFixSetUpdate(MapFixWaitMenu);
        } else {
            MapFixSetUpdate(MapFixMain);
        }
    } else {
        MapSetCameraTarget(gFieldState->spawnX, gFieldState->spawnY);
        MapFixSetUpdate(MapFixMain);
    }

    MapFixSnapCamera();
    ClearFieldResume();
    SeedRandom(gFrameCounter);
    m4aSongNumStartOrContinue(fixedDef->song);

    if (gMapFloorState.flags & FLOOR_FLAG_WARP_IN) {
        flags = gMapFloorState.flags & ~FLOOR_FLAG_WARP_IN;
        gMapFloorState.flags = flags;
        MosaicStartIn(16, 15);
        FadeStartIn(FADE_MODE_BLACK, 16);
    } else {
        FadeStartIn(FADE_MODE_BLACK, 16);
    }
}

void Mode_MapFix_1() {
    sMapFixUpdate();
    UpdatePlayTime();
}

void Mode_MapFix_2() {
    DestroyMapField();
    EwramFree(gFieldState);
    EwramFree(gMapRoomState);
    EwramFree(gMapGmkPlacements);
}

void NewGameSlotMenuLoadFloorTiles(u8 slot, u8 selected, u8 floor) {
    const u8* src;

    slot &= 1;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        if (selected) {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorSelectedTiles[floor];
            } else {
                src = gSaveSlotSoraFloorSelectedTiles[floor];
            }
        } else {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorTiles[floor];
            } else {
                src = gSaveSlotSoraFloorTiles[floor];
            }
        }

        break;
    case LANGUAGE_FRENCH:
        if (selected) {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorSelectedFrenchTiles[floor];
            } else {
                src = gSaveSlotSoraFloorSelectedFrenchTiles[floor];
            }
        } else {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorFrenchTiles[floor];
            } else {
                src = gSaveSlotSoraFloorFrenchTiles[floor];
            }
        }

        break;
    case LANGUAGE_SPANISH:
        if (selected) {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorSelectedSpanishTiles[floor];
            } else {
                src = gSaveSlotSoraFloorSelectedSpanishTiles[floor];
            }
        } else {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorSpanishTiles[floor];
            } else {
                src = gSaveSlotSoraFloorSpanishTiles[floor];
            }
        }

        break;
    case LANGUAGE_ITALIAN:
        if (selected) {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorSelectedItalianTiles[floor];
            } else {
                src = gSaveSlotSoraFloorSelectedItalianTiles[floor];
            }
        } else {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorItalianTiles[floor];
            } else {
                src = gSaveSlotSoraFloorItalianTiles[floor];
            }
        }

        break;
    case LANGUAGE_GERMAN:
    default:
        if (selected) {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorSelectedGermanTiles[floor];
            } else {
                src = gSaveSlotSoraFloorSelectedGermanTiles[floor];
            }
        } else {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                src = gSaveSlotRikuFloorGermanTiles[floor];
            } else {
                src = gSaveSlotSoraFloorGermanTiles[floor];
            }
        }

        break;
    }
#else
    if (selected) {
        if (gGameState.flags & GAME_FLAG_RIKU) {
            src = gSaveSlotRikuFloorSelectedTiles[floor];
        } else {
            src = gSaveSlotSoraFloorSelectedTiles[floor];
        }
    } else {
        if (gGameState.flags & GAME_FLAG_RIKU) {
            src = gSaveSlotRikuFloorTiles[floor];
        } else {
            src = gSaveSlotSoraFloorTiles[floor];
        }
    }
#endif

    RequestDma3Copy((void*)src, (u8*)GetBgCharBase(1) + (slot * 608 + 320), 320);
}

void NewGameSlotMenuLoadLevelTiles(u8 slot, u16 level) {
    u16 digits[4];
    s32 off;
    u16* digit;
    s32 i;

    slot &= 1;
    digits[0] = level / 100;
    digits[1] = level / 10 - digits[0] * 10;
    digits[2] = level - digits[0] * 100 - digits[1] * 10;
    i = 1;
    off = slot * 608 + 32;
    digit = &digits[1];

    while (i <= 2) {
        RequestDma3Copy((void*)&gSaveSlotDigitTiles[*digit * 32], (u8*)GetBgCharBase(1) + off + i * 32, 0x20);
        PORT_BG_DIGIT((u8*)GetBgCharBase(1) + off + i * 32, *digit, &gSaveSlotDigitTiles[*digit * 32]);
        digit++;
        i++;
    }
}

void NewGameSlotMenuLoadTimeTiles(u8 slot, u32 playTime) {
    u16 digits[6];
    s32 off;
    u16* digit;
    s32 i;
    u32 part;

    slot &= 1;
    part = playTime / 3600;
    digits[0] = part / 10;
    digits[1] = part - digits[0] * 10;
    playTime -= part * 3600;
    part = playTime / 60;
    digits[2] = part / 10;
    digits[3] = part - digits[2] * 10;
    playTime -= part * 60;
    digits[4] = playTime / 10;
    digits[5] = playTime - digits[4] * 10;
    i = 0;
    off = slot * 608 + 128;
    digit = &digits[0];

    while (i <= 5) {
        RequestDma3Copy((void*)&gSaveSlotDigitTiles[*digit * 32], (u8*)GetBgCharBase(1) + off + i * 32, 0x20);
        PORT_BG_DIGIT((u8*)GetBgCharBase(1) + off + i * 32, *digit, &gSaveSlotDigitTiles[*digit * 32]);
        digit++;
        i++;
    }
}

s32 NewGameSlotMenuShowSummary(u8 slot) {
    SaveFileSummary* summary = &gGameState.fileSummaries[slot];

    if (summary->level != 0) {
        NewGameSlotMenuLoadLevelTiles(slot, summary->level);
        NewGameSlotMenuLoadTimeTiles(slot, summary->playTime);
        NewGameSlotMenuLoadFloorTiles(slot, FALSE, summary->floor);
        return TRUE;
    }

    NewGameSlotMenuLoadFloorTiles(slot, FALSE, 13);
    return FALSE;
}

void NewGameSlotMenuSelectSlot(u8 slot) {
    SaveFileSummary* summary;
    u8 fileIndex = sNewGameSlotMenuWork->isRiku != 0 ? slot + 2 : slot;
    summary = &gGameState.fileSummaries[fileIndex];

    if (summary->level != 0) {
        NewGameSlotMenuLoadFloorTiles(fileIndex, TRUE, summary->floor);
        sNewGameSlotMenuWork->textSlotCount = LoadTextSlots(GetMapWorldName(summary->world), sNewGameSlotMenuWork->textSlots);

        if (sNewGameSlotMenuWork->isRiku == 0) {
            LoadObjPaletteBank(sNewGameSlotMenuWork->palette8->index, gSaveFloorSoraPalette);
        } else {
            LoadObjPaletteBank(sNewGameSlotMenuWork->palette8->index, gSaveFloorRikuPalette);
        }
    } else {
        NewGameSlotMenuLoadFloorTiles(fileIndex, TRUE, 13);
        sNewGameSlotMenuWork->textSlotCount = 0;
    }

    if (sNewGameSlotMenuWork->selectedSlot == 0) {
        if (sNewGameSlotMenuWork->isRiku != 0) {
            LoadBgMap(1, &gMenuNewMaps[0xC00], 0x800);
        } else {
            LoadBgMap(1, &gMenuNewMaps[0x400], 0x800);
        }

        SetBgScroll(1, 0, (u16)-9);
    } else {
        if (sNewGameSlotMenuWork->isRiku != 0) {
            LoadBgMap(1, &gMenuNewMaps[0x1000], 0x800);
        } else {
            LoadBgMap(1, &gMenuNewMaps[0x800], 0x800);
        }

        SetBgScroll(1, 0, (u16)-6);
    }
}

void NewGameSlotMenuDeselectSlot(u8 slot) {
    u8 fileIndex = sNewGameSlotMenuWork->isRiku != 0 ? slot + 2 : slot;
    SaveFileSummary* summary = &gGameState.fileSummaries[fileIndex];

    if (summary->level != 0) {
        NewGameSlotMenuLoadFloorTiles(fileIndex, FALSE, summary->floor);
    } else {
        NewGameSlotMenuLoadFloorTiles(fileIndex, FALSE, 13);
    }

    sNewGameSlotMenuWork->textSlotCount = 0;
}

void NewGameSlotMenuDraw() {
    s32 spacing;
    s32 slotOffset;

    PORT_BAND(DrawSprite(128, sNewGameSlotMenuWork->y >> 8, gMenuLoadTitleFrames[1], sNewGameSlotMenuWork->tiles2,
        sNewGameSlotMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
    PORT_BAND(DrawSprite(128, sNewGameSlotMenuWork->y2 >> 8, gMenuLoadTitleFrames[2], sNewGameSlotMenuWork->tiles2,
        sNewGameSlotMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));

    if (sNewGameSlotMenuWork->isRiku == 0) {
        DrawSprite(56, 112, gSor1ff00Frames[0], sNewGameSlotMenuWork->tiles4,
            sNewGameSlotMenuWork->palette4, NULL, SPRITE_PRIORITY(1), 80);
        DrawSprite(72, 96, gDona2Fl00Frames[0], sNewGameSlotMenuWork->tiles5, sNewGameSlotMenuWork->palette5, NULL,
            SPRITE_PRIORITY(1) | SPRITE_FLAG_HFLIP, 81);
        DrawSprite(40, 96, gGoofy2Fl00Frames[0], sNewGameSlotMenuWork->tiles6, sNewGameSlotMenuWork->palette6, NULL,
            SPRITE_PRIORITY(1), 81);
    } else {
        DrawSprite(56, 112, gRikuFf00Frames[0], sNewGameSlotMenuWork->tiles7, sNewGameSlotMenuWork->palette7, NULL,
            SPRITE_PRIORITY(1), 81);
    }

    DrawSprite(0, 16, gSaveSlotCharaWinFrame0, sNewGameSlotMenuWork->tiles3, sNewGameSlotMenuWork->palette3, NULL, SPRITE_PRIORITY(1), 90);
    spacing = 45;
    slotOffset = sNewGameSlotMenuWork->selectedSlot * spacing;
    ApproachValueHalf(&sNewGameSlotMenuWork->y3, (sNewGameSlotMenuWork->slotBaseY + slotOffset) << 8);
    DrawSprite(76, sNewGameSlotMenuWork->y3 >> 8, AnimGetGfx(&sNewGameSlotMenuWork->anim),
        sNewGameSlotMenuWork->tiles, sNewGameSlotMenuWork->palette, NULL, 0, 70);
    DrawTextSlots(100, slotOffset + (sNewGameSlotMenuWork->slotBaseY + 22), sNewGameSlotMenuWork->textSlots,
        sNewGameSlotMenuWork->palette8, 50, sNewGameSlotMenuWork->textSlotCount);
    DrawTextSlots(
        (DISPLAY_WIDTH - GetTextSlotsWidth(sNewGameSlotMenuWork->textSlots2, sNewGameSlotMenuWork->textSlotCount2)) / 2, 134,
        sNewGameSlotMenuWork->textSlots2, sNewGameSlotMenuWork->palette9, 50, sNewGameSlotMenuWork->textSlotCount2);
}

void NewGameSlotMenuMoveCursor(NewGameSlotMenuWork* work) {
    u8 prev = work->selectedSlot;

    if (GetKeysRepeat() & DPAD_UP) {
        work->selectedSlot = work->selectedSlot != 0 ? work->selectedSlot - 1 : 1;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysRepeat() & DPAD_DOWN) {
        work->selectedSlot = work->selectedSlot == 0 ? work->selectedSlot + 1 : 0;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (prev != work->selectedSlot) {
        NewGameSlotMenuDeselectSlot(prev);
        NewGameSlotMenuSelectSlot(work->selectedSlot);
    }
}

void NewGameSlotMenuSlideIn(NewGameSlotMenuWork* work) {
    if (work->timer != 0) {
        ApproachValue(&work->y, 0, work->timer);
        ApproachValue(&work->y2, 0x9800, work->timer);
        work->timer--;
    } else {
        work->update = NewGameSlotMenuInput;
    }
}

void NewGameSlotMenuInput(NewGameSlotMenuWork* work) {
    NewGameSlotMenuMoveCursor(work);

    if (GetKeysPressed() & B_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
    } else {
        if (!(GetKeysPressed() & (A_BUTTON | START_BUTTON))) {
            return;
        }

        switch (work->selectedSlot) {
        case 0:
            gGameState.flags &= ~GAME_FLAG_SECOND_FILE;
            break;
        case 1:
            gGameState.flags |= GAME_FLAG_SECOND_FILE;
            break;
        }

        m4aSongNumStart(SONG_SYS_SAVELOAD);
        work->confirmed = TRUE;
    }

    work->timer = 16;
    work->update = NewGameSlotMenuSlideOut;
}

void NewGameSlotMenuSlideOut(NewGameSlotMenuWork* work) {
    if (work->timer != 0) {
        ApproachValue(&work->y, -0x800, work->timer);
        ApproachValue(&work->y2, 0xA000, work->timer);
        work->timer--;
    } else {
        FadeStartOut(FADE_MODE_BLACK, 90);
        work->update = NewGameSlotMenuExit;
    }
}

void NewGameSlotMenuExit(NewGameSlotMenuWork* work) {
    if (FadeIsActive()) {
        return;
    }

    if (work->confirmed) {
#ifdef PLATFORM_PC
        PortUiOpenNewGame(); /* how to start: version, difficulty, HD text, QoL */
#endif
        if (work->isRiku != 0) {
            SetupRikuNewGame();
            RequestEventMode(EVENT_149_RIKU_B12F_OPNING);
        } else {
            SetupSoraNewGame();
            ModeRequestHeapReset(&gModeMovie, MOVIE_OPENING);
        }
    } else {
        ModeRequest(&gModeTitle, 0);
    }
}

void Mode_MenuNew_0() {
    u8 firstUsed;
    u8 secondUsed;

    sNewGameSlotMenuWork = EwramAlloc(sizeof(NewGameSlotMenuWork));
    sNewGameSlotMenuWork->confirmed = FALSE;
    sNewGameSlotMenuWork->isRiku = (gGameState.flags >> 3) & 1;
    sNewGameSlotMenuWork->slotBaseY = 33;
    sNewGameSlotMenuWork->timer = 16;
    sNewGameSlotMenuWork->update = NewGameSlotMenuSlideIn;
    SetBgMode0();
    SetupBg(3, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(0, 0, 30, 0);
    SetBgPriority(3, 3);
    SetBgPriority(1, 0);
    SetBgPriority(0, 0);
#ifdef VERSION_EU
    LoadBgTiles(0, gSaveSlotBgTiles, sizeof(gSaveSlotBgTiles));

    switch (gLanguage) {
    case LANGUAGE_FRENCH:
        RequestDma3Copy((void*)gSaveSlotLabelsFrenchTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsFrenchTiles));
        break;
    case LANGUAGE_SPANISH:
        RequestDma3Copy((void*)gSaveSlotLabelsSpanishTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsSpanishTiles));
        break;
    case LANGUAGE_ITALIAN:
        RequestDma3Copy((void*)gSaveSlotLabelsItalianTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsItalianTiles));
        break;
    case LANGUAGE_GERMAN:
        RequestDma3Copy((void*)gSaveSlotLabelsGermanTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsGermanTiles));
        break;
    }

    LoadBgPalette(3, gMenuNewBgPalette, sizeof(gMenuNewBgPalette));
    LoadBgMap(3, gMenuNewMaps, 0x800);
    LoadBgPalette(1, gMenuNewBgPalette, sizeof(gMenuNewBgPalette));
#else
    LoadBgTiles(3, gSaveSlotBgTiles, sizeof(gSaveSlotBgTiles));
    LoadBgPalette(3, gMenuNewBgPalette, sizeof(gMenuNewBgPalette));
    LoadBgMap(3, gMenuNewMaps, 0x800);
    LoadBgTiles(1, gSaveSlotBgTiles, sizeof(gSaveSlotBgTiles));
    LoadBgPalette(1, gMenuNewBgPalette, sizeof(gMenuNewBgPalette));
    LoadBgTiles(0, gSaveSlotBgTiles, sizeof(gSaveSlotBgTiles));
#endif
    LoadBgPalette(0, gMenuNewBgPalette, sizeof(gMenuNewBgPalette));
    LoadBgMap(0, gMenuNewMsgWinMap, sizeof(gMenuNewMsgWinMap));
    SetBgScroll(0, 0, 0xFFFC);
    sNewGameSlotMenuWork->palette2 = LoadObjPalette(gSaveMenuTitlePalette, sizeof(gSaveMenuTitlePalette));
    sNewGameSlotMenuWork->tiles2 = LoadObjTiles(gMapSaveTitleTiles, sizeof(gMapSaveTitleTiles));
    sNewGameSlotMenuWork->y = -0x800;
    sNewGameSlotMenuWork->y2 = 0xA000;
    sNewGameSlotMenuWork->tiles4 = AllocObjTiles(0x340, gSor1ff00Tiles);
    sNewGameSlotMenuWork->palette4 = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    sNewGameSlotMenuWork->tiles5 = AllocObjTiles(0x280, gDona2Fl00Tiles);
    sNewGameSlotMenuWork->palette5 = LoadObjPalette(gDonald2Palette, sizeof(gDonald2Palette));
    sNewGameSlotMenuWork->tiles6 = AllocObjTiles(0x400, gGoofy2Fl00Tiles);
    sNewGameSlotMenuWork->palette6 = LoadObjPalette(gGoofy2Palette, sizeof(gGoofy2Palette));
    sNewGameSlotMenuWork->tiles7 = AllocObjTiles(0x400, gRikuFf00Tiles);
    sNewGameSlotMenuWork->palette7 = LoadObjPalette(gRikuPalette, sizeof(gRikuPalette));
    sNewGameSlotMenuWork->palette3 = LoadObjPalette(gFileMenuWindowPalette, sizeof(gFileMenuWindowPalette));
    sNewGameSlotMenuWork->tiles3 = LoadObjTiles(gSaveSlotCharaWinTiles, sizeof(gSaveSlotCharaWinTiles));
    sNewGameSlotMenuWork->palette = LoadObjPalette(gFileMenuCursorPalette, sizeof(gFileMenuCursorPalette));
    sNewGameSlotMenuWork->tiles = AllocObjTiles(0x120, gSaveSlotCursorTiles);
    AnimInit(&sNewGameSlotMenuWork->anim, gSaveSlotCursorAnims, gSaveSlotCursorFrames);
    AnimStart(&sNewGameSlotMenuWork->anim, 0, ANIM_FLAG_LOOP);
    sNewGameSlotMenuWork->palette8 = LoadObjPalette(gSaveFloorSoraPalette, sizeof(gSaveFloorSoraPalette));
    sNewGameSlotMenuWork->textSlotCount = 0;
    InitTextSlots(sNewGameSlotMenuWork->textSlots, ARRAY_COUNT(sNewGameSlotMenuWork->textSlots));
    InitTextSlots(sNewGameSlotMenuWork->textSlots2, ARRAY_COUNT(sNewGameSlotMenuWork->textSlots2));
    sNewGameSlotMenuWork->palette9 = LoadObjPalette(gFileMenuCursorPalette, sizeof(gFileMenuCursorPalette));
    sNewGameSlotMenuWork->textSlotCount2 = LoadTextSlots(LOCALIZED_STRING(gNewGameSlotMenuText), sNewGameSlotMenuWork->textSlots2);

    if (sNewGameSlotMenuWork->isRiku != 0) {
        firstUsed = NewGameSlotMenuShowSummary(2);
        secondUsed = NewGameSlotMenuShowSummary(3);
    } else {
        firstUsed = NewGameSlotMenuShowSummary(0);
        secondUsed = NewGameSlotMenuShowSummary(1);
    }

    if (firstUsed) {
        sNewGameSlotMenuWork->selectedSlot = !secondUsed ? 1 : 0;
    } else {
        sNewGameSlotMenuWork->selectedSlot = 0;
    }

    NewGameSlotMenuSelectSlot(sNewGameSlotMenuWork->selectedSlot);
    sNewGameSlotMenuWork->y3 = (sNewGameSlotMenuWork->slotBaseY + sNewGameSlotMenuWork->selectedSlot * 45) << 8;
    FadeStartIn(FADE_MODE_BLACK, 8);
}

void Mode_MenuNew_1() {
    if (sNewGameSlotMenuWork->update != NULL) {
        sNewGameSlotMenuWork->update(sNewGameSlotMenuWork);
    }

    NewGameSlotMenuDraw();
}

void Mode_MenuNew_2() {
    ReleaseObjPalette(sNewGameSlotMenuWork->palette2);
    ReleaseObjTiles(sNewGameSlotMenuWork->tiles2);
    ReleaseObjPalette(sNewGameSlotMenuWork->palette3);
    ReleaseObjTiles(sNewGameSlotMenuWork->tiles3);
    ReleaseObjPalette(sNewGameSlotMenuWork->palette);
    ReleaseObjTiles(sNewGameSlotMenuWork->tiles);
    ReleaseObjPalette(sNewGameSlotMenuWork->palette4);
    ReleaseObjTiles(sNewGameSlotMenuWork->tiles4);
    ReleaseObjPalette(sNewGameSlotMenuWork->palette5);
    ReleaseObjTiles(sNewGameSlotMenuWork->tiles5);
    ReleaseObjPalette(sNewGameSlotMenuWork->palette6);
    ReleaseObjTiles(sNewGameSlotMenuWork->tiles6);
    ReleaseObjPalette(sNewGameSlotMenuWork->palette7);
    ReleaseObjTiles(sNewGameSlotMenuWork->tiles7);
    ReleaseObjPalette(sNewGameSlotMenuWork->palette8);
    FreeTextSlots(sNewGameSlotMenuWork->textSlots, ARRAY_COUNT(sNewGameSlotMenuWork->textSlots));
    ReleaseObjPalette(sNewGameSlotMenuWork->palette9);
    FreeTextSlots(sNewGameSlotMenuWork->textSlots2, ARRAY_COUNT(sNewGameSlotMenuWork->textSlots2));
    EwramFree(sNewGameSlotMenuWork);
}

s32 LoadGameMenuLoadFile(u8 slot) {
    switch (slot) {
    case 0:
        if (SaveRepairFileLarge(0) == SAVE_OK) {
            SaveLoadFileLarge(0);
            return TRUE;
        }

        break;
    case 1:
        if (SaveRepairFileLarge(1) == SAVE_OK) {
            SaveLoadFileLarge(1);
            return TRUE;
        }

        break;
    case 2:
        if (SaveRepairFileSmall(0) == SAVE_OK) {
            SaveLoadFileSmall(0);
            InitRikuDeckForWorld(gGameState.world);
            return TRUE;
        }

        break;
    case 3:
        if (SaveRepairFileSmall(1) == SAVE_OK) {
            SaveLoadFileSmall(1);
            InitRikuDeckForWorld(gGameState.world);
            return TRUE;
        }

        break;
    }

    return FALSE;
}

void LoadGameMenuLoadFloorTiles(u8 slot, u8 selected, u8 floor) {
    const u8* src;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        if (slot <= 1) {
            if (selected) {
                src = gSaveSlotSoraFloorSelectedTiles[floor];
            } else {
                src = gSaveSlotSoraFloorTiles[floor];
            }
        } else {
            if (selected) {
                src = gSaveSlotRikuFloorSelectedTiles[floor];
            } else {
                src = gSaveSlotRikuFloorTiles[floor];
            }
        }

        break;
    case LANGUAGE_FRENCH:
        if (slot <= 1) {
            if (selected) {
                src = gSaveSlotSoraFloorSelectedFrenchTiles[floor];
            } else {
                src = gSaveSlotSoraFloorFrenchTiles[floor];
            }
        } else {
            if (selected) {
                src = gSaveSlotRikuFloorSelectedFrenchTiles[floor];
            } else {
                src = gSaveSlotRikuFloorFrenchTiles[floor];
            }
        }

        break;
    case LANGUAGE_SPANISH:
        if (slot <= 1) {
            if (selected) {
                src = gSaveSlotSoraFloorSelectedSpanishTiles[floor];
            } else {
                src = gSaveSlotSoraFloorSpanishTiles[floor];
            }
        } else {
            if (selected) {
                src = gSaveSlotRikuFloorSelectedSpanishTiles[floor];
            } else {
                src = gSaveSlotRikuFloorSpanishTiles[floor];
            }
        }

        break;
    case LANGUAGE_ITALIAN:
        if (slot <= 1) {
            if (selected) {
                src = gSaveSlotSoraFloorSelectedItalianTiles[floor];
            } else {
                src = gSaveSlotSoraFloorItalianTiles[floor];
            }
        } else {
            if (selected) {
                src = gSaveSlotRikuFloorSelectedItalianTiles[floor];
            } else {
                src = gSaveSlotRikuFloorItalianTiles[floor];
            }
        }

        break;
    case LANGUAGE_GERMAN:
    default:
        if (slot <= 1) {
            if (selected) {
                src = gSaveSlotSoraFloorSelectedGermanTiles[floor];
            } else {
                src = gSaveSlotSoraFloorGermanTiles[floor];
            }
        } else {
            if (selected) {
                src = gSaveSlotRikuFloorSelectedGermanTiles[floor];
            } else {
                src = gSaveSlotRikuFloorGermanTiles[floor];
            }
        }

        break;
    }
#else
    if (slot <= 1) {
        if (selected) {
            src = gSaveSlotSoraFloorSelectedTiles[floor];
        } else {
            src = gSaveSlotSoraFloorTiles[floor];
        }
    } else {
        if (selected) {
            src = gSaveSlotRikuFloorSelectedTiles[floor];
        } else {
            src = gSaveSlotRikuFloorTiles[floor];
        }
    }
#endif

    RequestDma3Copy((void*)src, (u8*)GetBgCharBase(1) + (slot * 608 + 320), 320);
}

void LoadGameMenuLoadLevelTiles(u8 slot, u16 level) {
    u16 digits[4];
    s32 off;
    u16* digit;
    s32 i;

    digits[0] = level / 100;
    digits[1] = level / 10 - digits[0] * 10;
    digits[2] = level - digits[0] * 100 - digits[1] * 10;
    i = 1;
    off = slot * 608 + 32;
    digit = &digits[1];

    while (i <= 2) {
        RequestDma3Copy((void*)&gSaveSlotDigitTiles[*digit * 32], (u8*)GetBgCharBase(1) + off + i * 32, 0x20);
        PORT_BG_DIGIT((u8*)GetBgCharBase(1) + off + i * 32, *digit, &gSaveSlotDigitTiles[*digit * 32]);
        digit++;
        i++;
    }
}

void LoadGameMenuLoadTimeTiles(u8 slot, u32 playTime) {
    u16 digits[6];
    s32 off;
    u16* digit;
    s32 i;
    u32 part;

    part = playTime / 3600;
    digits[0] = part / 10;
    digits[1] = part - digits[0] * 10;
    playTime -= part * 3600;
    part = playTime / 60;
    digits[2] = part / 10;
    digits[3] = part - digits[2] * 10;
    playTime -= part * 60;
    digits[4] = playTime / 10;
    digits[5] = playTime - digits[4] * 10;
    i = 0;
    off = slot * 608 + 128;
    digit = digits;

    while (i <= 5) {
        RequestDma3Copy((void*)&gSaveSlotDigitTiles[*digit * 32], (u8*)GetBgCharBase(1) + off + i * 32, 0x20);
        PORT_BG_DIGIT((u8*)GetBgCharBase(1) + off + i * 32, *digit, &gSaveSlotDigitTiles[*digit * 32]);
        digit++;
        i++;
    }
}

void LoadGameMenuShowSummary(u8 slot) {
    SaveFileSummary* summary = &gGameState.fileSummaries[slot];

    if (summary->level != 0) {
        LoadGameMenuLoadLevelTiles(slot, summary->level);
        LoadGameMenuLoadTimeTiles(slot, summary->playTime);
        LoadGameMenuLoadFloorTiles(slot, FALSE, summary->floor);
    } else {
        LoadGameMenuLoadFloorTiles(slot, FALSE, 13);
    }
}

void LoadGameMenuSelectSlot(u8 slot) {
    SaveFileSummary* summary = &gGameState.fileSummaries[slot];

    if (summary->level != 0) {
        LoadGameMenuLoadFloorTiles(slot, TRUE, summary->floor);
        sLoadGameMenuWork->textSlotCount = LoadTextSlots(GetMapWorldName(summary->world), sLoadGameMenuWork->textSlots);

        if (slot <= 1) {
            LoadObjPaletteBank(sLoadGameMenuWork->palette7->index, gSaveFloorSoraPalette);
        } else {
            LoadObjPaletteBank(sLoadGameMenuWork->palette7->index, gSaveFloorRikuPalette);
        }
    } else {
        LoadGameMenuLoadFloorTiles(slot, TRUE, 13);
        sLoadGameMenuWork->textSlotCount = 0;
    }
}

void LoadGameMenuDeselectSlot(u8 slot) {
    SaveFileSummary* summary = &gGameState.fileSummaries[slot];

    if (summary->level != 0) {
        LoadGameMenuLoadFloorTiles(slot, FALSE, summary->floor);
    } else {
        LoadGameMenuLoadFloorTiles(slot, FALSE, 13);
    }

    sLoadGameMenuWork->textSlotCount = 0;
}

void LoadGameMenuDraw() {
    s32 spacing;
    s32 slotOffset;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y >> 8, gMenuLoadTitleFrames[1], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y2 >> 8, gMenuLoadTitleFrames[2], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        DrawMenuHeader(sLoadGameMenuWork->x >> 8, 0, gMenuLoadTitleFrames[0], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 80);
        break;
    case LANGUAGE_FRENCH:
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y >> 8, gMenuLoadTitleFrenchFrames[1], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y2 >> 8, gMenuLoadTitleFrenchFrames[2], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        DrawMenuHeader(sLoadGameMenuWork->x >> 8, 0, gMenuLoadTitleFrenchFrames[0], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 80);
        break;
    case LANGUAGE_SPANISH:
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y >> 8, gMenuLoadTitleSpanishFrames[1], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y2 >> 8, gMenuLoadTitleSpanishFrames[2], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        DrawMenuHeader(sLoadGameMenuWork->x >> 8, 0, gMenuLoadTitleSpanishFrames[0], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 80);
        break;
    case LANGUAGE_ITALIAN:
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y >> 8, gMenuLoadTitleItalianFrames[1], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y2 >> 8, gMenuLoadTitleItalianFrames[2], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        DrawMenuHeader(sLoadGameMenuWork->x >> 8, 0, gMenuLoadTitleItalianFrames[0], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 80);
        break;
    case LANGUAGE_GERMAN:
    default:
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y >> 8, gMenuLoadTitleGermanFrames[1], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y2 >> 8, gMenuLoadTitleGermanFrames[2], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
        DrawMenuHeader(sLoadGameMenuWork->x >> 8, 0, gMenuLoadTitleGermanFrames[0], sLoadGameMenuWork->tiles2,
            sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 80);
        break;
    }
#else
    PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y >> 8, gMenuLoadTitleFrames[1], sLoadGameMenuWork->tiles2,
        sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
    PORT_BAND(DrawSprite(128, sLoadGameMenuWork->y2 >> 8, gMenuLoadTitleFrames[2], sLoadGameMenuWork->tiles2,
        sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 90));
    DrawMenuHeader(sLoadGameMenuWork->x >> 8, 0, gMenuLoadTitleFrames[0], sLoadGameMenuWork->tiles2,
        sLoadGameMenuWork->palette2, NULL, SPRITE_PRIORITY(3), 80);
#endif

    if (sLoadGameMenuWork->selectedSlot <= 1) {
        DrawSprite(56, 112, gSor1ff00Frames[0], sLoadGameMenuWork->tiles3,
            sLoadGameMenuWork->palette3, NULL, SPRITE_PRIORITY(1), 80);
        DrawSprite(72, 96, gDonaFl00Frames[0], sLoadGameMenuWork->tiles4,
            sLoadGameMenuWork->palette4, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_HFLIP, 81);
        DrawSprite(40, 96, gGoofyFl00Frames[0], sLoadGameMenuWork->tiles5,
            sLoadGameMenuWork->palette5, NULL, SPRITE_PRIORITY(1), 81);
    } else {
        DrawSprite(56, 112, gRikuFf00Frames[0], sLoadGameMenuWork->tiles6, sLoadGameMenuWork->palette6, NULL,
            SPRITE_PRIORITY(1), 81);
    }

    if (sLoadGameMenuWork->showRikuSlots != 0) {
        spacing = 32;
    } else {
        spacing = 45;
    }

    slotOffset = spacing * sLoadGameMenuWork->selectedSlot;
    ApproachValueHalf(&sLoadGameMenuWork->y3, (sLoadGameMenuWork->slotBaseY + slotOffset) << 8);
    DrawSprite(76, sLoadGameMenuWork->y3 >> 8, AnimGetGfx(&sLoadGameMenuWork->anim),
        sLoadGameMenuWork->tiles, sLoadGameMenuWork->palette, NULL, SPRITE_PRIORITY(1), 70);
    DrawTextSlots(100, slotOffset + (sLoadGameMenuWork->slotBaseY + 22), sLoadGameMenuWork->textSlots,
        sLoadGameMenuWork->palette7, 50, sLoadGameMenuWork->textSlotCount);
}

void LoadGameMenuMoveCursor(LoadGameMenuWork* work) {
    u8 old = work->selectedSlot;

    if (GetKeysRepeat() & DPAD_UP) {
        work->selectedSlot = work->selectedSlot != 0 ? work->selectedSlot - 1 : work->lastSlot;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysRepeat() & DPAD_DOWN) {
        work->selectedSlot = work->selectedSlot < work->lastSlot ? work->selectedSlot + 1 : 0;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (old != work->selectedSlot) {
        LoadGameMenuDeselectSlot(old);
        LoadGameMenuSelectSlot(work->selectedSlot);

        switch (work->selectedSlot) {
        case 0:
            if (work->showRikuSlots != 0) {
                LoadBgMap(1, gMenuLoadFourSlotMaps, 0x800);
                SetBgScroll(1, 0, (u16)-1);
            } else {
                LoadBgMap(1, gMenuLoadTwoSlotMaps, 0x800);
                SetBgScroll(1, 0, (u16)-3);
            }

            break;
        case 1:
            if (work->showRikuSlots != 0) {
                LoadBgMap(1, &gMenuLoadFourSlotMaps[0x400], 0x800);
                SetBgScroll(1, 0, (u16)-1);
            } else {
                LoadBgMap(1, &gMenuLoadTwoSlotMaps[0x400], 0x800);
                SetBgScroll(1, 0, 0);
            }

            break;
        case 2:
            LoadBgMap(1, &gMenuLoadFourSlotMaps[0x800], 0x800);
            SetBgScroll(1, 0, (u16)-1);
            break;
        case 3:
            LoadBgMap(1, &gMenuLoadFourSlotMaps[0xC00], 0x800);
            SetBgScroll(1, 0, (u16)-1);
            break;
        }
    }
}

void LoadGameMenuSlideInY(LoadGameMenuWork* work) {
    if (work->timer != 0) {
        ApproachValue(&work->y, 0, work->timer);
        ApproachValue(&work->y2, 0x9800, work->timer);
        work->timer--;
    } else {
        work->timer = 16;
        work->update = LoadGameMenuSlideInX;
    }
}

void LoadGameMenuSlideInX(LoadGameMenuWork* work) {
    if (work->timer != 0) {
        ApproachValue(&work->x, 0, work->timer);
        work->timer--;
    } else {
        work->update = LoadGameMenuInput;
    }
}

void LoadGameMenuInput(LoadGameMenuWork* work) {
    LoadGameMenuMoveCursor(work);

    if (GetKeysPressed() & B_BUTTON) {
        work->timer = 16;
        work->update = LoadGameMenuSlideOutX;
        m4aSongNumStart(SONG_SYS_CLOSE);
    } else if (GetKeysPressed() & (A_BUTTON | START_BUTTON)) {
        if ((u8)LoadGameMenuLoadFile(work->selectedSlot)) {
            switch (work->selectedSlot) {
            case 0:
                gGameState.flags &= ~GAME_FLAG_RIKU;
                gGameState.flags &= ~GAME_FLAG_SECOND_FILE;
                break;
            case 1:
                gGameState.flags &= ~GAME_FLAG_RIKU;
                gGameState.flags |= GAME_FLAG_SECOND_FILE;
                break;
            case 2:
                gGameState.flags |= GAME_FLAG_RIKU;
                gGameState.flags &= ~GAME_FLAG_SECOND_FILE;
                break;
            case 3:
                gGameState.flags |= GAME_FLAG_RIKU;
                gGameState.flags |= GAME_FLAG_SECOND_FILE;
                break;
            }

#ifdef PLATFORM_PC
            PortLog("load-menu: loaded slot=%d flags=%u", work->selectedSlot, (unsigned)gGameState.flags);
#endif
            m4aSongNumStart(SONG_SYS_SAVELOAD);
            work->loaded = TRUE;
            work->timer = 16;
            work->update = LoadGameMenuSlideOutX;
        } else {
            m4aSongNumStart(SONG_SYS_BEEP);
        }
    }
}

void LoadGameMenuSlideOutX(LoadGameMenuWork* work) {
    if (work->timer != 0) {
        ApproachValue(&work->x, -0x8000, work->timer);
        work->timer--;
    } else {
        work->timer = 16;
        work->update = LoadGameMenuSlideOutY;
    }
}

void LoadGameMenuSlideOutY(LoadGameMenuWork* work) {
    if (work->timer != 0) {
        ApproachValue(&work->y, -0x800, work->timer);
        ApproachValue(&work->y2, 0xA000, work->timer);
        work->timer--;
    } else {
        FadeStartOut(FADE_MODE_BLACK, 16);
        work->update = LoadGameMenuExit;
    }
}

void LoadGameMenuExit(LoadGameMenuWork* work) {
#ifdef VERSION_EU
    SaveFileSummary* summary = &gGameState.fileSummaries[work->selectedSlot];
#endif

    if (FadeIsActive()) {
        return;
    }

    if (work->forSioBattle) {
        if (work->loaded) {
            ModeRequest(&gModeSioBattle, SIO_BATTLE_ENTRY_FILE_LOADED);
        } else {
            ModeRequest(&gModeSioBattle, SIO_BATTLE_ENTRY_OPEN);
        }
    } else if (work->loaded) {
#ifdef VERSION_EU
        if (summary->world != WORLD_100_ACRE_WOOD) {
#else
        if (gMapFloorState.world != WORLD_100_ACRE_WOOD) {
#endif
            RequestMapMode();
        } else {
            ModeRequest(&gModePooh, 2);
        }
    } else {
        ModeRequest(&gModeTitle, 0);
    }
}

void Mode_MenuLoad_0(s32 arg) {
    s32 i;

    sLoadGameMenuWork = EwramAlloc(sizeof(LoadGameMenuWork));
    sLoadGameMenuWork->forSioBattle = arg != 0;

    if (sLoadGameMenuWork->forSioBattle) {
        sLoadGameMenuWork->showRikuSlots = 0;
    } else {
#ifdef PLATFORM_PC
        /* Keep Riku saves intact but hidden until access is unlocked through
           Sora's completion or the optional cheat. Existing Riku data alone
           must not bypass this gate. */
        sLoadGameMenuWork->showRikuSlots = PortRikuAvailable(gGameState.flags & GAME_FLAG_SORA_CLEAR);
#else
        sLoadGameMenuWork->showRikuSlots = (gGameState.flags >> 5) & 1;
#endif
    }

#ifdef PLATFORM_PC
    PortLog("load-menu: riku=%d sio=%d flags=%u", sLoadGameMenuWork->showRikuSlots,
            sLoadGameMenuWork->forSioBattle, (unsigned)gGameState.flags);
#endif
    sLoadGameMenuWork->loaded = FALSE;
    sLoadGameMenuWork->selectedSlot = 0;

    if (sLoadGameMenuWork->showRikuSlots != 0) {
        sLoadGameMenuWork->lastSlot = 3;
        sLoadGameMenuWork->slotBaseY = 17;
        sLoadGameMenuWork->y3 = sLoadGameMenuWork->slotBaseY << 8;
    } else {
        sLoadGameMenuWork->lastSlot = 1;
        sLoadGameMenuWork->slotBaseY = 43;
        sLoadGameMenuWork->y3 = sLoadGameMenuWork->slotBaseY << 8;
    }

    sLoadGameMenuWork->timer = 16;
    sLoadGameMenuWork->update = LoadGameMenuSlideInY;
    SetBgMode0();
    SetupBg(3, 0, 28, 0);
    SetupBg(2, 0, 29, 0);
    SetupBg(1, 0, 30, 0);
    SetupBg(0, 3, 31, 0);
#ifdef VERSION_EU
    LoadBgTiles(1, gMenuLoadBgTiles, sizeof(gMenuLoadBgTiles));

    switch (gLanguage) {
    case LANGUAGE_FRENCH:
        RequestDma3Copy(gMenuLoadLabelsFrenchTiles, (u8*)GetBgCharBase(1) + 0xC00, sizeof(gMenuLoadLabelsFrenchTiles));
        break;
    case LANGUAGE_SPANISH:
        RequestDma3Copy((void*)gMenuLoadLabelsSpanishTiles, (u8*)GetBgCharBase(1) + 0xC00, sizeof(gMenuLoadLabelsSpanishTiles));
        break;
    case LANGUAGE_ITALIAN:
        RequestDma3Copy((void*)gMenuLoadLabelsItalianTiles, (u8*)GetBgCharBase(1) + 0xC00, sizeof(gMenuLoadLabelsItalianTiles));
        break;
    case LANGUAGE_GERMAN:
        // @bug Reads past the end of gMenuLoadLabelsGermanTiles.
        RequestDma3Copy((void*)gMenuLoadLabelsGermanTiles, (u8*)GetBgCharBase(1) + 0xC00, 0x800);
        break;
    }

    LoadBgPalette(3, gLoadMenuBgPalettes, sizeof(gLoadMenuBgPalettes));
    LoadBgMap(3, gMenuLoadBackdropMap, sizeof(gMenuLoadBackdropMap));
    LoadBgPalette(2, gLoadMenuBgPalettes, sizeof(gLoadMenuBgPalettes));
    LoadBgMap(2, gMenuLoadCharaWinMap, sizeof(gMenuLoadCharaWinMap));
#else
    LoadBgTiles(3, gMenuLoadBgTiles, sizeof(gMenuLoadBgTiles));
    LoadBgPalette(3, gLoadMenuBgPalettes, sizeof(gLoadMenuBgPalettes));
    LoadBgMap(3, gMenuLoadBackdropMap, sizeof(gMenuLoadBackdropMap));
    LoadBgTiles(2, gMenuLoadBgTiles, sizeof(gMenuLoadBgTiles));
    LoadBgPalette(2, gLoadMenuBgPalettes, sizeof(gLoadMenuBgPalettes));
    LoadBgMap(2, gMenuLoadCharaWinMap, sizeof(gMenuLoadCharaWinMap));
    LoadBgTiles(1, gMenuLoadBgTiles, sizeof(gMenuLoadBgTiles));
#endif
    LoadBgPalette(1, gLoadMenuBgPalettes, sizeof(gLoadMenuBgPalettes));

    if (sLoadGameMenuWork->showRikuSlots != 0) {
        LoadBgMap(1, gMenuLoadFourSlotMaps, 0x800);
        SetBgScroll(1, 0, 0xFFFF);
    } else {
        LoadBgMap(1, gMenuLoadTwoSlotMaps, 0x800);
        SetBgScroll(1, 0, 0xFFFD);
    }

    sLoadGameMenuWork->palette2 = LoadObjPalette(gLoadMenuTitlePalette, sizeof(gLoadMenuTitlePalette));

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        sLoadGameMenuWork->tiles2 = LoadObjTiles(gMenuLoadTitleTiles, sizeof(gMenuLoadTitleTiles));
        break;
    case LANGUAGE_FRENCH:
        sLoadGameMenuWork->tiles2 = LoadObjTiles(gMenuLoadTitleFrenchTiles, sizeof(gMenuLoadTitleFrenchTiles));
        break;
    case LANGUAGE_SPANISH:
        sLoadGameMenuWork->tiles2 = LoadObjTiles(gMenuLoadTitleSpanishTiles, sizeof(gMenuLoadTitleSpanishTiles));
        break;
    case LANGUAGE_ITALIAN:
        sLoadGameMenuWork->tiles2 = LoadObjTiles(gMenuLoadTitleItalianTiles, sizeof(gMenuLoadTitleItalianTiles));
        break;
    case LANGUAGE_GERMAN:
    default:
        sLoadGameMenuWork->tiles2 = LoadObjTiles(gMenuLoadTitleGermanTiles, sizeof(gMenuLoadTitleGermanTiles));
        break;
    }
#else
    sLoadGameMenuWork->tiles2 = LoadObjTiles(gMenuLoadTitleTiles, sizeof(gMenuLoadTitleTiles));
#endif
    sLoadGameMenuWork->y = -0x800;
    sLoadGameMenuWork->y2 = 0xA000;
    sLoadGameMenuWork->x = -0x8000;
    sLoadGameMenuWork->tiles3 = AllocObjTiles(0x340, gSor1ff00Tiles);
    sLoadGameMenuWork->palette3 = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    sLoadGameMenuWork->tiles4 = AllocObjTiles(0x280, gDonaFl00Tiles);
    sLoadGameMenuWork->palette4 = LoadObjPalette(gDonaldPalette, sizeof(gDonaldPalette));
    sLoadGameMenuWork->tiles5 = AllocObjTiles(0x400, gGoofyFl00Tiles);
    sLoadGameMenuWork->palette5 = LoadObjPalette(gGoofyPalette, sizeof(gGoofyPalette));
    sLoadGameMenuWork->tiles6 = AllocObjTiles(0x400, gRikuFf00Tiles);
    sLoadGameMenuWork->palette6 = LoadObjPalette(gRikuPalette, sizeof(gRikuPalette));
    sLoadGameMenuWork->palette = LoadObjPalette(gFileMenuCursorPalette, sizeof(gFileMenuCursorPalette));
    sLoadGameMenuWork->tiles = AllocObjTiles(0x120, gSaveSlotCursorTiles);
    AnimInit(&sLoadGameMenuWork->anim, gSaveSlotCursorAnims, gSaveSlotCursorFrames);
    AnimStart(&sLoadGameMenuWork->anim, 0, ANIM_FLAG_LOOP);
    sLoadGameMenuWork->palette7 = LoadObjPalette(gSaveFloorSoraPalette, sizeof(gSaveFloorSoraPalette));
    sLoadGameMenuWork->textSlotCount = 0;
    InitTextSlots(sLoadGameMenuWork->textSlots, ARRAY_COUNT(sLoadGameMenuWork->textSlots));

    for (i = 0; i < 4; i++) {
        LoadGameMenuShowSummary(i);
    }

    LoadGameMenuSelectSlot(sLoadGameMenuWork->selectedSlot);
    FadeStartIn(FADE_MODE_BLACK, 16);
}

void Mode_MenuLoad_1() {
    if (sLoadGameMenuWork->update != NULL) {
        sLoadGameMenuWork->update(sLoadGameMenuWork);
    }

    LoadGameMenuDraw();
}

void Mode_MenuLoad_2() {
    ReleaseObjPalette(sLoadGameMenuWork->palette2);
    ReleaseObjTiles(sLoadGameMenuWork->tiles2);
    ReleaseObjPalette(sLoadGameMenuWork->palette);
    ReleaseObjTiles(sLoadGameMenuWork->tiles);
    ReleaseObjPalette(sLoadGameMenuWork->palette3);
    ReleaseObjTiles(sLoadGameMenuWork->tiles3);
    ReleaseObjPalette(sLoadGameMenuWork->palette4);
    ReleaseObjTiles(sLoadGameMenuWork->tiles4);
    ReleaseObjPalette(sLoadGameMenuWork->palette5);
    ReleaseObjTiles(sLoadGameMenuWork->tiles5);
    ReleaseObjPalette(sLoadGameMenuWork->palette6);
    ReleaseObjTiles(sLoadGameMenuWork->tiles6);
    ReleaseObjPalette(sLoadGameMenuWork->palette7);
    FreeTextSlots(sLoadGameMenuWork->textSlots, ARRAY_COUNT(sLoadGameMenuWork->textSlots));
    EwramFree(sLoadGameMenuWork);
}

void MenuMsgWaitMessage(MenuMsgWork* work) {
    if (!IsMessageWindowOpen()) {
        if (work->toTitle == 0) {
            BackdropFadeStartOut(1, 16);
            FadeStartOut(FADE_MODE_WHITE, 16);
        } else {
            BackdropFadeStartOut(0, 16);
            FadeStartOut(FADE_MODE_BLACK, 16);
        }

        work->update = MenuMsgWaitFade;
    }
}

void MenuMsgWaitFade(MenuMsgWork* work) {
    if (!FadeIsActive()) {
        if (work->toTitle == 0) {
            ModeRequest(&gModeCopyright1, 0);
        } else {
            ModeRequest(&gModeTitle, 0);
        }
    }
}

void Mode_MenuMsg_0(s32 arg) {
    sMenuMsgWork = EwramAlloc(sizeof(MenuMsgWork));
    sMenuMsgWork->toTitle = arg;
    SetBgMode0();
    TaskPoolInit(&sMenuMsgWork->tasks, 1);

    if (sMenuMsgWork->toTitle == 0) {
        CreateSysmsgwinTask(&sMenuMsgWork->tasks, CARD_MSG_SAVE_DATA_LOST);
        BackdropFadeReset();
        BackdropFadeSetColor(0, 0, 0);
        BackdropFadeStartIn(1, 16);
        FadeStartIn(FADE_MODE_WHITE, 16);
        FadeLock();
    } else {
        CreateSysmsgwinTask(&sMenuMsgWork->tasks, CARD_MSG_QUICK_SAVE_COMPLETE);
        BackdropFadeReset();
        BackdropFadeSetColor(0, 0, 0);
        BackdropFadeStartIn(0, 1);
        FadeStartIn(FADE_MODE_BLACK, 1);
        FadeLock();
    }

    sMenuMsgWork->update = MenuMsgWaitMessage;
}

void Mode_MenuMsg_1() {
    sMenuMsgWork->update(sMenuMsgWork);
    TaskPoolUpdate(&sMenuMsgWork->tasks);
    TaskPoolDraw(&sMenuMsgWork->tasks);
    BackdropFadeUpdate();
}

void Mode_MenuMsg_2() {
    TaskPoolDestroy(&sMenuMsgWork->tasks);
    EwramFree(sMenuMsgWork);
}

void Task_MapRnd_0(MapRndWork* work) {
    MapRoomDef* roomDef = gMapRoomDefs[gMapFloorState.world];
    s32 i;

    TaskPoolInit(&work->tasks, 4);
    LoadBgTiles(3, roomDef->tiles, roomDef->tilesSize);
    LoadBgTiles(2, roomDef->tiles, roomDef->tilesSize);
    LoadBgTiles(1, roomDef->tiles2, roomDef->tilesSize2);
    LoadBgPalette(3, roomDef->palette, roomDef->paletteSize);
    LoadBgPalette(2, roomDef->palette, roomDef->paletteSize);
    LoadBgPalette(1, roomDef->palette, roomDef->paletteSize);
    gMapRoomState->cols = GetRandomMapWidth();
    gMapRoomState->rows = 64;
    gFieldState->tileCols = gMapRoomState->cols * 4;
    gFieldState->tileRows = gMapRoomState->rows * 2;
    MapGenerateRoom(gMapRoomState->cols, gMapRoomState->rows);

    for (i = 0; i < 4; i++) {
        MapDoor* door = GetMapDoor(i);

        if (door->flags & DOOR_FLAG_PRESENT) {
            if ((door->flags & DOOR_FLAG_SEALED) == 0) {
                TaskCreate(&work->tasks, &gTaskDescMapDoor, door);
            }
        }
    }
}

s32 Task_MapRnd_1(MapRndWork* work) {
    MapUpdateCamera(gFieldState->x2, gFieldState->y2);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void Task_MapRnd_2(MapRndWork* work) {
    TaskPoolDraw(&work->tasks);
}

void Task_MapRnd_3(MapRndWork* work) {
    TaskPoolDestroy(&work->tasks);
    MapFreeRoom();
}

void MapFixInitColliders(MapFixWork* work, MapFixedCollider* collider) {
    s32 i;

    work->colliderCount = 0;

    if (collider != NULL) {
        i = 0;

        do {
            if (collider->radius != 0) {
                ColliderInit(&work->colliders[i], COLLIDER_TYPE_SOLID, collider->radius, 0xA0);
                ColliderSetPosition(&work->colliders[i], collider->x, collider->y, 0);
                work->colliderCount++;
            } else {
                break;
            }

            collider++;
            i++;
        } while (i < 5);
    }
}

void Task_MapFix_0(MapFixWork* work, MapFixedDef* def) {
    FldObj stair;

    switch (gMapFloorState.entrySide) {
    case MAP_DOOR_SIDE_UP_RIGHT:
        gFieldState->spawnAngle = FLD_ANGLE_DOWN_LEFT;
        gFieldState->spawnX = def->stairX - 0xC00;
        gFieldState->spawnY = def->stairY + 0x800;
        break;
    case MAP_DOOR_SIDE_DOWN_LEFT:
        gFieldState->spawnAngle = FLD_ANGLE_UP_RIGHT;
        gFieldState->spawnX = def->stair2X + 0xC00;
        gFieldState->spawnY = def->stair2Y - 0x800;
        break;
    default:
        gFieldState->spawnAngle = FLD_ANGLE_UP_RIGHT;
        gFieldState->spawnX = def->spawnX;
        gFieldState->spawnY = def->spawnY;
        break;
    }

#ifdef VERSION_EU
    work->bg1MapLoaded = FALSE;
    work->bg2MapLoaded = FALSE;
    work->bg3MapLoaded = FALSE;

    if (def->rawTiles == 0) {
        LoadBgTilesLz77(3, def->tiles);
    } else {
        LoadBgTiles(3, def->tiles, def->tilesSize);
    }
#else
    LoadBgTiles(3, def->tiles, def->tilesSize);
#endif
    LoadBgPalette(3, def->palette, def->paletteSize);
#ifdef VERSION_EU
    SetBgMapBlocksLz77(3, def->map3, def->mapWidth, def->mapHeight);
    work->bg3MapLoaded = TRUE;
#else
    SetBgMapBlocks(3, def->map3, def->mapWidth, def->mapHeight);
#endif

    if (def->map2 != NULL) {
#ifdef VERSION_EU
        if (def->rawTiles == 0) {
            LoadBgTilesLz77(2, def->tiles);
        } else {
            LoadBgTiles(2, def->tiles, def->tilesSize);
        }
#else
        LoadBgTiles(2, def->tiles, def->tilesSize);
#endif
        LoadBgPalette(2, def->palette, def->paletteSize);
#ifdef VERSION_EU
        SetBgMapBlocksLz77(2, def->map2, def->mapWidth, def->mapHeight);
        work->bg2MapLoaded = TRUE;
#else
        SetBgMapBlocks(2, def->map2, def->mapWidth, def->mapHeight);
#endif
    } else {
        DisableBg(2);
    }

    if (def->map != NULL) {
#ifdef VERSION_EU
        if (def->rawTiles == 0) {
            LoadBgTilesLz77(1, def->tiles2);
        } else {
            LoadBgTiles(1, def->tiles2, def->tilesSize2);
        }
#else
        LoadBgTiles(1, def->tiles2, def->tilesSize2);
#endif
        LoadBgPalette(1, def->palette, def->paletteSize);
#ifdef VERSION_EU
        SetBgMapBlocksLz77(1, def->map, def->mapWidth, def->mapHeight);
        work->bg1MapLoaded = TRUE;
#else
        SetBgMapBlocks(1, def->map, def->mapWidth, def->mapHeight);
#endif
    } else {
        DisableBg(1);
    }

    gFieldState->tileCols = def->mapWidth * 32;
    gFieldState->tileRows = def->mapHeight * 32;
    gMapRoomState->cols = gFieldState->tileCols / 4;
    gMapRoomState->rows = gFieldState->tileRows / 2;
    MapFixInitCells(def);
    TaskPoolInit(&work->tasks, 2);
    stair.fieldPosition.x = def->stairX;
    stair.fieldPosition.y = def->stairY;
    stair.angle = FLD_ANGLE_UP_RIGHT;
    TaskCreate(&work->tasks, &gTaskDescMapStair, &stair);

    if (def->stair2X != 0 || def->stair2Y != 0) {
        stair.fieldPosition.x = def->stair2X;
        stair.fieldPosition.y = def->stair2Y;
        stair.angle = FLD_ANGLE_DOWN_LEFT;
        TaskCreate(&work->tasks, &gTaskDescMapStair, &stair);
    }

    MapFixInitColliders(work, def->colliders);
}

s32 Task_MapFix_1(MapFixWork* work) {
    s32 tx = gFieldState->x2 - 0x7800;
    s32 ty = gFieldState->y2 - 0x6000;

    gFieldState->x += (tx - gFieldState->x) / 8;
    gFieldState->y += (ty - gFieldState->y) / 8;

    if (gFieldState->x < 0) {
        gFieldState->x = 0;
    }

    if (gFieldState->y < 0) {
        gFieldState->y = 0;
    }

    if (gFieldState->x + 0xF000 > gFieldState->tileCols << 11) {
        gFieldState->x = (gFieldState->tileCols << 11) - 0xF000;
    }

    if (gFieldState->y + 0xA000 > gFieldState->tileRows << 11) {
        gFieldState->y = (gFieldState->tileRows << 11) - 0xA000;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void Task_MapFix_2(MapFixWork* work) {
    ScrollBgMapTo(3, gFieldState->x >> 8, gFieldState->y >> 8);
    ScrollBgMapTo(2, gFieldState->x >> 8, gFieldState->y >> 8);

    if ((gMapRoomState->flags & ROOM_FLAG_BG1_FROZEN) == 0) {
        ScrollBgMapTo(1, gFieldState->x >> 8, gFieldState->y >> 8);
    }

    TaskPoolDraw(&work->tasks);
}

void Task_MapFix_3(MapFixWork* work) {
    s32 i;

    for (i = 0; i < work->colliderCount; i++) {
        ColliderUnregister(&work->colliders[i]);
    }

#ifdef VERSION_EU
    if (work->bg3MapLoaded) {
        FreeBgDecompressedMap(3);
    }

    if (work->bg2MapLoaded) {
        FreeBgDecompressedMap(2);
    }

    if (work->bg1MapLoaded) {
        FreeBgDecompressedMap(1);
    }
#endif

    TaskPoolDestroy(&work->tasks);
    MapFixFreeCells();
}

void MapDoorShowOpen(MapDoorWork* work) {
    UpdateSpriteFrameTiles(work->tiles, work->sprite, work->openSrc);
    UpdateSpriteFrameTiles(work->tiles2, work->sprite2, work->openSrc2);
}

void MapDoorShowClosed(MapDoorWork* work) {
    UpdateSpriteFrameTiles(work->tiles, work->sprite, work->closedSrc);
    UpdateSpriteFrameTiles(work->tiles2, work->sprite2, work->closedSrc2);
}

u8 MapDoorWaitHit(MapDoorWork* work) {
    MapDoor* door = work->door;
    FldObj* obj = &work->obj;

    if (!(gFieldState->flags & FIELD_FLAG_MENU_OPEN) && !(gMapRoomState->flags & (ROOM_FLAG_ENEMY_STRUCK | ROOM_FLAG_TUTORIAL_ACTIVE)) &&
        (u8)(door->room + 3) > 1 && (door->flags & (DOOR_FLAG_OPEN | DOOR_FLAG_EVENT)) != (DOOR_FLAG_OPEN | DOOR_FLAG_EVENT) &&
        IsHitByMapAttack(&obj->fieldPosition, 0, 8) && !(gFieldState->flags & FIELD_FLAG_PLAYER_JUMPING) &&
        gFieldState->actor.fieldPosition.z == gFieldState->actor.fieldPosition.ground) {
        TaskPool* pool;

        m4aSongNumStart(SONG_SND_220);
        pool = &work->tasks;
        TaskCreate(pool, &gTaskDescMapSpark, obj);
        gMapRoomState->flags |= ROOM_FLAG_ATTACK_HIT;
        gFieldState->lockonTarget = obj;
        gMapRoomState->door = obj;
        work->triggered = TRUE;
        work->update = MapDoorWaitCard;
        gMapRoomState->doorRoom = door->room;
        gMapRoomState->doorSide = door->side;
        FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
        FadeSetPaletteExcluded(work->palette2->index + 16, TRUE);
        TaskCreate(pool, &gTaskDescRoomcreate, NULL);
    }

    return 1;
}

u8 MapDoorWaitCard(MapDoorWork* work) {
    MapDoor* door = work->door;
    void* card = GetSelectedMapCard();

    if (card != NULL) {
        if (door->flags & DOOR_FLAG_EVENT) {
            CreateMapRoom(door->room, NULL);
        } else {
            CreateMapRoom(door->room, card);
        }

        work->update = MapDoorWaitOpen;
    }

    if (!(gFieldState->flags & FIELD_FLAG_ROOM_CREATE)) {
        gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
        FadeSetPaletteExcluded(work->palette->index + 16, FALSE);
        FadeSetPaletteExcluded(work->palette2->index + 16, FALSE);
        work->update = MapDoorWaitHit;
    }

    return 1;
}

u8 MapDoorWaitOpen(MapDoorWork* work) {
    MapDoor* door = work->door;
    u16 doorFlags;

    if (gFieldState->flags & FIELD_FLAG_DOOR_OPENED) {
        MapDoorShowOpen(work);
        doorFlags = door->flags | DOOR_FLAG_OPEN;
        door->flags = doorFlags;
        work->update = MapDoorIdle;
    }

    return 1;
}

u8 MapDoorIdle(MapDoorWork* work) {
    return 1;
}

#ifdef PLATFORM_PC
void PortBotDoor(int side, FldPos* pos);
#endif
void Task_MapDoor_0(MapDoorWork* work, MapDoor* door) {
    FldObj* obj = &work->obj;
    FldPos* pos = &obj->fieldPosition;
    const MapDoorGfx* doorGfx = &gWorldMapDoorGfx[gMapFloorState.world];

    work->door = door;
    work->triggered = FALSE;
    work->visible = TRUE;

    switch (door->side) {
    case MAP_DOOR_SIDE_UP_RIGHT:
        work->sprite = gMapDoorSide0Frame0;
        work->openSrc = doorGfx->side0Open;
        work->closedSrc = doorGfx->side0Closed;
        obj->angle = FLD_ANGLE_DOWN_LEFT;
        work->obj.fieldPosition.x = (door->cellX << 5) + 16;
        obj->fieldPosition.y = (door->cellY << 4) + 10;
        break;
    case MAP_DOOR_SIDE_DOWN_LEFT:
        work->sprite = gMapDoorSide1Frame0;
        work->openSrc = doorGfx->side1Open;
        work->closedSrc = doorGfx->side1Closed;
        obj->angle = FLD_ANGLE_UP_RIGHT;
        work->obj.fieldPosition.x = (door->cellX << 5) + 16;
        obj->fieldPosition.y = (door->cellY << 4) + 6;
        break;
    case MAP_DOOR_SIDE_DOWN_RIGHT:
        work->sprite = gMapDoorSide2Frame0;
        work->openSrc = doorGfx->side2Open;
        work->closedSrc = doorGfx->side2Closed;
        obj->angle = FLD_ANGLE_UP_LEFT;
        work->obj.fieldPosition.x = (door->cellX << 5) + 16;
        obj->fieldPosition.y = (door->cellY << 4) + 6;
        break;
    case MAP_DOOR_SIDE_UP_LEFT:
        work->sprite = gMapDoorSide3Frame0;
        work->openSrc = doorGfx->side3Open;
        work->closedSrc = doorGfx->side3Closed;
        obj->angle = FLD_ANGLE_DOWN_RIGHT;
        work->obj.fieldPosition.x = (door->cellX << 5) + 16;
        obj->fieldPosition.y = (door->cellY << 4) + 10;
        break;
    }

    pos->x <<= 8;
    pos->y <<= 8;
    pos->z = 0;
    pos->z = pos->ground = GetFldPosFloor(pos);
    pos->y -= pos->z;
    obj->height = 32;
    obj->kind = FLD_OBJ_KIND_FIXTURE;
#ifdef PLATFORM_PC
    PortBotDoor(door->side, pos); /* port_testbot.c: where the door can be struck */
#endif
    work->tiles = AllocSpriteFrameTiles(0x400);
    work->palette = LoadObjPalette(doorGfx->palette, 32);
    work->palette2 = LoadObjPalette(gMapDoorEmblemPalette, sizeof(gMapDoorEmblemPalette));
    work->tiles2 = AllocSpriteFrameTiles(0x100);

    switch (door->side) {
    case MAP_DOOR_SIDE_UP_RIGHT:
    case MAP_DOOR_SIDE_DOWN_LEFT:
        work->sprite2 = gMapDoorEmblemSide01Frame0;
        work->openSrc2 = gMapDoorEmblemTiles[0][1];
        work->closedSrc2 = gMapDoorEmblemTiles[1][1];
        break;
    case MAP_DOOR_SIDE_DOWN_RIGHT:
    case MAP_DOOR_SIDE_UP_LEFT:
        work->sprite2 = gMapDoorEmblemSide23Frame0;
        work->openSrc2 = gMapDoorEmblemTiles[0][0];
        work->closedSrc2 = gMapDoorEmblemTiles[1][0];
        break;
    }

    if (door->flags & DOOR_FLAG_OPEN) {
        work->update = MapDoorWaitHit;
        MapDoorShowOpen(work);
    } else {
        work->update = MapDoorWaitHit;
        MapDoorShowClosed(work);
    }

    TaskPoolInit(&work->tasks, 2);
}

s32 Task_MapDoor_1(MapDoorWork* work) {
    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        if (!work->triggered) {
            work->visible = FALSE;
        }
    } else {
        work->triggered = FALSE;
        work->visible = TRUE;
    }

    if (work->update != NULL) {
        if (work->update(work) == 0) {
            return 0;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void Task_MapDoor_2(MapDoorWork* work) {
    MapDoor* door = work->door;
    u16 sx;
    u16 sy;
    u16 priority;
    u16 drawFlags;
    s32 pixelY;

    if (work->visible == TRUE) {
        sx = (work->obj.fieldPosition.x >> 8) - (gFieldState->x >> 8);
        pixelY = work->obj.fieldPosition.y >> 8;
        sy = pixelY + (work->obj.fieldPosition.z >> 8) - (gFieldState->y >> 8);

        switch (door->side) {
        case MAP_DOOR_SIDE_UP_RIGHT:
        case MAP_DOOR_SIDE_UP_LEFT:
            priority = -0xFE4 - (work->obj.fieldPosition.y >> 8) * 4;
            break;
        case MAP_DOOR_SIDE_DOWN_LEFT:
        case MAP_DOOR_SIDE_DOWN_RIGHT:
            priority = -0x1024 - pixelY * 4;
            break;
        default:
            priority = 0;
            break;
        }

        drawFlags = SPRITE_PRIORITY(2);
        DrawSprite(sx, sy, NULL, work->tiles, work->palette, NULL, drawFlags, priority);

        if (door->flags & DOOR_FLAG_EVENT) {
            switch (door->side) {
            case MAP_DOOR_SIDE_UP_RIGHT:
            case MAP_DOOR_SIDE_DOWN_RIGHT:
                DrawSprite(sx, sy, NULL, work->tiles2, work->palette2, NULL, drawFlags, priority - 1);
                break;
            case MAP_DOOR_SIDE_DOWN_LEFT:
            case MAP_DOOR_SIDE_UP_LEFT:
                DrawSprite(sx, sy, NULL, work->tiles2, work->palette2, NULL, drawFlags, priority - 1);
                break;
            }
        }

        TaskPoolDraw(&work->tasks);
    }
}

void Task_MapDoor_3(MapDoorWork* work) {
#ifdef PLATFORM_PC
    PortBotDoor(work->door->side, NULL);
#endif
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette2);
    TaskPoolDestroy(&work->tasks);
}

enum MapMenuItem {
    MAP_MENU_ITEM_DECK,
    MAP_MENU_ITEM_MAP,
    MAP_MENU_ITEM_MAP_CARDS,
    MAP_MENU_ITEM_WORLDS,
    MAP_MENU_ITEM_STATUS,
    MAP_MENU_ITEM_JOURNAL,
    MAP_MENU_ITEM_QUICK_SAVE
};

enum MapConfirmCursor {
    MAP_CONFIRM_CURSOR_NONE,
    MAP_CONFIRM_CURSOR_YES,
    MAP_CONFIRM_CURSOR_NO
};

void MapMenuSetPanelPalettesExcluded(MapMenuWork* work, u8 excluded) {
    s32 i;

    // @bug Palettes the menu has not loaded are NULL (NULL read).
    FadeSetPaletteExcluded(BIOS_PTR(work->palette3)->index + 0x10, excluded);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette8)->index + 0x10, excluded);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette4)->index + 0x10, excluded);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette5)->index + 0x10, excluded);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 0x10, excluded);

    for (i = 0; i < 3; i++) {
        if (work->palette9[i] != NULL) {
            FadeSetPaletteExcluded(work->palette9[i]->index + 0x10, excluded);
        }
    }
}

void MapMenuSetCharaPalettesExcluded(MapMenuWork* work, u8 excluded) {
    // @bug Palettes the menu has not loaded are NULL (NULL read).
    FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 0x10, excluded);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette6)->index + 0x10, excluded);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette7)->index + 0x10, excluded);
}

void MapMenuWriteDigits3(ObjTiles* tiles, u8 offset, u16 value) {
    u16 digits[3];
    u16* digit;
    s32 i;

    digits[0] = value / 100;
    digits[1] = value / 10 - digits[0] * 10;
    digits[2] = value - digits[0] * 100 - digits[1] * 10;

    for (i = 0, digit = digits; i < 3; i++) {
        RequestDma3Copy((void*)&gMapMenuDigitTiles[*digit * 32], (void*)(OBJ_VRAM0 + (tiles->index + offset + i) * 32), 0x20);
        digit++;
    }
}

void MapMenuWriteDigits5(ObjTiles* tiles, u8 offset, u32 value) {
    u16 digits[5];
    u16* digit;
    s32 i;

    digits[0] = value / 10000;
    digits[1] = value / 1000 - digits[0] * 10;
    digits[2] = value / 100 - digits[0] * 100 - digits[1] * 10;
    digits[3] = value / 10 - digits[0] * 1000 - digits[1] * 100 - digits[2] * 10;
    digits[4] = value - digits[0] * 10000 - digits[1] * 1000 - digits[2] * 100 - digits[3] * 10;

    for (i = 0, digit = digits; i < 5; i++) {
        RequestDma3Copy((void*)&gMapMenuDigitTiles[*digit * 32], (void*)(OBJ_VRAM0 + (tiles->index + offset + i) * 32), 0x20);
        digit++;
    }
}

void MapMenuInitConfirm(MapMenuWork* work) {
    TextSlot* promptSlots;
    TextSlot* yesSlots;
    TextSlot* noSlots;

    LoadBgTiles(0, gConfirmWinTiles, sizeof(gConfirmWinTiles));
    LoadBgMap(0, gConfirmWinMap, sizeof(gConfirmWinMap));
    LoadPalette(gCard00Palette, (void*)(BG_PLTT + 15 * PLTT_SIZE_4BPP), sizeof(gCard00Palette));
    FadeSetPaletteExcluded(15, TRUE);
    SetBgScroll(0, 0, 0);
    work->confirmPalette = LoadTextPalette(1);
    promptSlots = work->textSlots2;
#ifdef VERSION_EU
    InitTextSlots(promptSlots, ARRAY_COUNT(work->textSlots2));
    yesSlots = work->textSlots3;
    InitTextSlots(yesSlots, ARRAY_COUNT(work->textSlots3));
    noSlots = work->textSlots4;
    InitTextSlots(noSlots, ARRAY_COUNT(work->textSlots4));
    work->textSlotCount2 = LoadTextSlots(GetLocalizedString(&gMapQuickSaveConfirmTextByLanguage), promptSlots);
    work->textSlotCount3 = LoadTextSlots(GetLocalizedString(&gYesChoiceTextByLanguage), yesSlots);
    work->textSlotCount4 = LoadTextSlots(GetLocalizedString(&gNoChoiceTextByLanguage), noSlots);
#else
    InitTextSlots(promptSlots, ARRAY_COUNT(work->textSlots2));
    yesSlots = work->textSlots3;
    InitTextSlots(yesSlots, ARRAY_COUNT(work->textSlots3));
    noSlots = work->textSlots4;
    InitTextSlots(noSlots, ARRAY_COUNT(work->textSlots4));
    work->textSlotCount2 = LoadTextSlots(gMapQuickSaveConfirmText, promptSlots);
    work->textSlotCount3 = LoadTextSlots(gYesChoiceText, yesSlots);
    work->textSlotCount4 = LoadTextSlots(gNoChoiceText, noSlots);
#endif
}

void MapMenuFreeConfirm(MapMenuWork* work) {
    FadeSetPaletteExcluded(15, FALSE);
    DisableBg(0);
    ReleaseObjPalette(work->confirmPalette);
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    FreeTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
}

s32 MapMenuOpen(MapMenuWork* work) {
    work->palette2 = LoadObjPalette(gMapMenuBarPalette, sizeof(gMapMenuBarPalette));
    work->tiles2 = LoadObjTiles(gMapMenuBarsTiles, sizeof(gMapMenuBarsTiles));
    work->y = -0x800;
    work->y2 = 0xA000;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        work->tiles8 = AllocObjTiles(0x400, gRikuFf00Tiles);
    } else {
        work->tiles8 = AllocObjTiles(0x340, gSor1ff00Tiles);
    }

    work->tiles7 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette7 = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    work->playerStartX = gFieldState->actor.fieldPosition.x - gFieldState->x;
    work->playerStartY = gFieldState->actor.fieldPosition.y + gFieldState->actor.fieldPosition.z - gFieldState->y;
    work->x8 = work->playerStartX;
    work->y4 = work->playerStartY;
    work->cursor = gGameState.mapMenuCursor;
    work->confirmCursor = MAP_CONFIRM_CURSOR_NONE;
    work->cursorVisible = FALSE;
    work->panelsVisible = FALSE;
    work->steps = work->reopened ? 1 : 16;
    work->update = MapMenuSlideInY;
    MapMenuSetCharaPalettesExcluded(work, TRUE);

    if (!work->reopened) {
        FadeToAmount(FADE_MODE_BLACK, 16, 16);
    }

    return 1;
}

s32 MapMenuSlideInY(MapMenuWork* work) {
    if (work->steps != 0) {
        ApproachValue(&work->y, 0, work->steps);
        ApproachValue(&work->y2, 0x9800, work->steps);
        work->steps--;
    } else {
        s32 i;

        gMapRoomState->flags |= ROOM_FLAG_HIDE_PLAYER;
        gFieldState->flags |= FIELD_FLAG_HIDE_ENEMIES;

#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            work->tiles5 = LoadObjTiles(gMapMenuStatusTiles, sizeof(gMapMenuStatusTiles));
            break;
        case LANGUAGE_FRENCH:
            work->tiles5 = LoadObjTiles(gMapMenuStatusFrenchTiles, sizeof(gMapMenuStatusFrenchTiles));
            break;
        case LANGUAGE_SPANISH:
            work->tiles5 = LoadObjTiles(gMapMenuStatusSpanishTiles, sizeof(gMapMenuStatusSpanishTiles));
            break;
        case LANGUAGE_ITALIAN:
            work->tiles5 = LoadObjTiles(gMapMenuStatusItalianTiles, sizeof(gMapMenuStatusItalianTiles));
            break;
        case LANGUAGE_GERMAN:
        default:
            work->tiles5 = LoadObjTiles(gMapMenuStatusGermanTiles, sizeof(gMapMenuStatusGermanTiles));
            break;
        }
#else
        work->tiles5 = LoadObjTiles(gMapMenuStatusTiles, sizeof(gMapMenuStatusTiles));
#endif
        work->palette3 = LoadObjPalette(gMapMenuStatusPalette, sizeof(gMapMenuStatusPalette));
        work->x3 = 0x11800;
        work->x4 = 0xF000;
        work->x5 = 0x10000;
        MapMenuWriteDigits3(work->tiles5, 0, gGameState.progression.level);
        MapMenuWriteDigits3(work->tiles5, 6, gGameState.progression.maxHp);
        MapMenuWriteDigits3(work->tiles5, 3, gGameState.hp);

        if (gGameState.flags & GAME_FLAG_RIKU) {
            MapMenuWriteDigits3(work->tiles5, 9, gGameState.progression.dp);
        } else {
            MapMenuWriteDigits5(work->tiles5, 9, gGameState.progression.mooglePoints);
        }

        work->palette4 = LoadObjPalette(gMapMenuHighlightPalette, sizeof(gMapMenuHighlightPalette));
        work->palette5 = LoadObjPalette(gMapMenuPanelPalette, sizeof(gMapMenuPanelPalette));

#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                work->tiles6 = LoadObjTiles(gMapMenuRikuCommandsTiles, sizeof(gMapMenuRikuCommandsTiles));
            } else {
                work->tiles6 = LoadObjTiles(gMapMenuSoraCommandsTiles, sizeof(gMapMenuSoraCommandsTiles));
            }

            break;
        case LANGUAGE_FRENCH:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                work->tiles6 = LoadObjTiles(gMapMenuRikuCommandsFrenchTiles, sizeof(gMapMenuRikuCommandsFrenchTiles));
            } else {
                work->tiles6 = LoadObjTiles(gMapMenuSoraCommandsFrenchTiles, sizeof(gMapMenuSoraCommandsFrenchTiles));
            }

            break;
        case LANGUAGE_SPANISH:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                work->tiles6 = LoadObjTiles(gMapMenuRikuCommandsSpanishTiles, sizeof(gMapMenuRikuCommandsSpanishTiles));
            } else {
                work->tiles6 = LoadObjTiles(gMapMenuSoraCommandsSpanishTiles, sizeof(gMapMenuSoraCommandsSpanishTiles));
            }

            break;
        case LANGUAGE_ITALIAN:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                work->tiles6 = LoadObjTiles(gMapMenuRikuCommandsItalianTiles, sizeof(gMapMenuRikuCommandsItalianTiles));
            } else {
                work->tiles6 = LoadObjTiles(gMapMenuSoraCommandsItalianTiles, sizeof(gMapMenuSoraCommandsItalianTiles));
            }

            break;
        case LANGUAGE_GERMAN:
        default:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                work->tiles6 = LoadObjTiles(gMapMenuRikuCommandsGermanTiles, sizeof(gMapMenuRikuCommandsGermanTiles));
            } else {
                work->tiles6 = LoadObjTiles(gMapMenuSoraCommandsGermanTiles, sizeof(gMapMenuSoraCommandsGermanTiles));
            }

            break;
        }
#else
        if (gGameState.flags & GAME_FLAG_RIKU) {
            work->tiles6 = LoadObjTiles(gMapMenuRikuCommandsTiles, sizeof(gMapMenuRikuCommandsTiles));
        } else {
            work->tiles6 = LoadObjTiles(gMapMenuSoraCommandsTiles, sizeof(gMapMenuSoraCommandsTiles));
        }
#endif

        work->x6 = -0x7800;
        work->palette = LoadObjPalette(gMapMenuCursorPalette, sizeof(gMapMenuCursorPalette));
        work->tiles = AllocObjTiles(0x120, gMapMenuCursorTiles);
        AnimInit(&work->anim, gMapMenuCursorAnims, gMapMenuCursorFrames);
        AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);

        for (i = 0; i < 3; i++) {
            work->tiles9[i] = NULL;
            work->palette9[i] = NULL;
            work->gfx[i] = NULL;
        }

        LoadFriendCardSprites(work->tiles9, (void**)work->palette9, work->gfx);
        InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
        work->palette8 = LoadTextPalette(1);
        work->textSlotCount = LoadTextSlots(GetDeckName(GetActiveDeckIndex()), work->textSlots);

#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            work->tiles3 = LoadObjTiles(gMapMenuTitleTiles, sizeof(gMapMenuTitleTiles));
            break;
        case LANGUAGE_FRENCH:
            work->tiles3 = LoadObjTiles(gMapMenuTitleTiles, sizeof(gMapMenuTitleTiles));
            break;
        case LANGUAGE_SPANISH:
            work->tiles3 = LoadObjTiles(gMapMenuTitleSpanishTiles, sizeof(gMapMenuTitleSpanishTiles));
            break;
        case LANGUAGE_ITALIAN:
            work->tiles3 = LoadObjTiles(gMapMenuTitleTiles, sizeof(gMapMenuTitleTiles));
            break;
        case LANGUAGE_GERMAN:
        default:
            work->tiles3 = LoadObjTiles(gMapMenuTitleGermanTiles, sizeof(gMapMenuTitleGermanTiles));
            break;
        }
#else
        work->tiles3 = LoadObjTiles(gMapMenuTitleTiles, sizeof(gMapMenuTitleTiles));
#endif
        work->x = -0x8000;
        work->tiles4 = LoadObjTiles(gMapMenuCharaWinTiles, sizeof(gMapMenuCharaWinTiles));
        work->x2 = 0xF800;
        MapMenuSetPanelPalettesExcluded(work, TRUE);
        work->panelsVisible = TRUE;
        work->steps = work->reopened ? 1 : 16;
        work->update = MapMenuSlideInX;
    }

    return 1;
}

s32 MapMenuSlideInX(MapMenuWork* work) {
    if (work->steps != 0) {
        ApproachValue(&work->x, 0, work->steps);
        ApproachValue(&work->x6, 0x800, work->steps);
        ApproachValue(&work->x2, 0x7800, work->steps);
        ApproachValue(&work->x3, 0x9800, work->steps);
        ApproachValue(&work->x4, 0x7000, work->steps);
        ApproachValue(&work->x5, 0x8000, work->steps);
        ApproachValue(&work->x8, 0xAC00, work->steps);
        ApproachValue(&work->y4, 0x6000, work->steps);
        work->steps--;
    } else {
        work->cursorVisible = TRUE;
        work->y3 = (work->cursor * 19 + 16) << 8;

        if (work->reopened) {
            work->reopened = FALSE;
            FadeToAmount(FADE_MODE_BLACK, 16, 1);
            work->update = MapMenuResume;
        } else {
            work->update = (gGameState.flags & GAME_FLAG_RIKU) ? MapMenuRikuInput : MapMenuSoraInput;
        }
    }

    return 1;
}

s32 MapMenuSoraInput(MapMenuWork* work) {
    if (GetKeysRepeat() & DPAD_UP) {
        work->cursor = work->cursor != 0 ? work->cursor - 1 : 6;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysRepeat() & DPAD_DOWN) {
        work->cursor = work->cursor <= 5 ? work->cursor + 1 : 0;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysPressed() & (B_BUTTON | START_BUTTON)) {
        work->cursorVisible = FALSE;
        work->steps = 16;
        work->update = MapMenuSlideOutX;
        m4aSongNumStart(SONG_SYS_CLOSE);
    } else if (GetKeysPressed() & A_BUTTON) {
        switch (work->cursor) {
        case MAP_MENU_ITEM_DECK:
        case MAP_MENU_ITEM_MAP_CARDS:
        case MAP_MENU_ITEM_WORLDS:
        case MAP_MENU_ITEM_STATUS:
        case MAP_MENU_ITEM_JOURNAL:
            RequestFieldResume();
            work->update = MapMenuOpenSubMode;
            m4aSongNumStart(SONG_SYS_KETTEI);
            break;
        case MAP_MENU_ITEM_MAP:
            if ((u8)(gMapFloorState.room + 4) > 2) {
                RequestFieldResume();
                work->update = MapMenuOpenSubMode;
                m4aSongNumStart(SONG_SYS_KETTEI);
            } else {
                m4aSongNumStart(SONG_SYS_BEEP);
            }

            break;
        case MAP_MENU_ITEM_QUICK_SAVE:
            m4aSongNumStart(SONG_SYS_KETTEI);
            MapMenuInitConfirm(work);
            work->confirmCursor = MAP_CONFIRM_CURSOR_NO;
            work->x7 = 0x8800;
            work->update = MapMenuConfirmInput;
            break;
        }
    }

    return 1;
}

s32 MapMenuRikuInput(MapMenuWork* work) {
    if (GetKeysRepeat() & DPAD_UP) {
        work->cursor = work->cursor != 0 ? work->cursor - 1 : 6;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysRepeat() & DPAD_DOWN) {
        work->cursor = work->cursor <= 5 ? work->cursor + 1 : 0;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysPressed() & (B_BUTTON | START_BUTTON)) {
        work->cursorVisible = FALSE;
        work->steps = 16;
        work->update = MapMenuSlideOutX;
        m4aSongNumStart(SONG_SYS_CLOSE);
    } else if (GetKeysPressed() & A_BUTTON) {
        switch (work->cursor) {
        case MAP_MENU_ITEM_DECK:
        case MAP_MENU_ITEM_MAP_CARDS:
        case MAP_MENU_ITEM_WORLDS:
        case MAP_MENU_ITEM_STATUS:
            m4aSongNumStart(SONG_SYS_KETTEI);
            RequestFieldResume();
            work->update = MapMenuOpenSubMode;
            break;
        case MAP_MENU_ITEM_JOURNAL:
            m4aSongNumStart(SONG_SYS_KETTEI);
            RequestFieldResume();
            work->update = MapMenuOpenSubMode;
            break;
        case MAP_MENU_ITEM_MAP:
            if ((u8)(gMapFloorState.room + 4) > 2) {
                m4aSongNumStart(SONG_SYS_KETTEI);
                RequestFieldResume();
                work->update = MapMenuOpenSubMode;
            } else {
                m4aSongNumStart(SONG_SYS_BEEP);
            }

            break;
        case MAP_MENU_ITEM_QUICK_SAVE:
            m4aSongNumStart(SONG_SYS_KETTEI);
            MapMenuInitConfirm(work);
            work->confirmCursor = MAP_CONFIRM_CURSOR_NO;
            work->x7 = 0x8800;
            work->update = MapMenuConfirmInput;
            break;
        }
    }

    return 1;
}

s32 MapMenuOpenSubMode(MapMenuWork* work) {
    gGameState.mapMenuCursor = work->cursor;

    switch (work->cursor) {
    case MAP_MENU_ITEM_DECK:
        if (gGameState.flags & GAME_FLAG_RIKU) {
            if (gGameState.progression.tutorialFlags & TUTORIAL_FLAG_RIKU_DECK) {
                ModeRequest(&gModeDeck, 0);
            } else {
                ModeRequest(&gModeRikuDeckTutorial, 0);
            }
        } else {
            ModeRequest(&gModeDeck, 0);
        }

        break;
    case MAP_MENU_ITEM_MAP:
        ModeRequest(&gModeAllmap, 0);
        break;
    case MAP_MENU_ITEM_STATUS:
        ModeRequest(&gModeStatus, 0);
        break;
    case MAP_MENU_ITEM_MAP_CARDS:
        ModeRequest(&gModeMapinspect, 0);
        break;
    case MAP_MENU_ITEM_WORLDS:
        ModeRequest(&gModeWorldinspect, 0);
        break;
    case MAP_MENU_ITEM_JOURNAL:
        ModeRequest(&gModeJiminy, 0);
        break;
    case MAP_MENU_ITEM_QUICK_SAVE:
        ModeRequest(&gModeMenuMsg, 1);
        break;
    }

    return 1;
}

s32 MapMenuSlideOutX(MapMenuWork* work) {
    if (work->steps != 0) {
        ApproachValue(&work->x, -0x8000, work->steps);
        ApproachValue(&work->x6, -0x7800, work->steps);
        ApproachValue(&work->x2, 0xF800, work->steps);
        ApproachValue(&work->x3, 0x11800, work->steps);
        ApproachValue(&work->x4, 0xF000, work->steps);
        ApproachValue(&work->x5, 0x10000, work->steps);
        ApproachValue(&work->x8, work->playerStartX, work->steps);
        ApproachValue(&work->y4, work->playerStartY, work->steps);
        work->steps--;
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_HIDE_PLAYER;
        gFieldState->flags &= ~FIELD_FLAG_HIDE_ENEMIES;
        MapMenuSetPanelPalettesExcluded(work, FALSE);
        FadeToOriginal(FADE_MODE_BLACK, 16);
        work->steps = 16;
        work->update = MapMenuSlideOutY;
    }

    return 1;
}

s32 MapMenuSlideOutY(MapMenuWork* work) {
    if (work->steps != 0) {
        ApproachValue(&work->y, -0x800, work->steps);
        ApproachValue(&work->y2, 0xA000, work->steps);
        work->steps--;
        return 1;
    }

    gGameState.mapMenuCursor = 0xFF;
    m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFF, 0x100);
    return 0;
}

s32 MapMenuConfirmInput(MapMenuWork* work) {
    if (GetKeysPressed() & DPAD_LEFT) {
        if (work->confirmCursor != MAP_CONFIRM_CURSOR_YES) {
            work->confirmCursor = MAP_CONFIRM_CURSOR_YES;
            m4aSongNumStart(SONG_SYS_CLICK);
        }
    }

    if (GetKeysPressed() & DPAD_RIGHT) {
        if (work->confirmCursor != MAP_CONFIRM_CURSOR_NO) {
            work->confirmCursor = MAP_CONFIRM_CURSOR_NO;
            m4aSongNumStart(SONG_SYS_CLICK);
        }
    }

    if ((GetKeysPressed() & B_BUTTON) || ((GetKeysPressed() & A_BUTTON) && work->confirmCursor == MAP_CONFIRM_CURSOR_NO)) {
        work->confirmCursor = MAP_CONFIRM_CURSOR_NONE;
        MapMenuFreeConfirm(work);
        work->y3 = (work->cursor * 19 + 16) << 8;
        work->update = (gGameState.flags & GAME_FLAG_RIKU) ? MapMenuRikuInput : MapMenuSoraInput;
        m4aSongNumStart(SONG_SYS_CLOSE);
    } else if (GetKeysPressed() & A_BUTTON) {
        SaveWriteSystem();
        work->update = MapMenuOpenSubMode;
        m4aSongNumStart(SONG_SYS_KETTEI);
    }

    return 1;
}

s32 MapMenuResume(MapMenuWork* work) {
    gDispCnt |= DISPCNT_OBJ_ON;
    work->update = (gGameState.flags & GAME_FLAG_RIKU) ? MapMenuRikuInput : MapMenuSoraInput;
    return 1;
}

void Task_MapMenu_0(MapMenuWork* work) {
    s8 cursor;

    gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
    gFieldState->flags |= FIELD_FLAG_FREEZE_ENEMIES;
    gFieldState->flags |= FIELD_FLAG_MENU_OPEN;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        work->palette6 = LoadObjPalette(gRikuPalette, sizeof(gRikuPalette));
    } else {
        work->palette6 = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    }

    FadeSetPaletteExcluded(work->palette6->index + 0x10, TRUE);
#ifdef PLATFORM_PC
    /* Task_MapMenu_1 tests panelsVisible before MapMenuOpen clears it, and the
       work comes from the heap uncleared: with leftovers (often the deck
       editor's) it animated a cursor that does not exist yet. The GBA read
       that garbage harmlessly and AnimInit replaced it frames later; here
       it is an access violation (issue #26). */
    work->panelsVisible = FALSE;
#endif
    cursor = gGameState.mapMenuCursor;

    if (cursor != -1) {
        work->reopened = TRUE;
    } else {
        gGameState.mapMenuCursor = MAP_MENU_ITEM_DECK;
        work->reopened = FALSE;
        m4aSongNumStart(SONG_SYS_CANSEL);
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFF, 0x80);
    }

    work->update = MapMenuOpen;
}

s32 Task_MapMenu_1(MapMenuWork* work) {
    if (work->reopened) {
        FadeStartIn(FADE_MODE_BLACK, 16);
    }

    if (work->panelsVisible) {
        AnimUpdate(&work->anim);
    }

    if (work->update != NULL && (u8)work->update(work) == 0) {
        return 0;
    }

    return 1;
}

void Task_MapMenu_2(MapMenuWork* work) {
    s32 i;
    s32 iconOffset;

#ifdef PLATFORM_PC
    PortScreenBegin(); /* menu: drawn on the screen layer, at the window's height */
#endif

    PORT_BAND(DrawSprite(128, work->y >> 8, gMapMenuBarsFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
    PORT_BAND(DrawSprite(128, work->y2 >> 8, gMapMenuBarsFrames[1], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));

    if (
#ifdef VERSION_EU
        work->confirmCursor == MAP_CONFIRM_CURSOR_NONE &&
#endif
        (gMapRoomState->flags & ROOM_FLAG_HIDE_PLAYER)) {
        if (gGameState.flags & GAME_FLAG_RIKU) {
            DrawSprite(PORT_MENU_RIGHT(work->x8 >> 8), work->y4 >> 8, gRikuFf00Frames[0], work->tiles8, work->palette6,
                NULL, SPRITE_PRIORITY(1), 80);
        } else {
            DrawSprite(PORT_MENU_RIGHT(work->x8 >> 8), work->y4 >> 8, gSor1ff00Frames[0], work->tiles8,
                work->palette6, NULL, SPRITE_PRIORITY(1), 80);
        }

        DrawSprite(PORT_MENU_RIGHT(work->x8 >> 8), work->y4 >> 8, gBtlShadowFrames[0], work->tiles7, work->palette7, NULL,
            SPRITE_PRIORITY(1), 81);
    }

    if (work->panelsVisible) {
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_SPANISH:
            DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapMenuTitleSpanishFrames[0], work->tiles3, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
            break;
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
        case LANGUAGE_ITALIAN:
            DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapMenuTitleFrames[0], work->tiles3, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
            break;
        case LANGUAGE_GERMAN:
        default:
            DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapMenuTitleGermanFrames[0], work->tiles3, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
            break;
        }
#else
        DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapMenuTitleFrames[0], work->tiles3, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
#endif
        DrawSprite(PORT_MENU_RIGHT(work->x2 >> 8), 14, gMapMenuCharaWinFrames[0], work->tiles4, work->palette2, NULL, SPRITE_PRIORITY(1), 90);

        if (gGameState.flags & GAME_FLAG_RIKU) {
#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuRikuStatusFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_FRENCH:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusFrenchFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusFrenchFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuRikuStatusFrenchFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_SPANISH:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusSpanishFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusSpanishFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuRikuStatusSpanishFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_ITALIAN:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusItalianFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusItalianFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuRikuStatusItalianFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_GERMAN:
            default:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusGermanFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusGermanFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuRikuStatusGermanFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            }
#else
            DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
            DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuRikuStatusFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
            DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuRikuStatusFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                81);
#endif

            if (work->tiles9[0] != NULL) {
                DrawSprite(PORT_MENU_RIGHT((work->x4 >> 8) + 18), 124, work->gfx[0], work->tiles9[0],
                    work->palette9[0], NULL, SPRITE_PRIORITY(1), 80);
            }
        } else {
#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuSoraStatusFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_FRENCH:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusFrenchFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusFrenchFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuSoraStatusFrenchFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_SPANISH:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusSpanishFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusSpanishFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuSoraStatusSpanishFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_ITALIAN:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusItalianFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusItalianFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuSoraStatusItalianFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_GERMAN:
            default:
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusGermanFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
                DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusGermanFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuSoraStatusGermanFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            }
#else
            DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusFrames[0], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 80);
            DrawSprite(PORT_MENU_RIGHT(work->x3 >> 8), 0, gMapMenuSoraStatusFrames[1], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1), 81);
            DrawSprite(PORT_MENU_RIGHT(work->x4 >> 8), 103, gMapMenuSoraStatusFrames[2], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                81);
#endif

            for (i = 0; i < 3; i++) {
                iconOffset = i * 20 + 14;

                if (work->tiles9[i] != NULL) {
                    DrawSprite(PORT_MENU_RIGHT((work->x4 >> 8) + iconOffset), 124, work->gfx[i], work->tiles9[i],
                        work->palette9[i], NULL, SPRITE_PRIORITY(1), 80);
                }
            }
        }

        if (!(gGameState.flags & GAME_FLAG_RIKU)) {
#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                DrawSprite(PORT_MENU_RIGHT(work->x5 >> 8), 144, gMapMenuSoraStatusFrames[3], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_FRENCH:
                DrawSprite(PORT_MENU_RIGHT(work->x5 >> 8), 144, gMapMenuSoraStatusFrenchFrames[3], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_SPANISH:
                DrawSprite(PORT_MENU_RIGHT(work->x5 >> 8), 144, gMapMenuSoraStatusSpanishFrames[3], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_ITALIAN:
                DrawSprite(PORT_MENU_RIGHT(work->x5 >> 8), 144, gMapMenuSoraStatusItalianFrames[3], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            case LANGUAGE_GERMAN:
            default:
                DrawSprite(PORT_MENU_RIGHT(work->x5 >> 8), 144, gMapMenuSoraStatusGermanFrames[3], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                    81);
                break;
            }
#else
            DrawSprite(PORT_MENU_RIGHT(work->x5 >> 8), 144, gMapMenuSoraStatusFrames[3], work->tiles5, work->palette3, NULL, SPRITE_PRIORITY(1),
                81);
#endif
            DrawTextSlots(PORT_MENU_RIGHT((work->x5 >> 8) + 16), 145, work->textSlots, work->palette8, 50,
                work->textSlotCount);
        }

#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            if (work->cursorVisible) {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsFrames[work->cursor], work->tiles6, work->palette5, NULL,
                    SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandHighlightFrames[work->cursor], work->tiles6, work->palette4, NULL,
                    SPRITE_PRIORITY(1), 81);
            } else {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsFrames[7], work->tiles6, work->palette5, NULL, SPRITE_PRIORITY(1),
                    80);
            }

            break;
        case LANGUAGE_FRENCH:
            if (work->cursorVisible) {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsFrenchFrames[work->cursor], work->tiles6, work->palette5, NULL,
                    SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandHighlightFrenchFrames[work->cursor], work->tiles6, work->palette4, NULL,
                    SPRITE_PRIORITY(1), 81);
            } else {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsFrenchFrames[7], work->tiles6, work->palette5, NULL, SPRITE_PRIORITY(1),
                    80);
            }

            break;
        case LANGUAGE_SPANISH:
            if (work->cursorVisible) {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsSpanishFrames[work->cursor], work->tiles6, work->palette5, NULL,
                    SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandHighlightSpanishFrames[work->cursor], work->tiles6, work->palette4, NULL,
                    SPRITE_PRIORITY(1), 81);
            } else {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsSpanishFrames[7], work->tiles6, work->palette5, NULL, SPRITE_PRIORITY(1),
                    80);
            }

            break;
        case LANGUAGE_ITALIAN:
            if (work->cursorVisible) {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsItalianFrames[work->cursor], work->tiles6, work->palette5, NULL,
                    SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandHighlightItalianFrames[work->cursor], work->tiles6, work->palette4, NULL,
                    SPRITE_PRIORITY(1), 81);
            } else {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsItalianFrames[7], work->tiles6, work->palette5, NULL, SPRITE_PRIORITY(1),
                    80);
            }

            break;
        case LANGUAGE_GERMAN:
        default:
            if (work->cursorVisible) {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsGermanFrames[work->cursor], work->tiles6, work->palette5, NULL,
                    SPRITE_PRIORITY(1), 81);
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandHighlightGermanFrames[work->cursor], work->tiles6, work->palette4, NULL,
                    SPRITE_PRIORITY(1), 81);
            } else {
                DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsGermanFrames[7], work->tiles6, work->palette5, NULL, SPRITE_PRIORITY(1),
                    80);
            }

            break;
        }
#else
        if (work->cursorVisible) {
            DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsFrames[work->cursor], work->tiles6, work->palette5, NULL,
                SPRITE_PRIORITY(1), 81);
            DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandHighlightFrames[work->cursor], work->tiles6, work->palette4, NULL,
                SPRITE_PRIORITY(1), 81);
        } else {
            DrawSprite(PORT_MENU_LEFT(work->x6 >> 8), 26, gMapMenuCommandsFrames[7], work->tiles6, work->palette5, NULL, SPRITE_PRIORITY(1),
                80);
        }
#endif

        if (work->cursorVisible) {
            switch (work->confirmCursor) {
            case MAP_CONFIRM_CURSOR_YES:
                ApproachValueHalf(&work->x7, 0x4800);
                DrawSprite(work->x7 >> 8, 80, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL,
                    SPRITE_FLAG_HFLIP, 60);
                break;
            case MAP_CONFIRM_CURSOR_NO:
                ApproachValueHalf(&work->x7, 0x8800);
                DrawSprite(work->x7 >> 8, 80, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL,
                    SPRITE_FLAG_HFLIP, 60);
                break;
            case MAP_CONFIRM_CURSOR_NONE:
            default:
                ApproachValueHalf(&work->y3, (work->cursor * 19 + 16) << 8);
                DrawSprite(PORT_MENU_LEFT(24), work->y3 >> 8, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL,
                    SPRITE_PRIORITY(1) | SPRITE_FLAG_HFLIP, 60);
                break;
            }
        }

        if (work->confirmCursor != MAP_CONFIRM_CURSOR_NONE) {
            DrawTextSlots(
#ifdef VERSION_EU
                120 - (GetTextSlotsWidth(work->textSlots2, work->textSlotCount2) >> 1),
#else
                (DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)) / 2,
#endif
                64, work->textSlots2,
                work->confirmPalette, 70, work->textSlotCount2);
            DrawTextSlots(80, 84, work->textSlots3, work->confirmPalette, 70, work->textSlotCount3);
            DrawTextSlots(144, 84, work->textSlots4, work->confirmPalette, 70, work->textSlotCount4);
        }
    }
#ifdef PLATFORM_PC
    PortScreenEnd();
#endif
}

void Task_MapMenu_3(MapMenuWork* work) {
    s32 i;

    // @bug Leaves the confirmation allocated when destroyed with it open.
    MapMenuSetCharaPalettesExcluded(work, FALSE);
    MapMenuSetPanelPalettesExcluded(work, FALSE);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette3);
    ReleaseObjTiles(work->tiles5);
    ReleaseObjPalette(work->palette4);
    ReleaseObjPalette(work->palette5);
    ReleaseObjTiles(work->tiles6);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette6);
    ReleaseObjTiles(work->tiles8);
    ReleaseObjPalette(work->palette7);
    ReleaseObjTiles(work->tiles7);

    for (i = 0; i < 3; i++) {
        if (work->tiles9[i] != NULL) {
            ReleaseObjTiles(work->tiles9[i]);
            ReleaseObjPalette(work->palette9[i]);
        }
    }

    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    ReleaseObjPalette(work->palette8);
    gMapRoomState->flags &= ~ROOM_FLAG_ATTACK_HIT;
    gFieldState->flags &= ~FIELD_FLAG_FREEZE_PLAYER;
    gFieldState->flags &= ~FIELD_FLAG_FREEZE_ENEMIES;
    gFieldState->flags &= ~FIELD_FLAG_MENU_OPEN;
}

void MapSaveSetPanelPalettesExcluded(MapSaveWork* work, u8 excluded) {
    FadeSetPaletteExcluded(0x0B, excluded);
    FadeSetPaletteExcluded(0x0C, excluded);
    FadeSetPaletteExcluded(0x0D, excluded);
    FadeSetPaletteExcluded(0x0E, excluded);
    FadeSetPaletteExcluded(work->palette3->index + 0x10, excluded);
    FadeSetPaletteExcluded(work->palette->index + 0x10, excluded);
    FadeSetPaletteExcluded(work->palette4->index + 0x10, excluded);
    FadeSetPaletteExcluded(work->palette8->index + 0x10, excluded);
}

void MapSaveSetCharaPalettesExcluded(MapSaveWork* work, u8 excluded) {
    FadeSetPaletteExcluded(work->palette2->index + 0x10, excluded);
    FadeSetPaletteExcluded(work->palette5->index + 0x10, excluded);
    FadeSetPaletteExcluded(work->palette6->index + 0x10, excluded);
    FadeSetPaletteExcluded(work->palette7->index + 0x10, excluded);
}

void MapSaveLoadFloorTiles(u8 floor) {
    const u8* src;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        if (gGameState.flags & GAME_FLAG_RIKU) {
            src = gSaveSlotRikuFloorSelectedTiles[floor];
        } else {
            src = gSaveSlotSoraFloorSelectedTiles[floor];
        }

        break;
    case LANGUAGE_FRENCH:
        if (gGameState.flags & GAME_FLAG_RIKU) {
            src = gSaveSlotRikuFloorSelectedFrenchTiles[floor];
        } else {
            src = gSaveSlotSoraFloorSelectedFrenchTiles[floor];
        }

        break;
    case LANGUAGE_SPANISH:
        if (gGameState.flags & GAME_FLAG_RIKU) {
            src = gSaveSlotRikuFloorSelectedSpanishTiles[floor];
        } else {
            src = gSaveSlotSoraFloorSelectedSpanishTiles[floor];
        }

        break;
    case LANGUAGE_ITALIAN:
        if (gGameState.flags & GAME_FLAG_RIKU) {
            src = gSaveSlotRikuFloorSelectedItalianTiles[floor];
        } else {
            src = gSaveSlotSoraFloorSelectedItalianTiles[floor];
        }

        break;
    case LANGUAGE_GERMAN:
    default:
        if (gGameState.flags & GAME_FLAG_RIKU) {
            src = gSaveSlotRikuFloorSelectedGermanTiles[floor];
        } else {
            src = gSaveSlotSoraFloorSelectedGermanTiles[floor];
        }

        break;
    }
#else
    if (gGameState.flags & GAME_FLAG_RIKU) {
        src = gSaveSlotRikuFloorSelectedTiles[floor];
    } else {
        src = gSaveSlotSoraFloorSelectedTiles[floor];
    }
#endif

    RequestDma3Copy((void*)src, (u8*)GetBgCharBase(0) + 320, 320);
}

void MapSaveLoadLevelTiles(u16 level) {
    u16 digits[4];
    s32 off;
    s32 i;

    digits[0] = level / 100;
    digits[1] = level / 10 - digits[0] * 10;
    digits[2] = level - digits[0] * 100 - digits[1] * 10;
    off = 0x40;

    for (i = 1; i < 3; i++) {
        RequestDma3Copy((void*)&gSaveSlotDigitTiles[digits[i] * 32], (u8*)GetBgCharBase(0) + off, 0x20);
        PORT_BG_DIGIT((u8*)GetBgCharBase(0) + off, digits[i], &gSaveSlotDigitTiles[digits[i] * 32]);
        off += 0x20;
    }
}

void MapSaveLoadTimeTiles(u32 playTime) {
    u16 digits[6];
    s32 off;
    u16* digit;
    s32 i;
    u32 part;

    part = playTime / 3600;
    digits[0] = part / 10;
    digits[1] = part - digits[0] * 10;
    playTime -= part * 3600;
    part = playTime / 60;
    digits[2] = part / 10;
    digits[3] = part - digits[2] * 10;
    playTime -= part * 60;
    digits[4] = playTime / 10;
    digits[5] = playTime - digits[4] * 10;
    off = 128;
    digit = digits;

    for (i = 0; i < 6; i++) {
        RequestDma3Copy((void*)&gSaveSlotDigitTiles[*digit * 32], (u8*)GetBgCharBase(0) + off, 0x20);
        PORT_BG_DIGIT((u8*)GetBgCharBase(0) + off, *digit, &gSaveSlotDigitTiles[*digit * 32]);
        off += 0x20;
        digit++;
    }
}

void MapSaveShowSummary(MapSaveWork* work, u8 slot) {
    SaveFileSummary* summary = &gGameState.fileSummaries[slot];

    if (summary->level == 0) {
        work->textSlotCount = 0;
    } else {
        MapSaveLoadFloorTiles(summary->floor);
        MapSaveLoadLevelTiles(summary->level);
        MapSaveLoadTimeTiles(summary->playTime);
        work->textSlotCount = LoadTextSlots(GetMapWorldName(summary->world), work->textSlots);
    }
}

s32 MapSaveSlideInY(MapSaveWork* work) {
    if (work->steps != 0) {
        ApproachValue(&work->y, 0, work->steps);
        ApproachValue(&work->y2, 0x9800, work->steps);
        work->steps -= 1;
    } else {
        gMapRoomState->flags |= ROOM_FLAG_HIDE_PLAYER;
        gFieldState->flags |= FIELD_FLAG_HIDE_ENEMIES;
        work->steps = 16;
        work->update = MapSaveSlideInX;
    }

    return 1;
}

s32 MapSaveSlideInX(MapSaveWork* work) {
    TextSlot* promptSlots;
    TextSlot* yesSlots;
    TextSlot* noSlots;

    if (work->steps != 0) {
        ApproachValue(&work->x, 0, work->steps);

        if (gGameState.flags & GAME_FLAG_RIKU) {
            ApproachValue(&work->x3, 0x3800, work->steps);
            ApproachValue(&work->y3, 0x7000, work->steps);
        } else {
            ApproachValue(&work->x3, 0x3800, work->steps);
            ApproachValue(&work->y3, 0x7000, work->steps);
        }

        work->steps -= 1;
    } else {
        if (!(gGameState.flags & GAME_FLAG_RIKU)) {
            work->palette4 = LoadObjPalette(gSaveFloorSoraPalette, sizeof(gSaveFloorSoraPalette));
        } else {
            work->palette4 = LoadObjPalette(gSaveFloorRikuPalette, sizeof(gSaveFloorRikuPalette));
        }

        work->textSlotCount = 0;
        InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
        SetupBg(0, 3, 31, 11);
        SetBgPriority(0, 0);
        LoadBgPalette(0, gMapSaveBgPalettes, sizeof(gMapSaveBgPalettes));
        LoadBgTiles(0, gSaveSlotBgTiles, sizeof(gSaveSlotBgTiles));

#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_FRENCH:
            RequestDma3Copy((void*)gSaveSlotLabelsFrenchTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsFrenchTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy((void*)gSaveSlotLabelsSpanishTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsSpanishTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy((void*)gSaveSlotLabelsItalianTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsItalianTiles));
            break;
        case LANGUAGE_GERMAN:
            RequestDma3Copy((void*)gSaveSlotLabelsGermanTiles, (u8*)GetBgCharBase(0) + 0x800, sizeof(gSaveSlotLabelsGermanTiles));
            break;
        }
#endif

        if (gGameState.flags & GAME_FLAG_RIKU) {
            if (gGameState.flags & GAME_FLAG_SECOND_FILE) {
                LoadBgMap(0, gMapSaveRikuFile2Map, sizeof(gMapSaveRikuFile2Map));

                if (SaveRepairFileSmall(1) == SAVE_OK) {
                    MapSaveShowSummary(work, 3);
                }
            } else {
                LoadBgMap(0, gMapSaveRikuFile1Map, sizeof(gMapSaveRikuFile1Map));

                if (SaveRepairFileSmall(0) == SAVE_OK) {
                    MapSaveShowSummary(work, 2);
                }
            }
        } else {
            if (gGameState.flags & GAME_FLAG_SECOND_FILE) {
                LoadBgMap(0, gMapSaveSoraFile2Map, sizeof(gMapSaveSoraFile2Map));

                if (SaveRepairFileLarge(1) == SAVE_OK) {
                    MapSaveShowSummary(work, 1);
                }
            } else {
                LoadBgMap(0, gMapSaveSoraFile1Map, sizeof(gMapSaveSoraFile1Map));

                if (SaveRepairFileLarge(0) == SAVE_OK) {
                    MapSaveShowSummary(work, 0);
                }
            }
        }

        SetBgScroll(0, 0, 0xFFFB);
        work->tiles5 = AllocObjTiles(0x280, gDonaFl00Tiles);
        work->tiles6 = AllocObjTiles(0x400, gGoofyFl00Tiles);
        work->palette3 = LoadObjPalette(gFileMenuWindowPalette, sizeof(gFileMenuWindowPalette));
        work->tiles3 = LoadObjTiles(gSaveSlotCharaWinTiles, sizeof(gSaveSlotCharaWinTiles));
        work->palette = LoadObjPalette(gMapMenuCursorPalette, sizeof(gMapMenuCursorPalette));
        work->tiles = AllocObjTiles(0x120, gMapMenuCursorTiles);
        AnimInit(&work->anim, gMapMenuCursorAnims, gMapMenuCursorFrames);
        AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
        work->palette8 = LoadTextPalette(1);
        promptSlots = work->textSlots2;
        InitTextSlots(promptSlots, ARRAY_COUNT(work->textSlots2));
        yesSlots = work->textSlots3;
        InitTextSlots(yesSlots, ARRAY_COUNT(work->textSlots3));
        noSlots = work->textSlots4;
        InitTextSlots(noSlots, ARRAY_COUNT(work->textSlots4));
        work->textSlotCount2 = LoadTextSlots(LOCALIZED_STRING(gMapSaveConfirmText), promptSlots);
        work->textSlotCount3 = LoadTextSlots(LOCALIZED_STRING(gYesChoiceText), yesSlots);
        work->textSlotCount4 = LoadTextSlots(LOCALIZED_STRING(gNoChoiceText), noSlots);
        MapSaveSetPanelPalettesExcluded(work, TRUE);
        work->dialogVisible = TRUE;
        work->confirmCursor = MAP_CONFIRM_CURSOR_NO;
        work->x2 = 0xB000;
        work->update = MapSaveInput;
    }

    return 1;
}

#ifdef PLATFORM_PC
static void MapSaveSelectTarget(MapSaveWork* work, u8 slot) {
    int ok;
    sMapSaveTargetSlot = slot & 1;
    /* the level, time and floor tiles (0x40-0x280) go back to the window's
       own blank ones, so an empty file does not keep the other file's numbers */
    RequestDma3Copy((u8*)gSaveSlotBgTiles + 0x40, (u8*)GetBgCharBase(0) + 0x40, 0x240);
    work->textSlotCount = 0;
    /* as when the window opens: the file's own summary, only if it holds a save */
    if (gGameState.flags & GAME_FLAG_RIKU) {
        if (sMapSaveTargetSlot)
            LoadBgMap(0, gMapSaveRikuFile2Map, sizeof(gMapSaveRikuFile2Map));
        else
            LoadBgMap(0, gMapSaveRikuFile1Map, sizeof(gMapSaveRikuFile1Map));
        ok = SaveRepairFileSmall(sMapSaveTargetSlot) == SAVE_OK;
    } else {
        if (sMapSaveTargetSlot)
            LoadBgMap(0, gMapSaveSoraFile2Map, sizeof(gMapSaveSoraFile2Map));
        else
            LoadBgMap(0, gMapSaveSoraFile1Map, sizeof(gMapSaveSoraFile1Map));
        ok = SaveRepairFileLarge(sMapSaveTargetSlot) == SAVE_OK;
    }
    if (ok)
        MapSaveShowSummary(work, sMapSaveTargetSlot + ((gGameState.flags & GAME_FLAG_RIKU) ? 2 : 0));
}
#endif

s32 MapSaveInput(MapSaveWork* work) {
#ifdef PLATFORM_PC
    u8 targetSlot = (gGameState.flags & GAME_FLAG_SECOND_FILE) ? 1 : 0;
    if (PortSaveSlotSelectEnabled()) {
        if (GetKeysRepeat() & (DPAD_UP | DPAD_DOWN)) {
            MapSaveSelectTarget(work, sMapSaveTargetSlot ^ 1);
            m4aSongNumStart(SONG_SYS_CLICK);
        }
        targetSlot = sMapSaveTargetSlot;
    }
#endif
    if (GetKeysRepeat() & DPAD_LEFT) {
        if (work->confirmCursor != MAP_CONFIRM_CURSOR_YES) {
            work->confirmCursor = MAP_CONFIRM_CURSOR_YES;
            m4aSongNumStart(SONG_SYS_CLICK);
        }
    }

    if (GetKeysRepeat() & DPAD_RIGHT) {
        if (work->confirmCursor != MAP_CONFIRM_CURSOR_NO) {
            work->confirmCursor = MAP_CONFIRM_CURSOR_NO;
            m4aSongNumStart(SONG_SYS_CLICK);
        }
    }

    if ((GetKeysPressed() & B_BUTTON) || ((GetKeysPressed() & A_BUTTON) && work->confirmCursor != MAP_CONFIRM_CURSOR_YES)) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        work->confirmCursor = MAP_CONFIRM_CURSOR_NONE;
        work->dialogVisible = FALSE;
        work->steps = 16;
        work->update = MapSaveSlideOutX;
        DisableBg(0);
    } else if (GetKeysPressed() & A_BUTTON) {
        m4aSongNumStart(SONG_SYS_SAVELOAD);
#ifdef PLATFORM_PC
        /* Saving to the other file makes it the current one, as loading it
           would: its summary and the header are written for that file only,
           later automatic saves go there, and the crystal opens on it again
           (the flag is stored in the save, so this lasts between sessions). */
        if (targetSlot)
            gGameState.flags |= GAME_FLAG_SECOND_FILE;
        else
            gGameState.flags &= ~GAME_FLAG_SECOND_FILE;

        if (gGameState.flags & GAME_FLAG_RIKU) {
            SaveWriteFileSmall(targetSlot);
            MapSaveShowSummary(work, targetSlot + 2);
        } else {
            SaveWriteFileLarge(targetSlot);
            MapSaveShowSummary(work, targetSlot);
        }
#else
        if (gGameState.flags & GAME_FLAG_RIKU) {
            if (gGameState.flags & GAME_FLAG_SECOND_FILE) {
                SaveWriteFileSmall(1);
                MapSaveShowSummary(work, 3);
            } else {
                SaveWriteFileSmall(0);
                MapSaveShowSummary(work, 2);
            }
        } else {
            if (gGameState.flags & GAME_FLAG_SECOND_FILE) {
                SaveWriteFileLarge(1);
                MapSaveShowSummary(work, 1);
            } else {
                SaveWriteFileLarge(0);
                MapSaveShowSummary(work, 0);
            }
        }
#endif

        work->textSlotCount2 = LoadTextSlots(LOCALIZED_STRING(gMapSaveCompleteText), work->textSlots2);
        work->textSlotCount3 = 0;
        work->textSlotCount4 = 0;
        work->confirmCursor = MAP_CONFIRM_CURSOR_NONE;
        work->update = MapSaveWaitClose;
    }

    return 1;
}

s32 MapSaveWaitClose(MapSaveWork* work) {
    if (GetKeysPressed() & (A_BUTTON | B_BUTTON)) {
        work->dialogVisible = FALSE;
        DisableBg(0);
        work->steps = 16;
        work->update = MapSaveSlideOutX;
    }

    return 1;
}

s32 MapSaveSlideOutX(MapSaveWork* work) {
    if (work->steps != 0) {
        ApproachValue(&work->x, -0x8000, work->steps);
        ApproachValue(&work->x3, work->playerStartX, work->steps);
        ApproachValue(&work->y3, work->playerStartY, work->steps);
        work->steps--;
    } else {
        gMapRoomState->flags &= ~ROOM_FLAG_HIDE_PLAYER;
        gFieldState->flags &= ~FIELD_FLAG_HIDE_ENEMIES;
        MapSaveSetPanelPalettesExcluded(work, FALSE);
        FadeToOriginal(FADE_MODE_BLACK, 16);
        work->steps = 16;
        work->update = MapSaveSlideOutY;
    }

    return 1;
}

s32 MapSaveSlideOutY(MapSaveWork* work) {
    if (work->steps != 0) {
        ApproachValue(&work->y, -0x800, work->steps);
        ApproachValue(&work->y2, 0xA000, work->steps);
        work->steps--;
        return 1;
    }

    return 0;
}

void Task_MapSave_0(MapSaveWork* work) {
#ifdef PLATFORM_PC
    sMapSaveTargetSlot = (gGameState.flags & GAME_FLAG_SECOND_FILE) ? 1 : 0;
#endif
    gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
    gFieldState->flags |= FIELD_FLAG_FREEZE_ENEMIES;
    gMapRoomState->flags |= ROOM_FLAG_SAVE_MENU_OPEN;
    gGameState.hp = gGameState.progression.maxHp;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->tiles2 = LoadObjTiles(gMapSaveTitleTiles, sizeof(gMapSaveTitleTiles));
        break;
    case LANGUAGE_FRENCH:
        work->tiles2 = LoadObjTiles(gMapSaveTitleFrenchTiles, sizeof(gMapSaveTitleFrenchTiles));
        break;
    case LANGUAGE_SPANISH:
        work->tiles2 = LoadObjTiles(gMapSaveTitleSpanishTiles, sizeof(gMapSaveTitleSpanishTiles));
        break;
    case LANGUAGE_ITALIAN:
        work->tiles2 = LoadObjTiles(gMapSaveTitleItalianTiles, sizeof(gMapSaveTitleItalianTiles));
        break;
    case LANGUAGE_GERMAN:
    default:
        work->tiles2 = LoadObjTiles(gMapSaveTitleGermanTiles, sizeof(gMapSaveTitleGermanTiles));
        break;
    }

    work->palette2 = LoadObjPalette(gSaveMenuTitlePalette, sizeof(gSaveMenuTitlePalette));
#else
    work->palette2 = LoadObjPalette(gSaveMenuTitlePalette, sizeof(gSaveMenuTitlePalette));
    work->tiles2 = LoadObjTiles(gMapSaveTitleTiles, sizeof(gMapSaveTitleTiles));
#endif
    work->y = -0x800;
    work->y2 = 0xA000;
    work->x = -0x8000;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        work->tiles4 = AllocObjTiles(0x400, gRikuFf00Tiles);
        work->palette5 = LoadObjPalette(gRikuPalette, sizeof(gRikuPalette));
    } else {
        work->tiles4 = AllocObjTiles(0x340, gSor1ff00Tiles);
        work->palette5 = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    }

    work->palette6 = LoadObjPalette(gDonaldPalette, sizeof(gDonaldPalette));
    work->palette7 = LoadObjPalette(gGoofyPalette, sizeof(gGoofyPalette));
    work->playerStartX = gFieldState->actor.fieldPosition.x - gFieldState->x;
    work->playerStartY = gFieldState->actor.fieldPosition.y + gFieldState->actor.fieldPosition.z - gFieldState->y;
    work->x3 = work->playerStartX;
    work->y3 = work->playerStartY;
    work->confirmCursor = MAP_CONFIRM_CURSOR_NONE;
    work->dialogVisible = FALSE;
    work->steps = 16;
    work->update = MapSaveSlideInY;
    TaskPoolInit(&work->tasks, 1);
    MapSaveSetCharaPalettesExcluded(work, TRUE);
    FadeToAmount(FADE_MODE_BLACK, 16, 16);
    m4aSongNumStart(SONG_SYS_CANSEL);
}

s32 Task_MapSave_1(MapSaveWork* work) {
    TaskPoolUpdate(&work->tasks);

    if (work->confirmCursor != MAP_CONFIRM_CURSOR_NONE) {
        AnimUpdate(&work->anim);
    }

    if (work->update != NULL) {
        if ((u8)work->update(work) == 0) {
            return 0;
        }
    }

    return 1;
}

void Task_MapSave_2(MapSaveWork* work) {
#ifdef PLATFORM_PC
    PortScreenBegin(); /* menu: drawn on the screen layer, at the window's height */
    PortBgSplit(0, 8); /* the file panel goes right with its text */
#endif
    TaskPoolDraw(&work->tasks);

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        PORT_BAND(DrawSprite(128, work->y >> 8, gMapSaveTitleFrames[1], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        PORT_BAND(DrawSprite(128, work->y2 >> 8, gMapSaveTitleFrames[2], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapSaveTitleFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
        break;
    case LANGUAGE_FRENCH:
        PORT_BAND(DrawSprite(128, work->y >> 8, gMapSaveTitleFrenchFrames[1], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        PORT_BAND(DrawSprite(128, work->y2 >> 8, gMapSaveTitleFrenchFrames[2], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapSaveTitleFrenchFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
        break;
    case LANGUAGE_SPANISH:
        PORT_BAND(DrawSprite(128, work->y >> 8, gMapSaveTitleSpanishFrames[1], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        PORT_BAND(DrawSprite(128, work->y2 >> 8, gMapSaveTitleSpanishFrames[2], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapSaveTitleSpanishFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
        break;
    case LANGUAGE_ITALIAN:
        PORT_BAND(DrawSprite(128, work->y >> 8, gMapSaveTitleItalianFrames[1], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        PORT_BAND(DrawSprite(128, work->y2 >> 8, gMapSaveTitleItalianFrames[2], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapSaveTitleItalianFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
        break;
    case LANGUAGE_GERMAN:
    default:
        PORT_BAND(DrawSprite(128, work->y >> 8, gMapSaveTitleGermanFrames[1], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        PORT_BAND(DrawSprite(128, work->y2 >> 8, gMapSaveTitleGermanFrames[2], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
        DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapSaveTitleGermanFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
        break;
    }
#else
    PORT_BAND(DrawSprite(128, work->y >> 8, gMapSaveTitleFrames[1], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
    PORT_BAND(DrawSprite(128, work->y2 >> 8, gMapSaveTitleFrames[2], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 90));
    DrawSprite(PORT_MENU_LEFT(work->x >> 8), 0, gMapSaveTitleFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1), 80);
#endif

    if (gMapRoomState->flags & ROOM_FLAG_HIDE_PLAYER) {
        if (gGameState.flags & GAME_FLAG_RIKU) {
            DrawSprite(PORT_MENU_LEFT(work->x3 >> 8), work->y3 >> 8, gRikuFf00Frames[0], work->tiles4, work->palette5,
                NULL, SPRITE_PRIORITY(1), 80);
        } else {
            DrawSprite(PORT_MENU_LEFT(work->x3 >> 8), work->y3 >> 8, gSor1ff00Frames[0], work->tiles4,
                work->palette5, NULL, SPRITE_PRIORITY(1), 80);
        }
    }

    if (work->dialogVisible) {
        if (!(gGameState.flags & GAME_FLAG_RIKU)) {
            DrawSprite(PORT_MENU_LEFT(72), 96, gDonaFl00Frames[0], work->tiles5, work->palette6, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_HFLIP, 81);
            DrawSprite(PORT_MENU_LEFT(40), 96, gGoofyFl00Frames[0], work->tiles6, work->palette7, NULL, SPRITE_PRIORITY(1), 81);
        }

        DrawSprite(PORT_MENU_LEFT(0), 16, gSaveSlotCharaWinFrame0, work->tiles3, work->palette3, NULL, SPRITE_PRIORITY(1), 90);
        DrawTextSlots(PORT_MENU_RIGHT(100), 59, work->textSlots, work->palette4, 50, work->textSlotCount);

        if (work->confirmCursor != MAP_CONFIRM_CURSOR_NONE) {
#ifdef VERSION_EU
            DrawTextSlots(PORT_MENU_RIGHT(166 - (GetTextSlotsWidth(work->textSlots2, work->textSlotCount2) >> 1)), 92, work->textSlots2, work->palette8, 50, work->textSlotCount2);
#elif defined(VERSION_JP)
            DrawTextSlots(PORT_MENU_RIGHT(129), 92, work->textSlots2, work->palette8, 50, work->textSlotCount2);
#else
            DrawTextSlots(PORT_MENU_RIGHT(124), 92, work->textSlots2, work->palette8, 50, work->textSlotCount2);
#endif
            DrawTextSlots(PORT_MENU_RIGHT(128), 114, work->textSlots3, work->palette8, 50, work->textSlotCount3);
            DrawTextSlots(PORT_MENU_RIGHT(184), 114, work->textSlots4, work->palette8, 50, work->textSlotCount4);
        } else {
#ifdef VERSION_EU
            DrawTextSlots(PORT_MENU_RIGHT(166 - (GetTextSlotsWidth(work->textSlots2, work->textSlotCount2) >> 1)), 102, work->textSlots2, work->palette8, 50, work->textSlotCount2);
#elif defined(VERSION_JP)
            DrawTextSlots(PORT_MENU_RIGHT(129), 103, work->textSlots2, work->palette8, 50, work->textSlotCount2);
#else
            DrawTextSlots(PORT_MENU_RIGHT(130), 102, work->textSlots2, work->palette8, 50, work->textSlotCount2);
#endif
        }

        switch (work->confirmCursor) {
        case MAP_CONFIRM_CURSOR_YES:
            ApproachValueHalf(&work->x2, 0x7800);
            DrawSprite(PORT_MENU_RIGHT(work->x2 >> 8), 110, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_FLAG_HFLIP,
                40);
            break;
        case MAP_CONFIRM_CURSOR_NO:
            ApproachValueHalf(&work->x2, 0xB000);
            DrawSprite(PORT_MENU_RIGHT(work->x2 >> 8), 110, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_FLAG_HFLIP,
                40);
            break;
        }
    }
#ifdef PLATFORM_PC
    PortScreenEnd();
#endif
}

void Task_MapSave_3(MapSaveWork* work) {
#ifdef PLATFORM_PC
    PortBgSplit(0, 0);
#endif
    u32 roomFlags;

    MapSaveSetCharaPalettesExcluded(work, FALSE);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette3);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette5);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette6);
    ReleaseObjTiles(work->tiles5);
    ReleaseObjPalette(work->palette7);
    ReleaseObjTiles(work->tiles6);
    ReleaseObjPalette(work->palette4);
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    ReleaseObjPalette(work->palette8);
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    FreeTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
    roomFlags = gMapRoomState->flags & ~ROOM_FLAG_ATTACK_HIT;
    gMapRoomState->flags = roomFlags;
    gFieldState->flags &= ~FIELD_FLAG_FREEZE_PLAYER;
    gFieldState->flags &= ~FIELD_FLAG_FREEZE_ENEMIES;
    gMapRoomState->flags = roomFlags & ~ROOM_FLAG_SAVE_MENU_OPEN;
    TaskPoolDestroy(&work->tasks);
}

void Task_MapAnm_0(MapAnmWork* work, MapAnmEntry* list) {
    MapAnmSlot* slot;
    s32 i;

    slot = work->slots;

    for (i = 0; i < 8; i++) {
        MapAnmResetSlot(slot);
        slot++;
    }

    if (list != NULL) {
        if (list->script != NULL) {
            slot = work->slots;

            do {
                MapAnmSetupSlot(slot, list);
                slot++;
                list++;
            } while (list->script != NULL);
        }
    }
}

s32 Task_MapAnm_1(MapAnmWork* work) {
    s32 i;

    for (i = 0; i < 8; i++) {
        if (work->slots[i].script != NULL) {
            MapAnmUpdateSlot(&work->slots[i]);
        }
    }

    return 1;
}

void Task_MapAnm_2(MapAnmWork* work) {
}

void Task_MapAnm_3(MapAnmWork* work) {
}

const u8 gCellMasks[16][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x1F, 0x7F },
    { 0x01, 0x07, 0x1F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x80, 0xE0, 0xF8, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x00, 0x00, 0x00, 0x00, 0x80, 0xE0, 0xF8, 0xFE },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xE0, 0x80 },
    { 0xFE, 0xF8, 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00 },
    { 0x7F, 0x1F, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x07, 0x01 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0x00, 0x00, 0x02, 0x03, 0x02, 0x03, 0x01, 0x01 },
    { 0x04, 0x05, 0x00, 0x00, 0x01, 0x01, 0x04, 0x05 },
    { 0x01, 0x01, 0x06, 0x07, 0x06, 0x07, 0x00, 0x00 },
    { 0x08, 0x09, 0x01, 0x01, 0x00, 0x00, 0x08, 0x09 },
};

static const char sModeNameMapDbg[] = "Mode_MapDbg";

static const char sModeNameMapFld[] = "Mode_MapFld";

const u8 gSoraFloorEvents[13] = {
    EVENT_012_2F_ENTRANCE,
    EVENT_014_2F_DEMO,
    EVENT_017_3F_DEMO,
    EVENT_020_4F_DEMO,
    EVENT_023_5F_DEMO,
    EVENT_028_6F_DEMO,
    EVENT_032_8F_ENTRANCE,
    EVENT_035_8F_DEMO,
    EVENT_038_9F_DEMO,
    EVENT_042_10F_DEMO,
    EVENT_050_11F_GOAL_3,
    MAP_EVENT_NONE,
    EVENT_068_13F_CASTLE_OBLIVION_LAST3,
};

const u8 gRikuFloorEvents[13] = {
    EVENT_157_RIKU_B12F_DEMO,
    EVENT_159_RIKU_B11F_DEMO,
    MAP_EVENT_NONE,
    EVENT_162_RIKU_B9F_DEMO,
    EVENT_165_RIKU_B8F_DEMO,
    EVENT_167_RIKU_B7F_DEMO,
    EVENT_168_RIKU_B6F_DEMO,
    EVENT_169_RIKU_B5F_DEMO,
    EVENT_175_RIKU_B4F_DEMO,
    EVENT_184_RIKU_B3F_DEMO,
    EVENT_191_RIKU_B1F_ENTRANCE,
    MAP_EVENT_NONE,
    0,
};

static const char sModeNameMapFix[] = "Mode_MapFix";

static const char sModeNameMenuNew[] = "Mode_MenuNew";

static const char sModeNameMenuLoad[] = "Mode_MenuLoad";

static const char sModeNameMenuMsg[] = "Mode_MenuMsg";

u8 gSoraWorldBattleBase[14] = {
    BATTLE_AGRABAH_0,
    BATTLE_AGRABAH_0,
    BATTLE_ATLANTICA_0,
    BATTLE_OLYMPUS_COLISEUM_0,
    BATTLE_WONDERLAND_0,
    BATTLE_MONSTRO_0,
    BATTLE_HALLOWEEN_TOWN_0,
    BATTLE_NEVER_LAND_0,
    BATTLE_HOLLOW_BASTION_0,
    BATTLE_DESTINY_ISLANDS_0,
    BATTLE_TRAVERSE_TOWN_0,
    BATTLE_TWILIGHT_TOWN_0,
    BATTLE_CASTLE_OBLIVION_0,
    BATTLE_AGRABAH_0,
};

u8 gRikuWorldBattleBase[14] = {
    BATTLE_AGRABAH_0,
    BATTLE_AGRABAH_0,
    BATTLE_ATLANTICA_0,
    BATTLE_OLYMPUS_COLISEUM_0,
    BATTLE_WONDERLAND_0,
    BATTLE_MONSTRO_0,
    BATTLE_HALLOWEEN_TOWN_0,
    BATTLE_NEVER_LAND_0,
    BATTLE_RIKU_HOLLOW_BASTION_0,
    BATTLE_DESTINY_ISLANDS_0,
    BATTLE_TRAVERSE_TOWN_0,
    BATTLE_TWILIGHT_TOWN_0,
    BATTLE_CASTLE_OBLIVION_0,
    BATTLE_AGRABAH_0,
};

u8 (*gMapGmkSpotFuncs[14])(FldPos*) = {
    MapGmkFindFloor2x2,
    MapGmkFindLeftWallBase2x3,
    MapGmkFindRightWallBase2x3,
    MapGmkFindBackWallBase2x3,
    MapGmkFindLeftWallFace3x3,
    MapGmkFindRightWallFace3x3,
    MapGmkFindLeftWallTop2x2,
    MapGmkFindBackWallTop1x2,
    MapGmkFindRightWallTop2x2,
    MapGmkFindFloor3x3,
    MapGmkFindLeftWallBase1x4,
    MapGmkFindFloor4x4,
    MapGmkFindFloor5x5,
    MapGmkFindBaseFloor2x2,
};

u8 (*gMapAnmCmds[2])(MapAnmSlot*) = {
    MapAnmCmdEnd,
    MapAnmCmdLoop,
};

Mode gModeMapDbg = {
    sModeNameMapDbg,
    (ModeInitFunc)Mode_MapDbg_0,
    Mode_MapDbg_1,
    Mode_MapDbg_2,
};

Mode gModeMapFld = {
    sModeNameMapFld,
    (ModeInitFunc)Mode_MapFld_0,
    Mode_MapFld_1,
    Mode_MapFld_2,
};

u8 gWorldEntryEvents[14] = {
    MAP_EVENT_NONE,
    EVENT_107_AGRABAH_E0,
    EVENT_101_ATLANTICA_E0,
    EVENT_120_COLISEUM_E0,
    EVENT_094_WONDERLAND_E0,
    EVENT_074_MONSTORO_E0,
    EVENT_087_HALLOWEEN_TOWN_E0,
    EVENT_115_NEVERLAND_E0,
    EVENT_129_HOLLOWBASTION_E0,
    EVENT_053_12F_DESTINY_ISLAND_E0,
    EVENT_002_1F_TRAVERSE_TOWN_E0_1,
    EVENT_044_11F_TWILIGHT_TOWN_E0,
    EVENT_061_13F_ENTRANCE,
    EVENT_135_100ACREWOOD_START_RETRY,
};

Mode gModeMapFix = {
    sModeNameMapFix,
    (ModeInitFunc)Mode_MapFix_0,
    Mode_MapFix_1,
    Mode_MapFix_2,
};

Mode gModeMenuNew = {
    sModeNameMenuNew,
    (ModeInitFunc)Mode_MenuNew_0,
    Mode_MenuNew_1,
    Mode_MenuNew_2,
};

Mode gModeMenuLoad = {
    sModeNameMenuLoad,
    Mode_MenuLoad_0,
    Mode_MenuLoad_1,
    Mode_MenuLoad_2,
};

Mode gModeMenuMsg = {
    sModeNameMenuMsg,
    Mode_MenuMsg_0,
    Mode_MenuMsg_1,
    Mode_MenuMsg_2,
};

TaskDesc gTaskDescMapRnd = {
    "Task_MapRnd",
    (TaskInitFunc)Task_MapRnd_0,
    (TaskUpdateFunc)Task_MapRnd_1,
    (TaskDrawFunc)Task_MapRnd_2,
    (TaskDestroyFunc)Task_MapRnd_3,
    sizeof(MapRndWork),
};

TaskDesc gTaskDescMapFix = {
    "Task_MapFix",
    (TaskInitFunc)Task_MapFix_0,
    (TaskUpdateFunc)Task_MapFix_1,
    (TaskDrawFunc)Task_MapFix_2,
    (TaskDestroyFunc)Task_MapFix_3,
    sizeof(MapFixWork),
};

const MapDoorGfx gWorldMapDoorGfx[14] = {
    { gMapDoorWonderlandPalette, gMapDoorWonderlandSide0ClosedTiles, gMapDoorWonderlandSide0OpenTiles, gMapDoorWonderlandSide3ClosedTiles, gMapDoorWonderlandSide3OpenTiles, gMapDoorWonderlandSide2ClosedTiles, gMapDoorWonderlandSide2OpenTiles, gMapDoorWonderlandSide1ClosedTiles, gMapDoorWonderlandSide1OpenTiles },
    { gMapDoorAgrabahPalette, gMapDoorAgrabahSide0ClosedTiles, gMapDoorAgrabahSide0OpenTiles, gMapDoorAgrabahSide3ClosedTiles, gMapDoorAgrabahSide3OpenTiles, gMapDoorAgrabahSide2ClosedTiles, gMapDoorAgrabahSide2OpenTiles, gMapDoorAgrabahSide1ClosedTiles, gMapDoorAgrabahSide1OpenTiles },
    { gMapDoorAtlanticaPalette, gMapDoorAtlanticaSide0ClosedTiles, gMapDoorAtlanticaSide0OpenTiles, gMapDoorAtlanticaSide3ClosedTiles, gMapDoorAtlanticaSide3OpenTiles, gMapDoorAtlanticaSide2ClosedTiles, gMapDoorAtlanticaSide2OpenTiles, gMapDoorAtlanticaSide1ClosedTiles, gMapDoorAtlanticaSide1OpenTiles },
    { gMapDoorOlympusColiseumPalette, gMapDoorOlympusColiseumSide0ClosedTiles, gMapDoorOlympusColiseumSide0OpenTiles, gMapDoorOlympusColiseumSide3ClosedTiles, gMapDoorOlympusColiseumSide3OpenTiles, gMapDoorOlympusColiseumSide2ClosedTiles, gMapDoorOlympusColiseumSide2OpenTiles, gMapDoorOlympusColiseumSide1ClosedTiles, gMapDoorOlympusColiseumSide1OpenTiles },
    { gMapDoorWonderlandPalette, gMapDoorWonderlandSide0ClosedTiles, gMapDoorWonderlandSide0OpenTiles, gMapDoorWonderlandSide3ClosedTiles, gMapDoorWonderlandSide3OpenTiles, gMapDoorWonderlandSide2ClosedTiles, gMapDoorWonderlandSide2OpenTiles, gMapDoorWonderlandSide1ClosedTiles, gMapDoorWonderlandSide1OpenTiles },
    { gMapDoorMonstroPalette, gMapDoorMonstroSide0ClosedTiles, gMapDoorMonstroSide0OpenTiles, gMapDoorMonstroSide3ClosedTiles, gMapDoorMonstroSide3OpenTiles, gMapDoorMonstroSide2ClosedTiles, gMapDoorMonstroSide2OpenTiles, gMapDoorMonstroSide1ClosedTiles, gMapDoorMonstroSide1OpenTiles },
    { gMapDoorHalloweenTownPalette, gMapDoorHalloweenTownSide0ClosedTiles, gMapDoorHalloweenTownSide0OpenTiles, gMapDoorHalloweenTownSide3ClosedTiles, gMapDoorHalloweenTownSide3OpenTiles, gMapDoorHalloweenTownSide2ClosedTiles, gMapDoorHalloweenTownSide2OpenTiles, gMapDoorHalloweenTownSide1ClosedTiles, gMapDoorHalloweenTownSide1OpenTiles },
    { gMapDoorNeverLandPalette, gMapDoorNeverLandSide0ClosedTiles, gMapDoorNeverLandSide0OpenTiles, gMapDoorNeverLandSide3ClosedTiles, gMapDoorNeverLandSide3OpenTiles, gMapDoorNeverLandSide2ClosedTiles, gMapDoorNeverLandSide2OpenTiles, gMapDoorNeverLandSide1ClosedTiles, gMapDoorNeverLandSide1OpenTiles },
    { gMapDoorHollowBastionPalette, gMapDoorHollowBastionSide0ClosedTiles, gMapDoorHollowBastionSide0OpenTiles, gMapDoorHollowBastionSide3ClosedTiles, gMapDoorHollowBastionSide3OpenTiles, gMapDoorHollowBastionSide2ClosedTiles, gMapDoorHollowBastionSide2OpenTiles, gMapDoorHollowBastionSide1ClosedTiles, gMapDoorHollowBastionSide1OpenTiles },
    { gMapDoorDestinyIslandsPalette, gMapDoorDestinyIslandsSide0ClosedTiles, gMapDoorDestinyIslandsSide0OpenTiles, gMapDoorDestinyIslandsSide3ClosedTiles, gMapDoorDestinyIslandsSide3OpenTiles, gMapDoorDestinyIslandsSide2ClosedTiles, gMapDoorDestinyIslandsSide2OpenTiles, gMapDoorDestinyIslandsSide1ClosedTiles, gMapDoorDestinyIslandsSide1OpenTiles },
    { gMapDoorTraverseTownPalette, gMapDoorTraverseTownSide0ClosedTiles, gMapDoorTraverseTownSide0OpenTiles, gMapDoorTraverseTownSide3ClosedTiles, gMapDoorTraverseTownSide3OpenTiles, gMapDoorTraverseTownSide2ClosedTiles, gMapDoorTraverseTownSide2OpenTiles, gMapDoorTraverseTownSide1ClosedTiles, gMapDoorTraverseTownSide1OpenTiles },
    { gMapDoorTwilightTownPalette, gMapDoorTwilightTownSide0ClosedTiles, gMapDoorTwilightTownSide0OpenTiles, gMapDoorTwilightTownSide3ClosedTiles, gMapDoorTwilightTownSide3OpenTiles, gMapDoorTwilightTownSide2ClosedTiles, gMapDoorTwilightTownSide2OpenTiles, gMapDoorTwilightTownSide1ClosedTiles, gMapDoorTwilightTownSide1OpenTiles },
    { gMapDoorCastleOblivionPalette, gMapDoorCastleOblivionSide0ClosedTiles, gMapDoorCastleOblivionSide0OpenTiles, gMapDoorCastleOblivionSide3ClosedTiles, gMapDoorCastleOblivionSide3OpenTiles, gMapDoorCastleOblivionSide2ClosedTiles, gMapDoorCastleOblivionSide2OpenTiles, gMapDoorCastleOblivionSide1ClosedTiles, gMapDoorCastleOblivionSide1OpenTiles },
    { gMapDoorWonderlandPalette, gMapDoorWonderlandSide0ClosedTiles, gMapDoorWonderlandSide0OpenTiles, gMapDoorWonderlandSide3ClosedTiles, gMapDoorWonderlandSide3OpenTiles, gMapDoorWonderlandSide2ClosedTiles, gMapDoorWonderlandSide2OpenTiles, gMapDoorWonderlandSide1ClosedTiles, gMapDoorWonderlandSide1OpenTiles },
};

TaskDesc gTaskDescMapDoor = {
    "Task_MapDoor",
    (TaskInitFunc)Task_MapDoor_0,
    (TaskUpdateFunc)Task_MapDoor_1,
    (TaskDrawFunc)Task_MapDoor_2,
    (TaskDestroyFunc)Task_MapDoor_3,
    sizeof(MapDoorWork),
};

TaskDesc gTaskDescMapMenu = {
    "Task_MapMenu",
    (TaskInitFunc)Task_MapMenu_0,
    (TaskUpdateFunc)Task_MapMenu_1,
    (TaskDrawFunc)Task_MapMenu_2,
    (TaskDestroyFunc)Task_MapMenu_3,
    sizeof(MapMenuWork),
};

TaskDesc gTaskDescMapSave = {
    "Task_MapSave",
    (TaskInitFunc)Task_MapSave_0,
    (TaskUpdateFunc)Task_MapSave_1,
    (TaskDrawFunc)Task_MapSave_2,
    (TaskDestroyFunc)Task_MapSave_3,
    sizeof(MapSaveWork),
};

TaskDesc gTaskDescMapAnm = {
    "Task_MapAnm",
    (TaskInitFunc)Task_MapAnm_0,
    (TaskUpdateFunc)Task_MapAnm_1,
    (TaskDrawFunc)Task_MapAnm_2,
    (TaskDestroyFunc)Task_MapAnm_3,
    sizeof(MapAnmWork),
};
