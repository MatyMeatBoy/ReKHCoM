/**
 * bos_ursula.c
 * Ursula Boss
 */

#include "bos4.h"
#include "sprites_bos4.h"
#include "gba/io_reg.h"
#include "prize_types.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include <string.h>
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_bg_types.h"
#include "battle_work.h"
#include "bos4_api.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "display.h"
#include "engine_math.h"
#include "listpool.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include "registration_data.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "bos_ursula.h"
#include "sprite_palettes.h"
#include "default_bg_map.h"
#include "enemy_ids.h"
#include "gba/defines.h"
#include "macros.h"

#define BOS_URSULA_VIEW_Y_MIN 136
#define BOS_URSULA_VIEW_Y_MAX 480
#define BOS_URSULA_VIEW_Y_MAX_GIMMICK 544

static const EmyKind sBosUrsulaEmyKind = { ENEMY_URSULA, 0, 32, 24, 0, 0, 0 };

static const BattleBackgroundDef sBosUrsulaBattleBackgroundDef = {
    gBosUrsulaBgTiles, sizeof(gBosUrsulaBgTiles), gBosUrsulaBgPalettes, sizeof(gBosUrsulaBgPalettes), { gBosUrsulaBgMap0, gBosUrsulaBgMap1, gBosUrsulaBgMap2, gBosUrsulaBgMap3 }
};

static const u16* sBosUrsulaMapBlocksLeft[12] = {
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gBosUrsulaLeftMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
};

static const u16* sBosUrsulaMapBlocksHurtLeft[12] = {
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gBosUrsulaHurtLeftMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
};

static const u16* sBosUrsulaMapBlocksRight[12] = {
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gBosUrsulaRightMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
};

static const u16* sBosUrsulaMapBlocksHurtRight[12] = {
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gBosUrsulaHurtRightMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
};

TaskDesc gTaskDescBosUrsula = {
    "task_bos_ursula",
    (TaskInitFunc)task_bos_ursula_0,
    (TaskUpdateFunc)task_bos_ursula_1,
    (TaskDrawFunc)task_bos_ursula_2,
    (TaskDestroyFunc)task_bos_ursula_3,
    sizeof(UrsulaWork),
};

static TaskDesc sTaskDescBosUrsulaMap = {
    "task_bos_ursula_map",
    (TaskInitFunc)task_bos_ursula_map_0,
    (TaskUpdateFunc)task_bos_ursula_map_1,
    NULL,
    (TaskDestroyFunc)task_bos_ursula_map_3,
    sizeof(UrsulaMapWork),
};

static TaskDesc sTaskDescBosUrsulaBorder = {
    "task_bos_ursula_border",
    (TaskInitFunc)task_bos_ursula_border_0,
    (TaskUpdateFunc)task_bos_ursula_border_1,
    (TaskDrawFunc)task_bos_ursula_border_2,
    (TaskDestroyFunc)task_bos_ursula_border_3,
    sizeof(UrsulaBorderWork),
};

static const EmyKind sBosUrsulaTakoEmyKind = { ENEMY_URSULA, 0, 48, 16, 24, 0, EMY_KIND_FLAG_NO_COLLIDER };

static TaskDesc sTaskDescBosUrsulaTako = {
    "task_bos_ursula_tako",
    (TaskInitFunc)task_bos_ursula_tako_0,
    (TaskUpdateFunc)task_bos_ursula_tako_1,
    (TaskDrawFunc)task_bos_ursula_tako_2,
    (TaskDestroyFunc)task_bos_ursula_tako_3,
    sizeof(UrsulaTakoWork),
};

static TaskDesc sTaskDescBosUrsulaBacktako = {
    "task_bos_ursula_backtako",
    (TaskInitFunc)task_bos_ursula_backtako_0,
    (TaskUpdateFunc)task_bos_ursula_backtako_1,
    (TaskDrawFunc)task_bos_ursula_backtako_2,
    (TaskDestroyFunc)task_bos_ursula_backtako_3,
    sizeof(UrsulaBacktakoWork),
};

static const BosMapanimeFrame sBosUrsulaMapanimeIdleFrames[6] = { { 60, 0 }, { 4, 1 }, { 6, 2 }, { 20, 0 }, { 4, 1 }, { 6, 2 } };

static const BosMapanimeFrame sBosUrsulaMapanimeWindupFrames[12] = { { 5, 0 }, { 5, 1 }, { 5, 2 }, { 5, 3 }, { 5, 2 }, { 5, 1 }, { 5, 2 }, { 5, 3 }, { 5, 2 }, { 5, 1 }, { 5, 2 }, { 5, 3 } };

static const BosMapanimeFrame sBosUrsulaMapanimeBubbleFrames[5] = { { 10, 0 }, { 10, 1 }, { 10, 2 }, { 10, 3 }, { 10, 4 } };

static const BosMapanimeFrame sBosUrsulaMapanimeChargeFrames[5] = { { 10, 0 }, { 10, 1 }, { 120, 2 }, { 10, 3 }, { 10, 4 } };

static const BosMapanimeFrame sBosUrsulaMapanimeRecoverFrames[1] = { { 0, 0 } };

static const BosMapanimeDef sBosUrsulaMapanimeIdle = { sBosUrsulaMapanimeIdleFrames, ARRAY_COUNT(sBosUrsulaMapanimeIdleFrames), gBosUrsulaMapanimeIdleTiles, 0x0C00, 0x0300, 0x0400, 0 };

static const BosMapanimeDef sBosUrsulaMapanimeWindup = { sBosUrsulaMapanimeWindupFrames, ARRAY_COUNT(sBosUrsulaMapanimeWindupFrames), gBosUrsulaMapanimeWindupTiles, 0x0C00, 0x0860, 0x0C00, 0 };

static const BosMapanimeDef sBosUrsulaMapanimeBubble = { sBosUrsulaMapanimeBubbleFrames, ARRAY_COUNT(sBosUrsulaMapanimeBubbleFrames), gBosUrsulaMapanimeBubbleTiles, 0x0C00, 0x0860, 0x0C00, 0 };

static const BosMapanimeDef sBosUrsulaMapanimeCharge = { sBosUrsulaMapanimeChargeFrames, ARRAY_COUNT(sBosUrsulaMapanimeChargeFrames), gBosUrsulaMapanimeChargeTiles, 0x0C00, 0x0860, 0x0C00, 0 };

static const BosMapanimeDef sBosUrsulaMapanimeRecover = { sBosUrsulaMapanimeRecoverFrames, ARRAY_COUNT(sBosUrsulaMapanimeRecoverFrames), gBosUrsulaBgTiles + 0xC00, 0x0C00, 0x0860, 0x0C00, 0 };

static TaskDesc sTaskDescBosUrsulaMapanime = {
    "task_bos_ursula_mapanime",
    (TaskInitFunc)task_bos_ursula_mapanime_0,
    (TaskUpdateFunc)task_bos_ursula_mapanime_1,
    (TaskDrawFunc)task_bos_ursula_mapanime_2,
    (TaskDestroyFunc)task_bos_ursula_mapanime_3,
    sizeof(UrsulaMapanimeWork),
};

static UrsulaWork* sUrsulaWork;

static UrsulaMapanimeWork* sUrsulaMapanimeWork;

enum BosUrsulaState {
    BOS_URSULA_STATE_IDLE,
    BOS_URSULA_STATE_ATTACK,
    BOS_URSULA_STATE_CANCEL,
    BOS_URSULA_STATE_HURT,
    BOS_URSULA_STATE_DEFEATED,
    BOS_URSULA_STATE_MOVE
};

enum BosUrsulaAttack {
    BOS_URSULA_ATTACK_NONE,
    BOS_URSULA_ATTACK_BUBBLE,
    BOS_URSULA_ATTACK_CHARGE,
    BOS_URSULA_ATTACK_THUNDER,
    BOS_URSULA_ATTACK_BUSY
};

void BosUrsulaUpdateMapBlocks(UrsulaWork* work) {
    if (work->state >= BOS_URSULA_STATE_HURT && work->state <= BOS_URSULA_STATE_DEFEATED) {
        if (work->obj.x > gBtlWork->actor->x) {
            if (work->mapBlocks != sBosUrsulaMapBlocksHurtLeft) {
                work->mapBlocks = sBosUrsulaMapBlocksHurtLeft;
                SetBgMapBlocks(0, sBosUrsulaMapBlocksHurtLeft, 4, 3);
            } else {
                BosUrsulaStartAttack(BOS_URSULA_ATTACK_NONE);
            }
        } else {
            if (work->mapBlocks != sBosUrsulaMapBlocksHurtRight) {
                work->mapBlocks = sBosUrsulaMapBlocksHurtRight;
                SetBgMapBlocks(0, sBosUrsulaMapBlocksHurtRight, 4, 3);
            } else {
                BosUrsulaStartAttack(BOS_URSULA_ATTACK_NONE);
            }
        }
    } else if (BosUrsulaIsFacingLeft() != 0) {
        if (work->mapBlocks != sBosUrsulaMapBlocksLeft) {
            work->mapBlocks = sBosUrsulaMapBlocksLeft;
            SetBgMapBlocks(0, sBosUrsulaMapBlocksLeft, 4, 3);
        }
    } else {
        if (work->mapBlocks != sBosUrsulaMapBlocksRight) {
            work->mapBlocks = sBosUrsulaMapBlocksRight;
            SetBgMapBlocks(0, sBosUrsulaMapBlocksRight, 4, 3);
        }
    }
}

u8 BosUrsulaIsGuarded(UrsulaWork* work) {
    if (work->gimmickTimer == 0 && !BosUrsulaTakoIsBusy(work->tako->work) && !BosUrsulaTakoIsBusy(work->tako2->work)) {
        return TRUE;
    }

    return FALSE;
}

void task_bos_ursula_0(UrsulaWork* work) {
    u8 isLeft;

    sUrsulaWork = work;
    gBosUrsulaActive = TRUE;
    TaskCreate(&gBtlWork->taskPools[1], &sTaskDescBosUrsulaMap, (void*)&sBosUrsulaBattleBackgroundDef);
    TaskCreate(&gBtlWork->taskPools[0], &sTaskDescBosUrsulaBorder, NULL);
    work->state = BOS_URSULA_STATE_IDLE;
    work->timer = 0;
    work->mapBlocks = NULL;
    work->takoRecoverPending = FALSE;
    work->bobTimer = 0;
    work->bobTarget = 0;
    work->bobZ = 0;
    work->gimmickTimer = 0;
    SetBattleBounds(0, 0x200, 0x1A8, 0x1E0);
    SetBattleActorPosition(0x10000, 0x1A800, 0);
    gBtlWork->bossPriorityOffset = 0xFF00;
    gBosUrsulaBaseZ = -0x5000;
    InitEnemyBtlObj(&work->obj, &sBosUrsulaEmyKind, 0x10000, 0x19800, -0x5000);
    work->obj.groundZ = 0;
    work->obj.flags |= BTLOBJ_FLAG_FACING_LEFT;
    SetBtlObjUnhittable(&work->obj, TRUE);
    BosUrsulaUpdateMapBlocks(work);
    RedrawBgMapAt(0, (gBtlWork->viewX - (work->obj.x - 0x12000)) >> 8,
        (gBtlWork->viewY - (work->obj.y + work->obj.z - 0x12000)) >> 8);
    SetBtlPaletteFadeExcluded(0, TRUE);
    SetBtlPaletteFadeExcluded(1, TRUE);
    gBtlWork->bossX = work->obj.x;
    gBtlWork->bossY = work->obj.y;
    gBtlWork->bossZ = work->obj.z;
    TaskPoolInit(&work->tasks, 5);
    isLeft = 1;
    work->tako = TaskCreate(&work->tasks, &sTaskDescBosUrsulaTako, &isLeft);
    isLeft = 0;
    work->tako2 = TaskCreate(&work->tasks, &sTaskDescBosUrsulaTako, &isLeft);
    TaskCreate(&work->tasks, &sTaskDescBosUrsulaMapanime, NULL);
    isLeft = 1;
    TaskCreate(&work->tasks, &sTaskDescBosUrsulaBacktako, &isLeft);
    work->gimmickDelay = 0;
}

void BosUrsulaUpdateBob(UrsulaWork* work) {
    BtlObj* obj = &work->obj;

    if ((s16)work->bobTimer == 0) {
        work->bobTimer = 32;

        if (work->bobTarget == 0) {
            work->bobTarget = -0x400;
        } else {
            work->bobTarget = 0;
        }
    }

    ApproachValue(&work->bobZ, work->bobTarget, work->bobTimer);
    obj->z = gBosUrsulaBaseZ + work->bobZ;
    work->bobTimer--;
}

u8 BosUrsulaMoveForward(UrsulaWork* work) {
    BtlObj* obj = &work->obj;

    BosUrsulaUpdateBob(work);

    if (BosUrsulaIsFacingLeft() != 0) {
        obj->x -= 0x100;

        if (obj->x <= -0x9800) {
            obj->x = -0x9800;
            return FALSE;
        }
    } else {
        obj->x += 0x100;

        if (obj->x >= 0x28000) {
            obj->x = 0x28000;
            return FALSE;
        }
    }

    return TRUE;
}

s32 BosUrsulaChooseAttackPhase0(UrsulaWork* work) {
    BtlObj* player = gBtlWork->actor;

    if (player->x < work->obj.x - 0x5000 || work->obj.x + 0x5000 < player->x) {
        return BOS_URSULA_ATTACK_BUBBLE;
    }

    return BOS_URSULA_ATTACK_THUNDER;
}

s32 BosUrsulaChooseAttackPhase1(UrsulaWork* work) {
    if (work->obj.x - 0x3800 < gBtlWork->actor->x && gBtlWork->actor->x < work->obj.x + 0x3800) {
        return BOS_URSULA_ATTACK_THUNDER;
    }

    if (work->obj.x - 0x6800 < gBtlWork->actor->x && gBtlWork->actor->x < work->obj.x + 0x6800) {
        return BOS_URSULA_ATTACK_CHARGE;
    }

    return BOS_URSULA_ATTACK_BUBBLE;
}

s32 BosUrsulaChooseAttackPhase2(UrsulaWork* work) {
    if (gBtlWork->actor->z <= -0x5000) {
        return BOS_URSULA_ATTACK_THUNDER;
    } else {
        if (work->obj.x - 0x3800 < gBtlWork->actor->x && gBtlWork->actor->x < work->obj.x + 0x3800) {
            if ((u16)(GetRandom() % 100) < 50) {
                return BOS_URSULA_ATTACK_THUNDER;
            }

            return BOS_URSULA_ATTACK_BUBBLE;
        }
    }

    if (work->obj.x - 0x8000 < gBtlWork->actor->x && gBtlWork->actor->x < work->obj.x + 0x8000) {
        return BOS_URSULA_ATTACK_CHARGE;
    }

    return BOS_URSULA_ATTACK_BUBBLE;
}

s32 BosUrsulaChooseAttack(UrsulaWork* work) {
    switch (BosUrsulaGetHpPhase()) {
    case BOS_URSULA_HP_PHASE_HIGH:
        return BosUrsulaChooseAttackPhase0(work);
    case BOS_URSULA_HP_PHASE_MID:
        return BosUrsulaChooseAttackPhase1(work);
    case BOS_URSULA_HP_PHASE_LOW:
    default:
        return BosUrsulaChooseAttackPhase2(work);
    }
}

void BosUrsulaRecoverPendingTakos(UrsulaWork* work) {
    if (work->takoRecoverPending) {
        BosUrsulaTakoEndDown(work->tako->work);
        BosUrsulaTakoEndDown(work->tako2->work);
        work->takoRecoverPending = FALSE;
    }
}

void BosUrsulaUpdateTakoRecovery(UrsulaWork* work) {
    if (work->gimmickTimer == 0 && !BosUrsulaTakoIsStoodOn(work->tako->work) && !BosUrsulaTakoIsStoodOn(work->tako2->work)) {
        BosUrsulaRecoverPendingTakos(work);
        work->takoRecoverPending = TRUE;
    } else {
        work->takoRecoverPending = FALSE;
    }
}

u16 BosUrsulaGetCardInterval() {
    switch (BosUrsulaGetHpPhase()) {
    case BOS_URSULA_HP_PHASE_HIGH:
        return 150;
    case BOS_URSULA_HP_PHASE_MID:
        return 120;
    case BOS_URSULA_HP_PHASE_LOW:
    default:
        return 100;
    }
}

u8 task_bos_ursula_1(UrsulaWork* work) {
    BtlObj* obj = &work->obj;
    PrizeCardArg pos;
    s32 x;
    u16 chance;

    switch (UpdateBtlObjReaction(obj)) {
    case BTL_REACTION_CARD_ACTION:
        work->state = BOS_URSULA_STATE_ATTACK;
        work->timer = 0;
        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        BosUrsulaUpdateTakoRecovery(work);
        work->state = BOS_URSULA_STATE_HURT;
        work->timer = 0;
        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        work->state = BOS_URSULA_STATE_DEFEATED;
        work->timer = 0;
        break;
    case BTL_REACTION_CARD_BROKEN:
        work->state = BOS_URSULA_STATE_CANCEL;
        break;
    }

    if (ConsumeGimmickFlag(0)) {
        if (work->gimmickTimer == 0) {
            work->gimmickCameraX = gBtlWork->viewX;
            work->gimmickViewY = gBtlWork->viewY;
            work->gimmickCameraY = gBtlWork->viewY;
            work->sinkSteps = 40;
            work->riseSteps = 40;
            work->unk_15C = 20;
            work->sinkZ = 0;
            work->gimmickDelay = 9;
        }

        work->gimmickTimer = 300;

        if (work->state == BOS_URSULA_STATE_ATTACK) {
            work->state = BOS_URSULA_STATE_CANCEL;
        }
    }

    if (work->gimmickTimer == 0) {
        gBosUrsulaBaseZ = -0x5000;
    } else if (work->gimmickDelay == 0) {
        if (work->sinkSteps != 0) {
            ApproachValue((s32*)&work->sinkZ, 0x3800, work->sinkSteps);
            ApproachValue((s32*)&work->gimmickCameraY, work->gimmickViewY + 0x3800, work->sinkSteps >> 1);
            BtlMapSetCameraTarget(work->gimmickCameraX, work->gimmickCameraY);
            work->sinkSteps--;
        } else {
            if (work->gimmickTimer == 300) {
                BtlMapStartShake();
            }

            work->gimmickTimer--;

            if (work->gimmickTimer == 0 && work->state == BOS_URSULA_STATE_DEFEATED) {
                work->gimmickTimer = 1;
            }

            if (work->gimmickTimer > 280) {
                BtlMapSetCameraTarget(work->gimmickCameraX, work->gimmickCameraY);
            }
        }

        if (work->gimmickTimer == 0 && work->riseSteps != 0) {
            work->gimmickTimer++;
            ApproachValue((s32*)&work->sinkZ, 0, work->riseSteps);
            work->riseSteps--;
        }

        gBosUrsulaBaseZ = work->sinkZ - 0x5000;
    } else {
        work->gimmickDelay--;
    }

    if (BosUrsulaIsGuarded(work)) {
        SetBtlObjUnhittable(&work->obj, TRUE);
    } else {
        SetBtlObjUnhittable(&work->obj, FALSE);
    }

    switch (work->state) {
    case BOS_URSULA_STATE_ATTACK:
        if (work->timer == 0) {
            BosUrsulaStartAttack(BosUrsulaChooseAttack(work));
            work->timer = 1;
        } else {
            if (BosUrsulaIsCharging()) {
                BosUrsulaMoveForward(work);
            }

            if (!BosUrsulaIsAttacking()) {
                ClearBtlObjActionFlags(obj);
                work->state = BOS_URSULA_STATE_IDLE;
            }
        }

        break;
    case BOS_URSULA_STATE_CANCEL:
        ClearBtlObjActionFlags(obj);
        work->state = BOS_URSULA_STATE_IDLE;
        BosUrsulaStartAttack(BOS_URSULA_ATTACK_NONE);
        break;
    case BOS_URSULA_STATE_HURT:
        if (work->timer > 20) {
            ClearBtlObjActionFlags(obj);

            if (BosUrsulaGetHpPhase() == BOS_URSULA_HP_PHASE_MID && !BosUrsulaIsGimmickActive()) {
                work->state = BOS_URSULA_STATE_MOVE;
            } else {
                work->state = BOS_URSULA_STATE_IDLE;
            }

            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case BOS_URSULA_STATE_DEFEATED:
        if (work->timer == 0) {
            BeginBossDefeat(obj);
            BosUrsulaUpdateMapBlocks(work);
            work->timer++;
        } else if (work->timer == 1) {
            work->timer++;
        } else if (work->timer == 2) {
            if (BosUrsulaIsFacingLeft()) {
                x = obj->x + 0x1400;
            } else {
                x = obj->x - 0x1C00;
            }

            BgFxStartBossDeath(x, obj->y + obj->z + 0x1C00);
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
            work->timer++;
        } else if (work->timer == 3) {
            if (!FadeIsActive()) {
                work->timer++;
            }
        } else if (work->timer < 124) {
            work->timer++;

            if (work->timer == 124) {
                BgFxStartBossDeathFlash();
            }
        } else if (!BgFxIsActive()) {
            pos.x = obj->x;

            if (pos.x < 0x2000) {
                pos.x = 0x2000;
            }

            if (pos.x > 0x1E000) {
                pos.x = 0x1E000;
            }

            pos.y = 0x1A800;
            pos.z = obj->z;
            CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &pos);
            EndBossDefeat();
            DropBossPrizes(obj);
            DisableBg(0);
            gBosUrsulaActive = FALSE;
            return 0;
        }

        break;
    case BOS_URSULA_STATE_IDLE:
        if (work->obj.x > gBtlWork->actor->x) {
            obj->flags |= BTLOBJ_FLAG_FACING_LEFT;
        } else {
            obj->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        }

        if (!BosUrsulaIsGimmickActive()) {
            if (BosUrsulaIsGuarded(work)) {
                chance = BosUrsulaGetCardInterval();

                if ((u16)(GetRandom() % chance) == 0) {
                    RequestEnemyCardUse(&work->obj);
                }
            }

            if ((u32)obj->x > 0x20000) {
                work->state = BOS_URSULA_STATE_MOVE;
            }

            if (BosUrsulaGetHpPhase() == BOS_URSULA_HP_PHASE_LOW) {
                if (((obj->x - gBtlWork->actor->x) >= 0 ? obj->x - gBtlWork->actor->x : -(obj->x - gBtlWork->actor->x)) > 0x6800) {
                    work->state = BOS_URSULA_STATE_MOVE;
                }
            }
        }

        BosUrsulaUpdateBob(work);
        break;
    case BOS_URSULA_STATE_MOVE:
        if (BosUrsulaIsGimmickActive()) {
            BosUrsulaUpdateBob(work);
        } else {
            if (!BosUrsulaMoveForward(work)) {
                obj->flags ^= BTLOBJ_FLAG_FACING_LEFT;
            }

            if (BosUrsulaGetHpPhase() == BOS_URSULA_HP_PHASE_LOW && obj->x > 0x6800 && obj->x < 0x19800) {
                if (((obj->x - gBtlWork->actor->x) >= 0 ? obj->x - gBtlWork->actor->x : -(obj->x - gBtlWork->actor->x)) < 0x6800 && BosUrsulaIsGuarded(work)) {
                    RequestEnemyCardUse(&work->obj);
                    work->state = BOS_URSULA_STATE_IDLE;
                }
            }

            if ((!(obj->flags & BTLOBJ_FLAG_FACING_LEFT) && obj->x == 0x6800) || ((obj->flags & BTLOBJ_FLAG_FACING_LEFT) && obj->x == 0x19800)) {
                work->state = BOS_URSULA_STATE_IDLE;
            }
        }

        break;
    }

    if (work->state != BOS_URSULA_STATE_DEFEATED) {
        BosUrsulaUpdateMapBlocks(work);
    }

    if (BosUrsulaIsGuarded(work)) {
        ColliderSetPosition(&obj->collider, obj->x, obj->y, obj->z);
    } else {
        ColliderSetPosition(&obj->collider, obj->x, obj->y + 0x1000, obj->z - 0x1000);
    }

    gBtlWork->bossX = obj->x;
    gBtlWork->bossY = obj->y;
    gBtlWork->bossZ = obj->z;
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_bos_ursula_2(UrsulaWork* work) {
    BtlObj* obj = &work->obj;
    s32 offsetX = 0;

    if (BosUrsulaIsFacingLeft() != 0 && work->mapBlocks == sBosUrsulaMapBlocksHurtRight) {
        offsetX = -0x1000;
    } else if (BosUrsulaIsFacingLeft() == 0 && work->mapBlocks == sBosUrsulaMapBlocksHurtLeft) {
        offsetX = 0x1000;
    }

    ScrollBgMapTo(0, (gBtlWork->viewX - (obj->x - 0x12000) + offsetX) >> 8,
        (gBtlWork->viewY - (obj->y + obj->z - 0x12000)) >> 8);
    TaskPoolDraw(&work->tasks);
}

void task_bos_ursula_3(UrsulaWork* work) {
    ReleaseEnemyBtlObj(&work->obj);
    TaskPoolDestroy(&work->tasks);
    gDispCnt &= ~DISPCNT_WIN0_ON;
}

u8 BosUrsulaIsFacingLeft() {
    return sUrsulaWork->obj.flags & BTLOBJ_FLAG_FACING_LEFT;
}

u8 BosUrsulaIsGimmickActive() {
    if (sUrsulaWork->gimmickTimer == 0) {
        return FALSE;
    }

    return TRUE;
}

u8 BosUrsulaObjectsGone() {
    BtlObj* obj;
    u8 gone = TRUE;

    for (obj = ListPoolFirst(&gBtlWork->pool); obj != NULL; obj = ListPoolNext(&obj->node)) {
        if (obj->kind == ENEMY_URSULA) {
            gone = FALSE;
            break;
        }
    }

    return gone;
}

u8 BosUrsulaIsGimmickStarting() {
    if (BosUrsulaObjectsGone() || !BosUrsulaIsGimmickActive() || sUrsulaWork->gimmickDelay == 0) {
        return FALSE;
    }

    return TRUE;
}

u8 BosUrsulaIsGimmickInProgress() {
    if (BosUrsulaIsGimmickActive() && (sUrsulaWork->sinkSteps != 0 || sUrsulaWork->riseSteps != 0 || sUrsulaWork->unk_15C != 0)) {
        return TRUE;
    }

    return FALSE;
}

u32 BosUrsulaGetHpPhase() {
    UrsulaWork* work = sUrsulaWork;

    if (work->obj.hp > (s16)(work->obj.maxHp / 3) * 2) {
        return BOS_URSULA_HP_PHASE_HIGH;
    }

    if (work->obj.hp > (s16)(work->obj.maxHp / 3)) {
        return BOS_URSULA_HP_PHASE_MID;
    }

    return BOS_URSULA_HP_PHASE_LOW;
}

u8 BosUrsulaIsDefeated() {
    if (sUrsulaWork->state == BOS_URSULA_STATE_DEFEATED) {
        return TRUE;
    }

    return FALSE;
}

void task_bos_ursula_map_0(UrsulaMapWork* work, BattleBackgroundDef* arg) {
    SetupBg(0, 0, 0x1A, 0);
    SetupBg(1, 0, 0x18, 0);
    SetBgPriority(1, 3);
    SetBgPriority(0, 2);
    LoadBgTiles(1, arg->tiles, arg->tilesSize);
    LoadBgPalette(1, arg->palette, arg->paletteSize);
    SetBgMapBlocks(1, arg->map, 2, 2);
    gBtlWork->scale = Q_8_8(1);
    gBtlWork->zoomScale = Q_8_8(1);
    gBtlWork->x = 0x10000;
    gBtlWork->y = 0x17100;
    gBtlWork->viewX = 0x10000;
    gBtlWork->viewY = 0x17100;
    gBtlWork->x2 = 0x10000;
    gBtlWork->y2 = 0x17100;
    gBtlWork->zoomX = 0x10000;
    gBtlWork->zoomY = 0x17100;
    gBtlWork->zoomSteps = 0x0F;
    gBtlWork->rotation = 0;
    BtlMapResetShake();
    ScrollBgMapTo(1, gBtlWork->viewX >> 8, gBtlWork->viewY >> 8);
    gDispCnt |= DISPCNT_WIN0_ON;
    gWin0H = WIN_RANGE(0, DISPLAY_WIDTH);
    gWin0V = WIN_RANGE(80, DISPLAY_HEIGHT);
    gWinIn = (WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR);
    gWinOut = (WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
    work->viewYMax = BOS_URSULA_VIEW_Y_MAX << 8;
    work->viewYMaxTarget = BOS_URSULA_VIEW_Y_MAX << 8;
    work->viewYMaxSteps = 0;
}

u8 task_bos_ursula_map_1(UrsulaMapWork* work) {
    s32 dx;
    s32 dy;
    u8 top;

    if (BosUrsulaIsGimmickStarting()) {
        return 1;
    }

    BtlMapUpdateShake();
    dx = (gBtlWork->x2 - gBtlWork->x) >> 3;
    dy = (gBtlWork->y2 - gBtlWork->y) >> 3;

    if (dx > 0x500) {
        dx = 0x500;
    } else if (dx < -0x500) {
        dx = -0x500;
    }

    if (dy > 0x500) {
        dy = 0x500;
    } else if (dy < -0x500) {
        dy = -0x500;
    }

    gBtlWork->x += dx;
    gBtlWork->y += dy;
    gBtlWork->viewX = gBtlWork->x;
    gBtlWork->viewY = gBtlWork->y;

    if (gBtlWork->viewX - ((DISPLAY_WIDTH / 2) << 8) < gBtlWork->xMin * 256) {
        gBtlWork->viewX = (gBtlWork->xMin + DISPLAY_WIDTH / 2) << 8;
    } else if (gBtlWork->viewX + ((DISPLAY_WIDTH / 2) << 8) > gBtlWork->xMax * 256) {
        gBtlWork->viewX = (gBtlWork->xMax - DISPLAY_WIDTH / 2) << 8;
    }

    if (BosUrsulaIsGimmickInProgress() && work->viewYMaxTarget == (BOS_URSULA_VIEW_Y_MAX << 8)) {
        work->viewYMaxTarget = BOS_URSULA_VIEW_Y_MAX_GIMMICK << 8;
        work->viewYMaxSteps = 20;
    } else if (!BosUrsulaIsGimmickInProgress() && work->viewYMaxTarget == (BOS_URSULA_VIEW_Y_MAX_GIMMICK << 8)) {
        work->viewYMaxTarget = BOS_URSULA_VIEW_Y_MAX << 8;
        work->viewYMaxSteps = 20;
    }

    if (work->viewYMaxSteps != 0) {
        ApproachValue(&work->viewYMax, work->viewYMaxTarget, work->viewYMaxSteps);
        work->viewYMaxSteps--;
    }

    if (gBtlWork->viewY - ((DISPLAY_HEIGHT / 2) << 8) < (BOS_URSULA_VIEW_Y_MIN << 8)) {
        gBtlWork->viewY = (BOS_URSULA_VIEW_Y_MIN + DISPLAY_HEIGHT / 2) << 8;
    } else if (gBtlWork->viewY + ((DISPLAY_HEIGHT / 2) << 8) > work->viewYMax) {
        gBtlWork->viewY = work->viewYMax - ((DISPLAY_HEIGHT / 2) << 8);
    }

    gBtlWork->viewY += BtlMapGetShake();
    ScrollBgMapTo(1, (gBtlWork->viewX >> 8) - DISPLAY_WIDTH / 2, (gBtlWork->viewY >> 8) - DISPLAY_HEIGHT / 2);
    top = -0x18 - (gBtlWork->viewY >> 8);

    if (top > DISPLAY_HEIGHT || !gBosUrsulaActive) {
        gDispCnt &= ~DISPCNT_WIN0_ON;
    } else {
        gDispCnt |= DISPCNT_WIN0_ON;
        gWin0V = (top << 8) | DISPLAY_HEIGHT;
    }

    return 1;
}

void task_bos_ursula_map_3() {
}

void task_bos_ursula_border_0(UrsulaBorderWork* work) {
    work->tiles = LoadObjTiles(gBosUrsulaBorderTiles, sizeof(gBosUrsulaBorderTiles));
    work->palette = LoadObjPalette(gBosUrsulaBorderPalette, sizeof(gBosUrsulaBorderPalette));
    SetBtlPaletteFadeExcluded(work->palette->index + 16, FALSE);
}

s32 task_bos_ursula_border_1() {
    return 1;
}

void task_bos_ursula_border_2(UrsulaBorderWork* work) {
    s16 leftX;
    s16 leftY;
    s16 rightX;
    s16 rightY;

    GetBattleSpritePriorityFlags(0x19800);
    WorldToScreen(&leftX, &leftY, 0x8000, 0x19800, -0x800);
    WorldToScreen(&rightX, &rightY, 0x18000, 0x19800, -0x800);
    DrawSprite(leftX, leftY, gBosUrsulaBorderFrame0, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), 0xFB00);
    DrawSprite(rightX, rightY, gUnk_0979D8B8, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), 0xFB00);
#ifdef PLATFORM_PC
    {
        /* the wide view (Preliminar V2 of the boss arenas, waits for approval): the rocky ridge goes on beside the
           picture, the same two halves one cave (512 world pixels) to each side, as the cave behind them repeats */
        extern int PortArenasV2(void);
        int side;
        if (PortArenasV2())
            for (side = -1; side <= 1; side += 2) {
                WorldToScreen(&leftX, &leftY, 0x8000 + side * 0x20000, 0x19800, -0x800);
                WorldToScreen(&rightX, &rightY, 0x18000 + side * 0x20000, 0x19800, -0x800);
                DrawSprite(leftX, leftY, gBosUrsulaBorderFrame0, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2),
                           0xFB00);
                DrawSprite(rightX, rightY, gUnk_0979D8B8, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), 0xFB00);
            }
    }
#endif
}

void task_bos_ursula_border_3(UrsulaBorderWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void BosUrsulaTakoGetPosition(s32* x, s32* y, s32* z, UrsulaTakoWork* work) {
    s32* bossZ;
    s32 dz;

    *x = gBtlWork->bossX + work->offsetX;

    if (work->isLeft != 0) {
        if (BosUrsulaIsFacingLeft() != 0) {
            *x += -0x2200;
        } else {
            *x += -0x3600;
        }
    } else {
        if (BosUrsulaIsFacingLeft() != 0) {
            *x += 0x3600;
        } else {
            *x += 0x2200;
        }
    }

    *y = gBtlWork->bossY;
    bossZ = &gBtlWork->bossZ;
    dz = work->offsetZ + 0x5000;
    *z = *bossZ + dz;
}

s32 BosUrsulaGetTakoPlatformRadius(u8 isLeft) {
    if (isLeft == BosUrsulaIsFacingLeft()) {
        return 12;
    }

    return 6;
}

enum BosUrsulaTakoState {
    BOS_URSULA_TAKO_STATE_IDLE,
    BOS_URSULA_TAKO_STATE_HURT,
    BOS_URSULA_TAKO_STATE_COLLAPSE,
    BOS_URSULA_TAKO_STATE_DOWN,
    BOS_URSULA_TAKO_STATE_SUBMERGED,
    BOS_URSULA_TAKO_STATE_RESURFACE,
    BOS_URSULA_TAKO_STATE_RESURFACE_WAIT,
    BOS_URSULA_TAKO_STATE_ATTACK
};

void task_bos_ursula_tako_0(UrsulaTakoWork* work, u8* arg) {
    s32 x;
    s32 y;
    s32 z;

    work->isLeft = *arg;
    work->offsetX = 0;
    work->offsetZ = 0;
    BosUrsulaTakoGetPosition(&x, &y, &z, work);
    InitEnemyBtlObj(&work->obj, &sBosUrsulaTakoEmyKind, x, y, z);
    ColliderInit(&work->collider2, COLLIDER_TYPE_PLATFORM, 0x28, 0x20);

    if (work->isLeft != 0) {
        work->animBase = 0xFFFC;
        work->obj.flags |= BTLOBJ_FLAG_FACING_LEFT;
        work->collider2OffsetX = -0x2800;
    } else {
        work->animBase = 0;
        work->collider2OffsetX = 0x2800;
    }

    work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gBosUrsulaTakoFrames, 6), gBosUrsulaTakoTiles);
    work->palette = LoadObjPalette(gBosUrsulaTakoPalette, sizeof(gBosUrsulaTakoPalette));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    AnimInit(&work->anim, gBosUrsulaTakoAnims, gBosUrsulaTakoFrames);
    AnimStart(&work->anim, work->animBase + 4, ANIM_FLAG_LOOP);
    work->state = BOS_URSULA_TAKO_STATE_IDLE;
    ColliderInit(&work->collider, COLLIDER_TYPE_PLATFORM, BosUrsulaGetTakoPlatformRadius(work->isLeft), 1);
    ColliderSetPosition(&work->collider, work->obj.x, work->obj.y + 0x1000, -0x3800);
    SetEnemyHpFromStats(&work->obj, ENEMY_URSULA, Q_8_8(0.2));
}

u8 task_bos_ursula_tako_1(UrsulaTakoWork* work) {
    BtlObj* obj = &work->obj;
    s32 x;
    s32 y;
    s32 z;
    s32 dx;
    s32 dz;

    if (BosUrsulaIsGimmickActive()) {
        SetBtlObjUnhittable(obj, TRUE);
    } else if (work->state <= BOS_URSULA_TAKO_STATE_HURT) {
        SetBtlObjUnhittable(obj, FALSE);
    }

    if (BosUrsulaIsDefeated()) {
        return 1;
    }

    switch (UpdateBtlObjReaction(obj)) {
    case BTL_REACTION_CARD_ACTION:
        work->state = BOS_URSULA_TAKO_STATE_ATTACK;
        work->timer = 0;
        RequestBossCardRandom();
        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        work->state = BOS_URSULA_TAKO_STATE_HURT;
        work->timer = 0;
        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        work->state = BOS_URSULA_TAKO_STATE_COLLAPSE;
        work->timer = 0;
        break;
    case BTL_REACTION_CARD_BROKEN:
        ClearBtlObjActionFlags(obj);
        work->state = BOS_URSULA_TAKO_STATE_IDLE;
        RequestBossCardRandom();
        break;
    }

    switch (work->state) {
    case BOS_URSULA_TAKO_STATE_IDLE:
        AnimChange(&work->anim, work->animBase + 4, ANIM_FLAG_LOOP);
        break;
    case BOS_URSULA_TAKO_STATE_HURT:
        if (work->timer == 0) {
            AnimChange(&work->anim, work->animBase + 7, 0);
        }

        work->timer++;

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(obj);
            work->state = BOS_URSULA_TAKO_STATE_IDLE;
            work->timer = 0;
        }

        break;
    case BOS_URSULA_TAKO_STATE_COLLAPSE:
        if (AnimGetId(&work->anim) == (s16)work->animBase + 4) {
            if (AnimGetFrame(&work->anim) == 0 && AnimIsFrameEnding(&work->anim)) {
                AnimStart(&work->anim, work->animBase + 5, ANIM_FLAG_LOOP);
                SetBtlObjUnhittable(obj, TRUE);

                if ((u16)(GetRandom() % 100) <= 19) {
                    DropGimmickCard(0, obj->x, obj->y, obj->z);
                }
            }
        } else if (AnimGetId(&work->anim) == (s16)work->animBase + 5) {
            if (AnimIsFinished(&work->anim)) {
                ClearBtlObjActionFlags(obj);
                work->state = BOS_URSULA_TAKO_STATE_DOWN;
                work->timer = 0;
            }
        } else {
            AnimStart(&work->anim, work->animBase + 4, ANIM_FLAG_LOOP);
        }

        break;
    case BOS_URSULA_TAKO_STATE_DOWN:
        if (work->timer > 180) {
            work->state = BOS_URSULA_TAKO_STATE_SUBMERGED;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case BOS_URSULA_TAKO_STATE_SUBMERGED:
        if (work->timer > 180) {
            AnimStart(&work->anim, work->animBase + 6, 0);
            work->state = BOS_URSULA_TAKO_STATE_RESURFACE;
            work->offsetZ = 0x800;

            if (work->isLeft) {
                work->offsetX = -0x2AA;
            } else {
                work->offsetX = 0x2AA;
            }

            work->timer = 30;
        } else {
            work->timer++;
        }

        break;
    case BOS_URSULA_TAKO_STATE_RESURFACE:
        AnimReset(&work->anim);

        if (work->timer == 5) {
            ReleaseEnemyBtlObj(&work->obj);
            BosUrsulaTakoGetPosition(&x, &y, &z, work);
            InitEnemyBtlObj(&work->obj, &sBosUrsulaTakoEmyKind, x, y, z);
            work->obj.flags |= 0x400;
            SetEnemyHpFromStats(&work->obj, ENEMY_URSULA, Q_8_8(0.1));
        }

        if (work->timer == 0) {
            if (!BosUrsulaIsGimmickActive()) {
                RequestEnemyCardUse(&work->obj);
            }

            work->state = BOS_URSULA_TAKO_STATE_RESURFACE_WAIT;
            work->timer = 0;
        } else {
            ApproachValue(&work->offsetZ, 0, work->timer);
            ApproachValue(&work->offsetX, 0, work->timer);
            work->timer--;
        }

        break;
    case BOS_URSULA_TAKO_STATE_RESURFACE_WAIT:
        if (work->timer > 30) {
            work->state = BOS_URSULA_TAKO_STATE_IDLE;
        } else {
            work->timer++;
        }

        break;
    case BOS_URSULA_TAKO_STATE_ATTACK:
        AnimChange(&work->anim, work->animBase + 6, 0);

        if (AnimIsFinished(&work->anim) || BosUrsulaIsGimmickActive()) {
            work->state = BOS_URSULA_TAKO_STATE_IDLE;
            AnimStart(&work->anim, work->animBase + 4, ANIM_FLAG_LOOP);
            ClearBtlObjActionFlags(obj);
        } else {
            if (AnimGetFrame(&work->anim) == 1) {
                dx = 0x800;

                if (work->isLeft) {
                    dx = -0x800;
                }

                dz = -0x6000;
            } else if (AnimGetFrame(&work->anim) == 0) {
                dx = -0x1800;

                if (work->isLeft) {
                    dx = 0x1800;
                }

                dz = -0x3800;
            } else {
                dx = 0x1800;

                if (work->isLeft) {
                    dx = -0x1800;
                }

                dz = -0x3800;
            }

            if (ApplyAttackBox(241, obj->x + dx, obj->y + 0x1000, obj->z + dz, 24, 16, 8) == ATTACK_RESULT_HIT) {
                m4aSongNumStart(SONG_BTL_HANE_HIT);
            }
        }

        break;
    }

    AnimUpdate(&work->anim);
    BosUrsulaTakoGetPosition(&obj->x, &obj->y, &obj->z, work);

    if (work->state - 3 <= 4 && gBtlWork->actor->z < -0x5000 && !BosUrsulaIsGimmickActive()) {
        ColliderSetDisabled(&work->collider, FALSE);
        ColliderSetPosition(&work->collider, work->obj.x, work->obj.y + 0x1000, -0x5000);
    } else {
        ColliderSetDisabled(&work->collider, TRUE);
    }

    if (work->state == BOS_URSULA_TAKO_STATE_DOWN && gBtlWork->actor->z <= -0x2000 && gBtlWork->actor->z > -0x3000) {
        ColliderSetDisabled(&work->collider2, FALSE);
        ColliderSetPosition(&work->collider2, work->obj.x + work->collider2OffsetX, work->obj.y + 0x1000, 0);
    } else {
        ColliderSetDisabled(&work->collider2, TRUE);
    }

    return 1;
}

void task_bos_ursula_tako_2(UrsulaTakoWork* work) {
    BtlObj* obj = &work->obj;
    void* pal;
    s16 x;
    s16 y;

    if (work->state != BOS_URSULA_TAKO_STATE_SUBMERGED && !BosUrsulaIsGimmickActive()) {
        pal = StepHitFlash(obj) ? work->palette2 : work->palette;
        WorldToScreen(&x, &y, obj->x, obj->y, obj->z);
        DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, pal, NULL, SPRITE_PRIORITY(2), 0xFC00);
    }
}

void task_bos_ursula_tako_3(UrsulaTakoWork* work) {
    ReleaseEnemyBtlObj(&work->obj);
    ColliderUnregister(&work->collider2);
    ColliderUnregister(&work->collider);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
}

u8 BosUrsulaTakoIsBusy(UrsulaTakoWork* work) {
    if (work->state <= BOS_URSULA_TAKO_STATE_HURT) {
        return FALSE;
    }

    return TRUE;
}

void BosUrsulaTakoEndDown(UrsulaTakoWork* work) {
    if (work->state >= BOS_URSULA_TAKO_STATE_DOWN && work->state <= BOS_URSULA_TAKO_STATE_SUBMERGED) {
        work->state = BOS_URSULA_TAKO_STATE_SUBMERGED;
        work->timer = 180;
    }
}

u8 BosUrsulaTakoIsStoodOn(UrsulaTakoWork* work) {
    if (work->collider.standFlags & COLLIDER_STAND_STOOD_ON) {
        return TRUE;
    }

    return FALSE;
}

void BosUrsulaBacktakoGetPosition(s32* x, s32* y, s32* z, UrsulaBacktakoWork* work) {
    s32* bossZ;
    s32 dz;

    *x = gBtlWork->bossX + work->offsetX;

    if (work->isLeft != 0) {
        if (BosUrsulaIsFacingLeft() != 0) {
            *x += -0x4A00;
        } else {
            *x += -0x5E00;
        }
    } else {
        if (BosUrsulaIsFacingLeft() != 0) {
            *x += 0x5E00;
        } else {
            *x += 0x4A00;
        }
    }

    *y = gBtlWork->bossY + 0x800;
    bossZ = &gBtlWork->bossZ;
    dz = work->offsetZ + 0x5000;
    *z = *bossZ + dz;
}

void task_bos_ursula_backtako_0(UrsulaBacktakoWork* work, u8* arg) {
    work->isLeft = *arg;
    work->offsetX = 0;
    work->offsetZ = 0;
    BosUrsulaBacktakoGetPosition(&work->x, &work->y, &work->z, work);
    work->isLeft = work->isLeft == 0 ? 1 : 0;
    BosUrsulaBacktakoGetPosition(&work->x2, &work->y2, &work->z2, work);
    work->isLeft = work->isLeft == 0 ? 1 : 0;

    if (work->isLeft != 0) {
        work->animBase = 0xFFFC;
    } else {
        work->animBase = 0;
    }

    work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gBosUrsulaTakoFrames, 8), gBosUrsulaTakoTiles);
    work->palette = LoadObjPalette(gBosUrsulaTakoPalette, sizeof(gBosUrsulaTakoPalette));
    AnimInit(&work->anim, gBosUrsulaTakoAnims, gBosUrsulaTakoFrames);
    AnimStart(&work->anim, work->animBase + 4, ANIM_FLAG_LOOP);
    AnimSetFrame(&work->anim, GetRandom() % work->anim.frameCount + 1);
}

u8 task_bos_ursula_backtako_1(UrsulaBacktakoWork* work) {
    if (!BosUrsulaIsDefeated()) {
        BosUrsulaBacktakoGetPosition(&work->x, &work->y, &work->z, work);
        work->isLeft = work->isLeft == 0 ? 1 : 0;
        BosUrsulaBacktakoGetPosition(&work->x2, &work->y2, &work->z2, work);
        work->isLeft = work->isLeft == 0 ? 1 : 0;
        AnimUpdate(&work->anim);
    }

    return 1;
}

void task_bos_ursula_backtako_2(UrsulaBacktakoWork* work) {
    s16 x;
    s16 y;
    u8 gimmickActive = BosUrsulaIsGimmickActive();

    if (gimmickActive) {
        return;
    }

    WorldToScreen(&x, &y, work->x, work->y, work->z);
    DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(3),
        0xFE00);
    WorldToScreen(&x, &y, work->x2, work->y2, work->z2);
    DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(3) | SPRITE_FLAG_HFLIP,
        0xFE00);
}

void task_bos_ursula_backtako_3(UrsulaBacktakoWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_bos_ursula_mapanime_0(UrsulaMapanimeWork* work) {
    sUrsulaMapanimeWork = work;
    TaskPoolInit(&work->tasks, 1);
    work->task = NULL;
    work->attack = BOS_URSULA_ATTACK_BUSY;
    BosUrsulaStartAttack(BOS_URSULA_ATTACK_NONE);
}

u8 task_bos_ursula_mapanime_1(UrsulaMapanimeWork* work) {
    s32 offsetX;

    BosMapanimeUpdate(&work->anim, work->anim.def, FALSE);

    if (BosMapanimeIsAtEnd(&work->anim)) {
        if (work->anim.def == &sBosUrsulaMapanimeWindup) {
            if (work->attack == BOS_URSULA_ATTACK_BUBBLE) {
                BosMapanimeInit(&work->anim, &sBosUrsulaMapanimeBubble);
                work->attack = BOS_URSULA_ATTACK_BUSY;
            } else if (work->attack == BOS_URSULA_ATTACK_CHARGE) {
                BosMapanimeInit(&work->anim, &sBosUrsulaMapanimeCharge);
                work->attack = BOS_URSULA_ATTACK_BUSY;
            }
        } else if (work->anim.def == &sBosUrsulaMapanimeBubble
                || work->anim.def == &sBosUrsulaMapanimeCharge) {
            BosMapanimeInit(&work->anim, &sBosUrsulaMapanimeRecover);
            work->attack = BOS_URSULA_ATTACK_BUSY;
        } else if (work->anim.def == &sBosUrsulaMapanimeRecover) {
            BosMapanimeInit(&work->anim, &sBosUrsulaMapanimeIdle);
            work->attack = BOS_URSULA_ATTACK_NONE;
        }
    }

    if (work->anim.def == &sBosUrsulaMapanimeCharge && BosMapanimeGetFrameIndex(&work->anim) == 2) {
        if (!work->attackSpawned) {
            work->attackSpawned = TRUE;
            BgFxStartUrsulaBeam(gBtlWork->bossX, gBtlWork->bossY + 0xC00,
                gBtlWork->bossZ, BosUrsulaIsFacingLeft(), Q_8_8(2.4), 0x78);
        } else {
            BgFxSetPosition(gBtlWork->bossX, gBtlWork->bossY + 0xC00,
                gBtlWork->bossZ);
        }

        offsetX = BosUrsulaIsFacingLeft() != 0 ? -0x5000 : 0x5000;
        ApplyAttackBox(0xF3, gBtlWork->bossX + offsetX, 0x1C400, 0, 0x18, 0x38, 0x50);
    }

    if (work->anim.def == &sBosUrsulaMapanimeBubble && BosMapanimeGetFrameIndex(&work->anim) == 2
            && !work->attackSpawned) {
        work->attackSpawned = TRUE;
        work->task = TaskCreate(&work->tasks, &gTaskDescBosUrsulaBubble, NULL);
    }

    TaskPoolUpdate(&work->tasks);

    return 1;
}

void task_bos_ursula_mapanime_2(UrsulaMapanimeWork* work) {
    TaskPoolDraw(&work->tasks);
}

void task_bos_ursula_mapanime_3(UrsulaMapanimeWork* work) {
    TaskPoolDestroy(&work->tasks);
}

void BosUrsulaStartAttack(s32 attack) {
    if (IsTaskActive(sUrsulaMapanimeWork->task)) {
        if (strcmp(GetTaskName(sUrsulaMapanimeWork->task), "task_bos_ursula_bubble") == 0) {
            BosUrsulaPopBubbles(sUrsulaMapanimeWork->task->work);
        } else {
            TaskKill(&sUrsulaMapanimeWork->tasks, sUrsulaMapanimeWork->task);
        }
    }

    if (attack == BOS_URSULA_ATTACK_THUNDER) {
        sUrsulaMapanimeWork->task = TaskCreate(&sUrsulaMapanimeWork->tasks, &gTaskDescBosUrsulaThunder, NULL);
    } else if (sUrsulaMapanimeWork->attack != attack) {
        sUrsulaMapanimeWork->attack = attack;

        if (attack == BOS_URSULA_ATTACK_NONE) {
            BosMapanimeInit(&sUrsulaMapanimeWork->anim, &sBosUrsulaMapanimeRecover);
            sUrsulaMapanimeWork->attackSpawned = TRUE;
        } else {
            BosMapanimeInit(&sUrsulaMapanimeWork->anim, &sBosUrsulaMapanimeWindup);
            sUrsulaMapanimeWork->attackSpawned = FALSE;
            m4aSongNumStart(SONG_VO_UR_ATTACK00);
        }
    }
}

u8 BosUrsulaIsAttacking() {
    if (sUrsulaMapanimeWork->anim.def == &sBosUrsulaMapanimeBubble || sUrsulaMapanimeWork->anim.def == &sBosUrsulaMapanimeCharge || sUrsulaMapanimeWork->anim.def == &sBosUrsulaMapanimeWindup) {
        return TRUE;
    }

    return IsTaskActive(sUrsulaMapanimeWork->task);
}

u8 BosUrsulaIsCharging() {
    if (sUrsulaMapanimeWork->anim.def == &sBosUrsulaMapanimeCharge && BosMapanimeGetFrameIndex(&sUrsulaMapanimeWork->anim) == 2) {
        return TRUE;
    }

    return FALSE;
}
