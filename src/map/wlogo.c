/**
 * wlogo.c
 * World Logo Animations
 */

#include "macros.h"
#include "registration_data.h"
#include "intr.h"
#include "pallet.h"
#include "wlogo.h"
#include "sprites_boss_tm.h"
#include "sprites_wlogo.h"
#include "gba/io_reg.h"
#include "anim.h"
#include "display.h"
#include "engine_math.h"
#include "gba/defines.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"

extern const WlogoHwtObjA gWlogoHwtObjStarts[6];
extern const WlogoHwtObjB gWlogoHwtObjSteps[6][6];
extern const WlogoHwtObjB gWlogoNvlMovSteps[4];

extern WlogoWonEntry gWlogoWonCardsAlt[];
extern WlogoWonEntry gWlogoWonCards[];
extern s32 gWlogoFlipScales[];
extern WlogoAgrEntry gWlogoAgrEntries[];
extern WlogoTtMotion gWlogoTtMotion;
extern s16 gWlogoTtLinePoints[][3];
extern s8 gWlogoBksPaletteDurations[];
extern s8 gWlogoBksPaletteIndices[];
extern u8 gWlogoBksObjFrames[];
extern s16 gWlogoBksObjTargets[][2];
extern s16 gWlogoBksObjStarts[][2];
extern s16 gWlogoBksObjHoldTimes[];
extern u16 gWlogoBksObjPriorities[];
extern u8 gWlogoPooObjAnimIds[];

static TaskPool sWlogoHwtTaskPool;
static TaskPool sWlogoNvlTaskPool;
static TaskPool sWlogoNvlMovTaskPool;
static TaskPool sWlogoAgrTaskPool;
static TaskPool sWlogoPooTaskPool;
s32 gWlogoTtSkew EWRAM_COMMON(4);

enum WlogoMonsState {
    WLOGO_MONS_STATE_WAIT,
    WLOGO_MONS_STATE_FADE_IN,
    WLOGO_MONS_STATE_WAIT_EYE,
    WLOGO_MONS_STATE_EYE,
    WLOGO_MONS_STATE_HOLD,
    WLOGO_MONS_STATE_FADE_OUT
};

void task_wlogo_mons_0(WlogoMonsWork* work) {
    LoadBgPalette(0, gWlogoMonsPalette, sizeof(gWlogoMonsPalette));
    LoadBgTiles(0, gWlogoMonsTiles, sizeof(gWlogoMonsTiles));
    LoadBgMap(0, gWlogoMonsNoEyeMap, sizeof(gWlogoMonsNoEyeMap));
    work->tiles = LoadObjTiles(gWlogoMonsEyeTiles, sizeof(gWlogoMonsEyeTiles));
    work->palette = LoadObjPalette(gWlogoMonsPalette, sizeof(gWlogoMonsPalette));
    work->x = 64;
    work->y = 64;
    work->paletteStep = 0;
    work->timer = 0;
    work->state = WLOGO_MONS_STATE_WAIT;
    work->visible = FALSE;
    work->blend = 0;
    SetBgBlend(0, 16, 0);
    AnimInit(&work->anim, gWlogoMonsEyeAnims, gWlogoMonsEyeFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_mons_1(WlogoMonsWork* work) {
    switch (work->state) {
    case WLOGO_MONS_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_MONS_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_MONS_STATE_WAIT_EYE:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->visible = TRUE;
            work->state++;
        }

        break;
    case WLOGO_MONS_STATE_EYE:
        if (!AnimIsFinished(&work->anim)) {
            work->gfx = AnimUpdate(&work->anim);
        }

        work->timer++;

        if (work->timer > 7) {
            work->timer = 0;

            if (work->paletteStep <= 4) {
                LoadObjPaletteBank(BIOS_PTR(work->palette)->index, &gWlogoMonsPalettes[work->paletteStep * 16]);
                LoadPaletteWithEffect(&gWlogoMonsPalettes[work->paletteStep * 16], (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), 0x20);
            } else if (work->paletteStep > 11) {
                if (work->paletteStep <= 15) {
                    LoadObjPaletteBank(BIOS_PTR(work->palette)->index, &gWlogoMonsPalettes[(15 - work->paletteStep) * 16]);
                    LoadPaletteWithEffect(&gWlogoMonsPalettes[(15 - work->paletteStep) * 16], (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), 0x20);
                } else if (work->paletteStep == 20) {
                    work->visible = FALSE;
                    RequestDma3Copy(gWlogoMonsMap, GetBgScreenBase(0), sizeof(gWlogoMonsMap));
                    work->state++;
                }
            }

            work->paletteStep++;
        }

        break;
    case WLOGO_MONS_STATE_HOLD:
        work->timer++;

        if (work->timer > 49) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_MONS_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    return 1;
}

void task_wlogo_mons_2(WlogoMonsWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, 0, 0);
    }
}

void task_wlogo_mons_3(WlogoMonsWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoHwtState {
    WLOGO_HWT_STATE_GHOSTS,
    WLOGO_HWT_STATE_FADE_IN,
    WLOGO_HWT_STATE_HOLD,
    WLOGO_HWT_STATE_FADE_OUT,
    WLOGO_HWT_STATE_END_WAIT
};

void task_wlogo_hwt_0(WlogoHwtWork* work) {
    LoadBgPalette(0, gWlogoHwtPalette, sizeof(gWlogoHwtPalette));
    LoadBgTiles(0, gWlogoHwtTiles, sizeof(gWlogoHwtTiles));
    LoadBgMap(0, gWlogoHwtMap, sizeof(gWlogoHwtMap));
    work->paletteStep = 0;
    work->timer = 0;
    work->state = WLOGO_HWT_STATE_GHOSTS;
    work->blend = 0;
    SetBgBlend(0, 16, 0);
    TaskPoolInit(&sWlogoHwtTaskPool, 4);
}

u8 task_wlogo_hwt_1(WlogoHwtWork* work) {
    switch (work->state) {
    case WLOGO_HWT_STATE_GHOSTS:
        work->timer++;

        if (work->timer == 20) {
            TaskCreate(&sWlogoHwtTaskPool, &gTaskDescWlogoHwtObj, NULL);
            TaskCreate(&sWlogoHwtTaskPool, &gTaskDescWlogoHwtObj, (void*)1);
        }

        if (work->timer == 100) {
            TaskCreate(&sWlogoHwtTaskPool, &gTaskDescWlogoHwtObj, (void*)2);
        }

        if (work->timer == 140) {
            work->timer = 0;
            TaskCreate(&sWlogoHwtTaskPool, &gTaskDescWlogoHwtObj, (void*)3);
            work->state++;
        }

        break;
    case WLOGO_HWT_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_HWT_STATE_HOLD:
        work->timer++;

        if (work->timer > 119) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_HWT_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        if (work->timer == 0) {
            if (work->blend == 15) {
                TaskCreate(&sWlogoHwtTaskPool, &gTaskDescWlogoHwtObj, (void*)4);
            }

            if (work->blend == 12) {
                TaskCreate(&sWlogoHwtTaskPool, &gTaskDescWlogoHwtObj, (void*)5);
            }
        }

        break;
    case WLOGO_HWT_STATE_END_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            return 0;
        }

        break;
    }

    TaskPoolUpdate(&sWlogoHwtTaskPool);
    TaskPoolDraw(&sWlogoHwtTaskPool);
    return 1;
}

void task_wlogo_hwt_2(WlogoHwtWork* work) {
}

void task_wlogo_hwt_3(WlogoHwtWork* work) {
    TaskPoolDestroy(&sWlogoHwtTaskPool);
}

void task_wlogo_hwt_obj_0(WlogoHwtObjWork* work, s32 id) {
    work->id = id;
    work->tiles = LoadObjTiles(gWlogoHwtGhostTiles, sizeof(gWlogoHwtGhostTiles));
    work->palette = LoadObjPalette(gWlogoHwtPalette, sizeof(gWlogoHwtPalette));
    AnimInit(&work->anim, gWlogoHwtGhostAnims, gWlogoHwtGhostFrames);
    AnimStart(&work->anim, gWlogoHwtObjStarts[work->id].animId, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->x = gWlogoHwtObjStarts[work->id].x;
    work->y = gWlogoHwtObjStarts[work->id].y;
    work->vx = gWlogoHwtObjSteps[work->id][0].vx;
    work->vy = gWlogoHwtObjSteps[work->id][0].vy;
    work->ax = gWlogoHwtObjSteps[work->id][0].ax;
    work->ay = gWlogoHwtObjSteps[work->id][0].ay;
    work->unk_044 = gWlogoHwtObjStarts[work->id].unk_08;
    work->step = 0;
    work->stepTimer = 0;
    work->done = 0;
}

u8 task_wlogo_hwt_obj_1(WlogoHwtObjWork* work) {
    work->x += work->vx;
    work->y += work->vy;
    work->vx += work->ax;
    work->vy += work->ay;

    if (++work->stepTimer >= gWlogoHwtObjSteps[work->id][work->step].duration) {
        work->stepTimer = 0;

        if (gWlogoHwtObjSteps[work->id][work->step].isLast == TRUE) {
            return 0;
        }

        work->step++;
        work->vx = gWlogoHwtObjSteps[work->id][work->step].vx;
        work->vy = gWlogoHwtObjSteps[work->id][work->step].vy;
        work->ax = gWlogoHwtObjSteps[work->id][work->step].ax;
        work->ay = gWlogoHwtObjSteps[work->id][work->step].ay;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_wlogo_hwt_obj_2(WlogoHwtObjWork* work) {
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0);
}

void task_wlogo_hwt_obj_3(WlogoHwtObjWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoWonState {
    WLOGO_WON_STATE_WAIT,
    WLOGO_WON_STATE_CARDS,
    WLOGO_WON_STATE_FADE_OUT
};

void task_wlogo_won_0(WlogoWonWork* work) {
    s32 i;

    LoadBgPalette(0, gWlogoWonPalette, sizeof(gWlogoWonPalette));
    LoadBgTiles(0, gWlogoWonTiles, sizeof(gWlogoWonTiles));
    LoadBgMap(0, gWlogoWonMap, sizeof(gWlogoWonMap));
    work->tiles = LoadObjTiles(gWlogoWonCardTiles, sizeof(gWlogoWonCardTiles));
    work->palette = LoadObjPalette(gWlogoWonPalette, sizeof(gWlogoWonPalette));

    for (i = 0; i < 10; i++) {
        work->gfx[i] = gWlogoWonCardFrames[i];
        work->x[i] = gWlogoWonCards[i].x;
        work->y[i] = gWlogoWonCards[i].y;
        work->speedX[i] = gWlogoWonCards[i].speedX;
        work->scaleIndex[i] = gWlogoWonCards[i].scaleIndex;
        work->scaleTicks[i] = 0;
        work->cardTimers[i] = 0;
        work->cardPhases[i] = 0;
    }

    work->timer = 0;
    work->angle = 0;
    work->state = WLOGO_WON_STATE_WAIT;
    work->blend = 0;
    SetBgBlend(0, 16, 0);
}

u8 task_wlogo_won_1(WlogoWonWork* work) {
    s32 i;

    switch (work->state) {
    case WLOGO_WON_STATE_WAIT:
        work->timer++;

        if (work->timer > 9) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_WON_STATE_CARDS:
        for (i = 0; i < 10; i++) {
            if (work->cardPhases[i] == 0) {
                if (work->cardTimers[i] > gWlogoWonCards[i].delay) {
                    work->cardPhases[i]++;
                    work->cardTimers[i] = 0;
                }

                work->cardTimers[i]++;
                continue;
            }

            if (work->cardTimers[i] > gWlogoWonCards[i].duration) {
                work->cardPhases[i]++;
            } else {
                work->x[i] -= work->speedX[i];

                if (++work->scaleTicks[i] > 1) {
                    work->scaleTicks[i] = 0;

                    if (++work->scaleIndex[i] > 19) {
                        work->scaleIndex[i] = 0;
                    }
                }
            }

            work->cardTimers[i]++;
        }

        work->timer++;

        if (work->timer > 229) {
            work->timer = 0;
            work->blend = 16;
            SetBgBlend(0, 0, 16);
            work->state++;
        }

        break;
    case WLOGO_WON_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }
        }

        SetBgBlend(0, 16 - work->blend, work->blend);
        break;
    }

    return 1;
}

void task_wlogo_won_2(WlogoWonWork* work) {
    s32 i;
    ObjAffine* affine;

    if (work->state == WLOGO_WON_STATE_CARDS) {
        for (i = 0; i < 10; i++) {
            affine = AllocObjAffine(work->angle, gWlogoFlipScales[work->scaleIndex[i]], Q_8_8(1), FALSE);
            DrawSprite(work->x[i] >> 8, work->y[i] >> 8, work->gfx[i], work->tiles, work->palette, affine, 0, gWlogoWonCardsAlt[i].priority);
        }
    }
}

void task_wlogo_won_3(WlogoWonWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoState {
    WLOGO_STATE_WAIT,
    WLOGO_STATE_FADE_IN,
    WLOGO_STATE_HOLD,
    WLOGO_STATE_FADE_OUT
};

void task_wlogo_atl_0(WlogoAtlWork* work) {
    LoadBgPalette(0, gWlogoAtlPalette, sizeof(gWlogoAtlPalette));
    LoadBgTiles(0, gWlogoAtlTiles, sizeof(gWlogoAtlTiles));
    LoadBgMap(0, gWlogoAtlMap, sizeof(gWlogoAtlMap));
    RequestDma3Copy(gWlogoAtlFishTiles, (u8*)GetBgCharBase(0) + 32, 0x360);
    work->timer = 0;
    work->state = WLOGO_STATE_WAIT;
    work->blend = 0;
    work->tileFrame = 0;
    work->tileFrameTimer = 0;
    work->waveAmplitude = 4;
    work->waveTimer = 0;
    SetBgBlend(0, 16, 0);
    StartBgWave(WlogoAtlHBlankIntr);
    SetBgWaveParams(0, work->waveAmplitude, 4);
}

u8 task_wlogo_atl_1(WlogoAtlWork* work) {
    switch (work->state) {
    case WLOGO_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            EnableBgWave(0);
            work->state++;
        }

        break;
    case WLOGO_STATE_FADE_IN:
        work->waveTimer++;

        if (work->waveTimer > 39) {
            work->waveTimer = 0;
            work->waveAmplitude--;

            if (work->waveAmplitude <= 1) {
                work->waveAmplitude = 1;
            }

            SetBgWaveParams(0, work->waveAmplitude, 4);
        }

        work->timer++;

        if (work->timer > 7) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                StopBgWave(0);
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_STATE_HOLD:
        work->timer++;

        if (work->timer > 113) {
            work->timer = 0;
            work->state++;
        }

        work->tileFrameTimer++;
        break;
    case WLOGO_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        work->tileFrameTimer++;
        break;
    }

    if (work->tileFrameTimer > 9) {
        work->tileFrameTimer = 0;
        work->tileFrame++;

        if (work->tileFrame > 14) {
            work->tileFrame = 15;
        }

        RequestDma3Copy(&gWlogoAtlFishTiles[work->tileFrame * 1024], (u8*)GetBgCharBase(0) + 32, 0x360);
    }

    return 1;
}

void task_wlogo_atl_2(WlogoAtlWork* work) {
}

void task_wlogo_atl_3(WlogoAtlWork* work) {
    StopBgWave(0);
}

void WlogoAtlHBlankIntr() {
    gIntrCheck |= INTR_FLAG_HBLANK;
    HBlankIntrBgWave1(0);
}

void task_wlogo_nvl_0(WlogoNvlWork* work) {
    LoadBgPalette(0, gWlogoNvlPalette, sizeof(gWlogoNvlPalette));
    LoadBgTiles(0, gWlogoNvlTiles, sizeof(gWlogoNvlTiles));
    LoadBgMap(0, gWlogoNvlMap, sizeof(gWlogoNvlMap));
    RequestDma3Copy(gWlogoNvlShineTiles, GetBgCharBase(0), 0x340);
    work->timer = 0;
    work->state = WLOGO_STATE_WAIT;
    work->blend = 0;
    work->tileFrame = 0;
    work->tileFrameTimer = 0;
    work->frameCount = 0;
    SetBgBlend(0, 16, 0);
    TaskPoolInit(&sWlogoNvlTaskPool, 4);
}

u8 task_wlogo_nvl_1(WlogoNvlWork* work) {
    switch (work->state) {
    case WLOGO_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 5) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_STATE_HOLD:
        work->timer++;

        if (work->timer > 113) {
            work->timer = 0;
            work->state++;
        }

        if (work->timer == 1) {
            TaskCreate(&sWlogoNvlTaskPool, &gTaskDescWlogoNvlMov, NULL);
        }

        break;
    case WLOGO_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    if (work->frameCount >= 50 && work->frameCount <= 124) {
        work->tileFrameTimer++;

        if (work->tileFrameTimer > 14) {
            work->tileFrameTimer = 0;
            work->tileFrame++;

            if (work->tileFrame > 4) {
                work->tileFrame = 5;
            }

            RequestDma3Copy(&gWlogoNvlShineTiles[work->tileFrame * 1024], GetBgCharBase(0), 0x340);
        }
    } else if (work->frameCount >= 135 && work->frameCount <= 209) {
        work->tileFrameTimer++;

        if (work->tileFrameTimer > 14) {
            work->tileFrameTimer = 0;
            work->tileFrame--;

            if (work->tileFrame <= 0) {
                work->tileFrame = 0;
            }

            RequestDma3Copy(&gWlogoNvlShineTiles[work->tileFrame * 1024], GetBgCharBase(0), 0x340);
        }
    }

    work->frameCount++;
    TaskPoolUpdate(&sWlogoNvlTaskPool);
    TaskPoolDraw(&sWlogoNvlTaskPool);
    return 1;
}

void task_wlogo_nvl_2(WlogoNvlWork* work) {
}

void task_wlogo_nvl_3(WlogoNvlWork* work) {
    TaskPoolDestroy(&sWlogoNvlTaskPool);
}

void task_wlogo_nvl_mov_0(WlogoNvlMovWork* work) {
    work->stepTimer = 0;
    work->step = 0;
    work->x = 0x4E00;
    work->y = 0x5D00;
    work->vx = gWlogoNvlMovSteps[0].vx;
    work->vy = gWlogoNvlMovSteps[0].vy;
    work->ax = gWlogoNvlMovSteps[0].ax;
    work->ay = gWlogoNvlMovSteps[0].ay;
    work->frameCount = 0;
    work->trailAnimId = 1;
    work->done = 0;
    work->visible = TRUE;
    work->tiles = LoadObjTiles(gWlogoNvlSparkleTiles, sizeof(gWlogoNvlSparkleTiles));
    work->palette = LoadObjPalette(gWlogoNvlPalette, sizeof(gWlogoNvlPalette));
    work->animId = 3;
    AnimInit(&work->anim, gWlogoNvlSparkleAnims, gWlogoNvlSparkleFrames);
    AnimStart(&work->anim, work->animId, 0);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&sWlogoNvlMovTaskPool, 10);
}

u8 task_wlogo_nvl_mov_1(WlogoNvlMovWork* work) {
    WlogoNvlObjArg arg;

    if (work->done == 0) {
        work->x += work->vx;
        work->y += work->vy;
        work->vx += work->ax;
        work->vy += work->ay;

        if (++work->stepTimer >= gWlogoNvlMovSteps[work->step].duration) {
            work->stepTimer = 0;

            if (gWlogoNvlMovSteps[work->step].isLast == TRUE) {
                work->visible = FALSE;
                work->done++;
            }

            work->step++;
            work->vx = gWlogoNvlMovSteps[work->step].vx;
            work->vy = gWlogoNvlMovSteps[work->step].vy;
            work->ax = gWlogoNvlMovSteps[work->step].ax;
            work->ay = gWlogoNvlMovSteps[work->step].ay;
        }

        if (work->frameCount % 5 == 0) {
            arg.x = work->x;
            arg.y = work->y;
            arg.animId = work->trailAnimId;
            TaskCreate(&sWlogoNvlMovTaskPool, &gTaskDescWlogoNvlObj, &arg);
            work->trailAnimId = 1 - work->trailAnimId;
        }

        work->gfx = AnimUpdate(&work->anim);

        if (work->frameCount == 40) {
            AnimChangeWithTables(&work->anim, 2, 0, gWlogoNvlSparkleAnims, gWlogoNvlSparkleFrames);
        }

        if (work->frameCount == 55) {
            AnimChangeWithTables(&work->anim, 4, 0, gWlogoNvlSparkleAnims, gWlogoNvlSparkleFrames);
        }

        if (work->frameCount == 75) {
            AnimChangeWithTables(&work->anim, 2, 0, gWlogoNvlSparkleAnims, gWlogoNvlSparkleFrames);
        }

        work->frameCount++;
    } else {
        work->stepTimer++;

        if (work->stepTimer > 40) {
            return 0;
        }
    }

    TaskPoolUpdate(&sWlogoNvlMovTaskPool);
    TaskPoolDraw(&sWlogoNvlMovTaskPool);
    return 1;
}

void task_wlogo_nvl_mov_2(WlogoNvlMovWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 0);
    }
}

void task_wlogo_nvl_mov_3(WlogoNvlMovWork* work) {
    TaskPoolDestroy(&sWlogoNvlMovTaskPool);
}

void task_wlogo_nvl_obj_0(WlogoNvlObjWork* work, WlogoNvlObjArg* arg) {
    work->x = arg->x;
    work->y = arg->y;
    work->animId = arg->animId;
    work->tiles = LoadObjTiles(gWlogoNvlSparkleTiles, sizeof(gWlogoNvlSparkleTiles));
    work->palette = LoadObjPalette(gWlogoNvlPalette, sizeof(gWlogoNvlPalette));
    AnimInit(&work->anim, gWlogoNvlSparkleAnims, gWlogoNvlSparkleFrames);
    AnimStart(&work->anim, work->animId, 0);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_nvl_obj_1(WlogoNvlObjWork* work) {
    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_wlogo_nvl_obj_2(WlogoNvlObjWork* work) {
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 1);
}

void task_wlogo_nvl_obj_3(WlogoNvlObjWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoColState {
    WLOGO_COL_STATE_WAIT,
    WLOGO_COL_STATE_SPARKLE,
    WLOGO_COL_STATE_FADE_IN,
    WLOGO_COL_STATE_HOLD,
    WLOGO_COL_STATE_FADE_OUT
};

void task_wlogo_col_0(WlogoColWork* work) {
    LoadBgPalette(0, gWlogoColPalette, sizeof(gWlogoColPalette));
    LoadBgTiles(0, gWlogoColBlankTiles, sizeof(gWlogoColBlankTiles));
    LoadBgMap(0, gWlogoColMap, sizeof(gWlogoColMap));
    RequestDma3Copy(gWlogoColTiles, GetBgCharBase(0), 0x620);
    work->tiles = LoadObjTiles(gWlogoColSparkleTiles, sizeof(gWlogoColSparkleTiles));
    work->palette = LoadObjPalette(gWlogoColPalette, sizeof(gWlogoColPalette));
    work->x = gWlogoColSparkleAnim0.originX;
    work->y = gWlogoColSparkleAnim0.originY;
    work->timer = 0;
    work->state = WLOGO_COL_STATE_WAIT;
    work->blend = 0;
    work->tileFrame = 0;
    work->tileFrameTimer = 0;
    work->visible = FALSE;
    SetBgBlend(0, 16 - work->blend, work->blend);
    AnimInit(&work->anim, gWlogoColSparkleAnims, gWlogoColSparkleFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_col_1(WlogoColWork* work) {
    switch (work->state) {
    case WLOGO_COL_STATE_WAIT:
        work->timer++;

        if (work->timer > 9) {
            work->timer = 0;
            work->visible = TRUE;
            work->state++;
        }

        break;
    case WLOGO_COL_STATE_SPARKLE:
        if (AnimIsFinished(&work->anim)) {
            work->visible = FALSE;
            work->state++;
        } else {
            work->gfx = AnimUpdate(&work->anim);
        }

        break;
    case WLOGO_COL_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 1) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_COL_STATE_HOLD:
        if (++work->timer > 120) {
            work->timer = 0;
            work->state++;
        }

        if (++work->tileFrameTimer > 4) {
            work->tileFrameTimer = 0;
            work->tileFrame++;

            if (work->tileFrame > 10) {
                work->tileFrame = 11;
            } else {
                RequestDma3Copy(&gWlogoColShineTiles[(work->tileFrame - 1) * 2048], GetBgCharBase(0), 0x620);
            }
        }

        break;
    case WLOGO_COL_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    return 1;
}

void task_wlogo_col_2(WlogoColWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, 0, 0);
    }
}

void task_wlogo_col_3(WlogoColWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_wlogo_hlw_0(WlogoHlwWork* work) {
    LoadBgPalette(0, gWlogoHlwPalette, sizeof(gWlogoHlwPalette));
    LoadBgTiles(0, gWlogoHlwTiles, sizeof(gWlogoHlwTiles));
    LoadBgMap(0, gWlogoHlwMap, sizeof(gWlogoHlwMap));
    work->timer = 0;
    work->state = WLOGO_STATE_WAIT;
    work->blend = 0;
    SetBgBlend(0, 16, 0);
}

u8 task_wlogo_hlw_1(WlogoHlwWork* work) {
    switch (work->state) {
    case WLOGO_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 5) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_STATE_HOLD:
        work->timer++;

        if (work->timer > 113) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    return 1;
}

void task_wlogo_hlw_2(WlogoHlwWork* work) {
}

void task_wlogo_hlw_3(WlogoHlwWork* work) {
}

enum WlogoDilState {
    WLOGO_DIL_STATE_WAIT,
    WLOGO_DIL_STATE_FADE_IN,
    WLOGO_DIL_STATE_LOAD_NAME,
    WLOGO_DIL_STATE_FADE_IN_NAME,
    WLOGO_DIL_STATE_HOLD,
    WLOGO_DIL_STATE_FADE_OUT
};

void task_wlogo_dil_0(WlogoDilWork* work) {
    LoadBgPalette(0, gWlogoDilPalette, sizeof(gWlogoDilPalette));
    LoadBgTiles(0, gWlogoDilTiles, sizeof(gWlogoDilTiles));
    LoadBgMap(0, gWlogoDilIslandMap, sizeof(gWlogoDilIslandMap));
    work->tiles = LoadObjTiles(gWlogoDilIslandObjTiles, sizeof(gWlogoDilIslandObjTiles));
    work->palette = LoadObjPalette(gWlogoDilPalette, sizeof(gWlogoDilPalette));
    work->gfx = gWlogoDilIslandObjFrames[0];
    work->x = 64;
    work->y = 64;
    work->visible = FALSE;
    work->timer = 0;
    work->state = WLOGO_DIL_STATE_WAIT;
    work->blend = 0;
    SetBgBlend(0, 16, 0);
}

u8 task_wlogo_dil_1(WlogoDilWork* work) {
    switch (work->state) {
    case WLOGO_DIL_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_DIL_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 2) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->visible = TRUE;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_DIL_STATE_LOAD_NAME:
        LoadBgMap(0, gWlogoDilNameMap, sizeof(gWlogoDilNameMap));
        work->blend = 0;
        SetBgBlend(0, 16, 0);
        work->state++;
        break;
    case WLOGO_DIL_STATE_FADE_IN_NAME:
        work->timer++;

        if (work->timer > 2) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                LoadBgMap(0, gWlogoDilMap, sizeof(gWlogoDilMap));
                work->visible = FALSE;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_DIL_STATE_HOLD:
        work->timer++;

        if (work->timer > 113) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_DIL_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    return 1;
}

void task_wlogo_dil_2(WlogoDilWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0);
    }
}

void task_wlogo_dil_3(WlogoDilWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoAgrState {
    WLOGO_AGR_STATE_WAIT,
    WLOGO_AGR_STATE_FADE_IN_LAMP,
    WLOGO_AGR_STATE_LOAD_MAP,
    WLOGO_AGR_STATE_SMOKE,
    WLOGO_AGR_STATE_FLASHES,
    WLOGO_AGR_STATE_FADE_OUT
};

void task_wlogo_agr_0(WlogoAgrWork* work, s32 arg) {
    work->unk_017 = arg;
    LoadBgPalette(0, gWlogoAgrPalette, sizeof(gWlogoAgrPalette));
    LoadBgMap(0, gWlogoAgrLampMap, sizeof(gWlogoAgrLampMap));
    LoadBgTiles(0, gWlogoAgrTiles, sizeof(gWlogoAgrTiles));
    work->tiles = LoadObjTiles(gWlogoAgrObjTiles, sizeof(gWlogoAgrObjTiles));
    work->palette = LoadObjPalette(gWlogoAgrPalette, sizeof(gWlogoAgrPalette));
    work->gfx = gWlogoAgrObjFrames[11];
    work->x = 64;
    work->y = 64;
    work->visible = FALSE;
    work->timer = 0;
    work->state = WLOGO_AGR_STATE_WAIT;
    work->entryIndex = 0;
    work->blend = 0;
    SetBgBlend(0, 16, 0);
    TaskPoolInit(&sWlogoAgrTaskPool, 50);
}

u8 task_wlogo_agr_1(WlogoAgrWork* work) {
    WlogoAgrEntry smoke;
    WlogoAgrEntry flash;

    switch (work->state) {
    case WLOGO_AGR_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->state++;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash0, NULL);
        }

        break;
    case WLOGO_AGR_STATE_FADE_IN_LAMP:
        work->timer++;

        if (work->timer > 1) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->visible = TRUE;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_AGR_STATE_LOAD_MAP:
        LoadBgMap(0, gWlogoAgrMap, sizeof(gWlogoAgrMap));
        work->blend = 0;
        SetBgBlend(0, 16, 0);
        work->state++;
        break;
    case WLOGO_AGR_STATE_SMOKE:
        if (work->timer == gWlogoAgrEntries[work->entryIndex].time) {
            smoke.smokeX = gWlogoAgrEntries[work->entryIndex].smokeX;
            smoke.smokeY = gWlogoAgrEntries[work->entryIndex].smokeY;
            smoke.smokeAnimId = gWlogoAgrEntries[work->entryIndex].smokeAnimId;
            smoke.unk_07 = gWlogoAgrEntries[work->entryIndex].unk_07;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrSmoke, &smoke);
            flash.flashX = gWlogoAgrEntries[work->entryIndex].flashX;
            flash.flashY = gWlogoAgrEntries[work->entryIndex].flashY;
            flash.flashAnimId = gWlogoAgrEntries[work->entryIndex].flashAnimId;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
            flash.flashX = gWlogoAgrEntries[work->entryIndex].flashX + 20;
            flash.flashY = gWlogoAgrEntries[work->entryIndex].flashY + 20;
            flash.flashAnimId = gWlogoAgrEntries[work->entryIndex].flashAnimId + 1;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
            work->entryIndex++;

            if (work->entryIndex > 18) {
                work->timer = 0;
                work->entryIndex = 5;
                work->visible = FALSE;
                work->state++;
            }
        }

        if (work->timer >= 30 && work->timer <= 78) {
            if ((work->timer - 30) % 3 == 0) {
                work->blend++;

                if (work->blend > 15) {
                    work->blend = 16;
                }

                SetBgBlend(0, 16 - work->blend, work->blend);
            }
        }

        work->timer++;
        break;
    case WLOGO_AGR_STATE_FLASHES:
        if (work->timer <= 59) {
            if (work->timer % 20 == 0) {
                flash.flashX = gWlogoAgrEntries[work->entryIndex].flashX;
                flash.flashY = gWlogoAgrEntries[work->entryIndex].flashY;
                flash.flashAnimId = gWlogoAgrEntries[work->entryIndex].flashAnimId;
                TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
            }

            work->entryIndex++;

            if (work->entryIndex > 15) {
                work->entryIndex = 9;
            }
        }

        if (work->timer == 60) {
            flash.flashX = 115;
            flash.flashY = 80;
            flash.flashAnimId = 6;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
            flash.flashX = 110;
            flash.flashY = 60;
            flash.flashAnimId = 8;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
        }

        if (work->timer == 70) {
            flash.flashX = 95;
            flash.flashY = 65;
            flash.flashAnimId = 7;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
            flash.flashX = 100;
            flash.flashY = 80;
            flash.flashAnimId = 6;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
        }

        if (work->timer == 80) {
            flash.flashX = 134;
            flash.flashY = 42;
            flash.flashAnimId = 8;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
            flash.flashX = 126;
            flash.flashY = 50;
            flash.flashAnimId = 7;
            TaskCreate(&sWlogoAgrTaskPool, &gTaskDescWlogoAgrFlash1, &flash);
        }

        work->timer++;

        if (work->timer > 169) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_AGR_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    TaskPoolUpdate(&sWlogoAgrTaskPool);
    TaskPoolDraw(&sWlogoAgrTaskPool);
    return 1;
}

void task_wlogo_agr_2(WlogoAgrWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, 0, 3);
    }
}

void task_wlogo_agr_3(WlogoAgrWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    TaskPoolDestroy(&sWlogoAgrTaskPool);
}

void task_wlogo_agr_smoke_0(WlogoAgrSmokeWork* work, WlogoAgrEntry* arg) {
    work->x = arg->smokeX << 8;
    work->y = arg->smokeY << 8;
    work->animId = arg->smokeAnimId;
    work->unk_031 = arg->unk_07;
    work->unk_02C = 0x100;
    work->unk_032 = 0;
    work->unk_034 = 30;
    work->tiles = LoadObjTiles(gWlogoAgrObjTiles, sizeof(gWlogoAgrObjTiles));
    work->palette = LoadObjPalette(gWlogoAgrPalette, sizeof(gWlogoAgrPalette));
    AnimInit(&work->anim, gWlogoAgrObjAnims, gWlogoAgrObjFrames);
    AnimStart(&work->anim, work->animId, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_agr_smoke_1(WlogoAgrSmokeWork* work) {
    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_wlogo_agr_smoke_2(WlogoAgrSmokeWork* work) {
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 1);
}

void task_wlogo_agr_smoke_3(WlogoAgrSmokeWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_wlogo_agr_flash0_0(WlogoAgrFlashWork* work) {
    work->animId = 4;
    work->x = 64;
    work->y = 64;
    work->tiles = LoadObjTiles(gWlogoAgrObjTiles, sizeof(gWlogoAgrObjTiles));
    work->palette = LoadObjPalette(gWlogoAgrPalette, sizeof(gWlogoAgrPalette));
    AnimInit(&work->anim, gWlogoAgrObjAnims, gWlogoAgrObjFrames);
    AnimStart(&work->anim, work->animId, 0);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_agr_flash0_1(WlogoAgrFlashWork* work) {
    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_wlogo_agr_flash0_2(WlogoAgrFlashWork* work) {
    DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, 0, 2);
}

void task_wlogo_agr_flash0_3(WlogoAgrFlashWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_wlogo_agr_flash1_0(WlogoAgrFlashWork* work, WlogoAgrEntry* arg) {
    work->animId = arg->flashAnimId;
    work->x = arg->flashX;
    work->y = arg->flashY;
    work->tiles = LoadObjTiles(gWlogoAgrObjTiles, sizeof(gWlogoAgrObjTiles));
    work->palette = LoadObjPalette(gWlogoAgrPalette, sizeof(gWlogoAgrPalette));
    AnimInit(&work->anim, gWlogoAgrObjAnims, gWlogoAgrObjFrames);
    AnimStart(&work->anim, work->animId, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_agr_flash1_1(WlogoAgrFlashWork* work) {
    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_wlogo_agr_flash1_2(WlogoAgrFlashWork* work) {
    DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, 0, 0);
}

void task_wlogo_agr_flash1_3(WlogoAgrFlashWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoTvtState {
    WLOGO_TVT_STATE_WAIT,
    WLOGO_TVT_STATE_FADE_IN,
    WLOGO_TVT_STATE_LIGHTS,
    WLOGO_TVT_STATE_NAME,
    WLOGO_TVT_STATE_HOLD,
    WLOGO_TVT_STATE_FADE_OUT
};

void task_wlogo_tvt_0(WlogoTvtWork* work) {
    LoadBgPalette(0, gWlogoTvtPalette, sizeof(gWlogoTvtPalette));
    LoadBgTiles(0, gWlogoTvtSignTiles, sizeof(gWlogoTvtSignTiles));
    LoadBgMap(0, gWlogoTvtMap, sizeof(gWlogoTvtMap));
    RequestDma3Copy(gWlogoTvtLightTiles, (u8*)GetBgCharBase(0) + 32, 0x300);
    work->tiles = AllocObjTiles(0x780, gWlogoTvtNameTiles);
    work->palette = LoadObjPalette(gWlogoTvtPalette, sizeof(gWlogoTvtPalette));
    work->x = 64;
    work->y = 64;
    work->timer = 0;
    work->state = WLOGO_TVT_STATE_WAIT;
    work->blend = 0;
    work->tileFrame = 0;
    work->tileFrameTimer = 0;
    work->visible = FALSE;
    SetBgBlend(0, 16 - work->blend, work->blend);
    AnimInit(&work->anim, gWlogoTvtNameAnims, gWlogoTvtNameFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_tvt_1(WlogoTvtWork* work) {
    switch (work->state) {
    case WLOGO_TVT_STATE_WAIT:
        if (++work->timer > 29) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_TVT_STATE_FADE_IN:
        if (++work->tileFrameTimer > 44) {
            work->tileFrameTimer = 0;
            work->tileFrame = 1 - work->tileFrame;
            RequestDma3Copy(&gWlogoTvtLightTiles[work->tileFrame * 1024], (u8*)GetBgCharBase(0) + 32, 0x320);
        }

        if (++work->timer > 3) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_TVT_STATE_LIGHTS:
        if (++work->tileFrameTimer > 44) {
            work->tileFrameTimer = 0;
            work->tileFrame = 1 - work->tileFrame;
            RequestDma3Copy(&gWlogoTvtLightTiles[work->tileFrame * 1024], (u8*)GetBgCharBase(0) + 32, 0x320);
        }

        if (++work->timer > 89) {
            work->timer = 0;
            work->visible = TRUE;
            work->state++;
        }

        break;
    case WLOGO_TVT_STATE_NAME:
        if (++work->tileFrameTimer > 44) {
            work->tileFrameTimer = 0;
            work->tileFrame = 1 - work->tileFrame;
            RequestDma3Copy(&gWlogoTvtLightTiles[work->tileFrame * 1024], (u8*)GetBgCharBase(0) + 32, 0x320);
        }

        if (AnimIsFinished(&work->anim)) {
            LoadBgTiles(0, gWlogoTvtTiles, 0xC00);
            work->tileFrame += 2;
            RequestDma3Copy(&gWlogoTvtLightTiles[work->tileFrame * 1024], (u8*)GetBgCharBase(0) + 32, 0x320);
            work->visible = FALSE;
            work->state++;
        } else {
            work->gfx = AnimUpdate(&work->anim);
        }

        break;
    case WLOGO_TVT_STATE_HOLD:
        if (++work->tileFrameTimer > 44) {
            work->tileFrameTimer = 0;
            work->tileFrame = 5 - work->tileFrame;
            RequestDma3Copy(&gWlogoTvtLightTiles[work->tileFrame * 1024], (u8*)GetBgCharBase(0) + 32, 0x320);
        }

        if (++work->timer > 113) {
            work->timer = 0;
            work->tileFrame = 2;
            work->state++;
        }

        break;
    case WLOGO_TVT_STATE_FADE_OUT:
        if (++work->tileFrameTimer > 44) {
            work->tileFrameTimer = 0;
            work->tileFrame = 5 - work->tileFrame;
            RequestDma3Copy(&gWlogoTvtLightTiles[work->tileFrame * 1024], (u8*)GetBgCharBase(0) + 32, 0x320);
        }

        if (++work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    return 1;
}

void task_wlogo_tvt_2(WlogoTvtWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, 0, 0);
    }
}

void task_wlogo_tvt_3(WlogoTvtWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_wlogo_poo_0(WlogoPooWork* work) {
    LoadBgPalette(0, gWlogoPooPalette, sizeof(gWlogoPooPalette));
    LoadBgTiles(0, gWlogoPooTiles, sizeof(gWlogoPooTiles));
    LoadBgMap(0, gWlogoPooMap, sizeof(gWlogoPooMap));
    work->timer = 0;
    work->state = WLOGO_STATE_WAIT;
    work->blend = 0;
    work->tileFrame = 0;
    work->tileFrameTimer = 0;
    SetBgBlend(0, 16, 0);
    TaskPoolInit(&sWlogoPooTaskPool, 4);
}

u8 task_wlogo_poo_1(WlogoPooWork* work) {
    switch (work->state) {
    case WLOGO_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 5) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                TaskCreate(&sWlogoPooTaskPool, &gTaskDescWlogoPooObj, NULL);
                TaskCreate(&sWlogoPooTaskPool, &gTaskDescWlogoPooObj, (void*)1);
                TaskCreate(&sWlogoPooTaskPool, &gTaskDescWlogoPooObj, (void*)2);
                TaskCreate(&sWlogoPooTaskPool, &gTaskDescWlogoPooObj, (void*)3);
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_STATE_HOLD:
        work->timer++;

        if (work->timer > 113) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    TaskPoolUpdate(&sWlogoPooTaskPool);
    TaskPoolDraw(&sWlogoPooTaskPool);
    return 1;
}

void task_wlogo_poo_2(WlogoPooWork* work) {
}

void task_wlogo_poo_3(WlogoPooWork* work) {
    TaskPoolDestroy(&sWlogoPooTaskPool);
}

void task_wlogo_poo_obj_0(WlogoPooObjWork* work, s32 id) {
    work->id = id;
    work->tiles = LoadObjTiles(gWlogoPooBeeTiles, sizeof(gWlogoPooBeeTiles));
    work->palette = LoadObjPalette(gWlogoPooPalette, sizeof(gWlogoPooPalette));
    work->x = 0x8200;
    work->y = 0x4000;
    work->vx = gWlogoPooObjSteps[work->id][0].vx;
    work->vy = gWlogoPooObjSteps[work->id][0].vy;
    work->ax = gWlogoPooObjSteps[work->id][0].ax;
    work->ay = gWlogoPooObjSteps[work->id][0].ay;
    work->step = 0;
    work->stepTimer = 0;
    work->done = 0;
    work->visible = TRUE;
    work->animId = gWlogoPooObjAnimIds[work->id];
    AnimInit(&work->anim, gWlogoPooBeeAnims, gWlogoPooBeeFrames);
    AnimStart(&work->anim, work->animId, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_poo_obj_1(WlogoPooObjWork* work) {
    work->x += work->vx;
    work->y += work->vy;
    work->vx += work->ax;
    work->vy += work->ay;
    work->gfx = AnimUpdate(&work->anim);

    if (work->done != 0) {
        if (work->stepTimer > 150) {
            return 0;
        }
    } else if (work->stepTimer == gWlogoPooObjSteps[work->id][work->step].duration) {
        work->stepTimer = 0;
        work->vx = gWlogoPooObjSteps[work->id][work->step].vx;
        work->vy = gWlogoPooObjSteps[work->id][work->step].vy;
        work->ax = gWlogoPooObjSteps[work->id][work->step].ax;
        work->ay = gWlogoPooObjSteps[work->id][work->step].ay;
        work->step++;

        if (work->step > 4) {
            work->done++;
        }
    }

    work->stepTimer++;
    return 1;
}

void task_wlogo_poo_obj_2(WlogoPooObjWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 0);
    }
}

void task_wlogo_poo_obj_3(WlogoPooObjWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoTtState {
    WLOGO_TT_STATE_WAIT,
    WLOGO_TT_STATE_FADE_IN,
    WLOGO_TT_STATE_SKEW,
    WLOGO_TT_STATE_FIRST_GLOW,
    WLOGO_TT_STATE_FIRST_STRETCH,
    WLOGO_TT_STATE_SECOND_GLOW,
    WLOGO_TT_STATE_SECOND_STRETCH,
    WLOGO_TT_STATE_BG_OUT,
    WLOGO_TT_STATE_PALETTE_UP,
    WLOGO_TT_STATE_LOAD_NAME,
    WLOGO_TT_STATE_GLOW_END,
    WLOGO_TT_STATE_PALETTE_DOWN,
    WLOGO_TT_STATE_HOLD,
    WLOGO_TT_STATE_FADE_OUT
};

void task_wlogo_tt_0(WlogoTtWork* work) {
    s32 i;

    LoadBgPalette(0, gWlogoTtPalette, sizeof(gWlogoTtPalette));
    LoadBgTiles(0, gWlogoTtTiles, sizeof(gWlogoTtTiles));
    LoadBgMap(0, gWlogoTtTownMap, sizeof(gWlogoTtTownMap));
    LoadPalette(gWlogoTtPalettes[15], (void*)(BG_PLTT + 15 * PLTT_SIZE_4BPP), sizeof(gWlogoTtPalettes[0]));
    work->timer = 0;
    work->subStep = 0;
    work->state = WLOGO_TT_STATE_WAIT;
    work->blend = 0;
    work->paletteStep = 0;
    work->scaleX = Q_8_8(0.2);
    work->scaleX2 = Q_8_8(0.2);
    SetBgBlend(0, 16 - work->blend, work->blend);
    work->tiles = AllocObjTiles(0x200, gWlogoTtGlowTiles);
    work->tiles2 = AllocObjTiles(0x200, gWlogoTtGlowTiles);
    work->tiles3 = AllocObjTiles(0x200, gWlogoTtGlowTiles);
    work->tiles4 = AllocObjTiles(0x3C0, gWlogoTtGlowTiles);
    work->tiles5 = LoadObjTiles(gWlogoTtObjTiles, sizeof(gWlogoTtObjTiles));
    work->palette = LoadObjPalette(gWlogoTtPalette, sizeof(gWlogoTtPalette));
    LoadObjPaletteBank(BIOS_PTR(work->palette)->index, gWlogoTtPalettes[work->paletteStep]);
    AnimInit(&work->anim[0], gWlogoTtGlowAnims, gWlogoTtGlowFrames);
    AnimStart(&work->anim[0], 1, 0);
    work->gfx = AnimGetGfx(&work->anim[0]);
    AnimInit(&work->anim[1], gWlogoTtGlowAnims, gWlogoTtGlowFrames);
    AnimStart(&work->anim[1], 2, 0);
    work->gfx2 = AnimGetGfx(&work->anim[1]);
    AnimInit(&work->anim[2], gWlogoTtGlowAnims, gWlogoTtGlowFrames);
    AnimStart(&work->anim[2], 4, 0);
    work->gfx3 = AnimGetGfx(&work->anim[2]);
    AnimInit(&work->anim[3], gWlogoTtGlowAnims, gWlogoTtGlowFrames);
    AnimStart(&work->anim[3], 4, 0);
    work->gfx4 = AnimGetGfx(&work->anim[3]);
    AnimInit(&work->anim[4], gWlogoTtGlowAnims, gWlogoTtGlowFrames);
    AnimStart(&work->anim[4], 0, 0);
    work->gfx5 = AnimGetGfx(&work->anim[4]);
    AnimInit(&work->anim[5], gWlogoTtGlowAnims, gWlogoTtGlowFrames);
    AnimStart(&work->anim[5], 0, 0);
    work->gfx6 = AnimGetGfx(&work->anim[5]);
    work->gfx7 = gWlogoTtObjFrames[0];

    for (i = 0; i < 8; i++) {
        work->visible[i] = 0;
    }

    TaskPoolInit(&work->tasks, 1);
    WlogoEnableHBlank();
    gWlogoTtSkew = -0x299;
    work->scrollSpeed = 25;
}

u8 task_wlogo_tt_1(WlogoTtWork* work) {
    switch (work->state) {
    case WLOGO_TT_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_TT_STATE_FADE_IN:
        work->timer++;

        if (work->timer > 3) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
                work->state++;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    case WLOGO_TT_STATE_SKEW:
        gWlogoTtSkew += work->scrollSpeed;
        work->scrollSpeed = work->scrollSpeed;

        if (gWlogoTtSkew > 332) {
            work->visible[2] = 1;
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_TT_STATE_FIRST_GLOW:
        work->gfx3 = AnimUpdate(&work->anim[2]);

        if (work->timer > 13) {
            work->timer = 0;
            work->subStep = 0;
            work->scaleX = gWlogoTtMotion.first[0];
            work->visible[0] = 1;
            work->state++;
        } else {
            work->timer++;
        }

        break;
    case WLOGO_TT_STATE_FIRST_STRETCH:
        if (work->timer > 10) {
            work->timer = 0;
            work->subStep++;

            if (work->subStep > 4) {
                work->subStep = 4;
            }

            work->scaleX = gWlogoTtMotion.first[work->subStep];
        }

        work->timer++;

        if (AnimIsFinished(&work->anim[2])) {
            work->visible[2] = 0;
            work->visible[3] = 1;
            work->timer = 0;
            work->state++;
        } else {
            work->gfx = AnimUpdate(&work->anim[0]);
            work->gfx3 = AnimUpdate(&work->anim[2]);
        }

        break;
    case WLOGO_TT_STATE_SECOND_GLOW:
        work->gfx4 = AnimUpdate(&work->anim[3]);

        if (work->timer > 13) {
            work->timer = 0;
            work->subStep = 0;
            work->scaleX2 = gWlogoTtMotion.second[0];
            work->visible[1] = 1;
            work->state++;
        } else {
            work->timer++;
        }

        break;
    case WLOGO_TT_STATE_SECOND_STRETCH:
        if (work->timer > 13) {
            work->timer = 0;
            work->subStep++;

            if (work->subStep > 2) {
                work->subStep = 2;
            }

            work->scaleX2 = gWlogoTtMotion.second[work->subStep];
        }

        work->timer++;

        if (AnimIsFinished(&work->anim[3])) {
            work->visible[3] = 0;
            work->visible[4] = 1;
            work->visible[5] = 1;
            work->visible[6] = 1;
            TaskCreate(&work->tasks, &gTaskDescWlogoTtLine, NULL);
            work->timer = 0;
            work->subStep = 0;
            work->state++;
        } else {
            work->gfx2 = AnimUpdate(&work->anim[1]);
            work->gfx4 = AnimUpdate(&work->anim[3]);
        }

        break;
    case WLOGO_TT_STATE_BG_OUT:
        if (work->blend != 0) {
            work->blend--;

            if (work->blend == 0) {
                DisableBg(0);
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        } else if (work->timer == 0) {
            WlogoDisableHBlank();
            work->timer++;
        } else if (work->subStep > 1) {
            work->subStep = 0;
            work->paletteStep++;

            if (work->paletteStep == 4) {
                work->state++;
            }

            LoadObjPaletteBank(BIOS_PTR(work->palette)->index, gWlogoTtPalettes[work->paletteStep]);
        } else {
            work->subStep++;
        }

        work->gfx5 = AnimUpdate(&work->anim[4]);
        work->gfx6 = AnimUpdate(&work->anim[5]);
        break;
    case WLOGO_TT_STATE_PALETTE_UP:
        if (work->subStep > 1) {
            work->subStep = 0;
            work->paletteStep++;

            if (work->paletteStep > 8) {
                work->state++;
            } else {
                LoadObjPaletteBank(BIOS_PTR(work->palette)->index, gWlogoTtPalettes[work->paletteStep]);
            }
        } else {
            work->subStep++;
        }

        work->gfx5 = AnimUpdate(&work->anim[4]);
        work->gfx6 = AnimUpdate(&work->anim[5]);
        break;
    case WLOGO_TT_STATE_LOAD_NAME:
        work->visible[6] = 0;
        LoadBgMap(0, gWlogoTtMap, sizeof(gWlogoTtMap));
        LoadPalette(gWlogoTtPalettes[8], (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), sizeof(gWlogoTtPalettes[0]));
        EnableBg(0);
        work->blend = 16;
        SetBgBlend(0, 0, 16);
        work->visible[0] = 0;
        work->visible[1] = 0;
        work->gfx5 = AnimUpdate(&work->anim[4]);
        work->gfx6 = AnimUpdate(&work->anim[5]);
        work->state++;
        break;
    case WLOGO_TT_STATE_GLOW_END:
        if (AnimIsFinished(&work->anim[4])) {
            work->visible[4] = 0;
            work->visible[5] = 0;
            work->subStep = 0;
            work->timer = 0;
            work->paletteStep = 8;
            work->state++;
        } else {
            work->gfx5 = AnimUpdate(&work->anim[4]);
            work->gfx6 = AnimUpdate(&work->anim[5]);
        }

        break;
    case WLOGO_TT_STATE_PALETTE_DOWN:
        if (work->paletteStep == 4) {
            work->timer = 0;
            work->state++;
        } else if (work->subStep > 1) {
            work->subStep = 0;
            work->paletteStep--;
            LoadPalette(gWlogoTtPalettes[work->paletteStep], (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), sizeof(gWlogoTtPalettes[0]));
        } else {
            work->subStep++;
        }

        break;
    case WLOGO_TT_STATE_HOLD:
        work->timer++;

        if (work->timer > 113) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_TT_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    default:
        return 0;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_wlogo_tt_2(WlogoTtWork* work) {
    ObjAffine* affine;

    if (work->visible[4] == 1) {
        DrawSprite(72, 64, work->gfx5, work->tiles, work->palette, NULL, 0, 4);
    }

    if (work->visible[5] == 1) {
        DrawSprite(96, 80, work->gfx6, work->tiles, work->palette, NULL, 0, 6);
    }

    if (work->visible[0] == 1) {
        affine = AllocObjAffine(0, work->scaleX, Q_8_8(1), FALSE);
        DrawSprite(78, 72, work->gfx, work->tiles2, work->palette, affine, 0, 24);
    }

    if (work->visible[2] == 1) {
        DrawSprite(65, 65, work->gfx3, work->tiles4, work->palette, NULL, 0, 20);
    }

    if (work->visible[1] == 1) {
        affine = AllocObjAffine(0, work->scaleX2, Q_8_8(1), FALSE);
        DrawSprite(108, 94, work->gfx2, work->tiles3, work->palette, affine, 0, 26);
    }

    if (work->visible[3] == 1) {
        DrawSprite(96, 87, work->gfx4, work->tiles4, work->palette, NULL, 0, 22);
    }

    if (work->visible[6] == 1) {
        DrawSprite(64, 64, work->gfx7, work->tiles5, work->palette, NULL, 0, 32);
    }

    TaskPoolDraw(&work->tasks);
}

void task_wlogo_tt_3(WlogoTtWork* work) {
    WlogoDisableHBlank();
    TaskPoolDestroy(&work->tasks);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette);
}

void WlogoEnableHBlank() {
    SetHBlankCallback(WlogoHBlankIntr);
    EnableHBlankIntr();
}

void WlogoHBlankIntr() {
    vu16 line;

    gIntrCheck |= INTR_FLAG_HBLANK;
    line = REG_VCOUNT;
    line = (line + 1) % 228;

    if (line <= 96) {
        REG_BGHOFS(0) = 0;
    } else {
        REG_BGHOFS(0) = gWlogoTtSkew * (line - 97) >> 8;
    }
}

void WlogoDisableHBlank() {
    ResetHBlankCallback();
    DisableHBlankIntr();
}

void task_wlogo_tt_obj_0(WlogoTtObjWork* work, WlogoTtObjArg* arg) {
    work->x = arg->x;
    work->y = arg->y;
    work->unk_02C = 0;
    work->unk_02E = 0;
    work->tiles = LoadObjTiles(gWlogoTtObjTiles, sizeof(gWlogoTtObjTiles));
    work->palette = LoadObjPalette(gWlogoTtPalette, sizeof(gWlogoTtPalette));
    AnimInit(&work->anim, gWlogoTtObjAnims, gWlogoTtObjFrames);
    AnimStart(&work->anim, 1, 0);
    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_wlogo_tt_obj_1(WlogoTtObjWork* work) {
    work->x += 0x100;

    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_wlogo_tt_obj_2(WlogoTtObjWork* work) {
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 16);
}

void task_wlogo_tt_obj_3(WlogoTtObjWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

enum WlogoTtLineState {
    WLOGO_TT_LINE_STATE_SPAWN,
    WLOGO_TT_LINE_STATE_WAIT
};

void task_wlogo_tt_line_0(WlogoTtLineWork* work) {
    work->timer = 0;
    work->index = 0;
    work->state = WLOGO_TT_LINE_STATE_SPAWN;
    TaskPoolInit(&work->tasks, 33);
}

u8 task_wlogo_tt_line_1(WlogoTtLineWork* work) {
    WlogoTtObjArg arg;

    switch (work->state) {
    case WLOGO_TT_LINE_STATE_SPAWN:
        if (work->timer == gWlogoTtLinePoints[work->index][2]) {
            work->timer = 0;
            arg.x = gWlogoTtLinePoints[work->index][0] << 8;
            arg.y = gWlogoTtLinePoints[work->index][1] << 8;
            TaskCreate(&work->tasks, &gTaskDescWlogoTtObj, &arg);
            work->index++;

            if (work->index == 31) {
                work->state++;
            }
        } else {
            work->timer++;
        }

        break;
    case WLOGO_TT_LINE_STATE_WAIT:
        if (work->timer > 29) {
            work->state++;
        }

        work->timer++;
        break;
    default:
        return 0;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_wlogo_tt_line_2(WlogoTtLineWork* work) {
    TaskPoolDraw(&work->tasks);
}

void task_wlogo_tt_line_3(WlogoTtLineWork* work) {
    TaskPoolDestroy(&work->tasks);
}

enum WlogoBksState {
    WLOGO_BKS_STATE_WAIT,
    WLOGO_BKS_STATE_LETTERS,
    WLOGO_BKS_STATE_SHOW_NAME,
    WLOGO_BKS_STATE_NAME,
    WLOGO_BKS_STATE_HOLD,
    WLOGO_BKS_STATE_FADE_OUT
};

void task_wlogo_bks_0(WlogoBksWork* work) {
    LoadBgPalette(0, gWlogoBksPalette, sizeof(gWlogoBksPalette));
    LoadBgTiles(0, gWlogoBksTiles, sizeof(gWlogoBksTiles));
    LoadBgMap(0, gWlogoBksCastleMap, sizeof(gWlogoBksCastleMap));
    work->timer = 0;
    work->paletteStep = 0;
    work->state = WLOGO_BKS_STATE_WAIT;
    work->blend = 0;
    work->tileFrame = 0;
    work->tileFrameTimer = 0;
    work->frameCount = 0;
    work->visible = FALSE;
    SetBgBlend(0, 16 - work->blend, work->blend);
    TaskPoolInit(&work->tasks, 15);
    work->tiles = AllocObjTiles(0x580, gWlogoBksNameTiles);
    work->palette = LoadObjPalette(gWlogoBksPalette, sizeof(gWlogoBksPalette));
    AnimInit(&work->anim, gWlogoBksNameAnims, gWlogoBksNameFrames);
    AnimStart(&work->anim, 12, 0);
    work->gfx = AnimGetGfx(&work->anim);
    work->waveTimer = 0;
    work->unk_038 = 0;
    work->waveAmplitude = 4;
    work->waveFrequency = 6;
    StartBgWave(WlogoBksHBlankIntr);
    SetBgWaveParams(0, work->waveAmplitude, work->waveFrequency);
}

u8 task_wlogo_bks_1(WlogoBksWork* work) {
    switch (work->state) {
    case WLOGO_BKS_STATE_WAIT:
        work->timer++;

        if (work->timer > 29) {
            work->timer = 0;
            EnableBgWave(0);
            work->state++;
        }

        break;
    case WLOGO_BKS_STATE_LETTERS:
        work->timer++;

        if (work->timer > 5) {
            work->timer = 0;
            work->blend++;

            if (work->blend > 15) {
                work->blend = 16;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        work->waveTimer++;

        if (work->waveTimer > 29) {
            work->waveTimer = 0;

            if (work->waveAmplitude <= 1) {
                work->waveAmplitude = 1;
                StopBgWave(0);
            } else {
                work->waveAmplitude--;
                SetBgWaveParams(0, work->waveAmplitude, work->waveFrequency);
            }
        }

        switch (work->frameCount) {
        case 20:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)13);
            break;
        case 30:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)12);
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, NULL);
            break;
        case 40:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)11);
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)1);
            break;
        case 50:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)10);
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)2);
            break;
        case 60:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)9);
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)3);
            break;
        case 70:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)4);
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)8);
            break;
        case 80:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)7);
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)5);
            break;
        case 90:
            TaskCreate(&work->tasks, &gTaskDescWlogoBksObj, (void*)6);
            break;
        case 115:
            work->state++;
            break;
        }

        work->frameCount++;
        break;
    case WLOGO_BKS_STATE_SHOW_NAME:
        work->visible = TRUE;
        StopBgWave(0);
        work->paletteStep = 1;
        work->timer = 0;
        work->state++;
        break;
    case WLOGO_BKS_STATE_NAME:
        if (work->paletteStep <= 6) {
            if (++work->timer >= gWlogoBksPaletteDurations[work->paletteStep]) {
                work->timer = 0;
                LoadPalette(&gWlogoBksPalettes[gWlogoBksPaletteIndices[work->paletteStep] * 16], (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), 0x20);
                work->paletteStep++;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            StopBgWave(0);
            work->timer = 0;
            work->visible = FALSE;
            LoadPalette(gWlogoBksPalettes, (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), 0x20);
            LoadBgMap(0, gWlogoBksMap, sizeof(gWlogoBksMap));
            work->state++;
        } else {
            work->gfx = AnimUpdate(&work->anim);
        }

        break;
    case WLOGO_BKS_STATE_HOLD:
        work->timer++;

        if (work->timer > 113) {
            work->timer = 0;
            work->state++;
        }

        break;
    case WLOGO_BKS_STATE_FADE_OUT:
        work->timer++;

        if (work->timer > 4) {
            work->timer = 0;
            work->blend--;

            if (work->blend == 0) {
                work->blend = 0;
                SetBgBlend(0, 16, 0);
                DisableBg(0);
                return 0;
            }

            SetBgBlend(0, 16 - work->blend, work->blend);
        }

        break;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_wlogo_bks_2(WlogoBksWork* work) {
    if (work->visible == TRUE) {
        DrawSprite(64, 64, work->gfx, work->tiles, work->palette, NULL, 0, 0);
    }

    TaskPoolDraw(&work->tasks);
}

void task_wlogo_bks_3(WlogoBksWork* work) {
    StopBgWave(0);
    TaskPoolDestroy(&work->tasks);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void WlogoBksHBlankIntr() {
    gIntrCheck |= INTR_FLAG_HBLANK;
    HBlankIntrBgWave1(0);
}

enum WlogoBksObjState {
    WLOGO_BKS_OBJ_STATE_MOVE,
    WLOGO_BKS_OBJ_STATE_HOLD
};

void task_wlogo_bks_obj_0(WlogoBksObjWork* work, s32 id) {
    work->id = id;
    work->x = gWlogoBksObjStarts[work->id][0] << 8;
    work->y = gWlogoBksObjStarts[work->id][1] << 8;
    work->targetX = gWlogoBksObjTargets[work->id][0] << 8;
    work->targetY = gWlogoBksObjTargets[work->id][1] << 8;
    work->scaleX = Q_8_8(1);
    work->scaleY = Q_8_8(1);
    work->holdTimer = 0;
    work->moveTimer = 30;
    work->state = WLOGO_BKS_OBJ_STATE_MOVE;
    work->scaleIndex = 10;
    work->tiles = LoadObjTiles(gWlogoBksNameTiles, 0x800);
    work->palette = LoadObjPalette(gWlogoBksPalette, sizeof(gWlogoBksPalette));
    work->gfx = gWlogoBksNameFrames[gWlogoBksObjFrames[work->id]];
    work->priority = gWlogoBksObjPriorities[work->id];
    work->unk_044 = 0;
}

u8 task_wlogo_bks_obj_1(WlogoBksObjWork* work) {
    switch (work->state) {
    case WLOGO_BKS_OBJ_STATE_MOVE:
        if (work->moveTimer > 0) {
            ApproachValue(&work->x, work->targetX, work->moveTimer);
            ApproachValue(&work->y, work->targetY, work->moveTimer);
            work->moveTimer--;

            if (++work->scaleIndex > 19) {
                work->scaleIndex = 0;
            }

            work->scaleX = gWlogoFlipScales[work->scaleIndex];
        } else {
            work->holdTimer = 0;
            work->moveTimer = 0;
            work->state++;
        }

        break;
    case WLOGO_BKS_OBJ_STATE_HOLD:
        if (work->holdTimer >= gWlogoBksObjHoldTimes[work->id]) {
            return 0;
        }

        work->holdTimer++;
        break;
    }

    return 1;
}

void task_wlogo_bks_obj_2(WlogoBksObjWork* work) {
    ObjAffine* affine;

    affine = AllocObjAffine(0, work->scaleX, work->scaleY, TRUE);
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, affine, 0, work->priority);
}

void task_wlogo_bks_obj_3(WlogoBksObjWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescWlogoMons = {
    "task_wlogo_mons",
    (TaskInitFunc)task_wlogo_mons_0,
    (TaskUpdateFunc)task_wlogo_mons_1,
    (TaskDrawFunc)task_wlogo_mons_2,
    (TaskDestroyFunc)task_wlogo_mons_3,
    sizeof(WlogoMonsWork),
};

TaskDesc gTaskDescWlogoHwt = {
    "task_wlogo_hwt",
    (TaskInitFunc)task_wlogo_hwt_0,
    (TaskUpdateFunc)task_wlogo_hwt_1,
    (TaskDrawFunc)task_wlogo_hwt_2,
    (TaskDestroyFunc)task_wlogo_hwt_3,
    sizeof(WlogoHwtWork),
};

const WlogoHwtObjA gWlogoHwtObjStarts[6] = {
    { 17920, 46080, 384, 1 },
    { 43520, 46080, 384, 0 },
    { 33280, 46080, 384, 1 },
    { 30720, 46080, 384, 0 },
    { 35840, 19200, 384, 1 },
    { 25600, 46080, 384, 0 },
};

const WlogoHwtObjB gWlogoHwtObjSteps[6][6] = {
    {
        { 40, 102, -1536, -2, 51, FALSE },
        { 40, 51, 128, 2, 0, FALSE },
        { 30, 102, -768, -2, -25, TRUE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
    },
    {
        { 30, -102, -1536, 2, 76, FALSE },
        { 40, -51, 128, -2, 0, FALSE },
        { 40, -102, -768, 2, -25, TRUE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
    },
    {
        { 30, 102, -1536, 0, 51, FALSE },
        { 40, 25, 128, 2, 0, FALSE },
        { 40, 102, -768, 25, -51, TRUE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
    },
    {
        { 40, -102, -1484, 0, 38, FALSE },
        { 40, -25, 128, -2, 0, FALSE },
        { 30, -102, -768, -25, -51, TRUE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
    },
    {
        { 30, 51, -512, 0, 25, FALSE },
        { 40, 25, 76, 2, 0, FALSE },
        { 30, 51, -384, 5, -51, TRUE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
    },
    {
        { 120, 0, -384, 0, -25, TRUE },
        { 40, -25, 128, -2, 0, FALSE },
        { 40, -51, -358, -25, -51, TRUE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
        { 0, 0, 0, 0, 0, FALSE },
    },
};

TaskDesc gTaskDescWlogoHwtObj = {
    "task_wlogo_hwt_obj",
    (TaskInitFunc)task_wlogo_hwt_obj_0,
    (TaskUpdateFunc)task_wlogo_hwt_obj_1,
    (TaskDrawFunc)task_wlogo_hwt_obj_2,
    (TaskDestroyFunc)task_wlogo_hwt_obj_3,
    sizeof(WlogoHwtObjWork),
};

WlogoWonEntry gWlogoWonCardsAlt[10] = {
    {65280, 18432, 768, 10, 59, 35, 11},
    {64768, 18432, 819, 30, 50, 34, 15},
    {64768, 18432, 972, 46, 38, 33, 1},
    {65024, 18432, 1075, 72, 31, 32, 5},
    {64768, 18432, 1177, 90, 25, 31, 8},
    {64768, 18432, 1280, 100, 20, 30, 10},
    {65024, 22528, 1024, 60, 34, 28, 3},
    {64512, 22528, 1126, 82, 27, 27, 7},
    {65024, 22528, 1228, 96, 22, 26, 9},
    {64512, 22528, 1331, 104, 17, 25, 12},
};

WlogoWonEntry gWlogoWonCards[10] = {
    {-2560, 18432, -512, 85, 44, 35, 18},
    {-2816, 18432, -512, 70, 52, 34, 14},
    {-3072, 18432, -512, 55, 60, 33, 10},
    {-3328, 18432, -512, 40, 68, 32, 6},
    {-3584, 18432, -512, 25, 76, 31, 2},
    {-3840, 18432, -512, 10, 84, 30, 18},
    {65024, 22528, 512, 40, 68, 28, 6},
    {64768, 22528, 512, 55, 60, 27, 10},
    {64512, 22528, 512, 70, 52, 26, 14},
    {64256, 22528, 512, 85, 44, 25, 18},
};

s32 gWlogoFlipScales[20] = {
    Q_8_8(1),
    Q_8_8(0.92),
    Q_8_8(0.8),
    Q_8_8(0.6),
    Q_8_8(0.43),
    Q_8_8(0.1),
    Q_8_8(-0.43),
    Q_8_8(-0.6),
    Q_8_8(-0.8),
    Q_8_8(-0.92),
    Q_8_8(-1),
    Q_8_8(-0.92),
    Q_8_8(-0.8),
    Q_8_8(-0.6),
    Q_8_8(-0.43),
    Q_8_8(0.1),
    Q_8_8(0.43),
    Q_8_8(0.6),
    Q_8_8(0.8),
    Q_8_8(0.92),
};

TaskDesc gTaskDescWlogoWon = {
    "task_wlogo_won",
    (TaskInitFunc)task_wlogo_won_0,
    (TaskUpdateFunc)task_wlogo_won_1,
    (TaskDrawFunc)task_wlogo_won_2,
    (TaskDestroyFunc)task_wlogo_won_3,
    sizeof(WlogoWonWork),
};

TaskDesc gTaskDescWlogoAtl = {
    "task_wlogo_atl",
    (TaskInitFunc)task_wlogo_atl_0,
    (TaskUpdateFunc)task_wlogo_atl_1,
    (TaskDrawFunc)task_wlogo_atl_2,
    (TaskDestroyFunc)task_wlogo_atl_3,
    sizeof(WlogoAtlWork),
};

TaskDesc gTaskDescWlogoNvl = {
    "task_wlogo_nvl",
    (TaskInitFunc)task_wlogo_nvl_0,
    (TaskUpdateFunc)task_wlogo_nvl_1,
    (TaskDrawFunc)task_wlogo_nvl_2,
    (TaskDestroyFunc)task_wlogo_nvl_3,
    sizeof(WlogoNvlWork),
};

const WlogoHwtObjB gWlogoNvlMovSteps[4] = {
    { 20, 768, 256, -5, -12, FALSE },
    { 20, 921, -51, -46, -25, FALSE },
    { 20, 0, -512, -46, 23, FALSE },
    { 20, -921, -115, 19, -5, TRUE },
};

TaskDesc gTaskDescWlogoNvlMov = {
    "task_wlogo_nvl_mov",
    (TaskInitFunc)task_wlogo_nvl_mov_0,
    (TaskUpdateFunc)task_wlogo_nvl_mov_1,
    (TaskDrawFunc)task_wlogo_nvl_mov_2,
    (TaskDestroyFunc)task_wlogo_nvl_mov_3,
    sizeof(WlogoNvlMovWork),
};

TaskDesc gTaskDescWlogoNvlObj = {
    "task_wlogo_nvl_obj",
    (TaskInitFunc)task_wlogo_nvl_obj_0,
    (TaskUpdateFunc)task_wlogo_nvl_obj_1,
    (TaskDrawFunc)task_wlogo_nvl_obj_2,
    (TaskDestroyFunc)task_wlogo_nvl_obj_3,
    sizeof(WlogoNvlObjWork),
};

TaskDesc gTaskDescWlogoCol = {
    "task_wlogo_col",
    (TaskInitFunc)task_wlogo_col_0,
    (TaskUpdateFunc)task_wlogo_col_1,
    (TaskDrawFunc)task_wlogo_col_2,
    (TaskDestroyFunc)task_wlogo_col_3,
    sizeof(WlogoColWork),
};

TaskDesc gTaskDescWlogoHlw = {
    "task_wlogo_hlw",
    (TaskInitFunc)task_wlogo_hlw_0,
    (TaskUpdateFunc)task_wlogo_hlw_1,
    (TaskDrawFunc)task_wlogo_hlw_2,
    (TaskDestroyFunc)task_wlogo_hlw_3,
    sizeof(WlogoHlwWork),
};

TaskDesc gTaskDescWlogoDil = {
    "task_wlogo_dil",
    (TaskInitFunc)task_wlogo_dil_0,
    (TaskUpdateFunc)task_wlogo_dil_1,
    (TaskDrawFunc)task_wlogo_dil_2,
    (TaskDestroyFunc)task_wlogo_dil_3,
    sizeof(WlogoDilWork),
};

WlogoAgrEntry gWlogoAgrEntries[20] = {
    {99, 83, 10, 5, 0, 91, 85, 6, {0, 0, 0}},
    {88, 80, 20, 5, 0, 92, 85, 7, {0, 0, 0}},
    {84, 73, 26, 5, 0, 85, 73, 6, {0, 0, 0}},
    {101, 67, 32, 0, 0, 104, 67, 7, {0, 0, 0}},
    {118, 71, 36, 0, 0, 112, 66, 6, {0, 0, 0}},
    {128, 72, 40, 0, 0, 120, 68, 7, {0, 0, 0}},
    {142, 71, 46, 0, 0, 139, 72, 6, {0, 0, 0}},
    {149, 61, 50, 0, 0, 156, 71, 7, {0, 0, 0}},
    {166, 60, 54, 0, 0, 169, 57, 6, {0, 0, 0}},
    {141, 51, 60, 0, 1, 141, 62, 7, {0, 0, 0}},
    {125, 52, 66, 1, 1, 125, 54, 6, {0, 0, 0}},
    {114, 50, 72, 1, 1, 118, 42, 7, {0, 0, 0}},
    {100, 51, 78, 1, 1, 97, 53, 6, {0, 0, 0}},
    {88, 55, 82, 1, 1, 84, 65, 7, {0, 0, 0}},
    {78, 61, 88, 2, 1, 78, 56, 6, {0, 0, 0}},
    {98, 66, 92, 2, 1, 100, 66, 7, {0, 0, 0}},
    {117, 64, 98, 0, 1, 117, 69, 6, {0, 0, 0}},
    {122, 66, 104, 0, 1, 126, 66, 7, {0, 0, 0}},
    {135, 64, 110, 0, 1, 138, 59, 6, {0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0}},
};

TaskDesc gTaskDescWlogoAgr = {
    "task_wlogo_agr",
    (TaskInitFunc)task_wlogo_agr_0,
    (TaskUpdateFunc)task_wlogo_agr_1,
    (TaskDrawFunc)task_wlogo_agr_2,
    (TaskDestroyFunc)task_wlogo_agr_3,
    sizeof(WlogoAgrWork),
};

TaskDesc gTaskDescWlogoAgrSmoke = {
    "task_wlogo_agr_smoke",
    (TaskInitFunc)task_wlogo_agr_smoke_0,
    (TaskUpdateFunc)task_wlogo_agr_smoke_1,
    (TaskDrawFunc)task_wlogo_agr_smoke_2,
    (TaskDestroyFunc)task_wlogo_agr_smoke_3,
    sizeof(WlogoAgrSmokeWork),
};

TaskDesc gTaskDescWlogoAgrFlash0 = {
    "task_wlogo_agr_flash0",
    (TaskInitFunc)task_wlogo_agr_flash0_0,
    (TaskUpdateFunc)task_wlogo_agr_flash0_1,
    (TaskDrawFunc)task_wlogo_agr_flash0_2,
    (TaskDestroyFunc)task_wlogo_agr_flash0_3,
    sizeof(WlogoAgrFlashWork),
};

TaskDesc gTaskDescWlogoAgrFlash1 = {
    "task_wlogo_agr_flash1",
    (TaskInitFunc)task_wlogo_agr_flash1_0,
    (TaskUpdateFunc)task_wlogo_agr_flash1_1,
    (TaskDrawFunc)task_wlogo_agr_flash1_2,
    (TaskDestroyFunc)task_wlogo_agr_flash1_3,
    sizeof(WlogoAgrFlashWork),
};

TaskDesc gTaskDescWlogoTvt = {
    "task_wlogo_tvt",
    (TaskInitFunc)task_wlogo_tvt_0,
    (TaskUpdateFunc)task_wlogo_tvt_1,
    (TaskDrawFunc)task_wlogo_tvt_2,
    (TaskDestroyFunc)task_wlogo_tvt_3,
    sizeof(WlogoTvtWork),
};

TaskDesc gTaskDescWlogoPoo = {
    "task_wlogo_poo",
    (TaskInitFunc)task_wlogo_poo_0,
    (TaskUpdateFunc)task_wlogo_poo_1,
    (TaskDrawFunc)task_wlogo_poo_2,
    (TaskDestroyFunc)task_wlogo_poo_3,
    sizeof(WlogoPooWork),
};

const WlogoPooObjStep gWlogoPooObjSteps[5][5] = {
    {
        { 20, -128, 0, 0, -2 },
        { 20, -128, -51, 0, -2 },
        { 20, -128, -102, 0, -2 },
        { 20, -128, -153, 0, -2 },
        { 10, -128, -204, 0, 0 },
    },
    {
        { 20, -128, 0, 0, 5 },
        { 20, -128, 0, 0, -5 },
        { 20, -128, 0, 0, 5 },
        { 20, -128, 0, 0, -7 },
        { 10, -128, -25, 0, 0 },
    },
    {
        { 20, 128, 0, 0, -5 },
        { 20, 128, 0, 0, 7 },
        { 20, 128, 0, 0, -5 },
        { 20, 128, 0, 0, 2 },
        { 10, 128, -51, 0, 0 },
    },
    {
        { 20, 128, 0, 0, 5 },
        { 20, 128, 51, 0, 2 },
        { 20, 128, 76, 0, -5 },
        { 20, 128, 25, 0, -2 },
        { 10, 128, 25, 0, 0 },
    },
    {
        { 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0 },
    },
};

u8 gWlogoPooObjAnimIds[4] = { 0, 0, 2, 2 };

TaskDesc gTaskDescWlogoPooObj = {
    "task_wlogo_poo_obj",
    (TaskInitFunc)task_wlogo_poo_obj_0,
    (TaskUpdateFunc)task_wlogo_poo_obj_1,
    (TaskDrawFunc)task_wlogo_poo_obj_2,
    (TaskDestroyFunc)task_wlogo_poo_obj_3,
    sizeof(WlogoPooObjWork),
};

WlogoTtMotion gWlogoTtMotion =
{{Q_8_8(0.5), Q_8_8(0.1), Q_8_8(0.5), Q_8_8(0.8), Q_8_8(1), 0}, {Q_8_8(0.3), Q_8_8(0.7), Q_8_8(1), 0, 0, 0}}
;

TaskDesc gTaskDescWlogoTt = {
    "task_wlogo_tt",
    (TaskInitFunc)task_wlogo_tt_0,
    (TaskUpdateFunc)task_wlogo_tt_1,
    (TaskDrawFunc)task_wlogo_tt_2,
    (TaskDestroyFunc)task_wlogo_tt_3,
    sizeof(WlogoTtWork),
};

TaskDesc gTaskDescWlogoTtObj = {
    "task_wlogo_tt_obj",
    (TaskInitFunc)task_wlogo_tt_obj_0,
    (TaskUpdateFunc)task_wlogo_tt_obj_1,
    (TaskDrawFunc)task_wlogo_tt_obj_2,
    (TaskDestroyFunc)task_wlogo_tt_obj_3,
    sizeof(WlogoTtObjWork),
};

s16 gWlogoTtLinePoints[33][3] = {
    {72, 76, 5},
    {111, 90, 5},
    {85, 82, 5},
    {120, 95, 5},
    {102, 72, 5},
    {135, 87, 5},
    {120, 79, 5},
    {140, 99, 5},
    {80, 73, 5},
    {116, 92, 5},
    {92, 78, 5},
    {105, 97, 5},
    {98, 84, 5},
    {130, 88, 5},
    {122, 76, 5},
    {115, 100, 5},
    {102, 72, 5},
    {119, 99, 5},
    {122, 79, 5},
    {130, 89, 5},
    {105, 74, 5},
    {135, 94, 5},
    {135, 83, 5},
    {125, 86, 5},
    {130, 80, 5},
    {120, 90, 5},
    {140, 75, 5},
    {155, 94, 5},
    {118, 72, 5},
    {135, 96, 5},
    {140, 78, 5},
    {142, 98, 5},
    {0, 0, 0},
};

TaskDesc gTaskDescWlogoTtLine = {
    "task_wlogo_tt_line",
    (TaskInitFunc)task_wlogo_tt_line_0,
    (TaskUpdateFunc)task_wlogo_tt_line_1,
    (TaskDrawFunc)task_wlogo_tt_line_2,
    (TaskDestroyFunc)task_wlogo_tt_line_3,
    sizeof(WlogoTtLineWork),
};

s8 gWlogoBksPaletteDurations[8] = {
    6,
    6,
    6,
    10,
    7,
    7,
    7,
};

s8 gWlogoBksPaletteIndices[8] = {
    1,
    2,
    3,
    4,
    3,
    2,
    1,
};

TaskDesc gTaskDescWlogoBks = {
    "task_wlogo_bks",
    (TaskInitFunc)task_wlogo_bks_0,
    (TaskUpdateFunc)task_wlogo_bks_1,
    (TaskDrawFunc)task_wlogo_bks_2,
    (TaskDestroyFunc)task_wlogo_bks_3,
    sizeof(WlogoBksWork),
};

u8 gWlogoBksObjFrames[14] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    4,
    8,
    9,
    8,
    10,
    11,
};

s16 gWlogoBksObjTargets[14][2] = {
    {90, 80},
    {99, 81},
    {107, 81},
    {114, 81},
    {121, 81},
    {127, 81},
    {98, 91},
    {113, 90},
    {121, 90},
    {125, 90},
    {131, 90},
    {136, 90},
    {143, 90},
    {150, 90},
};

s16 gWlogoBksObjStarts[14][2] = {
    {250, 190},
    {200, 190},
    {150, 190},
    {100, 190},
    {50, 190},
    {0, 190},
    {245, -30},
    {210, -30},
    {175, -30},
    {140, -30},
    {105, -30},
    {70, -30},
    {35, -30},
    {0, -30},
};

s16 gWlogoBksObjHoldTimes[14] = {
    80,
    70,
    60,
    50,
    40,
    30,
    20,
    30,
    40,
    50,
    60,
    70,
    80,
    90,
};

u16 gWlogoBksObjPriorities[14] = {
    11,
    9,
    7,
    5,
    3,
    1,
    2,
    4,
    6,
    8,
    10,
    12,
    13,
    14,
};

TaskDesc gTaskDescWlogoBksObj = {
    "task_wlogo_bks_obj",
    (TaskInitFunc)task_wlogo_bks_obj_0,
    (TaskUpdateFunc)task_wlogo_bks_obj_1,
    (TaskDrawFunc)task_wlogo_bks_obj_2,
    (TaskDestroyFunc)task_wlogo_bks_obj_3,
    sizeof(WlogoBksObjWork),
};
