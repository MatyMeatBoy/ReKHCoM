/**
 * card_worldsel_before.c
 * World Select Lead-In Effect
 */

#include "registration_data.h"
#include "m4a_song.h"
#include "fade.h"
#include "obj_api.h"
#include "engine_math.h"
#include "obj.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_card.h"
#include "songs.h"
#include "field_state.h"
#include <stddef.h>
#include "types.h"
#include "sprite_palettes.h"

void WorldSel_Before_0(WorldSelBeforeWork* work, WorldSelBeforeArgs* args) {
    u8 i;

    FadeToAmount(FADE_MODE_BLACK, 16, 8);
    work->pos = *args;
    work->tiles = LoadObjTiles(gWorldSelBeforeCardTiles, sizeof(gWorldSelBeforeCardTiles));
    work->palette = LoadObjPalette(gWorldSelBeforeCardPalette, sizeof(gWorldSelBeforeCardPalette));
    work->tiles2 = LoadObjTiles(gWorldSelBeforeRingTiles, sizeof(gWorldSelBeforeRingTiles));
    work->palette2 = AllocObjPalette(32);
    work->spriteCount = 6;
    work->animStep = 0;
    work->animTimer = 0;
    work->risenCount = 0;
    UpdateAllocatedObjPalette(work->palette2, &gWorldSelBeforeRingPalettes[gWorldSelAnims[work->animStep].palette << 4]);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 16, TRUE);

    for (i = 0; i < work->spriteCount; i++) {
        work->angle[i] = 0x80;
        work->cardZ[i] = 0;
        work->cardX[i] = gSineTable[work->angle[i]] * 24 + work->pos.x;
        work->cardY[i] = -gSineTable[work->angle[i] + 64] * 12 + work->pos.y;
    }

    m4aSongNumStart(SONG_SND_212);
}

s32 WorldSel_Before_1(WorldSelBeforeWork* work) {
    u8 i;

    if (work->risenCount < work->spriteCount) {
        work->cardZ[work->risenCount] -= (s32)(24.0f / (256.0f / (float)work->spriteCount * 0.25f) * 256.0f);

        if (work->cardZ[work->risenCount] <= -6144) {
            work->risenCount++;
        }
    }

    for (i = 0; i < work->risenCount; i++) {
        work->cardX[i] = gSineTable[work->angle[i]] * 24 + work->pos.x;
        work->cardY[i] = -gSineTable[work->angle[i] + 64] * 12 + work->pos.y;
        work->angle[i] += 4;
    }

    if (++work->animTimer == gWorldSelAnims[work->animStep].duration) {
        if (work->animStep < 29) {
            work->animStep++;
        } else {
            work->animStep = 0;
        }

        work->animTimer = 0;
        UpdateAllocatedObjPalette(work->palette2, &gWorldSelBeforeRingPalettes[gWorldSelAnims[work->animStep].palette << 4]);
    }

    return 1;
}

void WorldSel_Before_2(WorldSelBeforeWork* work) {
    u8 i;

    for (i = 0; i < work->spriteCount; i++) {
        DrawSprite((work->cardX[i] >> 8) - (gFieldState->x >> 8),
                   (work->cardY[i] >> 8) + ((work->pos.z + work->cardZ[i]) >> 8) - (gFieldState->y >> 8),
                   gWorldSelBeforeCardFrames[0], work->tiles, work->palette, NULL, SPRITE_PRIORITY(2),
                   -0x1004 - (work->cardY[i] >> 8) * 4);
    }

    DrawSprite((work->pos.x >> 8) - (gFieldState->x >> 8) - 32,
               (work->pos.y >> 8) + (work->pos.z >> 8) - (gFieldState->y >> 8) - 16,
               gWorldSelBeforeRingFrames[0], work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(2),
               -0x1004 - ((work->pos.y - 512) >> 8) * 4);
}

void WorldSel_Before_3(WorldSelBeforeWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
}

void func_080A581C(u8* work) {
    TaskCreate((TaskPool*)&work[0x10], &gTaskDescWorldSelBefore, work);
}

void CreateWorldSelBeforeTask(void* pool, s32 x, s32 y, s32 z) {
    WorldSelBeforeArgs args;

    args.x = x;
    args.y = y;
    args.z = z;
    TaskCreate(pool, &gTaskDescWorldSelBefore, &args);
}

WorldSelAnim gWorldSelAnims[30] = {
    { 0, 10, { 0, 0 } },
    { 1, 6, { 0, 0 } },
    { 2, 4, { 0, 0 } },
    { 3, 4, { 0, 0 } },
    { 4, 4, { 0, 0 } },
    { 5, 4, { 0, 0 } },
    { 6, 4, { 0, 0 } },
    { 7, 4, { 0, 0 } },
    { 8, 4, { 0, 0 } },
    { 9, 4, { 0, 0 } },
    { 10, 4, { 0, 0 } },
    { 11, 4, { 0, 0 } },
    { 12, 4, { 0, 0 } },
    { 13, 4, { 0, 0 } },
    { 14, 6, { 0, 0 } },
    { 15, 10, { 0, 0 } },
    { 14, 6, { 0, 0 } },
    { 13, 4, { 0, 0 } },
    { 12, 4, { 0, 0 } },
    { 11, 4, { 0, 0 } },
    { 10, 4, { 0, 0 } },
    { 9, 4, { 0, 0 } },
    { 8, 4, { 0, 0 } },
    { 7, 4, { 0, 0 } },
    { 6, 4, { 0, 0 } },
    { 5, 4, { 0, 0 } },
    { 4, 4, { 0, 0 } },
    { 3, 4, { 0, 0 } },
    { 2, 4, { 0, 0 } },
    { 1, 6, { 0, 0 } },
};

TaskDesc gTaskDescWorldSelBefore = {
    "WorldSel Before",
    (TaskInitFunc)WorldSel_Before_0,
    (TaskUpdateFunc)WorldSel_Before_1,
    (TaskDrawFunc)WorldSel_Before_2,
    (TaskDestroyFunc)WorldSel_Before_3,
    sizeof(WorldSelBeforeWork),
};
