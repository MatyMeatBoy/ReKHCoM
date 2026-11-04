/**
 * bos_pc_acd.c
 * Parasite Cage Boss Acid
 */

#include "bos6.h"
#include "sprites_bos6.h"
#include "fade.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "evt_types.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"

TaskDesc gTaskDescBosPcAcd = {
    "task_bos_pc_acd",
    (TaskInitFunc)task_bos_pc_acd_0,
    (TaskUpdateFunc)task_bos_pc_acd_1,
    (TaskDrawFunc)task_bos_pc_acd_2,
    (TaskDestroyFunc)task_bos_pc_acd_3,
    sizeof(PcAcdWork),
};

s32 BosPcAcdSquare(s32 x) {
    return x * x;
}

s32 BosPcAcdSquare2(s32 x) {
    return x * x;
}

void BosPcAcdSetOff(Task* task, u8 off) {
    PcAcdWork* work = task->work;

    work->acdOff = off;
}

void task_bos_pc_acd_0(PcAcdWork* work, PcShared* arg) {
    AnimState* anim;

    work->unk_000 = 0;
    work->tiles = AllocObjTiles(0x300, gBosPcAcdTiles);
    work->palette = LoadObjPalette(gBosPcObjPalette, sizeof(gBosPcObjPalette));
    work->x = -1;
    work->y = -1;
    work->z = -1;
    work->shared = arg;
    anim = &work->anim;
    AnimInit(anim, gBosPcAcdAnims, gBosPcAcdFrames);

    if (work->shared->inEvent == TRUE) {
        work->acdOff = TRUE;
        AnimStart(anim, 1, 0);
    } else {
        work->acdOff = FALSE;
        AnimStart(anim, 0, 0);
    }
}

u8 task_bos_pc_acd_1(PcAcdWork* work) {
    AnimState* anim;
    s32 animId;

    FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 17, FALSE);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 18, FALSE);
    anim = &work->anim;
    AnimUpdate(anim);

    if (gBtlWork->actor->z >= 0) {
        if ((gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) == 0 ||
            (gBtlWork->flags & BTL_FLAG_SUMMON_ACTIVE) == 0) {
            if (work->x < 0 || AnimIsFinished(anim) == TRUE) {
                animId = 0;

                if (work->acdOff == TRUE) {
                    animId = 1;
                }

                AnimReset(anim);
                AnimStart(anim, animId, 0);
            }
        }
    }

    return 1;
}

void task_bos_pc_acd_2(PcAcdWork* work) {
    s16 sx;
    s16 sy;
    BtlWork** btl;
    BtlObj* actor;
    PcShared* shared;
    AnimState* anim;
    void** frames;
    s32 frame;
    void* gfx;
    s32 shakeX;
    s32 shakeY;

    btl = &gBtlWork;
    actor = (*btl)->actor;
    actor->flags &= ~BTLOBJ_FLAG_HIDE_SHADOW;
    shakeX = 0;
    shakeY = 0;
    shared = work->shared;

    if (shared->inEvent == TRUE) {
        shakeX = gEventState->shakeX << 8;
        shakeY = gEventState->shakeY << 8;
    }

    work->x = actor->x;
    work->y = actor->y - 0x400;
    work->z = 0;

    if (shared->forceRipple == TRUE) {
        frames = gBosPcAcdFrames;
        frame = AnimGetGfxIndex(&work->anim) + 5;
        gfx = frames[frame];

        if ((*btl)->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
            WorldToScreen(&sx, &sy, work->x - shakeX + 0x600, work->y - shakeY, 0);
        } else {
            WorldToScreen(&sx, &sy, work->x - shakeX + 0x200, work->y - shakeY, 0);
        }

        DrawSprite(sx, sy, gfx, work->tiles, work->palette, NULL, GetBattleSpritePriorityFlags(work->y),
                   (-0x1004 - ((work->y >> 8) << 2)) | 3);
    } else if (actor->z >= 0) {
        if (((*btl)->flags & BTL_FLAG_PLAYER_CARD_ACTION) && ((*btl)->flags & BTL_FLAG_SUMMON_ACTIVE)) {
            return;
        }

        anim = &work->anim;

        if (AnimGetId(anim) == 1) {
            (*btl)->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;

            if ((*btl)->actor->flags & BTLOBJ_FLAG_HIT_LOCKED) {
                return;
            }

            frames = gBosPcAcdFrames;
            frame = AnimGetGfxIndex(anim) + 5;
            gfx = frames[frame];

            if ((*btl)->actor->flags & BTLOBJ_FLAG_FACING_LEFT) {
                WorldToScreen(&sx, &sy, work->x - shakeX + 0x600, work->y - shakeY, 0);
            } else {
                WorldToScreen(&sx, &sy, work->x - shakeX + 0x200, work->y - shakeY, 0);
            }

            DrawSprite(sx, sy, gfx, work->tiles, work->palette, NULL, GetBattleSpritePriorityFlags(work->y),
                       (-0x1004 - ((work->y >> 8) << 2)) | 3);
        } else {
            WorldToScreen(&sx, &sy, work->x - shakeX, work->y - shakeY, 0);
            DrawSprite(sx, sy, AnimGetGfx(anim), work->tiles, work->palette, NULL, GetBattleSpritePriorityFlags((*btl)->actor->y),
                       -0x1004 - (((*btl)->actor->y >> 8) << 2));
        }
    }
}

void task_bos_pc_acd_3(PcAcdWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}
