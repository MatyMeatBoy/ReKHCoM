/**
 * bos_tm_body.c
 * Trickmaster Boss Body
 */

#include "macros.h"
#include "boss_tm.h"
#include "sprites_boss_tm.h"
#include "card_api.h"
#include "engine_math.h"
#include "system_state.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "enemy_ids.h"

BtlObj gBosTmBodyObjCopy EWRAM_COMMON(16);

static const EmyKind sBosTmEmyKind = { ENEMY_TRICKMASTER, 1300, 28, 14, 20, 40, EMY_KIND_FLAG_NO_COLLIDER };

static u8 sBosTmBodyAngles[11] = { 0, 2, 5, 15, 18, 20, 20, 18, 15, 5, 2 };

static s16 sBosTmBodyIdleZ[8] = { 8, -8, -12, 12, 8, -8, -12, 12 };

static s16 sBosTmBodyWalkZ[10] = { -11, -8, -10, 10, 19, -11, -8, -10, 10, 19 };

static TmBodyStep sBosTmBodyRecoilSteps[3] = {
    { -8, 0, 246, { 0, 0, 0 }, -3, 2, 246, { 0, 0, 0 }, -6, 2, { 0, 0 }, 0, -6, 2, { 0, 0 }, 0 },
    { -6, 8, 246, { 0, 0, 0 }, -2, 7, 246, { 0, 0, 0 }, -6, 6, { 0, 0 }, 0, -6, 6, { 0, 0 }, 0 },
    { -4, 0, 246, { 0, 0, 0 }, 0, 0, 246, { 0, 0, 0 }, -3, 0, { 0, 0 }, 0, -3, 0, { 0, 0 }, 0 },
};

TmBodyStep gUnk_09EF1DE8 = { 0, 0, 0, { 0, 0, 0 }, 0, 0, 0, { 0, 0, 0 }, 0, 0, { 0, 0 }, 0, 0, 0, { 0, 0 }, 0 };

static s16 sBosTmBodyRecoilZ[3] = { 15, 6, 0 };

s16 gUnk_09EF1E0E = -8;

s16 gUnk_09EF1E10 = -20;

s16 gUnk_09EF1E12 = -28;

static TmBodyStep sBosTmBodyThrowSteps[16] = {
    { 2, -11, 15, { 0, 0, 0 }, 0, -11, 5, { 0, 0, 0 }, 0, -7, { 0, 0 }, 0, 0, -7, { 0, 0 }, 0 },
    { 7, -12, 15, { 0, 0, 0 }, 5, -12, 5, { 0, 0, 0 }, 4, -8, { 0, 0 }, 1, 4, -8, { 0, 0 }, 1 },
    { 1, -6, 15, { 0, 0, 0 }, -2, -6, 5, { 0, 0, 0 }, 5, -5, { 0, 0 }, 2, 5, -7, { 0, 0 }, 2 },
    { 4, 2, 15, { 0, 0, 0 }, 1, 0, 5, { 0, 0, 0 }, 1, 2, { 0, 0 }, 3, 1, 2, { 0, 0 }, 3 },
    { -4, -2, 241, { 0, 0, 0 }, -1, 0, 251, { 0, 0, 0 }, -6, -2, { 0, 0 }, 2, -6, -2, { 0, 0 }, 2 },
    { -2, 0, 241, { 0, 0, 0 }, -1, -1, 251, { 0, 0, 0 }, 0, -4, { 0, 0 }, 2, 0, -4, { 0, 0 }, 2 },
    { -6, -1, 241, { 0, 0, 0 }, -2, 0, 251, { 0, 0, 0 }, -4, -2, { 0, 0 }, 2, -4, -2, { 0, 0 }, 2 },
    { -2, 1, 241, { 0, 0, 0 }, 0, 2, 251, { 0, 0, 0 }, 0, 2, { 0, 0 }, 1, 0, 2, { 0, 0 }, 1 },
    { -6, 5, 248, { 0, 0, 0 }, 0, 6, 248, { 0, 0, 0 }, -2, 8, { 0, 0 }, 1, -2, 8, { 0, 0 }, 1 },
    { -5, 14, 248, { 0, 0, 0 }, -3, 14, 248, { 0, 0, 0 }, -7, 13, { 0, 0 }, 1, -7, 13, { 0, 0 }, 1 },
    { -4, 12, 248, { 0, 0, 0 }, -1, 11, 248, { 0, 0, 0 }, -2, 11, { 0, 0 }, 0, -2, 11, { 0, 0 }, 0 },
    { -6, 13, 248, { 0, 0, 0 }, -2, 9, 248, { 0, 0, 0 }, -5, 9, { 0, 0 }, 0, -5, 9, { 0, 0 }, 0 },
    { 1, 6, 248, { 0, 0, 0 }, 1, 10, 248, { 0, 0, 0 }, -1, 11, { 0, 0 }, 0, -1, 11, { 0, 0 }, 0 },
    { 0, -2, 8, { 0, 0, 0 }, -1, 0, 10, { 0, 0, 0 }, 1, -1, { 0, 0 }, 0, 1, -1, { 0, 0 }, 0 },
    { 7, -11, 8, { 0, 0, 0 }, 2, -12, 10, { 0, 0, 0 }, 7, -14, { 0, 0 }, 0, 7, -14, { 0, 0 }, 0 },
    { -4, -8, 9, { 0, 0, 0 }, 4, -9, 10, { 0, 0, 0 }, 5, -7, { 0, 0 }, 0, 5, -7, { 0, 0 }, 0 },
};

TmBodyStep gUnk_09EF2014 = { 0, 0, 0, { 0, 0, 0 }, 0, 0, 0, { 0, 0, 0 }, 0, 0, { 0, 0 }, 0, 0, 0, { 0, 0 }, 0 };

static TmBodyStep sBosTmBodySpinSteps[9] = {
    { 0, 2, 0, { 0, 0, 0 }, 0, 2, 0, { 0, 0, 0 }, 0, 2, { 0, 0 }, 0, 0, 2, { 0, 0 }, 0 },
    { -8, 6, 246, { 0, 0, 0 }, -3, 7, 246, { 0, 0, 0 }, -6, 7, { 0, 0 }, 0, -6, 7, { 0, 0 }, 0 },
    { -6, 8, 246, { 0, 0, 0 }, -2, 9, 246, { 0, 0, 0 }, -6, 9, { 0, 0 }, 0, -6, 9, { 0, 0 }, 0 },
    { -4, 0, 246, { 0, 0, 0 }, 0, 0, 246, { 0, 0, 0 }, -3, 0, { 0, 0 }, 0, -3, 0, { 0, 0 }, 0 },
    { 2, -2, 8, { 0, 0, 0 }, -1, 0, 10, { 0, 0, 0 }, 1, -1, { 0, 0 }, 0, 1, -1, { 0, 0 }, 0 },
    { 9, -11, 8, { 0, 0, 0 }, 2, -12, 10, { 0, 0, 0 }, 7, -14, { 0, 0 }, 0, 7, -14, { 0, 0 }, 0 },
    { 6, -8, 9, { 0, 0, 0 }, 4, -9, 10, { 0, 0, 0 }, 5, -7, { 0, 0 }, 0, 5, -7, { 0, 0 }, 0 },
    { 2, -11, 15, { 0, 0, 0 }, 0, -11, 5, { 0, 0, 0 }, 0, -7, { 0, 0 }, 1, 0, -7, { 0, 0 }, 1 },
    { 7, -12, 15, { 0, 0, 0 }, 5, -12, 5, { 0, 0, 0 }, 4, -8, { 0, 0 }, 2, 4, -8, { 0, 0 }, 2 },
};

TmBodyStep gUnk_09EF2154 = { 0, 0, 0, { 0, 0, 0 }, 0, 0, 0, { 0, 0, 0 }, 0, 0, { 0, 0 }, 0, 0, 0, { 0, 0 }, 0 };

TmBodyStep gUnk_09EF2174 = { 0, 0, 0, { 0, 0, 0 }, 0, 0, 0, { 0, 0, 0 }, 0, 0, { 0, 0 }, 0, 0, 0, { 0, 0 }, 0 };

static s8 sBosTmActionChoices[4] = { BOS_TM_STATE_FIRE, BOS_TM_STATE_SLAM_GROUND, BOS_TM_STATE_SPIN, BOS_TM_STATE_WALK_LEFT };

TaskDesc gTaskDescBosTmBody = {
    "task_bos_tm_body",
    (TaskInitFunc)task_bos_tm_body_0,
    (TaskUpdateFunc)task_bos_tm_body_1,
    (TaskDrawFunc)task_bos_tm_body_2,
    (TaskDestroyFunc)task_bos_tm_body_3,
    sizeof(TmBodyWork),
};

void BosTmBodyResetTimers(TmBodyWork* work) {
    work->tm->stepTimer = 0;
    work->tm->step = 0;
    work->tm->stateTimer = 0;
}

void BosTmBodyInitEnemy(BtlObj* obj, s16 x, s16 y, s16 z) {
    obj->x = x << 8;
    obj->y = y << 8;
    obj->z = (z << 8) + 0x1900;
    InitEnemyBtlObj(obj, &sBosTmEmyKind, obj->x, obj->y, obj->z);
    obj->radiusX = 14;
    obj->radiusY = 40;
    obj->height = 28;
    obj->flags |= 0x400;
    obj->flags |= BTLOBJ_FLAG_FACING_LEFT;
}

void BosTmBodySetObjPos(BtlObj* obj, s16 x, s16 y, s16 z) {
    obj->x = x << 8;
    obj->y = y << 8;
    obj->z = z << 8;
}

void BosTmBodyReleaseEnemy(BtlObj* obj) {
    ReleaseEnemyBtlObj(obj);
}

void BosTmBodyUpdateAngle(TmBodyWork* work) {
    work->angleTimer++;

    if (work->angleTimer > 5) {
        work->angleTimer = 0;
        work->angleStep++;

        if (work->angleStep > 10) {
            work->angleStep = 0;
        }

        work->angle = sBosTmBodyAngles[work->angleStep];
    }
}

void BosTmBodyPlaceParts(TmBodyWork* work) {
    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->body.x = work->tm->x2 + 0x400;
        work->body2.x = work->tm->x2;
        work->body3.x = work->tm->x2 + 0xC00;
        work->body4.x = work->tm->x2 - 0x100;
    } else {
        work->body.x = work->tm->x2 - 0x400;
        work->body2.x = work->tm->x2;
        work->body3.x = work->tm->x2 - 0xC00;
        work->body4.x = work->tm->x2 + 0x100;
    }

    work->body.y = work->tm->y2;
    work->body2.y = work->tm->y2;
    work->body3.y = work->tm->y2 + 0x100;
    work->body4.y = work->tm->y2 - 0x400;
    work->body.z = work->tm->z2 - 0x2200;
    work->body2.z = work->tm->z2 + 0x900;
    work->body3.z = work->tm->z2 - 0x2100;
    work->body4.z = work->tm->z2 - 0x1E00;
}

void BosTmBodyResetPose(TmBodyWork* work) {
    work->angle = 0;
    work->angle2 = 0;
    work->gfx3 = gBosTmBodyPart2Frames[0];
    work->gfx4 = gBosTmBodyPart3Frames[0];
    work->tm->x2 = work->tm->baseX;
    work->tm->y2 = work->tm->baseY;
    work->tm->z2 = work->tm->baseZ;
    BosTmBodyPlaceParts(work);
}

void BosTmBodySetDefeatPose(TmBodyWork* work) {
    TmWork* tm;
    s32 dz;

    work->angle = 226;
    work->angle2 = 226;
    work->gfx3 = gBosTmBodyPart2Frames[0];
    work->gfx4 = gBosTmBodyPart3Frames[0];
    dz = 0xF00;
    tm = work->tm;

    if (tm->flags & TM_FLAG_FACING_LEFT) {
        work->tm->x2 = work->tm->baseX - 0xA00;
        work->tm->y2 = work->tm->baseY;
        work->tm->z2 = work->tm->baseZ + dz;
        work->body.x = work->tm->baseX - 0xF00;
        work->body2.x = work->tm->x2 + 0x500;
        work->body3.x = work->tm->x2 + 0x400;
        work->body4.x = work->tm->x2 - 0x900;
    } else {
        work->tm->x2 = work->tm->baseX + 0xA00;
        work->tm->y2 = work->tm->baseY;
        work->tm->z2 = work->tm->baseZ + 0xF00;
        work->body.x = work->tm->baseX + 0xF00;
        work->body2.x = work->tm->x2 - 0x500;
        work->body3.x = work->tm->x2 - 0x400;
        work->body4.x = work->tm->x2 + 0x900;
    }

    work->body.z = work->tm->z2 - 0x1F00;
    work->body2.z = work->tm->z2 + 0xE00;
    work->body3.z = work->tm->z2 - 0x1F00;
    work->body4.z = work->tm->z2 - 0x1C00;
}

void BosTmBodySetBreakPose(TmBodyWork* work) {
    work->angle = 30;
    work->angle2 = 10;
    work->gfx3 = gBosTmBodyPart2Frames[0];
    work->gfx4 = gBosTmBodyPart3Frames[0];

    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->tm->x2 = work->tm->baseX + 0xA00;
        work->tm->y2 = work->tm->baseY;
        work->tm->z2 = work->tm->baseZ + 0xF00;
        work->body.x = work->tm->baseX + 0xB00;
        work->body2.x = work->tm->x2 - 0x500;
        work->body3.x = work->tm->x2 + 0x900;
        work->body4.x = work->tm->x2 - 0x400;
    } else {
        work->tm->x2 = work->tm->baseX - 0xA00;
        work->tm->y2 = work->tm->baseY;
        work->tm->z2 = work->tm->baseZ + 0xF00;
        work->body.x = work->tm->baseX - 0xB00;
        work->body2.x = work->tm->x2 + 0x500;
        work->body3.x = work->tm->x2 - 0x900;
        work->body4.x = work->tm->x2 + 0x400;
    }

    work->body.z = work->tm->z2 - 0x2200;
    work->body2.z = work->tm->z2 + 0x900;
    work->body3.z = work->tm->z2 - 0x1F00;
    work->body4.z = work->tm->z2 - 0x1C00;
}

void BosTmBodyApplyThrowStep(TmBodyWork* work, s16 step) {
    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->angle += sBosTmBodyThrowSteps[step].dAngle;
        work->angle2 += sBosTmBodyThrowSteps[step].dAngle2;
        work->body.x += sBosTmBodyThrowSteps[step].dx << 8;
        work->body2.x += sBosTmBodyThrowSteps[step].dx2 << 8;
        work->body3.x += sBosTmBodyThrowSteps[step].dx3 << 8;
        work->body4.x += sBosTmBodyThrowSteps[step].dx4 << 8;
        work->tm->x2 += sBosTmBodyThrowSteps[step].dx3 << 8;
    } else {
        work->angle += sBosTmBodyThrowSteps[step].dAngle;
        work->angle2 += sBosTmBodyThrowSteps[step].dAngle2;
        work->body.x -= sBosTmBodyThrowSteps[step].dx << 8;
        work->body2.x -= sBosTmBodyThrowSteps[step].dx2 << 8;
        work->body3.x -= sBosTmBodyThrowSteps[step].dx3 << 8;
        work->body4.x -= sBosTmBodyThrowSteps[step].dx4 << 8;
        work->tm->x2 -= sBosTmBodyThrowSteps[step].dx3 << 8;
    }

    work->body.z += sBosTmBodyThrowSteps[step].dz << 8;
    work->body2.z += sBosTmBodyThrowSteps[step].dz2 << 8;
    work->body3.z += sBosTmBodyThrowSteps[step].dz3 << 8;
    work->body4.z += sBosTmBodyThrowSteps[step].dz4 << 8;
    work->tm->z2 += sBosTmBodyThrowSteps[step].dz2 << 8;
    work->gfx3 = gBosTmBodyPart2Frames[sBosTmBodyThrowSteps[step].gfx3Index];
    work->gfx4 = gBosTmBodyPart3Frames[sBosTmBodyThrowSteps[step].gfx4Index];
}

void BosTmBodySetWalkPose(TmBodyWork* work) {
    work->angle = 0;
    work->angle2 = 0;
    work->gfx3 = gBosTmBodyPart2Frames[0];
    work->gfx4 = gBosTmBodyPart3Frames[0];
    work->tm->x2 = work->tm->baseX;
    work->tm->y2 = work->tm->baseY;
    work->tm->z2 = work->tm->baseZ - 0x1500;
    BosTmBodyPlaceParts(work);
}

void BosTmBodyWalk(TmBodyWork* work) {
    if (work->tm->stepTimer == 0) {
        work->tm->baseX += work->tm->vx;
        work->tm->baseY += work->tm->vy;
        work->tm->x2 = work->tm->baseX;
        work->tm->y2 = work->tm->baseY;
        work->tm->z2 += sBosTmBodyWalkZ[work->tm->step] << 8;
        BosTmBodyPlaceParts(work);
    }

    BosTmBodyUpdateAngle(work);
}

void BosTmBodyUpdateRecoil(TmBodyWork* work) {
    s16 recoverStep;
    s16 zIndex;

    if (work->tm->hitCount == 1) {
        work->tm->step = 0;
        work->tm->stepTimer = 0;

        if (work->tm->flags & TM_FLAG_FACING_LEFT) {
            work->angle = sBosTmBodyRecoilSteps[work->tm->step].dAngle;
            work->angle2 = sBosTmBodyRecoilSteps[work->tm->step].dAngle2;
            work->body.x = work->tm->baseX + ((sBosTmBodyRecoilSteps[work->tm->step].dx + 4) << 8);
            work->body2.x = work->tm->baseX + (sBosTmBodyRecoilSteps[work->tm->step].dx2 << 8);
            work->body3.x = work->tm->baseX + ((sBosTmBodyRecoilSteps[work->tm->step].dx3 + 12) << 8);
            work->body4.x = work->tm->baseX + ((sBosTmBodyRecoilSteps[work->tm->step].dx4 - 1) << 8);
            work->tm->x2 = work->tm->baseX + (sBosTmBodyRecoilSteps[work->tm->step].dx3 << 8);
        } else {
            work->angle = sBosTmBodyRecoilSteps[work->tm->step].dAngle;
            work->angle2 = sBosTmBodyRecoilSteps[work->tm->step].dAngle2;
            work->body.x = work->tm->baseX + ((-4 - sBosTmBodyRecoilSteps[work->tm->step].dx) << 8);
            work->body2.x = work->tm->baseX - (sBosTmBodyRecoilSteps[work->tm->step].dx2 << 8);
            work->body3.x = work->tm->baseX + ((-12 - sBosTmBodyRecoilSteps[work->tm->step].dx3) << 8);
            work->body4.x = work->tm->baseX + ((1 - sBosTmBodyRecoilSteps[work->tm->step].dx4) << 8);
            work->tm->x2 = work->tm->baseX - (sBosTmBodyRecoilSteps[work->tm->step].dx3 << 8);
        }

        work->body.z = work->tm->baseZ + ((sBosTmBodyRecoilSteps[work->tm->step].dz - 34 + sBosTmBodyRecoilZ[2]) << 8);
        work->body2.z = work->tm->baseZ + ((sBosTmBodyRecoilSteps[work->tm->step].dz2 + 9 + sBosTmBodyRecoilZ[2]) << 8);
        work->body3.z = work->tm->baseZ + ((sBosTmBodyRecoilSteps[work->tm->step].dz3 - 33 + sBosTmBodyRecoilZ[2]) << 8);
        work->body4.z = work->tm->baseZ + ((sBosTmBodyRecoilSteps[work->tm->step].dz4 - 30 + sBosTmBodyRecoilZ[2]) << 8);
        work->tm->z2 = work->tm->baseZ + ((sBosTmBodyRecoilSteps[work->tm->step].dz2 + 0 + sBosTmBodyRecoilZ[2]) << 8);
    } else {
        if (work->tm->step < 3) {
            if (work->tm->flags & TM_FLAG_FACING_LEFT) {
                work->angle += sBosTmBodyRecoilSteps[work->tm->step].dAngle;
                work->angle2 += sBosTmBodyRecoilSteps[work->tm->step].dAngle2;
                work->body.x += sBosTmBodyRecoilSteps[work->tm->step].dx << 8;
                work->body2.x += sBosTmBodyRecoilSteps[work->tm->step].dx2 << 8;
                work->body3.x += sBosTmBodyRecoilSteps[work->tm->step].dx3 << 8;
                work->body4.x += sBosTmBodyRecoilSteps[work->tm->step].dx4 << 8;
                work->tm->x2 += sBosTmBodyRecoilSteps[work->tm->step].dx3 << 8;
            } else {
                work->angle += sBosTmBodyRecoilSteps[work->tm->step].dAngle;
                work->angle2 += sBosTmBodyRecoilSteps[work->tm->step].dAngle2;
                work->body.x -= sBosTmBodyRecoilSteps[work->tm->step].dx << 8;
                work->body2.x -= sBosTmBodyRecoilSteps[work->tm->step].dx2 << 8;
                work->body3.x -= sBosTmBodyRecoilSteps[work->tm->step].dx3 << 8;
                work->body4.x -= sBosTmBodyRecoilSteps[work->tm->step].dx4 << 8;
                work->tm->x2 -= sBosTmBodyRecoilSteps[work->tm->step].dx3 << 8;
            }

            zIndex = 2 - work->tm->step;
            work->body.z += (sBosTmBodyRecoilSteps[work->tm->step].dz + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->body2.z += (sBosTmBodyRecoilSteps[work->tm->step].dz2 + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->body3.z += (sBosTmBodyRecoilSteps[work->tm->step].dz3 + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->body4.z += (sBosTmBodyRecoilSteps[work->tm->step].dz4 + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->tm->z2 += (sBosTmBodyRecoilSteps[work->tm->step].dz2 + sBosTmBodyRecoilZ[zIndex]) << 8;
        }

        if (work->tm->hurtTimer < 3) {
            recoverStep = work->tm->hurtTimer;

            if (work->tm->flags & TM_FLAG_FACING_LEFT) {
                work->angle -= sBosTmBodyRecoilSteps[recoverStep].dAngle;
                work->angle2 -= sBosTmBodyRecoilSteps[recoverStep].dAngle2;
                work->body.x -= sBosTmBodyRecoilSteps[recoverStep].dx << 8;
                work->body2.x -= sBosTmBodyRecoilSteps[recoverStep].dx2 << 8;
                work->body3.x -= sBosTmBodyRecoilSteps[recoverStep].dx3 << 8;
                work->body4.x -= sBosTmBodyRecoilSteps[recoverStep].dx4 << 8;
                work->tm->x2 -= sBosTmBodyRecoilSteps[recoverStep].dx3 << 8;
            } else {
                work->angle -= sBosTmBodyRecoilSteps[recoverStep].dAngle;
                work->angle2 -= sBosTmBodyRecoilSteps[recoverStep].dAngle2;
                work->body.x += sBosTmBodyRecoilSteps[recoverStep].dx << 8;
                work->body2.x += sBosTmBodyRecoilSteps[recoverStep].dx2 << 8;
                work->body3.x += sBosTmBodyRecoilSteps[recoverStep].dx3 << 8;
                work->body4.x += sBosTmBodyRecoilSteps[recoverStep].dx4 << 8;
                work->tm->x2 += sBosTmBodyRecoilSteps[recoverStep].dx3 << 8;
            }

            zIndex = 2 - recoverStep;
            work->body.z -= (sBosTmBodyRecoilSteps[recoverStep].dz + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->body2.z -= (sBosTmBodyRecoilSteps[recoverStep].dz2 + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->body3.z -= (sBosTmBodyRecoilSteps[recoverStep].dz3 + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->body4.z -= (sBosTmBodyRecoilSteps[recoverStep].dz4 + sBosTmBodyRecoilZ[zIndex]) << 8;
            work->tm->z2 -= (sBosTmBodyRecoilSteps[recoverStep].dz2 + sBosTmBodyRecoilZ[zIndex]) << 8;
        }
    }
}

void BosTmBodyApplySpinStep(TmBodyWork* work, s16 step) {
    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->angle += sBosTmBodySpinSteps[step].dAngle;
        work->angle2 += sBosTmBodySpinSteps[step].dAngle2;
        work->body.x += sBosTmBodySpinSteps[step].dx << 8;
        work->body2.x += sBosTmBodySpinSteps[step].dx2 << 8;
        work->body3.x += sBosTmBodySpinSteps[step].dx3 << 8;
        work->body4.x += sBosTmBodySpinSteps[step].dx4 << 8;
        work->tm->x2 += sBosTmBodySpinSteps[step].dx3 << 8;
    } else {
        work->angle += sBosTmBodySpinSteps[step].dAngle;
        work->angle2 += sBosTmBodySpinSteps[step].dAngle2;
        work->body.x -= sBosTmBodySpinSteps[step].dx << 8;
        work->body2.x -= sBosTmBodySpinSteps[step].dx2 << 8;
        work->body3.x -= sBosTmBodySpinSteps[step].dx3 << 8;
        work->body4.x -= sBosTmBodySpinSteps[step].dx4 << 8;
        work->tm->x2 -= sBosTmBodySpinSteps[step].dx3 << 8;
    }

    work->body.z += sBosTmBodySpinSteps[step].dz << 8;
    work->body2.z += sBosTmBodySpinSteps[step].dz2 << 8;
    work->body3.z += sBosTmBodySpinSteps[step].dz3 << 8;
    work->body4.z += sBosTmBodySpinSteps[step].dz4 << 8;
    work->tm->z2 += sBosTmBodySpinSteps[step].dz2 << 8;
    work->gfx3 = gBosTmBodyPart2Frames[sBosTmBodySpinSteps[step].gfx3Index];
    work->gfx4 = gBosTmBodyPart3Frames[sBosTmBodySpinSteps[step].gfx4Index];
}

s32 GetAbsoluteDifference(s32 x0, s32 x1) {
    if (x0 > x1) {
        return x0 - x1;
    }

    if (x0 < x1) {
        return x1 - x0;
    }

    return 0;
}

void BosTmBodyChooseAction(TmBodyWork* work) {
    s32 st;
    s32 next;
    u16 roll;

    if (work->tm->flags & TM_FLAG_SWITCHING_SIDES) {
        work->tm->state = work->tm->resumeState;
        return;
    }

    if (work->body2.hp < work->body2.maxHp / 2) {
        st = work->tm->tableState;

        if (st == BOS_TM_TABLE_STATE_UP) {
            if (work->tm->flags & TM_FLAG_TABLE_JUST_RAISED) {
                work->tm->state = BOS_TM_STATE_FIRE;
                work->tm->flags = work->tm->flags & ~TM_FLAG_TABLE_JUST_RAISED;
            } else if (GetAbsoluteDifference(gBtlWork->actor->x, work->tm->baseX) <= 0x1DFF) {
                roll = GetRandom() % 100;

                if (roll > 20) {
                    work->tm->state = BOS_TM_STATE_SPIN;
                } else {
                    work->tm->state = BOS_TM_STATE_FIRE;
                }
            } else {
                work->tm->state = sBosTmActionChoices[GetRandom() % 4];

                if (work->tm->state == BOS_TM_STATE_SLAM_GROUND) {
                    work->tm->state = st;
                }

                if (work->tm->state == BOS_TM_STATE_WALK_LEFT) {
                    work->tm->state = BOS_TM_STATE_FIRE;
                }
            }
        } else if (st == BOS_TM_TABLE_STATE_DOWN) {
            if (GetAbsoluteDifference(gBtlWork->actor->x, work->tm->baseX) <= 0x1DFF) {
                roll = GetRandom() % 100;

                if (roll > 30) {
                    work->tm->state = BOS_TM_STATE_SPIN;
                } else {
                    work->tm->state = BOS_TM_STATE_FIRE;
                }
            } else {
                work->tm->state = sBosTmActionChoices[GetRandom() % 4];

                if (work->tm->state == BOS_TM_STATE_WALK_LEFT) {
                    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
                        work->tm->flags |= TM_FLAG_SWITCHING_SIDES;
                        work->tm->state = BOS_TM_STATE_WALK_LEFT;
                        work->tm->resumeState = BOS_TM_STATE_WALK_LEFT;
                    } else {
                        work->tm->flags |= TM_FLAG_SWITCHING_SIDES;
                        work->tm->state = BOS_TM_STATE_WALK_RIGHT;
                        work->tm->resumeState = BOS_TM_STATE_WALK_RIGHT;
                    }

                    next = work->tm->state;

                    if (next == BOS_TM_STATE_SLAM_GROUND) {
                        roll = GetRandom() % 100;

                        if (roll <= 49) {
                            work->tm->state = BOS_TM_STATE_SLAM_GROUND_SLOW;
                        } else {
                            work->tm->state = next;
                        }
                    }
                }
            }
        } else {
            roll = GetRandom() % 100;

            if (roll <= 59) {
                work->tm->state = BOS_TM_STATE_SPIN;
            } else {
                work->tm->state = BOS_TM_STATE_FIRE_TWICE;
            }
        }

        st = work->tm->state;

        if (st == BOS_TM_STATE_FIRE) {
            roll = GetRandom() % 100;

            if (roll <= 59) {
                work->tm->state = st;
            } else {
                work->tm->state = BOS_TM_STATE_FIRE_TWICE;
            }
        }
    } else {
        st = work->tm->tableState;

        if (st == BOS_TM_TABLE_STATE_UP) {
            if (work->tm->flags & TM_FLAG_TABLE_JUST_RAISED) {
                work->tm->state = BOS_TM_STATE_FIRE;
                work->tm->flags = work->tm->flags & ~TM_FLAG_TABLE_JUST_RAISED;
            } else if (GetAbsoluteDifference(gBtlWork->actor->x, work->tm->baseX) <= 0x1DFF) {
                roll = GetRandom() % 100;

                if (roll > 20) {
                    work->tm->state = BOS_TM_STATE_SPIN;
                } else {
                    work->tm->state = BOS_TM_STATE_FIRE;
                }
            } else {
                work->tm->state = sBosTmActionChoices[GetRandom() % 4];

                if (work->tm->state == BOS_TM_STATE_SLAM_GROUND) {
                    work->tm->state = st;
                }

                if (work->tm->state == BOS_TM_STATE_WALK_LEFT) {
                    work->tm->state = BOS_TM_STATE_FIRE;
                }
            }
        } else if (st == BOS_TM_TABLE_STATE_DOWN) {
            if (GetAbsoluteDifference(gBtlWork->actor->x, work->tm->baseX) <= 0x1DFF) {
                roll = GetRandom() % 100;

                if (roll > 30) {
                    work->tm->state = BOS_TM_STATE_SPIN;
                } else {
                    work->tm->state = BOS_TM_STATE_FIRE;
                }
            } else {
                work->tm->state = sBosTmActionChoices[GetRandom() % 4];

                if (work->tm->state == BOS_TM_STATE_WALK_LEFT) {
                    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
                        work->tm->flags |= TM_FLAG_SWITCHING_SIDES;
                        work->tm->state = BOS_TM_STATE_WALK_LEFT;
                        work->tm->resumeState = BOS_TM_STATE_WALK_LEFT;
                    } else {
                        work->tm->flags |= TM_FLAG_SWITCHING_SIDES;
                        work->tm->state = BOS_TM_STATE_WALK_RIGHT;
                        work->tm->resumeState = BOS_TM_STATE_WALK_RIGHT;
                    }
                }
            }
        } else {
            work->tm->state = BOS_TM_STATE_SPIN;
        }
    }
}

void BosTmBodyUpdateReaction(BtlObj* obj, TmBodyWork* work) {
    u16 switchingSides;

    if (obj->hp <= 0) {
        return;
    }

    if (work->tm->baseX < 0x8E00 || work->tm->baseX > 0x16F00) {
        obj->flags |= BTLOBJ_FLAG_INTANGIBLE;
    } else {
        obj->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
    }

    switch (UpdateBtlObjReaction(obj)) {
    case BTL_REACTION_CARD_ACTION:
        BosTmBodyResetTimers(work);
        BosTmBodyRollBossCard(work);
        BosTmBodyChooseAction(work);
        work->tm->flags &= ~TM_FLAG_HURT;
        break;
    case BTL_REACTION_CARD_BROKEN:
        work->tm->stateTimer = 0;
        work->tm->state = BOS_TM_STATE_CARD_BROKEN;
        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        work->hp = obj->hp;
        work->tm->flags |= TM_FLAG_HURT;
        work->tm->hitCount++;

        if (work->prevHp - work->hp >= 9999) {
            work->tm->hurtTimer = 55;
            work->tm->state = BOS_TM_STATE_RECOIL;
            work->tm->flags &= ~TM_FLAG_HURT_NO_RECOIL;
        } else if (work->tm->state != BOS_TM_STATE_RECOIL) {
            work->tm->hurtTimer = 20;
            work->tm->flags |= TM_FLAG_HURT_NO_RECOIL;
        }

        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        BeginBossDefeat(obj);
        work->tm->step = 0;
        work->tm->state = BOS_TM_STATE_DEFEATED;
        break;
    }

    if (work->tm->flags & TM_FLAG_HURT) {
        work->tm->hurtTimer--;

        if (work->tm->hurtTimer <= 0) {
            work->tm->hitCount = 0;
            work->tm->flags &= ~TM_FLAG_HURT;
            ClearBtlObjActionFlags(obj);

            if (work->tm->flags & TM_FLAG_HURT_NO_RECOIL) {
                work->tm->flags &= ~TM_FLAG_HURT_NO_RECOIL;
            } else {
                BosTmBodyResetTimers(work);
                switchingSides = work->tm->flags & TM_FLAG_SWITCHING_SIDES;

                if (switchingSides) {
                    BosTmBodySetWalkPose(work);
                    work->tm->state = BOS_TM_STATE_RESUME_WALK;
                } else {
                    BosTmBodyResetPose(work);
                    work->tm->state = BOS_TM_STATE_IDLE;
                }
            }
        }
    }

    work->prevHp = obj->hp;
}

void task_bos_tm_body_0(TmBodyWork* work, TmWork* arg) {
    work->tiles = LoadObjTiles(gBosTmObjTiles, sizeof(gBosTmObjTiles));
    work->palette = LoadObjPalette(gBoss03objPalettes, sizeof(gBoss03objPalettes));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    work->gfx = gBosTmBodyPart0Frames[0];
    work->gfx2 = gBosTmBodyPart1Frames[0];
    work->gfx3 = gBosTmBodyPart2Frames[0];
    work->gfx4 = gBosTmBodyPart3Frames[0];
    work->tm = arg;
    if (PORT_TILES_OK(work->tiles)) { /* tiles a full pool did not give: none to count (OptiPort v2.8) */
        work->tm->tileIndex = work->tiles->index;
        work->tm->tileCount += work->tiles->count;
    }
    work->tm->paletteIndex = BIOS_PTR(work->palette)->index;
    work->unk_480 = 0;
    work->angleStep = 0;
    work->angleTimer = 0;
    work->unk_486 = 0;
    work->angle = 0;
    work->angle2 = 0;
    work->angle3 = 0;
    work->angle4 = 0;
    work->unk_488 = 0;
    work->hp = 1300;
    work->prevHp = 1300;
    work->unk_48E = 0;
    work->unk_490 = 10;
    work->unk_492 = 0;

    if (work->tm->flags & TM_FLAG_IN_EVENT) {
        BosTmBodySetObjPos(&work->body, work->tm->x + 4, work->tm->y,
                      work->tm->z - 34);
        BosTmBodySetObjPos(&work->body2, work->tm->x, work->tm->y,
                      work->tm->z + 9);
        BosTmBodySetObjPos(&work->body3, work->tm->x + 12, work->tm->y + 1,
                      work->tm->z - 33);
        BosTmBodySetObjPos(&work->body4, work->tm->x - 1, work->tm->y - 4,
                      work->tm->z - 30);
    } else {
        BosTmBodySetObjPos(&work->body, work->tm->x + 4, work->tm->y,
                      work->tm->z - 34);
        BosTmBodyInitEnemy(&work->body2, work->tm->x, work->tm->y,
                      work->tm->z - 16);
        BosTmBodySetObjPos(&work->body3, work->tm->x + 12, work->tm->y + 1,
                      work->tm->z - 33);
        BosTmBodySetObjPos(&work->body4, work->tm->x - 1, work->tm->y - 4,
                      work->tm->z - 30);
        gBosTmBodyObjCopy = work->body2;
    }
}

u8 task_bos_tm_body_1(TmBodyWork* work) {
    s16* idleZ;
    u16 step;
    u16 switchingSides;

    if (!(work->tm->flags & TM_FLAG_IN_EVENT)) {
        BosTmBodyUpdateReaction(&work->body2, work);
    }

    switch (work->tm->state) {
    case BOS_TM_STATE_IDLE:
    case BOS_TM_STATE_EVENT_IDLE:
        if (work->tm->stateTimer == 0) {
            BosTmBodyResetPose(work);
        } else {
            if (gBtlWork->phase && (u16)(GetRandom() % 80) == 0) {
                RequestEnemyCardUse(&work->body2);
            }

            if (work->tm->stepTimer == 0) {
                idleZ = sBosTmBodyIdleZ;
                work->tm->z2 += idleZ[work->tm->step] << 8;
                work->body.z = work->tm->z2 - 0x2200;
                work->body2.z = work->tm->z2 + 0x900;
                work->body3.z = work->tm->z2 - 0x2100;
                work->body4.z = work->tm->z2 - 0x1E00;
            }

            BosTmBodyUpdateAngle(work);
        }

        break;
    case BOS_TM_STATE_WALK_LEFT:
        if (work->tm->stateTimer == 0) {
            BosTmBodySetWalkPose(work);
            work->tm->vx = -0x900;
            work->tm->vy = 0;
        } else {
            BosTmBodyWalk(work);

            if (work->tm->baseX <= 0x8E00) {
                work->tm->flags &= ~TM_FLAG_FACING_LEFT;
                work->body2.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                work->tm->state = BOS_TM_STATE_WALK_LEFT_SETTLE;
                work->tm->resumeState = BOS_TM_STATE_WALK_LEFT_SETTLE;
                BosTmBodyResetTimers(work);
                BosTmBodySetWalkPose(work);
                work->tm->vx = 0x900;
                work->tm->vy = 0;
            }
        }

        break;
    case BOS_TM_STATE_WALK_LEFT_SETTLE:
        if (work->tm->stateTimer == 0) {
            BosTmBodySetWalkPose(work);
            work->tm->vx = 0x900;
            work->tm->vy = 0;
        } else {
            BosTmBodyWalk(work);

            if (work->tm->baseX > 0x9FFF) {
                ClearBtlObjActionFlags(&work->body2);
                work->tm->flags &= ~TM_FLAG_SWITCHING_SIDES;
                work->tm->state = BOS_TM_STATE_IDLE;
                work->tm->resumeState = BOS_TM_STATE_NONE;
                BosTmBodyResetTimers(work);
                BosTmBodyResetPose(work);
            }
        }

        break;
    case BOS_TM_STATE_WALK_RIGHT:
        if (work->tm->stateTimer == 0) {
            BosTmBodySetWalkPose(work);
            work->tm->vx = 0x900;
            work->tm->vy = 0;
        } else {
            BosTmBodyWalk(work);

            if (work->tm->baseX > 0x16EFF) {
                work->tm->flags |= TM_FLAG_FACING_LEFT;
                work->body2.flags |= BTLOBJ_FLAG_FACING_LEFT;
                work->tm->state = BOS_TM_STATE_WALK_RIGHT_SETTLE;
                work->tm->resumeState = BOS_TM_STATE_WALK_RIGHT_SETTLE;
                BosTmBodyResetTimers(work);
                BosTmBodySetWalkPose(work);
                work->tm->vx = -0x900;
                work->tm->vy = 0;
            }
        }

        break;
    case BOS_TM_STATE_WALK_RIGHT_SETTLE:
        if (work->tm->stateTimer == 0) {
            BosTmBodySetWalkPose(work);
            work->tm->vx = -0x900;
            work->tm->vy = 0;
        } else {
            BosTmBodyWalk(work);

            if (work->tm->baseX <= 0x15D00) {
                ClearBtlObjActionFlags(&work->body2);
                work->tm->flags &= ~TM_FLAG_SWITCHING_SIDES;
                work->tm->state = BOS_TM_STATE_IDLE;
                work->tm->resumeState = BOS_TM_STATE_NONE;
                BosTmBodyResetTimers(work);
                BosTmBodyResetPose(work);
            }
        }

        break;
    case BOS_TM_STATE_FIRE:
    case BOS_TM_STATE_FIRE_TWICE:
        if (work->tm->stateTimer == 0) {
            BosTmBodyResetPose(work);
        } else if (work->tm->flags & TM_FLAG_ATTACK_DONE) {
            work->tm->flags &= ~TM_FLAG_ATTACK_DONE;
            ClearBtlObjActionFlags(&work->body2);
            work->tm->state = BOS_TM_STATE_IDLE;
            BosTmBodyResetTimers(work);
            work->tm->z2 = (s16)work->tm->z << 8;
        }

        break;
    case BOS_TM_STATE_SLAM_TABLE:
    case BOS_TM_STATE_SLAM_GROUND:
        if (work->tm->stateTimer == 0) {
            BosTmBodyResetPose(work);
        } else {
            step = work->tm->step;

            if (work->tm->step <= 3) {
                BosTmBodyApplyThrowStep(work, work->tm->step);
            } else if (step >= 66 && step <= 74) {
                BosTmBodyApplyThrowStep(work, step - 62);
            } else if (step >= 98 && step <= 100) {
                BosTmBodyApplyThrowStep(work, step - 85);
            }

            if (work->tm->flags & TM_FLAG_ATTACK_DONE) {
                work->tm->flags &= ~TM_FLAG_ATTACK_DONE;
                ClearBtlObjActionFlags(&work->body2);
                work->tm->state = BOS_TM_STATE_IDLE;
                BosTmBodyResetTimers(work);
                BosTmBodyResetPose(work);
            }
        }

        break;
    case BOS_TM_STATE_SLAM_GROUND_SLOW:
        if (work->tm->stateTimer == 0) {
            BosTmBodyResetPose(work);
        } else {
            step = work->tm->step;

            if (work->tm->step <= 3) {
                BosTmBodyApplyThrowStep(work, work->tm->step);
            } else if (step >= 96 && step <= 104) {
                BosTmBodyApplyThrowStep(work, step - 92);
            } else if (step >= 128 && step <= 130) {
                BosTmBodyApplyThrowStep(work, step - 115);
            }

            if (work->tm->flags & TM_FLAG_ATTACK_DONE) {
                work->tm->flags &= ~TM_FLAG_ATTACK_DONE;
                ClearBtlObjActionFlags(&work->body2);
                work->tm->state = BOS_TM_STATE_IDLE;
                BosTmBodyResetTimers(work);
                BosTmBodyResetPose(work);
            }
        }

        break;
    case BOS_TM_STATE_SPIN:
        if (work->tm->stateTimer == 0) {
            BosTmBodyResetPose(work);
        } else {
            step = work->tm->step;

            if (work->tm->step <= 2) {
                BosTmBodyApplySpinStep(work, work->tm->step);
            } else if (step >= 41 && step <= 46) {
                BosTmBodyApplySpinStep(work, step - 38);
            }

            if (work->tm->flags & TM_FLAG_ATTACK_DONE) {
                work->tm->flags &= ~TM_FLAG_ATTACK_DONE;
                ClearBtlObjActionFlags(&work->body2);
                work->tm->state = BOS_TM_STATE_IDLE;
                BosTmBodyResetTimers(work);
                BosTmBodyResetPose(work);
            }
        }

        break;
    case BOS_TM_STATE_RECOIL:
        BosTmBodyUpdateRecoil(work);
        break;
    case BOS_TM_STATE_CARD_BROKEN:
        if (work->tm->stateTimer == 0) {
            BosTmBodySetBreakPose(work);

            if (work->tm->flags & TM_FLAG_SWITCHING_SIDES) {
                RequestBossCardValue(9);
            }
        } else if (work->tm->stateTimer > 59) {
            BosTmBodyResetTimers(work);
            work->tm->flags &= ~TM_FLAG_GIMMICK_DROP_ROLLED;
            switchingSides = work->tm->flags & TM_FLAG_SWITCHING_SIDES;

            if (switchingSides) {
                ClearBtlObjActionFlags(&work->body2);
                BosTmBodySetBreakPose(work);
                work->tm->state = BOS_TM_STATE_RESUME_WALK;
            } else {
                ClearBtlObjActionFlags(&work->body2);
                BosTmBodyResetPose(work);
                work->tm->state = BOS_TM_STATE_IDLE;
            }
        } else if (!(work->tm->flags & TM_FLAG_GIMMICK_DROP_ROLLED) && work->tm->tableState == BOS_TM_TABLE_STATE_DOWN && (work->tm->flags & TM_FLAG_HURT)) {
            if ((u16)(GetRandom() % 100) <= 30) {
                DropGimmickCard(0, work->tm->baseX, work->tm->baseY, work->tm->baseZ);
                work->tm->flags |= TM_FLAG_GIMMICK_DROP_ROLLED;
            } else {
                work->tm->flags |= TM_FLAG_GIMMICK_DROP_ROLLED;
            }
        }

        break;
    case BOS_TM_STATE_RESUME_WALK:
        RequestEnemyCardUse(&work->body2);
        break;
    case BOS_TM_STATE_DEFEATED:
        if (work->tm->step == 0) {
            BosTmBodySetDefeatPose(work);
        }

        break;
    case BOS_TM_STATE_NONE:
    case BOS_TM_STATE_FROZEN:
        break;
    }

    return 1;
}

void task_bos_tm_body_2(TmBodyWork* work) {
    BtlObj* body;
    BtlObj* body2;
    BtlObj* body3;
    BtlObj* body4;
    ObjAffine* affine;
    ObjAffine* affine2;
    u16 flags;
    void* pal;
    s16 x;
    s16 y;

    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        affine = AllocObjAffine(work->angle, Q_8_8(1), Q_8_8(1), TRUE);
        affine2 = AllocObjAffine(work->angle2, Q_8_8(1), Q_8_8(1), TRUE);
        flags = SPRITE_PRIORITY(2);
    } else {
        affine = AllocObjAffine(work->angle, Q_8_8(-1), Q_8_8(1), TRUE);
        affine2 = AllocObjAffine(work->angle2, Q_8_8(-1), Q_8_8(1), TRUE);
        flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
    }

    if (gBtlWork->paused) {
        pal = work->palette;
    } else if (work->tm->flags & TM_FLAG_HURT) {
        if (gFrameCounter & 1) {
            pal = work->palette2;
        } else {
            pal = work->palette;
        }
    } else {
        pal = work->palette;
    }

    body = &work->body;
    body2 = &work->body2;
    body3 = &work->body3;
    body4 = &work->body4;
    WorldToScreen(&x, &y, body->x, body->y, body->z);
    DrawSprite(x, y, work->gfx, work->tiles, pal, affine, SPRITE_PRIORITY(2),
               -4101 - (body->y >> 8) * 4);
    WorldToScreen(&x, &y, body2->x, body2->y, body2->z - 0x1900);
    DrawSprite(x, y, work->gfx2, work->tiles, pal, affine2, SPRITE_PRIORITY(2),
               -4100 - (body2->y >> 8) * 4);
    WorldToScreen(&x, &y, body3->x, body3->y, body3->z);
    DrawSprite(x, y, work->gfx3, work->tiles, pal, NULL, flags,
               -4100 - (body3->y >> 8) * 4);
    WorldToScreen(&x, &y, body4->x, body4->y, body4->z);
    DrawSprite(x, y, work->gfx4, work->tiles, pal, NULL, flags,
               -4100 - (body4->y >> 8) * 4);
}

void task_bos_tm_body_3(TmBodyWork* work) {
    if ((work->tm->flags & TM_FLAG_IN_EVENT) == 0) {
        BosTmBodyReleaseEnemy(&work->body2);
    }

    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
}

void BosTmBodyRollBossCard(TmBodyWork* work) {
    if (work->body2.hp < work->body2.maxHp / 2) {
        if (GetRandom() % 100 <= 9) {
            RequestBossCardValue(1);
        } else if (GetRandom() % 90 <= 19) {
            RequestBossCardValue(GetRandom() % 2 + 7);
        } else {
            RequestBossCardValue(GetRandom() % 4 + 3);
        }
    } else if (GetRandom() % 100 <= 29) {
        RequestBossCardValue(GetRandom() % 3 + 6);
    } else {
        RequestBossCardValue(GetRandom() % 6 + 1);
    }
}
