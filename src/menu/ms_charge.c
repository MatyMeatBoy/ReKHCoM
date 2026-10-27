/**
 * ms_charge.c
 * Moogle Card Trade-In Screen
 */

#include "system_state.h"
#include "ms_charge_api.h"
#include "display.h"
#include "text.h"
#include "monsgage.h"
#include "ms_charge.h"
#include "gba/keys.h"
#include "sprites_card.h"
#include "sprites_evt.h"
#include "sprites_moogle_shop.h"
#include "sprites_card_pictures.h"
#include "mode_ms_top_api.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include "jiminy_data.h"
#include "common_text.h"
#include "anim.h"
#include "card.h"
#include "card_def_data.h"
#include "card_types.h"
#include "engine_math.h"
#include "game_state.h"
#include "key.h"
#include "m4a_song.h"
#include "map_api.h"
#include "mode.h"
#include "obj.h"
#include "obj_api.h"
#include "registration_data.h"
#include "text_types.h"
#include "types.h"
#include "gba/defines.h"
#include "gba/macro.h"
#include <stddef.h>
#include "card_map_anim.h"
#include "card_deckmenu2.h"
#include "sprite_palettes.h"
#include "card_ids.h"

#ifdef VERSION_EU
static void* sMsChargeBgMapsByLanguage[5] = {
    gMsChargeBgMap,
    gMsChargeBgFrenchMap,
    gMsChargeBgGermanMap,
    gMsChargeBgItalianMap,
    gMsChargeBgSpanishMap,
};

static void* sMsChargeTabTilemapsByLanguage[5] = {
    gMsChargeTabMap,
    gMsChargeTabFrenchMap,
    gMsChargeTabGermanSpanishMap,
    gMsChargeTabItalianMap,
    gMsChargeTabGermanSpanishMap,
};

static void* sMsChargeValueCountBgMapsByLanguage[5] = {
    gMsChargeValueCountBgMap,
    gMsChargeValueCountBgFrenchMap,
    gMsChargeValueCountBgGermanMap,
    gMsChargeValueCountBgItalianMap,
    gMsChargeValueCountBgSpanishMap,
};

static void* sMsChargeEnemyCountBgMapsByLanguage[5] = {
    gMsChargeEnemyCountBgMap,
    gMsChargeEnemyCountBgFrenchMap,
    gMsChargeEnemyCountBgGermanMap,
    gMsChargeEnemyCountBgItalianMap,
    gMsChargeEnemyCountBgSpanishMap,
};
#endif

enum MsChargeState {
    MS_CHARGE_STATE_FADE_IN,
    MS_CHARGE_STATE_MENU,
    MS_CHARGE_STATE_EXIT
};

enum MsChargeMenuState {
    MS_CHARGE_MENU_STATE_GRID,
    MS_CHARGE_MENU_STATE_TAB,
    MS_CHARGE_MENU_STATE_VALUE,
    MS_CHARGE_MENU_STATE_CONFIRM,
    MS_CHARGE_MENU_STATE_NOTICE
};

static MsCard* sMsCards;
static s16 sMsChargeState;
static s16 sMsChargeMenuState;
static s16 sMsChargeTab;
static s16 sMsChargeCategoryStart[4];
static s16 sMsChargeCategoryEntryCount[4];
static s16 sMsChargeCategoryCardCount[4];
static s16 sMsChargeDeckCardCount;
static s16 sMsChargeCardTotal;
static u16 sMsChargeCollectionCount;
static struct ObjTiles* sMsChargeScrollbarTiles;
static struct ObjPalette* sMsChargeCategoryPalette;
static struct ObjTiles* sMsChargeHighlightTiles;
static AnimState sMsChargeArrowAnim;
static AnimState sMsChargeHighlightAnim;
static struct ObjPalette* sMsChargeMooglePalette;
static struct ObjTiles* sMsChargeMoogleTiles;
static AnimState sMsChargeCursorAnim;
static AnimState sMsChargeMoogleAnim;
static u8 sMsChargeBobPhase;
static s16 sMsChargeMoogleAnimId;
static s16 sMsChargeMoogleAnimTimer;
static struct ObjPalette* sMsChargeConfirmCursorPalette;
static struct ObjTiles* sMsChargeConfirmCursorTiles;
static AnimState sMsChargeConfirmCursorAnim;
static s16 sMsChargeGridCol;
static s16 sMsChargeGridRow;
static s16 sMsChargeGridScroll;
static void* sMsChargeGridPalettes[4][3];
static void* sMsChargeGridTiles[4][3];
static void* sMsChargeGridSprites[4][3];
static u8 sMsChargeGridPremium[4][3];
static struct ObjPalette* sMsChargeCardPalette;
static struct ObjTiles* sMsChargeCardTiles;
static void* sMsChargeCardSprite;
static struct ObjPalette* sMsChargeCard00Palette;
static struct ObjTiles* sMsChargeCardBackTiles;
static void* sMsChargeCardBackSprite;
static struct ObjTiles* sMsChargePremiumTiles;
static AnimState sMsChargePremiumAnim;
static struct ObjTiles* sMsChargeGridPremiumTiles;
static AnimState sMsChargeGridPremiumAnim;
static u8 sMsChargeCardPremium;
static TextSlot* sMsChargeNameText;
static u8 sMsChargeNameTextCount;
static TextSlot* sMsChargeDescText;
static u8 sMsChargeDescTextCount;
static TextSlot* sMsChargeConfirmText;
static u8 sMsChargeConfirmTextCount;
static u16 sMsChargeConfirmTextLength;
static TextSlot* sMsChargeNoticeText;
static u8 sMsChargeNoticeTextCount;
static u16 sMsChargeNoticeTextLength;
static TextSlot* sMsChargeYesText;
static u8 sMsChargeYesTextCount;
static u16 sMsChargeYesTextLength;
static TextSlot* sMsChargeNoText;
static u8 sMsChargeNoTextCount;
static u16 sMsChargeNoTextLength;
static s16 sMsChargeValueCol;
static s16 sMsChargeValueRow;
static s16 sMsChargeConfirmCursor;
static u32 sMsChargeConfirmCursorX;
static s32 sMsChargeCursorX;
static s32 sMsChargeCursorY;
static u8 sMsChargeBackToTop;

s16 GetMsChargeTabStart(s16 tab) {
    s16 start;

    if (tab <= 3) {
        start = sMsChargeCategoryStart[tab];
    } else {
        start = 0;
    }

    return start;
}

s16 GetMsChargeTabCount(s16 tab) {
    s16 count;
    s16 i;

    if (tab <= 3) {
        count = sMsChargeCategoryEntryCount[tab];
    } else {
        count = 0;

        for (i = 0; i < 4; i++) {
            count += sMsChargeCategoryEntryCount[i];
        }
    }

    return count;
}

s16 GetMsChargeSelectedIndex() {
    return GetMsChargeTabStart(sMsChargeTab) + (sMsChargeGridScroll + sMsChargeGridRow) * 3 + sMsChargeGridCol;
}

MsCard* GetMsChargeSelectedCard() {
    return sMsCards + GetMsChargeSelectedIndex();
}

void MsChargeSelectFirstValue() {
    MsCard* card;
    s16 i;

    card = GetMsChargeSelectedCard();

    if (card->kind != 0x8F) {
        for (i = 0; i < 10; i++) {
            if (card->values[i][0] > 0) {
                break;
            }
        }

        sMsChargeValueCol = i / 5;
        sMsChargeValueRow = i % 5;
    } else {
        sMsChargeValueCol = 0;
        sMsChargeValueRow = 0;
    }
}

void MsChargeLoadGrid() {
    s16 i;
    s16 k;
    s16 idx;
    s16 start;
    s16 limit;
    s32 defIdx;

    start = GetMsChargeTabStart(sMsChargeTab);
    limit = GetMsChargeTabCount(sMsChargeTab);
    idx = sMsChargeGridScroll * 3;

    for (i = 0; i < 4; i++) {
        for (k = 0; k < 3; k++) {
            if (sMsChargeGridPalettes[i][k] != NULL) {
                ReleaseObjPalette(sMsChargeGridPalettes[i][k]);
            }

            if (sMsChargeGridTiles[i][k] != NULL) {
                ReleaseObjTiles(sMsChargeGridTiles[i][k]);
            }

            if (idx < limit) {
                defIdx = sMsCards[start + idx].cardId;
                sMsChargeGridPalettes[i][k] = LoadObjPalette(gCardDefs[defIdx].palette2, 0x20);
                sMsChargeGridTiles[i][k] = LoadObjTiles(gCardDefs[defIdx].tiles2, 0x100);
                sMsChargeGridSprites[i][k] = gCardDefs[defIdx].gfx2;
                sMsChargeGridPremium[i][k] = sMsCards[start + idx].premium;
            } else {
                sMsChargeGridPalettes[i][k] = NULL;
                sMsChargeGridTiles[i][k] = NULL;
                sMsChargeGridSprites[i][k] = NULL;
                sMsChargeGridPremium[i][k] = 0;
            }

            idx++;
        }
    }
}

void MsChargeLoadSelectedCard() {
    MsCard* card;
    u8* nameCountPtr;
    u8* descCountPtr;
    s32 defIdx;

    card = GetMsChargeSelectedCard();

    if (sMsChargeCardPalette != NULL) {
        ReleaseObjPalette(sMsChargeCardPalette);
    }

    if (sMsChargeCardTiles != NULL) {
        ReleaseObjTiles(sMsChargeCardTiles);
    }

    if (sMsChargeCardBackTiles != NULL) {
        ReleaseObjTiles(sMsChargeCardBackTiles);
    }

    if (card->kind != 0x8F && GetMsChargeTabCount(sMsChargeTab) > 0) {
        defIdx = card->cardId;
        sMsChargeCardPalette = LoadObjPalette(gCardDefs[defIdx].palette, 0x20);
        sMsChargeCardTiles = LoadObjTiles(gCardDefs[defIdx].tiles, 0x200);
        sMsChargeCardSprite = gCardDefs[defIdx].gfx;
        sMsChargeCardBackTiles = LoadObjTiles(gCardBacks[card->category].tiles, 0x300);
        sMsChargeCardBackSprite = gCardBacks[card->category].gfx;
        sMsChargeCardPremium = card->premium;
        nameCountPtr = &sMsChargeNameTextCount;
#ifdef VERSION_EU
        *nameCountPtr = LoadTextSlots(GetLocalizedString(gCardDefs[defIdx].name), sMsChargeNameText);
#else
        *nameCountPtr = LoadTextSlots(gCardDefs[defIdx].name, sMsChargeNameText);
#endif
        descCountPtr = &sMsChargeDescTextCount;
        *descCountPtr = LoadTextSlots((void*)LANGSTR(gCardKindDescriptions[card->kind]), sMsChargeDescText);
        LoadObjPaletteBank(sMsChargeCategoryPalette->index, gMsChargeCategoryPalettes + card->category * 0x10);
    } else {
        sMsChargeCardPalette = NULL;
        sMsChargeCardTiles = NULL;
        sMsChargeCardSprite = NULL;
        sMsChargeCardBackTiles = NULL;
        sMsChargeCardBackSprite = NULL;
        sMsChargeCardPremium = 0;
        sMsChargeNameTextCount = 0;
        sMsChargeDescTextCount = 0;
    }
}

s16 GetMsChargeValueIndex(s16 col, s16 row) {
    return row + col * 5;
}

s16 GetMsChargeSelectedValue() {
    return GetMsChargeValueIndex(sMsChargeValueCol, sMsChargeValueRow);
}

u16 GetMsChargeCardPoints(u16 index) {
    MsCard* card;
    u16 id;

    card = &sMsCards[index];

    if (card->kind != 0x8F) {
        id = card->values[GetMsChargeSelectedValue()][1];
        return GetCardMooglePointValue(card->premium != 0 ? id | CARD_FLAG_PREMIUM : id);
    }

    return 0;
}

void MsChargeDrawPoints() {
    u32 points;

    points = GetMooglePoints();
    LoadDecimalDigitTiles(points, gMsChargeDigitTiles, (u8*)GetBgCharBase(0) + 0x20, 0x20, 5);

    if (sMsChargeMenuState >= MS_CHARGE_MENU_STATE_VALUE && sMsChargeMenuState <= MS_CHARGE_MENU_STATE_CONFIRM) {
        points = GetMsChargeCardPoints(GetMsChargeSelectedIndex());
    } else {
        points = 0;
    }

    LoadDecimalDigitTiles(points, gMsChargeCardPointsDigitTiles, (u8*)GetBgCharBase(0) + 0x180, 0x20, 2);
}

void MsChargeDrawCardCounts() {
    LoadDecimalDigitTiles(sMsChargeDeckCardCount, gMsChargeDigitTiles, (u8*)GetBgCharBase(0) + 0xC0, 0x20, 3);
    LoadDecimalDigitTiles(sMsChargeCollectionCount, gMsChargeDigitTiles, (u8*)GetBgCharBase(0) + 0x120, 0x20, 3);
}

void MsChargeDrawCategoryCounts() {
    s16 i;
    s32 count;

    for (i = 0; i < 4; i++) {
        count = sMsChargeCategoryCardCount[i];

        if (count != 0) {
            LoadDecimalDigitTiles(count, gMsChargeCategoryCountDigitTiles, (u8*)GetBgCharBase(0) + (i * 3 * 0x20 + 0x1C0), 0x20, 3);
        } else {
            LoadDecimalDigitTiles(0, gMsChargeCategoryCountZeroTiles, (u8*)GetBgCharBase(0) + (i * 3 * 0x20 + 0x1C0), 0x20, 3);
        }
    }
}

void MsChargeDrawValueCounts() {
    MsCard* card;
    s16 i;
    s32 count;

    card = GetMsChargeSelectedCard();

    if (GetMsChargeTabCount(sMsChargeTab) > 0) {
        LoadPalette(gMsChargeCategoryBgPalettes + card->category * 0x10, (void*)(BG_PLTT + 7 * PLTT_SIZE_4BPP), 0x0C);
    }

    if (sMsChargeMenuState == MS_CHARGE_MENU_STATE_TAB) {
        for (i = 0; i < 10; i++) {
            LoadDecimalDigitTiles(0, gMsChargeValueCountZeroTiles, (u8*)GetBgCharBase(0) + (i * 0x40 + 0x340), 0x20, 2);
            LoadPalette(gMsChargeValueEmptyPalette, (void*)(BG_PLTT + 7 * PLTT_SIZE_4BPP + 0xC + i * 2), sizeof(gMsChargeValueEmptyPalette[0]));
        }
    } else if (card->category == CARD_CATEGORY_ENEMY) {
        if (GetMsChargeTabCount(sMsChargeTab) > 0) {
#ifdef VERSION_EU
            LoadBgMap(1, sMsChargeEnemyCountBgMapsByLanguage[gLanguage], 0x500);
#else
            LoadBgMap(1, gMsChargeEnemyCountBgMap, sizeof(gMsChargeEnemyCountBgMap));
#endif
        }

        for (i = 0; i < 10; i++) {
            count = card->values[i][0];

            if (count != 0 && GetMsChargeTabCount(sMsChargeTab) > 0) {
                LoadDecimalDigitTiles(count, gMsChargeValueCountDigitTiles, (u8*)GetBgCharBase(0) + 0x340, 0x20, 2);
                LoadPalette(gMsChargeValueOwnedPalette, (void*)(BG_PLTT + 7 * PLTT_SIZE_4BPP + 0xC), sizeof(gMsChargeValueOwnedPalette[0]));
                break;
            }
        }

        if (i > 9) {
            LoadDecimalDigitTiles(0, gMsChargeValueCountZeroTiles, (u8*)GetBgCharBase(0) + 0x340, 0x20, 2);
            LoadPalette(gMsChargeValueEmptyPalette, (void*)(BG_PLTT + 7 * PLTT_SIZE_4BPP + 0xC), sizeof(gMsChargeValueEmptyPalette[0]));
        }
    } else {
        if (GetMsChargeTabCount(sMsChargeTab) > 0) {
#ifdef VERSION_EU
            LoadBgMap(1, sMsChargeValueCountBgMapsByLanguage[gLanguage], 0x500);
#else
            LoadBgMap(1, gMsChargeValueCountBgMap, sizeof(gMsChargeValueCountBgMap));
#endif
        }

        for (i = 0; i < 10; i++) {
            count = card->values[i][0];

            if (count != 0 && GetMsChargeTabCount(sMsChargeTab) > 0) {
                LoadDecimalDigitTiles(count, gMsChargeValueCountDigitTiles, (u8*)GetBgCharBase(0) + (i * 0x40 + 0x340), 0x20, 2);
                LoadPalette(gMsChargeValueOwnedPalette, (void*)(BG_PLTT + 7 * PLTT_SIZE_4BPP + 0xC + i * 2), sizeof(gMsChargeValueOwnedPalette[0]));
            } else {
                LoadDecimalDigitTiles(0, gMsChargeValueCountZeroTiles, (u8*)GetBgCharBase(0) + (i * 0x40 + 0x340), 0x20, 2);
                LoadPalette(gMsChargeValueEmptyPalette, (void*)(BG_PLTT + 7 * PLTT_SIZE_4BPP + 0xC + i * 2), sizeof(gMsChargeValueEmptyPalette[0]));
            }
        }
    }
}

void MsChargeDrawTab(s16 tab) {
    s16 cell;
    void* base;

    cell = 4 - tab;
    base = GetBgScreenBase(0);
#ifdef VERSION_EU
    RequestTilemapRectCopy(sMsChargeTabTilemapsByLanguage[gLanguage], base, cell % 3 * 10, cell / 3 * 2, 20, 2, 10, 2);
#else
    RequestTilemapRectCopy(gMsChargeTabMap, base, cell % 3 * 10, cell / 3 * 2, 20, 2, 10, 2);
#endif
}

void MsChargeSellCard() {
    MsCard* card;
    u16 id;
    s16 i;

    card = GetMsChargeSelectedCard();

    if (card->values[GetMsChargeSelectedValue()][0] > 0) {
        id = card->values[GetMsChargeSelectedValue()][1];

        if (card->premium != 0) {
            id |= CARD_FLAG_PREMIUM;
        }

        for (i = 0; i < gCardCount; i++) {
            if (gCardCollection[i] == id) {
                AddMooglePoints(GetMsChargeCardPoints(GetMsChargeSelectedIndex()));
                sMsChargeCategoryCardCount[card->category]--;
                sMsChargeCardTotal--;
                sMsChargeCollectionCount--;
                card->values[GetMsChargeSelectedValue()][0]--;
                ClearCardCollectionSlot(&gCardCollection[i]);
                break;
            }
        }
    }
}

u8 MsCardIsEmpty(MsCard* card) {
    s16 i;

    for (i = 0; i < 10; i++) {
        if (card->values[i][0] > 0) {
            break;
        }
    }

    if (i > 9) {
        return TRUE;
    }

    return FALSE;
}

u8 MsCardSelectedValueIsEmpty(MsCard* card) {
    if (card->values[GetMsChargeSelectedValue()][0] == 0) {
        return TRUE;
    }

    return FALSE;
}

void MsChargeSelectNextValue(MsCard* card) {
    s16 value;
    s16 n;

    value = GetMsChargeSelectedValue();

    for (n = 0; n < 10; n++) {
        if (card->values[value][0] > 0) {
            break;
        }

        value++;

        if (value > 9) {
            value = 0;
        }
    }

    sMsChargeValueCol = value / 5;
    sMsChargeValueRow = value % 5;
}

void MsChargeRemoveCard(MsCard* card) {
    u8 category;
    s16 j;

    category = gCardDefs[card->cardId].category;
    DmaCopy16(3, card + 1, card, (285 - GetMsChargeSelectedIndex()) * sizeof(MsCard));
    DmaFill16(3, 0, &sMsCards[285], sizeof(MsCard));
    sMsCards[285].kind = 0x8F;

    for (j = category + 1; j <= 3; j++) {
        sMsChargeCategoryStart[j]--;
    }

    sMsChargeCategoryEntryCount[category]--;

    if (GetMsChargeSelectedIndex() >= GetMsChargeTabCount(sMsChargeTab)) {
        sMsChargeGridCol--;

        if (sMsChargeGridCol < 0) {
            sMsChargeGridCol = 2;
            sMsChargeGridRow--;

            if (sMsChargeGridRow < 0) {
                sMsChargeGridRow = 0;
                sMsChargeGridScroll--;

                if (sMsChargeGridScroll < 0) {
                    sMsChargeGridCol = 0;
                    sMsChargeGridScroll = 0;
                }
            }
        }
    }

    sMsChargeValueCol = 0;
    sMsChargeValueRow = 0;
}

s32 FindMsCard(u16 kind, u8 premium, s16 count) {
    s16 i;

    for (i = 0; i < count; i++) {
        if (sMsCards[i].kind == kind && sMsCards[i].premium == premium) {
            return i;
        }
    }

    return -1;
}

void MsChargeBuildCardList() {
    MsCard card;
    s16* deckCountPtr;
    s16 collectionCount;
    s16 n;
    u16 id;
    s16 i;
    u16 raw;
    u16 kind;
    u16 flags;
    u32 sortKey;
    u8 premium;
    s16 j;
    s16 idx;

    DmaFill16(3, 0, sMsCards, 0x3A18);

    for (n = 0; n <= 285; n++) {
        sMsCards[n].kind = 0x8F;
        sMsCards[n].cardId = 0x3B6;
        sMsCards[n].premium = 0;
    }

    collectionCount = CountCollectionCards();
    sMsChargeCollectionCount = collectionCount;
    deckCountPtr = &sMsChargeDeckCardCount;
    *deckCountPtr = CountCardsInDecks();

    for (j = 0; j < 4; j++) {
        sMsChargeCategoryStart[j] = 0;
        sMsChargeCategoryCardCount[j] = 0;
        sMsChargeCategoryEntryCount[j] = 0;
    }

    sMsChargeCardTotal = 0;
    n = 0;

    for (i = 0; i < gCardCount; i++) {
        raw = gCardCollection[i];
        id = raw & CARD_ID_MASK;

        if (raw != CARD_COLLECTION_EMPTY && (raw & CARD_FLAG_IN_ANY_DECK) == 0 && id <= CARD_CRESCENDO_6) {
            j = gCardDefs[id].category;
            kind = gCardDefs[id].kind;
            flags = raw & CARD_FLAG_PREMIUM;
            premium = flags != 0;
            sMsChargeCategoryCardCount[j]++;
            sMsChargeCardTotal++;

            if ((idx = FindMsCard(kind, premium, n)) >= 0) {
                sMsCards[idx].values[raw = gCardDefs[id].value][0]++;
                sMsCards[idx].values[raw][1] = id;
            } else {
                sMsCards[n].kind = kind;
                sMsCards[n].cardId = gCardDefs[id].catalogNumber;
                sMsCards[n].category = j;
                raw = gCardDefs[id].value;
                sMsCards[n].values[raw][0]++;
                sMsCards[n].values[raw][1] = id;
                sMsCards[n].premium = premium;
                sortKey = 0x01000000;

                if (premium != 0) {
                    sortKey = 0x03000000;
                }

                sMsCards[n].sortKey = (sortKey << (sMsCards[n].category * 2)) | sMsCards[n].cardId;
                sMsChargeCategoryEntryCount[j]++;
                n++;
            }
        }
    }

    for (j = 1; j < 4; j++) {
        sMsChargeCategoryStart[j] = sMsChargeCategoryStart[j - 1] + sMsChargeCategoryEntryCount[j - 1];
    }

    for (i = 1; i < n; i++) {
        card = sMsCards[i];

        for (j = i - 1; j >= 0; j--) {
            if (sMsCards[j].sortKey > card.sortKey) {
                sMsCards[j + 1] = sMsCards[j];
            } else {
                break;
            }
        }

        sMsCards[j + 1] = card;
    }
}

s32 MsChargeReadMenuKeys() {
    s32 keys;

    keys = GetKeysPressed() & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON);
    return keys | (GetKeysRepeat() & (DPAD_ANY | L_BUTTON | R_BUTTON));
}

void MsChargeHandleGridInput() {
    s16 oldCol;
    s16 oldRow;
    s16 oldScroll;
    u16 keys;

    oldCol = sMsChargeGridCol;
    oldRow = sMsChargeGridRow;
    oldScroll = sMsChargeGridScroll;
    keys = MsChargeReadMenuKeys();

    if (keys & A_BUTTON) {
        MsChargeSelectFirstValue();
        m4aSongNumStart(SONG_SYS_KETTEI);
        AnimStart(&sMsChargeHighlightAnim, 0, ANIM_FLAG_LOOP);
        sMsChargeMoogleAnimId = 1;
        sMsChargeMenuState = MS_CHARGE_MENU_STATE_VALUE;
        MsChargeDrawPoints();
    } else if (keys & B_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsChargeBackToTop = TRUE;
        FadeStartOut(FADE_MODE_BLACK, 0x10);
        sMsChargeState = MS_CHARGE_STATE_EXIT;
    } else if (keys & START_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsChargeBackToTop = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 0x10);
        sMsChargeState = MS_CHARGE_STATE_EXIT;
    } else if (keys & SELECT_BUTTON) {
        sMsChargeGridCol = 0;
        sMsChargeGridRow = 0;
        sMsChargeGridScroll = 0;
        sMsChargeMoogleAnimId = 0;
        m4aSongNumStart(SONG_SYS_CLICKI04B);
        sMsChargeMenuState = MS_CHARGE_MENU_STATE_TAB;
        MsChargeDrawValueCounts();
    } else if (keys & DPAD_UP) {
        if (sMsChargeGridRow > 0) {
            sMsChargeGridRow--;
        } else if (sMsChargeGridScroll > 0) {
            sMsChargeGridScroll--;
        } else {
            sMsChargeMoogleAnimId = 0;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
            sMsChargeMenuState = MS_CHARGE_MENU_STATE_TAB;
            MsChargeDrawValueCounts();
        }
    } else if (keys & DPAD_DOWN) {
        if ((sMsChargeGridScroll + sMsChargeGridRow + 1) * 3 + sMsChargeGridCol < GetMsChargeTabCount(sMsChargeTab)) {
            if (sMsChargeGridRow <= 2) {
                sMsChargeGridRow++;
            } else {
                sMsChargeGridScroll++;
            }
        } else if (sMsChargeGridRow == 3) {
            if ((sMsChargeGridScroll + sMsChargeGridRow + 1) * 3 < GetMsChargeTabCount(sMsChargeTab)) {
                sMsChargeGridCol = (GetMsChargeTabCount(sMsChargeTab) - 1) % 3;
                sMsChargeGridScroll++;
            }
        }
    } else if (keys & DPAD_LEFT) {
        if (sMsChargeGridCol > 0) {
            sMsChargeGridCol--;
        }
    } else if (keys & DPAD_RIGHT) {
        if ((sMsChargeGridScroll + sMsChargeGridRow) * 3 + sMsChargeGridCol + 1 < GetMsChargeTabCount(sMsChargeTab)) {
            if (sMsChargeGridCol <= 1) {
                sMsChargeGridCol++;
            }
        }
    }

    if (sMsChargeGridCol != oldCol || sMsChargeGridRow != oldRow || sMsChargeGridScroll != oldScroll) {
        MsChargeSelectFirstValue();
        MsChargeDrawValueCounts();
        MsChargeDrawPoints();
        MsChargeLoadSelectedCard();
        m4aSongNumStart(SONG_SYS_CLICKI04B);

        if (sMsChargeGridScroll != oldScroll) {
            MsChargeLoadGrid();
        }
    }
}

void MsChargeHandleTabInput() {
    s16 old;
    u16 keys;

    old = sMsChargeTab;
    keys = MsChargeReadMenuKeys();

    if ((keys & A_BUTTON) == 0) {
        if (keys & START_BUTTON) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            sMsChargeBackToTop = FALSE;
            FadeStartOut(FADE_MODE_BLACK, 0x10);
            sMsChargeState = MS_CHARGE_STATE_EXIT;
        } else if (keys & (B_BUTTON | DPAD_DOWN)) {
            if (GetMsChargeTabCount(sMsChargeTab) > 0) {
                sMsChargeGridCol = 0;
                sMsChargeGridRow = 0;
                sMsChargeGridScroll = 0;
                AnimStart(&sMsChargeHighlightAnim, 2, ANIM_FLAG_LOOP);
                sMsChargeMoogleAnimId = 0;
                m4aSongNumStart(SONG_SYS_CLICKI04B);
                sMsChargeMenuState = MS_CHARGE_MENU_STATE_GRID;
                MsChargeSelectFirstValue();
                MsChargeDrawValueCounts();
                MsChargeLoadSelectedCard();
            } else if (keys & B_BUTTON) {
                m4aSongNumStart(SONG_SYS_CLOSE);
                sMsChargeBackToTop = TRUE;
                FadeStartOut(FADE_MODE_BLACK, 0x10);
                sMsChargeState = MS_CHARGE_STATE_EXIT;
            } else if (keys & DPAD_DOWN) {
                m4aSongNumStart(SONG_SYS_BEEP);
            }
        } else if (keys & DPAD_LEFT) {
            if (sMsChargeTab > 0) {
                sMsChargeTab--;
            }
        } else if (keys & DPAD_RIGHT) {
            if (sMsChargeTab <= 3) {
                sMsChargeTab++;
            }
        }
    }

    if (sMsChargeTab != old) {
        MsChargeDrawTab(sMsChargeTab);
        MsChargeLoadGrid();
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

s32 MsChargeSelectValueInColumn(MsCard* card, u16 col) {
    s16 base;
    s16 i;
    s32 row;
    s32 found;

    found = FALSE;
    base = sMsChargeValueRow;

    for (i = 0; i <= 4; i++) {
        row = base - i;

        if (row >= 0 && card->values[GetMsChargeValueIndex(col, row)][0] > 0) {
            sMsChargeValueCol = col;
            sMsChargeValueRow = row;
            found = TRUE;
            break;
        }

        row = base + i;

        if (row <= 4 && card->values[GetMsChargeValueIndex(col, row)][0] > 0) {
            sMsChargeValueCol = col;
            sMsChargeValueRow = row;
            found = TRUE;
            break;
        }
    }

    return found;
}

void MsChargeHandleValueInput() {
    MsCard* card;
    s16 oldCol;
    s16 oldRow;
    u16 keys;
    s16 i;

    card = GetMsChargeSelectedCard();
    oldCol = sMsChargeValueCol;
    oldRow = sMsChargeValueRow;
    keys = MsChargeReadMenuKeys();

    if (keys & A_BUTTON) {
        if (GetMooglePoints() + GetMsChargeCardPoints(GetMsChargeSelectedIndex()) > 99999) {
            m4aSongNumStart(SONG_SYS_BEEP);
        } else if (sMsChargeCollectionCount <= 1) {
            EnableBg(2);
            m4aSongNumStart(SONG_SYS_CANSEL);
            sMsChargeMenuState = MS_CHARGE_MENU_STATE_NOTICE;
        } else {
            sMsChargeConfirmCursor = 0;
            sMsChargeConfirmCursorX = 0x4800;
            AnimStart(&sMsChargeConfirmCursorAnim, 0, ANIM_FLAG_LOOP);
            EnableBg(2);
            m4aSongNumStart(SONG_SYS_CANSEL);
            sMsChargeMenuState = MS_CHARGE_MENU_STATE_CONFIRM;
        }
    } else if (keys & B_BUTTON) {
        AnimStart(&sMsChargeHighlightAnim, 2, ANIM_FLAG_LOOP);
        sMsChargeMoogleAnimId = 0;
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsChargeMenuState = MS_CHARGE_MENU_STATE_GRID;
        MsChargeDrawPoints();
    } else if (keys & START_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsChargeBackToTop = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 0x10);
        sMsChargeState = MS_CHARGE_STATE_EXIT;
    } else if (keys & DPAD_LEFT) {
        MsChargeSelectValueInColumn(card, 0);
    } else if (keys & DPAD_RIGHT) {
        MsChargeSelectValueInColumn(card, 1);
    } else if (keys & DPAD_UP) {
        for (i = 0; i <= 4; i++) {
            sMsChargeValueRow--;

            if (sMsChargeValueRow < 0) {
                sMsChargeValueRow = 4;
            }

            if (card->values[GetMsChargeSelectedValue()][0] > 0) {
                break;
            }
        }
    } else if (keys & DPAD_DOWN) {
        for (i = 0; i <= 4; i++) {
            sMsChargeValueRow++;

            if (sMsChargeValueRow > 4) {
                sMsChargeValueRow = 0;
            }

            if (card->values[GetMsChargeSelectedValue()][0] > 0) {
                break;
            }
        }
    }

    if (card->category == CARD_CATEGORY_ENEMY) {
        sMsChargeValueCol = oldCol;
        sMsChargeValueRow = oldRow;
    }

    if (sMsChargeValueCol != oldCol || sMsChargeValueRow != oldRow) {
        MsChargeDrawPoints();
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void MsChargeHandleConfirmInput() {
    MsCard* card;
    s16 old;
    u16 keys;

    card = GetMsChargeSelectedCard();
    old = sMsChargeConfirmCursor;
    keys = MsChargeReadMenuKeys();

    if (keys & A_BUTTON) {
        DisableBg(2);

        if (sMsChargeConfirmCursor == 0) {
            sMsChargeMoogleAnimTimer = 180;
            MsChargeSellCard();
            m4aSongNumStart(SONG_SYS_KAIHUKU);

            if (MsCardIsEmpty(card)) {
                MsChargeRemoveCard(card);
                MsChargeLoadGrid();
                MsChargeLoadSelectedCard();

                if (GetMsChargeTabCount(sMsChargeTab) > 0) {
                    AnimStart(&sMsChargeHighlightAnim, 2, ANIM_FLAG_LOOP);
                    sMsChargeMoogleAnimId = 0;
                    sMsChargeMenuState = MS_CHARGE_MENU_STATE_GRID;
                } else {
                    sMsChargeMoogleAnimId = 0;
                    sMsChargeMenuState = MS_CHARGE_MENU_STATE_TAB;
                }
            } else {
                if (MsCardSelectedValueIsEmpty(card)) {
                    MsChargeSelectNextValue(card);
                }

                AnimStart(&sMsChargeHighlightAnim, 0, ANIM_FLAG_LOOP);
                sMsChargeMoogleAnimId = 1;
                sMsChargeMenuState = MS_CHARGE_MENU_STATE_VALUE;
            }

            MsChargeDrawPoints();
            MsChargeDrawValueCounts();
            MsChargeDrawCardCounts();
            MsChargeDrawCategoryCounts();
        } else {
            m4aSongNumStart(SONG_SYS_CLOSE);
            AnimStart(&sMsChargeHighlightAnim, 0, ANIM_FLAG_LOOP);
            sMsChargeMoogleAnimId = 1;
            sMsChargeMenuState = MS_CHARGE_MENU_STATE_VALUE;
        }
    } else if (keys & B_BUTTON) {
        DisableBg(2);
        m4aSongNumStart(SONG_SYS_CLOSE);
        AnimStart(&sMsChargeHighlightAnim, 0, ANIM_FLAG_LOOP);
        sMsChargeMoogleAnimId = 1;
        sMsChargeMenuState = MS_CHARGE_MENU_STATE_VALUE;
    } else if (keys & START_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsChargeBackToTop = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 0x10);
        sMsChargeState = MS_CHARGE_STATE_EXIT;
    } else if (keys & DPAD_LEFT) {
        sMsChargeConfirmCursor = 0;
    } else if (keys & DPAD_RIGHT) {
        sMsChargeConfirmCursor = 1;
    }

    if (sMsChargeConfirmCursor != old) {
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void MsChargeHandleNoticeInput() {
    u16 keys;

    keys = MsChargeReadMenuKeys();

    if (keys & (A_BUTTON | B_BUTTON)) {
        DisableBg(2);
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsChargeMenuState = MS_CHARGE_MENU_STATE_VALUE;
    } else if (keys & START_BUTTON) {
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsChargeBackToTop = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 0x10);
        sMsChargeState = MS_CHARGE_STATE_EXIT;
    }
}

void MsChargeDraw() {
    MsCard* card;
    void* anim;
    s32 sine;
    s16 maxScroll;
    s16 barOffset;
    s16 col;
    s16 row;
    s32 i;
    s32 j;

    if (sMsChargeMenuState != MS_CHARGE_MENU_STATE_TAB) {
        DrawSprite(16, 60, AnimUpdate(&sMsChargeArrowAnim), sMsChargeHighlightTiles, sMsChargeCategoryPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);
    }

    maxScroll = (GetMsChargeTabCount(sMsChargeTab) + 2) / 3 - 4;

    if (sMsChargeGridScroll <= maxScroll
#ifdef PLATFORM_PC
        /* Ten to twelve entries fill one page, with no scroll range. */
        && maxScroll > 0 && sMsChargeGridScroll >= 0
#endif
    ) {
        barOffset = 84 * sMsChargeGridScroll / maxScroll;
    } else {
        barOffset = 0;
    }

    DrawSprite(160, barOffset + 40, gMsChargeScrollbarFrame0, sMsChargeScrollbarTiles, sMsChargeCard00Palette, NULL, SPRITE_PRIORITY(2), 0x898);
    DrawSprite(24, 58, AnimUpdate(&sMsChargeMoogleAnim), sMsChargeMoogleTiles, sMsChargeMooglePalette, NULL, SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP, 0x834);

    if (sMsChargeState == MS_CHARGE_STATE_MENU) {
        switch (sMsChargeMenuState) {
        case MS_CHARGE_MENU_STATE_TAB:
            sMsChargeCursorX = sMsChargeTab * 3584 + 0xAC00;
            sMsChargeCursorY = 0x1000;
            DrawSprite(sMsChargeCursorX >> 8, (sMsChargeCursorY >> 8) + ((sine = gSineTable[sMsChargeBobPhase]) >> 6), AnimUpdate(&sMsChargeCursorAnim), sMsChargeMoogleTiles, sMsChargeMooglePalette, NULL, SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP, 0x7D0);
            break;
        case MS_CHARGE_MENU_STATE_GRID:
            ApproachValueHalf(&sMsChargeCursorX, (sMsChargeGridCol * 23 + 181) << 8);
            ApproachValueHalf(&sMsChargeCursorY, (sMsChargeGridRow * 26 + 40) << 8);
            DrawSprite(sMsChargeCursorX >> 8, (sMsChargeCursorY >> 8) + ((sine = gSineTable[sMsChargeBobPhase]) >> 6), AnimUpdate(&sMsChargeCursorAnim), sMsChargeMoogleTiles, sMsChargeMooglePalette, NULL, SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP, 0x7D0);
            DrawSprite(sMsChargeGridCol * 23 + 165, sMsChargeGridRow * 26 + 27, AnimUpdate(&sMsChargeHighlightAnim), sMsChargeHighlightTiles, sMsChargeCategoryPalette, NULL, SPRITE_PRIORITY(2), 0x7DA);
            break;
        case MS_CHARGE_MENU_STATE_VALUE:
            card = GetMsChargeSelectedCard();

            if (card->category == CARD_CATEGORY_ENEMY) {
                col = 0;
                row = 0;
            } else {
                col = sMsChargeValueCol;
                row = sMsChargeValueRow;
            }

            ApproachValueHalf(&sMsChargeCursorX, (col * 48 + 64) << 8);
            ApproachValueHalf(&sMsChargeCursorY, (row * 8 + 64) << 8);
            DrawSprite(sMsChargeCursorX >> 8, (sMsChargeCursorY >> 8) + ((sine = gSineTable[sMsChargeBobPhase]) >> 6), AnimUpdate(&sMsChargeCursorAnim), sMsChargeMoogleTiles, sMsChargeMooglePalette, NULL, SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP, 0x7D0);
            DrawSprite(col * 48 + 53, row * 8 + 67, AnimUpdate(&sMsChargeHighlightAnim), sMsChargeHighlightTiles, sMsChargeCategoryPalette, NULL, SPRITE_PRIORITY(2), 0x7DA);
            break;
        case MS_CHARGE_MENU_STATE_CONFIRM:
            ApproachValueHalf(&sMsChargeConfirmCursorX, sMsChargeConfirmCursor == 0 ? 0x4800 : 0x8800);
            DrawSprite(sMsChargeConfirmCursorX >> 8, 98, AnimUpdate(&sMsChargeConfirmCursorAnim), sMsChargeConfirmCursorTiles, sMsChargeConfirmCursorPalette, NULL, SPRITE_FLAG_HFLIP, 0);

            if (sMsChargeConfirmTextCount != 0) {
                DrawTextSlots(120 - GetTextSlotsWidth(sMsChargeConfirmText, sMsChargeConfirmTextCount) / 2, 60, sMsChargeConfirmText, sMsChargeMooglePalette, 1, sMsChargeConfirmTextCount);
            }

            if (sMsChargeYesTextCount != 0) {
                DrawTextSlots(80, 88, sMsChargeYesText, sMsChargeMooglePalette, 1, sMsChargeYesTextCount);
            }

            if (sMsChargeNoTextCount != 0) {
                DrawTextSlots(144, 88, sMsChargeNoText, sMsChargeMooglePalette, 1, sMsChargeNoTextCount);
            }

            break;
        case MS_CHARGE_MENU_STATE_NOTICE:
            if (sMsChargeNoticeTextCount != 0) {
                DrawTextSlots(120 - GetTextSlotsWidth(sMsChargeNoticeText, sMsChargeNoticeTextCount) / 2, 74, sMsChargeNoticeText, sMsChargeMooglePalette, 1, sMsChargeNoticeTextCount);
            }

            break;
        }
    }

    anim = AnimUpdate(&sMsChargeGridPremiumAnim);

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            if (sMsChargeGridSprites[i][j] != NULL) {
                DrawSprite(j * 23 + 181, i * 26 + 47, sMsChargeGridSprites[i][j], sMsChargeGridTiles[i][j], sMsChargeGridPalettes[i][j], NULL, SPRITE_PRIORITY(2), 0x83E);

                if (sMsChargeGridPremium[i][j] != 0) {
                    DrawSprite(j * 23 + 181, i * 26 + 47, anim, sMsChargeGridPremiumTiles, sMsChargeCard00Palette, NULL, SPRITE_PRIORITY(2), 0x834);
                }
            }
        }
    }

    if (sMsChargeMenuState != MS_CHARGE_MENU_STATE_TAB) {
        if (sMsChargeCardSprite != NULL) {
            DrawSprite(24, 92, sMsChargeCardSprite, sMsChargeCardTiles, sMsChargeCardPalette, NULL, SPRITE_PRIORITY(2), 0x848);
        }

        if (sMsChargeCardBackSprite != NULL) {
            DrawSprite(24, 92, sMsChargeCardBackSprite, sMsChargeCardBackTiles, sMsChargeCard00Palette, NULL, SPRITE_PRIORITY(2), 0x83E);

            if (sMsChargeCardPremium != 0) {
                DrawSprite(24, 92, AnimUpdate(&sMsChargePremiumAnim), sMsChargePremiumTiles, sMsChargeCard00Palette, NULL, SPRITE_PRIORITY(2), 0x834);
            }
        }

        if (sMsChargeNameTextCount != 0) {
            DrawTextSlots(12, 115, sMsChargeNameText, sMsChargeCategoryPalette, 1, sMsChargeNameTextCount);
        }

        if (sMsChargeDescTextCount != 0) {
            DrawTextSlots(6, 131, sMsChargeDescText, sMsChargeMooglePalette, 1, sMsChargeDescTextCount);
        }
    }
}

void mode_ms_charge_0() {
    s16 i;
    s16 j;
    u8* countPtr;
    u16 length;

    {
        MsCard** dst = &sMsCards;
        *dst = EwramAlloc(286 * sizeof(MsCard));
    }

    SpriteReset();
    FadeStartIn(FADE_MODE_BLACK, 16);
    SetBgMode0();
    SetupBg(0, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 0, 30, 0);
    SetupBg(3, 0, 31, 0);
    SetBgPriority(0, 3);
    SetBgPriority(1, 2);
    SetBgPriority(2, 1);
    SetBgPriority(3, 0);
    sMsChargeState = MS_CHARGE_STATE_FADE_IN;
    sMsChargeTab = 4;
    MsChargeBuildCardList();
    sMsChargeGridCol = 0;
    sMsChargeGridRow = 0;
    sMsChargeGridScroll = 0;

    if (sMsChargeCardTotal > 0) {
        s32* position = &sMsChargeCursorX;
        s16 state = MS_CHARGE_MENU_STATE_GRID;
        *position = 0xB500;
        sMsChargeCursorY = 0x2800;
        sMsChargeMenuState = state;
    } else {
        sMsChargeCursorX = sMsChargeTab * 3584 + 0xAC00;
        sMsChargeCursorY = 0x1000;
        sMsChargeMenuState = MS_CHARGE_MENU_STATE_TAB;
    }

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 2; j++) {
            sMsChargeGridPalettes[i][j] = NULL;
            sMsChargeGridTiles[i][j] = NULL;
            sMsChargeGridSprites[i][j] = NULL;
            sMsChargeGridPremium[i][j] = 0;
        }
    }

    sMsChargeCardPalette = NULL;
    sMsChargeCardTiles = NULL;
    sMsChargeCardSprite = NULL;
    sMsChargeCardBackTiles = NULL;
    sMsChargeCardBackSprite = NULL;
    sMsChargeCardPremium = 0;
    sMsChargeValueCol = 0;
    sMsChargeValueRow = 0;
    sMsChargeBobPhase = 0;
    sMsChargeMoogleAnimId = -1;
    sMsChargeMoogleAnimTimer = 0;
    LoadBgPalette(0, gMsChargeBgPalette, sizeof(gMsChargeBgPalette));
    LoadBgTiles(0, gMsChargeBgTiles, sizeof(gMsChargeBgTiles));
    LoadBgMap(0,
#ifdef VERSION_EU
        sMsChargeBgMapsByLanguage[gLanguage]
#else
        gMsChargeBgMap
#endif
    , 0x500);

    if (GetMsChargeSelectedCard()->category == CARD_CATEGORY_ENEMY) {
        LoadBgMap(1,
#ifdef VERSION_EU
        sMsChargeEnemyCountBgMapsByLanguage[gLanguage]
#else
        gMsChargeEnemyCountBgMap
#endif
    , 0x500);
    } else {
        LoadBgMap(1,
#ifdef VERSION_EU
        sMsChargeValueCountBgMapsByLanguage[gLanguage]
#else
        gMsChargeValueCountBgMap
#endif
    , 0x500);
    }

    MsChargeDrawTab(sMsChargeTab);
    LoadBgMap(2,
        gMsChargeWindowMap
    , sizeof(gMsChargeWindowMap));
    MsChargeDrawPoints();
    MsChargeDrawCardCounts();
    MsChargeDrawCategoryCounts();
    MsChargeDrawValueCounts();
    sMsChargeCard00Palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    sMsChargePremiumTiles = LoadObjTiles(gCardPremiumTiles, sizeof(gCardPremiumTiles));
    AnimInit(&sMsChargePremiumAnim, gCardPremiumAnims, gCardPremiumFrames);
    AnimStart(&sMsChargePremiumAnim, 0, ANIM_FLAG_LOOP);
    sMsChargeGridPremiumTiles = LoadObjTiles(gCardPremiumSmallTiles, sizeof(gCardPremiumSmallTiles));
    AnimInit(&sMsChargeGridPremiumAnim, gCardPremiumSmallAnims, gCardPremiumSmallFrames);
    AnimStart(&sMsChargeGridPremiumAnim, 0, ANIM_FLAG_LOOP);
    sMsChargeScrollbarTiles = LoadObjTiles(gMsChargeScrollbarTiles, sizeof(gMsChargeScrollbarTiles));
    sMsChargeCategoryPalette = LoadObjPalette(gMsChargeCategoryPalettes, sizeof(gMsChargeCategoryPalettes));
    sMsChargeHighlightTiles = LoadObjTiles(gMsChargeHighlightTiles, sizeof(gMsChargeHighlightTiles));
    AnimInit(&sMsChargeArrowAnim, gMsChargeHighlightAnims, gMsChargeHighlightFrames);
    AnimStart(&sMsChargeArrowAnim, 1, ANIM_FLAG_LOOP);
    AnimInit(&sMsChargeHighlightAnim, gMsChargeHighlightAnims, gMsChargeHighlightFrames);
    AnimStart(&sMsChargeHighlightAnim, 2, ANIM_FLAG_LOOP);
    sMsChargeMooglePalette = LoadObjPalette(gMoguPalette, sizeof(gMoguPalette));
    sMsChargeMoogleTiles = LoadObjTiles(gMsChargeMoogleTiles, sizeof(gMsChargeMoogleTiles));
    AnimInit(&sMsChargeCursorAnim,
        gMsChargeMoogleAnims
    ,
        gMsChargeMoogleFrames
    );
    AnimStart(&sMsChargeCursorAnim, 3, ANIM_FLAG_LOOP);
    AnimInit(&sMsChargeMoogleAnim,
        gMsChargeMoogleAnims
    ,
        gMsChargeMoogleFrames
    );
    AnimStart(&sMsChargeMoogleAnim, 0, ANIM_FLAG_LOOP);
    sMsChargeConfirmCursorPalette = LoadObjPalette(gMoguPalette, sizeof(gMoguPalette));
    sMsChargeConfirmCursorTiles = LoadObjTiles(gMoguFl00Tiles, sizeof(gMoguFl00Tiles));
    AnimInit(&sMsChargeConfirmCursorAnim, gMoguFl00Anims, gMoguFl00Frames);

    {
        TextSlot** dst = &sMsChargeNameText;
        *dst = EwramAlloc(36 * sizeof(TextSlot));
    }

    InitTextSlots(sMsChargeNameText, 36);

    {
        TextSlot** dst = &sMsChargeDescText;
        *dst = EwramAlloc(90 * sizeof(TextSlot));
    }

    InitTextSlots(sMsChargeDescText, 90);
    length = GetTextLength(
        LOCALIZED_STRING(gMsChargeConfirmText)
    );
    sMsChargeConfirmTextLength = length;

    {
        TextSlot** dst = &sMsChargeConfirmText;
        *dst = EwramAlloc(sMsChargeConfirmTextLength * sizeof(TextSlot));
    }

    InitTextSlots(sMsChargeConfirmText, sMsChargeConfirmTextLength);
    countPtr = &sMsChargeConfirmTextCount;
    *countPtr = LoadTextSlots(
        LOCALIZED_STRING(gMsChargeConfirmText)
    , sMsChargeConfirmText);
    length = GetTextLength(
        LOCALIZED_STRING(gMsChargeNoticeText)
    );
    sMsChargeNoticeTextLength = length;

    {
        TextSlot** dst = &sMsChargeNoticeText;
        *dst = EwramAlloc(sMsChargeNoticeTextLength * sizeof(TextSlot));
    }

    InitTextSlots(sMsChargeNoticeText, sMsChargeNoticeTextLength);
    countPtr = &sMsChargeNoticeTextCount;
    *countPtr = LoadTextSlots(
        LOCALIZED_STRING(gMsChargeNoticeText)
    , sMsChargeNoticeText);
    length = GetTextLength(
        LOCALIZED_STRING(gYesChoiceText)
    );
    sMsChargeYesTextLength = length;

    {
        TextSlot** dst = &sMsChargeYesText;
        *dst = EwramAlloc(sMsChargeYesTextLength * sizeof(TextSlot));
    }

    InitTextSlots(sMsChargeYesText, sMsChargeYesTextLength);
    countPtr = &sMsChargeYesTextCount;
    *countPtr = LoadTextSlots(
        LOCALIZED_STRING(gYesChoiceText)
    , sMsChargeYesText);
    length = GetTextLength(
        LOCALIZED_STRING(gNoChoiceText)
    );
    sMsChargeNoTextLength = length;

    {
        TextSlot** dst = &sMsChargeNoText;
        *dst = EwramAlloc(sMsChargeNoTextLength * sizeof(TextSlot));
    }

    InitTextSlots(sMsChargeNoText, sMsChargeNoTextLength);
    countPtr = &sMsChargeNoTextCount;
    *countPtr = LoadTextSlots(
        LOCALIZED_STRING(gNoChoiceText)
    , sMsChargeNoText);
    MsChargeLoadGrid();
    MsChargeLoadSelectedCard();
    EnableBg(0);
    EnableBg(1);
    DisableBg(2);
    DisableBg(3);
}

void mode_ms_charge_1() {
    UpdatePlayTime();
    sMsChargeBobPhase += 2;

    switch (sMsChargeState) {
    case MS_CHARGE_STATE_FADE_IN:
        if (!FadeIsActive()) {
            sMsChargeState = MS_CHARGE_STATE_MENU;
        }

        break;
    case MS_CHARGE_STATE_MENU:
        switch (sMsChargeMenuState) {
        case MS_CHARGE_MENU_STATE_GRID:
            MsChargeHandleGridInput();
            break;
        case MS_CHARGE_MENU_STATE_TAB:
            MsChargeHandleTabInput();
            break;
        case MS_CHARGE_MENU_STATE_VALUE:
            MsChargeHandleValueInput();
            break;
        case MS_CHARGE_MENU_STATE_CONFIRM:
            MsChargeHandleConfirmInput();
            break;
        case MS_CHARGE_MENU_STATE_NOTICE:
            MsChargeHandleNoticeInput();
            break;
        }

        break;
    case MS_CHARGE_STATE_EXIT:
        if (!FadeIsActive()) {
            if (sMsChargeBackToTop) {
                ModeRequest(&gModeMsTop, 2);
            } else {
                RequestMapMode();
            }
        }

        break;
    }

    if (sMsChargeMoogleAnimTimer > 0) {
        if (AnimGetId(&sMsChargeMoogleAnim) != 2) {
            if (sMsChargeMoogleAnimId < 0) {
                sMsChargeMoogleAnimId = AnimGetId(&sMsChargeMoogleAnim);
            }

            AnimStart(&sMsChargeMoogleAnim, 2, ANIM_FLAG_LOOP);
        }

        if (--sMsChargeMoogleAnimTimer <= 0) {
            if (AnimGetId(&sMsChargeMoogleAnim) != sMsChargeMoogleAnimId) {
                AnimStart(&sMsChargeMoogleAnim, sMsChargeMoogleAnimId, ANIM_FLAG_LOOP);
            }

            sMsChargeMoogleAnimId = -1;
        }
    } else if (sMsChargeMoogleAnimId >= 0) {
        if (AnimGetId(&sMsChargeMoogleAnim) != sMsChargeMoogleAnimId) {
            AnimStart(&sMsChargeMoogleAnim, sMsChargeMoogleAnimId, ANIM_FLAG_LOOP);
        }

        sMsChargeMoogleAnimId = -1;
    }

    MsChargeDraw();
}

void mode_ms_charge_2() {
    s32 i;
    s32 j;

    ReleaseObjPalette(sMsChargeCard00Palette);
    ReleaseObjTiles(sMsChargePremiumTiles);
    ReleaseObjTiles(sMsChargeGridPremiumTiles);
    ReleaseObjTiles(sMsChargeScrollbarTiles);
    ReleaseObjPalette(sMsChargeCategoryPalette);
    ReleaseObjTiles(sMsChargeHighlightTiles);
    ReleaseObjPalette(sMsChargeMooglePalette);
    ReleaseObjTiles(sMsChargeMoogleTiles);
    ReleaseObjPalette(sMsChargeConfirmCursorPalette);
    ReleaseObjTiles(sMsChargeConfirmCursorTiles);

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 2; j++) {
            if (sMsChargeGridPalettes[i][j] != NULL) {
                ReleaseObjPalette(sMsChargeGridPalettes[i][j]);
            }

            if (sMsChargeGridTiles[i][j] != NULL) {
                ReleaseObjTiles(sMsChargeGridTiles[i][j]);
            }
        }
    }

    if (sMsChargeCardPalette != NULL) {
        ReleaseObjPalette(sMsChargeCardPalette);
    }

    if (sMsChargeCardTiles != NULL) {
        ReleaseObjTiles(sMsChargeCardTiles);
    }

    if (sMsChargeCardBackTiles != NULL) {
        ReleaseObjTiles(sMsChargeCardBackTiles);
    }

    FreeTextSlots(sMsChargeNameText, 36);
    EwramFree(sMsChargeNameText);
    FreeTextSlots(sMsChargeDescText, 90);
    EwramFree(sMsChargeDescText);
    FreeTextSlots(sMsChargeConfirmText, sMsChargeConfirmTextLength);
    EwramFree(sMsChargeConfirmText);
    FreeTextSlots(sMsChargeNoticeText, sMsChargeNoticeTextLength);
    EwramFree(sMsChargeNoticeText);
    FreeTextSlots(sMsChargeYesText, sMsChargeYesTextLength);
    EwramFree(sMsChargeYesText);
    FreeTextSlots(sMsChargeNoText, sMsChargeNoTextLength);
    EwramFree(sMsChargeNoText);
    EwramFree(sMsCards);
}

Mode gModeMsCharge = {
    "mode_ms_charge",
    (ModeInitFunc)mode_ms_charge_0,
    mode_ms_charge_1,
    mode_ms_charge_2,
};
