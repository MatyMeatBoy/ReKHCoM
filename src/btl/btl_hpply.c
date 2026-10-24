/**
 * btl_hpply.c
 * Player HP Gauge
 */

#include "m4a_song.h"
#include "btl2.h"
#include "sprites_btl.h"
#include "songs.h"
#include "anim.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "game_state.h"
#include "obj.h"
#include "obj_api.h"
#include "taskpool.h"
#include "types.h"
#include "engine.h"
#ifdef PLATFORM_PC
#include "port/port.h"
#ifdef PLATFORM_PC
#include "port/port_hdsprites.h"
#include "port/port_hdlayout.h"
#endif
#endif
#include <stddef.h>
#include "sprite_palettes.h"
#include "engine_math.h"

enum BtlHpplyGaugeMode {
    BTL_HPPLY_GAUGE_MODE_SINGLE,
    BTL_HPPLY_GAUGE_MODE_EXTRA_BAR,
    BTL_HPPLY_GAUGE_MODE_MAIN_BAR
};

void task_btl_hpply_0(BtlHpplyWork* work) {
    if (gGameState.flags & GAME_FLAG_RIKU) {
        work->palette = LoadObjPalette(gRikuPalette, sizeof(gRikuPalette));
        work->tiles = AllocObjTiles(0x280, gBtlHpRikuFaceTiles);
        work->gfx = gBtlHpRikuFaceFrame0;
        AnimInit(&work->anim, gBtlHpRikuFaceAnims, gBtlHpRikuFaceFrames);
    } else {
        work->palette = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
        work->tiles = AllocObjTiles(0x280, gBtlHpSoraFaceTiles);
        work->gfx = gBtlHpSoraFaceFrame0;
        AnimInit(&work->anim, gBtlHpSoraFaceAnims, gBtlHpSoraFaceFrames);
    }

    work->palette2 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->tiles2 = AllocObjTiles(0x280, gBHpgagTiles);
    work->tiles3 = AllocObjTiles(0x120, gBHpgagTiles);
    work->tiles4 = AllocObjTiles(0x80, gBHpgagTiles);
    work->gfx2 = gBHpgagFrame1;
    AnimInit(&work->anim2, gBHpgagAnims, gBHpgagFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);

    if (gBtlWork->actor->maxHp <= 40) {
        work->gaugeSize = 0;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_SINGLE;
    } else if (gBtlWork->actor->maxHp <= 80) {
        work->gaugeSize = 1;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_SINGLE;
    } else if (gBtlWork->actor->maxHp <= 120) {
        work->gaugeSize = 2;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_SINGLE;
    } else if (gBtlWork->actor->maxHp <= 160) {
        work->gaugeSize = 3;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_SINGLE;
    } else if (gBtlWork->actor->maxHp <= 200) {
        work->gaugeSize = 4;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_SINGLE;
    } else if (gBtlWork->actor->maxHp <= 240) {
        work->gaugeSize = 5;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_SINGLE;
    } else if (gBtlWork->actor->maxHp <= 280) {
        work->gaugeSize = 6;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_SINGLE;
    } else if (gBtlWork->actor->maxHp <= 320) {
        work->gaugeSize = 0;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    } else if (gBtlWork->actor->maxHp <= 360) {
        work->gaugeSize = 1;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    } else if (gBtlWork->actor->maxHp <= 400) {
        work->gaugeSize = 2;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    } else if (gBtlWork->actor->maxHp <= 440) {
        work->gaugeSize = 3;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    } else if (gBtlWork->actor->maxHp <= 480) {
        work->gaugeSize = 4;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    } else if (gBtlWork->actor->maxHp <= 520) {
        work->gaugeSize = 5;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    } else if (gBtlWork->actor->maxHp <= 560) {
        work->gaugeSize = 6;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    } else {
        work->gaugeSize = 6;
        work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
    }

    if (work->gaugeMode == BTL_HPPLY_GAUGE_MODE_SINGLE) {
        switch (work->gaugeSize) {
        case 0:
        case 1:
            AnimStart(&work->anim2, 1, ANIM_FLAG_LOOP);
            break;
        case 2:
            AnimStart(&work->anim2, 3, ANIM_FLAG_LOOP);
            break;
        case 3:
            AnimStart(&work->anim2, 5, ANIM_FLAG_LOOP);
            break;
        case 4:
            AnimStart(&work->anim2, 7, ANIM_FLAG_LOOP);
            break;
        case 5:
            AnimStart(&work->anim2, 9, ANIM_FLAG_LOOP);
            break;
        case 6:
            AnimStart(&work->anim2, 11, ANIM_FLAG_LOOP);
            break;
        default:
            AnimStart(&work->anim2, 11, ANIM_FLAG_LOOP);
            break;
        }

        work->gfx3 = NULL;
    } else {
        AnimStart(&work->anim2, 11, ANIM_FLAG_LOOP);

        switch (work->gaugeSize) {
        case 0:
            work->gfx3 = gBHpgagFrame19;
            break;
        case 1:
            work->gfx3 = gBHpgagFrame20;
            break;
        case 2:
            work->gfx3 = gBHpgagFrame21;
            break;
        case 3:
            work->gfx3 = gBHpgagFrame22;
            break;
        case 4:
            work->gfx3 = gBHpgagFrame23;
            break;
        case 5:
            work->gfx3 = gBHpgagFrame24;
            break;
        case 6:
            work->gfx3 = gBHpgagFrame25;
            break;
        default:
            work->gfx3 = gBHpgagFrame25;
            break;
        }
    }

    work->hpRatio = Q_8_8(1);
    work->firstUpdate = TRUE;
    work->unk_5C = 1;
    work->timer = 0;
    work->prevHp = 0;
    work->displayHp = 0;
    work->alarmPlaying = FALSE;
}

s32 task_btl_hpply_1(BtlHpplyWork* work) {
    BtlObj* actor;
    s32 lowHp;

    actor = gBtlWork->actor;

    if (actor == NULL) {
        return 0;
    }

    if (gBtlWork->flags & BTL_FLAG_FIELD_HIDDEN) {
        return 0;
    }

    if (work->gaugeMode != BTL_HPPLY_GAUGE_MODE_EXTRA_BAR && work->hpRatio <= 63) {
        lowHp = TRUE;
    } else {
        lowHp = FALSE;
    }

    if (actor->hp < work->prevHp) {
        work->timer = 44;
    }

    if (work->timer != 0) {
        AnimChange(&work->anim, 1, ANIM_FLAG_LOOP);
        work->timer--;
    } else if (lowHp) {
        AnimChange(&work->anim, 2, ANIM_FLAG_LOOP);
    } else {
        AnimChange(&work->anim, 0, ANIM_FLAG_LOOP);
    }

    if (work->firstUpdate) {
        work->firstUpdate = FALSE;
        work->displayHp = actor->hp;
    } else if (work->displayHp < actor->hp) {
        work->displayHp += 3;

        if (work->displayHp > actor->hp) {
            work->displayHp = actor->hp;
        }
    } else if (work->displayHp > actor->hp) {
        work->displayHp -= 3;

        if (work->displayHp < actor->hp) {
            work->displayHp = actor->hp;
        }
    }

    if (work->gaugeMode == BTL_HPPLY_GAUGE_MODE_EXTRA_BAR) {
        if (work->displayHp <= 280) {
            work->gaugeMode = BTL_HPPLY_GAUGE_MODE_MAIN_BAR;
        }
    } else if (work->gaugeMode == BTL_HPPLY_GAUGE_MODE_MAIN_BAR) {
        if (work->displayHp > 280) {
            work->gaugeMode = BTL_HPPLY_GAUGE_MODE_EXTRA_BAR;
        }
    }

    switch (work->gaugeMode) {
    case BTL_HPPLY_GAUGE_MODE_SINGLE:
#ifdef PLATFORM_PC
        /* @bug: leaving the deck menu back into a battle, a frame can run before the maximum HP is set; the
           cartridge's division by zero returns garbage, platforms without fault recovery close (issue #40) */
        if (actor->maxHp <= 0) {
            work->hpRatio = 0;
            break;
        }
#endif
        work->hpRatio = (work->displayHp << 8) / actor->maxHp;
        break;
    case BTL_HPPLY_GAUGE_MODE_EXTRA_BAR:
#ifdef PLATFORM_PC
        if (actor->maxHp <= 280) {
            work->hpRatio = 0;
            break;
        }
#endif
        work->hpRatio = ((work->displayHp - 280) << 8) / (actor->maxHp - 280);
        break;
    case BTL_HPPLY_GAUGE_MODE_MAIN_BAR:
        work->hpRatio = (work->displayHp << 8) / 280;
        break;
    }

    if (lowHp) {
        if (!work->alarmPlaying) {
            work->alarmPlaying = TRUE;
            m4aSongNumStart(SONG_SYS_ALART);
        }

        if (work->gaugeMode == BTL_HPPLY_GAUGE_MODE_SINGLE) {
            switch (work->gaugeSize) {
            case 0:
            case 1:
                AnimChange(&work->anim2, 2, ANIM_FLAG_LOOP);
                break;
            case 2:
                AnimChange(&work->anim2, 4, ANIM_FLAG_LOOP);
                break;
            case 3:
                AnimChange(&work->anim2, 6, ANIM_FLAG_LOOP);
                break;
            case 4:
                AnimChange(&work->anim2, 8, ANIM_FLAG_LOOP);
                break;
            case 5:
                AnimChange(&work->anim2, 10, ANIM_FLAG_LOOP);
                break;
            case 6:
                AnimChange(&work->anim2, 12, ANIM_FLAG_LOOP);
                break;
            default:
                AnimChange(&work->anim2, 12, ANIM_FLAG_LOOP);
                break;
            }
        } else {
            AnimChange(&work->anim2, 12, ANIM_FLAG_LOOP);
        }
    } else {
        if (work->gaugeMode == BTL_HPPLY_GAUGE_MODE_SINGLE) {
            switch (work->gaugeSize) {
            case 0:
            case 1:
                AnimChange(&work->anim2, 1, ANIM_FLAG_LOOP);
                break;
            case 2:
                AnimChange(&work->anim2, 3, ANIM_FLAG_LOOP);
                break;
            case 3:
                AnimChange(&work->anim2, 5, ANIM_FLAG_LOOP);
                break;
            case 4:
                AnimChange(&work->anim2, 7, ANIM_FLAG_LOOP);
                break;
            case 5:
                AnimChange(&work->anim2, 9, ANIM_FLAG_LOOP);
                break;
            case 6:
                AnimChange(&work->anim2, 11, ANIM_FLAG_LOOP);
                break;
            default:
                AnimChange(&work->anim2, 11, ANIM_FLAG_LOOP);
                break;
            }
        } else {
            AnimChange(&work->anim2, 11, ANIM_FLAG_LOOP);
        }

        if (work->alarmPlaying) {
            work->alarmPlaying = FALSE;
            m4aSongNumStop(SONG_SYS_ALART);
        }
    }

    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
    work->prevHp = actor->hp;
    return 1;
}

#ifdef PLATFORM_PC
static void task_btl_hpply_2_Body(BtlHpplyWork* work);

/* Player HP gauge: top-left corner of the screen. */
void task_btl_hpply_2(BtlHpplyWork* work) {
    PortHudBegin(-1, -1);
    task_btl_hpply_2_Body(work);
    PortHudEnd();
}

static void task_btl_hpply_2_Body(BtlHpplyWork* work) {
#else
void task_btl_hpply_2(BtlHpplyWork* work) {
#endif
    s32 scale;
    ObjAffine* affine;

#ifdef PLATFORM_PC
    /* HD cards: Re:CoM's HP ring around the HD face takes the place of the gauge (port_hdsprites.c); the face
       moves in so the ring and its bar fit in the corner */
    {
        BtlObj* actor = gBtlWork->actor;
        int ratio = actor != NULL && actor->maxHp > 0 ? work->displayHp * 256 / actor->maxHp : 0;
        /* Re:CoM 1.5's own gauge (port_hdlayout.c, its logic read from Re:CoM's executable): face, ring and bar
           from gauge.lad, so nothing of the cartridge's gauge is drawn */
        if (actor != NULL && PortHdGauge(actor->hp, actor->maxHp, (gGameState.flags & GAME_FLAG_RIKU) != 0))
            return;
        if (PortHdHpRing(((ObjTiles*)work->tiles)->src, work->gfx, ratio)) {
            DrawSprite(14, 15, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 1);
            return;
        }
    }
#endif
    DrawSprite(4, 2, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 1);

#ifdef PLATFORM_PC
    gPortHdHudPart = PortHdHudPlayerFrameReady() ? 5 : 0;
#endif

    switch (work->gaugeMode) {
    case BTL_HPPLY_GAUGE_MODE_SINGLE:
        DrawSprite(4, 2, work->gfx2, work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 4);
        break;
    case BTL_HPPLY_GAUGE_MODE_EXTRA_BAR:
        DrawSprite(4, 2, gBHpgagFrame27, work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 4);
        DrawSprite(4, 2, work->gfx3, work->tiles3, work->palette2, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 3);
        break;
    case BTL_HPPLY_GAUGE_MODE_MAIN_BAR:
        DrawSprite(4, 2, work->gfx2, work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 4);
        DrawSprite(4, 2, work->gfx3, work->tiles3, work->palette2, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 5);
        break;
    }

#ifdef PLATFORM_PC
    gPortHdHudPart = 0;
#endif
    switch (work->gaugeMode) {
    case BTL_HPPLY_GAUGE_MODE_MAIN_BAR:
        scale = work->hpRatio;
        break;
    case BTL_HPPLY_GAUGE_MODE_SINGLE:
        switch (work->gaugeSize) {
        case 0:
        case 1:
            scale = (work->hpRatio * 72) >> 8;
            break;
        case 2:
            scale = (work->hpRatio * 109) >> 8;
            break;
        case 3:
            scale = (work->hpRatio * 146) >> 8;
            break;
        case 4:
            scale = (work->hpRatio * 182) >> 8;
            break;
        case 5:
            scale = (work->hpRatio * 219) >> 8;
            break;
        case 6:
            scale = work->hpRatio;
            break;
        default:
            scale = work->hpRatio;
            break;
        }

        break;
    case BTL_HPPLY_GAUGE_MODE_EXTRA_BAR:
    default:
        switch (work->gaugeSize) {
        case 0:
            scale = (work->hpRatio * 36) >> 8;
            break;
        case 1:
            scale = (work->hpRatio * 72) >> 8;
            break;
        case 2:
            scale = (work->hpRatio * 109) >> 8;
            break;
        case 3:
            scale = (work->hpRatio * 146) >> 8;
            break;
        case 4:
            scale = (work->hpRatio * 182) >> 8;
            break;
        case 5:
            scale = (work->hpRatio * 219) >> 8;
            break;
        case 6:
            scale = work->hpRatio;
            break;
        default:
            scale = work->hpRatio;
            break;
        }

        break;
    }

    scale *= 2;

    if (work->displayHp > 0) {
#ifdef PLATFORM_PC
        gPortHdHudPart = PortHdHudEnemy(1, 0, 0) ?
            (work->displayHp >= gBtlWork->actor->maxHp ? 6 : 4) : 0;
#endif
        if (scale <= 9) {
            scale = 10;
        }

        if (scale > Q_8_8(1)) {
            affine = AllocObjAffine(0, scale, Q_8_8(1), TRUE);
        } else {
            affine = AllocObjAffine(0, scale, Q_8_8(1), FALSE);
        }

        if (work->gaugeMode == BTL_HPPLY_GAUGE_MODE_EXTRA_BAR) {
            DrawSprite(31, 9, gBHpgagFrame26, work->tiles4, work->palette2, affine, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 2);
        } else {
            DrawSprite(31, 6, gBHpgagFrame0, work->tiles4, work->palette2, affine, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 2);
        }
#ifdef PLATFORM_PC
        gPortHdHudPart = 0;
#endif
    }
}

void task_btl_hpply_3(BtlHpplyWork* work) {
    m4aSongNumStop(SONG_SYS_ALART);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescBtlHpply = {
    "task_btl_hpply",
    (TaskInitFunc)task_btl_hpply_0,
    (TaskUpdateFunc)task_btl_hpply_1,
    (TaskDrawFunc)task_btl_hpply_2,
    (TaskDestroyFunc)task_btl_hpply_3,
    sizeof(BtlHpplyWork),
};
