/**
 * title.c
 * Title Screen Logo and Menu
 */

#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "registration_data.h"
#include "system_state.h"
#include "title_api.h"
#include "pallet.h"
#include "title.h"
#include "gba/keys.h"
#include "sprites_title.h"
#include "songs.h"
#include <string.h>
#include "anim.h"
#include "engine_math.h"
#include "game_state.h"
#include "key.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "title_types.h"
#include "types.h"
#include "mode_title.h"
#include "gba/defines.h"
#include "macros.h"

#ifdef VERSION_EU
extern void** gTitleLumiSpritesEu[5];
extern void** gTitleLumiSpritesAltEu[5];
#endif

TaskDesc gTaskDescTitleLogo = {
    "task_title_logo",
    (TaskInitFunc)task_title_logo_0,
    (TaskUpdateFunc)task_title_logo_1,
    (TaskDrawFunc)task_title_logo_2,
    (TaskDestroyFunc)task_title_logo_3,
    sizeof(TitleLogoWork),
};

TaskDesc gTaskDescTitleObj = {
    "task_title_obj",
    (TaskInitFunc)task_title_obj_0,
    (TaskUpdateFunc)task_title_obj_1,
    (TaskDrawFunc)task_title_obj_2,
    (TaskDestroyFunc)task_title_obj_3,
    sizeof(TitleObjWork),
};

static const s32 sTitleMenuChoiceOrder[4] = {
    TITLE_MENU_NEW_GAME_SORA,
    TITLE_MENU_NEW_GAME_RIKU,
    TITLE_MENU_CONTINUE,
    TITLE_MENU_LINK_BATTLE,
};

#ifdef VERSION_EU
static void** sTitleMenuEntrySpritesEu[5] = {
    gTitleMenuEntryFrames,
    gTitleMenuEntryFrenchFrames,
    gTitleMenuEntryGermanFrames,
    gTitleMenuEntryItalianFrames,
    gTitleMenuEntrySpanishFrames,
};
#endif

TaskDesc gTaskDescTitleMenu = {
    "task_title_menu",
    (TaskInitFunc)task_title_menu_0,
    (TaskUpdateFunc)task_title_menu_1,
    (TaskDrawFunc)task_title_menu_2,
    (TaskDestroyFunc)task_title_menu_3,
    sizeof(TitleMenuWork),
};

static const s16 sTitleLumiLevels[3] = {-7, 0, 3};

static u8 sTitleLogoScaleDone;
static u16 sUnk_02034ECE;
static u8 sTitleObjSlideDone;
static u16 sUnk_02034ED2;

void TitleLogoLoadSprites(TitleLogoWork* work) {
    work->sprites[0].tiles = LoadObjTiles(gTitleLogoDisneySquareEnixTiles, sizeof(gTitleLogoDisneySquareEnixTiles));
    work->sprites[0].gfx = gTitleLogoDisneySquareEnixFrames[0];

    if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
        work->sprites[1].tiles = LoadObjTiles(gTitleLogoReverseRebirthTiles, sizeof(gTitleLogoReverseRebirthTiles));
        work->sprites[1].gfx = gTitleLogoReverseRebirthFrames[0];
        work->sprites[1].palette = LoadObjPalette(gTitleLogoReverseRebirthPalette, sizeof(gTitleLogoReverseRebirthPalette));
    } else {
        work->sprites[1].tiles = LoadObjTiles(gTitleLogoKanaTiles, sizeof(gTitleLogoKanaTiles));
        work->sprites[1].gfx = gTitleLogoKanaFrames[0];
        work->sprites[1].palette = LoadObjPalette(gTitleLogoKanaPalette, sizeof(gTitleLogoKanaPalette));
    }

    work->sprites[2].tiles = LoadObjTiles(gTitleLogoChainOfMemoriesTiles, sizeof(gTitleLogoChainOfMemoriesTiles));
    work->sprites[2].gfx = gTitleLogoChainOfMemoriesFrames[0];
    work->sprites[3].tiles = LoadObjTiles(gTitleLogoCrownTiles, sizeof(gTitleLogoCrownTiles));
    work->sprites[3].gfx = gTitleLogoCrownFrames[0];
    work->sprites[4].tiles = LoadObjTiles(gTitleLogoKingdomHeartsTiles, sizeof(gTitleLogoKingdomHeartsTiles));
    work->sprites[4].gfx = gTitleLogoKingdomHeartsFrames[0];
    work->sprites[5].tiles = LoadObjTiles(gTitleLogoHeartTiles, sizeof(gTitleLogoHeartTiles));
    work->sprites[5].gfx = gTitleLogoHeartFrames[0];
    work->sprites[0].palette = LoadObjPalette(gTitleLogoDisneySquareEnixPalette, sizeof(gTitleLogoDisneySquareEnixPalette));
    work->sprites[2].palette = LoadObjPalette(gTitleLogoChainOfMemoriesPalette, sizeof(gTitleLogoChainOfMemoriesPalette));
    work->sprites[3].palette = LoadObjPalette(gTitleLogoChainOfMemoriesPalette, sizeof(gTitleLogoChainOfMemoriesPalette));
    work->sprites[4].palette = LoadObjPalette(gTitleLogoKingdomHeartsPalette, sizeof(gTitleLogoKingdomHeartsPalette));
    work->sprites[5].palette = LoadObjPalette(gTitleLogoHeartPalette, 0x20);
    work->scale = 0;
    work->unk_48 = 0xC00;
    work->unk_50 = 0;
    sTitleLogoScaleDone = FALSE;
}

void task_title_logo_0(TitleLogoWork* work) {
    TitleLogoLoadSprites(work);
}

u8 task_title_logo_1(TitleLogoWork* work) {
    if (IsTitleLogoShown() && !sTitleLogoScaleDone) {
        work->unk_48 -= 76;
        work->scale += 6;

        if (work->scale > 255) {
            work->scale = Q_8_8(1);
            work->unk_48 = 0;
            sTitleLogoScaleDone = TRUE;
        }
    }

    return 1;
}

void task_title_logo_2(TitleLogoWork* work) {
    s32 i;
    ObjAffine* affine;
    s16 x;
    s16 y;

    for (i = 0; i < 6; i++) {
#ifndef VERSION_JP
        if (i == 1 && !(gGameState.flags & GAME_FLAG_RIKU_TITLE)) {
            continue;
        }
#endif

        if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
            x = 0xA4;
        } else {
            x = 0x50;
        }

        y = 70;

        if (i == 0) {
            x++;
        }

        if (i == 1) {
            if (work->scale == 0) {
                continue;
            }

            affine = AllocObjAffine(0, Q_8_8(1), work->scale, FALSE);

            if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
                y = 86;
                x--;
            } else {
                y = 87;
            }
        } else {
            affine = NULL;
        }

        DrawSprite(x, y, work->sprites[i].gfx, work->sprites[i].tiles, work->sprites[i].palette, affine, 0, i + 20);
    }
}

void task_title_logo_3(TitleLogoWork* work) {
    s32 i;

    for (i = 0; i < 6; i++) {
        ReleaseObjTiles(work->sprites[i].tiles);
        ReleaseObjPalette(work->sprites[i].palette);
    }
}

void PackLowBytes(u8* src, u16* dst, u16 size) {
    while (size != 0) {
        *dst = src[0] + (src[2] << 8);
        dst++;
        src += 4;
        size -= 4;
    }
}

u8 IsTitleLogoScaleDone() {
    return sTitleLogoScaleDone;
}

void task_title_obj_0(TitleObjWork* work) {
    s32 paletteOffset;

    paletteOffset = (gGameState.flags & GAME_FLAG_RIKU_TITLE) ? 0x20 : 0;
#ifdef VERSION_EU
    work->sprites[0].palette = LoadObjPalette(gTitleLogoKingdomHeartsPalette, sizeof(gTitleLogoKingdomHeartsPalette));

    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->sprites[0].tiles = LoadObjTiles(gTitleDevelopedByTiles, sizeof(gTitleDevelopedByTiles));
        work->sprites[0].gfx = gTitleDevelopedByFrames[0];
        break;
    case LANGUAGE_FRENCH:
        work->sprites[0].tiles = LoadObjTiles(gTitleDevelopedByFrenchTiles, sizeof(gTitleDevelopedByFrenchTiles));
        work->sprites[0].gfx = gTitleDevelopedByFrenchFrames[0];
        break;
    case LANGUAGE_GERMAN:
        work->sprites[0].tiles = LoadObjTiles(gTitleDevelopedByGermanTiles, sizeof(gTitleDevelopedByGermanTiles));
        work->sprites[0].gfx = gTitleDevelopedByGermanFrames[0];
        break;
    case LANGUAGE_ITALIAN:
        work->sprites[0].tiles = LoadObjTiles(gTitleDevelopedByItalianTiles, sizeof(gTitleDevelopedByItalianTiles));
        work->sprites[0].gfx = gTitleDevelopedByItalianFrames[0];
        break;
    case LANGUAGE_SPANISH:
        work->sprites[0].tiles = LoadObjTiles(gTitleDevelopedBySpanishTiles, sizeof(gTitleDevelopedBySpanishTiles));
        work->sprites[0].gfx = gTitleDevelopedBySpanishFrames[0];
        break;
    case 5:
    case 6:
        break;
    }
#else
    work->sprites[0].tiles = LoadObjTiles(gTitleDevelopedByTiles, sizeof(gTitleDevelopedByTiles));
    work->sprites[0].palette = LoadObjPalette(gTitleLogoKingdomHeartsPalette, sizeof(gTitleLogoKingdomHeartsPalette));
    work->sprites[0].gfx = gTitleDevelopedByFrames[0];
#endif

    if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
#ifdef VERSION_EU
        if (gLanguage == LANGUAGE_SPANISH || gLanguage == LANGUAGE_GERMAN) {
            work->sprites[0].x = 0xB500;
        } else {
            work->sprites[0].x = 0xBA00;
        }
#else
        work->sprites[0].x = 0xBA00;
#endif
        work->sprites[0].y = 0x76;
    } else {
        work->sprites[0].x = 0x3D00;
        work->sprites[0].y = 0x77;
    }

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->sprites[1].tiles = LoadObjTiles(gTitlePressStartTiles, sizeof(gTitlePressStartTiles));
        break;
    case LANGUAGE_FRENCH:
        work->sprites[1].tiles = LoadObjTiles(gTitlePressStartFrenchTiles, sizeof(gTitlePressStartFrenchTiles));
        break;
    case LANGUAGE_GERMAN:
        work->sprites[1].tiles = LoadObjTiles(gTitlePressStartGermanTiles, sizeof(gTitlePressStartGermanTiles));
        break;
    case LANGUAGE_ITALIAN:
        work->sprites[1].tiles = LoadObjTiles(gTitlePressStartItalianTiles, sizeof(gTitlePressStartItalianTiles));
        break;
    case LANGUAGE_SPANISH:
        work->sprites[1].tiles = LoadObjTiles(gTitlePressStartSpanishTiles, sizeof(gTitlePressStartSpanishTiles));
        break;
    case 5:
    case 6:
        break;
    }
#else
    work->sprites[1].tiles = LoadObjTiles(gTitlePressStartTiles, sizeof(gTitlePressStartTiles));
#endif
    work->sprites[1].palette = LoadObjPalette(&gTitleObjPalettes[paletteOffset], 0x20);
    work->sprites[1].x = -0x7800;
    work->sprites[1].targetX = 0x7C00;
    work->sprites[1].y = 0xA0;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        AnimInit(&work->anim, gTitlePressStartAnims, gTitlePressStartFrames);
        break;
    case LANGUAGE_FRENCH:
        AnimInit(&work->anim, gTitlePressStartFrenchAnims, gTitlePressStartFrenchFrames);
        break;
    case LANGUAGE_GERMAN:
        AnimInit(&work->anim, gTitlePressStartGermanAnims, gTitlePressStartGermanFrames);
        break;
    case LANGUAGE_ITALIAN:
        AnimInit(&work->anim, gTitlePressStartItalianAnims, gTitlePressStartItalianFrames);
        break;
    case LANGUAGE_SPANISH:
        AnimInit(&work->anim, gTitlePressStartSpanishAnims, gTitlePressStartSpanishFrames);
        break;
    case 5:
    case 6:
        break;
    }
#else
    AnimInit(&work->anim, gTitlePressStartAnims, gTitlePressStartFrames);
#endif
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->sprites[1].gfx = AnimGetGfx(&work->anim);
    work->sprites[2].tiles = LoadObjTiles(gTitleGameStartTiles, sizeof(gTitleGameStartTiles));
    work->sprites[2].palette = LoadObjPalette(&gTitleObjPalettes[paletteOffset], 0x20);
    work->sprites[2].gfx = gTitleGameStartFrames[0];
    work->sprites[2].x = 0x15800;
    work->sprites[2].targetX = 0xB800;
    work->sprites[2].y = 0x91;
    work->slideTimer = 30;
    sTitleObjSlideDone = FALSE;
    work->slideDelay = 0;
}

u8 task_title_obj_1(TitleObjWork* work) {
    if (IsTitleIntroDone()) {
        if (work->slideDelay != 0) {
            work->slideDelay--;
        } else if (work->slideTimer != 0) {
            ApproachValue(&work->sprites[1].x, work->sprites[1].targetX, work->slideTimer);
            ApproachValue(&work->sprites[2].x, work->sprites[2].targetX, work->slideTimer);
            work->slideTimer--;

            if (work->slideTimer == 0) {
                sTitleObjSlideDone = TRUE;
            }
        }
    }

    return 1;
}

#ifdef VERSION_JP
#define TITLE_OBJ_DRAW_COUNT 3
#else
#define TITLE_OBJ_DRAW_COUNT 2
#endif

void task_title_obj_2(TitleObjWork* work) {
    s32 i;

    work->sprites[1].gfx = AnimUpdate(&work->anim);

    for (i = 0; i < TITLE_OBJ_DRAW_COUNT; i++) {
        DrawSprite(work->sprites[i].x >> 8, work->sprites[i].y, work->sprites[i].gfx,
                   work->sprites[i].tiles, work->sprites[i].palette, NULL, 0, i);
    }
}

void task_title_obj_3(TitleObjWork* work) {
    s32 i;

    for (i = 0; i < 3; i++) {
        ReleaseObjTiles(work->sprites[i].tiles);
        ReleaseObjPalette(work->sprites[i].palette);
    }
}

u8 IsTitleObjSlideDone() {
    return sTitleObjSlideDone;
}

enum TitleMenuLayout {
    TITLE_MENU_LAYOUT_BASIC,
    TITLE_MENU_LAYOUT_SINGLE_NEW_GAME,
    TITLE_MENU_LAYOUT_SINGLE_RESUME,
    TITLE_MENU_LAYOUT_FULL,
    TITLE_MENU_LAYOUT_NEW_GAME
};

#ifdef PLATFORM_PC
static int TitleRikuAvailable(void) {
    return PortRikuAvailable(gGameState.flags & GAME_FLAG_SORA_CLEAR);
}
/* Riku can be unlocked before Sora's story is cleared: the menus then have one row fewer */
#define TITLE_MENU_FULL_COUNT ((gGameState.flags & GAME_FLAG_SORA_CLEAR) ? 3 : 2)
#define TITLE_MENU_FULL_ROWS ((gGameState.flags & GAME_FLAG_SORA_CLEAR) ? 4 : 3)
#else
#define TitleRikuAvailable() (gGameState.flags & GAME_FLAG_SORA_CLEAR)
#define TITLE_MENU_FULL_COUNT 3
#define TITLE_MENU_FULL_ROWS 4
#endif

void task_title_menu_0(TitleMenuWork* work, s16* choice) {
    s32 paletteOffset;
    u8* objPal;
    u8* lumiPal;

    paletteOffset = (gGameState.flags & GAME_FLAG_RIKU_TITLE) ? 0x20 : 0;
    work->choice = choice;

    if (choice[0] == TITLE_MENU_NEW_GAME) {
        if (TitleRikuAvailable()) {
            work->layout = TITLE_MENU_LAYOUT_NEW_GAME;
            choice[0] = TITLE_MENU_NEW_GAME_SORA;
        } else {
            work->layout = TITLE_MENU_LAYOUT_SINGLE_NEW_GAME;
        }
    } else if (choice[0] == TITLE_MENU_RESUME) {
        work->layout = TITLE_MENU_LAYOUT_SINGLE_RESUME;
    } else if (TitleRikuAvailable()) {
        work->layout = TITLE_MENU_LAYOUT_FULL;
    } else {
        work->layout = TITLE_MENU_LAYOUT_BASIC;
    }

#ifdef PLATFORM_PC
    { extern void PortLog(const char*, ...);
      PortLog("title-access: layout=%d choice=%d flags=%u riku=%d", work->layout, choice[0],
              (unsigned)gGameState.flags, TitleRikuAvailable()); }
#endif

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->tiles = LoadObjTiles(gTitleMenuEntryTiles, sizeof(gTitleMenuEntryTiles));
        break;
    case LANGUAGE_FRENCH:
        work->tiles = LoadObjTiles(gTitleMenuEntryFrenchTiles, sizeof(gTitleMenuEntryFrenchTiles));
        break;
    case LANGUAGE_GERMAN:
        work->tiles = LoadObjTiles(gTitleMenuEntryGermanTiles, sizeof(gTitleMenuEntryGermanTiles));
        break;
    case LANGUAGE_ITALIAN:
        work->tiles = LoadObjTiles(gTitleMenuEntryItalianTiles, sizeof(gTitleMenuEntryItalianTiles));
        break;
    case LANGUAGE_SPANISH:
        work->tiles = LoadObjTiles(gTitleMenuEntrySpanishTiles, sizeof(gTitleMenuEntrySpanishTiles));
        break;
    case 5:
    case 6:
        break;
    }
#else
    work->tiles = LoadObjTiles(gTitleMenuEntryTiles, sizeof(gTitleMenuEntryTiles));
#endif
    work->palette = LoadObjPalette(gTitleMenuEntryPalette, sizeof(gTitleMenuEntryPalette));
    TitleCopyToPaletteBuffer(work->palette->index + 16, gTitleMenuEntryPalette, sizeof(gTitleMenuEntryPalette));
    work->tiles2[0] = LoadObjTiles(gTitleMenuCursorTiles, sizeof(gTitleMenuCursorTiles));
#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->tiles2[1] = LoadObjTiles(gTitleMenuSelectedEntryTiles, sizeof(gTitleMenuSelectedEntryTiles));
        break;
    case LANGUAGE_FRENCH:
        work->tiles2[1] = LoadObjTiles(gTitleMenuSelectedEntryFrenchTiles, sizeof(gTitleMenuSelectedEntryFrenchTiles));
        break;
    case LANGUAGE_GERMAN:
        work->tiles2[1] = LoadObjTiles(gTitleMenuSelectedEntryGermanTiles, sizeof(gTitleMenuSelectedEntryGermanTiles));
        break;
    case LANGUAGE_ITALIAN:
        work->tiles2[1] = LoadObjTiles(gTitleMenuSelectedEntryItalianTiles, sizeof(gTitleMenuSelectedEntryItalianTiles));
        break;
    case LANGUAGE_SPANISH:
        work->tiles2[1] = LoadObjTiles(gTitleMenuSelectedEntrySpanishTiles, sizeof(gTitleMenuSelectedEntrySpanishTiles));
        break;
    case 5:
    case 6:
        break;
    }

    work->tiles2[2] = LoadObjTiles(gTitleMenuSelectedBarTiles, sizeof(gTitleMenuSelectedBarTiles));
#else
    work->tiles2[1] = LoadObjTiles(gTitleMenuSelectedEntryTiles, sizeof(gTitleMenuSelectedEntryTiles));
    work->tiles2[2] = LoadObjTiles(gTitleMenuSelectedBarTiles, sizeof(gTitleMenuSelectedBarTiles));
#endif
    objPal = &gTitleObjPalettes[paletteOffset];
    work->palette2[0] = LoadObjPalette(objPal, 0x20);
    work->palette2[1] = LoadObjPalette(objPal, 0x20);
    lumiPal = &gTitleSoraLumiPalette[paletteOffset];
    work->palette2[2] = LoadObjPalette(lumiPal, 0x20);
    TitleCopyToPaletteBuffer(work->palette2[0]->index + 16, objPal, 0x20);
    TitleCopyToPaletteBuffer(work->palette2[2]->index + 16, lumiPal, 0x20);
    AnimInit(&work->anim, gTitleMenuCursorAnims, gTitleMenuCursorFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx[0] = AnimGetGfx(&work->anim);

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->gfx[1] = gTitleMenuSelectedEntryFrames[work->choice[0]];
        break;
    case LANGUAGE_FRENCH:
        work->gfx[1] = gTitleMenuSelectedEntryFrenchFrames[work->choice[0]];
        break;
    case LANGUAGE_GERMAN:
        work->gfx[1] = gTitleMenuSelectedEntryGermanFrames[work->choice[0]];
        break;
    case LANGUAGE_ITALIAN:
        work->gfx[1] = gTitleMenuSelectedEntryItalianFrames[work->choice[0]];
        break;
    case LANGUAGE_SPANISH:
        work->gfx[1] = gTitleMenuSelectedEntrySpanishFrames[work->choice[0]];
        break;
    case 5:
    case 6:
        break;
    }
#else
    work->gfx[1] = gTitleMenuSelectedEntryFrames[work->choice[0]];
#endif
    work->gfx[2] = gTitleMenuSelectedBarFrames[work->choice[0]];
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescTitleLumichange, NULL);
}

s16 TitleMenuChoiceRow(s16 choice) {
    s16 i;

    for (i = 0; i <= 3; i++) {
        if (choice == sTitleMenuChoiceOrder[i]) {
            break;
        }
    }

    if (i > 3) {
        i = 0;
    }

    return i;
}

void TitleMenuMoveBasic(s16* choice) {
    s16 max;
    u16 keys;

    max = (gGameState.flags & GAME_FLAG_SORA_CLEAR) ? TITLE_MENU_LINK_BATTLE : TITLE_MENU_CONTINUE;
    keys = GetKeysPressed() & DPAD_UP;

    if (keys != 0) {
        m4aSongNumStart(SONG_SYS_CLICK);
        (*choice)--;

        if (*choice < 0) {
            *choice = max;
        }
    } else if (GetKeysPressed() & DPAD_DOWN) {
        m4aSongNumStart(SONG_SYS_CLICK);
        *choice = *choice + 1;

        if (*choice > max) {
            *choice = TITLE_MENU_NEW_GAME;
        }
    }
}

void TitleMenuMoveOrdered(s16* choice, s16 count) {
    s16 i;

    if (GetKeysPressed() & DPAD_UP) {
        m4aSongNumStart(SONG_SYS_CLICK);
        i = TitleMenuChoiceRow(*choice);
        i--;

        if (i < 0) {
            i = count;
        }
    } else if (GetKeysPressed() & DPAD_DOWN) {
        m4aSongNumStart(SONG_SYS_CLICK);
        i = TitleMenuChoiceRow(*choice);
        i++;

        if (i > count) {
            i = 0;
        }
    } else {
        return;
    }

    *choice = sTitleMenuChoiceOrder[i];
}

#ifdef PLATFORM_PC
/* The options overlay pauses this task. Reconcile access when it resumes,
   preserving Continue and never turning a story flag into a cheat flag. */
static void RefreshTitleMenuAccess(TitleMenuWork* work) {
    int before = work->layout;
    if (TitleRikuAvailable()) {
        if (work->layout == TITLE_MENU_LAYOUT_BASIC) {
            work->layout = TITLE_MENU_LAYOUT_FULL;
            if (*work->choice == TITLE_MENU_NEW_GAME) *work->choice = TITLE_MENU_NEW_GAME_SORA;
        } else if (work->layout == TITLE_MENU_LAYOUT_SINGLE_NEW_GAME) {
            work->layout = TITLE_MENU_LAYOUT_NEW_GAME;
            *work->choice = TITLE_MENU_NEW_GAME_SORA;
        }
    } else {
        if (work->layout == TITLE_MENU_LAYOUT_FULL) {
            work->layout = TITLE_MENU_LAYOUT_BASIC;
            if (*work->choice == TITLE_MENU_NEW_GAME_SORA || *work->choice == TITLE_MENU_NEW_GAME_RIKU)
                *work->choice = TITLE_MENU_NEW_GAME;
        } else if (work->layout == TITLE_MENU_LAYOUT_NEW_GAME) {
            work->layout = TITLE_MENU_LAYOUT_SINGLE_NEW_GAME;
            *work->choice = TITLE_MENU_NEW_GAME;
        }
    }
    if (before != work->layout) {
        extern void PortLog(const char*, ...);
        PortLog("title-access: refreshed layout=%d choice=%d flags=%u riku=%d", work->layout,
                *work->choice, (unsigned)gGameState.flags, TitleRikuAvailable());
    }
}
#endif

u8 task_title_menu_1(TitleMenuWork* work) {
#ifdef PLATFORM_PC
    RefreshTitleMenuAccess(work);
#endif
    if (work->layout == TITLE_MENU_LAYOUT_BASIC) {
        TitleMenuMoveBasic(work->choice);
    } else if (work->layout == TITLE_MENU_LAYOUT_FULL) {
        TitleMenuMoveOrdered(work->choice, TITLE_MENU_FULL_COUNT);
    } else if (work->layout == TITLE_MENU_LAYOUT_NEW_GAME) {
        TitleMenuMoveOrdered(work->choice, 1);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void TitleMenuDrawBasic(TitleMenuWork* work) {
    s32 i;
    s32 soraClear;
    s16 y;
    s16 count;

    y = 32;
    soraClear = gGameState.flags & GAME_FLAG_SORA_CLEAR;
    count = 3;

    if (soraClear == 0) {
        count = 2;
        y = 48;
    }

    for (i = 0; i < count; i++) {
#ifdef VERSION_EU
        void** spr = sTitleMenuEntrySpritesEu[gLanguage];

        DrawSprite(work->x, y, spr[i], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), i + 100);
#else
        DrawSprite(work->x, y, gTitleMenuEntryFrames[i], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), i + 100);
#endif
        y += 24;
    }

    if (gGameState.flags & GAME_FLAG_SORA_CLEAR) {
        y = work->choice[0] * 24 + 32;
    } else {
        y = work->choice[0] * 24 + 48;
    }

    for (i = 0; i < 3; i++) {
        DrawSprite(work->x, y, work->gfx[i], work->tiles2[i], work->palette2[i], NULL, 0, i);
    }
}

void TitleMenuDrawFull(TitleMenuWork* work) {
    s32 i;
    s16 y;

    y = 16;

    for (i = 0; i < TITLE_MENU_FULL_ROWS; i++) {
#ifdef VERSION_EU
        void** spr = sTitleMenuEntrySpritesEu[gLanguage];

        DrawSprite(work->x, y, spr[sTitleMenuChoiceOrder[i]], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), i + 100);
#else
        DrawSprite(work->x, y, gTitleMenuEntryFrames[sTitleMenuChoiceOrder[i]], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), i + 100);
#endif
        y += 24;
    }

    y = TitleMenuChoiceRow(work->choice[0]) * 24 + 16;

    for (i = 0; i < 3; i++) {
        DrawSprite(work->x, y, work->gfx[i], work->tiles2[i], work->palette2[i], NULL, 0, i);
    }
}

void TitleMenuDrawNewGame(TitleMenuWork* work) {
    s32 i;
    s16 y;

    y = 48;

    for (i = 0; i < 2; i++) {
#ifdef VERSION_EU
        void** spr = sTitleMenuEntrySpritesEu[gLanguage];

        DrawSprite(work->x, y, spr[sTitleMenuChoiceOrder[i]], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), i + 100);
#else
        DrawSprite(work->x, y, gTitleMenuEntryFrames[sTitleMenuChoiceOrder[i]], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), i + 100);
#endif
        y += 24;
    }

    y = TitleMenuChoiceRow(work->choice[0]) * 24 + 48;

    for (i = 0; i < 3; i++) {
        DrawSprite(work->x, y, work->gfx[i], work->tiles2[i], work->palette2[i], NULL, 0, i);
    }
}

void TitleMenuDrawSingle(TitleMenuWork* work) {
    s32 i;
    s16 y;

    y = 56;

#ifdef VERSION_EU
    {
        void** spr = sTitleMenuEntrySpritesEu[gLanguage];

        DrawSprite(work->x, y, spr[work->choice[0]], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 100);
    }
#else
    DrawSprite(work->x, y, gTitleMenuEntryFrames[work->choice[0]], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 100);
#endif

    for (i = 0; i < 3; i++) {
        DrawSprite(work->x, y, work->gfx[i], work->tiles2[i], work->palette2[i], NULL, 0, i);
    }
}

void task_title_menu_2(TitleMenuWork* work) {
    work->gfx[0] = AnimUpdate(&work->anim);

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->gfx[1] = gTitleMenuSelectedEntryFrames[work->choice[0]];
        break;
    case LANGUAGE_FRENCH:
        work->gfx[1] = gTitleMenuSelectedEntryFrenchFrames[work->choice[0]];
        break;
    case LANGUAGE_GERMAN:
        work->gfx[1] = gTitleMenuSelectedEntryGermanFrames[work->choice[0]];
        break;
    case LANGUAGE_ITALIAN:
        work->gfx[1] = gTitleMenuSelectedEntryItalianFrames[work->choice[0]];
        break;
    case LANGUAGE_SPANISH:
        work->gfx[1] = gTitleMenuSelectedEntrySpanishFrames[work->choice[0]];
        break;
    case 5:
    case 6:
        break;
    }
#else
    work->gfx[1] = gTitleMenuSelectedEntryFrames[work->choice[0]];
#endif
    work->gfx[2] = gTitleMenuSelectedBarFrames[work->choice[0]];

    if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
        work->x = 120;
    } else {
        work->x = 0;
    }

    if (work->layout == TITLE_MENU_LAYOUT_BASIC) {
        TitleMenuDrawBasic(work);
    } else if (work->layout == TITLE_MENU_LAYOUT_FULL) {
        TitleMenuDrawFull(work);
    } else if (work->layout == TITLE_MENU_LAYOUT_NEW_GAME) {
        TitleMenuDrawNewGame(work);
    } else {
        TitleMenuDrawSingle(work);
    }

    TaskPoolDraw(&work->tasks);
}

void task_title_menu_3(TitleMenuWork* work) {
    s32 i;

    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);

    for (i = 0; i < 3; i++) {
        ReleaseObjTiles(work->tiles2[i]);
        ReleaseObjPalette(work->palette2[i]);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_title_lumichange_0(TitleLumiChangeWork* work) {
    if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            work->tiles = LoadObjTiles(gTitleRikuLumiTiles, sizeof(gTitleRikuLumiTiles));
            break;
        case LANGUAGE_FRENCH:
            work->tiles = LoadObjTiles(gTitleRikuLumiFrenchTiles, sizeof(gTitleRikuLumiFrenchTiles));
            break;
        case LANGUAGE_GERMAN:
            work->tiles = LoadObjTiles(gTitleRikuLumiGermanTiles, sizeof(gTitleRikuLumiGermanTiles));
            break;
        case LANGUAGE_ITALIAN:
            work->tiles = LoadObjTiles(gTitleRikuLumiItalianTiles, sizeof(gTitleRikuLumiItalianTiles));
            break;
        case LANGUAGE_SPANISH:
            work->tiles = LoadObjTiles(gTitleRikuLumiSpanishTiles, sizeof(gTitleRikuLumiSpanishTiles));
            break;
        case 5:
        case 6:
            break;
        }
#else
        work->tiles = LoadObjTiles(gTitleRikuLumiTiles, sizeof(gTitleRikuLumiTiles));
#endif
        work->palette = LoadObjPalette(gTitleRikuLumiPalette, sizeof(gTitleRikuLumiPalette));
    } else {
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            work->tiles = LoadObjTiles(gTitleSoraLumiTiles, sizeof(gTitleSoraLumiTiles));
            break;
        case LANGUAGE_FRENCH:
            work->tiles = LoadObjTiles(gTitleSoraLumiFrenchTiles, sizeof(gTitleSoraLumiFrenchTiles));
            break;
        case LANGUAGE_GERMAN:
            work->tiles = LoadObjTiles(gTitleSoraLumiGermanTiles, sizeof(gTitleSoraLumiGermanTiles));
            break;
        case LANGUAGE_ITALIAN:
            work->tiles = LoadObjTiles(gTitleSoraLumiItalianTiles, sizeof(gTitleSoraLumiItalianTiles));
            break;
        case LANGUAGE_SPANISH:
            work->tiles = LoadObjTiles(gTitleSoraLumiSpanishTiles, sizeof(gTitleSoraLumiSpanishTiles));
            break;
        case 5:
        case 6:
            break;
        }
#else
        work->tiles = LoadObjTiles(gTitleSoraLumiTiles, sizeof(gTitleSoraLumiTiles));
#endif
        work->palette = LoadObjPalette(gTitleSoraLumiPalette, sizeof(gTitleSoraLumiPalette));
    }
}

u8 task_title_lumichange_1(TitleLumiChangeWork* work) {
    s16 levels[3];
    s16 effect;
    u32 i;
    s32 j;

    effect = GetPaletteEffect();
    memcpy(levels, sTitleLumiLevels, sizeof(levels));
#ifdef PLATFORM_PC
    {
        /* the brightness switch made up for the GBA's unlit screen: only in GBA classic or with its debug option */
        int PortTitleBrightness(void);
        if (!PortTitleBrightness()) {
            if (effect != 0) {
                SetPaletteEffect(0);
                TitleLoadPaletteBuffer();
            }
            return 1;
        }
    }
#endif

    switch (GetKeysPressed() & (L_BUTTON | R_BUTTON)) {
    case R_BUTTON:
        for (i = 0; i < ARRAY_COUNT(levels); i++) {
            if (effect < levels[i]) {
                effect = levels[i];
                m4aSongNumStart(SONG_SYS_CANSEL);
                break;
            }
        }

        break;
    case L_BUTTON:
        for (j = ARRAY_COUNT(levels) - 1; j > -1; j--) {
            if (effect > levels[j]) {
                effect = levels[j];
                m4aSongNumStart(SONG_SYS_CANSEL);
                break;
            }
        }

        break;
    }

    if (GetKeysPressed() & (L_BUTTON | R_BUTTON)) {
        SetPaletteEffect(effect);
        TitleLoadPaletteBuffer();
    }

    return 1;
}

void task_title_lumichange_2(TitleLumiChangeWork* work) {
    s16 effect;
    void** frames;
    s16 x;

    effect = GetPaletteEffect();

#ifdef VERSION_EU
    {
        void** soraFrames = gTitleLumiSpritesEu[gLanguage];
        void** rikuFrames = gTitleLumiSpritesAltEu[gLanguage];

        frames = (gGameState.flags & GAME_FLAG_RIKU_TITLE) ? rikuFrames : soraFrames;
    }
#else
    frames = (gGameState.flags & GAME_FLAG_RIKU_TITLE) ? gTitleRikuLumiFrames : gTitleSoraLumiFrames;
#endif

    if (effect < 0) {
        work->gfx = frames[0];
    } else if (effect == 0) {
        work->gfx = frames[1];
    } else if (effect > 0) {
        work->gfx = frames[2];
    }

    x = (gGameState.flags & GAME_FLAG_RIKU_TITLE) ? DISPLAY_WIDTH : 0;
#ifdef PLATFORM_PC
    {
        int PortTitleBrightness(void);
        if (!PortTitleBrightness())
            return; /* its banner goes with the switch */
    }
#endif
    DrawSprite(x, 0x8F, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 100);
}

void task_title_lumichange_3(TitleLumiChangeWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

#ifdef VERSION_EU
void** gTitleLumiSpritesEu[5] = {
    gTitleSoraLumiFrames,
    gTitleSoraLumiFrenchFrames,
    gTitleSoraLumiGermanFrames,
    gTitleSoraLumiItalianFrames,
    gTitleSoraLumiSpanishFrames,
};

void** gTitleLumiSpritesAltEu[5] = {
    gTitleRikuLumiFrames,
    gTitleRikuLumiFrenchFrames,
    gTitleRikuLumiGermanFrames,
    gTitleRikuLumiItalianFrames,
    gTitleRikuLumiSpanishFrames,
};
#endif

TaskDesc gTaskDescTitleLumichange = {
    "task_title_lumichange",
    (TaskInitFunc)task_title_lumichange_0,
    (TaskUpdateFunc)task_title_lumichange_1,
    (TaskDrawFunc)task_title_lumichange_2,
    (TaskDestroyFunc)task_title_lumichange_3,
    sizeof(TitleLumiChangeWork),
};
