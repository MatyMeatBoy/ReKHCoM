/**
 * bos_jf_majin.c
 * Genie Jafar Body
 */

#include "macros.h"
#include "bos2.h"
#include "sprites_bos2.h"
#include "system_state.h"
#include "prize_types.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "display.h"
#include "engine_math.h"
#include "gba/defines.h"
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

const void* gBosJfMajinMapBlockTable[4] EWRAM_COMMON(16);
const void* gBosJfMajinMapBlocks EWRAM_COMMON(4);
u8 gUnk_0203B504[12] EWRAM_COMMON(4);
u8 gBosJfMajinMapBuffer[0x800] EWRAM_COMMON(16);

void BosJfMajinSetBgFrame(u8 frame, u16 tileCount, JfMajinWork* work) {
    BosJfMajinCopyBgMap(frame, work);
    SetBgMapBlocks(1, gBosJfMajinMapBlocks, 2, 2);
    LoadBgTiles(1, gBosJfMajinFrameTiles[frame], tileCount * 32);
    work->jf->flags &= ~JF_FLAG_NEEDS_BG_CLIP;
}

void BosJfMajinCopyBgMap(u8 frame, JfMajinWork* work) {
    s16 n;

    if (work->jf->body.z < -0x8000) {
        RequestDma3Copy(gBosJfMajinFrameMaps[frame], gBosJfMajinMapBuffer, sizeof(gBosJfMajinMapBuffer));
    } else {
        n = ((work->jf->body.z >> 8) + 0x88) / 8 + work->extraClipRows;

        if (n > 0x20) {
            RequestDma3Clear(gBosJfMajinMapBuffer, sizeof(gBosJfMajinMapBuffer));
        } else {
            RequestDma3Copy(gBosJfMajinFrameMaps[frame], gBosJfMajinMapBuffer, (0x20 - n) * 64);
            RequestDma3Clear(gBosJfMajinMapBuffer + (0x20 - n) * 64, n * 64);
        }
    }
}

void BosJfMajinUpdateBgClip(u8 frame, JfMajinWork* work) {
    s16 n;

    if (work->jf->body.z >= -0x8000) {
        n = ((work->jf->body.z >> 8) + 0x88) / 8 + work->extraClipRows;

        if (n > 0x20) {
            RequestDma3Clear(gBosJfMajinMapBuffer, sizeof(gBosJfMajinMapBuffer));
        } else {
            RequestDma3Copy(gBosJfMajinFrameMaps[frame], gBosJfMajinMapBuffer, (0x20 - n) * 64);
            RequestDma3Clear(gBosJfMajinMapBuffer + (0x20 - n) * 64, n * 64);
        }

        SetBgMapBlocks(1, gBosJfMajinMapBlocks, 2, 2);
    }
}

#define GET_ACTOR_POSITION(actor, out_x, out_y, out_z) do { \
    (out_x) = (actor)->x; \
    (out_y) = (actor)->y; \
    (out_z) = (actor)->z; \
} while (0)
void task_bos_jf_majin_0(JfMajinWork* work, void* jf) {
    JfWork* boss = jf;
    s32 x;
    union {
        s32 coordinate;
        BtlWork* bounds;
    } y;
    s32 z;

    work->jf = boss;
    GET_ACTOR_POSITION(&boss->body, x, y.coordinate, z);
    work->step = 0;
    work->stepTimer = 0;
    work->moveSteps = 0;
    work->x = x;
    work->y2 = y.coordinate;
    work->z = z;
    work->baseFrame = 0;
    work->leftLevel = gBosJfLeftPillarLevel;
    work->middleLevel = gBosJfMiddlePillarLevel;
    work->rightLevel = gBosJfRightPillarLevel;
    work->leftTarget = gBosJfLeftPillarLevel;
    work->middleTarget = gBosJfMiddlePillarLevel;
    work->rightTarget = gBosJfRightPillarLevel;
    work->beamAngle = 0;
    work->beamLength = 0;
    work->beamScale = Q_8_8(1.2);
    work->extraClipRows = 0;
    gBosJfMajinMapBlockTable[0] = gDefaultBgMap;
    gBosJfMajinMapBlockTable[1] = gDefaultBgMap;
    gBosJfMajinMapBlockTable[2] = gDefaultBgMap;
    gBosJfMajinMapBlockTable[3] = gBosJfMajinMapBuffer;
    RequestDma3Copy(gBosJfMajinFrame8Map, gBosJfMajinMapBuffer, sizeof(gBosJfMajinFrame8Map));
    gBosJfMajinMapBlocks = gBosJfMajinMapBlockTable;
    LoadBgPalette(1, gBosJfMajinPalette, sizeof(gBosJfMajinPalette));
    LoadBgTiles(1, gBosJfMajinFrame8Tiles, sizeof(gBosJfMajinFrame8Tiles));
    SetBgMapBlocks(1, gBosJfMajinMapBlocks, 2, 2);
    work->tiles = LoadObjTiles(gBosJfObjTiles, sizeof(gBosJfObjTiles));
    work->palette = LoadObjPalette(gBosJfObjPalette, sizeof(gBosJfObjPalette));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    work->spriteVisible = TRUE;
    work->unk_30 = 0x2A200;
    work->unk_34 = 0x12600;
    work->y = 0;
    work->idleStep = 0;
    x = 0x308;
    AnimInit(&work->anim, gBosJfObjAnims, gBosJfObjFrames);
    AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    y.bounds = gBtlWork;
    ScrollBgMapTo(1, ((y.bounds->viewX - boss->body.x) >> 8) + x,
                  ((z = y.bounds->viewY - (boss->body.y + boss->body.z)) >> 8) + 0x126);
    TaskPoolInit(&work->tasks, 2);
    TaskCreate(&work->tasks, &gTaskDescBosJfBorderline, work->jf);
}

#undef GET_ACTOR_POSITION
u8 task_bos_jf_majin_1(JfMajinWork* work) {
    JfWork* jf = work->jf;

    jf->flags |= JF_FLAG_NEEDS_BG_CLIP;

    switch (work->jf->state) {
    case BOS_JF_STATE_IDLE:
        BosJfMajinUpdateIdle(work);
        break;
    case BOS_JF_STATE_SWITCH_SIDE:
        BosJfMajinUpdateSwitchSide(work);
        break;
    case BOS_JF_STATE_ROCK_THROW:
        BosJfMajinUpdateRockAttack(work);
        break;
    case BOS_JF_STATE_SLAM:
        BosJfMajinUpdateSlam(work);
        break;
    case BOS_JF_STATE_BEAM:
        BosJfMajinUpdateBeam(work);
        break;
    case BOS_JF_STATE_SWEEP_BEAM:
        BosJfMajinUpdateSweepBeam(work);
        break;
    case BOS_JF_STATE_UNUSED:
        BosJfMajinUpdateState8(work);
        break;
    case BOS_JF_STATE_SHIFT_PILLARS:
        BosJfMajinUpdatePillars(work);
        break;
    case BOS_JF_STATE_CARD_BROKEN:
        BosJfMajinUpdateBreak(work);
        break;
    case BOS_JF_STATE_DEFEATED:
        BosJfMajinUpdateDefeat(work);
        break;
    case BOS_JF_STATE_EVENT_IDLE:
        BosJfMajinUpdateEventIdle(work);
        break;
    case BOS_JF_STATE_GIMMICK:
        BosJfMajinUpdateGimmick(work);
        break;
    }

    ColliderSetPosition(&jf->body.collider, jf->body.x, jf->body.y, jf->body.z);
    TaskPoolUpdate(&work->tasks);

    if (work->jf->flags & JF_FLAG_NEEDS_BG_CLIP) {
        if (work->jf->state != BOS_JF_STATE_SHIFT_PILLARS) {
            BosJfMajinUpdateBgClip(work->jf->bgFrame, work);
        }
    }

    return 1;
}

void task_bos_jf_majin_2(JfMajinWork* work) {
    JfWork* jf = work->jf;
    void* palette;
    u16 flags;
    s16 x;
    s16 y;

    if (!gBtlWork->paused) {
        if (jf->flags & JF_FLAG_HURT) {
            if (gFrameCounter & 1) {
                LoadPaletteWithEffect(gHitFlashPalette, (void*)PLTT, sizeof(gHitFlashPalette));
                palette = work->palette2;
            } else {
                LoadPaletteWithEffect(gBosJfMajinPalette, (void*)PLTT, sizeof(gBosJfMajinPalette));
                palette = work->palette;
            }
        } else {
            palette = work->palette;
        }
    } else {
        LoadPaletteWithEffect(gBosJfMajinPalette, (void*)PLTT, sizeof(gBosJfMajinPalette));
        palette = work->palette;
    }

    ScrollBgMapTo(1, ((gBtlWork->viewX - jf->body.x) >> 8) + 776,
                  ((gBtlWork->viewY - (jf->body.y + jf->body.z)) >> 8) + 294);

    if (work->spriteVisible == TRUE) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            flags = GetBattleSpritePriorityFlags(jf->body.y);
        } else {
            flags = GetBattleSpritePriorityFlags(jf->body.y);
            flags |= SPRITE_FLAG_HFLIP;
        }

        WorldToScreen(&x, &y, jf->body.x, jf->body.y, jf->body.z);
        DrawSprite(x, work->y + (y - 61), work->gfx, work->tiles, palette, NULL, flags,
                   -4100 - (jf->body.y >> 8) * 4);
    }

    TaskPoolDraw(&work->tasks);
}

void task_bos_jf_majin_3(JfMajinWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    TaskPoolDestroy(&work->tasks);
}

enum BosJfPillar {
    BOS_JF_PILLAR_LEFT,
    BOS_JF_PILLAR_MIDDLE,
    BOS_JF_PILLAR_RIGHT
};

enum BosJfPillarDistance {
    BOS_JF_PILLAR_NEAR,
    BOS_JF_PILLAR_MIDWAY,
    BOS_JF_PILLAR_FAR
};

enum BosJfRockHit {
    BOS_JF_ROCK_HIT_NONE,
    BOS_JF_ROCK_HIT_PILLAR,
    BOS_JF_ROCK_HIT_OUT_OF_BOUNDS
};

s32 BosJfGetActorPillar() {
    s32 x = gBtlWork->actor->x;

    if (x < 0x1EA00) {
        return BOS_JF_PILLAR_LEFT;
    }

    if (x < 0x22200) {
        return BOS_JF_PILLAR_MIDDLE;
    }

    return BOS_JF_PILLAR_RIGHT;
}

s32 BosJfMajinGetActorPillarDistance(JfMajinWork* work) {
    s32 x;

    if (work->jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
        x = gBtlWork->actor->x;

        if (x < 0x1EA00) {
            return BOS_JF_PILLAR_FAR;
        }

        if (x < 0x22200) {
            return BOS_JF_PILLAR_MIDWAY;
        }

        return BOS_JF_PILLAR_NEAR;
    }

    x = gBtlWork->actor->x;

    if (x > 0x22200) {
        return BOS_JF_PILLAR_FAR;
    }

    if (x > 0x1EA00) {
        return BOS_JF_PILLAR_MIDWAY;
    }

    return BOS_JF_PILLAR_NEAR;
}

void BosJfMajinUpdateIdle(JfMajinWork* work) {
    JfWork* jf = work->jf;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 8;
            work->baseFrame = 8;
        } else {
            jf->bgFrame = 28;
            work->baseFrame = 28;
        }

        work->jf->bgFrameTimer = 0;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->idleStep = 0;
        work->spriteVisible = TRUE;
        work->jf->stateStep++;
    } else {
        if (jf->bgFrameTimer >= gBosJfMajinFrameDurations[jf->bgFrame]) {
            jf->bgFrameTimer = 0;
            work->jf->bgFrame++;
            work->idleStep++;

            if (work->jf->bgFrame > work->baseFrame + 5) {
                work->jf->bgFrame = work->baseFrame;
                work->idleStep = 0;
            }

            BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        }

        work->jf->bgFrameTimer++;
        work->gfx = AnimUpdate(&work->anim);
        work->y = gBosJfMajinIdleOffsets[work->idleStep];

        if (gBtlWork->phase != BTL_PHASE_START) {
            if (GetRandom() % 80 == 0) {
                BosJfMajinChooseAttack(work);
                work->jf->stateStep = 0;
            }
        }
    }
}

enum BosJfMajinSwitchSideStep {
    BOS_JF_MAJIN_SWITCH_SIDE_STEP_SINK,
    BOS_JF_MAJIN_SWITCH_SIDE_STEP_WARP,
    BOS_JF_MAJIN_SWITCH_SIDE_STEP_RISE,
    BOS_JF_MAJIN_SWITCH_SIDE_STEP_END
};

void BosJfMajinUpdateSwitchSide(JfMajinWork* work) {
    JfWork* jf = work->jf;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 0;
        } else {
            jf->bgFrame = 7;
        }

        work->jf->bgFrameTimer = 0;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
        work->step = BOS_JF_MAJIN_SWITCH_SIDE_STEP_SINK;
        work->stepTimer = 0;
        work->spriteVisible = FALSE;
        work->jf->stateStep++;
    } else {
        switch (work->step) {
        case BOS_JF_MAJIN_SWITCH_SIDE_STEP_SINK:
            if (jf->bgFrameTimer >= gBosJfMajinFrameDurations[jf->bgFrame]) {
                jf->bgFrameTimer = 0;

                if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->jf->bgFrame++;

                    if (work->jf->bgFrame > 7) {
                        work->jf->bgFrame = 0;
                    }
                } else {
                    work->jf->bgFrame--;

                    if (work->jf->bgFrame < 0) {
                        work->jf->bgFrame = 7;
                    }
                }

                BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
            }

            work->jf->bgFrameTimer++;
            jf->body.z += 0x400;
            work->stepTimer++;

            if (work->stepTimer > 40) {
                work->stepTimer = 0;
                work->step++;
            }

            if (work->stepTimer == 21) {
                jf->body.flags |= BTLOBJ_FLAG_UNHITTABLE;
            }

            break;
        case BOS_JF_MAJIN_SWITCH_SIDE_STEP_WARP:
            work->stepTimer++;

            if (work->stepTimer > 60) {
                work->stepTimer = 0;
                work->jf->bgFrameTimer = 0;

                if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                    jf->body.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                    jf->body.x = 0x16A00;
                    work->jf->bgFrame = 3;
                } else {
                    jf->body.flags |= BTLOBJ_FLAG_FACING_LEFT;
                    jf->body.x = 0x2A200;
                    work->jf->bgFrame = 3;
                }

                BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
                m4aSongNumStart(SONG_BTL_JF_UP);
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_SWITCH_SIDE_STEP_RISE:
            if (jf->bgFrameTimer >= gBosJfMajinFrameDurations[jf->bgFrame]) {
                jf->bgFrameTimer = 0;

                if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->jf->bgFrame++;

                    if (work->jf->bgFrame > 7) {
                        work->jf->bgFrame = 0;
                    }
                } else {
                    work->jf->bgFrame--;

                    if (work->jf->bgFrame < 0) {
                        work->jf->bgFrame = 7;
                    }
                }

                BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
            }

            work->jf->bgFrameTimer++;
            jf->body.z -= 0x400;
            work->stepTimer++;

            if (work->stepTimer > 40) {
                work->stepTimer = 0;
                work->step++;
            }

            if (work->stepTimer == 22) {
                jf->body.flags &= ~BTLOBJ_FLAG_UNHITTABLE;
            }

            break;
        default:
            work->jf->stateStep = 0;
            work->jf->state = BOS_JF_STATE_IDLE;
            break;
        }
    }
}

enum BosJfMajinRockThrowStep {
    BOS_JF_MAJIN_ROCK_THROW_STEP_SINK,
    BOS_JF_MAJIN_ROCK_THROW_STEP_SPAWN_ROCK,
    BOS_JF_MAJIN_ROCK_THROW_STEP_RISE,
    BOS_JF_MAJIN_ROCK_THROW_STEP_THROW,
    BOS_JF_MAJIN_ROCK_THROW_STEP_WAIT_ROCK,
    BOS_JF_MAJIN_ROCK_THROW_STEP_RETURN,
    BOS_JF_MAJIN_ROCK_THROW_STEP_END
};

void BosJfMajinUpdateRockAttack(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &jf->sub;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 0;
            work->baseFrame = 14;
        } else {
            jf->bgFrame = 7;
            work->baseFrame = 34;
        }

        work->jf->bgFrameTimer = 0;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
        work->step = BOS_JF_MAJIN_ROCK_THROW_STEP_SINK;
        work->stepTimer = 0;
        work->spriteVisible = FALSE;
        work->jf->stateStep++;
    } else {
        switch (work->step) {
        case BOS_JF_MAJIN_ROCK_THROW_STEP_SINK:
            if (work->jf->bgFrameTimer >= gBosJfMajinFrameDurations[work->jf->bgFrame]) {
                work->jf->bgFrameTimer = 0;

                if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->jf->bgFrame++;

                    if (work->jf->bgFrame > 7) {
                        work->jf->bgFrame = 0;
                    }
                } else {
                    work->jf->bgFrame--;

                    if (work->jf->bgFrame < 0) {
                        work->jf->bgFrame = 7;
                    }
                }

                BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
            }

            work->jf->bgFrameTimer++;
            jf->body.z += 0x400;
            work->stepTimer++;

            if (work->stepTimer > 60) {
                work->stepTimer = 0;
                work->step++;
            }

            if (work->stepTimer == 23) {
                jf->body.flags |= BTLOBJ_FLAG_UNHITTABLE;
            }

            break;
        case BOS_JF_MAJIN_ROCK_THROW_STEP_SPAWN_ROCK:
            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->jf->bgFrame = 14;
                work->z = -0x2000 - ((gBosJfRightPillarLevel + 1) << 11);
            } else {
                work->jf->bgFrame = 34;
                work->z = -0x2000 - ((gBosJfLeftPillarLevel + 1) << 11);
            }

            work->jf->bgFrameTimer = 0;
            work->task = TaskCreate(&work->tasks, &gTaskDescBosJfRock, work->jf);
            BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
            work->moveSteps = 120;
            work->step++;
            break;
        case BOS_JF_MAJIN_ROCK_THROW_STEP_RISE:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;

                if (work->moveSteps == 30) {
                    jf->body.flags &= ~BTLOBJ_FLAG_UNHITTABLE;
                }
            } else {
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_ROCK_THROW_STEP_THROW:
            if (work->jf->bgFrameTimer >= gBosJfMajinFrameDurations[work->jf->bgFrame]) {
                work->jf->bgFrameTimer = 0;
                work->jf->bgFrame++;

                if (work->jf->bgFrame > work->baseFrame + 4) {
                    if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                        work->jf->bgFrame = 8;
                    } else {
                        work->jf->bgFrame = 28;
                    }

                    work->spriteVisible = TRUE;
                    work->step++;
                }

                BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
            }

            work->jf->bgFrameTimer++;
            break;
        case BOS_JF_MAJIN_ROCK_THROW_STEP_WAIT_ROCK:
            if (!IsTaskActive(work->task)) {
                work->z = -0x3800;
                work->moveSteps = 10;
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_ROCK_THROW_STEP_RETURN:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
            }

            break;
        default:
            ClearBtlObjActionFlags(sub);
            work->jf->stateStep = 0;
            work->jf->state = BOS_JF_STATE_SHIFT_PILLARS;
            break;
        }
    }
}

enum BosJfMajinSlamStep {
    BOS_JF_MAJIN_SLAM_STEP_WINDUP,
    BOS_JF_MAJIN_SLAM_STEP_HOLD,
    BOS_JF_MAJIN_SLAM_STEP_DROP,
    BOS_JF_MAJIN_SLAM_STEP_IMPACT,
    BOS_JF_MAJIN_SLAM_STEP_RECOVER,
    BOS_JF_MAJIN_SLAM_STEP_RETURN,
    BOS_JF_MAJIN_SLAM_STEP_END
};

void BosJfMajinUpdateSlam(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &jf->sub;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 19;
            work->jf->bgFrameTimer = 0;
            work->baseFrame = 19;
            work->x = 0x27A00;
            work->z = -0x5200 - ((gBosJfRightPillarLevel + 1) << 11);
        } else {
            jf->bgFrame = 39;
            work->jf->bgFrameTimer = 0;
            work->baseFrame = 39;
            work->x = 0x19200;
            work->z = -0x5200 - ((gBosJfLeftPillarLevel + 1) << 11);
        }

        BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
        work->step = BOS_JF_MAJIN_SLAM_STEP_WINDUP;
        work->stepTimer = 0;
        work->moveSteps = 40;
        work->spriteVisible = FALSE;
        work->jf->stateStep++;
    } else {
        switch (work->step) {
        case BOS_JF_MAJIN_SLAM_STEP_WINDUP:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.x, work->x, work->moveSteps);
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;

                if (work->jf->bgFrameTimer >= gBosJfMajinFrameDurations[work->jf->bgFrame]) {
                    work->jf->bgFrameTimer = 0;
                    work->jf->bgFrame++;

                    if (work->jf->bgFrame >= work->baseFrame + 1) {
                        work->jf->bgFrame = work->baseFrame + 1;
                    }

                    BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
                }

                work->jf->bgFrameTimer++;
            } else {
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_SLAM_STEP_HOLD:
            work->stepTimer++;

            if (work->stepTimer > 20) {
                work->stepTimer = 0;
                work->extraClipRows = 1;
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_SLAM_STEP_DROP:
            jf->body.z += 0xA00;

            if (work->jf->bgFrameTimer >= gBosJfMajinFrameDurations[work->jf->bgFrame]) {
                work->jf->bgFrameTimer = 0;
                work->jf->bgFrame++;

                if (work->jf->bgFrame == work->baseFrame + 4) {
                    work->extraClipRows = 0;
                    work->step++;
                }

                BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
            }

            work->jf->bgFrameTimer++;
            break;
        case BOS_JF_MAJIN_SLAM_STEP_IMPACT:
            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartGroundImpact(jf->body.x - 0x3000, jf->body.y + jf->body.z + 0x1800);
                ApplyAttackBox(0xE8, jf->body.x - 0x3000, jf->body.y, jf->body.z + 0x1800, 30, 30, 30);
            } else {
                BgFxStartGroundImpact(jf->body.x + 0x3000, jf->body.y + jf->body.z + 0x1800);
                ApplyAttackBox(0xE8, jf->body.x + 0x3000, jf->body.y, jf->body.z + 0x1800, 30, 30, 30);
            }

            BtlMapStartShake();
            m4aSongNumStart(SONG_BTL_LB_RUMB);
            work->stepTimer = 0;
            work->step++;
            break;
        case BOS_JF_MAJIN_SLAM_STEP_RECOVER:
            if (work->stepTimer == 10) {
                work->jf->bgFrame = work->baseFrame + 5;
                BosJfMajinSetBgFrame(work->jf->bgFrame, 0xA0, work);
            }

            if (work->stepTimer > 20) {
                work->stepTimer = 0;

                if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->x = 0x2A200;
                } else {
                    work->x = 0x16A00;
                }

                work->z = -0x3800;
                work->moveSteps = 10;
                work->step++;
            }

            work->stepTimer++;
            break;
        case BOS_JF_MAJIN_SLAM_STEP_RETURN:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.x, work->x, work->moveSteps);
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
            }

            break;
        default:
            ClearBtlObjActionFlags(sub);
            work->jf->stateStep = 0;
            work->jf->state = BOS_JF_STATE_SHIFT_PILLARS;
            break;
        }
    }
}

enum BosJfMajinBeamStep {
    BOS_JF_MAJIN_BEAM_STEP_RISE,
    BOS_JF_MAJIN_BEAM_STEP_WINDUP,
    BOS_JF_MAJIN_BEAM_STEP_LUNGE,
    BOS_JF_MAJIN_BEAM_STEP_FIRE,
    BOS_JF_MAJIN_BEAM_STEP_SUSTAIN,
    BOS_JF_MAJIN_BEAM_STEP_RETURN,
    BOS_JF_MAJIN_BEAM_STEP_END
};

void BosJfMajinUpdateBeam(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &jf->sub;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 25;
            work->jf->bgFrameTimer = 0;
            work->baseFrame = 25;
            work->x = 0x2A200;
            work->z = -0x2400 - ((gBosJfRightPillarLevel + 1) << 11);
        } else {
            jf->bgFrame = 45;
            work->jf->bgFrameTimer = 0;
            work->baseFrame = 45;
            work->x = 0x16A00;
            work->z = -0x2400 - ((gBosJfLeftPillarLevel + 1) << 11);
        }

        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->step = BOS_JF_MAJIN_BEAM_STEP_RISE;
        work->stepTimer = 0;
        work->moveSteps = 40;
        work->spriteVisible = FALSE;
        work->jf->stateStep++;
    } else {
        switch (work->step) {
        case BOS_JF_MAJIN_BEAM_STEP_RISE:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_BEAM_STEP_WINDUP:
            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                jf->body.x += 0x100;
            } else {
                jf->body.x -= 0x100;
            }

            work->stepTimer++;

            if (work->stepTimer > 20) {
                work->stepTimer = 0;
                work->jf->bgFrame = work->baseFrame + 1;
                BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
                work->moveSteps = 2;
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_BEAM_STEP_LUNGE:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.x, work->x, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
                work->moveSteps = 0;
            }

            break;
        case BOS_JF_MAJIN_BEAM_STEP_FIRE:
            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartJfMajinBeam(jf->body.x - 0x1A00, jf->body.y, jf->body.z - 0x3100, Q_8_8(1.2), 160, 45);
            } else {
                BgFxStartJfMajinBeam(jf->body.x + 0x1A00, jf->body.y, jf->body.z - 0x3100, Q_8_8(1.2), 96, 45);
            }

            m4aSongNumStart(SONG_EF_JF_BEEM);
            jf->body.x = (work->x - 0x100) + (work->moveSteps++ % 2) * 0x200;
            work->stepTimer = 0;
            work->step++;
            break;
        case BOS_JF_MAJIN_BEAM_STEP_SUSTAIN:
            jf->body.x = (work->x - 0x100) + (work->moveSteps++ % 2) * 0x200;

            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxSetPosition(jf->body.x - 0x1A00, jf->body.y, jf->body.z - 0x3100);
            } else {
                BgFxSetPosition(jf->body.x + 0x1A00, jf->body.y, jf->body.z - 0x3100);
            }

            if (work->stepTimer > 10 && work->stepTimer % 10 == 9) {
                if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(0xE9, jf->body.x - 0x6400, jf->body.y + 0xA00, jf->body.z + 0x2000, 20, 20, 20);
                } else {
                    ApplyAttackBox(0xE9, jf->body.x + 0x6400, jf->body.y + 0xA00, jf->body.z + 0x2000, 20, 20, 20);
                }
            }

            if (work->stepTimer > 80) {
                work->stepTimer = 0;
                work->z = -0x3800;
                work->moveSteps = 10;
                work->jf->bgFrame = work->baseFrame + 2;
                BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
                work->step++;
            }

            work->stepTimer++;
            break;
        case BOS_JF_MAJIN_BEAM_STEP_RETURN:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.x, work->x, work->moveSteps);
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
            }

            break;
        default:
            ClearBtlObjActionFlags(sub);
            work->jf->stateStep = 0;
            work->jf->state = BOS_JF_STATE_SHIFT_PILLARS;
            break;
        }
    }
}

enum BosJfMajinSweepBeamStep {
    BOS_JF_MAJIN_SWEEP_BEAM_STEP_ALIGN,
    BOS_JF_MAJIN_SWEEP_BEAM_STEP_WINDUP,
    BOS_JF_MAJIN_SWEEP_BEAM_STEP_LUNGE,
    BOS_JF_MAJIN_SWEEP_BEAM_STEP_FIRE,
    BOS_JF_MAJIN_SWEEP_BEAM_STEP_SWEEP,
    BOS_JF_MAJIN_SWEEP_BEAM_STEP_RETURN,
    BOS_JF_MAJIN_SWEEP_BEAM_STEP_END
};

void BosJfMajinUpdateSweepBeam(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &jf->sub;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 25;
            work->jf->bgFrameTimer = 0;
            work->baseFrame = 25;
            work->x = 0x2A200;
            work->y2 = gBtlWork->actor->y - 0x1400;
            work->z = -0x2400 - ((gBosJfRightPillarLevel + 1) << 11);
        } else {
            jf->bgFrame = 45;
            work->jf->bgFrameTimer = 0;
            work->baseFrame = 45;
            work->x = 0x16A00;
            work->y2 = gBtlWork->actor->y - 0x1400;
            work->z = -0x2400 - ((gBosJfLeftPillarLevel + 1) << 11);
        }

        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->step = BOS_JF_MAJIN_SWEEP_BEAM_STEP_ALIGN;
        work->stepTimer = 0;
        work->moveSteps = 40;
        work->spriteVisible = FALSE;
        work->jf->stateStep++;
    } else {
        switch (work->step) {
        case BOS_JF_MAJIN_SWEEP_BEAM_STEP_ALIGN:
            BtlMapSetCameraTarget(jf->body.x, jf->body.y + jf->body.z);

            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.y, work->y2, work->moveSteps);
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_SWEEP_BEAM_STEP_WINDUP:
            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                jf->body.x += 0x100;
            } else {
                jf->body.x -= 0x100;
            }

            work->stepTimer++;

            if (work->stepTimer > 20) {
                work->stepTimer = 0;
                work->jf->bgFrame = work->baseFrame + 1;
                BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
                work->moveSteps = 2;
                work->step++;
            }

            break;
        case BOS_JF_MAJIN_SWEEP_BEAM_STEP_LUNGE:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.x, work->x, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
                work->moveSteps = 0;
            }

            break;
        case BOS_JF_MAJIN_SWEEP_BEAM_STEP_FIRE:
            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->beamAngle = 148;
                work->beamScale = Q_8_8(1);
                BgFxStartJfMajinBeam(jf->body.x - 0x1A00, jf->body.y, jf->body.z - 0x3100, Q_8_8(1), work->beamAngle, 45);
                BgFxSetScale(Q_8_8(1.2), Q_8_8(1));
            } else {
                work->beamAngle = 108;
                work->beamScale = Q_8_8(1.2);
                BgFxStartJfMajinBeam(jf->body.x + 0x1A00, jf->body.y, jf->body.z - 0x3100, Q_8_8(1), work->beamAngle, 45);
                BgFxSetScale(Q_8_8(1.2), Q_8_8(1));
            }

            m4aSongNumStart(SONG_EF_JF_BEEM);
            jf->body.x = (work->x - 0x100) + (work->moveSteps++ % 2) * 0x200;
            work->stepTimer = 0;
            work->beamLength = 0;
            work->step++;
            break;
        case BOS_JF_MAJIN_SWEEP_BEAM_STEP_SWEEP:
            work->moveSteps++;

            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                jf->body.x = (work->x - 0x100) + (work->moveSteps % 2) * 0x200;
                BgFxSetPosition(jf->body.x - 0x1A00, jf->body.y, jf->body.z - 0x3100);

                if (work->beamAngle <= 173 && work->moveSteps % 2 == 0) {
                    work->beamAngle++;
                    BgFxSetAngle(work->beamAngle);
                    work->beamLength++;
                    work->beamScale = gBosJfMajinBeamScales[work->beamLength];
                    BgFxSetScale(Q_8_8(1.2), work->beamScale);
                }
            } else {
                jf->body.x = (work->x - 0x100) + (work->moveSteps % 2) * 0x200;
                BgFxSetPosition(jf->body.x + 0x1A00, jf->body.y, jf->body.z - 0x3100);

                if (work->beamAngle > 82 && work->moveSteps % 2 == 0) {
                    work->beamAngle--;
                    BgFxSetAngle(work->beamAngle);
                    work->beamLength++;
                    work->beamScale = gBosJfMajinBeamScales[work->beamLength];
                    BgFxSetScale(Q_8_8(1.2), work->beamScale);
                }
            }

            if (work->stepTimer >= 11 && work->stepTimer <= 50) {
                if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(0xEA, jf->body.x - 0x3200 - work->beamLength * 3 * 512, jf->body.y + 0xA00, jf->body.z + 0x2C00, 20, 20, 20);
                } else {
                    ApplyAttackBox(0xEA, jf->body.x + 0x3200 + work->beamLength * 3 * 512, jf->body.y + 0xA00, jf->body.z + 0x2C00, 20, 20, 20);
                }
            }

            if (work->stepTimer > 80) {
                work->stepTimer = 0;
                work->y2 = 0x15E00;
                work->z = -0x3800;
                work->moveSteps = 10;
                work->jf->bgFrame = work->baseFrame + 2;
                BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
                work->step++;
            } else {
                work->stepTimer++;
            }

            break;
        case BOS_JF_MAJIN_SWEEP_BEAM_STEP_RETURN:
            if (work->moveSteps > 0) {
                ApproachValue(&jf->body.x, work->x, work->moveSteps);
                ApproachValue(&jf->body.y, work->y2, work->moveSteps);
                ApproachValue(&jf->body.z, work->z, work->moveSteps);
                work->moveSteps--;
            } else {
                work->step++;
            }

            break;
        default:
            ClearBtlObjActionFlags(sub);
            work->jf->stateStep = 0;
            work->jf->state = BOS_JF_STATE_SHIFT_PILLARS;
            break;
        }
    }
}

void BosJfMajinUpdateState8(JfMajinWork* work) {
}

void BosJfMajinUpdateBreak(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &jf->sub;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->body.x = 0x2A200;
            jf->bgFrame = 8;
        } else {
            jf->body.x = 0x16A00;
            jf->bgFrame = 28;
        }

        jf->body.y = 0x15E00;
        jf->body.z = -0x3800;
        work->jf->bgFrameTimer = 0;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->idleStep = 0;
        work->spriteVisible = TRUE;
        work->extraClipRows = 0;
        jf->body.flags &= ~BTLOBJ_FLAG_UNHITTABLE;
        CreateBtlPopTask(&jf->body, BTL_POP_CARD_BREAK);
        work->jf->stateStep++;
    } else if (jf->stateStep > 60) {
        ClearBtlObjActionFlags(sub);
        work->jf->stateStep = 0;

        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->jf->bgFrame = 8;
        } else {
            work->jf->bgFrame = 28;
        }

        work->jf->bgFrameTimer = 0;
        work->jf->state = BOS_JF_STATE_SHIFT_PILLARS;
    } else {
        work->jf->stateStep++;
    }
}

void BosJfMajinUpdateGimmick(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &jf->sub;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->body.x = 0x2A200;
            jf->bgFrame = 8;
        } else {
            jf->body.x = 0x16A00;
            jf->bgFrame = 28;
        }

        jf->body.y = 0x15E00;
        jf->body.z = -0x3800;
        work->jf->bgFrameTimer = 0;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->idleStep = 0;
        work->spriteVisible = TRUE;
        work->extraClipRows = 0;
        jf->body.flags &= ~BTLOBJ_FLAG_UNHITTABLE;
        work->jf->stateStep++;
    } else {
        ClearBtlObjActionFlags(sub);
        work->jf->stateStep = 0;

        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->jf->bgFrame = 8;
        } else {
            work->jf->bgFrame = 28;
        }

        work->jf->bgFrameTimer = 0;
        work->jf->state = BOS_JF_STATE_SHIFT_PILLARS;
    }
}

enum BosJfMajinDefeatStep {
    BOS_JF_MAJIN_DEFEAT_STEP_DELAY,
    BOS_JF_MAJIN_DEFEAT_STEP_START_DEATH_FX,
    BOS_JF_MAJIN_DEFEAT_STEP_START_FLASH,
    BOS_JF_MAJIN_DEFEAT_STEP_DROP_PRIZES,
    BOS_JF_MAJIN_DEFEAT_STEP_END
};

void BosJfMajinUpdateDefeat(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &jf->sub;
    PrizeCardArg prize;
    s32 x;

    if (jf->stateStep == 0) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 8;
            x = 0x2A200;
        } else {
            jf->bgFrame = 28;
            x = 0x16A00;
        }

        jf->body.x = x;

        jf->body.y = 0x15E00;
        jf->body.z = -0x3800;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->spriteVisible = TRUE;
        BtlMapSetCameraTarget(jf->body.x, jf->body.y + jf->body.z);
        BeginBossDefeat(&jf->body);
        work->step = BOS_JF_MAJIN_DEFEAT_STEP_DELAY;
        work->moveSteps = 0;
        work->jf->stateStep++;
        return;
    }

    switch (work->step) {
    case BOS_JF_MAJIN_DEFEAT_STEP_DELAY:
        BtlMapSetCameraTarget(jf->body.x, jf->body.y + jf->body.z);

        if (work->moveSteps > 1) {
            work->moveSteps = 0;
            work->step++;
        } else {
            work->moveSteps++;
        }

        break;
    case BOS_JF_MAJIN_DEFEAT_STEP_START_DEATH_FX:
        BtlMapSetCameraTarget(jf->body.x, jf->body.y + jf->body.z);

        if (FadeIsActive()) {
            break;
        }

        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            BgFxStartBossDeath(jf->body.x - 0x800, jf->body.y + jf->body.z - 0x800);
        } else {
            BgFxStartBossDeath(jf->body.x + 0x800, jf->body.y + jf->body.z - 0x800);
        }

        FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        work->step++;
        break;
    case BOS_JF_MAJIN_DEFEAT_STEP_START_FLASH:
        BtlMapSetCameraTarget(jf->body.x, jf->body.y + jf->body.z);

        if (work->moveSteps <= 119) {
            work->moveSteps++;
        } else {
            BgFxStartBossDeathFlash();
            work->step++;
        }

        break;
    case BOS_JF_MAJIN_DEFEAT_STEP_DROP_PRIZES:
        if (!BgFxIsActive()) {
            if (sub->x < 0x1B200) {
                sub->x = 0x1BA00;
            }

            if (sub->x > 0x25A00) {
                sub->x = 0x25200;
            }

            prize.x = sub->x;
            prize.y = sub->y;
            prize.z = -0x7800;
            CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &prize);
#ifdef VERSION_EU
            DropBossPrizes(sub);
#else
            DropBossPrizes(&jf->body);
#endif
            gBosJfLeftPillarLevel = 0;
            gBosJfMiddlePillarLevel = 0;
            gBosJfRightPillarLevel = 0;
            BosJfDrawPillars();
            jf->body.x = 0;
            jf->body.y = 0;
            jf->body.z = 0;
            ScrollBgMapTo(1, (gBtlWork->viewX >> 8) + 776, (gBtlWork->viewY >> 8) + 294);
            work->step++;
        } else {
            BtlMapSetCameraTarget(jf->body.x, jf->body.y + jf->body.z);
        }

        break;
    default:
        EndBossDefeat();
        work->jf->flags |= JF_FLAG_DEFEAT_DONE;
        break;
    }
}

u8 BosJfStepPillarLevel(u16* level, s16 target, u8 actorPillar, u8 pillar) {
    if ((s16)*level == target) {
        return 1;
    }

    if ((s16)*level > target) {
        *level = *level - 1;
    } else {
        BtlObj* actor;
        s32 pillarZ;

        *level = *level + 1;

        if (actorPillar == pillar) {
            actor = gBtlWork->actor;
            pillarZ = -(((s16)*level + 1) << 11);

            if (actor->z >= pillarZ) {
                actor->z = pillarZ;
            }
        }
    }

    return 0;
}

enum BosJfShiftPillarsStep {
    BOS_JF_SHIFT_PILLARS_STEP_CHOOSE_LAYOUT,
    BOS_JF_SHIFT_PILLARS_STEP_RUMBLE,
    BOS_JF_SHIFT_PILLARS_STEP_MOVE
};

void BosJfMajinUpdatePillars(JfMajinWork* work) {
    JfWork* jf = work->jf;
    BtlObj* sub = &work->jf->sub;
    s16 n = 0;
    s16 pattern;
    u8 actorPillar;

    if (jf->gimmickTimer > 0) {
        jf->stateStep = 0;
        work->jf->state = BOS_JF_STATE_IDLE;
        return;
    }

    switch (jf->stateStep) {
    case BOS_JF_SHIFT_PILLARS_STEP_CHOOSE_LAYOUT:
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            jf->bgFrame = 8;
            work->baseFrame = 8;
        } else {
            jf->bgFrame = 28;
            work->baseFrame = 28;
        }

        work->jf->bgFrameTimer = 0;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->idleStep = 0;
        work->spriteVisible = TRUE;

        if (work->jf->flags & JF_FLAG_GIMMICK_PENDING) {
            work->jf->flags &= ~JF_FLAG_GIMMICK_PENDING;
            work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_GIMMICK;
            pattern = 14;
        } else if (sub->hp < sub->maxHp / 2) {
            switch (work->jf->pillarPhase) {
            case BOS_JF_PILLAR_PHASE_START:
                pattern = GetRandom() & 1;
                break;
            case BOS_JF_PILLAR_PHASE_STAIRS:
            case BOS_JF_PILLAR_PHASE_GIMMICK:
                pattern = GetRandom() % 6 + 2;
                break;
            case BOS_JF_PILLAR_PHASE_FLAT:
                pattern = GetRandom() % 6 + 8;
                break;
            default:
                pattern = 0;
                break;
            }
        } else if (work->jf->pillarPhase == BOS_JF_PILLAR_PHASE_START) {
            pattern = GetRandom() % 6 + 8;
        } else {
            pattern = GetRandom() % 8;
        }

        work->leftLevel = gBosJfLeftPillarLevel;
        work->middleLevel = gBosJfMiddlePillarLevel;
        work->rightLevel = gBosJfRightPillarLevel;
        work->leftTarget = gBosJfPillarPatterns[pattern][0];
        work->middleTarget = gBosJfPillarPatterns[pattern][1];
        work->rightTarget = gBosJfPillarPatterns[pattern][2];
        work->step = 0;
        work->stepTimer = 0;
        work->moveSteps = 0;
        BosJfStartShake(60);
        work->jf->stateStep++;
        break;
    case BOS_JF_SHIFT_PILLARS_STEP_RUMBLE:
        work->stepTimer++;

        if (work->stepTimer > 80) {
            work->stepTimer = 0;
            m4aSongNumStart(SONG_BTL_IRON_GIMICBREAK);
            work->jf->stateStep++;
        }

        break;
    default:
        work->stepTimer++;

        if (work->stepTimer <= 1) {
            return;
        }

        work->stepTimer = 0;
        actorPillar = BosJfGetActorPillar();
        n += (s8)BosJfStepPillarLevel(&work->leftLevel, work->leftTarget, actorPillar, BOS_JF_PILLAR_LEFT);
        n += (s8)BosJfStepPillarLevel(&work->middleLevel, work->middleTarget, actorPillar, BOS_JF_PILLAR_MIDDLE);
        n += (s8)BosJfStepPillarLevel(&work->rightLevel, work->rightTarget, actorPillar, BOS_JF_PILLAR_RIGHT);

        if (n == 3) {
            if (sub->hp < sub->maxHp / 2) {
                switch (work->jf->pillarPhase) {
                case BOS_JF_PILLAR_PHASE_START:
                    work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_FLAT;
                    break;
                case BOS_JF_PILLAR_PHASE_STAIRS:
                    work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_START;
                    break;
                case BOS_JF_PILLAR_PHASE_GIMMICK:
                    work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_START;
                    work->jf->gimmickTimer = 300;
                    break;
                case BOS_JF_PILLAR_PHASE_FLAT:
                    work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_STAIRS;
                    break;
                }
            } else {
                switch (work->jf->pillarPhase) {
                case BOS_JF_PILLAR_PHASE_START:
                    work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_STAIRS;
                    break;
                case BOS_JF_PILLAR_PHASE_STAIRS:
                    work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_START;
                    break;
                case BOS_JF_PILLAR_PHASE_GIMMICK:
                    work->jf->pillarPhase = BOS_JF_PILLAR_PHASE_START;
                    work->jf->gimmickTimer = 300;
                    break;
                }
            }

            work->jf->stateStep = 0;
            work->jf->state = BOS_JF_STATE_IDLE;
        } else {
            gBosJfLeftPillarLevel = work->leftLevel;
            gBosJfMiddlePillarLevel = work->middleLevel;
            gBosJfRightPillarLevel = work->rightLevel;
            BosJfDrawPillars();
        }

        break;
    }
}

void BosJfMajinUpdateEventIdle(JfMajinWork* work) {
    if (work->jf->stateStep == 0) {
        work->jf->bgFrame = 8;
        work->baseFrame = 8;
        work->jf->bgFrameTimer = 0;
        BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        work->idleStep = 0;
        work->spriteVisible = TRUE;
        work->jf->stateStep++;
    } else {
        if (work->jf->bgFrameTimer >= gBosJfMajinFrameDurations[work->jf->bgFrame]) {
            work->jf->bgFrameTimer = 0;
            work->jf->bgFrame++;
            work->idleStep++;

            if (work->jf->bgFrame > work->baseFrame + 5) {
                work->jf->bgFrame = work->baseFrame;
                work->idleStep = 0;
            }

            BosJfMajinSetBgFrame(work->jf->bgFrame, 0x80, work);
        }

        work->jf->bgFrameTimer++;
        work->gfx = AnimUpdate(&work->anim);
        work->y = gBosJfMajinIdleOffsets[work->idleStep];
    }
}

void BosJfMajinChooseAttack(JfMajinWork* work) {
    BtlObj* sub = &work->jf->sub;
    u8 dist;
    s32 roll;

    if (sub->hp < sub->maxHp / 2) {
        if (GetRandom() % 100 <= 9) {
            RequestBossCardValue(1);
        } else if (GetRandom() % 90 <= 19) {
            RequestBossCardValue(GetRandom() % 2 + 7);
        } else {
            RequestBossCardValue(GetRandom() % 4 + 3);
        }

        if (gBosJfLeftPillarLevel == gBosJfMiddlePillarLevel && gBosJfLeftPillarLevel == gBosJfRightPillarLevel && GetRandom() % 100 <= 79) {
            RequestEnemyCardUse(sub);
            work->jf->attackState = BOS_JF_STATE_SWEEP_BEAM;
        } else {
            dist = BosJfMajinGetActorPillarDistance(work);

            switch (dist) {
            case BOS_JF_PILLAR_NEAR:
                RequestEnemyCardUse(sub);
                roll = (s16)(GetRandom() % 100);

                if (roll <= 39) {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_SLAM;
                } else if (roll <= 79) {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_BEAM;
                } else {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_ROCK_THROW;
                }

                break;
            case BOS_JF_PILLAR_MIDWAY:
                RequestEnemyCardUse(sub);
                work->jf->attackState = BOS_JF_STATE_ROCK_THROW;
                break;
            case BOS_JF_PILLAR_FAR:
                if (GetRandom() % 100 <= 49) {
                    work->jf->state = BOS_JF_STATE_SWITCH_SIDE;
                } else {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_ROCK_THROW;
                }

                break;
            }
        }
    } else {
        if (GetRandom() % 100 <= 29) {
            RequestBossCardValue(GetRandom() % 3 + 6);
        } else {
            RequestBossCardValue(GetRandom() % 6 + 1);
        }

        if (gBosJfLeftPillarLevel == gBosJfMiddlePillarLevel && gBosJfLeftPillarLevel == gBosJfRightPillarLevel && GetRandom() % 100 <= 19) {
            RequestEnemyCardUse(sub);
            work->jf->attackState = BOS_JF_STATE_SWEEP_BEAM;
        } else {
            dist = BosJfMajinGetActorPillarDistance(work);

            switch (dist) {
            case BOS_JF_PILLAR_NEAR:
                RequestEnemyCardUse(sub);

                if (GetRandom() % 100 <= 59) {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_SLAM;
                } else {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_BEAM;
                }

                break;
            case BOS_JF_PILLAR_MIDWAY:
                if (GetRandom() % 100 <= 79) {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_ROCK_THROW;
                } else {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_BEAM;
                }

                break;
            case BOS_JF_PILLAR_FAR:
                if (GetRandom() % 100 <= 69) {
                    work->jf->state = BOS_JF_STATE_SWITCH_SIDE;
                } else {
                    RequestEnemyCardUse(sub);
                    work->jf->attackState = BOS_JF_STATE_ROCK_THROW;
                }

                break;
            }
        }
    }
}

enum BosJfRockState {
    BOS_JF_ROCK_STATE_RISE,
    BOS_JF_ROCK_STATE_THROW,
    BOS_JF_ROCK_STATE_FLY,
    BOS_JF_ROCK_STATE_EXPLODE,
    BOS_JF_ROCK_STATE_OUT_OF_BOUNDS,
    BOS_JF_ROCK_STATE_INTERRUPTED
};

void task_bos_jf_rock_0(JfRockWork* work, JfWork* arg) {
    work->jf = arg;
    work->body.x = arg->body.x;
    work->body.y = arg->body.y + 0x500;
    work->body.z = arg->body.z - 0x4800;
    work->body.groundZ = 0;
    work->body.shadowPriority = 0xFE00;
    work->body.flags = BTLOBJ_FLAG_LARGE_SHADOW;
    work->targetX = 0;
    work->targetY = 0;
    work->targetZ = 0;
    work->vx = 0;
    work->vy = 0;
    work->vz = 0;
    work->accelZ = 0;
    work->paletteFrame = 0;
    work->paletteTimer = 0;
    work->shadowVisible = FALSE;

    if (arg->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
        work->x2 = arg->body.x + 0x2000;
        work->y2 = arg->body.y + 0xA00;
        work->z2 = arg->body.z - 0x1900;
        work->targetZ = -((gBosJfRightPillarLevel + 1) << 11) - 0x2000;
    } else {
        work->x2 = arg->body.x - 0x2000;
        work->y2 = arg->body.y + 0xA00;
        work->z2 = arg->body.z - 0x1900;
        work->targetZ = -0x2000 - ((gBosJfLeftPillarLevel + 1) << 11);
    }

    work->visible2 = FALSE;
    work->gfx2Index = 0;
    work->visible = FALSE;
    work->animIndex = 0;
    work->riseSteps = 120;
    work->throwTimer = 0;
    work->state = BOS_JF_ROCK_STATE_RISE;
    work->tiles = LoadObjTiles(gBosJfObjTiles, sizeof(gBosJfObjTiles));
    work->palette = LoadObjPalette(gBosJfObjPalette, sizeof(gBosJfObjPalette));
    AnimInit(&work->anim, gBosJfObjAnims, gBosJfObjFrames);
    AnimStart(&work->anim, gBosJfRockAnims[work->animIndex], 0);
    work->gfx = AnimGetGfx(&work->anim);
    work->tiles2 = LoadObjTiles(gBosJfObjTiles, sizeof(gBosJfObjTiles));
    work->palette2 = LoadObjPalette(gBosJfObjPalette, sizeof(gBosJfObjPalette));
    work->gfx2 = gBosJfObjFrames[gBosJfRockGfx2Frames[work->gfx2Index]];
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBosJfShadow, &work->body);
}

u8 task_bos_jf_rock_1(JfRockWork* work) {
    JfWork* jf = work->jf;
    BtlObj* actor;
    s16 index;

    switch (work->state) {
    case BOS_JF_ROCK_STATE_RISE:
        if (work->riseSteps > 40) {
            BtlMapSetCameraTarget(work->body.x, work->body.y + work->body.z - 0x2000);
        }

        work->paletteTimer++;

        if (work->paletteTimer > 2) {
            work->paletteTimer = 0;
            work->paletteFrame++;

            if (work->paletteFrame > 8) {
                work->paletteFrame = 0;
            }

            LoadObjPaletteBank(BIOS_PTR(work->palette)->index + 2, gBosJfRockPalettes + (work->paletteFrame << 4));
        }

        if (work->riseSteps > 0) {
            ApproachValue(&work->body.z, work->targetZ - 0x4800, work->riseSteps);
            ApproachValue(&work->z2, work->targetZ - 0x1C00, work->riseSteps);
            work->riseSteps--;

            if (work->body.z <= -0xC00) {
                work->animIndex = 8;
                work->visible = TRUE;
            } else {
                index = 8 - ((work->body.z >> 8) + 12) / 8;

                if (index < 0) {
                    work->animIndex = 0;
                    work->visible = FALSE;
                } else {
                    work->animIndex = index;
                    work->visible = TRUE;
                }
            }

            AnimStart(&work->anim, gBosJfRockAnims[work->animIndex], 0);
            work->gfx = AnimGetGfx(&work->anim);

            if (work->z2 <= -0x1000) {
                work->gfx2Index = 11;
                work->visible2 = TRUE;
            } else {
                index = 11 - ((work->z2 >> 8) + 16) / 8;

                if (index < 0) {
                    work->gfx2Index = 0;
                    work->visible2 = FALSE;
                } else {
                    work->gfx2Index = index;
                    work->visible2 = TRUE;
                }
            }

            work->gfx2 = gBosJfObjFrames[gBosJfRockGfx2Frames[work->gfx2Index]];
        } else {
            work->targetX = (actor = gBtlWork->actor)->x;
            work->targetY = actor->y;
            work->targetZ = actor->z;
            work->vx = (work->targetX - work->body.x) / 40;
            work->vy = (work->targetY - work->body.y) / 40;
            work->vz = 0;
            work->accelZ = (work->targetZ - work->body.z) / 820;
            work->state++;
        }

        if (work->jf->state == BOS_JF_STATE_CARD_BROKEN || work->jf->state == BOS_JF_STATE_GIMMICK) {
            m4aSongNumStart(SONG_EF_FIRE01);
            work->visible2 = FALSE;
            work->riseSteps = 0;
            work->state = BOS_JF_ROCK_STATE_INTERRUPTED;
        }

        break;
    case BOS_JF_ROCK_STATE_THROW:
        work->paletteTimer++;

        if (work->paletteTimer > 2) {
            work->paletteTimer = 0;
            work->paletteFrame++;

            if (work->paletteFrame > 8) {
                work->paletteFrame = 0;
            }

            LoadObjPaletteBank(BIOS_PTR(work->palette)->index + 2, gBosJfRockPalettes + (work->paletteFrame << 4));
        }

        work->throwTimer++;

        if (work->throwTimer > 3) {
            work->throwTimer = 0;

            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->x2 = jf->body.x + 0x1000;
                work->y2 = jf->body.y + 0xA00;
                work->z2 = jf->body.z - 0x1000;
            } else {
                work->x2 = jf->body.x - 0x1000;
                work->y2 = jf->body.y + 0xA00;
                work->z2 = jf->body.z - 0x1000;
            }

            work->gfx2 = gBosJfObjFrames[15];
            m4aSongNumStart(SONG_BTL_JF_BALLTHR);
            work->shadowVisible = TRUE;
            work->state++;
        }

        if (work->jf->state == BOS_JF_STATE_CARD_BROKEN || work->jf->state == BOS_JF_STATE_GIMMICK) {
            m4aSongNumStart(SONG_EF_FIRE01);
            work->visible2 = FALSE;
            work->shadowVisible = FALSE;
            work->riseSteps = 0;
            work->state = BOS_JF_ROCK_STATE_INTERRUPTED;
        }

        break;
    case BOS_JF_ROCK_STATE_FLY:
        work->paletteTimer++;

        if (work->paletteTimer > 2) {
            work->paletteTimer = 0;
            work->paletteFrame++;

            if (work->paletteFrame > 8) {
                work->paletteFrame = 0;
            }

            LoadObjPaletteBank(BIOS_PTR(work->palette)->index + 2, gBosJfRockPalettes + (work->paletteFrame << 4));
        }

        work->body.x += work->vx;
        work->body.y += work->vy;
        work->body.z += work->vz;
        work->vz += work->accelZ;

        if (work->throwTimer == 7) {
            work->visible2 = FALSE;
        }

        if (ApplyAttackBox(231, work->body.x, work->body.y, work->body.z - 0x2000, 28, 28, 28) == ATTACK_RESULT_HIT) {
            m4aSongNumStart(SONG_EF_JF_BALLHIT);
            BgFxStartExplosion(work->body.x - 0x800, work->body.y + work->body.z - 0x2400, 0);
            work->shadowVisible = FALSE;
            work->state = BOS_JF_ROCK_STATE_EXPLODE;
        }

        if (work->jf->state == BOS_JF_STATE_CARD_BROKEN || work->jf->state == BOS_JF_STATE_GIMMICK) {
            m4aSongNumStart(SONG_EF_FIRE01);
            work->visible2 = FALSE;
            work->shadowVisible = FALSE;
            work->riseSteps = 0;
            work->state = BOS_JF_ROCK_STATE_INTERRUPTED;
        }

        switch ((s8)BosJfRockTestPillars(work->body.x, work->body.y, work->body.z - 0x2000)) {
        case BOS_JF_ROCK_HIT_PILLAR:
            m4aSongNumStart(SONG_EF_FIRE01);
            BgFxStartExplosion(work->body.x - 0x800, work->body.y + work->body.z - 0x2400, 0);
            work->shadowVisible = FALSE;
            work->state = BOS_JF_ROCK_STATE_EXPLODE;
            break;
        case BOS_JF_ROCK_HIT_OUT_OF_BOUNDS:
            work->shadowVisible = FALSE;
            work->state = BOS_JF_ROCK_STATE_OUT_OF_BOUNDS;
            break;
        }

        work->throwTimer++;
        break;
    case BOS_JF_ROCK_STATE_INTERRUPTED:
        if (!MosaicIsActive()) {
            if (work->riseSteps == 0) {
                BgFxStartExplosion(work->body.x - 0x800, work->body.y + work->body.z - 0x2400, 0);
                work->riseSteps++;
            } else {
                if (AnimIsFinished(&work->anim)) {
                    return 0;
                }

                work->gfx = AnimUpdate(&work->anim);
            }
        }

        break;
    case BOS_JF_ROCK_STATE_EXPLODE:
        if (AnimIsFinished(&work->anim)) {
            return 0;
        }

        work->gfx = AnimUpdate(&work->anim);
        break;
    default:
        return 0;
    }

    BosJfGetGroundZ(&work->body.x, &work->body.y, &work->body.z, &work->body.groundZ);
    TaskPoolUpdate(&work->tasks);

    return 1;
}

void task_bos_jf_rock_2(JfRockWork* work) {
    JfWork* jf = work->jf;
    u16 flags;
    s32 prio;
    s16 x;
    s16 y;

    if (work->visible == TRUE) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            if (work->body.x <= 0x259FF) {
                flags = GetBattleSpritePriorityFlags(work->body.y);
                prio = 0xFD00;
            } else {
                flags = SPRITE_PRIORITY(1);
                prio = 0xFFF5;
            }
        } else if (work->body.x > 0x1B200) {
            flags = GetBattleSpritePriorityFlags(work->body.y);
            prio = 0xFD00;
        } else {
            flags = SPRITE_PRIORITY(1);
            prio = 0xFFF5;
        }

        WorldToScreen(&x, &y, work->body.x, work->body.y, work->body.z);
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, prio);
    }

    if (work->visible2 == TRUE) {
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            flags = SPRITE_PRIORITY(1);
        } else {
            flags = SPRITE_PRIORITY(1);
            flags |= SPRITE_FLAG_HFLIP;
        }

        WorldToScreen(&x, &y, work->x2, work->y2, work->z2);
        DrawSprite(x, y, work->gfx2, work->tiles2, work->palette2, NULL, flags, 0xFFF2);
    }

    if (work->shadowVisible == TRUE) {
        TaskPoolDraw(&work->tasks);
    }
}

void task_bos_jf_rock_3(JfRockWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette2);
    TaskPoolDestroy(&work->tasks);
}

u8 BosJfRockTestPillars(s32 x, s32 y, s32 z) {
    s32 leftZ = -((gBosJfLeftPillarLevel + 1) << 11);
    s32 middleZ = -((gBosJfMiddlePillarLevel + 1) << 11);
    s32 rightZ = -((gBosJfRightPillarLevel + 1) << 11);
    s32 hi = x + 0x1C00;
    s32 lo = x - 0x1C00;
    s32 zh = z + 0x1C00;
    s32 zl = z - 0x1C00;

    if (zh >= leftZ && hi > 0x1B200 && lo < 0x1EA00) {
        return BOS_JF_ROCK_HIT_PILLAR;
    }

    if (zh >= middleZ && hi > 0x1EA00 && lo < 0x22200) {
        return BOS_JF_ROCK_HIT_PILLAR;
    }

    if (zh >= rightZ && hi > 0x22200 && lo < 0x25A00) {
        return BOS_JF_ROCK_HIT_PILLAR;
    }

    if (zl > 0 || lo > 0x2CA00 || hi < 0x14200) {
        return BOS_JF_ROCK_HIT_OUT_OF_BOUNDS;
    }

    return BOS_JF_ROCK_HIT_NONE;
}

void task_bos_jf_borderline_0(JfBorderlineWork* work, JfWork* arg) {
    work->jf = arg;
    BosJfBorderlineUpdateLayout(work);
    work->offsetY = 0xA00;
    work->offsetZ = 0x3600;
    work->x = arg->body.x + work->offsetX;
    work->y = arg->body.y + work->offsetY;
    work->z = arg->body.z + work->offsetZ;
    work->unk_0B0 = 0;
    work->unk_0B2 = 0;
    work->unk_0B4 = 0;
    work->wide = 0;
    work->tiles = LoadObjTiles(gBosJfObjTiles, sizeof(gBosJfObjTiles));
    work->palette = LoadObjPalette(gBosJfObjPalette, sizeof(gBosJfObjPalette));
    AnimInit(&work->anim, gBosJfObjAnims, gBosJfObjFrames);
    AnimStart(&work->anim, 27, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    AnimInit(&work->anim2, gBosJfObjAnims, gBosJfObjFrames);
    AnimStart(&work->anim2, 8, ANIM_FLAG_LOOP);
    work->gfx2 = AnimGetGfx(&work->anim2);
    AnimInit(&work->anim3, gBosJfObjAnims, gBosJfObjFrames);
    AnimStart(&work->anim3, 7, ANIM_FLAG_LOOP);
    work->gfx3 = AnimGetGfx(&work->anim3);
    AnimInit(&work->anim4, gBosJfObjAnims, gBosJfObjFrames);
    AnimStart(&work->anim4, 28, ANIM_FLAG_LOOP);
    work->gfx4 = AnimGetGfx(&work->anim4);
    AnimInit(&work->anim5, gBosJfObjAnims, gBosJfObjFrames);
    AnimStart(&work->anim5, 6, ANIM_FLAG_LOOP);
    work->gfx5 = AnimGetGfx(&work->anim5);
    SetBtlPaletteFadeExcluded(BIOS_PTR(work->palette)->index + 16, FALSE);
}

u8 task_bos_jf_borderline_1(JfBorderlineWork* work) {
    JfWork* jf = work->jf;

    BosJfBorderlineUpdateLayout(work);
    work->x = jf->body.x + work->offsetX;
    work->y = jf->body.y + work->offsetY;
    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
    work->gfx3 = AnimUpdate(&work->anim3);
    work->gfx4 = AnimUpdate(&work->anim4);
    work->gfx5 = AnimUpdate(&work->anim5);

    return 1;
}

void task_bos_jf_borderline_2(JfBorderlineWork* work) {
    s16 sx;
    s16 sy;

    WorldToScreen(&sx, &sy, work->x, work->y, work->z);

    switch (work->wide) {
    case 0:
        DrawSprite(sx - 16, sy - 1, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 16, sy - 1, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx, sy + 1, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx + 16, sy - 1, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx + 16, sy - 1, work->gfx2, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 8, sy + 4, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 8, sy + 4, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFF60);
        DrawSprite(sx + 8, sy + 4, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFF60);
        DrawSprite(sx + 8, sy + 4, work->gfx2, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        break;
    case 1:
        DrawSprite(sx - 40, sy - 1, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 40, sy - 1, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 24, sy + 1, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 8, sy + 2, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx + 8, sy + 2, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx + 24, sy + 1, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx + 40, sy - 1, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx + 40, sy - 1, work->gfx2, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 32, sy + 4, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        DrawSprite(sx - 32, sy + 4, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFF60);
        DrawSprite(sx - 16, sy + 6, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFF60);
        DrawSprite(sx, sy + 7, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFF60);
        DrawSprite(sx + 16, sy + 6, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFF60);
        DrawSprite(sx + 32, sy + 4, work->gfx3, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFF60);
        DrawSprite(sx + 32, sy - 4, work->gfx2, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1),
            0xFFF0);
        break;
    }

    DrawSprite(sx, sy - 8, work->gfx5, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0xFF00);
}

void task_bos_jf_borderline_3(JfBorderlineWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void BosJfBorderlineUpdateLayout(JfBorderlineWork* work) {
    JfWork* jf = work->jf;

    switch (jf->state) {
    case BOS_JF_STATE_IDLE:
    case BOS_JF_STATE_SHIFT_PILLARS:
    case BOS_JF_STATE_CARD_BROKEN:
    case BOS_JF_STATE_UNUSED:
    case BOS_JF_STATE_DEFEATED:
    case BOS_JF_STATE_EVENT_IDLE:
    case BOS_JF_STATE_GIMMICK:
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->offsetX = -0x500;
        } else {
            work->offsetX = 0x500;
        }

        break;
    case BOS_JF_STATE_SLAM:
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->offsetX = -0x100;
        } else {
            work->offsetX = 0x100;
        }

        break;
    case BOS_JF_STATE_BEAM:
    case BOS_JF_STATE_SWEEP_BEAM:
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->offsetX = 0x1000;
        } else {
            work->offsetX = -0x1000;
        }

        break;
    case BOS_JF_STATE_SWITCH_SIDE:
    case BOS_JF_STATE_ROCK_THROW:
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->offsetX = -0xA00;
        } else {
            work->offsetX = 0xA00;
        }

        work->wide = 1;
        return;
    default:
        if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->offsetX = -0x500;
        } else {
            work->offsetX = 0x500;
        }

        break;
    }

    work->wide = 0;
}

const s16 gBosJfMajinFrameDurations[49] = {
    9, 9, 8, 8, 8, 8, 8, 8, 30, 6, 6, 12, 6, 6, 2, 8,
    8, 8, 8, 12, 2, 2, 2, 2, 12, 12, 12, 12, 30, 6, 6, 12,
    6, 6, 2, 8, 8, 8, 8, 12, 2, 2, 2, 2, 12, 12, 12, 12,
    0,
};

const s8 gBosJfMajinIdleOffsets[6] = { 0, -1, -2, -2, -1, 0 };

const u16 gBosJfPillarPatterns[16][3] = {
    { 0, 0, 0 },
    { 7, 7, 7 },
    { 7, 0, 0 },
    { 0, 7, 0 },
    { 0, 0, 7 },
    { 7, 7, 0 },
    { 7, 0, 7 },
    { 0, 7, 7 },
    { 0, 7, 14 },
    { 0, 14, 7 },
    { 7, 0, 14 },
    { 7, 14, 0 },
    { 14, 0, 7 },
    { 14, 7, 0 },
    { 14, 14, 14 },
    { 0, 0, 0 },
};

void* gBosJfMajinFrameMaps[49] = {
    gBosJfMajinFrame0Map,
    gBosJfMajinFrame1Map,
    gBosJfMajinFrame2Map,
    gBosJfMajinFrame3Map,
    gBosJfMajinFrame4Map,
    gBosJfMajinFrame5Map,
    gBosJfMajinFrame6Map,
    gBosJfMajinFrame7Map,
    gBosJfMajinFrame8Map,
    gBosJfMajinFrame9Map,
    gBosJfMajinFrame10Map,
    gBosJfMajinFrame11Map,
    gBosJfMajinFrame12Map,
    gBosJfMajinFrame13Map,
    gBosJfMajinFrame14Map,
    gBosJfMajinFrame15Map,
    gBosJfMajinFrame16Map,
    gBosJfMajinFrame17Map,
    gBosJfMajinFrame18Map,
    gBosJfMajinFrame19Map,
    gBosJfMajinFrame20Map,
    gBosJfMajinFrame21Map,
    gBosJfMajinFrame22Map,
    gBosJfMajinFrame23Map,
    gBosJfMajinFrame24Map,
    gBosJfMajinFrame25Map,
    gBosJfMajinFrame26Map,
    gBosJfMajinFrame27Map,
    gBosJfMajinFrame28Map,
    gBosJfMajinFrame29Map,
    gBosJfMajinFrame30Map,
    gBosJfMajinFrame31Map,
    gBosJfMajinFrame32Map,
    gBosJfMajinFrame33Map,
    gBosJfMajinFrame34Map,
    gBosJfMajinFrame35Map,
    gBosJfMajinFrame36Map,
    gBosJfMajinFrame37Map,
    gBosJfMajinFrame38Map,
    gBosJfMajinFrame39Map,
    gBosJfMajinFrame40Map,
    gBosJfMajinFrame41Map,
    gBosJfMajinFrame42Map,
    gBosJfMajinFrame43Map,
    gBosJfMajinFrame44Map,
    gBosJfMajinFrame45Map,
    gBosJfMajinFrame46Map,
    gBosJfMajinFrame47Map,
    gDefaultBgMap,
};

void* gBosJfMajinFrameTiles[49] = {
    gBosJfMajinFrame0Tiles,
    gBosJfMajinFrame1Tiles,
    gBosJfMajinFrame2Tiles,
    gBosJfMajinFrame3Tiles,
    gBosJfMajinFrame4Tiles,
    gBosJfMajinFrame5Tiles,
    gBosJfMajinFrame6Tiles,
    gBosJfMajinFrame7Tiles,
    gBosJfMajinFrame8Tiles,
    gBosJfMajinFrame9Tiles,
    gBosJfMajinFrame10Tiles,
    gBosJfMajinFrame11Tiles,
    gBosJfMajinFrame12Tiles,
    gBosJfMajinFrame13Tiles,
    gBosJfMajinFrame14Tiles,
    gBosJfMajinFrame15Tiles,
    gBosJfMajinFrame16Tiles,
    gBosJfMajinFrame17Tiles,
    gBosJfMajinFrame18Tiles,
    gBosJfMajinFrame19Tiles,
    gBosJfMajinFrame20Tiles,
    gBosJfMajinFrame21Tiles,
    gBosJfMajinFrame22Tiles,
    gBosJfMajinFrame23Tiles,
    gBosJfMajinFrame24Tiles,
    gBosJfMajinFrame25Tiles,
    gBosJfMajinFrame26Tiles,
    gBosJfMajinFrame27Tiles,
    gBosJfMajinFrame8Tiles,
    gBosJfMajinFrame9Tiles,
    gBosJfMajinFrame10Tiles,
    gBosJfMajinFrame11Tiles,
    gBosJfMajinFrame12Tiles,
    gBosJfMajinFrame13Tiles,
    gBosJfMajinFrame14Tiles,
    gBosJfMajinFrame15Tiles,
    gBosJfMajinFrame16Tiles,
    gBosJfMajinFrame17Tiles,
    gBosJfMajinFrame18Tiles,
    gBosJfMajinFrame19Tiles,
    gBosJfMajinFrame20Tiles,
    gBosJfMajinFrame21Tiles,
    gBosJfMajinFrame22Tiles,
    gBosJfMajinFrame23Tiles,
    gBosJfMajinFrame24Tiles,
    gBosJfMajinFrame25Tiles,
    gBosJfMajinFrame26Tiles,
    gBosJfMajinFrame27Tiles,
    NULL,
};

u32 gBosJfMajinBeamScales[27] = { Q_8_8(1), Q_8_8(1.04), Q_8_8(1.08), Q_8_8(1.12), Q_8_8(1.16), Q_8_8(1.2), Q_8_8(1.24), Q_8_8(1.28), Q_8_8(1.32), Q_8_8(1.36), Q_8_8(1.4), Q_8_8(1.44), Q_8_8(1.48), Q_8_8(1.52), Q_8_8(1.56), Q_8_8(1.6), Q_8_8(1.64), Q_8_8(1.69), Q_8_8(1.74), Q_8_8(1.79), Q_8_8(1.84), Q_8_8(1.9), Q_8_8(1.96), Q_8_8(2.02), Q_8_8(2.08), Q_8_8(2.14), Q_8_8(2.2) };

u32 gUnk_09EF2A00 = 578;

u32 gUnk_09EF2A04 = 593;

u32 gUnk_09EF2A08 = 609;

u32 gUnk_09EF2A0C = 622;

u32 gUnk_09EF2A10 = 637;

u32 gUnk_09EF2A14 = 652;

u32 gUnk_09EF2A18 = 668;

u32 gUnk_09EF2A1C = 683;

TaskDesc gTaskDescBosJfMajin = {
    "task_bos_jf_majin",
    (TaskInitFunc)task_bos_jf_majin_0,
    (TaskUpdateFunc)task_bos_jf_majin_1,
    (TaskDrawFunc)task_bos_jf_majin_2,
    (TaskDestroyFunc)task_bos_jf_majin_3,
    sizeof(JfMajinWork),
};

s8 gBosJfRockAnims[9] = { 9, 10, 11, 12, 13, 14, 15, 26, 0 };

s8 gUnk_09EF2A41 = -1;

s16 gBosJfRockGfx2Frames[12] = { 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 55 };

TaskDesc gTaskDescBosJfRock = {
    "task_bos_jf_rock",
    (TaskInitFunc)task_bos_jf_rock_0,
    (TaskUpdateFunc)task_bos_jf_rock_1,
    (TaskDrawFunc)task_bos_jf_rock_2,
    (TaskDestroyFunc)task_bos_jf_rock_3,
    sizeof(JfRockWork),
};

TaskDesc gTaskDescBosJfBorderline = {
    "task_bos_jf_borderline",
    (TaskInitFunc)task_bos_jf_borderline_0,
    (TaskUpdateFunc)task_bos_jf_borderline_1,
    (TaskDrawFunc)task_bos_jf_borderline_2,
    (TaskDestroyFunc)task_bos_jf_borderline_3,
    sizeof(JfBorderlineWork),
};
