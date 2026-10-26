/**
 * btl_exp.c
 * Experience Point Display
 */

#include "system_state.h"
#include "btl4.h"
#include "sprites_btl_hud.h"
#include "battle_work.h"
#include "game_state.h"
#include "obj.h"
#include "obj_api.h"
#include "player_progression_types.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"

#ifdef PLATFORM_PC
#include "engine.h"
/* Battle EXP notices retain their original pixel-art numbers even with HD
 * text enabled for dialogue, subtitles and menus. Restore any caller tag. */
extern int gPortTextChar;
#endif

void* GetExpDigitGfx(s32 digit, u8 leading) {
    switch (digit) {
    case 0:
        if (leading) {
#ifdef VERSION_JP
            return gBtlExpNextFrame4;
#else
            return gBtlExpNextLvFrame4;
#endif
        }

        break;
    case 1:
#ifdef VERSION_JP
        return gBtlExpNextFrame5;
#else
        return gBtlExpNextLvFrame5;
#endif
    case 2:
#ifdef VERSION_JP
        return gBtlExpNextFrame6;
#else
        return gBtlExpNextLvFrame6;
#endif
    case 3:
#ifdef VERSION_JP
        return gBtlExpNextFrame7;
#else
        return gBtlExpNextLvFrame7;
#endif
    case 4:
#ifdef VERSION_JP
        return gBtlExpNextFrame8;
#else
        return gBtlExpNextLvFrame8;
#endif
    case 5:
#ifdef VERSION_JP
        return gBtlExpNextFrame9;
#else
        return gBtlExpNextLvFrame9;
#endif
    case 6:
#ifdef VERSION_JP
        return gBtlExpNextFrame10;
#else
        return gBtlExpNextLvFrame10;
#endif
    case 7:
#ifdef VERSION_JP
        return gBtlExpNextFrame11;
#else
        return gBtlExpNextLvFrame11;
#endif
    case 8:
#ifdef VERSION_JP
        return gBtlExpNextFrame12;
#else
        return gBtlExpNextLvFrame12;
#endif
    case 9:
#ifdef VERSION_JP
        return gBtlExpNextFrame13;
#else
        return gBtlExpNextLvFrame13;
#endif
    }

    return NULL;
}

void BtlExpSetNumber(BtlExpWork* work, u32 value) {
    void* tenThousands;
    void* thousands;
    void* hundreds;
    void* tens;
    u8 leading;

    tenThousands = GetExpDigitGfx(value / 10000, FALSE);
    work->gfx2[0] = tenThousands;
    value %= 10000;
    leading = tenThousands != NULL;

    thousands = GetExpDigitGfx(value / 1000, leading);
    work->gfx2[1] = thousands;
    value %= 1000;

    if (thousands != NULL) {
        leading = TRUE;
    }

    hundreds = GetExpDigitGfx(value / 100, leading);
    work->gfx2[2] = hundreds;
    value %= 100;

    if (hundreds != NULL) {
        leading = TRUE;
    }

    tens = GetExpDigitGfx(value / 10, leading);
    work->gfx2[3] = tens;
    value %= 10;

    if (tens != NULL) {
        leading = TRUE;
    }

#ifdef VERSION_JP
    work->gfx2[4] = gBtlExpNextFrames[value + 4];
    work->gfx2[5] = gBtlExpNextFrame14;
#else
    work->gfx2[4] = gBtlExpNextLvFrames[value + 4];
    work->gfx2[5] = gBtlExpNextLvFrame14;
#endif
}

enum BtlExpState {
    BTL_EXP_STATE_HIDDEN,
    BTL_EXP_STATE_GAIN,
    BTL_EXP_STATE_NEXT_LEVEL,
    BTL_EXP_STATE_LEVEL_UP
};

void task_btl_exp_0(BtlExpWork* work) {
    s32 i;

    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
#if defined(VERSION_EU)
    work->tiles = AllocObjTiles(0xC0, gBtlExpNextLvTiles);
#elif defined(VERSION_JP)
    work->tiles = AllocObjTiles(0xA0, gBtlExpNextTiles);
#elif defined(VERSION_US)
    work->tiles = AllocObjTiles(0xA0, gBtlExpNextLvTiles);
#endif
    work->gfx = NULL;

    for (i = 0; i <= 5; i++) {
#ifdef VERSION_JP
        work->tiles2[i] = AllocObjTiles(32, gBtlExpNextTiles);
#else
        work->tiles2[i] = AllocObjTiles(32, gBtlExpNextLvTiles);
#endif
        work->gfx2[i] = NULL;
    }

    work->timer = 0;
    work->level = gGameState.progression.level;
    work->lastExp = gGameState.progression.exp;
    work->state = BTL_EXP_STATE_HIDDEN;
    work->gainedExp = 0;
}

s32 task_btl_exp_1(BtlExpWork* work) {
    if (gBtlWork->flags & BTL_FLAG_FIELD_HIDDEN) {
        return 0;
    }

    if (work->level < gGameState.progression.level) {
        BtlExpSetNumber(work, gGameState.progression.level);

#if defined(VERSION_EU)
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            work->gfx = gBtlExpNextLvFrame0;
            break;
        case LANGUAGE_FRENCH:
            work->gfx = gBtlExpNextLvFrame16;
            break;
        case LANGUAGE_SPANISH:
            work->gfx = gBtlExpNextLvFrame18;
            break;
        case LANGUAGE_ITALIAN:
            work->gfx = gBtlExpNextLvFrame20;
            break;
        case LANGUAGE_GERMAN:
        default:
            work->gfx = gBtlExpNextLvFrame24;
            break;
        }
#elif defined(VERSION_JP)
        work->gfx = gBtlExpNextFrame0;
#elif defined(VERSION_US)
        work->gfx = gBtlExpNextLvFrame0;
#endif
        work->timer = 0;
        work->state = BTL_EXP_STATE_LEVEL_UP;
        work->level = gGameState.progression.level;
        work->gainedExp = 0;
    }

    if (work->state != BTL_EXP_STATE_LEVEL_UP) {
        if (work->lastExp < gGameState.progression.exp) {
            work->gainedExp += gGameState.progression.exp - work->lastExp;
            BtlExpSetNumber(work, work->gainedExp);

#if defined(VERSION_EU)
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                work->gfx = gBtlExpNextLvFrame2;
                break;
            case LANGUAGE_FRENCH:
                work->gfx = gBtlExpNextLvFrame2;
                break;
            case LANGUAGE_SPANISH:
                work->gfx = gBtlExpNextLvFrame2;
                break;
            case LANGUAGE_ITALIAN:
                work->gfx = gBtlExpNextLvFrame22;
                break;
            case LANGUAGE_GERMAN:
            default:
                work->gfx = gBtlExpNextLvFrame26;
                break;
            }
#elif defined(VERSION_JP)
            work->gfx = gBtlExpNextFrame2;
#elif defined(VERSION_US)
            work->gfx = gBtlExpNextLvFrame2;
#endif
            work->timer = 0;
            work->state = BTL_EXP_STATE_GAIN;
            work->lastExp = gGameState.progression.exp;
        }
    }

    switch (work->state) {
    case BTL_EXP_STATE_HIDDEN:
        break;
    case BTL_EXP_STATE_LEVEL_UP:
        if (work->timer > 100) {
            if (gGameState.progression.level > 98) {
                work->state = BTL_EXP_STATE_HIDDEN;
            } else {
                work->state = BTL_EXP_STATE_NEXT_LEVEL;
                BtlExpSetNumber(work, gGameState.progression.nextExp - gGameState.progression.exp);

#if defined(VERSION_EU)
                switch (gLanguage) {
                case LANGUAGE_ENGLISH:
                    work->gfx = gBtlExpNextLvFrame3;
                    break;
                case LANGUAGE_FRENCH:
                    work->gfx = gBtlExpNextLvFrame17;
                    break;
                case LANGUAGE_SPANISH:
                    work->gfx = gBtlExpNextLvFrame19;
                    break;
                case LANGUAGE_ITALIAN:
                    work->gfx = gBtlExpNextLvFrame21;
                    break;
                case LANGUAGE_GERMAN:
                default:
                    work->gfx = gBtlExpNextLvFrame25;
                    break;
                }
#elif defined(VERSION_JP)
                work->gfx = gBtlExpNextFrame3;
#elif defined(VERSION_US)
                work->gfx = gBtlExpNextLvFrame3;
#endif
            }

            work->timer = 0;
            work->gainedExp = 0;
        } else {
            work->timer++;
        }

        break;
    case BTL_EXP_STATE_GAIN:
        if (work->timer > 60) {
            if (gGameState.progression.level > 98) {
                work->state = BTL_EXP_STATE_HIDDEN;
            } else {
                work->state = BTL_EXP_STATE_NEXT_LEVEL;
                BtlExpSetNumber(work, gGameState.progression.nextExp - gGameState.progression.exp);

#if defined(VERSION_EU)
                switch (gLanguage) {
                case LANGUAGE_ENGLISH:
                    work->gfx = gBtlExpNextLvFrame3;
                    break;
                case LANGUAGE_FRENCH:
                    work->gfx = gBtlExpNextLvFrame17;
                    break;
                case LANGUAGE_SPANISH:
                    work->gfx = gBtlExpNextLvFrame19;
                    break;
                case LANGUAGE_ITALIAN:
                    work->gfx = gBtlExpNextLvFrame21;
                    break;
                case LANGUAGE_GERMAN:
                default:
                    work->gfx = gBtlExpNextLvFrame25;
                    break;
                }
#elif defined(VERSION_JP)
                work->gfx = gBtlExpNextFrame3;
#elif defined(VERSION_US)
                work->gfx = gBtlExpNextLvFrame3;
#endif
            }

            work->timer = 0;
            work->gainedExp = 0;
        } else {
            work->timer++;
        }

        break;
    case BTL_EXP_STATE_NEXT_LEVEL:
        if (work->timer > 100) {
            work->timer = 0;
            work->state = BTL_EXP_STATE_HIDDEN;
        } else {
            work->timer++;
        }

        break;
    }

    return 1;
}

#ifdef PLATFORM_PC
static void task_btl_exp_2_Body(BtlExpWork* work);

/* EXP gained / to the next level: under the player's HP gauge, so at the top-left corner of the screen with it
   (it stayed at the GBA picture's left edge in the wide view, user 25/10). */
void task_btl_exp_2(BtlExpWork* work) {
    PortHudBegin(-1, -1);
    task_btl_exp_2_Body(work);
    PortHudEnd();
}

static void task_btl_exp_2_Body(BtlExpWork* work) {
#else
void task_btl_exp_2(BtlExpWork* work) {
#endif
    s32 i;
    s16 x;
    u16 y;

    if (work->state != BTL_EXP_STATE_HIDDEN) {
        y = 40;
        x = 0;
        DrawSprite(0, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, x);

#ifdef VERSION_JP
        x = 32;
#else
        if (work->state == BTL_EXP_STATE_NEXT_LEVEL) {
#ifdef VERSION_EU
            x = 48;
#else
            x = 40;
#endif
        } else {
            x = 32;
        }
#endif

        for (i = 0; i <= 5; i++) {
            if (work->gfx2[i] != NULL) {
#ifdef PLATFORM_PC
                int previousChar = gPortTextChar;
                gPortTextChar = 0;
#endif
                DrawSprite(x, y, work->gfx2[i], work->tiles2[i], work->palette, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC, 0);
#ifdef PLATFORM_PC
                gPortTextChar = previousChar;
#endif
                x += 8;
            }
        }
    }
}

void task_btl_exp_3(BtlExpWork* work) {
    s32 i;

    for (i = 0; i < 6; i++) {
        ReleaseObjTiles(work->tiles2[i]);
    }

    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescBtlExp = {
    "task_btl_exp",
    (TaskInitFunc)task_btl_exp_0,
    (TaskUpdateFunc)task_btl_exp_1,
    (TaskDrawFunc)task_btl_exp_2,
    (TaskDestroyFunc)task_btl_exp_3,
    sizeof(BtlExpWork),
};
