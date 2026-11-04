/**
 * bos_boogie.c
 * Oogie Boogie Boss
 */

#include "bos_boogie.h"
#include "bos4_api.h"
#include "registration_data.h"
#include "card_api.h"
#include "engine_math.h"
#include "fade.h"
#include "btl_effect.h"
#include "btl_collision.h"
#include "obj_api.h"
#include "battle_actor.h"
#include "battle_work.h"
#include "battle_bg_types.h"
#include "prize_types.h"
#include "copyright_screens.h"
#include "sprites_evt.h"
#include "sprites_title.h"
#include "songs.h"
#include "anim.h"
#include "battle_actor_types.h"
#include "listpool.h"
#include "m4a_song.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "default_bg_map.h"
#include "enemy_ids.h"
#include "macros.h"
#include "obj.h"

static BoogieWork* sBoogieWork;

static const StatusAnimDef sBosBoogieAnimDefs[9] = {
    { gBosBoogieIdleAnims, gBosBoogieIdleFrames, gBosBoogieIdleTiles, 0 },
    { gBosBoogieWalkAnims, gBosBoogieWalkFrames, gBosBoogieWalkTiles, 0 },
    { gBosBoogieDiceThrowAnims, gBosBoogieDiceThrowFrames, gBosBoogieDiceThrowTiles, 0 },
    { gBosBoogieDiceThrowAnims, gBosBoogieDiceThrowFrames, gBosBoogieDiceThrowTiles, 1 },
    { gBosBoogieHurtAnims, gBosBoogieHurtFrames, gBosBoogieHurtTiles, 0 },
    { gBosBoogieAttackHitAnims, gBosBoogieAttackHitFrames, gBosBoogieAttackHitTiles, 0 },
    { gBosBoogieDiceFaceAnims, gBosBoogieDiceFaceFrames, gBosBoogieDiceFaceTiles, 0 },
    { gBosBoogieDiceFaceAnims, gBosBoogieDiceFaceFrames, gBosBoogieDiceFaceTiles, 1 },
    { gBosBoogieHurtAnims, gBosBoogieHurtFrames, gBosBoogieHurtTiles, 2 },
};

static const StatusObjDef sBosBoogieSpriteDefs[6] = {
    { gBosBoogieIdleFrames, ARRAY_COUNT(gBosBoogieIdleFrames) },
    { gBosBoogieWalkFrames, ARRAY_COUNT(gBosBoogieWalkFrames) },
    { gBosBoogieDiceThrowFrames, ARRAY_COUNT(gBosBoogieDiceThrowFrames) },
    { gBosBoogieHurtFrames, ARRAY_COUNT(gBosBoogieHurtFrames) },
    { gBosBoogieAttackHitFrames, ARRAY_COUNT(gBosBoogieAttackHitFrames) },
    { gBosBoogieDiceFaceFrames, ARRAY_COUNT(gBosBoogieDiceFaceFrames) },
};

static const EmyKind sBosBoogieEmyKind = { ENEMY_OOGIE_BOOGIE, 0, 68, 16, 32, 0, EMY_KIND_FLAG_LARGE_BODY };

static const BattleBackgroundDef sBosBoogieBattleBackgroundDef = {
    gBosBoogieBgTiles, sizeof(gBosBoogieBgTiles), gBosBoogieBgPalette, sizeof(gBosBoogieBgPalette),
    { gDefaultBgMap, gBosBoogieBgMaps[1], gDefaultBgMap, gBosBoogieBgMaps[0] },
};

void BosBoogieApplyDiceFace(BoogieWork* work) {
    if (gBosBoogieDiceBreakCount <= 2) {
        gBosBoogieAttackHit = FALSE;
        gBosBoogieTaskKnockedDown = FALSE;

        if (gBosBoogieDiceFace == BOS_BOOGIE_DICE_FACE_DISK) {
            work->state = BOS_BOOGIE_STATE_WAIT_TASK;
            work->task = TaskCreate(&work->tasks, &gTaskDescBosBoogieDisk, &work->actor);
        } else if (gBosBoogieDiceFace == BOS_BOOGIE_DICE_FACE_GARGOYLES) {
            work->state = BOS_BOOGIE_STATE_SUMMON;
            work->timer = 0;
            SpawnEnemy(ENEMY_GARGOYLE, 0xA000, 0x24000, 0);
            SpawnEnemy(ENEMY_GARGOYLE, 0x15000, 0x24000, 0);
        } else if (gBosBoogieDiceFace == BOS_BOOGIE_DICE_FACE_WIGHT_KNIGHTS) {
            work->state = BOS_BOOGIE_STATE_SUMMON;
            work->timer = 0;
            SpawnEnemy(ENEMY_WIGHT_KNIGHT, 0xA000, 0x24000, 0);
            SpawnEnemy(ENEMY_WIGHT_KNIGHT, 0x15000, 0x24000, 0);
        } else if (gBosBoogieDiceFace == BOS_BOOGIE_DICE_FACE_KNIVES) {
            work->state = BOS_BOOGIE_STATE_WAIT_TASK;
            work->task = TaskCreate(&work->tasks, &gTaskDescBosBoogieKnifereader, NULL);
        } else if (gBosBoogieDiceFace == BOS_BOOGIE_DICE_FACE_SEARCH_GHOSTS) {
            work->state = BOS_BOOGIE_STATE_SUMMON;
            work->timer = 0;
            SpawnEnemy(ENEMY_SEARCH_GHOST, 0xA000, 0x24000, 0);
            SpawnEnemy(ENEMY_SEARCH_GHOST, 0x15000, 0x24000, 0);
        } else {
            work->state = BOS_BOOGIE_STATE_WAIT_TASK;
            work->task = TaskCreate(&work->tasks, &gTaskDescBosBoogieKaihuku, work);
        }
    }
}

void SetBoogieAnimation(BoogieWork* work, s32 index, u16 flags) {
    if (work->animationIndex != index) {
        work->animationIndex = index;
        AnimChangeWithTables(&work->anim, sBosBoogieAnimDefs[index].animId, flags, sBosBoogieAnimDefs[index].anims, sBosBoogieAnimDefs[index].gfxTable);
        SetObjTileSource(work->tiles, sBosBoogieAnimDefs[index].tiles);
    }
}

u8 ClampBoogiePosition(s32* x, s32* y) {
    u8 clamped;

    clamped = FALSE;

    if (*x < 0xA000) {
        *x = 0xA000;
        clamped = TRUE;
    }

    if (*x > 0x15000) {
        *x = 0x15000;
        clamped = TRUE;
    }

    if (*y < 0x22800) {
        *y = 0x22800;
        clamped = TRUE;
    }

    if (*y > 0x22800) {
        *y = 0x22800;
        clamped = TRUE;
    }

    return clamped;
}

void task_bos_boogie_0(BoogieWork* work) {
    u8 i;
    u16 size;
    u16 bytes;

    sBoogieWork = work;
    TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosBoogieMap, (void*)&sBosBoogieBattleBackgroundDef);
    work->state = BOS_BOOGIE_STATE_IDLE;
    work->timer = 0;
    gBosBoogieDiceFaceReady = FALSE;
    gBosBoogieGimmickCardDropped = FALSE;
    gBosBoogieSakuOpenTime = 0;
    work->cardRequested = FALSE;
    gBosBoogieActor = &work->actor;
    gBosBoogieDiceBreakCount = 0;
    SetBattleBounds(128, 368, 576, 632);
    InitEnemyBtlObj(&work->actor, &sBosBoogieEmyKind, 0x15000, 0x22800, -0x2000);
    work->actor.groundZ = -0x2000;
    work->actor.flags |= BTLOBJ_FLAG_FACING_LEFT;
    SetBtlObjUnhittable(&work->actor, TRUE);
    work->vx = 0;
    work->vy = 0;
    work->vz = 0;
    work->palette = LoadObjPalette(gBoss02objPalette, sizeof(gBoss02objPalette));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    size = 0;

    for (i = 0; i < ARRAY_COUNT(sBosBoogieSpriteDefs); i++) {
        bytes = GetMaxSpriteTileBytes(sBosBoogieSpriteDefs[i].sprites, sBosBoogieSpriteDefs[i].spriteCount);

        if (size < bytes) {
            size = bytes;
        }
    }

    work->tiles = AllocObjTiles(size, NULL);
    AnimInit(&work->anim, NULL, NULL);
    work->animationIndex = 9;
    SetBoogieAnimation(work, 0, 1);
    TaskPoolInit(&work->tasks, 7);
    TaskCreate(&work->tasks, &gTaskDescBosShadow, &work->actor);
    TaskCreate(&work->tasks, &gTaskDescBosBoogieMapanime, NULL);
    TaskCreate(&work->tasks, &gTaskDescBosBoogieSaku, work);
    work->dice = NULL;
    work->task = NULL;
    work->dice2 = NULL;
    work->dice3 = NULL;
    gBtlWork->bossX = work->actor.x;
    gBtlWork->bossY = work->actor.y;
    gBtlWork->bossZ = work->actor.z;
}

enum BosBoogieDefeatStep {
    BOS_BOOGIE_DEFEAT_STEP_DELAY,
    BOS_BOOGIE_DEFEAT_STEP_BEGIN,
    BOS_BOOGIE_DEFEAT_STEP_START_DEATH_FX,
    BOS_BOOGIE_DEFEAT_STEP_DEATH_FX,
    BOS_BOOGIE_DEFEAT_STEP_FLASH
};

u8 task_bos_boogie_1(BoogieWork* work) {
    BtlObj* actor = &work->actor;
    PrizeCardArg pos;
    u16 roll;

    switch (UpdateBtlObjReaction(actor)) {
    case BTL_REACTION_CARD_ACTION:
        work->state = BOS_BOOGIE_STATE_CARD_ACTION;
        work->timer = 0;
        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        work->state = BOS_BOOGIE_STATE_HURT;
        work->timer = 0;
        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        if (work->state != BOS_BOOGIE_STATE_DEFEATED) {
            work->state = BOS_BOOGIE_STATE_DEFEATED;
            work->defeatStep = BOS_BOOGIE_DEFEAT_STEP_DELAY;
            work->timer = 0;
        }

        break;
    case BTL_REACTION_CARD_BROKEN:
        work->state = BOS_BOOGIE_STATE_CARD_BROKEN;
        work->timer = 0;
        break;
    default:
        if (gBosBoogieDiceFaceReady && work->state != BOS_BOOGIE_STATE_DEFEATED) {
            work->state = BOS_BOOGIE_STATE_DICE_FACE;
            work->timer = 0;
        }

        break;
    }

    switch (work->state) {
    case BOS_BOOGIE_STATE_HURT:
        if (work->timer == 0) {
            AnimReset(&work->anim);
            SetBoogieAnimation(work, 4, 1);
            work->vz = -((actor->knockbackLift << 9) >> 8);
            work->vx = ((gSineTable[actor->angle] * 375) >> 8) * actor->knockbackSpeed >> 8;
            work->vy = ((-gSineTable[actor->angle + 64] * 375) >> 8) * actor->knockbackSpeed >> 8;
            work->timer++;
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(actor);
            work->state = BOS_BOOGIE_STATE_IDLE;
            work->timer = 0;
        }

        break;
    case BOS_BOOGIE_STATE_DEFEATED:
        SetBoogieAnimation(work, 8, 0);

        switch (work->defeatStep) {
        case BOS_BOOGIE_DEFEAT_STEP_DELAY:
            if (work->timer <= 1) {
                work->timer++;
            } else {
                work->defeatStep = BOS_BOOGIE_DEFEAT_STEP_BEGIN;
            }

            break;
        case BOS_BOOGIE_DEFEAT_STEP_BEGIN:
            BeginBossDefeat(actor);
            work->defeatStep = BOS_BOOGIE_DEFEAT_STEP_START_DEATH_FX;
            break;
        case BOS_BOOGIE_DEFEAT_STEP_START_DEATH_FX:
            if (!FadeIsActive()) {
                BgFxStartBossDeath(actor->x, actor->y + actor->z - ((s16)sBosBoogieEmyKind.centerHeight << 8));
                SetBtlPaletteFadeExcluded(BIOS_PTR(work->palette)->index + 16, FALSE);
                FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
                work->defeatStep = BOS_BOOGIE_DEFEAT_STEP_DEATH_FX;
                work->timer = 0;
            }

            break;
        case BOS_BOOGIE_DEFEAT_STEP_DEATH_FX:
            if (work->timer <= 119) {
                work->timer++;
            } else {
                work->defeatStep = BOS_BOOGIE_DEFEAT_STEP_FLASH;
                BgFxStartBossDeathFlash();
            }

            break;
        case BOS_BOOGIE_DEFEAT_STEP_FLASH:
            if (!BgFxIsActive()) {
                pos.x = actor->x;
                pos.y = 0x24000;
                pos.z = -0x6400;
                CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &pos);
                EndBossDefeat();
                DropBossPrizes(actor);
                return 0;
            }

            break;
        }

        break;
    case BOS_BOOGIE_STATE_IDLE:
        SetBoogieAnimation(work, 0, 1);

        if (AnimIsFinished(&work->anim)) {
            roll = GetRandom();

            if ((roll & 15) <= 7 && !FadeIsActive()) {
                work->state = BOS_BOOGIE_STATE_WALK;

                if (work->cardRequested) {
                    RequestBossCardRandom();
                    work->cardRequested = FALSE;
                }

                work->timer = 0;
                SetBoogieAnimation(work, 1, 1);
            } else {
                AnimReset(&work->anim);
            }
        }

        break;
    case BOS_BOOGIE_STATE_WALK:
        SetBoogieAnimation(work, 1, 1);
        work->timer++;

        if (gBosBoogieDiceBreakCount <= 2 && !IsTaskActive(work->dice) &&
            !IsTaskActive(work->dice2) && !IsTaskActive(work->dice3) &&
            !IsTaskActive(work->task) && gBtlWork->enemyTileCount <= 0 && !work->cardRequested) {
            roll = GetRandom() % 100;

            if (roll == 0) {
                RequestBossCardValue(8);
                work->cardRequested = TRUE;
                work->timer = 0;
            }
        }

        if (GetBossCardShownValue() == 8) {
            if (work->cardRequested) {
#ifdef VERSION_EU
                if (ConsumeGimmickFlag(0)) {
                    BosBoogieApplyGimmick();
                    break;
                }
#endif

                RequestBossCardRandom();
                work->cardRequested = FALSE;
                work->diceFollower = FALSE;
                work->dice = TaskCreate(&work->tasks, &gTaskDescBosBoogieDice, work);
                work->diceFollower = TRUE;
                work->dice2 = TaskCreate(&work->tasks, &gTaskDescBosBoogieDice, work);
                work->dice3 = TaskCreate(&work->tasks, &gTaskDescBosBoogieDice, work);
                SetBoogieAnimation(work, 2, 1);
                m4aSongNumStart(SONG_VO_BO_ATTACK00);
                work->state = BOS_BOOGIE_STATE_DICE_THROW;
                work->timer = 0;

#ifndef VERSION_EU
                if (ConsumeGimmickFlag(0)) {
                    gBosBoogieGimmickCardDropped = FALSE;
                }
#endif

                break;
            }
        } else if (work->cardRequested && work->timer > 10) {
            RequestBossCardRandom();
            work->cardRequested = FALSE;
        }

        roll = GetRandom();

        if ((roll & 255) == 0 && !work->cardRequested) {
            work->state = BOS_BOOGIE_STATE_IDLE;
            work->timer = 0;
        } else if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
            actor->x -= 256;

            if (actor->x <= 0xA000) {
                actor->x = 0xA000;
                actor->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            }
        } else {
            actor->x += 256;

            if (actor->x >= 0x15000) {
                actor->x = 0x15000;
                actor->flags |= BTLOBJ_FLAG_FACING_LEFT;
            }
        }

        break;
    case BOS_BOOGIE_STATE_DICE_FACE:
        gBosBoogieDiceFaceReady = FALSE;
        SetBoogieAnimation(work, 6, 0);

        if (ConsumeGimmickFlag(0)) {
            BosBoogieApplyGimmick();
            work->state = BOS_BOOGIE_STATE_IDLE;
            work->timer = 0;
        } else if (AnimIsFinished(&work->anim)) {
            BosBoogieApplyDiceFace(work);
        }

        break;
    case BOS_BOOGIE_STATE_SUMMON:
        if (work->timer > 29) {
            work->state = BOS_BOOGIE_STATE_DICE_FACE_END;
        } else {
            work->timer++;
        }

        break;
    case BOS_BOOGIE_STATE_WAIT_TASK:
        if (gBosBoogieAttackHit) {
            work->state = BOS_BOOGIE_STATE_ATTACK_HIT;
            work->timer = 0;
        } else if (gBosBoogieTaskKnockedDown) {
            work->state = BOS_BOOGIE_STATE_IDLE;
            work->timer = 0;
        } else if (!IsTaskActive(work->task)) {
            work->state = BOS_BOOGIE_STATE_DICE_FACE_END;
            work->timer = 0;
        }

        break;
    case BOS_BOOGIE_STATE_DICE_FACE_END:
        SetBoogieAnimation(work, 7, 0);

        if (AnimIsFinished(&work->anim)) {
            work->state = BOS_BOOGIE_STATE_IDLE;
            work->timer = 0;
        }

        break;
    case BOS_BOOGIE_STATE_DICE_THROW:
        SetBoogieAnimation(work, 2, 1);

        if (work->timer == 0) {
            m4aSongNumStart(SONG_BTL_BU_XAI);
        }

        work->timer++;

        if (AnimIsFinished(&work->anim)) {
            work->state = BOS_BOOGIE_STATE_IDLE;
            work->timer = 0;
        }

        break;
    case BOS_BOOGIE_STATE_ATTACK_HIT:
        gBosBoogieAttackHit = FALSE;
        SetBoogieAnimation(work, 5, 1);

        if (AnimIsFinished(&work->anim)) {
            work->state = BOS_BOOGIE_STATE_IDLE;
            work->timer = 0;
        }

        break;
    }

    AnimUpdate(&work->anim);
    actor->z += work->vz;
    work->vz += 66;

    if (actor->z > -0x2000) {
        actor->z = -0x2000;
        work->vz = 0;
    }

    if (work->vx > 0) {
        actor->x += work->vx;
        work->vx -= 17;

        if (work->vx < 0) {
            work->vx = 0;
        }
    } else if (work->vx < 0) {
        actor->x += work->vx;
        work->vx += 17;

        if (work->vx > 0) {
            work->vx = 0;
        }
    }

    if (work->vy > 0) {
        actor->y += work->vy / 2;
        work->vy -= 17;

        if (work->vy < 0) {
            work->vy = 0;
        }
    } else if (work->vy < 0) {
        actor->y += work->vy / 2;
        work->vy += 17;

        if (work->vy > 0) {
            work->vy = 0;
        }
    }

    ClampBoogiePosition(&actor->x, &actor->y);
    ColliderSetPosition(&actor->collider, actor->x, actor->y, actor->z);
    TaskPoolUpdate(&work->tasks);

    if (ConsumeGimmickFlag(0)) {
        BosBoogieApplyGimmick();
    }

    gBtlWork->bossX = actor->x;
    gBtlWork->bossY = actor->y;
    gBtlWork->bossZ = actor->z;
    return 1;
}

void task_bos_boogie_2(BoogieWork* work) {
    BtlObj* actor;
    u16 flags;
    void* pal;
    s16 x;
    s16 y;

    actor = &work->actor;
    flags = GetBattleSpritePriorityFlags(actor->y);

    if (!(actor->flags & BTLOBJ_FLAG_FACING_LEFT)) {
        flags |= SPRITE_FLAG_HFLIP;
    }

    if (StepHitFlash(actor) && work->state != BOS_BOOGIE_STATE_DEFEATED) {
        pal = work->palette2;
    } else {
        pal = work->palette;
    }

    WorldToScreen(&x, &y, actor->x, actor->y, actor->z);
    DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, pal, NULL, flags, -4100 - (actor->y >> 8) * 4);
    TaskPoolDraw(&work->tasks);
}

void task_bos_boogie_3(BoogieWork* work) {
    ReleaseEnemyBtlObj(&work->actor);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    TaskPoolDestroy(&work->tasks);
}

void BosBoogieRemoveOtherEnemies() {
    BtlObj* obj;

    obj = ListPoolFirst(&gBtlWork->pool);

    while (obj != NULL) {
        if (obj->kind != ENEMY_OOGIE_BOOGIE) {
            obj->flags |= BTLOBJ_FLAG_WARP_PENDING;
            obj->hitFlags = 0;
        }

        obj = ListPoolNext(&obj->node);
    }
}

void BosBoogieApplyGimmick() {
    BosBoogieRemoveOtherEnemies();
    gBosBoogieGimmickCardDropped = FALSE;

    if (gBosBoogieDiceBreakCount <= 2) {
        gBosBoogieDiceBreakCount = 3;
        gBosBoogieSakuOpenTime += 540;
    }
}

u32 GetBoogieDiceState() {
    if (IsTaskActive(sBoogieWork->dice)) {
        return ((BoogieDiceWork*)sBoogieWork->dice->work)->state;
    }

    return BOS_BOOGIE_DICE_STATE_NONE;
}

TaskDesc gTaskDescBosBoogie = {
    "task_bos_boogie",
    (TaskInitFunc)task_bos_boogie_0,
    (TaskUpdateFunc)task_bos_boogie_1,
    (TaskDrawFunc)task_bos_boogie_2,
    (TaskDestroyFunc)task_bos_boogie_3,
    sizeof(BoogieWork),
};
