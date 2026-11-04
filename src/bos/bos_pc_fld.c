/**
 * bos_pc_fld.c
 * Parasite Cage Boss Arena
 */

#include "bos6.h"
#include "sprites_bos6.h"
#include "sprites_staff_roll.h"
#include "btl_api.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_bg_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "display.h"
#include "obj.h"
#include "obj_api.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "engine_math.h"
#include "gba/defines.h"

#define BOS_PC_FLD_VIEW_TOP_MARGIN 48

static u8 sBosPcFldShakeActive;
static s16 sBosPcFldShakePattern;
static s16 sBosPcFldShakeStep;
static s32 sBosPcFldShakeOffset;

static const s8 sBosPcFldShakePattern0[33] = {
    4, 4, 4, 4, -4, -4, -4, -4, 3, 3, 3, 3, -3, -3, -3, -3, 2, 2, 2, 2, -2, -2, -2, -2, 1, 1, 1, 1, -1, -1, -1, -1, 0,
};

static const s8 sBosPcFldShakePattern1[9] = {
    1, 2, 2, 1, -1, -2, -2, -1, 0,
};

static const u16 sBosPcFldPaletteCycleNext[3] = {
    1, 2, 0,
};

static const s16 sBosPcFldPaletteCycleFrames[3] = {
    6, 6, 6,
};

static const s8* sBosPcFldShakePatterns[2] = { sBosPcFldShakePattern0, sBosPcFldShakePattern1 };

TaskDesc gTaskDescBosPcFld = {
    "task_bos_pc_fld",
    (TaskInitFunc)task_bos_pc_fld_0,
    (TaskUpdateFunc)task_bos_pc_fld_1,
    (TaskDrawFunc)task_bos_pc_fld_2,
    (TaskDestroyFunc)task_bos_pc_fld_3,
    sizeof(PcFldWork),
};

static s32 Square(s32 x) {
    return x * x;
}

void BosPcFldSetPaletteCycle(Task* task, u8 on) {
    PcFldWork* work = task->work;

    work->paletteCycle = on;
}

void BosPcFldEnableObject(Task* task, u8 on) {
    PcFldWork* work;
    ObjPalette* pal;

    work = task->work;

    if (on == TRUE) {
        on = FALSE;
    } else {
        on = TRUE;
    }

    ColliderSetDisabled(&work->collider, on);

    if (!on) {
        if (work->tiles == NULL) {
            work->tiles = LoadObjTiles(gBosPcFldTiles, sizeof(gBosPcFldTiles));
        }

        if (work->palette == NULL) {
            pal = LoadObjPalette(gBosPcObjPalette, sizeof(gBosPcObjPalette));
            work->palette = pal;
            LoadPalette(gBosPcFldPalette, (void*)PLTT_MIRROR(OBJ_PLTT + (BIOS_PTR(pal)->index + 1) * PLTT_SIZE_4BPP), sizeof(gBosPcFldPalette));
        }
    }
}

void BosPcFldResetShake() {
    sBosPcFldShakeActive = FALSE;
    sBosPcFldShakePattern = 0;
    sBosPcFldShakeStep = 0;
    sBosPcFldShakeOffset = 0;
}

void BosPcFldStartShake(s16 pattern) {
    sBosPcFldShakeActive = TRUE;
    sBosPcFldShakePattern = pattern;
    sBosPcFldShakeStep = 0;
    sBosPcFldShakeOffset = 0;
}

void BosPcFldUpdateShake() {
    const s8* pattern;

    if (sBosPcFldShakeActive) {
        pattern = sBosPcFldShakePatterns[sBosPcFldShakePattern];
        sBosPcFldShakeOffset += ((pattern[sBosPcFldShakeStep] << 12) - sBosPcFldShakeOffset) >> 3;
        sBosPcFldShakeStep += 1;

        if (pattern[sBosPcFldShakeStep] == 0) {
            sBosPcFldShakeActive = FALSE;
            sBosPcFldShakeOffset = 0;
        }
    }
}

s32 BosPcFldGetShake() {
    return sBosPcFldShakeOffset;
}

void BosPcFldResetPaletteCycle(PcFldWork* work) {
    u16 zero;

    zero = 0;
    work->paletteCycle = zero;
    work->paletteIndex = zero;
    work->paletteTimer = zero;
}

void BosPcFldUpdatePaletteCycle(PcFldWork* work) {
    u16 next;
    u16 zero;

    if (work->paletteCycle != 0) {
        if (work->paletteTimer > sBosPcFldPaletteCycleFrames[work->paletteIndex]) {
            next = sBosPcFldPaletteCycleNext[work->paletteIndex];
            zero = 0;
            work->paletteIndex = next;
            work->paletteTimer = zero;
        }

        work->paletteTimer += 1;
    }
}

void BosPcFldLoadPaletteCycle(PcFldWork* work) {
    if (work->paletteCycle != 0) {
        LoadPalette(gBosPcCyclePalettes + work->paletteIndex * 16, (void*)(BG_PLTT + 4 * PLTT_SIZE_4BPP), 32);
    }
}

void BosPcFldStopPaletteCycle(PcFldWork* work) {
    work->paletteCycle = 0;
}

void task_bos_pc_fld_0(PcFldWork* work, PcBattleBackgroundDef* arg) {
    Collider* collider;

    LoadBgTiles(0, arg->tiles, arg->tilesSize);
    LoadBgPalette(0, arg->palette, arg->paletteSize);
    SetBgMapBlocks(0, &arg->map, 2, 3);
    gBtlWork->scale = Q_8_8(1);
    gBtlWork->zoomScale = Q_8_8(1);
    gBtlWork->x = 0x11400;
    gBtlWork->y = 0x15300;
    gBtlWork->viewX = 0x11400;
    gBtlWork->viewY = 0x15300;
    gBtlWork->x2 = 0x11400;
    gBtlWork->y2 = 0x15300;
    gBtlWork->zoomX = 0x11400;
    gBtlWork->zoomY = 0x15300;
    gBtlWork->zoomSteps = 15;
    gBtlWork->rotation = 0;
    BtlMapResetShake();
    BosPcFldResetShake();
    ScrollBgMapTo(0, gBtlWork->viewX >> 8, gBtlWork->viewY >> 8);
    BosPcFldResetPaletteCycle(work);
    BosPcFldUpdatePaletteCycle(work);
    work->tiles = NULL;
    work->palette = NULL;
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_SOLID, 40, 8);
    ColliderSetPosition(collider, 0x17400, 0x15400, 0);
    ColliderSetDisabled(collider, TRUE);
}

u8 task_bos_pc_fld_1(PcFldWork* work) {
    s32 screenX;
    s32 viewLeft;
    s32 dx;
    s32 dy;
    BtlObj* actor;

    BtlMapUpdateShake();
    BosPcFldUpdateShake();
    actor = gBtlWork->actor;
    viewLeft = gBtlWork->viewX - (DISPLAY_WIDTH / 2) * 256;
    screenX = actor->x - viewLeft;

    if (screenX < 0) {
        screenX = 0;
    }

    gBtlWork->x2 = screenX / 2 + 0xF000;
    dx = (gBtlWork->x2 - gBtlWork->x) >> 3;
    dy = (gBtlWork->y2 - gBtlWork->y) >> 3;

    // @bug Should clamp to +-0x500, not add it.
    if (dx > 0x500) {
        dx += 0x500;
    } else if (dx < -0x500) {
        dx -= 0x500;
    }

    gBtlWork->x += dx;
    gBtlWork->y += dy;
    gBtlWork->viewX = gBtlWork->x;
    gBtlWork->viewY = gBtlWork->y;

    if (gBtlWork->viewX - (DISPLAY_WIDTH / 2) * 256 < gBtlWork->xMin * 256) {
        gBtlWork->viewX = (gBtlWork->xMin + DISPLAY_WIDTH / 2) * 256;
    } else if (gBtlWork->viewX + (DISPLAY_WIDTH / 2) * 256 > gBtlWork->xMax * 256) {
        gBtlWork->viewX = (gBtlWork->xMax - DISPLAY_WIDTH / 2) * 256;
    }

    if (gBtlWork->viewY + BOS_PC_FLD_VIEW_TOP_MARGIN * 256 < gBtlWork->yMin * 256) {
        gBtlWork->viewY = (gBtlWork->yMin - BOS_PC_FLD_VIEW_TOP_MARGIN) * 256;
    } else if (gBtlWork->viewY + (DISPLAY_HEIGHT / 2) * 256 > gBtlWork->yMax * 256) {
        gBtlWork->viewY = (gBtlWork->yMax - DISPLAY_HEIGHT / 2) * 256;
    }

    gBtlWork->viewY += BtlMapGetShake();
    gBtlWork->viewY += BosPcFldGetShake();
    ScrollBgMapTo(0, (gBtlWork->viewX >> 8) + 8, (gBtlWork->viewY >> 8) + 40);
    BosPcFldUpdatePaletteCycle(work);
    return 1;
}

void task_bos_pc_fld_2(PcFldWork* work) {
    s16 sx;
    s16 sy;
    BtlObj* actor;
    u32 x;
    u32 y;
    s32 z;

    BosPcFldLoadPaletteCycle(work);
    actor = gBtlWork->actor;

    if (actor->z >= -0x100) {
        if ((actor->flags & BTLOBJ_FLAG_HIT_LOCKED) == 0) {
            if (work->tiles != NULL) {
                if (work->palette != NULL) {
                    x = 0x17000;
                    y = 0x14800;
                    z = -0x800;
                    WorldToScreen(&sx, &sy, x, y, z);
                    DrawSprite(sx, sy, gBosPcFldFrames[0], work->tiles,
                        work->palette, NULL, GetBattleSpritePriorityFlags(y),
                        -0x1004 - (s32)(y >> 6));
                }
            }
        }
    }
}

void task_bos_pc_fld_3(PcFldWork* work) {
    BosPcFldStopPaletteCycle(work);
    ColliderUnregister(&work->collider);

    if (work->tiles != NULL) {
        ReleaseObjTiles(work->tiles);
    }

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
    }
}
