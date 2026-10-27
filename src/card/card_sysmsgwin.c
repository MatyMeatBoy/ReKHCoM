/**
 * card_sysmsgwin.c
 * System Message Window
 */

#include "system_state.h"
#include "msg_api.h"
#include "m4a_song.h"
#include "text.h"
#include "monsgage.h"
#include "fade.h"
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
#include "sprites_map.h"
#include "sprites_card.h"
#include "gba/keys.h"
#include "songs.h"
#include "jiminy_data.h"
#include "common_text.h"
#include "card_message_data.h"
#include "types.h"
#include "gba/defines.h"
#include "gba/macro.h"
#include <stddef.h>
#include "card_sysmsgwin.h"
#include "sprite_palettes.h"
#include "macros.h"
#include "msg_types.h"

static SysMsgWinWork* sActiveSysmsgwin;

static const s32 sSysmsgwinTextY[4] = { 0xE00, 0x6C00, 0xE00, 0x6C00 };

void sysmsgwin_0(SysMsgWinWork* work, CardMessageArgs* args) {
    CpuFill32(0, work, sizeof(SysMsgWinWork));
    work->args = *args;
    work->messageDef = &gCardMessageDefs[work->args.messageId];

    if (work->messageDef->flags & CARD_MSG_FLAG_ALT_HIGHLIGHT) {
        work->glyphPaletteIndex = InitCardMsgGlyphSprites(1, TRUE);
    } else {
        work->glyphPaletteIndex = InitCardMsgGlyphSprites(1, FALSE);
    }

    FadeSetPaletteExcluded(work->glyphPaletteIndex + 16, TRUE);
    work->unk_13C = 0;
    work->steps = 8;
    work->shownChars = 0;
    work->charTimer = 0;
    work->charCount = 0;
    work->unk_143 = 0;
    work->nextText = NULL;
    work->tiles3 = NULL;
    work->palette = NULL;
    work->tiles4 = NULL;
    work->palette2 = NULL;
    work->tiles2 = NULL;
    work->palette4 = NULL;
    work->tiles = NULL;
    work->palette3 = NULL;
    work->textPalette = NULL;
    work->unk_142 = 1;
    work->waitIconVisible = TRUE;
    work->choiceVisible = FALSE;
    work->textSlotCount = 0;
    work->textSlotCount2 = 0;
    work->messagePending = FALSE;
    work->keepOpen = TRUE;
#ifdef VERSION_JP
    work->charCount = LayoutCardMsgGlyphsPageSjis(0x2E00, gMsgwinTextY[work->messageDef->positionIndex],
                                   CARD_MSG_TEXT(work), &work->nextText);
#else
    if (work->nextText != NULL) {
        work->charCount = LayoutCardMsgGlyphsPage(0x2E00, sSysmsgwinTextY[work->messageDef->positionIndex] - 0x200,
                                       CARD_MSG_TEXT(work), &work->nextText);
    } else {
        work->charCount = LayoutCardMsgGlyphsPage(0x2E00, sSysmsgwinTextY[work->messageDef->positionIndex] - 0x200,
                                       CARD_MSG_TEXT(work), &work->nextText);
    }
#endif

    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    gMessageWindowOpen = TRUE;
    gMessageWindowAnswerYes = FALSE;
    work->shownChars = work->charCount;

    switch (work->args.mode) {
    case CARD_MESSAGE_MODE_BG_WINDOW:
        work->tiles3 = AllocObjTiles(0x40, NULL);
        work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
        // @bug Palette allocation can fail; preserve the original GBA BIOS read.
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
        SetObjTileSource(work->tiles3, gFEventTiles);
        AnimInit(&work->anim2, gFEventAnims, gFEventFrames);
        AnimStart(&work->anim2, 2, ANIM_FLAG_LOOP);
        work->gfx4 = AnimGetGfx(&work->anim2);
        SetBgPriority(work->args.bg, 0);
        break;
    case CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT:
        SetBgPriority(work->args.bg, 0);
        break;
    case CARD_MESSAGE_MODE_SPRITE_WINDOW:
        work->tiles3 = AllocObjTiles(0x40, NULL);
        work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
        SetObjTileSource(work->tiles3, gFEventTiles);
        AnimInit(&work->anim2, gFEventAnims, gFEventFrames);
        AnimStart(&work->anim2, 2, ANIM_FLAG_LOOP);
        work->gfx4 = AnimGetGfx(&work->anim2);
        break;
    }

    sActiveSysmsgwin = work;
}

u8 sysmsgwin_1(SysMsgWinWork* work, void* task) {
    void* pal;

    switch (work->args.mode) {
    case CARD_MESSAGE_MODE_BG_WINDOW:
    case CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT:
        pal = (void*)(BG_PLTT + 15 * PLTT_SIZE_4BPP);
        LoadBgTiles(work->args.bg, gSysMsgWinTiles, sizeof(gSysMsgWinTiles));
        LoadBgMap(work->args.bg, gSysMsgWinMap, sizeof(gSysMsgWinMap));
        LoadPalette(gCard00Palette, pal, sizeof(gCard00Palette));

        switch (work->messageDef->positionIndex) {
        case MSG_WIN_POSITION_TOP_FACE_RIGHT:
        case MSG_WIN_POSITION_TOP_FACE_LEFT:
            SetBgScroll(work->args.bg, (u16)-24, 0);
            break;
        case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
            SetBgScroll(work->args.bg, (u16)-24, (u16)-94);
            break;
        default:
            SetBgScroll(work->args.bg, (u16)-24, (u16)-94);
            break;
        }

        break;
    case CARD_MESSAGE_MODE_SPRITE_WINDOW:
    case CARD_MESSAGE_MODE_SPRITE_WINDOW_PERSISTENT:
        switch (work->messageDef->positionIndex) {
        case MSG_WIN_POSITION_TOP_FACE_RIGHT:
        case MSG_WIN_POSITION_TOP_FACE_LEFT:
            work->frameX = 0x7800;
            work->frameY = 0x2200;
            break;
        case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        default:
            work->frameX = 0x7800;
            work->frameY = 0x7E00;
            break;
        }

        work->tiles2 = LoadObjTiles(gLargeDialogBoxTiles, sizeof(gLargeDialogBoxTiles));

        if (work->tiles2 == NULL) {
            work->fallbackFrame = TRUE;
            work->tiles2 = LoadObjTiles(gSysMsgWinFallbackTiles, sizeof(gSysMsgWinFallbackTiles));
        } else {
            work->fallbackFrame = FALSE;
        }

        work->palette4 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
        // @bug palette4 can be NULL (NULL read).
        FadeSetPaletteExcluded(BIOS_PTR(work->palette4)->index + 16, TRUE);
        break;
    }

    switch (work->args.mode) {
    case CARD_MESSAGE_MODE_BG_WINDOW:
    case CARD_MESSAGE_MODE_SPRITE_WINDOW:
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinWaitInput);
        break;
    case CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT:
    case CARD_MESSAGE_MODE_SPRITE_WINDOW_PERSISTENT:
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinPersistent);
        break;
    }

    return 1;
}

u8 UpdateSysmsgwinWaitInput(SysMsgWinWork* work, void* task) {
    u16* pal;

    if (work->tiles3 != NULL) {
        work->gfx4 = AnimUpdate(&work->anim2);
    }

    if (GetKeysPressed() & A_BUTTON) {
        m4aSongNumStart(SONG_SYS_KETTEI);

        if (work->nextText != NULL) {
#ifdef VERSION_JP
            work->charCount = LayoutCardMsgGlyphsPageSjis(0x2E00, gMsgwinTextY[work->messageDef->positionIndex],
                                           work->nextText, &work->nextText);
#else
            work->charCount = LayoutCardMsgGlyphsPage(0x2E00, sSysmsgwinTextY[work->messageDef->positionIndex] - 0x200,
                                           work->nextText, &work->nextText);
#endif
            work->shownChars = work->charCount;
        } else if (!(work->messageDef->flags & CARD_MSG_FLAG_CHOICE_AT_END)) {
            AnimStart(&work->anim2, 3, ANIM_FLAG_LOOP);
            work->unk_142 = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinClose);
            work->closeTimer = 0;
            work->steps = 8;
        } else {
            ReleaseObjTiles(work->tiles3);
            ReleaseObjPalette(work->palette);
            work->tiles3 = NULL;
            work->palette = NULL;
            work->tiles4 = AllocObjTiles(0x120, NULL);
            pal = gDialogBoxPalette;
            work->palette2 = LoadObjPalette(pal, 32);
#ifdef VERSION_EU
            FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 16, TRUE);
#else
            // @bug palette4 can be NULL (NULL read).
            FadeSetPaletteExcluded(BIOS_PTR(work->palette4)->index + 16, TRUE);
#endif
#ifdef PLATFORM_PC
            // A BIOS read is not an allocated palette bank to upload into.
            if (work->palette2 != NULL)
#endif
            LoadObjPaletteBank(BIOS_PTR(work->palette2)->index, pal);
            SetObjTileSource(work->tiles4, gHandCursorTiles);
            AnimInit(&work->anim3, gHandCursorAnims, gHandCursorFrames);
            AnimStart(&work->anim3, 2, ANIM_FLAG_LOOP);
            work->gfx = AnimGetGfx(&work->anim3);
            work->choice = 1;
            work->x = 0x5800;
            work->cursorY = gMsgwaitYesnoCursorY[work->choice] - 0x500;
            work->textSlotCount = LoadTextSlots(LOCALIZED_STRING(gYesChoiceText), work->textSlots);
            work->textSlotCount2 = LoadTextSlots(LOCALIZED_STRING(gNoChoiceText), work->textSlots2);
            work->textPalette = LoadTextPalette(1);
            work->choiceVisible = TRUE;
            work->tiles = LoadObjTiles(gMsgBoxTiles, sizeof(gMsgBoxTiles));
            work->palette3 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
#ifdef VERSION_EU
            FadeSetPaletteExcluded(BIOS_PTR(work->palette3)->index + 16, TRUE);
#else
            // @bug palette4 can be NULL (NULL read).
            FadeSetPaletteExcluded(BIOS_PTR(work->palette4)->index + 16, TRUE);
#endif
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinChoice);
        }
    }

    return 1;
}

s32 UpdateSysmsgwinClose(SysMsgWinWork* work) {
    if (work->tiles3 != NULL) {
        work->gfx4 = AnimUpdate(&work->anim2);
    }

    work->closeTimer += 1;

    if (work->closeTimer > 15) {
        work->waitIconVisible = FALSE;
        return 0;
    }

    return 1;
}

u8 UpdateSysmsgwinChoice(SysMsgWinWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim3);

    switch (GetKeysPressed()) {
    case DPAD_UP:
        if (work->choice != 0) {
            work->choice--;
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        work->cursorSteps = 4;
        break;
    case DPAD_DOWN:
        if (work->choice == 0) {
            work->choice++;
            m4aSongNumStart(SONG_SYS_CLICK);
        }

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

        work->unk_142 = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinClose);
        break;
    }

    if (work->cursorSteps != 0) {
        ApproachValue(&work->cursorY, gMsgwaitYesnoCursorY[work->choice] - 0x500, work->cursorSteps);
        work->cursorSteps--;
    }

    return 1;
}

void sysmsgwin_2(SysMsgWinWork* work) {
    DrawCardMsgGlyphs(work->shownChars);

    switch (work->args.mode) {
    case CARD_MESSAGE_MODE_SPRITE_WINDOW:
    case CARD_MESSAGE_MODE_SPRITE_WINDOW_PERSISTENT:
        if (work->tiles2 != NULL) {
            if (work->fallbackFrame) {
                DrawSprite(work->frameX >> 8, work->frameY >> 8, gSysMsgWinFallbackFrames[0],
                           work->tiles2, work->palette4, NULL, 0, 10);
            } else {
                DrawSprite(work->frameX >> 8, work->frameY >> 8, gLargeDialogBoxFrames[0],
                           work->tiles2, work->palette4, NULL, 0, 10);
            }
        }

        break;
    }

    if (work->tiles3 != NULL) {
        if (work->waitIconVisible) {
            DrawSprite(120, gMsgwaitIconPos[work->messageDef->positionIndex][1] >> 8, work->gfx4,
                       work->tiles3, work->palette, NULL, 0, 5);
        }
    }

    if (work->tiles4 != NULL) {
        DrawSprite(work->x >> 8, work->cursorY >> 8, work->gfx,
                   work->tiles4, work->palette2, NULL, SPRITE_FLAG_HFLIP, 5);
    }

    if (work->choiceVisible) {
        DrawSprite(120, 75, gMsgBoxFrames[1], work->tiles, work->palette3, NULL, 0, 10);
        DrawTextSlots((DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) >> 1, 62, work->textSlots,
                      work->textPalette, 0, work->textSlotCount);
        DrawTextSlots((DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)) >> 1, 77, work->textSlots2,
                      work->textPalette, 0, work->textSlotCount2);
    }
}

void sysmsgwin_3(SysMsgWinWork* work) {
    if (work->args.mode <= CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT) {
        DisableBg(work->args.bg);
    }

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

    if (work->tiles2 != NULL) {
        ReleaseObjTiles(work->tiles2);
    }

    if (work->palette4 != NULL) {
        ReleaseObjPalette(work->palette4);
    }

    if (work->palette3 != NULL) {
        ReleaseObjPalette(work->palette3);
    }

    if (work->textPalette != NULL) {
        ReleaseObjPalette(work->textPalette);
    }

    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    gMessageWindowOpen = FALSE;
    sActiveSysmsgwin = NULL;
}

u8 UpdateSysmsgwinPersistent(SysMsgWinWork* work, void* task) {
#ifndef VERSION_JP
    TextChar** nextText;
#endif

    if (!work->keepOpen) {
        return 0;
    }

    if (work->messagePending == TRUE) {
        work->messagePending = FALSE;
        work->messageDef = &gCardMessageDefs[work->args.messageId];
#ifdef VERSION_JP
        work->charCount = LayoutCardMsgGlyphsPageSjis(
            0x2E00,
            gMsgwinTextY[work->messageDef->positionIndex],
            CARD_MSG_TEXT(work),
            &work->nextText);
#else
        nextText = &work->nextText;

        if (*nextText != NULL) {
            work->charCount = LayoutCardMsgGlyphsPage(
                0x2E00,
                sSysmsgwinTextY[work->messageDef->positionIndex] - 0x200,
                *nextText, nextText);
        } else {
            work->charCount = LayoutCardMsgGlyphsPage(
                0x2E00,
                sSysmsgwinTextY[work->messageDef->positionIndex] - 0x200,
                CARD_MSG_TEXT(work),
                nextText);
        }
#endif

        work->shownChars = work->charCount;
    }

    return 1;
}

s32 ReplaceSysmsgwinMessage(CardMessageArgs* src) {
    if (sActiveSysmsgwin != NULL) {
        sActiveSysmsgwin->args = *src;
        sActiveSysmsgwin->messagePending = TRUE;

        return TRUE;
    }

    return FALSE;
}

s32 CloseSysmsgwin() {
    if (sActiveSysmsgwin != NULL) {
        sActiveSysmsgwin->keepOpen = FALSE;
        return TRUE;
    }

    return FALSE;
}

void sysmsgwinChoice_0(SysMsgWinWork* work, CardMessageArgs* args) {
    CpuFill32(0, work, sizeof(SysMsgWinWork));
    work->args = *args;
    work->messageDef = &gCardMessageDefs[work->args.messageId];
    work->glyphPaletteIndex = InitCardMsgGlyphSprites(1, FALSE);
    FadeSetPaletteExcluded(work->glyphPaletteIndex + 16, TRUE);
    work->unk_13C = 0;
    work->steps = 8;
    work->shownChars = 0;
    work->charTimer = 0;
    work->charCount = 0;
    work->unk_143 = 0;
    work->nextText = NULL;
    work->tiles3 = NULL;
    work->palette = NULL;
    work->tiles4 = NULL;
    work->palette2 = NULL;
    work->tiles2 = NULL;
    work->palette4 = NULL;
    work->tiles = NULL;
    work->palette3 = NULL;
    work->textPalette = NULL;
    work->unk_142 = 1;
    work->waitIconVisible = TRUE;
    work->choiceVisible = FALSE;
    work->textSlotCount = 0;
    work->textSlotCount2 = 0;
    work->messagePending = FALSE;
    work->keepOpen = TRUE;
#ifdef VERSION_JP
    work->charCount = LayoutCardMsgGlyphsPageSjis(0x4000, 0x4000, (TextChar*)work->messageDef->text, &work->nextText);
#else
    if (work->nextText != NULL) {
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
            work->charCount = LayoutCardMsgGlyphsPage(0x4D00, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        case LANGUAGE_GERMAN:
            work->charCount = LayoutCardMsgGlyphsPage(0x4100, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        case LANGUAGE_SPANISH:
            work->charCount = LayoutCardMsgGlyphsPage(0x4400, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        case LANGUAGE_ITALIAN:
            work->charCount = LayoutCardMsgGlyphsPage(0x4600, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        default:
            work->charCount = LayoutCardMsgGlyphsPage(0x4D00, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        }
#else
        work->charCount = LayoutCardMsgGlyphsPage(0x4D00, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
#endif
    } else {
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
        case LANGUAGE_FRENCH:
            work->charCount = LayoutCardMsgGlyphsPage(0x4D00, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        case LANGUAGE_GERMAN:
            work->charCount = LayoutCardMsgGlyphsPage(0x4100, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        case LANGUAGE_SPANISH:
            work->charCount = LayoutCardMsgGlyphsPage(0x4400, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        case LANGUAGE_ITALIAN:
            work->charCount = LayoutCardMsgGlyphsPage(0x4600, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        default:
            work->charCount = LayoutCardMsgGlyphsPage(0x4D00, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
            break;
        }
#else
        work->charCount = LayoutCardMsgGlyphsPage(0x4D00, 0x4000, (TextChar*)LANGSTR(work->messageDef->text), &work->nextText);
#endif
    }
#endif

    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    gMessageWindowOpen = TRUE;
    gMessageWindowAnswerYes = FALSE;
    work->shownChars = work->charCount;
    sActiveSysmsgwin = work;
}

u8 sysmsgwinChoice_1(SysMsgWinWork* work, void* task) {
    void* pal;

    switch (work->args.mode) {
    case CARD_MESSAGE_MODE_BG_WINDOW:
    case CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT:
        pal = (void*)(BG_PLTT + 15 * PLTT_SIZE_4BPP);
        LoadBgTiles(work->args.bg, gConfirmWinTiles, sizeof(gConfirmWinTiles));
        LoadBgMap(work->args.bg, gConfirmWinMap, sizeof(gConfirmWinMap));
        LoadPalette(gCard00Palette, pal, sizeof(gCard00Palette));

        switch (work->messageDef->positionIndex) {
        case MSG_WIN_POSITION_TOP_FACE_RIGHT:
        case MSG_WIN_POSITION_TOP_FACE_LEFT:
            SetBgScroll(work->args.bg, 0, 0);
            break;
        case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        case MSG_WIN_POSITION_BOTTOM_FACE_LEFT:
            SetBgScroll(work->args.bg, 0, 0);
            break;
        default:
            SetBgScroll(work->args.bg, 0, 0);
            break;
        }

        break;
    case CARD_MESSAGE_MODE_SPRITE_WINDOW:
    case CARD_MESSAGE_MODE_SPRITE_WINDOW_PERSISTENT:
        switch (work->messageDef->positionIndex) {
        case MSG_WIN_POSITION_TOP_FACE_RIGHT:
        case MSG_WIN_POSITION_TOP_FACE_LEFT:
            work->frameX = 0x7800;
            work->frameY = 0x2000;
            break;
        case MSG_WIN_POSITION_BOTTOM_FACE_RIGHT:
        default:
            work->frameX = 0x7800;
            work->frameY = 0x8200;
            break;
        }

        work->tiles2 = LoadObjTiles(gDialogBoxTiles, sizeof(gDialogBoxTiles));
        work->palette4 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
        // @bug palette4 can be NULL (NULL read).
        FadeSetPaletteExcluded(BIOS_PTR(work->palette4)->index + 16, TRUE);
        break;
    }

    if (work->args.mode == CARD_MESSAGE_MODE_BG_WINDOW || work->args.mode == CARD_MESSAGE_MODE_SPRITE_WINDOW) {
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinChoiceSetup);
    }

    return 1;
}

u8 UpdateSysmsgwinChoiceSetup(SysMsgWinWork* work, void* task) {
    u16* pal;

    work->tiles4 = AllocObjTiles(0x120, NULL);
    pal = gDialogBoxPalette;
    work->palette2 = LoadObjPalette(pal, 32);
#ifdef VERSION_EU
    FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 16, TRUE);
#else
    // @bug palette4 can be NULL (NULL read).
    FadeSetPaletteExcluded(BIOS_PTR(work->palette4)->index + 16, TRUE);
#endif
#ifdef PLATFORM_PC
    // A BIOS read is not an allocated palette bank to upload into.
    if (work->palette2 != NULL)
#endif
    LoadObjPaletteBank(BIOS_PTR(work->palette2)->index, pal);
    SetObjTileSource(work->tiles4, gHandCursorTiles);
    AnimInit(&work->anim3, gHandCursorAnims, gHandCursorFrames);
    AnimStart(&work->anim3, 2, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim3);
    work->choice = 1;
    work->x = 0x8500;
    work->cursorY = 0x5000;
    work->textSlotCount = LoadTextSlots(LOCALIZED_STRING(gYesChoiceText), work->textSlots);
    work->textSlotCount2 = LoadTextSlots(LOCALIZED_STRING(gNoChoiceText), work->textSlots2);
    work->textPalette = LoadTextPalette(1);
    work->choiceVisible = TRUE;
    work->tiles = LoadObjTiles(gMsgBoxTiles, sizeof(gMsgBoxTiles));
    work->palette3 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
#ifdef VERSION_EU
    FadeSetPaletteExcluded(BIOS_PTR(work->palette3)->index + 16, TRUE);
#else
    // @bug palette4 can be NULL (NULL read).
    FadeSetPaletteExcluded(BIOS_PTR(work->palette4)->index + 16, TRUE);
#endif
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinChoiceInput);
    return 1;
}

s32 UpdateSysmsgwinChoiceClose(SysMsgWinWork* work) {
    if (work->tiles3 != NULL) {
        work->gfx4 = AnimUpdate(&work->anim2);
    }

    work->closeTimer += 1;

    if (work->closeTimer > 15) {
        work->waitIconVisible = FALSE;
        return 0;
    }

    return 1;
}

u8 UpdateSysmsgwinChoiceInput(SysMsgWinWork* work, void* task) {
    s32 cursorX[2];

    *(u64*)cursorX = *(u64*)gSysmsgwinChoiceCursorX;
    work->gfx = AnimUpdate(&work->anim3);

    switch (GetKeysPressed()) {
    case DPAD_LEFT:
        if (work->choice != 0) {
            work->choice--;
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        work->cursorSteps = 1;
        break;
    case DPAD_RIGHT:
        if (work->choice == 0) {
            work->choice++;
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        work->cursorSteps = 1;
        break;
    }

    switch (GetKeysPressed()) {
    case A_BUTTON:
    case START_BUTTON:
        m4aSongNumStart(SONG_SYS_KETTEI);

        if (work->choice == 0) {
            gMessageWindowAnswerYes = TRUE;
        } else {
            gMessageWindowAnswerYes = FALSE;
        }

        work->unk_142 = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinChoiceClose);
        break;
    case B_BUTTON:
        m4aSongNumStart(SONG_SYS_CLOSE);
        gMessageWindowAnswerYes = FALSE;
        work->unk_142 = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSysmsgwinChoiceClose);
        break;
    }

    if (work->cursorSteps != 0) {
        ApproachValue(&work->x, cursorX[work->choice], work->cursorSteps);
        work->cursorSteps--;
    }

    return 1;
}

void sysmsgwinChoice_2(SysMsgWinWork* work) {
    DrawCardMsgGlyphs(work->shownChars);

    switch (work->args.mode) {
    case CARD_MESSAGE_MODE_SPRITE_WINDOW:
    case CARD_MESSAGE_MODE_SPRITE_WINDOW_PERSISTENT:
        if (work->tiles2 != NULL) {
            DrawSprite(work->frameX >> 8, work->frameY >> 8, gDialogBoxFrames[0], work->tiles2, work->palette4, NULL, 0, 20);
        }

        break;
    }

    if (work->tiles3 != NULL && work->waitIconVisible) {
        DrawSprite(120, gMsgwaitIconPos[work->messageDef->positionIndex][1] >> 8, work->gfx4, work->tiles3, work->palette, NULL, 0, 10);
    }

    if (work->tiles4 != NULL) {
        DrawSprite(work->x >> 8, work->cursorY >> 8, work->gfx, work->tiles4, work->palette2, NULL, SPRITE_FLAG_HFLIP, 10);
    }

    DrawTextSlots(89, 86, work->textSlots, work->textPalette, 0, work->textSlotCount);
    DrawTextSlots(135, 86, work->textSlots2, work->textPalette, 0, work->textSlotCount2);
}

void sysmsgwinChoice_3(SysMsgWinWork* work) {
    if (work->args.mode <= CARD_MESSAGE_MODE_BG_WINDOW_PERSISTENT) {
        DisableBg(work->args.bg);
    }

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

    if (work->tiles2 != NULL) {
        ReleaseObjTiles(work->tiles2);
    }

    if (work->palette4 != NULL) {
        ReleaseObjPalette(work->palette4);
    }

    if (work->palette3 != NULL) {
        ReleaseObjPalette(work->palette3);
    }

    if (work->textPalette != NULL) {
        ReleaseObjPalette(work->textPalette);
    }

    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    gMessageWindowOpen = FALSE;
    sActiveSysmsgwin = NULL;
}

TaskDesc gTaskDescSysmsgwin = {
    "sysmsgwin",
    (TaskInitFunc)sysmsgwin_0,
    (TaskUpdateFunc)sysmsgwin_1,
    (TaskDrawFunc)sysmsgwin_2,
    (TaskDestroyFunc)sysmsgwin_3,
    sizeof(SysMsgWinWork),
};

TaskDesc gTaskDescSysmsgwinChoice = {
    "sysmsgwin",
    (TaskInitFunc)sysmsgwinChoice_0,
    (TaskUpdateFunc)sysmsgwinChoice_1,
    (TaskDrawFunc)sysmsgwinChoice_2,
    (TaskDestroyFunc)sysmsgwinChoice_3,
    sizeof(SysMsgWinWork),
};

const s32 gSysmsgwinChoiceCursorX[2] = { 0x5000, 0x8000 };
