/**
 * hum_hades.c
 * Hades Boss
 */

#include "system_state.h"
#include "obj_api.h"
#include "hum.h"
#include "sprites_evt.h"
#include "sprites_hum.h"
#include "hum_common.h"
#include "songs.h"
#include "hum_tasks.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "display.h"
#include "engine_math.h"
#include "hum_types.h"
#include "m4a_song.h"
#include "obj.h"
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "card_label_data.h"
#include "enemy_ids.h"
#include "card_ids.h"

static const u32 sHumHadesStockMoves[3] = {
    MOVE_BOSS_A, MOVE_BOSS_A, MOVE_BOSS_A,
};

static const u32 sHumHadesAngryStockMoves[3] = {
    MOVE_BOSS_B, MOVE_BOSS_A, MOVE_BOSS_B,
};

static const AnimDef sHumHadesAnimDefs[10] = {
    { gHadesFloatFollowFrames, gHadesFloatFollowAnims, gHadesFloatFollowTiles, 0 },
    { gHadesFloatBackFrames, gHadesFloatBackAnims, gHadesFloatBackTiles, 0 },
    { gHadesDamageFrames, gHadesDamageAnims, gHadesDamageTiles, 0 },
    { gHadesFirashotFrames, gHadesFirashotAnims, gHadesFirashotTiles, 0 },
    { gHadesFigaballFrames, gHadesFigaballAnims, gHadesFigaballTiles, 0 },
    { gHadesNailofframeFrames, gHadesNailofframeAnims, gHadesNailofframeTiles, 0 },
    { gHadesAngryFrames, gHadesAngryAnims, gHadesAngryTiles, 0 },
    { gHadesFramespreadFrames, gHadesFramespreadAnims, gHadesFramespreadTiles, 1 },
    { gHadesFramespreadFrames, gHadesFramespreadAnims, gHadesFramespreadTiles, 2 },
    { gHadesFramespreadFrames, gHadesFramespreadAnims, gHadesFramespreadTiles, 3 },
};

static const AnimDef sHumHadesEffectAnimDefs[5] = {
    { gHadesAngryHiFrames, gHadesAngryHiAnims, gHadesAngryHiTiles, 0 },
    { gHadesNailFrameFrames, gHadesNailFrameAnims, gHadesNailFrameTiles, 0 },
    { gHadesFigaballBallFrames, gHadesFigaballBallAnims, gHadesFigaballBallTiles, 0 },
    { gHadesFigaballBallFrames, gHadesFigaballBallAnims, gHadesFigaballBallTiles, 1 },
    { gHadesFirashotFiraFrames, gHadesFirashotFiraAnims, gHadesFirashotFiraTiles, 0 },
};

static const HumSubDef sHumHadesSubDef = { gBStatesPalette, 75 };

static const HumDef sHumHadesDef = {
#ifdef VERSION_EU
        100
#else
        128
#endif
    , gHadesPalette, 0, { ENEMY_HADES, 99, 90, 14, 52, 99, EMY_KIND_FLAG_LARGE_BODY } };

TaskDesc gTaskDescHumHades = {
    "task_hum_hades",
    (TaskInitFunc)task_hum_hades_0,
    (TaskUpdateFunc)task_hum_hades_1,
    (TaskDrawFunc)task_hum_hades_2,
    (TaskDestroyFunc)task_hum_hades_3,
    sizeof(HadesWork),
};

void HadesHover(HumWork* work, s32 hoverZ) {
    BtlObj* act = &work->actor;
    s32 bobZ;

    if (hoverZ != 0) {
        bobZ = hoverZ + gSineTable[gFrameCounter * 4 % 256] * 4;
        work->vz = 0;
        act->z += (bobZ - act->z) >> 4;
    }
}

void HadesEndAttack(HadesWork* work) {
    BtlObj* act = &work->base.actor;

    if (work->angryAttacks > 2) {
        LoadObjPaletteBank(BIOS_PTR(work->base.palette)->index, gHadesPalette);
        work->base.paletteData = gHadesPalette;
        work->flags &= ~HADES_FLAG_ANGRY;
        work->base.stockMoves = sHumHadesStockMoves;
    } else {
        work->angryAttacks++;
    }

    ClearBtlObjActionFlags(act);
}

void task_hum_hades_0(HadesWork* work) {
    HumInit(&work->base, &sHumHadesDef);
    HumSubInit(&work->base, &work->sub, &sHumHadesSubDef);
    work->base.actor.flags |= BTLOBJ_FLAG_ABSORB_FIRE;
    work->base.flags |= HUM_FLAG_BOSS_DEATH;
    work->flags = 0;
    work->hoverZ = -0xA00;
    work->sub.flags |= (HUM_SUB_FLAG_IN_FRONT | HUM_SUB_FLAG_HIDDEN);
    work->tiles = AllocObjTiles(0x80, gHadesFramespreadHiTiles);
    work->tiles2 = AllocObjTiles(0x280, gHadesFramespreadHiTiles);
    work->tiles3 = AllocObjTiles(0x3A0, gHadesFramespreadHiTiles);
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    AnimInit(&work->anim, gHadesFramespreadHiAnims, gHadesFramespreadHiFrames);
    AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
    AnimInit(&work->anim2, gHadesFramespreadHiAnims, gHadesFramespreadHiFrames);
    AnimStart(&work->anim2, 1, ANIM_FLAG_LOOP);
    AnimInit(&work->anim3, gHadesFramespreadHiAnims, gHadesFramespreadHiFrames);
    AnimStart(&work->anim3, 0, ANIM_FLAG_LOOP);
    work->base.stockMoves = sHumHadesStockMoves;
}

enum HumHadesState {
    HUM_HADES_STATE_ENRAGE = 19,
    HUM_HADES_STATE_FIRA_SHOT,
    HUM_HADES_STATE_NAIL_OF_FLAME,
    HUM_HADES_STATE_FIRAGA_BALL_START,
    HUM_HADES_STATE_FIRAGA_BALL,
    HUM_HADES_STATE_FLAME_SPREAD_START,
    HUM_HADES_STATE_FLAME_SPREAD,
    HUM_HADES_STATE_FLAME_SPREAD_END
};

u8 task_hum_hades_1(HadesWork* work) {
    HadesWork* w;
    BtlObj* act;
    s32 x;
    s32 y;
    s32 z;
    s16 farX;
    s16 farZ;
    s16 nearX;
    s16 nearZ;
    u16 frame;
    u8 angle;
    u8 alive;

    w = work;
    act = &work->base.actor;
    GetEnemyTargetPosition(act, &x, &y, &z);

    switch (HumUpdateReaction(&work->base)) {
    case BTL_REACTION_CARD_ACTION:
        work->base.stateTimer = 0;

        switch ((u32)HumResolveCardMove(&work->base)) {
        case MOVE_BOSS_A:
        case MOVE_BOSS_C:
            if (w->flags & HADES_FLAG_ANGRY) {
                work->base.state = HUM_HADES_STATE_NAIL_OF_FLAME;
            } else {
                work->base.state = HUM_HADES_STATE_FIRA_SHOT;
            }

            break;
        case MOVE_BOSS_B:
        case MOVE_BOSS_D:
            if (w->flags & HADES_FLAG_ANGRY) {
                work->base.state = HUM_HADES_STATE_FLAME_SPREAD_START;
            } else {
                work->base.state = HUM_HADES_STATE_FIRA_SHOT;
            }

            break;
        case MOVE_TEMPER_FLARE:
        case MOVE_FIRAGA_BALL:
            if (w->flags & HADES_FLAG_ANGRY) {
                work->base.state = HUM_HADES_STATE_FIRAGA_BALL_START;
            } else {
                work->base.state = HUM_HADES_STATE_ENRAGE;
            }

            break;
        }

        break;
    case BTL_REACTION_CARD_BROKEN:
        work->sub.flags |= HUM_SUB_FLAG_HIDDEN;
        work->flags &= ~HADES_FLAG_FLAMES_ACTIVE;
        break;
    }

    if (w->flags & HADES_FLAG_ANGRY) {
        HumChooseCardAction(&work->base, 3, 40, 40, 24);
    } else {
        HumChooseCardAction(&work->base, 15, 40, 40, 24);
    }

    w->hoverZ = -0xA00;

    switch (work->base.state) {
    case HUM_STATE_ENTER:
    case HUM_STATE_RELOAD:
    case HUM_STATE_USE_ITEM:
        AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 1, ANIM_FLAG_LOOP, w->base.tiles);
        break;
    case HUM_STATE_IDLE:
    case HUM_STATE_MOVE:
        angle = -((u8)work->base.stateTimer * 2);
        work->base.targetX = x + gSineTable[angle] * 90;
        work->base.targetY = y + (-gSineTable[angle + 64]) * 45;

        if (w->flags & HADES_FLAG_ANGRY) {
            HumMoveToward(&work->base, work->base.targetX, work->base.targetY, 0x333);
        } else {
            HumMoveToward(&work->base, work->base.targetX, work->base.targetY, 0x140);
        }

        HumFaceTarget(&work->base, 5);

        if (AnimIsFinished(&work->base.anim)) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                if (work->base.targetX < act->x) {
                    AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 0, ANIM_FLAG_LOOP, w->base.tiles);
                } else {
                    AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 1, ANIM_FLAG_LOOP, w->base.tiles);
                }
            } else {
                if (work->base.targetX < act->x) {
                    AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 1, ANIM_FLAG_LOOP, w->base.tiles);
                } else {
                    AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 0, ANIM_FLAG_LOOP, w->base.tiles);
                }
            }
        }

        work->base.stateTimer++;
        break;
    case HUM_STATE_HURT:
    case HUM_STATE_DEFEATED:
    case HUM_STATE_CARD_BROKEN:
    case HUM_STATE_STUNNED:
    case HUM_STATE_GRAVITY:
        w->hoverZ = 0;
        AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 2, 0, w->base.tiles);
        break;
    case HUM_HADES_STATE_ENRAGE:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 6, 0, w->base.tiles);
            AnimReset(&w->sub.anim);
            AnimChangeWithDef(sHumHadesEffectAnimDefs, &w->base.sub->anim, 0, 0, w->base.sub->tiles);
            w->sub.flags &= ~HUM_SUB_FLAG_HIDDEN;
        }

        w->sub.x = act->x;
        w->sub.y = act->y;
        w->sub.z = act->z;

        if (AnimGetFrame(&w->sub.anim) == 1 && w->sub.anim.timer == 0) {
            m4aSongNumStart(SONG_BTL_HA_IKARI);
        }

        if (AnimGetFrame(&work->base.anim) == 5 && work->base.anim.timer == 0) {
            LoadObjPaletteBank(BIOS_PTR(work->base.palette)->index, gHadesAngryPalette);
            work->base.paletteData = gHadesAngryPalette;
            w->flags |= HADES_FLAG_ANGRY;
            work->base.stockMoves = sHumHadesAngryStockMoves;
            w->angryAttacks = 0;
        }

        if (AnimIsFinished(&work->base.anim)) {
            w->sub.flags |= HUM_SUB_FLAG_HIDDEN;
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HADES_STATE_FIRAGA_BALL_START:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 4, 0, w->base.tiles);
            AnimReset(&w->sub.anim);
            AnimChangeWithDef(sHumHadesEffectAnimDefs, &w->base.sub->anim, 2, 0, w->base.sub->tiles);
            w->sub.flags &= ~HUM_SUB_FLAG_HIDDEN;
            m4aSongNumStart(SONG_VO_HA_ATTACK00);
            m4aSongNumStart(SONG_BTL_HA_FIREENTRY);
        }

        w->sub.x = act->x;
        w->sub.y = act->y;
        w->sub.z = act->z;

        if (AnimIsFinished(&w->sub.anim)) {
            work->base.state = HUM_HADES_STATE_FIRAGA_BALL;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HADES_STATE_FIRAGA_BALL:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesEffectAnimDefs, &w->base.sub->anim, 3, ANIM_FLAG_LOOP, w->base.sub->tiles);

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                w->sub.x -= 0xA00;
            } else {
                w->sub.x += 0xA00;
            }

            w->sub.z -= 0x4E00;
            w->subVz = 0;
            m4aSongNumStart(SONG_BTL_HA_BALLSHOT);
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            w->sub.x -= 0x700;
        } else {
            w->sub.x += 0x700;
        }

        w->sub.y += (y - w->sub.y) >> 4;
        w->sub.z += w->subVz;
        w->subVz += 89;

        if (w->sub.z + 0xF00 > 0) {
            w->sub.z = -0xF00;
            w->subVz = -(w->subVz >> 1);
        }

        if (ApplyAttackBox(0x120, w->sub.x, w->sub.y, w->sub.z, 16, 16, 16)) {
            m4aSongNumStart(SONG_EF_TARU_BOMB);
        }

        if (w->sub.x < (gBtlWork->xMin - 32) << 8 || w->sub.x > (gBtlWork->xMax + 32) << 8) {
            w->sub.flags |= HUM_SUB_FLAG_HIDDEN;
            HadesEndAttack(w);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HADES_STATE_FLAME_SPREAD_START:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 7, 0, w->base.tiles);

#ifdef VERSION_EU
            if (act->btl->hcEffect == HC_EFFECT_BERSERK) {
                act->btl->hcEffectCount--;
            }
#endif
        }

        if (AnimGetFrame(&work->base.anim) == 4) {
            work->base.state = HUM_HADES_STATE_FLAME_SPREAD;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HADES_STATE_FLAME_SPREAD:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 8, ANIM_FLAG_LOOP, w->base.tiles);
            w->scale = 10;
            work->base.steps = 8;
            w->flags = (w->flags & ~HADES_FLAG_FLAMES_ENDING) | HADES_FLAG_FLAMES_ACTIVE;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                w->sub2[0].x = w->sub2[0].x2 = w->sub2[0].x3 = act->x - 0x2800;
                w->sub2[0].y = w->sub2[0].y2 = w->sub2[0].y3 = act->y;
                w->sub2[0].z = w->sub2[0].z2 = w->sub2[0].z3 = act->z - 0x3600;
                w->sub2[1].x = w->sub2[1].x2 = w->sub2[1].x3 = act->x + 0x600;
                w->sub2[1].y = w->sub2[1].y2 = w->sub2[1].y3 = act->y;
                w->sub2[1].z = w->sub2[1].z2 = w->sub2[1].z3 = act->z - 0x3400;
            } else {
                w->sub2[0].x = w->sub2[0].x2 = w->sub2[0].x3 = act->x + 0x2800;
                w->sub2[0].y = w->sub2[0].y2 = w->sub2[0].y3 = act->y;
                w->sub2[0].z = w->sub2[0].z2 = w->sub2[0].z3 = act->z - 0x3600;
                w->sub2[1].x = w->sub2[1].x2 = w->sub2[1].x3 = act->x - 0x600;
                w->sub2[1].y = w->sub2[1].y2 = w->sub2[1].y3 = act->y;
                w->sub2[1].z = w->sub2[1].z2 = w->sub2[1].z3 = act->z - 0x3400;
            }

            m4aSongNumStart(SONG_BTL_LEC_ROCKB1);
        }

        w->hoverZ = 0;

        if (work->base.steps > 0) {
            if (w->flags & HADES_FLAG_FLAMES_ENDING) {
                ApproachValue(&w->scale, 10, work->base.steps);
            } else {
                ApproachValue(&w->scale, Q_8_8(1), work->base.steps);
            }

            work->base.steps--;
        }

        if (work->base.stateTimer == 180) {
            w->flags |= HADES_FLAG_FLAMES_ENDING;
            work->base.steps = 8;
        }

        HumFaceTarget(&work->base, 8);
        act->y += (y - act->y) >> 4;
        act->x += gSineTable[(u8)work->base.stateTimer];
        w->sub2[0].groundY = act->y;
        w->sub2[1].groundY = act->y;

        if (w->flags & HADES_FLAG_FLAMES_ACTIVE) {
            switch (AnimGetGfxIndex(&work->base.anim)) {
            case 2:
                farX = 44;
                farZ = -50;
                nearX = 2;
                nearZ = -44;
                break;
            case 3:
                farX = 44;
                farZ = -39;
                nearX = 33;
                nearZ = -32;
                break;
            case 4:
                farX = 46;
                farZ = -42;
                nearX = 21;
                nearZ = -32;
                break;
            case 5:
                farX = 46;
                farZ = -50;
                nearX = -3;
                nearZ = -30;
                break;
            case 6:
                farX = 48;
                farZ = -43;
                nearX = -2;
                nearZ = -30;
                break;
            case 7:
                farX = 45;
                farZ = -43;
                nearX = 2;
                nearZ = -31;
                break;
            case 8:
                farX = 42;
                farZ = -48;
                nearX = 19;
                nearZ = -34;
                break;
            case 9:
            default:
                farX = 33;
                farZ = -49;
                nearX = 20;
                nearZ = -41;
                break;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                w->sub2[0].x += (act->x - (farX << 8) - w->sub2[0].x) >> 1;
                w->sub2[1].x += (act->x - (nearX << 8) - w->sub2[1].x) >> 1;
            } else {
                w->sub2[0].x += (act->x + (farX << 8) - w->sub2[0].x) >> 1;
                w->sub2[1].x += (act->x + (nearX << 8) - w->sub2[1].x) >> 1;
            }

            w->sub2[0].y += (act->y - w->sub2[0].y) >> 1;
            w->sub2[1].y += (act->y - w->sub2[1].y) >> 1;
            w->sub2[0].z += (act->z + (farZ << 8) - w->sub2[0].z) >> 1;
            w->sub2[1].z += (act->z + (nearZ << 8) - w->sub2[1].z) >> 1;

            {
                s32 scale = w->scale;
                farX = farX + (scale * 14 >> 8);
                farZ = farZ + (scale * 10 >> 8);
                nearX = nearX + (scale * 8 >> 8);
                nearZ = nearZ + (scale * 12 >> 8);
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                w->sub2[0].x2 += (act->x - (farX << 8) - w->sub2[0].x2) >> 3;
                w->sub2[1].x2 += (act->x - (nearX << 8) - w->sub2[1].x2) >> 3;
            } else {
                w->sub2[0].x2 += (act->x + (farX << 8) - w->sub2[0].x2) >> 3;
                w->sub2[1].x2 += (act->x + (nearX << 8) - w->sub2[1].x2) >> 3;
            }

            w->sub2[0].y2 += (act->y - w->sub2[0].y2) >> 3;
            w->sub2[1].y2 += (act->y - w->sub2[1].y2) >> 3;
            w->sub2[0].z2 += (act->z + (farZ << 8) - w->sub2[0].z2) >> 3;
            w->sub2[1].z2 += (act->z + (nearZ << 8) - w->sub2[1].z2) >> 3;

            {
                s32 scale = w->scale;
                farX = farX + (scale * 24 >> 8);
                farZ = farZ + (scale * 20 >> 8);
                nearX = nearX + (scale * 10 >> 8);
                nearZ = nearZ + (scale * 20 >> 8);
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                w->sub2[0].x3 += (act->x - (farX << 8) - w->sub2[0].x3) >> 4;
                w->sub2[1].x3 += (act->x - (nearX << 8) - w->sub2[1].x3) >> 4;
            } else {
                w->sub2[0].x3 += (act->x + (farX << 8) - w->sub2[0].x3) >> 4;
                w->sub2[1].x3 += (act->x + (nearX << 8) - w->sub2[1].x3) >> 4;
            }

            w->sub2[0].y3 += (act->y - w->sub2[0].y3) >> 4;
            w->sub2[1].y3 += (act->y - w->sub2[1].y3) >> 4;
            w->sub2[0].z3 += (act->z + (farZ << 8) - w->sub2[0].z3) >> 4;
            w->sub2[1].z3 += (act->z + (nearZ << 8) - w->sub2[1].z3) >> 4;
        }

#ifndef VERSION_EU
        if (act->btl->hcEffect == HC_EFFECT_BERSERK && act->hp < act->maxHp >> 1) {
            gBtlWork->damageScale = Q_8_8(2);
        }
#endif

        if (w->scale == Q_8_8(1)) {
            if (ApplyAttackBox(0x121, w->sub2[0].x3, w->sub2[0].groundY, w->sub2[0].z3, 16, 16, 16)) {
                m4aSongNumStart(SONG_EF_FIRE01);
            }

            if (ApplyAttackBox(0x121, w->sub2[1].x3, w->sub2[1].groundY, w->sub2[1].z3, 16, 16, 16)) {
                m4aSongNumStart(SONG_EF_FIRE01);
            }
        }

#ifndef VERSION_EU
        gBtlWork->damageScale = 0;
#endif

        if ((w->flags & HADES_FLAG_FLAMES_ENDING) && work->base.steps <= 0) {
            w->flags &= ~HADES_FLAG_FLAMES_ACTIVE;
            work->base.state = HUM_HADES_STATE_FLAME_SPREAD_END;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HADES_STATE_FLAME_SPREAD_END:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 9, 0, w->base.tiles);
        }

        if (AnimIsFinished(&work->base.anim)) {
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HADES_STATE_NAIL_OF_FLAME:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 5, 0, w->base.tiles);
            AnimReset(&w->sub.anim);
            AnimChangeWithDef(sHumHadesEffectAnimDefs, &w->base.sub->anim, 1, 0, w->base.sub->tiles);
            w->sub.flags &= ~HUM_SUB_FLAG_HIDDEN;

#ifdef VERSION_EU
            if (act->btl->hcEffect == HC_EFFECT_BERSERK) {
                act->btl->hcEffectCount--;
            }
#endif
        }

        w->sub.x = act->x;
        w->sub.y = act->y;
        w->sub.z = act->z;

        frame = AnimGetFrame(&w->sub.anim);

        switch (frame) {
        case 2:
        case 3:
        case 4:
#ifndef VERSION_EU
            if (act->btl->hcEffect == HC_EFFECT_BERSERK && act->hp < act->maxHp >> 1) {
                gBtlWork->damageScale = Q_8_8(2);
            }
#endif

            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(0x11F, act->x - 0x1E00, act->y, act->z, 30, 16, 60)
                : ApplyAttackBox(0x11F, act->x + 0x1E00, act->y, act->z, 30, 16, 60)) {
                m4aSongNumStart(SONG_EF_FIRE01);
            }

#ifndef VERSION_EU
            gBtlWork->damageScale = 0;
#endif
            break;
        }

        if (AnimIsFinished(&work->base.anim)) {
            w->sub.flags |= HUM_SUB_FLAG_HIDDEN;
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    case HUM_HADES_STATE_FIRA_SHOT:
        if (work->base.stateTimer == 0) {
            AnimChangeWithDef(sHumHadesAnimDefs, &w->base.anim, 3, 0, w->base.tiles);
            AnimReset(&w->sub.anim);
            AnimChangeWithDef(sHumHadesEffectAnimDefs, &w->base.sub->anim, 4, 0, w->base.sub->tiles);
            w->sub.flags &= ~HUM_SUB_FLAG_HIDDEN;
            w->sub.x = act->x;
            w->sub.y = act->y;
            w->sub.z = act->z;
            m4aSongNumStart(SONG_VO_HA_ATTACK01);

#ifdef VERSION_EU
            if (act->btl->hcEffect == HC_EFFECT_BERSERK) {
                act->btl->hcEffectCount--;
            }
#endif
        }

        switch (AnimGetFrame(&w->sub.anim)) {
        case 1:
            if (w->sub.anim.timer == 0) {
                m4aSongNumStart(SONG_EF_HA_FINGFIRE);
            }

            break;
        case 2:
#ifndef VERSION_EU
            if (act->btl->hcEffect == HC_EFFECT_BERSERK && act->hp < act->maxHp >> 1) {
                gBtlWork->damageScale = Q_8_8(2);
            }
#endif

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                ApplyAttackBox(0x11E, act->x - 0x2000, act->y, act->z, 24, 16, 60);
            } else {
                ApplyAttackBox(0x11E, act->x + 0x2000, act->y, act->z, 24, 16, 60);
            }

#ifndef VERSION_EU
            gBtlWork->damageScale = 0;
#endif
            break;
        }

        if (AnimIsFinished(&work->base.anim)) {
            w->sub.flags |= HUM_SUB_FLAG_HIDDEN;
            ClearBtlObjActionFlags(act);
            work->base.state = HUM_STATE_IDLE;
            work->base.stateTimer = 0;
        } else {
            work->base.stateTimer++;
        }

        break;
    }

    if (!(act->flags & BTLOBJ_FLAG_HURT) && act->badStatus != BAD_STATUS_STOP) {
        HadesHover(&work->base, w->hoverZ);
    }

    if (w->flags & HADES_FLAG_FLAMES_ACTIVE) {
        AnimUpdate(&w->anim);
        AnimUpdate(&w->anim2);
        AnimUpdate(&w->anim3);
    }

    alive = HumUpdate(&work->base);
    return alive;
}

void task_hum_hades_2(HadesWork* work) {
    BtlObj* act;
    HadesSub* flame;
    void* gfx;
    u16 attr;
    s32 sx;
    ObjAffine* affine;
    s16 x;
    s16 y;
    s32 i;

    HumDraw(&work->base);

    if ((work->flags & HADES_FLAG_FLAMES_ACTIVE) == 0) {
        return;
    }

    act = &work->base.actor;

    for (i = 0; i < 2; i++) {
        flame = &work->sub2[i];
        attr = GetBattleSpritePriorityFlags(flame->groundY);

        if (work->scale == Q_8_8(1)) {
            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT) == 0) {
                attr |= SPRITE_FLAG_HFLIP;
            }

            sx = work->scale;
        } else {
            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT) == 0) {
                sx = work->scale;
            } else {
                sx = -work->scale;
            }
        }

        affine = AllocObjAffine(0, sx, work->scale, FALSE);
        gfx = AnimGetGfx(&work->anim);
        WorldToScreen(&x, &y, flame->x, flame->y, flame->z);
        DrawSprite(x, y, gfx, work->tiles, work->palette, affine, attr,
            -0x1005 - (flame->groundY >> 8) * 4);
        gfx = AnimGetGfx(&work->anim2);
        WorldToScreen(&x, &y, flame->x2, flame->y2, flame->z2);
        DrawSprite(x, y, gfx, work->tiles2, work->palette, affine, attr,
            -0x1006 - (flame->groundY >> 8) * 4);
        gfx = AnimGetGfx(&work->anim3);
        WorldToScreen(&x, &y, flame->x3, flame->y3, flame->z3);
        DrawSprite(x, y, gfx, work->tiles3, work->palette, affine, attr,
            -0x1007 - (flame->groundY >> 8) * 4);
    }
}

void task_hum_hades_3(HadesWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjPalette(work->palette);
    HumReleaseResources(&work->base);
}
