/**
 * card_deckmenu2.c
 * Deck Data and Sora Deck Menu
 */

#include "macros.h"
#ifdef PLATFORM_PC
#include <string.h>
#include "display.h"
#include "port/port_hdsprites.h"
#endif
#include "card_localized_data.h"
#include "msg_localized_data.h"
#include "registration_data.h"
#include "system_state.h"
#include "player_progression.h"
#include "m4a_song.h"
#include "game_state.h"
#include "text.h"
#include "monsgage.h"
#include "fade.h"
#include "obj_api.h"
#include "display.h"
#include "engine_math.h"
#include "listpool.h"
#include "anim.h"
#include "obj.h"
#include "text_types.h"
#include "taskpool.h"
#include "key.h"
#include "malloc.h"
#include "card.h"
#include "sprites_card.h"
#include "sprites_deck_menu.h"

#ifdef PLATFORM_PC
#include "engine.h"
#include <stdlib.h>
void PortLog(const char* format, ...);
void PortRegisterBgTextTile(void* address, int code, int style, const void* source32);
void PortClearBgTextTiles(void);
#endif

/* Keep cartridge tile transfers intact; attach optional HD metadata to
   numeric cells. The renderer checks the tile signature before using it. */
#ifdef PLATFORM_PC
static void PortCopyDeckDigit(const void* source, void* destination, u16 size) {
    RequestDma3Copy(source, destination, size);
#ifdef PLATFORM_PC
    {
        int digit = ((const u8*)source - gDeckCountDigitTiles) / 32 - 1;
        if (size == 32 && digit >= 0 && digit <= 9)
            PortRegisterBgTextTile(destination, '0' + digit, 2, source);
    }
#endif
}

/* Statistics in category and value grids are text, not card artwork.
   Zero variants have their own palette; keep their exact source tile too. */
static void PortCopyDeckGridNumber(const void* source, void* destination, u16 size,
                                   int digit, const char* family, int sourceOffset, int style) {
    RequestDma3Copy(source, destination, size);
#ifdef PLATFORM_PC
    if (size == 32 && digit >= 0 && digit <= 9) {
        PortRegisterBgTextTile(destination, '0' + digit, style, source);
        if (getenv("KHCOM_NUMERIC_TRACE"))
            PortLog("numeric-text: family=%s byte=%d code=%d destination=%p size=%u style=%x",
                    family, sourceOffset, '0' + digit, destination, (unsigned)size, (unsigned)style);
    }
#endif
}

static void PortCopyDeckCategoryDigit(const void* source, void* destination, u16 size) {
    int zero = source == gDeckCategoryZeroTiles;
    int offset = zero ? 0 : (const u8*)source - gDeckCategoryDigitTiles;
    PortCopyDeckGridNumber(source, destination, size, zero ? 0 : offset / 32 - 1,
                          zero ? "deck-category-zero" : "deck-category", offset,
                          2 | ((zero ? 15 : 12) << 8) | ((zero ? 3 : 13) << 12) | (1 << 16));
}

static void PortCopyDeckValueDigit(const void* source, void* destination, u16 size) {
    int zero = source == gDeckValueZeroTiles;
    int offset = zero ? 0 : (const u8*)source - gDeckValueDigitTiles;
    PortCopyDeckGridNumber(source, destination, size, zero ? 0 : offset / 32 - 3,
                          zero ? "deck-value-zero" : "deck-value", offset,
                          2 | ((zero ? 4 : 2) << 8) | (3 << 12) | (1 << 16));
}
#else
/* the cartridge copies these tiles directly */
#define PortCopyDeckDigit RequestDma3Copy
#define PortCopyDeckCategoryDigit RequestDma3Copy
#define PortCopyDeckValueDigit RequestDma3Copy
#endif

#include "sprites_card_pictures.h"
#include "card_ids.h"
#include "gba/keys.h"
#include "songs.h"
#include "card_deck_data.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "card_ui_types.h"
#include "gba/defines.h"
#include "player_progression_types.h"
#include "types.h"
#include "gba/macro.h"
#include "sprite_palettes.h"
#include <stddef.h>
#include "card_deckmenu2.h"
#include "card_map_anim.h"
#include "ui_text.h"
#include "default_bg_map.h"
#include "world_types.h"

const u16 gRikuDeckCards0[21] = {
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_HI_POTION, 7),
};

const u16 gRikuDeckCards1[20] = {
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
};

const u16 gRikuDeckCards2[21] = {
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 0),
};

const u16 gRikuDeckCards3[12] = {
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
};

const u16 gRikuDeckCards4[20] = {
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 1),
};

const u16 gRikuDeckCards5[18] = {
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
};

const u16 gRikuDeckCards6[17] = {
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 9),
};

const u16 gRikuDeckCards7[16] = {
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_POTION, 9),
};

const u16 gRikuDeckCards8[19] = {
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 0),
};

const u16 gRikuDeckCards9[5] = {
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 8),
};

const u16 gRikuDeckCards10[25] = {
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 5),
    CARD_ID(CARD_SOUL_EATER, 3),
    CARD_ID(CARD_SOUL_EATER, 1),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 4),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 2),
    CARD_ID(CARD_SOUL_EATER, 9),
};

const u16 gRikuDeckCards11[30] = {
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 9),
    CARD_ID(CARD_SOUL_EATER, 6),
    CARD_ID(CARD_SOUL_EATER, 7),
    CARD_ID(CARD_SOUL_EATER, 8),
    CARD_ID(CARD_HI_POTION, 9),
    CARD_ID(CARD_HI_POTION, 0),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 0),
    CARD_ID(CARD_SOUL_EATER, 0),
};

const u16 gRikuDeckEnemyCard0 = CARD_FAT_BANDIT_3;

const u16 gRikuDeckEnemyCard1 = 492;

const u16 gRikuDeckEnemyCard2 = 471;

const u16 gRikuDeckEnemyCard3 = CARD_LARGE_BODY_1;

const u16 gRikuDeckEnemyCard4 = CARD_SEARCH_GHOST_1;

const u16 gRikuDeckEnemyCard5 = CARD_WIGHT_KNIGHT_2;

const u16 gRikuDeckEnemyCard6 = CARD_PIRATE_1;

const u16 gRikuDeckEnemyCard7 = CARD_DEFENDER_5;

const u16 gRikuDeckEnemyCard9 = 450;

const u16 gRikuDeckCardCounts[12] = {
    21,
    20,
    21,
    12,
    20,
    18,
    17,
    16,
    19,
    5,
    25,
    30,
};

const u16 gRikuDeckEnemyCardCounts[12] = {
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    1,
    0,
    0,
};

const u16 gUnk_090356EA = 0;

static u8 sActiveDeck;
static u16 sUnk_02034AB2;

CardUiSpriteState gCardUiSpriteState EWRAM_COMMON(16);

void CountCardsNotInDeckByKind(CardKindEntry* out, u8 deck, u8 thisDeckOnly, u16 entryCount, void* p) {
    u16 mask;
    u16 i;
    s32 x;

    mask = 0;

    if (thisDeckOnly == TRUE) {
        switch (deck) {
        case 0:
            mask = CARD_FLAG_IN_DECK_1;
            break;
        case 1:
            mask = CARD_FLAG_IN_DECK_2;
            break;
        case 2:
            mask = CARD_FLAG_IN_DECK_3;
            break;
        }
    } else {
        mask = CARD_FLAG_IN_ANY_DECK;
    }

    for (i = 0; i < gCardCount; i++) {
        if (gCardCollection[i] == CARD_COLLECTION_EMPTY) {
            continue;
        }

        if (gCardCollection[i] & mask) {
            continue;
        }

        if (!(gCardCollection[i] & CARD_FLAG_PREMIUM)) {
            x = gCardDefs[gCardCollection[i] & CARD_ID_MASK].kind;
            out[x].kind = x;
            out[x].count++;
        } else {
            x = gCardDefs[gCardCollection[i] & CARD_ID_MASK].kind;
            out[x + 0x8F].kind = x + 0x8F;
            out[x + 0x8F].count++;
        }
    }
}

u16 ListCardsNotInDeckByKind(CardKindEntry* out, u8 deck, u8 thisDeckOnly, u16 entryCount, void* p) {
    u16 mask;
    u16 i;
    u16 count;
    u32 id;
    u16 x;

    mask = 0;

    if (thisDeckOnly == TRUE) {
        switch (deck) {
        case 0:
            mask = CARD_FLAG_IN_DECK_1;
            break;
        case 1:
            mask = CARD_FLAG_IN_DECK_2;
            break;
        case 2:
            mask = CARD_FLAG_IN_DECK_3;
            break;
        }
    } else {
        mask = CARD_FLAG_IN_ANY_DECK;
    }

    for (i = 0, count = 0; i < entryCount; i++) {
        if (out[i].count != 0) {
            out[i].indices = EwramAlloc(out[i].count * sizeof(u16));
            count++;
        } else {
            out[i].indices = NULL;
        }

        out[i].indexCount = 0;
    }

    for (i = 0; i < gCardCount; i++) {
        if (gCardCollection[i] == CARD_COLLECTION_EMPTY) {
            continue;
        }

        if (gCardCollection[i] & mask) {
            continue;
        }

        if (!(gCardCollection[i] & CARD_FLAG_PREMIUM)) {
            id = gCardCollection[i] & CARD_ID_MASK;
            x = gCardDefs[id].kind;

            if (id > CARD_ID_LAST_VALUED) {
                out[x].valueCounts[0]++;
            } else {
                out[x].valueCounts[gCardDefs[id].value]++;
            }

            out[x].indices[out[x].indexCount++] = i;
        } else {
            id = gCardCollection[i] & CARD_ID_MASK;
            x = gCardDefs[id].kind + 0x8F;

            if (id > CARD_ID_LAST_VALUED) {
                out[x].valueCounts[0]++;
            } else {
                out[x].valueCounts[gCardDefs[id].value]++;
            }

            out[x].indices[out[x].indexCount++] = i;
        }
    }

    return count;
}

void func_08084FA8() {
}

u16 CountCollectionCardsOfCategory(u8 category) {
    u16 count;
    u16 i;

    count = 0;

    for (i = 0; i < gCardCount; i++) {
        if (gCardCollection[i] != CARD_COLLECTION_EMPTY) {
            if (gCardDefs[gCardCollection[i] & CARD_ID_MASK].category == category) {
                count++;
            }
        }
    }

    return count;
}

void CountCardsNotInDeckByCategory(u8 deck, u16* out) {
    u16 mask;
    u16 i;

    mask = 0;
    out[CARD_CATEGORY_ATTACK] = 0;
    out[CARD_CATEGORY_MAGIC] = 0;
    out[CARD_CATEGORY_ITEM] = 0;
    out[CARD_CATEGORY_ENEMY] = 0;

    switch (deck) {
    case 0:
        mask = CARD_FLAG_IN_DECK_1;
        break;
    case 1:
        mask = CARD_FLAG_IN_DECK_2;
        break;
    case 2:
        mask = CARD_FLAG_IN_DECK_3;
        break;
    case DECK_ANY:
        mask = CARD_FLAG_IN_ANY_DECK;
        break;
    }

    for (i = 0; i < gCardCount; i++) {
        if (gCardCollection[i] != CARD_COLLECTION_EMPTY) {
            if (!(gCardCollection[i] & mask)) {
                out[gCardDefs[gCardCollection[i] & CARD_ID_MASK].category]++;
            }
        }
    }
}

void ClearCardCollectionSlot(u16* slot) {
    *slot = CARD_COLLECTION_EMPTY;
}

void RemoveUnequippedCardById(u16 id) {
    s32 i;

    for (i = 0; i < gCardCount; i++) {
        if (gCardCollection[i] != CARD_COLLECTION_EMPTY &&
            (gCardCollection[i] & CARD_FLAG_IN_ANY_DECK) == 0 &&
            (gCardCollection[i] & CARD_ID_MASK) == id) {
            gCardCollection[i] = CARD_COLLECTION_EMPTY;
            return;
        }
    }
}

u8 CollectionHasCard(u16 id) {
    s32 i;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        return FALSE;
    }

    for (i = 0; i < gCardCount; i++) {
        if ((gCardCollection[i] & CARD_ID_MASK) == id) {
            return TRUE;
        }
    }

    return FALSE;
}

void InitDecks() {
    u16 i;
    u16 j;

    for (i = 0; i < ARRAY_COUNT(gDecks); i++) {
        for (j = 0; j < ARRAY_COUNT(gDecks[i].cards); j++) {
            gDecks[i].cards[j] |= CARD_NONE;
        }

        for (j = 0; j < ARRAY_COUNT(gDecks[i].name); j++) {
            gDecks[i].name[j] = 0;
        }

        gDecks[i].cpCost = 0;
        gDecks[i].cardCount = 0;
    }
}

void ClearDeck(u8 deck) {
    u16 mask;
    u16 i;

    mask = 0;

    switch (deck) {
    case 0:
        mask = CARD_FLAG_IN_DECK_1;
        break;
    case 1:
        mask = CARD_FLAG_IN_DECK_2;
        break;
    case 2:
        mask = CARD_FLAG_IN_DECK_3;
        break;
    }

    for (i = 0; i < ARRAY_COUNT(gDecks[deck].cards); i++) {
        if (gDecks[deck].cards[i] != CARD_NONE) {
            gCardCollection[gDecks[deck].cards[i]] &= ~mask;
            gDecks[deck].cards[i] |= CARD_NONE;
        }
    }

    gDecks[deck].cpCost = 0;
    gDecks[deck].cardCount = 0;
}

u8 AddCardToActiveDeck(u16 card) {
    u16* cards;
    u16 i;
    u16 cpCost;

    i = 0;
    cards = GetActiveDeck()->cards;

    while (cards[i] != CARD_NONE) {
        i++;

        if (i == 99) {
            return FALSE;
        }
    }

    cards[i] = card;

    switch (sActiveDeck) {
    case 0:
        gCardCollection[card] |= CARD_FLAG_IN_DECK_1;
        break;
    case 1:
        gCardCollection[card] |= CARD_FLAG_IN_DECK_2;
        break;
    case 2:
        gCardCollection[card] |= CARD_FLAG_IN_DECK_3;
        break;
    }

    cpCost = GetCardCpCost(gCardCollection[card]);
    gDecks[sActiveDeck].cpCost += cpCost;
    gDecks[sActiveDeck].cardCount++;
    return TRUE;
}

u8 AddCardToDeck(u16 card, u8 deck) {
    u16* cards;
    u16 i;
    u16 cpCost;

    cards = GetDeck(deck)->cards;

    for (i = 0; i < 99 && cards[i] != CARD_NONE; i++) {
    }

    if (i == 99) {
        return FALSE;
    }

    cards[i] = card;

    switch (deck) {
    case 0:
        gCardCollection[card] |= CARD_FLAG_IN_DECK_1;
        break;
    case 1:
        gCardCollection[card] |= CARD_FLAG_IN_DECK_2;
        break;
    case 2:
        gCardCollection[card] |= CARD_FLAG_IN_DECK_3;
        break;
    }

    cpCost = GetCardCpCost(gCardCollection[card]);
    gDecks[deck].cpCost += cpCost;
    gDecks[deck].cardCount++;
    return TRUE;
}

void RemoveCardFromActiveDeck(u16 slot) {
    u16* cards;
    u16 cpCost;

    cards = GetActiveDeck()->cards;

    if (cards[slot] != 0) {
        switch (sActiveDeck) {
        case 0:
            gCardCollection[cards[slot]] &= ~CARD_FLAG_IN_DECK_1;
            break;
        case 1:
            gCardCollection[cards[slot]] &= ~CARD_FLAG_IN_DECK_2;
            break;
        case 2:
            gCardCollection[cards[slot]] &= ~CARD_FLAG_IN_DECK_3;
            break;
        }
    }

    cpCost = GetCardCpCost(gCardCollection[cards[slot]]);
    gDecks[sActiveDeck].cpCost -= cpCost;
    gDecks[sActiveDeck].cardCount--;
    cards[slot] = CARD_NONE;
}

void RemoveCardFromDeck(u16* slot, u8 deck) {
    u16 cpCost;

    switch (deck) {
    case 0:
        gCardCollection[*slot] &= ~CARD_FLAG_IN_DECK_1;
        break;
    case 1:
        gCardCollection[*slot] &= ~CARD_FLAG_IN_DECK_2;
        break;
    case 2:
        gCardCollection[*slot] &= ~CARD_FLAG_IN_DECK_3;
        break;
    }

    cpCost = GetCardCpCost(gCardCollection[*slot]);
    gDecks[deck].cpCost -= cpCost;
    gDecks[deck].cardCount--;
    *slot = CARD_NONE;
}

void RecalculateInactiveDeckCpCosts() {
    u8 i;
    u16 total;
    s32 j;
    Deck* deck;

    for (i = 0; i < ARRAY_COUNT(gDecks); i++) {
        if (i == sActiveDeck) {
            continue;
        }

        total = 0;
        deck = GetDeck(i);

        for (j = 0; j < 99; j++) {
            if (deck->cards[j] != CARD_NONE) {
                total += GetCardCpCost(gCardCollection[deck->cards[j]]);
            }
        }

        gDecks[i].cpCost = total;
    }
}

void ConvertActiveDeckCardToPremium(u16 index) {
    u16* cards;
    u16 cpCost;

    cards = GetActiveDeck()->cards;
    gDecks[sActiveDeck].cpCost -= GetCardCpCost(gCardCollection[cards[index]]);
    gCardCollection[cards[index]] |= CARD_FLAG_PREMIUM;
    cpCost = GetCardCpCost(gCardCollection[cards[index]]) + gDecks[sActiveDeck].cpCost;
    gDecks[sActiveDeck].cpCost = cpCost;
    RecalculateInactiveDeckCpCosts();
}

u8 HasNonPremiumCardsInActiveDeck() {
    Deck* deck;
    s32 count;
    s32 i;

    count = 0;
    deck = GetActiveDeck();

    for (i = 0; i < DECK_SIZE; i++) {
        if (deck->cards[i] != CARD_NONE) {
            if (gCardCollection[deck->cards[i]] & CARD_FLAG_PREMIUM) {
                count++;
            }
        }
    }

    if (gDecks[sActiveDeck].cardCount == count) {
        return FALSE;
    }

    return TRUE;
}

Deck* GetActiveDeck() {
    return &gDecks[sActiveDeck];
}

Deck* GetDeck(u8 index) {
    return &gDecks[index];
}

u16 GetDeckCpCost(u8 index) {
    return gDecks[index].cpCost;
}

void SetDeckName(u8 index, const void* src) {
    u8* deck;
    u32 offset;
    u8* deckName;
    const u8* text;

#ifdef VERSION_US
    if (*(const u16*)src == 0) {
#else
    if (*(const u8*)src == 0) {
#endif
        return;
    }

    deck = (u8*)&gDecks;
    offset = index * (sizeof(Deck) / sizeof(u16));
    text = src;
    offset *= sizeof(u16);
    deckName = deck + offsetof(Deck, name);
    deckName += offset;

#ifdef VERSION_US
    do {
        deckName[0] = text[0];
        deckName[1] = text[1];
        deckName += 2;
        text += 2;
    } while (*(const u16*)text != 0);
#else
    do {
        *deckName = *text;
#ifdef VERSION_EU
        deckName++;
        text++;
#else
        text++;
        deckName++;
#endif
    } while (*text != 0);
#endif
}

u8* GetDeckName(u8 index) {
    return gDecks[index].name;
}

u16 CountActiveDeckCardsOfCategory(u8 category) {
    u16* cards;
    u16 count;
    u16 i;

    count = 0;
    cards = GetActiveDeck()->cards;

    for (i = 0; i < DECK_SIZE; i++) {
        if (cards[i] != CARD_NONE) {
            if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category == category) {
                count++;
            }
        }
    }

    return count;
}

u16 CountDeckCardsOfCategory(u8 category, u8 deckIndex) {
    u16* cards;
    u16 count;
    u16 i;

    count = 0;
    cards = GetDeck(deckIndex)->cards;

    for (i = 0; i < DECK_SIZE; i++) {
        if (cards[i] != CARD_NONE) {
            if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category == category) {
                count++;
            }
        }
    }

    return count;
}

s16 CountActiveDeckCards(s32 cardSet) {
    u16 count;
    s16 i;
    u16* cards;

    count = 0;
    cards = GetActiveDeck()->cards;

    switch (cardSet) {
    case DECK_CARD_SET_MAIN:
        for (i = 0; i <= 98; i++) {
            if (cards[i] != CARD_NONE) {
                if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category <= CARD_CATEGORY_ITEM) {
                    count = ((count << 16) + 0x10000) >> 16;
                }
            }
        }

        break;
    case DECK_CARD_SET_ENEMY:
        for (i = 0; i <= 98; i++) {
            if (cards[i] != CARD_NONE) {
                if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category == CARD_CATEGORY_ENEMY) {
                    count = ((count << 16) + 0x10000) >> 16;
                }
            }
        }

        break;
    }

    return count;
}

s16 CountDeckCards(s32 cardSet, const Deck* deck) {
    s16 count;
    s16 i;

    count = 0;

    switch (cardSet) {
    case DECK_CARD_SET_MAIN:
        for (i = 0; i < deck->cardCount; i++) {
            if (gCardDefs[deck->cards[i] & CARD_ID_MASK].category <= CARD_CATEGORY_ITEM) {
                count++;
            }
        }

        break;
    case DECK_CARD_SET_ENEMY:
        for (i = 0; i < deck->cardCount; i++) {
            if (gCardDefs[deck->cards[i] & CARD_ID_MASK].category == CARD_CATEGORY_ENEMY) {
                count++;
            }
        }

        break;
    }

    return count;
}

void CopyActiveDeckCards(s32 cardSet, u16* out) {
    u16* cards;
    u16 i;

    cards = GetActiveDeck()->cards;

    switch (cardSet) {
    case DECK_CARD_SET_MAIN:
        for (i = 0; i < 99; i++) {
            if (cards[i] != CARD_NONE) {
                if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category <= CARD_CATEGORY_ITEM) {
                    *out++ = gCardCollection[cards[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                }
            }
        }

        break;
    case DECK_CARD_SET_ENEMY:
        for (i = 0; i < 99; i++) {
            if (cards[i] != CARD_NONE) {
                if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category == CARD_CATEGORY_ENEMY) {
                    *out++ = gCardCollection[cards[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                }
            }
        }

        break;
    }
}

u16 GetDeckCardCount(u8 index) {
    return gDecks[index].cardCount;
}

void SetActiveDeckIndex(u8 index) {
    sActiveDeck = index;
}

u16 GetCollectionCardKind(u16 index) {
    return gCardDefs[gCardCollection[index] & CARD_ID_MASK].kind;
}

u8 GetCollectionCardCategory(u16 index) {
    return gCardDefs[gCardCollection[index] & CARD_ID_MASK].category;
}

u8 IsActiveDeckAllPremium() {
    Deck* deck;
    s32 attackMagicCount;
    s32 premiumCount;
    s32 i;

    attackMagicCount = 0;
    premiumCount = 0;
    deck = GetActiveDeck();

    for (i = 0; i < DECK_SIZE; i++) {
        if (deck->cards[i] != CARD_NONE) {
            if (GetCollectionCardCategory(deck->cards[i]) != CARD_CATEGORY_ITEM) {
                if (GetCollectionCardCategory(deck->cards[i]) != CARD_CATEGORY_ENEMY) {
                    attackMagicCount++;
                }
            }
        }
    }

    for (i = 0; i < DECK_SIZE; i++) {
        if (deck->cards[i] != CARD_NONE) {
            if (gCardCollection[deck->cards[i]] & CARD_FLAG_PREMIUM) {
                premiumCount++;
            }
        }
    }

    if (premiumCount == attackMagicCount) {
        return TRUE;
    }

    return FALSE;
}

#ifdef VERSION_JP
#include "deck_names_shift_jis.inc"
#endif
void InitSoraDecks() {
    sActiveDeck = 0;
    InitCardCollection();
    InitDecks();

    if (gDebugFlags & DEBUG_FLAG_ALL_BTL_CARD) {
        FillDebugCardCollection();
        BuildDebugKeybladeDeck(2);
        BuildDebugKingdomKeyDeck(1);
        BuildDebugMixedDeck(0);
    } else {
        ObtainStarterCards();
        FillStarterDeck();
        func_080AB964();
        func_080AB968();
    }

#ifdef VERSION_EU
    SetDeckName(0, gDefaultDeckName0TextByLanguage.strings[gLanguage]);
    SetDeckName(1, gDefaultDeckName1TextByLanguage.strings[gLanguage]);
    SetDeckName(2, gDefaultDeckName2TextByLanguage.strings[gLanguage]);
#elif defined(VERSION_JP)
    SetDeckName(0, gDefaultDeckName0TextJapanese);
    SetDeckName(1, gDefaultDeckName1TextJapanese);
    SetDeckName(2, gDefaultDeckName2TextJapanese);
#else
    SetDeckName(0, gDefaultDeckName0Text);
    SetDeckName(1, gDefaultDeckName1Text);
    SetDeckName(2, gDefaultDeckName2Text);
#endif
}

void InitDebugDecks() {
    sActiveDeck = 0;
    InitCardCollection();
    InitDecks();
    FillDebugCardCollection();
    BuildDebugKingdomKeyDeck(0);
    BuildDebugKeybladeDeck(1);
    BuildDebugMixedDeck(2);
#ifdef VERSION_EU
    SetDeckName(0, gDefaultDeckName0TextByLanguage.strings[gLanguage]);
    SetDeckName(1, gDefaultDeckName1TextByLanguage.strings[gLanguage]);
    SetDeckName(2, gDefaultDeckName2TextByLanguage.strings[gLanguage]);
#elif defined(VERSION_JP)
    SetDeckName(0, gDefaultDeckName0TextJapanese);
    SetDeckName(1, gDefaultDeckName1TextJapanese);
    SetDeckName(2, gDefaultDeckName2TextJapanese);
#else
    SetDeckName(0, gDefaultDeckName0Text);
    SetDeckName(1, gDefaultDeckName1Text);
    SetDeckName(2, gDefaultDeckName2Text);
#endif
}

void InitRikuDeckForWorld(u8 world) {
    u8 n = 0;

    InitCardCollection();
    InitDecks();
    SetActiveDeckIndex(0);

    switch (world) {
    case WORLD_ATLANTICA:
        n = 1;
        break;
    case WORLD_OLYMPUS_COLISEUM:
        n = 2;
        break;
    case WORLD_WONDERLAND:
        n = 3;
        break;
    case WORLD_MONSTRO:
        n = 4;
        break;
    case WORLD_HALLOWEEN_TOWN:
        n = 5;
        break;
    case WORLD_NEVER_LAND:
        n = 6;
        break;
    case WORLD_HOLLOW_BASTION:
        n = 7;
        break;
    case WORLD_DESTINY_ISLANDS:
        n = 8;
        break;
    case WORLD_TRAVERSE_TOWN:
        n = 9;
        break;
    case WORLD_TWILIGHT_TOWN:
        n = 10;
        break;
    case 0:
    case WORLD_CASTLE_OBLIVION:
        n = 11;
        break;
    case WORLD_AGRABAH:
    case WORLD_100_ACRE_WOOD:
        n = 0;
        break;
    }

    BuildRikuDeck(n);
}

void BuildRikuDeck(u8 deck) {
    s32 i;
    s32 j;

    for (i = 0, j = 0; i < gRikuDeckCardCounts[deck]; i++, j++) {
        ObtainCard(gRikuDeckCards[deck][i]);
        AddCardToActiveDeck(i);
    }

    for (i = 0; i < gRikuDeckEnemyCardCounts[deck]; j++, i++) {
        ObtainCard(gRikuDeckEnemyCards[deck][i]);
        AddCardToActiveDeck(j);
    }

    gCardCollection[200] = CARD_GUARD_ARMOR_1;
    gCardCollection[201] = CARD_PARASITE_CAGE_1;
    gCardCollection[202] = CARD_TRICKMASTER_1;
    gCardCollection[203] = CARD_DARKSIDE_1;
    gCardCollection[204] = CARD_HADES_9;
    gCardCollection[205] = CARD_JAFAR_1;
    gCardCollection[206] = CARD_OOGIE_BOOGIE_1;
    gCardCollection[207] = CARD_URSULA_1;
    gCardCollection[208] = CARD_HOOK_9;
    gCardCollection[209] = CARD_DRAGON_MALEFICENT_1;
    gCardCollection[210] = CARD_RIKU_9;
    gCardCollection[211] = CARD_VEXEN_9;
    gCardCollection[212] = CARD_LEXAEUS_9;
    gCardCollection[213] = CARD_ANSEM_9;

    if (IsCardKindObtained(OBTAINED_CARD_GUARD_ARMOR)) {
        AddCardToActiveDeck(200);
    }

    if (IsCardKindObtained(OBTAINED_CARD_PARASITE_CAGE)) {
        AddCardToActiveDeck(201);
    }

    if (IsCardKindObtained(OBTAINED_CARD_TRICKMASTER)) {
        AddCardToActiveDeck(202);
    }

    if (IsCardKindObtained(OBTAINED_CARD_DARKSIDE)) {
        AddCardToActiveDeck(203);
    }

    if (IsCardKindObtained(OBTAINED_CARD_HADES)) {
        AddCardToActiveDeck(204);
    }

    if (IsCardKindObtained(OBTAINED_CARD_JAFAR)) {
        AddCardToActiveDeck(205);
    }

    if (IsCardKindObtained(OBTAINED_CARD_OOGIE_BOOGIE)) {
        AddCardToActiveDeck(206);
    }

    if (IsCardKindObtained(OBTAINED_CARD_URSULA)) {
        AddCardToActiveDeck(207);
    }

    if (IsCardKindObtained(OBTAINED_CARD_HOOK)) {
        AddCardToActiveDeck(208);
    }

    if (IsCardKindObtained(OBTAINED_CARD_DRAGON_MALEFICENT)) {
        AddCardToActiveDeck(209);
    }

    if (IsCardKindObtained(OBTAINED_CARD_RIKU)) {
        AddCardToActiveDeck(210);
    }

    if (IsCardKindObtained(OBTAINED_CARD_VEXEN)) {
        AddCardToActiveDeck(211);
    }

    if (IsCardKindObtained(OBTAINED_CARD_LEXAEUS)) {
        AddCardToActiveDeck(212);
    }

    if (IsCardKindObtained(OBTAINED_CARD_ANSEM)) {
        AddCardToActiveDeck(213);
    }
}

u8 GetActiveDeckIndex() {
    return sActiveDeck;
}

void func_08085FB0() {
}

#ifdef VERSION_EU
static const u16 sDeckButtonLabelTileSizes[5] = { 0x280, 0x280, 0x280, 0x280, 0x280 };
static const u16 sDeckCommandMenuTileSizes[5] = { 0x1800, 0x1800, 0x1800, 0x1800, 0x1800 };
static const u16 sDeckTitleBannerTileSizes[5] = { 0x320, 0x320, 0x320, 0x320, 0x320 };
#endif
static const s16 sDeckTabPointerX[3] = { 116, 116, 116 };

#ifdef VERSION_EU
static const s16 sDeckTabPointerY[3] = { 51, 99, 148 };
#else
static const s16 sDeckTabPointerY[3] = { 56, 104, 148 };
#endif

static const s16 sDeckFilterTabX[5] = { 12, 28, 42, 56, 70 };

static const s16 sCollectionFilterTabX[6] = { 172, 172, 188, 202, 216, 230 };

enum DeckCommand {
    DECK_COMMAND_EQUIP,
    DECK_COMMAND_RENAME,
    DECK_COMMAND_CLEAR,
    DECK_COMMAND_ADD,
    DECK_COMMAND_REMOVE,
    DECK_COMMAND_DELETE
};

static const s16 sDeckCommandY[6] = { 50, 67, 85, 108, 126, 149 };

static const s16 sValueGridX[2] = { 80, 128 };

static const s16 sValueGridY[9] = { 80, 88, 96, 104, 112, 45, 93, 141, 30 };

#ifdef VERSION_JP
static const PromptChoiceLayout sDeckPromptChoiceLayout = { { 94, 151 } };
#else
static const PromptChoiceLayout sDeckPromptChoiceLayout = { { 102, 148 } };
#endif

static void Deckmenu2_0(DeckMenuWork* work, void* resultOut) {
#ifdef PLATFORM_PC
    PortClearBgTextTiles();
#endif
    work->resultOut = resultOut;
    SetBgMode0();
    SetBackdropColor(0, 0, 0);
#ifdef VERSION_EU
    SetupBg(0, 0, 31, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 0, 28, 0);
#else
    SetupBg(0, 3, 31, 0);
    SetupBg(1, 2, 23, 0);
    SetupBg(2, 1, 15, 0);
#endif
    SetupBg(3, 0, 30, 0);
    SetBgPriority(0, 0);
    SetBgPriority(1, 1);
    SetBgPriority(2, 2);
    SetBgPriority(3, 3);
    FadeStartIn(FADE_MODE_BLACK, 16);
    ListPoolInit(&work->pool);
    TaskPoolInit(&work->taskpool, 286);
    TaskPoolInit(&work->cardpool, 1);
    work->deckIndex = GetActiveDeckIndex();
    CreateDeckGridCards(work, 0);
    work->tiles = AllocObjTiles(0x120, NULL);
    SetObjTileSource(work->tiles, gHandCursorTiles);
    AnimInit(&work->anim2, gHandCursorAnims, gHandCursorFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim2);
    work->handX = sDeckTabPointerX[0] << 8;
    work->handY = sDeckTabPointerY[0] << 8;
    work->handFlags = 0;
    work->tiles4 = LoadObjTiles(gDeckScrollThumbTiles, sizeof(gDeckScrollThumbTiles));
    work->palette = LoadObjPalette(gDialogBoxPalette, sizeof(gDialogBoxPalette));
#ifdef VERSION_EU
    work->tiles5 = LoadObjTiles(gDeckButtonLabelTilesByLanguage[gLanguage], sDeckButtonLabelTileSizes[gLanguage]);
#else
    work->tiles5 = LoadObjTiles(gDeckButtonLabelTiles, sizeof(gDeckButtonLabelTiles));
#endif
#ifdef VERSION_EU
    work->gfx7 = gDeckButtonLabelSpritesByLanguage[gLanguage][0];
    work->gfx8 = gDeckButtonLabelSpritesByLanguage[gLanguage][1];
#else
    work->gfx7 = gDeckButtonLabelFrames[0];
    work->gfx8 = gDeckButtonLabelFrames[1];
#endif
    work->tiles2 = AllocObjTiles(0x280, NULL);
    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
    work->palette4 = LoadObjPalette(gDeckMenuTextPalette, sizeof(gDeckMenuTextPalette));
    gCardUiSpriteState.tiles = AllocObjTiles(0x100, NULL);
    gCardUiSpriteState.palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    SetObjTileSource(gCardUiSpriteState.tiles, gCardPremiumSmallTiles);
    AnimInit(&gCardUiSpriteState.anim, gCardPremiumSmallAnims, gCardPremiumSmallFrames);
    AnimStart(&gCardUiSpriteState.anim, 0, ANIM_FLAG_LOOP);
    gCardUiSpriteState.gfx = AnimUpdate(&gCardUiSpriteState.anim);
    work->tiles10 = NULL;
    work->tiles7 = NULL;
    work->tiles8 = NULL;
    work->tiles9 = NULL;
    work->palette5 = NULL;
    work->palette6 = NULL;
    work->tiles3 = NULL;
    work->palette2 = NULL;
    work->tiles12 = NULL;
    work->tiles6 = NULL;
    work->palette3 = NULL;
    work->cursorCol = 0;
    work->cursorRow = 0;
    work->prevCursor[0] = 0;
    work->prevCursor[1] = 0;
    work->timer = 16;
    work->commandCursor = DECK_COMMAND_EQUIP;
    work->cursorCard = NULL;
    work->prevCursorCard = NULL;
    work->mode = DECK_MENU_MODE_NONE;
    work->view = DECK_MENU_VIEW_DECK_GRID;
    work->deckAttackCount = CountActiveDeckCardsOfCategory(CARD_CATEGORY_ATTACK);
    work->deckMagicCount = CountActiveDeckCardsOfCategory(CARD_CATEGORY_MAGIC);
    work->deckItemCount = CountActiveDeckCardsOfCategory(CARD_CATEGORY_ITEM);
    work->deckEnemyCount = CountActiveDeckCardsOfCategory(CARD_CATEGORY_ENEMY);
    work->categoryFilter = CATEGORY_FILTER_DECK_ALL;
    work->entryCount = 0;
    work->entries = NULL;
    work->popupActive = 0;
    work->exitRequested = FALSE;
    work->barSlideTimer = 16;
    work->bannerSlideTimer = 16;
    work->topBarX = 0x7800;
    work->topBarY = -0x800;
    work->bottomBarX = 0xA400;
    work->bottomBarY = 0xA000;
    work->bannerX = -0x8000;
    work->holding = FALSE;
    work->handVisible = 0;
    work->removeLabelX = 95;
    work->removeLabelY = -2;
    work->addLabelX = 135;
    work->addLabelY = -2;
    work->inputDelay = 0;
    work->textSlotCount = 0;
    work->textSlotCount2 = 0;
    work->textSlotCount3 = 0;
    work->textSlotCount4 = 0;
#ifdef PLATFORM_PC
    /* The Add transition can draw the caption before selecting a card.
       Empty slots alone are insufficient: an old count reads past them. */
    work->textSlotCount5 = 0;
    work->textSlotCount6 = 0;
#endif
    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    InitTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    InitTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
    InitTextSlots(work->textSlots5, ARRAY_COUNT(work->textSlots5));
    work->step = 0;
    work->result = DECK_MENU_RESULT_NONE;
}

static u8 Deckmenu2_1(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 16);

    if (work->step == 0) {
        RequestDma3Clear(GetBgCharBase(0), 0x2000);
    }

    if (work->step == 1) {
        RequestDma3Clear(GetBgCharBase(0) + 0x2000, 0x2000);
    }

    if (work->step == 2) {
        RequestDma3Clear(GetBgCharBase(1), 0x2000);
    }

    if (work->step == 3) {
        RequestDma3Clear(GetBgCharBase(1) + 0x2000, 0x2000);
    }

    if (work->step == 4) {
        RequestDma3Clear(GetBgCharBase(2), 0x2000);
    }

    if (work->step == 5) {
        RequestDma3Clear(GetBgCharBase(2) + 0x2000, 0x2000);
    }

    if (work->step == 6) {
        RequestDma3Clear(GetBgCharBase(3), 0x2000);
    }

    if (work->step == 7) {
        RequestDma3Clear(GetBgCharBase(3) + 0x2000, 0x2000);
    }

    work->step++;

    if (work->step == 8) {
        work->step = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuLoadBgs);
    }

    return 1;
}

u8 UpdateDeckMenuLoadBgs(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 16);

    switch (work->step) {
    case 0:
        LoadBgTiles(3, gDeckMenuTiles, 0x2000);
        LoadBgPalette(3, gDeckMenuPalettes, sizeof(gDeckMenuPalettes));
        break;
    case 1:
#ifdef VERSION_EU
        RequestDma3Copy(&gDeckMenuTiles[0x2000],
                        (u8*)GetBgCharBase(3) + 0x2000, 0x1800);
#else
        RequestDma3Copy(&gDeckMenuTiles[0x2000],
                        (u8*)GetBgCharBase(3) + 0x2000, 0x2000);
#endif
        break;
    case 2:
        LoadBgMap(3, gDeckMenuMap, sizeof(gDeckMenuMap));
        break;
    case 3:
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            RequestDma3Copy(gDeckMenuTextTiles, (u8*)GetBgCharBase(3) + 0x3800, sizeof(gDeckMenuTextTiles));
            break;
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gDeckMenuTextFrenchTiles, (u8*)GetBgCharBase(3) + 0x3800, sizeof(gDeckMenuTextFrenchTiles));
            break;
        case LANGUAGE_GERMAN:
            RequestDma3Copy(gDeckMenuTextGermanTiles, (u8*)GetBgCharBase(3) + 0x3800, sizeof(gDeckMenuTextGermanTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gDeckMenuTextItalianTiles, (u8*)GetBgCharBase(3) + 0x3800, sizeof(gDeckMenuTextItalianTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gDeckMenuTextSpanishTiles, (u8*)GetBgCharBase(3) + 0x3800, sizeof(gDeckMenuTextSpanishTiles));
            break;
        }
#else
        LoadBgTiles(0, gDeck1PanelTiles, sizeof(gDeck1PanelTiles));
#endif
        break;
    case 4:
        LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
        break;
#ifndef VERSION_EU
    case 5:
        LoadBgTiles(1, gDeck2PanelTiles, 0x2000);
        break;
    case 6:
        RequestDma3Copy(&gDeck2PanelTiles[0x2000],
                        (u8*)GetBgCharBase(1) + 0x2000, 0x1E20);
        break;
#endif
    case 7:
        LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
        break;
#ifndef VERSION_EU
    case 8:
        LoadBgTiles(2, gDeck3PanelTiles, 0x2000);
        break;
    case 9:
        RequestDma3Copy(&gDeck3PanelTiles[0x2000],
                        (u8*)GetBgCharBase(2) + 0x2000, 0x1E20);
        break;
#endif
    case 10:
        LoadBgMap(2, gDefaultBgMap, sizeof(gDefaultBgMap));
        break;
    case 11:
        SetBgScroll(0, (u16)-88, (u16)-16);
        SetBgScroll(1, (u16)-88, (u16)-64);
        SetBgScroll(2, (u16)-88, (u16)-112);
        work->step = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuLoadDeckInfo);
        return 1;
    }

    work->step++;
    return 1;
}

u8 UpdateDeckMenuLoadDeckInfo(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 16);

    switch (work->step) {
    case 0:
        LoadBgMap(0, gDeck1PanelMap, 0x180);
        LoadBgMap(1, gDeck2PanelMap, 0x180);
        LoadBgMap(2, gDeck3PanelMap, 0x180);
        work->step++;
        break;
    case 1:
        DrawDeckCategoryCount(work->deckAttackCount, CARD_CATEGORY_ATTACK);
        DrawDeckCategoryCount(work->deckMagicCount, CARD_CATEGORY_MAGIC);
        DrawDeckCategoryCount(work->deckItemCount, CARD_CATEGORY_ITEM);
        DrawDeckCategoryCount(work->deckEnemyCount, CARD_CATEGORY_ENEMY);
        HighlightDeckTab(work, work->deckIndex);
        work->step++;
        break;
    case 2:
        DrawDeckCardCount(0);
        DrawDeckCardCount(1);
        DrawDeckCardCount(2);
        work->step++;
        break;
    case 3:
        DrawDeckCpCost(0);
        DrawDeckCpCost(1);
        DrawDeckCpCost(2);
        work->step++;
        break;
    case 4:
        DrawDeckEquipMarker(GetActiveDeckIndex());
        DrawCardTotals();
        work->thumbX = 0x4800;
        work->thumbY = 0x2800;
        work->cursorRow = work->deckIndex;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuSlideIn);
        work->view = DECK_MENU_VIEW_DECK_SELECT;
        SetDeckMenuHandAnim(work);
        LoadDeckNameTexts(work);
        work->handX = sDeckTabPointerX[work->cursorCol] << 8;
        work->handY = sDeckTabPointerY[work->cursorRow] << 8;
        work->tiles6 = LoadObjTiles(gDeckMenuBarTiles, sizeof(gDeckMenuBarTiles));
#ifdef VERSION_EU
        work->tiles12 = LoadObjTiles(gDeckTitleBannerTilesByLanguage[gLanguage], sDeckTitleBannerTileSizes[gLanguage]);
#elif defined(VERSION_US)
        if (gGameState.flags & GAME_FLAG_RIKU) {
            work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTiles, sizeof(gRikuDeckTitleBannerTiles));
        } else {
            work->tiles12 = LoadObjTiles(gDeckTitleBannerTiles, sizeof(gDeckTitleBannerTiles));
        }
#else
        work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTiles, sizeof(gRikuDeckTitleBannerTiles));
#endif
        work->palette3 = LoadObjPalette(gDeckTitleBannerPalette, sizeof(gDeckTitleBannerPalette));
        work->step = DECK_MENU_SLIDE_IN_STEP_VERTICAL;
        work->timer = 16;
        return 1;
    }

    return 1;
}

u8 UpdateDeckMenuSlideIn(DeckMenuWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (!FadeIsActive()) {
        switch (work->step) {
        case DECK_MENU_SLIDE_IN_STEP_VERTICAL:
            ApproachValue(&work->topBarY, 0, work->timer);
            ApproachValue(&work->bottomBarY, 0x9800, work->timer);
            work->timer--;

            if (work->timer == 0) {
                work->timer = 16;
                work->step++;
            }

            break;
        case DECK_MENU_SLIDE_IN_STEP_HORIZONTAL:
            ApproachValue(&work->bannerX, 0, work->timer);
            work->timer--;

            if (work->timer == 0) {
                ReleaseObjTiles(work->tiles6);
                ReleaseObjTiles(work->tiles12);
                ReleaseObjPalette(work->palette3);
                work->tiles6 = NULL;
                work->tiles12 = NULL;
                work->palette3 = NULL;
                work->handVisible = 1;
                LoadBgMap(3, gDeckReviewGridMap, sizeof(gDeckReviewGridMap));
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckSelect);
            }

            break;
        }
    }

    return 1;
}

u8 UpdateDeckMenuEnterDeckGrid(DeckMenuWork* work, void* task) {
    LoadDeckNameTexts(work);
    ApproachValueHalf(&work->handX, gDeckGridColumnX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, gDeckGridRowY[work->cursorRow] << 8);
    work->timer--;

    if (work->timer == 0) {
        work->view = DECK_MENU_VIEW_DECK_GRID;
        SetDeckMenuHandAnim(work);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckGrid);
    }

    return 1;
}

u8 UpdateDeckMenuDeckGrid(DeckMenuWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (FadeIsActive()) {
        TaskPoolUpdate(&work->taskpool);
        return 1;
    }

    if (work->popupActive != 0) {
        ApproachValueHalf(&work->handX, gDeckGridColumnX[work->cursorCol] << 8);
        ApproachValueHalf(&work->handY, gDeckGridRowY[work->cursorRow] << 8);

        if (work->timer != 0) {
            work->timer--;
        }

        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->exitRequested = TRUE;
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        work->inputDelay = 4;
        return 1;
    }

    if (work->inputDelay > 0) {
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        work->inputDelay--;
        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        } else {
            work->exitRequested = FALSE;
        }
    }

    switch (GetKeysRepeat()) {
    case DPAD_UP:
        if (work->cursorRow > 0) {
            work->cursorRow--;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        } else {
            if (!ScrollGridUp(work, TRUE)) {
                if (!work->holding) {
                    work->cursorCol = work->categoryFilter;
                    work->timer = 1;
                    m4aSongNumStart(SONG_SYS_CLICKI04B);
                    work->prevView = work->view;
                    work->view = DECK_MENU_VIEW_DECK_FILTER;
                    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
                    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckFilter);
                    DrawCpCost(0);
                    return 1;
                }
            } else {
                if (work->holding) {
                    work->heldRow++;

                    if ((u16)work->heldRow <= 3) {
                        work->heldY = gDeckGridRowY[work->heldRow] << 8;
                    } else {
                        work->heldY = -0x10000;
                    }
                }
            }
        }

        break;
    case DPAD_DOWN:
        if (work->cursorRow <= 2) {
            work->cursorRow++;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        } else {
            ScrollGridDown(work);

            if (work->holding) {
                if ((u16)work->heldRow <= 3) {
                    work->heldY = gDeckGridRowY[work->heldRow] << 8;
                } else {
                    work->heldY = -0x10000;
                }
            }
        }

        break;
    case DPAD_LEFT:
        if (work->cursorCol > 0) {
            work->cursorCol--;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        break;
    case DPAD_RIGHT:
        if (work->cursorCol > 1) {
            if (!work->holding) {
                work->cursorCol = 0;
                work->cursorRow = work->deckIndex;
                work->timer = 1;
                work->prevView = work->view;
                work->view = DECK_MENU_VIEW_DECK_SELECT;
                SetDeckMenuHandAnim(work);
                m4aSongNumStart(SONG_SYS_CLICK);
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckSelect);
                return 1;
            }
        } else {
            work->cursorCol++;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        break;
    }

    switch (GetKeysPressed()) {
    case B_BUTTON:
        if (!work->holding) {
            work->cursorCol = 0;
            work->cursorRow = work->deckIndex;
            work->timer = 1;
            work->prevView = work->view;
            work->view = DECK_MENU_VIEW_DECK_SELECT;
            SetDeckMenuHandAnim(work);
            m4aSongNumStart(SONG_SYS_CLICK);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckSelect);
            return 1;
        } else {
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->holding = FALSE;
            AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
            return 1;
        }
    case A_BUTTON:
        if (work->categoryFilter != CATEGORY_FILTER_DECK_ALL) {
            return 1;
        }

        if (!work->holding) {
            m4aSongNumStart(SONG_SYS_KETEI2);
            work->holding = TRUE;
            work->heldCol = work->cursorCol;
            work->heldRow = work->cursorRow;
            work->heldX = gDeckGridColumnX[work->heldCol] << 8;
            work->heldY = gDeckGridRowY[work->heldRow] << 8;
            AnimStart(&work->anim2, 4, ANIM_FLAG_LOOP);
        } else {
            if (SwapHeldDeckCard(work)) {
                m4aSongNumStart(SONG_SYS_KETEI2);
                work->holding = FALSE;
                AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
            } else {
                m4aSongNumStart(SONG_SYS_BEEP);
            }
        }

        return 1;
    case START_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        return 1;
    case L_BUTTON:
        m4aSongNumStart(SONG_SYS_CANSEL);
        work->holding = FALSE;
        AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenRemoveMode);
        return 1;
    case R_BUTTON:
        FreeCollectionEntries(work);
        m4aSongNumStart(SONG_SYS_CANSEL);
        work->holding = FALSE;
        AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenAddMode);
        return 1;
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        work->holding = FALSE;
        work->cursorRow = 0;
        work->scrollRowEnd = 4;
        ResetGridScroll(work);
        work->thumbX = 0x4800;
        work->thumbY = 0x2800;
        work->cursorCol = work->categoryFilter;
        work->timer = 1;
        m4aSongNumStart(SONG_SYS_CLICKI04B);
        work->view = DECK_MENU_VIEW_DECK_FILTER;
        AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckFilter);
        DrawCpCost(0);
        return 1;
    }

    work->cursorCard = GetCardAtCursor(work);
    ApproachValueHalf(&work->handX, gDeckGridColumnX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, gDeckGridRowY[work->cursorRow] << 8);

    if (work->timer != 0) {
        work->timer--;
    }

    work->prevCursorCard = work->cursorCard;
    work->prevCursor[0] = work->cursorCol;
    work->prevCursor[1] = work->cursorRow;
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuDeckFilter(DeckMenuWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim2);

    if (work->popupActive != 0) {
        ApproachValueHalf(&work->handX, sDeckFilterTabX[work->cursorCol] << 8);
        ApproachValueHalf(&work->handY, 0x1E00);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        work->inputDelay = 4;
        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        work->exitRequested = FALSE;
    }

    switch (GetKeysRepeat()) {
    case DPAD_LEFT:
        if (work->cursorCol > CATEGORY_FILTER_DECK_ALL) {
            work->cursorCol--;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICK);
            work->categoryFilter = work->cursorCol;
            DrawDeckFilterTab(work->categoryFilter, work->mode);
            ClearCardGrid(work);
            CreateDeckGridCards(work, work->categoryFilter);
        }

        break;
    case DPAD_RIGHT:
        if (work->cursorCol < CATEGORY_FILTER_ENEMY) {
            work->cursorCol++;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICK);
            work->categoryFilter = work->cursorCol;
            DrawDeckFilterTab(work->categoryFilter, work->mode);
            ClearCardGrid(work);
            CreateDeckGridCards(work, work->categoryFilter);
        }

        break;
    }

    switch (GetKeysPressed()) {
    case B_BUTTON:
    case DPAD_DOWN:
        work->cursorCol = 0;
        work->cursorRow = 0;
        work->timer = 1;
        m4aSongNumStart(SONG_SYS_CLICKI04B);

        if (work->view == DECK_MENU_VIEW_DECK_FILTER) {
            work->view = DECK_MENU_VIEW_DECK_GRID;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuEnterDeckGrid);
        }

        if (work->view == DECK_MENU_VIEW_REMOVE_FILTER) {
            work->view = DECK_MENU_VIEW_REMOVE_GRID;
            ShowDeckCardPreview(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuRemoveGrid);
        }

        work->thumbX = 0x4800;
        work->thumbY = 0x2800;
        work->scrollRowEnd = 4;
        return 1;
    case R_BUTTON:
        FreeCollectionEntries(work);
        work->unk_8CA = 1;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenAddMode);
        m4aSongNumStart(SONG_SYS_CANSEL);
        return 1;
    case L_BUTTON:
        if (work->view == DECK_MENU_VIEW_DECK_FILTER) {
            m4aSongNumStart(SONG_SYS_CANSEL);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenRemoveMode);
        }

        return 1;
    case START_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 16);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        return 1;
    }

    ApproachValueHalf(&work->handX, sDeckFilterTabX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, 0x1E00);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuDeletePrompt(DeckMenuWork* work, void* task) {
    PromptChoiceLayout layout = sDeckPromptChoiceLayout;

    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (GetKeysPressed() & DPAD_LEFT) {
        if (work->promptChoice != 0) {
            work->promptChoice--;
        }

        work->timer = 1;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysPressed() & DPAD_RIGHT) {
        if (work->promptChoice == 0) {
            work->promptChoice++;
        }

        work->timer = 1;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysPressed() & A_BUTTON) {
        if (work->promptChoice == 1) {
            work->timer = 1;
            work->view = DECK_MENU_VIEW_DELETE_VALUE_SELECT;
            SetDeckMenuHandAnim(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteValueSelect);
            TaskPoolUpdate(&work->taskpool);
            TaskPoolUpdate(&work->cardpool);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        work->timer = 1;
        DeleteSelectedValueCard(work);
        DrawCardTotals();
        CountCardsNotInDeckByCategory(DECK_ANY, work->collectionCategoryCounts);
        DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ATTACK], CARD_CATEGORY_ATTACK);
        DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_MAGIC], CARD_CATEGORY_MAGIC);
        DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ITEM], CARD_CATEGORY_ITEM);
        DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ENEMY], CARD_CATEGORY_ENEMY);

        if (!(u8)MoveValueCursor(work, 0)) {
            RemoveEmptyCollectionEntry(work, TRUE);

            if (work->gridEntryCount != 0) {
                SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
                work->cursorCol = work->savedCol;
                work->cursorRow = work->savedRow;

                while (!(u8)IsCardAtCursor(work)) {
                    work->cursorCol--;

                    if (work->cursorCol < 0) {
                        work->cursorRow--;

                        if (work->cursorRow < 0) {
                            ScrollGridUp(work, FALSE);
                            work->cursorRow = 0;
                        }

                        work->cursorCol = 2;
                    }
                }

                work->handX = gCollectionGridColumnX[work->cursorCol] << 8;
                work->handY = gCollectionGridRowY[work->cursorRow] << 8;
                ShowCollectionCardPreview(work);
                work->view = DECK_MENU_VIEW_DELETE_GRID;
                SetDeckMenuHandAnim(work);
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteGrid);
                TaskPoolUpdate(&work->taskpool);
                TaskPoolUpdate(&work->cardpool);
                return 1;
            } else {
                SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
                work->cursorCol = work->categoryFilter;
                work->timer = 1;
                work->view = DECK_MENU_VIEW_DELETE_FILTER;
                SetDeckMenuHandAnim(work);
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
                work->gridEntryCount = 0;
                TaskPoolUpdate(&work->taskpool);
                TaskPoolUpdate(&work->cardpool);
                return 1;
            }
        } else {
            DrawSelectedValueCpCost(work);
            work->view = DECK_MENU_VIEW_DELETE_VALUE_SELECT;
            SetDeckMenuHandAnim(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteValueSelect);
            TaskPoolUpdate(&work->taskpool);
            TaskPoolUpdate(&work->cardpool);
            return 1;
        }
    } else if (GetKeysPressed() & B_BUTTON) {
        work->timer = 1;
        work->view = DECK_MENU_VIEW_DELETE_VALUE_SELECT;
        SetDeckMenuHandAnim(work);
        m4aSongNumStart(SONG_SYS_CANSEL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteValueSelect);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        return 1;
    } else {
        ApproachValueHalf(&work->handX, layout.x[work->promptChoice] << 8);
        ApproachValueHalf(&work->handY, 0x7200);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        return 1;
    }
}

u8 UpdateDeckMenuDeleteValueSelect(DeckMenuWork* work, void* task) {
    s8 prevRow;
    s16 saved;

    if (work->popupActive != 0) {
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        work->inputDelay = 8;
        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        work->exitRequested = FALSE;
    }

    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    switch (GetKeysRepeat()) {
    case DPAD_LEFT:
    case (DPAD_LEFT | DPAD_UP):
    case (DPAD_LEFT | DPAD_DOWN):
        if (work->cursorCol > 0) {
            work->cursorCol--;
            work->timer = 1;

            if ((u8)MoveValueCursor(work, DPAD_LEFT)) {
                m4aSongNumStart(SONG_SYS_CLICK);
            }
        }

        DrawSelectedValueCpCost(work);
        break;
    case DPAD_RIGHT:
    case (DPAD_RIGHT | DPAD_UP):
    case (DPAD_RIGHT | DPAD_DOWN):
        if (work->cursorCol <= 0) {
            work->cursorCol++;
            work->timer = 1;

            if ((u8)MoveValueCursor(work, DPAD_RIGHT)) {
                m4aSongNumStart(SONG_SYS_CLICK);
            }
        }

        DrawSelectedValueCpCost(work);
        break;
    case DPAD_UP:
        prevRow = work->cursorRow;

        if (work->cursorRow > 0) {
            work->cursorRow = work->cursorRow - 1;
        } else {
            work->cursorRow = 4;
        }

        work->timer = 1;
        MoveValueCursor(work, DPAD_UP);

        if (prevRow != work->cursorRow) {
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        DrawSelectedValueCpCost(work);
        break;
    case DPAD_DOWN:
        prevRow = work->cursorRow;

        if (work->cursorRow <= 3) {
            work->cursorRow = work->cursorRow + 1;
        } else {
            work->cursorRow = 0;
        }

        work->timer = 1;
        MoveValueCursor(work, DPAD_DOWN);

        if (prevRow != work->cursorRow) {
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        DrawSelectedValueCpCost(work);
        break;
    }

    switch (GetKeysPressed()) {
    case B_BUTTON:
        SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
        saved = work->savedCol;
        work->cursorCol = saved;
        saved = work->savedRow;
        work->cursorRow = saved;
        work->handX = gCollectionGridColumnX[work->cursorCol] << 8;
        work->handY = gCollectionGridRowY[work->cursorRow] << 8;
        ShowCollectionCardPreview(work);
        work->view = DECK_MENU_VIEW_DELETE_GRID;
        m4aSongNumStart(SONG_SYS_CLOSE);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteGrid);
        return 1;
    case A_BUTTON:
        if (work->inputDelay > 0) {
            return 1;
        }

        if (!CheckCardDeletable(work)) {
            return 1;
        }

        TaskCreate(&work->cardpool, &gTaskDescDeckYesNo, &work->popupActive);
        work->promptChoice = 1;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeletePrompt);
        work->timer = 1;
        work->view = DECK_MENU_VIEW_DELETE_PROMPT;
        m4aSongNumStart(SONG_SYS_CLOSE);
        SetDeckMenuHandAnim(work);
        return 1;
    case START_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        return 1;
    }

    ApproachValueHalf(&work->handX, sValueGridX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, (sValueGridY[work->cursorRow] - 16) << 8);

    if (work->inputDelay > 0) {
        work->inputDelay--;
    }

    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

void RemoveEmptyCollectionEntry(DeckMenuWork* work, u8 excludeBossCards) {
    DeckCard2Work* node;
    DeckCard2Work* next;
    CardKindEntry* entry;
    s32 i;

    node = ListPoolFirst(&work->pool);
    i = 0;
    entry = &work->entries[work->entryIndex];
    EwramFree(work->entries[work->entryIndex].indices);
    work->entries[work->entryIndex].indices = NULL;

    for (i = work->entryIndex; i < work->entryCount - 1; i++) {
        work->entries[i] = work->entries[i + 1];
    }

    work->entryCount -= 1;
    work->gridEntryCount -= 1;

    while (node != NULL) {
        if (node->args.col == work->savedCol && node->args.row == work->savedRow) {
            break;
        }

        node = ListPoolNext(&node->node);
    }

    next = ListPoolNext(&node->node);

    while (next != NULL) {
        next->args.col--;

        if (next->args.col < 0) {
            next->args.col = 2;
            next->args.row--;
        }

        next = ListPoolNext(&next->node);
    }

    node->done = TRUE;
    TaskPoolUpdate(&work->taskpool);
    ShowCollectionCardPreview(work);
#ifdef VERSION_EU
    SetGridRowCount(work, work->gridEntryCount);
#else
    SetGridRowCount(work, work->entryCount);
#endif
    UpdateGridScrollBar(work);
}

u8 UpdateDeckMenuAddValueSelect(DeckMenuWork* work, void* task) {
    s8 n;

    if (work->popupActive != 0) {
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        work->exitRequested = FALSE;
    }

    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    switch (GetKeysRepeat()) {
    case DPAD_LEFT:
    case (DPAD_LEFT | DPAD_UP):
    case (DPAD_LEFT | DPAD_DOWN):
        if (work->cursorCol > 0) {
            work->cursorCol--;
            work->timer = 1;

            if ((u8)MoveValueCursor(work, DPAD_LEFT)) {
                m4aSongNumStart(SONG_SYS_CLICK);
            }
        }

        DrawSelectedValueCpCost(work);
        break;
    case DPAD_RIGHT:
    case (DPAD_RIGHT | DPAD_UP):
    case (DPAD_RIGHT | DPAD_DOWN):
        if (work->cursorCol <= 0) {
            work->cursorCol++;
            work->timer = 1;

            if ((u8)MoveValueCursor(work, DPAD_RIGHT)) {
                m4aSongNumStart(SONG_SYS_CLICK);
            }
        }

        DrawSelectedValueCpCost(work);
        break;
    case DPAD_UP:
        n = work->cursorRow;

        if (work->cursorRow > 0) {
            work->cursorRow = work->cursorRow - 1;
        } else {
            work->cursorRow = 4;
        }

        work->timer = 1;
        MoveValueCursor(work, DPAD_UP);

        if (n != work->cursorRow) {
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        DrawSelectedValueCpCost(work);
        break;
    case DPAD_DOWN:
        n = work->cursorRow;

        if (work->cursorRow <= 3) {
            work->cursorRow = work->cursorRow + 1;
        } else {
            work->cursorRow = 0;
        }

        work->timer = 1;
        MoveValueCursor(work, DPAD_DOWN);

        if (n != work->cursorRow) {
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        DrawSelectedValueCpCost(work);
        break;
    }

    switch (GetKeysPressed()) {
    case B_BUTTON:
        SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
        n = work->savedCol;
        work->cursorCol = n;
        n = work->savedRow;
        work->cursorRow = n;
        work->handX = gCollectionGridColumnX[work->cursorCol] << 8;
        work->handY = gCollectionGridRowY[work->cursorRow] << 8;
        ShowCollectionCardPreview(work);
        DrawCpCost(0);
        work->view = DECK_MENU_VIEW_ADD_GRID;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuAddGrid);
        m4aSongNumStart(SONG_SYS_CLOSE);
        return 1;
    case A_BUTTON:
        if (GetDeckCardCount(work->deckIndex) <= 98) {
            AddSelectedValueCardToDeck(work);
            DrawDeckCardCount(work->deckIndex);
            DrawDeckCpCost(work->deckIndex);
            work->deckAttackCount = CountDeckCardsOfCategory(CARD_CATEGORY_ATTACK, work->deckIndex);
            work->deckMagicCount = CountDeckCardsOfCategory(CARD_CATEGORY_MAGIC, work->deckIndex);
            work->deckItemCount = CountDeckCardsOfCategory(CARD_CATEGORY_ITEM, work->deckIndex);
            work->deckEnemyCount = CountDeckCardsOfCategory(CARD_CATEGORY_ENEMY, work->deckIndex);
            DrawDeckCategoryCount(work->deckAttackCount, CARD_CATEGORY_ATTACK);
            DrawDeckCategoryCount(work->deckMagicCount, CARD_CATEGORY_MAGIC);
            DrawDeckCategoryCount(work->deckItemCount, CARD_CATEGORY_ITEM);
            DrawDeckCategoryCount(work->deckEnemyCount, CARD_CATEGORY_ENEMY);
            DrawCardTotals();
            CountCardsNotInDeckByCategory(work->deckIndex, work->collectionCategoryCounts);
            DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ATTACK], CARD_CATEGORY_ATTACK);
            DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_MAGIC], CARD_CATEGORY_MAGIC);
            DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ITEM], CARD_CATEGORY_ITEM);
            DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ENEMY], CARD_CATEGORY_ENEMY);
            work->timer = 1;

            if (!(u8)MoveValueCursor(work, 0)) {
                RemoveEmptyCollectionEntry(work, FALSE);

                if (work->gridEntryCount != 0) {
                    u8 ready;
                    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
                    work->cursorCol = work->savedCol;
                    work->cursorRow = work->savedRow;

                    while ((ready = IsCardAtCursor(work)) == 0) {
                        if (--work->cursorCol < 0) {
                            if (--work->cursorRow < 0) {
                                ScrollGridUp(work, FALSE);
                                work->cursorRow = ready;
                            }

                            work->cursorCol = 2;
                        }
                    }

                    work->handX = gCollectionGridColumnX[work->cursorCol] << 8;
                    work->handY = gCollectionGridRowY[work->cursorRow] << 8;
                    ShowCollectionCardPreview(work);
                    work->view = DECK_MENU_VIEW_ADD_GRID;
                    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuAddGrid);
                    return 1;
                } else {
                    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
                    work->cursorCol = work->categoryFilter;
                    work->timer = 1;
                    work->view = DECK_MENU_VIEW_ADD_FILTER;
                    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
                    work->gridEntryCount = 0;
                    return 1;
                }
            } else {
                DrawSelectedValueCpCost(work);
            }
        } else {
            m4aSongNumStart(SONG_SYS_BEEP);
            TaskCreate(&work->cardpool, &gTaskDescDeckErrorDeckFull, &work->popupActive);
            return 1;
        }

        break;
    case L_BUTTON:
        FreeCollectionEntries(work);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenRemoveMode);
        m4aSongNumStart(SONG_SYS_CANSEL);
        return 1;
    case START_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        return 1;
    }

    ApproachValueHalf(&work->handX, sValueGridX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, sValueGridY[work->cursorRow] << 8);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuCollectionFilter(DeckMenuWork* work, void* task) {
    u16 i;
    u16 n;

    work->gfx = AnimUpdate(&work->anim2);

    if (work->popupActive != 0) {
        ApproachValueHalf(&work->handX, sCollectionFilterTabX[work->cursorCol] << 8);
        ApproachValueHalf(&work->handY, 0x1E00);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        work->exitRequested = FALSE;
    }

    switch (GetKeysRepeat()) {
    case DPAD_LEFT:
        if (work->cursorCol > CATEGORY_FILTER_ATTACK) {
            work->cursorCol--;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICK);
            n = work->cursorCol;
            work->categoryFilter = n;
            DrawCollectionFilterTab(work->categoryFilter, work->mode);
            ClearCardGrid(work);

            if (work->view == DECK_MENU_VIEW_ADD_FILTER) {
                work->gridEntryCount = CreateCollectionGridCards(work, work->categoryFilter, FALSE);
            } else {
                work->gridEntryCount = CreateCollectionGridCards(work, work->categoryFilter, TRUE);
            }
        }

        for (i = 0; i <= 9; i++) {
            DrawValueCount(0, i);
        }

        break;
    case DPAD_RIGHT:
        if (work->cursorCol <= CATEGORY_FILTER_ENEMY) {
            work->cursorCol++;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICK);
            n = work->cursorCol;
            work->categoryFilter = n;
            DrawCollectionFilterTab(work->categoryFilter, work->mode);
            ClearCardGrid(work);

            if (work->view == DECK_MENU_VIEW_ADD_FILTER) {
                work->gridEntryCount = CreateCollectionGridCards(work, work->categoryFilter, FALSE);
            } else {
                work->gridEntryCount = CreateCollectionGridCards(work, work->categoryFilter, TRUE);
            }
        }

        for (i = 0; i <= 9; i++) {
            DrawValueCount(0, i);
        }

        break;
    case DPAD_DOWN:
        if (work->gridEntryCount != 0) {
            work->cursorCol = 0;
            work->cursorRow = 0;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
            ShowCollectionCardPreview(work);

            if (work->view == DECK_MENU_VIEW_ADD_FILTER) {
                work->view = DECK_MENU_VIEW_ADD_GRID;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuAddGrid);
            }

            if (work->view == DECK_MENU_VIEW_DELETE_FILTER) {
                work->view = DECK_MENU_VIEW_DELETE_GRID;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteGrid);
            }
        } else {
            m4aSongNumStart(SONG_SYS_BEEP);
        }

        work->thumbX = 0xA000;
        work->thumbY = 0x2800;
        work->scrollRowEnd = 4;
        return 1;
    case B_BUTTON:
        if (work->gridEntryCount != 0) {
            work->cursorCol = 0;
            work->cursorRow = 0;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
            ShowCollectionCardPreview(work);

            if (work->view == DECK_MENU_VIEW_ADD_FILTER) {
                work->view = DECK_MENU_VIEW_ADD_GRID;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuAddGrid);
            }

            if (work->view == DECK_MENU_VIEW_DELETE_FILTER) {
                work->view = DECK_MENU_VIEW_DELETE_GRID;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteGrid);
            }
        } else {
            m4aSongNumStart(SONG_SYS_CANSEL);

            if (work->view == DECK_MENU_VIEW_ADD_FILTER) {
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseAddMode);
            }

            if (work->view == DECK_MENU_VIEW_DELETE_FILTER) {
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseDeleteMode);
            }
        }

        work->thumbX = 0xA000;
        work->thumbY = 0x2800;
        work->scrollRowEnd = 4;
        return 1;
    case L_BUTTON:
        if (work->view == DECK_MENU_VIEW_ADD_FILTER) {
            FadeStartIn(FADE_MODE_BLACK, 1);
            FreeCollectionEntries(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenRemoveMode);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        break;
    case START_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        return 1;
    }

    ApproachValueHalf(&work->handX, sCollectionFilterTabX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, 0x1E00);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuClearPrompt(DeckMenuWork* work, void* task) {
    PromptChoiceLayout layout;

    layout = sDeckPromptChoiceLayout;
    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (GetKeysPressed() & DPAD_LEFT) {
        if (work->promptChoice != 0) {
            work->promptChoice--;
        }

        work->timer = 1;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysPressed() & DPAD_RIGHT) {
        if (work->promptChoice == 0) {
            work->promptChoice++;
        }

        work->timer = 1;
        m4aSongNumStart(SONG_SYS_CLICK);
    }

    if (GetKeysPressed() & A_BUTTON) {
        if (work->promptChoice == 1) {
            work->timer = 1;
            work->view = DECK_MENU_VIEW_COMMANDS;
            SetDeckMenuHandAnim(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCommands);
            TaskPoolUpdate(&work->taskpool);
            TaskPoolUpdate(&work->cardpool);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        m4aSongNumStart(SONG_SYS_KETTEI);
        ClearDeck(work->deckIndex);
        DrawDeckCardCount(work->deckIndex);
        DrawDeckCpCost(work->deckIndex);
        DrawCardTotals();
        work->deckAttackCount = CountDeckCardsOfCategory(CARD_CATEGORY_ATTACK, work->deckIndex);
        work->deckMagicCount = CountDeckCardsOfCategory(CARD_CATEGORY_MAGIC, work->deckIndex);
        work->deckItemCount = CountDeckCardsOfCategory(CARD_CATEGORY_ITEM, work->deckIndex);
        work->deckEnemyCount = CountDeckCardsOfCategory(CARD_CATEGORY_ENEMY, work->deckIndex);
        DrawDeckCategoryCount(work->deckAttackCount, CARD_CATEGORY_ATTACK);
        DrawDeckCategoryCount(work->deckMagicCount, CARD_CATEGORY_MAGIC);
        DrawDeckCategoryCount(work->deckItemCount, CARD_CATEGORY_ITEM);
        DrawDeckCategoryCount(work->deckEnemyCount, CARD_CATEGORY_ENEMY);
        ClearCardGrid(work);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        work->timer = 1;
        work->view = DECK_MENU_VIEW_COMMANDS;
        SetDeckMenuHandAnim(work);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCommands);
    }

    if (GetKeysPressed() & B_BUTTON) {
        work->timer = 1;
        work->view = DECK_MENU_VIEW_COMMANDS;
        SetDeckMenuHandAnim(work);
        m4aSongNumStart(SONG_SYS_CANSEL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCommands);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        return 1;
    }

    ApproachValueHalf(&work->handX, layout.x[work->promptChoice] << 8);
    ApproachValueHalf(&work->handY, 0x7200);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuDeckSelect(DeckMenuWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

#ifdef VERSION_EU
    if (FadeIsActive()) {
        return 1;
    }
#endif

    if (work->popupActive != 0) {
        ApproachValueHalf(&work->handX, sDeckTabPointerX[work->cursorCol] << 8);
        ApproachValueHalf(&work->handY, sDeckTabPointerY[work->cursorRow] << 8);
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        work->inputDelay = 4;
        return 1;
    }

    if (work->inputDelay > 0) {
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        work->inputDelay--;
        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        } else {
            work->exitRequested = FALSE;
        }
    }

    switch (GetKeysRepeat()) {
    case DPAD_UP:
        if (work->cursorRow > 0) {
            work->cursorRow--;
            work->timer = 1;
            work->deckIndex = work->cursorRow;
            ClearCardGrid(work);
            CreateDeckGridCards(work, work->categoryFilter);
            work->deckAttackCount = CountDeckCardsOfCategory(CARD_CATEGORY_ATTACK, work->deckIndex);
            work->deckMagicCount = CountDeckCardsOfCategory(CARD_CATEGORY_MAGIC, work->deckIndex);
            work->deckItemCount = CountDeckCardsOfCategory(CARD_CATEGORY_ITEM, work->deckIndex);
            work->deckEnemyCount = CountDeckCardsOfCategory(CARD_CATEGORY_ENEMY, work->deckIndex);
            DrawDeckCategoryCount(work->deckAttackCount, CARD_CATEGORY_ATTACK);
            DrawDeckCategoryCount(work->deckMagicCount, CARD_CATEGORY_MAGIC);
            DrawDeckCategoryCount(work->deckItemCount, CARD_CATEGORY_ITEM);
            DrawDeckCategoryCount(work->deckEnemyCount, CARD_CATEGORY_ENEMY);
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        break;
    case DPAD_DOWN:
        if (work->cursorRow < 2) {
            work->cursorRow++;
            work->deckIndex = work->cursorRow;
            work->timer = 1;
            ClearCardGrid(work);
            CreateDeckGridCards(work, work->categoryFilter);
            work->deckAttackCount = CountDeckCardsOfCategory(CARD_CATEGORY_ATTACK, work->deckIndex);
            work->deckMagicCount = CountDeckCardsOfCategory(CARD_CATEGORY_MAGIC, work->deckIndex);
            work->deckItemCount = CountDeckCardsOfCategory(CARD_CATEGORY_ITEM, work->deckIndex);
            work->deckEnemyCount = CountDeckCardsOfCategory(CARD_CATEGORY_ENEMY, work->deckIndex);
            DrawDeckCategoryCount(work->deckAttackCount, CARD_CATEGORY_ATTACK);
            DrawDeckCategoryCount(work->deckMagicCount, CARD_CATEGORY_MAGIC);
            DrawDeckCategoryCount(work->deckItemCount, CARD_CATEGORY_ITEM);
            DrawDeckCategoryCount(work->deckEnemyCount, CARD_CATEGORY_ENEMY);
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        break;

    case DPAD_LEFT:
        if (work->cursorRow == 2) {
            work->cursorRow = 3;
        }

        work->cursorCol = 2;
        work->timer = 1;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuEnterDeckGrid);
        m4aSongNumStart(SONG_SYS_CANSEL);
        return 1;
    }

    switch (GetKeysPressed()) {
    case A_BUTTON:
        work->timer = 1;
        work->prevView = work->view;
        m4aSongNumStart(SONG_SYS_CANSEL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenCommands);
        return 1;
    case SELECT_BUTTON:
#ifndef VERSION_EU
        work->scrollRowEnd = 4;
#endif
        SetActiveDeckIndex(work->deckIndex);
        DrawDeckEquipMarker(work->deckIndex);
        TaskCreate(&work->cardpool, &gTaskDescDeckEquip, &work->popupActive);
        m4aSongNumStart(SONG_SYS_DECKSET);
        break;
    case B_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuStartSlideOut);
            work->result = DECK_MENU_RESULT_RETURN_TO_MENU;
            m4aSongNumStart(SONG_SYS_CLOSE);
        }

        return 1;
    case START_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        return 1;
    case L_BUTTON:
        FreeCollectionEntries(work);
        m4aSongNumStart(SONG_SYS_CANSEL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenRemoveMode);
        return 1;
    case R_BUTTON:
        FreeCollectionEntries(work);
        m4aSongNumStart(SONG_SYS_CANSEL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenAddMode);
        return 1;
    }

    HighlightDeckTab(work, work->deckIndex);
    ApproachValueHalf(&work->handX, sDeckTabPointerX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, sDeckTabPointerY[work->cursorRow] << 8);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuOpenCommands(DeckMenuWork* work, void* task) {
    u8* view;
    u8 z;

#ifdef VERSION_EU
    work->tiles3 = LoadObjTiles(gDeckCommandMenuTilesByLanguage[gLanguage], sDeckCommandMenuTileSizes[gLanguage]);
#else
    work->tiles3 = LoadObjTiles(gDeckCommandMenuTiles, sizeof(gDeckCommandMenuTiles));
#endif
    work->palette2 = LoadObjPalette(gDeckCommandMenuPalette, sizeof(gDeckCommandMenuPalette));
    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_ROW);
    view = &work->view;
    z = 0;
    *view = DECK_MENU_VIEW_COMMANDS;
    work->commandCursor = z;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCommands);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuCommands(DeckMenuWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (work->popupActive != 0) {
        work->view = DECK_MENU_VIEW_DECK_SELECT;
        SetDeckMenuHandAnim(work);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseCommands);
    }

    switch (GetKeysRepeat()) {
    case DPAD_UP:
        if (work->commandCursor != DECK_COMMAND_EQUIP) {
            work->commandCursor--;
        } else {
            work->commandCursor = DECK_COMMAND_DELETE;
        }

        m4aSongNumStart(SONG_SYS_CLICK);
        work->timer = 1;
        break;
    case DPAD_DOWN:
        if (work->commandCursor < DECK_COMMAND_DELETE) {
            work->commandCursor++;
        } else {
            work->commandCursor = DECK_COMMAND_EQUIP;
        }

        work->timer = 1;
        m4aSongNumStart(SONG_SYS_CLICK);
        break;
    }

    switch (GetKeysPressed()) {
    case B_BUTTON:
        work->timer = 1;

        if (work->prevView == DECK_MENU_VIEW_DECK_GRID) {
            work->view = DECK_MENU_VIEW_DECK_GRID;
            SetDeckMenuHandAnim(work);
            SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
            m4aSongNumStart(SONG_SYS_CLOSE);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckGrid);
            ReleaseCommandMenuGfx(work);
        } else {
            work->view = DECK_MENU_VIEW_DECK_SELECT;
            SetDeckMenuHandAnim(work);
            m4aSongNumStart(SONG_SYS_CLOSE);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseCommands);
        }

        break;
    case START_BUTTON:
        work->exitRequested = TRUE;
        work->timer = 1;

        if (work->prevView == DECK_MENU_VIEW_DECK_GRID) {
            work->view = DECK_MENU_VIEW_DECK_GRID;
            SetDeckMenuHandAnim(work);
            SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
            m4aSongNumStart(SONG_SYS_CLOSE);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckGrid);
            ReleaseCommandMenuGfx(work);
        } else {
            m4aSongNumStart(SONG_SYS_CLOSE);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseCommands);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        break;
    case A_BUTTON:
        switch (work->commandCursor) {
        case DECK_COMMAND_EQUIP:
            SetActiveDeckIndex(work->deckIndex);
            DrawDeckEquipMarker(work->deckIndex);
            TaskCreate(&work->cardpool, &gTaskDescDeckEquip, &work->popupActive);
            m4aSongNumStart(SONG_SYS_DECKSET);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseCommands);
            return 1;
        case DECK_COMMAND_RENAME:
            m4aSongNumStart(SONG_SYS_KETTEI);
            ClearCardGrid(work);
            work->step = 0;
            ReleaseCommandMenuGfx(work);
#ifdef VERSION_EU
            FadeStartIn(FADE_MODE_BLACK, 16);
#endif
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenKeyboard);
            break;
        case DECK_COMMAND_CLEAR:
            m4aSongNumStart(SONG_SYS_KETTEI);
            work->view = DECK_MENU_VIEW_CLEAR_PROMPT;
            TaskCreate(&work->cardpool, &gTaskDescDeckClear, &work->popupActive);
            work->promptChoice = 1;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuClearPrompt);
            return 1;
        case DECK_COMMAND_ADD:
            m4aSongNumStart(SONG_SYS_KETTEI);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenAddMode);
            break;
        case DECK_COMMAND_REMOVE:
            m4aSongNumStart(SONG_SYS_KETTEI);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenRemoveMode);
            break;
        case DECK_COMMAND_DELETE:
            m4aSongNumStart(SONG_SYS_KETTEI);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenDeleteMode);
            break;
        }

        break;
    }

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        ApproachValueHalf(&work->handX, 0x6600);
        break;
    case LANGUAGE_FRENCH:
        ApproachValueHalf(&work->handX, 0x6200);
        break;
    case LANGUAGE_GERMAN:
        ApproachValueHalf(&work->handX, 0x5E00);
        break;
    case LANGUAGE_ITALIAN:
        ApproachValueHalf(&work->handX, 0x6200);
        break;
    case LANGUAGE_SPANISH:
        ApproachValueHalf(&work->handX, 0x5E00);
        break;
    default:
        ApproachValueHalf(&work->handX, 0x6600);
        break;
    }
#else
    ApproachValueHalf(&work->handX, 0x6600);
#endif
    ApproachValueHalf(&work->handY, sDeckCommandY[work->commandCursor] << 8);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuCloseCommands(DeckMenuWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);
    ReleaseCommandMenuGfx(work);
    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);

    switch (work->commandCursor) {
    case DECK_COMMAND_EQUIP:
    case DECK_COMMAND_RENAME:
    case DECK_COMMAND_CLEAR:
    case DECK_COMMAND_ADD:
    case DECK_COMMAND_REMOVE:
    case DECK_COMMAND_DELETE:
        if (work->prevView == DECK_MENU_VIEW_DECK_GRID) {
            work->view = DECK_MENU_VIEW_DECK_GRID;
            SetDeckMenuHandAnim(work);
            work->timer = 4;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckGrid);
        } else {
            work->view = DECK_MENU_VIEW_DECK_SELECT;
            work->timer = 4;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckSelect);
        }

        break;
    }

    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuOpenAddMode(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 4);

    switch (work->deckIndex) {
    case 0:
#ifdef VERSION_EU
        SetupBg(0, 0, 31, 0);
#else
        SetupBg(0, 3, 31, 0);
#endif
#ifdef VERSION_EU
        SetupBg(1, 0, 28, 0);
#else
        SetupBg(1, 0, 23, 0);
#endif
#ifdef VERSION_EU
        SetupBg(2, 0, 29, 0);
#else
        SetupBg(2, 0, 15, 0);
#endif
        SetupBg(3, 0, 30, 0);
        LoadBgMap(0, gDeck1PanelMap, 0x180);
        LoadBgMap(1, gDeckCollectionInfoMap, sizeof(gDeckCollectionInfoMap));
        LoadBgMap(2, gDeckCardsInUseMap, sizeof(gDeckCardsInUseMap));
        SetBgScroll(0, 0, 0xFFF0);
        SetBgScroll(1, 0, 0);
        SetBgScroll(2, 0, 0);
        work->deckNameX = 16;
        work->deckNameY = 28;
        break;
    case 1:
        SetupBg(0, 0, 31, 0);
#ifdef VERSION_EU
        SetupBg(1, 0, 28, 0);
#else
        SetupBg(1, 2, 23, 0);
#endif
#ifdef VERSION_EU
        SetupBg(2, 0, 29, 0);
#else
        SetupBg(2, 0, 15, 0);
#endif
        SetupBg(3, 0, 30, 0);
        LoadBgMap(0, gDeckCollectionInfoMap, sizeof(gDeckCollectionInfoMap));
        LoadBgMap(1, gDeck2PanelMap, 0x180);
        LoadBgMap(2, gDeckCardsInUseMap, sizeof(gDeckCardsInUseMap));
        SetBgScroll(0, 0, 0);
        SetBgScroll(1, 0, 0xFFF0);
        SetBgScroll(2, 0, 0);
        work->deckName2X = 16;
        work->deckName2Y = 28;
        break;
    case 2:
        SetupBg(0, 0, 31, 0);
#ifdef VERSION_EU
        SetupBg(1, 0, 28, 0);
#else
        SetupBg(1, 0, 23, 0);
#endif
#ifdef VERSION_EU
        SetupBg(2, 0, 29, 0);
#else
        SetupBg(2, 1, 15, 0);
#endif
        SetupBg(3, 0, 30, 0);
        LoadBgMap(0, gDeckCollectionInfoMap, sizeof(gDeckCollectionInfoMap));
        LoadBgMap(1, gDeckCardsInUseMap, sizeof(gDeckCardsInUseMap));
        LoadBgMap(2, gDeck3PanelMap, 0x180);
        SetBgScroll(0, 0, 0);
        SetBgScroll(1, 0, 0);
        SetBgScroll(2, 0, 0xFFF0);
        work->deckName3X = 16;
        work->deckName3Y = 28;
        break;
    }

    CountCardsNotInDeckByCategory(work->deckIndex, work->collectionCategoryCounts);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ATTACK], CARD_CATEGORY_ATTACK);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_MAGIC], CARD_CATEGORY_MAGIC);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ITEM], CARD_CATEGORY_ITEM);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ENEMY], CARD_CATEGORY_ENEMY);
    work->step = 0;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuBuildAddList);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuBuildAddList(DeckMenuWork* work, void* task) {
    u16 i;
    u16 n;
    u16 zero;
    CardKindEntry* entries;
    u16* count;
    u8* categoryFilter;
    u8* view;

    FadeStartIn(FADE_MODE_BLACK, 4);

    switch (work->step) {
    case 0:
        view = &work->view;
        entries = NULL;
        *view = DECK_MENU_VIEW_ADD_GRID;
        work->descriptionX = 7;
        work->descriptionY = 130;
        ReleaseCommandMenuGfx(work);
        SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
        ClearCardGrid(work);
        LoadBgMap(3, gDeckAddGridMap, sizeof(gDeckAddGridMap));
        count = &work->entryCount;
        *count = n = 0x11E;
        work->kindEntries = EwramAlloc(n * sizeof(CardKindEntry));
        CpuFill32(0, work->kindEntries, *count * sizeof(CardKindEntry));
        work->entries = entries;
        break;
    case 1:
        CountCardsNotInDeckByKind(work->kindEntries, work->deckIndex, TRUE, work->entryCount, work->unk_4FC);
        break;
    case 2:
        work->entryCount = ListCardsNotInDeckByKind(work->kindEntries, work->deckIndex, TRUE, work->entryCount, work->unk_4FC);
        break;
    case 3:
        if (work->entryCount != 0) {
            BuildCollectionEntries(work);
        }

        break;
    case 4:
        for (i = 0; i < 0x11E; i++) {
            if (work->kindEntries[i].count != 0) {
                EwramFree(work->kindEntries[i].indices);
            }
        }

        EwramFree(work->kindEntries);
        break;
    case 5:
        categoryFilter = &work->categoryFilter;
        zero = 0;
        *categoryFilter = CATEGORY_FILTER_COLLECTION_ALL;
        work->gridEntryCount = CreateCollectionGridCards(work, 5, FALSE);
        SetDeckMenuHandAnim(work);
        work->handX = gCollectionGridColumnX[0] << 8;
        work->handY = gCollectionGridRowY[0] << 8;
        work->mode = DECK_MENU_MODE_ADD;
        work->cursorCol = zero;
        work->cursorRow = zero;
        ShowCollectionCardPreview(work);
        DrawCpCost(0);

        if (work->gridEntryCount != 0) {
            work->step = 4;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuAddGrid);
        } else {
            work->cursorCol = *categoryFilter;
            work->timer = 1;
            work->view = DECK_MENU_VIEW_ADD_FILTER;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
        }

        break;
    }

    work->step++;
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuAddGrid(DeckMenuWork* work, void* task) {
    u16 i;

#ifdef VERSION_EU
    if (FadeIsActive()) {
        return 1;
    }
#endif

    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (work->popupActive != 0) {
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        } else {
            work->exitRequested = FALSE;
        }
    }

    if (work->step == 0) {
        switch (GetKeysRepeat()) {
        case DPAD_UP:
            if (work->cursorRow > 0) {
                if (IsCardAt(work, work->cursorCol, work->cursorRow - 1)) {
                    work->cursorRow--;
                    work->timer = 1;
                    m4aSongNumStart(SONG_SYS_CLICKI04B);
                }
            } else {
                if (!ScrollGridUp(work, TRUE)) {
                    work->cursorCol = work->categoryFilter;
                    work->timer = 1;
                    m4aSongNumStart(SONG_SYS_CLICKI04B);
                    work->view = DECK_MENU_VIEW_ADD_FILTER;

                    for (i = 0; i < 10; i++) {
                        DrawValueCount(0, i);
                    }

                    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
                    return 1;
                }
            }

            ShowCollectionCardPreview(work);
            break;
        case DPAD_DOWN:
            if (work->cursorRow <= 2) {
                if (IsCardAt(work, work->cursorCol, work->cursorRow + 1)) {
                    work->cursorRow++;
                    work->timer = 1;
                    m4aSongNumStart(SONG_SYS_CLICKI04B);
                }
            } else {
                if (IsCardAt(work, work->cursorCol, work->cursorRow + 1)) {
                    ScrollGridDown(work);
                }
            }

            ShowCollectionCardPreview(work);
            break;
        case DPAD_LEFT:
            if (work->cursorCol > 0) {
                if (IsCardAt(work, work->cursorCol - 1, work->cursorRow)) {
                    work->cursorCol--;
                    work->timer = 1;
                    m4aSongNumStart(SONG_SYS_CLICKI04B);
                }
            }

            ShowCollectionCardPreview(work);
            break;
        case DPAD_RIGHT:
            if (work->cursorCol > 1) {
                work->timer = 1;
                return 1;
            }

            if (IsCardAt(work, work->cursorCol + 1, work->cursorRow)) {
                work->cursorCol++;
                work->timer = 1;
                m4aSongNumStart(SONG_SYS_CLICKI04B);
            }

            ShowCollectionCardPreview(work);
            break;
        }

        switch (GetKeysPressed()) {
        case A_BUTTON:
            if ((u8)IsCardAtCursor(work)) {
                work->savedCol = work->cursorCol;
                work->savedRow = work->cursorRow;
                work->cursorCol = 0;
                work->cursorRow = 0;

                if ((u8)MoveValueCursor(work, 0)) {
                    m4aSongNumStart(SONG_SYS_KETTEI);
                    DrawSelectedValueCpCost(work);
                    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_ROW);
                    work->view = DECK_MENU_VIEW_ADD_VALUE_SELECT;
                    work->handX = sValueGridX[work->cursorCol] << 8;
                    work->handY = sValueGridY[work->cursorRow] << 8;
                    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuAddValueSelect);
                    return 1;
                } else {
                    work->cursorCol = work->savedCol;
                    work->cursorRow = work->savedRow;
                    m4aSongNumStart(SONG_SYS_BEEP);
                    return 1;
                }
            } else {
                m4aSongNumStart(SONG_SYS_BEEP);
                return 1;
            }
        case B_BUTTON:
            FadeStartIn(FADE_MODE_BLACK, 1);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseAddMode);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        case L_BUTTON:
            FadeStartIn(FADE_MODE_BLACK, 1);
            FreeCollectionEntries(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenRemoveMode);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        case START_BUTTON:
            if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
                FadeStartOut(FADE_MODE_BLACK, 4);
                m4aSongNumStart(SONG_SYS_CLOSE);
                work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            }

            return 1;
        }

        if (GetKeysPressed() & SELECT_BUTTON) {
            work->cursorRow = 0;
            ResetGridScroll(work);
            work->cursorCol = work->categoryFilter;
            work->timer = 1;
            work->thumbX = 0xA000;
            work->thumbY = 0x2800;
            work->scrollRowEnd = 4;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
            work->view = DECK_MENU_VIEW_ADD_FILTER;

            for (i = 0; i < 10; i++) {
                DrawValueCount(0, i);
            }

            TaskPoolUpdate(&work->taskpool);
            TaskPoolUpdate(&work->cardpool);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
            return 1;
        }
    } else {
        work->step--;
    }

    ApproachValueHalf(&work->handX, gCollectionGridColumnX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, gCollectionGridRowY[work->cursorRow] << 8);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuCloseAddMode(DeckMenuWork* work, void* task) {
    u8* view;

    FadeStartIn(FADE_MODE_BLACK, 4);
    view = &work->view;
    *view = DECK_MENU_VIEW_ADD_GRID;
    work->removeLabelX = 95;
    ReleaseCardPreview(work);
    SetupBg(3, 0, 30, 0);
#ifdef VERSION_EU
    SetupBg(2, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(0, 0, 31, 0);
#else
    SetupBg(2, 1, 15, 0);
    SetupBg(1, 2, 23, 0);
    SetupBg(0, 3, 31, 0);
#endif
    LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(2, gDefaultBgMap, sizeof(gDefaultBgMap));
    HighlightDeckTab(work, work->deckIndex);
    LoadBgMap(3, gDeckReviewGridMap, sizeof(gDeckReviewGridMap));
    FreeCollectionEntries(work);
    ClearCardGrid(work);
    work->categoryFilter = CATEGORY_FILTER_DECK_ALL;
    work->cursorCol = 0;
    work->cursorRow = work->deckIndex;
    CreateDeckGridCards(work, work->categoryFilter);
    work->mode = DECK_MENU_MODE_NONE;
    *view = DECK_MENU_VIEW_DECK_SELECT;
    SetDeckMenuHandAnim(work);
    ApproachValueHalf(&work->handX, sDeckTabPointerX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, sDeckTabPointerY[work->cursorRow] << 8);
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckSelect);
    LoadPalette(gDeckMenuTextPalette,
                (void*)(work->palette4->index * 32 +
                        OBJ_PLTT),
                work->palette4->count << 5);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuOpenRemoveMode(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 4);

    switch (work->deckIndex) {
    case 0:
#ifdef VERSION_EU
        SetupBg(0, 0, 31, 0);
#else
        SetupBg(0, 3, 31, 0);
#endif
#ifdef VERSION_EU
        SetupBg(1, 0, 28, 0);
#else
        SetupBg(1, 0, 23, 0);
#endif
#ifdef VERSION_EU
        SetupBg(2, 0, 29, 0);
#else
        SetupBg(2, 0, 15, 0);
#endif
        SetupBg(3, 0, 30, 0);
        LoadBgMap(0, gDeck1PanelMap, 0x180);
        LoadBgMap(1, gDeckDescriptionWindowMap, sizeof(gDeckDescriptionWindowMap));
        LoadBgMap(2, gDeckCpLabelMap, sizeof(gDeckCpLabelMap));
        SetBgScroll(0, (u16)-88, (u16)-16);
        SetBgScroll(1, (u16)-88, (u16)-112);
        SetBgScroll(2, (u16)-140, (u16)-96);
        work->deckNameX = 102;
        work->deckNameY = 28;
        break;
    case 1:
        SetupBg(0, 0, 31, 0);
#ifdef VERSION_EU
        SetupBg(1, 0, 28, 0);
#else
        SetupBg(1, 2, 23, 0);
#endif
#ifdef VERSION_EU
        SetupBg(2, 0, 29, 0);
#else
        SetupBg(2, 0, 15, 0);
#endif
        SetupBg(3, 0, 30, 0);
        LoadBgMap(0, gDeckDescriptionWindowMap, sizeof(gDeckDescriptionWindowMap));
        LoadBgMap(1, gDeck2PanelMap, 0x180);
        LoadBgMap(2, gDeckCpLabelMap, sizeof(gDeckCpLabelMap));
        SetBgScroll(0, (u16)-88, (u16)-112);
        SetBgScroll(1, (u16)-88, (u16)-16);
        SetBgScroll(2, (u16)-140, (u16)-96);
        work->deckName2X = 102;
        work->deckName2Y = 28;
        break;
    case 2:
        SetupBg(0, 0, 31, 0);
#ifdef VERSION_EU
        SetupBg(1, 0, 28, 0);
#else
        SetupBg(1, 0, 23, 0);
#endif
#ifdef VERSION_EU
        SetupBg(2, 0, 29, 0);
#else
        SetupBg(2, 1, 15, 0);
#endif
        SetupBg(3, 0, 30, 0);
        LoadBgMap(0, gDeckDescriptionWindowMap, sizeof(gDeckDescriptionWindowMap));
        LoadBgMap(1, gDeckCpLabelMap, sizeof(gDeckCpLabelMap));
        LoadBgMap(2, gDeck3PanelMap, 0x180);
        SetBgScroll(0, (u16)-88, (u16)-112);
        SetBgScroll(1, (u16)-140, (u16)-96);
        SetBgScroll(2, (u16)-88, (u16)-16);
        work->deckName3X = 102;
        work->deckName3Y = 28;
        break;
    }

    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuBuildRemoveGrid);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuBuildRemoveGrid(DeckMenuWork* work, void* task) {
    work->descriptionX = 94;
    work->descriptionY = 130;
    work->view = DECK_MENU_VIEW_REMOVE_GRID;
    ReleaseCommandMenuGfx(work);
    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
    SetDeckMenuHandAnim(work);
    LoadBgMap(3, gDeckRemoveGridMap, sizeof(gDeckRemoveGridMap));
    ClearCardGrid(work);
    work->categoryFilter = CATEGORY_FILTER_DECK_ALL;
    work->cursorCol = 0;
    work->cursorRow = 0;
    CreateDeckGridCards(work, work->categoryFilter);
    work->handX = gDeckGridColumnX[work->cursorCol] << 8;
    work->handY = gDeckGridRowY[work->cursorRow] << 8;
    ShowDeckCardPreview(work);
    work->mode = DECK_MENU_MODE_REMOVE;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuRemoveGrid);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuRemoveGrid(DeckMenuWork* work, void* task) {
    u8 categoryFilter;

    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

#ifdef VERSION_EU
    if (FadeIsActive()) {
        return 1;
    }
#endif

    if (work->popupActive != 0) {
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        work->exitRequested = FALSE;
    }

    switch (GetKeysRepeat()) {
    case DPAD_LEFT:
        if (work->cursorCol > 0) {
            work->cursorCol--;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        ShowDeckCardPreview(work);
        break;
    case DPAD_RIGHT:
        if (work->cursorCol <= 1) {
            work->cursorCol++;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        ShowDeckCardPreview(work);
        break;
    case DPAD_UP:
        if (work->cursorRow > 0) {
            work->cursorRow--;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        } else if (!ScrollGridUp(work, TRUE)) {
            categoryFilter = work->categoryFilter;
            work->cursorCol = categoryFilter;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
            work->view = DECK_MENU_VIEW_REMOVE_FILTER;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckFilter);
            DrawCpCost(0);
            return 1;
        }

        ShowDeckCardPreview(work);
        break;
    case DPAD_DOWN:
        if (work->cursorRow <= 2) {
            work->cursorRow++;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        } else {
            ScrollGridDown(work);
        }

        ShowDeckCardPreview(work);
        break;
    }

    switch (GetKeysPressed()) {
    case A_BUTTON:
        RemoveCursorCardFromDeck(work);
        ShowDeckCardPreview(work);
        DrawDeckCardCount(work->deckIndex);
        DrawDeckCpCost(work->deckIndex);
        work->deckAttackCount = CountDeckCardsOfCategory(CARD_CATEGORY_ATTACK, work->deckIndex);
        work->deckMagicCount = CountDeckCardsOfCategory(CARD_CATEGORY_MAGIC, work->deckIndex);
        work->deckItemCount = CountDeckCardsOfCategory(CARD_CATEGORY_ITEM, work->deckIndex);
        work->deckEnemyCount = CountDeckCardsOfCategory(CARD_CATEGORY_ENEMY, work->deckIndex);
        DrawDeckCategoryCount(work->deckAttackCount, CARD_CATEGORY_ATTACK);
        DrawDeckCategoryCount(work->deckMagicCount, CARD_CATEGORY_MAGIC);
        DrawDeckCategoryCount(work->deckItemCount, CARD_CATEGORY_ITEM);
        DrawDeckCategoryCount(work->deckEnemyCount, CARD_CATEGORY_ENEMY);
        DrawCardTotals();
        break;
    case B_BUTTON:
        work->unk_8CA = 0;
        FadeStartIn(FADE_MODE_BLACK, 1);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseRemoveMode);
        m4aSongNumStart(SONG_SYS_CANSEL);
        return 1;
    case R_BUTTON:
        work->unk_8CA = 1;
        FreeCollectionEntries(work);
        FadeStartIn(FADE_MODE_BLACK, 1);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuOpenAddMode);
        m4aSongNumStart(SONG_SYS_CANSEL);
        return 1;
    case START_BUTTON:
        if (!(u8)CheckDeckCpCost(work)) {
            return 1;
        }

        if (!(u8)CheckDeckHasAttackCard(work)) {
            return 1;
        }

        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
        FadeStartOut(FADE_MODE_BLACK, 4);
        m4aSongNumStart(SONG_SYS_CLOSE);
        work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        return 1;
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        work->cursorRow = 0;
        ResetGridScroll(work);
        categoryFilter = work->categoryFilter;
        work->cursorCol = categoryFilter;
        work->timer = 1;
        work->thumbX = 0x4800;
        work->thumbY = 0x2800;
        work->scrollRowEnd = 4;
        m4aSongNumStart(SONG_SYS_CLICKI04B);
        work->view = DECK_MENU_VIEW_REMOVE_FILTER;
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckFilter);
        DrawCpCost(0);
        return 1;
    }

    ApproachValueHalf(&work->handX, gDeckGridColumnX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, gDeckGridRowY[work->cursorRow] << 8);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuCloseRemoveMode(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 4);
    work->removeLabelX = 95;
    ReleaseCardPreview(work);
    SetupBg(3, 0, 30, 0);
#ifdef VERSION_EU
    SetupBg(2, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(0, 0, 31, 0);
#else
    SetupBg(2, 1, 15, 0);
    SetupBg(1, 2, 23, 0);
    SetupBg(0, 3, 31, 0);
#endif
    LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(2, gDefaultBgMap, sizeof(gDefaultBgMap));
    HighlightDeckTab(work, work->deckIndex);
    LoadBgMap(3, gDeckReviewGridMap, sizeof(gDeckReviewGridMap));
    FreeCollectionEntries(work);
    ClearCardGrid(work);
    work->categoryFilter = CATEGORY_FILTER_DECK_ALL;
    work->cursorCol = 0;
    work->cursorRow = work->deckIndex;
    CreateDeckGridCards(work, work->categoryFilter);
    work->mode = DECK_MENU_MODE_NONE;
    work->view = DECK_MENU_VIEW_DECK_SELECT;
    SetDeckMenuHandAnim(work);
    ApproachValueHalf(&work->handX, sDeckTabPointerX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, sDeckTabPointerY[work->cursorRow] << 8);
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckSelect);
    LoadPalette(gDeckMenuTextPalette,
                (void*)(work->palette4->index * 32 +
                        OBJ_PLTT),
                work->palette4->count << 5);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuOpenDeleteMode(DeckMenuWork* work, void* task) {
    u8 z;
    u8* view;

    FadeStartIn(FADE_MODE_BLACK, 4);
    SetupBg(3, 0, 30, 0);
    SetupBg(2, 0, 15, 0);
    SetupBg(1, 0, 23, 0);
    SetupBg(0, 0, 31, 0);
    SetBgScroll(0, 0, 0);
    SetBgScroll(1, 0, 0);
    SetBgScroll(2, 0, 16);
    LoadBgMap(3, gDeckDeleteGridMap, sizeof(gDeckDeleteGridMap));
    LoadBgMap(2, gDeckCollectionInfoMap, sizeof(gDeckCollectionInfoMap));
    LoadBgMap(1, gDeckCardsInUseMap, sizeof(gDeckCardsInUseMap));
    DisableBg(0);
    CountCardsNotInDeckByCategory(DECK_ANY, work->collectionCategoryCounts);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ATTACK], CARD_CATEGORY_ATTACK);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_MAGIC], CARD_CATEGORY_MAGIC);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ITEM], CARD_CATEGORY_ITEM);
    DrawCollectionCategoryCount(work->collectionCategoryCounts[CARD_CATEGORY_ENEMY], CARD_CATEGORY_ENEMY);
    view = &work->view;
    z = 0;
    *view = DECK_MENU_VIEW_DELETE_GRID;
    ReleaseCommandMenuGfx(work);
    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
    ClearCardGrid(work);
    work->step = z;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuBuildDeleteList);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuBuildDeleteList(DeckMenuWork* work, void* task) {
    u16 i;

    FadeStartIn(FADE_MODE_BLACK, 4);

    switch (work->step) {
    case 0:
        work->entryCount = 286;
        work->kindEntries = EwramAlloc(work->entryCount * sizeof(CardKindEntry));
        CpuFill32(0, work->kindEntries, work->entryCount * sizeof(CardKindEntry));
        work->entries = NULL;
        work->descriptionX = 7;
        work->descriptionY = 113;
        break;
    case 1:
        CountCardsNotInDeckByKind(work->kindEntries, work->deckIndex, FALSE, work->entryCount, work->unk_4FC);
        break;
    case 2:
        work->entryCount = ListCardsNotInDeckByKind(work->kindEntries, work->deckIndex, FALSE, work->entryCount, work->unk_4FC);
        break;
    case 3:
        if (work->entryCount != 0) {
            BuildCollectionEntries(work);
        }

        break;
    case 4:
        for (i = 0; i < 286; i++) {
            if (work->kindEntries[i].count != 0) {
                EwramFree(work->kindEntries[i].indices);
            }
        }

        EwramFree(work->kindEntries);
        break;
    case 5:
        work->categoryFilter = CATEGORY_FILTER_COLLECTION_ALL;
        work->gridEntryCount = CreateCollectionGridCards(work, 5, TRUE);
        SetDeckMenuHandAnim(work);
        work->handX = gCollectionGridColumnX[0] << 8;
        work->handY = gCollectionGridRowY[0] << 8;
        work->mode = DECK_MENU_MODE_DELETE;
        work->cursorCol = 0;
        work->cursorRow = 0;
        ShowCollectionCardPreview(work);

        if (work->gridEntryCount != 0) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteGrid);
        } else {
            work->view = DECK_MENU_VIEW_DELETE_FILTER;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
            work->cursorCol = work->categoryFilter;
            work->timer = 1;
        }

        break;
    }

    work->step++;
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuDeleteGrid(DeckMenuWork* work, void* task) {
    u16 i;

    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (work->popupActive != 0) {
        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
            work->exitRequested = TRUE;
        }

        return 1;
    }

    if (work->exitRequested) {
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        } else {
            work->exitRequested = FALSE;
        }
    }

    switch (GetKeysRepeat()) {
    case DPAD_UP:
        if (work->cursorRow > 0) {
            if (IsCardAt(work, work->cursorCol, work->cursorRow - 1)) {
                work->cursorRow--;
                work->timer = 1;
                m4aSongNumStart(SONG_SYS_CLICKI04B);
            }
        } else {
            if (!ScrollGridUp(work, TRUE)) {
                work->cursorCol = work->categoryFilter;
                work->timer = 1;
                m4aSongNumStart(SONG_SYS_CLICKI04B);
                work->view = DECK_MENU_VIEW_DELETE_FILTER;

                for (i = 0; i < 10; i++) {
                    DrawValueCount(0, i);
                }

                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
                return 1;
            }
        }

        ShowCollectionCardPreview(work);
        break;
    case DPAD_DOWN:
        if (work->cursorRow <= 2) {
            if (IsCardAt(work, work->cursorCol, work->cursorRow + 1)) {
                work->cursorRow++;
                work->timer = 1;
                m4aSongNumStart(SONG_SYS_CLICKI04B);
            }
        } else {
            if (IsCardAt(work, work->cursorCol, work->cursorRow + 1)) {
                ScrollGridDown(work);
            }
        }

        ShowCollectionCardPreview(work);
        break;
    case DPAD_LEFT:
        if (work->cursorCol > 0) {
            if (IsCardAt(work, work->cursorCol - 1, work->cursorRow)) {
                work->cursorCol--;
                work->timer = 1;
                m4aSongNumStart(SONG_SYS_CLICKI04B);
            }
        }

        ShowCollectionCardPreview(work);
        break;
    case DPAD_RIGHT:
        if (work->cursorCol > 1) {
            work->timer = 1;
            return 1;
        }

        if (IsCardAt(work, work->cursorCol + 1, work->cursorRow)) {
            work->cursorCol++;
            work->timer = 1;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        ShowCollectionCardPreview(work);
        break;
    }

    switch (GetKeysPressed()) {
    case A_BUTTON:
        if ((u8)IsCardAtCursor(work)) {
            work->savedCol = work->cursorCol;
            work->savedRow = work->cursorRow;
            work->cursorCol = 0;
            work->cursorRow = 0;
            SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_ROW);

            if ((u8)MoveValueCursor(work, 0)) {
                m4aSongNumStart(SONG_SYS_KETTEI);
                DrawSelectedValueCpCost(work);
                work->view = DECK_MENU_VIEW_DELETE_VALUE_SELECT;
                work->handX = sValueGridX[work->cursorCol] << 8;
                work->handY = (sValueGridY[work->cursorRow] - 16) << 8;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeleteValueSelect);
                return 1;
            } else {
                work->cursorCol = work->savedCol;
                work->cursorRow = work->savedRow;
                SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
                work->view = DECK_MENU_VIEW_DELETE_GRID;
                m4aSongNumStart(SONG_SYS_BEEP);
                return 1;
            }
        } else {
            m4aSongNumStart(SONG_SYS_BEEP);
            return 1;
        }
    case B_BUTTON:
        FadeStartIn(FADE_MODE_BLACK, 1);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseDeleteMode);
        m4aSongNumStart(SONG_SYS_CANSEL);
        return 1;
    case START_BUTTON:
        if ((u8)CheckDeckCpCost(work) && (u8)CheckDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        }

        return 1;
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        ResetGridScroll(work);
        work->cursorCol = work->categoryFilter;
        work->timer = 1;
        work->thumbX = 0xA000;
        work->thumbY = 0x2800;
        work->scrollRowEnd = 4;
        m4aSongNumStart(SONG_SYS_CLICKI04B);
        work->view = DECK_MENU_VIEW_DELETE_FILTER;

        for (i = 0; i < 10; i++) {
            DrawValueCount(0, i);
        }

        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCollectionFilter);
        return 1;
    }

    ApproachValueHalf(&work->handX, gCollectionGridColumnX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, gCollectionGridRowY[work->cursorRow] << 8);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuCloseDeleteMode(DeckMenuWork* work, void* task) {
    u8* view;

    FadeStartIn(FADE_MODE_BLACK, 4);
    view = &work->view;
    *view = DECK_MENU_VIEW_DELETE_GRID;
    work->removeLabelX = 95;
    ReleaseCardPreview(work);
    SetupBg(3, 0, 30, 0);
#ifdef VERSION_EU
    SetupBg(2, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(0, 0, 31, 0);
#else
    SetupBg(2, 1, 15, 0);
    SetupBg(1, 2, 23, 0);
    SetupBg(0, 3, 31, 0);
#endif
    LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(2, gDefaultBgMap, sizeof(gDefaultBgMap));
    HighlightDeckTab(work, work->deckIndex);
    LoadBgMap(3, gDeckReviewGridMap, sizeof(gDeckReviewGridMap));
    FreeCollectionEntries(work);
    ClearCardGrid(work);
    work->categoryFilter = CATEGORY_FILTER_DECK_ALL;
    work->cursorCol = 0;
    work->cursorRow = work->deckIndex;
    CreateDeckGridCards(work, work->categoryFilter);
    work->mode = DECK_MENU_MODE_NONE;
    *view = DECK_MENU_VIEW_DECK_SELECT;
    SetDeckMenuHandAnim(work);
    ApproachValueHalf(&work->handX, sDeckTabPointerX[work->cursorCol] << 8);
    ApproachValueHalf(&work->handY, sDeckTabPointerY[work->cursorRow] << 8);
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuDeckSelect);
    LoadPalette(gDeckMenuTextPalette,
                (void*)(work->palette4->index * 32 +
                        OBJ_PLTT),
                work->palette4->count << 5);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuFadeOut(DeckMenuWork* work) {
    if (!FadeIsActive()) {
        return 0;
    }

    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuStartSlideOut(DeckMenuWork* work, void* task) {
    work->tiles6 = LoadObjTiles(gDeckMenuBarTiles, sizeof(gDeckMenuBarTiles));

#ifdef VERSION_EU
    work->tiles12 = LoadObjTiles(gDeckTitleBannerTilesByLanguage[gLanguage], sDeckTitleBannerTileSizes[gLanguage]);
#elif defined(VERSION_US)
    if (gGameState.flags & GAME_FLAG_RIKU) {
        work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTiles, sizeof(gRikuDeckTitleBannerTiles));
    } else {
        work->tiles12 = LoadObjTiles(gDeckTitleBannerTiles, sizeof(gDeckTitleBannerTiles));
    }
#else
    work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTiles, sizeof(gRikuDeckTitleBannerTiles));
#endif

    work->palette3 = LoadObjPalette(gDeckTitleBannerPalette, sizeof(gDeckTitleBannerPalette));
    LoadBgMap(3, gDeckMenuMap, sizeof(gDeckMenuMap));
    work->topBarX = 0x7800;
    work->topBarY = 0;
    work->bottomBarX = 0xA400;
    work->bottomBarY = 0x9800;
    work->bannerX = 0;
    work->barSlideTimer = 16;
    work->bannerSlideTimer = 16;
    work->handVisible = 0;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuSlideOut);
    return 1;
}

u8 UpdateDeckMenuSlideOut(DeckMenuWork* work, void* task) {
    if ((s8)work->bannerSlideTimer > 0) {
        ApproachValue(&work->bannerX, -0x8000, (s8)work->bannerSlideTimer);
        work->bannerSlideTimer--;
    } else if ((s8)work->barSlideTimer > 0) {
        ApproachValue(&work->topBarY, -0x800, (s8)work->barSlideTimer);
        ApproachValue(&work->bottomBarY, 0xA000, (s8)work->barSlideTimer);
        work->barSlideTimer--;
    } else {
        FadeStartOut(FADE_MODE_BLACK, 4);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuFadeOut);
    }

    return 1;
}

void DrawCardDescription(DeckMenuWork* work) {
    DrawTextSlotsUnsorted(work->descriptionX, work->descriptionY, work->textSlots5,
                  work->palette, 20, work->textSlotCount5);
}

void DrawDeckNames(DeckMenuWork* work, u8 flag) {
    if (!flag) {
        switch (work->deckIndex) {
        case 0:
            DrawTextSlots(work->deckNameX, work->deckNameY, work->textSlots, work->palette, 20, work->textSlotCount);
            DrawTextSlots(work->deckName2X, work->deckName2Y, work->textSlots2, work->palette4, 20, work->textSlotCount2);
            DrawTextSlots(work->deckName3X, work->deckName3Y, work->textSlots3, work->palette4, 20, work->textSlotCount3);
            break;
        case 1:
            DrawTextSlots(work->deckNameX, work->deckNameY, work->textSlots, work->palette4, 20, work->textSlotCount);
            DrawTextSlots(work->deckName2X, work->deckName2Y, work->textSlots2, work->palette, 20, work->textSlotCount2);
            DrawTextSlots(work->deckName3X, work->deckName3Y, work->textSlots3, work->palette4, 20, work->textSlotCount3);
            break;
        case 2:
            DrawTextSlots(work->deckNameX, work->deckNameY, work->textSlots, work->palette4, 20, work->textSlotCount);
            DrawTextSlots(work->deckName2X, work->deckName2Y, work->textSlots2, work->palette4, 20, work->textSlotCount2);
            DrawTextSlots(work->deckName3X, work->deckName3Y, work->textSlots3, work->palette, 20, work->textSlotCount3);
            break;
        }
    } else {
        switch (work->deckIndex) {
        case 0:
            DrawTextSlots(work->deckNameX, work->deckNameY, work->textSlots, work->palette, 20, work->textSlotCount);
            break;
        case 1:
            DrawTextSlots(work->deckName2X, work->deckName2Y, work->textSlots2, work->palette, 20, work->textSlotCount2);
            break;
        case 2:
            DrawTextSlots(work->deckName3X, work->deckName3Y, work->textSlots3, work->palette, 20, work->textSlotCount3);
            break;
        }
    }
}

static void Deckmenu2_2(DeckMenuWork* work) {
#ifdef PLATFORM_PC
    /* The collection panel moves to the right in Add mode. Keep the
       extended gutter attached to that panel, not to the caption box. */
    PortMenuPanelSide(work->view == DECK_MENU_VIEW_ADD_GRID ||
                      work->view == DECK_MENU_VIEW_ADD_FILTER ||
                      work->view == DECK_MENU_VIEW_ADD_VALUE_SELECT ? 1 : -1);
#endif
    gCardUiSpriteState.gfx = AnimUpdate(&gCardUiSpriteState.anim);

    if (work->popupActive == 0) {
        if (work->handVisible != 0) {
            DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 30, work->gfx, work->tiles, work->palette, NULL, work->handFlags, 3);
        }
    }

    if (work->view != DECK_MENU_VIEW_KEYBOARD) {
        DrawSprite(work->thumbX >> 8, work->thumbY >> 8, gDeckScrollThumbFrames[0], work->tiles4, work->palette, NULL, SPRITE_PRIORITY(2), 10);
    }

    if (work->tiles6 != NULL) {
        DrawSprite(work->topBarX >> 8, work->topBarY >> 8, gDeckMenuBarFrames[0], work->tiles6, work->palette3, NULL, SPRITE_PRIORITY(3), 10000);
        DrawSprite(work->bottomBarX >> 8, work->bottomBarY >> 8, gDeckMenuBarFrames[1], work->tiles6, work->palette3, NULL, SPRITE_PRIORITY(3), 10000);
    }

    if (work->tiles12 != NULL) {
        DrawMenuHeader(work->bannerX >> 8, 0,
#ifdef VERSION_EU
                   gDeckTitleBannerSpritesByLanguage[gLanguage][0],
#elif defined(VERSION_US)
                   gDeckTitleBannerFrames[0],
#else
                   gRikuDeckTitleBannerFrames[0],
#endif
                   work->tiles12, work->palette3, NULL, 0, 10);
    }
#ifdef PLATFORM_PC
    /* the title's HD banner over the BG copy the menu keeps once it has slid in (its tiles released): while BG3's
       top rows are the review grid's, which carry the title */
    else if (work->view != DECK_MENU_VIEW_KEYBOARD && memcmp(GetBgScreenBase(3), gDeckReviewGridMap, 128) == 0) {
        int level = (int)(gFadeWork->amount >> 8) * 255 / 31;
        PortHdStillSprite(
#ifdef VERSION_EU
            gDeckTitleBannerTilesByLanguage[gLanguage], gDeckTitleBannerSpritesByLanguage[gLanguage][0],
#elif defined(VERSION_US)
            (gGameState.flags & GAME_FLAG_RIKU) ? (void*)gRikuDeckTitleBannerTiles : (void*)gDeckTitleBannerTiles,
            gDeckTitleBannerFrames[0],
#else
            gRikuDeckTitleBannerTiles, gRikuDeckTitleBannerFrames[0],
#endif
            work->bannerX >> 8, 0, gFadeWork->mode == FADE_MODE_BLACK ? level : 0,
            gFadeWork->mode == FADE_MODE_WHITE || gFadeWork->mode == FADE_MODE_ADD_WHITE ? level : 0);
    }
#if defined(VERSION_EU) || defined(VERSION_US)
    /* the Remove / Add cards titles live in BG3 itself (the grid maps' top rows, the language's text tiles): their
       HD banners over them (hd_menu_titles.py BG_BANNERS) */
    else if (work->view != DECK_MENU_VIEW_KEYBOARD) {
#ifdef VERSION_EU
        static const void* const textTiles[5] = { gDeckMenuTextTiles, gDeckMenuTextFrenchTiles, gDeckMenuTextGermanTiles,
                                                  gDeckMenuTextItalianTiles, gDeckMenuTextSpanishTiles };
        const void* tiles = gLanguage < 5 ? textTiles[gLanguage] : NULL;
#else
        const void* tiles = gDeckMenuTiles; /* the American menu's tiles carry its text */
#endif
        const void* map = memcmp(GetBgScreenBase(3), gDeckRemoveGridMap, 128) == 0 ? (const void*)gDeckRemoveGridMap
                          : memcmp(GetBgScreenBase(3), gDeckAddGridMap, 128) == 0 ? (const void*)gDeckAddGridMap
                                                                                   : NULL;
        int level = (int)(gFadeWork->amount >> 8) * 255 / 31;
        if (map != NULL && tiles != NULL)
            PortHdStillSprite(tiles, map, 0, 0, gFadeWork->mode == FADE_MODE_BLACK ? level : 0,
                              gFadeWork->mode == FADE_MODE_WHITE || gFadeWork->mode == FADE_MODE_ADD_WHITE ? level : 0);
    }
#endif
#endif

    switch (work->view) {
    case DECK_MENU_VIEW_DECK_GRID:
        if (work->holding) {
            DrawSprite((work->heldX >> 8) - 16, (work->heldY >> 8) - 20, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);
        }

        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 20, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);
        DrawDeckNames(work, FALSE);
        DrawSprite(work->removeLabelX, work->removeLabelY, work->gfx7, work->tiles5, work->palette, NULL, 0, 10);
        DrawSprite(work->addLabelX, work->addLabelY, work->gfx8, work->tiles5, work->palette, NULL, 0, 10);
        break;
    case DECK_MENU_VIEW_COMMANDS:
#ifdef VERSION_EU
        if (work->tiles3 != NULL) {
            DrawSprite(120, 80, gDeckCommandMenuSpritesByLanguage[gLanguage][0], work->tiles3, work->palette2, NULL, 0, 8);
        }
#else
        DrawSprite(120, 80, gDeckCommandMenuFrames[0], work->tiles3, work->palette2, NULL, 0, 8);
#endif
        DrawDeckNames(work, FALSE);
        DrawSprite(work->removeLabelX, work->removeLabelY, work->gfx7, work->tiles5, work->palette, NULL, 0, 10);
        DrawSprite(work->addLabelX, work->addLabelY, work->gfx8, work->tiles5, work->palette, NULL, 0, 10);
        break;
    case DECK_MENU_VIEW_DECK_FILTER:
        DrawDeckNames(work, FALSE);
        DrawSprite(work->removeLabelX, work->removeLabelY, work->gfx7, work->tiles5, work->palette, NULL, 0, 10);
        DrawSprite(work->addLabelX, work->addLabelY, work->gfx8, work->tiles5, work->palette, NULL, 0, 10);
        break;
    case DECK_MENU_VIEW_DECK_SELECT:
        DrawSprite(work->removeLabelX, work->removeLabelY, work->gfx7, work->tiles5, work->palette, NULL, 0, 10);
        DrawSprite(work->addLabelX, work->addLabelY, work->gfx8, work->tiles5, work->palette, NULL, 0, 10);
        DrawDeckNames(work, FALSE);
        break;
    case DECK_MENU_VIEW_ADD_GRID:
        DrawDeckNames(work, TRUE);
        DrawSprite(work->removeLabelX, work->removeLabelY, work->gfx7, work->tiles5, work->palette, NULL, 0, 10);
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 20, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);

        if (work->tiles7 != NULL) {
            if (work->popupActive == 0) {
                DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 20, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);
            }

            DrawSprite(24, 82, work->gfx4, work->tiles7, work->palette5, NULL, SPRITE_PRIORITY(1), 100);
            DrawSprite(24, 82, work->gfx5, work->tiles8, work->palette6, NULL, SPRITE_PRIORITY(1), 101);
            DrawTextSlots(10, 116, work->textSlots4, work->palette4, 20, work->textSlotCount4);

            if (work->tiles10 != NULL) {
                work->gfx3 = AnimUpdate(&work->anim);
                DrawSprite(24, 82, work->gfx3, work->tiles10, work->palette5, NULL, 0, 1);
            }
        }

        if (work->popupActive == 0) {
            DrawCardDescription(work);
        }

        break;
    case DECK_MENU_VIEW_REMOVE_GRID:
        DrawDeckNames(work, TRUE);
        DrawSprite(work->addLabelX, work->addLabelY, work->gfx8, work->tiles5, work->palette, NULL, 0, 10);
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 20, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);

        if (work->tiles7 != NULL) {
            DrawSprite(164, 82, work->gfx4, work->tiles7, work->palette5, NULL, SPRITE_PRIORITY(1), 100);
            DrawSprite(164, 82, work->gfx5, work->tiles8, work->palette6, NULL, SPRITE_PRIORITY(1), 101);

            if (work->tiles10 != NULL) {
                work->gfx3 = AnimUpdate(&work->anim);
                DrawSprite(164, 82, work->gfx3, work->tiles10, work->palette5, NULL, 0, 1);
            }

            if (work->tiles9 != NULL) {
                DrawSprite(164, 82, work->gfx6, work->tiles9, work->palette5, NULL, 0, 19);
            }

            DrawTextSlots(100, 116, work->textSlots4, work->palette4, 20, work->textSlotCount4);

            if (work->popupActive == 0) {
                DrawCardDescription(work);
            }
        }

        break;
    case DECK_MENU_VIEW_ADD_VALUE_SELECT:
        DrawSprite(work->removeLabelX, work->removeLabelY, work->gfx7, work->tiles5, work->palette, NULL, 0, 10);
        DrawSprite((work->handX >> 8) - 26, (work->handY >> 8) - 13, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);
        DrawDeckNames(work, TRUE);

        if (work->tiles7 != NULL) {
            DrawSprite(24, 82, work->gfx4, work->tiles7, work->palette5, NULL, SPRITE_PRIORITY(1), 100);
            DrawSprite(24, 82, work->gfx5, work->tiles8, work->palette6, NULL, SPRITE_PRIORITY(1), 101);

            if (work->tiles10 != NULL) {
                work->gfx3 = AnimUpdate(&work->anim);
                DrawSprite(24, 82, work->gfx3, work->tiles10, work->palette5, NULL, 0, 1);
            }

            DrawTextSlots(10, 116, work->textSlots4, work->palette4, 20, work->textSlotCount4);
        }

        if (work->popupActive == 0) {
            DrawCardDescription(work);
        }

        break;
    case DECK_MENU_VIEW_ADD_FILTER:
        DrawSprite(work->removeLabelX, work->removeLabelY, work->gfx7, work->tiles5, work->palette, NULL, 0, 10);
        DrawDeckNames(work, TRUE);
        break;
    case DECK_MENU_VIEW_REMOVE_FILTER:
        DrawSprite(work->addLabelX, work->addLabelY, work->gfx8, work->tiles5, work->palette, NULL, 0, 10);
        DrawDeckNames(work, TRUE);
        break;
    case DECK_MENU_VIEW_DELETE_GRID:
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 20, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);

        if (work->tiles7 != NULL) {
            DrawSprite(24, 66, work->gfx4, work->tiles7, work->palette5, NULL, SPRITE_PRIORITY(1), 100);
            DrawSprite(24, 66, work->gfx5, work->tiles8, work->palette6, NULL, SPRITE_PRIORITY(1), 101);

            if (work->tiles10 != NULL) {
                work->gfx3 = AnimUpdate(&work->anim);
                DrawSprite(24, 66, work->gfx3, work->tiles10, work->palette5, NULL, 0, 1);
            }

            DrawTextSlots(10, 100, work->textSlots4, work->palette4, 20, work->textSlotCount4);
        }

        if (work->popupActive == 0) {
            DrawCardDescription(work);
        }

        break;
    case DECK_MENU_VIEW_DELETE_VALUE_SELECT:
        DrawSprite((work->handX >> 8) - 26, (work->handY >> 8) - 13, work->gfx2, work->tiles2, work->palette4, NULL, 0, 8);

        if (work->tiles7 != NULL) {
            DrawSprite(24, 66, work->gfx4, work->tiles7, work->palette5, NULL, SPRITE_PRIORITY(1), 100);
            DrawSprite(24, 66, work->gfx5, work->tiles8, work->palette6, NULL, SPRITE_PRIORITY(1), 101);

            if (work->tiles10 != NULL) {
                work->gfx3 = AnimUpdate(&work->anim);
                DrawSprite(24, 66, work->gfx3, work->tiles10, work->palette5, NULL, 0, 1);
            }

            DrawTextSlots(10, 100, work->textSlots4, work->palette4, 20, work->textSlotCount4);
        }

        if (work->popupActive == 0) {
            DrawCardDescription(work);
        }

        break;
    case DECK_MENU_VIEW_KEYBOARD:
        DrawSprite(work->keyCursorX >> 8, work->keyCursorY >> 8, work->gfx9, work->tiles11, work->palette7, NULL, 0, 20);
        DrawSprite(work->caretX >> 8, 18, NULL, work->tiles13, work->palette7, NULL, 0, 21);
        DrawTextSlots(138, 16, work->textSlots6, work->palette, 20, work->textSlotCount6);
        break;
    case DECK_MENU_VIEW_DELETE_PROMPT:
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 30, work->gfx, work->tiles, work->palette, NULL, work->handFlags, 0);

        if (work->tiles7 != NULL) {
            DrawSprite(24, 66, work->gfx4, work->tiles7, work->palette5, NULL, SPRITE_PRIORITY(1), 100);
            DrawSprite(24, 66, work->gfx5, work->tiles8, work->palette6, NULL, SPRITE_PRIORITY(1), 101);

            if (work->tiles10 != NULL) {
                work->gfx3 = AnimUpdate(&work->anim);
                DrawSprite(24, 66, work->gfx3, work->tiles10, work->palette5, NULL, 0, 1);
            }

            DrawTextSlots(10, 100, work->textSlots4, work->palette4, 20, work->textSlotCount4);
        }

        if (work->popupActive == 0) {
            DrawCardDescription(work);
        }

        break;
    case DECK_MENU_VIEW_CLEAR_PROMPT:
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 30, work->gfx, work->tiles, work->palette, NULL, work->handFlags, 0);

        if (work->tiles7 != NULL) {
            DrawSprite(24, 66, work->gfx4, work->tiles7, work->palette5, NULL, SPRITE_PRIORITY(1), 100);
            DrawSprite(24, 66, work->gfx5, work->tiles8, work->palette6, NULL, SPRITE_PRIORITY(1), 101);

            if (work->tiles10 != NULL) {
                work->gfx3 = AnimUpdate(&work->anim);
                DrawSprite(24, 66, work->gfx3, work->tiles10, work->palette5, NULL, 0, 1);
            }
        }

        break;
    }

    TaskPoolDraw(&work->taskpool);
    TaskPoolDraw(&work->cardpool);
}

void DeckMenuDestroy(DeckMenuWork* work) {
#ifdef PLATFORM_PC
    PortClearBgTextTiles();
#endif
    ClearCardGrid(work);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette);

    if (work->tiles12 != NULL) {
        ReleaseObjTiles(work->tiles12);
    }

    if (work->tiles6 != NULL) {
        ReleaseObjTiles(work->tiles6);
    }

    if (work->palette3 != NULL) {
        ReleaseObjPalette(work->palette3);
    }

    ReleaseCommandMenuGfx(work);
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    FreeTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
    FreeTextSlots(work->textSlots5, ARRAY_COUNT(work->textSlots5));
    ReleaseObjPalette(work->palette4);
    TaskPoolDestroy(&work->taskpool);
    TaskPoolDestroy(&work->cardpool);
    FreeCollectionEntries(work);
    *work->resultOut = work->result;
    ReleaseObjTiles(work->tiles5);
    ReleaseObjTiles(gCardUiSpriteState.tiles);
    ReleaseObjPalette(gCardUiSpriteState.palette);
}

void CreateDeckGridCards(DeckMenuWork* work, u8 categoryFilter) {
    DeckCard2Args args;
    u16* deck;
    u8 i;
    s8 x;
    s8 y;

    deck = (u16*)GetDeck(work->deckIndex);
    x = 0;
    y = 0;

    if (categoryFilter == CATEGORY_FILTER_DECK_ALL) {
        for (i = 0; i < 99; i++) {
            if (deck[i] != CARD_NONE) {
                if (categoryFilter == CATEGORY_FILTER_DECK_ALL) {
                    args.pool = &work->pool;
                    args.cardId = gCardCollection[deck[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                    args.col = x;
                    args.row = y;
                    args.panel = DECK_CARD2_PANEL_DECK;
                    args.slot = &deck[i];
                    TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                } else if (gCardDefs[gCardCollection[deck[i]] & CARD_ID_MASK].category == categoryFilter - 1) {
                    args.pool = &work->pool;
                    args.cardId = gCardCollection[deck[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                    args.col = x;
                    args.row = y;
                    args.panel = DECK_CARD2_PANEL_DECK;
                    args.slot = &deck[i];
                    TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                }
            } else {
                args.pool = &work->pool;
                args.cardId = CARD_ID_NONE;
                args.col = x;
                args.row = y;
                args.panel = DECK_CARD2_PANEL_DECK;
                args.slot = &deck[i];
                TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
            }

            x++;

            if (x > 2) {
                x = 0;
                y++;
            }
        }
    } else {
        for (i = 0; i < 99; i++) {
            if (deck[i] != CARD_NONE && gCardDefs[gCardCollection[deck[i]] & CARD_ID_MASK].category == categoryFilter - 1) {
                args.pool = &work->pool;
                args.cardId = gCardCollection[deck[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                args.col = x;
                args.row = y;
                args.panel = DECK_CARD2_PANEL_DECK;
                args.slot = &deck[i];
                TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                x++;
            }

            if (x > 2) {
                x = 0;
                y++;
            }
        }
    }

    work->thumbX = 0x4800;
    work->thumbY = 0x2800;
    work->scrollRowEnd = 4;
    SetGridRowCount(work, 99);
}

s32 CreateCollectionGridCards(DeckMenuWork* work, u8 categoryFilter, u8 excludeBossCards) {
    DeckCard2Args args;
    u16 i;
    u16 count;
    s8 x;
    s8 y;

    x = 0;
    y = 0;
    count = 0;

    if (!excludeBossCards) {
        for (i = 0; i < work->entryCount; i++) {
            if (categoryFilter == CATEGORY_FILTER_COLLECTION_ALL) {
                if (work->entries[i].count != 0) {
                    args.pool = &work->pool;
                    args.cardId = GetCardIdForKindEntry(
                        work->entries[i].kind);
                    args.col = x;
                    args.row = y;
                    args.panel = DECK_CARD2_PANEL_COLLECTION;
                    args.slot = NULL;
                    TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                    x++;
                    count++;
                }
            } else if (work->entries[i].count != 0) {
                args.pool = &work->pool;
                args.cardId = GetCardIdForKindEntry(
                    work->entries[i].kind);

                if (gCardDefs[args.cardId & CARD_ID_MASK].category == categoryFilter - 1) {
                    args.col = x;
                    args.row = y;
                    args.panel = DECK_CARD2_PANEL_COLLECTION;
                    args.slot = NULL;
                    TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                    x++;
                    count++;
                }
            }

            if (x > 2) {
                x = 0;
                y++;
            }
        }
    } else {
        for (i = 0; i < work->entryCount; i++) {
            if (categoryFilter == CATEGORY_FILTER_COLLECTION_ALL) {
                if (work->entries[i].count != 0) {
                    if ((u16)(work->entries[i].kind - 78) >
                        64) {
                        args.pool = &work->pool;
                        args.cardId = GetCardIdForKindEntry(
                            work->entries[i].kind);
                        args.col = x;
                        args.row = y;
                        args.panel = DECK_CARD2_PANEL_COLLECTION;
                        args.slot = NULL;
                        TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                        x++;
                        count++;
                    }
                }
            } else if (work->entries[i].count != 0) {
                args.pool = &work->pool;
                args.cardId = GetCardIdForKindEntry(
                    work->entries[i].kind);

                if (gCardDefs[args.cardId & CARD_ID_MASK].category == categoryFilter - 1) {
                    if ((u16)(work->entries[i].kind - 78) >
                        64) {
                        args.col = x;
                        args.row = y;
                        args.panel = DECK_CARD2_PANEL_COLLECTION;
                        args.slot = NULL;
                        TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                        x++;
                        count++;
                    }
                }
            }

            if (x > 2) {
                x = 0;
                y++;
            }
        }
    }

    work->thumbX = 0xA000;
    work->thumbY = 0x2800;
    work->scrollRowEnd = 4;
    SetGridRowCount(work, y * 3 + x);

    return count;
}

s32 GetCardIdForKindEntry(s32 kind) {
    u32 i;

    for (i = 0; i < 950; i++) {
        if (gCardDefs[i].kind == kind) {
            return i;
        }

        if (gCardDefs[i].kind + 143 == kind) {
            return i | CARD_FLAG_PREMIUM;
        }
    }
}

void ClearCardGrid(DeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        node->done = TRUE;
        node = ListPoolNext(&node->node);
    }

    TaskPoolUpdate(&work->taskpool);
}

void SetGridRowCount(DeckMenuWork* work, s16 cardCount) {
    work->rowCount = cardCount / 3;

    if (cardCount % 3 != 0) {
        work->rowCount = cardCount / 3 + 1;
    }
}

void UpdateGridScrollBar(DeckMenuWork* work) {
    s32 rowStep;

    // @bug rowCount is 4 when 10-12 entries are left (division by zero).
    rowStep = DIV0(0x5400, work->rowCount - 4);
    work->thumbY = rowStep * (work->scrollRowEnd - 4) + 0x2800;

    if (work->thumbY > 0x7C00) {
        work->thumbY = 0x7C00;
    }

    if (work->thumbY <= 0x27FF) {
        work->thumbY = 0x2800;
    }
}

void ScrollGridDown(DeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    if (work->scrollRowEnd != work->rowCount) {
        while (node != NULL) {
            node->args.row--;

            if (node->args.row < 0) {
                node->y = 0x20000;
                DeckCard2ReleaseGfx(node);
            }

            node = ListPoolNext(&node->node);
        }

        m4aSongNumStart(SONG_SYS_CLICKI04B);
        work->scrollRowEnd++;
        work->thumbY += 0x300;

        if (work->thumbY > 0x7C00) {
            work->thumbY = 0x7C00;
        }

        if (work->holding) {
            work->heldRow--;
        }

        UpdateGridScrollBar(work);
    }
}

u8 ScrollGridUp(DeckMenuWork* work, u8 playSound) {
    DeckCard2Work* node;
    u8 withSound;
    u16 scrollRowEnd;

    withSound = playSound;
    node = ListPoolFirst(&work->pool);

    if (work->scrollRowEnd <= 4) {
        return FALSE;
    }

    if (node == NULL) {
        work->thumbY -= 0x300;

        scrollRowEnd = work->scrollRowEnd;

        if ((s16)scrollRowEnd > 4) {
            work->scrollRowEnd = scrollRowEnd - 1;
        }

        if (work->thumbY < 0x2800) {
            work->thumbY = 0x2800;
            return FALSE;
        }

        if (playSound) {
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }
    } else {
        if (withSound) {
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        do {
            node->args.row++;

            if (node->args.row > 3) {
                node->y = 0x20000;
                DeckCard2ReleaseGfx(node);
            }

            node = ListPoolNext(&node->node);
        } while (node != NULL);

        work->scrollRowEnd--;
        work->thumbY -= 0x300;

        if (work->thumbY < 0x2800) {
            work->thumbY = 0x2800;
        }
    }

    UpdateGridScrollBar(work);
    return TRUE;
}

DeckCard2Work* GetCardAtCursor(DeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (work->cursorCol == node->args.col &&
            work->cursorRow == node->args.row) {
            return node;
        }

        node = ListPoolNext(&node->node);
    }

    return NULL;
}

void DrawDeckCategoryCount(u8 count, u8 category) {
    u8 digits[2];
    u8* base;

    if (count == 0) {
        base = GetBgCharBase(3);
        PortCopyDeckCategoryDigit(gDeckCategoryZeroTiles, base + (category * 64 + 0x360), sizeof(gDeckCategoryZeroTiles));
        PortCopyDeckCategoryDigit(gDeckCategoryZeroTiles, base + (category * 64 + 0x360) + 32, sizeof(gDeckCategoryZeroTiles));
    } else {
        digits[0] = count / 10;
        digits[1] = count - (u8)(count / 10) * 10;
        base = GetBgCharBase(3);
        PortCopyDeckCategoryDigit(&gDeckCategoryDigitTiles[(digits[0] + 1) * 32], base + (category * 64 + 0x360), 32);
        PortCopyDeckCategoryDigit(&gDeckCategoryDigitTiles[(digits[1] + 1) * 32], base + (category * 64 + 0x360) + 32, 32);
    }
}

void DrawCollectionCategoryCount(u16 count, u8 category) {
    u8 digits[3];
    u8* base;

    if (count == 0) {
        base = GetBgCharBase(3);
        PortCopyDeckCategoryDigit(gDeckCategoryZeroTiles, base + (category * 96 + 0x120), sizeof(gDeckCategoryZeroTiles));
        PortCopyDeckCategoryDigit(gDeckCategoryZeroTiles, base + (category * 96 + 0x120) + 32, sizeof(gDeckCategoryZeroTiles));
        PortCopyDeckCategoryDigit(gDeckCategoryZeroTiles, base + (category * 96 + 0x120) + 64, sizeof(gDeckCategoryZeroTiles));
    } else {
        digits[0] = count / 100;
        digits[1] = count / 10 - digits[0] * 10;
        digits[2] = count - digits[0] * 100 - digits[1] * 10;
        base = GetBgCharBase(3);
        PortCopyDeckCategoryDigit(&gDeckCategoryDigitTiles[(digits[0] + 1) * 32], base + (category * 96 + 0x120), 32);
        PortCopyDeckCategoryDigit(&gDeckCategoryDigitTiles[(digits[1] + 1) * 32], base + (category * 96 + 0x120) + 32, 32);
        PortCopyDeckCategoryDigit(&gDeckCategoryDigitTiles[(digits[2] + 1) * 32], base + (category * 96 + 0x120) + 64, 32);
    }
}

void SetDeckMenuHandAnim(DeckMenuWork* work) {
    u16 handFlags;

    switch (work->view) {
    case DECK_MENU_VIEW_DECK_GRID:
    case DECK_MENU_VIEW_DECK_FILTER:
    case DECK_MENU_VIEW_ADD_GRID:
    case DECK_MENU_VIEW_ADD_VALUE_SELECT:
    case DECK_MENU_VIEW_ADD_FILTER:
    case DECK_MENU_VIEW_REMOVE_GRID:
    case DECK_MENU_VIEW_REMOVE_FILTER:
    case DECK_MENU_VIEW_DELETE_GRID:
    case DECK_MENU_VIEW_DELETE_FILTER:
    case DECK_MENU_VIEW_DELETE_VALUE_SELECT:
    case DECK_MENU_VIEW_KEYBOARD:
        AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
        work->handFlags &= ~SPRITE_FLAG_HFLIP;
        break;
    case DECK_MENU_VIEW_DECK_SELECT:
    case DECK_MENU_VIEW_COMMANDS:
    case DECK_MENU_VIEW_DELETE_PROMPT:
        AnimStart(&work->anim2, 2, ANIM_FLAG_LOOP);
        handFlags = work->handFlags | SPRITE_FLAG_HFLIP;
        work->handFlags = handFlags;
        break;
    }
}

void HighlightDeckTab(DeckMenuWork* work, u8 deckIndex) {
    void* dst;

    switch (deckIndex) {
    case 0:
        dst = (void*)(BG_PLTT + 8 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckTabHighlightPalette, dst, sizeof(gDeckTabHighlightPalette));
        dst = (void*)(BG_PLTT + 9 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckMenuPalettes[9], dst, sizeof(gDeckMenuPalettes[0]));
        dst = (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckMenuPalettes[10], dst, sizeof(gDeckMenuPalettes[0]));
        LoadBgMap(0, &gDeck1PanelMap[0xC0], 0x180);
        LoadBgMap(1, gDeck2PanelMap, 0x180);
        LoadBgMap(2, gDeck3PanelMap, 0x180);
        SetBgScroll(0, 0xFFB4, 0xFFF2);
        SetBgScroll(1, 0xFFA8, 0xFFC0);
        SetBgScroll(2, 0xFFA8, 0xFF90);
        work->deckNameX = 100;
        work->deckNameY = 25;
        work->deckName2X = 102;
        work->deckName2Y = 75;
        work->deckName3X = 102;
        work->deckName3Y = 122;
        break;
    case 1:
        dst = (void*)(BG_PLTT + 9 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckTabHighlightPalette, dst, sizeof(gDeckTabHighlightPalette));
        dst = (void*)(BG_PLTT + 8 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckMenuPalettes[8], dst, sizeof(gDeckMenuPalettes[0]));
        dst = (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckMenuPalettes[10], dst, sizeof(gDeckMenuPalettes[0]));
        LoadBgMap(0, gDeck1PanelMap, 0x180);
        LoadBgMap(1, &gDeck2PanelMap[0xC0], 0x180);
        LoadBgMap(2, gDeck3PanelMap, 0x180);
        SetBgScroll(0, 0xFFA8, 0xFFF0);
        SetBgScroll(1, 0xFFB4, 0xFFC2);
        SetBgScroll(2, 0xFFA8, 0xFF90);
        work->deckNameX = 102;
        work->deckNameY = 27;
        work->deckName2X = 100;
        work->deckName2Y = 73;
        work->deckName3X = 102;
        work->deckName3Y = 122;
        break;
    case 2:
        dst = (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckTabHighlightPalette, dst, sizeof(gDeckTabHighlightPalette));
        dst = (void*)(BG_PLTT + 8 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckMenuPalettes[8], dst, sizeof(gDeckMenuPalettes[0]));
        dst = (void*)(BG_PLTT + 9 * PLTT_SIZE_4BPP);
        LoadPalette(gDeckMenuPalettes[9], dst, sizeof(gDeckMenuPalettes[0]));
        LoadBgMap(0, gDeck1PanelMap, 0x180);
        LoadBgMap(1, gDeck2PanelMap, 0x180);
        LoadBgMap(2, &gDeck3PanelMap[0xC0], 0x180);
        SetBgScroll(0, 0xFFA8, 0xFFF0);
        SetBgScroll(1, 0xFFA8, 0xFFC0);
        SetBgScroll(2, 0xFFB4, 0xFF92);
        work->deckNameX = 102;
        work->deckNameY = 27;
        work->deckName2X = 102;
        work->deckName2Y = 75;
        work->deckName3X = 100;
        work->deckName3Y = 121;
        break;
    }
}

void DrawDeckCardCount(u8 deck) {
    u8 countDigits[2];
    u8 maxDigits[2];
    u8* base;
    u16 n;

    base = NULL;
    n = GetDeckCardCount(deck);
    countDigits[0] = n / 10;
    countDigits[1] = n - (u16)(n / 10) * 10;
    maxDigits[0] = 9;
    maxDigits[1] = 9;

    switch (deck) {
    case 0:
#ifdef VERSION_EU
        base = (u8*)GetBgCharBase(0) + 0x2BE0;
#else
        base = GetBgCharBase(0);
#endif
        break;
    case 1:
#ifdef VERSION_EU
        base = (u8*)GetBgCharBase(1) + 0x2F40;
#else
        base = GetBgCharBase(1);
#endif
        break;
    case 2:
#ifdef VERSION_EU
        base = (u8*)GetBgCharBase(2) + 0x32A0;
#else
        base = GetBgCharBase(2);
#endif
        break;
    }

    PortCopyDeckDigit(&gDeckCountDigitTiles[(countDigits[0] + 1) * 32], base + 0x20, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(countDigits[1] + 1) * 32], base + 0x40, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(maxDigits[0] + 1) * 32], base + 0x60, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(maxDigits[1] + 1) * 32], base + 0x80, 32);
}

void DrawDeckEquipMarker(u8 deck) {
    u8* base0;
    u8* base1;
    u8* base2;
#ifdef VERSION_EU
    u8* src;

    base0 = (u8*)GetBgCharBase(0) + 0x2D80;
    base1 = (u8*)GetBgCharBase(1) + 0x30E0;
    base2 = (u8*)GetBgCharBase(2) + 0x3440;
    src = gDeckEquipMarkerTilesByLanguage[gLanguage];

    switch (deck) {
    case 0:
        RequestDma3Copy(src + 0x20, base0, 0x1E0);
        RequestDma3Copy(src + 0x420, base1, 0x1E0);
        RequestDma3Copy(src + 0x420, base2, 0x1E0);
        break;
    case 1:
        RequestDma3Copy(src + 0x420, base0, 0x1E0);
        RequestDma3Copy(src + 0x20, base1, 0x1E0);
        RequestDma3Copy(src + 0x420, base2, 0x1E0);
        break;
    case 2:
        RequestDma3Copy(src + 0x420, base0, 0x1E0);
        RequestDma3Copy(src + 0x420, base1, 0x1E0);
        RequestDma3Copy(src + 0x20, base2, 0x1E0);
        break;
    }
#else
    base0 = GetBgCharBase(0);
    base1 = GetBgCharBase(1);
    base2 = GetBgCharBase(2);

    switch (deck) {
    case 0:
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x20, base0 + 0x1A0, 0x1E0);
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x420, base1 + 0x1A0, 0x1E0);
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x420, base2 + 0x1A0, 0x1E0);
        break;
    case 1:
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x420, base0 + 0x1A0, 0x1E0);
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x20, base1 + 0x1A0, 0x1E0);
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x420, base2 + 0x1A0, 0x1E0);
        break;
    case 2:
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x420, base0 + 0x1A0, 0x1E0);
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x420, base1 + 0x1A0, 0x1E0);
        RequestDma3Copy(gDeckEquipMarkerTiles + 0x20, base2 + 0x1A0, 0x1E0);
        break;
    }
#endif
}

void DrawDeckCpCost(u8 deck) {
    u8 costDigits[4];
    u8 cpDigits[4];
    u16 cpCost;

    u8* base;

    base = NULL;
    cpCost = GetDeckCpCost(deck);

    costDigits[0] = cpCost / 1000;
    costDigits[1] = cpCost / 100 - costDigits[0] * 10;
    costDigits[2] = cpCost / 10 - costDigits[0] * 100 - costDigits[1] * 10;
    costDigits[3] = cpCost - costDigits[0] * 1000 - costDigits[1] * 100 - costDigits[2] * 10;
    cpDigits[0] = gGameState.progression.cp / 1000;
    cpDigits[1] = gGameState.progression.cp / 100 - cpDigits[0] * 10;
    cpDigits[2] = gGameState.progression.cp / 10 - cpDigits[0] * 100 - cpDigits[1] * 10;
    cpDigits[3] = gGameState.progression.cp - cpDigits[0] * 1000 - cpDigits[1] * 100 - cpDigits[2] * 10;

    switch (deck) {
    case 0:
#ifdef VERSION_EU
        base = (u8*)GetBgCharBase(0) + 0x2BE0;
#else
        base = GetBgCharBase(0);
#endif
        break;
    case 1:
#ifdef VERSION_EU
        base = (u8*)GetBgCharBase(1) + 0x2F40;
#else
        base = GetBgCharBase(1);
#endif
        break;
    case 2:
#ifdef VERSION_EU
        base = (u8*)GetBgCharBase(2) + 0x32A0;
#else
        base = GetBgCharBase(2);
#endif
        break;
    }

    PortCopyDeckDigit(&gDeckCountDigitTiles[(costDigits[0] + 1) * 32], base + 0xA0, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(costDigits[1] + 1) * 32], base + 0xC0, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(costDigits[2] + 1) * 32], base + 0xE0, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(costDigits[3] + 1) * 32], base + 0x100, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(cpDigits[0] + 1) * 32], base + 0x120, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(cpDigits[1] + 1) * 32], base + 0x140, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(cpDigits[2] + 1) * 32], base + 0x160, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(cpDigits[3] + 1) * 32], base + 0x180, 32);
}
#ifdef VERSION_EU
#define FILTER_TAB_OFFSET(mode, euBlock) ((euBlock) * 128)
#else
#define FILTER_TAB_OFFSET(mode, euBlock) ((mode) * 128)
#endif
void DrawDeckFilterTab(u8 categoryFilter, u8 mode) {
#ifdef PLATFORM_PC
    PortMenuTabBg(-1, 3, 0, 16);
#endif
    u8* dst;

    dst = (u8*)GetBgScreenBase(3) + 0x80;

    switch (categoryFilter) {
    case CATEGORY_FILTER_DECK_ALL:
        RequestDma3Copy(gDeckFilterTabMap + FILTER_TAB_OFFSET(mode, 0), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x20 + FILTER_TAB_OFFSET(mode, 0), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_ATTACK:
        RequestDma3Copy(gDeckFilterTabMap + 0xA + FILTER_TAB_OFFSET(mode, 0), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0xA + 0x20 + FILTER_TAB_OFFSET(mode, 0), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_MAGIC:
        RequestDma3Copy(gDeckFilterTabMap + 0x14 + FILTER_TAB_OFFSET(mode, 0), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x14 + 0x20 + FILTER_TAB_OFFSET(mode, 0), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_ITEM:
        RequestDma3Copy(gDeckFilterTabMap + 0x40 + FILTER_TAB_OFFSET(mode, 0), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x40 + 0x20 + FILTER_TAB_OFFSET(mode, 0), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_ENEMY:
        RequestDma3Copy(gDeckFilterTabMap + 0x4A + FILTER_TAB_OFFSET(mode, 0), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x4A + 0x20 + FILTER_TAB_OFFSET(mode, 0), dst + 0x40, 20);
        break;
    }
}

void DrawCollectionFilterTab(u8 categoryFilter, u8 mode) {
#ifdef PLATFORM_PC
    PortMenuTabBg(1, 3, 239, 16);
#endif
    u8* dst;

    dst = (u8*)GetBgScreenBase(3) + 0xA8;

    switch (categoryFilter) {
    case CATEGORY_FILTER_COLLECTION_ALL:
        RequestDma3Copy(gDeckFilterTabMap + FILTER_TAB_OFFSET(mode, 1), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x20 + FILTER_TAB_OFFSET(mode, 1), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_ENEMY:
        RequestDma3Copy(gDeckFilterTabMap + 0xA + FILTER_TAB_OFFSET(mode, 1), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0xA + 0x20 + FILTER_TAB_OFFSET(mode, 1), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_ITEM:
        RequestDma3Copy(gDeckFilterTabMap + 0x14 + FILTER_TAB_OFFSET(mode, 1), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x14 + 0x20 + FILTER_TAB_OFFSET(mode, 1), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_MAGIC:
        RequestDma3Copy(gDeckFilterTabMap + 0x40 + FILTER_TAB_OFFSET(mode, 1), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x40 + 0x20 + FILTER_TAB_OFFSET(mode, 1), dst + 0x40, 20);
        break;
    case CATEGORY_FILTER_ATTACK:
        RequestDma3Copy(gDeckFilterTabMap + 0x4A + FILTER_TAB_OFFSET(mode, 1), dst, 20);
        RequestDma3Copy(gDeckFilterTabMap + 0x4A + 0x20 + FILTER_TAB_OFFSET(mode, 1), dst + 0x40, 20);
        break;
    }
}

void DrawCardTotals() {
    u8 inDeckDigits[3];
    u8 collectionDigits[3];
    u16 inDeckCount;
    u16 collectionCount;

    u8* base;

    inDeckCount = CountCardsInDecks();
    collectionCount = CountCollectionCards();

    inDeckDigits[0] = inDeckCount / 100;
    inDeckDigits[1] = inDeckCount / 10 - inDeckDigits[0] * 10;
    inDeckDigits[2] = inDeckCount - inDeckDigits[0] * 100 - inDeckDigits[1] * 10;
    collectionDigits[0] = collectionCount / 100;
    collectionDigits[1] = collectionCount / 10 - collectionDigits[0] * 10;
    collectionDigits[2] = collectionCount - collectionDigits[0] * 100 - collectionDigits[1] * 10;
    base = GetBgCharBase(3);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(inDeckDigits[0] + 1) * 32], base + 0x2A0, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(inDeckDigits[1] + 1) * 32], base + 0x2C0, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(inDeckDigits[2] + 1) * 32], base + 0x2E0, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(collectionDigits[0] + 1) * 32], base + 0x300, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(collectionDigits[1] + 1) * 32], base + 0x320, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(collectionDigits[2] + 1) * 32], base + 0x340, 32);
}

void LoadDeckNameTexts(DeckMenuWork* work) {
    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    InitTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    work->textSlotCount = LoadTextSlots(GetDeckName(0), work->textSlots);
    work->textSlotCount2 = LoadTextSlots(GetDeckName(1), work->textSlots2);
    work->textSlotCount3 = LoadTextSlots(GetDeckName(2), work->textSlots3);
}

void LoadCardNameText(DeckMenuWork* work, s32 id) {
    const CardDef* def;

    def = &gCardDefs[id];
#ifdef VERSION_EU
    work->textSlotCount4 = LoadTextSlots(GetLocalizedString(def->name), work->textSlots4);
#else
    work->textSlotCount4 = LoadTextSlots(def->name, work->textSlots4);
#endif

    switch (def->category) {
    case CARD_CATEGORY_ATTACK:
        LoadPalette(gDeckMenuTextRedPalette,
                    (void*)(work->palette4->index * 32 +
                            OBJ_PLTT),
                    work->palette4->count << 5);
        break;
    case CARD_CATEGORY_MAGIC:
        LoadPalette(gDeckMenuTextBluePalette,
                    (void*)(work->palette4->index * 32 +
                            OBJ_PLTT),
                    work->palette4->count << 5);
        break;
    case CARD_CATEGORY_ITEM:
        LoadPalette(gDeckMenuTextGreenPalette,
                    (void*)(work->palette4->index * 32 +
                            OBJ_PLTT),
                    work->palette4->count << 5);
        break;
    case CARD_CATEGORY_ENEMY:
        LoadPalette(gDeckMenuTextGrayPalette,
                    (void*)(work->palette4->index * 32 +
                            OBJ_PLTT),
                    work->palette4->count << 5);
        break;
    }
}

void LoadCardDescriptionText(DeckMenuWork* work, u16 index) {
    const CardDef* def;
    void* description;

    def = &gCardDefs[index];
#ifdef PLATFORM_PC
    description = def->kind < ARRAY_COUNT(gCardKindDescriptions) ? gCardKindDescriptions[def->kind] : NULL;
    /* Cheats/old edited saves may contain the boss-only Marluxia 2 card,
       whose cartridge description is NULL. Do not localize/dereference it. */
    work->textSlotCount5 = description ? LoadTextSlots(LANGSTR(description), work->textSlots5) : 0;
#else
    description = gCardKindDescriptions[def->kind];
    work->textSlotCount5 = LoadTextSlots(LANGSTR(description), work->textSlots5);
#endif
}

s32 ShowCollectionCardPreview(DeckMenuWork* work) {
    DeckCard2Work* node;
    const CardDef* def;
    void* dst;
    u16 id;
    u16 flag;
    u16 defIndex;
    u8 i;
    u8 j;
    u16 k;

    id = CARD_ID_NONE;
    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (node->args.row == work->cursorRow &&
            node->args.col == work->cursorCol) {
            id = node->args.cardId;
            break;
        }

        node = ListPoolNext(&node->node);
    }

    ReleaseCardPreview(work);

    if (id != CARD_ID_NONE) {
        flag = id & CARD_FLAG_PREMIUM;

        if (flag != 0) {
            work->tiles10 = AllocObjTiles(0x280, NULL);
            SetObjTileSource(work->tiles10, gCardPremiumTiles);
            AnimInit(&work->anim, gCardPremiumAnims, gCardPremiumFrames);
            AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
            work->gfx3 = AnimGetGfx(&work->anim);
        }

        def = &gCardDefs[id & CARD_ID_MASK];
        work->tiles7 = LoadObjTiles(gCardBacks[def->category].tiles, 0x300);
        work->tiles8 = LoadObjTiles(def->tiles, 0x200);
        work->palette6 = LoadObjPalette(def->palette, 32);
        work->palette5 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
        work->gfx4 = gCardBacks[def->category].gfx;
        work->gfx5 = def->gfx;

        for (i = 0; i < work->entryCount; i++) {
            if ((u16)(id & CARD_FLAG_PREMIUM) != 0) {
                if (work->entries[i].kind ==
                    def->kind + 143) {
                    break;
                }
            } else {
                if (work->entries[i].kind ==
                    def->kind) {
                    break;
                }
            }
        }

        work->entryIndex = i;
        dst = (void*)(BG_PLTT + 11 * PLTT_SIZE_4BPP);
        LoadPalette(&gCardCategoryPalettes[def->category * 16], dst, 32);

        for (j = 0; j < ARRAY_COUNT(work->entries[i].valueCounts); j++) {
            DrawValueCount(work->entries[i].valueCounts[j], j);
        }

        defIndex = id & CARD_ID_MASK;
        LoadCardNameText(work, defIndex);
        LoadCardDescriptionText(work, defIndex);

        if (work->view >= DECK_MENU_VIEW_DELETE_GRID &&work->view <= DECK_MENU_VIEW_DELETE_PROMPT) {
            if (def->kind > CARD_KIND_THE_KING) {
                LoadBgMap(2, gDeckCollectionEnemyInfoMap, sizeof(gDeckCollectionEnemyInfoMap));
                DrawCpCost(GetCardCpCost(id));
                return id;
            }

            LoadBgMap(2, gDeckCollectionInfoMap, sizeof(gDeckCollectionInfoMap));
            DrawCpCost(0);
        } else if (def->kind > CARD_KIND_THE_KING) {
            switch (work->deckIndex) {
            case 0:
                LoadBgMap(1, gDeckCollectionEnemyInfoMap, sizeof(gDeckCollectionEnemyInfoMap));
                break;
            case 1:
            case 2:
                LoadBgMap(0, gDeckCollectionEnemyInfoMap, sizeof(gDeckCollectionEnemyInfoMap));
                break;
            }

            DrawCpCost(0);
        } else {
            switch (work->deckIndex) {
            case 0:
                LoadBgMap(1, gDeckCollectionInfoMap, sizeof(gDeckCollectionInfoMap));
                break;
            case 1:
            case 2:
                LoadBgMap(0, gDeckCollectionInfoMap, sizeof(gDeckCollectionInfoMap));
                break;
            }

            DrawCpCost(0);
        }
    } else {
        for (k = 0; k <= 9; k++) {
            DrawValueCount(0, k);
        }

        DrawCpCost(0);
    }

    return id;
}

void ReleaseCardPreview(DeckMenuWork* work) {
    if (work->tiles10 != NULL) {
        ReleaseObjTiles(work->tiles10);
        work->tiles10 = NULL;
    }

    if (work->tiles7 != NULL) {
        ReleaseObjTiles(work->tiles7);
        ReleaseObjPalette(work->palette5);
        ReleaseObjTiles(work->tiles8);
        ReleaseObjPalette(work->palette6);

        if (work->tiles9 != NULL) {
            ReleaseObjTiles(work->tiles9);
            work->tiles9 = NULL;
        }

        work->tiles7 = NULL;
        work->palette5 = NULL;
        work->tiles8 = NULL;
        work->palette6 = NULL;
    }
}

void ShowDeckCardPreview(DeckMenuWork* work) {
    DeckCard2Work* node;
    const CardDef* def;
    void* dst;
    u16 id;

    id = CARD_ID_NONE;
    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (node->args.row == work->cursorRow && node->args.col == work->cursorCol) {
            id = node->args.cardId;
            break;
        }

        node = ListPoolNext(&node->node);
    }

    ReleaseCardPreview(work);

    if (id != CARD_ID_NONE) {
        if (id & CARD_FLAG_PREMIUM) {
            work->tiles10 = AllocObjTiles(0x280, NULL);
            SetObjTileSource(work->tiles10, gCardPremiumTiles);
            AnimInit(&work->anim, gCardPremiumAnims, gCardPremiumFrames);
            AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
            work->gfx3 = AnimGetGfx(&work->anim);
        }

        def = &gCardDefs[id & CARD_ID_MASK];
        work->tiles7 = LoadObjTiles(gCardBacks[def->category].tiles, 768);
        work->tiles8 = LoadObjTiles(def->tiles, 512);
        work->palette6 = LoadObjPalette(def->palette, 32);
        work->palette5 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
        work->gfx4 = gCardBacks[def->category].gfx;
        work->gfx5 = def->gfx;

        if ((id & CARD_ID_MASK) <= CARD_ID_LAST_VALUED) {
            work->tiles9 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
            work->gfx6 = gCardValueDigitFrames[def->value];
        }

        DrawCpCost(GetCardCpCost(id));
        dst = (void*)(BG_PLTT + 11 * PLTT_SIZE_4BPP);
        LoadPalette(&gCardCategoryPalettes[def->category * 16], dst, 32);
        LoadCardNameText(work, id & CARD_ID_MASK);
        LoadCardDescriptionText(work, id & CARD_ID_MASK);
    } else {
        DrawCpCost(0);
    }
}

void DrawValueCount(u8 count, u16 value) {
    u8 digits[2];
    u8* base;

    base = GetBgCharBase(3);

    if (count != 0) {
        u8* dst;

        digits[0] = count / 10;
        digits[1] = count - digits[0] * 10;
        PortCopyDeckValueDigit(&gDeckValueDigitTiles[(digits[0] + 3) * 32], dst = base + (value * 64 + 0xD20), 32);
        PortCopyDeckValueDigit(&gDeckValueDigitTiles[(digits[1] + 3) * 32], dst += 32, 32);
    } else {
        u8* dst;

        PortCopyDeckValueDigit(gDeckValueZeroTiles, dst = base + (value * 64 + 0xD20), 32);
        PortCopyDeckValueDigit(gDeckValueZeroTiles, dst += 32, 32);
        LoadPalette(gUnk_09614406, (void*)(value * 2 + BG_PLTT + 11 * PLTT_SIZE_4BPP + 0xC), sizeof(gUnk_09614406[0]));
    }
}

void DrawSelectedValueCpCost(DeckMenuWork* work) {
    u16 baseCardId;

    baseCardId = GetCardIdForKindEntry(work->entries[work->entryIndex].kind);
    DrawCpCost(GetCardCpCost(baseCardId + work->cursorCol * 5 + (u16)work->cursorRow));
}

void DrawCpCost(u8 cpCost) {
    u8 digits[2];
    u8* base;

    base = GetBgCharBase(3);

    if (cpCost != 0) {
        digits[0] = cpCost / 10;
        digits[1] = cpCost - digits[0] * 10;
        PortCopyDeckValueDigit(&gDeckValueDigitTiles[(digits[0] + 3) * 32], base + 0xCE0, 32);
        PortCopyDeckValueDigit(&gDeckValueDigitTiles[(digits[1] + 3) * 32], base + 0xD00, 32);
    } else {
        PortCopyDeckValueDigit(gDeckValueZeroTiles, base + 0xCE0, 32);
        PortCopyDeckValueDigit(gDeckValueZeroTiles, base + 0xD00, 32);
    }
}

u32 SumValueCounts(u16* data) {
    u32 sum;
    u16* count;
    s32 i;

    sum = 0;
    count = data;
    i = 9;

    do {
        sum += *count++;
    } while (--i >= 0);

    return sum;
}

s32 MoveValueCursor(DeckMenuWork* work, u16 keys) {
    CardKindEntry* entry;
    u8 startCol;
    u8 startRow;
    u8 idx;
    s8 row;
    s8 delta;
    s32 i;
    s32 sum;
    s32 n;
    u16 cursorRow;
    u16 cursorCol;
    s16 k;
    u8* x;

    k = work->cursorCol * 5 + *(u8*)&work->cursorRow;
    idx = k;
    entry = &work->entries[work->entryIndex];
    cursorCol = *(u16*)&work->cursorCol;
    startCol = work->cursorCol;
    startRow = *(u8*)&work->cursorRow;

    if (entry->valueCounts[idx] != 0) {
        return TRUE;
    }

    switch (keys) {
    case DPAD_UP:
        do {
            cursorRow = *(u16*)&work->cursorRow;
            *(u16*)&work->cursorRow = (s16)cursorRow > 0 ? cursorRow - 1 : 4;
            idx = work->cursorCol * 5 + *(u8*)&work->cursorRow;

            if (work->cursorCol == startCol && work->cursorRow == startRow) {
                return FALSE;
            }
        } while (entry->valueCounts[idx] == 0);

        break;
    case DPAD_DOWN:
        do {
            cursorRow = *(u16*)&work->cursorRow;
            *(u16*)&work->cursorRow = (s16)cursorRow <= 3 ? cursorRow + 1 : 0;
            idx = work->cursorCol * 5 + *(u8*)&work->cursorRow;

            if (work->cursorCol == startCol && work->cursorRow == startRow) {
                return FALSE;
            }
        } while (entry->valueCounts[idx] == 0);

        break;
    case DPAD_LEFT:
        if (entry->valueCounts[work->cursorRow] != 0) {
            if ((s16)cursorCol > 0) {
                *(u16*)&work->cursorCol = cursorCol - 1;
            }

            break;
        }

        sum = 0;

        for (i = 0; i < 5; i++) {
            sum += entry->valueCounts[i];
        }

        if (sum == 0) {
            *(u16*)&work->cursorCol = 1;
            return FALSE;
        }

        x = (u8*)&work->cursorRow;
        delta = -1;
        n = *x + delta;

        while (1) {
            row = n;

            if (row < 0) {
                row = 0;
            }

            if (row > 4) {
                row = 4;
            }

            if (entry->valueCounts[row] != 0) {
                break;
            }

            if (delta < 0) {
                delta = -delta;
            } else {
                delta = delta + 1;
                delta = -delta;
            }

            n = delta + *x;
        }

        goto store;
    case DPAD_RIGHT:
        if (entry->valueCounts[work->cursorRow + 5] != 0) {
            if ((s16)cursorCol <= 0) {
                *(u16*)&work->cursorCol = cursorCol + 1;
            }

            break;
        }

        sum = 0;

        for (i = 0; i < 5; i++) {
            sum += entry->valueCounts[i + 5];
        }

        if (sum == 0) {
            *(u16*)&work->cursorCol = 0;
            return FALSE;
        }

        x = (u8*)&work->cursorRow;
        delta = -1;
        n = *x + delta;

        for (;;) {
            row = n;

            if (row < 0) {
                row = 0;
            }

            if (row > 4) {
                row = 4;
            }

            if (entry->valueCounts[row + 5] != 0) {
                break;
            }

            if (delta < 0) {
                delta = -delta;
            } else {
                delta = delta + 1;
                delta = -delta;
            }

            n = delta + *x;
        }

store:
        k = row;
        *(u16*)&work->cursorRow = k;
        break;
    case 0:
        do {
            cursorRow = *(u16*)&work->cursorRow;

            if ((s16)cursorRow <= 3) {
                *(u16*)&work->cursorRow = cursorRow + 1;
            } else {
                *(u16*)&work->cursorRow = 0;
            }

            if (work->cursorCol == startCol && work->cursorRow == startRow) {
                cursorCol = *(u16*)&work->cursorCol;

                if ((s16)cursorCol <= 0) {
                    *(u16*)&work->cursorCol = cursorCol + 1;
                } else {
                    *(u16*)&work->cursorCol = 0;
                }

                *(u16*)&work->cursorRow = 0;

                if (SumValueCounts(entry->valueCounts) == 0) {
                    return FALSE;
                }
            }

            idx = work->cursorCol * 5 + *(u8*)&work->cursorRow;
        } while (entry->valueCounts[idx] == 0);

        break;
    }

    return TRUE;
}

s32 AddSelectedValueCardToDeck(DeckMenuWork* work) {
    u16 mask;
    u16 idx;
    CardKindEntry* entry;
    u16 i;
    u16 card;
    u16 raw;
    u32 id;
    const CardDef* def;

    mask = 0;
    idx = work->cursorCol * 5 + work->cursorRow;

    switch (work->deckIndex) {
    case 0:
        mask = CARD_FLAG_IN_DECK_1;
        break;
    case 1:
        mask = CARD_FLAG_IN_DECK_2;
        break;
    case 2:
        mask = CARD_FLAG_IN_DECK_3;
        break;
    }

    entry = &work->entries[work->entryIndex];

    if (entry->valueCounts[idx] == 0) {
        m4aSongNumStart(SONG_SYS_BEEP);
        return 0;
    }

    for (i = 0; i < entry->count; i++) {
        card = entry->indices[i];
        raw = gCardCollection[card];

        if (!(mask & raw)) {
            id = raw & CARD_ID_MASK;
            def = &gCardDefs[id];

            if (id > CARD_ID_LAST_VALUED) {
                if (idx == 0) {
                    AddCardToDeck(card, work->deckIndex);
                    entry->valueCounts[idx]--;
                    DrawValueCount(entry->valueCounts[idx], idx);
                    m4aSongNumStart(SONG_SYS_KETTEI);
                    return entry->valueCounts[idx];
                }
            } else if (def->value == idx) {
                AddCardToDeck(card, work->deckIndex);
                entry->valueCounts[idx]--;
                DrawValueCount(entry->valueCounts[idx], idx);
                m4aSongNumStart(SONG_SYS_KETTEI);
                return entry->valueCounts[idx];
            }
        }
    }

    m4aSongNumStart(SONG_SYS_BEEP);
}

void FreeCollectionEntries(DeckMenuWork* work) {
    u16 i;

    if (work->entries != NULL) {
        for (i = 0; i < work->entryCount; i++) {
            EwramFree(work->entries[i].indices);
        }

        EwramFree(work->entries);
        work->entries = NULL;
    }
}

void ReleaseCommandMenuGfx(DeckMenuWork* work) {
    if (work->tiles3 != NULL) {
        ReleaseObjTiles(work->tiles3);
        ReleaseObjPalette(work->palette2);
        work->tiles3 = NULL;
        work->palette2 = NULL;
    }
}

void SetDeckMenuFrameCursor(DeckMenuWork* work, u8 frameCursor) {
    switch (frameCursor) {
    case DECK_FRAME_CURSOR_CARD:
        SetObjTileSource(work->tiles2, gDeckCardCursorTiles);
        AnimInit(&work->anim3, gDeckCardCursorAnims, gDeckCardCursorFrames);
        AnimStart(&work->anim3, 0, ANIM_FLAG_LOOP);
        work->gfx2 = AnimGetGfx(&work->anim3);
        break;
    case DECK_FRAME_CURSOR_ROW:
        SetObjTileSource(work->tiles2, gDeckRowCursorTiles);
        AnimInit(&work->anim3, gDeckRowCursorAnims, gDeckRowCursorFrames);
        AnimStart(&work->anim3, 0, ANIM_FLAG_LOOP);
        work->gfx2 = AnimGetGfx(&work->anim3);
        break;
    }
}

void RemoveCursorCardFromDeck(DeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (node->args.row == work->cursorRow && node->args.col == work->cursorCol) {
            if (node->args.cardId != CARD_ID_NONE) {
                RemoveCardFromDeck(node->args.slot, work->deckIndex);
                node->done = TRUE;
                node->args.row = (u16)node->args.row | 0xFFFF;
                m4aSongNumStart(SONG_SYS_KETTEI);
            } else {
                m4aSongNumStart(SONG_SYS_BEEP);
            }

            return;
        }

        node = ListPoolNext(&node->node);
    }

    m4aSongNumStart(SONG_SYS_BEEP);
}

u8 CheckCardDeletable(DeckMenuWork* work) {
    u16 idx;
    CardKindEntry* entry;
    u16 i;
    u16 card;
    u16 cardId;
    const CardDef* def;

    idx = work->cursorCol * 5 + work->cursorRow;
    entry = &work->entries[work->entryIndex];

    if (entry->valueCounts[idx] == 0) {
        m4aSongNumStart(SONG_SYS_BEEP);
        return TRUE;
    }

    for (i = 0; i < entry->count; i++) {
        card = entry->indices[i];

        if (card == CARD_NONE) {
            continue;
        }

        cardId = gCardCollection[card] & CARD_ID_MASK;
        def = &gCardDefs[cardId];

        // @bug Should be >=, so card 450 (the first enemy card) is missed (harmless here).
        if (cardId > CARD_ID_FIRST_ENEMY) {
            if (idx != 0) {
                continue;
            }

            if (def->category != CARD_CATEGORY_ATTACK) {
                return TRUE;
            }

            if (CountCollectionCardsOfCategory(def->category) > 1) {
                return TRUE;
            }

            TaskCreate(&work->cardpool, &gTaskDescDeckErrorLastAttackCard, &work->popupActive);
            m4aSongNumStart(SONG_SYS_BEEP);
            return FALSE;
        } else {
            if (def->value != idx) {
                continue;
            }

            if (def->category != CARD_CATEGORY_ATTACK) {
                return TRUE;
            }

            if (CountCollectionCardsOfCategory(def->category) > 1) {
                return TRUE;
            }

            TaskCreate(&work->cardpool, &gTaskDescDeckErrorLastAttackCard, &work->popupActive);
            m4aSongNumStart(SONG_SYS_BEEP);
            return FALSE;
        }
    }

    return TRUE;
}

u8 DeleteSelectedValueCard(DeckMenuWork* work) {
    u16 idx;
    CardKindEntry* entry;
    u16 i;
    u16 card;
    u16 id;
    const CardDef* def;

    idx = work->cursorCol * 5 + work->cursorRow;
    entry = &work->entries[work->entryIndex];

    if (entry->valueCounts[idx] == 0) {
        m4aSongNumStart(SONG_SYS_BEEP);
        return TRUE;
    }

    for (i = 0; i < entry->count; i++) {
        card = entry->indices[i];

        if (card != CARD_NONE) {
            id = gCardCollection[card] & CARD_ID_MASK;
            def = &gCardDefs[id];

            if (id >= CARD_ID_FIRST_ENEMY) {
                if (idx == 0) {
                    if (def->category == CARD_CATEGORY_ATTACK) {
                        if (CountCollectionCardsOfCategory(def->category) > 1) {
                            ClearCardCollectionSlot(&gCardCollection[card]);
                            entry->indices[i] = CARD_NONE;
                            entry->valueCounts[idx]--;
                            DrawValueCount(entry->valueCounts[idx], idx);
                            m4aSongNumStart(SONG_SYS_CARD_DELETE);
                            return TRUE;
                        } else {
                            TaskCreate(&work->cardpool, &gTaskDescDeckErrorLastAttackCard, &work->popupActive);
                            m4aSongNumStart(SONG_SYS_BEEP);
                            return FALSE;
                        }
                    } else {
                        ClearCardCollectionSlot(&gCardCollection[card]);
                        entry->indices[i] = CARD_NONE;
                        entry->valueCounts[idx]--;
                        DrawValueCount(entry->valueCounts[idx], idx);
                        m4aSongNumStart(SONG_SYS_CARD_DELETE);
                        return TRUE;
                    }
                }
            } else if (def->value == idx) {
                if (def->category == CARD_CATEGORY_ATTACK) {
                    if (CountCollectionCardsOfCategory(def->category) > 1) {
                        ClearCardCollectionSlot(&gCardCollection[card]);
                        entry->indices[i] = CARD_NONE;
                        entry->valueCounts[idx]--;
                        DrawValueCount(entry->valueCounts[idx], idx);
                        m4aSongNumStart(SONG_SYS_CARD_DELETE);
                        return TRUE;
                    } else {
                        TaskCreate(&work->cardpool, &gTaskDescDeckErrorLastAttackCard, &work->popupActive);
                        m4aSongNumStart(SONG_SYS_BEEP);
                        return FALSE;
                    }
                } else {
                    ClearCardCollectionSlot(&gCardCollection[card]);
                    entry->indices[i] = CARD_NONE;
                    entry->valueCounts[idx]--;
                    DrawValueCount(entry->valueCounts[idx], idx);
                    m4aSongNumStart(SONG_SYS_CARD_DELETE);
                    return TRUE;
                }
            }
        }
    }

    m4aSongNumStart(SONG_SYS_BEEP);
    return TRUE;
}

s32 CheckDeckCpCost(DeckMenuWork* work) {
    if (GetDeckCpCost(GetActiveDeckIndex()) > gGameState.progression.cp) {
        TaskCreate(&work->cardpool, &gTaskDescDeckErrorCp, &work->popupActive);
        m4aSongNumStart(SONG_SYS_BEEP);

        return FALSE;
    }

    return TRUE;
}

s32 CheckDeckHasAttackCard(DeckMenuWork* work) {
    if (CountActiveDeckCardsOfCategory(CARD_CATEGORY_ATTACK) == 0) {
        m4aSongNumStart(SONG_SYS_BEEP);
        TaskCreate(&work->cardpool, &gTaskDescDeckErrorNoAttackCard, &work->popupActive);

        return FALSE;
    }

    return TRUE;
}

void ResetGridScroll(DeckMenuWork* work) {
    DeckCard2Work* node;
    s16 x;
    s16 y;

    node = ListPoolFirst(&work->pool);
    x = 0;
    y = 0;

    while (node != NULL) {
        node->args.col = x;
        node->args.row = y;
        x++;

        if (node->flags & DECK_CARD2_FLAG_GFX_LOADED) {
            ReleaseObjPalette(node->palette2);
            ReleaseObjTiles(node->tiles);
            ReleaseObjPalette(node->palette);
            ReleaseObjTiles(node->tiles2);
            node->flags &= ~DECK_CARD2_FLAG_GFX_LOADED;
            node->tiles = NULL;
            node->palette = NULL;
            node->tiles2 = NULL;
            node->palette2 = NULL;
        }

        if (x > 2) {
            x = 0;
            y++;
        }

        node = ListPoolNext(&node->node);
    }

    work->thumbY = 0x2800;
    work->scrollRowEnd = 4;
}

s32 IsCardAtCursor(DeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (node->args.col == work->cursorCol) {
            if (node->args.row == work->cursorRow) {
                return TRUE;
            }
        }

        node = ListPoolNext(&node->node);
    }

    return FALSE;
}

u8 IsCardAt(DeckMenuWork* work, s16 col, s16 row) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (node->args.col == col && node->args.row == row) {
            return TRUE;
        }

        node = ListPoolNext(&node->node);
    }

    return FALSE;
}

u8 FindCardInDirection(DeckMenuWork* work, s16 x, s16 y, u16 dir) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (node->args.col == x && node->args.row == y) {
            return TRUE;
        }

        node = ListPoolNext(&node->node);
    }

    switch (dir) {
    case DPAD_UP:
        return FindCardInDirection(work, x, y - 1, DPAD_UP);
    case DPAD_DOWN:
        return FindCardInDirection(work, x, y + 1, DPAD_DOWN);
    case DPAD_LEFT:
        return FindCardInDirection(work, x - 1, y, DPAD_LEFT);
    case DPAD_RIGHT:
        return FindCardInDirection(work, x + 1, y, DPAD_RIGHT);
    }

    return FALSE;
}

void RecreateDeckGridCards(DeckMenuWork* work, u8 categoryFilter) {
    DeckCard2Args args;
    u16* deck;
    u8 i;
    s8 x;
    s8 y;

    deck = (u16*)GetDeck(work->deckIndex);
    x = 0;
    y = 4 - work->scrollRowEnd;

    if (categoryFilter == CATEGORY_FILTER_DECK_ALL) {
        for (i = 0; i < 99; i++) {
            if (deck[i] != CARD_NONE) {
                if (categoryFilter == CATEGORY_FILTER_DECK_ALL) {
                    args.pool = &work->pool;
                    args.cardId = gCardCollection[deck[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                    args.col = x;
                    args.row = y;
                    args.panel = DECK_CARD2_PANEL_DECK;
                    args.slot = &deck[i];
                    TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                } else if (gCardDefs[gCardCollection[deck[i]] & CARD_ID_MASK].category == categoryFilter - 1) {
                    args.pool = &work->pool;
                    args.cardId = gCardCollection[deck[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                    args.col = x;
                    args.row = y;
                    args.panel = DECK_CARD2_PANEL_DECK;
                    args.slot = &deck[i];
                    TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                }
            } else {
                args.pool = &work->pool;
                args.cardId = CARD_ID_NONE;
                args.col = x;
                args.row = y;
                args.panel = DECK_CARD2_PANEL_DECK;
                args.slot = &deck[i];
                TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
            }

            x++;

            if (x > 2) {
                x = 0;
                y++;
            }
        }
    } else {
        for (i = 0; i < 99; i++) {
            if (deck[i] != CARD_NONE && gCardDefs[gCardCollection[deck[i]] & CARD_ID_MASK].category == categoryFilter - 1) {
                args.pool = &work->pool;
                args.cardId = gCardCollection[deck[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                args.col = x;
                args.row = y;
                args.panel = DECK_CARD2_PANEL_DECK;
                args.slot = &deck[i];
                TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                x++;
            }

            if (x > 2) {
                x = 0;
                y++;
            }
        }
    }
}

u8 ToggleDeckSlotGap(DeckMenuWork* work) {
    DeckCard2Work* card;
    DeckCard2Work* gap;
    DeckCard2Work* last;
    DeckCard2Work* neighbor;

    card = ListPoolFirst(&work->pool);
    last = ListPoolLast(&work->pool);
    gap = NULL;

    while (card != NULL) {
        if (work->cursorCol == card->args.col && work->cursorRow == card->args.row) {
            gap = card;
            break;
        }

        card = ListPoolNext(&card->node);
    }

    if (card->args.cardId != CARD_ID_NONE) {
        while (gap != NULL) {
            if (gap->args.cardId == CARD_ID_NONE) {
                break;
            }

            gap = ListPoolNext(&gap->node);
        }

        if (gap != NULL) {
            while (card != gap) {
                neighbor = ListPoolPrev(&gap->node);

                if (neighbor != NULL) {
                    *gap->args.slot = *neighbor->args.slot;
                }

                gap = ListPoolPrev(&gap->node);
            }

            *card->args.slot = CARD_NONE;
            ClearCardGrid(work);
            TaskPoolUpdate(&work->taskpool);
            RecreateDeckGridCards(work, work->categoryFilter);
            work->cursorCol++;

            if (work->cursorCol > 2) {
                work->cursorCol = 0;

                if (work->cursorRow <= 2) {
                    work->cursorRow++;
                } else {
                    ScrollGridDown(work);
                }
            }

            work->timer = 1;
            return TRUE;
        }
    } else {
        while (card != NULL) {
            neighbor = ListPoolNext(&card->node);

            if (neighbor != NULL) {
                *card->args.slot = *neighbor->args.slot;
            }

            card = ListPoolNext(&card->node);
        }

        *last->args.slot = CARD_NONE;
        ClearCardGrid(work);
        TaskPoolUpdate(&work->taskpool);
        RecreateDeckGridCards(work, work->categoryFilter);
        return TRUE;
    }

    return FALSE;
}

u8 SwapHeldDeckCard(DeckMenuWork* work) {
    DeckCard2Work* target;
    DeckCard2Work* held;
    DeckCard2Work* node;
    DeckCard2Work* prev;
    u16* targetSlot;
    u16* heldSlot;
    s16 x;
    s16 y;
    u16 targetCard;
    u16 heldCard;

    target = ListPoolFirst(&work->pool);
    held = ListPoolFirst(&work->pool);

    if (work->cursorCol == work->heldCol && work->cursorRow == work->heldRow) {
        return ToggleDeckSlotGap(work);
    }

    while (target != NULL) {
        if (work->cursorCol == target->args.col && work->cursorRow == target->args.row) {
            break;
        }

        target = ListPoolNext(&target->node);
    }

    if (target == NULL) {
        return FALSE;
    }

    while (held != NULL) {
        if (work->heldCol == held->args.col &&work->heldRow == held->args.row) {
            break;
        }

        held = ListPoolNext(&held->node);
    }

    x = target->args.col;
    y = target->args.row;
    target->args.col = held->args.col;
    target->args.row = held->args.row;
    held->args.col = x;
    held->args.row = y;
    targetSlot = target->args.slot;
    targetCard = *targetSlot;
    heldSlot = held->args.slot;
    heldCard = *heldSlot;
    *targetSlot = heldCard;
    *heldSlot = targetCard;
    target->args.slot = heldSlot;
    held->args.slot = targetSlot;

    if (ListPoolPrev(&target->node) == (MapcardWork*)held) {
        ListPoolRemove(&target->node, &work->pool);
        ListPoolInsertBefore(&target->node, &work->pool, &held->node);

        for (node = ListPoolFirst(&work->pool); node != NULL; node = ListPoolNext(&node->node)) {
            if (node == ListPoolNext(&node->node)) {
                break;
            }
        }
    } else if (ListPoolNext(&target->node) == held) {
        ListPoolRemove(&target->node, &work->pool);
        ListPoolInsertAfter(&target->node, &work->pool, &held->node);

        for (node = ListPoolFirst(&work->pool); node != NULL; node = ListPoolNext(&node->node)) {
            if (node == ListPoolNext(&node->node)) {
                break;
            }
        }
    } else {
        prev = ListPoolRemove(&target->node, &work->pool);
        ListPoolInsertBefore(&target->node, &work->pool, &held->node);
        ListPoolRemove(&held->node, &work->pool);

        if (prev == NULL) {
            ListPoolAppend(&held->node, &work->pool);
        } else {
            ListPoolInsertBefore(&held->node, &work->pool, &prev->node);
        }
    }

    return TRUE;
}

enum DeckKeyboardPage {
    DECK_KEYBOARD_PAGE_HIRAGANA,
    DECK_KEYBOARD_PAGE_KATAKANA,
    DECK_KEYBOARD_PAGE_ALPHANUMERIC,
    DECK_KEYBOARD_PAGE_SYMBOLS
};

u8 WrapKanaKeyboardCursor(DeckMenuWork* work, u16 dir) {
    u16 row;
    u16 row2;

    row = work->cursor.parts.y;

    if ((s16)row == 3 && (u16)work->cursor.parts.x > 9) {
        switch (dir) {
        case DPAD_UP:
            work->cursor.parts.y = row - 1;
            break;
        case DPAD_DOWN:
            work->cursor.parts.y = row + 1;
            break;
        case DPAD_LEFT:
            work->cursor.parts.x = 9;
            break;
        case DPAD_RIGHT:
            work->cursor.parts.x = 0;
            break;
        }
    }

    if (work->keyboardPage == DECK_KEYBOARD_PAGE_HIRAGANA) {
        row2 = work->cursor.parts.y;

        if ((s16)row2 == 5 && (u16)(work->cursor.parts.x - 5) <= 4) {
            switch (dir) {
            case DPAD_UP:
                work->cursor.parts.y = row2 - 1;
                break;
            case DPAD_DOWN:
                work->cursor.parts.y = row2 + 1;
                break;
            case DPAD_LEFT:
                work->cursor.parts.x = 4;
                break;
            case DPAD_RIGHT:
                work->cursor.parts.x = 10;
                break;
            }
        }
    }

    if (work->cursor.parts.y > 6) {
        work->cursor.parts.y = 0;
    }

    if (work->cursor.parts.y < 0) {
        work->cursor.parts.y = 6;
    }

    if (work->cursor.parts.x > 14) {
        work->cursor.parts.x = 0;
    }

    if (work->cursor.parts.x < 0) {
        work->cursor.parts.x = 14;
    }

    if (work->cursor.parts.y == 6 && work->cursor.parts.x > 11) {
        if (work->cursor.parts.x == 13 && dir == DPAD_LEFT) {
            work->cursor.parts.x = 11;
            AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
        } else {
            work->cursor.parts.x = 14;
            AnimStart(&work->anim4, 1, ANIM_FLAG_LOOP);
        }

        return FALSE;
    }

    return TRUE;
}

u8 WrapKeyboardCursor(DeckMenuWork* work, u16 keys) {
    if (work->cursor.parts.y == 1 && (u16)work->cursor.parts.x > 10) {
        switch (keys) {
        case DPAD_UP:
            work->cursor.parts.y--;
            break;
        case DPAD_DOWN:
            work->cursor.parts.y++;
            break;
        case DPAD_LEFT:
            work->cursor.parts.x = 10;
            break;
        case DPAD_RIGHT:
            work->cursor.parts.x = 0;
            break;
        }
    }

    if (work->cursor.parts.y == 3 && (u16)work->cursor.parts.x > 10) {
        switch (keys) {
        case DPAD_UP:
            work->cursor.parts.y--;
            break;
        case DPAD_DOWN:
            work->cursor.parts.y++;
            break;
        case DPAD_LEFT:
            work->cursor.parts.x = 10;
            break;
        case DPAD_RIGHT:
            work->cursor.parts.x = 0;
            break;
        }
    }

#if defined(VERSION_JP) || defined(VERSION_EU)
#ifdef VERSION_JP
    if (work->cursor.parts.y == 4 && (u16)work->cursor.parts.x > 9) {
#else
    if (work->cursor.parts.y == 5 && (u16)work->cursor.parts.x > 9) {
#endif
        switch (keys) {
        case DPAD_UP:
#ifdef VERSION_JP
            work->cursor.parts.y -= 2;
#else
            work->cursor.parts.y--;
#endif
            break;
        case DPAD_DOWN:
            work->cursor.parts.y++;
            break;
        case DPAD_LEFT:
            work->cursor.parts.x = 9;
            break;
        case DPAD_RIGHT:
            work->cursor.parts.x = 0;
            break;
        }
    }
#endif

    if (work->cursor.parts.x > 14) {
        work->cursor.parts.x = 0;
    }

    if (work->cursor.parts.x < 0) {
        work->cursor.parts.x = 14;
    }

#ifdef VERSION_EU
    if (work->cursor.parts.y > 7) {
#else
    if (work->cursor.parts.y > 6) {
#endif
        work->cursor.parts.y = 0;
    }

    if (work->cursor.parts.y < 0) {
#ifdef VERSION_EU
        work->cursor.parts.y = 7;
#else
        work->cursor.parts.y = 6;
#endif
    }

#ifndef VERSION_JP
    if (work->cursor.parts.x > gKeyboardRowLayouts[work->cursor.parts.y].count - 1) {
        work->cursor.parts.x = 0;
    }

    if (work->cursor.parts.x < 0) {
        work->cursor.parts.x = gKeyboardRowLayouts[work->cursor.parts.y].count - 1;
    }

    if (work->cursor.parts.y > gKeyboardColumnLayouts[work->cursor.parts.x].count - 1) {
        work->cursor.parts.y = 0;
    }

    if (work->cursor.parts.y < 0) {
        work->cursor.parts.y = gKeyboardColumnLayouts[work->cursor.parts.x].count - 1;
    }
#endif

#ifdef VERSION_EU
    if (work->cursor.parts.y == 7 &&work->cursor.parts.x > 9) {
#else
    if (work->cursor.parts.y == 6 &&work->cursor.parts.x > 9) {
#endif
        if (work->cursor.parts.x == 13 && keys == DPAD_LEFT) {
            work->cursor.parts.x = 9;
            AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
#ifdef VERSION_EU
            work->onEndKey = FALSE;
#endif
        } else {
            work->cursor.parts.x = 14;
            AnimStart(&work->anim4, 1, ANIM_FLAG_LOOP);
#ifdef VERSION_EU
            work->onEndKey = TRUE;
#endif
        }

        return FALSE;
    }

    return TRUE;
}

#ifdef VERSION_EU
u8 func_eu_0808E94C(DeckMenuWork* work, u16 keys) {
    if (work->cursor.parts.y == 1 && (u16)work->cursor.parts.x > 5) {
        switch (keys) {
        case DPAD_UP:
            work->cursor.parts.y--;
            break;
        case DPAD_DOWN:
            work->cursor.parts.y++;
            break;
        case DPAD_LEFT:
            work->cursor.parts.x = 5;
            break;
        case DPAD_RIGHT:
            work->cursor.parts.x = 0;
            break;
        }
    }

    if (work->cursor.parts.y == 4 && (u16)work->cursor.parts.x > 2) {
        switch (keys) {
        case DPAD_UP:
            work->cursor.parts.y--;
            break;
        case DPAD_DOWN:
            work->cursor.parts.y++;
            break;
        case DPAD_LEFT:
            work->cursor.parts.x = 2;
            break;
        case DPAD_RIGHT:
            work->cursor.parts.x = 0;
            break;
        }
    }

    if (work->cursor.parts.x > 14) {
        work->cursor.parts.x = 0;
    }

    if (work->cursor.parts.x < 0) {
        work->cursor.parts.x = 14;
    }

    if (work->cursor.parts.y > 6) {
        work->cursor.parts.y = 0;
    }

    if (work->cursor.parts.y < 0) {
        work->cursor.parts.y = 6;
    }

    if (work->cursor.parts.x > gKeyboardSymbolRowLayouts[work->cursor.parts.y].count - 1) {
        work->cursor.parts.x = 0;
    }

    if (work->cursor.parts.x < 0) {
        work->cursor.parts.x = gKeyboardSymbolRowLayouts[work->cursor.parts.y].count - 1;
    }

    if (work->cursor.parts.y > gKeyboardSymbolColumnLayouts[work->cursor.parts.x].count - 1) {
        work->cursor.parts.y = 0;
    }

    if (work->cursor.parts.y < 0) {
        work->cursor.parts.y = gKeyboardSymbolColumnLayouts[work->cursor.parts.x].count - 1;
    }

    if (work->cursor.parts.y == 6 &&work->cursor.parts.x > 1) {
        if (work->cursor.parts.x == 13 && keys == DPAD_LEFT) {
            work->cursor.parts.x = 1;
            AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
            work->onEndKey = FALSE;
        } else {
            work->cursor.parts.x = 14;
            AnimStart(&work->anim4, 1, ANIM_FLAG_LOOP);
            work->onEndKey = TRUE;
        }

        return FALSE;
    }

    return TRUE;
}
#endif

void DrawKeyboardDeckNumber(u8 deckIndex) {
    u8* base;

    base = GetBgCharBase(3);
    RequestDma3Copy(&gDeckKeyboardDeckNumberTiles[deckIndex * 64], base + 32, 64);
}

void CopyDeckNameToBuffer(DeckMenuWork* work) {
    u8* deckName;
    u8* nameBuffer;
    s32 i;

    deckName = GetDeckName(work->deckIndex);

    for (i = 0; i <= 19; i++) {
        nameBuffer = work->nameBuffer;
        nameBuffer[i] = deckName[i];
    }

    work->nameBuffer[18] = 0;
    work->nameBuffer[19] = 0;
}

void SaveDeckNameFromBuffer(DeckMenuWork* work) {
    u8* deckName;
    u8* nameBuffer;
    s32 i;

    deckName = GetDeckName(work->deckIndex);

    for (i = 0; i <= 19; i++) {
        nameBuffer = work->nameBuffer;
        deckName[i] = nameBuffer[i];
    }

    deckName[18] = 0;
    deckName[19] = 0;
}

void DeleteLastNameChar(DeckMenuWork* work) {
    u8* nameBuffer;
    s32 offset;
    u8 i;

    if (work->textSlotCount6 == 0) {
        m4aSongNumStart(SONG_SYS_BEEP);
        return;
    }

#ifdef VERSION_EU
    for (i = work->textSlotCount6 - 1; i <= 19; i++) {
        nameBuffer = work->nameBuffer;
        nameBuffer[i] = 0;
    }
#else
    for (i = work->textSlotCount6 - 1; i <= 8; i++) {
        nameBuffer = work->nameBuffer;
        offset = i * 2;
        nameBuffer[offset] = 0;
        offset++;
        nameBuffer[offset] = 0;
    }
#endif

    m4aSongNumStart(SONG_SYS_CLOSE);
}

s32 AppendKeyboardChar(DeckMenuWork* work) {
    const u8* src = NULL;
    u8* dst;
    s32 offset;
    s32 zero;
    s32 offset2;
    u8 value;
    u8* out;

    if (work->textSlotCount6 <= 7) {
        m4aSongNumStart(SONG_SYS_KETTEI);

#ifdef VERSION_EU
        if (work->keyboardPage == DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
            src = gDeckKeyboardLetterRows[work->cursor.parts.y];
        } else {
            src = gDeckKeyboardSymbolRows[work->cursor.parts.y];
        }
#else
#ifdef VERSION_JP
        switch (work->keyboardPage) {
        case DECK_KEYBOARD_PAGE_HIRAGANA:
            src = gDeckKeyboardRows[work->cursor.parts.y];
            break;
        case DECK_KEYBOARD_PAGE_KATAKANA:
            src = gDeckKeyboardKatakanaRows[work->cursor.parts.y];
            break;
        case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
            src = gDeckKeyboardAlphanumericRows[work->cursor.parts.y];
            break;
        }
#else
        src = gDeckKeyboardRows[work->cursor.parts.y];
#endif
#endif
#ifdef VERSION_EU
        offset = work->textSlotCount6;
        dst = work->nameBuffer;
        out = &dst[offset];
        value = src[work->cursor.parts.x];
        zero = 0;
        *out = value;
        offset2 = work->textSlotCount6 + 1;
        dst[offset2] = zero;
#else
        offset = work->textSlotCount6 * 2;
        dst = work->nameBuffer;
        out = &dst[offset];
        value = src[work->cursor.parts.x * 2];
        zero = 0;
        *out = value;
        offset2 = work->textSlotCount6 * 2;
        offset2++;
        dst[offset2] = src[work->cursor.parts.x * 2 + 1];
        offset = (work->textSlotCount6 + 1) * 2;
        dst[offset] = zero;
        offset = (work->textSlotCount6 + 1) * 2;
        offset++;
        dst[offset] = zero;
#endif
        return TRUE;
    } else {
        m4aSongNumStart(SONG_SYS_BEEP);
        return FALSE;
    }
}

#if defined(VERSION_JP) || defined(VERSION_EU)
void func_jp_0808F240(DeckMenuWork* work) {
    switch (work->keyboardPage) {
#ifdef VERSION_JP
    case DECK_KEYBOARD_PAGE_HIRAGANA:
        if (work->cursor.parts.y == 5 && (u16)(work->cursor.parts.x - 5) <= 4) {
            work->cursor.parts.x = 4;
        }

        break;
    case DECK_KEYBOARD_PAGE_KATAKANA:
        switch (work->cursor.parts.y) {
        case 0:
        case 1:
        case 2:
            break;
        case 3:
            if (work->cursor.parts.x > 9) {
                work->cursor.parts.x = 9;
            }

            break;
        case 4:
        case 5:
        case 6:
            break;
        }

        break;
    case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
        switch (work->cursor.parts.y) {
        case 0:
            break;
        case 1:
            if (work->cursor.parts.x > 10) {
                work->cursor.parts.x = 10;
            }

            break;
        case 2:
        case 3:
            break;
        case 4:
            if (work->cursor.parts.x > 9) {
                work->cursor.parts.x = 9;
            }

            break;
        case 5:
            break;
        case 6:
            if (work->cursor.parts.x >= 10 &&work->cursor.parts.x <= 13) {
                work->cursor.parts.x = 9;
            }

            break;
        }

        break;
#else
    case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
        switch (work->cursor.parts.y) {
        case 0:
            break;
        case 1:
        case 3:
            if (work->cursor.parts.x > 10) {
                work->cursor.parts.x = 10;
            }

            break;
        case 2:
        case 4:
            break;
        case 5:
            if (work->cursor.parts.x > 9) {
                work->cursor.parts.x = 9;
            }

            break;
        case 6:
            if (work->onEndKey == TRUE) {
                work->cursor.parts.x = 14;
                work->cursor.parts.y = 7;
            }

            break;
        case 7:
            if (work->cursor.parts.x >= 10 &&work->cursor.parts.x <= 13) {
                work->cursor.parts.x = 9;
            }

            break;
        }

        break;
    case DECK_KEYBOARD_PAGE_SYMBOLS:
        switch (work->cursor.parts.y) {
        case 0:
            break;
        case 1:
            if (work->cursor.parts.x > 5) {
                work->cursor.parts.x = 5;
            }

            break;
        case 2:
        case 3:
            break;
        case 4:
            if (work->cursor.parts.x > 2) {
                work->cursor.parts.x = 2;
            }

            break;
        case 5:
            break;
        case 6:
            if (work->onEndKey == TRUE) {
                work->cursor.parts.y = 6;
                work->cursor.parts.x = 14;
            } else if (work->cursor.parts.x > 1) {
                work->cursor.parts.x = 1;
            }

            break;
        case 7:
            if (work->cursor.parts.x == 14) {
                work->cursor.parts.y = 6;
            } else {
                work->cursor.parts.y = 6;
                work->cursor.parts.x = 0;
            }

            break;
        }

        break;
#endif
    }
}

void func_jp_0808F34C(DeckMenuWork* work) {
    switch (work->keyboardPage) {
#ifdef VERSION_JP
    case DECK_KEYBOARD_PAGE_HIRAGANA:
        LoadBgMap(3, gDeckKeyboardMap, sizeof(gDeckKeyboardMap));
        break;
    case DECK_KEYBOARD_PAGE_KATAKANA:
        LoadBgMap(3, gDeckKeyboardKatakanaMap, sizeof(gDeckKeyboardKatakanaMap));
        break;
    case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
        LoadBgMap(3, gDeckKeyboardAlphanumericMap, sizeof(gDeckKeyboardAlphanumericMap));
        break;
#else
    case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
        LoadBgMap(3, gDeckKeyboardMap, sizeof(gDeckKeyboardMap));
        break;
    case DECK_KEYBOARD_PAGE_SYMBOLS:
        LoadBgMap(3, gDeckKeyboardSymbolMap, sizeof(gDeckKeyboardSymbolMap));
        break;
#endif
    }

    func_jp_0808F240(work);
}
#endif

u8 UpdateDeckMenuOpenKeyboard(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 16);

#ifdef VERSION_EU
    work->handVisible = 0;
    work->onEndKey = FALSE;
#endif

    switch (work->step) {
    case 0:
        work->view = DECK_MENU_VIEW_KEYBOARD;
        SetDeckMenuHandAnim(work);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
#ifdef VERSION_EU
        work->tiles11 = AllocObjTiles(0x400, NULL);
#else
        work->tiles11 = AllocObjTiles(0x200, NULL);
#endif
        work->palette7 = LoadObjPalette(gDeckKeyboardCursorPalette, sizeof(gDeckKeyboardCursorPalette));
        work->tiles13 = AllocSpriteFrameTiles(0x80);
#ifdef VERSION_EU
        SetObjTileSource(work->tiles11, gDeckKeyboardCursorTilesByLanguage[gLanguage]);
        AnimInit(&work->anim4, gDeckKeyboardCursorAnimsByLanguage[gLanguage], gDeckKeyboardCursorSpritesByLanguage[gLanguage]);
#else
        SetObjTileSource(work->tiles11, gDeckKeyboardCursorTiles);
        AnimInit(&work->anim4, gDeckKeyboardCursorAnims, gDeckKeyboardCursorFrames);
#endif
        AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
        work->gfx9 = AnimGetGfx(&work->anim4);
#ifdef VERSION_EU
        UpdateSpriteFrameTiles(work->tiles13, gDeckKeyboardCursorSpritesByLanguage[gLanguage][10], gDeckKeyboardCursorTilesByLanguage[gLanguage]);
#else
        UpdateSpriteFrameTiles(work->tiles13, gDeckKeyboardCursorFrames[10], gDeckKeyboardCursorTiles);
#endif
        FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
        FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
        FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
        FreeTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
        FreeTextSlots(work->textSlots5, ARRAY_COUNT(work->textSlots5));
        InitTextSlots(work->textSlots6, ARRAY_COUNT(work->textSlots6));
        CopyDeckNameToBuffer(work);
        work->textSlotCount6 = LoadTextSlots(work->nameBuffer, work->textSlots6);
        work->caretX = (GetTextSlotsWidth(work->textSlots6, work->textSlotCount6) << 8) + 0x8300;
        break;
    case 1:
        LoadBgTiles(3, gDeckKeyboard0Tiles, sizeof(gDeckKeyboard0Tiles));
        break;
    case 2:
#ifdef VERSION_JP
        RequestDma3Copy(gDeckKeyboard1Tiles, (u8*)GetBgCharBase(3) + 0x2000, sizeof(gDeckKeyboard1Tiles));
#else
        RequestDma3Copy(gDeckKeyboard1Tiles, (u8*)GetBgCharBase(3) + 0x1000, sizeof(gDeckKeyboard1Tiles));
#endif
        break;
    case 3:
#ifdef VERSION_JP
        RequestDma3Copy(gDeckKeyboard2Tiles, (u8*)GetBgCharBase(3) + 0x4000, sizeof(gDeckKeyboard2Tiles));
#else
        RequestDma3Copy(gDeckKeyboard2Tiles, (u8*)GetBgCharBase(3) + 0x2000, sizeof(gDeckKeyboard2Tiles));
#endif
        break;
    case 4:
#ifdef VERSION_JP
        RequestDma3Copy(gDeckKeyboard3Tiles, (u8*)GetBgCharBase(3) + 0x6000, sizeof(gDeckKeyboard3Tiles));
#elif defined(VERSION_EU)
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            break;
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gDeckKeyboardTextFrenchTiles, (u8*)GetBgCharBase(3) + 0x4800, sizeof(gDeckKeyboardTextFrenchTiles));
            break;
        case LANGUAGE_GERMAN:
            // @bug Reads past the end of gDeckKeyboardTextGermanTiles.
            RequestDma3Copy(gDeckKeyboardTextGermanTiles, (u8*)GetBgCharBase(3) + 0x4800, 0x800);
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gDeckKeyboardTextItalianTiles, (u8*)GetBgCharBase(3) + 0x4800, sizeof(gDeckKeyboardTextItalianTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gDeckKeyboardTextSpanishTiles, (u8*)GetBgCharBase(3) + 0x4800, sizeof(gDeckKeyboardTextSpanishTiles));
            break;
        }
#endif

        break;
    case 5:
        LoadBgMap(3, gDeckKeyboardMap, sizeof(gDeckKeyboardMap));
        LoadBgPalette(3, gDeckKeyboardPalettes, sizeof(gDeckKeyboardPalettes));
        break;
    case 6:
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuKeyboard);
        work->keyCursorX = gKeyboardKeyX[0] << 8;
        work->keyCursorY = gKeyboardKeyY[0] << 8;
        work->keyCursorSteps = 4;
        work->cursor.parts.x = 0;
        work->cursor.parts.y = 0;
#ifdef VERSION_JP
        work->keyboardPage = DECK_KEYBOARD_PAGE_HIRAGANA;
#else
        work->keyboardPage = DECK_KEYBOARD_PAGE_ALPHANUMERIC;
#endif
        DrawKeyboardDeckNumber(work->deckIndex);
        break;
    }

    work->step++;
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

#if defined(VERSION_JP) || defined(VERSION_EU)
u8 func_jp_0808F638(DeckMenuWork* work, void* task) {
#ifdef VERSION_EU
    u8* page = &work->keyboardPage;
    work->handVisible = 1;
#endif

    switch (GetKeysRepeat()) {
    case DPAD_RIGHT:
#ifdef VERSION_EU
        work->keyCursorSteps = 1;

        if (*page <= DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
            (*page)++;
#else
        if (work->keyboardPage <= DECK_KEYBOARD_PAGE_KATAKANA) {
            work->keyboardPage++;
#endif
            func_jp_0808F34C(work);
            m4aSongNumStart(SONG_SYS_CANSEL);
            work->keyCursorSteps = 1;
        }

        break;
    case DPAD_LEFT:
#ifdef VERSION_EU
        work->keyCursorSteps = 1;

        if (*page > DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
            (*page)--;
#else
        if (work->keyboardPage != DECK_KEYBOARD_PAGE_HIRAGANA) {
            work->keyboardPage--;
#endif
            func_jp_0808F34C(work);
            m4aSongNumStart(SONG_SYS_CANSEL);
            work->keyCursorSteps = 1;
        }

        break;
    case SELECT_BUTTON:
    case DPAD_DOWN:
        work->keyCursorSteps = 1;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuKeyboard);
        work->view = DECK_MENU_VIEW_KEYBOARD;
        m4aSongNumStart(SONG_SYS_CLICKI04B);
#ifdef VERSION_EU
        work->handVisible = 0;
#endif
        return 1;
    }

#ifdef VERSION_EU
    if (work->keyCursorSteps != 0) {
        if (work->onEndKey == TRUE) {
            if (gLanguage == LANGUAGE_GERMAN) {
                work->keyCursorX = 0xC800;
            } else if (gLanguage == LANGUAGE_ITALIAN) {
                work->keyCursorX = 0xD100;
            } else {
                work->keyCursorX = 0xD300;
            }

            work->keyCursorY = 0x8C00;
        } else if (work->keyboardPage == DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
            ApproachValue(&work->keyCursorX, gKeyboardRowLayouts[work->cursor.parts.y].positions[work->cursor.parts.x] << 8, work->keyCursorSteps);
            ApproachValue(&work->keyCursorY, gKeyboardColumnLayouts[work->cursor.parts.x].positions[work->cursor.parts.y] << 8, work->keyCursorSteps);
        } else {
            ApproachValue(&work->keyCursorX, gKeyboardSymbolRowLayouts[work->cursor.parts.y].positions[work->cursor.parts.x] << 8, work->keyCursorSteps);
            ApproachValue(&work->keyCursorY, gKeyboardSymbolColumnLayouts[work->cursor.parts.x].positions[work->cursor.parts.y] << 8, work->keyCursorSteps);
        }

        work->keyCursorSteps--;
    }

    ApproachValueHalf(&work->handX, (gKeyboardPageTabXEu[work->keyboardPage - DECK_KEYBOARD_PAGE_ALPHANUMERIC] + 8) << 8);
#else
    ApproachValueHalf(&work->handX, (gKeyboardPageTabXJp[work->keyboardPage] + 8) << 8);
#endif
    ApproachValueHalf(&work->handY, 0x1A00);
    work->gfx9 = AnimUpdate(&work->anim4);
    work->gfx = AnimUpdate(&work->anim2);
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}
#endif

u8 UpdateDeckMenuKeyboard(DeckMenuWork* work, void* task) {
#ifdef VERSION_EU
    u8 page = work->keyboardPage;
    s32 bottom = 6;

    if (page == DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
        bottom = 7;
    }
#endif

    work->gfx9 = AnimUpdate(&work->anim4);
    work->gfx = AnimUpdate(&work->anim2);

    switch (GetKeysRepeat()) {
    case DPAD_LEFT:
        work->keyCursorSteps = 1;
        work->cursor.parts.x--;

        switch (work->keyboardPage) {
        case DECK_KEYBOARD_PAGE_HIRAGANA:
        case DECK_KEYBOARD_PAGE_KATAKANA:
            if (WrapKanaKeyboardCursor(work, DPAD_LEFT)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
            }

            break;
        case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
            if (WrapKeyboardCursor(work, DPAD_LEFT)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
#ifdef VERSION_EU
                work->onEndKey = FALSE;
#endif
            }

            break;
#ifdef VERSION_EU
        case DECK_KEYBOARD_PAGE_SYMBOLS:
            if (func_eu_0808E94C(work, DPAD_LEFT)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
                work->onEndKey = FALSE;
            }

            break;
#endif
        }

        m4aSongNumStart(SONG_SYS_CLICKI04B);
        break;
    case DPAD_RIGHT:
        work->keyCursorSteps = 1;
        work->cursor.parts.x++;

        switch (work->keyboardPage) {
        case DECK_KEYBOARD_PAGE_HIRAGANA:
        case DECK_KEYBOARD_PAGE_KATAKANA:
            if (WrapKanaKeyboardCursor(work, DPAD_RIGHT)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
            }

            break;
        case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
            if (WrapKeyboardCursor(work, DPAD_RIGHT)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
#ifdef VERSION_EU
                work->onEndKey = FALSE;
#endif
            }

            break;
#ifdef VERSION_EU
        case DECK_KEYBOARD_PAGE_SYMBOLS:
            if (func_eu_0808E94C(work, DPAD_RIGHT)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
                work->onEndKey = FALSE;
            }

            break;
#endif
        }

        m4aSongNumStart(SONG_SYS_CLICKI04B);
        break;
    case DPAD_UP:
        work->keyCursorSteps = 1;
        work->cursor.parts.y--;

#if defined(VERSION_JP) || defined(VERSION_EU)
        if (work->cursor.parts.y < 0) {
            work->keyCursorSteps = 1;
            work->cursor.parts.y = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)func_jp_0808F638);
            m4aSongNumStart(SONG_SYS_CLICKI04B);
            return 1;
        }
#endif

        switch (work->keyboardPage) {
        case DECK_KEYBOARD_PAGE_HIRAGANA:
        case DECK_KEYBOARD_PAGE_KATAKANA:
            if (WrapKanaKeyboardCursor(work, DPAD_UP)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
            }

            break;
        case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
            if (WrapKeyboardCursor(work, DPAD_UP)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
#ifdef VERSION_EU
                work->onEndKey = FALSE;
#endif
            }

            break;
#ifdef VERSION_EU
        case DECK_KEYBOARD_PAGE_SYMBOLS:
            if (func_eu_0808E94C(work, DPAD_UP)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
                work->onEndKey = FALSE;
            }

            break;
#endif
        }

        m4aSongNumStart(SONG_SYS_CLICKI04B);
        break;
    case DPAD_DOWN:
        work->keyCursorSteps = 1;
        work->cursor.parts.y++;

        switch (work->keyboardPage) {
        case DECK_KEYBOARD_PAGE_HIRAGANA:
        case DECK_KEYBOARD_PAGE_KATAKANA:
            if (WrapKanaKeyboardCursor(work, DPAD_DOWN)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
            }

            break;
        case DECK_KEYBOARD_PAGE_ALPHANUMERIC:
            if (WrapKeyboardCursor(work, DPAD_DOWN)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
#ifdef VERSION_EU
                work->onEndKey = FALSE;
#endif
            }

            break;
#ifdef VERSION_EU
        case DECK_KEYBOARD_PAGE_SYMBOLS:
            if (func_eu_0808E94C(work, DPAD_DOWN)) {
                AnimStart(&work->anim4, 0, ANIM_FLAG_LOOP);
                work->onEndKey = FALSE;
            }

            break;
#endif
        }

        m4aSongNumStart(SONG_SYS_CLICKI04B);
        break;
    }

    switch (GetKeysPressed()) {
    case B_BUTTON:
        DeleteLastNameChar(work);
        work->textSlotCount6 = LoadTextSlots(work->nameBuffer, work->textSlots6);
        work->caretX = (GetTextSlotsWidth(work->textSlots6, work->textSlotCount6) << 8) + 0x8300;
        break;
    case A_BUTTON:
#ifdef VERSION_EU
        if (work->cursor.parts.x == 14 &&work->cursor.parts.y == bottom) {
#else
        if (work->cursor.packed == 0x6000E) {
#endif
            SaveDeckNameFromBuffer(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuCloseKeyboard);
            m4aSongNumStart(SONG_SYS_KETTEI);
            FadeStartIn(FADE_MODE_BLACK, 16);
        } else {
            if ((u8)AppendKeyboardChar(work)) {
                work->textSlotCount6 = LoadTextSlots(work->nameBuffer, work->textSlots6);
                work->caretX = (GetTextSlotsWidth(work->textSlots6, work->textSlotCount6) << 8) + 0x8300;
            } else {
                work->cursor.parts.x = 14;
#ifdef VERSION_EU
                work->cursor.parts.y = bottom;
#else
                work->cursor.parts.y = 6;
#endif
                AnimStart(&work->anim4, 1, ANIM_FLAG_LOOP);
#ifdef VERSION_EU
                work->onEndKey = TRUE;
#endif
            }
        }

        break;
    case START_BUTTON:
        work->cursor.parts.x = 14;
#ifdef VERSION_EU
        work->cursor.parts.y = bottom;

        if (gLanguage == LANGUAGE_GERMAN) {
            work->keyCursorX = 0xC800;
        } else if (gLanguage == LANGUAGE_ITALIAN) {
            work->keyCursorX = 0xD100;
        } else {
            work->keyCursorX = 0xD300;
        }

        work->keyCursorY = 0x8C00;
#else
        work->cursor.parts.y = 6;
#endif
        AnimStart(&work->anim4, 1, ANIM_FLAG_LOOP);
        m4aSongNumStart(SONG_SYS_CLICKI04B);
#ifdef VERSION_EU
        work->onEndKey = TRUE;
#endif
        break;
#if defined(VERSION_JP) || defined(VERSION_EU)
    case R_BUTTON:
#ifdef VERSION_JP
        if (work->keyboardPage <= DECK_KEYBOARD_PAGE_KATAKANA) {
#else
        if (work->keyboardPage <= DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
#endif
            work->keyboardPage++;
            func_jp_0808F34C(work);
            m4aSongNumStart(SONG_SYS_CANSEL);
        }

        break;
    case L_BUTTON:
#ifdef VERSION_JP
        if (work->keyboardPage != DECK_KEYBOARD_PAGE_HIRAGANA) {
#else
        if (work->keyboardPage > DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
#endif
            work->keyboardPage--;
            func_jp_0808F34C(work);
            m4aSongNumStart(SONG_SYS_CANSEL);
        }

        break;
    case SELECT_BUTTON:
        work->keyCursorSteps = 1;
        m4aSongNumStart(SONG_SYS_CLICKI04B);
#ifdef VERSION_EU
        // @bug Should subtract DECK_KEYBOARD_PAGE_ALPHANUMERIC; reads past gKeyboardPageTabXEu.
        work->handX = (gKeyboardPageTabXEu[work->keyboardPage] + 8) << 8;
        work->handY = 0x1A00;
#endif
        SetTaskUpdate(task, (TaskUpdateFunc)func_jp_0808F638);
#ifdef VERSION_JP
        return 1;
#else
        break;
#endif
#endif
    }

    if (work->keyCursorSteps != 0) {
#ifdef VERSION_EU
        if (work->cursor.parts.x == 14 &&work->cursor.parts.y == bottom) {
            if (gLanguage == LANGUAGE_GERMAN) {
                ApproachValue(&work->keyCursorX, 0xC800, work->keyCursorSteps);
            } else if (gLanguage == LANGUAGE_ITALIAN) {
                ApproachValue(&work->keyCursorX, 0xD100, work->keyCursorSteps);
            } else {
                ApproachValue(&work->keyCursorX, 0xD300, work->keyCursorSteps);
            }
#else
        if (work->cursor.packed == 0x6000E) {
            ApproachValue(&work->keyCursorX, 0xD300, work->keyCursorSteps);
#endif
            ApproachValue(&work->keyCursorY, 0x8C00, work->keyCursorSteps);
#ifdef VERSION_EU
        } else if (work->keyboardPage == DECK_KEYBOARD_PAGE_ALPHANUMERIC) {
#else
        } else {
#endif
            ApproachValue(&work->keyCursorX, gKeyboardRowLayouts[work->cursor.parts.y].positions[work->cursor.parts.x] << 8, work->keyCursorSteps);
            ApproachValue(&work->keyCursorY, gKeyboardColumnLayouts[work->cursor.parts.x].positions[work->cursor.parts.y] << 8, work->keyCursorSteps);
#ifdef VERSION_EU
        } else {
            ApproachValue(&work->keyCursorX, gKeyboardSymbolRowLayouts[work->cursor.parts.y].positions[work->cursor.parts.x] << 8, work->keyCursorSteps);
            ApproachValue(&work->keyCursorY, gKeyboardSymbolColumnLayouts[work->cursor.parts.x].positions[work->cursor.parts.y] << 8, work->keyCursorSteps);
#endif
        }
    }

    work->handX = work->keyCursorX + 0x800;
    work->handY = work->keyCursorY + 0x800;
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateDeckMenuCloseKeyboard(DeckMenuWork* work, void* task) {
    FadeStartIn(FADE_MODE_BLACK, 16);
    work->view = DECK_MENU_VIEW_DECK_GRID;
    SetDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
    SetDeckMenuHandAnim(work);
    FreeTextSlots(work->textSlots6, ARRAY_COUNT(work->textSlots6));
    ReleaseObjTiles(work->tiles11);
    ReleaseObjTiles(work->tiles13);
    ReleaseObjPalette(work->palette7);
    work->step = 0;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateDeckMenuLoadBgs);
    CreateDeckGridCards(work, 0);

#ifdef VERSION_EU
    {
        u8* handVisible;
        u8 n;

        handVisible = &work->handVisible;
        n = 1;
        *handVisible = n;
        return n;
    }
#else
    return 1;
#endif
}

void BuildCollectionEntries(DeckMenuWork* work) {
    u16 i;
    u16 j;
    u16 n;

    work->entries = EwramAlloc(work->entryCount * sizeof(CardKindEntry));

    for (i = 0, n = 0; i <= 16; i++) {
        if (work->kindEntries[i].count != 0) {
            work->entries[n] = work->kindEntries[i];
            work->entries[n].indices = EwramAlloc(work->kindEntries[i].indexCount * sizeof(u16));

            for (j = 0; j < work->kindEntries[i].indexCount; j++) {
                work->entries[n].indices[j] = work->kindEntries[i].indices[j];
            }

            n++;
        }
    }

    for (i = 143; i <= 159; i++) {
        if (work->kindEntries[i].count != 0) {
            work->entries[n] = work->kindEntries[i];
            work->entries[n].indices = EwramAlloc(work->kindEntries[i].indexCount * sizeof(u16));

            for (j = 0; j < work->kindEntries[i].indexCount; j++) {
                work->entries[n].indices[j] = work->kindEntries[i].indices[j];
            }

            n++;
        }
    }

    for (i = 18; i <= 31; i++) {
        if (work->kindEntries[i].count != 0) {
            work->entries[n] = work->kindEntries[i];
            work->entries[n].indices = EwramAlloc(work->kindEntries[i].indexCount * sizeof(u16));

            for (j = 0; j < work->kindEntries[i].indexCount; j++) {
                work->entries[n].indices[j] = work->kindEntries[i].indices[j];
            }

            n++;
        }
    }

    for (i = 161; i <= 174; i++) {
        if (work->kindEntries[i].count != 0) {
            work->entries[n] = work->kindEntries[i];
            work->entries[n].indices = EwramAlloc(work->kindEntries[i].indexCount * sizeof(u16));

            for (j = 0; j < work->kindEntries[i].indexCount; j++) {
                work->entries[n].indices[j] = work->kindEntries[i].indices[j];
            }

            n++;
        }
    }

    for (i = 32; i <= 142; i++) {
        if (work->kindEntries[i].count != 0) {
            work->entries[n] = work->kindEntries[i];
            work->entries[n].indices = EwramAlloc(work->kindEntries[i].indexCount * sizeof(u16));

            for (j = 0; j < work->kindEntries[i].indexCount; j++) {
                work->entries[n].indices[j] = work->kindEntries[i].indices[j];
            }

            n++;
        }
    }
}

const u16* gRikuDeckCards[12] = {
    gRikuDeckCards0,
    gRikuDeckCards1,
    gRikuDeckCards2,
    gRikuDeckCards3,
    gRikuDeckCards4,
    gRikuDeckCards5,
    gRikuDeckCards6,
    gRikuDeckCards7,
    gRikuDeckCards8,
    gRikuDeckCards9,
    gRikuDeckCards10,
    gRikuDeckCards11,
};

const u16* gRikuDeckEnemyCards[12] = {
    &gRikuDeckEnemyCard0,
    &gRikuDeckEnemyCard1,
    &gRikuDeckEnemyCard2,
    &gRikuDeckEnemyCard3,
    &gRikuDeckEnemyCard4,
    &gRikuDeckEnemyCard5,
    &gRikuDeckEnemyCard6,
    &gRikuDeckEnemyCard7,
    &gRikuDeckEnemyCard9,
    &gRikuDeckEnemyCard9,
    gRikuDeckCardCounts,
    gRikuDeckCardCounts,
};

#include "deck_names.inc"
#ifdef VERSION_EU
void* gDeckButtonLabelTilesByLanguage[5] = { gDeckButtonLabelTiles, gDeckButtonLabelFrenchTiles, gDeckButtonLabelGermanTiles, gDeckButtonLabelItalianTiles, gDeckButtonLabelSpanishTiles };
void** gDeckButtonLabelSpritesByLanguage[5] = { gDeckButtonLabelFrames, gDeckButtonLabelFrenchFrames, gDeckButtonLabelGermanFrames, gDeckButtonLabelItalianFrames, gDeckButtonLabelSpanishFrames };
void* gDeckCommandMenuTilesByLanguage[5] = { gDeckCommandMenuTiles, gDeckCommandMenuFrenchTiles, gDeckCommandMenuGermanTiles, gDeckCommandMenuItalianTiles, gDeckCommandMenuSpanishTiles };

void** gDeckCommandMenuSpritesByLanguage[5] = {
    gDeckCommandMenuFrames,
    gDeckCommandMenuFrenchFrames,
    gDeckCommandMenuGermanFrames,
    gDeckCommandMenuItalianFrames,
    gDeckCommandMenuSpanishFrames,
};

void* gDeckTitleBannerTilesByLanguage[5] = { gDeckTitleBannerTiles, gDeckTitleBannerFrenchTiles, gDeckTitleBannerGermanTiles, gDeckTitleBannerItalianTiles, gDeckTitleBannerSpanishTiles };

void** gDeckTitleBannerSpritesByLanguage[5] = {
    gDeckTitleBannerFrames,
    gDeckTitleBannerFrenchFrames,
    gDeckTitleBannerGermanFrames,
    gDeckTitleBannerItalianFrames,
    gDeckTitleBannerSpanishFrames,
};

u8* gDeckEquipMarkerTilesByLanguage[5] = { gDeckEquipMarkerTiles, gDeckEquipMarkerFrenchTiles, gDeckEquipMarkerGermanTiles, gDeckEquipMarkerItalianTiles, gDeckEquipMarkerSpanishTiles };
#endif

TaskDesc gTaskDescDeckmenu2 = {
    "Deckmenu2",
    (TaskInitFunc)Deckmenu2_0,
    (TaskUpdateFunc)Deckmenu2_1,
    (TaskDrawFunc)Deckmenu2_2,
    (TaskDestroyFunc)DeckMenuDestroy,
    sizeof(DeckMenuWork),
};

#ifdef VERSION_EU
void* gDeckKeyboardCursorTilesByLanguage[5] = { gDeckKeyboardCursorTiles, gDeckKeyboardCursorTiles, gDeckKeyboardCursorGermanTiles, gDeckKeyboardCursorItalianTiles, gDeckKeyboardCursorTiles };
void** gDeckKeyboardCursorSpritesByLanguage[5] = { gDeckKeyboardCursorFrames, gDeckKeyboardCursorFrames, gDeckKeyboardCursorGermanFrames, gDeckKeyboardCursorItalianFrames, gDeckKeyboardCursorFrames };
void* gDeckKeyboardCursorAnimsByLanguage[5] = { gDeckKeyboardCursorAnims, gDeckKeyboardCursorAnims, gDeckKeyboardCursorGermanAnims, gDeckKeyboardCursorItalianAnims, gDeckKeyboardCursorAnims };
#endif

#ifdef VERSION_US
#include "deck_keyboard.inc"
const u8* gDeckKeyboardRows[7] = {
    (const u8*)gDeckKeyboardRow0,
    (const u8*)gDeckKeyboardRow1,
    (const u8*)gDeckKeyboardRow2,
    (const u8*)gDeckKeyboardRow3,
    (const u8*)gDeckKeyboardRow4,
    (const u8*)gDeckKeyboardRow5,
    (const u8*)gDeckKeyboardRow6,
};
#endif
#ifdef VERSION_JP
#include "deck_keyboard.inc"
const u8* gDeckKeyboardRows[7] = {
    gDeckKeyboardHiraganaRow0,
    gDeckKeyboardHiraganaRow1,
    gDeckKeyboardHiraganaRow2,
    gDeckKeyboardHiraganaRow3,
    gDeckKeyboardHiraganaRow4,
    gDeckKeyboardHiraganaRow5,
    gDeckKeyboardHiraganaRow6,
};

const u8* gDeckKeyboardKatakanaRows[7] = {
    gDeckKeyboardKatakanaRow0,
    gDeckKeyboardKatakanaRow1,
    gDeckKeyboardKatakanaRow2,
    gDeckKeyboardKatakanaRow3,
    gDeckKeyboardKatakanaRow4,
    gDeckKeyboardKatakanaRow5,
    gDeckKeyboardKatakanaRow6,
};

const u8* gDeckKeyboardAlphanumericRows[7] = {
    gDeckKeyboardAlphanumericRow0,
    gDeckKeyboardAlphanumericRow1,
    gDeckKeyboardAlphanumericRow2,
    gDeckKeyboardAlphanumericRow3,
    gDeckKeyboardAlphanumericRow4,
    gDeckKeyboardAlphanumericRow5,
    gDeckKeyboardAlphanumericRow6,
};
#endif
#ifdef VERSION_EU
#include "deck_keyboard.inc"
const u8* gDeckKeyboardLetterRows[8] = {
    gDeckKeyboardLetterRow0,
    gDeckKeyboardLetterRow1,
    gDeckKeyboardLetterRow2,
    gDeckKeyboardLetterRow3,
    gDeckKeyboardLetterRow4,
    gDeckKeyboardLetterRow5,
    gDeckKeyboardLetterRow6,
    gDeckKeyboardLetterRow7,
};

const u8* gDeckKeyboardSymbolRows[7] = {
    gDeckKeyboardSymbolRow0,
    gDeckKeyboardSymbolRow1,
    gDeckKeyboardSymbolRow2,
    gDeckKeyboardSymbolRow3,
    gDeckKeyboardSymbolRow4,
    gDeckKeyboardSymbolRow5,
    gDeckKeyboardSymbolRow6,
};
#endif

#ifdef VERSION_EU
const s16 gKeyboardKeyX[15] = {
    5, 19, 33, 47, 60, 83, 97, 111, 126, 140, 163, 177, 191, 205, 219,
};

const s16 gKeyboardKeyY[8] = {
    33, 47, 61, 76, 92, 110, 128, 143,
};

static const s16 sKeyboardSymbolKeyX[15] = {
    4, 18, 32, 46, 60, 83, 97, 109, 123, 137, 164, 177, 191, 205, 219,
};

static const s16 sKeyboardSymbolKeyY[7] = {
    40, 56, 70, 86, 102, 118, 134,
};

const KeyboardLineLayout gKeyboardRowLayouts[8] = {
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
    { gKeyboardKeyX, ARRAY_COUNT(gKeyboardKeyX) },
};

const KeyboardLineLayout gKeyboardColumnLayouts[15] = {
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
    { gKeyboardKeyY, ARRAY_COUNT(gKeyboardKeyY) },
};

const KeyboardLineLayout gKeyboardSymbolRowLayouts[8] = {
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
    { sKeyboardSymbolKeyX, ARRAY_COUNT(sKeyboardSymbolKeyX) },
};

const KeyboardLineLayout gKeyboardSymbolColumnLayouts[15] = {
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
    { sKeyboardSymbolKeyY, ARRAY_COUNT(sKeyboardSymbolKeyY) },
};

const s16 gKeyboardPageTabXEu[2] = { 1, 15 };
#else
const s16 gKeyboardKeyX[54] = {
    3, 16, 30, 45, 58, 83, 97, 110, 124, 138, 162, 177, 190, 205, 219,
    3, 16, 30, 45, 58, 83, 97, 124, 162, 177, 190, 205, 219, 3, 16,
    30, 45, 58, 3, 16, 30, 45, 58, 162, 177, 190, 205, 219, 3, 16,
    30, 45, 58, 83, 97, 110, 124, 138, 208,
};

const s16 gKeyboardKeyY[18] = {
    39, 55, 71, 87, 103, 119, 136, 39, 55, 71, 103, 136, 39, 55, 71,
    87, 103, 119,
};

const KeyboardLineLayout gKeyboardRowLayouts[7] = {
    { gKeyboardKeyX, 15 },
    { gKeyboardKeyX, 15 },
    { gKeyboardKeyX, 15 },
    { gKeyboardKeyX, 15 },
    { gKeyboardKeyX, 15 },
    { gKeyboardKeyX, 15 },
    { gKeyboardKeyX, 15 },
};

const KeyboardLineLayout gKeyboardColumnLayouts[16] = {
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
    { gKeyboardKeyY, 7 },
};
#endif

#ifdef VERSION_JP
const s16 gKeyboardPageTabXJp[3] = { 1, 15, 29 };
#endif
