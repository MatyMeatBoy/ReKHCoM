/**
 * mode_premire.c
 * Enemy Card Effect Names and Level-Up Test
 */

#include "registration_data.h"
#include "system_state.h"
#include "msg_api.h"
#include "card_battle.h"
#include "obj_api.h"
#include "display.h"
#include "engine_math.h"
#include "obj.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_number_plus.h"
#include "battle_backgrounds.h"
#include "sprites_btl_hud.h"
#include "sprites_card.h"
#include "battle_work.h"
#include "card_api.h"
#include "card_label_data.h"
#include "mode.h"
#include <stddef.h>
#include "types.h"
#include "mode_premire.h"
#include "sprite_palettes.h"
#include "gba/io_reg.h"

static TaskPool sModePremireTasks;
#ifndef VERSION_EU
static u8 sUnk_02034AF4[4];
#endif

void Mode_Premire_0() {
    func_08085FB0();
    InitSoraDecks();
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
    TaskPoolInit(&sModePremireTasks, 1);
    TaskCreate(&sModePremireTasks, &gTaskDescLevelUp, NULL);
}

void Mode_Premire_1() {
    TaskPoolUpdate(&sModePremireTasks);
    TaskPoolDraw(&sModePremireTasks);
}

void Mode_Premire_2() {
    TaskPoolDestroy(&sModePremireTasks);
}

enum HcEffectCountUnit {
    HC_EFFECT_COUNT_UNIT_ATTACKS,
    HC_EFFECT_COUNT_UNIT_BREAKS,
    HC_EFFECT_COUNT_UNIT_CARDS,
    HC_EFFECT_COUNT_UNIT_HITS,
    HC_EFFECT_COUNT_UNIT_USES,
    HC_EFFECT_COUNT_UNIT_SLEIGHTS,
    HC_EFFECT_COUNT_UNIT_RELOADS
};

u8 GetHcEffectCountUnit(HcEffectNameWork* work, u16 effect) {
    switch (effect) {
    case HC_EFFECT_NONE:
    case HC_EFFECT_INCREMENTOR:
    case HC_EFFECT_COMBO_PLUS:
    case HC_EFFECT_FIRE_BOOST:
    case HC_EFFECT_COMBO_FINISH:
    case HC_EFFECT_DRAW:
    case HC_EFFECT_CARDBLIND:
    case HC_EFFECT_QUICKLOAD:
    case HC_EFFECT_COMBO_PLUS_2:
    case HC_EFFECT_BLIZZARD_BOOST:
    case HC_EFFECT_THUNDER_BOOST:
    case HC_EFFECT_CURE_BOOST:
    case HC_EFFECT_PROTECT:
    case HC_EFFECT_RANDOM_VALUES:
    case HC_EFFECT_ALL_ZEROS:
    case HC_EFFECT_VANISH:
    case HC_EFFECT_LEAF_BRACER:
    case HC_EFFECT_DECREMENTOR:
    case HC_EFFECT_BIO:
    case HC_EFFECT_DRAW_2:
    case HC_EFFECT_ITEM_BRACER:
    case HC_EFFECT_RELOAD_KINESIS:
    case HC_EFFECT_RETROGRADE:
    case HC_EFFECT_DRAIN:
    case HC_EFFECT_BACK_ATTACK:
    case HC_EFFECT_RANDOM_FLUSH:
    case HC_EFFECT_MAGIC_BOOST:
    case HC_EFFECT_SUMMON_BOOST:
    case HC_EFFECT_AUTO_RELOAD:
    case HC_EFFECT_HYPER_HEALING:
    case HC_EFFECT_GUARD:
    case HC_EFFECT_FLOAT:
        return HC_EFFECT_COUNT_UNIT_RELOADS;
    case HC_EFFECT_DISPEL:
    case HC_EFFECT_MIMIC:
    case HC_EFFECT_DASH:
        return HC_EFFECT_COUNT_UNIT_CARDS;
    case HC_EFFECT_QUICK_RECOVERY:
    case HC_EFFECT_SHELL:
        return HC_EFFECT_COUNT_UNIT_HITS;
    case HC_EFFECT_SLEIGHT_LOCK:
    case HC_EFFECT_SLEIGHTBLIND:
    case HC_EFFECT_DOUBLE_SLEIGHT:
        return HC_EFFECT_COUNT_UNIT_SLEIGHTS;
    case HC_EFFECT_REGEN:
    case HC_EFFECT_SECOND_CHANCE:
    case HC_EFFECT_AUTO_LIFE:
        return HC_EFFECT_COUNT_UNIT_USES;
    case HC_EFFECT_VALUE_BREAK:
        return HC_EFFECT_COUNT_UNIT_BREAKS;
    }

    return HC_EFFECT_COUNT_UNIT_ATTACKS;
}

void HCEffectName_0(HcEffectNameWork* work, u8* arg) {
    void** tiles;

    work->side = arg[0];
    work->timer = 0;
    work->blinkInterval = 32;
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->tiles2 = AllocSpriteFrameTiles(0x3C0);
    work->tiles3 = AllocSpriteFrameTiles(32);
    work->randomIndex = 0;
    work->visible = 1;

    switch (work->side) {
    case CARD_SIDE_SORA:
        work->x = 48;
        work->effect = gCardBattleState->soraHcEffect;
#ifdef VERSION_EU
        work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->soraHcEffect);
#endif
        tiles = LANGSTR(gHcEffectDefs[gCardBattleState->soraHcEffect].sprites);
        UpdateSpriteFrameTiles(work->tiles2, tiles[gHcEffectDefs[gCardBattleState->soraHcEffect].spriteIndex], LANGSTR(gHcEffectDefs[gCardBattleState->soraHcEffect].tiles));
#ifdef VERSION_EU
        UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitSpritesByLanguage[gLanguage][work->countUnit], gHcEffectCountUnitTilesByLanguage[gLanguage]);
#else
        work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->soraHcEffect);
        UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitFrames[work->countUnit], gHcEffectCountUnitTiles);
#endif

        if (gCardBattleState->soraHcEffect == HC_EFFECT_NONE) {
            work->visible = 0;
        }

        break;
    case CARD_SIDE_RIKU:
        work->x = 162;
        work->effect = gCardBattleState->rikuHcEffect;
#ifdef VERSION_EU
        work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->rikuHcEffect);
#endif
        tiles = LANGSTR(gHcEffectDefs[gCardBattleState->rikuHcEffect].sprites);
        UpdateSpriteFrameTiles(work->tiles2, tiles[gHcEffectDefs[gCardBattleState->rikuHcEffect].spriteIndex], LANGSTR(gHcEffectDefs[gCardBattleState->rikuHcEffect].tiles));
#ifdef VERSION_EU
        UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitSpritesByLanguage[gLanguage][work->countUnit], gHcEffectCountUnitTilesByLanguage[gLanguage]);
#else
        work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->rikuHcEffect);
        UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitFrames[work->countUnit], gHcEffectCountUnitTiles);
#endif

        if (gCardBattleState->rikuHcEffect == HC_EFFECT_NONE) {
            work->visible = 0;
        }

        break;
    }

    work->tiles = LoadObjTiles(gBtlExpNextTiles, sizeof(gBtlExpNextTiles));
    work->countThousands = 0;
    work->countHundreds = 0;
    work->countTens = 0;
    work->countOnes = 0;
}

u8 HCEffectName_1(HcEffectNameWork* work, void* task) {
    u8 done;
    s32 div;
    CardBattleState* state;

    done = IsHcEffectNameShuffling(work);

    if (done) {
#ifdef VERSION_EU
        SplitFourDigits(gBtlWork->hcEffectCount, &work->countThousands);
#endif
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateHcEffectNameShuffle);
        return 1;
    }

    switch (work->side) {
    case CARD_SIDE_SORA:
        div = gHcEffectDefs[gBtlWork->hcEffect].count << 8;
#ifdef PLATFORM_PC
        /* NONE has count 0 during Dispel/replacement/teardown. Match the GBA
           zero quotient, then keep the existing task/state checks unchanged. */
        work->blinkInterval = (u32)DIV0((s16)gBtlWork->hcEffectCount << 16, div) >> 3;
#else
        work->blinkInterval = (u32)(((s16)gBtlWork->hcEffectCount << 16) / div) >> 3;
#endif
        state = gCardBattleState;

        if (state->soraHcEffect == HC_EFFECT_NONE) {
            state->soraHcEffectReplaced = FALSE;
            return 0;
        }

        if (state->soraHcEffect != work->effect) {
            state->soraHcEffectReplaced = FALSE;
            return 0;
        }

        if (state->soraHcEffectReplaced == TRUE) {
            state->soraHcEffectReplaced = FALSE;
            return 0;
        }

        if ((s16)gBtlWork->hcEffectCount <= 0) {
            state->soraHcEffect = HC_EFFECT_NONE;
            gBtlWork->hcEffect = HC_EFFECT_NONE;
            state->soraHcEffectReplaced = FALSE;
            return 0;
        }

        SplitFourDigits(gBtlWork->hcEffectCount, &work->countThousands);
        break;
    case CARD_SIDE_RIKU:
        div = gHcEffectDefs[gRikuBtlWork->hcEffect].count << 8;
#ifdef PLATFORM_PC
        work->blinkInterval = (u32)DIV0((s16)gRikuBtlWork->hcEffectCount << 16, div) >> 3;
#else
        work->blinkInterval = (u32)(((s16)gRikuBtlWork->hcEffectCount << 16) / div) >> 3;
#endif
        state = gCardBattleState;

        if (state->rikuHcEffect == HC_EFFECT_NONE) {
            state->rikuHcEffectReplaced = FALSE;
            return 0;
        }

        if (state->rikuHcEffect != work->effect) {
            state->rikuHcEffectReplaced = FALSE;
            return 0;
        }

        if (state->rikuHcEffectReplaced == TRUE) {
            state->rikuHcEffectReplaced = FALSE;
            return 0;
        }

        if ((s16)gRikuBtlWork->hcEffectCount <= 0) {
            state->rikuHcEffect = HC_EFFECT_NONE;
            gRikuBtlWork->hcEffect = HC_EFFECT_NONE;
            state->rikuHcEffectReplaced = FALSE;
            return 0;
        }

        SplitFourDigits(gRikuBtlWork->hcEffectCount, &work->countThousands);
        break;
    }

    work->timer++;

    if ((s16)work->blinkInterval <= 2) {
        work->blinkInterval = 2;
    }

    if ((s16)work->timer >= (s16)work->blinkInterval) {
        work->visible ^= 1;
        work->timer = 0;
    }

    return 1;
}

u8 IsHcEffectNameShuffling(HcEffectNameWork* work) {
    if (work->side != CARD_SIDE_SORA) {
        if (work->side != CARD_SIDE_RIKU) {
            return FALSE;
        }
    }

    if (work->effect != HC_EFFECT_RANDOM_FLUSH) {
        return FALSE;
    }

    return TRUE;
}

u8 UpdateHcEffectNameShuffle(HcEffectNameWork* work, void* task) {
    void** tiles;

    switch (work->side) {
    case CARD_SIDE_SORA:
        if (gCardBattleState->soraHcEffect != HC_EFFECT_RANDOM_FLUSH) {
            work->effect = gCardBattleState->soraHcEffect;

            if (gCardBattleState->soraHcEffect == HC_EFFECT_NONE) {
                gCardBattleState->soraHcEffectReplaced = FALSE;
                return 0;
            }

#ifdef VERSION_EU
            work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->soraHcEffect);
#endif
            tiles = LANGSTR(gHcEffectDefs[gCardBattleState->soraHcEffect].sprites);
            UpdateSpriteFrameTiles(work->tiles2, tiles[gHcEffectDefs[gCardBattleState->soraHcEffect].spriteIndex],
                         LANGSTR(gHcEffectDefs[gCardBattleState->soraHcEffect].tiles));
#ifdef VERSION_EU
            UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitSpritesByLanguage[gLanguage][work->countUnit],
                         gHcEffectCountUnitTilesByLanguage[gLanguage]);
#else
            work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->soraHcEffect);
            UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitFrames[work->countUnit], gHcEffectCountUnitTiles);
#endif
            SetTaskUpdate(task, (TaskUpdateFunc)HCEffectName_1);
        } else {
            u16 id = GetNextRandomHcEffect(&work->randomIndex);
            work->effect = id;
            tiles = LANGSTR(gHcEffectDefs[id].sprites);
            UpdateSpriteFrameTiles(work->tiles2, tiles[gHcEffectDefs[id].spriteIndex],
                         LANGSTR(gHcEffectDefs[id].tiles));
        }

        break;
    case CARD_SIDE_RIKU:
        if (gCardBattleState->rikuHcEffect != HC_EFFECT_RANDOM_FLUSH) {
            work->effect = gCardBattleState->rikuHcEffect;

            if (gCardBattleState->rikuHcEffect == HC_EFFECT_NONE) {
                gCardBattleState->soraHcEffectReplaced = FALSE;
                return 0;
            }

#ifdef VERSION_EU
            work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->rikuHcEffect);
#endif
            tiles = LANGSTR(gHcEffectDefs[gCardBattleState->rikuHcEffect].sprites);
            UpdateSpriteFrameTiles(work->tiles2, tiles[gHcEffectDefs[gCardBattleState->rikuHcEffect].spriteIndex],
                         LANGSTR(gHcEffectDefs[gCardBattleState->rikuHcEffect].tiles));
#ifdef VERSION_EU
            UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitSpritesByLanguage[gLanguage][work->countUnit],
                         gHcEffectCountUnitTilesByLanguage[gLanguage]);
#else
            work->countUnit = GetHcEffectCountUnit(work, gCardBattleState->rikuHcEffect);
            UpdateSpriteFrameTiles(work->tiles3, gHcEffectCountUnitFrames[work->countUnit], gHcEffectCountUnitTiles);
#endif
            SetTaskUpdate(task, (TaskUpdateFunc)HCEffectName_1);
        } else {
            u16 id = GetNextRandomHcEffect(&work->randomIndex);
            work->effect = id;
            tiles = LANGSTR(gHcEffectDefs[id].sprites);
            UpdateSpriteFrameTiles(work->tiles2, tiles[gHcEffectDefs[id].spriteIndex],
                         LANGSTR(gHcEffectDefs[id].tiles));
        }

        break;
    }

    return 1;
}

void HCEffectName_2(HcEffectNameWork* work) {
#ifdef VERSION_EU
    s32 pri;

    if (work->visible == 1) {
        pri = SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC;
        DrawSprite(work->x, 0x90, NULL, work->tiles2, work->palette, NULL, pri, 10);
        DrawSprite(work->x, 0x8A, gBtlExpNextFrames[15], work->tiles, work->palette, NULL, pri, 10);
        DrawSprite(work->x + 8, 0x8A, gBtlExpNextFrames[work->countTens + 4], work->tiles, work->palette, NULL, pri, 10);
        DrawSprite(work->x + 16, 0x8A, gBtlExpNextFrames[work->countOnes + 4], work->tiles, work->palette, NULL, pri, 10);
        DrawSprite(work->x + 24, 0x8A, NULL, work->tiles3, work->palette, NULL, pri, 10);
        DrawSprite(work->x + 32, 0x8A, gBtlExpNextFrames[14], work->tiles, work->palette, NULL, pri, 10);
    }
#else
    if (work->visible == 1) {
        DrawSprite(work->x, 0x90, NULL, work->tiles2, work->palette, NULL, SPRITE_PRIORITY(1), 10);
        DrawSprite(work->x, 0x8A, gBtlExpNextFrames[15], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 10);
        DrawSprite(work->x + 8, 0x8A, gBtlExpNextFrames[work->countTens + 4], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 10);
        DrawSprite(work->x + 16, 0x8A, gBtlExpNextFrames[work->countOnes + 4], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 10);
        DrawSprite(work->x + 24, 0x8A, NULL, work->tiles3, work->palette, NULL, SPRITE_PRIORITY(1), 10);
        DrawSprite(work->x + 32, 0x8A, gBtlExpNextFrames[14], work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 10);
    }
#endif
}

void HCEffectName_3(HcEffectNameWork* work) {
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjPalette(work->palette);
    gCardBattleState->unk_0D8 = 0;
    gCardBattleState->unk_0E5 = 0;
    gCardBattleState->unk_0C8 = 256;
}

void NumberPlus_0(NumberPlusWork* work, NumberPlusArgs* args) {
    work->args = *args;
    work->tiles = LoadObjTiles(gNumberPlusTiles, sizeof(gNumberPlusTiles));
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->x = work->args.x >> 8;
    work->y = (work->args.y >> 8) - 20;
    work->steps = 16;
    work->unk_29 = 0;
}

s32 NumberPlus_1(NumberPlusWork* work) {
    s32 y;

    y = work->y << 8;

    if (work->steps != 0) {
        ApproachValue(&y, work->args.y - 0x2800, work->steps);
        work->y = y >> 8;
        work->steps--;
        return 1;
    }

    return 0;
}

void NumberPlus_2(NumberPlusWork* work) {
    DrawSprite(work->x, work->y, gNumberPlusFrames[0], work->tiles, work->palette, NULL, SPRITE_FLAG_NO_MOSAIC, 0);
}

void NumberPlus_3(NumberPlusWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

Mode gModePremire = {
    "Mode_Premire",
    (ModeInitFunc)Mode_Premire_0,
    Mode_Premire_1,
    Mode_Premire_2,
};

#ifdef VERSION_EU
void* gHcEffectCountUnitTilesByLanguage[5] = { gHcEffectCountUnitTiles, gHcEffectCountUnitFrenchTiles, gHcEffectCountUnitGermanTiles, gHcEffectCountUnitItalianTiles, gHcEffectCountUnitSpanishTiles };
void** gHcEffectCountUnitSpritesByLanguage[5] = { gHcEffectCountUnitFrames, gHcEffectCountUnitFrenchFrames, gHcEffectCountUnitSpanishFrames, gHcEffectCountUnitItalianFrames, gHcEffectCountUnitGermanFrames };
#endif

TaskDesc gTaskDescHCEffectName = {
    "HCEffectName",
    (TaskInitFunc)HCEffectName_0,
    (TaskUpdateFunc)HCEffectName_1,
    (TaskDrawFunc)HCEffectName_2,
    (TaskDestroyFunc)HCEffectName_3,
    sizeof(HcEffectNameWork),
};

TaskDesc gTaskDescNumberPlus = {
    "NumberPlus",
    (TaskInitFunc)NumberPlus_0,
    (TaskUpdateFunc)NumberPlus_1,
    (TaskDrawFunc)NumberPlus_2,
    (TaskDestroyFunc)NumberPlus_3,
    sizeof(NumberPlusWork),
};
