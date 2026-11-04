/**
 * sroll_b_secn.c
 * Staff Roll Section Headers
 */

#include "sroll.h"
#include "sprites_staff_roll.h"
#include "fade.h"
#include <stdlib.h>
#include "anim.h"
#include "engine_math.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "gba/defines.h"

static s32 Square(s32 x) {
    return x * x;
}

void task_sroll_b_secn_0(SrollBSecnWork* work, SrollBSecnArg* arg) {
    u32 i;

    work->timer = 0;
    work->x = arg->x;
    work->y = arg->y;
    work->scrollY = arg->scrollY;
    work->scrollSpeed = arg->scrollSpeed;

    if (arg->index < 0) {
        work->tiles = LoadObjTiles(gSrollSecnThemeSongTiles, sizeof(gSrollSecnThemeSongTiles));
        work->palette = LoadObjPalette(gSrollSecnThemeSongPalette, sizeof(gSrollSecnThemeSongPalette));
        AnimInit(&work->anim, gSrollSecnThemeSongAnims, gSrollSecnThemeSongFrames);
        AnimStart(&work->anim, 0, 0);
        AnimInit(&work->anim2, gSrollSecnThemeSongAnims, gSrollSecnThemeSongFrames);
        AnimStart(&work->anim2, 0, 0);
    } else {
        work->tiles = LoadObjTiles(gSrollSecnSprites[arg->index].tiles, gSrollSecnSprites[arg->index].tileSize);
        work->palette = LoadObjPalette(gSrollSecnPalettes, sizeof(gSrollSecnPalettes));
        AnimInit(&work->anim, gSrollSecnSprites[arg->index].anims, gSrollSecnSprites[arg->index].gfxTable);
        AnimStart(&work->anim, 0, 0);
        AnimInit(&work->anim2, gSrollSecnSprites[arg->index].anims, gSrollSecnSprites[arg->index].gfxTable);
        AnimStart(&work->anim2, 1, 0);
    }

    for (i = 0; i < 8; i++) {
        FadeSetPaletteExcluded((BIOS_PTR(work->palette)->index + i) % 16 + 16, TRUE);
    }
}

u8 task_sroll_b_secn_1(SrollBSecnWork* work) {
    u8 alive;
    s16 y;

    alive = 1;
    y = (work->y >> 8) - (*work->scrollY >> 8);

    if (y <= -32) {
        alive = 0;
    }

    if (y <= DISPLAY_HEIGHT - 1) {
        ApproachValueHalfSteps(&work->x, 0x7800, 20);

        if (abs(work->x - 0x7800) <= 255) {
            work->x = 0x7800;
        }

        if (work->x == 0x7800) {
            AnimUpdate(&work->anim);
            AnimUpdate(&work->anim2);
        }
    }

    work->timer++;
    return alive;
}

void task_sroll_b_secn_2(SrollBSecnWork* work) {
    u16 y;

    y = (work->y >> 8) - (*work->scrollY >> 8);
    DrawSprite(120, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, 0, 0xEF0);
    DrawSprite(120, y, AnimGetGfx(&work->anim2), work->tiles, work->palette, NULL, 0, 0xEE0);
}

void task_sroll_b_secn_3(SrollBSecnWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

const SrollSecnSprite gSrollSecnSprites[] = {
    { gSrollSecnScenarioTiles, sizeof(gSrollSecnScenarioTiles), gSrollSecnScenarioAnims, gSrollSecnScenarioFrames },
    { gSrollSecn2DArtTiles, sizeof(gSrollSecn2DArtTiles), gSrollSecn2DArtAnims, gSrollSecn2DArtFrames },
    { gSrollSecn3DAnimationTiles, sizeof(gSrollSecn3DAnimationTiles), gSrollSecn3DAnimationAnims, gSrollSecn3DAnimationFrames },
    { gSrollSecnPlanningTiles, sizeof(gSrollSecnPlanningTiles), gSrollSecnPlanningAnims, gSrollSecnPlanningFrames },
    { gSrollSecnEventCreationTiles, sizeof(gSrollSecnEventCreationTiles), gSrollSecnEventCreationAnims, gSrollSecnEventCreationFrames },
    { gSrollSecnProgrammingTiles, sizeof(gSrollSecnProgrammingTiles), gSrollSecnProgrammingAnims, gSrollSecnProgrammingFrames },
    { gSrollSecnGraphicDesignTiles, sizeof(gSrollSecnGraphicDesignTiles), gSrollSecnGraphicDesignAnims, gSrollSecnGraphicDesignFrames },
    { gSrollSecnSoundTiles, sizeof(gSrollSecnSoundTiles), gSrollSecnSoundAnims, gSrollSecnSoundFrames },
    { gSrollSecnEndingThemeTiles, sizeof(gSrollSecnEndingThemeTiles), gSrollSecnEndingThemeAnims, gSrollSecnEndingThemeFrames },
    { gSrollSecnBuenaVistaGamesJapanTiles, sizeof(gSrollSecnBuenaVistaGamesJapanTiles), gSrollSecnBuenaVistaGamesJapanAnims, gSrollSecnBuenaVistaGamesJapanFrames },
    { gSrollSecnBuenaVistaGamesTiles, sizeof(gSrollSecnBuenaVistaGamesTiles), gSrollSecnBuenaVistaGamesAnims, gSrollSecnBuenaVistaGamesFrames },
#ifdef VERSION_EU
    { gSrollSecnBuenaVistaGamesEmeaTiles, sizeof(gSrollSecnBuenaVistaGamesEmeaTiles), gSrollSecnBuenaVistaGamesEmeaAnims, gSrollSecnBuenaVistaGamesEmeaFrames },
#endif
    { gSrollSecnVoiceTalentsTiles, sizeof(gSrollSecnVoiceTalentsTiles), gSrollSecnVoiceTalentsAnims, gSrollSecnVoiceTalentsFrames },
#ifdef VERSION_JP
    { gSrollSecnVoiceRecordingTiles, sizeof(gSrollSecnVoiceRecordingTiles), gSrollSecnVoiceRecordingAnims, gSrollSecnVoiceRecordingFrames },
#endif
    { gSrollSecnOutsideContractorsTiles, sizeof(gSrollSecnOutsideContractorsTiles), gSrollSecnOutsideContractorsAnims, gSrollSecnOutsideContractorsFrames },
    { gSrollSecnManagementTiles, sizeof(gSrollSecnManagementTiles), gSrollSecnManagementAnims, gSrollSecnManagementFrames },
    { gSrollSecnQualityAssuranceTiles, sizeof(gSrollSecnQualityAssuranceTiles), gSrollSecnQualityAssuranceAnims, gSrollSecnQualityAssuranceFrames },
#ifdef VERSION_JP
    { gSrollSecnRatingProofreadingTiles, sizeof(gSrollSecnRatingProofreadingTiles), gSrollSecnRatingProofreadingAnims, gSrollSecnRatingProofreadingFrames },
    { gSrollSecnInformationTechnologyTiles, sizeof(gSrollSecnInformationTechnologyTiles), gSrollSecnInformationTechnologyAnims, gSrollSecnInformationTechnologyFrames },
    { gSrollSecnPublicityTeamTiles, sizeof(gSrollSecnPublicityTeamTiles), gSrollSecnPublicityTeamAnims, gSrollSecnPublicityTeamFrames },
    { gSrollSecnSalesMarketingTiles, sizeof(gSrollSecnSalesMarketingTiles), gSrollSecnSalesMarketingAnims, gSrollSecnSalesMarketingFrames },
    { gSrollSecnSalesAdministrationTiles, sizeof(gSrollSecnSalesAdministrationTiles), gSrollSecnSalesAdministrationAnims, gSrollSecnSalesAdministrationFrames },
#endif
    { gSrollSecnLegalAffairsTiles, sizeof(gSrollSecnLegalAffairsTiles), gSrollSecnLegalAffairsAnims, gSrollSecnLegalAffairsFrames },
#ifndef VERSION_JP
    { gSrollSecnLocalizationTeamTiles, sizeof(gSrollSecnLocalizationTeamTiles), gSrollSecnLocalizationTeamAnims, gSrollSecnLocalizationTeamFrames },
    { gSrollSecnSquareEnixIncTiles, sizeof(gSrollSecnSquareEnixIncTiles), gSrollSecnSquareEnixIncAnims, gSrollSecnSquareEnixIncFrames },
#endif
    { gSrollSecnSpecialThanksTiles, sizeof(gSrollSecnSpecialThanksTiles), gSrollSecnSpecialThanksAnims, gSrollSecnSpecialThanksFrames },
    { gSrollSecnJupiterCorporationTiles, sizeof(gSrollSecnJupiterCorporationTiles), gSrollSecnJupiterCorporationAnims, gSrollSecnJupiterCorporationFrames },
};

TaskDesc gTaskDescSrollBSecn = {
    "task_sroll_b_secn",
    (TaskInitFunc)task_sroll_b_secn_0,
    (TaskUpdateFunc)task_sroll_b_secn_1,
    (TaskDrawFunc)task_sroll_b_secn_2,
    (TaskDestroyFunc)task_sroll_b_secn_3,
    sizeof(SrollBSecnWork),
};
