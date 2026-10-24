/**
 * btl_hpenm.c
 * Enemy HP Gauge
 */

#include "system_state.h"
#include "btl2.h"
#include "sprites_btl_hud.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "obj.h"
#include "obj_api.h"
#include "taskpool.h"
#include "types.h"
#include "engine.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "sprites_mode_test.h"
#include "engine_math.h"
#ifdef PLATFORM_PC
#include "port/port.h"
#endif

enum BtlHpenmGaugeLayer {
    BTL_HPENM_GAUGE_LAYER_GREEN,
    BTL_HPENM_GAUGE_LAYER_BLUE,
    BTL_HPENM_GAUGE_LAYER_YELLOW,
    BTL_HPENM_GAUGE_LAYER_RED
};

void task_btl_hpenm_0(BtlHpenmWork* work) {
    work->tiles = AllocObjTiles(0x140, gBHpgagETiles);
    work->tiles2 = AllocObjTiles(0x80, gBHpgagETiles);
    work->tiles3 = AllocObjTiles(0x20, gBHpgagETiles);
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->visible = FALSE;
    work->hpRatio = Q_8_8(1);
    work->actor = NULL;
    work->gaugeSize = 0;
    work->gaugeLayer = BTL_HPENM_GAUGE_LAYER_GREEN;
}

s32 task_btl_hpenm_1(BtlHpenmWork* work) {
    BtlObj* actor;

    if (gBtlWork->phase == BTL_PHASE_END) {
        return 0;
    }

    if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
        actor = gRikuBtlWork->actor;
        work->visible = TRUE;
    } else {
        if (gBtlWork->actor2 == NULL) {
            if (work->visible) {
                work->visible = FALSE;
            }

            return 1;
        }

        work->visible = TRUE;
        actor = gBtlWork->actor2;
    }

    if (actor->parent != NULL) {
        actor = actor->parent;
    }

    if (work->actor != actor) {
        work->actor = actor;
        work->displayHp = actor->hp;

        if (actor->maxHp <= 80) {
            work->gaugeSize = 0;
        } else if (actor->maxHp <= 160) {
            work->gaugeSize = 1;
        } else if (actor->maxHp <= 240) {
            work->gaugeSize = 2;
        } else if (actor->maxHp <= 320) {
            work->gaugeSize = 3;
        } else if (actor->maxHp <= 400) {
            work->gaugeSize = 4;
        } else if (actor->maxHp <= 480) {
            work->gaugeSize = 5;
        } else if (actor->maxHp <= 560) {
            work->gaugeSize = 6;
        } else {
            work->gaugeSize = 7;
        }
    } else if (work->displayHp < actor->hp) {
        work->displayHp += 5;

        if (work->displayHp > actor->hp) {
            work->displayHp = actor->hp;
        }
    } else if (work->displayHp > actor->hp) {
        work->displayHp -= 5;

        if (work->displayHp < actor->hp) {
            work->displayHp = actor->hp;
        }
    }

    if (work->displayHp <= 560) {
        work->gaugeLayer = BTL_HPENM_GAUGE_LAYER_GREEN;
    } else if (work->displayHp <= 1120) {
        work->gaugeLayer = BTL_HPENM_GAUGE_LAYER_BLUE;
    } else if (work->displayHp <= 1680) {
        work->gaugeLayer = BTL_HPENM_GAUGE_LAYER_YELLOW;
    } else {
        work->gaugeLayer = BTL_HPENM_GAUGE_LAYER_RED;
    }

    switch (work->gaugeLayer) {
    case BTL_HPENM_GAUGE_LAYER_RED:
        work->hpRatio = ((work->displayHp - 1680) << 8) / 560;
        break;
    case BTL_HPENM_GAUGE_LAYER_YELLOW:
        work->hpRatio = ((work->displayHp - 1120) << 8) / 560;
        break;
    case BTL_HPENM_GAUGE_LAYER_BLUE:
        work->hpRatio = ((work->displayHp - 560) << 8) / 560;
        break;
    case BTL_HPENM_GAUGE_LAYER_GREEN:
        if (work->gaugeSize <= 6) {
#ifdef PLATFORM_PC
            /* @bug: a frame before the maximum HP is set divides by zero (the gauges' #40 class) */
            work->hpRatio = actor->maxHp > 0 ? (work->displayHp << 8) / actor->maxHp : 0;
#else
            work->hpRatio = (work->displayHp << 8) / actor->maxHp;
#endif
        } else {
            work->hpRatio = (work->displayHp << 8) / 560;
        }

        break;
    }

    return 1;
}

#ifdef PLATFORM_PC
static void task_btl_hpenm_2_Body(BtlHpenmWork* work);

/* Enemy HP gauge: top-right corner of the screen. */
void task_btl_hpenm_2(BtlHpenmWork* work) {
    extern int PortHdEnemyGauge(int visible, int hp, int maxHp, const void* who);
    /* Re:CoM 1.5's enemy gauge (port_hdlayout.c, its logic read from Re:CoM's executable) in the cartridge's place */
    if (PortHdEnemyGauge(work->visible, work->actor ? work->actor->hp : 0, work->actor ? work->actor->maxHp : 0,
                         work->actor))
        return;
    PortHudBegin(1, -1);
    gPortHdHudPart = PortHdHudEnemy(work->visible, work->hpRatio, work->gaugeLayer) ? 1 : 0;
    task_btl_hpenm_2_Body(work);
    gPortHdHudPart = 0;
    PortHudEnd();
}

static void task_btl_hpenm_2_Body(BtlHpenmWork* work) {
#else
void task_btl_hpenm_2(BtlHpenmWork* work) {
#endif
    void* gfx;
    void* bar;
    s32 scale;
    ObjAffine* affine;

    if (!work->visible) {
        return;
    }

    switch (work->gaugeLayer) {
    case BTL_HPENM_GAUGE_LAYER_RED:
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
            gfx = gBHpgagEFrame12Eu;
            break;
        case LANGUAGE_SPANISH:
            gfx = gBHpgagEFrame22Eu;
            break;
        case LANGUAGE_ITALIAN:
            gfx = gBHpgagEFrame31Eu;
            break;
        case LANGUAGE_GERMAN:
        default:
            gfx = gBHpgagEFrame40Eu;
            break;
        }

        bar = gBHpgagEFrame9Eu;
#else
        gfx = gBHpgagEFrame12;
        bar = gBHpgagEFrame9;
#endif
        break;
    case BTL_HPENM_GAUGE_LAYER_YELLOW:
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
            gfx = gBHpgagEFrame11Eu;
            break;
        case LANGUAGE_SPANISH:
            gfx = gBHpgagEFrame21Eu;
            break;
        case LANGUAGE_ITALIAN:
            gfx = gBHpgagEFrame30Eu;
            break;
        case LANGUAGE_GERMAN:
        default:
            gfx = gBHpgagEFrame39Eu;
            break;
        }

        bar = gBHpgagEFrame8Eu;
#else
        gfx = gBHpgagEFrame11;
        bar = gBHpgagEFrame8;
#endif
        break;
    case BTL_HPENM_GAUGE_LAYER_BLUE:
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
            gfx = gBHpgagEFrame10Eu;
            break;
        case LANGUAGE_SPANISH:
            gfx = gBHpgagEFrame20Eu;
            break;
        case LANGUAGE_ITALIAN:
            gfx = gBHpgagEFrame29Eu;
            break;
        case LANGUAGE_GERMAN:
        default:
            gfx = gBHpgagEFrame38Eu;
            break;
        }

        bar = gBHpgagEFrame7Eu;
#else
        gfx = gBHpgagEFrame10;
        bar = gBHpgagEFrame7;
#endif
        break;
    case BTL_HPENM_GAUGE_LAYER_GREEN:
    default:
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
            switch (work->gaugeSize) {
            case 0:
            case 1:
                gfx = gBHpgagEFrame1Eu;
                break;
            case 2:
                gfx = gBHpgagEFrame2Eu;
                break;
            case 3:
                gfx = gBHpgagEFrame3Eu;
                break;
            case 4:
                gfx = gBHpgagEFrame4Eu;
                break;
            case 5:
                gfx = gBHpgagEFrame5Eu;
                break;
            case 6:
            default:
                gfx = gBHpgagEFrame6Eu;
                break;
            }

            break;
        case LANGUAGE_SPANISH:
            switch (work->gaugeSize) {
            case 0:
            case 1:
                gfx = gBHpgagEFrame14Eu;
                break;
            case 2:
                gfx = gBHpgagEFrame15Eu;
                break;
            case 3:
                gfx = gBHpgagEFrame16Eu;
                break;
            case 4:
                gfx = gBHpgagEFrame17Eu;
                break;
            case 5:
                gfx = gBHpgagEFrame18Eu;
                break;
            case 6:
            default:
                gfx = gBHpgagEFrame19Eu;
                break;
            }

            break;
        case LANGUAGE_ITALIAN:
            switch (work->gaugeSize) {
            case 0:
            case 1:
                gfx = gBHpgagEFrame23Eu;
                break;
            case 2:
                gfx = gBHpgagEFrame24Eu;
                break;
            case 3:
                gfx = gBHpgagEFrame25Eu;
                break;
            case 4:
                gfx = gBHpgagEFrame26Eu;
                break;
            case 5:
                gfx = gBHpgagEFrame27Eu;
                break;
            case 6:
            default:
                gfx = gBHpgagEFrame28Eu;
                break;
            }

            break;
        case LANGUAGE_GERMAN:
        default:
            switch (work->gaugeSize) {
            case 0:
            case 1:
                gfx = gBHpgagEFrame32Eu;
                break;
            case 2:
                gfx = gBHpgagEFrame33Eu;
                break;
            case 3:
                gfx = gBHpgagEFrame34Eu;
                break;
            case 4:
                gfx = gBHpgagEFrame35Eu;
                break;
            case 5:
                gfx = gBHpgagEFrame36Eu;
                break;
            case 6:
            default:
                gfx = gBHpgagEFrame37Eu;
                break;
            }

            break;
        }

        bar = gBHpgagEFrame0Eu;
#else
        switch (work->gaugeSize) {
        case 0:
        case 1:
            gfx = gBHpgagEFrame1;
            break;
        case 2:
            gfx = gBHpgagEFrame2;
            break;
        case 3:
            gfx = gBHpgagEFrame3;
            break;
        case 4:
            gfx = gBHpgagEFrame4;
            break;
        case 5:
            gfx = gBHpgagEFrame5;
            break;
        case 6:
            gfx = gBHpgagEFrame6;
            break;
        default:
            gfx = gBHpgagEFrame6;
            break;
        }

        bar = gBHpgagEFrame0;
#endif
        break;
    }

    DrawSprite(236, 2, gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 3);
#ifdef PLATFORM_PC
    if (gPortHdHudPart) gPortHdHudPart = 2;
#endif
#ifdef VERSION_EU
    DrawSprite(236, 2, gBHpgagEFrame13Eu, work->tiles3, work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 1);
#else
    DrawSprite(236, 2, gBHpgagEFrame13, work->tiles3, work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 1);
#endif

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

    scale *= 2;

    if (work->displayHp > 0) {
        if (scale <= 9) {
            scale = 10;
        }

        if (scale > Q_8_8(1)) {
            affine = AllocObjAffine(0, scale, Q_8_8(1), TRUE);
        } else {
            affine = AllocObjAffine(0, scale, Q_8_8(1), FALSE);
        }

#ifdef PLATFORM_PC
        if (gPortHdHudPart) gPortHdHudPart = 3;
#endif
        DrawSprite(217, 6, bar, work->tiles2, work->palette, affine, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 2);
    }
}

void task_btl_hpenm_3(BtlHpenmWork* work) {
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescBtlHpenm = {
    "task_btl_hpenm",
    (TaskInitFunc)task_btl_hpenm_0,
    (TaskUpdateFunc)task_btl_hpenm_1,
    (TaskDrawFunc)task_btl_hpenm_2,
    (TaskDestroyFunc)task_btl_hpenm_3,
    sizeof(BtlHpenmWork),
};
