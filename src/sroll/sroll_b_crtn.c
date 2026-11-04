/**
 * sroll_b_crtn.c
 * Staff Roll Curtain
 */

#include "sroll.h"
#include "sprites_evt.h"
#include "sprites_smn.h"
#include "fade.h"
#include "anim.h"
#include "engine_math.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"

static s32 Square(s32 x) {
    return x * x;
}

static inline s32 GetSrollCurtainOffset() {
    return (GetRandom() % 9) * 256 - 0x400;
}

void task_sroll_b_crtn_0(SrollBCrtnWork* work, SrollBCrtnArg* arg) {
    AnimState* anim;
    s32 offset;

    work->timer = 0;
    work->kind = arg->kind;

    switch (work->kind) {
    case 0:
    case 1:
    case 5:
        work->x = arg->x;
        work->y = arg->y + 0xFFFFE000;
        work->tiles = AllocObjTiles(128, gFEventTiles);
        work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
        anim = &work->anim;
        AnimInit(anim, gFEventAnims, gFEventFrames);
        AnimStart(anim, work->kind, 0);
        break;
    case 3:
        work->x = arg->x;
        work->y = arg->y + 0xFFFFD000;
        work->tiles = AllocObjTiles(128, gFEventTiles);
        work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
        anim = &work->anim;
        AnimInit(anim, gFEventAnims, gFEventFrames);
        AnimStart(anim, 0, 0);
        break;
    case 2:
        offset = GetSrollCurtainOffset();
        work->x = arg->x + offset;
        offset = GetSrollCurtainOffset();
        work->y = arg->y + offset;
        work->tiles = AllocObjTiles(128, gSmnTinkEffTiles);
        work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
        anim = &work->anim;
        AnimInit(anim, gSmnTinkEffAnims, gSmnTinkEffFrames);
        AnimStart(anim, work->kind, 0);
        break;
    }

    FadeSetPaletteExcluded((BIOS_PTR(work->palette)->index & 15) + 16, FALSE);
}

u8 task_sroll_b_crtn_1(SrollBCrtnWork* work) {
    u8 alive;

    alive = 1;
    AnimUpdate(&work->anim);
    work->timer++;

    switch (work->kind) {
    case 1:
        if (work->timer > 120) {
            alive = 0;
        }

        break;
    case 2:
        work->y += 0x100;

        if (work->timer > 20) {
            alive = 0;
        }

        break;
    case 5:
        if (work->timer == 12) {
            AnimStart(&work->anim, 6, ANIM_FLAG_LOOP);
        }
    case 0:
    case 3:
        if (work->timer > 50) {
            alive = 0;
        }

        break;
    }

    return alive;
}

void task_sroll_b_crtn_2(SrollBCrtnWork* work) {
    DrawSprite(work->x >> 8, work->y >> 8, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
               0xFE0);
}

void task_sroll_b_crtn_3(SrollBCrtnWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescSrollBCrtn = {
    "task_sroll_b_crtn",
    (TaskInitFunc)task_sroll_b_crtn_0,
    (TaskUpdateFunc)task_sroll_b_crtn_1,
    (TaskDrawFunc)task_sroll_b_crtn_2,
    (TaskDestroyFunc)task_sroll_b_crtn_3,
    sizeof(SrollBCrtnWork),
};
