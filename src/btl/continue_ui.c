/**
 * continue_ui.c
 * Continue Screen Menu
 */

#include "system_state.h"
#include "anim.h"
#include "types.h"
#include "bg_animation_data.h"
#include "engine_math.h"
#include "display.h"
#include "fade.h"
#include "obj_api.h"
#include "pallet.h"
#include "key.h"
#include "m4a_song.h"
#include "sprites_continue.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "songs.h"
#include "continue_types.h"
#include "obj.h"
#include "sprite_palettes.h"
#include <stddef.h>
#include "taskpool.h"
#include "default_bg_map.h"

static const s32 sContinueCursorY[2] = {
    0x4000, 0x5600,
};

#ifdef VERSION_EU
static void* sContinueLanguageBgTiles[5] = {
    gContinueBgTiles,
    gContinueBgFrenchTiles,
    gContinueBgGermanTiles,
    gContinueBgItalianTiles,
    gContinueBgSpanishTiles,
};
#endif

void LoadContinueCursorPalette(s32 cursor) {
    switch (cursor) {
    case CONTINUE_OPTION_CONTINUE:
        LoadBgPalette(0, gContinueCursor0BgPalettes, sizeof(gContinueCursor0BgPalettes));
        break;
    case CONTINUE_OPTION_RETURN_TO_TITLE:
        LoadBgPalette(0, gContinueCursor1BgPalettes, sizeof(gContinueCursor1BgPalettes));
        break;
    }
}
#ifdef VERSION_JP
#define MSG_CONT_BG_TILES 0x1A40
#define MSG_CONT_X 0xA400
#else
#define MSG_CONT_BG_TILES 0x1AA0
#define MSG_CONT_X 0xBC00
#endif

void ContinueSora_0(ContinueWork* work) {
    u8 i;

    SetBgMode1();
    work->cursor = CONTINUE_OPTION_CONTINUE;
    SetBackdropColor(0, 0, 0);
    SetupBg(0, 0, 31, 0);
    SetupBg(2, 2, 28, 10);
    SetBgPriority(2, 0);
    SetBgPriority(0, 1);
    SetBgPriority(1, 2);
#ifdef VERSION_EU
    LoadBgTilesLz77(0, sContinueLanguageBgTiles[gLanguage]);
    LoadBgMapLz77(0, gContinueBgMap);
#else
    LoadBgTiles(0, gContinueBgTiles, MSG_CONT_BG_TILES);
    LoadBgMap(0, gContinueBgMap, sizeof(gContinueBgMap));
#endif
    BgAnimInit(2, BGCNT_AFF512x512, BGCNT_256COLOR);
    BgAnimStart(&gBgAnimDefCharaDefeatEnd, 120, 46);
    BgAnimSetLoopStartFrame(0);
    work->tiles3 = LoadObjTiles(gContinueLineTiles, sizeof(gContinueLineTiles));
    work->palette3 = LoadObjPalette(gContinueLinePalette, sizeof(gContinueLinePalette));
    LoadContinueCursorPalette(work->cursor);
    work->tiles = AllocObjTiles(512, NULL);
    PushPaletteEffect(0);
    work->palette = LoadObjPalette(gContinueCursorPalettes, sizeof(gContinueCursorPalettes));
    PopPaletteEffect();
    SetObjTileSource(work->tiles, gContinueCursorTiles);
    AnimInit(&work->anim, gContinueCursorAnims, gContinueCursorFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->tiles2 = AllocObjTiles(1024, NULL);
    work->palette2 = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    SetObjTileSource(work->tiles2, gSoraContinueTiles);
    AnimInit(&work->anim2, gSoraContinueAnims, gSoraContinueFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->x2 = -2048;
    work->y2 = 0xA000;
    work->steps = 16;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
    case LANGUAGE_FRENCH:
    case LANGUAGE_SPANISH:
    case 5:
    case 6:
        work->x = 0xBC00;
        break;
    case LANGUAGE_GERMAN:
    case LANGUAGE_ITALIAN:
        work->x = 0xC000;
        break;
    }
#else
    work->x = MSG_CONT_X;
#endif
    work->y = 0x4000;
    work->unk_64 = 0;
    work->blendAlpha = 0;
    FadeStartIn(FADE_MODE_WHITE, 24);

    for (i = 0; i < 5; i++) {
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + i, FALSE);
    }

    work->blendAlpha = 0x1000;
    work->state = CONTINUE_STATE_FADE_IN;
}

void ContinueRiku_0(ContinueWork* work) {
    u8 i;

    SetBgMode1();
    work->cursor = CONTINUE_OPTION_CONTINUE;
    SetBackdropColor(0, 0, 0);
    SetupBg(0, 0, 31, 0);
    SetupBg(2, 2, 28, 10);
    SetBgPriority(2, 0);
    SetBgPriority(0, 1);
    SetBgPriority(1, 2);
#ifdef VERSION_EU
    LoadBgTilesLz77(0, sContinueLanguageBgTiles[gLanguage]);
    LoadBgMapLz77(0, gContinueBgMap);
#else
    LoadBgTiles(0, gContinueBgTiles, MSG_CONT_BG_TILES);
    LoadBgMap(0, gContinueBgMap, sizeof(gContinueBgMap));
#endif
    BgAnimInit(2, BGCNT_AFF512x512, BGCNT_256COLOR);
    BgAnimStart(&gBgAnimDefCharaDefeatEnd, 120, 46);
    BgAnimSetLoopStartFrame(0);
    work->tiles3 = LoadObjTiles(gContinueLineTiles, sizeof(gContinueLineTiles));
    work->palette3 = LoadObjPalette(gContinueLinePalette, sizeof(gContinueLinePalette));
    LoadContinueCursorPalette(work->cursor);
    work->tiles = AllocObjTiles(512, NULL);
    PushPaletteEffect(0);
    work->palette = LoadObjPalette(gContinueCursorPalettes, sizeof(gContinueCursorPalettes));
    PopPaletteEffect();
    SetObjTileSource(work->tiles, gContinueCursorTiles);
    AnimInit(&work->anim, gContinueCursorAnims, gContinueCursorFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->tiles2 = AllocObjTiles(1024, NULL);
    work->palette2 = LoadObjPalette(gRikuPalette, sizeof(gRikuPalette));
    SetObjTileSource(work->tiles2, gRikuContinueTiles);
    AnimInit(&work->anim2, gRikuContinueAnims, gRikuContinueFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->x2 = -2048;
    work->y2 = 0xA000;
    work->steps = 16;

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
    case LANGUAGE_FRENCH:
    case LANGUAGE_SPANISH:
    case 5:
    case 6:
        work->x = 0xBC00;
        break;
    case LANGUAGE_GERMAN:
    case LANGUAGE_ITALIAN:
        work->x = 0xC000;
        break;
    }
#else
    work->x = MSG_CONT_X;
#endif
    work->y = 0x4000;
    work->unk_64 = 0;
    work->blendAlpha = 0;
    FadeStartIn(FADE_MODE_WHITE, 24);

    for (i = 0; i < 5; i++) {
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + i, FALSE);
    }

    work->blendAlpha = 0x1000;
    work->state = CONTINUE_STATE_FADE_IN;
}

static s32 Continue_1(ContinueWork* work) {
    const s32* cursorY;

    BgAnimUpdate();
    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
    gBldCnt = (BLDCNT_TGT1_BG2 | BLDCNT_TGT1_OBJ | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG3);
    gBldAlpha = work->blendAlpha;

    if (work->state == CONTINUE_STATE_FADE_IN) {
        if (!FadeIsActive()) {
            work->state = CONTINUE_STATE_SELECT;
        }
    }

    if (work->state == CONTINUE_STATE_SELECT) {
        if (work->steps > 0) {
            ApproachValue(&work->x2, 0, work->steps);
            ApproachValue(&work->y2, 0x9800, work->steps);
            work->steps--;
        }

        if (work->blendAlpha < 0x1010) {
            work->blendAlpha++;
        } else {
            work->blendAlpha = 0x1010;
        }

        if ((GetKeysPressed() & DPAD_UP) != 0) {
            if (work->cursor == CONTINUE_OPTION_RETURN_TO_TITLE) {
                work->cursor = CONTINUE_OPTION_CONTINUE;
                m4aSongNumStart(SONG_SYS_CLICK);
            }
        }

        if ((GetKeysPressed() & DPAD_DOWN) != 0) {
            if (work->cursor == CONTINUE_OPTION_CONTINUE) {
                work->cursor = CONTINUE_OPTION_RETURN_TO_TITLE;
                m4aSongNumStart(SONG_SYS_CLICK);
            }
        }

        if ((GetKeysHeld() & A_BUTTON) != 0) {
            switch (work->cursor) {
            case CONTINUE_OPTION_CONTINUE:
                FadeStartOut(FADE_MODE_BLACK, 96);
                break;
            case CONTINUE_OPTION_RETURN_TO_TITLE:
                FadeStartOut(FADE_MODE_BLACK, 96);
                break;
            }

            m4aSongNumStart(SONG_SYS_KETTEI);
            work->state = CONTINUE_STATE_FADE_OUT;
            work->steps = 16;
        }
    }

    if (work->state == CONTINUE_STATE_FADE_OUT) {
        if (work->steps > 0) {
            ApproachValue(&work->x2, -2048, work->steps);
            ApproachValue(&work->y2, 0xA000, work->steps);
            work->steps--;
        }

        if (work->blendAlpha > 0x1000) {
            work->blendAlpha--;
        } else {
            work->blendAlpha = 0x1000;
        }

        if (!FadeIsActive()) {
            DisableBg(0);
            DisableBg(2);
            LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
            LoadBgMap(2, gDefaultBgMap, sizeof(gDefaultBgMap));
            work->state = CONTINUE_STATE_DONE;
        }
    }

    LoadContinueCursorPalette(work->cursor);
    cursorY = sContinueCursorY;
    work->y += (cursorY[work->cursor] - work->y) >> 3;
    work->unk_64 += 4;
    // @bug No return value (the caller reads it).
#ifdef PLATFORM_PC
    return 1; /* x86 has no leftover r0: keep the task running, as on the GBA */
#endif
}

static void Continue_2(ContinueWork* work) {
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, SPRITE_FLAG_BLEND, 100);
    DrawSprite(120, 120, work->gfx2, work->tiles2, work->palette2, NULL, 0, 100);
}

static void Continue_3(ContinueWork* work) {
    DisableBg(0);
    DisableBg(2);
    LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(2, gDefaultBgMap, sizeof(gDefaultBgMap));
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjPalette(work->palette3);
    gBldCnt = 0;
}

TaskDesc gTaskDescContinueSora = {
    "Continue",
    (TaskInitFunc)ContinueSora_0,
    (TaskUpdateFunc)Continue_1,
    (TaskDrawFunc)Continue_2,
    (TaskDestroyFunc)Continue_3,
    sizeof(ContinueWork),
};

TaskDesc gTaskDescContinueRiku = {
    "Continue",
    (TaskInitFunc)ContinueRiku_0,
    (TaskUpdateFunc)Continue_1,
    (TaskDrawFunc)Continue_2,
    (TaskDestroyFunc)Continue_3,
    sizeof(ContinueWork),
};
