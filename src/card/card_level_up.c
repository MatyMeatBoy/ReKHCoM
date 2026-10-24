/**
 * card_level_up.c
 * Level-Up Bonus Screen
 */

#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "card_localized_data.h"
#include "registration_data.h"
#include "system_state.h"
#include "card_battle.h"
#include "m4a_song.h"
#include "game_state.h"
#include "text.h"
#include "fade.h"
#include "obj_api.h"
#include "battle_actor.h"
#include "display.h"
#include "engine_math.h"
#include "anim.h"
#include "obj.h"
#include "taskpool.h"
#include "key.h"
#include "card.h"
#include "sprites_evt.h"
#include "sprites_fld.h"
#include "sprites_level_up.h"
#include "sprites_sora.h"
#include "sprites_card.h"
#include "gba/io_reg.h"
#include "gba/keys.h"
#include "songs.h"
#include "battle.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "player_progression_types.h"
#include "types.h"
#include "gba/defines.h"
#include "sprite_palettes.h"
#include <stddef.h>
#include "card_level_up.h"
#include "default_bg_map.h"
#include "battle_ids.h"
#include "macros.h"
#ifdef PLATFORM_PC
#include "port/port_hdsprites.h"
#endif

#ifndef PORT_MENU_LEFT
#define PORT_MENU_LEFT(x) (x)
#define PORT_MENU_RIGHT(x) (x)
#endif
#ifndef PORT_BAND
#define PORT_BAND(draw) draw /* GBA: one band */
#endif

#ifdef VERSION_EU
static const u16 sLevelUpHeaderTileSizesByLanguage[5] = { 0x500, 0x500, 0x580, 0x500, 0x500 };

const u16 gLevelUpOptionTileSizesByLanguage[5] = { 0xE80, 0x1140, 0x1280, 0xF00, 0xF60 };

const u8 gLevelUpDisabledText[] = "\x19\x19\x19";

const u8* const gLevelUpDisabledTextByLanguage[5] = { gLevelUpDisabledText, gLevelUpDisabledText, gLevelUpDisabledText, gLevelUpDisabledText, gLevelUpDisabledText };
#elif defined(VERSION_JP)
const u8 gLevelUpDisabledText[] = "\x81\x7c\x81\x7c\x81\x7c";
#else
const u16 gLevelUpDisabledText[4] = { 0xE000, 0xE000, 0xE000, 0 };
#endif

static const s16 sLevelUpCursorY[3] = { 30, 78, 128 };

static const s16 sLevelUpApLevels[20] = { 2, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95 };

enum LevelUpState {
    LEVEL_UP_STATE_SLIDE_IN,
    LEVEL_UP_STATE_SELECT,
    LEVEL_UP_STATE_RESULT,
    LEVEL_UP_STATE_SLIDE_OUT
};

enum LevelUpStat {
    LEVEL_UP_STAT_LEVEL,
    LEVEL_UP_STAT_MAX_HP,
    LEVEL_UP_STAT_CP,
    LEVEL_UP_STAT_DP,
    LEVEL_UP_STAT_AP
};

void Level_Up_0(LevelUpWork* work) {
#ifdef PLATFORM_PC
    /* screen layer: bonus boxes left, stat panels right */
    PortBgSplit(0, 146);
    PortBgSplit(1, 146);
    PortBgSplitBands(0, 1);
    PortBgSplitBands(1, 1);
    /* the stat panels leave past the right edge; on the GBA that is off
       screen, in the wide view the wrapped map would reach the left margin */
    PortBgClip(gBtlWork->battleId == BATTLE_URSULA ? 0 : 1, 1);
#endif
    s16 x;
    s16 y;

    work->tiles4 = NULL;
    work->palette5 = NULL;
    work->tilesPalettes[0] = NULL;
    work->tilesPalettes[1] = NULL;
    work->tilesPalettes[2] = NULL;
    work->tilesPalettes[3] = NULL;
    work->tilesPalettes[4] = NULL;
    work->tilesPalettes[5] = NULL;
    work->tilesPalettes[6] = NULL;
    work->tilesPalettes[7] = NULL;
    work->palette = NULL;
    work->palette2 = NULL;
    work->tiles = NULL;
    work->palette3 = NULL;
    work->tiles2 = NULL;
    work->tiles3 = NULL;
    work->palette4 = NULL;
    work->optionEnabled[0] = 1;
    work->optionEnabled[1] = 1;
    work->optionEnabled[2] = 1;

    if (gCardBattleState != NULL) {
        gCardBattleState->levelUpShown = TRUE;
    }

#ifndef VERSION_EU
    InitTextSlots(work->textSlots[0], ARRAY_COUNT(work->textSlots[0]));
    InitTextSlots(work->textSlots[1], ARRAY_COUNT(work->textSlots[0]));
    InitTextSlots(work->textSlots[2], ARRAY_COUNT(work->textSlots[0]));
    InitTextSlots(work->textSlots[3], ARRAY_COUNT(work->textSlots[0]));
    InitTextSlots(work->textSlots[4], ARRAY_COUNT(work->textSlots[0]));
    InitTextSlots(work->textSlots[5], ARRAY_COUNT(work->textSlots[0]));
    work->tilesPalettes[6] = LoadObjTiles(gLevelUpHeaderTiles, sizeof(gLevelUpHeaderTiles));
#else
    work->tilesPalettes[6] = LoadObjTiles(gLevelUpHeaderTilesByLanguage[gLanguage], sLevelUpHeaderTileSizesByLanguage[gLanguage]);
    work->tiles5[0] = AllocSpriteFrameTiles(0x500);
    work->tiles5[1] = AllocSpriteFrameTiles(0x500);
    work->tiles5[2] = AllocSpriteFrameTiles(0x500);
#endif
    work->tilesPalettes[7] = LoadObjPalette(gLevelUpHeaderPalette, sizeof(gLevelUpHeaderPalette));
    FadeSetPaletteExcluded(((ObjPalette*)work->tilesPalettes[7])->index + 16, TRUE);
    work->tiles2 = LoadObjTiles(gLevelUpBarTiles, sizeof(gLevelUpBarTiles));
    TaskPoolInit(&work->pool, 10);

    if (!(gGameState.flags & GAME_FLAG_RIKU)) {
        work->tiles4 = AllocObjTiles(0x500, NULL);
        work->palette5 = AllocObjPalette(32);
        UpdateAllocatedObjPalette(work->palette5, gSoraPalette);
        FadeSetPaletteExcluded(work->palette5->index + 16, TRUE);
        WorldToScreen(&x, &y, gBtlWork->actor->x,
                      gBtlWork->actor->y,
                      gBtlWork->actor->z);
        SetObjTileSource(work->tiles4, gSor1ll51Tiles);
        AnimInit(&work->anim2, gSor1ll51Anims, gSor1ll51Frames);
        AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    } else {
        work->tiles4 = AllocObjTiles(0x800, NULL);
        work->palette5 = AllocObjPalette(32);
        UpdateAllocatedObjPalette(work->palette5, gRikuPalette);
        FadeSetPaletteExcluded(work->palette5->index + 16, TRUE);
        WorldToScreen(&x, &y, gBtlWork->actor->x,
                      gBtlWork->actor->y,
                      gBtlWork->actor->z);
        SetObjTileSource(work->tiles4, gRikuBt00Tiles);
        AnimInit(&work->anim2, gRikuBt00Anims, gRikuBt00Frames);
        AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    }

    if (gBtlWork->flags & BTL_FLAG_FIELD_HIDDEN) {
        work->playerX = 0x1C400;
        work->playerY = 0x5000;
    } else {
        work->playerX = x << 8;
        work->playerY = y << 8;
    }

    work->gfx = AnimGetGfx(&work->anim2);

    if (!(gBtlWork->flags & BTL_FLAG_BOSS_BATTLE)) {
        work->bossBattle = FALSE;
        gDispCnt = (gDispCnt & ~DISPCNT_MODE_MASK) | DISPCNT_MODE_1;
        gBg1Cnt &= ~BGCNT_256COLOR;
        SetBgSize(1, BGCNT_TXT256x256);
        SetupBg(2, 0, 12, 0);
        SetupBg(1, 2, 24, 0);
        SetupBg(0, 2, 25, 0);
        LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
        LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
        EnableBg(2);
        DisableBg(1);
        DisableBg(0);
        gBg2Cnt = gBg3Cnt;
        gBg2PA = gBg3PA;
        gBg2PB = gBg3PB;
        gBg2PC = gBg3PC;
        gBg2PD = gBg3PD;
        gBg2X = gBg3X;
        gBg2Y = gBg3Y;
    } else {
        work->bossBattle = TRUE;

        switch (gBtlWork->battleId) {
        case BATTLE_URSULA:
            SetBgSize(0, BGCNT_TXT256x256);
            SetupBg(0, 0, 26, 0);
            LoadBgMap(0, gDefaultBgMap, sizeof(gDefaultBgMap));
            DisableBg(0);
            break;
        case BATTLE_PARASITE_CAGE:
            SetBgSize(1, BGCNT_TXT256x256);
            SetupBg(1, 1, 24, 0);
            LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
            DisableBg(1);
            break;
        case BATTLE_GUARD_ARMOR:
            SetBgSize(1, BGCNT_TXT256x256);
            SetupBg(1, 2, 26, 0);
            LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
            DisableBg(1);
            break;
        default:
            SetBgSize(1, BGCNT_TXT256x256);
            LoadBgMap(1, gDefaultBgMap, sizeof(gDefaultBgMap));
            DisableBg(1);
            break;
        }
    }

    gBldCnt &= ~BLDCNT_EFFECT_BLEND;
    work->headerX = -128;
    work->bonusX[0] = -128;
    work->bonusX[1] = -128;
    work->bonusX[2] = -128;
    work->bonusY[0] = 16;
    work->bonusY[1] = 64;
    work->bonusY[2] = 112;
    work->slideSteps = 24;
    work->headerSteps = 16;
    work->optionSteps[0] = 16;
    work->optionSteps[1] = 16;
    work->optionSteps[2] = 16;
    work->textX[0] = 8;
    work->textX[1] = 8;
    work->textX[2] = 8;
    work->textY[0] = 31;
    work->textY[1] = 79;
    work->textY[2] = 127;
    work->topBarX = 128;
    work->bottomBarX = 128;
    work->topBarY = -0x800;
    work->bottomBarY = 0xA000;
    work->barSteps = 16;
    work->bgScrollX = 0;
    work->statsOffsetX = 256;
    work->cursorX = 132;
    work->cursorY = sLevelUpCursorY[0];
    work->state = LEVEL_UP_STATE_SLIDE_IN;
    work->cursor = 0;
    work->applied = FALSE;
    LevelUpSplitDigits3(gGameState.progression.level, work->levelDigits);
    LevelUpSplitDigits3(gGameState.progression.maxHp, work->maxHpDigits);
    LevelUpSplitDigits4(gGameState.progression.cp, work->cpDigits);
    LevelUpSplitDigits3(gGameState.progression.dp, work->dpDigits);
    LevelUpSplitDigits2(gGameState.progression.ap, work->apDigits);
    work->timer = 0;
    work->loaded[1] = 0;
    work->loaded[0] = 0;
    work->blinkTimer = 0;
    work->blinkOn = 0;
    work->playerSteps = 16;
    work->effectShown = 0;
}
#ifdef VERSION_EU
#define CARD_E7A4_DST 0x20
#else
#define CARD_E7A4_DST 0x2480
#endif
void LoadLevelUpRikuBgTiles() {
    u8* base;

    if (gBtlWork->battleId == BATTLE_URSULA) {
        base = GetBgCharBase(0);
        RequestDma3Copy(gLevelUpRikuIconTiles, base + CARD_E7A4_DST, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x400], base + CARD_E7A4_DST + 0x120, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x800], base + CARD_E7A4_DST + 0x240, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[288], base + CARD_E7A4_DST + 0x360, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x520], base + CARD_E7A4_DST + 0x480, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x920], base + CARD_E7A4_DST + 0x5A0, 288);
#ifdef VERSION_EU
        RequestDma3Copy((u8*)gLevelUpBgTilesByLanguage[gLanguage] + 0xC00, base + 0x800, 0xA80);
#else
        RequestDma3Copy(gLevelUpRikuStatLabelTiles, base + 0x2C00, 0xA80);
#endif
    } else {
        base = GetBgCharBase(1);
        RequestDma3Copy(gLevelUpRikuIconTiles, base + CARD_E7A4_DST, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x400], base + CARD_E7A4_DST + 0x120, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x800], base + CARD_E7A4_DST + 0x240, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[288], base + CARD_E7A4_DST + 0x360, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x520], base + CARD_E7A4_DST + 0x480, 288);
        RequestDma3Copy(&gLevelUpRikuIconTiles[0x920], base + CARD_E7A4_DST + 0x5A0, 288);
#ifdef VERSION_EU
        RequestDma3Copy((u8*)gLevelUpBgTilesByLanguage[gLanguage] + 0xC00, base + 0x800, 0xA80);
#else
        RequestDma3Copy(gLevelUpRikuStatLabelTiles, base + 0x2C00, 0xA80);
#endif
    }
}

u8 Level_Up_1(LevelUpWork* work, void* task) {
    s32 bonusX[3];
#ifdef VERSION_EU
    enum { tileSize = 0xC80, mapSize = 0x500, bgSize = 0x2C00 };
#else
    enum { tileSize = 0xD80, mapSize = 0x800, bgSize = 0x3680 };
#endif

    if (work->loaded[1] == 0) {
        work->timer++;

        if (work->timer > 7) {
            if (!work->bossBattle) {
                LoadBgTiles(1, gLevelUpBgTiles, bgSize);

#ifdef VERSION_EU
                switch (gLanguage) {
                case LANGUAGE_ENGLISH:
                    break;
                case LANGUAGE_FRENCH:
                    RequestDma3Copy(gLevelUpBannerFrenchTiles, GetBgCharBase(1) + 0x2400, sizeof(gLevelUpBannerFrenchTiles));
                    RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                    break;
                case LANGUAGE_GERMAN:
                    // @bug Reads past the end of gLevelUpBannerGermanTiles.
                    RequestDma3Copy(gLevelUpBannerGermanTiles, GetBgCharBase(1) + 0x2400, 0x800);
                    RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                    break;
                case LANGUAGE_ITALIAN:
                    RequestDma3Copy(gLevelUpBannerItalianTiles, GetBgCharBase(1) + 0x2400, sizeof(gLevelUpBannerItalianTiles));
                    RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                    break;
                case LANGUAGE_SPANISH:
                    RequestDma3Copy(gLevelUpBannerSpanishTiles, GetBgCharBase(1) + 0x2400, sizeof(gLevelUpBannerSpanishTiles));
                    RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                    break;
                }
#endif

                if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                    LoadPalette(gLevelUpSoraStatsPalettes, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), sizeof(gLevelUpSoraStatsPalettes));
                } else {
                    LoadLevelUpRikuBgTiles();
                    LoadPalette(gLevelUpRikuStatsPalettes, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), sizeof(gLevelUpRikuStatsPalettes));
                }

                LoadBgMap(1, gLevelUpStatsMap, mapSize);
                SetBgMapBlocks(1, gLevelUpBgMapBlocks, 2, 1);
                RedrawBgMapAt(1, 0, 0);
            } else {
                if (gBtlWork->battleId == BATTLE_URSULA) {
                    LoadBgTiles(0, gLevelUpBgTiles, bgSize);

#ifdef VERSION_EU
                    switch (gLanguage) {
                    case LANGUAGE_ENGLISH:
                        break;
                    case LANGUAGE_FRENCH:
                        RequestDma3Copy(gLevelUpBannerFrenchTiles, GetBgCharBase(0) + 0x2400, sizeof(gLevelUpBannerFrenchTiles));
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(0) + 0x800, 0xC00);
                        break;
                    case LANGUAGE_GERMAN:
                        // @bug Reads past the end of gLevelUpBannerGermanTiles.
                        RequestDma3Copy(gLevelUpBannerGermanTiles, GetBgCharBase(0) + 0x2400, 0x800);
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(0) + 0x800, 0xC00);
                        break;
                    case LANGUAGE_ITALIAN:
                        RequestDma3Copy(gLevelUpBannerItalianTiles, GetBgCharBase(0) + 0x2400, sizeof(gLevelUpBannerItalianTiles));
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(0) + 0x800, 0xC00);
                        break;
                    case LANGUAGE_SPANISH:
                        RequestDma3Copy(gLevelUpBannerSpanishTiles, GetBgCharBase(0) + 0x2400, sizeof(gLevelUpBannerSpanishTiles));
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(0) + 0x800, 0xC00);
                        break;
                    }
#endif

                    if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                        LoadPalette(gLevelUpSoraStatsPalettes, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), sizeof(gLevelUpSoraStatsPalettes));
                    } else {
                        LoadLevelUpRikuBgTiles();
                        LoadPalette(gLevelUpRikuStatsPalettes, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), sizeof(gLevelUpRikuStatsPalettes));
                    }

                    SetBgMapBlocks(0, gLevelUpBgMapBlocks, 2, 1);
                    RedrawBgMapAt(0, 0, 0);
                } else {
                    LoadBgTiles(1, gLevelUpBgTiles, bgSize);

#ifdef VERSION_EU
                    switch (gLanguage) {
                    case LANGUAGE_ENGLISH:
                        break;
                    case LANGUAGE_FRENCH:
                        RequestDma3Copy(gLevelUpBannerFrenchTiles, GetBgCharBase(1) + 0x2400, sizeof(gLevelUpBannerFrenchTiles));
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                        break;
                    case LANGUAGE_GERMAN:
                        // @bug Reads past the end of gLevelUpBannerGermanTiles.
                        RequestDma3Copy(gLevelUpBannerGermanTiles, GetBgCharBase(1) + 0x2400, 0x800);
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                        break;
                    case LANGUAGE_ITALIAN:
                        RequestDma3Copy(gLevelUpBannerItalianTiles, GetBgCharBase(1) + 0x2400, sizeof(gLevelUpBannerItalianTiles));
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                        break;
                    case LANGUAGE_SPANISH:
                        RequestDma3Copy(gLevelUpBannerSpanishTiles, GetBgCharBase(1) + 0x2400, sizeof(gLevelUpBannerSpanishTiles));
                        RequestDma3Copy(gLevelUpBgTilesByLanguage[gLanguage], GetBgCharBase(1) + 0x800, 0xC00);
                        break;
                    }
#endif

                    if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                        LoadPalette(gLevelUpSoraStatsPalettes, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), sizeof(gLevelUpSoraStatsPalettes));
                    } else {
                        LoadLevelUpRikuBgTiles();
                        LoadPalette(gLevelUpRikuStatsPalettes, (void*)(BG_PLTT + 10 * PLTT_SIZE_4BPP), sizeof(gLevelUpRikuStatsPalettes));
                    }

                    SetBgMapBlocks(1, gLevelUpBgMapBlocks, 2, 1);
                    RedrawBgMapAt(1, 0, 0);
                }
            }

            FadeSetPaletteExcluded(10, TRUE);
            FadeSetPaletteExcluded(11, TRUE);
            FadeSetPaletteExcluded(12, TRUE);
            FadeSetPaletteExcluded(13, TRUE);
            work->loaded[1] = 1;
            work->timer = 0;
            return 1;
        }

        return 1;
    }

    if (work->loaded[0] == 0) {
        work->timer++;

        if (work->timer > 7) {
            if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                work->tilesPalettes[0] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[1] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[2] = AllocSpriteFrameTiles(tileSize);

#ifdef VERSION_EU
                if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpSoraBonusFrames[0], gLevelUpSoraBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpSoraBonusFrames[1], gLevelUpSoraBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpSoraBonusFrames[2], gLevelUpSoraBonusTiles);
#ifdef VERSION_EU
                } else {
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpSoraBonusItalianFrames[0], gLevelUpSoraBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpSoraBonusItalianFrames[1], gLevelUpSoraBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpSoraBonusItalianFrames[2], gLevelUpSoraBonusItalianTiles);
                }
#endif

                work->tilesPalettes[4] = LoadObjPalette(gLevelUpSoraBonusPalette, sizeof(gLevelUpSoraBonusPalette));
                work->tilesPalettes[5] = LoadObjPalette(gLevelUpSoraBonusSelectedPalette, sizeof(gLevelUpSoraBonusSelectedPalette));
            } else {
                work->tilesPalettes[0] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[1] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[2] = AllocSpriteFrameTiles(tileSize);

#ifdef VERSION_EU
                if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpRikuBonusFrames[0], gLevelUpRikuBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpRikuBonusFrames[1], gLevelUpRikuBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpRikuBonusFrames[2], gLevelUpRikuBonusTiles);
#ifdef VERSION_EU
                } else {
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpRikuBonusItalianFrames[0], gLevelUpRikuBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpRikuBonusItalianFrames[1], gLevelUpRikuBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpRikuBonusItalianFrames[2], gLevelUpRikuBonusItalianTiles);
                }
#endif

                work->tilesPalettes[4] = LoadObjPalette(gLevelUpRikuBonusPalette, sizeof(gLevelUpRikuBonusPalette));
                work->tilesPalettes[5] = LoadObjPalette(gLevelUpRikuBonusSelectedPalette, sizeof(gLevelUpRikuBonusSelectedPalette));
            }

#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                work->tiles3 = LoadObjTiles(gLvupLogoTiles, sizeof(gLvupLogoTiles));
                break;
            case LANGUAGE_FRENCH:
                work->tiles3 = LoadObjTiles(gLvupLogoFrenchTiles, sizeof(gLvupLogoFrenchTiles));
                break;
            case LANGUAGE_GERMAN:
                work->tiles3 = LoadObjTiles(gLvupLogoGermanTiles, sizeof(gLvupLogoGermanTiles));
                break;
            case LANGUAGE_ITALIAN:
                work->tiles3 = LoadObjTiles(gLvupLogoItalianTiles, sizeof(gLvupLogoItalianTiles));
                break;
            case LANGUAGE_SPANISH:
                work->tiles3 = LoadObjTiles(gLvupLogoSpanishTiles, sizeof(gLvupLogoSpanishTiles));
                break;
            default:
                work->tiles3 = LoadObjTiles(gLvupLogoTiles, sizeof(gLvupLogoTiles));
                break;
            }
#else
            work->tiles3 = LoadObjTiles(gLvupLogoTiles, sizeof(gLvupLogoTiles));
#endif
            work->palette4 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
            FadeSetPaletteExcluded(((ObjPalette*)work->tilesPalettes[4])->index + 16, TRUE);
            FadeSetPaletteExcluded(((ObjPalette*)work->tilesPalettes[5])->index + 16, TRUE);
            FadeSetPaletteExcluded(work->palette4->index + 16, TRUE);
            work->tiles = AllocObjTiles(0x3C0, NULL);
            work->palette3 = LoadObjPalette(gSmallHandCursorPalette, sizeof(gSmallHandCursorPalette));
            FadeSetPaletteExcluded(work->palette3->index + 16, TRUE);
            SetObjTileSource(work->tiles, gSmallHandCursorTiles);
            AnimInit(&work->anim, gSmallHandCursorAnims, gSmallHandCursorFrames);
            AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
            work->gfx2 = AnimGetGfx(&work->anim);
            work->loaded[0] = 1;
            work->timer = 0;
            FadeToAmount(FADE_MODE_BLACK, 8, 16);
        }

        return 1;
    }

    work->gfx = AnimUpdate(&work->anim2);

    if (work->playerSteps != 0) {
        ApproachValue(&work->playerX, 0xBE00, work->playerSteps);
        ApproachValue(&work->playerY, 0x5000, work->playerSteps);
        work->playerSteps--;
    }

    if (work->barSteps != 0) {
        ApproachValue(&work->topBarY, 0, work->barSteps);
        ApproachValue(&work->bottomBarY, 0x9800, work->barSteps);
        work->barSteps--;
    } else {
        s32 bgScrollX = work->bgScrollX << 8;
        s32 statsOffsetX = work->statsOffsetX << 8;
        ApproachValue(&bgScrollX, 0x10000, work->slideSteps);
        ApproachValue(&statsOffsetX, 0, work->slideSteps);

        if (gBtlWork->battleId == BATTLE_URSULA) {
            ScrollBgMapTo(0, bgScrollX >> 8, 0);
        } else {
            ScrollBgMapTo(1, bgScrollX >> 8, 0);
        }

        work->bgScrollX = bgScrollX >> 8;
        work->statsOffsetX = statsOffsetX >> 8;

        if (work->slideSteps > 0) {
            work->slideSteps--;
        }

        if (work->slideSteps <= 11) {
            s32 headerX = work->headerX << 8;
            ApproachValue(&headerX, 0, work->headerSteps);
            work->headerX = headerX >> 8;

            if (work->headerSteps > 0) {
                work->headerSteps--;
            }

            if (work->headerSteps <= 6) {
                bonusX[0] = work->bonusX[0] << 8;
                bonusX[1] = work->bonusX[1] << 8;
                bonusX[2] = work->bonusX[2] << 8;
                ApproachValue(&bonusX[0], 0x1000, work->optionSteps[0]);

                if (work->optionSteps[0] > 0) {
                    work->optionSteps[0]--;
                }

                if (work->optionSteps[0] <= 6) {
                    ApproachValue(&bonusX[1], 0x1000, work->optionSteps[1]);

                    if (work->optionSteps[1] > 0) {
                        work->optionSteps[1]--;
                    }
                }

                if (work->optionSteps[1] <= 6) {
                    ApproachValue(&bonusX[2], 0x1000, work->optionSteps[2]);

                    if (work->optionSteps[2] > 0) {
                        work->optionSteps[2]--;
                    }
                }

                work->bonusX[0] = bonusX[0] >> 8;
                work->bonusX[1] = bonusX[1] >> 8;
                work->bonusX[2] = bonusX[2] >> 8;

                if (work->optionSteps[2] == 0) {
                    u8 i;

                    if (gBtlWork->battleId == BATTLE_URSULA) {
                        LoadBgMap(0, gLevelUpBonusAMap, mapSize);
                    } else {
                        LoadBgMap(1, gLevelUpBonusAMap, mapSize);
                    }

                    work->state = LEVEL_UP_STATE_SELECT;

                    if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                        if (gGameState.progression.maxHp > 559) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[0] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[0]);
                            work->textSlotCounts[3] = 0;
#endif
                            work->optionEnabled[0] = 0;
                        } else {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][0], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[0] = LoadTextSlots(gLevelUpSoraTexts[0], work->textSlots[0]);
                            work->textSlotCounts[3] = LoadTextSlots(gLevelUpSoraTexts[3], work->textSlots[3]);
#endif
                        }

                        if (gGameState.progression.cp > 1899) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[1]);
                            work->textSlotCounts[4] = 0;
#endif
                            work->optionEnabled[1] = 0;
                        } else {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][1], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[1], work->textSlots[1]);
                            work->textSlotCounts[4] = LoadTextSlots(gLevelUpSoraTexts[4], work->textSlots[4]);
#endif
                        }

                        if (gGameState.progression.levelMilestone > 10) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[2]);
                            work->textSlotCounts[5] = 0;
#endif
                            work->optionEnabled[2] = 0;
                        } else if (!IsLevelUpStockUnlocked()) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[2]);
                            work->textSlotCounts[5] = 0;
#endif
                            work->optionEnabled[2] = 0;
                        } else {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][2], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[2], work->textSlots[2]);
                            work->textSlotCounts[5] = LoadTextSlots(gLevelUpSoraTexts[5], work->textSlots[5]);
#endif
                        }

                        work->palette = LoadObjPalette(gLevelUpSoraOptionSelectedPalette, sizeof(gLevelUpSoraOptionSelectedPalette));
                        work->palette2 = LoadObjPalette(gLevelUpSoraOptionPalette, sizeof(gLevelUpSoraOptionPalette));
                    } else {
                        if (gGameState.progression.maxHp > 559) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[0] = LoadTextSlots(gLevelUpRikuTexts[6], work->textSlots[0]);
                            work->textSlotCounts[3] = 0;
#endif
                            work->optionEnabled[0] = 0;
                        } else {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][3], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[0] = LoadTextSlots(gLevelUpRikuTexts[0], work->textSlots[0]);
                            work->textSlotCounts[3] = LoadTextSlots(gLevelUpRikuTexts[3], work->textSlots[3]);
#endif
                        }

                        if (gGameState.progression.ap > 29) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[1]);
                            work->textSlotCounts[4] = 0;
#endif
                            work->optionEnabled[1] = 0;
                        } else if (!(u8)IsLevelUpApUnlocked()) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[1]);
                            work->textSlotCounts[4] = 0;
#endif
                            work->optionEnabled[1] = 0;
                        } else {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][4], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[1] = LoadTextSlots(gLevelUpRikuTexts[1], work->textSlots[1]);
                            work->textSlotCounts[4] = LoadTextSlots(gLevelUpRikuTexts[4], work->textSlots[4]);
#endif
                        }

                        if (gGameState.progression.dp > 299) {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[2]);
                            work->textSlotCounts[5] = 0;
#endif
                            work->optionEnabled[2] = 0;
                        } else {
#ifdef VERSION_EU
                            UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][5], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                            work->textSlotCounts[2] = LoadTextSlots(gLevelUpRikuTexts[2], work->textSlots[2]);
                            work->textSlotCounts[5] = LoadTextSlots(gLevelUpRikuTexts[5], work->textSlots[5]);
#endif
                        }

                        work->palette = LoadObjPalette(gLevelUpRikuOptionSelectedPalette, sizeof(gLevelUpRikuOptionSelectedPalette));
                        work->palette2 = LoadObjPalette(gLevelUpRikuOptionPalette, sizeof(gLevelUpRikuOptionPalette));
                    }

                    FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
                    FadeSetPaletteExcluded(work->palette2->index + 16, TRUE);

                    for (i = 0; i < ARRAY_COUNT(work->optionEnabled); i++) {
                        if (work->optionEnabled[i] == 1) {
                            break;
                        }
                    }

                    work->cursor = i;
                    work->cursorY = sLevelUpCursorY[work->cursor];
                    SetTaskUpdate(task, (TaskUpdateFunc)UpdateLevelUpSelect);

                    if (gBtlWork->battleId == BATTLE_URSULA) {
                        LoadBgMap(0, gLevelUpOptionBgMaps[work->cursor], 0x800);
                    } else {
                        LoadBgMap(1, gLevelUpOptionBgMaps[work->cursor], 0x800);
                    }
                }
            }
        }
    }

    TaskPoolUpdate(&work->pool);
    return 1;
}

u8 UpdateLevelUpSelect(LevelUpWork* work, void* task) {
    s32 cursorY;
    s8 i;
    u8* optionEnabled;
#ifdef VERSION_EU
    enum { mapSize = 0x500 };
#else
    enum { mapSize = 0x800 };
#endif

    if (GetKeysRepeat() & DPAD_DOWN) {
        i = work->cursor;
        optionEnabled = work->optionEnabled;

        do {
            i++;

            if (i > 2) {
                i = 0;
            }
        } while (optionEnabled[i] == 0);

        if (i != work->cursor) {
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        work->cursor = i;

        if (gBtlWork->battleId == BATTLE_URSULA) {
            LoadBgMap(0, gLevelUpOptionBgMaps[work->cursor], 0x800);
        } else {
            LoadBgMap(1, gLevelUpOptionBgMaps[work->cursor], 0x800);
        }

        work->cursorSteps = 8;
    }

    if (GetKeysRepeat() & DPAD_UP) {
        i = work->cursor;
        optionEnabled = work->optionEnabled;

        do {
            i--;

            if (i < 0) {
                i = 2;
            }
        } while (optionEnabled[i] == 0);

        if (i != work->cursor) {
            m4aSongNumStart(SONG_SYS_CLICK);
        }

        work->cursor = i;

        if (gBtlWork->battleId == BATTLE_URSULA) {
            LoadBgMap(0, gLevelUpOptionBgMaps[work->cursor], 0x800);
        } else {
            LoadBgMap(1, gLevelUpOptionBgMaps[work->cursor], 0x800);
        }

        work->cursorSteps = 8;
    }

    while (GetKeysRepeat() & A_BUTTON) {
        work->state = LEVEL_UP_STATE_RESULT;

        if (gBtlWork->battleId == BATTLE_URSULA) {
            LoadBgMap(0, gLevelUpChosenMap, mapSize);
        } else {
            LoadBgMap(1, gLevelUpChosenMap, mapSize);
        }

        work->optionSteps[0] = 16;
        work->optionSteps[1] = 16;
        work->optionSteps[2] = 16;
        m4aSongNumStart(SONG_SYS_KETTEI);
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette3);
        work->tiles = AllocObjTiles(128, NULL);
        work->palette3 = LoadObjPalette(gLevelUpChosenCursorPalette, sizeof(gLevelUpChosenCursorPalette));
        FadeSetPaletteExcluded(work->palette3->index + 16, TRUE);
        SetObjTileSource(work->tiles, gLevelUpChosenCursorTiles);
        AnimInit(&work->anim, gLevelUpChosenCursorAnims, gLevelUpChosenCursorFrames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
        work->gfx2 = AnimGetGfx(&work->anim);
        work->cursorSteps = 16;
        work->cursorX = 136;

        if (!(gGameState.flags & GAME_FLAG_RIKU)) {
            SetObjTileSource(work->tiles4, gSor1ff00Tiles);
            AnimInit(&work->anim2, gSor1ff00Anims, gSor1ff00Frames);
            AnimStart(&work->anim2, 1, 0);
        } else {
            SetObjTileSource(work->tiles4, gRikuFf00Tiles);
            AnimInit(&work->anim2, gRikuFf00Anims, gRikuFf00Frames);
            AnimStart(&work->anim2, 1, 0);
        }

        SetTaskUpdate(task, (TaskUpdateFunc)UpdateLevelUpResult);
        return 1;
    }

    cursorY = work->cursorY << 8;
    ApproachValue(&cursorY, sLevelUpCursorY[work->cursor] << 8, work->cursorSteps);
    work->cursorSteps--;
    work->cursorY = cursorY >> 8;
    work->gfx2 = AnimUpdate(&work->anim);
    work->blinkTimer++;

    if (work->blinkTimer == 32) {
        work->blinkTimer = 0;
        work->blinkOn ^= 1;
    }

    work->gfx = AnimUpdate(&work->anim2);
    TaskPoolUpdate(&work->pool);
    return 1;
}

u8 UpdateLevelUpResult(LevelUpWork* work, void* task) {
    u8 i;

    if (work->effectShown == 0) {
#ifndef VERSION_US
        LevelUpEffectArgs args;
        args.x = 192;
        args.y = 60;
        args.unk_08 = 0;
        args.target = NULL;
        TaskCreate(&work->pool, &gTaskDescLVUPEFFECT, &args);
        m4aSongNumStart(SONG_SYS_LVUP);
#endif
        work->effectShown++;
    }

    for (i = 0; i < ARRAY_COUNT(work->bonusX); i++) {
        if (i != work->cursor) {
            s32 bonusX = work->bonusX[i] << 8;
            s32 textX = work->textX[i] << 8;
            ApproachValue(&bonusX, -0x10000, work->optionSteps[i]);
            ApproachValue(&textX, -0xF800, work->optionSteps[i]);
            work->bonusX[i] = bonusX >> 8;
            work->textX[i] = textX >> 8;
        } else {
            s32 bonusY = work->bonusY[i] << 8;
            s32 textY = work->textY[i] << 8;
            ApproachValue(&bonusY, 0x2000, work->optionSteps[i]);
            ApproachValue(&textY, 0x3100, work->optionSteps[i]);
            work->bonusY[i] = bonusY >> 8;
            work->textY[i] = textY >> 8;
        }

        if (work->optionSteps[i] > 0) {
            work->optionSteps[i]--;
        }
    }

    if (work->optionSteps[0] == 0) {
        for (i = 0; i < 3; i++) {
            if (i != work->cursor && work->tilesPalettes[i] != NULL) {
                ReleaseObjTiles(work->tilesPalettes[i]);
                work->tilesPalettes[i] = NULL;
            }
        }

        if (!work->applied) {
            switch (work->cursor) {
            case 0: {
                s32 amount = LevelUpMaxHp();
                StatIncreaseDisplayArgs args;
                work->messageActive = 1;
                args.amount = amount;
                args.done = &work->messageActive;
                args.flags = STAT_INCREASE_FLAG_MAX_HP;
                TaskCreate(&work->pool, &gTaskDescLvupMsg, &args);
                break;
            }
            case 1:
                if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                    s32 amount = LevelUpCp();
                    StatIncreaseDisplayArgs args;
                    work->messageActive = 1;
                    args.amount = amount;
                    args.done = &work->messageActive;
                    args.flags = 0;
                    TaskCreate(&work->pool, &gTaskDescLvupMsg, &args);
                } else {
                    s32 amount = LevelUpAp();
                    StatIncreaseDisplayArgs args;
                    work->messageActive = 1;
                    args.amount = amount;
                    args.done = &work->messageActive;
                    args.flags = 0;
                    gGameState.progression.levelMilestone++;
                    TaskCreate(&work->pool, &gTaskDescLvupMsg, &args);
                }

                break;
            case 2:
                if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                    work->messageActive = 1;
                    TaskCreate(&work->pool, &gTaskDescStockInfo, &work->messageActive);
                } else {
                    s32 amount = LevelUpDp();
                    StatIncreaseDisplayArgs args;
                    work->messageActive = 1;
                    args.amount = amount;
                    args.done = &work->messageActive;
                    args.flags = STAT_INCREASE_FLAG_DP;
                    TaskCreate(&work->pool, &gTaskDescLvupMsg, &args);
                }

                break;
            }

            LevelUpSplitDigits3(gGameState.progression.maxHp, work->maxHpDigits);
            LevelUpSplitDigits4(gGameState.progression.cp, work->cpDigits);
            LevelUpSplitDigits3(gGameState.progression.dp, work->dpDigits);
            LevelUpSplitDigits2(gGameState.progression.ap, work->apDigits);
            work->applied = TRUE;
        }
    }

    {
        s32 cursorY = work->cursorY << 8;
        ApproachValue(&cursorY, 0x3000, work->cursorSteps);
        work->cursorSteps--;
        work->cursorY = cursorY >> 8;
    }

    TaskPoolUpdate(&work->pool);
    work->blinkTimer++;

    if (work->blinkTimer == 32) {
        work->blinkTimer = 0;
        work->blinkOn ^= 1;
    }

    if (work->timer == 180) {
        if (GetKeysPressed() & A_BUTTON) {
            if (gBtlWork->battleId == BATTLE_URSULA) {
                SetBgMapBlocks(0, gLevelUpBgMapBlocks, 2, 1);
            } else {
                SetBgMapBlocks(1, gLevelUpBgMapBlocks, 2, 1);
            }

            work->headerSteps = 16;
            work->slideSteps = 16;
            work->optionSteps[work->cursor] = 16;
            work->barSteps = 16;
            work->state = LEVEL_UP_STATE_SLIDE_OUT;
            work->blinkOn = 0;
            work->messageActive = 0;
            gBtlWork->pendingLevelUps--;

            if (gBtlWork->pendingLevelUps == 0) {
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateLevelUpClose);
            } else {
                SetTaskUpdate(task, (TaskUpdateFunc)UpdateLevelUpNextSlideOut);
            }

            m4aSongNumStart(SONG_SYS_KETTEI);
        }
    } else {
        work->timer++;
    }

    work->gfx2 = AnimUpdate(&work->anim);
    work->gfx = AnimUpdate(&work->anim2);
    return 1;
}

u8 UpdateLevelUpClose(LevelUpWork* work, void* task) {
    s32 bonusX;
    s32 textX;
    s32 bgScrollX;
    s32 statsOffsetX;
    s32 headerX;
    s8 slideSteps;

    bonusX = work->bonusX[work->cursor] << 8;
    textX = work->textX[work->cursor] << 8;
    bgScrollX = work->bgScrollX << 8;
    statsOffsetX = work->statsOffsetX << 8;
    headerX = work->headerX << 8;
    ApproachValue(&headerX, -0x8000, work->headerSteps);
    ApproachValue(&bonusX, -0x8000, work->optionSteps[work->cursor]);
    ApproachValue(&textX, -0xF800, work->optionSteps[work->cursor]);
    work->bonusX[work->cursor] = bonusX >> 8;
    work->textX[work->cursor] = textX >> 8;
    work->headerX = headerX >> 8;
    work->headerSteps--;
    work->optionSteps[work->cursor]--;
    ApproachValue(&bgScrollX, 0, work->slideSteps);
    ApproachValue(&statsOffsetX, 0x10000, work->slideSteps);
    ApproachValue(&work->playerX, 0x1BE00, work->slideSteps);
    ApproachValue(&work->playerY, 0x4800, work->slideSteps);

    if (gBtlWork->battleId == BATTLE_URSULA) {
        ScrollBgMapTo(0, bgScrollX >> 8, 0);
    } else {
        ScrollBgMapTo(1, bgScrollX >> 8, 0);
    }

    work->bgScrollX = bgScrollX >> 8;
    work->statsOffsetX = statsOffsetX >> 8;

    if (work->slideSteps > 0) {
        work->slideSteps--;
    }

    slideSteps = work->slideSteps;

    if (slideSteps == 0) {
        if (work->barSteps != 0) {
            ApproachValue(&work->topBarY, -0x800, work->barSteps);
            ApproachValue(&work->bottomBarY, 0xA000, work->barSteps);
            work->barSteps--;
        } else {
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateLevelUpWaitFade);
        }
    }

    work->gfx = AnimUpdate(&work->anim2);
    TaskPoolUpdate(&work->pool);
    return 1;
}

u8 UpdateLevelUpWaitFade() {
    if (!FadeIsActive()) {
        return 0;
    }

    return 1;
}

void Level_Up_2(LevelUpWork* work) {
    u8 i = 0;

#ifdef PLATFORM_PC
    PortScreenBegin(); /* menu: drawn on the screen layer, at the window's height */
#endif

    if (work->loaded[0] != 0) {
#ifdef PLATFORM_PC
        /* the header's HD banner: the sprite stays over the BG copy the choice screens carry */
        if ((work->state == LEVEL_UP_STATE_SELECT || work->state == LEVEL_UP_STATE_RESULT) &&
            work->tilesPalettes[6] != NULL &&
            PortHdSpriteFind(((ObjTiles*)work->tilesPalettes[6])->src,
#ifdef VERSION_EU
                             gLevelUpHeaderSpritesByLanguage[gLanguage][10]
#else
                             gLevelUpHeaderFrames[10]
#endif
                             ))
            DrawSprite(PORT_MENU_LEFT(work->headerX), 0,
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage][10],
#else
                       gLevelUpHeaderFrames[10],
#endif
                       work->tilesPalettes[6], work->tilesPalettes[7], NULL, 0, 50);
#endif
        switch (work->state) {
        case LEVEL_UP_STATE_SLIDE_IN:
            DrawSprite(PORT_MENU_LEFT(work->headerX), 0,
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage][10],
#else
                       gLevelUpHeaderFrames[10],
#endif
                       work->tilesPalettes[6], work->tilesPalettes[7], NULL, 0, 50);

            if (work->tilesPalettes[0] != NULL) {
#ifdef VERSION_EU
                if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                    DrawSprite(PORT_MENU_LEFT(work->bonusX[0]), work->bonusY[0], gLevelUpSoraBonusFrames[0], work->tilesPalettes[0], work->tilesPalettes[4], NULL, 0, 50);
#ifdef VERSION_EU
                } else {
                    DrawSprite(PORT_MENU_LEFT(work->bonusX[0]), work->bonusY[0], gLevelUpSoraBonusItalianFrames[0], work->tilesPalettes[0], work->tilesPalettes[4], NULL, 0, 50);
                }
#endif
            }

            if (work->tilesPalettes[1] != NULL) {
#ifdef VERSION_EU
                if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                    DrawSprite(PORT_MENU_LEFT(work->bonusX[1]), work->bonusY[1], gLevelUpSoraBonusFrames[1], work->tilesPalettes[1], work->tilesPalettes[4], NULL, 0, 50);
#ifdef VERSION_EU
                } else {
                    DrawSprite(PORT_MENU_LEFT(work->bonusX[1]), work->bonusY[1], gLevelUpSoraBonusItalianFrames[1], work->tilesPalettes[1], work->tilesPalettes[4], NULL, 0, 50);
                }
#endif
            }

            if (work->tilesPalettes[2] != NULL) {
#ifdef VERSION_EU
                if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                    DrawSprite(PORT_MENU_LEFT(work->bonusX[2]), work->bonusY[2], gLevelUpSoraBonusFrames[2], work->tilesPalettes[2], work->tilesPalettes[4], NULL, 0, 50);
#ifdef VERSION_EU
                } else {
                    DrawSprite(PORT_MENU_LEFT(work->bonusX[2]), work->bonusY[2], gLevelUpSoraBonusItalianFrames[2], work->tilesPalettes[2], work->tilesPalettes[4], NULL, 0, 50);
                }
#endif
            }

            PORT_BAND(DrawSprite(work->topBarX, work->topBarY >> 8, gLevelUpBarFrames[0], work->tiles2, work->tilesPalettes[7], NULL, SPRITE_PRIORITY(1), 51));
            PORT_BAND(DrawSprite(work->bottomBarX, work->bottomBarY >> 8, gLevelUpBarFrames[1], work->tiles2, work->tilesPalettes[7], NULL, SPRITE_PRIORITY(1), 51));
            break;
        case LEVEL_UP_STATE_SELECT:
            for (; i < ARRAY_COUNT(work->textX); i++) {
                if (i == work->cursor) {
#ifdef VERSION_EU
                    DrawSprite(PORT_MENU_LEFT(work->textX[i] + 5), work->textY[i] - 4, NULL, work->tiles5[i], work->palette, NULL, 0, 40);
#else
                    DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 22), work->textY[i] - 5, work->textSlots[i], work->palette, 40, work->textSlotCounts[i]);
                    DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 4), work->textY[i] + 13, work->textSlots[i + 3], work->palette, 40, work->textSlotCounts[i + 3]);
#endif
                } else {
#ifdef VERSION_EU
                    DrawSprite(PORT_MENU_LEFT(work->textX[i] + 3), work->textY[i] - 2, NULL, work->tiles5[i], work->palette2, NULL, 0, 40);
#else
                    DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 20), work->textY[i] - 3, work->textSlots[i], work->palette2, 40, work->textSlotCounts[i]);
                    DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 2), work->textY[i] + 15, work->textSlots[i + 3], work->palette2, 40, work->textSlotCounts[i + 3]);
#endif
                }
            }

            DrawSprite(PORT_MENU_LEFT(work->cursorX), work->cursorY, work->gfx2, work->tiles, work->palette3, NULL, 0, 40);
            break;
        case LEVEL_UP_STATE_RESULT:
            for (; i < ARRAY_COUNT(work->textX); i++) {
                if (i == work->cursor) {
#ifdef VERSION_EU
                    DrawSprite(PORT_MENU_LEFT(work->textX[i] + 4), work->textY[i] - 3, NULL, work->tiles5[i], work->palette, NULL, 0, 40);
#else
                    DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 22), work->textY[i] - 5, work->textSlots[i], work->palette, 40, work->textSlotCounts[i]);
                    DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 4), work->textY[i] + 13, work->textSlots[i + 3], work->palette, 40, work->textSlotCounts[i + 3]);
#endif
                    DrawSprite(PORT_MENU_LEFT(work->bonusX[i]), work->bonusY[i], gLevelUpSoraBonusChosenFrames[i], work->tilesPalettes[i], work->tilesPalettes[5], NULL, 0, 50);
                } else {
                    if (work->tilesPalettes[i] != NULL) {
#ifdef VERSION_EU
                        DrawSprite(PORT_MENU_LEFT(work->textX[i] + 2), work->textY[i] - 1, NULL, work->tiles5[i], work->palette2, NULL, 0, 40);
#else
                        DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 20), work->textY[i] - 3, work->textSlots[i], work->palette2, 40, work->textSlotCounts[i]);
                        DrawTextSlots(PORT_MENU_LEFT(work->textX[i] + 2), work->textY[i] + 15, work->textSlots[i + 3], work->palette2, 40, work->textSlotCounts[i + 3]);
#endif
#ifdef VERSION_EU
                        if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                            DrawSprite(PORT_MENU_LEFT(work->bonusX[i]), work->bonusY[i], gLevelUpSoraBonusFrames[i], work->tilesPalettes[i], work->tilesPalettes[4], NULL, 0, 50);
#ifdef VERSION_EU
                        } else {
                            DrawSprite(PORT_MENU_LEFT(work->bonusX[i]), work->bonusY[i], gLevelUpSoraBonusItalianFrames[i], work->tilesPalettes[i], work->tilesPalettes[4], NULL, 0, 50);
                        }
#endif
                    }
                }
            }

            DrawSprite(PORT_MENU_LEFT(work->cursorX), work->cursorY, work->gfx2, work->tiles, work->palette3, NULL, 0, 40);
            break;
        case LEVEL_UP_STATE_SLIDE_OUT:
#ifdef VERSION_EU
            DrawSprite(PORT_MENU_LEFT(work->textX[work->cursor] + 2), work->textY[work->cursor] - 1, NULL, work->tiles5[work->cursor], work->palette, NULL, 0, 40);
#else
            DrawTextSlots(PORT_MENU_LEFT(work->textX[work->cursor] + 22), work->textY[work->cursor] - 5, work->textSlots[work->cursor], work->palette, 40, work->textSlotCounts[work->cursor]);
            DrawTextSlots(PORT_MENU_LEFT(work->textX[work->cursor] + 4), work->textY[work->cursor] + 13, work->textSlots[work->cursor + 3], work->palette, 40, work->textSlotCounts[work->cursor + 3]);
#endif
            DrawSprite(PORT_MENU_LEFT(work->bonusX[work->cursor]), work->bonusY[work->cursor], NULL, work->tilesPalettes[work->cursor], work->tilesPalettes[5], NULL, 0, 50);
            DrawSprite(PORT_MENU_LEFT(work->headerX), 0,
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage][10],
#else
                       gLevelUpHeaderFrames[10],
#endif
                       work->tilesPalettes[6], work->tilesPalettes[7], NULL, 0, 50);
            PORT_BAND(DrawSprite(work->topBarX, work->topBarY >> 8, gLevelUpBarFrames[0], work->tiles2, work->tilesPalettes[7], NULL, SPRITE_PRIORITY(1), 51));
            PORT_BAND(DrawSprite(work->bottomBarX, work->bottomBarY >> 8, gLevelUpBarFrames[1], work->tiles2, work->tilesPalettes[7], NULL, SPRITE_PRIORITY(1), 51));
            break;
        }

#ifdef VERSION_JP
        if (work->blinkOn != 0) {
            DrawSprite(PORT_MENU_RIGHT(192), 82, gLvupLogoFrames[0], work->tiles3, work->palette4, NULL, 0, 10);
        }
#endif

        if (!(gGameState.flags & GAME_FLAG_RIKU)) {
            DrawLevelUpStatDigits(PORT_MENU_RIGHT(work->statsOffsetX + 214), 17, work->tilesPalettes[6], work->tilesPalettes[7],
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage],
#else
                       gLevelUpHeaderFrames,
#endif
                       work->levelDigits, LEVEL_UP_STAT_LEVEL);
            DrawLevelUpStatDigits(PORT_MENU_RIGHT(work->statsOffsetX + 214), 111, work->tilesPalettes[6], work->tilesPalettes[7],
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage],
#else
                       gLevelUpHeaderFrames,
#endif
                       work->maxHpDigits, LEVEL_UP_STAT_MAX_HP);
            DrawLevelUpStatDigits(PORT_MENU_RIGHT(work->statsOffsetX + 206), 132, work->tilesPalettes[6], work->tilesPalettes[7],
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage],
#else
                       gLevelUpHeaderFrames,
#endif
                       work->cpDigits, LEVEL_UP_STAT_CP);
        } else {
            DrawLevelUpStatDigits(PORT_MENU_RIGHT(work->statsOffsetX + 214), 17, work->tilesPalettes[6], work->tilesPalettes[7],
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage],
#else
                       gLevelUpHeaderFrames,
#endif
                       work->levelDigits, LEVEL_UP_STAT_LEVEL);
            DrawLevelUpStatDigits(PORT_MENU_RIGHT(work->statsOffsetX + 214), 106, work->tilesPalettes[6], work->tilesPalettes[7],
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage],
#else
                       gLevelUpHeaderFrames,
#endif
                       work->maxHpDigits, LEVEL_UP_STAT_MAX_HP);
            DrawLevelUpStatDigits(PORT_MENU_RIGHT(work->statsOffsetX + 214), 122, work->tilesPalettes[6], work->tilesPalettes[7],
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage],
#else
                       gLevelUpHeaderFrames,
#endif
                       work->apDigits, LEVEL_UP_STAT_AP);
            DrawLevelUpStatDigits(PORT_MENU_RIGHT(work->statsOffsetX + 214), 137, work->tilesPalettes[6], work->tilesPalettes[7],
#ifdef VERSION_EU
                       gLevelUpHeaderSpritesByLanguage[gLanguage],
#else
                       gLevelUpHeaderFrames,
#endif
                       work->dpDigits, LEVEL_UP_STAT_DP);
        }
    }

    DrawSprite(PORT_MENU_RIGHT(work->playerX >> 8), work->playerY >> 8, work->gfx, work->tiles4, work->palette5, NULL, SPRITE_PRIORITY(1), 40);
    TaskPoolDraw(&work->pool);
#ifdef PLATFORM_PC
    PortScreenEnd();
#endif
}

void Level_Up_3(LevelUpWork* work) {
#ifdef PLATFORM_PC
    PortBgSplit(0, 0);
    PortBgSplit(1, 0);
    PortBgSplitBands(0, 0);
    PortBgSplitBands(1, 0);
#endif
#ifdef VERSION_EU
    if (work->tiles5[0] != NULL) {
        ReleaseObjTiles(work->tiles5[0]);
    }

    if (work->tiles5[1] != NULL) {
        ReleaseObjTiles(work->tiles5[1]);
    }

    if (work->tiles5[2] != NULL) {
        ReleaseObjTiles(work->tiles5[2]);
    }
#else
    FreeTextSlots(work->textSlots[0], ARRAY_COUNT(work->textSlots[0]));
    FreeTextSlots(work->textSlots[1], ARRAY_COUNT(work->textSlots[0]));
    FreeTextSlots(work->textSlots[2], ARRAY_COUNT(work->textSlots[0]));
    FreeTextSlots(work->textSlots[3], ARRAY_COUNT(work->textSlots[0]));
    FreeTextSlots(work->textSlots[4], ARRAY_COUNT(work->textSlots[0]));
    FreeTextSlots(work->textSlots[5], ARRAY_COUNT(work->textSlots[0]));
#endif

    if (work->tilesPalettes[6] != NULL) {
        ReleaseObjTiles(work->tilesPalettes[6]);
    }

    if (work->tilesPalettes[7] != NULL) {
        ReleaseObjPalette(work->tilesPalettes[7]);
    }

    if (work->tilesPalettes[0] != NULL) {
        ReleaseObjTiles(work->tilesPalettes[0]);
    }

    if (work->tilesPalettes[1] != NULL) {
        ReleaseObjTiles(work->tilesPalettes[1]);
    }

    if (work->tilesPalettes[2] != NULL) {
        ReleaseObjTiles(work->tilesPalettes[2]);
    }

    if (work->tilesPalettes[4] != NULL) {
        ReleaseObjPalette(work->tilesPalettes[4]);
    }

    if (work->tilesPalettes[5] != NULL) {
        ReleaseObjPalette(work->tilesPalettes[5]);
    }

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
    }

    if (work->palette2 != NULL) {
        ReleaseObjPalette(work->palette2);
    }

    if (work->tiles != NULL) {
        ReleaseObjTiles(work->tiles);
    }

    if (work->palette3 != NULL) {
        ReleaseObjPalette(work->palette3);
    }

    if (work->tiles2 != NULL) {
        ReleaseObjTiles(work->tiles2);
    }

    if (work->tiles3 != NULL) {
        ReleaseObjTiles(work->tiles3);
    }

    if (work->palette4 != NULL) {
        ReleaseObjPalette(work->palette4);
    }

    if (work->tiles4 != NULL) {
        ReleaseObjTiles(work->tiles4);
    }

    if (work->palette5 != NULL) {
        ReleaseObjPalette(work->palette5);
    }

    TaskPoolDestroy(&work->pool);
}

void DrawLevelUpStatDigits(s16 x, s16 y, void* tiles, void* pal, void** gfx, u16* digits, u8 stat) {
    switch (stat) {
    case LEVEL_UP_STAT_LEVEL:
        DrawSprite(x + 8, y, gfx[digits[1]], tiles, pal, NULL, 0, 0);
        x += 16;
        DrawSprite(x, y, gfx[digits[2]], tiles, pal, NULL, 0, 0);
        break;
    case LEVEL_UP_STAT_MAX_HP:
        DrawSprite(x, y, gfx[digits[0]], tiles, pal, NULL, 0, 0);
        DrawSprite(x + 8, y, gfx[digits[1]], tiles, pal, NULL, 0, 0);
        x += 16;
        DrawSprite(x, y, gfx[digits[2]], tiles, pal, NULL, 0, 0);
        break;
    case LEVEL_UP_STAT_CP:
        DrawSprite(x, y, gfx[digits[0]], tiles, pal, NULL, 0, 0);
        DrawSprite(x + 8, y, gfx[digits[1]], tiles, pal, NULL, 0, 0);
        DrawSprite(x + 16, y, gfx[digits[2]], tiles, pal, NULL, 0, 0);
        x += 24;
        DrawSprite(x, y, gfx[digits[3]], tiles, pal, NULL, 0, 0);
        break;
    case LEVEL_UP_STAT_AP:
        DrawSprite(x + 8, y, gfx[digits[1]], tiles, pal, NULL, 0, 0);
        x += 16;
        DrawSprite(x, y, gfx[digits[2]], tiles, pal, NULL, 0, 0);
        break;
    case LEVEL_UP_STAT_DP:
        DrawSprite(x, y, gfx[digits[0]], tiles, pal, NULL, 0, 0);
        DrawSprite(x + 8, y, gfx[digits[1]], tiles, pal, NULL, 0, 0);
        x += 16;
        DrawSprite(x, y, gfx[digits[2]], tiles, pal, NULL, 0, 0);
        break;
    }
}

void LevelUpSplitDigits2(u16 value, u16* digits) {
    u16 tens;
    u16 ones;

    tens = value / 10;
    ones = value - tens * 10;
    digits[1] = tens;
    digits[2] = ones;
}

void LevelUpSplitDigits3(u16 value, u16* digits) {
    u16 hundreds;
    u16 tens;
    u16 ones;

    hundreds = value / 100;
    tens = value / 10 - hundreds * 10;
    ones = value - hundreds * 100 - tens * 10;
    digits[0] = hundreds;
    digits[1] = tens;
    digits[2] = ones;
}

void LevelUpSplitDigits4(u16 value, u16* out) {
    u16 thousands;
    u16 hundreds;
    u16 tens;
    u16 ones;

    thousands = value / 1000;
    hundreds = value / 100 - thousands * 10;
    tens = value / 10 - hundreds * 10 - thousands * 100;
    ones = value - thousands * 1000 - hundreds * 100 - tens * 10;
    out[0] = thousands;
    out[1] = hundreds;
    out[2] = tens;
    out[3] = ones;
}

u8 UpdateLevelUpNextSlideIn(LevelUpWork* work, void* task) {
#ifdef VERSION_EU
    enum { tileSize = 0xC80, mapSize = 0x500 };
#else
    enum { tileSize = 0xD80, mapSize = 0x800 };
#endif

    if (work->loaded[0] == 0) {
        work->timer++;

        if (work->timer > 7) {
#ifdef VERSION_EU
            work->tiles5[0] = AllocSpriteFrameTiles(0x500);
            work->tiles5[1] = AllocSpriteFrameTiles(0x500);
            work->tiles5[2] = AllocSpriteFrameTiles(0x500);
#endif

            if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                work->tilesPalettes[0] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[1] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[2] = AllocSpriteFrameTiles(tileSize);

#ifdef VERSION_EU
                if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpSoraBonusFrames[0], gLevelUpSoraBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpSoraBonusFrames[1], gLevelUpSoraBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpSoraBonusFrames[2], gLevelUpSoraBonusTiles);
#ifdef VERSION_EU
                } else {
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpSoraBonusItalianFrames[0], gLevelUpSoraBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpSoraBonusItalianFrames[1], gLevelUpSoraBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpSoraBonusItalianFrames[2], gLevelUpSoraBonusItalianTiles);
                }
#endif

                work->tilesPalettes[4] = LoadObjPalette(gLevelUpSoraBonusPalette, sizeof(gLevelUpSoraBonusPalette));
                work->tilesPalettes[5] = LoadObjPalette(gLevelUpSoraBonusSelectedPalette, sizeof(gLevelUpSoraBonusSelectedPalette));
            } else {
                work->tilesPalettes[0] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[1] = AllocSpriteFrameTiles(tileSize);
                work->tilesPalettes[2] = AllocSpriteFrameTiles(tileSize);

#ifdef VERSION_EU
                if (gLanguage != LANGUAGE_ITALIAN) {
#endif
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpRikuBonusFrames[0], gLevelUpRikuBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpRikuBonusFrames[1], gLevelUpRikuBonusTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpRikuBonusFrames[2], gLevelUpRikuBonusTiles);
#ifdef VERSION_EU
                } else {
                    UpdateSpriteFrameTiles(work->tilesPalettes[0], gLevelUpRikuBonusItalianFrames[0], gLevelUpRikuBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[1], gLevelUpRikuBonusItalianFrames[1], gLevelUpRikuBonusItalianTiles);
                    UpdateSpriteFrameTiles(work->tilesPalettes[2], gLevelUpRikuBonusItalianFrames[2], gLevelUpRikuBonusItalianTiles);
                }
#endif

                work->tilesPalettes[4] = LoadObjPalette(gLevelUpRikuBonusPalette, sizeof(gLevelUpRikuBonusPalette));
                work->tilesPalettes[5] = LoadObjPalette(gLevelUpRikuBonusSelectedPalette, sizeof(gLevelUpRikuBonusSelectedPalette));
            }

#ifdef VERSION_EU
            switch (gLanguage) {
            case LANGUAGE_ENGLISH:
                work->tiles3 = LoadObjTiles(gLvupLogoTiles, sizeof(gLvupLogoTiles));
                break;
            case LANGUAGE_FRENCH:
                work->tiles3 = LoadObjTiles(gLvupLogoFrenchTiles, sizeof(gLvupLogoFrenchTiles));
                break;
            case LANGUAGE_GERMAN:
                work->tiles3 = LoadObjTiles(gLvupLogoGermanTiles, sizeof(gLvupLogoGermanTiles));
                break;
            case LANGUAGE_ITALIAN:
                work->tiles3 = LoadObjTiles(gLvupLogoItalianTiles, sizeof(gLvupLogoItalianTiles));
                break;
            case LANGUAGE_SPANISH:
                work->tiles3 = LoadObjTiles(gLvupLogoSpanishTiles, sizeof(gLvupLogoSpanishTiles));
                break;
            default:
                work->tiles3 = LoadObjTiles(gLvupLogoTiles, sizeof(gLvupLogoTiles));
                break;
            }
#else
            work->tiles3 = LoadObjTiles(gLvupLogoTiles, sizeof(gLvupLogoTiles));
#endif
            work->palette4 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
            FadeSetPaletteExcluded(((ObjPalette*)work->tilesPalettes[4])->index + 16, TRUE);
            FadeSetPaletteExcluded(((ObjPalette*)work->tilesPalettes[5])->index + 16, TRUE);
            FadeSetPaletteExcluded(work->palette4->index + 16, TRUE);
            work->tiles = AllocObjTiles(0x3C0, NULL);
            work->palette3 = LoadObjPalette(gSmallHandCursorPalette, sizeof(gSmallHandCursorPalette));
            FadeSetPaletteExcluded(work->palette3->index + 16, TRUE);
            SetObjTileSource(work->tiles, gSmallHandCursorTiles);
            AnimInit(&work->anim, gSmallHandCursorAnims, gSmallHandCursorFrames);
            AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
            work->gfx2 = AnimGetGfx(&work->anim);
            work->loaded[0] = 1;
            work->timer = 0;
            FadeToAmount(FADE_MODE_BLACK, 8, 16);
        }

        return 1;
    }

    work->gfx = AnimUpdate(&work->anim2);

    {
        s32 bonusX0 = work->bonusX[0] << 8;
        s32 bonusX1 = work->bonusX[1] << 8;
        s32 bonusX2 = work->bonusX[2] << 8;
        ApproachValue(&bonusX0, 0x1000, work->optionSteps[0]);

        if (work->optionSteps[0] > 0) {
            work->optionSteps[0]--;
        }

        if (work->optionSteps[0] <= 6) {
            ApproachValue(&bonusX1, 0x1000, work->optionSteps[1]);

            if (work->optionSteps[1] > 0) {
                work->optionSteps[1]--;
            }
        }

        if (work->optionSteps[1] <= 6) {
            ApproachValue(&bonusX2, 0x1000, work->optionSteps[2]);

            if (work->optionSteps[2] > 0) {
                work->optionSteps[2]--;
            }
        }

        work->bonusX[0] = bonusX0 >> 8;
        work->bonusX[1] = bonusX1 >> 8;
        work->bonusX[2] = bonusX2 >> 8;
    }

    if (work->optionSteps[2] == 0) {
        u8 i;

        if (gBtlWork->battleId == BATTLE_URSULA) {
            LoadBgMap(0, gLevelUpBonusAMap, mapSize);
        } else {
            LoadBgMap(1, gLevelUpBonusAMap, mapSize);
        }

        work->state = LEVEL_UP_STATE_SELECT;

        if (!(gGameState.flags & GAME_FLAG_RIKU)) {
            if (gGameState.progression.maxHp > 559) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[0] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[0]);
                work->textSlotCounts[3] = 0;
#endif
                work->optionEnabled[0] = 0;
            } else {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][0], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[0] = LoadTextSlots(gLevelUpSoraTexts[0], work->textSlots[0]);
                work->textSlotCounts[3] = LoadTextSlots(gLevelUpSoraTexts[3], work->textSlots[3]);
#endif
            }

            if (gGameState.progression.cp > 1899) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[1]);
                work->textSlotCounts[4] = 0;
#endif
                work->optionEnabled[1] = 0;
            } else {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][1], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[1], work->textSlots[1]);
                work->textSlotCounts[4] = LoadTextSlots(gLevelUpSoraTexts[4], work->textSlots[4]);
#endif
            }

            if (gGameState.progression.levelMilestone > 10) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[2]);
                work->textSlotCounts[5] = 0;
#endif
                work->optionEnabled[2] = 0;
            } else if (!IsLevelUpStockUnlocked()) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[2]);
                work->textSlotCounts[5] = 0;
#endif
                work->optionEnabled[2] = 0;
            } else {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][2], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[2], work->textSlots[2]);
                work->textSlotCounts[5] = LoadTextSlots(gLevelUpSoraTexts[5], work->textSlots[5]);
#endif
            }

            work->palette = LoadObjPalette(gLevelUpSoraOptionSelectedPalette, sizeof(gLevelUpSoraOptionSelectedPalette));
            work->palette2 = LoadObjPalette(gLevelUpSoraOptionPalette, sizeof(gLevelUpSoraOptionPalette));
        } else {
            if (gGameState.progression.maxHp > 559) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[0] = LoadTextSlots(gLevelUpRikuTexts[6], work->textSlots[0]);
                work->textSlotCounts[3] = 0;
#endif
                work->optionEnabled[0] = 0;
            } else {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[0], gLevelUpOptionSpritesByLanguage[gLanguage][3], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[0] = LoadTextSlots(gLevelUpRikuTexts[0], work->textSlots[0]);
                work->textSlotCounts[3] = LoadTextSlots(gLevelUpRikuTexts[3], work->textSlots[3]);
#endif
            }

            if (gGameState.progression.ap > 29) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[1]);
                work->textSlotCounts[4] = 0;
#endif
                work->optionEnabled[1] = 0;
            } else if (!(u8)IsLevelUpApUnlocked()) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[1] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[1]);
                work->textSlotCounts[4] = 0;
#endif
                work->optionEnabled[1] = 0;
            } else {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[1], gLevelUpOptionSpritesByLanguage[gLanguage][4], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[1] = LoadTextSlots(gLevelUpRikuTexts[1], work->textSlots[1]);
                work->textSlotCounts[4] = LoadTextSlots(gLevelUpRikuTexts[4], work->textSlots[4]);
#endif
            }

            if (gGameState.progression.dp > 299) {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][6], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[2] = LoadTextSlots(gLevelUpSoraTexts[6], work->textSlots[2]);
                work->textSlotCounts[5] = 0;
#endif
                work->optionEnabled[2] = 0;
            } else {
#ifdef VERSION_EU
                UpdateSpriteFrameTiles(work->tiles5[2], gLevelUpOptionSpritesByLanguage[gLanguage][5], gLevelUpOptionTilesByLanguage[gLanguage]);
#else
                work->textSlotCounts[2] = LoadTextSlots(gLevelUpRikuTexts[2], work->textSlots[2]);
                work->textSlotCounts[5] = LoadTextSlots(gLevelUpRikuTexts[5], work->textSlots[5]);
#endif
            }

            work->palette = LoadObjPalette(gLevelUpRikuOptionSelectedPalette, sizeof(gLevelUpRikuOptionSelectedPalette));
            work->palette2 = LoadObjPalette(gLevelUpRikuOptionPalette, sizeof(gLevelUpRikuOptionPalette));
        }

        FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
        FadeSetPaletteExcluded(work->palette2->index + 16, TRUE);

        for (i = 0; i < ARRAY_COUNT(work->optionEnabled); i++) {
            if (work->optionEnabled[i] == 1) {
                break;
            }
        }

        work->cursor = i;
        work->cursorY = sLevelUpCursorY[work->cursor];
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateLevelUpSelect);

        if (gBtlWork->battleId == BATTLE_URSULA) {
            LoadBgMap(0, gLevelUpOptionBgMaps[work->cursor], 0x800);
        } else {
            LoadBgMap(1, gLevelUpOptionBgMaps[work->cursor], 0x800);
        }
    }

    TaskPoolUpdate(&work->pool);
    return 1;
}

u8 UpdateLevelUpNextSlideOut(LevelUpWork* work, void* task) {
    s32 bonusX;
    s32 textX;
    s8 slideSteps;

    bonusX = work->bonusX[work->cursor] << 8;
    textX = work->textX[work->cursor] << 8;
    ApproachValue(&bonusX, -0x8000, work->optionSteps[work->cursor]);
    ApproachValue(&textX, -0xF800, work->optionSteps[work->cursor]);
    work->bonusX[work->cursor] = bonusX >> 8;
    work->textX[work->cursor] = textX >> 8;
    work->optionSteps[work->cursor]--;

    if (work->slideSteps > 0) {
        work->slideSteps--;
    }

    slideSteps = work->slideSteps;

    if (slideSteps == 0) {
        if (work->barSteps != 0) {
            ApproachValue(&work->topBarY, 0, work->barSteps);
            ApproachValue(&work->bottomBarY, 0x9800, work->barSteps);
            work->barSteps--;
        } else {
            if (work->tilesPalettes[0] != NULL) {
                ReleaseObjTiles(work->tilesPalettes[0]);
            }

            if (work->tilesPalettes[1] != NULL) {
                ReleaseObjTiles(work->tilesPalettes[1]);
            }

            if (work->tilesPalettes[2] != NULL) {
                ReleaseObjTiles(work->tilesPalettes[2]);
            }

#ifdef VERSION_EU
            if (work->tiles5[0] != NULL) {
                ReleaseObjTiles(work->tiles5[0]);
            }

            if (work->tiles5[1] != NULL) {
                ReleaseObjTiles(work->tiles5[1]);
            }

            if (work->tiles5[2] != NULL) {
                ReleaseObjTiles(work->tiles5[2]);
            }
#endif

            if (work->tilesPalettes[4] != NULL) {
                ReleaseObjPalette(work->tilesPalettes[4]);
            }

            if (work->tilesPalettes[5] != NULL) {
                ReleaseObjPalette(work->tilesPalettes[5]);
            }

            if (work->palette != NULL) {
                ReleaseObjPalette(work->palette);
            }

            if (work->palette2 != NULL) {
                ReleaseObjPalette(work->palette2);
            }

            if (work->tiles != NULL) {
                ReleaseObjTiles(work->tiles);
            }

            if (work->palette3 != NULL) {
                ReleaseObjPalette(work->palette3);
            }

            if (work->tiles3 != NULL) {
                ReleaseObjTiles(work->tiles3);
            }

            if (work->palette4 != NULL) {
                ReleaseObjPalette(work->palette4);
            }

            work->bonusX[0] = 0xFF80;
            work->bonusX[1] = 0xFF80;
            work->bonusX[2] = 0xFF80;
            work->bonusY[0] = 16;
            work->bonusY[1] = 64;
            work->bonusY[2] = 112;
            work->slideSteps = 24;
            work->headerSteps = 16;
            work->optionSteps[0] = 16;
            work->optionSteps[1] = 16;
            work->optionSteps[2] = 16;
            work->textX[0] = 8;
            work->textX[1] = 8;
            work->textX[2] = 8;
            work->textY[0] = 31;
            work->textY[1] = 79;
            work->textY[2] = 127;
            work->cursorX = 132;
            work->cursorY = sLevelUpCursorY[0];

            if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                SetObjTileSource(work->tiles4, gSor1ll51Tiles);
                AnimInit(&work->anim2, gSor1ll51Anims, gSor1ll51Frames);
                AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
            } else {
                SetObjTileSource(work->tiles4, gRikuBt00Tiles);
                AnimInit(&work->anim2, gRikuBt00Anims, gRikuBt00Frames);
                AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
            }

            work->loaded[0] = 0;
            work->state = LEVEL_UP_STATE_SLIDE_IN;
            work->effectShown = 0;
            work->timer = 0;
            work->applied = FALSE;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateLevelUpNextSlideIn);
        }
    }

    work->optionEnabled[0] = 1;
    work->optionEnabled[1] = 1;
    work->optionEnabled[2] = 1;
    work->gfx = AnimUpdate(&work->anim2);
    TaskPoolUpdate(&work->pool);
    return 1;
}

s32 IsLevelUpApUnlocked() {
    if (gGameState.progression.level >= sLevelUpApLevels[gGameState.progression.levelMilestone]) {
        return TRUE;
    }

    return FALSE;
}

#ifdef VERSION_EU
void* gLevelUpBgTilesByLanguage[5] = { gLevelUpStatLabelTiles, gLevelUpStatLabelFrenchTiles, gLevelUpStatLabelGermanTiles, gLevelUpStatLabelItalianTiles, gLevelUpStatLabelSpanishTiles };
void* gLevelUpHeaderTilesByLanguage[5] = { gLevelUpHeaderTiles, gLevelUpHeaderFrenchTiles, gLevelUpHeaderGermanTiles, gLevelUpHeaderItalianTiles, gLevelUpHeaderSpanishTiles };

void** gLevelUpHeaderSpritesByLanguage[5] = {
    gLevelUpHeaderFrames,
    gLevelUpHeaderFrenchFrames,
    gLevelUpHeaderGermanFrames,
    gLevelUpHeaderItalianFrames,
    gLevelUpHeaderSpanishFrames,
};

void* gLevelUpOptionTilesByLanguage[5] = {
    gLevelUpOptionTiles,
    gLevelUpOptionFrenchTiles,
    gLevelUpOptionGermanTiles,
    gLevelUpOptionItalianTiles,
    gLevelUpOptionSpanishTiles,
};

void** gLevelUpOptionSpritesByLanguage[5] = {
    gLevelUpOptionFrames,
    gLevelUpOptionFrenchFrames,
    gLevelUpOptionGermanFrames,
    gLevelUpOptionItalianFrames,
    gLevelUpOptionSpanishFrames,
};
#endif

#ifndef VERSION_EU
u16* gLevelUpSoraTexts[7] = { gLevelUpHpBoostText, gLevelUpCpBoostText, gLevelUpSleightsText, gLevelUpRaiseSoraHpText, gLevelUpRaiseSoraCpText, gLevelUpLearnSleightText, (u16*)gLevelUpDisabledText };
u16* gLevelUpRikuTexts[7] = { gLevelUpHpBoostText, gLevelUpAttackBoostText, gLevelUpDarknessBoostText, gLevelUpRaiseRikuHpText, gLevelUpRaiseRikuApText, gLevelUpRaiseRikuDpText, (u16*)gLevelUpDisabledText };
#endif
const void* gLevelUpBgMapBlocks[2] = { gDefaultBgMap, gLevelUpStatsMap };

void* gLevelUpOptionBgMaps[3] = { gLevelUpBonusAMap, gLevelUpBonusBMap, gLevelUpBonusCMap };

TaskDesc gTaskDescLevelUp = {
    "Level_Up",
    (TaskInitFunc)Level_Up_0,
    (TaskUpdateFunc)Level_Up_1,
    (TaskDrawFunc)Level_Up_2,
    (TaskDestroyFunc)Level_Up_3,
    sizeof(LevelUpWork),
};
