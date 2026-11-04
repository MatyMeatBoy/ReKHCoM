/**
 * card_lvup_msg.c
 * Level-Up Message
 */

#include "system_state.h"
#include "m4a_song.h"
#include "game_state.h"
#include "text.h"
#include "monsgage.h"
#include "fade.h"
#include "obj_api.h"
#include "engine_math.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_card.h"
#include "songs.h"
#include "jiminy_data.h"
#include "common_text.h"
#include "msg.h"
#include <stddef.h>
#include "types.h"
#include "card_lvup_msg.h"
#include "sprite_palettes.h"
#include "text_types.h"
#include "macros.h"

#ifdef VERSION_EU
static const u8 sLvupMsgPeriod[] = ".";
#elif defined(VERSION_US)
static const u16 sLvupMsgPeriod[2] = { '.', 0 };
#endif

void Lvup_msg_0(LvupMsgWork* work, StatIncreaseDisplayArgs* arg) {
    StatIncreaseDisplayArgs args = *arg;

    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    InitTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
#ifndef VERSION_JP
    InitTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
    work->textSlotCount4 = LoadTextSlots(sLvupMsgPeriod, work->textSlots4);
#endif
    work->amount = arg->amount;
    work->active = arg->done;

    if (args.flags & STAT_INCREASE_FLAG_MAX_HP) {
        work->textSlotCount = LoadTextSlots(LOCALIZED_STRING(gLvupMsgHpText), work->textSlots);
    } else if (args.flags & STAT_INCREASE_FLAG_DP) {
        work->textSlotCount = LoadTextSlots(LOCALIZED_STRING(gLvupMsgDpText), work->textSlots);
    } else if (!(gGameState.flags & GAME_FLAG_RIKU)) {
        work->textSlotCount = LoadTextSlots(LOCALIZED_STRING(gLvupMsgCpText), work->textSlots);
    } else {
        work->textSlotCount = LoadTextSlots(LOCALIZED_STRING(gLvupMsgApText), work->textSlots);
    }

    work->textSlotCount2 = LoadTwoDigitTextSlots(work->amount, work->textSlots2);

#ifdef VERSION_EU
    if ((args.flags & STAT_INCREASE_FLAG_MAX_HP) && gLanguage == LANGUAGE_SPANISH) {
        work->textSlotCount3 = LoadTextSlots(GetLocalizedString(&gLvupMsgHpIncreasedTextByLanguage), work->textSlots3);
    } else {
        work->textSlotCount3 = LoadTextSlots(GetLocalizedString(&gLvupMsgIncreasedTextByLanguage), work->textSlots3);
    }
#else
    work->textSlotCount3 = LoadTextSlots(gLvupMsgIncreasedText, work->textSlots3);
#endif
    work->textPalette = LoadTextPalette(1);
    FadeSetPaletteExcluded(BIOS_PTR(work->textPalette)->index + 16, TRUE);
    work->x = 0x1000;
    work->x2 = 0x3000;
    work->x3 = 0x4200;
    work->y2 = 0xC800;
    work->y3 = 0xC800;
    work->y4 = 0xC800;
    work->y = 0xCE00;
    work->slideSteps = 14;
    work->unk_2B1 = 0;
    work->x = ((144 - (GetTextSlotsWidth(work->textSlots, work->textSlotCount)
#ifndef VERSION_JP
                         + GetTextSlotsWidth(work->textSlots4, work->textSlotCount4)
#endif
                         + GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)
                         + GetTextSlotsWidth(work->textSlots3, work->textSlotCount3))) / 2) << 8;
#ifdef VERSION_EU
    work->tiles = AllocSpriteFrameTiles(0x780);
    UpdateSpriteFrameTiles(work->tiles, gMsgBoxFrames[0], gMsgBoxTiles);
#else
    work->tiles = LoadObjTiles(gMsgBoxTiles, sizeof(gMsgBoxTiles));
#endif
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
}

s32 Lvup_msg_1(LvupMsgWork* work, void* task) {
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateLvupMsgSlideIn);
    work->unk_2B1++;
    return 1;
}

u8 UpdateLvupMsgSlideIn(LvupMsgWork* work, void* task) {
    s8* counter = &work->slideSteps;

    if (*counter > 0) {
        ApproachValue(&work->y, 0x6C00, *counter);
        ApproachValue(&work->y2, 0x6600, *counter);
        ApproachValue(&work->y3, 0x6600, *counter);
        ApproachValue(&work->y4, 0x6600, *counter);
        (*counter)--;
    } else if (*counter == 0) {
        m4aSongNumStart(SONG_SYS_CHAGEF2);
        work->slideSteps = -1;
    }

    if (*work->active == 0) {
        return 0;
    }

    return 1;
}

void Lvup_msg_2(LvupMsgWork* work) {
#ifdef VERSION_JP
    work->x2 = work->x + work->textSlotCount * 0xA00;
    work->x3 = work->x2 + work->textSlotCount2 * 0xA00;
#elif defined(VERSION_EU)
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->x3 = work->x + ((GetTextSlotsWidth(work->textSlots, work->textSlotCount) + 3) << 8);
        work->x2 = work->x3 + ((GetTextSlotsWidth(work->textSlots3, work->textSlotCount3) + 3) << 8);
        DrawTextSlots((work->x2 >> 8) + GetTextSlotsWidth(work->textSlots2, work->textSlotCount2), work->y3 >> 8,
                      work->textSlots4, work->textPalette, 40, work->textSlotCount4);
        break;
    case LANGUAGE_FRENCH:
        work->x3 = work->x + ((GetTextSlotsWidth(work->textSlots, work->textSlotCount) + 3) << 8);
        work->x2 = work->x3 + ((GetTextSlotsWidth(work->textSlots3, work->textSlotCount3) + 3) << 8);
        DrawTextSlots((work->x2 >> 8) + GetTextSlotsWidth(work->textSlots2, work->textSlotCount2), work->y3 >> 8,
                      work->textSlots4, work->textPalette, 40, work->textSlotCount4);
        break;
    case LANGUAGE_GERMAN:
        work->x2 = work->x + ((GetTextSlotsWidth(work->textSlots, work->textSlotCount) + 3) << 8);
        work->x3 = work->x2 + ((GetTextSlotsWidth(work->textSlots2, work->textSlotCount2) + 3) << 8);
        break;
    case LANGUAGE_ITALIAN:
        work->x3 = work->x + ((GetTextSlotsWidth(work->textSlots, work->textSlotCount) + 3) << 8);
        work->x2 = work->x3 + ((GetTextSlotsWidth(work->textSlots3, work->textSlotCount3) + 3) << 8);
        DrawTextSlots((work->x2 >> 8) + GetTextSlotsWidth(work->textSlots2, work->textSlotCount2), work->y3 >> 8,
                      work->textSlots4, work->textPalette, 40, work->textSlotCount4);
        break;
    case LANGUAGE_SPANISH:
        work->x3 = work->x + ((GetTextSlotsWidth(work->textSlots, work->textSlotCount) + 3) << 8);
        work->x2 = work->x3 + ((GetTextSlotsWidth(work->textSlots3, work->textSlotCount3) + 3) << 8);
        DrawTextSlots((work->x2 >> 8) + GetTextSlotsWidth(work->textSlots2, work->textSlotCount2), work->y3 >> 8,
                      work->textSlots4, work->textPalette, 40, work->textSlotCount4);
        break;
    }
#else
    work->x3 = work->x + ((GetTextSlotsWidth(work->textSlots, work->textSlotCount) + 3) << 8);
    work->x2 = work->x3 + ((GetTextSlotsWidth(work->textSlots3, work->textSlotCount3) + 3) << 8);
#endif
#ifdef VERSION_EU
    DrawSprite(72, work->y >> 8, NULL, work->tiles, work->palette, NULL, 0, 41);
#else
    DrawSprite(72, work->y >> 8, gMsgBoxFrames[0], work->tiles, work->palette, NULL, 0, 41);
#endif
    DrawTextSlots(work->x >> 8, work->y2 >> 8, work->textSlots, work->textPalette, 40, work->textSlotCount);
    DrawTextSlots(work->x2 >> 8, work->y3 >> 8, work->textSlots2, work->textPalette, 40, work->textSlotCount2);
    DrawTextSlots(work->x3 >> 8, work->y4 >> 8, work->textSlots3, work->textPalette, 40, work->textSlotCount3);
#ifdef VERSION_US
    DrawTextSlots((work->x2 >> 8) + GetTextSlotsWidth(work->textSlots2, work->textSlotCount2), work->y3 >> 8,
                  work->textSlots4, work->textPalette, 40, work->textSlotCount4);
#endif
}

void Lvup_msg_3(LvupMsgWork* work) {
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
#ifndef VERSION_JP
    FreeTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
#endif
    ReleaseObjPalette(work->textPalette);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

TaskDesc gTaskDescLvupMsg = {
    "Lvup msg",
    (TaskInitFunc)Lvup_msg_0,
    (TaskUpdateFunc)Lvup_msg_1,
    (TaskDrawFunc)Lvup_msg_2,
    (TaskDestroyFunc)Lvup_msg_3,
    sizeof(LvupMsgWork),
};
