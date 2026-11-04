/**
 * card_msgwin.c
 * Card Message Window
 */

#include "macros.h"
#include "msg_localized_data.h"
#include "registration_data.h"
#include "system_state.h"
#include "msg_api.h"
#include "m4a_song.h"
#include "text.h"
#include "monsgage.h"
#include "obj_api.h"
#include "display.h"
#include "engine_math.h"
#include "anim.h"
#include "obj.h"
#include "text_types.h"
#include "taskpool.h"
#include "key.h"
#include "card.h"
#include "sprites_deck_menu.h"
#include "sprites_evt.h"
#include "sprites_msg.h"
#include "sprites_card.h"
#include "gba/keys.h"
#include "songs.h"
#include "card_message_text.h"
#include "event_text.h"
#include "jiminy_data.h"
#include "common_text.h"
#include "card_api.h"
#include "card_message_data.h"
#include "msg_portrait_data.h"
#include "sprite_palettes.h"
#include "types.h"
#include "engine.h"
#include "gba/macro.h"
#include <stddef.h>
#include "card_msgwin.h"
#include "gba/defines.h"
#include "msg_types.h"

static CardMsgWinWork* sActiveCardMsgwin;

u8 gMessageWindowOpen EWRAM_COMMON(4);

u8 gMessageWindowAnswerYes EWRAM_COMMON(4);

static void msgwin_0(CardMsgWinWork* work, CardMessageArgs* args) {
    CpuFill32(0, work, sizeof(CardMsgWinWork));
    work->glyphPaletteIndex = InitCardMsgGlyphSprites(0, FALSE);
    work->args = *args;
    work->messageDef = &gCardMessageDefs[work->args.messageId];
    work->tiles3 = NULL;
    work->palette = NULL;
    work->tiles4 = NULL;
    work->palette2 = NULL;
    work->tiles = NULL;
    work->palette3 = NULL;
    work->tiles2 = NULL;
    work->palette4 = NULL;
    work->textPalette = NULL;
    work->x = gMsgwinClosedScrollX[work->messageDef->positionIndex];
    work->faceX = 0;
    work->faceY = 0;
    work->cursorX = 0;
    work->cursorY = 0;
    work->gfx = NULL;
    work->gfx2 = NULL;
    work->gfx3 = NULL;
    work->nextText = NULL;
    work->glyphPaletteIndex = 0;
    work->closeTimer = 0;
    work->steps = 8;
    work->shownChars = 0;
    work->charTimer = 0;
    work->charCount = 0;
    work->choice = 0;
    work->cursorSteps = 0;
    work->textSlotCounts[0] = 0;
    work->textSlotCounts[1] = 0;
    work->waitIconVisible = TRUE;
    work->textVisible = FALSE;
    work->unk_14A[0] = 0;
    work->unk_14A[1] = 0;
    work->unk_14C = 0;
    work->faceFlip = FALSE;
    work->messagePending = FALSE;
    work->keepOpen = TRUE;

    switch (work->messageDef->positionIndex) {
    case MSG_WIN_POSITION_TOP_FACE_RIGHT:
    case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        work->faceFlip = TRUE;
        break;
    case MSG_WIN_POSITION_TOP_FACE_LEFT:
    case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
        work->faceFlip = FALSE;
        break;
    }

    gMessageWindowOpen = TRUE;
    gMessageWindowAnswerYes = FALSE;
    SetBgScroll(work->args.bg, 0, 0);

    switch (work->args.mode) {
    case CARD_MESSAGE_MODE_BG_WINDOW:
    case CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT:
        SetBgPriority(work->args.bg, 0);
        break;
    case CARD_MESSAGE_MODE_SPRITE_WINDOW:
    case CARD_MESSAGE_MODE_SPRITE_WINDOW_PERSISTENT:
        break;
    }

    sActiveCardMsgwin = work;
}

u8 UpdateCardMsgwinOpen(CardMsgWinWork* work, void* task) {
    const MsgFaceAnim* faceAnims;

    ApproachValue(&work->x, gMsgwinOpenScrollX[work->messageDef->positionIndex], work->steps);
    ApproachValue(&work->faceX, gMsgfaceShownX[work->messageDef->positionIndex], work->steps);
    ScrollBgMapTo(work->args.bg, work->x, 0);
    work->gfx = AnimUpdate(&work->anim);

    if (work->steps != 0) {
        work->steps--;
    } else {
        faceAnims = gMsgFaceAnims[work->messageDef->portraitId];

        if (faceAnims[work->messageDef->expressionId].animCount > 1) {
            AnimStart(&work->anim, 1, faceAnims[work->messageDef->expressionId].animFlags);
        }

        switch (work->args.mode) {
        case CARD_MESSAGE_MODE_BG_WINDOW:
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinTyping);
            break;
        case CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT:
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinTypingPersistent);
            break;
        }
    }

    return 1;
}

static u8 msgwin_1(CardMsgWinWork* work, void* task) {
    LoadBgTiles(work->args.bg, gMsgwinTiles, sizeof(gMsgwinTiles));
    LoadBgPalette(work->args.bg, gMsgwinPalette, sizeof(gMsgwinPalette));
    SetBgMapBlocks(work->args.bg, gMsgwinMapBlocks[work->messageDef->positionIndex], 2, 1);
    ScrollBgMapTo(work->args.bg, work->x, 0);
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinLoadText);
    return 1;
}

u8 UpdateCardMsgwinLoadText(CardMsgWinWork* work, void* task) {
#ifdef VERSION_JP
    work->charCount = LayoutCardMsgGlyphsPageSjis(gMsgwinTextX[work->messageDef->positionIndex],
                               gMsgwinTextY[work->messageDef->positionIndex],
                               CARD_MSG_TEXT(work),
                               &work->nextText);
#else
    TextChar** nextText;

    nextText = &work->nextText;

    if (*nextText != NULL) {
        work->charCount = LayoutCardMsgGlyphsPage(gMsgwinTextX[work->messageDef->positionIndex],
                                   gMsgwinTextY[work->messageDef->positionIndex] - 0x200,
                                   *nextText, nextText);
    } else {
        work->charCount = LayoutCardMsgGlyphsPage(gMsgwinTextX[work->messageDef->positionIndex],
                                   gMsgwinTextY[work->messageDef->positionIndex] - 0x200,
                                   CARD_MSG_TEXT(work), nextText);
    }
#endif

    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinLoadFace);

    return 1;
}

u8 UpdateCardMsgwinLoadFace(CardMsgWinWork* work, void* task) {
    CardMessageDef* messageDef;
    const MsgFaceAnim* faceAnims;

    work->textVisible = TRUE;
    messageDef = work->messageDef;

    if (messageDef->portraitId != PORTRAIT_NONE) {
        faceAnims = gMsgFaceAnims[messageDef->portraitId];
        work->tiles3 = AllocObjTiles(0xD80, NULL);
        work->palette = LoadObjPalette(faceAnims[work->messageDef->expressionId].palette, 32);
        SetObjTileSource(work->tiles3, faceAnims[work->messageDef->expressionId].tiles);
        AnimInit(&work->anim, faceAnims[work->messageDef->expressionId].anims, faceAnims[work->messageDef->expressionId].gfxTable);
        AnimStart(&work->anim, 0, faceAnims[work->messageDef->expressionId].animFlags);
        work->gfx = AnimGetGfx(&work->anim);
        work->faceX = gMsgfaceHiddenX[work->messageDef->positionIndex];
        work->faceY = gMsgfaceY[work->messageDef->positionIndex];
    } else {
        work->tiles3 = NULL;
        work->palette = NULL;
    }

    SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinOpen);
    return 1;
}

#ifdef PLATFORM_PC
/* Same layout as the event message windows: dock to the portrait's edge. */
static int CardMsgwinDockShift(CardMsgWinWork* work) {
    switch (work->messageDef->positionIndex) {
    case MSG_WIN_POSITION_TOP_FACE_RIGHT:
    case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        return PortWideMarginX();
    case MSG_WIN_POSITION_TOP_FACE_LEFT:
    case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
        return -PortWideMarginX();
    }
    return 0;
}

static void msgwin_2_Body(CardMsgWinWork* work);

static void msgwin_2(CardMsgWinWork* work) {
    gPortSpriteShiftX = CardMsgwinDockShift(work);
    PortDockBg(work->args.bg, gPortSpriteShiftX);
    msgwin_2_Body(work);
    gPortSpriteShiftX = 0;
}

static void msgwin_2_Body(CardMsgWinWork* work) {
#else
static void msgwin_2(CardMsgWinWork* work) {
#endif
    void** frames;

    if (work->textVisible) {
        DrawCardMsgGlyphs(work->shownChars);
    }

    if (work->tiles3 != NULL) {
        if (work->faceFlip) {
            DrawSprite(work->faceX >> 8, work->faceY >> 8, work->gfx, work->tiles3, work->palette, NULL, SPRITE_FLAG_HFLIP, 0);
        } else {
            DrawSprite(work->faceX >> 8, work->faceY >> 8, work->gfx, work->tiles3, work->palette, NULL, 0, 0);
        }

        if (work->tiles4 != NULL && work->waitIconVisible) {
            DrawSprite(gMsgwaitIconPos[work->messageDef->positionIndex][0] >> 8, gMsgwaitIconPos[work->messageDef->positionIndex][1] >> 8,
                       work->gfx2, work->tiles4, work->palette2, NULL, 0, 10);
        }
    }

    if (work->tiles != NULL) {
        DrawSprite(work->cursorX >> 8, work->cursorY >> 8, work->gfx3, work->tiles, work->palette3, NULL, SPRITE_FLAG_HFLIP, 9);
    }

    if (work->tiles2 != NULL) {
#ifdef PLATFORM_PC
        gPortSpriteShiftX = 0; /* the yes/no box is centred */
#endif
        frames = gMsgBoxFrames;
        DrawSprite(120, 80, frames[1], work->tiles2, work->palette4, NULL, 0, 10);
        DrawTextSlots((DISPLAY_WIDTH - work->textSlotCounts[0] * 10) >> 1, 67, work->textSlots, work->textPalette, 0, work->textSlotCounts[0]);
        DrawTextSlots((DISPLAY_WIDTH - work->textSlotCounts[1] * 10) >> 1, 82, work->textSlots2, work->textPalette, 0, work->textSlotCounts[1]);
    }
}

static void msgwin_3(CardMsgWinWork* work) {
#ifdef PLATFORM_PC
    /* the window is gone: its layer is no longer docked beside the portrait
       (a save window drawn on it later would sit shifted by the margin) */
    PortSetBgShiftX(work->args.bg, 0);
#endif
    FreeCardMsgGlyphSprites();

    if (work->tiles3 != NULL) {
        ReleaseObjTiles(work->tiles3);
    }

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
    }

    if (work->tiles4 != NULL) {
        ReleaseObjTiles(work->tiles4);
    }

    if (work->palette2 != NULL) {
        ReleaseObjPalette(work->palette2);
    }

    if (work->tiles != NULL) {
        ReleaseObjTiles(work->tiles);
    }

    if (work->palette3 != NULL) {
        ReleaseObjPalette(work->palette3);
    }

    if (work->tiles2 != NULL) {
        ReleaseObjTiles(work->tiles2);
    }

    if (work->palette4 != NULL) {
        ReleaseObjPalette(work->palette4);
    }

    if (work->textPalette != NULL) {
        ReleaseObjPalette(work->textPalette);
    }

    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    gMessageWindowOpen = FALSE;
    sActiveCardMsgwin = NULL;
}

u8 UpdateCardMsgwinTyping(CardMsgWinWork* work, void* task) {
    CardMessageDef* messageDef;
    const MsgFaceAnim* faceAnims;

    work->gfx = AnimUpdate(&work->anim);

    if (GetKeysPressed() & A_BUTTON) {
        work->shownChars = work->charCount;
    }

    work->charTimer++;
    messageDef = work->messageDef;

    if (work->charTimer >= messageDef->charDelay) {
        if (work->shownChars < work->charCount) {
            work->shownChars++;
            m4aSongNumStart(SONG_SYS_MESSAGE);
        } else {
            faceAnims = gMsgFaceAnims[messageDef->portraitId];
            AnimStart(&work->anim, 0, faceAnims[messageDef->expressionId].animFlags);

            if (work->tiles4 == NULL) {
                work->tiles4 = AllocObjTiles(0x40, NULL);
                work->palette2 = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
                SetObjTileSource(work->tiles4, gFEventTiles);
                AnimInit(&work->anim2, gFEventAnims, gFEventFrames);
                AnimStart(&work->anim2, 2, ANIM_FLAG_LOOP);
                work->gfx2 = AnimGetGfx(&work->anim2);
            }

            work->waitIconVisible = TRUE;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinWaitInput);
        }

        work->charTimer = 0;
    }

    return 1;
}

u8 UpdateCardMsgwinWaitInput(CardMsgWinWork* work, void* task) {
    const MsgFaceAnim* faceAnims;
    u16* pal;

    work->gfx2 = AnimUpdate(&work->anim2);
    work->gfx = AnimUpdate(&work->anim);

    if (GetKeysPressed() & A_BUTTON) {
        m4aSongNumStart(SONG_SYS_KETTEI);

        if (work->nextText != NULL) {
#ifdef VERSION_JP
            *((u8*)work + offsetof(CardMsgWinWork, charCount)) = LayoutCardMsgGlyphsPageSjis(gMsgwinTextX[work->messageDef->positionIndex],
                                      gMsgwinTextY[work->messageDef->positionIndex],
                                      work->nextText, &work->nextText);
#else
            *((u8*)work + offsetof(CardMsgWinWork, charCount)) = LayoutCardMsgGlyphsPage(gMsgwinTextX[work->messageDef->positionIndex],
                                      gMsgwinTextY[work->messageDef->positionIndex] - 0x200,
                                      work->nextText, &work->nextText);
#endif
            work->charTimer = 0;
            work->shownChars = 0;
            faceAnims = gMsgFaceAnims[work->messageDef->portraitId];

            if (faceAnims[work->messageDef->expressionId].animCount > 1) {
                AnimStart(&work->anim, 1, faceAnims[work->messageDef->expressionId].animFlags);
            }

            work->waitIconVisible = FALSE;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinTyping);
        } else if (!(work->messageDef->flags & CARD_MSG_FLAG_CHOICE_AT_END)) {
            AnimStart(&work->anim2, 3, ANIM_FLAG_LOOP);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinClose);
            work->closeTimer = 0;
            work->steps = 8;
        } else {
            ReleaseObjTiles(work->tiles4);
            ReleaseObjPalette(work->palette2);
            work->tiles4 = NULL;
            work->palette2 = NULL;
            work->tiles = AllocObjTiles(0x120, NULL);
            pal = gDialogBoxPalette;
            work->palette3 = LoadObjPalette(pal, 32);
            LoadObjPaletteBank(BIOS_PTR(work->palette3)->index, pal);
            SetObjTileSource(work->tiles, gHandCursorTiles);
            AnimInit(&work->anim3, gHandCursorAnims, gHandCursorFrames);
            AnimStart(&work->anim3, 2, ANIM_FLAG_LOOP);
            work->gfx3 = AnimGetGfx(&work->anim3);
            work->tiles2 = LoadObjTiles(gMsgBoxTiles, sizeof(gMsgBoxTiles));
            work->palette4 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
            work->choice = 0;
            work->cursorX = 0x5800;
            work->cursorY = gMsgwaitYesnoCursorY[work->choice];
            work->textSlotCounts[0] = LoadTextSlots(LOCALIZED_STRING(gYesChoiceText), work->textSlots);
            work->textSlotCounts[1] = LoadTextSlots(LOCALIZED_STRING(gNoChoiceText), work->textSlots2);
            work->textPalette = LoadTextPalette(1);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinChoice);
        }
    }

    return 1;
}

u8 UpdateCardMsgwinClose(CardMsgWinWork* work) {
    if (work->tiles4 != NULL) {
        work->gfx2 = AnimUpdate(&work->anim2);
    }

    work->gfx = AnimUpdate(&work->anim);
    work->closeTimer++;

    if (work->closeTimer > 15) {
        work->textVisible = FALSE;
        work->waitIconVisible = FALSE;
        ApproachValue(&work->x, gMsgwinClosedScrollX[work->messageDef->positionIndex], work->steps);
        ApproachValue(&work->faceX, gMsgfaceHiddenX[work->messageDef->positionIndex], work->steps);
        ScrollBgMapTo(work->args.bg, work->x, 0);

        if (work->steps == 0) {
            return 0;
        }

        work->steps--;
    }

    return 1;
}

u8 UpdateCardMsgwinChoice(CardMsgWinWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim);
    work->gfx3 = AnimUpdate(&work->anim3);

    switch (GetKeysRepeat()) {
    case DPAD_UP:
    case DPAD_DOWN:
        work->choice ^= 1;
        work->cursorSteps = 4;
        break;
    case A_BUTTON:
    case START_BUTTON:
        m4aSongNumStart(SONG_SYS_KETTEI);

        if (work->choice == 0) {
            gMessageWindowAnswerYes = TRUE;
        } else {
            gMessageWindowAnswerYes = FALSE;
        }

        SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinClose);
        break;
    }

    if (work->cursorSteps != 0) {
        ApproachValue(&work->cursorY, gMsgwaitYesnoCursorY[work->choice], work->cursorSteps);
        work->cursorSteps--;
    }

    return 1;
}

u8 UpdateCardMsgwinTypingPersistent(CardMsgWinWork* work, void* task) {
    const MsgFaceAnim* faceAnims;

    work->gfx = AnimUpdate(&work->anim);

    if (GetKeysPressed() & A_BUTTON) {
        work->shownChars = work->charCount;
    }

    work->charTimer++;

    if (work->charTimer >= work->messageDef->charDelay) {
        if (work->shownChars < work->charCount) {
            work->shownChars++;
            m4aSongNumStart(SONG_SYS_MESSAGE);
        } else {
            faceAnims = gMsgFaceAnims[work->messageDef->portraitId];
            AnimStart(&work->anim, 0, faceAnims[work->messageDef->expressionId].animFlags);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinPersistent);
        }

        work->charTimer = 0;
    }

    return 1;
}

u8 UpdateCardMsgwinPersistent(CardMsgWinWork* work, void* task) {
#ifndef VERSION_JP
    TextChar** nextText;
#endif
    work->gfx = AnimUpdate(&work->anim);

    if (!work->keepOpen) {
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateCardMsgwinClose);
    } else if (work->messagePending == TRUE) {
        work->messagePending = FALSE;
        work->messageDef = &gCardMessageDefs[work->args.messageId];
#ifdef VERSION_JP
        work->charCount = LayoutCardMsgGlyphsPageSjis(gMsgwinTextX[work->messageDef->positionIndex],
                                   gMsgwinTextY[work->messageDef->positionIndex],
                                   CARD_MSG_TEXT(work), &work->nextText);
#else
        nextText = &work->nextText;

        if (*nextText != NULL) {
            work->charCount = LayoutCardMsgGlyphsPage(gMsgwinTextX[work->messageDef->positionIndex],
                                       gMsgwinTextY[work->messageDef->positionIndex] - 0x200, *nextText, nextText);
        } else {
            work->charCount = LayoutCardMsgGlyphsPage(gMsgwinTextX[work->messageDef->positionIndex],
                                       gMsgwinTextY[work->messageDef->positionIndex] - 0x200,
                                       CARD_MSG_TEXT(work), nextText);
        }
#endif

        work->shownChars = work->charCount;
    }

    return 1;
}

s32 ReplaceCardMsgwinMessage(CardMessageArgs* src) {
    if (sActiveCardMsgwin != NULL) {
        sActiveCardMsgwin->args = *src;
        sActiveCardMsgwin->messagePending = TRUE;

        return TRUE;
    }

    return FALSE;
}

void CreateCardMessageTask(void* pool, u32 bg, u16 message) {
    CardMessageArgs args;

    args.bg = bg;
    args.messageId = message;
    args.mode = CARD_MESSAGE_MODE_BG_WINDOW;

    if (gCardMessageDefs[message].portraitId == PORTRAIT_NONE) {
        if (gCardMessageDefs[message].flags & CARD_MSG_FLAG_CHOICE_WINDOW) {
            TaskCreate(pool, &gTaskDescSysmsgwinChoice, &args);
        } else {
            TaskCreate(pool, &gTaskDescSysmsgwin, &args);
        }
    } else {
        TaskCreate(pool, &gTaskDescCardMsgwin, &args);
    }
}

void CreateSysmsgwinTask(void* pool, u16 message) {
    CardMessageArgs args;

    args.bg = 0;
    args.messageId = message;
    args.mode = CARD_MESSAGE_MODE_SPRITE_WINDOW;

    if (gCardMessageDefs[message].flags & CARD_MSG_FLAG_CHOICE_WINDOW) {
        TaskCreate(pool, &gTaskDescSysmsgwinChoice, &args);
    } else {
        TaskCreate(pool, &gTaskDescSysmsgwin, &args);
    }
}

void CreatePersistentSysmsgwinTask(void* pool, u16 message) {
    CardMessageArgs args;

    IsMessageWindowOpen();
    args.bg = 0;
    args.messageId = message;
    args.mode = CARD_MESSAGE_MODE_SPRITE_WINDOW_PERSISTENT;
    TaskCreate(pool, &gTaskDescSysmsgwin, &args);
}

void ShowPersistentCardMessage(void* pool, u32 bg, u16 message) {
    CardMessageArgs args;

    args.bg = bg;
    args.messageId = message;
    args.mode = CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT;

    if (IsMessageWindowOpen()) {
        if (!(u8)ReplaceCardMsgwinMessage(&args)) {
            ReplaceSysmsgwinMessage(&args);
        }
    } else if (gCardMessageDefs[message].portraitId == PORTRAIT_NONE) {
        TaskCreate(pool, &gTaskDescSysmsgwin, &args);
    } else {
        TaskCreate(pool, &gTaskDescCardMsgwin, &args);
    }
}

void ResetMessageWindowFlags() {
    gMessageWindowOpen = FALSE;
    gMessageWindowAnswerYes = FALSE;
}

u8 IsMessageWindowOpen() {
    return gMessageWindowOpen;
}

u8 IsMessageWindowAnswerYes() {
    return gMessageWindowAnswerYes;
}

u8 CloseMessageWindow() {
    if (sActiveCardMsgwin != NULL) {
        sActiveCardMsgwin->keepOpen = FALSE;
        return TRUE;
    }

    return CloseSysmsgwin();
}

CardMessageDef gCardMessageDefs[] = {
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText00),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText01),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 4, 3,
        LOCALIZED(gDonaldTalkText02),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gDonaldTalkText03),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 3, 3,
        LOCALIZED(gDonaldTalkText04),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gDonaldTalkText05),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText06),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gDonaldTalkText07),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gDonaldTalkText08),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 3, 3,
        LOCALIZED(gDonaldTalkText09),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 3, 3,
        LOCALIZED(gDonaldTalkText10),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText11),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText12),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText13),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 3, 3,
        LOCALIZED(gDonaldTalkText14),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 4, 3,
        LOCALIZED(gDonaldTalkText15),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 2, 3,
        LOCALIZED(gDonaldTalkText16),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText17),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gDonaldTalkText18),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gDonaldTalkText19),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText20),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 2, 3,
        LOCALIZED(gDonaldTalkText21),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText22),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldTalkText23),
        0,
    },
    {
        PORTRAIT_DONALD, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gDonaldExitHallTalkText),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText00),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText01),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 3, 3,
        LOCALIZED(gGoofyTalkText02),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText03),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText04),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText05),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText06),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText07),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText08),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText09),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText10),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText11),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 4, 3,
        LOCALIZED(gGoofyTalkText12),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 3, 3,
        LOCALIZED(gGoofyTalkText13),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText14),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText15),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText16),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText17),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText18),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 4, 3,
        LOCALIZED(gGoofyTalkText19),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 3, 3,
        LOCALIZED(gGoofyTalkText20),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 2, 3,
        LOCALIZED(gGoofyTalkText21),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gGoofyTalkText22),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyTalkText23),
        0,
    },
    {
        PORTRAIT_GOOFY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gGoofyExitHallTalkText),
        0,
    },
    {
        PORTRAIT_NAMINE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 4, 3,
        LOCALIZED(gNamineTalkText0),
        0,
    },
    {
        PORTRAIT_NAMINE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gNamineTalkText1),
        0,
    },
    {
        PORTRAIT_RIKU_REPLICA, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gRikuReplicaTalkText),
        0,
    },
    {
        PORTRAIT_WINNIE_THE_POOH, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
#if defined(VERSION_EU)
        &gPoohTalkTextByLanguage,
#elif defined(VERSION_JP)
        gPoohTalkText,
#elif defined(VERSION_US)
        gEventTextUs_0900868C,
#endif
        CARD_MSG_FLAG_CHOICE_AT_END,
    },
    {
        PORTRAIT_PIGLET, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gPigletTalkText),
        0,
    },
    {
        PORTRAIT_OWL, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gOwlTalkText),
        0,
    },
    {
        PORTRAIT_EEYORE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gEeyoreTalkText0),
        0,
    },
    {
        PORTRAIT_EEYORE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gEeyoreTalkText1),
        0,
    },
    {
        PORTRAIT_ROO, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gRooTalkText),
        0,
    },
    {
        PORTRAIT_RABBIT, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gRabbitTalkText0),
        0,
    },
    {
        PORTRAIT_RABBIT, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gRabbitTalkText1),
        0,
    },
    {
        PORTRAIT_MICKEY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 4, 3,
        LOCALIZED(gMickeyTalkText0),
        0,
    },
    {
        PORTRAIT_MICKEY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 1, 3,
        LOCALIZED(gMickeyTalkText1),
        0,
    },
    {
        PORTRAIT_MICKEY, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMickeyTalkText2),
        0,
    },
    {
        PORTRAIT_MOOGLE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMsTopShopOptionText),
        0,
    },
    {
        PORTRAIT_MOOGLE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMsTopChargeOptionText),
        0,
    },
    {
        PORTRAIT_MOOGLE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMsTopFreePackText),
        0,
    },
    {
        PORTRAIT_MOOGLE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
#if defined(VERSION_EU)
        &gMsTopChargeOptionTextByLanguage,
#elif defined(VERSION_JP)
        gMsTopNothingToSellText,
#elif defined(VERSION_US)
        gCardMessageNoText,
#endif
        0,
    },
    {
        PORTRAIT_MOOGLE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
#if defined(VERSION_EU)
        &gMsTopChargeOptionTextByLanguage,
#elif defined(VERSION_JP)
        gMsTopNothingToTradeText,
#elif defined(VERSION_US)
        gCardMessageNoText,
#endif
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gPoohLeaveWorldText),
        CARD_MSG_FLAG_CHOICE_AT_END | CARD_MSG_FLAG_CHOICE_WINDOW,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gWorldselectTutorialText0),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gWorldselectTutorialText1),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText00),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText01),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText02),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText03),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText04),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText05),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText06),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText07),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText08),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
#if defined(VERSION_EU)
        &gRobeTutorialText09ByLanguage,
#elif defined(VERSION_JP)
        gRobeTutorialText04,
#elif defined(VERSION_US)
        gRobeTutorialText09,
#endif
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText10),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText11),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText12),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText13),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText14),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText15),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText16),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText17),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText18),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText19),
        0,
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText20),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText21),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_ROBED_FIGURE, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gRobeTutorialText22),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectTutorialText0),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectTutorialText1),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectTutorialText2),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectTutorialText3),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectValueTutorialText0),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectValueTutorialText1),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectValueTutorialText2),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectValueTutorialText3),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gSavePointTutorialText),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
#if defined(VERSION_EU)
        &gSavePointTutorialTextByLanguage,
#elif defined(VERSION_JP)
        gSaveMenuTutorialText,
#elif defined(VERSION_US)
        gSavePointTutorialText,
#endif
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gQuickSaveTutorialText),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapTutorialText0),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapTutorialText1),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapTutorialText2),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectEventDoorTutorialText0),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectEventDoorTutorialText1),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectEventDoorTutorialText2),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectEventDoorTutorialText3),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapSelectEventDoorTutorialText4),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText00),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText01),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText02),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText03),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText04),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText05),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText06),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText07),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText08),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText09),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText10),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_BOTTOM_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText11),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText12),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_BOTTOM_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText13),
        0,
    },
    {
        PORTRAIT_LEON, MSG_WIN_POSITION_TOP_FACE_RIGHT, 0, 3,
        LOCALIZED(gLeonTutorialText14),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMsTopIntroText0),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMsTopIntroText1),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMsTopIntroText2),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gWarpPointTutorialText),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedWarpinatorText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedTerrorText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedSynchroText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedBindText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedIdyllRompText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedTrinityLimitText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedConfuseText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedThunderRaidText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedGiftedMiracleText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedGravityRaidText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedFireRaidText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedAquaSplashText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedBlizzardRaidText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedStopRaidText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedShockImpactText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedHomingBlizzaraText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedQuakeText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedBlazingDonaldText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedHomingFiraText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedTeleportText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedTornadoText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedCrossSlashPlusText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedReflectRaidText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedFiragaBreakText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedWarpText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gLearnedJudgmentText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedElixirText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedSpellbinderText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedGenieText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedCloudText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedOathkeeperText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedOblivionText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedBambiText),
        CARD_MSG_FLAG_CHOICE_AT_END,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gEventSaveConfirmText),
        CARD_MSG_FLAG_CHOICE_AT_END,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gMapStairTutorialText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedKeyOfBeginningsText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedKeyOfGuidanceText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedKeyToTruthText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedKeyToRewardsText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedTinkerBellText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedWorldCardText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
#if defined(VERSION_EU)
        &gObtainedWorldCardsTextByLanguage,
#elif defined(VERSION_JP)
        gObtainedWorldCardText,
#elif defined(VERSION_US)
        gObtainedWorldCardsText,
#endif
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gObtainedSimbaText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gSaveDataLostText),
        0,
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
        LOCALIZED(gRikuDeckTutorialText),
#ifdef VERSION_JP
        0,
#else
        CARD_MSG_FLAG_ALT_HIGHLIGHT,
#endif
    },
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
#ifdef VERSION_EU
        &gQuickSaveCompleteTextByLanguage,
#else
        gRikuBattleTutorialText,
#endif
        0,
    },
#ifndef VERSION_EU
    {
        PORTRAIT_NONE, MSG_WIN_POSITION_BOTTOM_FACE_LEFT, 0, 3,
#ifndef VERSION_EU
        gQuickSaveCompleteText,
#endif
        0,
    },
#endif
};

TaskDesc gTaskDescCardMsgwin = {
    "msgwin",
    (TaskInitFunc)msgwin_0,
    (TaskUpdateFunc)msgwin_1,
    (TaskDrawFunc)msgwin_2,
    (TaskDestroyFunc)msgwin_3,
    sizeof(CardMsgWinWork),
};
