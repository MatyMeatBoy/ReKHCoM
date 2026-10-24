/**
 * card_deckmenu2_riku.c
 * Riku Deck Menu
 */

#include "card_localized_data.h"
#include "registration_data.h"
#include "system_state.h"
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
#include "taskpool.h"
#include "key.h"
#include "malloc.h"
#include "card.h"
#include "sprites_deck_menu.h"

#ifdef PLATFORM_PC
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "port/port_hdsprites.h"
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
#include "gba/keys.h"
#include "songs.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "gba/defines.h"
#include "player_progression_types.h"
#include "types.h"
#include <stddef.h>
#include "card_deckmenu2_riku.h"
#include "card_map_anim.h"
#include "card_deckmenu2.h"
#include "default_bg_map.h"
#include "sprite_palettes.h"
#include "macros.h"

#ifdef VERSION_EU
static const u16 sRikuDeckTitleBannerTileSizes[5] = { 0x320, 0x320, 0x320, 0x320, 0x320 };
#endif

static const s16 sRikuDeckTabPointerX[3] = { 116, 116, 116 };

static const s16 sRikuDeckTabPointerY[3] = { 56, 104, 148 };

const s16 gUnk_09041EC0[5] = { 12, 28, 42, 56, 70 };

const s16 gRikuDeckFilterTabX[6] = { 172, 172, 188, 202, 216, 230 };

const s16 gUnk_09041ED6[5] = { 64, 82, 100, 118, 136 };

const s16 gRikuDeckValueGridX[2] = { 80, 128 };

const s16 gRikuDeckValueGridY[5] = { 80, 88, 96, 104, 112 };

static const u16 sRikuDeckRowY[4] = { 45, 93, 141, 30 };

static void Deckmenu2_0(RikuDeckMenuWork* work, void* resultOut) {
#ifdef PLATFORM_PC
    PortClearBgTextTiles();
#endif
    u16 rowY;

    work->resultOut = resultOut;
    SetBgMode0();
    SetBackdropColor(0, 0, 0);
    SetupBg(0, 0, 31, 0);
#ifdef VERSION_EU
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 0, 28, 0);
#else
    SetupBg(1, 2, 23, 0);
    SetupBg(2, 1, 15, 0);
#endif
    SetupBg(3, 0, 30, 0);
    RequestDma3Clear(GetBgCharBase(0), 0x4000);
    RequestDma3Clear(GetBgCharBase(1), 0x4000);
    RequestDma3Clear(GetBgCharBase(2), 0x4000);
    RequestDma3Clear(GetBgCharBase(3), 0x4000);
    SetBgPriority(0, 0);
    SetBgPriority(1, 1);
    SetBgPriority(2, 2);
    FadeStartIn(FADE_MODE_BLACK, 16);
    ListPoolInit(&work->pool);
    TaskPoolInit(&work->taskpool, 99);
    TaskPoolInit(&work->cardpool, 1);
    work->deckIndex = GetActiveDeckIndex();
    CreateRikuDeckGridCards(work, 0);
    work->tiles = AllocObjTiles(0x120, NULL);
    SetObjTileSource(work->tiles, gHandCursorTiles);
    AnimInit(&work->anim2, gHandCursorAnims, gHandCursorFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim2);
    work->handX = sRikuDeckTabPointerX[0] << 8;
    work->handY = sRikuDeckTabPointerY[0] << 8;
    work->handFlags = 0;
    work->tiles4 = LoadObjTiles(gDeckScrollThumbTiles, sizeof(gDeckScrollThumbTiles));
    work->palette = LoadObjPalette(gDialogBoxPalette, sizeof(gDialogBoxPalette));
    work->handVisible = FALSE;
    work->tiles2 = AllocObjTiles(0x280, NULL);
    SetRikuDeckMenuFrameCursor(work, DECK_FRAME_CURSOR_CARD);
    work->palette4 = LoadObjPalette(gDeckMenuTextPalette, sizeof(gDeckMenuTextPalette));
    work->tiles10 = NULL;
    work->tiles7 = NULL;
    work->tiles8 = NULL;
    work->tiles9 = NULL;
    work->palette5 = NULL;
    work->palette6 = NULL;
    work->tiles3 = NULL;
    work->palette2 = NULL;
    work->cursorCol = 0;
    work->cursorRow = 0;
    work->prevCursorCol = 0;
    work->prevCursorRow = 0;
    work->timer = 4;
    work->commandCursor = 0;
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
#ifdef VERSION_EU
    work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTilesByLanguage[gLanguage], sRikuDeckTitleBannerTileSizes[gLanguage]);
#else
    work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTiles, sizeof(gRikuDeckTitleBannerTiles));
#endif
    work->tiles6 = LoadObjTiles(gDeckMenuBarTiles, sizeof(gDeckMenuBarTiles));
    work->palette3 = LoadObjPalette(gDeckTitleBannerPalette, sizeof(gDeckTitleBannerPalette));
    work->unk_4BC = 79;
    rowY = sRikuDeckRowY[work->deckIndex];
    work->unk_4BE = rowY;
    work->unk_4C0 = 225;
    rowY = sRikuDeckRowY[work->deckIndex];
    work->unk_4C2 = rowY;
    work->unk_503 = 0;
    work->inputDelay = 0;
    work->textSlotCount = 0;
    work->textSlotCount2 = 0;
    work->textSlotCount3 = 0;
    work->textSlotCount4 = 0;
    work->result = DECK_MENU_RESULT_NONE;
    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    InitTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    InitTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    InitTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
    InitTextSlots(work->textSlots5, ARRAY_COUNT(work->textSlots5));
    work->descriptionX = 94;
    work->descriptionY = 126;
    work->previewShown = FALSE;
}

void DrawRikuCardDescription(RikuDeckMenuWork* work) {
    DrawTextSlots(work->descriptionX, work->descriptionY, work->textSlots5,
                  work->palette, 20, work->textSlotCount5);
}

void LoadRikuCardDescriptionText(RikuDeckMenuWork* work, u16 card) {
    const CardDef* def;
    void* description;

    def = &gCardDefs[card];
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

u8 UpdateRikuDeckMenuLoadBgs(RikuDeckMenuWork* work, void* task) {
#ifdef VERSION_EU
    LoadBgTiles(3, gDeckMenuTiles, 0x5400);

    switch (gLanguage) {
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

    LoadBgPalette(3, gDeckMenuPalettes, sizeof(gDeckMenuPalettes));
    LoadBgMap(3, gRikuDeckMenuMap, sizeof(gRikuDeckMenuMap));
    LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
#else
    LoadBgTiles(3, gDeckMenuTiles, sizeof(gDeckMenuTiles));
    LoadBgPalette(3, gDeckMenuPalettes, sizeof(gDeckMenuPalettes));
    LoadBgMap(3, gRikuDeckMenuMap, sizeof(gRikuDeckMenuMap));
    LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgTiles(1, gDeck1PanelTiles, sizeof(gDeck1PanelTiles));
#endif
    LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
    LoadBgMap(2, gDefaultBgMap, sizeof(gDefaultBgMap));
    SetBgScroll(0, (u16)-88, (u16)-108);
    SetBgScroll(1, (u16)-88, (u16)-16);
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuLoadDeckInfo);
    return 1;
}

u8 UpdateRikuDeckMenuLoadDeckInfo(RikuDeckMenuWork* work, void* task) {
    u8* base;
    u16* pal;

    base = GetBgCharBase(1);
    pal = (u16*)(BG_PLTT + 8 * PLTT_SIZE_4BPP);
    LoadPalette(gDeckTabHighlightPalette, pal, sizeof(gDeckTabHighlightPalette));
#ifdef VERSION_EU
    RequestDma3Copy(gRikuDeckEquipMarkerTilesByLanguage[gLanguage] + 0x20, base + 0x2D80, 0x1E0);
#else
    RequestDma3Copy(gDeckEquipMarkerTiles + 0x20, base + 0x1A0, 0x1E0);
#endif
    LoadBgMap(0, gDeckDescriptionWindowMap, sizeof(gDeckDescriptionWindowMap));
    LoadBgMap(1, gRikuDeckPanelMap, sizeof(gRikuDeckPanelMap));
    DrawRikuDeckCategoryCount(work->deckAttackCount, CARD_CATEGORY_ATTACK);
    DrawRikuDeckCategoryCount(work->deckMagicCount, CARD_CATEGORY_MAGIC);
    DrawRikuDeckCategoryCount(work->deckItemCount, CARD_CATEGORY_ITEM);
    DrawRikuDeckCategoryCount(work->deckEnemyCount, CARD_CATEGORY_ENEMY);
    DrawRikuDeckCardCount(0);
    DrawRikuCardTotals();
    work->thumbX = 0x4800;
    work->thumbY = 0x2800;
    work->cursorRow = work->deckIndex;
    ApproachValue(&work->handX, sRikuDeckTabPointerX[work->cursorCol] << 8, work->timer);
    ApproachValue(&work->handY, sRikuDeckTabPointerY[work->cursorRow] << 8, work->timer);
    work->view = DECK_MENU_VIEW_DECK_SELECT;
    SetRikuDeckMenuHandAnim(work);
    LoadRikuDeckNameTexts(work);
    work->step = DECK_MENU_SLIDE_IN_STEP_VERTICAL;
    work->timer = 16;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuSlideIn);
    return 1;
}

u8 UpdateRikuDeckMenuSlideIn(RikuDeckMenuWork* work, void* task) {
    u8 n;

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
            n = --work->timer;

            if (n == 0) {
                LoadBgMap(3, gRikuDeckReviewGridMap, sizeof(gRikuDeckReviewGridMap));
                ReleaseObjTiles(work->tiles12);
                work->tiles12 = NULL;
                ReleaseObjTiles(work->tiles6);
                work->tiles6 = NULL;
                ReleaseObjPalette(work->palette3);
                work->palette3 = NULL;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuEnterDeckGrid);
            }

            break;
        }
    }

    return 1;
}

u8 UpdateRikuDeckMenuEnterDeckGrid(RikuDeckMenuWork* work, void* task) {
    work->handX = gDeckGridColumnX[work->cursorCol] << 8;
    work->handY = gDeckGridRowY[work->cursorRow] << 8;
    work->view = DECK_MENU_VIEW_DECK_GRID;
    SetRikuDeckMenuHandAnim(work);
    ShowRikuDeckCardPreview(work);
    work->handVisible = TRUE;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuDeckGrid);
    return 1;
}

u8 UpdateRikuDeckMenuDeckGrid(RikuDeckMenuWork* work, void* task) {
    u16 x;

    work->gfx = AnimUpdate(&work->anim2);
    work->gfx2 = AnimUpdate(&work->anim3);

    if (FadeIsActive()) {
        TaskPoolUpdate(&work->taskpool);
        return 1;
    }

    if (work->popupActive != 0) {
        ApproachValue(&work->handX, gDeckGridColumnX[work->cursorCol] << 8, work->timer);
        ApproachValue(&work->handY, gDeckGridRowY[work->cursorRow] << 8, work->timer);

        if (work->timer != 0) {
            work->timer--;
        }

        TaskPoolUpdate(&work->taskpool);
        TaskPoolUpdate(&work->cardpool);

        if (GetKeysPressed() & START_BUTTON) {
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
        if (CheckRikuDeckCpCost(work) && CheckRikuDeckHasAttackCard(work)) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuFadeOut);
            FadeStartOut(FADE_MODE_BLACK, 4);
            m4aSongNumStart(SONG_SYS_CANSEL);
            return 1;
        }

        work->exitRequested = FALSE;
    }

    switch (GetKeysRepeat()) {
    case DPAD_UP:
        if (work->cursorRow > 0) {
            (work->cursorRow)--;
            work->timer = 4;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        } else {
            ScrollRikuGridUp(work);
        }

        ShowRikuDeckCardPreview(work);
        break;
    case DPAD_DOWN:
        if (work->cursorRow <= 2) {
            (work->cursorRow)++;
            work->timer = 4;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        } else {
            ScrollRikuGridDown(work);

            if (work->holding) {
                if ((u16)work->heldRow <= 3) {
                    work->heldY = gDeckGridRowY[work->heldRow] << 8;
                } else {
                    work->heldY = 0xFFFF0000;
                }
            }
        }

        ShowRikuDeckCardPreview(work);
        break;
    case DPAD_LEFT:
        if (work->cursorCol > 0) {
            (work->cursorCol)--;
            work->timer = 4;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        ShowRikuDeckCardPreview(work);
        break;
    case DPAD_RIGHT:
        if (work->cursorCol <= 1) {
            (work->cursorCol)++;
            work->timer = 4;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        ShowRikuDeckCardPreview(work);
        break;
    case START_BUTTON:
        work->result = DECK_MENU_RESULT_RETURN_TO_MAP;
        m4aSongNumStart(SONG_SYS_CANSEL);
        FadeStartOut(FADE_MODE_BLACK, 4);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuFadeOut);
        return 1;
    case B_BUTTON:
        work->result = DECK_MENU_RESULT_RETURN_TO_MENU;
        m4aSongNumStart(SONG_SYS_CANSEL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuStartSlideOut);
        return 1;
    }

    work->cursorCard = GetRikuCardAtCursor(work);
    ApproachValue(&work->handX, gDeckGridColumnX[work->cursorCol] << 8, work->timer);
    ApproachValue(&work->handY, gDeckGridRowY[work->cursorRow] << 8, work->timer);

    if (work->timer != 0) {
        work->timer--;
    }

    work->prevCursorCard = work->cursorCard;
    x = work->cursorCol;
    work->prevCursorCol = x;
    x = work->cursorRow;
    work->prevCursorRow = x;
    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

s32 UpdateRikuDeckMenuFadeOut(RikuDeckMenuWork* work) {
    if (!FadeIsActive()) {
        return 0;
    }

    TaskPoolUpdate(&work->taskpool);
    TaskPoolUpdate(&work->cardpool);
    return 1;
}

u8 UpdateRikuDeckMenuStartSlideOut(RikuDeckMenuWork* work, void* task) {
#ifdef VERSION_EU
    work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTilesByLanguage[gLanguage], sRikuDeckTitleBannerTileSizes[gLanguage]);
#else
    work->tiles12 = LoadObjTiles(gRikuDeckTitleBannerTiles, sizeof(gRikuDeckTitleBannerTiles));
#endif
    work->tiles6 = LoadObjTiles(gDeckMenuBarTiles, sizeof(gDeckMenuBarTiles));
    work->palette3 = LoadObjPalette(gDeckTitleBannerPalette, sizeof(gDeckTitleBannerPalette));
    LoadBgMap(3, gRikuDeckMenuMap, sizeof(gRikuDeckMenuMap));
    work->topBarX = 0x7800;
    work->topBarY = 0;
    work->bottomBarX = 0xA400;
    work->bottomBarY = 0x9800;
    work->bannerX = 0;
    work->barSlideTimer = 16;
    work->bannerSlideTimer = 16;
    work->handVisible = FALSE;
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuSlideOut);
    return 1;
}

u8 UpdateRikuDeckMenuSlideOut(RikuDeckMenuWork* work, void* task) {
    u8* slideTimer;

    slideTimer = &work->bannerSlideTimer;

    if ((s8)*slideTimer > 0) {
        ApproachValue(&work->bannerX, -0x8000, (s8)*slideTimer);
        (*slideTimer)--;
    } else {
        slideTimer = &work->barSlideTimer;

        if ((s8)*slideTimer > 0) {
            ApproachValue(&work->topBarY, -0x800, (s8)*slideTimer);
            ApproachValue(&work->bottomBarY, 0xA000, (s8)*slideTimer);
            (*slideTimer)--;
        } else {
            FadeStartOut(FADE_MODE_BLACK, 4);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateRikuDeckMenuFadeOut);
        }
    }

    return 1;
}

static void Deckmenu2_2(RikuDeckMenuWork* work) {
    if (work->tiles12 != NULL) {
#ifdef VERSION_EU
        DrawSprite(work->bannerX >> 8, 0, gRikuDeckTitleBannerSpritesByLanguage[gLanguage][0], work->tiles12, work->palette3, NULL, 0, 10);
#else
        DrawSprite(work->bannerX >> 8, 0, gRikuDeckTitleBannerFrames[0], work->tiles12, work->palette3, NULL, 0, 10);
#endif
    }
#ifdef PLATFORM_PC
    /* the title's HD banner over the BG copy the menu keeps once it has slid in (its tiles released), as in Sora's */
    else if (memcmp(GetBgScreenBase(3), gRikuDeckReviewGridMap, 128) == 0) {
        int level = (int)(gFadeWork->amount >> 8) * 255 / 31;
        PortHdStillSprite(
#ifdef VERSION_EU
            gRikuDeckTitleBannerTilesByLanguage[gLanguage], gRikuDeckTitleBannerSpritesByLanguage[gLanguage][0],
#else
            gRikuDeckTitleBannerTiles, gRikuDeckTitleBannerFrames[0],
#endif
            work->bannerX >> 8, 0, gFadeWork->mode == FADE_MODE_BLACK ? level : 0,
            gFadeWork->mode == FADE_MODE_WHITE || gFadeWork->mode == FADE_MODE_ADD_WHITE ? level : 0);
    }
#endif

    if (work->tiles6 != NULL) {
        DrawSprite(work->topBarX >> 8, work->topBarY >> 8, gDeckMenuBarFrames[0], work->tiles6,
                   work->palette3, NULL, SPRITE_PRIORITY(3), 10000);
        DrawSprite(work->bottomBarX >> 8, work->bottomBarY >> 8, gDeckMenuBarFrames[1], work->tiles6,
                   work->palette3, NULL, SPRITE_PRIORITY(3), 10000);
    }

    if (work->handVisible) {
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 30, work->gfx,
                   work->tiles, work->palette, NULL, work->handFlags, 3);
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 20, work->gfx2,
                   work->tiles2, work->palette4, NULL, 0, 8);
    }

    DrawSprite(work->thumbX >> 8, work->thumbY >> 8, gDeckScrollThumbFrames[0], work->tiles4,
               work->palette, NULL, SPRITE_PRIORITY(2), 10);

    if (work->tiles7 != NULL) {
        DrawSprite(168, 86, work->gfx4, work->tiles7, work->palette5, NULL, 0, 20);
    }

    if (work->tiles8 != NULL) {
        DrawSprite(168, 86, work->gfx5, work->tiles8, work->palette6, NULL, 0, 21);
    }

    if (work->tiles9 != NULL) {
        DrawSprite(168, 86, work->gfx6, work->tiles9, work->palette5, NULL, 0, 19);
    }

    if (work->previewShown) {
        if (work->textSlotCount4 != 0) {
            DrawTextSlots(100, 112, work->textSlots4, work->palette4, 20, work->textSlotCount4);
        }

        DrawRikuCardDescription(work);
    }

    TaskPoolDraw(&work->taskpool);
    TaskPoolDraw(&work->cardpool);
}

static void Deckmenu2_3(RikuDeckMenuWork* work) {
#ifdef PLATFORM_PC
    PortClearBgTextTiles();
#endif
    ClearRikuCardGrid(work);
    // @bug Leaves the card preview's tiles allocated.
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette);
    ReleaseRikuCommandMenuGfx(work);

    if (work->tiles12 != NULL) {
        ReleaseObjTiles(work->tiles12);
    }

    if (work->tiles6 != NULL) {
        ReleaseObjTiles(work->tiles6);
    }

    if (work->palette3 != NULL) {
        ReleaseObjPalette(work->palette3);
    }

    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    FreeTextSlots(work->textSlots4, ARRAY_COUNT(work->textSlots4));
    FreeTextSlots(work->textSlots5, ARRAY_COUNT(work->textSlots5));
    ReleaseObjPalette(work->palette4);
    TaskPoolDestroy(&work->taskpool);
    TaskPoolDestroy(&work->cardpool);
    FreeRikuCollectionEntries(work);
    *work->resultOut = work->result;
}

void CreateRikuDeckGridCards(RikuDeckMenuWork* work, u8 categoryFilter) {
    DeckCard2Args args;
    u16* cards;
    u8 i;
    s8 x;
    s8 y;

    cards = GetDeck(work->deckIndex)->cards;
    x = 0;
    y = 0;

    for (i = 0; i < DECK_SIZE; i++) {
        if (cards[i] != CARD_NONE) {
            if (categoryFilter == CATEGORY_FILTER_DECK_ALL) {
                args.pool = &work->pool;
                args.cardId = gCardCollection[cards[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                args.col = x;
                args.row = y;
                args.panel = DECK_CARD2_PANEL_DECK;
                args.slot = &cards[i];
                TaskCreate(&work->taskpool, &gTaskDescDeckCard2, &args);
                x++;
            } else if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category == categoryFilter - 1) {
                args.pool = &work->pool;
                args.cardId = gCardCollection[cards[i]] & (CARD_ID_MASK | CARD_FLAG_PREMIUM);
                args.col = x;
                args.row = y;
                args.panel = DECK_CARD2_PANEL_DECK;
                args.slot = &cards[i];
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
}

void ClearRikuCardGrid(RikuDeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        node->done = TRUE;
        node = ListPoolNext(&node->node);
    }

    TaskPoolUpdate(&work->taskpool);
}

void ScrollRikuGridDown(RikuDeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    if ((s8)work->scrollRowEnd == 33) {
        return;
    }

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
}

u8 ScrollRikuGridUp(RikuDeckMenuWork* work) {
    DeckCard2Work* node;

    node = ListPoolFirst(&work->pool);

    if (node == NULL) {
        work->thumbY -= 0x300;

        if (work->thumbY < 0x2800) {
            work->thumbY = 0x2800;
            return FALSE;
        }

        return TRUE;
    }

    if (node->args.row == 0) {
        return FALSE;
    }

    do {
        node->args.row++;

        if (node->args.row > 3) {
            node->y = 0x20000;
            DeckCard2ReleaseGfx(node);
        }

        node = ListPoolNext(&node->node);
    } while (node != NULL);

    m4aSongNumStart(SONG_SYS_CLICKI04B);
    work->scrollRowEnd--;
    work->thumbY -= 0x300;

    if (work->thumbY < 0x2800) {
        work->thumbY = 0x2800;
    }

    return TRUE;
}

DeckCard2Work* GetRikuCardAtCursor(RikuDeckMenuWork* work) {
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

void DrawRikuDeckCategoryCount(u8 count, u8 category) {
    u8 digits[2];
    u8* base;

    digits[0] = count / 10;
    digits[1] = count - (u8)(count / 10) * 10;
    base = GetBgCharBase(3);
    PortCopyDeckCategoryDigit(&gDeckCategoryDigitTiles[(digits[0] + 1) * 32], base + (category * 64 + 0x360), 32);
    PortCopyDeckCategoryDigit(&gDeckCategoryDigitTiles[(digits[1] + 1) * 32], base + (category * 64 + 0x360) + 32, 32);
}

void SetRikuDeckMenuHandAnim(RikuDeckMenuWork* work) {
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
        AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
        work->handFlags &= ~SPRITE_FLAG_HFLIP;
        break;
    case DECK_MENU_VIEW_DECK_SELECT:
    case DECK_MENU_VIEW_COMMANDS:
        AnimStart(&work->anim2, 2, ANIM_FLAG_LOOP);
        handFlags = work->handFlags | SPRITE_FLAG_HFLIP;
        work->handFlags = handFlags;
        break;
    }
}

void DrawRikuDeckCardCount(u8 deck) {
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
        base = GetBgCharBase(1);
        break;
    case 1:
        base = GetBgCharBase(1);
        break;
    case 2:
        base = GetBgCharBase(1);
        break;
    }

#ifdef VERSION_EU
    PortCopyDeckDigit(&gDeckCountDigitTiles[(countDigits[0] + 1) * 32], base + 0x2C00, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(countDigits[1] + 1) * 32], base + 0x2C20, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(maxDigits[0] + 1) * 32], base + 0x2C40, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(maxDigits[1] + 1) * 32], base + 0x2C60, 32);
#else
    PortCopyDeckDigit(&gDeckCountDigitTiles[(countDigits[0] + 1) * 32], base + 0x20, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(countDigits[1] + 1) * 32], base + 0x40, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(maxDigits[0] + 1) * 32], base + 0x60, 32);
    PortCopyDeckDigit(&gDeckCountDigitTiles[(maxDigits[1] + 1) * 32], base + 0x80, 32);
#endif
}

void DrawRikuCardTotals() {
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

void LoadRikuDeckNameTexts(RikuDeckMenuWork* work) {
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    FreeTextSlots(work->textSlots2, ARRAY_COUNT(work->textSlots2));
    FreeTextSlots(work->textSlots3, ARRAY_COUNT(work->textSlots3));
    work->textSlotCount = LoadTextSlots(GetDeckName(0), work->textSlots);
    work->textSlotCount2 = LoadTextSlots(GetDeckName(1), work->textSlots2);
    work->textSlotCount3 = LoadTextSlots(GetDeckName(2), work->textSlots3);
}

void LoadRikuCardNameText(RikuDeckMenuWork* work, s32 id) {
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

void ReleaseRikuCardPreview(RikuDeckMenuWork* work) {
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

void ShowRikuDeckCardPreview(RikuDeckMenuWork* work) {
    DeckCard2Work* node;
    const CardDef* def;
    void* dst;
    u16 id;
    u32 defIndex;

    id = CARD_ID_NONE;
    node = ListPoolFirst(&work->pool);

    while (node != NULL) {
        if (node->args.row == work->cursorRow && node->args.col == work->cursorCol) {
            id = node->args.cardId;
            break;
        }

        node = ListPoolNext(&node->node);
    }

    ReleaseRikuCardPreview(work);

    if (id != CARD_ID_NONE) {
        if (id & CARD_FLAG_PREMIUM) {
            work->tiles10 = AllocObjTiles(0x280, NULL);
            SetObjTileSource(work->tiles10, gCardPremiumTiles);
            AnimInit(&work->anim, gCardPremiumAnims, gCardPremiumFrames);
            AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
            work->gfx3 = AnimGetGfx(&work->anim);
        }

        defIndex = id & CARD_ID_MASK;
        def = &gCardDefs[defIndex];
        work->tiles7 = LoadObjTiles(gCardBacks[def->category].tiles, 768);
        work->tiles8 = LoadObjTiles(def->tiles, 512);
        work->palette6 = LoadObjPalette(def->palette, 32);
        work->palette5 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
        work->gfx4 = gCardBacks[def->category].gfx;
        work->gfx5 = def->gfx;

        if (def->category != CARD_CATEGORY_ENEMY) {
            work->tiles9 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
            work->gfx6 = gCardValueDigitFrames[def->value];
        }

        DrawRikuCpCost(def->cpCost);
        dst = (void*)(BG_PLTT + 11 * PLTT_SIZE_4BPP);
        LoadPalette(&gCardCategoryPalettes[def->category * 16], dst, 32);
        LoadRikuCardNameText(work, defIndex);
        LoadRikuCardDescriptionText(work, defIndex);
        work->previewShown = TRUE;
    } else {
        DrawRikuCpCost(0);
        work->previewShown = FALSE;
    }
}

void DrawRikuCpCost(u8 cpCost) {
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

void FreeRikuCollectionEntries(RikuDeckMenuWork* work) {
    CardKindEntry** entries;
    u16 i;

    if (work->entries != NULL) {
        for (i = 0; i < work->entryCount; i++) {
            EwramFree(work->entries[i].indices);
        }

        entries = &work->entries;
        EwramFree(*entries);
        *entries = NULL;
    }
}

void ReleaseRikuCommandMenuGfx(RikuDeckMenuWork* work) {
    if (work->tiles3 != NULL) {
        ReleaseObjTiles(work->tiles3);
        ReleaseObjPalette(work->palette2);
        work->tiles3 = NULL;
        work->palette2 = NULL;
    }
}

void SetRikuDeckMenuFrameCursor(RikuDeckMenuWork* work, u8 frameCursor) {
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

u8 CheckRikuDeckCpCost(RikuDeckMenuWork* work) {
    if (GetDeckCpCost(GetActiveDeckIndex()) > gGameState.progression.cp) {
        TaskCreate(&work->cardpool, &gTaskDescDeckErrorCp, &work->popupActive);
        m4aSongNumStart(SONG_SYS_BEEP);
        return FALSE;
    }

    return TRUE;
}

u8 CheckRikuDeckHasAttackCard(RikuDeckMenuWork* work) {
    if (CountActiveDeckCardsOfCategory(CARD_CATEGORY_ATTACK) == 0) {
        m4aSongNumStart(SONG_SYS_BEEP);
        TaskCreate(&work->cardpool, &gTaskDescDeckErrorNoAttackCard, &work->popupActive);
        return FALSE;
    }

    return TRUE;
}

u8 FindRikuCardInDirection(RikuDeckMenuWork* work, s16 x, s16 y, u16 dir) {
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
        return FindRikuCardInDirection(work, x, y - 1, DPAD_UP);
    case DPAD_DOWN:
        return FindRikuCardInDirection(work, x, y + 1, DPAD_DOWN);
    case DPAD_LEFT:
        return FindRikuCardInDirection(work, x - 1, y, DPAD_LEFT);
    case DPAD_RIGHT:
        return FindRikuCardInDirection(work, x + 1, y, DPAD_RIGHT);
    }

    return FALSE;
}

#ifdef VERSION_EU
void* gRikuDeckTitleBannerTilesByLanguage[5] = { gRikuDeckTitleBannerTiles, gRikuDeckTitleBannerFrenchTiles, gRikuDeckTitleBannerGermanTiles, gRikuDeckTitleBannerItalianTiles, gRikuDeckTitleBannerSpanishTiles };

void** gRikuDeckTitleBannerSpritesByLanguage[5] = {
    gRikuDeckTitleBannerFrames,
    gRikuDeckTitleBannerFrenchFrames,
    gRikuDeckTitleBannerGermanFrames,
    gRikuDeckTitleBannerItalianFrames,
    gRikuDeckTitleBannerSpanishFrames,
};

u8* gRikuDeckEquipMarkerTilesByLanguage[5] = { gDeckEquipMarkerTiles, gDeckEquipMarkerFrenchTiles, gDeckEquipMarkerGermanTiles, gDeckEquipMarkerItalianTiles, gDeckEquipMarkerSpanishTiles };
#endif

TaskDesc gTaskDescDeckmenu2Riku = {
    "Deckmenu2",
    (TaskInitFunc)Deckmenu2_0,
    (TaskUpdateFunc)UpdateRikuDeckMenuLoadBgs,
    (TaskDrawFunc)Deckmenu2_2,
    (TaskDestroyFunc)Deckmenu2_3,
    sizeof(RikuDeckMenuWork),
};
