/**
 * evt_obj.c
 * Event Objects and Shadows
 */

#include "macros.h"
#include "task_descriptors.h"
#include "evt_obj.h"
#include "sprites_btl.h"
#include "evt_tasks.h"
#include "anim.h"
#include "evt_data.h"
#include "evt_object_types.h"
#include "evt_types.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "engine_math.h"

static TaskDesc sTaskDescEvtObj = {
    "task_evt_obj",
    (TaskInitFunc)task_evt_obj_0,
    (TaskUpdateFunc)task_evt_obj_1,
    (TaskDrawFunc)task_evt_obj_2,
    (TaskDestroyFunc)task_evt_obj_3,
    sizeof(EvtObjWork),
};

EventState* gEventState EWRAM_COMMON(4);

void EvtObjSetAnim(EvtObj* obj, s32 anim) {
    u16 flags = obj->flags | EVTOBJ_FLAG_ANIM_CHANGED;

    obj->flags = flags;
    obj->animEntry = &gEvtObjAnims[anim];
}

void EvtObjSetPos(EvtObj* obj, s32 x, s32 y, s32 z) {
    obj->x = x;
    obj->y = y;
    obj->z = z;
}

void EvtObjSetGroundZ(EvtObj* obj, s32 groundZ) {
    obj->groundZ = groundZ;
}

void CreateEvtObjTask(void* pool, EvtObj* obj, s32 res, s32 anim, s32 x, s32 y, s32 z) {
    EvtObjParam param;

    param.res = &gEvtObjResources[res].res;
    param.obj = obj;
    EvtObjSetAnim(obj, anim);
    EvtObjSetPos(obj, x, y, z);
    obj->flags = 0;
    obj->groundZ = 0;
    obj->drawFlags = SPRITE_PRIORITY(2);
    obj->scaleY = Q_8_8(1);
    obj->scaleX = Q_8_8(1);
    obj->angle = 0;
    TaskCreate(pool, &sTaskDescEvtObj, &param);
}

void EvtObjSetDrawFlags(EvtObj* obj, u16 flags) {
    obj->drawFlags = flags;
}

Task* CreateEvtObjTaskWithDesc(void* pool, void* desc, EvtObj* obj, s32 res, s32 anim, s32 x, s32 y, s32 z) {
    EvtObjParam param;

    param.res = &gEvtObjResources[res].res;
    param.obj = obj;
    EvtObjSetAnim(obj, anim);
    EvtObjSetPos(obj, x, y, z);
    obj->flags = 0;
    obj->groundZ = 0;
    obj->drawFlags = SPRITE_PRIORITY(2);
    obj->scaleY = Q_8_8(1);
    obj->scaleX = Q_8_8(1);
    obj->angle = 0;
    return TaskCreate(pool, desc, &param);
}

void EvtObjChangeAnim(EvtObjWork* work) {
    EvtObj* obj;
    const EvtObjAnim* anim;
    const EvtAnimDef* def;

    obj = work->obj;
    anim = obj->animEntry;
    def = anim->animDef;
    AnimChangeWithTables(&work->anim, anim->animId, anim->flags, def->anims, def->gfxTable);
    SetObjTileSource(work->tiles, def->tiles);
    work->obj->flags &= ~EVTOBJ_FLAG_ANIM_CHANGED;
}

void task_evt_obj_0(EvtObjWork* work, EvtObjParam* param) {
    const EvtObjRes* res;

    res = param->res;
    work->obj = param->obj;
    work->tiles = AllocObjTiles(res->tileCount * 32, NULL);
    work->palette = LoadObjPalette(res->palette, 32);
    AnimInit(&work->anim, NULL, NULL);
    work->obj->anim = &work->anim;
    work->obj->paletteIndex = BIOS_PTR(work->palette)->index;
    EvtObjChangeAnim(work);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescEvtShadow, work->obj);
}

s32 task_evt_obj_1(EvtObjWork* work) {
    if (work->obj->flags & EVTOBJ_FLAG_ANIM_CHANGED) {
        EvtObjChangeAnim(work);
    }

    AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);

    return 1;
}

void task_evt_obj_2(EvtObjWork* work) {
    EvtObj* obj;
    u16 x;
    u16 y;
    void* gfx;

    obj = work->obj;

    if (obj->flags & EVTOBJ_FLAG_HIDDEN) {
        return;
    }

    x = (obj->x >> 8) - (gEventState->x >> 8);
    y = (obj->y >> 8) + (obj->z >> 8) - (gEventState->y >> 8);
    gfx = AnimGetGfx(&work->anim);
    DrawSprite(x, y, gfx, work->tiles, work->palette,
        AllocObjAffine(obj->angle, obj->scaleX, obj->scaleY, TRUE), obj->drawFlags,
        -0x1002 - (obj->y >> 8) * 4);
    TaskPoolDraw(&work->tasks);
}

void task_evt_obj_3(EvtObjWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&work->tasks);
}

void task_evt_shadow_0(EvtShadowWork* work, EvtObj* obj) {
    work->obj = obj;
    work->tiles = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->tiles3 = LoadObjTiles(gBtlShadowSmallTiles, sizeof(gBtlShadowSmallTiles));
    work->tiles2 = LoadObjTiles(gBtlShadowLargeTiles, sizeof(gBtlShadowLargeTiles));
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
}

s32 task_evt_shadow_1() {
    return 1;
}

void task_evt_shadow_2(EvtShadowWork* work) {
    EvtObj* obj;
    void* gfx;
    void* vram;
    s32 size;
    ObjAffine* sprite;
    s32 x;
    s32 y;

    obj = work->obj;

    if (obj->flags & EVTOBJ_FLAG_NO_SHADOW) {
        return;
    }

    if (obj->flags & EVTOBJ_FLAG_SHADOW_WIDE) {
        gfx = gBtlShadowLargeFrame0;
        vram = work->tiles2;
    } else if (obj->flags & EVTOBJ_FLAG_SHADOW_SMALL) {
        gfx = gBtlShadowSmallFrame0;
        vram = work->tiles3;
    } else {
        gfx = gBtlShadowFrame0;
        vram = work->tiles;
    }

    if (obj->z >= obj->groundZ) {
        sprite = NULL;
    } else {
        size = Q_8_8(1) - (obj->groundZ - obj->z) / 128;

        if (size <= 0x18) {
            size = Q_8_8(0.1);
        }

        sprite = AllocObjAffine(0, size, size, FALSE);
    }

    x = (obj->x >> 8) - (gEventState->x >> 8);
    y = (obj->y >> 8) + (obj->groundZ >> 8) - (gEventState->y >> 8);
    DrawSprite(x, y, gfx, vram, work->palette, sprite, obj->drawFlags, 0xFFF0);
}

void task_evt_shadow_3(EvtShadowWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescEvtShadow = {
    "task_evt_shadow",
    (TaskInitFunc)task_evt_shadow_0,
    (TaskUpdateFunc)task_evt_shadow_1,
    (TaskDrawFunc)task_evt_shadow_2,
    (TaskDestroyFunc)task_evt_shadow_3,
    sizeof(EvtShadowWork),
};
