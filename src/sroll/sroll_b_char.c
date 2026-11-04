/**
 * sroll_b_char.c
 * Staff Roll Credit Characters
 */

#include "sroll.h"
#include "evt_obj.h"
#include "fade.h"
#include "anim.h"
#include "evt_object_types.h"
#include "evt_types.h"
#include "obj.h"
#include "obj_api.h"
#include "registration_data.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"

static s32 Square(s32 x) {
    return x * x;
}

enum SrollBCharMotion {
    SROLL_B_CHAR_MOTION_NONE,
    SROLL_B_CHAR_MOTION_SLIDE_DOWN_LEFT,
    SROLL_B_CHAR_MOTION_HOP,
    SROLL_B_CHAR_MOTION_BOB_SPARKLE,
    SROLL_B_CHAR_MOTION_SLIDE_UP_RIGHT,
    SROLL_B_CHAR_MOTION_SWAY
};

void SrollBCharSetMotion(Task* task, s32 motion) {
    ((SrollBCharWork*)task->work)->motion = motion;
}

void SrollBCharChangeAnim(SrollBCharWork* work) {
    const EvtObjAnim* def;
    const EvtAnimDef* gfx;

    def = work->obj->animEntry;
    gfx = def->animDef;
    AnimChangeWithTables(&work->anim, def->animId, def->flags, gfx->anims, gfx->gfxTable);
    SetObjTileSource(work->tiles, gfx->tiles);
    work->obj->flags &= ~EVTOBJ_FLAG_ANIM_CHANGED;
}

void task_sroll_b_char_0(SrollBCharWork* work, EvtObjParam* arg) {
    const EvtObjRes* res;
    AnimState* anim;

    res = arg->res;
    work->motion = SROLL_B_CHAR_MOTION_NONE;
    work->motionTimer = 0;
    work->obj = arg->obj;
    work->tiles = AllocObjTiles(res->tileCount * 32, NULL);
    work->palette = LoadObjPalette(res->palette, 32);
    anim = &work->anim;
    AnimInit(anim, NULL, NULL);
    work->obj->anim = anim;
    work->obj->paletteIndex = BIOS_PTR(work->palette)->index;
    SrollBCharChangeAnim(work);
    TaskPoolInit(&work->tasks, 4);
}

s32 task_sroll_b_char_1(SrollBCharWork* work) {
    SrollBCrtnArg sparkle;

    if (work->obj->flags & EVTOBJ_FLAG_ANIM_CHANGED) {
        SrollBCharChangeAnim(work);
    }

    if ((work->obj->drawFlags & SPRITE_FLAG_BLEND) == 0) {
        FadeSetPaletteExcluded((BIOS_PTR(work->palette)->index & 15) + 16, FALSE);
    } else {
        FadeSetPaletteExcluded((BIOS_PTR(work->palette)->index & 15) + 16, TRUE);
    }

    AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);

    switch (work->motion) {
    case SROLL_B_CHAR_MOTION_SLIDE_DOWN_LEFT:
        work->obj->x -= 128;
        work->obj->y += 128;
        break;
    case SROLL_B_CHAR_MOTION_HOP:
        work->obj->z = gSrollBCharHopOffsets[(work->motionTimer >> 2) & 15] << 8;
        work->motionTimer++;
        break;
    case SROLL_B_CHAR_MOTION_BOB_SPARKLE:
        if ((work->motionTimer & 3) == 0) {
            sparkle.kind = 2;
            sparkle.x = work->obj->x;
            sparkle.y = work->obj->y;
            TaskCreate(&work->tasks, &gTaskDescSrollBCrtn, &sparkle);
        }

        work->obj->z = (gSrollBCharHopOffsets[(work->motionTimer >> 2) & 15] << 8) >> 2;
        work->motionTimer++;
        break;
    case SROLL_B_CHAR_MOTION_SLIDE_UP_RIGHT:
        work->obj->x += 128;
        work->obj->y -= 128;
        break;
    case SROLL_B_CHAR_MOTION_SWAY:
        work->obj->x += (gSrollBCharSwayOffsets[(work->motionTimer >> 2) & 15] << 8) >> 2;
        work->motionTimer++;
        break;
    }

    return 1;
}

void task_sroll_b_char_2(SrollBCharWork* work) {
    EvtObj* obj;
    void* gfx;
    u16 x;
    u16 y;

    obj = work->obj;

    if ((obj->flags & EVTOBJ_FLAG_HIDDEN) == 0) {
        x = obj->x >> 8;
        y = (obj->y + obj->z) >> 8;
        gfx = AnimGetGfx(&work->anim);
        DrawSprite(x, y, gfx, work->tiles, work->palette,
                   AllocObjAffine(obj->angle, obj->scaleX, obj->scaleY, TRUE), obj->drawFlags, 0xFF0);
        TaskPoolDraw(&work->tasks);
    }
}

void task_sroll_b_char_3(SrollBCharWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}

const s32 gSrollBCharSwayOffsets[16] = { -1, -2, -3, -4, -3, -2, -1, 0, 1, 2, 3, 4, 3, 2, 1, 0 };

const s32 gSrollBCharHopOffsets[16] = { -1, -2, -3, -4, -5, -6, -7, -8, -7, -6, -5, -4, -3, -2, -1, 0 };

TaskDesc gTaskDescSrollBChar = {
    "task_sroll_b_char",
    (TaskInitFunc)task_sroll_b_char_0,
    (TaskUpdateFunc)task_sroll_b_char_1,
    (TaskDrawFunc)task_sroll_b_char_2,
    (TaskDestroyFunc)task_sroll_b_char_3,
    sizeof(SrollBCharWork),
};
