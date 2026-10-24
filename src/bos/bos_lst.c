/**
 * bos_lst.c
 * Marluxia Final Form Boss
 */

#include "bos6.h"
#include "sprites_bos6.h"
#include "bos7_api.h"
#include "fade.h"
#include "songs.h"
#include <stdlib.h>
#include "bos7.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_bg_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "display.h"
#include "engine_math.h"
#include "gba/defines.h"
#include "m4a_song.h"
#include "obj_api.h"
#include "registration_data.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include "gba/macro.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "enemy_ids.h"
#include "macros.h"

static const EmyKind sBosLstEmyKind = { ENEMY_MARLUXIA_2, 256, 8, 8, 0, 128, 0 };

static const BattleBackgroundDef sBosLstBattleBackgroundDef = {
    gBosLstBgTiles, sizeof(gBosLstBgTiles), gBosLstBgPalettes, sizeof(gBosLstBgPalettes), { gBosLstBgMap, gBosLstBgMap, gBosLstBgMap, gBosLstBgMap }
};

static const LstAnimDef sLstAnimDefs[8] = {
    { gBosLstAnim0BgMap, 0, 8, FALSE, { 0, 0, 0 }, 3, 47, FALSE, -15, 17, 61, 65535, 60, FALSE, -43, -17, 80, 72, 0, 72 },
    { gBosLstAnim1BgMap, 0, 8, TRUE, { 0, 0, 0 }, 65533, 47, TRUE, 15, 17, 61, 1, 60, TRUE, 43, -17, 80, 184, 0, 72 },
    { gBosLstAnim2BgMap, 0, 8, FALSE, { 0, 0, 0 }, 3, 47, FALSE, -15, 17, 61, 65535, 60, FALSE, -43, -17, 80, 72, 0, 72 },
    { gBosLstAnim3BgMap, 0, 8, TRUE, { 0, 0, 0 }, 65533, 47, TRUE, 15, 17, 61, 1, 60, TRUE, 43, -17, 80, 184, 0, 72 },
    { gBosLstAnim4BgMap, 0, 8, FALSE, { 0, 0, 0 }, 3, 47, FALSE, -15, 17, 61, 65535, 60, FALSE, -43, -17, 80, 72, 0, 72 },
    { gBosLstAnim5BgMap, 0, 8, TRUE, { 0, 0, 0 }, 65533, 47, TRUE, 15, 17, 61, 1, 60, TRUE, 43, -17, 80, 184, 0, 72 },
    { gBosLstAnim6BgMap, 0, 8, FALSE, { 0, 0, 0 }, 3, 47, FALSE, -15, 17, 61, 65535, 60, FALSE, -43, -17, 80, 72, 0, 72 },
    { gBosLstAnim7BgMap, 0, 8, TRUE, { 0, 0, 0 }, 65533, 47, TRUE, 15, 17, 61, 1, 60, TRUE, 43, -17, 80, 184, 0, 72 },
};

static const u16 sBosLstBodyFrames[48] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
};

static const u8 sBosLstAnimSheets[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };

static const s32 sBosLstBobZ[16] = { -1, -2, -3, -4, -5, -6, -7, -8, -7, -6, -5, -4, -3, -2, -1, 0 };

static void* const sBosLstBgFrames[18][2] = {
    { gBosLstBgTiles, gBosLstAnim0BgMap },
    { gBosLstBgFrame1Tiles, gBosLstBgFrame1Map },
    { gBosLstBgFrame2Tiles, gBosLstBgFrame2Map },
    { gBosLstBgFrame3Tiles, gBosLstBgFrame3Map },
    { gBosLstBgFrame4Tiles, gBosLstBgFrame4Map },
    { gBosLstBgFrame5Tiles, gBosLstBgFrame5Map },
    { gBosLstBgFrame6Tiles, gBosLstBgFrame6Map },
    { gBosLstBgFrame7Tiles, gBosLstBgFrame7Map },
    { gBosLstBgFrame8Tiles, gBosLstBgFrame8Map },
    { gBosLstBgTiles, gBosLstAnim1BgMap },
    { gBosLstBgFrame1Tiles, gBosLstBgFrame10Map },
    { gBosLstBgFrame2Tiles, gBosLstBgFrame11Map },
    { gBosLstBgFrame3Tiles, gBosLstBgFrame12Map },
    { gBosLstBgFrame4Tiles, gBosLstBgFrame13Map },
    { gBosLstBgFrame5Tiles, gBosLstBgFrame14Map },
    { gBosLstBgFrame6Tiles, gBosLstBgFrame15Map },
    { gBosLstBgFrame7Tiles, gBosLstBgFrame16Map },
    { gBosLstBgFrame8Tiles, gBosLstBgFrame17Map },
};

TaskDesc gTaskDescBosLst = {
    "task_bos_lst",
    (TaskInitFunc)task_bos_lst_0,
    (TaskUpdateFunc)task_bos_lst_1,
    (TaskDrawFunc)task_bos_lst_2,
    (TaskDestroyFunc)task_bos_lst_3,
    sizeof(BosLstWork),
};

s32 BosLstSquare(s32 x) {
    return x * x;
}

s32 BosLstSquare2(s32 x) {
    return x * x;
}

enum BosLstEventStep {
    BOS_LST_EVENT_STEP_WAIT,
    BOS_LST_EVENT_STEP_APPROACH
};

void BosLstAdvanceEventStep(Task* task) {
    BosLstWork* work = task->work;

    work->eventStep = BOS_LST_EVENT_STEP_APPROACH;
}

void BosLstSetMode(BosLstWork* work, u16 moveMode, u16 attackKind) {
    u16 zero;

    zero = 0;
    work->moveMode = moveMode;
    work->attackKind = attackKind;
    work->cardRequests = zero;
    work->breakCount = zero;
}

void BosLstRequestCardUse(BosLstWork* work) {
    RequestEnemyCardUse(&work->body);
    work->cardRequests += 1;
}

void BosLstDestroyTasks(BosLstWork* work) {
    u32 i;

    for (i = 0; i < ARRAY_COUNT(work->lstTasks); i++) {
        if (work->lstTasks[i] != NULL) {
            TaskKill(&gBtlWork->taskPools[1], work->lstTasks[i]);
        }

        work->lstTasks[i] = NULL;
    }
}

u8 BosLstSpawnFal(BosLstWork* work, s32 kind) {
    LstFalArg arg;
    u8 spawned;
    TaskPool* pool;
    s32 range;
    s32 jitterY;
    s32 jitterZ;
    s32 spiralX;
    s32 spiralZ;
    s32 burstX;
    s32 burstY;
    s32 burstZ;

    spawned = FALSE;

    if (!work->hidden) {
        arg.kind = kind;
        arg.facing = work->facing;
        arg.falCount = &work->falCount;
        arg.x = work->x;
        arg.y = work->y + 0x400;
        arg.z = work->z - 0x1400;

        switch (kind) {
        default:
            arg.x += work->facing << 12;
            jitterY = (GetRandom() % 21 << 8) + 0x800;
            arg.y -= jitterY;
            jitterZ = (GetRandom() % 17 << 8) - 0x800;
            arg.z += jitterZ;
            pool = &gBtlWork->taskPools[1];
            break;
        case BOS_LST_FAL_DEFEAT_SPIRAL:
            range = 0x800;
            spiralX = (GetRandom() % 17 << 8) - range;
            arg.x += spiralX;
            spiralZ = (GetRandom() % 17 << 8) - range;
            arg.z += spiralZ;
            arg.angle = work->defeatTimer * 4;
            pool = &work->tasks;
            break;
        case BOS_LST_FAL_DEFEAT_BURST:
            burstX = (GetRandom() % 33 << 8) - 0x1000;
            arg.x += burstX;
            burstY = (GetRandom() % 21 << 8) + 0x800;
            arg.y -= burstY;
            burstZ = (GetRandom() % 65 << 8) - 0x2000;
            arg.z += burstZ;
            pool = &work->tasks;
            break;
        }

        TaskCreate(pool, &gTaskDescBosLstFal, &arg);
        spawned = TRUE;
    }

    return spawned;
}

void BosLstSetAnim(BosLstWork* work, u16 animId, u16 flags, u8 change) {
    u16 id;
    u16 sheetAnim;

    id = animId;
    sheetAnim = id * 2;

    if (work->facing < 0) {
        sheetAnim ^= 1;
    }

    if (work->turned == 1) {
        sheetAnim ^= 1;
    }

    switch (sBosLstAnimSheets[animId]) {
    case 0:
        SetObjTileSource(work->tiles, gBosLstMarluxiaTiles);
        AnimChangeWithTables(&work->anim, sheetAnim, flags, gBosLstMarluxiaAnims, gBosLstMarluxiaFrames);
        break;
    case 1:
        sheetAnim -= 14;
        SetObjTileSource(work->tiles, gBosLstMarluxiaDashTiles);
        AnimChangeWithTables(&work->anim, sheetAnim, flags, gBosLstMarluxiaDashAnims, gBosLstMarluxiaDashFrames);
        break;
    }

    if (change == TRUE) {
        AnimChange(&work->anim, sheetAnim, flags);
    } else {
        AnimReset(&work->anim);
        AnimStart(&work->anim, sheetAnim, flags);
    }

    work->animId = id;
    work->animFlags = flags;
    work->animFacing = work->facing;
}

void BosLstSetFacing(BosLstWork* work, s16 facing) {
    u8 faceLeft;

    if (work->facing != facing) {
        work->facing = facing;
        BosLstSetAnim(work, work->animId, work->animFlags, TRUE);
        faceLeft = 1;

        if (work->facing > 0) {
            faceLeft = (work->turned ^ faceLeft) != 0;
        } else if (work->turned == 0) {
            faceLeft = 0;
        }

        if (faceLeft == 1) {
            work->body.flags |= BTLOBJ_FLAG_FACING_LEFT;
            work->sub[0].body.flags |= BTLOBJ_FLAG_FACING_LEFT;
            work->sub[1].body.flags |= BTLOBJ_FLAG_FACING_LEFT;
        } else {
            work->body.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            work->sub[0].body.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            work->sub[1].body.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        }
    }
}

s16 BosLstFindActiveSub(BosLstWork* work) {
    s16 index;

    index = -1;

    if (!work->sub[0].defeated) {
        index = 0;
    } else if (!work->sub[1].defeated) {
        index = 1;
    }

    return index;
}

u8 BosLstSetSubAnim(BosLstWork* work, u16 animId) {
    s16 i;
    u8 changed;

    changed = FALSE;

    if (work->subsDefeated == TRUE) {
        work->sub[0].animId = 2;
        work->sub[1].animId = 2;
    } else {
        if (animId == 0) {
            i = 0;

            if (work->sub[i].defeated == TRUE) {
                work->sub[i].animId = 2;
            } else {
                work->sub[i].animId = 0;
            }

            work->sub[i].restartAnim = TRUE;
            i = 1;

            if (work->sub[i].defeated == TRUE) {
                work->sub[i].animId = 2;
            } else {
                work->sub[i].animId = 0;
            }

            work->sub[i].restartAnim = TRUE;
        } else {
            i = BosLstFindActiveSub(work);
            work->sub[i].animId = animId;
            i = i ^ 1;

            if (work->sub[i].defeated == TRUE) {
                work->sub[i].animId = 2;
            } else {
                work->sub[i].animId = 0;
            }

            work->sub[i].restartAnim = TRUE;
        }

        changed = TRUE;
    }

    return changed;
}

void BosLstTickCardDelay(BosLstWork* work) {
    s32 delay;
    BtlObj* actor;

    delay = work->cardDelay;
    work->cardDelay = delay - 0x100;

    if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
        work->cardDelay = delay - 0x200;
    }

    if (work->facing > 0) {
        actor = gBtlWork->actor;

        if (actor->x > work->x + 0x1000) {
            work->cardDelay -= 0x80;
        }
    } else {
        actor = gBtlWork->actor;

        if (actor->x < work->x - 0x1000) {
            work->cardDelay -= 0x80;
        }
    }
}

void BosLstUpdateBob(BosLstWork* work) {
    work->offsetZ = sBosLstBobZ[(work->bobFrame >> 2) & 15] << 8;
    work->bobFrame += 1;

    if (work->inEvent == TRUE) {
        work->offsetZ -= 0x1800;
    }
}

enum BosLstState {
    BOS_LST_STATE_MOVE,
    BOS_LST_STATE_ATTACK,
    BOS_LST_STATE_BREAK,
    BOS_LST_STATE_HURT,
    BOS_LST_STATE_DEFEATED,
    BOS_LST_STATE_RECOVER,
    BOS_LST_STATE_INACTIVE,
    BOS_LST_STATE_EVENT
};

enum BosLstMoveMode {
    BOS_LST_MOVE_MODE_HOVER,
    BOS_LST_MOVE_MODE_GROUND,
    BOS_LST_MOVE_MODE_KAMA,
    BOS_LST_MOVE_MODE_DASH,
    BOS_LST_MOVE_MODE_BITS,
    BOS_LST_MOVE_MODE_PLATFORM
};

enum BosLstAttackKind {
    BOS_LST_ATTACK_KIND_GROUND,
    BOS_LST_ATTACK_KIND_KAMA,
    BOS_LST_ATTACK_KIND_DASH,
    BOS_LST_ATTACK_KIND_CTR,
    BOS_LST_ATTACK_KIND_BITS,
    BOS_LST_ATTACK_KIND_HANABIRA,
    BOS_LST_ATTACK_KIND_PLATFORM_BITS
};

enum BosLstPlatformStep {
    BOS_LST_PLATFORM_STEP_DESCEND,
    BOS_LST_PLATFORM_STEP_RIDE,
    BOS_LST_PLATFORM_STEP_LEAVE
};

enum BosLstSubState {
    BOS_LST_SUB_STATE_IDLE,
    BOS_LST_SUB_STATE_CARD_ACTION,
    BOS_LST_SUB_STATE_CARD_BROKEN,
    BOS_LST_SUB_STATE_HURT,
    BOS_LST_SUB_STATE_DEFEATED,
    BOS_LST_SUB_STATE_RECOVER
};

void task_bos_lst_0(BosLstWork* work, TaskPool* pool) {
    BtlObj* actor;
    void* obj;
    void* anim;
    const void* kind;
    void* collider;
    BtlWork* btl;
    u32 i;

    if (pool == NULL) {
        work->inEvent = FALSE;
        work->eventStep = BOS_LST_EVENT_STEP_WAIT;
        work->task = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosLstFld, (void*)&sBosLstBattleBackgroundDef);
        work->state = BOS_LST_STATE_MOVE;
        work->x = 0x14000;
        work->z = -0x5400;
    } else {
        work->inEvent = TRUE;
        work->eventStep = BOS_LST_EVENT_STEP_WAIT;
        work->task = TaskCreate(pool, &gTaskDescBosLstFld, (void*)&sBosLstBattleBackgroundDef);
        work->state = BOS_LST_STATE_EVENT;
        work->x = 0x1D000;
        work->z = -0x14400;
    }

    work->hidden = FALSE;
    work->subsDefeated = FALSE;
    work->step = 0;
    BosLstSetMode(work, BOS_LST_MOVE_MODE_KAMA, BOS_LST_ATTACK_KIND_KAMA);
    work->bodyCycle = 0;
    work->facing = 1;
    work->flash = 0;
    work->prevFlash = 0xFFFF;
    work->y = 0x1F000;
    work->offsetX = 0;
    work->offsetY = 0;
    work->offsetZ = 0;
    actor = gBtlWork->actor;
    work->actorX = actor->x;
    work->actorY = actor->y;
    work->actorZ = actor->z;
#ifdef VERSION_EU
    work->unk_004 = 0;
#endif
    work->timer = 0;
    work->bobFrame = 0;
    work->frameCount = 0;
    work->platformTimer = 0;
    work->cardRequests = 0;
    work->breakCount = 0;
    work->falCount = 0;
    work->hurtTimer = 0;
    work->unk_078 = 0;
    work->cardDelay = 0x1E000;
    work->groundCount = 0;
    work->kamaCount = 0;
    work->dashCount = 0;
    work->bitRound = 0;
    work->playerOnPlatform = FALSE;
    work->platformStep = BOS_LST_PLATFORM_STEP_DESCEND;
    work->sub[0].defeated = FALSE;
    work->sub[0].unk_001 = 1;
    work->sub[0].restartAnim = TRUE;
    work->sub[0].state = BOS_LST_SUB_STATE_IDLE;
    work->sub[0].timer = 0;
    work->sub[0].hurtTimer = 0;
    work->sub[0].animId = 0;
    work->sub[0].curAnimId = -1;
    work->sub[1].defeated = FALSE;
    work->sub[1].unk_001 = 0;
    work->sub[1].restartAnim = TRUE;
    work->sub[1].state = BOS_LST_SUB_STATE_IDLE;
    work->sub[1].timer = 0;
    work->sub[1].hurtTimer = 0;
    work->sub[1].animId = 0;
    work->sub[1].curAnimId = -1;
    work->sub[0].tiles = AllocObjTiles(GetMaxSpriteTileBytes(gBosLstScythe0Frames, 16), gBosLstScythe0Tiles);
    work->sub[1].tiles = AllocObjTiles(GetMaxSpriteTileBytes(gBosLstScythe1Frames, 16), gBosLstScythe1Tiles);
    work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gBosLstMarluxiaFrames, 0x62), gBosLstMarluxiaTiles);
    work->palette = LoadObjPalette(gBosLstObjPalette, sizeof(gBosLstObjPalette));
    i = 0;
    obj = &work->body;
    anim = &work->anim;

    for (; i < ARRAY_COUNT(work->lstTasks); i++) {
        work->lstTasks[i] = NULL;
    }

    kind = &sBosLstEmyKind;
    InitEnemyBtlObj(obj, kind, work->x, work->y, work->z);
    work->body.flags |= 0x200000000400;
    SetBtlObjUnhittable(obj, TRUE);
    obj = &work->sub[0].body;
    InitEnemyBtlObj(obj, kind, work->x, work->y, work->z);

    work->sub[0].body.flags |= 0x400;
    SetEnemyHpFromStats(obj, ENEMY_MARLUXIA_2, Q_8_8(1));
    InitEnemyBtlObj(&work->sub[1].body, kind, work->x, work->y, work->z);
    work->sub[1].body.flags |= 0x400;
    SetEnemyHpFromStats(&work->sub[1].body, ENEMY_MARLUXIA_2, Q_8_8(1));
    ColliderInit(&work->collider, COLLIDER_TYPE_BOSS, 20, 20);
    collider = &work->collider2;
    ColliderInit(collider, COLLIDER_TYPE_BOSS, 28, 64);
    ColliderSetDisabled(collider, TRUE);

    for (i = 0; (s32)i < 8; i++) {
        ColliderInit(&work->colliders[i], COLLIDER_TYPE_PLATFORM, 24, 4);
        ColliderSetDisabled(&work->colliders[i], TRUE);
    }

    work->turned = 0;
    work->animId = 0;
    work->animFlags = 0;
    work->bgFrame = 0xFFFF;
    work->hittableTimer = 0;
    AnimInit(anim, gBosLstMarluxiaAnims, gBosLstMarluxiaFrames);
    BosLstSetAnim(work, 0, 1, FALSE);
    AnimInit(&work->sub[0].anim, gBosLstScythe0Anims, gBosLstScythe0Frames);
    AnimStart(&work->sub[0].anim, 0, ANIM_FLAG_LOOP);
    AnimInit(&work->sub[1].anim, gBosLstScythe1Anims, gBosLstScythe1Frames);
    AnimStart(&work->sub[1].anim, 0, ANIM_FLAG_LOOP);
    TaskPoolInit(&work->tasks, 0x60);
    SetBtlPaletteFadeExcluded(0, TRUE);
    SetBtlPaletteFadeExcluded(1, TRUE);
    SetBtlPaletteFadeExcluded(2, TRUE);
    SetBattleActorPosition(0xCC00, 0x1F000, 0);
    LoadBgMap(1, gBosBlankMap, sizeof(gBosBlankMap));
    LoadBgMap(1, gBosLstAnim0BgMap, sizeof(gBosLstAnim0BgMap));
    LoadBgMap(0, gBosLstBgMap, sizeof(gBosLstBgMap));
    btl = gBtlWork;
    btl->bossX = work->x;
    btl->bossY = work->y;
    btl->bossZ = work->z;
    btl->bossPriorityOffset = -16;
}

s32 BosLstApproachValue(s32 value, s32 target, s32 speed, s32 minSpeed, s32 maxSpeed) {
    if (speed == 0) {
        speed = Sqrt8((abs(value - target) << 8) / 768);

        if (speed < minSpeed) {
            speed = minSpeed;
        }

        if (speed > maxSpeed) {
            speed = maxSpeed;
        }
    }

    if (abs(value - target) < speed) {
        value = target;
    } else if (value < target) {
        value += speed;
    } else {
        value -= speed;
    }

    return value;
}

void BosLstMoveMode0(BosLstWork* work) {
    if (work->facing > 0) {
        work->x = BosLstApproachValue(work->x, 0x14800, 0, 0x80, 0x200);
    } else {
        work->x = BosLstApproachValue(work->x, 0xA800, 0, 0x80, 0x200);
    }

    work->y = BosLstApproachValue(work->y, 0x1F000, 0, 0x80, 0x200);
    work->z = BosLstApproachValue(work->z, -0x5400, 0, 0x80, 0x400);

    if (work->z == -0x5400) {
        BosLstTickCardDelay(work);

        if (work->cardDelay < 0) {
            work->cardDelay = 0x400;

            if (!(gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION)) {
                BosLstRequestCardUse(work);
            }
        }
    }
}

void BosLstMoveMode1(BosLstWork* work) {
    BtlWork** btl;
    s32 y;

    if (!work->subsDefeated) {
        BosLstSetMode(work, BOS_LST_MOVE_MODE_KAMA, BOS_LST_ATTACK_KIND_KAMA);
        work->kamaCount = 0;
    }

    btl = &gBtlWork;
    BosLstTickCardDelay(work);

    if (work->cardDelay < 0) {
        work->cardDelay = 0x400;

        if (((*btl)->flags & BTL_FLAG_PLAYER_CARD_ACTION) == 0) {
            work->groundVz = -0x1200;
            work->groundTargetX = (*btl)->actor->x - ((work->facing * 5) << 10);

            if (work->groundTargetX > 0x14000) {
                work->groundTargetX = 0x14000;
            }

            if (work->groundTargetX < 0xB000) {
                work->groundTargetX = 0xB000;
            }

            BosLstRequestCardUse(work);
        }
    }

    if (work->facing > 0) {
        if (work->x < 0x14800) {
            work->x = work->x + 192;
        }
    } else if (work->x > 0xA800) {
        work->x = work->x - 192;
    }

    y = 0x1F000;
    work->offsetX = (-COS(work->frameCount * 4) / 16) << 8;
    work->y = BosLstApproachValue(work->y, y, 0, 0x100, 0x200);
    work->z = BosLstApproachValue(work->z, -0xA400, 0x400, 0x100, 0x200);
}

void BosLstMoveMode2(BosLstWork* work) {
    if (work->subsDefeated == TRUE) {
        BosLstSetMode(work, BOS_LST_MOVE_MODE_DASH, BOS_LST_ATTACK_KIND_DASH);
        work->kamaCount = 0;
    }

    if (work->facing > 0) {
        work->x = BosLstApproachValue(work->x, 0x14800, 0, 0x80, 0x200);
    } else {
        work->x = BosLstApproachValue(work->x, 0xA800, 0, 0x80, 0x200);
    }

    work->y = BosLstApproachValue(work->y, 0x1F000, 0, 0x80, 0x200);
    work->z = BosLstApproachValue(work->z, -0x5400, 0, 0x80, 0x400);
    BosLstTickCardDelay(work);

    if (work->cardDelay < 0) {
        work->cardDelay = 0x400;

        if (!(gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION)) {
            BosLstRequestCardUse(work);
        }
    }
}

enum BosLstDashStep {
    BOS_LST_DASH_STEP_FLY_OFF,
    BOS_LST_DASH_STEP_ENTER,
    BOS_LST_DASH_STEP_REQUEST,
    BOS_LST_DASH_STEP_HOLD,
    BOS_LST_DASH_STEP_PASSED,
    BOS_LST_DASH_STEP_RETURN
};

void BosLstMoveDash(BosLstWork* work) {
    s16 height;
    s16 n;
    s32 target;
    s32 dir;
    s32 dir2;

    if (!work->subsDefeated) {
        BosLstSetMode(work, BOS_LST_MOVE_MODE_HOVER, BOS_LST_ATTACK_KIND_CTR);
        return;
    }

    if (work->dashCount > 0) {
        work->step = 1;
    }

    if (work->facing > 0) {
        work->x = BosLstApproachValue(work->x, 0x14800, 0, 0x80, 0x100);
    } else {
        work->x = BosLstApproachValue(work->x, 0xA800, 0, 0x80, 0x100);
    }

    work->y = BosLstApproachValue(work->y, 0x1F000, 0, 0x100, 0x400);
    work->z = BosLstApproachValue(work->z, -0x5400, 0, 0x100, 0x400);

    if (work->step == 0) {
        BosLstTickCardDelay(work);

        if (work->cardDelay < 0) {
            work->cardDelay = 0x400;

            if ((gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) == 0) {
                work->step += 1;
                work->dashCount += 1;
                work->dashStep = BOS_LST_DASH_STEP_FLY_OFF;
                work->dashSpeed = 0;
                work->unk_0AC = 0;
                work->timer = 0;
            }
        }

        if (work->step == 0) {
            return;
        }
    }

    switch (work->dashStep) {
    case BOS_LST_DASH_STEP_FLY_OFF:
        if (work->state != BOS_LST_STATE_DEFEATED) {
            if ((((s16)work->frameCount + 4) & 7) == 0) {
                BosLstSpawnFal(work, BOS_LST_FAL_DRIFT);
            }
        }

        if (work->z < -0x25400 || (work->facing > 0 && work->x < -0x7000) ||
            (work->facing < 0 && work->x > 0x26000)) {
            dir = work->facing;
            BosLstSetFacing(work, -dir);
            work->dashStep = BOS_LST_DASH_STEP_ENTER;
            work->timer = 0;
            work->dashSpeed = 0;
            height = 27 - (work->hpRatio >> 4);

            if (work->sub[0].defeated == TRUE) {
                height += 8;
                work->dashSpeed = 0x100;
            }

            if (work->sub[1].defeated == TRUE) {
                height += 8;
                work->dashSpeed += 0x100;
            }

            if (work->facing < 0) {
                work->x = -0x1000;
            } else {
                work->x = 0x20000;
            }

            work->y = 0x1F000;
            work->z = (-64 - height) << 8;
            work->dashVz = 0xA0;
        } else {
            work->dashSpeed += 64;

            if (work->dashSpeed > 0x800) {
                work->dashSpeed = 0x800;
            }

            work->x -= (work->facing << 1) * work->dashSpeed;
            work->z -= (work->dashSpeed * 192) >> 8;
        }

        break;
    case BOS_LST_DASH_STEP_ENTER:
        if (work->facing > 0) {
            work->x = BosLstApproachValue(work->x, 0x15800, 0, 0x100, 0x400);
        } else {
            work->x = BosLstApproachValue(work->x, 0x9800, 0, 0x100, 0x400);
        }

        work->y = BosLstApproachValue(work->y, 0x1F000, 0, 0x100, 0x400);
        work->timer += 1;

        if (work->timer == ((work->hpRatio * 60) >> 8) + 90) {
            work->dashStep = BOS_LST_DASH_STEP_REQUEST;
            work->timer = 0;
        }

        break;
    case BOS_LST_DASH_STEP_REQUEST:
        BosLstRequestCardUse(work);
        work->timer = 0;
        work->dashSpeed = (work->dashCount + 1) << 8;

        if (work->sub[0].defeated == TRUE) {
            work->dashSpeed += 0x100;
        }

        if (work->sub[1].defeated == TRUE) {
            work->dashSpeed += 0x100;
        }

        break;
    case BOS_LST_DASH_STEP_HOLD:
    case BOS_LST_DASH_STEP_PASSED:
        height = 39 - (work->hpRatio >> 4);

        if (work->sub[0].defeated == TRUE) {
            height += 8;
            work->dashSpeed += 0x100;
        }

        if (work->sub[1].defeated == TRUE) {
            height += 8;
            work->dashSpeed += 0x100;
        }

        work->z = (-64 - height) << 8;
        work->dashVz = 12;
        work->timer += 1;

        if (work->timer > 59) {
            n = 3;

            if (work->hpRatio <= 63) {
                n = 6;
            } else if (work->hpRatio <= 127) {
                n = 5;
            } else if (work->hpRatio <= 191) {
                n = 4;
            }

            work->dashCount += 1;

            if (work->dashCount >= n) {
                work->dashStep = BOS_LST_DASH_STEP_RETURN;
                work->timer = 0;

                if (gBtlWork->actor->x < 0xF800) {
                    work->x = 0x14800;
                    BosLstSetFacing(work, 1);
                } else {
                    work->x = 0xA800;
                    BosLstSetFacing(work, -1);
                }

                work->y = 0x1F000;
                work->z = -0x25400;
            } else {
                if (work->subsDefeated == TRUE) {
                    work->dashStep = BOS_LST_DASH_STEP_REQUEST;
                } else {
                    work->dashStep = BOS_LST_DASH_STEP_ENTER;
                }

                work->timer = 0;
                dir2 = work->facing;
                BosLstSetFacing(work, -dir2);
            }
        }

        break;
    case BOS_LST_DASH_STEP_RETURN:
        work->z = BosLstApproachValue(work->z, target = -0x5400, 0, 0x200, 0x800);

        if (work->z == target) {
            BosLstSetMode(work, BOS_LST_MOVE_MODE_HOVER, BOS_LST_ATTACK_KIND_CTR);
            work->timer = 0;
            work->dashCount = 0;
        }

        break;
    }
}

u8 BosLstAnyBitFiring(BosLstWork* work, s32 idx) {
    s32 i;
    u8 firing;

    firing = FALSE;

    if (idx < 0) {
        for (i = 0; i < work->lstTaskCount; i++) {
            if (BosLstBitHasShots(work->lstTasks[i]) == TRUE) {
                firing = TRUE;
                break;
            }
        }
    } else if (idx < work->lstTaskCount) {
        if (BosLstBitHasShots(work->lstTasks[idx]) == TRUE) {
            firing = TRUE;
        }
    }

    return firing;
}

#ifdef VERSION_EU
u8 BosLstAnyBitScaling(BosLstWork* work, s32 idx) {
    s32 i;
    u8 scaling;
    scaling = FALSE;

    if (idx < 0) {
        for (i = 0; i < work->lstTaskCount; i++) {
            if (BosLstBitIsScaling(work->lstTasks[i]) == TRUE) {
                scaling = TRUE;
                break;
            }
        }
    } else if (idx < work->lstTaskCount) {
        if (BosLstBitIsScaling(work->lstTasks[idx]) == TRUE) {
            scaling = TRUE;
        }
    }

    return scaling;
}
#endif

u8 BosLstAnyBitAlive(BosLstWork* work) {
    s16 found;
    s32 i;

    found = 0;

    for (i = 0; i < work->lstTaskCount; i++) {
        found = BosLstBitMarkFirstAlive(work->lstTasks[i], found);
    }

    return found != 0;
}

void BosLstHoverBits(BosLstWork* work) {
    s32 i;

    for (i = 0; i < work->lstTaskCount; i++) {
        if (BosLstBitIsAlive(work->lstTasks[i]) == TRUE) {
            BosLstBitStartHover(work->lstTasks[i]);
        }
    }
}

u8 BosLstFireBits(BosLstWork* work, s32 idx, s16 shots) {
    s32 i;
    u8 fired;

    fired = FALSE;

    if (idx < 0) {
        for (i = 0; i < work->lstTaskCount; i++) {
            if (BosLstBitIsAlive(work->lstTasks[i]) == TRUE) {
                BosLstBitStartFiring(work->lstTasks[i], shots);
                fired = TRUE;
            }
        }
    } else if (idx < work->lstTaskCount) {
        if (BosLstBitIsAlive(work->lstTasks[idx]) == TRUE) {
            BosLstBitStartFiring(work->lstTasks[idx], shots);
            fired = TRUE;
        }
    }

    return fired;
}

void BosLstReturnBits(BosLstWork* work) {
    s32 i;

    for (i = 0; i < work->lstTaskCount; i++) {
        if (BosLstBitIsAlive(work->lstTasks[i]) == TRUE) {
            BosLstBitStartReturn(work->lstTasks[i]);
        }
    }
}

void BosLstInterruptBits(BosLstWork* work) {
    s32 i;
    u8 destroy;

    destroy = TRUE;

    for (i = 0; i < work->lstTaskCount; i++) {
        if (BosLstBitIsAlive(work->lstTasks[i]) == TRUE) {
            if (BosLstBitInterrupt(work->lstTasks[i], destroy) == TRUE) {
                destroy = FALSE;
            }
        }
    }
}

enum BosLstBitStep {
    BOS_LST_BIT_STEP_HOVER,
    BOS_LST_BIT_STEP_FIRE,
    BOS_LST_BIT_STEP_RETURN,
    BOS_LST_BIT_STEP_END
};

void BosLstMoveBits(BosLstWork* work) {
    LstBitArg arg;
    BtlObj* obj;
    s32 i;
    s16* bitStep;
    s16* bitRound;
    s32 round;

    obj = &work->body;

    if (work->facing > 0) {
        work->x = BosLstApproachValue(work->x, 0x14800, 0, 0x100, 0x400);
    } else {
        work->x = BosLstApproachValue(work->x, 0xA800, 0, 0x100, 0x400);
    }

    work->y = BosLstApproachValue(work->y, 0x1F000, 0, 0x100, 0x400);
    work->z = BosLstApproachValue(work->z, -0x5400, 0, 0x100, 0x400);
    round = work->bitRound;
    bitRound = &work->bitRound;
    bitStep = &work->bitStep;

    if (round == 0) {
        work->lstTaskCount = 3;
        arg.x = obj->x;
        arg.y = obj->y - 0x1100;
        arg.z = obj->z + 0x800;

        for (i = 0; i < work->lstTaskCount; i++) {
            arg.kind = BOS_LST_BIT_KIND_BITS;
            arg.index = i;
            arg.facing = &work->facing;
            arg.falCount = &work->falCount;
            arg.unk_10 = &work->unk_004;
            arg.x2 = arg.x + (i << 11);
            arg.y2 = obj->y + 0x1400;
            arg.z2 = arg.z + ((i << 2) << 8);
            work->lstTasks[i] = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosLstBit, &arg);
        }

        *bitStep = BOS_LST_BIT_STEP_HOVER;
        *bitRound += 1;
        work->timer = 0;
        work->cardDelay = 0xC000;
        BosLstSetAnim(work, 5, 0, TRUE);
        m4aSongNumStart(SONG_VO_MARL_ATTACK01);
    }

    if (!BosLstAnyBitAlive(work)) {
        BosLstDestroyTasks(work);
        work->timer = 0;
        *bitStep = BOS_LST_BIT_STEP_HOVER;
        *bitRound = 0;
        BosLstSetMode(work, BOS_LST_MOVE_MODE_HOVER, BOS_LST_ATTACK_KIND_HANABIRA);
        BosLstSetAnim(work, 0, 1, FALSE);
    } else {
        switch (*bitStep) {
        case BOS_LST_BIT_STEP_HOVER:
            work->timer += 1;

            if (work->timer > 30) {
                work->timer = 0;
                *bitStep += 1;
            }

            break;
        case BOS_LST_BIT_STEP_FIRE:
            if (work->unk_004 <= 0) {
                work->cardDelay -= 0x100;
            }

            if (work->cardDelay <= 0) {
                work->cardDelay = 0x400;

#ifdef VERSION_EU
                if (BosLstAnyBitScaling(work, -1)) {
                    break;
                }
#endif

                if ((gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) == 0) {
                    BosLstRequestCardUse(work);
                    work->bitAttackStarted = 0;
                }
            } else if (((work->cardDelay >> 8) & 0x3F) == 0) {
                BosLstHoverBits(work);
            }

            break;
#ifdef VERSION_EU
        default:
            BosLstReturnBits(work);
            break;
#endif
        }
    }
}

s32 BosLstGetPlatformY(BosLstWork* work) {
    s32 base;
    s16 dist;
    s32 y;

    base = work->y + work->offsetZ;
    y = base + 0x1A00;
    dist = abs((gBtlWork->actor->x - work->x) >> 8);

    if (dist > 23) {
        if (dist <= 83) {
            y -= (dist / 3) << 8;
        } else if (dist <= 143) {
            y = base - 0x400;
        } else {
            base -= 0x400;
            y = base + (((dist - 144) / 4) << 8);
        }
    }

    return y;
}

void BosLstMovePlatform(BosLstWork* work) {
    LstBitArg arg;
    BtlObj* obj;
    s32 i;
    s32 step;
    s32 target;
    s32 minSpeed;
    u8 found;
    s32* pDC;
    s32* posZ;
    u16 timer;
    s32 z;

    work->hittableTimer = 30;
    step = work->platformStep;

    switch (step) {
    case BOS_LST_PLATFORM_STEP_DESCEND:
        work->z = BosLstApproachValue(work->z, target = -0x2000, 0x400, minSpeed = 0x100, 0x400);

        if (work->z != target) {
            break;
        }

        work->platformStep += 1;
        work->platformTimer = step;
        work->playerOnPlatform = FALSE;
        work->platformSpeed = minSpeed;
        work->bitRound = step;

        for (i = 0; i < 8; i++) {
            ColliderSetDisabled(&work->colliders[i], FALSE);
        }
    case BOS_LST_PLATFORM_STEP_RIDE:
        obj = &work->body;
        timer = work->platformTimer;

        if (work->platformTimer == 0) {
            work->platformTimer = timer + 1;
            work->lstTaskCount = 3;
            arg.x = obj->x;
            arg.y = obj->y - 0x1100;
            arg.z = obj->z + 0x800;

            for (i = 0; i < work->lstTaskCount; i++) {
                arg.kind = BOS_LST_BIT_KIND_PLATFORM;
                arg.index = i;
                arg.facing = &work->facing;
                arg.falCount = &work->falCount;
                arg.unk_10 = &work->unk_004;
                arg.x2 = arg.x + (i << 11);
                arg.y2 = obj->y + 0x1400;
                arg.z2 = arg.z + ((i << 2) << 8);
                work->lstTasks[i] = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosLstBit, &arg);
            }
        }

        if ((gBtlWork->flags & BTL_FLAG_PLAYER_OFFSCREEN) == 0) {
            work->platformTimer += 1;
        }

        found = FALSE;

        for (i = 0; i < 8; i++) {
            if (IsPlayerOnPlatform(&work->colliders[i]) == TRUE) {
                found = TRUE;
                break;
            }
        }

        if (found == TRUE) {
            if (!work->playerOnPlatform) {
                BosLstSetAnim(work, 2, 0, TRUE);
            }

            work->playerOnPlatform = found;
            work->z = BosLstApproachValue(work->z, -0x16800, 0, 0x100, 0x400);
        }

        if (work->playerOnPlatform == TRUE) {
            if (BosLstAnyBitAlive(work) == TRUE) {
                BosLstTickCardDelay(work);

                if (work->cardDelay < 0) {
                    work->cardDelay = 0x2000;
                    BosLstRequestCardUse(work);
                    work->bitAttackStarted = 0;
                } else if ((work->platformTimer & 0x1F) == 0) {
                    BosLstHoverBits(work);
                }
            } else {
                work->platformSpeed += 1;
            }

            work->platformSpeed += 1;

            if (work->platformSpeed > 0x900) {
                work->platformSpeed = 0x900;
            }

            BosLstFldSetScrollSpeed(work->task, work->platformSpeed);
        } else if (work->platformTimer > 180) {
            BosLstReturnBits(work);
            work->platformStep += 1;
            work->platformSpeed = 0;
        }

        if (work->platformTimer > 0x4AF ||
            (work->playerOnPlatform == TRUE && (gBtlWork->flags & BTL_FLAG_PLAYER_OFFSCREEN) == 0 &&
             gBtlWork->actor->z > work->z + work->offsetZ + 0x1800)) {
            BosLstReturnBits(work);
            work->platformStep += 1;
        }

        work->y = BosLstApproachValue(work->y, 0x1F000, 0x100, 0x100, 0x400);
        break;
    case BOS_LST_PLATFORM_STEP_LEAVE:
        posZ = &work->z;

        for (i = 0; i < 8; i++) {
            ColliderSetDisabled(&work->colliders[i], TRUE);
        }

        ApproachValueHalfSteps(posZ, -0x16800, 48);

        if (work->facing < 0) {
            if (work->x < 0x26000) {
                work->x += 0x800;
                return;
            }
        } else {
            if (work->x > -0x7000) {
                work->x -= 0x800;
                return;
            }
        }

        BosLstDestroyTasks(work);
        work->bitRound = 0;

        if (gBtlWork->actor->x < 0xF800) {
            work->x = 0x14800;
            BosLstSetFacing(work, 1);
        } else {
            work->x = 0xA800;
            BosLstSetFacing(work, -1);
        }

        work->y = 0x1F000;
        work->z = -0x26800;
        BosLstSetMode(work, BOS_LST_MOVE_MODE_HOVER, BOS_LST_ATTACK_KIND_CTR);
        work->playerOnPlatform = FALSE;
        work->platformStep = BOS_LST_PLATFORM_STEP_DESCEND;
        work->platformTimer = 0;
        work->turned = 0;
        BosLstSetAnim(work, 0, 1, FALSE);
        break;
    }
}

u8 BosLstUpdateMove(BosLstWork* work) {
    switch (work->moveMode) {
    case BOS_LST_MOVE_MODE_HOVER:
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_STAR_DRIFT, work->facing);
        BosLstMoveMode0(work);
        break;
    case BOS_LST_MOVE_MODE_GROUND:
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_TUNNEL, work->facing);
        BosLstMoveMode1(work);
        break;
    case BOS_LST_MOVE_MODE_KAMA:
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_TUNNEL, work->facing);
        BosLstMoveMode2(work);
        break;
    case BOS_LST_MOVE_MODE_DASH:
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_HORIZON, work->facing);
        BosLstMoveDash(work);
        break;
    case BOS_LST_MOVE_MODE_BITS:
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_STAR_DRIFT, work->facing);
        BosLstMoveBits(work);
        break;
    case BOS_LST_MOVE_MODE_PLATFORM:
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_STAR_STREAM, work->facing);
        BosLstMovePlatform(work);
        break;
    }

    BosLstUpdateBob(work);
    return 1;
}

enum BosLstGroundStep {
    BOS_LST_GROUND_STEP_LEAP,
    BOS_LST_GROUND_STEP_DONE,
    BOS_LST_GROUND_STEP_IDLE,
    BOS_LST_GROUND_STEP_IMPACT,
    BOS_LST_GROUND_STEP_SHOCKWAVE,
    BOS_LST_GROUND_STEP_RETURN
};

u8 BosLstAttackGround(BosLstWork* work) {
    s32 hpRatio;
    u8 running;

    running = 1;
    hpRatio = work->hpRatio;

    if (work->step == 0) {
        work->step += 1;
        work->groundCount += 1;
        work->groundStep = BOS_LST_GROUND_STEP_LEAP;
        work->timer = 0;
    }

    switch (work->groundStep) {
    case BOS_LST_GROUND_STEP_LEAP:
        if (work->timer == 0) {
            m4aSongNumStart(SONG_VO_MARL_ATTACK03);
            work->timer += 1;
        }

        work->x = BosLstApproachValue(work->x, work->groundTargetX, 0, 384, 768);
        work->z += work->groundVz;
        work->groundVz += 320;

        if (work->z > -0x5400) {
            work->z = -0x5400;
            work->groundStep = BOS_LST_GROUND_STEP_IMPACT;
            work->timer = 0;
        }

        break;
    case BOS_LST_GROUND_STEP_DONE:
    case BOS_LST_GROUND_STEP_IDLE:
        break;
    case BOS_LST_GROUND_STEP_IMPACT:
        BgFxStartMahluxiaGround(work->x + (work->facing << 12), work->y, 0, 268);
        m4aSongNumStart(SONG_EF_MARL_GROUND);
        work->groundStep = BOS_LST_GROUND_STEP_SHOCKWAVE;
        work->timer = 0;
    case BOS_LST_GROUND_STEP_SHOCKWAVE:
        work->timer += 1;

        if (work->timer <= 19) {
            if (ApplyAttackBox(268, gBtlWork->actor->x,
                              gBtlWork->actor->y, 0,
                              32, 32, ((hpRatio * 8) >> 8) + 8) != 0) {
                m4aSongNumStart(SONG_BTL_MARL_GROUNDHIT);
            }
        }

        if (!BgFxIsActive()) {
            if (work->timer >= ((hpRatio * 30) >> 8) + 31) {
                work->groundStep = BOS_LST_GROUND_STEP_RETURN;
                work->timer = 0;

                if (work->groundCount > 2) {
                    if (!work->subsDefeated) {
                        BosLstSetMode(work, BOS_LST_MOVE_MODE_KAMA, BOS_LST_ATTACK_KIND_KAMA);
                    } else {
                        BosLstSetMode(work, BOS_LST_MOVE_MODE_DASH, BOS_LST_ATTACK_KIND_DASH);
                    }

                    work->groundCount = 0;
                    running = 0;
                }
            }
        }

        break;
    case BOS_LST_GROUND_STEP_RETURN:
        work->z = BosLstApproachValue(work->z, -0x5400, 0, 256, 1024);

        if (work->z == -0x5400) {
            work->groundStep = BOS_LST_GROUND_STEP_DONE;
            work->timer = 0;
            running = 0;
        }

        break;
    }

    return running;
}

enum BosLstKamaStep {
    BOS_LST_KAMA_STEP_WINDUP,
    BOS_LST_KAMA_STEP_ASCEND,
    BOS_LST_KAMA_STEP_AIM,
    BOS_LST_KAMA_STEP_DIVE,
    BOS_LST_KAMA_STEP_SLASH,
    BOS_LST_KAMA_STEP_END
};

u8 BosLstAttackKama(BosLstWork* work) {
    BtlObj* sub;
    s16* kamaStep;
    s32 step;
    u16 attackStep;
    s16 zero;
    u8 running;

    running = 1;
    sub = &work->sub[BosLstFindActiveSub(work)].body;
    attackStep = work->step;

    if (work->step == 0) {
        work->step = attackStep + 1;
        work->kamaCount += 1;
        work->kamaStep = BOS_LST_KAMA_STEP_WINDUP;
        work->timer = 0;
    }

    step = work->kamaStep;
    kamaStep = &work->kamaStep;

    switch (step) {
    case BOS_LST_KAMA_STEP_WINDUP:
        if (work->timer == 0) {
            m4aSongNumStart(SONG_SND_712);
            BosLstSetSubAnim(work, 3);
        }

        work->timer += 1;

        if (work->timer > 30) {
            *kamaStep = BOS_LST_KAMA_STEP_ASCEND;
            work->timer = 0;
            m4aSongNumStart(SONG_VO_MARL_ATTACK00);
        }

        break;
    case BOS_LST_KAMA_STEP_ASCEND:
        work->timer += 1;
        work->x += work->facing * 0x600;
        work->z -= work->timer << 8;

        if (work->z < -0x1E000) {
            *kamaStep = BOS_LST_KAMA_STEP_DIVE;
            work->timer = 0;
            work->kamaStartY = work->y;
            work->kamaStartZ = work->z;

            if (gBtlWork->actor->x > 0xF7FF) {
                work->kamaStartX = (GetRandom() % 41 << 8) + 0xB000;
                BosLstSetFacing(work, 1);
            } else {
                work->kamaStartX = (GetRandom() % 40 << 8) + 0x11800;
                BosLstSetFacing(work, -1);
            }

            BosLstSetSubAnim(work, 5);
            work->x = work->kamaStartX;
            work->kamaTargetX = gBtlWork->actor->x + work->facing * 0x3000;
            work->kamaTargetY = 0x1F000;
            work->kamaTargetZ = -0x5400;
        }

        break;
    case BOS_LST_KAMA_STEP_AIM:
        zero = 0;
        *kamaStep = BOS_LST_KAMA_STEP_DIVE;
        work->timer = zero;
        work->kamaTargetX = gBtlWork->actor->x;
        work->kamaTargetY = 0x1F000;
        work->kamaTargetZ = -0x5400;
        break;
    case BOS_LST_KAMA_STEP_DIVE:
        if (work->timer <= 7) {
            ApproachValueHalfSteps(&work->kamaTargetX, gBtlWork->actor->x + work->facing * 0x3000, 8);
        }

        if (work->timer == 0) {
            work->kamaTargetY = BosLstApproachValue(work->kamaTargetY, gBtlWork->actor->y, 0x100, 0x100, 0x100);
        }

        work->x = BosLstApproachValue(work->x, work->kamaTargetX, 0, 0x100, 0x800);
        work->y = work->kamaTargetY;
        work->z = BosLstApproachValue(work->z, work->kamaTargetZ, 0, 0x1000, 0x1800);

        if (work->z == work->kamaTargetZ) {
            work->timer += 1;

            if (work->timer > 16) {
                *kamaStep = BOS_LST_KAMA_STEP_SLASH;
                work->timer = 0;
                work->unk_078 = 0;
                BosLstSetSubAnim(work, 4);
                BgFxStartKama(sub->x, work->kamaTargetY, sub->z + 0x2800, -(work->facing * 0x3000), 0x10A);
                m4aSongNumStart(SONG_EF_MARL_KAMAEF);
            }
        }

        break;
    case BOS_LST_KAMA_STEP_SLASH:
        switch (work->timer) {
        case 0:
            if (ApplyAttackBox(0x10A, sub->x - (work->facing << 13), work->kamaTargetY, sub->z, 48, 12, 64) != ATTACK_RESULT_NONE) {
                m4aSongNumStart(SONG_BTL_MARL_EFEHIT);
            }
        case 1:
        case 2:
        case 3:
            work->x -= work->facing << 9;
            work->z += 0x400;
            break;
        case 4:
        case 5:
        case 6:
            work->x -= work->facing * 0x300;
            work->z += 0x600;
            break;
        case 7:
        case 8:
        case 9:
            work->x -= work->facing << 9;
            work->z += 0x400;
            break;
        case 10:
        case 11:
            work->z -= 0x300;
            break;
        case 12:
        case 13:
            work->z -= 0x180;
            break;
        }

        work->timer += 1;

        if (!BgFxIsActive()) {
            *kamaStep = BOS_LST_KAMA_STEP_END;
            work->timer = 0;
            BosLstSetSubAnim(work, 0);
        }

        break;
    case BOS_LST_KAMA_STEP_END:
        if (work->kamaCount > 2) {
            BosLstSetMode(work, BOS_LST_MOVE_MODE_DASH, BOS_LST_ATTACK_KIND_DASH);
            work->kamaCount = 0;
            running = 0;
        } else {
            zero = 0;
            *kamaStep = zero;
            work->timer = zero;
            running = zero;
        }

        break;
    }

    return running;
}

u8 BosLstAttackDash(BosLstWork* work) {
    u8 running;

    running = 1;

    if (work->step == 0) {
        BosLstSetAnim(work, 7, 0, TRUE);
        m4aSongNumStart(SONG_VO_MARL_ATTACK01);
        m4aSongNumStart(SONG_VO_MARL_ATTACK02);
        work->step += 1;
    }

    BosLstSpawnFal(work, BOS_LST_FAL_DASH);

    if (work->timer == 30) {
        m4aSongNumStart(SONG_SND_711);
    }

    if (work->timer > 30) {
        work->dashSpeed += 32;

        if (work->dashSpeed > 0x800) {
            work->dashSpeed = 0x800;
        }

        work->x = work->x - work->facing * work->dashSpeed;

        if (work->facing > 0) {
            if (work->x <= -0x8000) {
                running = 0;
            }
        } else if (work->x >= 0x27000) {
            running = 0;
        }

        if (work->dashVz > 0) {
            work->dashVz = work->dashVz + 8;
        } else {
            work->dashVz = work->dashVz - 8;
        }

        work->z += work->dashVz;

        if (ApplyAttackBox(0x10B, work->x, work->y, work->z + 0x4000, 12, 32, 64) != ATTACK_RESULT_NONE) {
            m4aSongNumStart(SONG_BTL_MON_HIT06);
        }

        if (running == 0) {
            work->dashStep = BOS_LST_DASH_STEP_PASSED;
            work->timer = 0;
            BosLstSetAnim(work, 0, 1, TRUE);
        }
    } else {
        work->timer += 1;
    }

    return running;
}

enum BosLstCtrAttackStep {
    BOS_LST_CTR_ATTACK_STEP_START,
    BOS_LST_CTR_ATTACK_STEP_POSE,
    BOS_LST_CTR_ATTACK_STEP_SPAWN,
    BOS_LST_CTR_ATTACK_STEP_WAIT
};

u8 BosLstAttackCtr(BosLstWork* work) {
    LstCtrArg arg;
    BtlObj* obj;
    s16 interval;
    s32 i;
    u8 running;

    obj = &work->body;
    running = 1;

    switch (work->step) {
    case BOS_LST_CTR_ATTACK_STEP_START:
        m4aSongNumStart(SONG_SND_709);

        if (work->facing > 0) {
            BgFxStartLstCtrFlipped(obj->x - 0x1800, obj->y - 0x400, obj->z, Q_8_8(1));
        } else {
            BgFxStartLstCtr(obj->x + 0x1800, obj->y - 0x400, obj->z, Q_8_8(1));
        }

        work->timer = 0;
        work->step += 1;
        break;
    case BOS_LST_CTR_ATTACK_STEP_POSE:
        BosLstSetAnim(work, 6, 0, TRUE);
        m4aSongNumStart(SONG_VO_MARL_ATTACK01);
        work->step += 1;
        break;
    case BOS_LST_CTR_ATTACK_STEP_SPAWN:
        work->timer += 1;

        if (work->timer == 32) {
            work->ctrCount = 3 - work->breakCount;

            if (work->ctrCount <= 0) {
                work->ctrCount = 1;
            }

            if (work->sub[0].defeated == TRUE) {
                work->ctrCount += 1;
            }

            if (work->sub[1].defeated == TRUE) {
                work->ctrCount += 1;
            }

            interval = ((work->hpRatio * 30) / 256) + 30;

            if (interval <= 44) {
                interval = 45;
            }

            arg.x = obj->x;
            arg.y = obj->y + 0x800;
            arg.z = obj->z - 0x1000;

            for (i = 0; i < work->ctrCount; i++) {
                arg.unk_00 = &work->unk_004;
                arg.count = work->ctrCount;
                arg.index = i;
                arg.delay = i * interval + 90;
                work->lstTasks[i] = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosLstCtr, &arg);
            }

            work->unk_004 = 0;
            work->timer = 0;
            work->step += 1;
        }

        break;
    default:
        running = 0;

        for (i = 0; i < work->ctrCount; i++) {
            if (BosLstCtrIsActive(work->lstTasks[i]) == TRUE) {
                running = 1;
                break;
            }
        }

        if (running == 0) {
            BosLstDestroyTasks(work);
            BosLstSetMode(work, BOS_LST_MOVE_MODE_BITS, BOS_LST_ATTACK_KIND_BITS);
            BosLstSetAnim(work, 0, 1, FALSE);
        }

        break;
    }

    BosLstUpdateBob(work);
    return running;
}

u8 BosLstAttackBits(BosLstWork* work) {
    s16 shots;
    u8 running;

    running = 1;

    if (work->bitAttackStarted == 0) {
        work->bitAttackStarted += 1;

        if (work->bitRound <= 1) {
            work->bitStep = BOS_LST_BIT_STEP_FIRE;
        } else {
            work->bitStep = BOS_LST_BIT_STEP_HOVER;
        }

        work->timer = 0;
    }

    if (!BosLstAnyBitAlive(work)) {
        BosLstDestroyTasks(work);
        BosLstSetMode(work, BOS_LST_MOVE_MODE_HOVER, BOS_LST_ATTACK_KIND_HANABIRA);
        work->bitRound = 0;
        work->bitStep = BOS_LST_BIT_STEP_END;
        work->timer = 0;
    }

    switch (work->bitStep) {
    case BOS_LST_BIT_STEP_HOVER:
        if (work->timer == 0) {
            BosLstHoverBits(work);
        }

        work->timer += 1;

        if (work->timer > 59) {
            work->bitStep = BOS_LST_BIT_STEP_FIRE;
            work->timer = 0;
        }

        break;
    case BOS_LST_BIT_STEP_FIRE:
        shots = 1;

        if (work->sub[0].defeated == TRUE) {
            shots = 2;
        }

        if (work->sub[1].defeated == TRUE) {
            shots++;
        }

        switch (work->timer) {
        case 0:
            if (work->attackKind == BOS_LST_ATTACK_KIND_PLATFORM_BITS) {
                BosLstFireBits(work, -1, 1);
                break;
            }

            if (!BosLstAnyBitFiring(work, 1)) {
                if (BosLstFireBits(work, 1, shots) == TRUE) {
                    break;
                }
            }
        case 12:
            if (!BosLstAnyBitFiring(work, 0)) {
                if (BosLstFireBits(work, 0, shots) == TRUE) {
                    break;
                }
            }
        case 24:
            if (!BosLstAnyBitFiring(work, 2)) {
                BosLstFireBits(work, 2, shots);
            }

            break;
        }

        work->timer += 1;

        if (!BosLstAnyBitFiring(work, -1)) {
            work->timer = 0;

            if (work->bitRound > 6) {
                work->bitStep = BOS_LST_BIT_STEP_RETURN;
            } else {
                if (work->moveMode != BOS_LST_MOVE_MODE_PLATFORM) {
                    work->bitRound += 1;
                }

                running = 0;
            }
        }

        break;
    case BOS_LST_BIT_STEP_RETURN:
        if (work->timer == 30) {
            BosLstReturnBits(work);
        }

        work->timer += 1;

        if (work->timer > 74) {
            work->bitStep = BOS_LST_BIT_STEP_END;
            work->timer = 0;
        }

        break;
    case BOS_LST_BIT_STEP_END:
        BosLstDestroyTasks(work);
        BosLstSetMode(work, BOS_LST_MOVE_MODE_HOVER, BOS_LST_ATTACK_KIND_HANABIRA);
        work->bitRound = 0;
        running = 0;
        break;
    }

    BosLstUpdateBob(work);
    return running;
}

enum BosLstHanabiraStep {
    BOS_LST_HANABIRA_STEP_START,
    BOS_LST_HANABIRA_STEP_CHARGE,
    BOS_LST_HANABIRA_STEP_RELEASE,
    BOS_LST_HANABIRA_STEP_WAIT
};

u8 BosLstAttackHanabira(BosLstWork* work) {
    s16 step;
    u8 running;

    running = 1;
    step = work->step;

    switch (step) {
    case BOS_LST_HANABIRA_STEP_START:
        BosLstSetAnim(work, 4, 0, TRUE);
        m4aSongNumStart(SONG_VO_MARL_ATTACK03);
        work->step += 1;
        work->timer = 0;
        break;
    case BOS_LST_HANABIRA_STEP_CHARGE:
        work->timer += 1;

        if (work->timer > 120) {
            work->step += 1;
            work->timer = 0;
        }

        break;
    case BOS_LST_HANABIRA_STEP_RELEASE:
        if (AnimIsFinished(&work->anim) == TRUE) {
            BgFxStartHanabira(work->x, work->y, work->z - 0x2000, 270);
            m4aSongNumStart(SONG_EF_MARL_HANABIRA);
            work->step += 1;
            work->timer = 0;
        }

        break;
    case BOS_LST_HANABIRA_STEP_WAIT:
        if (!BgFxIsActive()) {
            BosLstSetAnim(work, 0, 1, FALSE);
            running = 0;
        }

        if (running == 0) {
            if (work->subsDefeated == TRUE) {
                BosLstSetMode(work, BOS_LST_MOVE_MODE_GROUND, BOS_LST_ATTACK_KIND_GROUND);
            } else {
                BosLstSetMode(work, BOS_LST_MOVE_MODE_KAMA, BOS_LST_ATTACK_KIND_KAMA);
            }
        }

        break;
    }

    if (work->z > -0x5400) {
        work->z = work->z - 0x400;
    }

    BosLstUpdateBob(work);
    return running;
}

u8 BosLstUpdateAttack(BosLstWork* work) {
    void* obj;
    s16 hpRatio;
    u8 running;

    obj = &work->body;
    hpRatio = work->hpRatio;

    if (hpRatio <= 63) {
        hpRatio = 64;
    }

    switch (work->attackKind) {
    case BOS_LST_ATTACK_KIND_GROUND:
        running = BosLstAttackGround(work);

        if (running != 0) {
            return 1;
        }

        ClearBtlObjActionFlags(obj);
        work->state = running;
        work->step = running;
        work->cardDelay = ((hpRatio * 15) >> 4) << 8;
        return 1;
    case BOS_LST_ATTACK_KIND_KAMA:
        running = BosLstAttackKama(work);

        if (running != 0) {
            return 1;
        }

        ClearBtlObjActionFlags(obj);
        work->state = running;
        work->step = running;
        work->cardDelay = ((hpRatio * 15) >> 4) << 8;
        return 1;
    case BOS_LST_ATTACK_KIND_DASH:
        running = BosLstAttackDash(work);

        if (running != 0) {
            return 1;
        }

        ClearBtlObjActionFlags(obj);
        work->state = running;
        work->step = running;
        work->cardDelay = ((hpRatio * 15) >> 4) << 8;
        return 1;
    case BOS_LST_ATTACK_KIND_CTR:
        running = BosLstAttackCtr(work);

        if (running != 0) {
            return 1;
        }

        ClearBtlObjActionFlags(obj);
        work->state = running;
        work->step = running;
        work->cardDelay = ((hpRatio * 15) >> 4) << 8;
        return 1;
    case BOS_LST_ATTACK_KIND_BITS:
    case BOS_LST_ATTACK_KIND_PLATFORM_BITS:
        running = BosLstAttackBits(work);

        if (running != 0) {
            return 1;
        }

        ClearBtlObjActionFlags(obj);
        work->state = running;
        work->step = running;
        work->cardDelay = 0x5A00;
        return 1;
    case BOS_LST_ATTACK_KIND_HANABIRA:
        running = BosLstAttackHanabira(work);

        if (running != 0) {
            return 1;
        }

        ClearBtlObjActionFlags(obj);
        work->state = running;
        work->step = running;
        work->cardDelay = ((hpRatio * 15) >> 4) << 8;
        return 1;
    default:
        ClearBtlObjActionFlags(obj);
        work->state = BOS_LST_STATE_MOVE;
        work->step = 0;
        work->cardDelay = ((hpRatio * 15) >> 4) << 8;
        return 1;
    }

    return 1;
}

u8 BosLstUpdateHurt(BosLstWork* work) {
    void* obj;

    obj = &work->body;

    if (AnimIsFinished(&work->anim) == TRUE) {
        ClearBtlObjActionFlags(obj);
        BosLstSetAnim(work, 0, 1, TRUE);
        work->state = BOS_LST_STATE_MOVE;
    }

    return 1;
}

u8 BosLstUpdateState5(BosLstWork* work) {
    ClearBtlObjActionFlags(&work->body);
    work->state = BOS_LST_STATE_MOVE;
    work->step = 0;
    return 1;
}

u8 BosLstUpdateBreak(BosLstWork* work) {
    LstSpawn3 arg;
    BtlObj* obj;

    obj = &work->body;
    ClearBtlObjActionFlags(obj);
    work->breakCount += 1;
    work->unk_078 = 0;

    if (work->animId != 2) {
        BosLstSetAnim(work, 0, 1, FALSE);
    }

    BosLstSetSubAnim(work, 0);
    work->cardDelay = ((((work->hpRatio * 240) >> 9) + 120) << 8);

    if (work->subsDefeated == TRUE) {
        work->hittableTimer = 120;
    }

    if (work->moveMode < BOS_LST_MOVE_MODE_BITS || work->moveMode > BOS_LST_MOVE_MODE_PLATFORM) {
        BosLstDestroyTasks(work);
    }

    switch (work->moveMode) {
    case BOS_LST_MOVE_MODE_HOVER:
        switch (work->attackKind) {
        case BOS_LST_ATTACK_KIND_CTR:
            if (work->breakCount > 2) {
                BosLstSetMode(work, BOS_LST_MOVE_MODE_BITS, BOS_LST_ATTACK_KIND_BITS);
            }

            break;
        case BOS_LST_ATTACK_KIND_HANABIRA:
            if (!work->subsDefeated) {
                BosLstSetMode(work, BOS_LST_MOVE_MODE_KAMA, BOS_LST_ATTACK_KIND_KAMA);
            } else {
                BosLstSetMode(work, BOS_LST_MOVE_MODE_GROUND, BOS_LST_ATTACK_KIND_GROUND);
            }

            break;
        }

        break;
    case BOS_LST_MOVE_MODE_GROUND:
        if (work->groundCount + work->breakCount > 3) {
            work->groundCount = 0;
            BosLstSetMode(work, BOS_LST_MOVE_MODE_DASH, BOS_LST_ATTACK_KIND_DASH);
            work->y = 0x1F000;
            work->z = -0x5400;
        }

        break;
    case BOS_LST_MOVE_MODE_KAMA:
        work->y = 0x1F000;
        work->z = -0x5400;

        if (work->kamaCount + work->breakCount > 3) {
            work->kamaCount = 0;
            BosLstSetMode(work, BOS_LST_MOVE_MODE_DASH, BOS_LST_ATTACK_KIND_DASH);
        }

        break;
    case BOS_LST_MOVE_MODE_DASH:
        work->dashCount = 0;
        BosLstSetMode(work, BOS_LST_MOVE_MODE_PLATFORM, BOS_LST_ATTACK_KIND_PLATFORM_BITS);
        work->platformTimer = 0;

        if (gBtlWork->actor->x < 0xF800) {
            work->x = 0x15000;
            BosLstSetFacing(work, -1);
        } else {
            work->x = 0xA000;
            BosLstSetFacing(work, 1);
        }

        work->y = 0x1F000;
        work->z = -0x5400;
        break;
    case BOS_LST_MOVE_MODE_BITS:
        BosLstInterruptBits(work);
        break;
    case BOS_LST_MOVE_MODE_PLATFORM:
        work->hittableTimer = 0;
        BosLstInterruptBits(work);
        break;
    }

    work->state = BOS_LST_STATE_MOVE;
    work->step = 0;
    arg.x = work->x + work->offsetX;
    arg.y = work->y + work->offsetY;
    arg.z = (work->z + work->offsetZ) - ((obj->height >> 1) << 8);
    arg.kind = BTL_POP_CARD_BREAK;
    TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlPop, &arg);
    return 1;
}

enum BosLstDefeatStep {
    BOS_LST_DEFEAT_STEP_BEGIN,
    BOS_LST_DEFEAT_STEP_SHAKE_LIGHT,
    BOS_LST_DEFEAT_STEP_SHAKE_HEAVY,
    BOS_LST_DEFEAT_STEP_DARKEN,
    BOS_LST_DEFEAT_STEP_FLASHES,
    BOS_LST_DEFEAT_STEP_WAIT,
    BOS_LST_DEFEAT_STEP_END
};

u8 BosLstUpdateDefeat(BosLstWork* work) {
    u8 alive;
    s32 i;
    s32 offsetX;
    s32 offsetZ;
    s32 range;

    alive = 1;
    SetBtlObjUnhittable(&work->body, TRUE);
    work->platformSpeed = 0;
    BosLstDestroyTasks(work);
    work->x = BosLstApproachValue(work->x, 0xF800, 0x80, 0x100, 0x100);
    work->y = BosLstApproachValue(work->y, 0x1F000, 0x80, 0x100, 0x100);

    if ((work->defeatTimer & 7) == 0) {
        BosLstSpawnFal(work, BOS_LST_FAL_DEFEAT_SPIRAL);
    }

    if ((work->defeatTimer & 7) == 4) {
        BosLstSpawnFal(work, BOS_LST_FAL_DEFEAT_BURST);
    }

    work->defeatTimer += 1;

    if (work->z >= -0x5500 || work->step > BOS_LST_DEFEAT_STEP_FLASHES) {
        BosLstFldSetCameraMode(work->task, BOS_LST_FLD_CAMERA_MODE_FOLLOW_PLAYER_SLOW);
    } else {
        BosLstFldSetCameraMode(work->task, BOS_LST_FLD_CAMERA_MODE_FOLLOW_BOSS);
    }

    switch (work->step) {
    case BOS_LST_DEFEAT_STEP_BEGIN:
        work->timer = 0;
        BeginBossDefeat(&work->body);
        m4aSongNumStart(SONG_SND_713);
        m4aSongNumStart(SONG_EV_FLASH00);
        BosLstSetAnim(work, 3, 0, FALSE);
        work->step += 1;
    case BOS_LST_DEFEAT_STEP_SHAKE_LIGHT:
        work->z = BosLstApproachValue(work->z, -0x5400, 0x80, 0x100, 0x140);

        if (work->timer & 0x20) {
            range = 0x200;
            offsetX = (GetRandom() % 5 << 8) - range;
            work->offsetX = offsetX;
            offsetZ = (GetRandom() % 5 << 8) - range;
            work->offsetZ = offsetZ;
        } else {
            work->offsetX = 0;
            work->offsetZ = 0;
        }

        work->timer += 1;

        if (work->timer <= 63) {
            break;
        }

        for (i = 0; i < 8; i++) {
            ColliderSetDisabled(&work->colliders[i], TRUE);
        }

        work->playerOnPlatform = FALSE;
        work->step += 1;
        work->timer = 0;
    case BOS_LST_DEFEAT_STEP_SHAKE_HEAVY:
        work->z = BosLstApproachValue(work->z, -0x5400, 0x80, 0x100, 0x140);

        if (work->timer & 0x10) {
            range = 0x400;
            offsetX = (GetRandom() % 9 << 8) - range;
            work->offsetX = offsetX;
            offsetZ = (GetRandom() % 9 << 8) - range;
            work->offsetZ = offsetZ;
        } else {
            work->offsetX = 0;
            work->offsetZ = 0;
        }

        work->timer += 1;

        if (work->timer > 180) {
            work->step += 1;
            work->timer = 0;
        }

        break;
    case BOS_LST_DEFEAT_STEP_DARKEN:
        BgFxStartHumDefeat(work->x, -0x800 + work->y + work->z);
        m4aSongNumStart(SONG_SND_718);
        FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        work->step += 1;
        work->timer = 0;
    case BOS_LST_DEFEAT_STEP_FLASHES:
        work->timer += 1;

        if (work->timer <= 39) {
            range = 0x600;
            offsetX = (GetRandom() % 13 << 8) - range;
            work->offsetX = offsetX;
            offsetZ = (GetRandom() % 13 << 8) - range;
            work->offsetZ = offsetZ;
        } else {
            work->offsetX = 0;
            work->offsetZ = 0;
        }

        if (work->timer <= 199) {
            switch (work->timer) {
            case 40:
                for (i = 0; i < 8; i++) {
                    BosLstSpawnFal(work, BOS_LST_FAL_DEFEAT_BURST);
                }

                FadeStartOut(FADE_MODE_ADD_WHITE, 4);
                m4aSongNumStart(SONG_SND_720);
                break;
            case 120:
            case 170:
            case 190:
                for (i = 0; i < 8; i++) {
                    BosLstSpawnFal(work, BOS_LST_FAL_DEFEAT_BURST);
                }

                FadeStartOut(FADE_MODE_ADD_WHITE, 2);
                m4aSongNumStart(SONG_SND_720);
                break;
            case 44:
                FadeStartIn(FADE_MODE_ADD_WHITE, 4);
                break;
            case 122:
            case 172:
            case 192:
                FadeStartIn(FADE_MODE_ADD_WHITE, 2);
                break;
            }
        } else {
            for (i = 0; i < 80; i++) {
                BosLstSpawnFal(work, BOS_LST_FAL_DEFEAT_BURST);
            }

            FadeStartIn(FADE_MODE_ADD_WHITE, 60);
            FadeLock();
            m4aSongNumStart(SONG_SND_719);
            gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
            work->hidden = TRUE;
            work->step += 1;
            work->timer = 0;
        }

        break;
    case BOS_LST_DEFEAT_STEP_WAIT:
        work->timer += 1;

        if (work->timer > 90) {
            work->step += 1;
            work->timer = 0;
        }

        break;
    case BOS_LST_DEFEAT_STEP_END:
    default:
        work->timer += 1;

        if (work->timer > 210) {
            EndBossDefeat();
            alive = 0;
        }

        break;
    }

    return alive;
}

u8 BosLstUpdateEvent(BosLstWork* work) {
    switch (work->eventStep) {
    case BOS_LST_EVENT_STEP_WAIT:
        work->x = BosLstApproachValue(work->x, 0x1D000, 0, 0x80, 0x200);
        work->y = BosLstApproachValue(work->y, 0x1F000, 0, 0x80, 0x200);
        work->z = BosLstApproachValue(work->z, -0x14400, 0, 0x80, 0x400);
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_BLANK, work->facing);
        break;
    case BOS_LST_EVENT_STEP_APPROACH:
        work->x = BosLstApproachValue(work->x, 0x15500, 0, 0x80, 0x200);
        work->y = BosLstApproachValue(work->y, 0x1F000, 0, 0x80, 0x200);
        work->z = BosLstApproachValue(work->z, -0x8400, 0, 0x80, 0x400);
        BosLstFldSetBgMode(work->task, BOS_LST_FLD_BG_MODE_HORIZON, work->facing);
        break;
    }

    BosLstUpdateBob(work);
    return 1;
}

void BosLstUpdateSub(BosLstWork* work, LstSub* sub) {
    LstSnpArg arg;
    BtlObj* obj;
    u8 flags;

    obj = &sub->body;

    if (sub->restartAnim == TRUE) {
        sub->anim.animId = 0xFFFF;
        sub->curAnimId = -1;
        sub->restartAnim = FALSE;
    }

    if (sub->animId == sub->curAnimId) {
        switch (sub->animId) {
        case 3:
        case 4:
            break;
        default:
            if (AnimIsFinished(&sub->anim) == TRUE) {
                if (sub->defeated == TRUE) {
                    sub->animId = 2;
                } else {
                    sub->animId = 0;
                }
            }

            break;
        }
    }

    flags = 1;

    switch (sub->animId) {
    case 1:
    case 3:
    case 4:
    case 6:
        flags = 0;
        break;
    }

    if (work->facing > 0) {
        if (sub->unk_001 == 1) {
            AnimChangeWithTables(&sub->anim, sub->animId * 2, flags, gBosLstScythe0Anims, gBosLstScythe0Frames);
        } else {
            AnimChangeWithTables(&sub->anim, sub->animId * 2, flags, gBosLstScythe1Anims, gBosLstScythe1Frames);
        }
    } else {
        if (sub->unk_001 == 1) {
            AnimChangeWithTables(&sub->anim, sub->animId * 2 + 1, flags, gBosLstScythe1Anims, gBosLstScythe1Frames);
        } else {
            AnimChangeWithTables(&sub->anim, sub->animId * 2 + 1, flags, gBosLstScythe0Anims, gBosLstScythe0Frames);
        }
    }

    sub->curAnimId = sub->animId;
    AnimUpdate(&sub->anim);

    switch (sub->animId) {
    default:
        SetBtlObjUnhittable(obj, TRUE);
        break;
    case 0:
        SetBtlObjUnhittable(obj, FALSE);
        break;
    case 2:
        SetBtlObjUnhittable(obj, TRUE);
        return;
    }

    switch (UpdateBtlObjReaction(obj)) {
    case BTL_REACTION_CARD_ACTION:
        sub->state = BOS_LST_SUB_STATE_CARD_ACTION;
        sub->timer = 0;
        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        work->cardDelay = work->cardDelay * 3;
        work->cardDelay = work->cardDelay / 4;
        sub->hurtTimer = 20;
        work->hurtTimer = 20;

        if (sub->state == BOS_LST_SUB_STATE_RECOVER) {
            ClearBtlObjActionFlags(obj);
        } else {
            sub->state = BOS_LST_SUB_STATE_HURT;
            sub->timer = 0;
        }

        break;
    case BTL_REACTION_DEFEATED:
        sub->state = BOS_LST_SUB_STATE_DEFEATED;
        sub->timer = 0;
        sub->restartAnim = TRUE;
        sub->animId = 6;
        arg.x = sub->body.x;
        arg.y = sub->body.y;
        arg.z = sub->body.z;
        arg.facing = work->facing;
        TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosLstSnp, &arg);
        break;
    case BTL_REACTION_CARD_BROKEN:
        sub->state = BOS_LST_SUB_STATE_CARD_BROKEN;
        sub->timer = 0;
        break;
    }

    switch (sub->state) {
    case BOS_LST_SUB_STATE_IDLE:
        break;
    case BOS_LST_SUB_STATE_HURT:
        sub->timer += 1;

        if (sub->timer > 20) {
            ClearBtlObjActionFlags(obj);
            sub->state = BOS_LST_SUB_STATE_IDLE;
            sub->timer = 0;
        }

        break;
    case BOS_LST_SUB_STATE_CARD_ACTION:
    case BOS_LST_SUB_STATE_CARD_BROKEN:
    case BOS_LST_SUB_STATE_RECOVER:
        ClearBtlObjActionFlags(obj);
        sub->state = BOS_LST_SUB_STATE_IDLE;
        sub->timer = 0;
        break;
    case BOS_LST_SUB_STATE_DEFEATED:
        sub->defeated = TRUE;
        sub->hurtTimer = 0;
        break;
    }
}

u8 task_bos_lst_1(BosLstWork* work) {
    s16 sx;
    s16 sy;
    u8 alive;
    BtlObj* obj;
    BtlObj* actor;
    BtlObj* body;
    LstSub* sub;
    BtlWork** btl;
    s32 x;
    BtlObj* rider;
    s32 speed;
    s32 y;
    s32 lim;
    s16 anim;
    s16 idx;
    s32 i;
    s32 front;
    s32 back;

    obj = &work->body;
    alive = 1;
    work->offsetX = work->offsetX / 512;
#ifdef PLATFORM_PC
    /* @bug: a frame before the maximum HP is set divides by zero (the gauges' #40 class) */
    work->hpRatio = work->body.maxHp > 0 ? (work->body.hp * 255) / work->body.maxHp : 0;
#else
    work->hpRatio = (work->body.hp * 255) / work->body.maxHp;
#endif
    btl = &gBtlWork;
    actor = (*btl)->actor;
    actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
    (*btl)->bossPriorityOffset = -16;

    if (work->playerOnPlatform == TRUE) {
        y = BosLstGetPlatformY(work);
        rider = (*btl)->actor;
        rider->y = y;

        if (((*btl)->flags & BTL_FLAG_PLAYER_OFFSCREEN) == 0) {
            speed = (work->platformSpeed * 70) >> 8;
            x = rider->x + speed * work->facing;
            rider->x = x;

            if (work->facing < 0) {
                lim = work->x - 0x2000;

                if (x > lim) {
                    rider->x = lim;
                }
            } else {
                lim = work->x + 0x2000;

                if (x < lim) {
                    rider->x = lim;
                }
            }
        }
    }

    anim = sBosLstBodyFrames[work->bodyCycle] << 1;
    idx = 0;

    if (work->facing < 0) {
        anim |= 1;
        idx = 1;
    }

    idx = (s16)idx;
    work->hittableTimer -= 1;

    if (work->hittableTimer < 0) {
        work->hittableTimer = 0;
    }

    SetBtlObjUnhittable(&work->body, TRUE);

    if (work->sub[0].defeated == TRUE && work->sub[1].defeated == TRUE) {
        work->subsDefeated = TRUE;

        if (work->moveMode == BOS_LST_MOVE_MODE_PLATFORM) {
            if (work->turned == 1) {
                SetBtlObjUnhittable(&work->body, FALSE);
            }
        } else if (work->hittableTimer > 0) {
            SetBtlObjUnhittable(&work->body, FALSE);
        }
    } else {
        work->subsDefeated = FALSE;
    }

    switch (UpdateBtlObjReaction(obj)) {
    case BTL_REACTION_CARD_ACTION:
        work->state = BOS_LST_STATE_ATTACK;
        work->step = 0;
        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        work->cardDelay = work->cardDelay * 3;
        work->cardDelay = work->cardDelay / 4;
        work->hurtTimer = 20;
        BosLstSpawnFal(work, BOS_LST_FAL_DRIFT);
        BosLstSpawnFal(work, BOS_LST_FAL_DRIFT);
        BosLstSpawnFal(work, BOS_LST_FAL_DRIFT);
        BosLstSpawnFal(work, BOS_LST_FAL_DRIFT);
        work->platformSpeed += 0x80;
        BosLstSetAnim(work, 1, 0, FALSE);

        if (work->state == BOS_LST_STATE_RECOVER) {
            ClearBtlObjActionFlags(obj);
        } else {
            work->state = BOS_LST_STATE_HURT;
        }

        break;
    case BTL_REACTION_DEFEATED:
        work->state = BOS_LST_STATE_DEFEATED;
        work->step = BOS_LST_DEFEAT_STEP_BEGIN;
        work->defeatTimer = 0;
        break;
    case BTL_REACTION_CARD_BROKEN:
        work->state = BOS_LST_STATE_BREAK;
        work->step = 0;
        break;
    }

    switch (work->state) {
    case BOS_LST_STATE_MOVE:
        BosLstUpdateMove(work);
        break;
    case BOS_LST_STATE_ATTACK:
        BosLstUpdateAttack(work);
        break;
    case BOS_LST_STATE_HURT:
        BosLstUpdateHurt(work);
        break;
    case BOS_LST_STATE_RECOVER:
        BosLstUpdateState5(work);
        break;
    case BOS_LST_STATE_BREAK:
        BosLstUpdateBreak(work);
        break;
    case BOS_LST_STATE_DEFEATED:
        alive = BosLstUpdateDefeat(work);
        break;
    case BOS_LST_STATE_EVENT:
        BosLstUpdateEvent(work);
        break;
    case BOS_LST_STATE_INACTIVE:
    default:
        break;
    }

    BosLstUpdateSub(work, &work->sub[0]);
    BosLstUpdateSub(work, &work->sub[1]);
    obj->x = work->x + work->offsetX;
    obj->y = work->y + work->offsetY;
    obj->z = work->z + work->offsetZ;
    front = idx;

    if (!work->sub[front].defeated) {
        body = &work->sub[front].body;
        body->x = work->x + work->offsetX + (sLstAnimDefs[anim].subX << 8);
        body->y = work->y + work->offsetY + (sLstAnimDefs[anim].subY << 8);
        body->z = work->z + work->offsetZ + (sLstAnimDefs[anim].subZ << 8);
    }

    back = idx ^ 1;

    if (!work->sub[back].defeated) {
        sub = &work->sub[back];
        body = &sub->body;
        body->x = work->x + work->offsetX + (sLstAnimDefs[anim].sub2X << 8);
        body->y = work->y + work->offsetY + (sLstAnimDefs[anim].sub2Y << 8);
        body->z = work->z + work->offsetZ + (sLstAnimDefs[anim].sub2Z << 8);
    }

    ColliderSetPosition(&obj->collider, obj->x + (work->facing << 10), obj->y, obj->z);
    ColliderSetPosition(&work->collider, obj->x, obj->y - 0x1000, obj->z + 0x1800);
    ColliderSetPosition(&work->collider2, obj->x, obj->y - 0x1000, obj->z + 0x4000);

    for (i = 0; i < 8; i++) {
        ColliderSetPosition(&work->colliders[i], work->x + ((i << 12) + 0x1800) * work->facing, work->y, work->z);
    }

    switch (work->animId) {
    case 1:
        if (AnimIsFinished(&work->anim) == TRUE) {
            BosLstSetAnim(work, 0, 1, FALSE);
        }

        break;
    case 2:
        if (AnimIsFinished(&work->anim) == TRUE) {
            work->turned = 1;
            BosLstSetAnim(work, 0, 1, FALSE);
        }

        break;
    }

    AnimUpdate(&work->anim);
    work->bodyCycle += 1;
    work->bodyCycle = (u32)work->bodyCycle % 48;
    gBtlWork->bossX = work->x;
    gBtlWork->bossY = work->y;
    gBtlWork->bossZ = work->z;

    if (work->state != BOS_LST_STATE_DEFEATED && (work->frameCount & 0xF) == 0) {
        WorldToScreen(&sx, &sy, work->x + work->offsetX, work->y + work->offsetY, work->z + work->offsetZ);

        if (sy < -16) {
            BosLstSpawnFal(work, BOS_LST_FAL_HIGH);
        } else if (work->playerOnPlatform == TRUE) {
            BosLstSpawnFal(work, BOS_LST_FAL_PLATFORM);
        } else {
            BosLstSpawnFal(work, BOS_LST_FAL_DRIFT);
        }
    }

    work->frameCount += 1;
    TaskPoolUpdate(&work->tasks);
    return alive;
}

void task_bos_lst_2(BosLstWork* work) {
    s16 sx;
    s16 sy;
    s16 idx;
    u32* src;
    LstSub* sub;
    s16 anim;
    s16 bgFrame;
    s32 front;
    s32 back;
    u16 frontTimer;
    u16 backTimer;

    TaskPoolDraw(&work->tasks);
    anim = sBosLstBodyFrames[work->bodyCycle] << 1;
    idx = 0;

    if (work->facing < 0) {
        anim |= 1;
        idx = 1;
    }

    idx = (s16)idx;

    if (StepHitFlash(&work->body) || StepHitFlash(&work->sub[0].body) ||
        StepHitFlash(&work->sub[1].body)) {
        work->flash = 1;
    } else {
        work->flash = 0;
    }

    if ((s16)work->flash != (s16)work->prevFlash) {
        if ((s16)work->flash == 0) {
            LoadPalette(gBosLstBgPalettes, (void*)PLTT, 3 * sizeof(gBosLstBgPalettes[0]));
            LoadPalette(gBosLstObjPalette, (void*)(OBJ_PLTT + ((work->palette->index & 15) << 5)), sizeof(gBosLstObjPalette));
        } else {
            LoadPalette(gHitFlashPalette, (void*)PLTT, sizeof(gHitFlashPalette));
            LoadPalette(gHitFlashPalette, (void*)(BG_PLTT + PLTT_SIZE_4BPP), sizeof(gHitFlashPalette));
            LoadPalette(gHitFlashPalette, (void*)(BG_PLTT + 2 * PLTT_SIZE_4BPP), sizeof(gHitFlashPalette));
            LoadPalette(gHitFlashPalette, (void*)(OBJ_PLTT + ((work->palette->index & 15) << 5)), sizeof(gHitFlashPalette));
            LoadPalette(gHitFlashPalette, (void*)(OBJ_PLTT + PLTT_SIZE_4BPP + ((work->palette->index & 15) << 5)), sizeof(gHitFlashPalette));
            LoadPalette(gHitFlashPalette, (void*)(OBJ_PLTT + 2 * PLTT_SIZE_4BPP + ((work->palette->index & 15) << 5)), sizeof(gHitFlashPalette));
        }

        work->prevFlash = work->flash;
    }

    if (work->hidden == TRUE || (work->unk_078 & 1)) {
        DisableBg(1);
        return;
    }

    WorldToScreen(&sx, &sy, work->x + work->offsetX - (sLstAnimDefs[anim].bgX << 8),
                  work->y + work->offsetY - (sLstAnimDefs[anim].bgY << 8),
                  work->z + work->offsetZ - (sLstAnimDefs[anim].bgZ << 8));
    SetBgScroll(1, (u16)(-sx), (u16)(-sy));

    if ((u16)(sy + 255) > 0x19E || (u16)(sx + 255) > 0x1FE) {
        DisableBg(1);
    } else {
        if (work->turned == 1) {
            bgFrame = 8;
        } else {
            switch (AnimGetGfxIndex(&work->anim)) {
            case 58:
                bgFrame = 1;
                break;
            case 59:
                bgFrame = 2;
                break;
            case 60:
                bgFrame = 3;
                break;
            case 61:
                bgFrame = 4;
                break;
            case 62:
                bgFrame = 5;
                break;
            case 63:
                bgFrame = 6;
                break;
            case 64:
                bgFrame = 7;
                break;
            case 28:
            case 29:
                bgFrame = 8;
                break;
            default:
                bgFrame = 0;
                break;
            }
        }

        if ((s16)work->bgFrame != bgFrame) {
            LoadBgTiles(1, sBosLstBgFrames[bgFrame][0], 0xC00);
            work->bgFrame = bgFrame;
        }

        if (work->facing < 0) {
            bgFrame += 9;
        }

        DmaCopy16(3, sLstAnimDefs[anim].bgMap, work->bgMap, sizeof(work->bgMap));
        src = sBosLstBgFrames[bgFrame][1];
        DmaCopy16(3, src, work->bgMap, 0x280);

        if (work->facing > 0) {
            DmaCopy16(3, src + 160, &work->bgMap[10 * 32], 0x1C);
        } else {
            DmaCopy16(3, src + 169, &work->bgMap[10 * 32 + 18], 0x1C);
        }

#ifdef PLATFORM_PC
        PortMarluxiaBodyMap(work->bgMap); /* whole, before the rows off the screen are cleared */
#endif
        if (sy < 0) {
            CpuFastFill(0, work->bgMap, ((-sy) >> 3) * 64);
        } else if (sy <= DISPLAY_HEIGHT - 1) {
            CpuFastFill(0, &work->bgMap[(20 - (sy >> 3)) * 32], ((sy >> 3) + 12) * 64);
        }

        LoadBgMap(1, work->bgMap, sizeof(work->bgMap));
    }

    WorldToScreen(&sx, &sy, work->x + work->offsetX, work->y + work->offsetY, work->z + work->offsetZ);
    DrawSprite(sx + sLstAnimDefs[anim].spriteX, sy + sLstAnimDefs[anim].spriteY, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL,
               GetBattleSpritePriorityFlags(work->y + work->offsetY + (gBtlWork->bossPriorityOffset << 8)),
               -0x1004 - (((work->y + work->offsetY + (gBtlWork->bossPriorityOffset << 8)) >> 8) << 2));
    WorldToScreen(&sx, &sy, work->x + work->offsetX, work->y + work->offsetY, work->z + work->offsetZ);
    front = idx;
    frontTimer = work->sub[front].hurtTimer;

    if (work->sub[front].hurtTimer > 0) {
        work->sub[front].hurtTimer = frontTimer - 1;
    }

    sub = &work->sub[front];
    DrawSprite(sx + sLstAnimDefs[anim].subSpriteX, sy + sLstAnimDefs[anim].subSpriteY, AnimGetGfx(&sub->anim), work->sub[0].tiles, work->palette, NULL,
               GetBattleSpritePriorityFlags(work->y + work->offsetY),
               -0x1004 - (((work->y + work->offsetY) >> 8) << 2));
    back = idx ^ 1;
    backTimer = work->sub[back].hurtTimer;

    if (work->sub[back].hurtTimer > 0) {
        work->sub[back].hurtTimer = backTimer - 1;
    }

    sub = &work->sub[back];
    DrawSprite(sx + sLstAnimDefs[anim].sub2SpriteX, sy + sLstAnimDefs[anim].sub2SpriteY, AnimGetGfx(&sub->anim), work->sub[1].tiles, work->palette, NULL,
               GetBattleSpritePriorityFlags(work->y + work->offsetY - 0x1100),
               -0x1004 - (((work->y + work->offsetY - 0x1100) >> 8) << 2));
}

void task_bos_lst_3(BosLstWork* work) {
    s32 i;

    ReleaseEnemyBtlObj(&work->body);
    ReleaseEnemyBtlObj(&work->sub[0].body);
    ReleaseEnemyBtlObj(&work->sub[1].body);
    ColliderUnregister(&work->collider);
    ColliderUnregister(&work->collider2);

    for (i = 0; i < 8; i++) {
        ColliderUnregister(&work->colliders[i]);
    }

    BosLstDestroyTasks(work);
    ReleaseObjTiles(work->sub[0].tiles);
    ReleaseObjTiles(work->sub[1].tiles);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}
