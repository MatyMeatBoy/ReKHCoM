/**
 * monsgage.c
 * Monstro Event Battle Gauge
 */

#include "system_state.h"
#include "monsgage.h"
#include "obj_api.h"
#include "sprites_btl_hud.h"
#include "battle_work.h"
#include "obj.h"
#include <stddef.h>
#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "engine_math.h"

#ifdef VERSION_EU
void* GetLocalizedString(const void* strings) {
    void* const* byLanguage = strings;

    switch (gLanguage) {
    case LANGUAGE_ITALIAN:
        return byLanguage[3];
    case LANGUAGE_FRENCH:
        return byLanguage[1];
    case LANGUAGE_SPANISH:
        return byLanguage[4];
    case LANGUAGE_GERMAN:
        return byLanguage[2];
    case LANGUAGE_ENGLISH:
    default:
        return byLanguage[0];
    }
}

void* GetLocalizedLines(const void* text) {
    void* const* byLanguage = text;

    switch (gLanguage) {
    case LANGUAGE_ITALIAN:
        return byLanguage[3];
    case LANGUAGE_FRENCH:
        return byLanguage[1];
    case LANGUAGE_SPANISH:
        return byLanguage[4];
    case LANGUAGE_GERMAN:
        return byLanguage[2];
    case LANGUAGE_ENGLISH:
    default:
        return byLanguage[0];
    }
}

s32 GetLocalizedLineCount(const void* text) {
    const u16* halfwords = text;

    switch (gLanguage) {
    case LANGUAGE_ITALIAN:
        return halfwords[13];
    case LANGUAGE_FRENCH:
        return halfwords[11];
    case LANGUAGE_SPANISH:
        return halfwords[14];
    case LANGUAGE_GERMAN:
        return halfwords[12];
    case LANGUAGE_ENGLISH:
    default:
        return halfwords[10];
    }
}
#endif

enum MonsgageState {
    MONSGAGE_STATE_HOLD,
    MONSGAGE_STATE_DRAIN,
    MONSGAGE_STATE_FULL
};

void task_monsgage_0(MonsgageWork* work) {
    work->tiles = AllocObjTiles(0x200, gMonsgageTiles);
    work->tiles2 = AllocObjTiles(0x80, gMonsgageTiles);
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->shownValue = 0;
    work->value = 0;
    work->gfx = gMonsgageFrame0;
    work->gfx2 = gMonsgageFrame1;
    work->timer = 0;
    work->state = MONSGAGE_STATE_HOLD;
    work->visible = TRUE;
}

s32 task_monsgage_1(MonsgageWork* work) {
    if (gBtlWork->phase != BTL_PHASE_START) {
        if (gBtlWork->phase == BTL_PHASE_END) {
            return 0;
        }

        switch (work->state) {
        case MONSGAGE_STATE_HOLD:
            if (work->timer == 0) {
                work->visible = TRUE;
                work->gfx2 = gMonsgageFrame1;
            }

#ifdef VERSION_EU
            if (gBtlWork->flags & BTL_FLAG_ENEMY_DEFEATED) {
                gBtlWork->flags &= ~BTL_FLAG_ENEMY_DEFEATED;
                work->value += 20;

                if (work->value > 255) {
                    work->value = 256;
                    work->state = MONSGAGE_STATE_FULL;
                    work->timer = 0;
                    break;
                }
            } else if (work->timer > 120) {
                work->state = MONSGAGE_STATE_DRAIN;
                work->timer = 0;
                break;
            }

            work->timer++;
#else
            if (work->timer > 120) {
                work->state = MONSGAGE_STATE_DRAIN;
                work->timer = 0;
            } else {
                work->timer++;
            }
#endif

            break;
        case MONSGAGE_STATE_DRAIN:
            if (work->timer == 0) {
                work->gfx2 = gMonsgageFrame2;
            }

            if (work->timer % 8 < 4) {
                work->visible = TRUE;
            } else {
                work->visible = FALSE;
            }

            if ((work->timer % 4) == 0) {
                work->value--;

                if (work->value < 0) {
                    work->value = 0;
                }
            }

            if (gBtlWork->flags & BTL_FLAG_ENEMY_DEFEATED) {
                gBtlWork->flags &= ~BTL_FLAG_ENEMY_DEFEATED;
                work->timer = 0;
#ifdef VERSION_EU
                work->value += 20;
#else
                work->value += 25;
#endif

                if (work->value <= 255) {
                    work->state = MONSGAGE_STATE_HOLD;
                } else {
                    work->value = 256;
                    work->state = MONSGAGE_STATE_FULL;
                }
            } else {
                work->timer++;
            }

            break;
        case MONSGAGE_STATE_FULL:
            if (work->timer == 0) {
                work->gfx2 = gMonsgageFrame3;
                work->gfx = gMonsgageFrame4;
                gBtlWork->flags |= BTL_FLAG_STOP_SPAWNING;
                gBtlWork->flags |= 0x100000;
            }

            if (work->timer % 8 < 4) {
                work->visible = TRUE;
            } else {
                work->visible = FALSE;
            }

            if (work->timer > 99 && gBtlWork->enemyCount == 0) {
                gBtlWork->flags |= BTL_FLAG_BATTLE_OVER;
            }

            work->timer++;
            break;
        }

        work->shownValue += (work->value - work->shownValue) >> 2;
    }

    return 1;
}

#ifdef PLATFORM_PC
static void task_monsgage_2_Body(MonsgageWork* work);

/* Monstro's objective gauge: docked to the top-right corner with the monster's symbol in the wide view, as the enemy
   gauge it sits beside (it stayed at the cartridge's place in the playfield). */
void task_monsgage_2(MonsgageWork* work) {
    PortHudBegin(1, -1);
    task_monsgage_2_Body(work);
    PortHudEnd();
}

static void task_monsgage_2_Body(MonsgageWork* work) {
#else
void task_monsgage_2(MonsgageWork* work) {
#endif
    ObjAffine* affine;

    if (gBtlWork->phase != BTL_PHASE_START) {
        DrawSprite(172, 12, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 3);

        if (work->visible) {
            if (work->shownValue * 2 > 4) {
                if (work->shownValue * 2 > Q_8_8(1)) {
                    affine = AllocObjAffine(0, work->shownValue * 2, Q_8_8(1), TRUE);
                } else {
                    affine = AllocObjAffine(0, work->shownValue * 2, Q_8_8(1), FALSE);
                }

                DrawSprite(174, 16, work->gfx2, work->tiles2, work->palette, affine, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 2);
            }
        }
    }
}

void task_monsgage_3(MonsgageWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescMonsgage = {
    "task_monsgage",
    (TaskInitFunc)task_monsgage_0,
    (TaskUpdateFunc)task_monsgage_1,
    (TaskDrawFunc)task_monsgage_2,
    (TaskDestroyFunc)task_monsgage_3,
    sizeof(MonsgageWork),
};
