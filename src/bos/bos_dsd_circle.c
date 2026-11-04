/**
 * bos_dsd_circle.c
 * Darkside Boss Energy Attacks
 */

#include "bos2.h"
#include "sprites_bos2.h"
#include "sprites_btl.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "display.h"
#include "engine_math.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "enemy_ids.h"

void task_bos_dsd_circle_0(DsdCircleWork* work, void* arg) {
    work->dsd = arg;
    work->x = (gBosDsdCircleOffsetsX[0] << 8) + 0xDC00;
    work->y = (gBosDsdCircleOffsetsY[0] << 8) + 0x16800;
    work->z = 0;
    work->paletteTimer = 0;
    work->paletteFrame = 0;
    work->frame = 0;
    work->summonTimer = 0;
    work->endTimer = 0;
    work->gfx = gBosDsdCircleFrames[0];
}

u8 task_bos_dsd_circle_1(DsdCircleWork* work) {
    DsdWork* dsd = work->dsd;

    if (dsd->state == BOS_DSD_STATE_CARD_BROKEN || dsd->state == BOS_DSD_STATE_RETURN) {
        if (work->endTimer > 66) {
            return 0;
        }

        work->endTimer++;
        return 1;
    }

    switch (dsd->stateStep) {
    case BOS_DSD_SUMMON_STEP_OPEN:
        work->frame = work->dsd->bgFrame - 21;
        work->gfx = gBosDsdCircleFrames[work->frame];
        work->x = (gBosDsdCircleOffsetsX[work->frame] << 8) + 0xDC00;
        work->y = (gBosDsdCircleOffsetsY[work->frame] << 8) + 0x16800;
        break;
    case BOS_DSD_SUMMON_STEP_START_SPAWN:
        break;
    case BOS_DSD_SUMMON_STEP_SPAWN_SHADOWS:
        work->paletteTimer++;

        if (work->paletteTimer >= gBosDsdCirclePaletteDurations[work->paletteFrame]) {
            work->paletteTimer = 0;
            work->paletteFrame++;

            if (work->paletteFrame > 7) {
                work->paletteFrame = 0;
            }

            LoadObjPaletteBank(BIOS_PTR(work->dsd->palette)->index,
                               &gBosDsdCircleCyclePalettes[work->paletteFrame * 16]);
        }

        if (work->summonTimer == 60 || work->summonTimer == 110) {
            SpawnEnemy(ENEMY_SHADOW, work->x + ((GetRandom() % 101 - 50) << 8),
                          work->y + ((GetRandom() % 17 - 8) << 8), 0);
        }

        work->summonTimer++;
        break;
    case BOS_DSD_SUMMON_STEP_STOP_SPAWN:
        LoadObjPaletteBank(BIOS_PTR(work->dsd->palette)->index, gBosDsdCircleCyclePalettes);
        work->frame = work->dsd->bgFrame - 21;
        break;
    case BOS_DSD_SUMMON_STEP_CLOSE:
        work->frame = work->dsd->bgFrame - 21;
        work->gfx = gBosDsdCircleFrames[work->frame];
        work->x = (gBosDsdCircleOffsetsX[work->frame] << 8) + 0xDC00;
        work->y = (gBosDsdCircleOffsetsY[work->frame] << 8) + 0x16800;
        break;
    case BOS_DSD_SUMMON_STEP_WARP_SHADOWS:
        work->frame = 0;
        work->gfx = gBosDsdCircleFrames[work->frame];
        work->x = (gBosDsdCircleOffsetsX[work->frame] << 8) + 0xDC00;
        work->y = (gBosDsdCircleOffsetsY[work->frame] << 8) + 0x16800;
        break;
    case BOS_DSD_SUMMON_STEP_END:
        return 0;
    }

    if (work->dsd->state == BOS_DSD_STATE_DEFEATED) {
        if (BgFxIsActive() == TRUE) {
            BgAnimStop();
        }

        return 0;
    }

    return 1;
}

void task_bos_dsd_circle_2(DsdCircleWork* work) {
    s16 x;
    s16 y;

    WorldToScreen(&x, &y, work->x, work->y, work->z);
    DrawSprite(x, y, work->gfx, work->dsd->tiles, work->dsd->palette, NULL, SPRITE_PRIORITY(3), 0xFFFF);
}

void task_bos_dsd_circle_3() {
}

enum BosDsdEnergy1State {
    BOS_DSD_ENERGY1_STATE_SPAWN,
    BOS_DSD_ENERGY1_STATE_FORM,
    BOS_DSD_ENERGY1_STATE_CHARGE,
    BOS_DSD_ENERGY1_STATE_ARC,
    BOS_DSD_ENERGY1_STATE_HOMING,
    BOS_DSD_ENERGY1_STATE_STRAIGHT
};

void task_bos_dsd_energy1_0(DsdEnergy1Work* work, void* arg) {
    work->dsd = arg;
    work->x = 0xBC00;
    work->y = 0x16800;
    work->z = -0x2400;
    work->unk_10 = 0;
    work->unk_14 = 0;
    work->unk_18 = 0;
    work->angle = 0xF4;
    work->targetAngle = 0xF4;
    work->speed = 0x800;
    work->unk_30 = 0x19;
    work->state = BOS_DSD_ENERGY1_STATE_SPAWN;
    work->unk_36 = 0;
    work->timer = 0;
    work->chargeTime = 0xF;
    work->unk_3A = 0x3C;
    work->visible = FALSE;
    work->vx = gSineTable[work->angle] * work->speed >> 8;
    work->vy = 0;
    work->vz = -gSineTable[work->angle + 0x40] * work->speed >> 8;
    work->gfx = gBtlShadowSmallFrame0;
}

u8 task_bos_dsd_energy1_1(DsdEnergy1Work* work) {
    switch (work->state) {
    case BOS_DSD_ENERGY1_STATE_SPAWN:
        BgFxStartEnemySpawn(work->x, work->y, work->z, Q_8_8(1));
        work->state++;
        break;
    case BOS_DSD_ENERGY1_STATE_FORM:
        if (BgFxIsActive()) {
            break;
        }

        BgFxStartDsdEnergy(work->x, work->y, work->z, Q_8_8(1), work->chargeTime, 0);
        m4aSongNumStart(SONG_SND_701);
        work->state++;
        break;
    case BOS_DSD_ENERGY1_STATE_CHARGE:
        work->timer++;

        if (work->timer >= work->chargeTime) {
            work->visible = TRUE;
            work->timer = 0;
            work->unk_36 = 10;
            work->vy = (gBtlWork->targetY - work->y) / 15;
            work->state++;
        }

        break;
    case BOS_DSD_ENERGY1_STATE_ARC:
        BosDsdEnergy1UpdateArc(work);
        break;
    case BOS_DSD_ENERGY1_STATE_HOMING:
        BosDsdEnergy1UpdateHoming(work);
        break;
    case BOS_DSD_ENERGY1_STATE_STRAIGHT:
        BgFxAddPosition(work->vx, work->vy, work->vz);
        work->x += work->vx;
        work->y += work->vy;
        work->z += work->vz;
        break;
    }

    if (ApplyAttackBox(0x102, work->x, work->y, work->z, 16, 16, 16) == ATTACK_RESULT_HIT) {
        BgFxSignalEnd(0);
        m4aSongNumStart(SONG_EF_RAC_BEEMENTRY);
        work->visible = FALSE;
        return 0;
    }

    if (work->z >= -0x800 || work->x <= -0x2000 || work->x > 0x11FFF ||
        work->dsd->state == BOS_DSD_STATE_CARD_BROKEN || work->dsd->state == BOS_DSD_STATE_DEFEATED) {
        BgFxSignalEnd(0);
        work->visible = FALSE;
        return 0;
    }

    return 1;
}

void task_bos_dsd_energy1_2(DsdEnergy1Work* work) {
    ObjAffine* affine;
    s32 scale;
    s32 doubleSize;
    s16 x;
    s16 y;

    if (work->visible == TRUE) {
        if (work->z >= 0 && gBtlWork->scale == Q_8_8(1)) {
            affine = NULL;
        } else {
            scale = Q_8_8(2) - -work->z / 128;

            if (scale <= 0x7F) {
                scale = Q_8_8(0.5);
            }

            doubleSize = FALSE;

            if (scale > Q_8_8(1)) {
                doubleSize = TRUE;
            }

            affine = AllocObjAffine(0, scale, scale, doubleSize);
        }

        WorldToScreen(&x, &y, work->x, work->y, 0);
        DrawSprite(x, y, work->gfx, work->dsd->tiles3, work->dsd->palette4, affine, SPRITE_PRIORITY(3), 0xFFF0);
    }
}

void task_bos_dsd_energy1_3() {
}

void BosDsdEnergy1UpdateArc(DsdEnergy1Work* work) {
    work->vx = gSineTable[work->angle] * work->speed >> 8;
    work->vz = -gSineTable[work->angle + 0x40] * work->speed >> 8;
    work->speed -= 76;
    work->angle -= 3;
    work->x += work->vx;
    work->y += work->vy;
    work->z += work->vz;
    BgFxAddPosition(work->vx, work->vy, work->vz);

    if (work->timer > 15) {
        work->state++;
    } else {
        work->timer++;
    }
}

void BosDsdEnergy1UpdateHoming(DsdEnergy1Work* work) {
    s16 turn;

    if (work->retargetTimer > 0) {
        work->retargetTimer = 0;
        work->targetAngle = GetAngle(work->x, work->z, gBtlWork->targetX, gBtlWork->targetZ);

        if (work->targetAngle >= work->angle) {
            turn = work->targetAngle - work->angle;

            if (turn > 10) {
                turn = 10;
            }
        } else {
            turn = work->targetAngle - work->angle;

            if (turn < -10) {
                turn = -10;
            }
        }

        work->angle += turn;
        work->vx = gSineTable[work->angle] * work->speed >> 8;
        work->vy = 0;
        work->vz = -gSineTable[work->angle + 0x40] * work->speed >> 8;
    }

    work->retargetTimer++;
    work->speed += 25;
    BgFxAddPosition(work->vx, work->vy, work->vz);
    work->x += work->vx;
    work->y += work->vy;
    work->z += work->vz;
}

enum BosDsdEnergy2State {
    BOS_DSD_ENERGY2_STATE_CHARGE,
    BOS_DSD_ENERGY2_STATE_RISE,
    BOS_DSD_ENERGY2_STATE_BURST,
    BOS_DSD_ENERGY2_STATE_WAIT_BURST,
    BOS_DSD_ENERGY2_STATE_FADE_OUT,
    BOS_DSD_ENERGY2_STATE_SPAWN_DROP,
    BOS_DSD_ENERGY2_STATE_FALL,
    BOS_DSD_ENERGY2_STATE_NEXT_DROP,
    BOS_DSD_ENERGY2_STATE_END
};

void task_bos_dsd_energy2_0(DsdEnergy2Work* work, void* arg) {
    work->dsd = arg;
    work->x = 0xBC00;
    work->y = 0x16800;
    work->z = -0x2C00;
    work->state = BOS_DSD_ENERGY2_STATE_CHARGE;
    work->unk_2E = 0;
    work->timer = 0;
    work->chargeTime = 0xF;
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->vx = 0;
    work->vy = 0;
    work->vz = -0x500;
    work->dropCount = 0;
    work->visible = FALSE;
    work->gfx = gBtlShadowSmallFrame0;
    BgFxStartDsdEnergy(work->x, work->y, work->z, work->scaleX, work->chargeTime, 0);
    m4aSongNumStart(SONG_SND_704);

    switch (work->dsd->hpPhase) {
    case BOS_DSD_HP_PHASE_MID:
        work->dropTotal = 5;
        break;
    case BOS_DSD_HP_PHASE_LOW:
        work->dropTotal = 7;
        break;
    case BOS_DSD_HP_PHASE_HIGH:
    default:
        work->dropTotal = 3;
        break;
    }
}

u8 task_bos_dsd_energy2_1(DsdEnergy2Work* work) {
    BtlObj* actor;

    switch (work->state) {
    case BOS_DSD_ENERGY2_STATE_CHARGE:
        BgFxSetScale(work->scaleX, work->scaleY);
        work->scaleX += Q_8_8(0.1);
        work->scaleY += Q_8_8(0.1);

        if (work->timer >= work->chargeTime) {
            BtlMapSetCameraTarget(work->x, work->y + work->z);
            work->state++;
        } else {
            work->timer++;
        }

        break;
    case BOS_DSD_ENERGY2_STATE_RISE:
        BgFxAddPosition(work->vx, work->vy, work->vz);
        work->x += work->vx;
        work->y += work->vy;
        work->z += work->vz;
        BtlMapSetCameraTarget(work->x, work->y + work->z);

        if (work->z <= -0xF000) {
            work->state++;
        }

        break;
    case BOS_DSD_ENERGY2_STATE_BURST:
        BgFxStartMahluxiaGround(work->x, work->y, work->z, 0x103);
        m4aSongNumStart(SONG_SND_705);
        BtlMapSetCameraTarget(work->x, work->y + work->z);
        work->state++;
        break;
    case BOS_DSD_ENERGY2_STATE_WAIT_BURST:
        BtlMapSetCameraTarget(work->x, work->y + work->z);

        if (!BgFxIsActive()) {
            work->state++;
        }

        break;
    case BOS_DSD_ENERGY2_STATE_FADE_OUT:
        FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        work->state++;
        break;
    case BOS_DSD_ENERGY2_STATE_SPAWN_DROP:
        work->x = (actor = gBtlWork->actor)->x + (-0x4000 + GetRandom() % 0x8001);

        if (work->x < -0xFFF || work->x > 0x10FFF) {
            work->x = actor->x;
        }

        work->y = gBtlWork->actor->y - 0x2400 + GetRandom() % 0x4001;
        work->z = -0xF000;
        work->vz = 0x600;
        BgFxStartDsdEnergy(work->x, work->y, work->z, Q_8_8(1), work->chargeTime, 0);
        work->visible = TRUE;
        work->timer = 0;
        work->state++;
        break;
    case BOS_DSD_ENERGY2_STATE_FALL:
        BgFxAddPosition(0, 0, work->vz);
        work->z += work->vz;

        if (ApplyAttackBox(0x104, work->x, work->y, work->z, 16, 16, 16) == ATTACK_RESULT_HIT) {
            m4aSongNumStart(SONG_BTL_RK_LIMITENTRY);
            BgFxSignalEnd(0);
            work->visible = FALSE;
            work->state = BOS_DSD_ENERGY2_STATE_NEXT_DROP;
        }

        if (work->z >= -0x800) {
            BgFxSignalEnd(0);
            m4aSongNumStart(SONG_SND_703);
            work->visible = FALSE;
            work->state = BOS_DSD_ENERGY2_STATE_NEXT_DROP;
        }

        work->timer++;
        break;
    case BOS_DSD_ENERGY2_STATE_NEXT_DROP:
        if (work->dropCount >= (s8)work->dropTotal - 1) {
            if (!BgFxIsActive()) {
                BgAnimStop();
                FadeToOriginal(FADE_MODE_BLACK, 8);
                work->state++;
            }

            return 1;
        }

        if (work->timer > 49) {
            work->timer = 0;
            work->dropCount++;
            work->state = BOS_DSD_ENERGY2_STATE_SPAWN_DROP;
        } else {
            work->timer++;
        }

        break;
    default:
        return 0;
    }

    if (work->dsd->state == BOS_DSD_STATE_CARD_BROKEN || work->dsd->state == BOS_DSD_STATE_DEFEATED) {
        if (BgFxIsActive() == TRUE) {
            BgAnimStop();
            FadeToOriginal(FADE_MODE_BLACK, 8);
        }

        work->visible = FALSE;
        return 0;
    }

    return 1;
}

void task_bos_dsd_energy2_2(DsdEnergy2Work* work) {
    ObjAffine* affine;
    s32 scale;
    s32 doubleSize;
    s16 x;
    s16 y;

    if (work->visible == TRUE) {
        if (work->z >= 0 && gBtlWork->scale == Q_8_8(1)) {
            affine = NULL;
        } else {
            scale = Q_8_8(2) - -work->z / 128;

            if (scale <= 0x7F) {
                scale = Q_8_8(0.5);
            }

            doubleSize = FALSE;

            if (scale > Q_8_8(1)) {
                doubleSize = TRUE;
            }

            affine = AllocObjAffine(0, scale, scale, doubleSize);
        }

        WorldToScreen(&x, &y, work->x + 0x100, work->y, 0);
        DrawSprite(x, y, work->gfx, work->dsd->tiles3, work->dsd->palette4, affine, SPRITE_PRIORITY(3), 0xFFF0);
    }
}

void task_bos_dsd_energy2_3() {
}

const s8 gBosDsdCirclePaletteDurations[10] = { 6, 12, 10, 9, 7, 8, 9, 10, 0, 0 };

const s16 gBosDsdCircleOffsetsX[9] = { -97, -98, -98, -94, -92, -88, -82, 0, 0 };

const s16 gBosDsdCircleOffsetsY[10] = { 2, 2, 2, 0, 0, 0, -1, 0, 0, 0 };

TaskDesc gTaskDescBosDsdCircle = {
    "task_bos_dsd_circle",
    (TaskInitFunc)task_bos_dsd_circle_0,
    (TaskUpdateFunc)task_bos_dsd_circle_1,
    (TaskDrawFunc)task_bos_dsd_circle_2,
    (TaskDestroyFunc)task_bos_dsd_circle_3,
    sizeof(DsdCircleWork),
};

TaskDesc gTaskDescBosDsdEnergy1 = {
    "task_bos_dsd_energy1",
    (TaskInitFunc)task_bos_dsd_energy1_0,
    (TaskUpdateFunc)task_bos_dsd_energy1_1,
    (TaskDrawFunc)task_bos_dsd_energy1_2,
    (TaskDestroyFunc)task_bos_dsd_energy1_3,
    sizeof(DsdEnergy1Work),
};

TaskDesc gTaskDescBosDsdEnergy2 = {
    "task_bos_dsd_energy2",
    (TaskInitFunc)task_bos_dsd_energy2_0,
    (TaskUpdateFunc)task_bos_dsd_energy2_1,
    (TaskDrawFunc)task_bos_dsd_energy2_2,
    (TaskDestroyFunc)task_bos_dsd_energy2_3,
    sizeof(DsdEnergy2Work),
};
