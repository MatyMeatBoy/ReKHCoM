/**
 * bos_dsd_main.c
 * Darkside Boss Body
 */

#include "bos2.h"
#include "sprites_bos2.h"
#include "sprites_btl.h"
#include "system_state.h"
#include "chara_types.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include "acgtrans.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "chara_api.h"
#include "display.h"
#include "engine_math.h"
#include "gba/defines.h"
#include "listpool.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include "pallet.h"
#include "registration_data.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "default_bg_map.h"
#include "sprite_palettes.h"
#include "enemy_ids.h"

extern const u16* gBosDsdFrameMaps[47][4];
extern void* gBosDsdFrameTiles[47];
extern const u16* gBosDsdMapBlocks[4];

void BosDsdSetBgMap(u8 index) {
    SetBgMapBlocks(1, gBosDsdFrameMaps[index], 2, 2);
}

void BosDsdSetBgFrame(u8 index, u16 tileCount) {
    SetBgMapBlocks(1, gBosDsdFrameMaps[index], 2, 2);
    LoadBgTiles(1, gBosDsdFrameTiles[index], tileCount * 32);
}

void task_bos_dsd_main_0(DsdMainWork* work, DsdWork* arg) {
    BtlObj* body = &work->body;

    work->dsd = arg;
    SetBgPriority(1, 1);
    SetBgPriority(0, 3);
    work->dsd->state = arg->state;
    work->step = 0;
    work->stepTimer = 0;
    work->moveSteps = 0;
    work->baseFrame = 0;
    work->spriteVisible = TRUE;
    work->energy2Task = NULL;
    work->lastBreakDifference = gBtlWork->breakDifference;
    SetBgMapBlocks(1, gBosDsdFrameMaps, 2, 2);
    work->tiles = LoadObjTiles(gBosDsdLimbTiles, sizeof(gBosDsdLimbTiles));
    AnimInit(&work->anim, gBosDsdLegAnims, gBosDsdLegFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    arg->body[0].y++;
    arg->body[0].y--;
    work->tiles2 = LoadObjTiles(gBosDsdLimbTiles, sizeof(gBosDsdLimbTiles));
    AnimInit(&work->anim2, gBosDsdArmAnims, gBosDsdArmFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->gfx2 = AnimGetGfx(&work->anim2);
    work->palette = LoadObjPalette(gBosDsdLimbPalette, sizeof(gBosDsdLimbPalette));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    work->dsd->tiles = AllocObjTiles(0x800, gBosDsdCircleTiles);
    work->dsd->palette = LoadObjPalette(gBosDsdCirclePalette, sizeof(gBosDsdCirclePalette));
    work->dsd->tiles2 = LoadObjTiles(gBosDsdItaTiles, 0x740);
    work->dsd->palette2 = LoadObjPalette(gBosDsdItaPalette, sizeof(gBosDsdItaPalette));
    work->dsd->palette3 = LoadObjPalette(gBosDsdItaShadowPalette, sizeof(gBosDsdItaShadowPalette));
    work->dsd->tiles3 = LoadObjTiles(gBtlShadowSmallTiles, sizeof(gBtlShadowSmallTiles));
    work->dsd->palette4 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    body->x = 0xDC00;
    body->y = 0x16800;
    body->z = 0;
    ColliderInit(&body->collider, COLLIDER_TYPE_BOSS, 24, 100);
    ColliderSetPosition(&body->collider, body->x, body->y, body->z);
    ScrollBgMapTo(1, ((gBtlWork->viewX - arg->body[0].x) >> 8) + 100,
                  ((gBtlWork->viewY - (arg->body[0].y + arg->body[0].z)) >> 8) + 280);
    TaskPoolInit(&work->tasks, 10);
    BosDsdMainResetPose(work);
}

u8 task_bos_dsd_main_1(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* body = &work->body;

    BosDsdMainUpdateDrift(work);

    switch (work->dsd->state) {
    case BOS_DSD_STATE_RETURN:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateReturn(work);
        break;
    case BOS_DSD_STATE_IDLE:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateIdle(work);
        break;
    case BOS_DSD_STATE_ATTACK_START:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateAttackStart(work);
        break;
    case BOS_DSD_STATE_APPROACH:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateApproach(work);
        break;
    case BOS_DSD_STATE_SHOCKWAVE:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateShockwave(work);
        break;
    case BOS_DSD_STATE_SUMMON:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateCircleAttack(work);
        break;
    case BOS_DSD_STATE_ENERGY_HOMING:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateEnergy1Attack(work);
        break;
    case BOS_DSD_STATE_ENERGY_RAIN:
        work->dsd->lastState = work->dsd->state;
        BosDsdMainUpdateEnergy2Attack(work);
        break;
    case BOS_DSD_STATE_CARD_BROKEN:
        BosDsdMainUpdateBreak(work);
        break;
    case BOS_DSD_STATE_EVENT_IDLE:
        BosDsdMainUpdateEventIdle(work);
        break;
    case BOS_DSD_STATE_UNUSED:
        BosDsdMainUpdateState10(work);
        break;
    case BOS_DSD_STATE_DEFEATED:
        BosDsdMainUpdateDefeat(work);
        break;
    }

    ColliderSetPosition(&dsd->body[0].collider, dsd->body[0].x, dsd->body[0].y, dsd->body[0].z);
    ColliderSetPosition(&body->collider, body->x, body->y, body->z);
    TaskPoolUpdate(&work->tasks);
    work->lastBreakDifference = gBtlWork->breakDifference;

    return 1;
}

void task_bos_dsd_main_2(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    void* palette;
    s16 x;
    s16 y;

    if (gBtlWork->paused) {
        LoadPaletteWithEffect(gBosDsdBgPalette, (void*)PLTT, 32);
        palette = work->palette;
    } else if (dsd->flags & DSD_FLAG_HURT) {
        if (gFrameCounter & 1) {
            LoadPaletteWithEffect(gHitFlashPalette, (void*)PLTT, sizeof(gHitFlashPalette));
            palette = work->palette2;
        } else {
            LoadPaletteWithEffect(gBosDsdBgPalette, (void*)PLTT, 32);
            palette = work->palette;
        }
    } else {
        palette = work->palette;
    }

    ScrollBgMapTo(1, ((gBtlWork->viewX - dsd->body[0].x) >> 8) + 100,
                  ((gBtlWork->viewY - (dsd->body[0].y + dsd->body[0].z)) >> 8) + 280);

    if (work->spriteVisible == TRUE) {
        GetBattleSpritePriorityFlags(dsd->body[0].y);
        WorldToScreen(&x, &y, dsd->body[0].x, dsd->body[0].y, -0x6400);
        DrawSprite(x - 96, y + 20, work->gfx, work->tiles, palette, NULL, SPRITE_PRIORITY(1),
                   -4101 - (dsd->body[0].y >> 8) * 4);
        DrawSprite(x - 96, y + 20, work->gfx2, work->tiles2, palette, NULL, SPRITE_PRIORITY(2),
                   -4099 - (dsd->body[0].y >> 8) * 4);
    }

    TaskPoolDraw(&work->tasks);
}

void task_bos_dsd_main_3(DsdMainWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->dsd->tiles);
    ReleaseObjPalette(work->dsd->palette);
    ReleaseObjTiles(work->dsd->tiles2);
    ReleaseObjPalette(work->dsd->palette2);
    ReleaseObjPalette(work->dsd->palette3);
    ReleaseObjTiles(work->dsd->tiles3);
    ReleaseObjPalette(work->dsd->palette4);
    ColliderUnregister(&work->body.collider);
    TaskPoolDestroy(&work->tasks);
}

void BosDsdMainUpdateDrift(DsdMainWork* work) {
    BtlObj* head = &work->dsd->body[1];

    if (head->hp > 0) {
        if (GetRandom() % 30 == 0) {
            TaskCreate(&work->tasks, &gTaskDescBosDsdRock, work->dsd);
        }

        if ((work->dsd->flags & DSD_FLAG_PLAYER_ON_PLATFORM) == 0) {
            /* @bug: no actor yet in the scene before the fight (EVENT_180); the cartridge writes to the BIOS */
            BIOS_PTR(gBtlWork->actor)->x += work->dsd->driftX;
        }

        if (work->lastBreakDifference != gBtlWork->breakDifference) {
            if (gBtlWork->breakDifference > 0) {
                work->dsd->flags |= DSD_FLAG_DRIFT_CHANGED;

                if (gBtlWork->breakDifference > 14) {
                    work->dsd->driftX = 0x180;
                } else {
                    work->dsd->driftX = (gBtlWork->breakDifference << 8) / 10;
                }
            } else if (gBtlWork->breakDifference < 0) {
                work->dsd->flags |= DSD_FLAG_DRIFT_CHANGED;
                work->dsd->driftX = (gBtlWork->breakDifference << 9) / 10;
            } else {
                work->dsd->flags &= ~DSD_FLAG_DRIFT_CHANGED;
            }
        } else {
            work->dsd->flags &= ~DSD_FLAG_DRIFT_CHANGED;
        }
    }
}

void BosDsdMainResetPose(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];

    dsd->bgFrame = 0;
    work->baseFrame = 0;
    work->dsd->bgFrameTimer = 0;
    BosDsdSetBgFrame(0, 0x60);
    work->spriteVisible = TRUE;
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    dsd->body[0].z = -0x6400;
    head->z = -0x8C00;
}

void BosDsdMainUpdateIdleFrames(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];

    if (dsd->bgFrameTimer >= gBosDsdFrameDurations[dsd->bgFrame]) {
        dsd->bgFrameTimer = 0;
        work->dsd->bgFrame++;

        if (work->dsd->bgFrame > work->baseFrame + 7) {
            work->dsd->bgFrame = work->baseFrame;
        }

        BosDsdSetBgFrame(work->dsd->bgFrame, 0x60);
        dsd->body[0].z += gBosDsdIdleBob[work->dsd->bgFrame] << 8;
        head->z += gBosDsdIdleBob[work->dsd->bgFrame] << 8;
    }

    work->dsd->bgFrameTimer++;
    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
}

void BosDsdMainUpdateIdle(DsdMainWork* work) {
    BosDsdMainUpdateIdleFrames(work);

    if (gBtlWork->phase != BTL_PHASE_START) {
        if (GetRandom() % 80 == 0) {
            BosDsdMainChooseAttack(work);
        }
    }
}

void BosDsdMainBeginTransition(DsdMainWork* work, s32 x, s32 y, s32 z) {
    BtlObj* head = &work->dsd->body[1];

    FadeSetPaletteExcluded(0, FALSE);
    FadeSetPaletteExcluded(0x13, FALSE);
    FadeToAmount(FADE_MODE_BLACK, 0x14, 8);
    BgFxStartDsdTransition(x - 0x1400, y, z - 0xA00, Q_8_8(1));
    m4aSongNumStart(SONG_SND_721);
    head->flags |= BTLOBJ_FLAG_UNHITTABLE;
}

void BosDsdMainEndTransition(DsdMainWork* work) {
    BtlObj* head = &work->dsd->body[1];

    FadeToOriginal(FADE_MODE_BLACK, 8);
    FadeSetPaletteExcluded(0, TRUE);
    FadeSetPaletteExcluded(19, TRUE);
    head->flags &= ~BTLOBJ_FLAG_UNHITTABLE;
}

enum BosDsdAttackStartStep {
    BOS_DSD_ATTACK_START_STEP_BEGIN_TRANSITION,
    BOS_DSD_ATTACK_START_STEP_HIDE,
    BOS_DSD_ATTACK_START_STEP_LOAD_TILES,
    BOS_DSD_ATTACK_START_STEP_WAIT_TILES,
    BOS_DSD_ATTACK_START_STEP_END
};

void BosDsdMainUpdateAttackStart(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];
    BtlObj* body = &work->body;

    switch (dsd->stateStep) {
    case BOS_DSD_ATTACK_START_STEP_BEGIN_TRANSITION:
        BosDsdMainBeginTransition(work, dsd->body[0].x, dsd->body[0].y, dsd->body[0].z);
        work->stepTimer = 0;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ATTACK_START_STEP_HIDE:
        work->stepTimer++;

        if (work->stepTimer > 11) {
            DisableBg(1);
            work->spriteVisible = FALSE;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_ATTACK_START_STEP_LOAD_TILES:
        switch (dsd->attackState) {
        case BOS_DSD_STATE_SHOCKWAVE:
        case BOS_DSD_STATE_SUMMON:
            CreateBgTileTransferTask(&work->tasks, 1, 0, 0x2C8, 8, gBosDsdFrame8Tiles);
            break;
        case BOS_DSD_STATE_ENERGY_HOMING:
            CreateBgTileTransferTask(&work->tasks, 1, 0, 0x280, 8, gBosDsdFrame36Tiles);
            break;
        case BOS_DSD_STATE_ENERGY_RAIN:
            CreateBgTileTransferTask(&work->tasks, 1, 0, 0x200, 8, gBosDsdFrame41Tiles);
            break;
        }

        work->stepTimer = 0;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ATTACK_START_STEP_WAIT_TILES:
        work->stepTimer++;

        if (work->stepTimer > 9) {
            work->stepTimer = 0;
            dsd->stateStep++;
        }

        break;
    default:
        work->dsd->stateStep = 0;

        switch (work->dsd->attackState) {
        case BOS_DSD_STATE_SHOCKWAVE:
        case BOS_DSD_STATE_SUMMON:
            body->x = 0xDC00;
            body->y = 0x16800;
            body->z = -0x4000;
            work->dsd->state = BOS_DSD_STATE_APPROACH;
            break;
        case BOS_DSD_STATE_ENERGY_HOMING:
            body->x = 0xBC00;
            body->y = 0x16800;
            body->z = 0;
            work->dsd->state = BOS_DSD_STATE_ENERGY_HOMING;
            break;
        case BOS_DSD_STATE_ENERGY_RAIN:
            body->x = 0xDC00;
            body->y = 0x16800;
            body->z = 0;
            work->dsd->state = BOS_DSD_STATE_ENERGY_RAIN;
            break;
        }

        head->flags &= ~BTLOBJ_FLAG_UNHITTABLE;
        break;
    }
}

enum BosDsdReturnStep {
    BOS_DSD_RETURN_STEP_BEGIN_TRANSITION,
    BOS_DSD_RETURN_STEP_HIDE,
    BOS_DSD_RETURN_STEP_LOAD_TILES,
    BOS_DSD_RETURN_STEP_RESET_POSE,
    BOS_DSD_RETURN_STEP_END_TRANSITION,
    BOS_DSD_RETURN_STEP_END
};

void BosDsdMainUpdateReturn(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];
    BtlObj* body = &work->body;

    switch (dsd->stateStep) {
    case BOS_DSD_RETURN_STEP_BEGIN_TRANSITION:
        BosDsdMainBeginTransition(work, dsd->body[0].x, dsd->body[0].y, dsd->body[0].z);
        work->stepTimer = 0;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_RETURN_STEP_HIDE:
        work->stepTimer++;

        if (work->stepTimer > 4) {
            DisableBg(1);
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_RETURN_STEP_LOAD_TILES:
        CreateBgTileTransferTask(&work->tasks, 1, 0, 0x120, 3, gBosDsdBgTiles);
        work->stepTimer = 0;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_RETURN_STEP_RESET_POSE:
        work->stepTimer++;

        if (work->stepTimer > 4) {
            work->stepTimer = 0;
            BosDsdMainResetPose(work);
            dsd->body[0].x = 0xDC00;
            head->x = 0xDC00;
            body->x = 0xDC00;
            body->y = 0x16800;
            body->z = 0;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_RETURN_STEP_END_TRANSITION:
        if (!BgFxIsActive()) {
            BosDsdMainEndTransition(work);
            work->dsd->stateStep++;
        }

        break;
    default:
        dsd->stateStep = 0;
        work->dsd->state = BOS_DSD_STATE_IDLE;
        break;
    }
}

enum BosDsdApproachStep {
    BOS_DSD_APPROACH_STEP_APPEAR,
    BOS_DSD_APPROACH_STEP_END_TRANSITION,
    BOS_DSD_APPROACH_STEP_MOVE_IN,
    BOS_DSD_APPROACH_STEP_RAISE_ARM,
    BOS_DSD_APPROACH_STEP_BOB_UP,
    BOS_DSD_APPROACH_STEP_BOB_DOWN,
    BOS_DSD_APPROACH_STEP_HOLD,
    BOS_DSD_APPROACH_STEP_SWING,
    BOS_DSD_APPROACH_STEP_REACH,
    BOS_DSD_APPROACH_STEP_END
};

void BosDsdMainUpdateApproach(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];

    switch (dsd->stateStep) {
    case BOS_DSD_APPROACH_STEP_APPEAR:
        work->dsd->bgFrame = 8;
        work->baseFrame = 8;
        work->dsd->bgFrameTimer = 0;
        work->stepTimer = 0;
        work->moveSteps = 30;
        BosDsdSetBgFrame(8, 0x80);
        EnableBg(1);
        dsd->body[0].x = 0x13C00;
        dsd->body[0].y = 0x16800;
        dsd->body[0].z = -0xAC00;
        head->x = 0x13C00;
        head->y = 0x16800;
        head->z = -0xD400;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_APPROACH_STEP_END_TRANSITION:
        if (BgFxIsActive()) {
            break;
        }

        BosDsdMainEndTransition(work);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_APPROACH_STEP_MOVE_IN:
        if ((s16)work->moveSteps > 0) {
            ApproachValue(&dsd->body[0].x, 0xDC00, work->moveSteps);
            ApproachValue(&dsd->body[0].z, -0x9400, work->moveSteps);
            ApproachValue(&head->x, 0xDC00, work->moveSteps);
            ApproachValue(&head->z, -0xBC00, work->moveSteps);
            BtlMapSetCameraTarget(dsd->body[0].x, dsd->body[0].y + dsd->body[0].z);
            work->moveSteps--;
        } else {
            work->stepTimer = 0;
            work->dsd->bgFrame = 9;
            BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
            head->x = 0xEC00;
            head->z = -0x9400;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_APPROACH_STEP_RAISE_ARM:
        work->stepTimer++;

        if (work->stepTimer > 3) {
            work->stepTimer = 0;
            work->dsd->bgFrame = 10;
            BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
            head->x = 0xFC00;
            head->z = -0x9400;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_APPROACH_STEP_BOB_UP:
        work->stepTimer++;

        if (work->stepTimer > 1) {
            work->stepTimer = 0;
            dsd->body[0].x += 0x100;
            dsd->body[0].z += -0x100;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_APPROACH_STEP_BOB_DOWN:
        work->stepTimer++;

        if (work->stepTimer > 1) {
            work->stepTimer = 0;
            dsd->body[0].x += -0x100;
            dsd->body[0].z += 0x100;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_APPROACH_STEP_HOLD:
        work->stepTimer++;

        if (work->stepTimer > 25) {
            work->stepTimer = 0;
            work->dsd->bgFrame = 11;
            BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
            head->x = 0xEC00;
            head->z = -0x9400;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_APPROACH_STEP_SWING:
        work->stepTimer++;

        if (work->stepTimer > 3) {
            work->stepTimer = 0;
            work->dsd->bgFrame = 12;
            BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
            head->x = 0xB400;
            head->z = -0x8400;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_APPROACH_STEP_REACH:
        work->stepTimer++;

        if (work->stepTimer > 3) {
            work->stepTimer = 0;
            work->dsd->stateStep++;
        }

        break;
    default:
        dsd->stateStep = 0;
        work->dsd->state = work->dsd->attackState;
        break;
    }
}

void BosDsdMainLoopFrames(DsdMainWork* work) {
    if (work->dsd->bgFrameTimer >= gBosDsdFrameDurations[work->dsd->bgFrame]) {
        work->dsd->bgFrameTimer = 0;
        work->dsd->bgFrame++;

        if (work->dsd->bgFrame > work->baseFrame + 7) {
            work->dsd->bgFrame = work->baseFrame;
        }

        BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
    }

    work->dsd->bgFrameTimer++;
}

enum BosDsdShockwaveStep {
    BOS_DSD_SHOCKWAVE_STEP_PUNCH,
    BOS_DSD_SHOCKWAVE_STEP_BOB_DOWN,
    BOS_DSD_SHOCKWAVE_STEP_BOB_UP,
    BOS_DSD_SHOCKWAVE_STEP_WAIT_WAVE,
    BOS_DSD_SHOCKWAVE_STEP_HOLD,
    BOS_DSD_SHOCKWAVE_STEP_LIFT,
    BOS_DSD_SHOCKWAVE_STEP_END
};

void BosDsdMainUpdateShockwave(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];
    BtlObj* hand = &dsd->body[2];

    switch (dsd->stateStep) {
    case BOS_DSD_SHOCKWAVE_STEP_PUNCH:
        work->dsd->bgFrame = 13;
        work->baseFrame = 13;
        work->dsd->bgFrameTimer = 0;
        work->stepTimer = 0;
        BosDsdSetBgFrame(13, 0x80);
        hand->x = 0x7600;
        hand->y = 0x16800;
        hand->z = 0;
        hand->flags &= ~BTLOBJ_FLAG_UNHITTABLE;
        BtlMapStartShake();
        m4aSongNumStart(SONG_EF_AIRO);
        BgFxStartLexceusGround(0x7800, 0x16800, 0, 0x100);
        ColliderSetDisabled(&hand->collider, FALSE);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_SHOCKWAVE_STEP_BOB_DOWN:
        BosDsdMainLoopFrames(work);
        work->stepTimer++;

        if (work->stepTimer > 1) {
            work->stepTimer = 0;
            dsd->body[0].x += -0x100;
            dsd->body[0].z += 0x100;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_SHOCKWAVE_STEP_BOB_UP:
        BosDsdMainLoopFrames(work);
        work->stepTimer++;

        if (work->stepTimer > 1) {
            work->stepTimer = 0;
            dsd->body[0].x += 0x100;
            dsd->body[0].z += -0x100;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_SHOCKWAVE_STEP_WAIT_WAVE:
        BosDsdMainLoopFrames(work);

        if (!BgFxIsActive()) {
            ClearBtlObjActionFlags(head);
            work->stepTimer = 0;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_SHOCKWAVE_STEP_HOLD:
        BosDsdMainLoopFrames(work);
        work->stepTimer++;

        if (work->stepTimer > 179) {
            work->stepTimer = 0;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_SHOCKWAVE_STEP_LIFT:
        BosDsdSetBgFrame(8, 0x80);
        hand->flags |= BTLOBJ_FLAG_UNHITTABLE;
        ColliderSetDisabled(&hand->collider, TRUE);
        work->dsd->stateStep++;
        break;
    default:
        dsd->stateStep = 0;
        work->dsd->state = BOS_DSD_STATE_RETURN;
        break;
    }
}

void BosDsdMainUpdateCircleAttack(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];
    BtlObj* hand = &dsd->body[2];
    BtlObj* obj;

    switch (dsd->stateStep) {
    case BOS_DSD_SUMMON_STEP_PLUNGE:
        work->dsd->bgFrame = 21;
        work->baseFrame = 21;
        work->dsd->bgFrameTimer = 0;
        BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
        TaskCreate(&work->tasks, &gTaskDescBosDsdCircle, work->dsd);
        BgFxStartGroundImpact(0x8000, 0x15400);
        ApplyAttackBox(0x101, 0x8000, 0x16800, -0x1400, 16, 16, 16);
        m4aSongNumStart(SONG_EF_DS_BEEM);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_SUMMON_STEP_OPEN:
        if (++work->dsd->bgFrameTimer >= gBosDsdFrameDurations[work->dsd->bgFrame]) {
            work->dsd->bgFrameTimer = 0;
            work->dsd->bgFrame++;
            head->z += 0x600;

            if (work->dsd->bgFrame > work->baseFrame + 6) {
                work->dsd->bgFrame = work->baseFrame + 6;
                work->dsd->stateStep++;
            }

            BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
        }

        break;
    case BOS_DSD_SUMMON_STEP_START_SPAWN:
        work->dsd->bgFrame = 28;
        work->baseFrame = 28;
        work->dsd->bgFrameTimer = 0;
        work->stepTimer = 0;
        ClearBtlObjActionFlags(head);
        hand->x = 0x9000;
        hand->y = 0x16800;
        hand->z = 0;
        hand->flags &= ~BTLOBJ_FLAG_UNHITTABLE;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_SUMMON_STEP_SPAWN_SHADOWS:
        if (++work->dsd->bgFrameTimer >= gBosDsdFrameDurations[work->dsd->bgFrame]) {
            work->dsd->bgFrameTimer = 0;
            work->dsd->bgFrame++;

            if (work->dsd->bgFrame > work->baseFrame + 6) {
                work->dsd->bgFrame = work->baseFrame;
            }

            BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
        }

        work->stepTimer++;

        if (work->stepTimer > 299) {
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_SUMMON_STEP_STOP_SPAWN:
        work->dsd->bgFrame = 27;
        work->baseFrame = 27;
        work->dsd->bgFrameTimer = 0;
        work->stepTimer = 0;
        hand->flags |= BTLOBJ_FLAG_UNHITTABLE;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_SUMMON_STEP_CLOSE:
        if (++work->dsd->bgFrameTimer >= gBosDsdFrameDurations[work->dsd->bgFrame]) {
            work->dsd->bgFrameTimer = 0;
            work->dsd->bgFrame--;
            head->z -= 0x600;

            if (work->dsd->bgFrame < work->baseFrame - 6) {
                work->dsd->bgFrame = work->baseFrame - 6;

                if (gBtlWork->enemyTileCount <= 0 && (work->dsd->flags & DSD_FLAG_PLATFORM_ACTIVE) == 0) {
                    DropGimmickCard(0, dsd->body[0].x, dsd->body[0].y, dsd->body[0].z);
                }

                work->dsd->stateStep++;
            }

            BosDsdSetBgFrame(work->dsd->bgFrame, 0x80);
        }

        break;
    case BOS_DSD_SUMMON_STEP_WARP_SHADOWS:
        work->stepTimer = 0;
        BosDsdSetBgFrame(8, 0x80);
        obj = ListPoolFirst(&gBtlWork->pool);

        while (obj != NULL) {
            if (obj->kind == ENEMY_SHADOW) {
                obj->flags |= BTLOBJ_FLAG_WARP_PENDING;
                m4aSongNumStart(SONG_BTL_DARKDEAD);
            }

            obj = ListPoolNext(&obj->node);
        }

        work->dsd->stateStep++;
        break;
    default:
        dsd->stateStep = 0;
        work->dsd->state = BOS_DSD_STATE_RETURN;
        break;
    }
}

void BosDsdMainLoopMapFrames(DsdMainWork* work) {
    if (work->dsd->bgFrameTimer >= gBosDsdFrameDurations[work->dsd->bgFrame]) {
        work->dsd->bgFrameTimer = 0;
        work->dsd->bgFrame++;

        if (work->dsd->bgFrame > work->baseFrame + 4) {
            work->dsd->bgFrame = work->baseFrame;
        }

        BosDsdSetBgMap(work->dsd->bgFrame);
    }

    work->dsd->bgFrameTimer++;
}

enum BosDsdEnergyHomingStep {
    BOS_DSD_ENERGY_HOMING_STEP_APPEAR,
    BOS_DSD_ENERGY_HOMING_STEP_END_TRANSITION,
    BOS_DSD_ENERGY_HOMING_STEP_LAUNCH,
    BOS_DSD_ENERGY_HOMING_STEP_WAIT_FIRST,
    BOS_DSD_ENERGY_HOMING_STEP_WAIT_SECOND,
    BOS_DSD_ENERGY_HOMING_STEP_WAIT_THIRD,
    BOS_DSD_ENERGY_HOMING_STEP_END_ACTION,
    BOS_DSD_ENERGY_HOMING_STEP_END
};

void BosDsdMainUpdateEnergy1Attack(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];

    switch (dsd->stateStep) {
    case BOS_DSD_ENERGY_HOMING_STEP_APPEAR:
        work->dsd->bgFrame = 36;
        work->baseFrame = 36;
        work->dsd->bgFrameTimer = 0;
        work->moveSteps = 30;
        work->spriteVisible = FALSE;
        BosDsdSetBgMap(work->dsd->bgFrame);
        head->x = 0xDC00;
        head->z = -0x6000;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_HOMING_STEP_END_TRANSITION:
        if (BgFxIsActive()) {
            break;
        }

        BosDsdMainEndTransition(work);
        work->stepTimer = 0;
        work->moveSteps = 0;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_HOMING_STEP_LAUNCH:
        BosDsdMainLoopMapFrames(work);
        work->energy1Task = TaskCreate(&work->tasks, &gTaskDescBosDsdEnergy1, work->dsd);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_HOMING_STEP_WAIT_FIRST:
        BosDsdMainLoopMapFrames(work);

        if (IsTaskActive(work->energy1Task)) {
            break;
        }

        if (work->dsd->hpPhase == BOS_DSD_HP_PHASE_HIGH) {
            work->dsd->stateStep = BOS_DSD_ENERGY_HOMING_STEP_END_ACTION;
            break;
        }

        work->stepTimer++;

        if (work->stepTimer > 4) {
            work->stepTimer = 0;
            work->energy1Task2 = TaskCreate(&work->tasks, &gTaskDescBosDsdEnergy1, work->dsd);
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_ENERGY_HOMING_STEP_WAIT_SECOND:
        BosDsdMainLoopMapFrames(work);

        if (IsTaskActive(work->energy1Task2)) {
            break;
        }

        if (work->dsd->hpPhase == BOS_DSD_HP_PHASE_MID) {
            work->dsd->stateStep = BOS_DSD_ENERGY_HOMING_STEP_END_ACTION;
            break;
        }

        work->stepTimer++;

        if (work->stepTimer > 4) {
            work->stepTimer = 0;
            work->energy1Task3 = TaskCreate(&work->tasks, &gTaskDescBosDsdEnergy1, work->dsd);
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_ENERGY_HOMING_STEP_WAIT_THIRD:
        BosDsdMainLoopMapFrames(work);

        if (IsTaskActive(work->energy1Task3)) {
            break;
        }

        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_HOMING_STEP_END_ACTION:
        BosDsdMainLoopMapFrames(work);
        ClearBtlObjActionFlags(head);
        work->dsd->stateStep++;
        break;
    default:
        dsd->stateStep = 0;
        work->dsd->state = BOS_DSD_STATE_RETURN;
        break;
    }
}

enum BosDsdEnergyRainStep {
    BOS_DSD_ENERGY_RAIN_STEP_APPEAR,
    BOS_DSD_ENERGY_RAIN_STEP_END_TRANSITION,
    BOS_DSD_ENERGY_RAIN_STEP_LAUNCH,
    BOS_DSD_ENERGY_RAIN_STEP_WAIT,
    BOS_DSD_ENERGY_RAIN_STEP_END_ACTION,
    BOS_DSD_ENERGY_RAIN_STEP_END
};

void BosDsdMainUpdateEnergy2Attack(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];

    switch (dsd->stateStep) {
    case BOS_DSD_ENERGY_RAIN_STEP_APPEAR:
        work->dsd->bgFrame = 41;
        work->baseFrame = 41;
        work->dsd->bgFrameTimer = 0;
        work->moveSteps = 30;
        work->spriteVisible = FALSE;
        BosDsdSetBgMap(work->dsd->bgFrame);
        work->stepTimer = 0;
        head->x = 0xE400;
        head->z = -0x5C00;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_RAIN_STEP_END_TRANSITION:
        if (BgFxIsActive()) {
            break;
        }

        BosDsdMainEndTransition(work);
        work->moveSteps = 0;
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_RAIN_STEP_LAUNCH:
        BosDsdMainLoopMapFrames(work);
        LoadPalette(gBosDsdBgPalette, (void*)PLTT, 32);
        work->energy2Task = TaskCreate(&work->tasks, &gTaskDescBosDsdEnergy2, work->dsd);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_RAIN_STEP_WAIT:
        BosDsdMainLoopMapFrames(work);

        if (IsTaskActive(work->energy2Task)) {
            break;
        }

        work->dsd->stateStep++;
        break;
    case BOS_DSD_ENERGY_RAIN_STEP_END_ACTION:
        BosDsdMainLoopMapFrames(work);
        ClearBtlObjActionFlags(head);
        work->dsd->stateStep++;
        break;
    default:
        dsd->stateStep = 0;
        work->dsd->state = BOS_DSD_STATE_RETURN;
        break;
    }
}

void BosDsdMainUpdateState10(DsdMainWork* work) {
}

void BosDsdMainUpdateBreak(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];
    BtlObj* hand = &dsd->body[2];

    if (dsd->lastState == BOS_DSD_STATE_ATTACK_START || dsd->lastState == BOS_DSD_STATE_APPROACH) {
        ClearBtlObjActionFlags(head);
        hand->flags |= BTLOBJ_FLAG_UNHITTABLE;
        ColliderSetDisabled(&hand->collider, TRUE);
        work->dsd->stateStep = 0;
        work->dsd->state = BOS_DSD_STATE_RETURN;
    } else if (dsd->stateStep > 60) {
        ClearBtlObjActionFlags(head);
        hand->flags |= BTLOBJ_FLAG_UNHITTABLE;
        ColliderSetDisabled(&hand->collider, TRUE);
        work->dsd->stateStep = 0;
        work->dsd->state = BOS_DSD_STATE_RETURN;
    } else {
        dsd->stateStep++;
    }
}

enum BosDsdDefeatStep {
    BOS_DSD_DEFEAT_STEP_WARP_SHADOWS,
    BOS_DSD_DEFEAT_STEP_BEGIN_DEFEAT,
    BOS_DSD_DEFEAT_STEP_HIDE,
    BOS_DSD_DEFEAT_STEP_LOAD_FRAME,
    BOS_DSD_DEFEAT_STEP_END_TRANSITION,
    BOS_DSD_DEFEAT_STEP_DELAY,
    BOS_DSD_DEFEAT_STEP_START_DISSOLVE,
    BOS_DSD_DEFEAT_STEP_DISSOLVE,
    BOS_DSD_DEFEAT_STEP_END
};

void BosDsdMainUpdateDefeat(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];
    BtlObj* hand = &dsd->body[2];
    BtlObj* body = &work->body;
    BtlObj* obj;
    CharaObjParam param;

    switch (dsd->stateStep) {
    case BOS_DSD_DEFEAT_STEP_WARP_SHADOWS:
        obj = ListPoolFirst(&gBtlWork->pool);

        while (obj != NULL) {
            if (obj->kind == ENEMY_SHADOW) {
                obj->flags |= BTLOBJ_FLAG_WARP_PENDING;
                m4aSongNumStart(SONG_BTL_DARKDEAD);
            }

            obj = ListPoolNext(&obj->node);
        }

        work->dsd->stateStep++;
        break;
    case BOS_DSD_DEFEAT_STEP_BEGIN_DEFEAT:
        BeginBossDefeat(head);
        ColliderSetDisabled(&head->collider, TRUE);
        ColliderSetDisabled(&hand->collider, TRUE);
        ColliderSetDisabled(&body->collider, TRUE);
        FadeSetPaletteExcluded(0, FALSE);
        FadeSetPaletteExcluded(19, FALSE);
        FadeToAmount(FADE_MODE_BLACK, 20, 8);
        BgFxStartDsdTransition(head->x - 0x1400, head->y, head->z - 0xA00, Q_8_8(1));
        m4aSongNumStart(SONG_SND_721);
        work->stepTimer = 0;
        work->dsd->stateStep++;
        BtlMapSetCameraTarget(dsd->body[0].x - 0x1400, dsd->body[0].y + dsd->body[0].z + 0x3000);
        break;
    case BOS_DSD_DEFEAT_STEP_HIDE:
        BtlMapSetCameraTarget(dsd->body[0].x - 0x1400, dsd->body[0].y + dsd->body[0].z + 0x3000);
        work->stepTimer++;

        if (work->stepTimer > 11) {
            DisableBg(1);
            work->spriteVisible = FALSE;
            work->dsd->stateStep++;
        }

        break;
    case BOS_DSD_DEFEAT_STEP_LOAD_FRAME:
        BosDsdSetBgFrame(36, 0xE0);
        EnableBg(1);
        BtlMapSetCameraTarget(dsd->body[0].x - 0x1400, dsd->body[0].y + dsd->body[0].z + 0x3000);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_DEFEAT_STEP_END_TRANSITION:
        BtlMapSetCameraTarget(dsd->body[0].x - 0x1400, dsd->body[0].y + dsd->body[0].z + 0x3000);

        if (BgFxIsActive()) {
            break;
        }

        FadeToOriginal(FADE_MODE_BLACK, 8);
        FadeSetPaletteExcluded(0, TRUE);
        FadeSetPaletteExcluded(19, TRUE);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_DEFEAT_STEP_DELAY:
        BtlMapSetCameraTarget(dsd->body[0].x - 0x1400, dsd->body[0].y + dsd->body[0].z + 0x3000);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_DEFEAT_STEP_START_DISSOLVE:
        BtlMapSetCameraTarget(dsd->body[0].x - 0x1400, dsd->body[0].y + dsd->body[0].z + 0x3000);
        param.tilesAddr = 0;
        param.tileCount = 0;
        param.tilesAddr2 = 0;
        param.tileCount2 = 0;
        param.tilesAddr3 = 0;
        param.tileCount3 = 0;
        param.paletteAddr = 0;
        param.paletteSize = 0;
        param.tilesAddr4 = VRAM;
        param.tileCount4 = 0xE0;
        param.paletteAddr2 = PLTT;
        param.paletteSize2 = 32;
        param.x = dsd->body[0].x - 0x1400;
        param.y = dsd->body[0].y;
        param.z = dsd->body[0].z + 0x3000;
        param.callback = NULL;
        param.prizeObj = head;
        param.flags = 1;
        CharaObjInitDefeat(&param);
        work->dsd->stateStep++;
        break;
    case BOS_DSD_DEFEAT_STEP_DISSOLVE:
        if (!CharaObjUpdateDefeat()) {
            dsd->body[0].x = 300;
            dsd->body[0].y = 0;
            dsd->body[0].z = 0;
            ScrollBgMapTo(1, ((gBtlWork->viewX - 300) >> 8) + 100, (gBtlWork->viewY >> 8) + 280);
            EndBossDefeat();
            work->dsd->stateStep++;
        } else {
            BtlMapSetCameraTarget(dsd->body[0].x - 0x1800, dsd->body[0].y + dsd->body[0].z + 0x3000);
        }

        break;
    default:
        dsd->flags |= DSD_FLAG_DEFEAT_DONE;
        break;
    }
}

void BosDsdMainUpdateEventIdle(DsdMainWork* work) {
    BosDsdMainUpdateIdleFrames(work);
}

void BosDsdMainChooseAttack(DsdMainWork* work) {
    DsdWork* dsd = work->dsd;
    BtlObj* head = &dsd->body[1];

    if (head->hp < head->maxHp / 3) {
        dsd->hpPhase = BOS_DSD_HP_PHASE_LOW;

        if (GetRandom() % 100 <= 9) {
            RequestBossCardValue(1);
        } else if (GetRandom() % 90 <= 49) {
            RequestBossCardValue(GetRandom() % 2 + 8);
        } else {
            RequestBossCardValue(GetRandom() % 3 + 5);
        }
    } else if (head->hp < head->maxHp / 3 * 2) {
        dsd->hpPhase = BOS_DSD_HP_PHASE_MID;

        if (GetRandom() % 100 <= 29) {
            RequestBossCardValue(GetRandom() % 2 + 8);
        } else {
            RequestBossCardValue(GetRandom() % 3 + 4);
        }
    } else {
        dsd->hpPhase = BOS_DSD_HP_PHASE_HIGH;

        if (GetRandom() % 100 <= 59) {
            RequestBossCardValue(GetRandom() % 3 + 7);
        } else {
            RequestBossCardValue(GetRandom() % 4 + 3);
        }
    }

    switch (work->dsd->attackCycle) {
    case 0:
        work->dsd->attackState = BOS_DSD_STATE_SUMMON;
        break;
    case 1:
        work->dsd->attackState = BOS_DSD_STATE_ENERGY_HOMING;
        break;
    case 2:
        work->dsd->attackState = BOS_DSD_STATE_SHOCKWAVE;
        break;
    case 3:
        work->dsd->attackState = BOS_DSD_STATE_ENERGY_RAIN;
        break;
    default:
        work->dsd->attackState = BOS_DSD_STATE_RETURN;
        break;
    }

    work->dsd->attackCycle++;

    if (work->dsd->attackCycle > 3) {
        work->dsd->attackCycle = 0;
    }

    RequestEnemyCardUse(head);
}

void task_bos_dsd_map_0() {
    LoadBgTiles(0, gBosDsdBgTiles, sizeof(gBosDsdBgTiles));
    LoadBgPalette(0, gBosDsdBgPalette, sizeof(gBosDsdBgPalette));
    SetBgMapBlocks(0, gBosDsdMapBlocks, 2, 2);
    gBtlWork->scale = Q_8_8(1);
    gBtlWork->zoomScale = Q_8_8(1);
    gBtlWork->x = 0xA000;
    gBtlWork->y = 0x13600;
    gBtlWork->viewX = 0xA000;
    gBtlWork->viewY = 0x13600;
    gBtlWork->x2 = 0xA000;
    gBtlWork->y2 = 0x13600;
    gBtlWork->zoomX = 0xA000;
    gBtlWork->zoomY = 0x13600;
    gBtlWork->zoomSteps = 0xF;
    gBtlWork->rotation = 0;
    BtlMapResetShake();
    ScrollBgMapTo(0, gBtlWork->viewX >> 8, gBtlWork->viewY >> 8);
}

u8 task_bos_dsd_map_1() {
    s32 dx;
    s32 dy;

    BtlMapUpdateShake();
    dx = (gBtlWork->x2 - gBtlWork->x) >> 3;
    dy = (gBtlWork->y2 - gBtlWork->y) >> 3;

    if (dx > 0x500) {
        dx = 0x500;
    } else if (dx < -0x500) {
        dx = -0x500;
    }

    gBtlWork->x += dx;
    gBtlWork->y += dy;
    gBtlWork->viewX = gBtlWork->x;
    gBtlWork->viewY = gBtlWork->y;

    if (gBtlWork->viewX < (gBtlWork->xMin + DISPLAY_WIDTH / 2) << 8) {
        gBtlWork->viewX = (gBtlWork->xMin + DISPLAY_WIDTH / 2) << 8;
    } else if (gBtlWork->viewX > (gBtlWork->xMax - DISPLAY_WIDTH / 2) << 8) {
        gBtlWork->viewX = (gBtlWork->xMax - DISPLAY_WIDTH / 2) << 8;
    }

    if (gBtlWork->viewY < ((DISPLAY_HEIGHT / 2) << 8)) {
        gBtlWork->viewY = ((DISPLAY_HEIGHT / 2) << 8);
    } else if (gBtlWork->viewY > (gBtlWork->yMax - DISPLAY_HEIGHT / 2) << 8) {
        gBtlWork->viewY = (gBtlWork->yMax - DISPLAY_HEIGHT / 2) << 8;
    }

    gBtlWork->viewY += BtlMapGetShake();
    ScrollBgMapTo(0, (gBtlWork->viewX >> 8) - DISPLAY_WIDTH / 2, (gBtlWork->viewY >> 8) - 0x28);

    return 1;
}

const s16 gBosDsdFrameDurations[47] = {
    10, 10, 10, 10, 10, 10, 10, 10, 4, 2, 2, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 4, 4, 4, 4, 4, 4, 4, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 0,
};

const s8 gBosDsdIdleBob[10] = { -1, 1, 1, 1, 1, -1, -1, -1, 0, 0 };

const u16* gBosDsdFrameMaps[47][4] = {
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame0Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame1Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame2Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame3Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame4Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame3Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame2Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame1Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame8Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame9Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame10Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame9Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame8Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame13Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame14Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame15Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame16Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame17Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame16Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame15Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame14Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame21Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame22Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame23Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame24Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame25Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame26Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame27Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame28Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame29Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame30Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame31Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame32Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame31Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame30Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame29Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame36Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame37Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame38Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame39Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame40Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame41Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame42Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame43Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame44Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gBosDsdFrame45Map, gDefaultBgMap },
    { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gDefaultBgMap },
};

void* gBosDsdFrameTiles[47] = {
    gBosDsdBgTiles,
    gBosDsdFrame1Tiles,
    gBosDsdFrame2Tiles,
    gBosDsdFrame3Tiles,
    gBosDsdFrame4Tiles,
    gBosDsdFrame3Tiles,
    gBosDsdFrame2Tiles,
    gBosDsdFrame1Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame14Tiles,
    gBosDsdFrame15Tiles,
    gBosDsdFrame16Tiles,
    gBosDsdFrame17Tiles,
    gBosDsdFrame16Tiles,
    gBosDsdFrame15Tiles,
    gBosDsdFrame14Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame8Tiles,
    gBosDsdFrame14Tiles,
    gBosDsdFrame15Tiles,
    gBosDsdFrame16Tiles,
    gBosDsdFrame17Tiles,
    gBosDsdFrame16Tiles,
    gBosDsdFrame15Tiles,
    gBosDsdFrame14Tiles,
    gBosDsdFrame36Tiles,
    gBosDsdFrame36Tiles,
    gBosDsdFrame36Tiles,
    gBosDsdFrame36Tiles,
    gBosDsdFrame36Tiles,
    gBosDsdFrame41Tiles,
    gBosDsdFrame41Tiles,
    gBosDsdFrame41Tiles,
    gBosDsdFrame41Tiles,
    gBosDsdFrame41Tiles,
    NULL,
};

TaskDesc gTaskDescBosDsdMain = {
    "task_bos_dsd_main",
    (TaskInitFunc)task_bos_dsd_main_0,
    (TaskUpdateFunc)task_bos_dsd_main_1,
    (TaskDrawFunc)task_bos_dsd_main_2,
    (TaskDestroyFunc)task_bos_dsd_main_3,
    sizeof(DsdMainWork),
};

const u16* gBosDsdMapBlocks[4] = {
    gBosDsdBgMap0,
    gDefaultBgMap,
    gBosDsdBgMap2,
    gDefaultBgMap,
};

TaskDesc gTaskDescBosDsdMap = {
    "task_bos_dsd_map",
    (TaskInitFunc)task_bos_dsd_map_0,
    (TaskUpdateFunc)task_bos_dsd_map_1,
    NULL,
    NULL,
    sizeof(DsdMapWork),
};
