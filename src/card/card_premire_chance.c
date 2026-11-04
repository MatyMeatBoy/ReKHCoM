/**
 * card_premire_chance.c
 * Premium Bonus Roulette Cards
 */

#include "fade.h"
#include "obj_api.h"
#include "engine_math.h"
#include "listpool.h"
#include "anim.h"
#include "obj.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_card_pictures.h"
#include "card_def_data.h"
#include "card_types.h"
#include "types.h"
#include <stddef.h>
#include "card_premire_chance.h"
#include "sprite_palettes.h"
#include "gba/defines.h"
#include "macros.h"

static const s16 sPremireChanceCardAngles[9] = { 0, 11, 23, 34, 46, 57, 68, 79, 90 };

static const u8 sPremireChanceCardPriorities[9] = { 6, 4, 2, 0, 2, 4, 6, 8, 12 };

void PremireChanceCard_0(PremireChanceCardWork* work, CardSlot* slot) {
    const CardDef* def;

    work->gfxLoaded = FALSE;
    work->tiles4 = NULL;

    if (slot->unk_06 < ARRAY_COUNT(sPremireChanceCardAngles)) {
        work->angle = sPremireChanceCardAngles[slot->unk_06];
    } else {
        work->angle = -0x20;
    }

    work->position = slot->unk_06;
    work->deckIndex = slot->index;
    def = &gCardDefs[slot->cardId];
    work->cardDef = def;

    if (def->flags & (CARD_DEF_FLAG_SUMMON | CARD_DEF_FLAG_FRIEND)) {
        work->cardBack = &gCardBacks[CARD_CATEGORY_MAGIC];
    } else {
        work->cardBack = &gCardBacks[def->category];
    }

    work->radius = 0;
    UpdatePremireChanceCardPos(work);
    ListNodeInit(&work->node, &gCardListWork->cards, work);
    ListPoolAppend(&work->node, &gCardListWork->cards);
    work->state = PREMIRE_CHANCE_CARD_STATE_DEAL;
    work->scaleX = Q_8_8(1);
    work->scaleY = Q_8_8(1);
    work->offsetX = 0;
    work->offsetY = 0;
    work->steps = 32;
    work->premium = FALSE;
}

u8 UpdatePremireChanceCardSpin(PremireChanceCardWork* work, void* task) {
    s32 angle;
    u8 (*fn)(PremireChanceCardWork*, void*);
    u16 lim;

    angle = work->angle << 8;

    if (work->position <= 8) {
        ApproachValue(&angle, sPremireChanceCardAngles[work->position] << 8, work->steps);
        work->angle = angle >> 8;
    } else {
        lim = 0xFFE0;
        work->angle = lim;
    }

    if (work->steps != 0) {
        work->steps--;
    }

    UpdatePremireChanceCardPos(work);

    if (IsPremireChanceCardOnScreen(work)) {
        LoadPremireChanceCardGfx(work);
    } else {
        ReleasePremireChanceCardGfx(work);
    }

    if (work->position == 3) {
        gCardListWork->selectedCard = work;
    }

    switch (work->state) {
    case PREMIRE_CHANCE_CARD_STATE_TO_CENTER:
        gCardListWork->unk_29 = 0;
        work->steps = 8;
        fn = UpdatePremireChanceCardToCenter;
        SetTaskUpdate(task, (TaskUpdateFunc)fn);
        return fn(work, task);
    case PREMIRE_CHANCE_CARD_STATE_MOVE_AWAY:
        work->steps = 10;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceCardMoveAway);
        break;
    }

    return 1;
}

u8 PremireChanceCard_1(PremireChanceCardWork* work, void* task) {
    s32 radius;

    radius = work->radius << 8;
    ApproachValue(&radius, 0x6800, work->steps);
    work->radius = radius >> 8;
    work->steps--;
    UpdatePremireChanceCardPos(work);

    if (work->position <= 8) {
        LoadPremireChanceCardGfx(work);
    } else {
        ReleasePremireChanceCardGfx(work);
    }

    if (work->steps == 0) {
        work->state = PREMIRE_CHANCE_CARD_STATE_SPIN;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceCardSpin);
    }

    return 1;
}

void PremireChanceCard_2(PremireChanceCardWork* work) {
    ObjAffine* affine;

    if (work->state == PREMIRE_CHANCE_CARD_STATE_HIDDEN) {
        return;
    }

    if (work->gfxLoaded) {
        affine = AllocObjAffine(0, work->scaleX, work->scaleY, TRUE);
        DrawSprite(work->x + work->offsetX, work->y + work->offsetY, work->cardDef->gfx, work->tiles, work->palette2, affine, SPRITE_PRIORITY(1),
                   sPremireChanceCardPriorities[work->position] + 70);
        DrawSprite(work->x + work->offsetX, work->y + work->offsetY, work->cardBack->gfx, work->tiles2, work->palette3, affine, SPRITE_PRIORITY(1),
                   sPremireChanceCardPriorities[work->position] + 69);

        if (!work->premium) {
            DrawSprite(work->x + work->offsetX, work->y + work->offsetY, gCardValueDigitFrames[work->cardDef->value], work->tiles3, work->palette3,
                       affine, SPRITE_PRIORITY(1), sPremireChanceCardPriorities[work->position] + 68);
        } else {
            DrawSprite(work->x + work->offsetX, work->y + work->offsetY, gCardPremiumValueDigitFrames[work->cardDef->value], work->tiles5, work->palette,
                       affine, SPRITE_PRIORITY(1), sPremireChanceCardPriorities[work->position] + 68);
        }
    }

    if (work->state == PREMIRE_CHANCE_CARD_STATE_TO_CENTER && work->tiles4 != NULL) {
        DrawSprite(work->x + work->offsetX, work->y + work->offsetY, work->gfx, work->tiles4, work->palette3, NULL, SPRITE_PRIORITY(1),
                   sPremireChanceCardPriorities[work->position] + 67);
    }
}

void PremireChanceCard_3(PremireChanceCardWork* work) {
    ReleasePremireChanceCardGfx(work);

    if (work->tiles4 != NULL) {
        ReleaseObjTiles(work->tiles4);
    }
}

void UpdatePremireChanceCardPos(PremireChanceCardWork* work) {
    work->x = (gSineTable[(u8)work->angle] * work->radius) >> 8;
    work->y = ((-gSineTable[(u8)work->angle + 64] * work->radius) >> 8) + 160;
}

u8 IsPremireChanceCardOnScreen(PremireChanceCardWork* work) {
    if (work->x >= 0) {
        if (work->x <= DISPLAY_WIDTH) {
            if (work->y >= 0) {
                if (work->y <= DISPLAY_HEIGHT) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void LoadPremireChanceCardGfx(PremireChanceCardWork* work) {
    if (!work->gfxLoaded) {
        work->tiles2 = LoadObjTiles(work->cardBack->tiles, 0x280);
        work->palette3 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
        work->tiles = LoadObjTiles(work->cardDef->tiles, 0x200);
        work->palette2 = LoadObjPalette(work->cardDef->palette, 32);
        work->tiles3 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
        work->tiles5 = LoadObjTiles(gCardPremiumValueDigitTiles, sizeof(gCardPremiumValueDigitTiles));
        work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
        FadeSetPaletteExcluded(BIOS_PTR(work->palette3)->index + 16, TRUE);
        FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 16, TRUE);
        work->gfxLoaded = TRUE;
    }
}

void ReleasePremireChanceCardGfx(PremireChanceCardWork* work) {
    if (work->gfxLoaded) {
        ReleaseObjTiles(work->tiles2);
        ReleaseObjPalette(work->palette3);
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette2);
        ReleaseObjTiles(work->tiles3);
        ReleaseObjPalette(work->palette);
        ReleaseObjTiles(work->tiles5);
        work->gfxLoaded = FALSE;
    }
}

u8 UpdatePremireChanceCardToCenter(PremireChanceCardWork* work, void* task) {
    s32 x;
    s32 y;

    x = work->x << 8;
    y = work->y << 8;
    ApproachValue(&x, 0x7800, work->steps);
    ApproachValue(&y, 0x4600, work->steps);
    work->x = x >> 8;
    work->y = y >> 8;
    work->steps--;

    if (work->steps == 0) {
        SetTaskUpdate(task, (TaskUpdateFunc)StartPremireChanceCardAnim);
    }

    return 1;
}

u8 StartPremireChanceCardAnim(PremireChanceCardWork* work, void* task) {
    work->tiles4 = AllocObjTiles(640, NULL);
    SetObjTileSource(work->tiles4, gCardPremiumTiles);
    AnimInit(&work->anim, gCardPremiumAnims, gCardPremiumFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceCardAnim);
    return 1;
}

s32 UpdatePremireChanceCardAnim(PremireChanceCardWork* work) {
    work->gfx = AnimUpdate(&work->anim);

    if (work->anim.timer == 0 && work->anim.frame == 4) {
        work->premium = TRUE;
    }

    return 1;
}

u8 UpdatePremireChanceCardMoveAway(PremireChanceCardWork* work, void* task) {
    s32 angle;

    angle = work->angle << 8;

    if (work->position <= 2) {
        ApproachValue(&angle, -0x2000, work->steps);
    }

    if ((u8)work->position >= 4 && (u8)work->position <= 7) {
        ApproachValue(&angle, 0x4400, work->steps);
    }

    work->angle = angle >> 8;

    if (work->steps != 0) {
        work->steps--;
    }

    UpdatePremireChanceCardPos(work);

    if (IsPremireChanceCardOnScreen(work)) {
        LoadPremireChanceCardGfx(work);
    } else {
        ReleasePremireChanceCardGfx(work);
    }

    return 1;
}

TaskDesc gTaskDescPremireChanceCard = {
    "Premire Chance",
    (TaskInitFunc)PremireChanceCard_0,
    (TaskUpdateFunc)PremireChanceCard_1,
    (TaskDrawFunc)PremireChanceCard_2,
    (TaskDestroyFunc)PremireChanceCard_3,
    sizeof(PremireChanceCardWork),
};
