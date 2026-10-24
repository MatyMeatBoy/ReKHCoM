/**
 * mode_mapinspect.c
 * Map Card List Screen
 */

#include "system_state.h"
#include "map_api.h"
#include "text.h"
#include "monsgage.h"
#include "mode_mapinspect.h"
#include "gba/keys.h"
#include "sprites_card.h"
#include "sprites_worldinspect.h"
#include "sprites_map.h"
#include "sprites_card_pictures.h"
#include "malloc.h"
#include "fade.h"
#include "mode_ms_top_api.h"
#include "songs.h"
#include "ms_types.h"
#include "jiminy_data.h"
#include "common_text.h"
#include "anim.h"
#include "card.h"
#include "card_api.h"
#include "card_description_data.h"
#include "card_ui_types.h"
#include "display.h"
#include "engine_math.h"
#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "game_state.h"
#include "gba/defines.h"
#include "key.h"
#include "m4a_song.h"
#include "mode.h"
#include "obj.h"
#include "obj_api.h"
#include "text_types.h"
#include "types.h"
#include "gba/macro.h"
#include <stddef.h>
#include "card_worldselect.h"
#include "registration_data.h"
#include "sprite_palettes.h"
#ifdef PLATFORM_PC
#include "port/port_hdsprites.h"
#endif

#ifdef VERSION_EU
static void* sMapInspectScrollMarkerSpritesByLanguage[5] = {
    gMapInspectBarFrame4,
    gMapInspectBarFrFrame4,
    gMapInspectBarDeFrame4,
    gMapInspectBarItFrame4,
    gMapInspectBarEsFrame4,
};

static void* sMapInspectTitleSpritesByLanguage[5] = {
    gMapInspectBarFrame5,
    gMapInspectBarFrFrame5,
    gMapInspectBarDeFrame5,
    gMapInspectBarItFrame5,
    gMapInspectBarEsFrame5,
};

static void* sMapInspectTopBarSpritesByLanguage[5] = {
    gMapInspectBarFrame6,
    gMapInspectBarFrFrame6,
    gMapInspectBarDeFrame6,
    gMapInspectBarItFrame6,
    gMapInspectBarEsFrame6,
};

static void* sMapInspectBottomBarSpritesByLanguage[5] = {
    gMapInspectBarFrame7,
    gMapInspectBarFrFrame7,
    gMapInspectBarDeFrame7,
    gMapInspectBarItFrame7,
    gMapInspectBarEsFrame7,
};

static void* sMapInspectBarTilesByLanguage[5] = {
    gMapInspectBarTiles,
    gMapInspectBarFrTiles,
    gMapInspectBarDeTiles,
    gMapInspectBarItTiles,
    gMapInspectBarEsTiles,
};

static AnimHeader** sMapInspectCursorAnimsByLanguage[5] = {
    gMapInspectBarAnims,
    gMapInspectBarFrAnims,
    gMapInspectBarDeAnims,
    gMapInspectBarItAnims,
    gMapInspectBarEsAnims,
};

static void** sMapInspectCursorFramesByLanguage[5] = {
    gMapInspectBarFrames,
    gMapInspectBarFrFrames,
    gMapInspectBarDeFrames,
    gMapInspectBarItFrames,
    gMapInspectBarEsFrames,
};
#endif

static MapCardCategoryDef sMapCardCategoryDefs[5] = {
    {4, 1, 1, 0},
    {0, 2, 2, 0},
    {1, 3, 3, 0},
    {2, 65535, 4, 0},
    {65535, 0, 0, 0},
};

#ifdef VERSION_EU
static const u16 sMapInspectBarTileSizesByLanguage[5] = {2752, 2816, 2816, 2816, 2816};
#endif
#define MAP_CARD_INVENTORY_EMPTY 27

static const u16 sMapCardCategoryTypes[4] = {MAP_CARD_COLOR_RED, MAP_CARD_COLOR_GREEN, MAP_CARD_COLOR_BLUE, MAP_CARD_COLOR_GOLD};

enum MapInspectState {
    MAP_INSPECT_STATE_BARS_IN,
    MAP_INSPECT_STATE_TITLE_IN,
    MAP_INSPECT_STATE_MENU,
    MAP_INSPECT_STATE_TITLE_OUT,
    MAP_INSPECT_STATE_BARS_OUT,
    MAP_INSPECT_STATE_EXIT
};

enum MapInspectMenuState {
    MAP_INSPECT_MENU_STATE_GRID,
    MAP_INSPECT_MENU_STATE_TAB,
    MAP_INSPECT_MENU_STATE_VALUE,
    MAP_INSPECT_MENU_STATE_CONFIRM,
    MAP_INSPECT_MENU_STATE_NOTICE
};

static MapCardInventoryEntry* sMapCardInventoryEntries;
static s16 sMapInspectMenuState;
static s16 sMapInspectTab;
static u16 sMapInspectCategoryStart[4];
static u16 sMapInspectCategoryEntryCount[4];
static s16 sMapInspectCategoryCardCount[4];
static s16 sMapInspectCardTotal;
static struct ObjPalette* sMapInspectBarPalette;
static struct ObjTiles* sMapInspectBarTiles;
static AnimState sMapInspectCursorAnim;
static struct ObjPalette* sMapInspectBarPalette2;
static struct ObjPalette* sMapInspectCategoryPalette;
static struct ObjTiles* sMapInspectHighlightTiles;
static AnimState sMapInspectHighlightAnim;
static s16 sMapInspectGridCol;
static s16 sMapInspectGridRow;
static s16 sMapInspectGridScroll;
static void* sMapInspectGridPalettes[4][3];
static void* sMapInspectGridTiles[4][3];
static void* sMapInspectGridSprites[4][3];
static u8 sMapInspectGridPremium[4][3];
static struct ObjPalette* sMapInspectCardPalette;
static struct ObjTiles* sMapInspectCardTiles;
static void* sMapInspectCardSprite;
static struct ObjPalette* sMapInspectCardBackPalette;
static struct ObjTiles* sMapInspectCardBackTiles;
static void* sMapInspectCardBackSprite;
static struct ObjTiles* sMapInspectPremiumTiles;
static AnimState sMapInspectPremiumAnim;
static struct ObjTiles* sMapInspectGridPremiumTiles;
static AnimState sMapInspectGridPremiumAnim;
static u8 sMapInspectCardPremium;
static TextSlot* sMapInspectNameText;
static u8 sMapInspectNameTextCount;
static TextSlot* sMapInspectDescText;
static u8 sMapInspectDescTextCount;
static TextSlot* sMapInspectConfirmText;
static u8 sMapInspectConfirmTextCount;
static u16 sMapInspectConfirmTextLength;
static TextSlot* sMapInspectYesText;
static u8 sMapInspectYesTextCount;
static u16 sMapInspectYesTextLength;
static TextSlot* sMapInspectNoText;
static u8 sMapInspectNoTextCount;
static u16 sMapInspectNoTextLength;
#ifdef VERSION_JP
static TextSlot* sMapInspectNoticeText[2];
static u8 sMapInspectNoticeTextCount[2];
static u16 sMapInspectNoticeTextLength[2];
#else
static TextSlot* sMapInspectNoticeText[1];
static u8 sMapInspectNoticeTextCount[1];
static u16 sMapInspectNoticeTextLength[1];
#endif

static s16 sMapInspectValueCol;
static s16 sMapInspectValueRow;
static s16 sMapInspectConfirmCursor;
static s16 sMapInspectState;
static s16 sMapInspectSteps;
static s32 sMapInspectBarY[2];
static s32 sMapInspectBarX;
static s32 sMapInspectCursorX;
static s32 sMapInspectCursorY;
static u8 sMapInspectReturnToMenu;

s16 GetMapInspectTabStart(s16 tab) {
    s16 start;

    if (tab <= 3) {
        start = sMapInspectCategoryStart[tab];
    } else {
        start = 0;
    }

    return start;
}

s16 GetMapInspectSelectedIndex() {
    return GetMapInspectTabStart(sMapInspectTab) + (sMapInspectGridScroll + sMapInspectGridRow) * 3 + sMapInspectGridCol;
}

MapCardInventoryEntry* GetMapInspectSelectedEntry() {
    return &sMapCardInventoryEntries[GetMapInspectSelectedIndex()];
}

void MapInspectSelectFirstValue() {
    MapCardInventoryEntry* entry;
    s16 i;

    entry = GetMapInspectSelectedEntry();

    if (entry->cardType <= 26) {
        for (i = 0; i < 10; i++) {
            if (entry->countsByValue[i] > 0) {
                break;
            }
        }

        sMapInspectValueCol = i / 5;
        sMapInspectValueRow = i % 5;
    } else {
        sMapInspectValueCol = 0;
        sMapInspectValueRow = 0;
    }
}

s16 GetMapInspectTabCount(s16 tab) {
    s16 count;
    s16 i;
    s32 categoryCount;

    if (tab <= 3) {
        count = sMapInspectCategoryEntryCount[tab];
    } else {
        count = 0;

        for (i = 0; i <= 3; i++) {
            categoryCount = sMapInspectCategoryEntryCount[i];
            count += categoryCount;
        }
    }

    return count;
}

u8 MapInspectCanDelete() {
    if (sMapInspectCardTotal > 20) {
        return TRUE;
    }

    return FALSE;
}

void MapInspectLoadGrid() {
    s16 start;
    s16 offset;
    s16 i;
    s16 j;
    s16 limit;
    u16 idx;

    start = GetMapInspectTabStart(sMapInspectTab);
    limit = GetMapInspectTabCount(sMapInspectTab);
    offset = sMapInspectGridScroll * 3;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 2; j++) {
            if (sMapInspectGridPalettes[i][j] != NULL) {
                ReleaseObjPalette(sMapInspectGridPalettes[i][j]);
            }

            if (sMapInspectGridTiles[i][j] != NULL) {
                ReleaseObjTiles(sMapInspectGridTiles[i][j]);
            }

            if (offset < limit) {
                idx = sMapCardInventoryEntries[start + offset].cardIndex;
                sMapInspectGridPalettes[i][j] = LoadObjPalette(gMapCardDefs[idx].palette2, 32);
                sMapInspectGridTiles[i][j] = LoadObjTiles(gMapCardDefs[idx].tiles2, gMapCardDefs[idx].tilesSize2);
                sMapInspectGridSprites[i][j] = *gMapCardDefs[idx].sprites2;
                sMapInspectGridPremium[i][j] = sMapCardInventoryEntries[start + offset].category == 3;
            } else {
                sMapInspectGridPalettes[i][j] = NULL;
                sMapInspectGridTiles[i][j] = NULL;
                sMapInspectGridSprites[i][j] = NULL;
                sMapInspectGridPremium[i][j] = 0;
            }

            offset++;
        }
    }
}

void MapInspectLoadSelectedCard() {
    MapCardInventoryEntry* entry;
    u16 idx;
    u16 color;
    u8* countPtr;

    entry = GetMapInspectSelectedEntry();

    if (sMapInspectCardPalette != NULL) {
        ReleaseObjPalette(sMapInspectCardPalette);
    }

    if (sMapInspectCardTiles != NULL) {
        ReleaseObjTiles(sMapInspectCardTiles);
    }

    if (sMapInspectCardBackPalette != NULL) {
        ReleaseObjPalette(sMapInspectCardBackPalette);
    }

    if (sMapInspectCardBackTiles != NULL) {
        ReleaseObjTiles(sMapInspectCardBackTiles);
    }

    if (sMapInspectCategoryPalette != NULL) {
        ReleaseObjPalette(sMapInspectCategoryPalette);
    }

    if (entry->cardType <= 26 && GetMapInspectTabCount(sMapInspectTab) > 0) {
        idx = entry->cardIndex;
        color = sMapCardCategoryTypes[entry->category];
        sMapInspectCardPalette = LoadObjPalette(gMapCardDefs[idx].palette, gMapCardDefs[idx].paletteSize);
        sMapInspectCardTiles = LoadObjTiles(gMapCardDefs[idx].tiles, gMapCardDefs[idx].tilesSize);
        sMapInspectCardSprite = *gMapCardDefs[idx].sprites;
        sMapInspectCardBackPalette = LoadObjPalette(gMapCardBackDefs[color].palette, gMapCardBackDefs[color].paletteSize);
        sMapInspectCardBackTiles = LoadObjTiles(gMapCardBackDefs[color].tiles, gMapCardBackDefs[color].tilesSize);
        sMapInspectCardBackSprite = *gMapCardBackDefs[color].sprites;
        sMapInspectCardPremium = entry->category == 3;
        sMapInspectCategoryPalette = LoadObjPalette(gMapInspectCategoryPalettes + entry->category * 16, 32);
        countPtr = &sMapInspectNameTextCount;
        *countPtr = LoadTextSlots(GetRoomName(entry->cardType), sMapInspectNameText);

#ifdef VERSION_EU
        {
            const u8** strings = gMapCardDescriptions[entry->cardType]->strings;
            countPtr = &sMapInspectDescTextCount;
            *countPtr = LoadTextSlots((void*)strings[gLanguage], sMapInspectDescText);
        }
#else
        countPtr = &sMapInspectDescTextCount;
        *countPtr = LoadTextSlots((void*)gMapCardDescriptions[entry->cardType], sMapInspectDescText);
#endif
    } else {
        sMapInspectCardPalette = NULL;
        sMapInspectCardTiles = NULL;
        sMapInspectCardSprite = NULL;
        sMapInspectCardBackPalette = NULL;
        sMapInspectCardBackTiles = NULL;
        sMapInspectCardBackSprite = NULL;
        sMapInspectCardPremium = FALSE;
        sMapInspectCategoryPalette = NULL;
        sMapInspectNameTextCount = 0;
        sMapInspectDescTextCount = 0;
    }
}

s16 GetMapInspectValueIndex(s16 col, s16 row) {
    return row + col * 5;
}

s16 GetMapInspectSelectedValue() {
    return GetMapInspectValueIndex(sMapInspectValueCol, sMapInspectValueRow);
}

void MapInspectDrawCardTotal() {
    LoadDecimalDigitTiles(sMapInspectCardTotal, gMapInspectTotalDigitTiles, (u8*)GetBgCharBase(0) + 0x3A0, 32, 2);
    LoadDecimalDigitTiles(99, gMapInspectTotalDigitTiles, (u8*)GetBgCharBase(0) + 0x3E0, 32, 2);
}

void MapInspectDrawCategoryCounts() {
    s16 i;
    s16 count;

    for (i = 0; i <= 3; i++) {
        count = sMapInspectCategoryCardCount[i];

        if (count != 0) {
            LoadDecimalDigitTiles(count, gMapInspectCategoryDigitTiles, (u8*)GetBgCharBase(0) + (i * 64 + 0x2A0), 32, 2);
        } else {
            LoadDecimalDigitTiles(0, gMapInspectCategoryZeroTiles, (u8*)GetBgCharBase(0) + (i * 64 + 0x2A0), 32, 2);
        }
    }
}

void MapInspectDrawValueCounts() {
    MapCardInventoryEntry* entry;
    s16 i;
    s32 count;

    entry = GetMapInspectSelectedEntry();

    if (GetMapInspectTabCount(sMapInspectTab) > 0) {
        LoadPalette(gMapInspectCategoryBgPalettes + entry->category * 16, (void*)PLTT, 12);
    }

    if (sMapInspectMenuState == MAP_INSPECT_MENU_STATE_TAB) {
        for (i = 0; i <= 9; i++) {
            LoadDecimalDigitTiles(0, gMapInspectValueZeroTiles, (u8*)GetBgCharBase(0) + (i * 64 + 0x40), 32, 1);
            LoadPalette(gMapInspectValueZeroPalette, (void*)(PLTT + (i + 6) * 2), sizeof(gMapInspectValueZeroPalette[0]));
        }
    } else if (entry->category == 3) {
        if (GetMapInspectTabCount(sMapInspectTab) > 0) {
            LoadBgMap(1, gMapInspectPremiumValuesMap, sizeof(gMapInspectPremiumValuesMap));
        }

        for (i = 0; i <= 9; i++) {
            count = entry->countsByValue[i];

            if (count != 0 && GetMapInspectTabCount(sMapInspectTab) > 0) {
                LoadDecimalDigitTiles(count, gMapInspectValueDigitTiles, (u8*)GetBgCharBase(0) + 0x40, 32, 1);
                LoadPalette(gMapInspectValueDigitPalette, (void*)(BG_PLTT + 0xC), sizeof(gMapInspectValueDigitPalette[0]));
                break;
            }
        }

        if (i > 9) {
            LoadDecimalDigitTiles(0, gMapInspectValueZeroTiles, (u8*)GetBgCharBase(0) + 0x40, 32, 1);
            LoadPalette(gMapInspectValueZeroPalette, (void*)(BG_PLTT + 0xC), sizeof(gMapInspectValueZeroPalette[0]));
        }
    } else {
        if (GetMapInspectTabCount(sMapInspectTab) > 0) {
            LoadBgMap(1, gMapInspectValuesMap, sizeof(gMapInspectValuesMap));
        }

        for (i = 0; i <= 9; i++) {
            count = entry->countsByValue[i];

            if (count != 0 && GetMapInspectTabCount(sMapInspectTab) > 0) {
                LoadDecimalDigitTiles(count, gMapInspectValueDigitTiles, (u8*)GetBgCharBase(0) + (i * 64 + 0x40), 32, 1);
                LoadPalette(gMapInspectValueDigitPalette, (void*)(PLTT + (i + 6) * 2), sizeof(gMapInspectValueDigitPalette[0]));
            } else {
                LoadDecimalDigitTiles(0, gMapInspectValueZeroTiles, (u8*)GetBgCharBase(0) + (i * 64 + 0x40), 32, 1);
                LoadPalette(gMapInspectValueZeroPalette, (void*)(PLTT + (i + 6) * 2), sizeof(gMapInspectValueZeroPalette[0]));
            }
        }
    }
}

void MapInspectDrawTab(s16 tab) {
#ifdef PLATFORM_PC
    PortMenuTabBg(-1, 0, 0, 16);
#endif
    RequestTilemapRectCopy(gMapInspectTabsMap, GetBgScreenBase(0), 0, sMapCardCategoryDefs[tab].displayIndex * 2, 0, 2, 11, 2);
}

void MapInspectDeleteCard() {
    MapCardInventoryEntry* entry;
    u16 card;

    entry = GetMapInspectSelectedEntry();

    if (entry->countsByValue[GetMapInspectSelectedValue()] > 0) {
        card = GetMapInspectSelectedValue() + entry->cardIndex;
        sMapInspectCategoryCardCount[entry->category]--;
        sMapInspectCardTotal--;
        entry->countsByValue[GetMapInspectSelectedValue()]--;
        RemoveMapCard(card);
        MapInspectDrawCardTotal();
        MapInspectDrawValueCounts();
        MapInspectDrawCategoryCounts();
    }
}

u8 MapCardEntryIsEmpty(MapCardInventoryEntry* entry) {
    s16 i;

    for (i = 0; i < 10; i++) {
        if (entry->countsByValue[i] > 0) {
            break;
        }
    }

    if (i > 9) {
        return TRUE;
    }

    return FALSE;
}

u8 MapCardEntrySelectedValueIsEmpty(MapCardInventoryEntry* entry) {
    if (entry->countsByValue[GetMapInspectSelectedValue()] == 0) {
        return TRUE;
    }

    return FALSE;
}

void MapInspectSelectNextValue(MapCardInventoryEntry* entry) {
    s16 value;
    s16 i;

    value = GetMapInspectSelectedValue();

    for (i = 0; i <= 9; i++) {
        if (entry->countsByValue[value] > 0) {
            break;
        }

        value++;

        if (value > 9) {
            value = 0;
        }
    }

    sMapInspectValueCol = value / 5;
    sMapInspectValueRow = value % 5;
}

void MapInspectRemoveEntry(MapCardInventoryEntry* entry) {
    u16 category;
    s16 j;

    category = entry->category;
    DmaCopy16(3, entry + 1, entry, (26 - GetMapInspectSelectedIndex()) * sizeof(MapCardInventoryEntry));
    DmaFill16(3, 0, &sMapCardInventoryEntries[26], sizeof(MapCardInventoryEntry));
    sMapCardInventoryEntries[26].cardType = MAP_CARD_INVENTORY_EMPTY;

    for (j = category + 1; j <= 3; j++) {
        sMapInspectCategoryStart[j]--;
    }

    sMapInspectCategoryEntryCount[category]--;

    if (GetMapInspectSelectedIndex() >= GetMapInspectTabCount(sMapInspectTab)) {
        if (--sMapInspectGridCol < 0) {
            sMapInspectGridCol = 2;

            if (--sMapInspectGridRow < 0) {
                sMapInspectGridRow = 0;

                if (--sMapInspectGridScroll < 0) {
                    sMapInspectGridCol = 0;
                    sMapInspectGridScroll = 0;
                }
            }
        }
    }

    sMapInspectValueCol = 0;
    sMapInspectValueRow = 0;
    MapInspectLoadGrid();
    MapInspectLoadSelectedCard();
    MapInspectDrawValueCounts();
}

void MapInspectBuildInventory() {
    s16* entryCountPtr;
    s32 start;
    s16 k;
    s16 j;
    s16 i;
    s16 entryCount;
    u16 kind;
    u16 n;
    u16 color;

    DmaFill16(3, 0, sMapCardInventoryEntries, 0x2F4);

    for (i = 0; i <= 26; i++) {
        sMapCardInventoryEntries[i].cardType = MAP_CARD_INVENTORY_EMPTY;
    }

    entryCount = 0;
    sMapInspectCardTotal = 0;

    for (j = 0; j <= 3; j++) {
        sMapInspectCategoryStart[j] = entryCount;
        sMapInspectCategoryCardCount[j] = 0;

        // @bug Reads past the end of gMapCardDefs at k = 26 (harmless).
        for (k = 0; k <= 26; k++) {
            color = gMapCardDefs[k * 10].color;

            if (color == sMapCardCategoryTypes[j]) {
                kind = gMapCardDefs[k * 10].kind;

                for (i = 0; i <= 9; i++) {
                    n = gMapCardCounts[(u16)(k * 10 + i)];

                    if (n != 0) {
                        if (color != MAP_CARD_COLOR_GOLD) {
                            sMapInspectCardTotal += n;
                        }

                        {
                            s16* counts = sMapInspectCategoryCardCount;
                            s32 index = entryCount;

                            counts[j] += n;
                            sMapCardInventoryEntries[index].cardType = kind;
                            sMapCardInventoryEntries[index].cardIndex = k * 10;
                            sMapCardInventoryEntries[index].category = j;
                            sMapCardInventoryEntries[index].countsByValue[i] = n;
                        }
                    }
                }
            }

            if (sMapCardInventoryEntries[entryCount].cardType <= 26) {
                entryCount++;
            }
        }

        entryCountPtr = &sMapInspectCategoryEntryCount[j];
        start = sMapInspectCategoryStart[j];
        *entryCountPtr = entryCount - start;
    }
}

u16 MapInspectReadMenuKeys() {
    u16 keys;

    keys = GetKeysPressed() & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON);
    keys |= GetKeysRepeat() & (DPAD_ANY | L_BUTTON | R_BUTTON);
    return keys;
}

void MapInspectHandleGridInput() {
    s16 oldCol;
    s16 oldRow;
    s16 oldScroll;
    u16 keys;

    oldCol = sMapInspectGridCol;
    oldRow = sMapInspectGridRow;
    oldScroll = sMapInspectGridScroll;
    keys = MapInspectReadMenuKeys();

    if (keys & A_BUTTON) {
        if (GetMapInspectSelectedEntry()->category != 3) {
            MapInspectSelectFirstValue();
            m4aSongNumStart(SONG_SYS_KETTEI);
            AnimStart(&sMapInspectHighlightAnim, 1, ANIM_FLAG_LOOP);
            sMapInspectMenuState = MAP_INSPECT_MENU_STATE_VALUE;
        } else {
            m4aSongNumStart(SONG_SYS_BEEP);
        }
    } else if (keys & B_BUTTON) {
        LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMapInspectReturnToMenu = TRUE;
        sMapInspectSteps = 16;
        sMapInspectState = MAP_INSPECT_STATE_TITLE_OUT;
    } else if (keys & START_BUTTON) {
        LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMapInspectReturnToMenu = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 16);
        sMapInspectState = MAP_INSPECT_STATE_EXIT;
    } else if (keys & SELECT_BUTTON) {
        sMapInspectGridCol = 0;
        sMapInspectGridRow = 0;
        sMapInspectGridScroll = 0;
        m4aSongNumStart(SONG_SYS_CLICKI04B);
        sMapInspectMenuState = MAP_INSPECT_MENU_STATE_TAB;
        MapInspectDrawValueCounts();
    } else if (keys & DPAD_UP) {
        if (sMapInspectGridRow > 0) {
            sMapInspectGridRow--;
        } else if (sMapInspectGridScroll > 0) {
            sMapInspectGridScroll--;
        } else {
            m4aSongNumStart(SONG_SYS_CLICKI04B);
            sMapInspectMenuState = MAP_INSPECT_MENU_STATE_TAB;
            MapInspectDrawValueCounts();
        }
    } else if (keys & DPAD_DOWN) {
        if ((sMapInspectGridScroll + sMapInspectGridRow + 1) * 3 + sMapInspectGridCol < GetMapInspectTabCount(sMapInspectTab)) {
            if (sMapInspectGridRow > 2) {
                sMapInspectGridScroll++;
            } else {
                sMapInspectGridRow++;
            }
        } else if (sMapInspectGridRow == 3) {
            if ((sMapInspectGridScroll + sMapInspectGridRow + 1) * 3 < GetMapInspectTabCount(sMapInspectTab)) {
                sMapInspectGridCol = (GetMapInspectTabCount(sMapInspectTab) - 1) % 3;
                sMapInspectGridScroll++;
            }
        }
    } else if (keys & DPAD_LEFT) {
        if (sMapInspectGridCol > 0) {
            sMapInspectGridCol--;
        }
    } else if (keys & DPAD_RIGHT) {
        if ((sMapInspectGridScroll + sMapInspectGridRow) * 3 + sMapInspectGridCol + 1 < GetMapInspectTabCount(sMapInspectTab)) {
            if (sMapInspectGridCol <= 1) {
                sMapInspectGridCol++;
            }
        }
    }

    if (sMapInspectGridCol != oldCol || sMapInspectGridRow != oldRow || sMapInspectGridScroll != oldScroll) {
        MapInspectSelectFirstValue();
        MapInspectDrawValueCounts();
        MapInspectLoadSelectedCard();
        m4aSongNumStart(SONG_SYS_CLICKI04B);

        if (sMapInspectGridScroll != oldScroll) {
            MapInspectLoadGrid();
        }
    }
}

void MapInspectHandleTabInput() {
    s16 old;
    u16 keys;

    old = sMapInspectTab;
    keys = MapInspectReadMenuKeys();

    if ((keys & A_BUTTON) == 0) {
        if (keys & START_BUTTON) {
            LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));
            m4aSongNumStart(SONG_SYS_CLOSE);
            sMapInspectReturnToMenu = FALSE;
            FadeStartOut(FADE_MODE_BLACK, 16);
            sMapInspectState = MAP_INSPECT_STATE_EXIT;
        } else if (keys & (B_BUTTON | DPAD_DOWN)) {
            if (GetMapInspectTabCount(sMapInspectTab) > 0) {
                sMapInspectGridCol = 0;
                sMapInspectGridRow = 0;
                sMapInspectGridScroll = 0;
                MapInspectSelectFirstValue();
                MapInspectDrawValueCounts();
                MapInspectLoadSelectedCard();
                AnimStart(&sMapInspectHighlightAnim, 0, ANIM_FLAG_LOOP);
                m4aSongNumStart(SONG_SYS_CLICKI04B);
                sMapInspectMenuState = MAP_INSPECT_MENU_STATE_GRID;
                MapInspectDrawValueCounts();
            } else if (keys & B_BUTTON) {
                LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));
                m4aSongNumStart(SONG_SYS_CLOSE);
                sMapInspectReturnToMenu = TRUE;
                sMapInspectSteps = 16;
                sMapInspectState = MAP_INSPECT_STATE_TITLE_OUT;
            } else if (keys & DPAD_DOWN) {
                m4aSongNumStart(SONG_SYS_BEEP);
            }
        } else if (keys & DPAD_LEFT) {
            sMapInspectTab = sMapCardCategoryDefs[sMapInspectTab].leftCategory;
        } else if (keys & DPAD_RIGHT) {
            sMapInspectTab = sMapCardCategoryDefs[sMapInspectTab].rightCategory;
        }
    }

    if (sMapInspectTab < 0) {
        sMapInspectTab = old;
    }

    if (sMapInspectTab != old) {
        MapInspectDrawTab(sMapInspectTab);
        MapInspectLoadGrid();
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void MapInspectSelectValueInColumn(MapCardInventoryEntry* entry, u16 col) {
    s16 base;
    s16 i;
    s32 row;

    base = sMapInspectValueRow;

    for (i = 0; i <= 4; i++) {
        row = base - i;

        if (row >= 0 && entry->countsByValue[GetMapInspectValueIndex(col, row)] > 0) {
            sMapInspectValueCol = col;
            sMapInspectValueRow = row;
            return;
        }

        row = base + i;

        if (row <= 4 && entry->countsByValue[GetMapInspectValueIndex(col, row)] > 0) {
            sMapInspectValueCol = col;
            sMapInspectValueRow = row;
            return;
        }
    }
}

void MapInspectHandleValueInput() {
    MapCardInventoryEntry* entry;
    s16 oldCol;
    s16 oldRow;
    s16 i;
    u16 keys;
    u16 trg;

    entry = GetMapInspectSelectedEntry();
    oldCol = sMapInspectValueCol;
    oldRow = sMapInspectValueRow;
    trg = MapInspectReadMenuKeys();
    keys = trg;

    if (keys & A_BUTTON) {
        if (MapInspectCanDelete()) {
            sMapInspectConfirmCursor = 1;
            sMapInspectCursorX = 0x7400;
            sMapInspectCursorY = 0x5000;
            AnimStart(&sMapInspectCursorAnim, 4, ANIM_FLAG_LOOP);
            EnableBg(2);
            m4aSongNumStart(SONG_SYS_CANSEL);
            sMapInspectMenuState = MAP_INSPECT_MENU_STATE_CONFIRM;
        } else {
            EnableBg(2);
            m4aSongNumStart(SONG_SYS_BEEP);
            sMapInspectMenuState = MAP_INSPECT_MENU_STATE_NOTICE;
        }
    } else {
        if (keys & B_BUTTON) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            AnimStart(&sMapInspectHighlightAnim, 0, ANIM_FLAG_LOOP);
            sMapInspectMenuState = MAP_INSPECT_MENU_STATE_GRID;
        } else if (keys & START_BUTTON) {
            LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));
            m4aSongNumStart(SONG_SYS_CLOSE);
            sMapInspectReturnToMenu = FALSE;
            FadeStartOut(FADE_MODE_BLACK, 16);
            sMapInspectState = MAP_INSPECT_STATE_EXIT;
        } else if (keys & DPAD_LEFT) {
            MapInspectSelectValueInColumn(entry, 0);
        } else if (keys & DPAD_RIGHT) {
            MapInspectSelectValueInColumn(entry, 1);
        } else if (keys & DPAD_UP) {
            for (i = 0; i <= 4; i++) {
                if (--sMapInspectValueRow < 0) {
                    sMapInspectValueRow = 4;
                }

                if (entry->countsByValue[GetMapInspectSelectedValue()] > 0) {
                    break;
                }
            }
        } else if (keys & DPAD_DOWN) {
            for (i = 0; i <= 4; i++) {
                if (++sMapInspectValueRow > 4) {
                    sMapInspectValueRow = 0;
                }

                if (entry->countsByValue[GetMapInspectSelectedValue()] > 0) {
                    break;
                }
            }
        }
    }

    if (sMapInspectValueCol != oldCol || sMapInspectValueRow != oldRow) {
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void MapInspectHandleConfirmInput() {
    MapCardInventoryEntry* entry;
    s16 old;
    u16 keys;

    entry = GetMapInspectSelectedEntry();
    old = sMapInspectConfirmCursor;
    keys = MapInspectReadMenuKeys();

    if (keys & A_BUTTON) {
        sMapInspectCursorX = sMapInspectValueCol * 12288 + 0x9200;
        sMapInspectCursorY = sMapInspectValueRow * 2048 + 0x1000;
        AnimStart(&sMapInspectCursorAnim, 0, ANIM_FLAG_LOOP);
        DisableBg(2);

        if (sMapInspectConfirmCursor == 0) {
            MapInspectDeleteCard();
            m4aSongNumStart(SONG_SYS_CARD_DELETE);

            if (MapCardEntryIsEmpty(entry)) {
                MapInspectRemoveEntry(entry);

                if (GetMapInspectTabCount(sMapInspectTab) > 0) {
                    AnimStart(&sMapInspectHighlightAnim, 0, ANIM_FLAG_LOOP);
                    sMapInspectMenuState = MAP_INSPECT_MENU_STATE_GRID;
                } else {
                    sMapInspectMenuState = MAP_INSPECT_MENU_STATE_TAB;
                }
            } else {
                if (MapCardEntrySelectedValueIsEmpty(entry)) {
                    MapInspectSelectNextValue(entry);
                }

                AnimStart(&sMapInspectHighlightAnim, 1, ANIM_FLAG_LOOP);
                sMapInspectMenuState = MAP_INSPECT_MENU_STATE_VALUE;
            }
        } else {
            m4aSongNumStart(SONG_SYS_CLOSE);
            AnimStart(&sMapInspectHighlightAnim, 1, ANIM_FLAG_LOOP);
            sMapInspectMenuState = MAP_INSPECT_MENU_STATE_VALUE;
        }
    } else if (keys & B_BUTTON) {
        sMapInspectCursorX = sMapInspectValueCol * 12288 + 0x9200;
        sMapInspectCursorY = sMapInspectValueRow * 2048 + 0x1000;
        AnimStart(&sMapInspectCursorAnim, 0, ANIM_FLAG_LOOP);
        DisableBg(2);
        m4aSongNumStart(SONG_SYS_CLOSE);
        AnimStart(&sMapInspectHighlightAnim, 1, ANIM_FLAG_LOOP);
        sMapInspectMenuState = MAP_INSPECT_MENU_STATE_VALUE;
    } else if (keys & START_BUTTON) {
        LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMapInspectReturnToMenu = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 16);
        sMapInspectState = MAP_INSPECT_STATE_EXIT;
    } else if (keys & DPAD_LEFT) {
        sMapInspectConfirmCursor = 0;
    } else if (keys & DPAD_RIGHT) {
        sMapInspectConfirmCursor = 1;
    }

    if (sMapInspectConfirmCursor != old) {
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void MapInspectHandleNoticeInput() {
    u16 keys;

    keys = MapInspectReadMenuKeys();

    if (keys & (A_BUTTON | B_BUTTON)) {
        DisableBg(2);
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMapInspectMenuState = MAP_INSPECT_MENU_STATE_VALUE;
    } else if (keys & START_BUTTON) {
        LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMapInspectReturnToMenu = FALSE;
        FadeStartOut(FADE_MODE_BLACK, 16);
        sMapInspectState = MAP_INSPECT_STATE_EXIT;
    }
}

void MapInspectDraw() {
    s32 i;
    s32 j;
    s16 maxScroll;
    s16 barOffset;
    void* anim;

#ifdef PLATFORM_PC
    /* the title's HD banner: the sprite stays over the BG copy the menu switches to once it has slid in */
    if (sMapInspectState == MAP_INSPECT_STATE_MENU && sMapInspectBarTiles != NULL &&
        PortHdSpriteFind(sMapInspectBarTiles->src,
#ifdef VERSION_EU
                         sMapInspectTitleSpritesByLanguage[gLanguage]
#else
                         gMapInspectBarFrame5
#endif
                         )) {
        DrawMenuHeader(0, 0,
#ifdef VERSION_EU
            sMapInspectTitleSpritesByLanguage[gLanguage],
#else
            gMapInspectBarFrame5,
#endif
            sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB8);
    }
#endif
    if (sMapInspectState != MAP_INSPECT_STATE_MENU) {
        DrawMenuHeader(sMapInspectBarX >> 8, 0,
#ifdef VERSION_EU
            sMapInspectTitleSpritesByLanguage[gLanguage],
#else
            gMapInspectBarFrame5,
#endif
            sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB8);
        DrawSprite(128, sMapInspectBarY[0] >> 8,
#ifdef VERSION_EU
            sMapInspectTopBarSpritesByLanguage[gLanguage],
#else
            gMapInspectBarFrame6,
#endif
            sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB9);
        DrawSprite(128, sMapInspectBarY[1] >> 8,
#ifdef VERSION_EU
            sMapInspectBottomBarSpritesByLanguage[gLanguage],
#else
            gMapInspectBarFrame7,
#endif
            sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(3), 0xBB9);
    }

    maxScroll = (GetMapInspectTabCount(sMapInspectTab) + 2) / 3 - 4;

    if (sMapInspectGridScroll <= maxScroll) {
        barOffset = 84 * sMapInspectGridScroll / maxScroll;
    } else {
        barOffset = 0;
    }

    DrawSprite(72, barOffset + 40,
#ifdef VERSION_EU
            sMapInspectScrollMarkerSpritesByLanguage[gLanguage],
#else
            gMapInspectBarFrame4,
#endif
            sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(2), 0x898);

    if (sMapInspectState == MAP_INSPECT_STATE_MENU) {
        switch (sMapInspectMenuState) {
        case MAP_INSPECT_MENU_STATE_TAB:
            ApproachValueHalf(&sMapInspectCursorX, sMapCardCategoryDefs[sMapInspectTab].displayIndex * 3584 - 256);
            ApproachValueHalf(&sMapInspectCursorY, 0);
            DrawSprite(sMapInspectCursorX >> 8, sMapInspectCursorY >> 8, AnimUpdate(&sMapInspectCursorAnim), sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);
            break;
        case MAP_INSPECT_MENU_STATE_GRID:
            ApproachValueHalf(&sMapInspectCursorX, (sMapInspectGridCol * 23 - 2) * 256);
            ApproachValueHalf(&sMapInspectCursorY, (sMapInspectGridRow * 26 + 16) * 256);
            DrawSprite(sMapInspectCursorX >> 8, sMapInspectCursorY >> 8, AnimUpdate(&sMapInspectCursorAnim), sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);
            DrawSprite(sMapInspectGridCol * 23 - 3, sMapInspectGridRow * 26 + 28, AnimUpdate(&sMapInspectHighlightAnim), sMapInspectHighlightTiles, sMapInspectCategoryPalette, NULL, SPRITE_PRIORITY(2), 0x7DA);
            break;
        case MAP_INSPECT_MENU_STATE_VALUE:
            GetMapInspectSelectedEntry();
            ApproachValueHalf(&sMapInspectCursorX, sMapInspectValueCol * 12288 + 0x9200);
            ApproachValueHalf(&sMapInspectCursorY, sMapInspectValueRow * 2048 + 0x1000);
            DrawSprite(sMapInspectCursorX >> 8, sMapInspectCursorY >> 8, AnimUpdate(&sMapInspectCursorAnim), sMapInspectBarTiles, sMapInspectBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);
            DrawSprite(sMapInspectValueCol * 48 + 133, sMapInspectValueRow * 8 + 35, AnimUpdate(&sMapInspectHighlightAnim), sMapInspectHighlightTiles, sMapInspectCategoryPalette, NULL, SPRITE_PRIORITY(2), 0x7DA);
            break;
        case MAP_INSPECT_MENU_STATE_CONFIRM:
            ApproachValueHalf(&sMapInspectCursorX, sMapInspectConfirmCursor == 0 ? 0x3400 : 0x7400);
            ApproachValueHalf(&sMapInspectCursorY, 0x5000);
            DrawSprite(sMapInspectCursorX >> 8, sMapInspectCursorY >> 8, AnimUpdate(&sMapInspectCursorAnim), sMapInspectBarTiles, sMapInspectBarPalette, NULL, 0, 0);

            if (sMapInspectConfirmTextCount != 0) {
                DrawTextSlots(120 - GetTextSlotsWidth(sMapInspectConfirmText, sMapInspectConfirmTextCount) / 2, 64, sMapInspectConfirmText, sMapInspectBarPalette, 1, sMapInspectConfirmTextCount);
            }

            if (sMapInspectYesTextCount != 0) {
                DrawTextSlots(80, 84, sMapInspectYesText, sMapInspectBarPalette, 1, sMapInspectYesTextCount);
            }

            if (sMapInspectNoTextCount != 0) {
                DrawTextSlots(144, 84, sMapInspectNoText, sMapInspectBarPalette, 1, sMapInspectNoTextCount);
            }

            break;
        case MAP_INSPECT_MENU_STATE_NOTICE:
            if (sMapInspectNoticeTextCount[0] != 0) {
                DrawTextSlots(120 - GetTextSlotsWidth(sMapInspectNoticeText[0], sMapInspectNoticeTextCount[0]) / 2, 68, sMapInspectNoticeText[0], sMapInspectBarPalette, 1, sMapInspectNoticeTextCount[0]);
#ifdef VERSION_JP
                DrawTextSlots(120 - GetTextSlotsWidth(sMapInspectNoticeText[1], sMapInspectNoticeTextCount[1]) / 2, 80, sMapInspectNoticeText[1], sMapInspectBarPalette, 1, sMapInspectNoticeTextCount[1]);
#endif
            }

            break;
        }
    }

    anim = AnimUpdate(&sMapInspectGridPremiumAnim);

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 2; j++) {
            if (sMapInspectGridSprites[i][j] != NULL) {
                DrawSprite(j * 23 + 13, i * 26 + 47, sMapInspectGridSprites[i][j], sMapInspectGridTiles[i][j], sMapInspectGridPalettes[i][j], NULL, SPRITE_PRIORITY(2), 0x83E);

                if (sMapInspectGridPremium[i][j] != 0) {
                    DrawSprite(j * 23 + 13, i * 26 + 47, anim, sMapInspectGridPremiumTiles, sMapInspectCardBackPalette, NULL, SPRITE_PRIORITY(2), 0x834);
                }
            }
        }
    }

    if (sMapInspectMenuState != MAP_INSPECT_MENU_STATE_TAB) {
        if (sMapInspectCardSprite != NULL) {
            DrawSprite(112, 56, sMapInspectCardSprite, sMapInspectCardTiles, sMapInspectCardPalette, NULL, SPRITE_PRIORITY(2), 0x848);
        }

        if (sMapInspectCardBackSprite != NULL) {
            DrawSprite(112, 56, sMapInspectCardBackSprite, sMapInspectCardBackTiles, sMapInspectCardBackPalette, NULL, SPRITE_PRIORITY(2), 0x83E);

            if (sMapInspectCardPremium) {
                DrawSprite(112, 56, AnimUpdate(&sMapInspectPremiumAnim), sMapInspectPremiumTiles, sMapInspectCardBackPalette, NULL, SPRITE_PRIORITY(2), 0x834);
            }
        }

        if (sMapInspectNameTextCount != 0) {
            if (sMapInspectMenuState != MAP_INSPECT_MENU_STATE_CONFIRM) {
                if (sMapInspectMenuState != MAP_INSPECT_MENU_STATE_NOTICE) {
                    DrawTextSlots(96, 92, sMapInspectNameText, sMapInspectCategoryPalette, 1, sMapInspectNameTextCount);
                }
            }
        }

        if (sMapInspectDescTextCount != 0) {
            DrawTextSlots(95, 107, sMapInspectDescText, sMapInspectBarPalette, 1, sMapInspectDescTextCount);
        }
    }
}

void mode_mapinspect_0() {
    s16 i;
    s16 j;
    s16 state;
    u16 length;

    sMapCardInventoryEntries = EwramAlloc(27 * sizeof(MapCardInventoryEntry));
    SpriteReset();
    FadeStartIn(FADE_MODE_BLACK, 16);
    SetBgMode0();
    SetupBg(0, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 2, 30, 15);
    SetupBg(3, 0, 31, 0);
    SetBgPriority(0, 3);
    SetBgPriority(1, 2);
    SetBgPriority(2, 1);
    SetBgPriority(3, 0);
    MapInspectBuildInventory();
    sMapInspectState = MAP_INSPECT_STATE_BARS_IN;
    sMapInspectSteps = 16;
    sMapInspectBarY[0] = -0x800;
    sMapInspectBarY[1] = 0xA800;
    sMapInspectBarX = -0x8000;
    sMapInspectTab = 4;
    sMapInspectGridCol = 0;
    sMapInspectGridRow = 0;
    sMapInspectGridScroll = 0;

    if (sMapInspectCardTotal > 0) {
        sMapInspectCursorX = (state = MAP_INSPECT_MENU_STATE_GRID, -0x200);
        sMapInspectCursorY = 0x1000;
        sMapInspectMenuState = state;
    } else {
        sMapInspectCursorX = sMapCardCategoryDefs[4].displayIndex * 7 * 512 - 0x100;
        sMapInspectCursorY = 0;
        sMapInspectMenuState = MAP_INSPECT_MENU_STATE_TAB;
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            sMapInspectGridPalettes[i][j] = NULL;
            sMapInspectGridTiles[i][j] = NULL;
            sMapInspectGridSprites[i][j] = NULL;
            sMapInspectGridPremium[i][j] = 0;
        }
    }

    sMapInspectCardPalette = NULL;
    sMapInspectCardTiles = NULL;
    sMapInspectCardSprite = NULL;
    sMapInspectCardBackPalette = NULL;
    sMapInspectCardBackTiles = NULL;
    sMapInspectCardBackSprite = NULL;
    sMapInspectCardPremium = FALSE;
    sMapInspectCategoryPalette = NULL;
    sMapInspectValueCol = 0;
    sMapInspectValueRow = 0;
    LoadBgPalette(0, gMapInspectPalettes, sizeof(gMapInspectPalettes));
    LoadBgTiles(0, gMapInspectTiles, sizeof(gMapInspectTiles));

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        break;
    case LANGUAGE_FRENCH:
        RequestDma3Copy(gMapInspectFrTiles, (u8*)GetBgCharBase(0) + 0x800, 0xC00);
        RequestDma3Copy(gMapInspectFrTiles + 0xC00, (u8*)GetBgCharBase(0) + 0x2400, 0x800);
        break;
    case LANGUAGE_GERMAN:
        RequestDma3Copy(gMapInspectDeTiles, (u8*)GetBgCharBase(0) + 0x800, 0xC00);
        RequestDma3Copy(gMapInspectDeTiles + 0xC00, (u8*)GetBgCharBase(0) + 0x2400, 0x800);
        break;
    case LANGUAGE_ITALIAN:
        RequestDma3Copy(gMapInspectItTiles, (u8*)GetBgCharBase(0) + 0x800, 0xC00);
        RequestDma3Copy(gMapInspectItTiles + 0xC00, (u8*)GetBgCharBase(0) + 0x2400, 0x800);
        break;
    case LANGUAGE_SPANISH:
        RequestDma3Copy(gMapInspectEsTiles, (u8*)GetBgCharBase(0) + 0x800, 0xC00);
        RequestDma3Copy(gMapInspectEsTiles + 0xC00, (u8*)GetBgCharBase(0) + 0x2400, 0x800);
        break;
    }
#endif
    LoadBgPalette(2, gCard00Palette, sizeof(gCard00Palette));
    LoadBgTiles(2, gConfirmWinTiles, sizeof(gConfirmWinTiles));
    LoadBgMap(2, gConfirmWinMap, sizeof(gConfirmWinMap));
    LoadBgMap(0, gMapInspectBgMap, sizeof(gMapInspectBgMap));

    if (GetMapInspectSelectedEntry()->category == 3) {
        LoadBgMap(1, gMapInspectPremiumValuesMap, sizeof(gMapInspectPremiumValuesMap));
    } else {
        LoadBgMap(1, gMapInspectValuesMap, sizeof(gMapInspectValuesMap));
    }

    MapInspectDrawTab(sMapInspectTab);
    MapInspectDrawCardTotal();
    MapInspectDrawCategoryCounts();
    MapInspectDrawValueCounts();
    sMapInspectBarPalette = LoadObjPalette(gMapInspectBarPalette, sizeof(gMapInspectBarPalette));
#ifdef VERSION_EU
    sMapInspectBarTiles = LoadObjTiles(sMapInspectBarTilesByLanguage[gLanguage], sMapInspectBarTileSizesByLanguage[gLanguage]);
    AnimInit(&sMapInspectCursorAnim, sMapInspectCursorAnimsByLanguage[gLanguage], sMapInspectCursorFramesByLanguage[gLanguage]);
#else
    sMapInspectBarTiles = LoadObjTiles(gMapInspectBarTiles, sizeof(gMapInspectBarTiles));
    AnimInit(&sMapInspectCursorAnim, gMapInspectBarAnims, gMapInspectBarFrames);
#endif
    AnimStart(&sMapInspectCursorAnim, 0, ANIM_FLAG_LOOP);
    sMapInspectHighlightTiles = LoadObjTiles(gMapInspectHighlightTiles, sizeof(gMapInspectHighlightTiles));
    AnimInit(&sMapInspectHighlightAnim, gMapInspectHighlightAnims, gMapInspectHighlightFrames);
    AnimStart(&sMapInspectHighlightAnim, 0, ANIM_FLAG_LOOP);
    sMapInspectBarPalette2 = LoadObjPalette(gMapInspectBarPalette, sizeof(gMapInspectBarPalette));
    sMapInspectPremiumTiles = LoadObjTiles(gCardPremiumTiles, sizeof(gCardPremiumTiles));
    AnimInit(&sMapInspectPremiumAnim, gCardPremiumAnims, gCardPremiumFrames);
    AnimStart(&sMapInspectPremiumAnim, 0, ANIM_FLAG_LOOP);
    sMapInspectGridPremiumTiles = LoadObjTiles(gCardPremiumSmallTiles, sizeof(gCardPremiumSmallTiles));
    AnimInit(&sMapInspectGridPremiumAnim, gCardPremiumSmallAnims, gCardPremiumSmallFrames);
    AnimStart(&sMapInspectGridPremiumAnim, 0, ANIM_FLAG_LOOP);

    sMapInspectNameText = EwramAlloc(0x24 * sizeof(TextSlot));
    InitTextSlots(sMapInspectNameText, 0x24);
    sMapInspectDescText = EwramAlloc(0x5A * sizeof(TextSlot));
    InitTextSlots(sMapInspectDescText, 0x5A);

    length = GetTextLength(LOCALIZED_STRING(gDeleteCardConfirmText));
    sMapInspectConfirmTextLength = length;
    sMapInspectConfirmText = EwramAlloc(sMapInspectConfirmTextLength * sizeof(TextSlot));
    InitTextSlots(sMapInspectConfirmText, sMapInspectConfirmTextLength);
    sMapInspectConfirmTextCount = LoadTextSlots(LOCALIZED_STRING(gDeleteCardConfirmText), sMapInspectConfirmText);

    length = GetTextLength(LOCALIZED_STRING(gYesChoiceText));
    sMapInspectYesTextLength = length;
    sMapInspectYesText = EwramAlloc(sMapInspectYesTextLength * sizeof(TextSlot));
    InitTextSlots(sMapInspectYesText, sMapInspectYesTextLength);
    sMapInspectYesTextCount = LoadTextSlots(LOCALIZED_STRING(gYesChoiceText), sMapInspectYesText);

    length = GetTextLength(LOCALIZED_STRING(gNoChoiceText));
    sMapInspectNoTextLength = length;
    sMapInspectNoText = EwramAlloc(sMapInspectNoTextLength * sizeof(TextSlot));
    InitTextSlots(sMapInspectNoText, sMapInspectNoTextLength);
    sMapInspectNoTextCount = LoadTextSlots(LOCALIZED_STRING(gNoChoiceText), sMapInspectNoText);

#ifdef VERSION_JP
    sMapInspectNoticeTextLength[0] = GetTextLength(gDeckErrorAnyMoreText);
    sMapInspectNoticeText[0] = EwramAlloc(sMapInspectNoticeTextLength[0] * sizeof(TextSlot));
    InitTextSlots(sMapInspectNoticeText[0], sMapInspectNoticeTextLength[0]);
    sMapInspectNoticeTextCount[0] = LoadTextSlots(gDeckErrorAnyMoreText, sMapInspectNoticeText[0]);

    sMapInspectNoticeTextLength[1] = GetTextLength(gDeckErrorLastAttackCardText);
    sMapInspectNoticeText[1] = EwramAlloc(sMapInspectNoticeTextLength[1] * sizeof(TextSlot));
    InitTextSlots(sMapInspectNoticeText[1], sMapInspectNoticeTextLength[1]);
    sMapInspectNoticeTextCount[1] = LoadTextSlots(gDeckErrorLastAttackCardText, sMapInspectNoticeText[1]);
#else
#ifdef VERSION_EU
    sMapInspectNoticeTextLength[0] = GetTextLength(GetLocalizedString(&gDeckErrorLastAttackCardTextByLanguage));
#else
    sMapInspectNoticeTextLength[0] = GetTextLength(gMapInspectNoticeText);
#endif
    sMapInspectNoticeText[0] = EwramAlloc(sMapInspectNoticeTextLength[0] * sizeof(TextSlot));
    InitTextSlots(sMapInspectNoticeText[0], sMapInspectNoticeTextLength[0]);
#ifdef VERSION_EU
    sMapInspectNoticeTextCount[0] = LoadTextSlots(GetLocalizedString(&gDeckErrorLastAttackCardTextByLanguage), sMapInspectNoticeText[0]);
#else
    sMapInspectNoticeTextCount[0] = LoadTextSlots(gMapInspectNoticeText, sMapInspectNoticeText[0]);
#endif
#endif

    MapInspectLoadGrid();
    MapInspectLoadSelectedCard();
    EnableBg(0);
    EnableBg(1);
    DisableBg(2);
    DisableBg(3);
}

void mode_mapinspect_1() {
    UpdatePlayTime();

    switch (sMapInspectState) {
    case MAP_INSPECT_STATE_BARS_IN:
        ApproachValue(&sMapInspectBarY[0], 0, sMapInspectSteps);
        ApproachValue(&sMapInspectBarY[1], 0x9800, sMapInspectSteps);
        sMapInspectSteps--;

        if (sMapInspectSteps <= 0) {
            sMapInspectSteps = 16;
            sMapInspectState = MAP_INSPECT_STATE_TITLE_IN;
        }

        break;
    case MAP_INSPECT_STATE_TITLE_IN:
        ApproachValue(&sMapInspectBarX, 0, sMapInspectSteps);
        sMapInspectSteps--;

        if (sMapInspectSteps <= 0) {
            LoadBgMap(0, gMapInspectBgHeaderMap, sizeof(gMapInspectBgHeaderMap));
            sMapInspectState = MAP_INSPECT_STATE_MENU;
        }

        break;
    case MAP_INSPECT_STATE_MENU:
        switch (sMapInspectMenuState) {
        case MAP_INSPECT_MENU_STATE_GRID:
            MapInspectHandleGridInput();
            break;
        case MAP_INSPECT_MENU_STATE_TAB:
            MapInspectHandleTabInput();
            break;
        case MAP_INSPECT_MENU_STATE_VALUE:
            MapInspectHandleValueInput();
            break;
        case MAP_INSPECT_MENU_STATE_CONFIRM:
            MapInspectHandleConfirmInput();
            break;
        case MAP_INSPECT_MENU_STATE_NOTICE:
            MapInspectHandleNoticeInput();
            break;
        }

        break;
    case MAP_INSPECT_STATE_TITLE_OUT:
        ApproachValue(&sMapInspectBarX, -0x8000, sMapInspectSteps);
        sMapInspectSteps--;

        if (sMapInspectSteps <= 0) {
            sMapInspectSteps = 16;
            sMapInspectState = MAP_INSPECT_STATE_BARS_OUT;
        }

        break;
    case MAP_INSPECT_STATE_BARS_OUT:
        ApproachValue(&sMapInspectBarY[0], -0x800, sMapInspectSteps);
        ApproachValue(&sMapInspectBarY[1], 0xA800, sMapInspectSteps);
        sMapInspectSteps--;

        if (sMapInspectSteps <= 0) {
            FadeStartOut(FADE_MODE_BLACK, 16);
            sMapInspectState = MAP_INSPECT_STATE_EXIT;
        }

        break;
    case MAP_INSPECT_STATE_EXIT:
        if (!FadeIsActive()) {
            ReturnToMap(sMapInspectReturnToMenu);
        }

        break;
    }

    MapInspectDraw();
}

void mode_mapinspect_2() {
    s32 i;
    s32 j;

    ReleaseObjPalette(sMapInspectBarPalette);
    ReleaseObjTiles(sMapInspectBarTiles);
    ReleaseObjTiles(sMapInspectHighlightTiles);
    ReleaseObjPalette(sMapInspectBarPalette2);
    ReleaseObjTiles(sMapInspectPremiumTiles);
    ReleaseObjTiles(sMapInspectGridPremiumTiles);

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            if (sMapInspectGridPalettes[i][j] != NULL) {
                ReleaseObjPalette(sMapInspectGridPalettes[i][j]);
            }

            if (sMapInspectGridTiles[i][j] != NULL) {
                ReleaseObjTiles(sMapInspectGridTiles[i][j]);
            }
        }
    }

    if (sMapInspectCardPalette != NULL) {
        ReleaseObjPalette(sMapInspectCardPalette);
    }

    if (sMapInspectCardTiles != NULL) {
        ReleaseObjTiles(sMapInspectCardTiles);
    }

    if (sMapInspectCardBackPalette != NULL) {
        ReleaseObjPalette(sMapInspectCardBackPalette);
    }

    if (sMapInspectCardBackTiles != NULL) {
        ReleaseObjTiles(sMapInspectCardBackTiles);
    }

    if (sMapInspectCategoryPalette != NULL) {
        ReleaseObjPalette(sMapInspectCategoryPalette);
    }

    FreeTextSlots(sMapInspectNameText, 0x24);
    EwramFree(sMapInspectNameText);
    FreeTextSlots(sMapInspectDescText, 0x5A);
    EwramFree(sMapInspectDescText);
    FreeTextSlots(sMapInspectConfirmText, sMapInspectConfirmTextLength);
    EwramFree(sMapInspectConfirmText);
    FreeTextSlots(sMapInspectYesText, sMapInspectYesTextLength);
    EwramFree(sMapInspectYesText);
    FreeTextSlots(sMapInspectNoText, sMapInspectNoTextLength);
    EwramFree(sMapInspectNoText);

#ifdef VERSION_JP
    for (i = 0; i < 2; i++) {
#else
    for (i = 0; i < 1; i++) {
#endif
        FreeTextSlots(sMapInspectNoticeText[i], sMapInspectNoticeTextLength[i]);
        EwramFree(sMapInspectNoticeText[i]);
    }

    EwramFree(sMapCardInventoryEntries);
}

Mode gModeMapinspect = {
    "mode_mapinspect",
    (ModeInitFunc)mode_mapinspect_0,
    mode_mapinspect_1,
    mode_mapinspect_2,
};
