/**
 * hum_hook.c
 * Captain Hook Boss
 */

#include "fade.h"
#include "obj_api.h"
#include "pallet.h"
#include "hum.h"
#include "sprites_btl.h"
#include "sprites_evt.h"
#include "sprites_hum.h"
#include "btl_api.h"
#include "hum_common.h"
#include "songs.h"
#include "hum_tasks.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "display.h"
#include "engine_math.h"
#include "hum_types.h"
#include "m4a_song.h"
#include "obj.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "enemy_ids.h"
#include "card_ids.h"

static const u32 sHumHookStockMovesA[3] = {
    MOVE_BOSS_A, MOVE_BOSS_A, MOVE_BOSS_C,
};

static const u32 sHumHookStockMovesB[3] = {
    MOVE_BOSS_B, MOVE_BOSS_B, MOVE_BOSS_C,
};

static const AnimDef sHumHookAnimDefs[15] = {
    { gHookBt00Frames, gHookBt00Anims, gHookBt00Tiles, 0 },
    { gHookBt01Frames, gHookBt01Anims, gHookBt01Tiles, 0 },
    { gHookBt02Frames, gHookBt02Anims, gHookBt02Tiles, 0 },
    { gHookBt03Frames, gHookBt03Anims, gHookBt03Tiles, 0 },
    { gHookBt03Frames, gHookBt03Anims, gHookBt03Tiles, 1 },
    { gHookBt03Frames, gHookBt03Anims, gHookBt03Tiles, 2 },
    { gHookBt03Frames, gHookBt03Anims, gHookBt03Tiles, 3 },
    { gHookBt03Frames, gHookBt03Anims, gHookBt03Tiles, 4 },
    { gHookBt10Frames, gHookBt10Anims, gHookBt10Tiles, 0 },
    { gHookBt11Frames, gHookBt11Anims, gHookBt11Tiles, 1 },
    { gHookBt11Frames, gHookBt11Anims, gHookBt11Tiles, 0 },
    { gHookBt11Frames, gHookBt11Anims, gHookBt11Tiles, 2 },
    { gHookBt12Frames, gHookBt12Anims, gHookBt12Tiles, 0 },
    { gHookF03Frames, gHookF03Anims, gHookF03Tiles, 2 },
    { gHookF03Frames, gHookF03Anims, gHookF03Tiles, 3 },
};

static const HumDef sHumHookDef = { 128, gHookPalette, 0, { ENEMY_HOOK, 99, 38, 14, 24, 99, 0 } };

static const u8 sHumHookRollAmplitudes[8] = {
    0, 4, 4, 6, 8, 10, 8, 6,
};

TaskDesc gTaskDescHumHook = {
    "task_hum_hook",
    (TaskInitFunc)task_hum_hook_0,
    (TaskUpdateFunc)task_hum_hook_1,
    (TaskDrawFunc)task_hum_hook_2,
    (TaskDestroyFunc)task_hum_hook_3,
    sizeof(HookWork),
};

static TaskDesc sTaskDescHumHookMoon = {
    "task_hum_hook_moon",
    (TaskInitFunc)task_hum_hook_moon_0,
    (TaskUpdateFunc)task_hum_hook_moon_1,
    (TaskDrawFunc)task_hum_hook_moon_2,
    (TaskDestroyFunc)task_hum_hook_moon_3,
    sizeof(HookMoonWork),
};

static TaskDesc sTaskDescHumHookBomb = {
    "task_hum_hook_bomb",
    (TaskInitFunc)task_hum_hook_bomb_0,
    (TaskUpdateFunc)task_hum_hook_bomb_1,
    (TaskDrawFunc)task_hum_hook_bomb_2,
    (TaskDestroyFunc)task_hum_hook_bomb_3,
    sizeof(HookBombWork),
};

enum HumHookState {
    HUM_HOOK_STATE_LUNGE = 19,
    HUM_HOOK_STATE_SLASH,
    HUM_HOOK_STATE_DASH_SLASH,
    HUM_HOOK_STATE_JUMP_CROUCH,
    HUM_HOOK_STATE_JUMP_AIR,
    HUM_HOOK_STATE_JUMP_LAND,
    HUM_HOOK_STATE_BOMB_THROW,
    HUM_HOOK_STATE_FLURRY,
    HUM_HOOK_STATE_FLURRY_FINISH,
    HUM_HOOK_STATE_BOMB_SCATTER,
    HUM_HOOK_STATE_BOMB_BARRAGE
};

enum HumHookBombVariant {
    HUM_HOOK_BOMB_AIMED,
    HUM_HOOK_BOMB_SCATTER,
    HUM_HOOK_BOMB_BARRAGE
};

void HookJumpOffset(CloudWork* work, s16 distance, s32 speed) {
    HumWork* base = &work->base;
    BtlObj* act = &base->actor;

    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
        work->base.targetX = act->x - (distance << 8);
    } else {
        work->base.targetX = act->x + (distance << 8);
    }

    base->targetY = act->y;
    base->state = HUM_HOOK_STATE_JUMP_CROUCH;
    base->stateTimer = 0;
    work->speed = -speed;
}

void HookJumpTo(CloudWork* work, s32 x, s32 y) {
    work->base.targetX = x;
    work->base.targetY = y;
    work->base.state = HUM_HOOK_STATE_JUMP_CROUCH;
    work->base.stateTimer = 0;
    work->speed = -0x680;
}

u8 HookTryJumpAway(CloudWork* work) {
    s32 x;
    s32 y;
    BtlObj* player;

    player = gBtlWork->actor;
    GetEnemyTargetPosition(&work->base.actor, &x, &y, NULL);
    HumFaceTarget(&work->base, 1);

    if (HumIsInPlayerReach(&work->base, 0x100, 0x100, 0x100)) {
        if (gBtlWork->flags & BTL_FLAG_PLAYER_AIRBORNE) {
            HookJumpOffset(work, -99, 0x280);
        } else if (GetRandom() & 1) {
            if (player->flags & BTLOBJ_FLAG_FACING_LEFT) {
                HookJumpTo(work, x + 0x2800, y);
            } else {
                HookJumpTo(work, x - 0x2800, y);
            }
        } else {
            HookJumpOffset(work, -80, 0x500);
        }

        return TRUE;
    }

    return FALSE;
}

void task_hum_hook_0(HookWork* work, void* arg) {
    TaskCreate(&gBtlWork->taskPools[0], &sTaskDescHumHookMoon, NULL);
    HumInit(&work->base, &sHumHookDef);
    work->base.actor.flags |= BTLOBJ_FLAG_IMMUNE_THUNDER;

    if (GetRandom() % 2) {
        work->base.stockMoves = sHumHookStockMovesA;
    } else {
        work->base.stockMoves = sHumHookStockMovesB;
    }

    work->base.flags |= HUM_FLAG_BOSS_DEATH;
    work->speed = 0;
    work->playerSlide = 0;
    work->slide = 0;
    work->angle = 0;
    work->rollLevel = 0;
    work->flags = 0;
    TaskPoolInit(&work->tasks, 3);
}

u8 task_hum_hook_1(HookWork* work) {
    HookWork* w;
    BtlObj* act;
    BtlObj* player;
    VixenNdlArgs args;
    s32 x;
    s32 y;
    s32 z;
    u16 frame;
    u8 angle;

    w = work;
    act = &work->base.actor;
    player = gBtlWork->actor;
    GetEnemyTargetPosition(act, &x, &y, &z);

    if (HumUpdateReaction(&work->base) == BTL_REACTION_CARD_ACTION) {
        work->base.stateTimer = 0;

        switch ((u32)HumResolveCardMove(&work->base)) {
        case MOVE_BOSS_A:
            work->base.state = HUM_HOOK_STATE_SLASH;
            break;
        case MOVE_BOSS_B:
            work->base.state = HUM_HOOK_STATE_DASH_SLASH;
            break;
        case MOVE_BOSS_C:
            work->base.state = HUM_HOOK_STATE_LUNGE;
            break;
        case MOVE_BOSS_D:
            work->base.state = HUM_HOOK_STATE_BOMB_THROW;
            break;
        case MOVE_COMBO_PRESENT:
            work->base.state = HUM_HOOK_STATE_FLURRY;
            break;
        case MOVE_RUSH_PRESENT:
            work->base.state = HUM_HOOK_STATE_BOMB_BARRAGE;
            work->base.steps = 0;
            break;
        }
    }

    if (HumChooseCardAction(&work->base, 13, 40, 40, 24)) {
        if (GetRandom() % 2) {
            w->base.stockMoves = sHumHookStockMovesA;
        } else {
            w->base.stockMoves = sHumHookStockMovesB;
        }
    }

    switch (work->base.state) {
    case HUM_STATE_ENTER:
    case HUM_STATE_USE_ITEM:
        AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 0, ANIM_FLAG_LOOP, w->base.tiles);
        break;
    case HUM_STATE_RELOAD: {
        s32 dx;

        AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 0, ANIM_FLAG_LOOP | ANIM_FLAG_RANDOM_START, w->base.tiles);

        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            if (HookTryJumpAway((CloudWork*)w)) {
                break;
            }
        }

        dx = act->x - x;

        if ((dx >= 0) ? dx <= 0x3FFF : (dx = x - act->x) <= 0x3FFF) {
            if (x <= 0xFFFF) {
                HookJumpTo((CloudWork*)w, (gBtlWork->xMax - 40) << 8,
                    (gBtlWork->yMin + gBtlWork->yMax) << 7);
            } else {
                HookJumpTo((CloudWork*)w, (gBtlWork->xMin + 40) << 8,
                    (gBtlWork->yMin + gBtlWork->yMax) << 7);
            }
        }

        break;
    }
    case HUM_STATE_IDLE:
        AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 0, ANIM_FLAG_LOOP | ANIM_FLAG_RANDOM_START, w->base.tiles);

        if (func_08081828()) {
            break;
        }

        if (GetRandom() % 150 == 0) {
            work->base.state = HUM_STATE_MOVE;
            work->base.stateTimer = 0;
            break;
        }

        if (HumIsNearAreaEdge(&work->base, 40)) {
            HookJumpTo((CloudWork*)w, 0x10000,
                (gBtlWork->yMin + gBtlWork->yMax) << 7);
            break;
        }

        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            if (HookTryJumpAway((CloudWork*)w)) {
                break;
            }
        } else {
            HumFaceTarget(&work->base, 8);
        }

        work->base.stateTimer++;
        break;
    case HUM_STATE_MOVE:
        AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 1, ANIM_FLAG_LOOP | ANIM_FLAG_RANDOM_START, w->base.tiles);
        work->base.targetX = x;
        work->base.targetY = y;

        if (HumMoveToward(&work->base, work->base.targetX, y, 358)) {
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
            break;
        }

        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            if (HookTryJumpAway((CloudWork*)w)) {
                break;
            }
        } else {
            HumFaceTarget(&work->base, 8);
        }

        work->base.stateTimer++;
        break;
    case HUM_STATE_DEFEATED:
        AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 2, 0, w->base.tiles);
        gBtlWork->rotation = 0;
        break;
    case HUM_STATE_HURT:
    case HUM_STATE_CARD_BROKEN:
    case HUM_STATE_STUNNED:
    case HUM_STATE_GRAVITY:
        AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 2, 0, w->base.tiles);
        break;
    case HUM_STATE_HURT_RECOVER:
        if (func_08081828() == 0) {
            break;
        }

        if (GetRandom() % 10 != 0) {
            break;
        }

        if (x <= 0xFFFF) {
            work->base.targetX = (gBtlWork->xMax - 40) << 8;
        } else {
            work->base.targetX = (gBtlWork->xMin + 40) << 8;
        }

        work->base.targetY = (gBtlWork->yMin + gBtlWork->yMax) << 7;
        work->base.state = HUM_HOOK_STATE_JUMP_AIR;
        work->base.stateTimer = 0;
        work->base.vz = -0x680;
        break;
    case HUM_HOOK_STATE_FLURRY:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 11, ANIM_FLAG_LOOP, w->base.tiles);
        }

        BtlMapFollowPosition(act->x, act->y, act->z);
        HumFaceTarget(&work->base, 1);

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            s32 frontX = act->x - 0x1000;
            act->x += (x - frontX) >> 4;
        } else {
            s32 frontX = act->x + 0x1000;
            act->x += (x - frontX) >> 4;
        }

        act->y += (y - act->y) >> 4;

        if (work->base.anim.timer == 0) {
            switch (AnimGetFrame(&work->base.anim)) {
            case 2:
            case 4:
            case 7:
                MakeOpponentsHittable();

                if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                    ? ApplyAttackBox(280, act->x - 0x1400, act->y, act->z, 20, 20, 50)
                    : ApplyAttackBox(280, act->x + 0x1400, act->y, act->z, 20, 20, 50)) {
                    m4aSongNumStart(SONG_BTL_MON_SWORD00);
                }

                break;
            }
        }

        if (work->base.stateTimer > 120) {
            work->base.state = HUM_HOOK_STATE_FLURRY_FINISH;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_FLURRY_FINISH:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 9, 0, w->base.tiles);
        }

        HumFaceTarget(&work->base, 1);

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            s32 frontX = act->x - 0x1000;
            act->x += (x - frontX) >> 4;
        } else {
            s32 frontX = act->x + 0x1000;
            act->x += (x - frontX) >> 4;
        }

        act->y += (y - act->y) >> 4;

        if (work->base.anim.timer == 0 && AnimGetFrame(&work->base.anim) == 3) {
            MakeOpponentsHittable();

            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(0x119, act->x - 0x1400, act->y, act->z, 20, 20, 50)
                : ApplyAttackBox(0x119, act->x + 0x1400, act->y, act->z, 20, 20, 50)) {
                m4aSongNumStart(SONG_BTL_MON_SWORD00);
            }
        }

        if (AnimIsFinished(&work->base.anim)) {
            work->base.state = HUM_HOOK_STATE_BOMB_SCATTER;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_BOMB_SCATTER:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 12, 0, w->base.tiles);
            w->bombTask = NULL;
            w->bombTask2 = NULL;
            w->bombTask3 = NULL;
            w->flags &= ~(HOOK_FLAG_BOMB_THROWN | HOOK_FLAG_POST_THROW_ANIM);
            m4aSongNumStart(SONG_VO_HO_VOICE00);
        }

        if ((w->flags & HOOK_FLAG_BOMB_THROWN) == 0) {
            if (AnimGetFrame(&work->base.anim) == 6 && work->base.anim.timer == 0) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    args.x = act->x - 0x3200;
                    args.y = act->y;
                    args.z = act->z - 0x1C00;
                    args.facingLeft = 1;
                    args.variant = HUM_HOOK_BOMB_SCATTER;
                } else {
                    args.x = act->x + 0x3200;
                    args.y = act->y;
                    args.z = act->z - 0x1C00;
                    args.facingLeft = 0;
                    args.variant = HUM_HOOK_BOMB_SCATTER;
                }

                w->bombTask = TaskCreate(&w->tasks, &sTaskDescHumHookBomb, &args);
                w->bombTask2 = TaskCreate(&w->tasks, &sTaskDescHumHookBomb, &args);
                w->bombTask3 = TaskCreate(&w->tasks, &sTaskDescHumHookBomb, &args);
                w->flags |= HOOK_FLAG_BOMB_THROWN;
            }
        } else if (w->flags & HOOK_FLAG_POST_THROW_ANIM) {
            if (AnimIsFinished(&work->base.anim)) {
                AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 14, ANIM_FLAG_LOOP, w->base.tiles);
            }
        } else {
            if (AnimIsFinished(&work->base.anim)) {
                AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 13, 0, w->base.tiles);
                w->flags |= HOOK_FLAG_POST_THROW_ANIM;
            }
        }

        if ((w->flags & HOOK_FLAG_BOMB_THROWN) &&
            !IsTaskActiveNamed(w->bombTask, sTaskDescHumHookBomb.name) &&
            !IsTaskActiveNamed(w->bombTask2, sTaskDescHumHookBomb.name) &&
            !IsTaskActiveNamed(w->bombTask3, sTaskDescHumHookBomb.name)) {
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_BOMB_THROW:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 12, 0, w->base.tiles);
            w->bombTask = NULL;
            w->flags &= ~(HOOK_FLAG_BOMB_THROWN | HOOK_FLAG_POST_THROW_ANIM);
            m4aSongNumStart(SONG_VO_HO_VOICE00);
        }

        if ((w->flags & HOOK_FLAG_BOMB_THROWN) == 0) {
            if (AnimGetFrame(&work->base.anim) == 6 && work->base.anim.timer == 0) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    args.x = act->x - 0x3200;
                    args.y = act->y;
                    args.z = act->z - 0x1C00;
                    args.facingLeft = 1;
                    args.variant = HUM_HOOK_BOMB_AIMED;
                } else {
                    args.x = act->x + 0x3200;
                    args.y = act->y;
                    args.z = act->z - 0x1C00;
                    args.facingLeft = 0;
                    args.variant = HUM_HOOK_BOMB_AIMED;
                }

                w->bombTask = TaskCreate(&w->tasks, &sTaskDescHumHookBomb, &args);
                w->flags |= HOOK_FLAG_BOMB_THROWN;
            }
        } else if (w->flags & HOOK_FLAG_POST_THROW_ANIM) {
            if (AnimIsFinished(&work->base.anim)) {
                AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 14, ANIM_FLAG_LOOP, w->base.tiles);
            }
        } else {
            if (AnimIsFinished(&work->base.anim)) {
                AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 13, 0, w->base.tiles);
                w->flags |= HOOK_FLAG_POST_THROW_ANIM;
            }
        }

        if ((w->flags & HOOK_FLAG_BOMB_THROWN) &&
            !IsTaskActiveNamed(w->bombTask, sTaskDescHumHookBomb.name)) {
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_BOMB_BARRAGE:
        if (work->base.stateTimer == 0) {
            AnimReset(&work->base.anim);
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 12, 0, w->base.tiles);
            w->bombTask = NULL;
            w->flags &= ~HOOK_FLAG_BOMB_THROWN;
            m4aSongNumStart(SONG_VO_HO_VOICE00);
        }

        HumFaceTarget(&work->base, 1);

        if ((w->flags & HOOK_FLAG_BOMB_THROWN) == 0) {
            if (AnimGetFrame(&work->base.anim) == 6 && work->base.anim.timer == 0) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    args.x = act->x - 0x3200;
                    args.y = act->y;
                    args.z = act->z - 0x1C00;
                    args.facingLeft = 1;
                    args.variant = HUM_HOOK_BOMB_BARRAGE;
                } else {
                    args.x = act->x + 0x3200;
                    args.y = act->y;
                    args.z = act->z - 0x1C00;
                    args.facingLeft = 0;
                    args.variant = HUM_HOOK_BOMB_BARRAGE;
                }

                w->bombTask = TaskCreate(&w->tasks, &sTaskDescHumHookBomb, &args);
                w->flags |= HOOK_FLAG_BOMB_THROWN;
            }
        }

        if (w->flags & HOOK_FLAG_BOMB_THROWN) {
            if (work->base.steps <= 4) {
                work->base.stateTimer = 0;
                work->base.steps++;
                work->base.state = HUM_HOOK_STATE_BOMB_BARRAGE;
            } else {
                if (!IsTaskActiveNamed(w->bombTask, sTaskDescHumHookBomb.name)) {
                    work->base.stateTimer = 0;
                    ClearBtlObjActionFlags(act);
                    work->base.state = HUM_STATE_IDLE;
                }
            }
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_SLASH:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 9, 0, w->base.tiles);
            m4aSongNumStart(SONG_VO_HO_VOICE00);
        }

        frame = AnimGetFrame(&work->base.anim);

        if (frame > 1) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 backX = act->x + 0x1400;
                act->x += (act->originX - backX) >> 3;
            } else {
                s32 backX = act->x - 0x1400;
                act->x += (act->originX - backX) >> 3;
            }
        }

        if (frame == 2) {
            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(0x115, act->x - 0x2000, act->y, act->z, 16, 16, 50)
                : ApplyAttackBox(0x115, act->x + 0x2000, act->y, act->z, 16, 16, 50)) {
                m4aSongNumStart(SONG_BTL_MON_SWORD00);
            }
        } else if (AnimIsFinished(&work->base.anim)) {
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
            break;
        }

        work->base.stateTimer++;
        break;
    case HUM_HOOK_STATE_LUNGE:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 8, 0, w->base.tiles);
            m4aSongNumStart(SONG_VO_HO_VOICE01);
        }

        frame = AnimGetFrame(&work->base.anim);

        if (frame >= 3 && frame <= 5) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 backX = act->x + 0x4000;
                act->x += (act->originX - backX) >> 3;
            } else {
                s32 backX = act->x - 0x4000;
                act->x += (act->originX - backX) >> 3;
            }
        }

        switch (frame) {
        case 4:
        case 5:
            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(0x115, act->x - 0x4400, act->y, act->z, 20, 16, 50)
                : ApplyAttackBox(0x115, act->x + 0x4400, act->y, act->z, 20, 16, 50)) {
                m4aSongNumStart(SONG_BTL_MON_SWORD00);
            }

            break;
        }

        if (AnimIsFinished(&work->base.anim)) {
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_DASH_SLASH:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 10, 0, w->base.tiles);
        }

        HumFaceTarget(&work->base, 1);
        frame = AnimGetFrame(&work->base.anim);

        switch (frame) {
        case 2:
        case 6:
            if (work->base.anim.timer == 0) {
                if (GetRandom() & 1) {
                    m4aSongNumStart(SONG_VO_HO_VOICE00);
                } else {
                    m4aSongNumStart(SONG_VO_HO_VOICE01);
                }

                angle = GetAngle(act->x, act->y, x, y);
                work->base.targetX = act->x + gSineTable[angle] * 50;
                work->base.targetY = act->y + -gSineTable[angle + 64] * 50;
                MakeOpponentsHittable();
            }

            act->x += (work->base.targetX - act->x) >> 3;
            act->y += (work->base.targetY - act->y) >> 3;

            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(278, act->x - 0x1000, act->y, act->z, 32, 24, 70)
                : ApplyAttackBox(278, act->x + 0x1000, act->y, act->z, 32, 24, 70)) {
                m4aSongNumStart(SONG_BTL_MON_SWORD01);
            }

            break;
        }

        if (AnimIsFinished(&work->base.anim)) {
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_JUMP_CROUCH:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 3, 0, w->base.tiles);
        }

        if (AnimIsFinished(&work->base.anim)) {
            work->base.stateTimer = 0;
            work->base.state = HUM_HOOK_STATE_JUMP_AIR;
            work->base.vz = w->speed;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HOOK_STATE_JUMP_AIR: {
        s32 vz;

        act->x += (work->base.targetX - act->x) >> 4;
        act->y += (work->base.targetY - act->y) >> 4;
        vz = work->base.vz;

        if (vz < 0) {
            if (vz > -0x200) {
                AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 5, 0, w->base.tiles);
            } else {
                AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 4, 0, w->base.tiles);
            }
        } else if (vz <= 0x1FF) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 5, 0, w->base.tiles);
        } else {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 6, 0, w->base.tiles);
        }

        if (act->z >= 0) {
            work->base.stateTimer = 0;
            work->base.state = HUM_HOOK_STATE_JUMP_LAND;
        } else {
            HumFaceTarget(&work->base, 1);
            work->base.stateTimer++;
        }

        break;
    }
    case HUM_HOOK_STATE_JUMP_LAND:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHookAnimDefs, &w->base.anim, 7, 0, w->base.tiles);
        }

        if (AnimIsFinished(&work->base.anim)) {
            work->base.stateTimer = 0;
            work->base.state = HUM_STATE_IDLE;
        } else {
            work->base.stateTimer++;
        }

        break;
    }

    if (act->hp > 0) {
        gBtlWork->rotation = (SIN(w->angle / 2) * sHumHookRollAmplitudes[w->rollLevel]) >> 8;

        {
            s32 rollAngle;

            rollAngle = w->angle + 1;
            w->angle = rollAngle;

            if ((rollAngle & 511) == 0) {
                w->rollLevel++;

                if (w->rollLevel > 7) {
                    w->rollLevel = 1;
                }
            }
        }

        if (player->z >= player->groundZ && player->hp > 0 && player->badStatus != BAD_STATUS_STOP &&
            !(player->flags & BTLOBJ_FLAG_IN_CARD_ACTION)) {
            w->playerSlide += ((GetAngleDiff(0, gBtlWork->rotation) << 6) - w->playerSlide) >> 4;
            player->x -= w->playerSlide;
        } else {
            w->playerSlide = 0;
        }

        if (act->z >= act->groundZ && act->hp > 0 && act->badStatus != BAD_STATUS_STOP &&
            !(act->flags & BTLOBJ_FLAG_IN_CARD_ACTION)) {
            w->slide += ((GetAngleDiff(0, gBtlWork->rotation) << 6) - w->slide) >> 4;
            act->x -= w->slide;
        } else {
            w->slide = 0;
        }
    }

    TaskPoolUpdate(&w->tasks);
    return HumUpdate(&work->base);
}

void task_hum_hook_2(HookWork* work) {
    TaskPoolDraw(&work->tasks);
    HumDraw(&work->base);
}

void task_hum_hook_3(HookWork* work) {
    TaskPoolDestroy(&work->tasks);
    HumReleaseResources(&work->base);
    gBtlWork->rotation = 0;
}

void task_hum_hook_moon_0(HookMoonWork* work) {
    work->tiles = LoadObjTiles(gHumHookMoonTiles, sizeof(gHumHookMoonTiles));
    PushPaletteEffect(0);
    work->palette = LoadObjPalette(gHumHookMoonPalette, sizeof(gHumHookMoonPalette));
    PopPaletteEffect();
    SetBtlPaletteFadeExcluded(BIOS_PTR(work->palette)->index + 16, FALSE);
    work->backdropSet = FALSE;
    work->angle = 0;
}

u8 task_hum_hook_moon_1(HookMoonWork* work) {
    work->angle++;
    return 1;
}

void task_hum_hook_moon_2(HookMoonWork* work) {
    s16 x;
    s16 y;
    u16 fade;
    u16 blue;
    s32 bob;

    x = 248 - (gBtlWork->viewX >> 9);
    y = 208 - (gBtlWork->viewY >> 9);
    bob = gSineTable[(u8)work->angle];
    y += bob >> 5;
    DrawSprite(x + 64, y - 28, gHumHookMoonFrame0, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 0xFFFF);
    DrawSprite(x - 144, y, gHumHookMoonFrame1, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 0xFFFE);
    DrawSprite(x - 88, y, gHumHookMoonFrame1, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 0xFFFE);
    DrawSprite(x - 32, y, gHumHookMoonFrame1, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 0xFFFE);
    DrawSprite(x + 24, y, gHumHookMoonFrame1, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 0xFFFE);
    DrawSprite(x + 80, y, gHumHookMoonFrame1, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 0xFFFE);
    fade = FadeGetAmount();

    if (fade != 0) {
        switch (FadeGetColor()) {
        case 0:
            blue = 9 - fade;

            if ((s16)blue < 0) {
                blue = 0;
            }

            SetBackdropColor(0, 0, blue);
            break;
        case 0x7FFF:
            blue = fade + 9;

            if ((s16)blue > 31) {
                blue = 31;
            }

            SetBackdropColor(fade, fade, blue);
            break;
        case 31:
            SetBackdropColor(fade, 0, 9);
            break;
        case 0x7C00:
            blue = fade + 9;

            if ((s16)blue > 31) {
                blue = 31;
            }

            SetBackdropColor(0, 0, blue);
            break;
        case 0x3E0:
            SetBackdropColor(0, fade, 9);
            break;
        }

        work->backdropSet = TRUE;
    } else if (work->backdropSet) {
        SetBackdropColor(0, 0, 9);
        work->backdropSet = fade;
    }
}

void task_hum_hook_moon_3(HookMoonWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum HumHookBombState {
    HUM_HOOK_BOMB_STATE_BOUNCE,
    HUM_HOOK_BOMB_STATE_EXPLODE
};

void task_hum_hook_bomb_0(HookBombWork* work, VixenNdlArgs* args) {
    if (args->facingLeft != 0) {
        work->facingLeft = TRUE;
    } else {
        work->facingLeft = FALSE;
    }

    work->palette = LoadObjPalette(gPBakudanPalette, sizeof(gPBakudanPalette));
    work->tiles = AllocObjTiles(0x280, gPBakudanTiles);
    AnimInit(&work->anim, gPBakudanAnims, gPBakudanFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->x = args->x;
    work->y = args->y;
    work->z = args->z;
    work->variant = args->variant;
    work->timer = 0;
    work->bounceCount = 0;
    work->speed = GetRandom() % 0x201 + 0x14C;
    work->vz = -(GetRandom() % 0x201 + 0x100);

    switch (work->variant) {
    case HUM_HOOK_BOMB_AIMED:
        work->angle = GetAngle(work->x, work->y,
            gBtlWork->targetX, gBtlWork->targetY);
        work->maxBounces = GetRandom() % 3 + 1;
        work->state = HUM_HOOK_BOMB_STATE_BOUNCE;
        break;
    case HUM_HOOK_BOMB_BARRAGE:
        work->angle = GetAngle(work->x, work->y,
            gBtlWork->targetX, gBtlWork->targetY);
        work->maxBounces = 0;
        work->state = HUM_HOOK_BOMB_STATE_EXPLODE;
        break;
    case HUM_HOOK_BOMB_SCATTER:
    default:
        work->angle = GetRandom();
        work->maxBounces = GetRandom() % 5 + 4;
        work->state = HUM_HOOK_BOMB_STATE_BOUNCE;
        break;
    }

    work->tiles2 = LoadObjTiles(gBtlShadowSmallTiles, sizeof(gBtlShadowSmallTiles));
    work->palette2 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->visible = TRUE;
}

u8 task_hum_hook_bomb_1(HookBombWork* work) {
    if ((gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) == 0) {
        return 0;
    }

    switch (work->state) {
    case HUM_HOOK_BOMB_STATE_BOUNCE:
        work->x += gSineTable[work->angle] * work->speed >> 8;
        work->y += -gSineTable[work->angle + 64] * work->speed >> 8;
        work->z += work->vz;
        work->vz += 64;

        if (work->z > 0) {
            work->z = 0;

            if (work->bounceCount >= work->maxBounces) {
                work->timer = 0;
                work->state = HUM_HOOK_BOMB_STATE_EXPLODE;
                break;
            }

            work->vz = -(GetRandom() % 0x301 + 0x200);

            if (work->variant == HUM_HOOK_BOMB_AIMED) {
                work->angle = GetAngle(work->x, work->y,
                    gBtlWork->targetX, gBtlWork->targetY);
            } else {
                work->angle = GetRandom();
            }

            work->bounceCount++;
        }

        if (TestAttackBox(work->x, work->y, work->z, 2, 2, 2)) {
            work->timer = 0;
            work->state = HUM_HOOK_BOMB_STATE_EXPLODE;
            break;
        }

        work->timer++;
        break;
    default:
        if (work->timer == 0) {
            AnimStart(&work->anim, 1, 0);
        }

        if (work->timer <= 17) {
            work->x += gSineTable[work->angle] * work->speed >> 8;
            work->y += -gSineTable[work->angle + 64] * work->speed >> 8;
            work->z += work->vz;
            work->vz += 64;

            if (work->z > 0) {
                work->z = 0;
                work->vz = -(GetRandom() % 0x301 + 0x200);
                work->angle = GetAngle(work->x, work->y,
                    gBtlWork->targetX, gBtlWork->targetY);
            }
        } else if (work->timer == 18) {
            MakeOpponentsHittable();
            BgFxStartExplosion(work->x, work->y, work->z);
            work->visible = FALSE;
        } else if (work->timer > 18) {
            if (ApplyAttackBox(0x117, work->x, work->y, work->z, 24, 24, 24)) {
                m4aSongNumStart(SONG_BTL_BW_PACHIN);
            }
        }

        if (work->timer > 17 && !BgFxIsActive()) {
            return 0;
        }

        work->timer++;
        break;
    }

    if (ClampBattlePosition(&work->x, &work->y, 0, 0)) {
        work->angle = (u8)(work->angle + 118) + GetRandom() % 21;
    }

    AnimUpdate(&work->anim);
    return 1;
}

void task_hum_hook_bomb_2(HookBombWork* work) {
    void* gfx;
    u16 attr;
    s16 x;
    s16 y;

    if (!work->visible) {
        return;
    }

    gfx = AnimGetGfx(&work->anim);
    attr = GetBattleSpritePriorityFlags(work->y);

    if (!work->facingLeft) {
        attr |= SPRITE_FLAG_HFLIP;
    }

    WorldToScreen(&x, &y, work->x, work->y, work->z);
    DrawSprite(x, y, gfx, work->tiles, work->palette, NULL, attr,
        -0x1004 - ((work->y + 0x800) >> 8) * 4);
    WorldToScreen(&x, &y, work->x, work->y, 0);
    DrawSprite(x, y, gBtlShadowSmallFrame0, work->tiles2, work->palette2, NULL, attr, 0xFFF0);
}

void task_hum_hook_bomb_3(HookBombWork* work) {
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}
