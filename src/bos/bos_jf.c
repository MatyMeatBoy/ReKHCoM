/**
 * bos_jf.c
 * Jafar Boss
 */

#ifdef PLATFORM_PC
#include <stdlib.h>
#endif
#include "macros.h"
#include "bos2.h"
#include "sprites_bos2.h"
#include "system_state.h"
#include "btl_api.h"
#include "fade.h"
#include "songs.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "boss_jafar_types.h"
#include "btl_collision.h"
#include "chara_api.h"
#include "display.h"
#include "engine_math.h"
#include "game_state.h"
#include "gba/defines.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include "pallet.h"
#include "registration_data.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "enemy_ids.h"

s16 gBosJfActorX EWRAM_COMMON(8);
JfMapArg gJfMapArg EWRAM_COMMON(16);
s16 gBosJfActorY EWRAM_COMMON(4);
s16 gBosJfActorZ EWRAM_COMMON(4);
s16 gBosJfRightPillarLevel EWRAM_COMMON(16);
s16 gBosJfLeftPillarLevel EWRAM_COMMON(4);
s16 gBosJfShakeTimer EWRAM_COMMON(4);
s16 gBosJfShakeDuration EWRAM_COMMON(4);
s8 gBosJfShakeActive EWRAM_COMMON(4);
s16 gBosJfMiddlePillarLevel EWRAM_COMMON(4);
void* gBosJfMapBlocks EWRAM_COMMON(4);
u32 gUnk_0203ACDC EWRAM_COMMON(4);
u8 gBosJfMapBuffer[0x800] EWRAM_COMMON(16);
s16 gBosJfShakeStep EWRAM_COMMON(4);
s16 gBosJfPillarShape EWRAM_COMMON(4);
s32 gBosJfShakeOffset EWRAM_COMMON(4);
u32 gUnk_0203B4EC EWRAM_COMMON(4);

void task_bos_jf_0(JfWork* work, s32 arg) {
    BtlObj* sub;
    BtlWork* btl;
    s32 inEvent;
    s32 inEvent2;

    gBosJfActorX = 0;
    gBosJfActorY = 0;
    gBosJfActorZ = 0;
    work->flags = 0;

    if (arg != 0) {
        work->flags = JF_FLAG_IN_EVENT;
    }

    gBosJfLeftPillarLevel = 7;
    gBosJfMiddlePillarLevel = 0;
    gBosJfRightPillarLevel = 0;
    gJfMapArg.tiles = gBosJfBgTiles;
    gJfMapArg.tilesSize = 0x8000;
    gJfMapArg.palette = gBosJfBgPalette;
    gJfMapArg.paletteSize = 128;
    gJfMapArg.maps[0] = gBosJfBgMap0;
    gJfMapArg.maps[1] = gBosJfBgMap1;
    gJfMapArg.maps[2] = gBosJfMapBuffer;
    gJfMapArg.maps[3] = gBosJfBgMap3;
    TaskPoolInit(&work->tasks, 4);

    if (work->flags & JF_FLAG_IN_EVENT) {
        TaskCreate(&work->tasks, &gTaskDescBosJfMap, &gJfMapArg);
    } else {
        TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosJfMap, &gJfMapArg);
    }

    work->unk_268 = 0;
    work->unk_26A = 0;
    inEvent = work->flags & JF_FLAG_IN_EVENT;

    if (inEvent != 0) {
        work->state = BOS_JF_STATE_EVENT_IDLE;
        work->attackState = BOS_JF_STATE_EVENT_IDLE;
    } else {
        work->state = BOS_JF_STATE_IDLE;
        work->attackState = BOS_JF_STATE_IDLE;
    }

    work->hitCount = 0;
    work->hurtTimer = 0;
    work->stateStep = 0;
    work->stepTimer = 0;
    work->bgFrame = 8;
    work->bgFrameTimer = 12;
    work->pillarPhase = BOS_JF_PILLAR_PHASE_START;
    work->gimmickTimer = 0;
    inEvent2 = work->flags & JF_FLAG_IN_EVENT;

    if (inEvent2 != 0) {
        work->bodyX = 0x2A200;
        work->bodyY = 0x15E00;
        work->bodyZ = -0x3800;
        InitEnemyBtlObj(&work->body, &gBosJfEmyKind, work->bodyX, work->bodyY, work->bodyZ);
        work->body.flags |= BTLOBJ_FLAG_FACING_LEFT;
        TaskCreate(&work->tasks, &gTaskDescBosJfMajin, work);
    } else {
        work->subX = 0x29600;
        work->subY = 0x15400;
        work->subZ = -0xB400;
        sub = &work->sub;
        InitEnemyBtlObj(sub, &gBosJfEmyKind, work->subX, work->subY, work->subZ);
        sub->flags |= 0x400;
        sub->flags |= BTLOBJ_FLAG_NO_BREAK_POP;
        sub->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        sub->hitFlags = inEvent2;
        work->sub.centerHeight = 4;
        work->bodyX = 0x2A200;
        work->bodyY = 0x15E00;
        work->bodyZ = -0x3800;
        InitEnemyBtlObj(&work->body, &gBosJfEmyKind, work->bodyX, work->bodyY, work->bodyZ);
        work->body.flags |= BTLOBJ_FLAG_FACING_LEFT;
        work->body.flags |= BTLOBJ_FLAG_GUARD_PHYSICAL;
        work->body.flags |= BTLOBJ_FLAG_INVULNERABLE;
        work->body.radiusX = 32;
        work->body.radiusY = 40;
        work->body.height = 28;
        SetBtlObjParent(&work->body, sub);
        gBtlWork->bossPriorityOffset = 0xFF00;
        SetBtlPaletteFadeExcluded(0, TRUE);
        SetBattleActorPosition(0x23E00, 0x16800, -0x4000);
        SetGimmickTarget(0x20600, 0x16800, -0x800);
        TaskCreate(&work->tasks, &gTaskDescBosJfLamp, work);
        TaskCreate(&work->tasks, &gTaskDescBosJfMajin, work);
        btl = gBtlWork;
        btl->bossX = sub->x;
        btl->bossY = sub->y;
        btl->bossZ = sub->z;
    }
}

u8 task_bos_jf_1(JfWork* work) {
    BtlObj* sub = &work->sub;
    BtlWork* btl;
    u16 timer;

    if (work->flags & JF_FLAG_IN_EVENT) {
        TaskPoolUpdate(&work->tasks);
        return 1;
    }

    switch (UpdateBtlObjReaction(sub)) {
    case BTL_REACTION_CARD_ACTION:
        work->state = work->attackState;
        work->stateStep = 0;
        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        work->flags |= JF_FLAG_HURT;
        work->hurtTimer = 20;
        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        work->state = BOS_JF_STATE_DEFEATED;
        work->stateStep = 0;
        break;
    case BTL_REACTION_CARD_BROKEN:
#ifdef PLATFORM_PC
        /* harness (issue #83): KHCOM_GIMMICK_PICKUP=1 also drops the card when Sora breaks one of his, so the
           pickup (Sora's Blizzard otherwise) can be exercised */
        if (!(gGameState.flags & GAME_FLAG_RIKU) && getenv("KHCOM_GIMMICK_PICKUP") && work->gimmickTimer == 0)
            DropGimmickCard(0, sub->x, sub->y, sub->z);
#endif
        if (gGameState.flags & GAME_FLAG_RIKU) {
            if (work->gimmickTimer == 0) {
                if (GetRandom() % 100 <= 19) {
                    DropGimmickCard(0, sub->x, sub->y, sub->z);
                }
            }
        }

        work->state = BOS_JF_STATE_CARD_BROKEN;
        work->stateStep = 0;
        break;
    }

    if (work->flags & JF_FLAG_HURT) {
        if (--work->hurtTimer <= 0) {
            work->hitCount = 0;
            work->flags &= ~JF_FLAG_HURT;
            LoadPaletteWithEffect(gBosJfMajinPalette, (void*)PLTT, sizeof(gBosJfMajinPalette));
            ClearBtlObjActionFlags(sub);

            if (sub->hp > 0) {
                if (work->state != BOS_JF_STATE_SWITCH_SIDE && work->state != BOS_JF_STATE_SHIFT_PILLARS &&
                    work->state != BOS_JF_STATE_CARD_BROKEN && work->state != BOS_JF_STATE_GIMMICK) {
                    work->state = BOS_JF_STATE_IDLE;
                    work->stateStep = 0;
                }
            }
        }
    }

    if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
        if (sub->hitFlags & ATTACK_FLAG_ELEMENT_BLIZZARD) {
            sub->hitFlags &= ~ATTACK_FLAG_ELEMENT_BLIZZARD;

            if ((work->flags & JF_FLAG_HURT) == 0) {
                if (work->gimmickTimer == 0) {
                    DropGimmickCard(0, sub->x, sub->y, sub->z);
                }
            }
        }
    }

    if (ConsumeGimmickFlag(0)) {
        work->stateStep = 0;
        work->state = BOS_JF_STATE_GIMMICK;
        work->flags |= JF_FLAG_GIMMICK_PENDING;

        if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
            gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
        }
    }

    timer = work->gimmickTimer;

    if ((s16)timer > 0) {
        work->gimmickTimer = timer - 1;
    }

    TaskPoolUpdate(&work->tasks);
    btl = gBtlWork;
    btl->bossX = sub->x;
    btl->bossY = sub->y;
    btl->bossZ = sub->z;

    if (work->flags & JF_FLAG_DEFEAT_DONE) {
        return 0;
    }

    gBosJfActorX = btl->actor->x >> 8;
    gBosJfActorY = btl->actor->y >> 8;
    gBosJfActorZ = btl->actor->z >> 8;
    return 1;
}

void task_bos_jf_2(JfWork* work) {
    TaskPoolDraw(&work->tasks);
}

void task_bos_jf_3(JfWork* work) {
    if ((work->flags & JF_FLAG_IN_EVENT) == 0) {
        ReleaseEnemyBtlObj(&work->sub);
        ReleaseEnemyBtlObj(&work->body);
    }

    TaskPoolDestroy(&work->tasks);
}

enum BosJfPillarShape {
    BOS_JF_PILLAR_SHAPE_DESCENDING,
    BOS_JF_PILLAR_SHAPE_VALLEY,
    BOS_JF_PILLAR_SHAPE_LEFT_HIGH,
    BOS_JF_PILLAR_SHAPE_PEAK,
    BOS_JF_PILLAR_SHAPE_ASCENDING,
    BOS_JF_PILLAR_SHAPE_LEFT_LOW,
    BOS_JF_PILLAR_SHAPE_RIGHT_LOW,
    BOS_JF_PILLAR_SHAPE_RIGHT_HIGH,
    BOS_JF_PILLAR_SHAPE_FLAT
};

u8 ClampBosJfBounds(s32* px, s32* py, s32* pz, s32* out) {
    s32 leftZ;
    s32 middleZ;
    s32 rightZ;
    s32 lo;
    s32 hi;
    s32 x;

    leftZ = -((gBosJfLeftPillarLevel + 1) << 11);
    middleZ = -((gBosJfMiddlePillarLevel + 1) << 11);
    rightZ = -((gBosJfRightPillarLevel + 1) << 11);
    gBtlWork->actor->flags &= ~BTLOBJ_FLAG_HIDE_SHADOW;

    if (gBosJfLeftPillarLevel > gBosJfMiddlePillarLevel) {
        lo = 0x1F600;

        if (gBosJfMiddlePillarLevel > gBosJfRightPillarLevel) {
            hi = 0x22E00;
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_DESCENDING;
            x = *px;

            if (x > hi) {
                *out = rightZ;
                return 0;
            }

            if (x > lo) {
                if (*pz <= middleZ) {
                    *out = middleZ;

                    if (x > hi - 0x1000)
                        gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
                } else {
                    *out = rightZ;
                    *px = hi;
                    return 1;
                }
            } else {
                if (*pz <= leftZ) {
                    *out = leftZ;

                    if (x > lo - 0x1000)
                        gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
                } else {
                    *out = middleZ;
                    *px = lo;
                    return 1;
                }
            }
        } else if (gBosJfMiddlePillarLevel < gBosJfRightPillarLevel) {
            hi = 0x21200;
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_VALLEY;
            x = *px;

            if (x <= lo) {
                if (*pz <= leftZ) {
                    *out = leftZ;

                    if (x > lo - 0x1000)
                        gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
                } else {
                    *out = middleZ;
                    *px = lo;
                    return 1;
                }
            } else {
                if (x < hi) {
                    *out = middleZ;
                    return 0;
                }

                if (*pz <= rightZ) {
                    *out = rightZ;

                    if (x < hi + 0x1000)
                        gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
                } else {
                    *out = middleZ;
                    *px = hi;
                    return 1;
                }
            }
        } else {
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_LEFT_HIGH;
            x = *px;

            if (x > lo) {
                *out = middleZ;
                return 0;
            }

            if (*pz <= leftZ) {
                *out = leftZ;

                if (x > lo - 0x1000)
                    gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
            } else {
                *out = middleZ;
                *px = lo;
                return 1;
            }
        }
    } else if (gBosJfLeftPillarLevel < gBosJfMiddlePillarLevel) {
        lo = 0x1DA00;

        if (gBosJfMiddlePillarLevel > gBosJfRightPillarLevel) {
            hi = 0x22E00;
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_PEAK;
            x = *px;

            if (x < lo) {
                *out = leftZ;
                return 0;
            }

            if (x > hi) {
                *out = rightZ;
                return 0;
            }

            if (*pz <= middleZ) {
                *out = middleZ;

                if ((x < lo + 0x1000) || (x > hi - 0x1000))
                    gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
            } else {
                if (x <= 0x205FF) {
                    *out = leftZ;
                    *px = lo;
                    return 1;
                }

                *out = rightZ;
                *px = hi;
                return 1;
            }
        } else if (gBosJfMiddlePillarLevel < gBosJfRightPillarLevel) {
            hi = 0x21200;
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_ASCENDING;
            x = *px;

            if (x < lo) {
                *out = leftZ;
                return 0;
            }

            if (x < hi) {
                if (*pz <= middleZ) {
                    *out = middleZ;

                    if (x < lo + 0x1000)
                        gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
                } else {
                    *out = leftZ;
                    *px = lo;
                    return 1;
                }
            } else {
                if (*pz <= rightZ) {
                    *out = rightZ;

                    if (x < hi + 0x1000)
                        gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
                } else {
                    *out = middleZ;
                    *px = hi;
                    return 1;
                }
            }
        } else {
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_LEFT_LOW;
            x = *px;

            if (x < lo) {
                *out = leftZ;
                return 0;
            }

            if (*pz <= middleZ) {
                *out = middleZ;

                if (x < lo + 0x1000)
                    gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
            } else {
                *out = leftZ;
                *px = lo;
                return 1;
            }
        }
    } else {
        if (gBosJfMiddlePillarLevel > gBosJfRightPillarLevel) {
            hi = 0x22E00;
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_RIGHT_LOW;
            x = *px;

            if (x > hi) {
                *out = rightZ;
                return 0;
            }

            if (*pz <= middleZ) {
                *out = middleZ;

                if (x > hi - 0x1000)
                    gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
            } else {
                *out = rightZ;
                *px = hi;
                return 1;
            }
        } else if (gBosJfMiddlePillarLevel < gBosJfRightPillarLevel) {
            hi = 0x21200;
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_RIGHT_HIGH;
            x = *px;

            if (x < hi) {
                *out = middleZ;
                return 0;
            }

            if (*pz <= rightZ) {
                *out = rightZ;

                if (x < hi + 0x1000)
                    gBtlWork->actor->flags |= BTLOBJ_FLAG_HIDE_SHADOW;
            } else {
                *out = middleZ;
                *px = hi;
                return 1;
            }
        } else {
            gBosJfPillarShape = BOS_JF_PILLAR_SHAPE_FLAT;
            leftZ = middleZ;
            *out = leftZ;
            return 0;
        }
    }

    return 0;
}

u8 BosJfGetGroundZ(s32* px, s32* py, s32* pz, s32* out) {
    s32 leftZ;
    s32 middleZ;
    s32 rightZ;

    leftZ = -((gBosJfLeftPillarLevel + 1) << 11);
    middleZ = -((gBosJfMiddlePillarLevel + 1) << 11);
    rightZ = -((gBosJfRightPillarLevel + 1) << 11);

    if (*px <= 0x259FF) {
        if (*px <= 0x221FF) {
            if (*px <= 0x1E9FF) {
                if (*px <= 0x1B1FF) {
                    *out = 0;

                    if (*px > 0x1AE00) {
                        return FALSE;
                    }
                } else {
                    *out = leftZ;

                    if (*px <= 0x1B5FF) {
                        return FALSE;
                    }

                    if (*px > 0x1E600 && leftZ != middleZ) {
                        return FALSE;
                    }
                }
            } else {
                *out = middleZ;

                if (*px <= 0x1EDFF && leftZ != middleZ) {
                    return FALSE;
                }

                if (*px > 0x21E00 && middleZ != rightZ) {
                    return FALSE;
                }
            }
        } else {
            *out = rightZ;

            if (*px <= 0x225FF && middleZ != rightZ) {
                return FALSE;
            }

            if (*px > 0x25600) {
                return FALSE;
            }
        }
    } else {
        *out = 0;

        if (*px <= 0x25DFF) {
            return FALSE;
        }
    }

    return TRUE;
}

void task_bos_jf_map_0(JfMapWork* work, JfMapArg* arg) {
    RequestDma3Copy(gBosJfBgMap2, gBosJfMapBuffer, sizeof(gBosJfBgMap2));
    gBosJfMapBlocks = arg->maps;
    BosJfDrawPillars();
    LoadBgTiles(0, arg->tiles, arg->tilesSize);
    LoadBgPalette(0, arg->palette, arg->paletteSize);
    gBtlWork->scale = Q_8_8(1);
    gBtlWork->zoomScale = Q_8_8(1);
    gBtlWork->x = 0x23E00;
    gBtlWork->y = 0x12800;
    gBtlWork->viewX = 0x23E00;
    gBtlWork->viewY = 0x12800;
    gBtlWork->x2 = 0x23E00;
    gBtlWork->y2 = 0x12800;
    gBtlWork->zoomX = 0x23E00;
    gBtlWork->zoomY = 0x12800;
    gBtlWork->zoomSteps = 0xF;
    gBtlWork->rotation = 0;
    BtlMapResetShake();
    ScrollBgMapTo(0, gBtlWork->viewX >> 8, gBtlWork->viewY >> 8);
    work->paletteTimer = 0;
    work->paletteFrame = 0;
    gBosJfShakeActive = FALSE;
    gBosJfShakeStep = 0;
    gBosJfShakeTimer = 0;
    gBosJfShakeDuration = 0;
    gBosJfShakeOffset = 0;
}

u8 task_bos_jf_map_1(JfMapWork* work) {
    s32 dx;
    s32 dy;

    work->paletteTimer++;

    if (work->paletteTimer > 14) {
        work->paletteTimer = 0;
        work->paletteFrame++;

        if (work->paletteFrame > 7) {
            work->paletteFrame = 0;
        }

        LoadPalette(gBosJfBgCyclePalettes + work->paletteFrame * 16, (void*)(BG_PLTT + PLTT_SIZE_4BPP), 0x20);
    }

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

    if (gBtlWork->viewX < (gBtlWork->xMin + 0x14) << 8) {
        gBtlWork->viewX = (gBtlWork->xMin + 0x14) << 8;
    } else if (gBtlWork->viewX > (gBtlWork->xMax - 0x1C) << 8) {
        gBtlWork->viewX = (gBtlWork->xMax - 0x1C) << 8;
    }

    if (gBtlWork->viewY < (gBtlWork->yMin - 0x90) << 8) {
        gBtlWork->viewY = (gBtlWork->yMin - 0x90) << 8;
    } else if (gBtlWork->viewY > (gBtlWork->yMax - 0x48) << 8) {
        gBtlWork->viewY = (gBtlWork->yMax - 0x48) << 8;
    }

    gBtlWork->viewY += BtlMapGetShake() + BosJfUpdateShake();
    ScrollBgMapTo(0, (gBtlWork->viewX >> 8) + 8, (gBtlWork->viewY >> 8) + 0x28);

    return 1;
}

void BosJfDrawPillars() {
    RequestMapRowsCopy(gBosJfPillarMaps[0][gBosJfLeftPillarLevel], gBosJfMapBuffer + 0x24c, 7, 0x17);
    RequestMapRowsCopy(gBosJfPillarMaps[1][gBosJfMiddlePillarLevel], gBosJfMapBuffer + 0x25a, 7, 0x17);
    RequestMapRowsCopy(gBosJfPillarMaps[0][gBosJfRightPillarLevel], gBosJfMapBuffer + 0x268, 7, 0x17);
    SetBgMapBlocks(0, gBosJfMapBlocks, 2, 2);
}

void BosJfStartShake(s16 duration) {
    gBosJfShakeActive = TRUE;
    gBosJfShakeStep = 0;
    gBosJfShakeTimer = 0;
    gBosJfShakeDuration = duration;
    gBosJfShakeOffset = 0;
}

s32 BosJfUpdateShake() {
    if (gBosJfShakeActive == TRUE) {
        gBosJfShakeTimer++;

        if (gBosJfShakeTimer < gBosJfShakeDuration) {
            if (gBosJfShakeStep % 4 == 0) {
                gBosJfShakeOffset = 0x200;
            } else if (gBosJfShakeStep % 4 == 2) {
                gBosJfShakeOffset = -0x200;
            }

            gBosJfShakeStep++;
        } else {
            gBosJfShakeActive = FALSE;
        }
    }

    return gBosJfShakeOffset;
}

enum BosJfLampState {
    BOS_JF_LAMP_STATE_FLY_OUT,
    BOS_JF_LAMP_STATE_HOVER_OUT,
    BOS_JF_LAMP_STATE_FLY_BACK,
    BOS_JF_LAMP_STATE_HOVER_BACK,
    BOS_JF_LAMP_STATE_LOW_PATROL,
    BOS_JF_LAMP_STATE_DEFEATED
};

void task_bos_jf_lamp_0(JfLampWork* work, JfWork* arg) {
    JfLampSpeed speed;

    speed.integer = 0;
    speed.fraction = 0x80;
    work->jf = arg;
    work->vx = speed.integer * 256 + speed.fraction;
    work->tiles = LoadObjTiles(gBosJfObjTiles, sizeof(gBosJfObjTiles));
    work->gfx = gBosJfObjFrames[12];
    work->tiles2 = LoadObjTiles(gBosJfObjTiles, sizeof(gBosJfObjTiles));
    work->gfx2 = gBosJfObjFrames[14];
    work->palette = LoadObjPalette(gBosJfObjPalette, sizeof(gBosJfObjPalette));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
    work->moveSteps = 0;
    work->tiles2Frame = 0;
    work->tiles2Timer = 0;
    work->voiceInterval = GetRandom() % 0x79 + 0x1E0;
    work->voiceTimer = 0;
    work->unk_24 = 1;
    work->onFlatGround = TRUE;
    work->state = BOS_JF_LAMP_STATE_FLY_OUT;
    work->stateTimer = 0;
    work->targetX = 0;
    work->angle = 0;
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, &(arg = work->jf)->sub);
}

u8 task_bos_jf_lamp_1(JfLampWork* work) {
    BtlObj* sub = &work->jf->sub;
    JfWork* jf = work->jf;
    ObjTiles* tiles;
    s32 dx;

    if (jf->state <= BOS_JF_STATE_SLAM) {
        if (++work->voiceTimer > work->voiceInterval) {
            work->voiceTimer = 0;
            m4aSongNumStart(SONG_VO_EG_DAMAGE00);
            work->voiceInterval = GetRandom() % 121 + 480;
        }
    }

    if (work->jf->state == BOS_JF_STATE_DEFEATED) {
        work->state = BOS_JF_LAMP_STATE_DEFEATED;
    }

    switch (work->state) {
    case BOS_JF_LAMP_STATE_FLY_OUT:
        if (work->stateTimer == 0) {
            work->targetX = BosJfLampChooseTargetX(work);
            dx = (s16)((work->targetX >> 8) - (sub->x >> 8));

            if (dx > 0) {
                sub->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            } else if (dx < 0) {
                sub->flags |= BTLOBJ_FLAG_FACING_LEFT;
            }

            work->moveSteps = 200;
            work->stateTimer++;
        } else if (work->moveSteps > 0) {
            ApproachValue(&sub->x, work->targetX, work->moveSteps);
            work->moveSteps--;
        } else {
            work->stateTimer = 0;
            work->state = BOS_JF_LAMP_STATE_HOVER_OUT;
        }

        sub->z = gSineTable[(u8)work->angle] * 20 - 0xB400;
        work->angle += 2;
        break;
    case BOS_JF_LAMP_STATE_HOVER_OUT:
        if (work->stateTimer == 0) {
            dx = (s16)((gBtlWork->actor->x >> 8) - (sub->x >> 8));

            if (dx > 0) {
                sub->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            } else if (dx < 0) {
                sub->flags |= BTLOBJ_FLAG_FACING_LEFT;
            }

            work->stateTimer++;
        } else if (work->stateTimer > 60) {
            work->stateTimer = 0;
            work->state = BOS_JF_LAMP_STATE_FLY_BACK;
        } else {
            work->stateTimer++;
        }

        sub->z = gSineTable[(u8)work->angle] * 20 - 0xB400;
        work->angle += 2;
        break;
    case BOS_JF_LAMP_STATE_FLY_BACK:
        if (work->stateTimer == 0) {
            if (jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->targetX = 0x27800;
            } else {
                work->targetX = 0x19400;
            }

            dx = (s16)((work->targetX >> 8) - (sub->x >> 8));

            if (dx > 0) {
                sub->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            } else if (dx < 0) {
                sub->flags |= BTLOBJ_FLAG_FACING_LEFT;
            }

            work->moveSteps = 200;
            work->stateTimer++;
        } else if (work->moveSteps > 0) {
            ApproachValue(&sub->x, work->targetX, work->moveSteps);
            ApproachValue(&sub->z, -0xB400, work->moveSteps);
            work->moveSteps--;
        } else {
            work->stateTimer = 0;
            work->state = BOS_JF_LAMP_STATE_HOVER_BACK;
        }

        sub->z = gSineTable[(u8)work->angle] * 20 - 0xB400;
        work->angle += 2;
        break;
    case BOS_JF_LAMP_STATE_HOVER_BACK:
        if (work->stateTimer == 0) {
            dx = (s16)((gBtlWork->actor->x >> 8) - (sub->x >> 8));

            if (dx > 0) {
                sub->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            } else if (dx < 0) {
                sub->flags |= BTLOBJ_FLAG_FACING_LEFT;
            }

            work->stateTimer++;
        } else if (work->stateTimer > 60) {
            work->stateTimer = 0;
            work->state = BOS_JF_LAMP_STATE_FLY_OUT;
        } else {
            work->stateTimer++;
        }

        sub->z = gSineTable[(u8)work->angle] * 20 - 0xB400;
        work->angle += 2;
        break;
    case BOS_JF_LAMP_STATE_LOW_PATROL:
        if (work->stateTimer == 0) {
            work->moveSteps = 20;
            sub->flags |= BTLOBJ_FLAG_FACING_LEFT;
            work->vx = -204;
            work->stateTimer++;
        } else {
            if (work->moveSteps > 0) {
                ApproachValue(&sub->z, -0xA000, work->moveSteps);
                work->moveSteps--;
            } else {
                if (sub->x <= 0x19400) {
                    sub->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                    work->vx = 204;
                }

                if (sub->x > 0x277FF) {
                    sub->flags |= BTLOBJ_FLAG_FACING_LEFT;
                    work->vx = -204;
                }

                sub->x += work->vx;
            }

            if (work->jf->gimmickTimer == 0) {
                work->stateTimer = 0;
                work->state = BOS_JF_LAMP_STATE_FLY_BACK;
            }
        }

        break;
    case BOS_JF_LAMP_STATE_DEFEATED:
        break;
    }

    if (work->jf->pillarPhase == BOS_JF_PILLAR_PHASE_GIMMICK) {
        work->stateTimer = 0;
        work->state = BOS_JF_LAMP_STATE_LOW_PATROL;
    }

    if (work->tiles2Timer > 3) {
        work->tiles2Timer = 0;
        work->tiles2Frame++;

        if (work->tiles2Frame > 5) {
            work->tiles2Frame = 0;
        }

        tiles = work->tiles2;
        RequestDma3Copy(gBosJfIagoTiles + (work->tiles2Frame << 9), (void*)(OBJ_VRAM0 + (tiles->index << 5)), 512);
    }

    work->tiles2Timer++;
    work->onFlatGround = BosJfGetGroundZ(&sub->x, &sub->y, &sub->z, &sub->groundZ);
    ColliderSetPosition(&sub->collider, sub->x, sub->y, sub->z);
    TaskPoolUpdate(&work->tasks);

    return 1;
}

void task_bos_jf_lamp_2(JfLampWork* work) {
    BtlObj* sub = &work->jf->sub;
    void* palette;
    u16 flags;
    s16 x;
    s16 y;

    flags = GetBattleSpritePriorityFlags(sub->y);

    if (sub->flags & BTLOBJ_FLAG_FACING_LEFT) {
        flags &= ~SPRITE_FLAG_HFLIP;
    } else {
        flags |= SPRITE_FLAG_HFLIP;
    }

    if (!gBtlWork->paused && (work->jf->flags & JF_FLAG_HURT) && (gFrameCounter & 1)) {
        palette = work->palette2;
    } else {
        palette = work->palette;
    }

    WorldToScreen(&x, &y, sub->x, sub->y, sub->z);
    DrawSprite(x, y, work->gfx, work->tiles, palette, NULL, flags, -4100 - (sub->y >> 8) * 4);
    DrawSprite(x, y - 14, work->gfx2, work->tiles2, work->palette, NULL, flags,
               -4101 - (sub->y >> 8) * 4);

    if (work->onFlatGround == TRUE) {
        TaskPoolDraw(&work->tasks);
    }
}

void task_bos_jf_lamp_3(JfLampWork* work) {
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    TaskPoolDestroy(&work->tasks);
}

s32 BosJfLampChooseTargetX(JfLampWork* work) {
    s16 roll;
    s32 targetX;

    if (work->jf->body.flags & BTLOBJ_FLAG_FACING_LEFT) {
        switch (gBosJfPillarShape) {
        case BOS_JF_PILLAR_SHAPE_LEFT_LOW:
        case BOS_JF_PILLAR_SHAPE_RIGHT_LOW:
            targetX = 0x27800;
            break;
        case BOS_JF_PILLAR_SHAPE_VALLEY:
        case BOS_JF_PILLAR_SHAPE_ASCENDING:
        case BOS_JF_PILLAR_SHAPE_RIGHT_HIGH:
            targetX = 0x21800;
            break;
        case BOS_JF_PILLAR_SHAPE_FLAT:
            roll = GetRandom() % 3;

            if (roll == 0) {
                targetX = 0x27800;
            } else if (roll == 1) {
                targetX = 0x24600;
            } else {
                targetX = 0x21800;
            }

            break;
        case BOS_JF_PILLAR_SHAPE_DESCENDING:
        case BOS_JF_PILLAR_SHAPE_LEFT_HIGH:
        case BOS_JF_PILLAR_SHAPE_PEAK:
            targetX = 0x24600;
            break;
        default:
            targetX = 0;
            break;
        }

        return targetX;
    }

    switch (gBosJfPillarShape) {
    case BOS_JF_PILLAR_SHAPE_DESCENDING:
    case BOS_JF_PILLAR_SHAPE_LEFT_HIGH:
        targetX = 0x1F400;
        break;
    case BOS_JF_PILLAR_SHAPE_PEAK:
    case BOS_JF_PILLAR_SHAPE_LEFT_LOW:
    case BOS_JF_PILLAR_SHAPE_RIGHT_HIGH:
        targetX = 0x19400;
        break;
    case BOS_JF_PILLAR_SHAPE_FLAT:
        roll = GetRandom() % 3;

        if (roll == 0) {
            targetX = 0x19400;
        } else if (roll == 1) {
            targetX = 0x1CE00;
        } else {
            targetX = 0x1F400;
        }

        break;
    case BOS_JF_PILLAR_SHAPE_VALLEY:
    case BOS_JF_PILLAR_SHAPE_ASCENDING:
    case BOS_JF_PILLAR_SHAPE_RIGHT_LOW:
        targetX = 0x1CE00;
        break;
    default:
        targetX = 0;
        break;
    }

    return targetX;
}

const EmyKind gBosJfEmyKind = { ENEMY_JAFAR, 1000, 16, 16, 24, 60, 0 };

TaskDesc gTaskDescBosJf = {
    "task_bos_jf",
    (TaskInitFunc)task_bos_jf_0,
    (TaskUpdateFunc)task_bos_jf_1,
    (TaskDrawFunc)task_bos_jf_2,
    (TaskDestroyFunc)task_bos_jf_3,
    sizeof(JfWork),
};

void* gBosJfPillarMaps[2][15] = {
    {
        gBosJfSidePillarMap0,
        gBosJfSidePillarMap0 + 7,
        gBosJfSidePillarMap0 + 14,
        gBosJfSidePillarMap0 + 21,
        gBosJfSidePillarMap1,
        gBosJfSidePillarMap1 + 7,
        gBosJfSidePillarMap1 + 14,
        gBosJfSidePillarMap1 + 21,
        gBosJfSidePillarMap2,
        gBosJfSidePillarMap2 + 7,
        gBosJfSidePillarMap2 + 14,
        gBosJfSidePillarMap2 + 21,
        gBosJfSidePillarMap3,
        gBosJfSidePillarMap3 + 7,
        gBosJfSidePillarMap3 + 14,
    },
    {
        gBosJfMiddlePillarMap0,
        gBosJfMiddlePillarMap0 + 7,
        gBosJfMiddlePillarMap0 + 14,
        gBosJfMiddlePillarMap0 + 21,
        gBosJfMiddlePillarMap1,
        gBosJfMiddlePillarMap1 + 7,
        gBosJfMiddlePillarMap1 + 14,
        gBosJfMiddlePillarMap1 + 21,
        gBosJfMiddlePillarMap2,
        gBosJfMiddlePillarMap2 + 7,
        gBosJfMiddlePillarMap2 + 14,
        gBosJfMiddlePillarMap2 + 21,
        gBosJfMiddlePillarMap3,
        gBosJfMiddlePillarMap3 + 7,
        gBosJfMiddlePillarMap3 + 14,
    },
};

TaskDesc gTaskDescBosJfMap = {
    "task_bos_jf_map",
    (TaskInitFunc)task_bos_jf_map_0,
    (TaskUpdateFunc)task_bos_jf_map_1,
    NULL,
    NULL,
    sizeof(JfMapWork),
};

u8 gUnk_09EF27EC[8] = { 0, 1, 1, 0, 0, 0, 0, 0 };

TaskDesc gTaskDescBosJfLamp = {
    "task_bos_jf_lamp",
    (TaskInitFunc)task_bos_jf_lamp_0,
    (TaskUpdateFunc)task_bos_jf_lamp_1,
    (TaskDrawFunc)task_bos_jf_lamp_2,
    (TaskDestroyFunc)task_bos_jf_lamp_3,
    sizeof(JfLampWork),
};
