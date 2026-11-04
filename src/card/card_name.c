/**
 * card_name.c
 * Card Name, Premium Effect and Print Layer
 */

#include "msg_localized_data.h"
#include "system_state.h"
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
#include "malloc.h"
#include "card.h"
#include "sprites_card.h"
#include "premium_card_effect.h"
#include "card_types.h"
#include "types.h"
#include <stddef.h>
#include "card_name.h"
#include "ui_text.h"
#include "default_bg_map.h"
#include "sprite_palettes.h"
#include "gba/defines.h"
#include "macros.h"

static PrintLine* sPrintLines;

static u8 sPrintLineCount;

static u8 sPrintBg;


#include "premium_message.inc"
#include "lockon.h"
void CardName_0(CardNameWork* work) {
    PremireChanceCardWork* selectedCard = gCardListWork->selectedCard;
    ObjPalette* pal;
    s32 x;
    s16 suffixX;

    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    InitTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    work->textPalette = LoadTextPalette(1);
#ifdef VERSION_EU
    work->textSlotCount = LoadTextSlots(GetLocalizedString(selectedCard->cardDef->name), work->textSlots);
    work->textSlotCount2 = LoadTextSlots((u16*)gPremiumCardMessageTextByLanguage.strings[gLanguage], work->textSlots2);
#else
    work->textSlotCount = LoadTextSlots(selectedCard->cardDef->name, work->textSlots);
#ifdef VERSION_JP
    work->textSlotCount3 = LoadTextSlots((u16*)gPremiumCardSuffixText, work->textSlots3);
    work->textSlotCount2 = LoadTextSlots((u16*)gPremiumCardMessageText, work->textSlots2);
#else
    work->textSlotCount2 = LoadTextSlots(gPremiumCardMessageText, work->textSlots2);
#endif
#endif
#ifndef VERSION_JP
    work->palette = LoadObjPalette(gTextYellowPalette, sizeof(gTextYellowPalette));
#endif

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
    case LANGUAGE_GERMAN:
    case LANGUAGE_ITALIAN:
        work->textSlotCount3 = 0;
        x = (230 - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) / 2;
        work->nameX = x;
        suffixX = (u16)work->nameX + GetTextSlotsWidth(work->textSlots, work->textSlotCount);
        work->suffixX = suffixX;
        x = (DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)) / 2;
        work->messageX = x;
        break;
    case LANGUAGE_FRENCH:
        work->textSlotCount3 = LoadTextSlots((u16*)gPremiumCardSuffixTextFrench, work->textSlots3);
        x = (230 - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) / 2;
        work->nameX = x;
        suffixX = (u16)work->nameX + GetTextSlotsWidth(work->textSlots, work->textSlotCount);
        work->suffixX = suffixX;
        x = (DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)) / 2;
        work->messageX = x;
        break;
    case LANGUAGE_SPANISH:
        work->textSlotCount3 = LoadTextSlots((u16*)gPremiumCardSuffixTextSpanish, work->textSlots3);
        x = (230 - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) / 2;
        work->nameX = x;
        work->suffixX = x - 3;
        x = (DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)) / 2;
        work->messageX = x;
        break;
    default:
        work->textSlotCount3 = 0;
        break;
    }
#else
    x = (230 - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) / 2;
    work->nameX = x;
    suffixX = (u16)work->nameX + GetTextSlotsWidth(work->textSlots, work->textSlotCount);
    work->suffixX = suffixX;
    x = (DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots2, work->textSlotCount2)) / 2;
    work->messageX = x;
#endif
    work->tiles = LoadObjTiles(gLargeDialogBoxTiles, sizeof(gLargeDialogBoxTiles));
    pal = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->palette2 = pal;
    FadeSetPaletteExcluded(BIOS_PTR(pal)->index + 16, TRUE);
    FadeSetPaletteExcluded(BIOS_PTR(work->textPalette)->index + 16, TRUE);
}

s32 CardName_1() {
    return 1;
}

void CardName_2(CardNameWork* work) {
    DrawSprite(120, 126, gLargeDialogBoxFrames[0], work->tiles, work->palette2, NULL, 0, 50);
#ifdef VERSION_JP
    DrawTextSlots(work->nameX, 115, work->textSlots, work->textPalette, 30, work->textSlotCount);
#else
    DrawTextSlots(work->nameX, 115, work->textSlots, work->palette, 30, work->textSlotCount);
#endif
#ifndef VERSION_US
    DrawTextSlots(work->suffixX, 115, work->textSlots3, work->textPalette, 30, work->textSlotCount3);
#endif
    DrawTextSlots(work->messageX, 130, work->textSlots2, work->textPalette, 30, work->textSlotCount2);
}

void CardName_3(CardNameWork* work) {
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
#ifdef VERSION_EU
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->textPalette);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette);
#else
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->textPalette);
    ReleaseObjPalette(work->palette2);
#ifndef VERSION_JP
    ReleaseObjPalette(work->palette);
#endif
#endif
}

void PremireEffectInit(PremiumCardEffectWork* work, s16* arg) {
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCardSparklePalette, sizeof(gCardSparklePalette));
    SetObjTileSource(work->tiles, gCardSparkleTiles);
    AnimInit(&work->anim, gCardSparkleAnims, gCardSparkleFrames);
    AnimStart(&work->anim, GetRandom() % 3, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->centerX = arg[1] << 8;
    work->centerY = arg[2] << 8;
    work->fallY = 0;
    work->radius = arg[0] << 8;
    work->angle = arg[3];
    work->speed = GetRandom() % 0x181 + 0x100;
    work->x = 0;
    work->y = 0;
    work->unk_38 = 0;
    work->fallSpeed = -(GetRandom() % 0x81 + 0x200);
    gCardListWork->effectCount++;
}

void PremireEffectConvergeInit(PremiumCardEffectWork* work, s16* arg) {
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCardSparklePalette, sizeof(gCardSparklePalette));
    SetObjTileSource(work->tiles, gCardSparkleTiles);
    AnimInit(&work->anim, gCardSparkleAnims, gCardSparkleFrames);
    AnimStart(&work->anim, GetRandom() % 3, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->centerX = arg[1] << 8;
    work->centerY = arg[2] << 8;
    work->fallY = 0;
    work->radius = arg[0] << 8;
    work->angle = arg[3];
    work->speed = GetRandom() % 0x81 + 0x200;
    work->x = 0;
    work->y = 0;
    work->unk_38 = 0;
    work->fallSpeed = -(GetRandom() % 0x81 + 0x200);
    PremireEffectSetOrbitPos(work);
    gCardListWork->effectCount++;
}

s32 PremireEffectSpiralUpdate(PremiumCardEffectWork* work) {
    PremireEffectSetOrbitPos(work);
    work->angle += 8;

    if (work->radius > 0) {
        work->radius += -0x180;
        work->gfx = AnimUpdate(&work->anim);
        return 1;
    }

    return 0;
}

s32 Premire_EFFECT2_1(PremiumCardEffectWork* work) {
    PremireEffectMoveFalling(work);
    work->gfx = AnimUpdate(&work->anim);

    if (work->y > 0xB400) {
        return 0;
    }

    return 1;
}

s32 PremireEffectConvergeUpdate(PremiumCardEffectWork* work) {
    PremireEffectMoveToCenter(work);
    work->angle += 8;
    work->gfx = AnimUpdate(&work->anim);

    if (work->radius <= 0x800) {
        return 0;
    }

    return 1;
}

void PremireEffectDraw(PremiumCardEffectWork* work) {
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 0);
}

void PremireEffectDestroy(PremiumCardEffectWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    gCardListWork->effectCount--;
}

void PremireEffectSetOrbitPos(PremiumCardEffectWork* work) {
    work->x = SIN(work->angle) * (work->radius >> 8) + work->centerX;
    work->y = -COS(work->angle) * (work->radius >> 8) + work->centerY;
}

void PremireEffectMoveFalling(PremiumCardEffectWork* work) {
    work->fallSpeed += 30;
    work->fallY += work->fallSpeed;
    work->centerX += SIN(work->angle) * (work->speed >> 8);
    work->centerY += -COS(work->angle) * (work->speed >> 8);
    work->x = work->centerX;
    work->y = work->centerY + work->fallY;
}

void PremireEffectMoveToCenter(PremiumCardEffectWork* work) {
    s32 speed;
    s32 step;

    work->vx = work->centerX - work->x;
    work->vy = work->centerY - work->y;
    work->radius = NormalizeVector2D8(&work->vx, &work->vy);
    speed = work->speed;
    step = speed >> 8;
    work->x += work->vx * step;
    work->y += work->vy * step;

    if (work->radius > 0) {
        work->speed = speed - 2;
    }
}

void InitPrintLayer(u8 bg) {
    PrintLine** lines;

    SetBgScroll(bg, 0, 0);
    SetBackdropColor(0, 0, 0);
    LoadBgTiles(bg, gPrintFontTiles, sizeof(gPrintFontTiles));
    LoadBgMap(bg, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgPalette(bg, gPrintFontPalettes, sizeof(gPrintFontPalettes));
    EnableBg(bg);
    sPrintBg = bg;
    lines = &sPrintLines;
    *lines = EwramAlloc(sizeof(PrintLine) * 32);
}

void FreePrintLayer() {
    EwramFree(sPrintLines);
}

u8 GetStringLength(const u8* text) {
    u8 n;

    n = 0;

    if (text == NULL) {
        return 0;
    }

    while (*text++ != 0) {
        if (*text != 0) {
            n++;
        }
    }

    return n + 1;
}

void PrintString(u8 x, u8 y, u8 color, const u8* text) {
    u8 n;
    u8 i;

    if (sPrintLineCount < 32) {
        n = GetStringLength(text);

        if (n > 32) {
            n = 32;
        }

        for (i = 0; i < n; i++) {
            sPrintLines[sPrintLineCount].tilemap[i] = text[i];
            sPrintLines[sPrintLineCount].tilemap[i] |= color << 12;
        }

        sPrintLines[sPrintLineCount].x = x;
        sPrintLines[sPrintLineCount].y = y;
        sPrintLines[sPrintLineCount].palette = color;
        sPrintLines[sPrintLineCount].length = n;
        RequestTilemapRectCopy(sPrintLines[sPrintLineCount].tilemap, GetBgScreenBase(sPrintBg), 0, 0,
                      sPrintLines[sPrintLineCount].x,
                      sPrintLines[sPrintLineCount].y,
                      sPrintLines[sPrintLineCount].length, 1);
        sPrintLineCount++;
    }
}

void ResetPrintLines() {
    sPrintLineCount = 0;
}

void ClearPrintLines() {
    s16 i;

    for (i = 0; i < sPrintLineCount; i++) {
        sPrintLines[i].length = 0;
        sPrintLines[i].x = 0;
        sPrintLines[i].y = 0;
        sPrintLines[i].palette = 0;
        sPrintLines[i].tilemap[0] = 0;
    }

    sPrintLineCount = 0;
}

void PrintNumber(u16 x, u16 y, u16 color, s32 value) {
    s32 digits[8];
    u8 text[10];
    s32 i;

    if (value >= 0) {
        digits[0] = value / 10000000;
        digits[1] = value / 1000000 - 10 * digits[0];
        digits[2] = value / 100000 - 100 * digits[0] - 10 * digits[1];
        digits[3] = value / 10000 - 1000 * digits[0] - 100 * digits[1] - 10 * digits[2];
        digits[4] = value / 1000 - 10000 * digits[0] - 1000 * digits[1] - 100 * digits[2] - 10 * digits[3];
        digits[5] = value / 100 - 100000 * digits[0] - 10000 * digits[1] - 1000 * digits[2] - 100 * digits[3] - 10 * digits[4];
        digits[6] = value / 10 - 1000000 * digits[0] - 100000 * digits[1] - 10000 * digits[2] - 1000 * digits[3] - 100 * digits[4] - 10 * digits[5];
        digits[7] = value - (10000000 * digits[0] + 1000000 * digits[1] + 100000 * digits[2] + 10000 * digits[3] + 1000 * digits[4] + 100 * digits[5] + 10 * digits[6]);
        text[0] = digits[0] + '0';
        text[1] = digits[1] + '0';
        text[2] = digits[2] + '0';
        text[3] = digits[3] + '0';
        text[4] = digits[4] + '0';
        text[5] = digits[5] + '0';
        text[6] = digits[6] + '0';
        text[7] = digits[7] + '0';
        text[8] = 0;
        i = 0;

        if (text[i] <= '0') {
            do {
                i++;

                if (i > 6) {
                    break;
                }
            } while (text[i] <= '0');
        }

        PrintString(x, y, color, &text[i]);
    } else {
        digits[0] = value / -10000000;
        digits[1] = value / -1000000 - 10 * digits[0];
        digits[2] = value / -100000 - 100 * digits[0] - 10 * digits[1];
        digits[3] = value / -10000 - 1000 * digits[0] - 100 * digits[1] - 10 * digits[2];
        digits[4] = value / -1000 - 10000 * digits[0] - 1000 * digits[1] - 100 * digits[2] - 10 * digits[3];
        digits[5] = value / -100 - 100000 * digits[0] - 10000 * digits[1] - 1000 * digits[2] - 100 * digits[3] - 10 * digits[4];
        digits[6] = value / -10 - 1000000 * digits[0] - 100000 * digits[1] - 10000 * digits[2] - 1000 * digits[3] - 100 * digits[4] - 10 * digits[5];
        digits[7] = -value - (10000000 * digits[0] + 1000000 * digits[1] + 100000 * digits[2] + 10000 * digits[3] + 1000 * digits[4] + 100 * digits[5] + 10 * digits[6]);
        text[0] = '-';
        text[1] = digits[0] + '0';
        text[2] = digits[1] + '0';
        text[3] = digits[2] + '0';
        text[4] = digits[3] + '0';
        text[5] = digits[4] + '0';
        text[6] = digits[5] + '0';
        text[7] = digits[6] + '0';
        text[8] = digits[7] + '0';
        text[9] = 0;
        i = 1;

        if (text[i] <= '0') {
            do {
                i++;

                if (i > 7) {
                    break;
                }
            } while (text[i] <= '0');
        }

        text[--i] = '-';
        PrintString(x, y, color, &text[i]);
    }
}

void PrintBinary16(u16 x, u16 y, u16 color, u16 bits) {
    u16 masked[16];
    u8 text[17];
    u16 i;
    u16 j;

    for (i = 0, j = 15; i < ARRAY_COUNT(masked); i++, j--) {
        masked[i] = bits & (1 << i);
        text[j] = (masked[i] >> i) + '0';
    }

    text[16] = 0;
    PrintString(x, y, color, text);
}

void PrintHex32(u16 x, u16 y, u16 color, u32 value) {
    u8 text[11];
    s32 i;

    text[0] = '0';
    text[1] = 'x';
    text[2] = value >> 28;
    text[3] = (value & 0x0F000000) >> 24;
    text[4] = (value & 0x00F00000) >> 20;
    text[5] = (value & 0x000F0000) >> 16;
    text[6] = (value & 0x0000F000) >> 12;
    text[7] = (value & 0x00000F00) >> 8;
    text[8] = (value & 0x000000F0) >> 4;
    text[9] = value & 0x0000000F;

    for (i = 0; i < 8; i++) {
        text[i + 2] += text[i + 2] <= 9 ? '0' : '7';
    }

    text[10] = 0;
    PrintString(x, y, color, text);
}

TaskDesc gTaskDescCardName = {
    "CardName",
    (TaskInitFunc)CardName_0,
    (TaskUpdateFunc)CardName_1,
    (TaskDrawFunc)CardName_2,
    (TaskDestroyFunc)CardName_3,
    sizeof(CardNameWork),
};

TaskDesc gTaskDescPremireEFFECTSpiral = {
    "Premire_EFFECT",
    (TaskInitFunc)PremireEffectInit,
    (TaskUpdateFunc)PremireEffectSpiralUpdate,
    (TaskDrawFunc)PremireEffectDraw,
    (TaskDestroyFunc)PremireEffectDestroy,
    sizeof(PremiumCardEffectWork),
};

TaskDesc gTaskDescPremireEFFECT2 = {
    "Premire_EFFECT2",
    (TaskInitFunc)PremireEffectInit,
    (TaskUpdateFunc)Premire_EFFECT2_1,
    (TaskDrawFunc)PremireEffectDraw,
    (TaskDestroyFunc)PremireEffectDestroy,
    sizeof(PremiumCardEffectWork),
};

TaskDesc gTaskDescPremireEFFECTConverge = {
    "Premire_EFFECT",
    (TaskInitFunc)PremireEffectConvergeInit,
    (TaskUpdateFunc)PremireEffectConvergeUpdate,
    (TaskDrawFunc)PremireEffectDraw,
    (TaskDestroyFunc)PremireEffectDestroy,
    sizeof(PremiumCardEffectWork),
};
