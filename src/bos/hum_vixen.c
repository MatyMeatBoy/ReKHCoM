/**
 * hum_vixen.c
 * Vexen Boss
 */

#include "system_state.h"
#include "fade.h"
#include "obj_api.h"
#include "hum.h"
#include "sprites_hum.h"
#include "hum_common.h"
#include "songs.h"
#include <stdlib.h>
#include "hum_tasks.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "engine_math.h"
#include "game_state.h"
#include "hum_types.h"
#include "m4a_song.h"
#include "obj.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "battle_ids.h"
#include "card_label_data.h"
#include "enemy_ids.h"
#include "card_ids.h"

static const u32 sHumVixenStockMovesA[3] = {
    MOVE_BOSS_A, MOVE_BOSS_B, MOVE_BOSS_A,
};

static const u32 sHumVixenStockMovesB[3] = {
    MOVE_BOSS_B, MOVE_BOSS_A, MOVE_BOSS_A,
};

static const u32 sHumVixenStockMovesC[3] = {
    MOVE_BOSS_A, MOVE_BOSS_A, MOVE_BOSS_B,
};

static const u32 sHumVixenStockMovesD[3] = {
    MOVE_BOSS_A, MOVE_BOSS_A, MOVE_BOSS_A,
};

static const AnimDef sHumVixenAnimDefs[15] = {
    { gVixenS1Frames, gVixenS1Anims, gVixenS1Tiles, 0 },
    { gVixenW1Frames, gVixenW1Anims, gVixenW1Tiles, 0 },
    { gVixenD1Frames, gVixenD1Anims, gVixenD1Tiles, 0 },
    { gVixenA1Frames, gVixenA1Anims, gVixenA1Tiles, 0 },
    { gVixenM1bFrames, gVixenM1bAnims, gVixenM1bTiles, 0 },
    { gVixenM2aFrames, gVixenM2aAnims, gVixenM2aTiles, 0 },
    { gVixenM2aFrames, gVixenM2aAnims, gVixenM2aTiles, 2 },
    { gVixenM2aFrames, gVixenM2aAnims, gVixenM2aTiles, 3 },
    { gVixenM4Frames, gVixenM4Anims, gVixenM4Tiles, 0 },
    { gVixenM1aFrames, gVixenM1aAnims, gVixenM1aTiles, 0 },
    { gVixenM1aFrames, gVixenM1aAnims, gVixenM1aTiles, 1 },
    { gVixenM1aFrames, gVixenM1aAnims, gVixenM1aTiles, 2 },
    { gHumVixenCastFrames, gHumVixenCastAnims, gHumVixenCastTiles, 0 },
    { gHumVixenCastFrames, gHumVixenCastAnims, gHumVixenCastTiles, 1 },
    { gHumVixenCastFrames, gHumVixenCastAnims, gHumVixenCastTiles, 2 },
};

static const HumDef sHumVixenDef = { 83, gVixenPalette, 0, { ENEMY_VEXEN, 99, 80, 14, 48, 99, 0 } };

TaskDesc gTaskDescHumVixen = {
    "task_hum_vixen",
    (TaskInitFunc)task_hum_vixen_0,
    (TaskUpdateFunc)task_hum_vixen_1,
    (TaskDrawFunc)task_hum_vixen_2,
    (TaskDestroyFunc)task_hum_vixen_3,
    sizeof(VixenWork),
};

static TaskDesc sTaskDescHumVixenNdl = {
    "task_hum_vixen_ndl",
    (TaskInitFunc)task_hum_vixen_ndl_0,
    (TaskUpdateFunc)task_hum_vixen_ndl_1,
    (TaskDrawFunc)task_hum_vixen_ndl_2,
    (TaskDestroyFunc)task_hum_vixen_ndl_3,
    sizeof(VixenNdlWork),
};

static TaskDesc sTaskDescHumVixenIce = {
    "task_hum_vixen_ice",
    (TaskInitFunc)task_hum_vixen_ice_0,
    (TaskUpdateFunc)task_hum_vixen_ice_1,
    (TaskDrawFunc)task_hum_vixen_ice_2,
    (TaskDestroyFunc)task_hum_vixen_ice_3,
    sizeof(VixenIceWork),
};

static const AnimDef sHumVixenFrzAnimDefs[13] = {
    { gVixenReitouFrames, gVixenReitouAnims, gVixenReitouTiles, 0 },
    { gVixenReitouFrames, gVixenReitouAnims, gVixenReitouTiles, 1 },
    { gReitouSoraFrames, gReitouSoraAnims, gReitouSoraTiles, 0 },
    { gVixenReitouFrames, gVixenReitouAnims, gVixenReitouTiles, 2 },
    { gReitouSoraFrames, gReitouSoraAnims, gReitouSoraTiles, 1 },
    { gVixenReitouFrames, gVixenReitouAnims, gVixenReitouTiles, 3 },
    { gReitouSoraFrames, gReitouSoraAnims, gReitouSoraTiles, 2 },
    { gReitouRikuFrames, gReitouRikuAnims, gReitouRikuTiles, 0 },
    { gReitouRikuFrames, gReitouRikuAnims, gReitouRikuTiles, 1 },
    { gReitouRikuFrames, gReitouRikuAnims, gReitouRikuTiles, 2 },
    { gReitouNiseFrames, gReitouNiseAnims, gReitouNiseTiles, 0 },
    { gReitouNiseFrames, gReitouNiseAnims, gReitouNiseTiles, 1 },
    { gReitouNiseFrames, gReitouNiseAnims, gReitouNiseTiles, 2 },
};

static TaskDesc sTaskDescHumVixenFrz = {
    "task_hum_vixen_frz",
    (TaskInitFunc)task_hum_vixen_frz_0,
    (TaskUpdateFunc)task_hum_vixen_frz_1,
    (TaskDrawFunc)task_hum_vixen_frz_2,
    (TaskDestroyFunc)task_hum_vixen_frz_3,
    sizeof(VixenFrzWork),
};

static const VixenFrgDef sVixenFrgDefs[15] = {
    { 12, -29, 3, 0 },
    { 5, -37, 0, 0 },
    { -16, -24, 3, SPRITE_FLAG_VFLIP },
    { -18, -6, 0, SPRITE_FLAG_HFLIP },
    { 14, -4, 5, 0 },
    { 17, -5, 4, 0 },
    { -12, -44, 5, SPRITE_FLAG_HFLIP },
    { -16, -40, 2, 0 },
    { 8, 0, 1, 0 },
    { 0, -13, 2, SPRITE_FLAG_VFLIP },
    { -4, -24, 5, 0 },
    { 8, -12, 5, 0 },
    { 4, -48, 5, SPRITE_FLAG_HFLIP | SPRITE_FLAG_VFLIP },
    { -7, -39, 4, SPRITE_FLAG_HFLIP },
    { 3, -54, 1, SPRITE_FLAG_HFLIP },
};

static TaskDesc sTaskDescHumVixenFrg = {
    "task_hum_vixen_frg",
    (TaskInitFunc)task_hum_vixen_frg_0,
    (TaskUpdateFunc)task_hum_vixen_frg_1,
    (TaskDrawFunc)task_hum_vixen_frg_2,
    (TaskDestroyFunc)task_hum_vixen_frg_3,
    sizeof(VixenFrgWork),
};

void VixenPlaceGroundIce(VixenWork* work) {
    VixenSub* ice;
    s32 i;

    m4aSongNumStart(SONG_BTL_VIC_GROUNDICE);
    ice = work->sub;

    for (i = 0; i < 3; i++) {
        ice[i].pending = ice[i].active = TRUE;
        ice[i].x = (gBtlWork->xMin + 32 +
            GetRandom() % (gBtlWork->xMax - gBtlWork->xMin - 0x3F)) << 8;
        ice[i].y = (gBtlWork->yMin + 16 +
            GetRandom() % (gBtlWork->yMax - gBtlWork->yMin - 0x1F)) << 8;
    }
}

void VixenCreateIceTasks(VixenWork* work) {
    VixenSub* ice;
    s32 i;
    u8 zero;

    zero = 0;
    ice = work->sub;

    for (i = 0; i < 3; i++) {
        ice->active = zero;
        ice->pending = zero;
        TaskCreate(&work->tasks, &sTaskDescHumVixenIce, &work->sub[i]);
        ice++;
    }
}

void VixenHover(HumWork* work, s32 hoverZ) {
    BtlObj* act;
    s32 bobZ;

    if (hoverZ != 0) {
        act = &work->actor;
        bobZ = hoverZ + gSineTable[gFrameCounter * 4 % 256] * 3;
        work->vz = 0;
        act->z += (bobZ - act->z) >> 4;
    }
}

void task_hum_vixen_0(VixenWork* work) {
    HumInit(&work->base, &sHumVixenDef);
    work->hoverZ = 0;
    work->base.actor.flags |= BTLOBJ_FLAG_IMMUNE_BLIZZARD;
    work->flags = 0;
    TaskPoolInit(&work->tasks, 15);
    VixenCreateIceTasks(work);
    work->base.stockMoves = sHumVixenStockMovesA;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        gBtlWork->tiles2 = AllocObjTiles(0x840, NULL);
    }
}

enum HumVixenState {
    HUM_VIXEN_STATE_LUNGE = 21,
    HUM_VIXEN_STATE_BLIZZARD,
    HUM_VIXEN_STATE_NEEDLE_WINDUP,
    HUM_VIXEN_STATE_NEEDLE_CHARGE,
    HUM_VIXEN_STATE_NEEDLE_RELEASE,
    HUM_VIXEN_STATE_NEEDLE_TRAIL,
    HUM_VIXEN_STATE_NEEDLE_END,
    HUM_VIXEN_STATE_ICE_FALL,
    HUM_VIXEN_STATE_GROUND_ICE_WINDUP,
    HUM_VIXEN_STATE_GROUND_ICE_CHARGE,
    HUM_VIXEN_STATE_GROUND_ICE_RELEASE,
    HUM_VIXEN_STATE_GROUND_ICE,
    HUM_VIXEN_STATE_FREEZE_WINDUP,
    HUM_VIXEN_STATE_FREEZE_CHARGE,
    HUM_VIXEN_STATE_FREEZE_RELEASE,
    HUM_VIXEN_STATE_FREEZE,
    HUM_VIXEN_STATE_REVIVE
};

u8 task_hum_vixen_1(VixenWork* work) {
    VixenWork* w;
    BtlObj* act;
    VixenNdlArgs args;
    s32 x;
    s32 y;
    s32 z;
    s32 speed;
    u8 ang;
    s32 dx;
    s32 zero;
    s32 playerX;
    s32 bossX;
    u16 hp;
    u8 alive;

    w = work;
    act = &work->base.actor;
    GetEnemyTargetPosition(act, &x, &y, &z);

    switch ((u32)HumUpdateReaction(&work->base)) {
    case BTL_REACTION_CARD_ACTION:
        work->base.stateTimer = 0;

        switch ((u32)HumResolveCardMove(&work->base)) {
        case MOVE_BOSS_A:
        case MOVE_BOSS_C:
            w->base.state = HUM_VIXEN_STATE_BLIZZARD;
            break;
        case MOVE_BOSS_B:
        case MOVE_BOSS_D:
            w->base.state = HUM_VIXEN_STATE_LUNGE;
            break;
        case MOVE_FREEZE:
            work->base.state = HUM_VIXEN_STATE_FREEZE_WINDUP;
            break;
        case MOVE_DIAMOND_DUST_SLEIGHT:
            work->base.state = HUM_VIXEN_STATE_ICE_FALL;
            break;
        case MOVE_ICEBURN:
            work->base.state = HUM_VIXEN_STATE_GROUND_ICE_WINDUP;
            break;
        case MOVE_ICE_NEEDLES:
            work->base.state = HUM_VIXEN_STATE_NEEDLE_WINDUP;
            break;
        }

        break;
    case BTL_REACTION_CARD_BROKEN:
        m4aSongNumStop(SONG_BTL_VIC_ICEFALL);
        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        if (act->btl->hcEffect == HC_EFFECT_AUTO_LIFE) {
            w->base.state = HUM_VIXEN_STATE_REVIVE;
            w->base.stateTimer = 0;
        }

        break;
    }

    switch (gBtlWork->battleId) {
    case BATTLE_VEXEN_1:
        if (HumChooseCardAction(&w->base, 30, 40, 40, 24)) {
            w->base.stockMoves = sHumVixenStockMovesA;
        }

        break;
    case BATTLE_VEXEN_2:
        if (HumChooseCardAction(&w->base, 5, 40, 40, 24)) {
            switch (GetRandom() % 3) {
            case 0:
                w->base.stockMoves = sHumVixenStockMovesA;
                break;
            case 1:
                w->base.stockMoves = sHumVixenStockMovesC;
                break;
            case 2:
                w->base.stockMoves = sHumVixenStockMovesD;
                break;
            }
        }

        break;
    case BATTLE_VEXEN_3:
    default:
        if (HumChooseCardAction(&w->base, 30, 40, 40, 24)) {
            switch (GetRandom() % 3) {
            case 0:
                w->base.stockMoves = sHumVixenStockMovesA;
                break;
            case 1:
                w->base.stockMoves = sHumVixenStockMovesB;
                break;
            case 2:
                w->base.stockMoves = sHumVixenStockMovesD;
                break;
            }
        }

        break;
    }

    switch (w->base.state) {
    case HUM_STATE_ENTER:
    case HUM_STATE_USE_ITEM:
        AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 0, ANIM_FLAG_LOOP, work->base.tiles);
        break;
    case HUM_STATE_RELOAD:
        AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 0, ANIM_FLAG_LOOP, work->base.tiles);
        work->hoverZ = -0x4000;
        HumFaceTarget(&w->base, 20);
        break;
    case HUM_STATE_IDLE:
        AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 0, ANIM_FLAG_LOOP, work->base.tiles);
        work->hoverZ = 0;

        if (func_08081828() == 0) {
            HumFaceTarget(&w->base, 80);

            if (AnimIsFinished(&w->base.anim) && GetRandom() % 80 == 0) {
                w->base.state = HUM_STATE_MOVE;
                w->base.stateTimer = 0;
            } else {
                w->base.stateTimer++;
            }
        }

        break;
    case HUM_STATE_MOVE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 1, ANIM_FLAG_LOOP, work->base.tiles);
            work->hoverZ = -0xF00;

            if (act->x <= 0xFFFF) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                w->base.targetX = (gBtlWork->xMax - 48) << 8;
            } else {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                w->base.targetX = (gBtlWork->xMin + 48) << 8;
            }

            work->slideSpeed = 0;
        }

        work->slideSpeed += 17;

        if (work->slideSpeed > 0x199) {
            work->slideSpeed = 0x199;
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            act->x -= work->slideSpeed;
        } else {
            act->x += work->slideSpeed;
        }

        if (w->base.flags & HUM_FLAG_AT_FIELD_EDGE) {
            work->flags ^= VIXEN_FLAG_SLIDE_DOWN;
        }

        if (work->flags & VIXEN_FLAG_SLIDE_DOWN) {
            act->y += work->slideSpeed;
        } else {
            act->y -= work->slideSpeed;
        }

        dx = act->x - w->base.targetX;

        if ((dx >= 0) ? dx <= 0xBFF : w->base.targetX - act->x <= 0xBFF) {
            w->base.state = HUM_STATE_IDLE;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_STATE_HURT:
    case HUM_STATE_DEFEATED:
    case HUM_STATE_CARD_BROKEN:
    case HUM_STATE_STUNNED:
    case HUM_STATE_GRAVITY:
        AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 2, 0, work->base.tiles);
        break;
    case HUM_VIXEN_STATE_LUNGE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 3, 0, work->base.tiles);
            work->hoverZ = 0;
        }

        if (AnimGetFrame(&w->base.anim) == 3) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 backX = act->x + 0x3200;
                act->x += (act->originX - backX) >> 2;
            } else {
                s32 backX = act->x - 0x3200;
                act->x += (act->originX - backX) >> 2;
            }

            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(312, act->x - 0x2000, act->y, act->z, 12, 12, 48)
                : ApplyAttackBox(312, act->x + 0x2000, act->y, act->z, 12, 12, 48)) {
                m4aSongNumStart(SONG_BTL_VIC_SWORDHIT);
            }
        }

        if (AnimIsFinished(&w->base.anim)) {
            ClearBtlObjActionFlags(act);
            w->base.state = HUM_STATE_IDLE;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_REVIVE:
        if (w->base.stateTimer == 0) {
            AnimReset(&w->base.anim);
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 2, 0, work->base.tiles);
        }

        if (AnimIsFinished(&w->base.anim)) {
            w->base.flags &= ~HUM_FLAG_PASS_THROUGH;
            act->btl->hcEffectCount--;
            zero = 0;
            act->hp = act->maxHp / 4;
            act->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
            ClearBtlObjActionFlags(act);
            CreateBtlPopTask(act, BTL_POP_RECOVER);
            w->base.state = zero;
            w->base.stateTimer = zero;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_BLIZZARD:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 4, 0, work->base.tiles);
            work->hoverZ = 0;
        }

        if (AnimGetFrame(&w->base.anim) == 3 && w->base.anim.timer == 0) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartBlizzard(SPELL_TIER_RA, act->x - 0x3700, act->y, act->z - 0x4000,
                    act->x - 0x6E00, act->y, -0x1400, TRUE, 0x139);
            } else {
                BgFxStartBlizzard(SPELL_TIER_RA, act->x + 0x3700, act->y, act->z - 0x4000,
                    act->x + 0x6E00, act->y, -0x1400, FALSE, 0x139);
            }
        }

        if (AnimIsFinished(&w->base.anim) && !BgFxIsActive()) {
            ClearBtlObjActionFlags(act);
            w->base.state = HUM_STATE_IDLE;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_NEEDLE_WINDUP:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 5, 0, work->base.tiles);
            work->hoverZ = 0;
            work->needleCount = 0;
        }

        if (AnimIsFinished(&w->base.anim)) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_NEEDLE_CHARGE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_NEEDLE_CHARGE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 6, ANIM_FLAG_LOOP, work->base.tiles);
        }

        if (w->base.stateTimer > 60) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_NEEDLE_RELEASE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_NEEDLE_RELEASE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 7, 0, work->base.tiles);
        }

        if (AnimIsFinished(&w->base.anim)) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_NEEDLE_TRAIL;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_NEEDLE_TRAIL:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 4, 0, work->base.tiles);

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->needleX = act->x - 0x2000;
                work->angle = 192;
            } else {
                work->needleX = act->x + 0x2000;
                work->angle = 64;
            }

            work->needleY = act->y;
            m4aSongNumStart(SONG_VO_VIC_ATTACK01);
            InitObjTilesAtSlot(&work->needleTiles, ((ObjTiles*)gBtlWork->tiles2)->index, gVixenE1Tiles, sizeof(gVixenE1Tiles));
        }

        if (AnimGetFrame(&w->base.anim) > 2) {
            ang = GetAngle(work->needleX, work->needleY, x, y);
            ApproachAngle(&work->angle, ang, 3);
            speed = abs(SIN((u16)w->base.stateTimer * 2));
            speed += 384;
            work->needleX += (gSineTable[(u8)work->angle] * speed) >> 8;
            work->needleY += (-gSineTable[(u8)work->angle + 64] * speed) >> 8;
            ClampBattlePosition(&work->needleX, &work->needleY, 0, 0);
            HumFaceTarget(&w->base, 1);

            if (w->base.stateTimer % 9 == 0) {
                args.x = work->needleX;
                args.y = work->needleY;
                args.z = 0;
                args.facingLeft = work->needleCount % 8;
                args.tiles = &work->needleTiles;
                work->needleCount++;
                TaskCreate(&work->tasks, &sTaskDescHumVixenNdl, &args);
            }
        }

        if (w->base.stateTimer > 360 || (gBtlWork->actor->flags & BTLOBJ_FLAG_HURT)) {
            w->base.state = HUM_VIXEN_STATE_NEEDLE_END;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_NEEDLE_END:
        if (w->base.stateTimer > 70) {
            ClearBtlObjActionFlags(act);
            w->base.state = HUM_STATE_IDLE;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_ICE_FALL:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 8, 0, work->base.tiles);
            work->hoverZ = 0;
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        }

        if (AnimGetFrame(&w->base.anim) > 4 && !BgFxIsActive()) {
            m4aSongNumStart(SONG_VO_VIC_ATTACK02);
            m4aSongNumStart(SONG_BTL_VIC_ICEFALL);
            BgFxStartVixenIceFall(9999);
        }

        if (w->base.stateTimer % 15 == 0) {
            hp = gBtlWork->actor->hp;

            if ((s16)hp > 1) {
                gBtlWork->actor->hp = hp - 1;
            }
        }

        if (w->base.stateTimer > 300 ||
            (w->base.stateTimer > 120 && gBtlWork->actor->hp <= 1)) {
            m4aSongNumStop(SONG_BTL_VIC_ICEFALL);
            FadeToOriginal(FADE_MODE_BLACK, 8);
            gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
            ClearBtlObjActionFlags(act);
            w->base.state = HUM_STATE_IDLE;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_GROUND_ICE_WINDUP:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 9, 0, work->base.tiles);
            work->hoverZ = 0;
        }

        if (AnimIsFinished(&w->base.anim)) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_GROUND_ICE_CHARGE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_GROUND_ICE_CHARGE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 10, ANIM_FLAG_LOOP, work->base.tiles);
        }

        if (w->base.stateTimer > 60) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_GROUND_ICE_RELEASE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_GROUND_ICE_RELEASE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 11, 0, work->base.tiles);
        }

        if (AnimIsFinished(&w->base.anim)) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_GROUND_ICE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_GROUND_ICE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 4, 0, work->base.tiles);
        }

        if (AnimGetFrame(&w->base.anim) == 3 && w->base.anim.timer == 0) {
            FadeStartIn(FADE_MODE_WHITE, 60);
            VixenPlaceGroundIce(work);
        }

        if (AnimIsFinished(&w->base.anim) && !FadeIsActive()) {
            ClearBtlObjActionFlags(act);
            w->base.state = HUM_STATE_IDLE;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_FREEZE_WINDUP:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 12, 0, work->base.tiles);
            work->hoverZ = 0;
        }

        if (AnimIsFinished(&w->base.anim)) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_FREEZE_CHARGE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_FREEZE_CHARGE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 13, ANIM_FLAG_LOOP, work->base.tiles);
        }

        if (w->base.stateTimer > 60) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_FREEZE_RELEASE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_FREEZE_RELEASE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 14, 0, work->base.tiles);
            m4aSongNumStart(SONG_VO_VIC_ATTACK00);
        }

        if (AnimIsFinished(&w->base.anim)) {
            w->base.stateTimer = 0;
            w->base.state = HUM_VIXEN_STATE_FREEZE;
        } else {
            w->base.stateTimer++;
        }

        break;
    case HUM_VIXEN_STATE_FREEZE:
        if (w->base.stateTimer == 0) {
            AnimChangeWithDef(sHumVixenAnimDefs, &work->base.anim, 4, 0, work->base.tiles);
            work->task = NULL;
        }

        if (AnimGetFrame(&w->base.anim) == 3 && w->base.anim.timer == 0) {
            args.x = x;
            args.y = y;
            args.z = 0;
            work->task = TaskCreate(&work->tasks, &sTaskDescHumVixenFrz, &args);
        }

        if (AnimIsFinished(&w->base.anim) &&
            !IsTaskActiveNamed(work->task, sTaskDescHumVixenFrz.name)) {
            ClearBtlObjActionFlags(act);
            w->base.state = HUM_STATE_IDLE;
            w->base.stateTimer = 0;
        } else {
            w->base.stateTimer++;
        }

        break;
    }

    if (!(act->flags & BTLOBJ_FLAG_HURT) && act->badStatus != BAD_STATUS_STOP) {
        VixenHover(&w->base, work->hoverZ);
    }

    alive = HumUpdate(&w->base);
    playerX = gBtlWork->actor->x;
    bossX = act->x;

    if ((playerX < bossX && (act->flags & BTLOBJ_FLAG_FACING_LEFT)) || (playerX > bossX && !(act->flags & BTLOBJ_FLAG_FACING_LEFT))) {
        act->flags |= BTLOBJ_FLAG_GUARD_PHYSICAL;
    } else {
        act->flags &= ~BTLOBJ_FLAG_GUARD_PHYSICAL;
    }

    TaskPoolUpdate(&work->tasks);
    return alive;
}

void task_hum_vixen_2(VixenWork* work) {
    HumDraw(&work->base);
    TaskPoolDraw(&work->tasks);
}

void task_hum_vixen_3(VixenWork* work) {
    if (gGameState.flags & GAME_FLAG_RIKU) {
        ReleaseObjTiles(gBtlWork->tiles2);
    }

    HumReleaseResources(&work->base);
    TaskPoolDestroy(&work->tasks);
}

void task_hum_vixen_ndl_0(VixenNdlWork* work, VixenNdlArgs* args) {
    work->palette = LoadObjPalette(gVixEPalette, sizeof(gVixEPalette));
    work->tiles = args->tiles;
    AnimInit(&work->anim, gVixenE1Anims, gVixenE1Frames);
    AnimStart(&work->anim, 0, 0);
    work->hitPhase = args->facingLeft;
    work->x = args->x;
    work->y = args->y + (GetRandom() % 11 - 5) * 256;
    work->z = args->z;
    work->hitDone = FALSE;
    m4aSongNumStart(SONG_BTL_VIC_ICEP);

    if ((GetRandom() & 1) != 0) {
        work->flipped = TRUE;
    } else {
        work->flipped = FALSE;
    }
}

u8 task_hum_vixen_ndl_1(VixenNdlWork* work) {
    if ((gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) == 0) {
        return 0;
    }

    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    switch (AnimGetFrame(&work->anim)) {
    case 1:
    case 2:
        if (!work->hitDone) {
            if (gFrameCounter % 8 == work->hitPhase) {
                ApplyAttackBox(0x13A, work->x, work->y, 0, 4, 4, 16);
            }
        }

        break;
    }

    if (gBtlWork->actor->flags & BTLOBJ_FLAG_HURT) {
        work->hitDone = TRUE;
    }

    AnimUpdate(&work->anim);
    return 1;
}

void task_hum_vixen_ndl_2(VixenNdlWork* work) {
    s16 x;
    s16 y;
    void* gfx;
    u16 attr;

    gfx = AnimGetGfx(&work->anim);
    attr = GetBattleSpritePriorityFlags(work->y);

    if (work->flipped) {
        attr |= SPRITE_FLAG_HFLIP;
    }

    WorldToScreen(&x, &y, work->x, work->y, work->z);
    DrawSprite(x, y, gfx, work->tiles, work->palette, NULL, attr,
        -0x1004 - (work->y >> 8) * 4);
}

void task_hum_vixen_ndl_3(VixenNdlWork* work) {
    ReleaseObjPalette(work->palette);
}

enum HumVixenIceState {
    HUM_VIXEN_ICE_STATE_GROW,
    HUM_VIXEN_ICE_STATE_IDLE,
    HUM_VIXEN_ICE_STATE_GLINT,
    HUM_VIXEN_ICE_STATE_INACTIVE
};

void task_hum_vixen_ice_0(VixenIceWork* work, VixenSub* args) {
    work->palette = LoadObjPalette(gVixEPalette, sizeof(gVixEPalette));
    work->tiles = LoadObjTiles(gVixenE2Tiles, sizeof(gVixenE2Tiles));
    work->sub = args;
    work->state = HUM_VIXEN_ICE_STATE_INACTIVE;
    AnimInit(&work->anim, gVixenE2Anims, gVixenE2Frames);
    AnimStart(&work->anim, 0, 0);
    ColliderInit(&work->collider, COLLIDER_TYPE_SLIPPERY, 27, 1);
    ColliderSetDisabled(&work->collider, TRUE);
}

u8 task_hum_vixen_ice_1(VixenIceWork* work) {
    if (!work->sub->active) {
        if (work->sub->pending) {
            FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
            work->sub->pending = FALSE;
            ColliderSetDisabled(&work->collider, TRUE);
        }

        return 1;
    }

    if (work->sub->pending) {
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, FALSE);
        work->sub->pending = FALSE;
        work->state = HUM_VIXEN_ICE_STATE_GROW;
        work->stateTimer = 0;
        work->scale = 10;

        switch (GetRandom() % 3) {
        case 0:
            work->targetScale = Q_8_8(1);
            break;
        case 1:
            work->targetScale = Q_8_8(0.75);
            break;
        case 2:
            work->targetScale = Q_8_8(0.5);
            break;
        }
    }

    switch (work->state) {
    case HUM_VIXEN_ICE_STATE_GROW:
        if (work->stateTimer == 0) {
            work->steps = 30;
            work->stateTimer++;
        }

        ApproachValue(&work->scale, work->targetScale, work->steps);
        work->steps--;

        if ((s16)work->steps <= 0) {
            ColliderSetDisabled(&work->collider, FALSE);
            work->state = HUM_VIXEN_ICE_STATE_IDLE;
            work->stateTimer = 0;
            work->lifetime = GetRandom() % 0x259 + 600;
        }

        break;
    case HUM_VIXEN_ICE_STATE_IDLE:
        if (work->stateTimer == 0) {
            AnimStart(&work->anim, 0, 0);
            work->stateTimer++;
        }

        if (GetRandom() % 300 == 0) {
            work->state = HUM_VIXEN_ICE_STATE_GLINT;
            work->stateTimer = 0;
        }

        break;
    case HUM_VIXEN_ICE_STATE_GLINT:
        if (work->stateTimer == 0) {
            AnimStart(&work->anim, 1, 0);
            work->stateTimer++;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = HUM_VIXEN_ICE_STATE_IDLE;
            work->stateTimer = 0;
        }

        break;
    }

    switch (work->state) {
    case HUM_VIXEN_ICE_STATE_IDLE:
    case HUM_VIXEN_ICE_STATE_GLINT:
        ApproachValue(&work->scale, 10, work->lifetime);
        work->lifetime--;

        if ((s16)work->lifetime <= 0) {
            work->sub->active = FALSE;
            work->sub->pending = TRUE;
        }

        ColliderSetRadius(&work->collider, work->scale * 27 >> 8);
        ColliderSetPosition(&work->collider, work->sub->x, work->sub->y, 0);
        break;
    }

    AnimUpdate(&work->anim);
    return 1;
}

void task_hum_vixen_ice_2(VixenIceWork* work) {
    s16 x;
    s16 y;
    void* gfx;
    s32 scale;
    ObjAffine* affine;

    if (work->sub->active) {
        gfx = AnimGetGfx(&work->anim);
        WorldToScreen(&x, &y, work->sub->x, work->sub->y, 0);
        scale = work->scale * gBtlWork->scale >> 8;

        if (gBtlWork->rotation != 0 || scale > Q_8_8(1)) {
            affine = AllocObjAffine(gBtlWork->rotation, scale, scale, TRUE);
        } else {
            affine = AllocObjAffine(gBtlWork->rotation, scale, scale, FALSE);
        }

        DrawSprite(x, y, gfx, work->tiles, work->palette, affine, SPRITE_PRIORITY(2), 0xFFFF);
    }
}

void task_hum_vixen_ice_3(VixenIceWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

enum HumVixenFrzVariant {
    HUM_VIXEN_FRZ_SORA,
    HUM_VIXEN_FRZ_RIKU,
    HUM_VIXEN_FRZ_DARK_RIKU
};

enum HumVixenFrzState {
    HUM_VIXEN_FRZ_STATE_CAST,
    HUM_VIXEN_FRZ_STATE_ENCASE,
    HUM_VIXEN_FRZ_STATE_FROZEN,
    HUM_VIXEN_FRZ_STATE_SHATTER,
    HUM_VIXEN_FRZ_STATE_MISS,
    HUM_VIXEN_FRZ_STATE_MISS_SHATTER,
    HUM_VIXEN_FRZ_STATE_DONE
};

void task_hum_vixen_frz_0(VixenFrzWork* work, VixenNdlArgs* args) {
    work->palette = LoadObjPalette(gVixEPalette, sizeof(gVixEPalette));
    work->tiles = gBtlWork->tiles2;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
            work->variant = HUM_VIXEN_FRZ_DARK_RIKU;
        } else {
            work->variant = HUM_VIXEN_FRZ_RIKU;
        }
    } else {
        work->variant = HUM_VIXEN_FRZ_SORA;
    }

    AnimInit(&work->anim, NULL, NULL);
    AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 0, 0, work->tiles);
    work->state = HUM_VIXEN_FRZ_STATE_CAST;

    if (gBtlWork->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
        work->flipped = 0;
    } else {
        work->flipped = SPRITE_FLAG_HFLIP;
    }

    m4aSongNumStart(SONG_EF_BURIZA02);
    work->x = args->x;
    work->y = args->y;
    work->z = args->z;
}

u8 task_hum_vixen_frz_1(VixenFrzWork* work) {
    VixenNdlArgs args;
    VixenNdlArgs missArgs;

    if ((gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) == 0) {
        return 0;
    }

    switch (work->state) {
    case HUM_VIXEN_FRZ_STATE_ENCASE:
        if (work->timer == 0) {
            switch (work->variant) {
            case HUM_VIXEN_FRZ_SORA:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 2, 0, work->tiles);
                break;
            case HUM_VIXEN_FRZ_RIKU:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 7, 0, work->tiles);
                break;
            case HUM_VIXEN_FRZ_DARK_RIKU:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 10, 0, work->tiles);
                break;
            }
        }

        work->x = gBtlWork->actor->x;
        work->y = gBtlWork->actor->y;
        work->z = gBtlWork->actor->z;

        if (AnimIsFinished(&work->anim)) {
            work->state = HUM_VIXEN_FRZ_STATE_FROZEN;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case HUM_VIXEN_FRZ_STATE_FROZEN:
        if (work->timer == 0) {
            switch (work->variant) {
            case HUM_VIXEN_FRZ_SORA:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 4, 0, work->tiles);
                break;
            case HUM_VIXEN_FRZ_RIKU:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 8, 0, work->tiles);
                break;
            case HUM_VIXEN_FRZ_DARK_RIKU:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 11, 0, work->tiles);
                break;
            }
        }

        work->x = gBtlWork->actor->x;
        work->y = gBtlWork->actor->y;
        work->z = gBtlWork->actor->z;

        if (gBtlWork->flags & 0x100000) {
            work->state = HUM_VIXEN_FRZ_STATE_SHATTER;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case HUM_VIXEN_FRZ_STATE_SHATTER:
        if (work->timer == 0) {
            switch (work->variant) {
            case HUM_VIXEN_FRZ_SORA:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 6, 0, work->tiles);
                break;
            case HUM_VIXEN_FRZ_RIKU:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 9, 0, work->tiles);
                break;
            case HUM_VIXEN_FRZ_DARK_RIKU:
                AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 12, 0, work->tiles);
                break;
            }
        }

        work->x = gBtlWork->actor->x;
        work->y = gBtlWork->actor->y;
        work->z = gBtlWork->actor->z;

        if (AnimIsFinished(&work->anim)) {
            args.x = work->x;
            args.y = work->y;
            args.z = work->z;
            TaskCreate(&gBtlWork->taskPools[0], &sTaskDescHumVixenFrg, &args);
            work->state = HUM_VIXEN_FRZ_STATE_DONE;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case HUM_VIXEN_FRZ_STATE_CAST:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        if (TestAttackBox(work->x, work->y, work->z, 8, 8, 1)) {
            gBtlWork->actor->x = work->x;
            gBtlWork->actor->y = work->y;
            gBtlWork->actor->z = work->z;
            gBtlWork->actor->flags |= (BTLOBJ_FLAG_CARD_USE_BLOCKED | BTLOBJ_FLAG_FREEZE_PENDING);
            work->state = HUM_VIXEN_FRZ_STATE_ENCASE;
            gBtlWork->flags &= ~0x100000;
            work->timer = 0;
        } else {
            work->state = HUM_VIXEN_FRZ_STATE_MISS;
            work->timer = 0;
        }

        break;
    case HUM_VIXEN_FRZ_STATE_MISS:
        if (work->timer == 0) {
            AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 1, 0, work->tiles);
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = HUM_VIXEN_FRZ_STATE_MISS_SHATTER;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case HUM_VIXEN_FRZ_STATE_MISS_SHATTER:
        if (work->timer == 0) {
            AnimChangeWithDef(sHumVixenFrzAnimDefs, &work->anim, 5, 0, work->tiles);
        }

        if (AnimIsFinished(&work->anim)) {
            missArgs.x = work->x;
            missArgs.y = work->y;
            missArgs.z = work->z;
            TaskCreate(&gBtlWork->taskPools[0], &sTaskDescHumVixenFrg, &missArgs);
            work->state = HUM_VIXEN_FRZ_STATE_DONE;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case HUM_VIXEN_FRZ_STATE_DONE:
        if (work->timer > 80) {
            return 0;
        }

        work->timer++;
        break;
    }

    AnimUpdate(&work->anim);
    return 1;
}

void task_hum_vixen_frz_2(VixenFrzWork* work) {
    s16 x;
    s16 y;
    void* gfx;
    u16 attr;

    if (work->state != HUM_VIXEN_FRZ_STATE_DONE) {
        gfx = AnimGetGfx(&work->anim);
        attr = GetBattleSpritePriorityFlags(work->y) | work->flipped;
        WorldToScreen(&x, &y, work->x, work->y, work->z);
        DrawSprite(x, y, gfx, work->tiles, work->palette, NULL, attr,
            -0x1004 - (work->y >> 8) * 4);
    }
}

void task_hum_vixen_frz_3(VixenFrzWork* work) {
    ReleaseObjPalette(work->palette);
}

void task_hum_vixen_frg_0(VixenFrgWork* work, VixenNdlArgs* args) {
    VixenFrgSub* shard;
    s32 i;
    s32 angle;
    s32 speed;

    InitObjTilesAtSlot(&work->tilesSlot, ((ObjTiles*)gBtlWork->tiles2)->index, gVixenReitouHahenTiles, sizeof(gVixenReitouHahenTiles));
    work->tiles = &work->tilesSlot;
    work->palette = LoadObjPalette(gVixEPalette, sizeof(gVixEPalette));
    work->timer = 0;
    work->blinking = FALSE;

    for (i = 0; i < 15; i++) {
        const VixenFrgDef* def = &sVixenFrgDefs[i];
        shard = &work->sub[i];
        shard->x = args->x + (def->x << 8);
        shard->y = args->y;
        shard->z = args->z + (def->z << 8);
        shard->spriteFlags = def->spriteFlags;
        shard->gfx = gVixenReitouHahenFrames[def->frame];
        shard->vz = GetRandom() % 0x401 - 0x500;
        angle = (u8)GetRandom();
        speed = GetRandom() % 0x380;
        shard->vx = gSineTable[angle] * speed >> 8;
        shard->vy = -gSineTable[angle + 64] * (speed >> 1) >> 8;
    }

    m4aSongNumStart(SONG_EF_VIC_ICEBREAK);
}

u8 task_hum_vixen_frg_1(VixenFrgWork* work) {
    VixenFrgSub* shard;
    s32 i;

    if (gBtlWork->flags & BTL_FLAG_SUMMON_ACTIVE) {
        return 0;
    }

    for (i = 0; i < 15; i++) {
        shard = &work->sub[i];
        shard->x += shard->vx;
        shard->y += shard->vy;
        shard->z += shard->vz;
        shard->vz += gBtlWork->gravity;

        if (shard->z > 0) {
            shard->z = 0;
            shard->vz = -(shard->vz >> 1);
            shard->vx = shard->vx >> 1;
            shard->vy = shard->vy >> 1;
        }

        ClampBattlePosition(&shard->x, &shard->y, 0, 0);
    }

    work->timer++;

    if (work->timer == 50) {
        work->blinking = TRUE;
    }

    if (work->timer > 70) {
        return 0;
    }

    return 1;
}

void task_hum_vixen_frg_2(VixenFrgWork* work) {
    VixenFrgSub* shards;
    s16 x;
    s16 y;
    u16 attr;
    s32 i;

    if (work->blinking) {
        if (work->timer & 1) {
            return;
        }
    }

    shards = work->sub;

    for (i = 0; i < 15; i++) {
        attr = GetBattleSpritePriorityFlags(shards[i].y) | shards[i].spriteFlags;
        WorldToScreen(&x, &y, shards[i].x, shards[i].y, shards[i].z);
        DrawSprite(x, y, shards[i].gfx, work->tiles, work->palette, NULL, attr,
            -0x1004 - (shards[i].y >> 8) * 4);
    }
}

void task_hum_vixen_frg_3(VixenFrgWork* work) {
    ReleaseObjPalette(work->palette);
}
