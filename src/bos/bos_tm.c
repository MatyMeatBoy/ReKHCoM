/**
 * bos_tm.c
 * Trickmaster Boss
 */

#include "macros.h"
#include "registration_data.h"
#include "boss_tm.h"
#include "event_backgrounds.h"
#include "chara_types.h"
#include "chara_api.h"
#include <stddef.h>
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_bg_types.h"
#include "battle_work.h"
#include "gba/defines.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"

s16 gBosTmActorZ EWRAM_COMMON(4);
s16 gBosTmActorOriginZ EWRAM_COMMON(4);
s16 gBosTmActorY EWRAM_COMMON(4);
s16 gBosTmBossY EWRAM_COMMON(4);

static const BattleBackgroundDef sBosTmBattleBackgroundDef = {
    gBosTmBgTiles, sizeof(gBosTmBgTiles), gBosTmBgPalette, sizeof(gBosTmBgPalette), { gBosTmBgMap, gBosTmBgMap, gBosTmBgMap, gBosTmBgMap }
};

TaskDesc gTaskDescBosTm = {
    "task_bos_tm",
    (TaskInitFunc)task_bos_tm_0,
    (TaskUpdateFunc)task_bos_tm_1,
    (TaskDrawFunc)task_bos_tm_2,
    (TaskDestroyFunc)task_bos_tm_3,
    sizeof(TmWork),
};

static Task* sBosTmBodyTask;
static Task* sBosTmArmTask;
static Task* sBosTmFootTask;
static Task* sBosTmTblTask;
static TaskPool sBosTmTaskPool;

void BosTmSetArmPositions(TmWork* work) {
    if (work->flags & TM_FLAG_FACING_LEFT) {
        work->arm.x = work->x2 + 0x1000;
        work->arm.x2 = work->x2 - 0x700;
        work->arm.y = work->y2 + 0x700;
        work->arm.y2 = work->y2 - 0x400;
        work->arm.z = work->z2 - 0x2200;
        work->arm.z2 = work->z2 - 0x1C00;
    } else {
        work->arm.x = work->x2 + 0x700;
        work->arm.x2 = work->x2 - 0xE00;
        work->arm.y = work->y2 - 0x400;
        work->arm.y2 = work->y2 + 0x700;
        work->arm.z = work->z2 - 0x1C00;
        work->arm.z2 = work->z2 - 0x2200;
    }
}

void task_bos_tm_0(TmWork* work, BtlObj* arg) {
    work->flags = 0;

    if (arg != NULL) {
        work->flags = TM_FLAG_IN_EVENT;
    }

    TaskPoolInit(&sBosTmTaskPool, 4);

    if (work->flags & TM_FLAG_IN_EVENT) {
        work->x = arg->x >> 8;
        work->y = arg->y >> 8;
        work->z = arg->z >> 8;
    } else {
        work->x = 0x15D;
        work->y = 0x16C;
        work->z = -0x3C;
        gBtlWork->bossX = work->x << 8;
        gBtlWork->bossY = 0x156 << 8;
        gBtlWork->bossZ = (s16)work->z << 8;
    }

    work->baseX = (s16)work->x << 8;
    work->baseY = (s16)work->y << 8;
    work->baseZ = (s16)work->z << 8;
    work->x2 = work->baseX;
    work->y2 = work->baseY;
    work->z2 = work->baseZ;
    work->vx = 0;
    work->vy = 0;
    work->step = 0;
    work->stepTimer = 0;
    work->hitCount = 0;
    work->hurtTimer = 55;
    work->stateTimer = 0;
    work->tableState = BOS_TM_TABLE_STATE_DOWN;
    work->flags |= (TM_FLAG_TABLE_JUST_RAISED | TM_FLAG_FACING_LEFT);
    work->unk_3B = 0;
    work->resumeState = BOS_TM_STATE_NONE;
    work->tileIndex = 0;
    work->tileCount = 0;
    work->paletteIndex = 0;
    work->arm.tm = work;
    BosTmSetArmPositions(work);

    if (work->flags & TM_FLAG_IN_EVENT) {
        work->state = BOS_TM_STATE_EVENT_IDLE;
        sBosTmBodyTask = TaskCreate(&sBosTmTaskPool, &gTaskDescBosTmBody, work);
        sBosTmFootTask = TaskCreate(&sBosTmTaskPool, &gTaskDescBosTmFoot, work);
        sBosTmArmTask = TaskCreate(&sBosTmTaskPool, &gTaskDescBosTmArm, &work->arm);
    } else {
        work->state = BOS_TM_STATE_IDLE;
        TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosMap, (void*)&sBosTmBattleBackgroundDef);
        sBosTmTblTask = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosTmTbl, work);
        sBosTmBodyTask = TaskCreate(&sBosTmTaskPool, &gTaskDescBosTmBody, work);
        sBosTmFootTask = TaskCreate(&sBosTmTaskPool, &gTaskDescBosTmFoot, work);
        sBosTmArmTask = TaskCreate(&sBosTmTaskPool, &gTaskDescBosTmArm, &work->arm);
        gBtlWork->bossPriorityOffset = 10;
    }
}

u8 task_bos_tm_1(TmWork* work) {
    CharaObjParam2 param;
    u16 hitCount;

    switch (work->state) {
    case BOS_TM_STATE_IDLE:
    case BOS_TM_STATE_EVENT_IDLE:
        work->stepTimer++;

        if (work->stepTimer > 8) {
            work->stepTimer = 0;
            work->step++;

            if (work->step > 7) {
                work->step = 0;
            }
        }

        break;
    case BOS_TM_STATE_RECOIL:
        work->step++;
        hitCount = work->hitCount;

        if ((s16)hitCount == 1) {
            work->hitCount = hitCount + 1;
        }

        break;
    case BOS_TM_STATE_WALK_LEFT:
    case BOS_TM_STATE_WALK_LEFT_SETTLE:
    case BOS_TM_STATE_WALK_RIGHT:
    case BOS_TM_STATE_WALK_RIGHT_SETTLE:
        work->stepTimer++;

        if (work->stepTimer > 6) {
            work->stepTimer = 0;
            work->step++;

            if (work->step > 9) {
                work->step = 0;
            }
        }

        break;
    case BOS_TM_STATE_DEFEATED:
        if (work->step != 0) {
            if (!CharaObjUpdateDefeat2()) {
                EndBossDefeat();
                return 0;
            }
        } else {
            param.tilesAddr = OBJ_VRAM0 + (work->tileIndex << 5);
            param.tileCount = work->tileCount;
            param.paletteAddr = OBJ_PLTT + (work->paletteIndex << 5);
            param.paletteSize = 0x60;
            param.x = work->x2;
            param.y = work->y2;
            param.z = work->z2;
            param.callback = BosTmDestroyParts;
            gBosTmBodyObjCopy.x = work->x2;
            gBosTmBodyObjCopy.y = work->y2;
            gBosTmBodyObjCopy.z = work->z2;
            param.prizeObj = &gBosTmBodyObjCopy;
            CharaObjInitDefeat2(&param);
            work->flags &= ~TM_FLAG_HURT;
            work->step++;
        }

        break;
    case BOS_TM_STATE_SLAM_TABLE:
    case BOS_TM_STATE_SLAM_GROUND:
    case BOS_TM_STATE_SPIN:
    case BOS_TM_STATE_SLAM_GROUND_SLOW:
        work->step++;
        break;
    case BOS_TM_STATE_FROZEN:
        break;
    }

    /* @bug: during the scene before the battle (EVENT_099) there is no actor yet; the cartridge reads the BIOS */
    gBosTmActorY = BIOS_PTR(gBtlWork->actor)->y >> 8;
    gBosTmActorOriginZ = BIOS_PTR(gBtlWork->actor)->originZ >> 8;
    gBosTmActorZ = BIOS_PTR(gBtlWork->actor)->z >> 8;
    gBosTmBossY = gBtlWork->bossY >> 8;

    if ((gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) && work->state != BOS_TM_STATE_DEFEATED) {
        if (BIOS_PTR(gBtlWork->actor)->originZ <= -0x2D00) {
            gBtlWork->bossPriorityOffset = -10;
        } else {
            gBtlWork->bossPriorityOffset = 10;
        }
    } else {
        if (BIOS_PTR(gBtlWork->actor)->z <= -0x2D00) {
            gBtlWork->bossPriorityOffset = -10;
        } else {
            gBtlWork->bossPriorityOffset = 10;
        }
    }

    TaskPoolUpdate(&sBosTmTaskPool);
    BosTmSetArmPositions(work);
    work->stateTimer++;
    return 1;
}

void task_bos_tm_2(TmWork* work) {
    TaskPoolDraw(&sBosTmTaskPool);
}

void task_bos_tm_3(TmWork* work) {
    TaskPoolDestroy(&sBosTmTaskPool);
}

void BosTmDestroyParts() {
    TaskKill(&gBtlWork->taskPools[1], sBosTmTblTask);
    TaskKill(&sBosTmTaskPool, sBosTmBodyTask);
    TaskKill(&sBosTmTaskPool, sBosTmFootTask);
    TaskKill(&sBosTmTaskPool, sBosTmArmTask);
}
