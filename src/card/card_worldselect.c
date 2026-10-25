/**
 * card_worldselect.c
 * Map Card Selection and Inventory
 */

#ifdef PLATFORM_PC
#include "engine.h"
#include "sprite.h"
#include "port/synthesis_requirement_hd.h"
#endif
#ifdef PLATFORM_PC
/* Door menu: the panel, cards and caption sit a few lines above the bottom
   bar instead of resting on it; the title and the bars stay in place. */
#define MAPSEL_RAISE 0 /* v1.5: the menu is on its own layer now, where the panel
                          keeps the cartridge's place; raising only the cursor,
                          hand and caption left them 8 lines off the cards */
#define MAPSEL_SHIFT(dy) (gPortSpriteShiftY = mapselBaseY + (dy))
#else
#define MAPSEL_RAISE 0
#define MAPSEL_SHIFT(dy) ((void)0)
#endif
#include "macros.h"
#include "registration_data.h"
#include "system_state.h"
#include "map_api.h"
#include "card_battle.h"
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
#include "taskpool.h"
#include "key.h"
#include "gba/syscall.h"
#include "malloc.h"
#include "card.h"
#include "sprites_map.h"
#include "sprites_worldselect.h"
#include "battle_backgrounds.h"
#include "sprites_card_pictures.h"
#include "sprites_card.h"
#include "sprites_deck_menu.h"
#include "gba/keys.h"
#include "songs.h"
#include "card_selection_data.h"
#include "battle_work.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "card_ui_types.h"
#include "field_state.h"
#include "fld_types.h"
#include "gba/defines.h"
#include "map.h"
#include "map_runtime.h"
#include "map_text_data.h"
#include "mode.h"
#include "player_progression_types.h"
#include "types.h"
#include "gba/macro.h"
#include "sprite_palettes.h"
#include <stddef.h>
#include "lockon.h"
#include "card_worldselect.h"
#include "default_bg_map.h"
#include "card_ids.h"
#include "card_label_data.h"
#include "card_message_data.h"
#include "jiminy_records_index_data.h"
#include "gba/io_reg.h"
#if defined(PLATFORM_PC) && defined(PORT_SYNTHESIS_TEST_FIXTURE)
#include "synthesis_requirement_fixture.inc"
#endif

#ifndef PORT_MENU_LEFT
#define PORT_MENU_LEFT(x) (x)
#define PORT_MENU_RIGHT(x) (x)
#endif
#ifndef PORT_BAND
#define PORT_BAND(draw) draw /* GBA: one band */
#endif

static TaskPool sModeWorldselectTasks;

static u8 sMapCardDelivered;

static void* sSelectedMapCard;

MapCardUiResources gMapCardUiResources EWRAM_COMMON(16);

u8 gMapCardCounts[270] EWRAM_COMMON(16);

void WORLDSELECT_0() {
    SetBgMode2();
    SetupBg(3, 0, 12, 0);
    SetupBg(2, 2, 28, 10);
    SetBgSize(3, BGCNT_AFF512x512);
    LoadBgTiles(3, gBtlBgMonstroTiles, sizeof(gBtlBgMonstroTiles));
    LoadBgPalette(3, gBtlBgMonstroPalette, sizeof(gBtlBgMonstroPalette));
#ifdef VERSION_EU
    LoadBgMapLz77(3, gBtlBgMonstroMap);
#else
    LoadBgMap(3, gBtlBgMonstroMap, sizeof(gBtlBgMonstroMap));
#endif
    SetBgAffine(3, 0, Q_8_8(1), Q_8_8(1), 0x10000, 0x16800);
    TaskPoolInit(&sModeWorldselectTasks, 1);
    TaskCreate(&sModeWorldselectTasks, &gTaskDescPremireChance, NULL);
}

void WORLDSELECT_1() {
    TaskPoolUpdate(&sModeWorldselectTasks);
    TaskPoolDraw(&sModeWorldselectTasks);
}

void WORLDSELECT_2() {
    TaskPoolDestroy(&sModeWorldselectTasks);
}

enum MapSelectPageScroll {
    MAP_SELECT_PAGE_SCROLL_NONE,
    MAP_SELECT_PAGE_SCROLL_NEXT,
    MAP_SELECT_PAGE_SCROLL_PREV
};

void MapSelect_0(MapSelectWork* work, u8* status) {
    s32 n;

    ResetMessageWindowFlags();
    CpuFill32(0, work, sizeof(MapSelectWork));
    work->status = status;
    *status = MAP_SELECT_STATUS_OPEN;
    work->messageTimer = 0;
    work->cancelled = FALSE;
    work->pageScroll = MAP_SELECT_PAGE_SCROLL_NONE;
    work->card2 = NULL;
    sMapCardDelivered = FALSE;
    work->valueColumn = 0;
    work->valueRow = 0;
    work->scrollBarVisible = FALSE;
    work->isEventDoor = SelectCurrentEventDoor();
    n = GetCurrentRoomCardValue() + 1;
    work->requiredValue = n;

    if (n == 10) {
        work->requiredValue = 0;
    }

    work->tiles3 = NULL;

    if (!work->isEventDoor) {
        if (work->requiredValue == 0) {
            work->tiles4 = AllocSpriteFrameTiles(0x80);
            UpdateSpriteFrameTiles(work->tiles4, gKeyValueFrames[0], gMapSelectRequirementTiles);
        } else {
            work->tiles4 = AllocSpriteFrameTiles(0x180);
            UpdateSpriteFrameTiles(work->tiles4, gKeyValueFrames[1], gMapSelectRequirementTiles);
            RequestDma3Copy(work->tiles4->src + (work->requiredValue << 7),
                           (void*)(OBJ_VRAM0 + ((work->tiles4->index + 4) << 5)), 0x80);
            RequestDma3Copy(work->tiles4->src + 0x500,
                           (void*)(OBJ_VRAM0 + (work->tiles4->index << 5)), 0x80);
        }

        work->tiles2 = LoadObjTiles(gCardBacks[CARD_BACK_WHITE].tiles2, 0x300);
        work->palette = LoadObjPalette(gDoorCardPalette, sizeof(gDoorCardPalette));
        FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    } else {
        work->tiles4 = NULL;
        work->tiles2 = NULL;
        work->palette = LoadObjPalette(gDoorCardPalette, sizeof(gDoorCardPalette));
        FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    }

    gMapCardUiResources.tiles = AllocObjTiles(0x280, NULL);
    SetObjTileSource(gMapCardUiResources.tiles, gCardPremiumTiles);
    AnimInit(&gMapCardUiResources.anim, gCardPremiumAnims, gCardPremiumFrames);
    AnimStart(&gMapCardUiResources.anim, 0, ANIM_FLAG_LOOP);
    gMapCardUiResources.gfx = AnimGetGfx(&gMapCardUiResources.anim);
    work->tiles = AllocObjTiles(0x3C0, NULL);
    work->palette2 = LoadObjPalette(gCardSelectBoxPalette, sizeof(gCardSelectBoxPalette));
    SetObjTileSource(work->tiles, gCardSelectBoxTiles);
    AnimInit(&work->anim, gCardSelectBoxAnims, gCardSelectBoxFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->tiles5 = AllocObjTiles(0x120, NULL);
    work->palette3 = LoadObjPalette(gSmallHandCursorPalette, sizeof(gSmallHandCursorPalette));
    SetObjTileSource(work->tiles5, gSmallHandCursorTiles);
    AnimInit(&work->anim2, gSmallHandCursorAnims, gSmallHandCursorFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->gfx2 = AnimGetGfx(&work->anim2);
    work->tiles7 = LoadObjTiles(gMapSelectBarTiles, sizeof(gMapSelectBarTiles));
    work->topBarY = -0x800;
    work->bottomBarY = 0xA000;
    work->barSteps = 16;
    work->slideSteps = 16;
    work->kindCount = CountOwnedMapCardKinds();
    TaskPoolInit(&work->tasks, work->kindCount + 10);
    ListPoolInit(&work->cards);
    work->kindEntries = EwramAlloc(work->kindCount * sizeof(MapSelectKindEntry));
    ListOwnedMapCardKinds(work->kindEntries);
    CreateMapSelectCards(work);
#ifdef VERSION_EU
    gMapCardUiResources.extraTiles = LoadObjTiles(gMapCardUiExtraTilesByLanguage[gLanguage], gMapCardUiExtraTileSizes[gLanguage]);
#else
    gMapCardUiResources.extraTiles = LoadObjTiles(gMapCardUiExtraTiles, sizeof(gMapCardUiExtraTiles));
#endif
    gMapCardUiResources.palette = work->palette3;
#ifdef VERSION_EU
    gMapCardUiResources.sprites = gMapCardUiSpritesByLanguage[gLanguage];
#else
    gMapCardUiResources.sprites = gMapCardUiExtraFrames;
#endif
    work->tiles6 = LoadObjTiles(gMapSelectScrollBarTiles, sizeof(gMapSelectScrollBarTiles));
    work->page = 0;
    work->lastPage = 0;
    work->card = ListPoolFirst(&work->cards);
    work->prevCard = NULL;
    m4aSongNumStart(SONG_SYS_CLICKI02);
    sSelectedMapCard = NULL;
    work->handX = 0x1600;
    work->handY = 0x16400;
    work->unk_28D[0] = 0;
    work->unk_260 = 0;
    work->cursorTargetX = 0x1600;
    work->cursorTargetY = 0x6400;
    work->unk_28D[1] = 0;
    work->steps = 0;

    if (work->card != NULL) {
        work->x = work->card->x;
        work->y = work->card->y;
        work->card->flags |= MAPCARD_FLAG_CURSOR;
    } else {
        work->x = -0x6400;
        work->y = -0x6400;
    }

    work->titleX = -0xA000;
    work->titleY = 0;
    work->palette4 = LoadTextPalette(1);
    work->textSlotCounts[0] = 0;
    work->textSlotCounts[1] = 0;
    work->textSlotCounts[2] = 0;
    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));

    if (work->card != NULL) {
        work->prevCard = work->card;
        work->textSlotCounts[0] = LoadTextSlots(GetRoomName(gMapCardDefs[work->card->args.baseCardId].kind), work->textSlots);
    }

    work->nameY = 0x19100;
    FadeSetPaletteExcluded(work->palette3->index + 16, TRUE);
    FadeSetPaletteExcluded(work->palette2->index + 16, TRUE);
    FadeSetPaletteExcluded(15, TRUE);
    FadeSetPaletteExcluded(work->palette4->index + 16, TRUE);
    work->unk_1E4 = NULL;
    work->mosaicX = 9;
    work->mosaicY = 9;
    work->mosaicTimer = 0;
    work->inTutorial = FALSE;
}

u8 MapSelect_1(MapSelectWork* work, void* task) {
#ifdef PLATFORM_PC
    PortScreenBg(1, TRUE);
#endif
#ifdef VERSION_EU
    LoadBgTiles(1, gMapSelectTiles, sizeof(gMapSelectTiles));

    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
        work->tiles3 = LoadObjTiles(gMapSelectTitleTiles, sizeof(gMapSelectTitleTiles));
        break;
    case LANGUAGE_FRENCH:
        work->tiles3 = LoadObjTiles(gMapSelectTitleFrenchTiles, sizeof(gMapSelectTitleFrenchTiles));
        RequestDma3Copy(gMapSelectCardInfoFrenchTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoFrenchTiles));
        break;
    case LANGUAGE_GERMAN:
        work->tiles3 = LoadObjTiles(gMapSelectTitleGermanTiles, sizeof(gMapSelectTitleGermanTiles));
        RequestDma3Copy(gMapSelectCardInfoGermanTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoGermanTiles));
        break;
    case LANGUAGE_ITALIAN:
        work->tiles3 = LoadObjTiles(gMapSelectTitleItalianTiles, sizeof(gMapSelectTitleItalianTiles));
        RequestDma3Copy(gMapSelectCardInfoItalianTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoItalianTiles));
        break;
    case LANGUAGE_SPANISH:
        work->tiles3 = LoadObjTiles(gMapSelectTitleSpanishTiles, sizeof(gMapSelectTitleSpanishTiles));
        RequestDma3Copy(gMapSelectCardInfoSpanishTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoSpanishTiles));
        break;
    }
#else
    work->tiles3 = LoadObjTiles(gMapSelectTitleTiles, sizeof(gMapSelectTitleTiles));
    LoadBgTiles(1, gMapSelectTiles, sizeof(gMapSelectTiles));
#endif
    LoadPalette(gMapSelectBgPalettes, (void*)(BG_PLTT + 12 * PLTT_SIZE_4BPP), 32);
    LoadPalette(&gMapSelectBgPalettes[0x40], (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), 64);
    FadeSetPaletteExcluded(12, TRUE);
    FadeSetPaletteExcluded(14, TRUE);
    FadeSetPaletteExcluded(15, TRUE);
    work->messageTimer++;
    DisableBg(1);
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectSetup);
    return 1;
}

u8 UpdateMapSelectSetup(MapSelectWork* work, void* task) {
    s32 n;

    SetBgMapBlocks(1, gMapSelectBgMapBlocks, 1, 2);
    work->bgScrollY = 0;
    ScrollBgMapTo(1, 0, MAPSEL_RAISE);
    EnableBg(1);

    if (work->isEventDoor == TRUE) {
        work->remainingKeys = CountRemainingEventKeys();
        work->nextEventKey = GetEventKey(0);
        n = work->remainingKeys;

        while (n != 0) {
            n--;
        }

        work->eventKeyArgs.palette = work->palette3;
        work->eventKeyArgs.closeMode = SELMAP_EVENT_KEY_CLOSE_NONE;
        work->eventKey =
            TaskCreate(&work->tasks, &gTaskDescSELMAPEVKEY, &work->eventKeyArgs)->work;
    }

    // @bug card can be NULL (NULL read).
    LoadMapSelectKindPalette(BIOS_PTR(work->card)->args.baseCardId, work);
#if defined(PLATFORM_PC) && defined(PORT_SYNTHESIS_TEST_FIXTURE)
    SynthesisFixtureBegin();
#endif
    SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectSlideIn);
    return 1;
}

u8 UpdateMapSelectSlideIn(MapSelectWork* work, void* task) {
    MapcardWork* node;

    if (work->barSteps != 0) {
        ApproachValue(&work->topBarY, 0, work->barSteps);
        ApproachValue(&work->bottomBarY, 0x9800, work->barSteps);
        work->barSteps--;
    } else {
        ApproachValue(&work->bgScrollY, 0x10000, work->slideSteps);
        ApproachValue(&work->titleX, 0, work->slideSteps);
        ScrollBgMapTo(1, 0, ((u32)work->bgScrollY >> 8) + MAPSEL_RAISE);

        if (work->slideSteps != 0) {
            work->slideSteps--;
        } else {
            if ((gGameState.progression.tutorialFlags & TUTORIAL_FLAG_MAP_SELECT) == 0) {
                work->inTutorial = TRUE;
                ResetMessageWindowFlags();
                work->tutorialMessage = CARD_MSG_MAP_SELECT_TUTORIAL_0;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectTutorial);
            } else if ((gGameState.progression.tutorialFlags & TUTORIAL_FLAG_MAP_SELECT_EVENT_DOOR) == 0 && work->isEventDoor == TRUE) {
                ResetMessageWindowFlags();
                work->tutorialMessage = CARD_MSG_MAP_SELECT_EVENT_DOOR_TUTORIAL_0;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectEventDoorTutorial);
                gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_MAP_SELECT_EVENT_DOOR;
            } else {
                node = ListPoolFirst(&work->cards);
                SetBgMapBlocks(1, gMapSelectBgMapBlocks, 1, 2);
#ifdef PLATFORM_PC
                ScrollBgMapTo(1, 0, ((u32)work->bgScrollY >> 8) + MAPSEL_RAISE); /* the panel keeps its place */
#endif

                while (node != NULL) {
                    node->flags |= MAPCARD_FLAG_RAISED;
                    node = ListPoolNext(&node->node);
                }

                if (work->kindCount <= 6) {
                    work->lastPage = 0;
                } else {
                    work->lastPage = work->kindCount / 6;

#ifdef VERSION_EU
                    if ((u16)(work->kindCount % 6) == 0) {
                        work->lastPage--;
                    }
#endif
                }

                work->handY = 0x6400;
                work->y = 0x7A00;
                work->nameY = 0x9100;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectKindInput);
                work->scrollBarVisible = TRUE;
            }
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateMapSelectEnterValues(MapSelectWork* work, void* task) {
    s8 firstValue;

    if (work->card->steps == 0) {
        LoadBgTiles(1, gMapSelectValuesTiles, sizeof(gMapSelectValuesTiles));

#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            break;
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gMapSelectCardInfoFrenchTiles, (u8*)GetBgCharBase(1) + 0x1EA0, sizeof(gMapSelectCardInfoFrenchTiles));
            break;
        case LANGUAGE_GERMAN:
            RequestDma3Copy(gMapSelectCardInfoGermanTiles, (u8*)GetBgCharBase(1) + 0x1EA0, sizeof(gMapSelectCardInfoGermanTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gMapSelectCardInfoItalianTiles, (u8*)GetBgCharBase(1) + 0x1EA0, sizeof(gMapSelectCardInfoItalianTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gMapSelectCardInfoSpanishTiles, (u8*)GetBgCharBase(1) + 0x1EA0, sizeof(gMapSelectCardInfoSpanishTiles));
            break;
        }
#endif

        LoadBgMap(1, gMapSelectValuesMap, sizeof(gMapSelectValuesMap));
        LoadMapSelectGridPalette(work->card->args.baseCardId, work);
        firstValue = LoadMapSelectValueCounts(work->card->args.baseCardId, work);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectValueInput);
        ReleaseObjTiles(work->tiles);
        work->tiles = AllocObjTiles(0x1E0, NULL);
        SetObjTileSource(work->tiles, gMapSelectValueBoxTiles);
        AnimInit(&work->anim, gMapSelectValueBoxAnims, gMapSelectValueBoxFrames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(&work->anim);
        work->savedCursorX = work->handX >> 8;
        work->savedCursorY = work->handY >> 8;

        if (firstValue > 4) {
            work->valueColumn = firstValue - 5;
            work->valueRow = 1;
        } else {
            work->valueColumn = firstValue;
            work->valueRow = 0;
        }

        work->steps = 4;
        work->steps2 = 4;
        work->scrollBarVisible = FALSE;
    }

    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->mosaicTimer == 2) {
        if (work->mosaicX != 0) {
            work->mosaicX--;
        }

        if (work->mosaicY != 0) {
            work->mosaicY--;
        }

        work->mosaicTimer = 0;
    }

    work->mosaicTimer++;
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateMapSelectValueInput(MapSelectWork* work, void* task) {
    u16 keys;
    s16 sel;
    u8 n;

    keys = GetKeysPressed();
    sel = work->valueColumn + work->valueRow * 5;

    if ((gGameState.progression.tutorialFlags & TUTORIAL_FLAG_MAP_SELECT) == 0) {
        if (work->steps == 0) {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectValueTutorial);
            TaskPoolUpdate(&work->tasks);
            return 1;
        }

        keys = 0;
    }

    switch (keys & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON | L_BUTTON | R_BUTTON)) {
    case B_BUTTON:
        LoadBgTiles(1, gMapSelectTiles, sizeof(gMapSelectTiles));
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            break;
        case LANGUAGE_FRENCH:
            RequestDma3Copy(gMapSelectCardInfoFrenchTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoFrenchTiles));
            break;
        case LANGUAGE_GERMAN:
            RequestDma3Copy(gMapSelectCardInfoGermanTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoGermanTiles));
            break;
        case LANGUAGE_ITALIAN:
            RequestDma3Copy(gMapSelectCardInfoItalianTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoItalianTiles));
            break;
        case LANGUAGE_SPANISH:
            RequestDma3Copy(gMapSelectCardInfoSpanishTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoSpanishTiles));
            break;
        }
#endif
        LoadBgMap(1, gMapSelectMap, sizeof(gMapSelectMap));
        work->card->flags &= ~MAPCARD_FLAG_OPENED;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectLeaveValues);
        m4aSongNumStart(SONG_SYS_CLOSE);
        break;
    case A_BUTTON:
        n = DoorAcceptsMapCard((struct MapCardAttributes*)&gMapCardDefs[work->card->args.baseCardId + sel].kind);

        if (n == 1) {
            if (gMapCardCounts[work->card->args.baseCardId + sel] != 0) {
                if (work->isEventDoor == TRUE) {
                    m4aSongNumStart(SONG_SYS_KETEI2);

                    if (CountMapCardsOfKind(work->card->args.baseCardId) == 0) {
                        work->card->value = sel;
                        work->card->flags |= MAPCARD_FLAG_CHOSEN;
                    }

                    RemoveMapCard(work->card->args.baseCardId + sel);

                    if ((u8)PayEventKey((struct MapCardAttributes*)&gMapCardDefs[work->card->args.baseCardId + sel].kind) == TRUE) {
                        work->remainingKeys = CountRemainingEventKeys();
                        work->eventKey->paidCount++;
                        work->eventKey->slideSteps = 8;

                        if (work->remainingKeys == 0) {
                            work->card->value = sel;
                            work->card->flags |= MAPCARD_FLAG_CHOSEN;
                            *work->status = MAP_SELECT_STATUS_CARD_CHOSEN;
                            work->slideSteps = 16;
                            work->barSteps = 16;
                            work->lastPage = 0;
                            work->page = 0;
                            work->eventKeyArgs.closeMode = n;
                            SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectClose);
                            return 1;
                        }
                    }

                    LoadMapSelectKindPalette(work->card->args.baseCardId, work);
                    LoadMapSelectGridPalette(work->card->args.baseCardId, work);

                    if ((s8)LoadMapSelectValueCounts(work->card->args.baseCardId, work) == -1) {
                        LoadBgTiles(1, gMapSelectTiles, sizeof(gMapSelectTiles));
#ifdef VERSION_EU
                        switch (gLanguage) {
                        case LANGUAGE_ENGLISH:
                            break;
                        case LANGUAGE_FRENCH:
                            RequestDma3Copy(gMapSelectCardInfoFrenchTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoFrenchTiles));
                            break;
                        case LANGUAGE_GERMAN:
                            RequestDma3Copy(gMapSelectCardInfoGermanTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoGermanTiles));
                            break;
                        case LANGUAGE_ITALIAN:
                            RequestDma3Copy(gMapSelectCardInfoItalianTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoItalianTiles));
                            break;
                        case LANGUAGE_SPANISH:
                            RequestDma3Copy(gMapSelectCardInfoSpanishTiles, (u8*)GetBgCharBase(1) + 0x1AA0, sizeof(gMapSelectCardInfoSpanishTiles));
                            break;
                        }
#endif
                        LoadBgMap(1, gMapSelectMap, sizeof(gMapSelectMap));
                        work->card->flags &= ~MAPCARD_FLAG_OPENED;
                        RemoveMapSelectCard(work);
                        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectLeaveValues);
                    }

                    break;
                }

                work->card->value = sel;
                work->card->flags |= MAPCARD_FLAG_CHOSEN;
                RemoveMapCard(work->card->args.baseCardId + sel);
                *work->status = MAP_SELECT_STATUS_CARD_CHOSEN;
                work->slideSteps = 16;
                work->barSteps = 16;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectClose);
                m4aSongNumStart(SONG_SYS_KETEI2);
                work->lastPage = 0;
                work->page = 0;
                work->eventKeyArgs.closeMode = n;
                return 1;
            }
        }

        m4aSongNumStart(SONG_SYS_BEEP);
        break;
    }

    work->gfx2 = AnimUpdate(&work->anim2);
    work->gfx = AnimUpdate(&work->anim);
    gMapCardUiResources.gfx = AnimUpdate(&gMapCardUiResources.anim);
    TaskPoolUpdate(&work->tasks);
    HandleMapSelectValueCursor(work);
    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->mosaicTimer == 2) {
        if (work->mosaicX != 0) {
            work->mosaicX--;
        }

        if (work->mosaicY != 0) {
            work->mosaicY--;
        }

        work->mosaicTimer = 0;
    }

    work->mosaicTimer++;
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateMapSelectLeaveValues(MapSelectWork* work, void* task) {
    MapcardWork* node;

    if (CountOwnedMapCardKinds() == 0) {
        return 0;
    }

    if (work->card->steps == 0) {
        node = ListPoolFirst(&work->cards);

        while (node != NULL) {
            if (work->card != node) {
                node->flags |= MAPCARD_FLAG_RAISED;
            }

            node = ListPoolNext(&node->node);
        }

        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectKindInput);
        ReleaseObjTiles(work->tiles);
        work->tiles = AllocObjTiles(0x3C0, NULL);
        SetObjTileSource(work->tiles, gCardSelectBoxTiles);
        AnimInit(&work->anim, gCardSelectBoxAnims, gCardSelectBoxFrames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(&work->anim);
        work->handX = work->savedCursorX << 8;
        work->handY = work->savedCursorY << 8;
        work->x = work->card->x;
        work->y = work->card->y;
        work->scrollBarVisible = TRUE;
    }

    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->mosaicTimer == 2) {
        if (work->mosaicX != 0) {
            work->mosaicX--;
        }

        if (work->mosaicY != 0) {
            work->mosaicY--;
        }

        work->mosaicTimer = 0;
    }

    work->mosaicTimer++;
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateMapSelectKindInput(MapSelectWork* work, void* task) {
    u16 keys = GetKeysPressed();
    MapcardWork* node;

    ApplyMapSelectPageScroll(work);
    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->mosaicTimer == 2) {
        if (work->mosaicX != 0) {
            work->mosaicX--;
        }

        if (work->mosaicY != 0) {
            work->mosaicY--;
        }

        work->mosaicTimer = 0;
    }

    work->mosaicTimer++;

    switch (keys & (A_BUTTON | B_BUTTON | SELECT_BUTTON | START_BUTTON | L_BUTTON | R_BUTTON)) {
    case A_BUTTON:
        if (work->card != NULL) {
            if (work->card->args.baseCardId >= MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_BEGINNINGS, 0)) {
                if (DoorAcceptsMapCard((struct MapCardAttributes*)&gMapCardDefs[work->card->args.baseCardId + 1].kind) == 1) {
                    if (gMapCardCounts[work->card->args.baseCardId + 1] != 0) {
                        m4aSongNumStart(SONG_SYS_KETEI2);
                        RemoveMapCard(work->card->args.baseCardId + 1);

                        if ((u8)PayEventKey((struct MapCardAttributes*)&gMapCardDefs[work->card->args.baseCardId + 1].kind) == TRUE) {
                            work->remainingKeys = CountRemainingEventKeys();
                            work->eventKey->paidCount++;
                            work->eventKey->slideSteps = 8;

                            if (work->remainingKeys == 0) {
                                work->card->value = 1;
                                work->card->flags |= MAPCARD_FLAG_CHOSEN;

                                for (node = ListPoolFirst(&work->cards); node != NULL; node = ListPoolNext(&node->node)) {
                                    if (work->card != node) {
                                        node->flags &= ~MAPCARD_FLAG_RAISED;
                                    }
                                }

                                *work->status = MAP_SELECT_STATUS_CARD_CHOSEN;
                                work->slideSteps = 16;
                                work->barSteps = 16;
                                work->lastPage = 0;
                                work->page = 0;
                                work->eventKeyArgs.closeMode = SELMAP_EVENT_KEY_CLOSE_ACCEPTED;
                                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectClose);
                                return 1;
                            }
                        }

                        break;
                    }
                }
            } else {
                if (CountMapCardsOfKind(work->card->args.baseCardId) != 0) {
                    for (node = ListPoolFirst(&work->cards); node != NULL; node = ListPoolNext(&node->node)) {
                        if (work->card != node) {
                            node->flags &= ~MAPCARD_FLAG_RAISED;
                        }
                    }

                    if (work->card != NULL) {
                        work->openedCardX = work->card->x;
                        work->card->flags |= MAPCARD_FLAG_OPENED;
                    }

                    m4aSongNumStart(SONG_SYS_KETTEI);
                    SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectEnterValues);
                    return 1;
                }
            }
        }

        m4aSongNumStart(SONG_SYS_BEEP);
        break;
    case B_BUTTON:
        if (!work->inTutorial) {
            for (node = ListPoolFirst(&work->cards); node != NULL; node = ListPoolNext(&node->node)) {
                node->flags &= ~MAPCARD_FLAG_RAISED;
            }

            work->cancelled = TRUE;
            work->slideSteps = 16;
            work->barSteps = 16;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectClose);
            m4aSongNumStart(SONG_SYS_CANSEL);
            sSelectedMapCard = NULL;
            work->lastPage = 0;
            work->page = 0;
            work->eventKeyArgs.closeMode = SELMAP_EVENT_KEY_CLOSE_CANCELLED;
        } else {
            m4aSongNumStart(SONG_SYS_BEEP);
        }

        break;
    }

    HandleMapSelectKindCursor(work);

    if (work->pageScroll == MAP_SELECT_PAGE_SCROLL_NONE) {
        if (work->card != work->prevCard) {
            if (work->card != NULL) {
                work->textSlotCounts[0] = LoadTextSlots(GetRoomName(gMapCardDefs[work->card->args.baseCardId].kind), work->textSlots);
                LoadMapSelectKindPalette(work->card->args.baseCardId, work);
            } else {
                work->textSlotCounts[0] = 0;
                work->textSlotCounts[1] = 0;
                work->textSlotCounts[2] = 0;
            }

            work->prevCard = work->card;
            work->steps = 4;
        } else if (work->card != NULL) {
            ApproachValue(&work->x, work->card->x, work->steps);
            ApproachValue(&work->y, work->card->y, work->steps);

            if (work->steps != 0) {
                work->steps--;
            }
        }
    }

    work->gfx2 = AnimUpdate(&work->anim2);
    work->gfx = AnimUpdate(&work->anim);
    gMapCardUiResources.gfx = AnimUpdate(&gMapCardUiResources.anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateMapSelectClose(MapSelectWork* work) {
    ApproachValue(&work->bgScrollY, 0, work->slideSteps);
    ApproachValue(&work->titleX, -0xA000, work->slideSteps);
    ApproachValue(&work->handY, 0x16400, work->slideSteps);
    ApproachValue(&work->y, 0x17A00, work->slideSteps);
    ApproachValue(&work->nameY, 0x19100, work->slideSteps);
    ScrollBgMapTo(1, 0, ((u32)work->bgScrollY >> 8) + MAPSEL_RAISE);
    work->scrollBarVisible = FALSE;

    if (work->mosaicX <= 8) {
        work->mosaicX++;
    }

    if (work->mosaicY <= 8) {
        work->mosaicY++;
    }

    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->slideSteps != 0) {
        work->slideSteps--;
    } else if (work->barSteps != 0) {
        ApproachValue(&work->topBarY, -0x800, work->barSteps);
        ApproachValue(&work->bottomBarY, 0xA000, work->barSteps);
        work->barSteps--;
    } else {
        if (work->cancelled || (work->card->flags & MAPCARD_FLAG_DELIVERED)) {
            return 0;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void MapSelect_2(MapSelectWork* work) {
#ifdef PLATFORM_PC
    PortScreenBegin(); /* menu: drawn on the screen layer, at the window's height */
    int mapselBaseY = gPortSpriteShiftY;
#endif
    if (work->tiles3 != NULL && work->palette3 != NULL) {
        DrawSprite(PORT_MENU_LEFT(work->titleX >> 8), work->titleY >> 8,
#ifdef VERSION_EU
                   gMapSelectTitleSpritesByLanguage[gLanguage][0],
#else
                   gMapSelectTitleFrames[0],
#endif
                   work->tiles3, work->palette3, NULL, 0, 80);
    }
    MAPSEL_SHIFT(-MAPSEL_RAISE); /* the panel and cards, not the title or the bars */

    if (work->isEventDoor == TRUE) {
        switch (gMapCardDefs[work->card->args.baseCardId].backIndex) {
        case MAP_CARD_COLOR_NONE:
            break;
        case MAP_CARD_COLOR_RED:
            DrawSprite((work->card->x >> 8) + 4, (work->card->y >> 8) - 32, gMapCardUiResources.sprites[3], gMapCardUiResources.extraTiles, gMapCardUiResources.palette, NULL, 0, 20);
            break;
        case MAP_CARD_COLOR_BLUE:
            DrawSprite((work->card->x >> 8) + 4, (work->card->y >> 8) - 32, gMapCardUiResources.sprites[7], gMapCardUiResources.extraTiles, gMapCardUiResources.palette, NULL, 0, 20);
            break;
        case MAP_CARD_COLOR_GREEN:
            DrawSprite((work->card->x >> 8) + 4, (work->card->y >> 8) - 32, gMapCardUiResources.sprites[5], gMapCardUiResources.extraTiles, gMapCardUiResources.palette, NULL, 0, 20);
            break;
        case MAP_CARD_COLOR_GOLD:
            break;
        }
    }

    if (!IsMessageWindowOpen()) {
        DrawSprite((work->x >> 8) - 23, (work->y >> 8) - 27, work->gfx, work->tiles, work->palette2, NULL, 0, 41);
        DrawSprite((work->handX >> 8) - 16, (work->handY >> 8) - 12, work->gfx2, work->tiles5, work->palette3, NULL, 0, 40);
    }

    if (work->scrollBarVisible
#ifdef PLATFORM_PC
        && work->lastPage > 0 /* One page has no scroll range. */
#endif
    ) {
        DrawSprite(224,
#ifdef VERSION_EU
                   (18 / work->lastPage) * work->page + 108,
#else
                   (17 / work->lastPage) * work->page + 108,
#endif
                   gDeckScrollThumbFrames[0], work->tiles6, work->palette, NULL, 0, 40);
    }

    MAPSEL_SHIFT(0); /* the card under the spotlight stays with the light */
    if (!work->isEventDoor && work->mosaicX != 9 && work->mosaicY != 9) {
#if defined(PLATFORM_PC) && defined(PORT_SYNTHESIS_TEST_FIXTURE)
        if (!SynthesisFixtureDraw()) {
#endif
#ifdef PLATFORM_PC
        int synthesisFirstEntry = gSpriteWork->entryCount;
#endif
        DrawSprite(120, 56, NULL, work->tiles4, work->palette, NULL, SPRITE_FLAG_MOSAIC, 60);
        DrawSprite(120, 56, gCardBacks[CARD_BACK_WHITE].gfx2, work->tiles2, work->palette, NULL, SPRITE_FLAG_MOSAIC, 60);
#ifdef PLATFORM_PC
        PortSynthesisHdTagRoom(work->tiles2, work->tiles4, work->requiredValue, synthesisFirstEntry);
#endif
#if defined(PLATFORM_PC) && defined(PORT_SYNTHESIS_TEST_FIXTURE)
        }
#endif
    }

    PORT_BAND(DrawSprite(128, work->topBarY >> 8, gMapSelectBarFrames[0], work->tiles7, work->palette3, NULL, SPRITE_PRIORITY(2), 80));
    PORT_BAND(DrawSprite(128, work->bottomBarY >> 8, gMapSelectBarFrames[1], work->tiles7, work->palette3, NULL, SPRITE_PRIORITY(2), 80));
    MAPSEL_SHIFT(-MAPSEL_RAISE);
#ifdef PLATFORM_PC
    /* The tutorial owns the caption area, as it owns the hand/highlight.
       Suppress the underlying card name for both pixel and HD text. */
    if (!IsMessageWindowOpen())
#endif
        DrawTextSlots(16, work->nameY >> 8, work->textSlots, work->palette4, 50, work->textSlotCounts[0]);
    TaskPoolDraw(&work->tasks);
    MAPSEL_SHIFT(0);
#ifdef PLATFORM_PC
    PortScreenEnd();
#endif
}

void MapSelect_3(MapSelectWork* work) {
#if defined(PLATFORM_PC) && defined(PORT_SYNTHESIS_TEST_FIXTURE)
    SynthesisFixtureEnd();
#endif
#ifdef PLATFORM_PC
    PortScreenBg(1, FALSE);
#endif
    TaskPoolDestroy(&work->tasks);

    if (work->kindEntries != NULL) {
        EwramFree(work->kindEntries);
    }

    FadeSetPaletteExcluded(work->palette3->index + 16, FALSE);
    FadeSetPaletteExcluded(work->palette2->index + 16, FALSE);
    FadeSetPaletteExcluded(15, FALSE);
    FadeSetPaletteExcluded(work->palette4->index + 16, FALSE);
    ReleaseObjTiles(work->tiles5);
    ReleaseObjPalette(work->palette3);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles3);

    if (work->tiles4 != NULL) {
        ReleaseObjTiles(work->tiles4);
    }

    if (work->tiles2 != NULL) {
        ReleaseObjTiles(work->tiles2);
    }

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
    }

    ReleaseObjTiles(work->tiles7);
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    ReleaseObjPalette(work->palette4);
    *work->status = MAP_SELECT_STATUS_CLOSED;
    ReleaseObjTiles(work->tiles6);
    ReleaseObjTiles(gMapCardUiResources.tiles);
    ReleaseObjTiles(gMapCardUiResources.extraTiles);
}

void CreateMapSelectCards(MapSelectWork* work) {
    MapcardArgs args;
    MapSelectKindEntry* kindEntries;
    u16 i;

    for (i = 0; i < work->kindCount; i++) {
        kindEntries = work->kindEntries;
        args.baseCardId = kindEntries[i].baseCardId;
        args.index = i;
        args.kindCount = work->kindCount;
        args.count = kindEntries[i].count;
        args.pool = &work->cards;
        args.parent = work;
        CreateMapCard(&args, &work->tasks);
    }
}

u16 CountOwnedMapCardKinds() {
    u16 count;
    u16 i;
    u16 j;

    count = 0;

    for (j = 0; j < 27; j++) {
        for (i = j * 10; i < j * 10 + 10; i++) {
            if (gMapCardCounts[i] != 0) {
                count++;
                break;
            }
        }
    }

    return count;
}

void ListOwnedMapCardKinds(MapSelectKindEntry* out) {
    u16 i;
    u16 j;

    for (i = 0; i <= 26; i++) {
        j = i * 10;

        while (j < i * 10 + 10) {
            if (gMapCardCounts[j] != 0) {
                out->baseCardId = i * 10;
                out->count = gMapCardCounts[j];
                out++;
                break;
            }

            j++;
        }
    }
}

void HandleMapSelectKindCursor(MapSelectWork* work) {
    MapcardWork* firstLoaded;
    MapcardWork* node;
    MapcardWork* found;
    MapcardWork* target;
    MapcardWork* cur;
    s8 cnt;

    if (work->pageScroll != MAP_SELECT_PAGE_SCROLL_NONE) {
        return;
    }

    if (work->card == NULL) {
        return;
    }

    switch (GetKeysRepeat() & DPAD_ANY) {
    case DPAD_RIGHT:
        target = ListPoolNext(&work->card->node);

        if (target != NULL && (target->flags & MAPCARD_FLAG_GFX_LOADED)) {
            work->cursorTargetX = target->x;
            work->steps2 = 4;
            work->card->flags &= ~MAPCARD_FLAG_CURSOR;
            work->card = ListPoolNext(&work->card->node);
            work->card->flags |= MAPCARD_FLAG_CURSOR;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        break;
    case DPAD_LEFT:
        target = ListPoolPrev(&work->card->node);

        if (target != NULL && (target->flags & MAPCARD_FLAG_GFX_LOADED)) {
            work->cursorTargetX = target->x;
            work->steps2 = 4;
            work->card->flags &= ~MAPCARD_FLAG_CURSOR;
            work->card = ListPoolPrev(&work->card->node);
            work->card->flags |= MAPCARD_FLAG_CURSOR;
            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        break;
    case DPAD_DOWN:
        firstLoaded = ListPoolFirst(&work->cards);
        node = NULL;
        found = NULL;
        cnt = 0;

        while (firstLoaded != NULL) {
            if (firstLoaded->flags & MAPCARD_FLAG_GFX_LOADED) {
                node = firstLoaded;
                break;
            }

            firstLoaded = ListPoolNext(&firstLoaded->node);
        }

        for (;;) {
            if (node != NULL) {
                if (node->flags & MAPCARD_FLAG_GFX_LOADED) {
                    cnt++;
                    node = ListPoolNext(&node->node);
                    continue;
                }

                if (cnt == 6) {
                    found = node;
                }
            }

            break;
        }

        if (found != NULL) {
            work->card2 = found;
            work->pageScroll = MAP_SELECT_PAGE_SCROLL_NEXT;

            if (firstLoaded != NULL) {
                do {
                    if (firstLoaded->flags & MAPCARD_FLAG_GFX_LOADED) {
                        firstLoaded->x = -0x6400;
                        firstLoaded->flags &= ~MAPCARD_FLAG_CURSOR;
                        firstLoaded = ListPoolNext(&firstLoaded->node);
                    } else {
                        break;
                    }
                } while (firstLoaded != NULL);
            }

            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        node = ListPoolFirst(&work->cards);

        while (node != NULL) {
            if (node->x == -0x6400) {
                // @bug Clears firstLoaded's flag instead of node's.
                firstLoaded->flags &= ~MAPCARD_FLAG_CURSOR;
            }

            node = ListPoolNext(&node->node);
        }

        break;
    case DPAD_UP:
        firstLoaded = ListPoolFirst(&work->cards);
        node = NULL;

        while (firstLoaded != NULL) {
            if (firstLoaded->flags & MAPCARD_FLAG_GFX_LOADED) {
                node = firstLoaded;
                break;
            }

            firstLoaded = ListPoolNext(&firstLoaded->node);
        }

        if (ListPoolPrev(&firstLoaded->node) != NULL) {
            work->card2 = ListPoolPrev(&firstLoaded->node);
            work->pageScroll = MAP_SELECT_PAGE_SCROLL_PREV;

            if (node != NULL) {
                cur = node;

                do {
                    if ((cur->flags & MAPCARD_FLAG_GFX_LOADED) == 0) {
                        break;
                    }

                    node->x = -0x6400;
                    node = cur = ListPoolNext(&node->node);
                } while (node != NULL);
            }

            m4aSongNumStart(SONG_SYS_CLICKI04B);
        }

        node = ListPoolFirst(&work->cards);

        while (node != NULL) {
            if (node->x == -0x6400) {
                // @bug Clears firstLoaded's flag instead of node's.
                firstLoaded->flags &= ~MAPCARD_FLAG_CURSOR;
            }

            node = ListPoolNext(&node->node);
        }

        break;
    }

    ApproachValue(&work->handX, work->cursorTargetX, work->steps2);

    if (work->steps2 != 0) {
        work->steps2--;
    }
}

void ApplyMapSelectPageScroll(MapSelectWork* work) {
    MapcardWork* node;
    s8 i;

    switch (work->pageScroll) {
    case MAP_SELECT_PAGE_SCROLL_NONE:
        break;
    case MAP_SELECT_PAGE_SCROLL_NEXT:
        node = work->card2;
        i = 0;

        while (node != NULL) {
            node->x = gMapcardSlotX[i++] << 8;

            if (i == 6) {
                break;
            }

            node = ListPoolNext(&node->node);
        }

        work->pageScroll = MAP_SELECT_PAGE_SCROLL_NONE;
        SelectNearestMapSelectCard(work);

        if (work->card != NULL) {
            work->cursorTargetX = work->card->x;
            work->steps2 = 4;
        }

        work->page++;
        break;
    case MAP_SELECT_PAGE_SCROLL_PREV:
        node = work->card2;
        i = 5;

        while (node != NULL) {
            node->x = gMapcardSlotX[i--] << 8;

            if (i < 0) {
                break;
            }

            node = ListPoolPrev(&node->node);
        }

        work->pageScroll = MAP_SELECT_PAGE_SCROLL_NONE;
        SelectNearestMapSelectCard(work);

        if (work->card != NULL) {
            work->cursorTargetX = work->card->x;
            work->steps2 = 4;
        }

        work->page--;
        break;
    }
}

s32 SelectNearestMapSelectCard(MapSelectWork* work) {
    MapcardWork* node;
    s32 best;
    s32 dx;
    u16 r;
    void* empty;
    void* card;

    node = ListPoolFirst(&work->cards);
    best = 0x100;
    empty = NULL;
    work->card = empty;

    while (node != NULL) {
        if (node->x != -0x6400) {
            dx = (node->x >> 8) - (work->handX >> 8);
            r = Sqrt(dx * dx);

            if (best > r) {
                best = r;
                work->card = node;
            }
        }

        node = ListPoolNext(&node->node);
    }

    card = work->card;

    if (card != NULL) {
        card = &((MapcardWork*)card)->flags;
        r = *(u16*)card | MAPCARD_FLAG_CURSOR;
        *(u16*)card = r;

        return (work->card->x - work->handX) >> 8;
    }

    return 0;
}

u16 CountMapCards() {
    u8 sum;
    s32 i;

    sum = 0;

    for (i = 0; i <= 0x10D; i++) {
        sum += gMapCardCounts[i];
    }

    return sum;
}

s32 AddMapCard(u16 cardId) {
    if (CountRegularMapCards() <= 98) {
        if (gMapCardCounts[cardId] <= 8) {
            gMapCardCounts[cardId]++;

            switch (gMapCardDefs[cardId].kind) {
            case MAP_CARD_TEEMING_DARKNESS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_TEEMING_DARKNESS);
                break;
            case MAP_CARD_TRANQUIL_DARKNESS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_TRANQUIL_DARKNESS);
                break;
            case MAP_CARD_GUARDED_TROVE:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_GUARDED_TROVE);
                break;
            case MAP_CARD_LOOMING_DARKNESS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_LOOMING_DARKNESS);
                break;
            case MAP_CARD_SLEEPING_DARKNESS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_SLEEPING_DARKNESS);
                break;
            case MAP_CARD_MOMENTS_REPRIEVE:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_MOMENTS_REPRIEVE);
                break;
            case MAP_CARD_FEEBLE_DARKNESS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_FEEBLE_DARKNESS);
                break;
            case MAP_CARD_ALMIGHTY_DARKNESS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_ALMIGHTY_DARKNESS);
                break;
            case MAP_CARD_CALM_BOUNTY:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_CALM_BOUNTY);
                break;
            case MAP_CARD_FALSE_BOUNTY:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_FALSE_BOUNTY);
                break;
            case MAP_CARD_MOOGLE_ROOM:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_MOOGLE_ROOM);
                break;
            case MAP_CARD_SORCEROUS_WAKING:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_SORCEROUS_WAKING);
                break;
            case MAP_CARD_MARTIAL_WAKING:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_MARTIAL_WAKING);
                break;
            case MAP_CARD_ALCHEMIC_WAKING:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_ALCHEMIC_WAKING);
                break;
            case MAP_CARD_MEETING_GROUND:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_MEETING_GROUND);
                break;
            case MAP_CARD_MINGLING_WORLDS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_MINGLING_WORLDS);
                break;
            case MAP_CARD_STRONG_INITIATIVE:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_STRONG_INITIATIVE);
                break;
            case MAP_CARD_LASTING_DAZE:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_LASTING_DAZE);
                break;
            case MAP_CARD_STAGNANT_SPACE:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_STAGNANT_SPACE);
                break;
            case MAP_CARD_PREMIUM_ROOM:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_PREMIUM_ROOM);
                break;
            case MAP_CARD_WHITE_ROOM:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_WHITE_ROOM);
                break;
            case MAP_CARD_BLACK_ROOM:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_BLACK_ROOM);
                break;
            case MAP_CARD_KEY_OF_BEGINNINGS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_OF_BEGINNINGS);
                break;
            case MAP_CARD_KEY_OF_GUIDANCE:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_OF_GUIDANCE);
                break;
            case MAP_CARD_KEY_TO_TRUTH:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_TO_TRUTH);
                break;
            case MAP_CARD_KEY_TO_REWARDS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_TO_REWARDS);
                break;
            }

            return TRUE;
        }
    } else {
        if (gMapCardDefs[cardId].kind > MAP_CARD_BLACK_ROOM) {
            gMapCardCounts[cardId]++;

            switch (gMapCardDefs[cardId].kind) {
            case MAP_CARD_KEY_OF_BEGINNINGS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_OF_BEGINNINGS);
                break;
            case MAP_CARD_KEY_OF_GUIDANCE:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_OF_GUIDANCE);
                break;
            case MAP_CARD_KEY_TO_TRUTH:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_TO_TRUTH);
                break;
            case MAP_CARD_KEY_TO_REWARDS:
                SetJiminyFlag(JIMINY_RECORD_MAP_CARD_KEY_TO_REWARDS);
                break;
            }

            return TRUE;
        }
    }

    return FALSE;
}

s32 RemoveMapCard(u16 cardId) {
    if (gMapCardCounts[cardId] != 0) {
        gMapCardCounts[cardId]--;
        return TRUE;
    }

    return FALSE;
}

s32 AddRandomMapCard() {
    AddMapCard(GetRandom() % 270);
}

u16 CountMapCardsOfKind(u16 baseCardId) {
    u16 sum;
    s32 i;

    sum = 0;

    for (i = baseCardId; i < baseCardId + 10; i++) {
        sum += gMapCardCounts[i];
    }

    return sum;
}

u16 CountRegularMapCards() {
    u16 sum;
    s32 i;

    sum = 0;

    for (i = 0; i < 220; i++) {
        sum += gMapCardCounts[i];
    }

    return sum;
}

u16 CountZeroValueMapCards() {
    u16 sum;
    s32 i;

    sum = 0;

    for (i = 0; i < 27; i++) {
        sum += gMapCardCounts[i * 10];
    }

    return sum;
}

void CreateMapCardSelection(TaskPool* pool, u8* status) {
    TaskCreate(pool, &gTaskDescMapSelect, status);
}

void ClearMapCardInventory() {
    u16 i;

    for (i = 0; i < ARRAY_COUNT(gMapCardCounts); i++) {
        gMapCardCounts[i] = 0;
    }
}

void InitMapCardInventory() {
    ClearMapCardInventory();

    if (gDebugFlags & DEBUG_FLAG_ALL_MAP_CARD) {
#ifdef VERSION_EU
        s32 i;

        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 1));
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 1));

        for (i = 0; i <= 219; i += 10) {
            if (CountMapCards() <= 98) {
                AddMapCard(i);
            }
        }

        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_BEGINNINGS, 1));
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_GUIDANCE, 1));
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_TO_TRUTH, 1));
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_TO_REWARDS, 1));
#endif
    } else {
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 1));
    }
}

u8 IsMapCardDelivered() {
    return sMapCardDelivered;
}

void SetMapCardDelivered() {
    sMapCardDelivered = TRUE;
}

void SetSelectedMapCard(void* card) {
    sSelectedMapCard = card;
}

void* GetSelectedMapCard() {
    return sSelectedMapCard;
}

void ResetSelectedMapCard() {
    sSelectedMapCard = NULL;
    sMapCardDelivered = FALSE;
}

const void* GetRoomName(u16 kind) {
#ifdef VERSION_EU
    return GetLocalizedString(gRoomNames[kind]);
#else
    return gRoomNames[kind];
#endif
}

u8 HasMapCard(u16 cardId) {
    if (gMapCardCounts[cardId] != 0) {
        return TRUE;
    }

    return FALSE;
}

void LoadMapSelectKindPalette(u16 baseCardId, MapSelectWork* work) {
    u16 i;
    u16 j;
    u8* pal;
    s32 k;
    s32 k2;
    MapCardDef* card;
    MapCardDef* cards;

    for (i = 0; i < 22; i++) {
        pal = work->paletteBuffer;
        pal[i] = gMapSelectGridPalettes[i + 32];
    }

    for (i = 22; i < 32; i++) {
        pal = work->paletteBuffer;
        pal[i] = gMapSelectBgPalettes[i + 64];
    }

    for (i = baseCardId, j = 2; i < baseCardId + 10; i++, j += 2) {
        cards = gMapCardDefs;
        card = &cards[baseCardId];
        pal = work->paletteBuffer;

        if (gMapCardCounts[i] != 0) {
            if (card->backIndex != MAP_CARD_COLOR_GOLD) {
                pal[j] = -1;
                k = j + 1;
                pal[k] = 0x7F;
            } else {
                pal[j] = gMapSelectBgPalettes[j];
                k2 = j + 1;
                pal[k2] = gMapSelectBgPalettes[k2];
            }
        }
    }

    LoadPalette(work->paletteBuffer, (void*)(BG_PLTT + 14 * PLTT_SIZE_4BPP), sizeof(work->paletteBuffer));
}

void LoadMapSelectGridPalette(u16 baseCardId, MapSelectWork* work) {
    u16 i;
    u16 j;
    u8* pal;
    MapcardWork** card;
    s32 k;

    for (i = 0; i < 22; i++) {
        pal = work->paletteBuffer;
        pal[i] = gMapSelectGridPalettes[i + 32];
    }

    for (i = 22, j = 2; i < 26; i++, j++) {
        card = &work->card;
        pal = work->paletteBuffer;

        switch ((*card)->cardDef->backIndex) {
        case MAP_CARD_COLOR_GREEN:
            pal[i] = gMapSelectGridPalettes[j + 0x60];
            break;
        case MAP_CARD_COLOR_RED:
            pal[i] = gMapSelectGridPalettes[j + 0x40];
            break;
        case MAP_CARD_COLOR_BLUE:
            pal[i] = gMapSelectGridPalettes[j + 0xA0];
            break;
        case MAP_CARD_COLOR_GOLD:
            pal[i] = gMapSelectGridPalettes[j + 0x80];
            break;
        }
    }

    for (i = 26; i < 32; i++) {
        pal = work->paletteBuffer;
        pal[i] = gMapSelectBgPalettes[i];
    }

    for (i = baseCardId, j = 2; i < baseCardId + 10; i++, j += 2) {
        pal = work->paletteBuffer;

        if (gMapCardCounts[i] != 0) {
            pal[j] = -1;
            k = j + 1;
            pal[k] = 0x7F;
        }
    }

    LoadPalette(work->paletteBuffer, (void*)(BG_PLTT + 12 * PLTT_SIZE_4BPP), sizeof(work->paletteBuffer));
}

s32 LoadMapSelectValueCounts(u16 baseCardId, MapSelectWork* work) {
    u16 i;
    u16 j;
    u8 count;
    u8* src;
    u8* base;

    for (i = baseCardId, j = 0; i < baseCardId + 10; i++, j++) {
        if (gMapCardCounts[i] != 0) {
            count = gMapCardCounts[i];

            if (count > 9) {
                count = 9;
                gMapCardCounts[i] = count;
            }

            src = &gMapSelectCountBlankTiles[(count + 1) * 32];
            base = GetBgCharBase(1);
            base += gMapSelectCountTileIndices[i - baseCardId] * 32;
            RequestDma3Copy(src, base, 32);
            work->valueCounts[j] = count;
        } else {
            base = GetBgCharBase(1);
            base += gMapSelectCountTileIndices[i - baseCardId] * 32;
            RequestDma3Copy(gMapSelectCountDigitTiles, base, 32);
            work->valueCounts[j] = 0;
        }
    }

    for (i = 0; i < ARRAY_COUNT(work->valueCounts); i++) {
        if (work->valueCounts[i] != 0) {
            return (s8)i;
        }
    }

    return -1;
}

s32 FindLastMapSelectValueInRow(MapSelectWork* work) {
    u8* valueCounts;
    s32 base;
    u8 i;

    i = 4;
    base = work->valueRow * 5;
    valueCounts = work->valueCounts;

    do {
        if (valueCounts[i + base] == 0) {
            i--;
        } else {
            return (s8)i;
        }
    } while (i != 0);

    return -1;
}

void HandleMapSelectValueCursor(MapSelectWork* work) {
    s8 prevColumn;
    s8 prevRow;
    s32 pastEnd;

    prevColumn = work->valueColumn;
    prevRow = work->valueRow;
    pastEnd = FALSE;

    switch (GetKeysRepeat() & DPAD_ANY) {
    case DPAD_RIGHT:
        do {
            if (work->valueColumn <= 3) {
                work->valueColumn++;
            } else {
                work->valueColumn = 0;
                work->valueRow ^= 1;
            }
        } while (work->valueCounts[work->valueColumn + work->valueRow * 5] == 0);

        work->steps2 = 4;
        break;
    case DPAD_LEFT:
        do {
            if (work->valueColumn > 0) {
                work->valueColumn--;
            } else {
                work->valueColumn = 4;
                work->valueRow ^= 1;
            }
        } while (work->valueCounts[work->valueColumn + work->valueRow * 5] == 0);

        work->steps2 = 4;
        break;
    case DPAD_UP:
        work->valueRow ^= 1;
        prevColumn = work->valueColumn;

        while (work->valueCounts[work->valueColumn + work->valueRow * 5] == 0) {
            if (work->valueColumn > 3) {
                pastEnd = TRUE;
                break;
            }

            work->valueColumn++;
            pastEnd = FALSE;
        }

        if (pastEnd == TRUE) {
            work->valueColumn = FindLastMapSelectValueInRow(work);

            if (work->valueColumn == -1) {
                work->valueRow ^= 1;
                work->valueColumn = prevColumn;
            }
        }

        work->steps2 = 4;
        break;
    case DPAD_DOWN:
        work->valueRow ^= 1;
        prevColumn = work->valueColumn;

        while (work->valueCounts[work->valueColumn + work->valueRow * 5] == 0) {
            if (work->valueColumn > 3) {
                pastEnd = TRUE;
                break;
            }

            work->valueColumn++;
            pastEnd = FALSE;
        }

        if (pastEnd == TRUE) {
            work->valueColumn = FindLastMapSelectValueInRow(work);

            if (work->valueColumn == -1) {
                work->valueRow ^= 1;
                work->valueColumn = prevColumn;
            }
        }

        work->steps2 = 4;
        break;
    }

    if (prevColumn != work->valueColumn || prevRow != work->valueRow) {
        m4aSongNumStart(SONG_SYS_CLICKI04B);
    }

    ApproachValue(&work->handX, gMapSelectValueColumnX[work->valueColumn] << 8, work->steps2);
    ApproachValue(&work->handY, gMapSelectValueRowY[work->valueRow] << 8, work->steps2);
    ApproachValue(&work->x, gMapSelectValueColumnX[work->valueColumn] << 8, work->steps);
    ApproachValue(&work->y, (gMapSelectValueRowY[work->valueRow] + 34) << 8,
                  work->steps);

    if (work->steps != 0) {
        work->steps--;
    }

    if (work->steps2 != 0) {
        work->steps2--;
    }
}

u8 UpdateMapSelectTutorial(MapSelectWork* work, void* task) {
    MapcardWork* node;
    u8 lastPage;

    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->mosaicTimer == 2) {
        if (work->mosaicX != 0) {
            work->mosaicX--;
        }

        if (work->mosaicY != 0) {
            work->mosaicY--;
        }

        work->mosaicTimer = 0;
    }

    work->mosaicTimer++;

    if (!IsMessageWindowOpen()) {
        if (work->messageTimer == 8) {
            work->messageTimer = 0;

            if (work->tutorialMessage <= CARD_MSG_MAP_SELECT_TUTORIAL_3) {
                CreateSysmsgwinTask(&work->tasks, work->tutorialMessage);
                work->tutorialMessage++;
            } else {
                node = ListPoolFirst(&work->cards);
                SetBgMapBlocks(1, gMapSelectBgMapBlocks, 1, 2);
#ifdef PLATFORM_PC
                ScrollBgMapTo(1, 0, ((u32)work->bgScrollY >> 8) + MAPSEL_RAISE); /* the panel keeps its place */
#endif

                while (node != NULL) {
                    node->flags |= MAPCARD_FLAG_RAISED;
                    node = ListPoolNext(&node->node);
                }

                if (work->kindCount <= 6) {
                    work->lastPage = 0;
                } else {
                    lastPage = work->kindCount / 6;
                    work->lastPage = lastPage;
                }

                work->handY = 0x6400;
                work->y = 0x7A00;
                work->nameY = 0x9100;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectKindInput);
                work->scrollBarVisible = TRUE;
                work->tutorialMessage = CARD_MSG_MAP_SELECT_VALUE_TUTORIAL_0;
                return 1;
            }
        } else {
            work->messageTimer++;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateMapSelectValueTutorial(MapSelectWork* work, void* task) {
    u8 windowOpen;

    windowOpen = IsMessageWindowOpen();

    if (!windowOpen) {
        if (work->messageTimer == 8) {
            work->messageTimer = 0;

            if (work->tutorialMessage <= CARD_MSG_MAP_SELECT_VALUE_TUTORIAL_3) {
                CreateSysmsgwinTask(&work->tasks, work->tutorialMessage);
                work->tutorialMessage++;
            } else {
                gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_MAP_SELECT;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectValueInput);
            }
        } else {
            work->messageTimer++;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateMapSelectEventDoorTutorial(MapSelectWork* work, void* task) {
    MapcardWork* node;
    u32 pages;

    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->mosaicTimer == 2) {
        if (work->mosaicX != 0) {
            work->mosaicX--;
        }

        if (work->mosaicY != 0) {
            work->mosaicY--;
        }

        work->mosaicTimer = 0;
    }

    work->mosaicTimer++;

    if (!IsMessageWindowOpen()) {
        if (work->messageTimer == 8) {
            work->messageTimer = 0;

            if (work->tutorialMessage <= CARD_MSG_MAP_SELECT_EVENT_DOOR_TUTORIAL_4) {
                CreateSysmsgwinTask(&work->tasks, work->tutorialMessage);
                work->tutorialMessage++;
            } else {
                node = ListPoolFirst(&work->cards);
                SetBgMapBlocks(1, gMapSelectBgMapBlocks, 1, 2);
#ifdef PLATFORM_PC
                ScrollBgMapTo(1, 0, ((u32)work->bgScrollY >> 8) + MAPSEL_RAISE); /* the panel keeps its place */
#endif

                while (node != NULL) {
                    node->flags |= MAPCARD_FLAG_RAISED;
                    node = ListPoolNext(&node->node);
                }

                if (work->kindCount <= 6) {
                    work->lastPage = 0;
                } else {
                    pages = work->kindCount / 6;
                    work->lastPage = pages;
                }

                work->handY = 0x6400;
                work->y = 0x7A00;
                work->nameY = 0x9100;
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapSelectKindInput);
                work->scrollBarVisible = TRUE;
                return 1;
            }
        } else {
            work->messageTimer++;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void RemoveMapSelectCard(MapSelectWork* work) {
    MapcardWork* node;
    s32 openedCardX;
    s32 i;

    openedCardX = work->openedCardX >> 8;

    for (i = 0; i < 6; i++) {
        if (openedCardX == gMapcardSlotX[i]) {
            break;
        }
    }

    node = ListPoolRemove(&work->card->node, &work->cards);
    work->card->flags |= MAPCARD_FLAG_REMOVED;
    work->card = node;

    while (node != NULL) {
        node->args.index--;

        if (i <= 5) {
            node->x = gMapcardSlotX[i] << 8;
            i++;
        }

        node = ListPoolNext(&node->node);
    }

    for (node = ListPoolFirst(&work->cards); node != NULL; node = ListPoolNext(&node->node)) {
        node->flags |= MAPCARD_FLAG_RAISED;
    }

    if (work->card == NULL) {
        work->card = ListPoolLast(&work->cards);

        if (work->card == NULL) {
            work->cursorTargetX = 0x1600;
        } else {
            work->cursorTargetX = work->card->x;
        }
    } else {
        work->cursorTargetX = work->card->x;
    }
}

void Mapcard_0(MapcardWork* work, MapcardArgs* args) {
    work->tiles = NULL;
    work->unk_04 = NULL;
    work->tiles2 = NULL;
    work->palette = NULL;
    work->args = *args;
    work->x = work->args.index <= 5 ? gMapcardSlotX[work->args.index] << 8 : -0x6400;
    work->y = 0x10500;
    work->priority = 50;
    work->deceleration = 0;
    work->speed = 0;
    work->distance = 0;
    work->flags = 0;
    work->angle = 0;
    work->steps = 16;
    work->holdTimer = 0;
    work->scale = Q_8_8(1);
    work->unk_71 = 0;
    work->unk_72 = 0;
    work->value = gMapCardDefs[work->args.baseCardId].value;
    work->cardDef = &gMapCardDefs[work->args.baseCardId];
    work->cardBack = &gMapCardBackDefs[work->cardDef->backIndex];
    LinkMapcardNode(work);
    UpdateMapcardRise(work);
    UpdateMapcardGfx(work);
}

u8 Mapcard_1(MapcardWork* work, void* task) {
    if (work->flags & 0xC) {
        work->steps = 12;
        func_08094DEC(work);
        SetTaskUpdate(task, (TaskUpdateFunc)func_080948F0);
    }

    if (work->flags & MAPCARD_FLAG_OPENED) {
        work->angle = 0;
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapcardMoveToFront);
    }

    if (work->flags & MAPCARD_FLAG_CHOSEN) {
        work->angle = 0;
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapcardToCenter);
    }

    if (work->flags & MAPCARD_FLAG_REMOVED) {
        return 0;
    }

    work->angle = 0;
    UpdateMapcardRise(work);
    UpdateMapcardGfx(work);
    return 1;
}

u8 UpdateMapcardMoveToFront(MapcardWork* work, void* task) {
    ApproachValue(&work->x, gMapcardSlotX[0] << 8, work->steps);
    work->steps--;

    if (!(work->flags & MAPCARD_FLAG_OPENED)) {
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapcardMoveBack);
    }

    if (work->flags & MAPCARD_FLAG_CURSOR) {
        work->angle += 8;
    } else {
        work->angle = 0;
    }

    if (work->flags & MAPCARD_FLAG_CHOSEN) {
        work->angle = 0;
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapcardToCenter);
    }

    if (work->flags & MAPCARD_FLAG_REMOVED) {
        return 0;
    }

    UpdateMapcardRise(work);
    UpdateMapcardGfx(work);
    return 1;
}

u8 UpdateMapcardMoveBack(MapcardWork* work, void* task) {
    ApproachValue(&work->x, gMapcardSlotX[work->args.index % 6] << 8, work->steps);
    work->steps--;

    if (work->steps == 0) {
        SetTaskUpdate(task, (TaskUpdateFunc)Mapcard_1);
    }

    if (work->flags & MAPCARD_FLAG_CURSOR) {
        work->angle += 8;
    } else {
        work->angle = 0;
    }

    UpdateMapcardRise(work);
    UpdateMapcardGfx(work);
    return 1;
}

s32 func_080948F0(MapcardWork* work, void* task) {
    u8 busy;

    busy = func_08094E4C(work);
    UpdateMapcardRise(work);
    UpdateMapcardGfx(work);

    if (!busy) {
        work->flags &= 0xFFF3;
        SetTaskUpdate(task, (TaskUpdateFunc)Mapcard_1);
    }

    return 1;
}

u8 UpdateMapcardToCenter(MapcardWork* work, void* task) {
    work->angle = 0;
    ApproachValue(&work->x, 0x7800, work->steps);
    ApproachValue(&work->y, 0x3800, work->steps);

    if (work->steps != 0) {
        work->steps--;
    } else {
        work->holdTimer++;

        if (work->holdTimer > 15) {
            AimMapcardAtDoor(work);
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateMapcardFlyToDoor);
        }
    }

    UpdateMapcardGfx(work);
    return 1;
}

void AimMapcardAtDoor(MapcardWork* work) {
    s32 toDoor[2];
    s32 dx;
    s32 dy;
    FldObj* door = GetMapRoomDoor();

    dx = (door->fieldPosition.x >> 8) - (gFieldState->x >> 8);
    dy = (door->fieldPosition.y >> 8) + (door->fieldPosition.z >> 8) - (gFieldState->y >> 8) - 24;
    toDoor[0] = dx * 256 - work->x;
    toDoor[1] = dy * 256 - work->y;
    work->distance = NormalizeVector2D8(&toDoor[0], &toDoor[1]);
    work->dirX = -toDoor[0];
    work->dirY = -toDoor[1];
    work->speed = 0x300;
    work->deceleration = 25;
    work->angle = 0;
    work->scale = Q_8_8(1);
}

u8 UpdateMapcardFlyToDoor(MapcardWork* work, void* task) {
    FldObj* door;
    s32 dx;
    s32 dy;
    s32 distance;
    s32 x;
    s32 y;
    u16 oldScale;
    u16 flags;

    door = GetMapRoomDoor();
    dx = (door->fieldPosition.x >> 8) - (gFieldState->x >> 8);
    dy = (door->fieldPosition.y >> 8) + (door->fieldPosition.z >> 8) - (gFieldState->y >> 8) - 24;

    if (work->speed < 0) {
        x = (dx << 8) - work->x;
        y = (dy << 8) - work->y;
        NormalizeVector2D8(&x, &y);
        work->dirX = -x;
        work->dirY = -y;
    }

    {
        u8* angle = &work->angle;
        u16* scale;

        *angle += 24;
        scale = (u16*)(angle - (offsetof(MapcardWork, angle) - offsetof(MapcardWork, scale)));
        oldScale = *scale;
        *scale = (s16)oldScale > 25 ? oldScale - 12 : 25;
    }

    work->x += (work->dirX * work->speed) >> 8;
    work->y += (work->dirY * work->speed) >> 8;
    distance = VectorLength2D((dx << 8) - work->x, (dy << 8) - work->y);
    work->distance = distance;
    work->speed -= work->deceleration;
    work->deceleration += 2;

    if (work->args.parent->isEventDoor != TRUE) {
        if (distance <= 0x7FF) {
            SetMapCardDelivered();
            flags = work->flags | MAPCARD_FLAG_DELIVERED;
            work->flags = flags;
            SetSelectedMapCard(&gMapCardDefs[work->args.baseCardId + work->value].kind);
        }
    } else {
        if (distance <= 0x7FF && work->args.parent->remainingKeys == 0) {
            SetMapCardDelivered();
            flags = work->flags | MAPCARD_FLAG_DELIVERED;
            work->flags = flags;
            SetSelectedMapCard(&gMapCardDefs[work->args.baseCardId + work->value].kind);
        }
    }

    return 1;
}

void Mapcard_2(MapcardWork* work) {
    ObjAffine* affine;
    u16 y;
    void* sprite;

    if (!IsMessageWindowOpen()) {
        y = (work->y >> 8) + (gSineTable[work->angle] >> 8);

        if (work->flags & MAPCARD_FLAG_GFX_LOADED) {
            if (work->x > 0) {
                if (work->x <= 0xEFFF) {
                    goto draw;
                }
            }

            // fakematch
            do {
                return;
            } while (0);

        draw:
            affine = NULL;

            if (work->flags & MAPCARD_FLAG_CHOSEN) {
                affine = AllocObjAffine(work->angle, (s16)work->scale, (s16)work->scale, TRUE);
            }

            if (gMapCardDefs[work->args.baseCardId].backIndex == MAP_CARD_COLOR_GOLD) {
                DrawSprite(work->x >> 8, y, gMapCardUiResources.gfx, gMapCardUiResources.tiles, work->palette2, affine, 0,
                           work->priority - 2);
            }

            sprite = work->cardBack->sprites[0];
            DrawSprite(work->x >> 8, y, sprite, work->tiles3, work->palette2, affine, 0, work->priority);
            sprite = work->cardDef->sprites[0];
            DrawSprite(work->x >> 8, y, sprite, work->tiles2, work->palette, affine, 0, work->priority + 1);
        }
    }
}

void Mapcard_3(MapcardWork* work) {
    if (work->flags & MAPCARD_FLAG_GFX_LOADED) {
        ReleaseObjTiles(work->tiles2);
        ReleaseObjPalette(work->palette);
        ReleaseObjTiles(work->tiles3);
        ReleaseObjPalette(work->palette2);
    }
}

u8 IsMapcardOnScreen(MapcardWork* work) {
    if (work->x >= -4096) {
        if (work->x <= 0x10000) {
            if (work->y >= -5120) {
                if (work->y <= 0xC000) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void UpdateMapcardGfx(MapcardWork* work) {
    MapCardDef* def;
    MapCardBackDef* back;
    u16 flags;

    if (IsMapcardOnScreen(work)) {
        if (!(work->flags & MAPCARD_FLAG_GFX_LOADED)) {
            def = work->cardDef;
            back = work->cardBack;
            work->tiles2 = LoadObjTiles(def->tiles, def->tilesSize);
            work->palette = LoadObjPalette(def->palette, 32);
            work->tiles3 = LoadObjTiles(back->tiles, back->tilesSize);
            work->palette2 = LoadObjPalette(back->palette, 32);
            work->tiles = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
            FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
            FadeSetPaletteExcluded(work->palette2->index + 16, TRUE);
            flags = work->flags | MAPCARD_FLAG_GFX_LOADED;
            work->flags = flags;
        }
    } else if (work->flags & MAPCARD_FLAG_GFX_LOADED) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjTiles(work->tiles2);
        ReleaseObjPalette(work->palette);
        ReleaseObjTiles(work->tiles3);
        ReleaseObjPalette(work->palette2);
        work->flags &= ~MAPCARD_FLAG_GFX_LOADED;
    }
}

void UpdateMapcardRise(MapcardWork* work) {
    if (work->flags & MAPCARD_FLAG_RAISED) {
        ApproachValue(&work->y, 0x7900, work->steps);
    } else {
        ApproachValue(&work->y, 0x10500, work->steps);
    }

    if (work->steps != 0) {
        work->steps--;
    }
}

void func_08094DEC(MapcardWork* work) {
    if ((work->flags & 4) && work->speed - work->deceleration > 0x8000) {
        work->deceleration += 0x10000;
    }

    if ((work->flags & 8) && work->speed < work->deceleration) {
        work->deceleration -= 0x10000;
    }

    ApproachValue(&work->deceleration, work->speed, work->steps);
    work->steps--;
}

u8 func_08094E4C(MapcardWork* work) {
    ApproachValue(&work->deceleration, work->speed, work->steps);

    if (work->steps != 0) {
        work->steps--;
        return TRUE;
    }

    return FALSE;
}

MapcardWork* CreateMapCard(MapcardArgs* args, TaskPool* pool) {
    return TaskCreate(pool, &gTaskDescMapcard, args)->work;
}

void LinkMapcardNode(MapcardWork* work) {
    ListNodeInit(&work->node, work->args.pool, work);
    ListPoolAppend(&work->node, work->args.pool);
}

void Reload_Gage_0(CardDisplayWork* work, CardDisplayArgs* arg) {
    ReloadGauge* gauge;
    ReloadChildArgs args;
    u16 reloadCounter;
    s8 n;
    s8 i;

    work->tiles = NULL;
    work->tiles2 = NULL;
    work->tiles4 = NULL;
    work->tiles3 = NULL;
    work->tiles5 = NULL;
    work->palette = NULL;
    work->command = CARD_DISP_COMMAND_NONE;
    work->args = *arg;
    work->flags = 0;
    work->timer = 16;
    work->ringRadius = 0;
    work->stockIndex = 0;
    work->priority = 4;
    work->ringRadiusTarget = 0x2400;
    work->ringAngleTarget = 0;
    work->ringAngle = 0;
    work->reloadGauge = EwramAlloc(sizeof(ReloadGauge));
    work->phase = 0;
    work->swingSteps = 0;
    gauge = work->reloadGauge;
    reloadCounter = arg->index;

    if ((s16)reloadCounter >= 0) {
        gauge->reloadCounter = reloadCounter;
    } else {
        gauge->reloadCounter = -1;
    }

    if (gauge->reloadCounter > 17) {
        gauge->reloadCounter = 18;
    }

    gauge->chargeTick = 0;
    work->children = EwramAlloc(sizeof(ListPool));

    switch (work->args.variant) {
    case CARD_SIDE_SORA:
        work->ringCenterX = gSoraCardLayout[0][0];
        work->ringCenterY = gSoraCardLayout[0][1];
        work->x = gSoraCardLayout[4][0];
        work->y = gSoraCardLayout[4][1];
        work->swingAngle = work->swingAngleTarget = 0x2000;
        work->flags |= 0x8000000;
        break;
    case CARD_SIDE_RIKU:
        work->ringCenterX = gRikuCardLayout[0][0];
        work->ringCenterY = gRikuCardLayout[0][1];
        work->x = gRikuCardLayout[4][0];
        work->y = gRikuCardLayout[4][1];
        work->swingAngle = work->swingAngleTarget = -0x2000;
        break;
    }

    work->scaleX = Q_8_8(1);
    work->scaleY = 0;
    ListPoolInit(work->children);
    ListNodeInit(&work->node, work->args.pool, work);
    ListPoolAppend(&work->node, work->args.pool);
    work->tiles2 = LoadObjTiles(gReloadCardTiles[work->args.listIndex], 0x280);
    work->tiles3 = AllocObjTiles(0x200, NULL);
    SetObjTileSource(work->tiles3, gReloadCardTiles[1]);
    work->tiles4 = AllocObjTiles(0x80, NULL);
    SetObjTileSource(work->tiles4, gReloadCardTiles[1]);
    InitReloadGageAnims(work->reloadGauge, work, work->args.listIndex);
    work->tiles = LoadObjTiles(gAButtonIconTiles, sizeof(gAButtonIconTiles));
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->tiles5 = AllocObjTiles(0x100, NULL);
    SetObjTileSource(work->tiles5, gReloadCounterTiles[work->args.listIndex]);
    InitReloadGageCounterAnim(work->reloadGauge, work->tiles5, work->args.listIndex, gauge->reloadCounter);
    work->flags |= (CARD_DISP_FLAG_NO_CARD | CARD_DISP_FLAG_SELECTED | CARD_DISP_FLAG_OPEN | CARD_DISP_FLAG_RELOAD_GAUGE);

    if (gauge->reloadCounter >= 0) {
        TaskPoolInit(&work->tasks, gauge->reloadCounter + 1);
        n = gauge->reloadCounter;

        if (gauge->reloadCounter > 3) {
            n = 3;
        }

        for (i = 0; i < n; i++) {
            args.pool = work->children;
            args.index = i;
            args.parentX = &work->x;
            args.parentY = &work->y;
            args.flags = 0;
            args.listIndex = work->args.listIndex;
            args.side = work->args.variant;
            TaskCreate(&work->tasks, &gTaskDescReloadChildren, &args);
        }
    } else {
        TaskPoolInit(&work->tasks, 1);
    }
}

u8 Reload_Gage_1(CardDisplayWork* work, void* task) {
    ReloadChildArgs args;
    ReloadGauge* gauge;
    ReloadChildWork* node;
    ReloadChildWork* child;
    s32 flags;
    u8 reloadCharging;

    gauge = work->reloadGauge;
    reloadCharging = 0;

    switch (work->args.variant) {
    case CARD_SIDE_SORA:
        if (gCardBattleState->soraListIndex == work->args.listIndex) {
            reloadCharging = gCardBattleState->soraReloadCharging;
            gCardBattleState->soraReloadCharging = 0;
        }

        break;
    case CARD_SIDE_RIKU:
        if (gCardBattleState->rikuListIndex == work->args.listIndex) {
            reloadCharging = gCardBattleState->rikuReloadCharging;
            gCardBattleState->rikuReloadCharging = 0;
        }

        break;
    }

    if ((work->flags & (CARD_DISP_FLAG_SELECTED | CARD_DISP_FLAG_SETTLED)) == (CARD_DISP_FLAG_SELECTED | CARD_DISP_FLAG_SETTLED)) {
        if (reloadCharging == 1) {
            if ((s8)gauge->chargeTick == 2) {
                switch (work->args.variant) {
                case CARD_SIDE_SORA:
                    if ((gBtlWork->flags & BTL_FLAG_RELOAD_CHARGING) == 0) {
                        m4aSongNumStart(SONG_SYS_CHAGE);
                        gBtlWork->flags |= BTL_FLAG_RELOAD_CHARGING;
                    }

                    break;
                case CARD_SIDE_RIKU:
                    if ((gRikuBtlWork->flags & BTL_FLAG_RELOAD_CHARGING) == 0) {
                        m4aSongNumStart(SONG_SYS_CHAGE);
                        gRikuBtlWork->flags |= BTL_FLAG_RELOAD_CHARGING;
                    }

                    break;
                }

                node = ListPoolFirst(work->children);

                while (node != NULL) {
                    node->args.flags &= ~RELOAD_CHILD_FLAG_IDLE;
                    node = ListPoolNext(&node->node);
                }

                if (work->stockIndex == 0) {
                    work->scaleY += Q_8_8(0.1);

                    if (work->scaleY > Q_8_8(1)) {
                        work->scaleY = Q_8_8(1);
                        work->stockIndex = 1;
                    }
                } else {
                    work->priority += 3;
                    AdvanceReloadGageAnim(work->reloadGauge, work);

                    if (work->priority == 10) {
                        work->priority = 4;
                        work->scaleY = 0;
                        work->stockIndex = 0;
                        child = ListPoolFirst(work->children);

                        while (child != NULL) {
                            child->args.flags |= RELOAD_CHILD_FLAG_SHIFTED;
                            child->args.index--;
                            child = ListPoolNext(&child->node);
                        }

                        gauge->reloadCounter--;

                        if (gauge->reloadCounter > 2) {
                            args.pool = work->children;
                            args.index = 3;
                            args.parentX = &work->x;
                            args.parentY = &work->y;
                            args.flags = 0;
                            args.listIndex = work->args.listIndex;
                            args.side = work->args.variant;
                            child = TaskCreate(&work->tasks, &gTaskDescReloadChildren, &args)->work;
                            flags = child->args.flags | RELOAD_CHILD_FLAG_SHIFTED;
                            flags &= 0xFFFD;
                            child->args.flags = flags;
                            child->args.index--;
                        }

                        work->timer = 8;
                        work->phase = 1;
                        ResetReloadGageAnim(work->reloadGauge);
                        m4aSongNumStart(SONG_SYS_CHAGEF1);
                    }
                }

                switch (work->args.variant) {
                case CARD_SIDE_SORA:
                    if (gBtlWork->hcEffect == HC_EFFECT_QUICKLOAD) {
                        gauge->chargeTick = 1;
                    } else if (gBtlWork->hcEffect == HC_EFFECT_OVERDRIVE) {
                        gauge->chargeTick = 254;
                    } else {
                        gauge->chargeTick = 0;
                    }

                    break;
                case CARD_SIDE_RIKU:
                    if (gRikuBtlWork->hcEffect == HC_EFFECT_QUICKLOAD) {
                        gauge->chargeTick = 1;
                    } else if (gRikuBtlWork->hcEffect == HC_EFFECT_OVERDRIVE) {
                        gauge->chargeTick = 254;
                    } else {
                        gauge->chargeTick = 0;
                    }

                    break;
                }
            }

            UpdateReloadGageAnims(work->reloadGauge, work);
            gauge->chargeTick++;
        } else {
            SetReloadGageIdleFrames(work->reloadGauge, work);
            gauge->chargeTick = 0;
            work->flags |= 0x8000000;
            m4aSongNumStop(SONG_SYS_CHAGE);

            switch (work->args.variant) {
            case CARD_SIDE_SORA:
                gBtlWork->flags &= ~BTL_FLAG_RELOAD_CHARGING;
                break;
            case CARD_SIDE_RIKU:
                gRikuBtlWork->flags &= ~BTL_FLAG_RELOAD_CHARGING;
                break;
            }

            node = ListPoolFirst(work->children);

            while (node != NULL) {
                node->args.flags |= RELOAD_CHILD_FLAG_IDLE;
                node = ListPoolNext(&node->node);
            }
        }
    }

    if (gauge->reloadCounter < 0) {
        if ((work->flags & CARD_DISP_FLAG_RELOAD_DONE) == 0) {
            work->flags |= CARD_DISP_FLAG_RELOAD_DONE;
            m4aSongNumStart(SONG_SYS_CHAGEF2);
        }

        if (FadeGetAmount() == 0) {
            FadeFromAmount(FADE_MODE_ADD_WHITE, 16, 20);
        }

        return 0;
    }

    UpdateReloadGageSlide(work->reloadGauge, work);

    if (work->phase == 1 && (s16)work->timer == 1) {
        SetReloadGageCounterAnim(work->reloadGauge, gauge->reloadCounter);
    }

    UpdateReloadGageRingPosition(work);

    if (work->flags & CARD_DISP_FLAG_REMOVE) {
        return 0;
    }

    StepReloadGageSine(work->reloadGauge);
    work->bobAngle += 4;

    if ((work->flags & CARD_DISP_FLAG_OPEN) == 0) {
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateReloadGageIdle);
        m4aSongNumStop(SONG_SYS_CHAGE);

        switch (work->args.variant) {
        case CARD_SIDE_SORA:
            gBtlWork->flags &= ~BTL_FLAG_RELOAD_CHARGING;
            break;
        case CARD_SIDE_RIKU:
            gRikuBtlWork->flags &= ~BTL_FLAG_RELOAD_CHARGING;
            break;
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateReloadGageIdle(CardDisplayWork* work, void* task) {
    if (work->command == CARD_DISP_COMMAND_REMOVE) {
        return 0;
    }

    work->ringRadius += -work->ringRadius >> 1;
    work->x += (gSoraCardLayout[4][0] - work->x) >> 1;
    work->y += (gSoraCardLayout[4][1] - work->y) >> 1;

    if (work->flags & CARD_DISP_FLAG_OPEN) {
        SetTaskUpdate(task, (TaskUpdateFunc)Reload_Gage_1);
    }

    return 1;
}

void Reload_Gage_2(CardDisplayWork* work) {
    ReloadGauge* gauge;
    ObjAffine* affine;
    void* gfx;
    s32 offsetY;

    gauge = work->reloadGauge;
    gfx = gCardBacks[work->args.listIndex].gfx2;
    DrawSprite((work->x >> 8) + (gauge->offsetX >> 8),
               (work->y >> 8) + (gSineTable[work->bobAngle] >> 8),
               gfx, work->tiles2,
               gCardBattleState->palette, NULL, SPRITE_PRIORITY(1), 50);

    if (work->scaleY > 0) {
        affine = AllocObjAffine(0, Q_8_8(1), work->scaleY, FALSE);
        DrawSprite((work->x >> 8) + (gauge->offsetX >> 8),
                   (work->y >> 8) + (offsetY = (gSineTable[work->bobAngle] >> 8) + 17),
                   gauge->gfx, work->tiles3, gCardBattleState->palette, affine,
                   SPRITE_PRIORITY(1), 49);

        if (work->stockIndex == 1) {
            DrawSprite((work->x >> 8) + (gauge->offsetX >> 8),
                       (work->y >> 8) + (gSineTable[work->bobAngle] >> 8),
                       gauge->gfx2, work->tiles4, gCardBattleState->palette, NULL,
                       SPRITE_PRIORITY(1), 49);
        }
    }

    if (gauge->gfx3 != NULL) {
        DrawSprite((work->x >> 8) + (gauge->offsetX >> 8),
                   (work->y >> 8) + (gSineTable[work->bobAngle] >> 8),
                   gauge->gfx3, work->tiles5,
                   gCardBattleState->palette, NULL, SPRITE_PRIORITY(1), 48);
    }

    TaskPoolDraw(&work->tasks);
}

void Reload_Gage_3(CardDisplayWork* work) {
    TaskPoolDestroy(&work->tasks);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjTiles(work->tiles5);
    EwramFree(work->children);
    EwramFree(work->reloadGauge);

    switch (work->args.variant) {
    case CARD_SIDE_SORA:
        gBtlWork->flags &= ~BTL_FLAG_RELOAD_CHARGING;

        switch (gBtlWork->hcEffect) {
        case HC_EFFECT_QUICKLOAD:
        case HC_EFFECT_COMBO_PLUS_2:
        case HC_EFFECT_DRAW_2:
        case HC_EFFECT_RELOAD_KINESIS:
        case HC_EFFECT_AUTO_RELOAD:
        case HC_EFFECT_OVERDRIVE:
        case HC_EFFECT_INCREMENTOR_2:
            gBtlWork->hcEffectCount--;
            break;
        }

        break;
    case CARD_SIDE_RIKU:
        gRikuBtlWork->flags &= ~BTL_FLAG_RELOAD_CHARGING;

        switch (gRikuBtlWork->hcEffect) {
        case HC_EFFECT_QUICKLOAD:
        case HC_EFFECT_COMBO_PLUS_2:
        case HC_EFFECT_DRAW_2:
        case HC_EFFECT_RELOAD_KINESIS:
        case HC_EFFECT_AUTO_RELOAD:
        case HC_EFFECT_OVERDRIVE:
        case HC_EFFECT_INCREMENTOR_2:
            gRikuBtlWork->hcEffectCount--;
            break;
        }

        break;
    }

    ListPoolRemove(&work->node, work->args.pool);
}

void UpdateReloadGageRingPosition(CardDisplayWork* work) {
    ApproachValue(&work->swingAngle, work->swingAngleTarget, work->swingSteps);

    if (work->swingSteps != 0) {
        work->swingSteps--;
    }

    work->ringRadius += (work->ringRadiusTarget - work->ringRadius) >> 1;

    if ((s16)work->timer > 0) {
        work->timer--;
        work->flags &= ~CARD_DISP_FLAG_SETTLED;
    } else {
        work->timer = 0;
        work->flags |= CARD_DISP_FLAG_SETTLED;
    }

    switch (work->args.variant) {
    case CARD_SIDE_SORA:
        work->ringCenterX = SIN(work->swingAngle >> 8) * 80 + gSoraCardLayout[0][0];
        work->ringCenterY = -COS(work->swingAngle >> 8) * 80 + gSoraCardLayout[0][1];
        work->x = gSineTable[0x20] * (work->ringRadius >> 8) + work->ringCenterX;
        work->y = -gSineTable[0x60] * (work->ringRadius >> 8) + work->ringCenterY;
        break;
    case CARD_SIDE_RIKU:
        work->ringCenterX = SIN(work->swingAngle >> 8) * 80 + gRikuCardLayout[0][0];
        work->ringCenterY = -COS(work->swingAngle >> 8) * 80 + gRikuCardLayout[0][1];
        work->x = gSineTable[0xE0] * (work->ringRadius >> 8) + work->ringCenterX;
        work->y = -gSineTable[0x120] * (work->ringRadius >> 8) + work->ringCenterY;
        break;
    }
}

void StepReloadGageSine(ReloadGauge* gauge) {
    gauge->sine = gSineTable[(u8)gauge->angle] >> 8;
    gauge->angle += 16;
}

void InitReloadGageCounterAnim(ReloadGauge* gauge, void* tiles, u8 listIndex, s32 count) {
    s8 animId = count;

    AnimInit(&gauge->anim, gReloadCounterAnims[listIndex], gReloadCounterFrames[listIndex]);

    if (animId >= 0) {
        AnimStart(&gauge->anim, animId, 0);
    } else {
        AnimStart(&gauge->anim, 0, 0);
    }

    gauge->gfx3 = AnimGetGfx(&gauge->anim);
}

void SetReloadGageCounterAnim(ReloadGauge* gauge, s32 count) {
    void* gfx;

    if ((u16)count <= 18) {
        AnimStart(&gauge->anim, count, 0);
        gfx = AnimGetGfx(&gauge->anim);
    } else {
        gfx = NULL;
    }

    gauge->gfx3 = gfx;
}

s32 UpdateReloadGageSlide(ReloadGauge* gauge, CardDisplayWork* work) {
    if ((s16)work->timer > 0 && work->phase == 1) {
        switch (work->args.variant) {
        case CARD_SIDE_SORA:
            ApproachValue(&gauge->offsetX, -0x3000, work->timer);
            break;
        case CARD_SIDE_RIKU:
            ApproachValue(&gauge->offsetX, 0x12000, work->timer);
            break;
        }
    } else {
        gauge->offsetX = 0;
    }
}

void InitReloadGageAnims(ReloadGauge* gauge, CardDisplayWork* work, u8 idx) {
    gauge->gaugeAnim = 2;
    AnimInit(&gauge->anim2, gReloadGaugeAnims[idx], gReloadGaugeFrames[idx]);
    AnimStart(&gauge->anim2, 1, ANIM_FLAG_LOOP);
    gauge->gfx = gReloadGaugeFrames[idx][3];
    AnimInit(&gauge->anim3, gReloadGaugeAnims[idx], gReloadGaugeFrames[idx]);
    AnimStart(&gauge->anim3, 2, ANIM_FLAG_LOOP);
    gauge->gfx2 = gReloadGaugeFrames[idx][6];
}

void UpdateReloadGageAnims(ReloadGauge* gauge, CardDisplayWork* work) {
    gauge->gfx = AnimUpdate(&gauge->anim2);
    gauge->gfx2 = AnimUpdate(&gauge->anim3);
}

void SetReloadGageIdleFrames(ReloadGauge* gauge, CardDisplayWork* work) {
    gauge->gfx = gReloadGaugeFrames[work->args.listIndex][3];
    gauge->gfx2 = gReloadGaugeFrames[work->args.listIndex][work->priority + 2];
}

void AdvanceReloadGageAnim(ReloadGauge* gauge, CardDisplayWork* work) {
    if (gauge->gaugeAnim <= 3) {
        gauge->gaugeAnim++;
    }

    AnimStart(&gauge->anim3, gauge->gaugeAnim, ANIM_FLAG_LOOP | ANIM_FLAG_KEEP_FRAME);
}

void ResetReloadGageAnim(ReloadGauge* gauge) {
    gauge->gaugeAnim = 2;
}

void* CreateReloadGageTask(CardBattleWork* work, u16 index, void* pool, u8 side) {
    CardDisplayArgs args;

    args.pool = &work->cardDisplays[work->listIndex];
    args.slot = NULL;

    switch (side) {
    case CARD_SIDE_SORA:
        if (gBtlWork->hcEffect == HC_EFFECT_COMBO_PLUS_2) {
            args.index = index - 2;
        } else {
            args.index = index;
        }

        break;
    case CARD_SIDE_RIKU:
        if (gRikuBtlWork->hcEffect == HC_EFFECT_COMBO_PLUS_2) {
            args.index = index - 2;
        } else {
            args.index = index;
        }

        break;
    }

    args.variant = side;
    args.listIndex = work->listIndex;
    return TaskCreate(pool, &gTaskDescReloadGage, &args)->work;
}

Mode gModeWORLDSELECT = {
    "WORLDSELECT",
    (ModeInitFunc)WORLDSELECT_0,
    WORLDSELECT_1,
    WORLDSELECT_2,
};

#ifdef VERSION_EU
const u16 gMapCardUiExtraTileSizes[5] = {
    864, 864, 864, 864, 864,
};
#endif

#ifdef VERSION_EU
void* gMapCardUiExtraTilesByLanguage[5] = {
    gMapCardUiExtraTiles, gMapCardUiExtraFrenchTiles, gMapCardUiExtraTiles, gMapCardUiExtraFrenchTiles, gMapCardUiExtraSpanishTiles,
};

void** gMapCardUiSpritesByLanguage[5] = {
    gMapCardUiExtraFrames, gMapCardUiExtraFrenchFrames, gMapCardUiExtraFrames, gMapCardUiExtraFrenchFrames, gMapCardUiExtraSpanishFrames,
};
#endif

const void* gMapSelectBgMapBlocks[2] = {
    gDefaultBgMap, gMapSelectMap,
};

s16 gMapSelectValueColumnX[5] = {
    59, 99, 139, 179, 219,
};

s16 gMapSelectValueRowY[2] = {
    88, 112,
};

#ifdef VERSION_EU
void** gMapSelectTitleSpritesByLanguage[5] = {
    gMapSelectTitleFrames, gMapSelectTitleFrenchFrames, gMapSelectTitleGermanFrames, gMapSelectTitleItalianFrames, gMapSelectTitleSpanishFrames,
};
#endif

TaskDesc gTaskDescMapSelect = {
    "MapSelect",
    (TaskInitFunc)MapSelect_0,
    (TaskUpdateFunc)MapSelect_1,
    (TaskDrawFunc)MapSelect_2,
    (TaskDestroyFunc)MapSelect_3,
    sizeof(MapSelectWork),
};

u16 gMapSelectCountTileIndices[10] = {
    72, 77, 82, 87, 92, 168, 173, 178, 183, 188,
};

MapCardBackDef gMapCardBackDefs[5] = {
    { gMapCardBorderBlueTiles, gMapCardBorderPalettes, gMapCardBorderBlueFrames, gCardOutlineWhiteTiles, gCardOutlineWhiteFrames, sizeof(gMapCardBorderBlueTiles), 32, 640 },
    { gMapCardBorderGreenTiles, gMapCardBorderPalettes, gMapCardBorderGreenFrames, gCardOutlineGreenTiles, gCardOutlineGreenFrames, sizeof(gMapCardBorderGreenTiles), 32, 640 },
    { gMapCardBorderRedTiles, gMapCardBorderPalettes, gMapCardBorderRedFrames, gCardOutlineRedTiles, gCardOutlineRedFrames, sizeof(gMapCardBorderRedTiles), 32, 640 },
    { gMapCardBorderBlueTiles, gMapCardBorderPalettes, gMapCardBorderBlueFrames, gCardOutlineBlueTiles, gCardOutlineBlueFrames, sizeof(gMapCardBorderBlueTiles), 32, 640 },
    { gMapCardBorderBlackTiles, gMapCardBorderPalettes, gMapCardBorderBlackFrames, gCardOutlineWhiteTiles, gCardOutlineWhiteFrames, sizeof(gMapCardBorderBlackTiles), 32, 640 },
};

MapCardDef gMapCardDefs[260] = {
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 0, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 1, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 2, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 3, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 4, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 5, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 6, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 7, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 8, MAP_CARD_COLOR_RED },
    { gCardRoom02Tiles, gCardRoom02Palette, gCardRoom02Frames, gTranquilDarknessSmallCardTiles, gTranquilDarknessSmallCardPalette, gTranquilDarknessSmallCardFrames, sizeof(gCardRoom02Tiles), sizeof(gCardRoom02Palette), sizeof(gTranquilDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TRANQUIL_DARKNESS, 9, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 0, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 1, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 2, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 3, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 4, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 5, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 6, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 7, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 8, MAP_CARD_COLOR_RED },
    { gCardRoom01Tiles, gCardRoom01Palette, gCardRoom01Frames, gTeemingDarknessSmallCardTiles, gTeemingDarknessSmallCardPalette, gTeemingDarknessSmallCardFrames, sizeof(gCardRoom01Tiles), sizeof(gCardRoom01Palette), sizeof(gTeemingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_TEEMING_DARKNESS, 9, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 0, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 1, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 2, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 3, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 4, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 5, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 6, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 7, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 8, MAP_CARD_COLOR_RED },
    { gCardRoom07Tiles, gCardRoom07Palette, gCardRoom07Frames, gFeebleDarknessSmallCardTiles, gFeebleDarknessSmallCardPalette, gFeebleDarknessSmallCardFrames, sizeof(gCardRoom07Tiles), sizeof(gCardRoom07Palette), sizeof(gFeebleDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_FEEBLE_DARKNESS, 9, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 0, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 1, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 2, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 3, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 4, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 5, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 6, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 7, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 8, MAP_CARD_COLOR_RED },
    { gCardRoom08Tiles, gCardRoom08Palette, gCardRoom08Frames, gAlmightyDarknessSmallCardTiles, gAlmightyDarknessSmallCardPalette, gAlmightyDarknessSmallCardFrames, sizeof(gCardRoom08Tiles), sizeof(gCardRoom08Palette), sizeof(gAlmightyDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_ALMIGHTY_DARKNESS, 9, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 0, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 1, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 2, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 3, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 4, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 5, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 6, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 7, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 8, MAP_CARD_COLOR_RED },
    { gCardRoom05Tiles, gCardRoom05Palette, gCardRoom05Frames, gSleepingDarknessSmallCardTiles, gSleepingDarknessSmallCardPalette, gSleepingDarknessSmallCardFrames, sizeof(gCardRoom05Tiles), sizeof(gCardRoom05Palette), sizeof(gSleepingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_SLEEPING_DARKNESS, 9, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 0, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 1, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 2, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 3, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 4, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 5, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 6, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 7, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 8, MAP_CARD_COLOR_RED },
    { gCardRoom04Tiles, gCardRoom04Palette, gCardRoom04Frames, gLoomingDarknessSmallCardTiles, gLoomingDarknessSmallCardPalette, gLoomingDarknessSmallCardFrames, sizeof(gCardRoom04Tiles), sizeof(gCardRoom04Palette), sizeof(gLoomingDarknessSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_LOOMING_DARKNESS, 9, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 0, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 1, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 2, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 3, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 4, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 5, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 6, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 7, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 8, MAP_CARD_COLOR_RED },
    { gCardRoom20Tiles, gCardRoom20Palette, gCardRoom20Frames, gPremiumRoomSmallCardTiles, gPremiumRoomSmallCardPalette, gPremiumRoomSmallCardFrames, sizeof(gCardRoom20Tiles), sizeof(gCardRoom20Palette), sizeof(gPremiumRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_PREMIUM_ROOM, 9, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 0, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 1, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 2, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 3, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 4, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 5, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 6, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 7, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 8, MAP_CARD_COLOR_RED },
    { gCardRoom21Tiles, gCardRoom21Palette, gCardRoom21Frames, gWhiteRoomSmallCardTiles, gWhiteRoomSmallCardPalette, gWhiteRoomSmallCardFrames, sizeof(gCardRoom21Tiles), sizeof(gCardRoom21Palette), sizeof(gWhiteRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_WHITE_ROOM, 9, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 0, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 1, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 2, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 3, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 4, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 5, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 6, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 7, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 8, MAP_CARD_COLOR_RED },
    { gCardRoom22Tiles, gCardRoom22Palette, gCardRoom22Frames, gBlackRoomSmallCardTiles, gBlackRoomSmallCardPalette, gBlackRoomSmallCardFrames, sizeof(gCardRoom22Tiles), sizeof(gCardRoom22Palette), sizeof(gBlackRoomSmallCardTiles), MAP_CARD_COLOR_RED, MAP_CARD_BLACK_ROOM, 9, MAP_CARD_COLOR_RED },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 0, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 1, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 2, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 3, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 4, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 5, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 6, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 7, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 8, MAP_CARD_COLOR_GREEN },
    { gCardRoom13Tiles, gCardRoom13Palette, gCardRoom13Frames, gMartialWakingSmallCardTiles, gMartialWakingSmallCardPalette, gMartialWakingSmallCardFrames, sizeof(gCardRoom13Tiles), sizeof(gCardRoom13Palette), sizeof(gMartialWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MARTIAL_WAKING, 9, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 0, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 1, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 2, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 3, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 4, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 5, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 6, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 7, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 8, MAP_CARD_COLOR_GREEN },
    { gCardRoom12Tiles, gCardRoom12Palette, gCardRoom12Frames, gSorcerousWakingSmallCardTiles, gSorcerousWakingSmallCardPalette, gSorcerousWakingSmallCardFrames, sizeof(gCardRoom12Tiles), sizeof(gCardRoom12Palette), sizeof(gSorcerousWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_SORCEROUS_WAKING, 9, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 0, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 1, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 2, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 3, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 4, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 5, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 6, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 7, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 8, MAP_CARD_COLOR_GREEN },
    { gCardRoom14Tiles, gCardRoom14Palette, gCardRoom14Frames, gAlchemicWakingSmallCardTiles, gAlchemicWakingSmallCardPalette, gAlchemicWakingSmallCardFrames, sizeof(gCardRoom14Tiles), sizeof(gCardRoom14Palette), sizeof(gAlchemicWakingSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_ALCHEMIC_WAKING, 9, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 0, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 1, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 2, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 3, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 4, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 5, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 6, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 7, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 8, MAP_CARD_COLOR_GREEN },
    { gCardRoom15Tiles, gCardRoom15Palette, gCardRoom15Frames, gMeetingGroundSmallCardTiles, gMeetingGroundSmallCardPalette, gMeetingGroundSmallCardFrames, sizeof(gCardRoom15Tiles), sizeof(gCardRoom15Palette), sizeof(gMeetingGroundSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_MEETING_GROUND, 9, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 0, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 1, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 2, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 3, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 4, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 5, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 6, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 7, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 8, MAP_CARD_COLOR_GREEN },
    { gCardRoom19Tiles, gCardRoom19Palette, gCardRoom19Frames, gStagnantSpaceSmallCardTiles, gStagnantSpaceSmallCardPalette, gStagnantSpaceSmallCardFrames, sizeof(gCardRoom19Tiles), sizeof(gCardRoom19Palette), sizeof(gStagnantSpaceSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STAGNANT_SPACE, 9, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 0, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 1, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 2, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 3, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 4, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 5, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 6, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 7, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 8, MAP_CARD_COLOR_GREEN },
    { gCardRoom17Tiles, gCardRoom17Palette, gCardRoom17Frames, gStrongInitiativeSmallCardTiles, gStrongInitiativeSmallCardPalette, gStrongInitiativeSmallCardFrames, sizeof(gCardRoom17Tiles), sizeof(gCardRoom17Palette), sizeof(gStrongInitiativeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_STRONG_INITIATIVE, 9, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 0, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 1, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 2, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 3, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 4, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 5, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 6, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 7, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 8, MAP_CARD_COLOR_GREEN },
    { gCardRoom18Tiles, gCardRoom18Palette, gCardRoom18Frames, gLastingDazeSmallCardTiles, gLastingDazeSmallCardPalette, gLastingDazeSmallCardFrames, sizeof(gCardRoom18Tiles), sizeof(gCardRoom18Palette), sizeof(gLastingDazeSmallCardTiles), MAP_CARD_COLOR_GREEN, MAP_CARD_LASTING_DAZE, 9, MAP_CARD_COLOR_GREEN },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 0, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 1, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 2, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 3, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 4, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 5, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 6, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 7, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 8, MAP_CARD_COLOR_BLUE },
    { gCardRoom09Tiles, gCardRoom09Palette, gCardRoom09Frames, gCalmBountySmallCardTiles, gCalmBountySmallCardPalette, gCalmBountySmallCardFrames, sizeof(gCardRoom09Tiles), sizeof(gCardRoom09Palette), sizeof(gCalmBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_CALM_BOUNTY, 9, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 0, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 1, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 2, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 3, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 4, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 5, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 6, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 7, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 8, MAP_CARD_COLOR_BLUE },
    { gCardRoom03Tiles, gCardRoom03Palette, gCardRoom03Frames, gGuardedTroveSmallCardTiles, gGuardedTroveSmallCardPalette, gGuardedTroveSmallCardFrames, sizeof(gCardRoom03Tiles), sizeof(gCardRoom03Palette), sizeof(gGuardedTroveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_GUARDED_TROVE, 9, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 0, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 1, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 2, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 3, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 4, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 5, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 6, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 7, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 8, MAP_CARD_COLOR_BLUE },
    { gCardRoom10Tiles, gCardRoom10Palette, gCardRoom10Frames, gFalseBountySmallCardTiles, gFalseBountySmallCardPalette, gFalseBountySmallCardFrames, sizeof(gCardRoom10Tiles), sizeof(gCardRoom10Palette), sizeof(gFalseBountySmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_FALSE_BOUNTY, 9, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 0, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 1, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 2, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 3, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 4, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 5, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 6, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 7, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 8, MAP_CARD_COLOR_BLUE },
    { gCardRoom06Tiles, gCardRoom06Palette, gCardRoom06Frames, gMomentsReprieveSmallCardTiles, gMomentsReprieveSmallCardPalette, gMomentsReprieveSmallCardFrames, sizeof(gCardRoom06Tiles), sizeof(gCardRoom06Palette), sizeof(gMomentsReprieveSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOMENTS_REPRIEVE, 9, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 0, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 1, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 2, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 3, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 4, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 5, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 6, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 7, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 8, MAP_CARD_COLOR_BLUE },
    { gCardRoom16Tiles, gCardRoom16Palette, gCardRoom16Frames, gMinglingWorldsSmallCardTiles, gMinglingWorldsSmallCardPalette, gMinglingWorldsSmallCardFrames, sizeof(gCardRoom16Tiles), sizeof(gCardRoom16Palette), sizeof(gMinglingWorldsSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MINGLING_WORLDS, 9, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 0, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 1, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 2, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 3, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 4, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 5, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 6, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 7, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 8, MAP_CARD_COLOR_BLUE },
    { gCardRoom11Tiles, gCardRoom11Palette, gCardRoom11Frames, gMoogleRoomSmallCardTiles, gMoogleRoomSmallCardPalette, gMoogleRoomSmallCardFrames, sizeof(gCardRoom11Tiles), sizeof(gCardRoom11Palette), sizeof(gMoogleRoomSmallCardTiles), MAP_CARD_COLOR_BLUE, MAP_CARD_MOOGLE_ROOM, 9, MAP_CARD_COLOR_BLUE },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 0, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 1, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 2, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 3, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 4, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 5, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 6, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 7, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 8, MAP_CARD_COLOR_GOLD },
    { gCardEve00Tiles, gCardEve00Palette, gCardEve00Frames, gKeyOfBeginningsSmallCardTiles, gKeyOfBeginningsSmallCardPalette, gKeyOfBeginningsSmallCardFrames, sizeof(gCardEve00Tiles), sizeof(gCardEve00Palette), sizeof(gKeyOfBeginningsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_BEGINNINGS, 9, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 0, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 1, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 2, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 3, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 4, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 5, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 6, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 7, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 8, MAP_CARD_COLOR_GOLD },
    { gCardEve01Tiles, gCardEve01Palette, gCardEve01Frames, gKeyOfGuidanceSmallCardTiles, gKeyOfGuidanceSmallCardPalette, gKeyOfGuidanceSmallCardFrames, sizeof(gCardEve01Tiles), sizeof(gCardEve01Palette), sizeof(gKeyOfGuidanceSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_OF_GUIDANCE, 9, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 0, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 1, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 2, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 3, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 4, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 5, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 6, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 7, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 8, MAP_CARD_COLOR_GOLD },
    { gCardEve02Tiles, gCardEve02Palette, gCardEve02Frames, gKeyToTruthSmallCardTiles, gKeyToTruthSmallCardPalette, gKeyToTruthSmallCardFrames, sizeof(gCardEve02Tiles), sizeof(gCardEve02Palette), sizeof(gKeyToTruthSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_TRUTH, 9, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 0, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 1, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 2, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 3, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 4, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 5, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 6, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 7, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 8, MAP_CARD_COLOR_GOLD },
    { gCardRoom23Tiles, gCardRoom23Palette, gCardRoom23Frames, gKeyToRewardsSmallCardTiles, gKeyToRewardsSmallCardPalette, gKeyToRewardsSmallCardFrames, sizeof(gCardRoom23Tiles), sizeof(gCardRoom23Palette), sizeof(gKeyToRewardsSmallCardTiles), MAP_CARD_COLOR_GOLD, MAP_CARD_KEY_TO_REWARDS, 9, MAP_CARD_COLOR_GOLD },
};

s16 gMapcardSlotX[6] = {
    20,
    57,
    94,
    131,
    168,
    205,
};

PrizeMapCardBackAnimStep gPrizeMapCardBackAnim[7] = {
    { 0, 5, 0, 0 },
    { 1, 5, 0, 0 },
    { 2, 5, 0, 0 },
    { 3, 5, 0, 0 },
    { 4, 5, 0, 0 },
    { 5, 5, 0, 0 },
    { 0, 100, 0, 0 },
};

TaskDesc gTaskDescMapcard = {
    "Mapcard",
    (TaskInitFunc)Mapcard_0,
    (TaskUpdateFunc)Mapcard_1,
    (TaskDrawFunc)Mapcard_2,
    (TaskDestroyFunc)Mapcard_3,
    sizeof(MapcardWork),
};

TaskDesc gTaskDescReloadGage = {
    "Reload Gage",
    (TaskInitFunc)Reload_Gage_0,
    (TaskUpdateFunc)Reload_Gage_1,
    (TaskDrawFunc)Reload_Gage_2,
    (TaskDestroyFunc)Reload_Gage_3,
    sizeof(CardDisplayWork),
};

void* gReloadCounterTiles[4] = {
    gReloadCounterWhiteTiles,
    gReloadCounterBlackTiles,
    gReloadCounterBlueTiles,
    gReloadCounterBlackTiles,
};

AnimHeader** gReloadCounterAnims[4] = {
    gReloadCounterWhiteAnims,
    gReloadCounterBlackAnims,
    gReloadCounterBlueAnims,
    gReloadCounterBlackAnims,
};

void** gReloadCounterFrames[4] = {
    gReloadCounterWhiteFrames,
    gReloadCounterBlackFrames,
    gReloadCounterBlueFrames,
    gReloadCounterBlackFrames,
};

void* gReloadCardTiles[4] = {
    gReloadCardGreenTiles,
    gRiCardF0RedTiles,
    gReloadCardBlackTiles,
    gReloadCardBlackTiles,
};

void** gReloadGaugeFrames[4] = {
    gReloadCardGreenFrames,
    gReloadCardBlackFrames,
    gReloadCardBlueFrames,
    gReloadCardBlackFrames,
};

AnimHeader** gReloadGaugeAnims[4] = {
    gReloadCardGreenAnims,
    gReloadCardBlackAnims,
    gReloadCardBlueAnims,
    gReloadCardBlackAnims,
};
