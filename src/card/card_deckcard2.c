/**
 * card_deckcard2.c
 * Deck Grid Card Sprite
 */

#include "obj_api.h"
#include "listpool.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_card_pictures.h"
#include "card_def_data.h"
#include "card_types.h"
#include "card_ui_types.h"
#include "types.h"
#include <stddef.h>
#include "card_deckcard2.h"
#include "sprite_palettes.h"
#include "gba/defines.h"
#include "macros.h"

const s16 gDeckGridColumnX[3] = { 13, 36, 59 };

const s16 gDeckGridRowY[4] = { 47, 73, 99, 125 };

const s16 gCollectionGridColumnX[3] = { 181, 204, 227 };

const s16 gCollectionGridRowY[4] = { 47, 73, 99, 125 };

void DeckCard2_0(DeckCard2Work* work, DeckCard2Args* args) {
    work->args = *args;
    work->tiles = NULL;
    work->palette = NULL;
    work->tiles2 = NULL;
    work->palette2 = NULL;
    work->flags = 0;

    switch (work->args.panel) {
    case DECK_CARD2_PANEL_DECK:
        if ((u16)work->args.row < ARRAY_COUNT(gDeckGridRowY)) {
            work->x = gDeckGridColumnX[work->args.col] << 8;
            work->y = gDeckGridRowY[work->args.row] << 8;
        } else {
            work->x = gDeckGridColumnX[work->args.col] << 8;
            work->y = 0x20000;
        }

        break;
    case DECK_CARD2_PANEL_COLLECTION:
        if ((u16)work->args.row < ARRAY_COUNT(gCollectionGridRowY)) {
            work->x = gCollectionGridColumnX[work->args.col] << 8;
            work->y = gCollectionGridRowY[work->args.row] << 8;
        } else {
            work->x = gCollectionGridColumnX[work->args.col] << 8;
            work->y = 0x20000;
        }

        break;
    }

    if (work->args.cardId != CARD_ID_NONE) {
        if (!(work->args.cardId & CARD_FLAG_PREMIUM)) {
            work->premium = FALSE;
        } else {
            work->premium = TRUE;
        }

        work->cardDef = &gCardDefs[work->args.cardId & CARD_ID_MASK];

        if (work->cardDef->flags & (CARD_DEF_FLAG_SUMMON | CARD_DEF_FLAG_FRIEND)) {
            work->cardBack = &gCardBacks[CARD_CATEGORY_ENEMY];
        } else {
            work->cardBack = &gCardBacks[work->cardDef->category];
        }
    }

    work->done = FALSE;
    ListNodeInit(&work->node, work->args.pool, work);
    ListPoolAppend(&work->node, work->args.pool);
}

u8 DeckCard2_1(DeckCard2Work* work) {
    if (work->done == TRUE) {
        return 0;
    }

    switch (work->args.panel) {
    case DECK_CARD2_PANEL_DECK:
        if ((u16)work->args.row < ARRAY_COUNT(gDeckGridRowY)) {
            work->x = gDeckGridColumnX[work->args.col] << 8;
            work->y = gDeckGridRowY[work->args.row] << 8;
        } else {
            work->x = gDeckGridColumnX[work->args.col] << 8;
            work->y = 0x20000;
        }

        break;
    case DECK_CARD2_PANEL_COLLECTION:
        if ((u16)work->args.row < ARRAY_COUNT(gCollectionGridRowY)) {
            work->x = gCollectionGridColumnX[work->args.col] << 8;
            work->y = gCollectionGridRowY[work->args.row] << 8;
        } else {
            work->x = gCollectionGridColumnX[work->args.col] << 8;
            work->y = 0x20000;
        }

        break;
    }

    if (DeckCard2IsOnScreen(work)) {
        DeckCard2LoadGfx(work);
    } else {
        DeckCard2ReleaseGfx(work);
    }

    // @bug No return value (the caller reads it).
#ifdef PLATFORM_PC
    return 1; /* x86 has no leftover r0: keep the task running, as on the GBA */
#endif
}

void DeckCard2_2(DeckCard2Work* work) {
    if (!(work->flags & DECK_CARD2_FLAG_GFX_LOADED)) {
        return;
    }

    if (work->tiles != NULL && work->palette != NULL) {
        DrawSprite(work->x >> 8, work->y >> 8, work->cardDef->gfx2, work->tiles, work->palette, NULL, 0, 0x33);

        if (work->premium) {
            DrawSprite(work->x >> 8, work->y >> 8, gCardUiSpriteState.gfx, gCardUiSpriteState.tiles, gCardUiSpriteState.palette, NULL, 0, 0x28);
        }
    }

    if (work->args.panel == DECK_CARD2_PANEL_DECK && work->cardDef->category != CARD_CATEGORY_ENEMY) {
        DrawSprite((work->x >> 8) - 3, (work->y >> 8) - 4, gCardValueDigitFrames[work->cardDef->value], work->tiles2, work->palette2, NULL, 0, 0x31);
    }
}

void DeckCard2_3(DeckCard2Work* work) {
    DeckCard2ReleaseGfx(work);
    ListPoolRemove(&work->node, work->args.pool);
}

void DeckCard2LoadGfx(DeckCard2Work* work) {
    if (work->args.cardId == CARD_ID_NONE) {
        return;
    }

    if (work->flags & DECK_CARD2_FLAG_GFX_LOADED) {
        return;
    }

    work->palette2 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles = LoadObjTiles(work->cardDef->tiles2, 0x200);
#ifdef PLATFORM_PC
    {
        /* Options > Experimental > gold premium cards: the premium card's frame in gold (port_ui.c) */
        extern int PortGoldPremiumPalette(const void* tiles, const void* palette, u16* out);
        u16 gold[16];
        if (work->premium && PortGoldPremiumPalette(work->cardDef->tiles2, work->cardDef->palette2, gold))
            work->palette = LoadObjPalette(gold, 32);
        else
            work->palette = LoadObjPalette(work->cardDef->palette2, 32);
    }
#else
    work->palette = LoadObjPalette(work->cardDef->palette2, 32);
#endif
    work->tiles2 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));

    if (work->tiles != NULL && work->palette != NULL) {
        work->flags |= DECK_CARD2_FLAG_GFX_LOADED;
    }
}

void DeckCard2ReleaseGfx(DeckCard2Work* work) {
    if (work->flags & DECK_CARD2_FLAG_GFX_LOADED) {
        ReleaseObjPalette(work->palette2);
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ReleaseObjTiles(work->tiles2);
        work->flags &= ~DECK_CARD2_FLAG_GFX_LOADED;
        work->tiles = NULL;
        work->palette = NULL;
        work->tiles2 = NULL;
        work->palette2 = NULL;
    }
}

u8 DeckCard2IsOnScreen(DeckCard2Work* work) {
    s16 x;
    s16 y;

    x = work->x >> 8;
    y = work->y >> 8;

    if (x < 0) {
        return FALSE;
    }

    if (x > DISPLAY_WIDTH) {
        return FALSE;
    }

    if (y < 0) {
        return FALSE;
    }

    if (y > DISPLAY_HEIGHT) {
        return FALSE;
    }

    return TRUE;
}

TaskDesc gTaskDescDeckCard2 = {
    "DeckCard2",
    (TaskInitFunc)DeckCard2_0,
    (TaskUpdateFunc)DeckCard2_1,
    (TaskDrawFunc)DeckCard2_2,
    (TaskDestroyFunc)DeckCard2_3,
    sizeof(DeckCard2Work),
};
