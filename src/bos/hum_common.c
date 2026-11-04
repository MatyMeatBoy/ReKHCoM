/**
 * hum_common.c
 * Humanoid Boss Common Routines
 */

#include "prize_types.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "engine_math.h"
#include "hum_types.h"
#include "m4a_song.h"
#include "obj_api.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "battle.h"
#include "display.h"
#include "field_state.h"
#include "hum_common.h"
#include "macros.h"
#include "obj.h"
#include "romcri_backgrounds.h"
#include "card_battle_riku.h"
#include "sprite_palettes.h"
#include "card_label_data.h"
#include "card.h"
#include "card_ids.h"
#include "btl.h"

BtlWork* gRikuBtlWork EWRAM_COMMON(4);
FieldState* gFieldState EWRAM_COMMON(4);

static const u8 sHumReloadPaletteCycle[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };

void HumInit(HumWork* work, const HumDef* def) {
    BtlObj* actor = &work->actor;

    s32 x = 0x14000;
    s32 y = 0x18100;
    s32 z = 0;

    InitEnemyBtlObj(actor, &def->kind, x, y, z);
    actor->attackOffset = 0;
    actor->attackRangeX = 0;
    actor->attackRangeY = 0;
    actor->cardInterval = 1;
    actor->flags |= BTLOBJ_FLAG_HUM_BOSS;

    if (gBtlWork->actor->x < actor->x) {
        actor->flags |= BTLOBJ_FLAG_FACING_LEFT;
    }

    work->def = def;
    work->tiles = AllocObjTiles(def->tileCount * 32, NULL);
    work->palette = LoadObjPalette(def->palette, 32);
    work->paletteData = def->palette;
    work->stateTimer = 0;
    work->steps = 0;
    work->flags = 0;
    work->vz = 0;
    actor->vx = 0;
    actor->vy = 0;
    work->targetX = 0;
    work->targetY = 0;
    work->targetZ = 0;
    work->boundsMargin = 0xFFF0;
    work->unk_17C = 1;
    AnimInit(&work->anim, NULL, NULL);
    TaskPoolInit(&work->tasks, 3);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, actor);
    TaskCreate(&work->tasks, &gTaskDescBtlBadstatus, actor);
    work->state = HUM_STATE_ENTER;
    work->scaleX = Q_8_8(1);
    work->scaleY = Q_8_8(1);
    work->sub = NULL;
    work->sub2 = NULL;
    work->stockMoves = NULL;
    gRikuBtlWork->actor = actor;
    gBtlWork->actor3 = actor;
    actor->btl = gRikuBtlWork;
    actor->flags |= (BTLOBJ_FLAG_IMMUNE_TERROR | BTLOBJ_FLAG_IMMUNE_CONFUSE);
}

void HumSubInit(HumWork* work, HumSub* sub, const HumSubDef* def) {
    if (work->sub == NULL) {
        work->sub = sub;
    } else {
        work->sub2 = sub;
    }

    sub->tiles = AllocObjTiles(def->tileCount * 32, NULL);
    sub->palette2 = sub->palette = LoadObjPalette(def->palette, 32);
    sub->x = work->actor.x;
    sub->y = work->actor.y;
    sub->z = work->actor.z;
    sub->flags = 0;
    AnimInit(&sub->anim, NULL, NULL);
}

void HumSubReleaseGraphics(HumSub* sub) {
    if (sub != NULL) {
        ReleaseObjTiles(sub->tiles);
        ReleaseObjPalette(sub->palette);
    }
}

void HumReleaseResources(HumWork* work) {
    if (gBtlWork->actor2 == &work->actor) {
        gBtlWork->actor2 = NULL;
    }

    HumSubReleaseGraphics(work->sub);
    HumSubReleaseGraphics(work->sub2);
    gBtlWork->actor3 = NULL;
    ReleaseEnemyBtlObj(&work->actor);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}

void HumStartKnockback(HumWork* work) {
    work->vz = -work->actor.knockbackLift * 3;
    work->actor.vx = ((gSineTable[work->actor.angle] << 1) * work->actor.knockbackSpeed) >> 8;
    work->actor.vy = ((-gSineTable[work->actor.angle + 0x40] << 1) * work->actor.knockbackSpeed) >> 8;
}

s32 HumUpdateReaction(HumWork* work) {
    BtlObj* actor = &work->actor;
    s32 reaction;

    actor->prevX = actor->x;
    actor->prevY = actor->y;
    reaction = UpdateBtlObjReaction(actor);

    switch (reaction) {
    case BTL_REACTION_CARD_ACTION:
        work->flags |= HUM_FLAG_PASS_THROUGH;
        gRikuBtlWork->flags &= ~BTL_FLAG_DISMISS_SUMMONS;
        work->itemIndex = 0;
        work->stateTimer = 0;
        AnimReset(&work->anim);
        break;
    case BTL_REACTION_WARPED:
        FadeStartIn(FADE_MODE_ADD_WHITE, 20);
        gBtlWork->hitStop = 15;

        if (actor->badStatus != BAD_STATUS_STUN) {
            actor->badStatus = BAD_STATUS_STUN;
            actor->badStatusTimer = 0x168;
        }

        work->state = HUM_STATE_STUNNED;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_STUNNED:
        HumStartKnockback(work);
        work->state = HUM_STATE_STUNNED;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_GRAVITY:
    case BTL_REACTION_GRAVITY_DEFEATED:
        work->state = HUM_STATE_GRAVITY;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_HURT:
        HumStartKnockback(work);
        work->state = HUM_STATE_HURT;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_DEFEATED:
        work->flags |= HUM_FLAG_PASS_THROUGH;
        work->state = HUM_STATE_DEFEATED;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_HEALED:
        work->state = HUM_STATE_HEALED;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_CARD_BROKEN:
        work->state = HUM_STATE_CARD_BROKEN;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_STOPPED:
        if (work->state != HUM_STATE_STOPPED) {
            work->state = HUM_STATE_STOPPED;
            work->stateTimer = 0;
            actor->vx = actor->vy = 0;
        }

        break;
    }

    return reaction;
}

void HumSubUpdateAnimation(HumSub* sub) {
    if (sub != NULL) {
        if (!(sub->flags & HUM_SUB_FLAG_HIDDEN)) {
            sub->gfx = AnimUpdate(&sub->anim);
        }
    }
}

s32 HumUpdate(HumWork* work) {
    BtlObj* actor = &work->actor;
    s32 x;

    GetEnemyTargetPosition(actor, &x, NULL, NULL);

    switch (work->state) {
    case HUM_STATE_ENTER:
        if (work->stateTimer > 100) {
            work->state = HUM_STATE_IDLE;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_HURT:
        if (work->stateTimer == 0) {
            AnimReset(&work->anim);
        }

        if (work->stateTimer > 10) {
            ClearBtlObjActionFlags(actor);
            work->state = HUM_STATE_HURT_RECOVER;
            work->stateTimer = 0;

            if (actor->x < x) {
                if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    if (GetRandom() % 3 == 0) {
                        actor->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                    }
                }
            } else {
                if (!(actor->flags & BTLOBJ_FLAG_FACING_LEFT)) {
                    if (GetRandom() % 3 == 0) {
                        actor->flags |= BTLOBJ_FLAG_FACING_LEFT;
                    }
                }
            }

            HumChooseCardAction(work, 3, 64, 64, 32);
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_HURT_RECOVER:
        if (AnimIsFinished(&work->anim)) {
            work->state = HUM_STATE_IDLE;
            work->stateTimer = 0;
        }

        break;
    case HUM_STATE_GRAVITY:
        if (work->stateTimer == 0) {
            AnimReset(&work->anim);
            ColliderSetDisabled(&actor->collider, TRUE);
            actor->flags |= BTLOBJ_FLAG_INTANGIBLE;
            work->anim.frame = 0;
            work->anim.timer = 0;
            work->vz = 0x400;
            actor->vx = 0;
            actor->vy = 0;
            work->steps = 10;
        }

        ApproachValue(&work->scaleY, Q_8_8(0.25), work->steps--);

        if (work->steps > 0) {
            work->stateTimer++;
        } else {
            work->stateTimer = 0;
            work->state = HUM_STATE_GRAVITY_FLAT;
        }

        break;
    case HUM_STATE_GRAVITY_FLAT:
        if (work->stateTimer > 44) {
            if (actor->hp <= 0) {
                work->state = HUM_STATE_DEFEATED;
            } else {
                work->state = HUM_STATE_GRAVITY_RECOVER;
            }

            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_GRAVITY_RECOVER:
        if (work->stateTimer == 0) {
            ColliderSetDisabled(&actor->collider, FALSE);
            work->steps = 10;
        }

        ApproachValueHalfSteps(&work->scaleY, Q_8_8(1), work->steps--);

        if (work->steps <= 0) {
            actor->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
            ClearBtlObjActionFlags(actor);
            work->state = HUM_STATE_IDLE;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_STUNNED:
        if (work->stateTimer == 0) {
            AnimReset(&work->anim);
            work->stateTimer++;
        }

        if (AnimIsFinished(&work->anim)) {
            actor->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
            actor->flags &= ~BTLOBJ_FLAG_HURT;
        }

        if (GetRandom() % 3 == 0) {
            actor->badStatusTimer -= 6;
        }

        if (actor->badStatus != BAD_STATUS_STUN) {
            ClearBtlObjActionFlags(actor);
            work->state = HUM_STATE_IDLE;
            work->stateTimer = 0;
        }

        break;
    case HUM_STATE_CARD_BROKEN:
        if (AnimIsFinished(&work->anim) && work->stateTimer > 60) {
            ClearBtlObjActionFlags(actor);
            work->state = HUM_STATE_IDLE;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_HEALED:
        if (work->stateTimer == 0) {
            work->scaleX = Q_8_8(1);
            work->scaleY = Q_8_8(1);
            actor->vx = 0;
            actor->vy = 0;
        }

        work->vz = 0;

        if (work->stateTimer == 40) {
            CreateBtlPopTask(actor, BTL_POP_RECOVER);
            actor->hp -= actor->damage;

            if (actor->hp > actor->maxHp) {
                actor->hp = actor->maxHp;
            }

            ClearBtlObjActionFlags(actor);
            work->state = HUM_STATE_IDLE;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_USE_ITEM:
        if (work->stateTimer == 23) {
            BgFxStartPotion(actor->x, actor->y, actor->z - ((actor->height - 48) << 8));
        }

        if (work->stateTimer > 23 && !BgFxIsActive()) {
            switch (work->itemIndex) {
            case BTL_ITEM_POTION:
                RequestRikuPotion();
                break;
            case BTL_ITEM_HI_POTION:
                RequestRikuHiPotion();
                break;
            case BTL_ITEM_MEGA_POTION:
                RequestRikuMegaPotion();
                break;
            case BTL_ITEM_ETHER:
                RequestRikuEther();
                break;
            case BTL_ITEM_MEGA_ETHER:
                RequestRikuMegaEther();
                break;
            case BTL_ITEM_ELIXIR:
                RequestRikuElixir();
                break;
            default:
                RequestRikuMegalixir();
                break;
            }

            ClearBtlObjActionFlags(actor);
            work->state = HUM_STATE_IDLE;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_STOPPED:
        if (actor->badStatus != BAD_STATUS_STOP) {
            work->state = HUM_STATE_IDLE;
            ClearBtlObjActionFlags(actor);
        }

        break;
    case HUM_STATE_DEFEATED:
        if (work->stateTimer == 0) {
            BeginBossDefeat(actor);

            if (!(work->flags & HUM_FLAG_BOSS_DEATH)) {
                m4aSongNumStart(SONG_BTL_GF_LOOP);
            }

            SetBattleZoom(1, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
        }

        if (!FadeIsActive()) {
            work->stateTimer = 0;

            if (work->flags & HUM_FLAG_BOSS_DEATH) {
                work->state = HUM_STATE_BOSS_DEATH;
            } else {
                work->state = HUM_STATE_DEFEAT_EFFECT;
            }
        } else {
            BtlMapFollowPosition(actor->x, actor->y, actor->z);
            work->stateTimer++;
        }

        break;
    case HUM_STATE_DEFEAT_EFFECT:
        if (work->stateTimer == 0) {
            BgFxStartHumDefeat(actor->x, actor->y + actor->z - (actor->centerHeight << 8));
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        }

        BtlMapFollowPosition(actor->x, actor->y, actor->z);
        work->vz = 0;

        if (work->stateTimer > 150) {
            work->stateTimer = 0;
            work->state = HUM_STATE_DROP_PRIZES;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_DROP_PRIZES:
        if (work->stateTimer == 0) {
            PrizeCardArg prize;

            FadeStartIn(FADE_MODE_ADD_WHITE, 60);
            FadeLock();
            m4aSongNumStart(SONG_BTL_KU_JUMP);
            gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
            EndBossDefeat();
            DropBossPrizes(actor);
            prize.x = actor->x;
            prize.y = actor->y;
            prize.z = -0x4600;
            CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &prize);
            return 0;
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_BOSS_DEATH:
        if (work->stateTimer == 0) {
            BgFxStartBossDeath(actor->x, actor->y + actor->z - (actor->centerHeight << 8));
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        }

        BtlMapFollowPosition(actor->x, actor->y, actor->z);
        work->vz = 0;

        if (work->stateTimer > 150) {
            work->stateTimer = 0;
            work->state = HUM_STATE_BOSS_DEATH_FLASH;
            BgFxStartBossDeathFlash();
        } else {
            work->stateTimer++;
        }

        break;
    case HUM_STATE_BOSS_DEATH_FLASH:
        BtlMapFollowPosition(actor->x, actor->y, actor->z);

        if (!BgFxIsActive()) {
            PrizeCardArg prize;

            EndBossDefeat();
#ifdef VERSION_EU
            ClampBattlePosition(&actor->x, &actor->y, (s16)(work->boundsMargin - 8), -16);
#endif
            DropBossPrizes(actor);
            prize.x = actor->x;
            prize.y = actor->y;
            prize.z = -0x4600;
            CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &prize);
            return 0;
        }

        work->stateTimer++;
        break;
    case HUM_STATE_IDLE:
        work->flags &= ~HUM_FLAG_PASS_THROUGH;

        if (IsRikuReloadCardSelected()) {
            work->stateTimer = 0;
            work->state = HUM_STATE_RELOAD;
        }

        break;
    case HUM_STATE_RELOAD:
        SetRikuReloadCharging();

        if (!IsRikuReloadCardSelected()) {
            work->stateTimer = 0;
            work->state = HUM_STATE_IDLE;
        } else {
            work->stateTimer++;
        }

        break;
    }

    if (actor->badStatus != BAD_STATUS_STOP) {
        actor->z += work->vz;
        work->vz += gBtlWork->gravity;

        if (actor->z > 0) {
            actor->z = 0;
            work->vz = 0;
        }

        if (actor->collider.colliding && !(work->flags & HUM_FLAG_PASS_THROUGH) && !(actor->collider.other->flags & COLLIDER_FLAG_PASS_THROUGH)) {
            actor->x += actor->collider.pushX >> 1;
            actor->y += actor->collider.pushY >> 1;
        }
    }

    if (actor->vx > 0) {
        actor->x += actor->vx;
        actor->vx -= 17;

        if (actor->vx < 0) {
            actor->vx = 0;
        }
    } else if (actor->vx < 0) {
        actor->x += actor->vx;
        actor->vx += 17;

        if (actor->vx > 0) {
            actor->vx = 0;
        }
    }

    if (actor->vy > 0) {
        actor->y += actor->vy;
        actor->vy -= 17;

        if (actor->vy < 0) {
            actor->vy = 0;
        }
    } else if (actor->vy < 0) {
        actor->y += actor->vy;
        actor->vy += 17;

        if (actor->vy > 0) {
            actor->vy = 0;
        }
    }

    if (!(work->flags & HUM_FLAG_IGNORE_BOUNDS)) {
        switch (ClampBattlePosition(&actor->x, &actor->y, work->boundsMargin, 0)) {
        case BATTLE_EDGE_LEFT:
        case BATTLE_EDGE_RIGHT:
            actor->vx = -(actor->vx >> 1);
            work->flags |= HUM_FLAG_AT_FIELD_EDGE;
            break;
        case BATTLE_EDGE_TOP:
        case BATTLE_EDGE_BOTTOM:
            actor->vy = -(actor->vy >> 1);
            work->flags |= HUM_FLAG_AT_FIELD_EDGE;
            break;
        default:
            work->flags &= ~HUM_FLAG_AT_FIELD_EDGE;
            break;
        }
    }

    if (actor->badStatus != BAD_STATUS_STOP) {
        work->gfx = AnimUpdate(&work->anim);
        HumSubUpdateAnimation(work->sub);
        HumSubUpdateAnimation(work->sub2);
    }

    if (actor->badStatus == BAD_STATUS_BIND) {
        actor->x = actor->prevX;
        actor->y = actor->prevY;
    }

    TaskPoolUpdate(&work->tasks);
    ColliderSetPosition(&actor->collider, actor->x, actor->y, actor->z);
    return 1;
}

void HumDrawSub(HumWork* work, HumSub* sub) {
    s16 x;
    s16 y;
    BtlObj* actor;
    u16 attr;
    ObjAffine* affine;
    u16 prio;
    s32 sx;
    s32 sy;

    if (sub == NULL) {
        return;
    }

    if (sub->flags & HUM_SUB_FLAG_HIDDEN) {
        return;
    }

    actor = &work->actor;
    attr = GetBattleSpritePriorityFlags(sub->y);

    if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        sy = gBtlWork->scale;
        sx = sy;
    } else {
        sy = gBtlWork->scale;

        if (sy == Q_8_8(1)) {
            sx = sy;
            attr |= SPRITE_FLAG_HFLIP;
        } else {
            sx = -sy;
        }
    }

    if (sy == Q_8_8(1) && sx == Q_8_8(1)) {
        affine = NULL;
    } else if (sy <= 0xFF) {
        affine = AllocObjAffine(0, sx, sy, FALSE);
    } else {
        affine = AllocObjAffine(0, sx, sy, TRUE);
    }

    if (sub->flags & HUM_SUB_FLAG_OWN_DEPTH) {
        prio = (-0x1004 - (sub->y >> 8) * 4) | 3;
    } else if (sub->flags & HUM_SUB_FLAG_IN_FRONT) {
        prio = ((-0x1004 - (actor->y >> 8) * 4) | 3) - 1;
    } else {
        prio = ((-0x1004 - (actor->y >> 8) * 4) | 3) + 1;
    }

    WorldToScreen(&x, &y, sub->x, sub->y, sub->z);
    DrawSprite(x, y, sub->gfx, sub->tiles, sub->palette2, affine, attr, prio);
}

void HumDraw(HumWork* work) {
    s16 x;
    s16 y;
    BtlObj* actor = &work->actor;
    u16 attr;
    ObjAffine* affine;
    s32 sx;
    s32 sy;
    s32 scale;
    s16 idx;

    if (work->flags & HUM_FLAG_BEHIND_BG_FX) {
        attr = SPRITE_PRIORITY(2);
    } else {
        attr = GetBattleSpritePriorityFlags(actor->y);
    }

    WorldToScreen(&x, &y, actor->x, actor->y, actor->z);

    if (work->scaleX == Q_8_8(1) && work->scaleY == Q_8_8(1)) {
        if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
            sy = gBtlWork->scale;
            sx = sy;
        } else {
            sy = gBtlWork->scale;

            if (sy == Q_8_8(1)) {
                sx = sy;
                attr |= SPRITE_FLAG_HFLIP;
            } else {
                sx = -sy;
            }
        }
    } else {
        if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
            sx = (gBtlWork->scale * work->scaleX >> 8);
            scale = gBtlWork->scale;
            sy = scale * work->scaleY >> 8;
        } else {
            sx = -(gBtlWork->scale * work->scaleX >> 8);
            scale = gBtlWork->scale;
            sy = scale * work->scaleY >> 8;
        }
    }

    if (sy == Q_8_8(1) && sx == Q_8_8(1)) {
        affine = NULL;
    } else if (sy <= 0xFF) {
        affine = AllocObjAffine(0, sx, sy, FALSE);
    } else {
        affine = AllocObjAffine(0, sx, sy, TRUE);
    }

    if (work->state == HUM_STATE_RELOAD) {
        idx = (work->stateTimer >> 2) % 8;

        if (work->stateTimer & 1) {
            work->flags |= HUM_FLAG_FLASH_PALETTE;
            LoadObjPaletteBank(BIOS_PTR(work->palette)->index, gHumFlashPalettes[sHumReloadPaletteCycle[idx]]);
        } else {
            work->flags &= ~HUM_FLAG_FLASH_PALETTE;
            LoadObjPaletteBank(BIOS_PTR(work->palette)->index, work->paletteData);
        }
    } else if (StepHitFlash(actor)) {
        work->flags |= HUM_FLAG_FLASH_PALETTE;
        LoadObjPaletteBank(BIOS_PTR(work->palette)->index, gHitFlashPalette);
    } else if (work->flags & HUM_FLAG_FLASH_PALETTE) {
        work->flags &= ~HUM_FLAG_FLASH_PALETTE;
        LoadObjPaletteBank(BIOS_PTR(work->palette)->index, work->paletteData);
    }

    DrawSprite(x, y, work->gfx, work->tiles, work->palette, affine, attr, (-0x1004 - (actor->y >> 8) * 4) | 3);
    HumDrawSub(work, work->sub);
    HumDrawSub(work, work->sub2);
    TaskPoolDraw(&work->tasks);
}

void HandleRikuAiCardInput() {
    BtlObj* actor = gRikuBtlWork->actor;
    u8 keys;
    u16 timer;

    if (gRikuBtlWork->flags & BTL_FLAG_RELOAD_CHARGING) {
        return;
    }

    timer = gRikuBtlWork->listSwitchTimer;

    if ((s16)timer > 0) {
        gRikuBtlWork->listSwitchTimer = timer - 1;

        if (gRikuBtlWork->listSwitchTimer == 0) {
            RequestSwitchRikuCardList();
        }

        return;
    }

    keys = gBtlWork->rikuKeys;
    gBtlWork->rikuKeys = 0;

    if (keys & RIKU_KEY_NEXT_CARD) {
        RequestRikuNextCard();
    }

    if (keys & RIKU_KEY_PREV_CARD) {
        RequestRikuPrevCard();
    }

    if (keys & RIKU_KEY_SWITCH_LIST) {
        RequestSwitchRikuCardList();
    }

    if (actor->flags & BTLOBJ_FLAG_CARD_USE_BLOCKED) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_BUSY) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
        return;
    }

    if (actor->flags & BTLOBJ_FLAG_DAMAGE_PENDING) {
        return;
    }

    if (keys & RIKU_KEY_STOCK) {
        if (GetRikuStockCount() > 2) {
            RequestRikuStockUse();
        } else {
            RequestRikuCardStock();
        }
    }

    if (keys & RIKU_KEY_USE_CARD) {
        RequestRikuCardUse();

        if (GetRikuCardListIndex() == CARD_LIST_ENEMY && !IsRikuSelectionEmpty()) {
            gRikuBtlWork->listSwitchTimer = 15;
        }
    }
}

#ifdef VERSION_EU
void HandleRikuTutorialCardInput() {
    BtlObj* actor = gRikuBtlWork->actor;
    u8 keys;
    keys = gBtlWork->rikuKeys;
    gBtlWork->rikuKeys = 0;

    if (actor->flags & BTLOBJ_FLAG_CARD_USE_BLOCKED) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_BUSY) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
        return;
    }

    if (actor->flags & BTLOBJ_FLAG_DAMAGE_PENDING) {
        return;
    }

    if (keys & RIKU_KEY_USE_CARD) {
        RequestRikuCardUse();
    }
}
#endif

void HumFaceTarget(HumWork* work, u16 interval) {
    s32 x;
    GetEnemyTargetPosition(&work->actor, &x, NULL, NULL);

    if (GetRandom() % interval == 0) {
        if (work->actor.x > x) {
            work->actor.flags |= BTLOBJ_FLAG_FACING_LEFT;
        } else {
            work->actor.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        }
    }
}

u8 HumMoveToward(HumWork* work, s32 x, s32 y, s32 spd) {
    u8 ang = GetAngle(work->actor.x, work->actor.y, x, y);

    work->actor.x += gSineTable[ang] * spd >> 8;
    work->actor.y += -gSineTable[ang + 64] * spd >> 8;

    if (work->actor.x - x >= 0 ? work->actor.x - x > 0x1E00 : x - work->actor.x > 0x1E00) {
        return FALSE;
    }

    if (work->actor.y - y >= 0 ? work->actor.y - y > 0x1000 : y - work->actor.y > 0x1000) {
        return FALSE;
    }

    return TRUE;
}

u8 HumIsTargetInReach(HumWork* work, s16 offset, u16 width, u16 depth) {
    s32 x;
    s32 y;
    BtlObj* actor = &work->actor;
    s32 dy;
    s32 centerX;
    s32 maxDx;
    s32 maxDy;

    GetEnemyTargetPosition(actor, &x, &y, NULL);
    maxDy = depth << 8;
    dy = actor->y - y;

    if (dy >= 0 ? dy > maxDy : y - actor->y > maxDy) {
        return FALSE;
    }

    if (actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        centerX = actor->x - (offset << 8);
        maxDx = width << 8;

        if (centerX - maxDx > x) {
            return FALSE;
        }

        if (centerX + maxDx < x) {
            return FALSE;
        }
    } else {
        centerX = actor->x + (offset << 8);
        maxDx = width << 8;

        if (centerX + maxDx < x) {
            return FALSE;
        }

        if (centerX - maxDx > x) {
            return FALSE;
        }
    }

    return TRUE;
}

u8 HumIsNearAreaEdge(HumWork* work, u16 margin) {
    if (work->actor.x < (gBtlWork->xMin + margin) << 8) {
        return TRUE;
    }

    if (work->actor.x > (gBtlWork->xMax - margin) << 8) {
        return TRUE;
    }

    return FALSE;
}

u8 HumIsInPlayerReach(HumWork* work, s16 offset, u16 width, u16 depth) {
    s32 x;
    s32 y;
    BtlObj* actor = &work->actor;
    BtlObj* player = gBtlWork->actor;
    s32 dy;
    s32 centerX;
    s32 maxDx;
    s32 maxDy;

    GetEnemyTargetPosition(actor, &x, &y, NULL);
    maxDy = depth << 8;
    dy = actor->y - y;

    if (dy >= 0 ? dy > maxDy : y - actor->y > maxDy) {
        return FALSE;
    }

    if (player->flags & BTLOBJ_FLAG_FACING_LEFT) {
        centerX = x - (offset << 8);
        maxDx = width << 8;

        if (centerX - maxDx > actor->x) {
            return FALSE;
        }

        if (centerX + maxDx < actor->x) {
            return FALSE;
        }
    } else {
        centerX = x + (offset << 8);
        maxDx = width << 8;

        if (centerX + maxDx < actor->x) {
            return FALSE;
        }

        if (centerX - maxDx > actor->x) {
            return FALSE;
        }
    }

    return TRUE;
}

u8 HumChooseCardAction(HumWork* work, u16 interval, u16 offset, u16 width, u16 depth) {
    u32 value;
    u32 cards;
    u32 id;
    s32 count;
    s32 n;

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
        return FALSE;
    }

    if ((s16)gRikuBtlWork->listSwitchTimer > 0) {
        return FALSE;
    }

    if ((u16)((u32)GetRandom() % interval) != 0) {
        return FALSE;
    }

    value = GetRikuSelectedCardValue();
    cards = GetRikuStockCount();
    id = GetRikuSelectedMove();
    count = GetRikuCardsLeft();

    switch (id) {
    case MOVE_POTION:
    case MOVE_HI_POTION:
    case MOVE_MEGA_POTION:
    case MOVE_ETHER:
    case MOVE_MEGA_ETHER:
    case MOVE_ELIXIR:
    case MOVE_MEGALIXIR:
        n = count >> 1;

        if (n <= 0) {
            n = 1;
        }

        if (GetRandom() % n == 0) {
            gBtlWork->rikuKeys |= RIKU_KEY_USE_CARD;
        } else {
            gBtlWork->rikuKeys |= RIKU_KEY_NEXT_CARD;
        }

        return FALSE;
    }

    if (IsRikuReloadCardSelected()) {
        if (count > 1 && (u16)(GetRandom() % 20U) == 0) {
            gBtlWork->rikuKeys |= RIKU_KEY_NEXT_CARD;
        }

        return FALSE;
    }

    if (GetRikuCardListIndex() == CARD_LIST_ENEMY) {
        if ((GetRandom() & 3) == 0) {
            if (count <= 0) {
                work->flags |= HUM_FLAG_ENEMY_CARDS_SPENT;
                gBtlWork->rikuKeys |= RIKU_KEY_SWITCH_LIST;
            } else {
                gBtlWork->rikuKeys |= RIKU_KEY_USE_CARD;

                if (count == 1) {
                    work->flags |= HUM_FLAG_ENEMY_CARDS_SPENT;
                }
            }
        } else if (count > 1) {
            gBtlWork->rikuKeys |= RIKU_KEY_NEXT_CARD;
        }

        return FALSE;
    }

    if (!(work->flags & HUM_FLAG_ENEMY_CARDS_SPENT) && gRikuBtlWork->hcEffect == HC_EFFECT_NONE
        && (u16)(GetRandom() % 60U) == 0) {
        gBtlWork->rikuKeys |= RIKU_KEY_SWITCH_LIST;
        return FALSE;
    }

    if (cards > 2) {
        if ((u16)(GetRandom() % 6U) == 0) {
            gBtlWork->rikuKeys |= RIKU_KEY_STOCK;
            return TRUE;
        }
    } else if (GetRandom() % 2 == 0) {
        if (count <= 1 && cards != 0) {
            gBtlWork->rikuKeys |= RIKU_KEY_STOCK;
            return TRUE;
        }

        // @bug stockMoves is NULL for every humanoid boss but Lexaeus (NULL read).
        if (value == 0 || BIOS_PTR(work->stockMoves)[cards] != id) {
            gBtlWork->rikuKeys |= RIKU_KEY_NEXT_CARD;
        } else {
            gBtlWork->rikuKeys |= RIKU_KEY_STOCK;
        }

        return FALSE;
    }

    if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
        if (HumIsTargetInReach(work, offset, width, depth)) {
            if (GetActiveCardValue() <= value || value == 0) {
                gBtlWork->rikuKeys |= RIKU_KEY_USE_CARD;
            }
        } else if (GetActiveCardValue() == value) {
            gBtlWork->rikuKeys |= RIKU_KEY_USE_CARD;
        }
    } else if (HumIsTargetInReach(work, offset, width, depth)) {
        gBtlWork->rikuKeys |= RIKU_KEY_USE_CARD;
    }

    return FALSE;
}

s32 HumResolveCardMove(HumWork* work) {
    s32 moves[6];
    s32 id = ResolveActiveCardsMove(moves);

    if (id == MOVE_STOCK_SEQUENCE) {
        if (!(gRikuBtlWork->flags & BTL_FLAG_STOCK_SEQUENCE)) {
            gRikuBtlWork->flags |= BTL_FLAG_STOCK_SEQUENCE;
            gRikuBtlWork->stockMove = 0;
        }

        id = moves[gRikuBtlWork->stockMove];
        gRikuBtlWork->stockMove++;
    }

    work->stateTimer = 0;

    switch (id) {
    case MOVE_POTION:
        work->state = HUM_STATE_USE_ITEM;
        work->itemIndex = BTL_ITEM_POTION;
        break;
    case MOVE_HI_POTION:
        work->state = HUM_STATE_USE_ITEM;
        work->itemIndex = BTL_ITEM_HI_POTION;
        break;
    case MOVE_MEGA_POTION:
        work->state = HUM_STATE_USE_ITEM;
        work->itemIndex = BTL_ITEM_MEGA_POTION;
        break;
    case MOVE_ETHER:
        work->state = HUM_STATE_USE_ITEM;
        work->itemIndex = BTL_ITEM_ETHER;
        break;
    case MOVE_MEGA_ETHER:
        work->state = HUM_STATE_USE_ITEM;
        work->itemIndex = BTL_ITEM_MEGA_ETHER;
        break;
    case MOVE_ELIXIR:
        work->state = HUM_STATE_USE_ITEM;
        work->itemIndex = BTL_ITEM_ELIXIR;
        break;
    case MOVE_MEGALIXIR:
        work->state = HUM_STATE_USE_ITEM;
        work->itemIndex = BTL_ITEM_MEGALIXIR;
        break;
    }

    return id;
}
