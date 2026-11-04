/**
 * card_reload_children.c
 * Card Reload, Boss Prize and Scrollbar
 */

#include "registration_data.h"
#include "m4a_song.h"
#include "game_state.h"
#include "monsgage.h"
#include "fade.h"
#include "btl_collision.h"
#include "obj_api.h"
#include "battle_actor.h"
#include "engine_math.h"
#include "listpool.h"
#include "anim.h"
#include "obj.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_btl.h"
#include "sprites_card_pictures.h"
#include "sprites_card.h"
#include "songs.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "types.h"
#include "engine.h"
#include "gba/macro.h"
#include <stddef.h>
#include "lockon.h"
#include "card_reload_children.h"
#include "sprite_palettes.h"
#include "card_label_data.h"
#include "macros.h"
#include "card_ids.h"

static const s16 sSoraReloadChildOffsetX[4] = { 16, 29, 42, 51 };

static const s16 sRikuReloadChildOffsetX[4] = { -16, -29, -42, -51 };

static const s16 sReloadChildOffsetY[4] = { 0, 0, 0, 24 };

void RELOAD_CHILDREN_0(ReloadChildWork* work, ReloadChildArgs* args) {
    work->args = *args;
    work->tiles = LoadObjTiles(gReloadChildTiles[work->args.listIndex], 128);
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles2 = NULL;

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        if ((s8)work->args.index <= 3) {
            work->offsetX = sSoraReloadChildOffsetX[(s8)work->args.index] << 8;
            work->offsetY = sReloadChildOffsetY[(s8)work->args.index] << 8;
        } else {
            work->offsetX = sSoraReloadChildOffsetX[3] << 8;
            work->offsetY = sReloadChildOffsetY[3] << 8;
        }

        break;
    case CARD_SIDE_RIKU:
        if ((s8)work->args.index <= 3) {
            work->offsetX = sRikuReloadChildOffsetX[(s8)work->args.index] << 8;
            work->offsetY = sReloadChildOffsetY[(s8)work->args.index] << 8;
        } else {
            work->offsetX = sRikuReloadChildOffsetX[3] << 8;
            work->offsetY = sReloadChildOffsetY[3] << 8;
        }

        break;
    }

    ListNodeInit(&work->node, work->args.pool, work);
    ListPoolAppend(&work->node, work->args.pool);
    work->retractTimer = 0;
}

u8 RELOAD_CHILDREN_1(ReloadChildWork* work, void* task) {
    u8 (*fn)(ReloadChildWork*, void*);

    if (work->args.flags & RELOAD_CHILD_FLAG_IDLE) {
        work->retractTimer++;

        if (work->retractTimer == 30) {
            work->steps = 8;
            fn = UpdateReloadChildRetracted;
            SetTaskUpdate(task, (TaskUpdateFunc)fn);
            return fn(work, task);
        }
    }

    if (work->args.flags & RELOAD_CHILD_FLAG_SHIFTED) {
        work->steps = 8;
        work->args.flags &= ~RELOAD_CHILD_FLAG_SHIFTED;
    }

    if (work->args.index < ARRAY_COUNT(sReloadChildOffsetY)) {
        switch (work->args.side) {
        case CARD_SIDE_SORA:
            ApproachValue(&work->offsetX, sSoraReloadChildOffsetX[(s8)work->args.index] << 8, work->steps);
            break;
        case CARD_SIDE_RIKU:
            ApproachValue(&work->offsetX, sRikuReloadChildOffsetX[(s8)work->args.index] << 8, work->steps);
            break;
        }

        ApproachValue(&work->offsetY, sReloadChildOffsetY[(s8)work->args.index] << 8, work->steps);
    } else if ((s8)work->args.index < 0) {
        ListPoolRemove(&work->node, work->args.pool);
        work->tiles2 = LoadObjTiles(gCardBacks[work->args.listIndex].tiles2, 0xD00);
        work->steps = 8;
        work->scale = Q_8_8(0.4);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateReloadChildAbsorb);
        return 1;
    }

    if (work->steps != 0) {
        work->steps--;
    }

    work->angle += 8;
    return 1;
}

u8 UpdateReloadChildRetracted(ReloadChildWork* work, void* task) {
    u8 (*fn)(ReloadChildWork*, void*);
    u16 idle;

    idle = work->args.flags & RELOAD_CHILD_FLAG_IDLE;

    if (idle == 0) {
        work->steps = 8;
        fn = RELOAD_CHILDREN_1;
        SetTaskUpdate(task, (TaskUpdateFunc)fn);
        work->retractTimer = 0;
        return fn(work, task);
    }

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        ApproachValue(&work->offsetX, sSoraReloadChildOffsetX[(s8)work->args.index] << 8, work->steps);
        break;
    case CARD_SIDE_RIKU:
        ApproachValue(&work->offsetX, sRikuReloadChildOffsetX[(s8)work->args.index] << 8, work->steps);
        break;
    }

    ApproachValue(&work->offsetY, sReloadChildOffsetY[3] << 8, work->steps);

    if (work->steps != 0) {
        work->steps--;
    }

    return 1;
}

s32 UpdateReloadChildAbsorb(ReloadChildWork* work) {
    ApproachValue(&work->offsetX, 0, work->steps);
    ApproachValue(&work->offsetY, 0, work->steps);
    ApproachValue(&work->scale, Q_8_8(1), work->steps);

    if (work->steps != 0) {
        work->steps--;
        return 1;
    }

    return 0;
}

#ifdef PLATFORM_PC
static void RELOAD_CHILDREN_2_Body(ReloadChildWork* work);

/* Reload cards: bottom-left corner, with the deck. */
void RELOAD_CHILDREN_2(ReloadChildWork* work) {
    PortHudBegin(-1, 1);
    RELOAD_CHILDREN_2_Body(work);
    PortHudEnd();
}

static void RELOAD_CHILDREN_2_Body(ReloadChildWork* work) {
#else
void RELOAD_CHILDREN_2(ReloadChildWork* work) {
#endif
    s16 x;
    s16 y;
    ObjAffine* affine;

    if (work->args.index <= 3) {
        x = (work->offsetX + *work->args.parentX) >> 8;
        y = (work->offsetY + *work->args.parentY) >> 8;
        DrawSprite(x, y + (gSineTable[work->angle] >> 8), gReloadChildRedFrames[0], work->tiles, work->palette, NULL, 0, 50);
    }

    if ((s8)work->args.index < 0) {
        x = (work->offsetX + *work->args.parentX) >> 8;
        y = (work->offsetY + *work->args.parentY) >> 8;
        affine = AllocObjAffine(0, work->scale, work->scale, FALSE);
        DrawSprite(x, y + (gSineTable[work->angle] >> 8), gCardBacks[work->args.listIndex].gfx2, work->tiles2, work->palette, affine, 0, 49);
    }
}

void RELOAD_CHILDREN_3(ReloadChildWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);

    if (work->tiles2 != NULL) {
        ReleaseObjTiles(work->tiles2);
    }
}

void REV_COUNT_0(RevCountWork* work, RevCountArgs* args) {
    s16* count;
    s16* count2;
    void** row;
    u8 idx;

    CpuFill32(0, work, sizeof(RevCountWork));
    work->args = *args;
    idx = work->args.list;
    work->list = idx;
    work->tiles = AllocSpriteFrameTiles(320);
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));

    if (work->list == CARD_LIST_MAIN) {
        count = work->args.count;

        if (*count >= 2 && *count <= 100) {
            row = gRevCountSprites[work->list];
            UpdateSpriteFrameTiles(work->tiles, row[*count - 2], gRevCountTileSources[work->list]);
        } else if (*count > 100) {
            row = gRevCountSprites[work->list];
            UpdateSpriteFrameTiles(work->tiles, row[98], gRevCountTileSources[work->list]);
        } else {
            row = gRevCountSprites[work->list];
            UpdateSpriteFrameTiles(work->tiles, row[0], gRevCountTileSources[work->list]);
        }
    } else {
        count2 = work->args.count;

        if (*count2 >= 1 && *count2 <= 99) {
            row = gRevCountSprites[work->list];
            UpdateSpriteFrameTiles(work->tiles, row[*count2 - 1], gRevCountTileSources[work->list]);
        } else {
            row = gRevCountSprites[work->list];
            UpdateSpriteFrameTiles(work->tiles, row[0], gRevCountTileSources[work->list]);
        }
    }

    work->gfx = AnimGetGfx(&work->anim);
    work->shownCount = *work->args.count;

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        work->x = -0x2000;
        work->y = 0x9800;
        break;
    case CARD_SIDE_RIKU:
        work->x = 0x11000;
        work->y = 0x9800;
        break;
    }

    work->steps = 8;
}

u8 REV_COUNT_1(RevCountWork* work, void* task) {
    s16* count;
    void** row;

    count = work->args.count;

    if (*count != (s16)work->shownCount) {
        if (work->list == CARD_LIST_MAIN) {
            if (*count >= 2 && *count <= 100) {
                row = gRevCountSprites[work->list];
                UpdateSpriteFrameTiles(work->tiles, row[*count - 2],
                              gRevCountTileSources[work->list]);
            } else {
                u8 (*fn)(RevCountWork*, void*);

                work->steps = 8;
                fn = UpdateRevCountEmpty;
                SetTaskUpdate(task, (TaskUpdateFunc)fn);
                work->shownCount = *work->args.count;
                return fn(work, task);
            }
        } else {
            if (*count >= 1 && *count <= 99) {
                row = gRevCountSprites[work->list];
                UpdateSpriteFrameTiles(work->tiles, row[*count - 1],
                              gRevCountTileSources[work->list]);
            } else {
                u8 (*fn)(RevCountWork*, void*);

                work->steps = 8;
                fn = UpdateRevCountEmpty;
                SetTaskUpdate(task, (TaskUpdateFunc)fn);
                work->shownCount = *work->args.count;
                return fn(work, task);
            }
        }

        work->shownCount = *work->args.count;
    } else if (*count <= 0) {
        u8 (*fn)(RevCountWork*, void*);

        work->steps = 8;
        fn = UpdateRevCountEmpty;
        SetTaskUpdate(task, (TaskUpdateFunc)fn);
        work->shownCount = *work->args.count;
        return fn(work, task);
    }

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        ApproachValue(&work->x, 0, work->steps);
        break;
    case CARD_SIDE_RIKU:
        ApproachValue(&work->x, 0xD800, work->steps);
        break;
    }

    if (work->steps != 0) {
        work->steps--;
    }

    if (work->args.list != *work->args.shownList) {
        u8 (*fn)(RevCountWork*, void*);

        fn = (u8 (*)(RevCountWork*, void*))UpdateRevCountListChanged;
        SetTaskUpdate(task, (TaskUpdateFunc)fn);
        work->steps = 8;
        return fn(work, task);
    }

    if (gBtlWork->phase == BTL_PHASE_END) {
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateRevCountHidden);
    }

    if (*work->args.visible == 0) {
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateRevCountHidden);
    }

    return 1;
}

u8 UpdateRevCountListChanged(RevCountWork* work) {
    switch (work->args.side) {
    case CARD_SIDE_SORA:
        ApproachValue(&work->x, -0x2000, work->steps);
        break;
    case CARD_SIDE_RIKU:
        ApproachValue(&work->x, 0x11000, work->steps);
        break;
    }

    if (work->steps != 0) {
        work->steps--;
    }

    if (work->args.list == *work->args.shownList && *work->args.count > 0) {
        return 0;
    }

    return 1;
}

u8 UpdateRevCountHidden(RevCountWork* work, void* task) {
    u8 (*fn)(RevCountWork*, void*);

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        ApproachValue(&work->x, -0x2000, work->steps);
        break;
    case CARD_SIDE_RIKU:
        ApproachValue(&work->x, 0x11000, work->steps);
        break;
    }

    if (work->steps == 0) {
        return 0;
    }

    work->steps--;

    if (*work->args.visible == 1) {
        work->steps = 8;
        fn = REV_COUNT_1;
        SetTaskUpdate(task, (TaskUpdateFunc)fn);
        return fn(work, task);
    }

    return 1;
}

u8 UpdateRevCountEmpty(RevCountWork* work, void* task) {
    switch (work->args.side) {
    case CARD_SIDE_SORA:
        ApproachValue(&work->x, -0x2000, work->steps);
        break;
    case CARD_SIDE_RIKU:
        ApproachValue(&work->x, 0x11000, work->steps);
        break;
    }

    if (work->steps != 0) {
        work->steps--;
    }

    if (work->list == CARD_LIST_MAIN) {
        if (*work->args.count > 1) {
            SetTaskUpdate(task, (TaskUpdateFunc)REV_COUNT_1);
            work->steps = 8;
            return REV_COUNT_1(work, task);
        }
    } else {
        if (*work->args.count > 0) {
            SetTaskUpdate(task, (TaskUpdateFunc)REV_COUNT_1);
            work->steps = 8;
            return REV_COUNT_1(work, task);
        }
    }

    return 1;
}

#ifdef VERSION_EU
#define REV_COUNT_SPRITE_FLAGS (SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC)
#else
#define REV_COUNT_SPRITE_FLAGS SPRITE_PRIORITY(1)
#endif

#ifdef PLATFORM_PC
static void REV_COUNT_2_Body(RevCountWork* work);

/* Deck counter: bottom-left corner, with the deck. */
void REV_COUNT_2(RevCountWork* work) {
    PortHudBegin(-1, 1);
    REV_COUNT_2_Body(work);
    PortHudEnd();
}

static void REV_COUNT_2_Body(RevCountWork* work) {
#else
void REV_COUNT_2(RevCountWork* work) {
#endif
    DrawSprite(work->x >> 8, work->y >> 8, NULL, work->tiles, work->palette, NULL, REV_COUNT_SPRITE_FLAGS, 15);
}

void REV_COUNT_3(RevCountWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void CreateREVCOUNTTask(void* pool, u8* shownList, s16* count, u8* visible, u8 side) {
    RevCountArgs args;

    visible[0] = 1;
    args.shownList = shownList;
    args.count = count;
    args.visible = visible;
    args.list = shownList[0];
    args.side = side;
    TaskCreate(pool, &gTaskDescREVCOUNT, &args);
}

void RELOAD_0(ReloadWork* work, ReloadArgs* args) {
    work->tiles = AllocObjTiles(0xA0, NULL);
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->args = *args;
    SetObjTileSource(work->tiles, gReloadTiles[work->args.listIndex]);
    AnimInit(&work->anim, gReloadAnims[work->args.listIndex], gReloadFrames[work->args.listIndex]);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        work->x = -0x3000;
        work->y = 0x7E00;
        break;
    case CARD_SIDE_RIKU:
        work->x = 0xB4800;
        work->y = 0x7E00;
        break;
    }

    work->steps = 6;
}

u8 RELOAD_1(ReloadWork* work, void* task) {
    work->gfx = AnimUpdate(&work->anim);

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        ApproachValue(&work->x, 0x1800, work->steps);
        break;
    case CARD_SIDE_RIKU:
        ApproachValue(&work->x, 0xD800, work->steps);
        break;
    }

    if (work->steps != 0) {
        work->steps--;
    }

    if (*work->args.state == 0) {
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateReloadSlideOut);
    }

    if (gBtlWork->phase == BTL_PHASE_END) {
        work->steps = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateReloadSlideOut);
    }

    return 1;
}

u8 UpdateReloadSlideOut(ReloadWork* work) {
    work->gfx = AnimUpdate(&work->anim);

    switch (work->args.side) {
    case CARD_SIDE_SORA:
        ApproachValue(&work->x, -0x3000, work->steps);
        break;
    case CARD_SIDE_RIKU:
        ApproachValue(&work->x, 0x12000, work->steps);
        break;
    }

    if (work->steps != 0) {
        work->steps--;
        return 1;
    }

    return 0;
}

#ifdef PLATFORM_PC
static void RELOAD_2_Body(ReloadWork* work);

/* Reload gauge: bottom-left corner, with the deck. */
void RELOAD_2(ReloadWork* work) {
    PortHudBegin(-1, 1);
    RELOAD_2_Body(work);
    PortHudEnd();
}

static void RELOAD_2_Body(ReloadWork* work) {
#else
void RELOAD_2(ReloadWork* work) {
#endif
    DrawSprite(work->x >> 8, work->y >> 8, work->gfx, work->tiles, work->palette, NULL, 0, 10);
}

void RELOAD_3(ReloadWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void PrizeBoss_0(BossPrizeWork* work, PrizeCardTaskArgs* args) {
    const CardDef* def;
    const CardBack* back;
    Collider* collider;

    work->cardId = args->cardId;
    def = &gCardDefs[args->cardId];
    work->tiles = LoadObjTiles(def->tiles, 0x300);
    work->palette = LoadObjPalette(def->palette, 32);
    work->stat = *(CardStat*)&def->kind;
    back = &gCardBacks[def->category];
    work->tiles2 = LoadObjTiles(back->tiles, 0x280);
    work->tiles3 = LoadObjTiles(back->tiles3, 0x600);
    work->palette2 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles4 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    work->tiles5 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette3 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->posX = args->x;
    work->posY = args->y;
    work->posZ = args->z;
    work->groundZ = 0;
    work->rotation = 24;
    work->vz = -(GetRandom() % 129 + 0x300);
    work->speed = GetRandom() % 129 + 0x80;
    work->moveAngle = GetRandom() % 256;
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->scale = Q_8_8(0.5);
    work->flipAngleY = 0;
    work->flipAngleX = 0;
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_PICKUP, 8, 10);
    ColliderSetDisabled(collider, TRUE);
    ColliderSetPosition(collider, work->posX, work->posY, work->posZ);
    work->timer = 0;
    work->collected = FALSE;
    work->steps = 0;
    work->holdTimer = 0;
    work->effectCount = 0;
    work->effectTimer = 0;
    m4aSongNumStart(SONG_EF_BOSS_DEAD4);
    TaskPoolInit(&work->tasks, 10);
    gBtlWork->prizeCount++;
}

#ifdef PLATFORM_PC
extern u8 gPortHarnessTakePrize; /* main.c, harness mode takeprize */
#endif

u8 PrizeBoss_1(BossPrizeWork* work, void* task) {
    s16 x;
    s16 y;

#ifdef PLATFORM_PC
    /* harness (takeprize): the card lies at the player's feet once it can be taken, so the game's own pickup
       collider takes it, as the player walking up to it would; the battle closes only once it is taken */
    if (gPortHarnessTakePrize && work->timer >= 60 && gBtlWork->actor != NULL) {
        work->posX = gBtlWork->actor->x;
        work->posY = gBtlWork->actor->y;
        work->posZ = gBtlWork->actor->z;
    }
#endif

    if (work->posZ < 0) {
        work->posZ += 51;
        SpawnBossPrizeCardEffects(work);
    }

    if (gBtlWork->hcEffect == HC_EFFECT_DRAW) {
        ColliderSetRadius(&work->collider, 30);
    } else {
        ColliderSetRadius(&work->collider, 10);
    }

    ColliderSetPosition(&work->collider, work->posX, work->posY, work->posZ);
    WorldToScreen(&work->x, &work->y, work->posX, work->posY, work->posZ);
    WorldToScreen(&work->shadowX, &work->shadowY, work->posX, work->posY, work->groundZ);
    work->priority = -0x1004 - (work->posY >> 8) * 4;
    UpdateBossPrizeScale(work);
    work->flipAngleX += 2;

    if (work->timer == 60) {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->timer <= 59) {
        work->timer++;
    }

    if (work->collider.colliding) {
        work->collected = TRUE;
        m4aSongNumStart(SONG_SYS_ITEMGET);
        ObtainCard(work->cardId);

        if (gGameState.flags & GAME_FLAG_RIKU) {
            InitRikuDeckForWorld(gGameState.world);
        }

        SetTaskUpdate(task, (TaskUpdateFunc)UpdateBossPrizeFlight);
        WorldToScreen(&x, &y, work->posX, work->posY, work->posZ);
        work->posX = x << 8;
        work->posY = y << 8;
        ColliderSetDisabled(&work->collider, TRUE);
        work->steps = 16;
        work->priority = 50;
        AimBossPrizeAtCenter(work);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void PrizeBoss_2(BossPrizeWork* work) {
    u16 pal;
    ObjAffine* affine;
    void* gfx;
    const CardBack* back;
    const CardDef* def;
    s16 shadowScale;

    pal = !work->collected ? GetBattleSpritePriorityFlags(work->posY) : 0;
    affine = AllocObjAffine(work->rotation, work->scaleX, work->scaleY, TRUE);
    def = &gCardDefs[work->cardId];
    DrawSprite(work->x, (u16)work->y - 8, def->gfx, work->tiles, work->palette, affine, pal,
               work->priority + 1);
    back = &gCardBacks[work->stat.category];
    DrawSprite(work->x, (u16)work->y - 8, back->gfx, work->tiles2, work->palette2, affine, pal,
               work->priority);
    gfx = gCardValueDigitFrames[work->stat.value];

    if (def->category != CARD_CATEGORY_ENEMY) {
        DrawSprite(work->x, (u16)work->y - 8, gfx, work->tiles4, work->palette2, affine, pal,
                   work->priority - 1);
    }

    if (!work->collected) {
        shadowScale = Q_8_8(0.8) - ((work->groundZ - work->posZ) >> 7);

        if (shadowScale <= 2) {
            shadowScale = 2;
        }

        DrawSprite(work->shadowX, work->shadowY, gBtlShadowFrames[0], work->tiles5, work->palette3,
                   AllocObjAffine(0, shadowScale, shadowScale, FALSE), pal, work->priority + 2);
    }

    TaskPoolDraw(&work->tasks);
}

void PrizeBoss_3(BossPrizeWork* work) {
    FadeSetPaletteExcluded(BIOS_PTR(work->palette2)->index + 16, FALSE);
    FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, FALSE);
    ColliderUnregister(&work->collider);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles5);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette3);
    TaskPoolDestroy(&work->tasks);
    gBtlWork->prizeCount--;
}

void UpdateBossPrizeScale(BossPrizeWork* work) {
    work->scaleX = (-COS(work->flipAngleX + 0x80) * work->scale) >> 8;
    work->scaleY = (-COS(work->flipAngleY + 0x80) * work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }
}

void AimBossPrizeAtCenter(BossPrizeWork* work) {
    s16 x;
    s16 y;
    s32 dx;
    s32 dy;
    s32 tx;
    s32 ty;

    WorldToScreen(&x, &y, gBtlWork->actor->x, gBtlWork->actor->y, gBtlWork->actor->z);
    tx = 0x7800;
    ty = 0x5000;
    dx = tx - work->posX;
    dy = ty - work->posY;
    work->distance = NormalizeVector2D8(&dx, &dy);
    work->dirX = -dx;
    work->dirY = -dy;
    work->speed = 0x300;
    work->vz = 2;
}

u8 UpdateBossPrizeFlight(BossPrizeWork* work, void* task) {
    s32 dx;
    s32 dy;
    u8 zero;
    u8 rotation;
    s32 x;
    s32 y;
    s16* px;
    s16* py;

    if (work->speed < 0) {
        dx = 0x7800 - work->posX;
        dy = 0x5000 - work->posY;
        NormalizeVector2D8(&dx, &dy);
        work->dirX = -dx;
        work->dirY = -dy;

        if (work->distance <= 0x7FF) {
            work->steps = 0;
            work->rotation = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateBossPrizeShow);
#ifdef VERSION_EU
            CreateCardNameDisplay(&work->tasks, GetLocalizedString(gCardDefs[work->cardId].name));
#else
            CreateCardNameDisplay(&work->tasks, gCardDefs[work->cardId].name);
#endif
        }
    }

    work->posX += (work->dirX * work->speed) >> 8;
    work->posY += (work->dirY * work->speed) >> 8;
    rotation = work->rotation + 32;
    zero = 0;
    work->rotation = rotation;
    work->flipAngleY += (64 - work->flipAngleY) >> 4;
    work->flipAngleX = zero;
    work->distance = VectorLength2D(0x7800 - work->posX, 0x5000 - work->posY);
    work->speed -= work->vz;
    work->vz += 2;

    if (work->scale <= 0xFF) {
        work->scale += 3;
    }

    x = work->posX >> 8;
    px = &work->x;
    *px = x;
    y = work->posY >> 8;
    py = &work->y;
    *py = y;
    UpdateBossPrizeScale(work);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateBossPrizeShow(BossPrizeWork* work, void* task) {
    s32 rotation;
    u16 scale;
    s32 maxScale;

    rotation = work->rotation << 8;
    ApproachValue((s32*)&work->flipAngleY, 0, work->steps);
    ApproachValue(&rotation, 0, work->steps);
    ApproachValue(&work->posX, 0x7800, work->steps);
    ApproachValue(&work->posY, 0x5800, work->steps);
    work->rotation = rotation >> 8;

    if (work->steps != 0) {
        work->steps--;
    }

    scale = work->scale;

    if ((s16)scale <= 0xFF) {
        work->scale = scale + 2;
    } else {
        maxScale = Q_8_8(1);
        work->scale = maxScale;
    }

    work->x = work->posX >> 8;
    work->y = work->posY >> 8;
    UpdateBossPrizeScale(work);
    work->holdTimer++;

    // @bug Should be >=, so card 450 (the first enemy card) is missed (no boss prize is 450).
    if ((u32)work->cardId > CARD_ID_FIRST_ENEMY) {
        if (work->holdTimer == 120) {
            work->holdTimer = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateBossPrizeShrink);
        }
    } else {
        if (work->holdTimer == 30) {
            work->holdTimer = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateBossPrizeShrink);
        }
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdateBossPrizeShrink(BossPrizeWork* work) {
    work->rotation += 32;
    WorldToScreen(&work->targetX, &work->targetY, gBtlWork->actor->x, gBtlWork->actor->y, gBtlWork->actor->z);
    work->x += (work->targetX - work->x) >> 3;
    work->y += (work->targetY - work->y) >> 3;
    work->scaleX -= 10;
    work->scaleY -= 10;

    if (work->scaleX <= 10) {
        return 0;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void SpawnBossPrizeCardEffects(BossPrizeWork* work) {
    CardEffectArgs args;

    if (!work->collected) {
        if (work->effectTimer == 8) {
            if (work->effectCount <= 3) {
                args.x = work->posX;
                args.y = work->posY;
                args.z = work->posZ;
                args.screenSpace = work->collected;
                args.count = &work->effectCount;
                TaskCreate(&work->tasks, &gTaskDescCardEFFECT, &args);
            }

            work->effectTimer = 0;
        } else {
            work->effectTimer++;
        }
    } else {
        if (work->effectTimer == 8) {
            if (work->effectCount <= 7) {
                args.x = work->posX;
                args.y = work->posY;
                args.z = work->posZ;
                args.screenSpace = work->collected;
                args.count = &work->effectCount;
                TaskCreate(&work->tasks, &gTaskDescCardEFFECT, &args);
            }

            work->effectTimer = 0;
        } else {
            work->effectTimer++;
        }
    }
}

void Card_EFFECT_0(CardEffectWork* work, CardEffectArgs* args) {
    work->args = *args;

    if (!work->args.screenSpace) {
        work->posX = args->x + ((GetRandom() % 9 - 4) << 8);
        work->posY = args->y;
        work->posZ = args->z - 0x800;
    } else {
        work->posX = args->x + ((GetRandom() % 33 - 16) << 8);
        work->posY = args->y - 0x1000;
        work->posZ = 0;
    }

    work->tiles = AllocObjTiles(0x80, NULL);
    work->palette = LoadObjPalette(gCardSparklePalette, sizeof(gCardSparklePalette));
    SetObjTileSource(work->tiles, gCardSparkleTiles);
    AnimInit(&work->anim, gCardSparkleAnims, gCardSparkleFrames);
    AnimStart(&work->anim, GetRandom() % 3, 0);
    work->gfx = AnimGetGfx(&work->anim);
    (*work->args.count)++;
}

u8 Card_EFFECT_1(CardEffectWork* work) {
    work->gfx = AnimUpdate(&work->anim);

    if (!work->args.screenSpace) {
        WorldToScreen(&work->x, &work->y, work->posX, work->posY, work->posZ);
        work->posZ -= 0x100;
    } else {
        work->x = work->posX >> 8;
        work->y = work->posY >> 8;
        work->posY -= 0x100;
    }

    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    return 1;
}

void Card_EFFECT_2(CardEffectWork* work) {
    s16 priority;
    s32 flags;

    priority = -4100 - ((work->y >> 8) * 4);
    flags = 0;
    work->priority = priority;
    DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, flags, work->priority);
}

void Card_EFFECT_3(CardEffectWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    work->args.count[0]--;
}

void scrollbar_0(ScrollBarWork* work, u16* args) {
    work->unk_08 = args[0];
    work->unk_0A = args[1];
    work->unk_0C = args[2];
    work->position = args[3];
    work->remaining = args[4];
    work->active = 1;
    work->unk_0E = 0;
    work->unk_17 = 0;
}

u8 scrollbar_1(ScrollBarWork* work) {
    return work->active;
}

void scrollbar_2() {
}

void scrollbar_3() {
}

void ScrollbarRequestClose(ScrollBarWork* work) {
    if (work != NULL) {
        work->active = 0;
    }
}

void ScrollbarAdvance(ScrollBarWork* work) {
    if (work != NULL) {
        if (work->remaining != 0) {
            work->position++;
            work->remaining--;
        } else {
            work->remaining = work->count - 1;
            work->position = 0;
        }
    }
}

void ScrollbarRetreat(ScrollBarWork* work) {
    if (work != NULL) {
        if (work->position != 0) {
            work->position--;
            work->remaining++;
        } else {
            work->position = work->count - 1;
            work->remaining = 0;
        }
    }
}

void ScrollbarDecrementCount(ScrollBarWork* work) {
    if (work != NULL) {
        work->count--;
    }
}

void ScrollbarIncrementCount(ScrollBarWork* work) {
    if (work != NULL) {
        work->count++;
    }
}

void func_08099FE8(ScrollBarWork* work, u16 b, u8 c) {
    if (work != NULL) {
        work->unk_0E = b;
        work->unk_17 = c;
    }
}

ScrollBarWork* CreateScrollbar(void* pool, u16 a, u16 b, u16 c, u16 position, u16 remaining) {
    u16 args[5];

    args[0] = a;
    args[1] = b;
    args[2] = c;
    args[3] = position;
    args[4] = remaining;
    return TaskCreate(pool, &gTaskDescScrollbar, args)->work;
}

void* gReloadChildTiles[4] = {
    gReloadChildWhiteTiles,
    gReloadChildRedTiles,
    gReloadChildBlueTiles,
    gReloadChildBlackTiles,
};

TaskDesc gTaskDescReloadChildren = {
    "RELOAD_CHILDREN",
    (TaskInitFunc)RELOAD_CHILDREN_0,
    (TaskUpdateFunc)RELOAD_CHILDREN_1,
    (TaskDrawFunc)RELOAD_CHILDREN_2,
    (TaskDestroyFunc)RELOAD_CHILDREN_3,
    sizeof(ReloadChildWork),
};

void* gRevCountTileSources[4] = {
    gRevCountBlackTiles,
    gRevCountBlackTiles,
    gRevCountBlackTiles,
    gRevCountBlackTiles,
};

void** gRevCountSprites[4] = {
    gRevCountRedFrames,
    gRevCountRedFrames,
    gRevCountRedFrames,
    gRevCountRedFrames,
};

AnimHeader** gUnk_09EE76E0 = gRevCountRedAnims;

AnimHeader** gUnk_09EE76E4 = gRevCountBlackAnims;

AnimHeader** gUnk_09EE76E8 = gRevCountBlueAnims;

AnimHeader** gUnk_09EE76EC = gRevCountGrayAnims;

TaskDesc gTaskDescREVCOUNT = {
    "REV_COUNT",
    (TaskInitFunc)REV_COUNT_0,
    (TaskUpdateFunc)REV_COUNT_1,
    (TaskDrawFunc)REV_COUNT_2,
    (TaskDestroyFunc)REV_COUNT_3,
    sizeof(RevCountWork),
};

void* gReloadTiles[3] = {
    gReloadRedTiles,
    gReloadGreenTiles,
    gReloadBlueTiles,
};

AnimHeader** gReloadAnims[3] = {
    gReloadRedAnims,
    gReloadGreenAnims,
    gReloadBlueAnims,
};

void** gReloadFrames[3] = {
    gReloadRedFrames,
    gReloadGreenFrames,
    gReloadBlueFrames,
};

TaskDesc gTaskDescRELOAD = {
    "RELOAD",
    (TaskInitFunc)RELOAD_0,
    (TaskUpdateFunc)RELOAD_1,
    (TaskDrawFunc)RELOAD_2,
    (TaskDestroyFunc)RELOAD_3,
    sizeof(ReloadWork),
};

TaskDesc gTaskDescPrizeBoss = {
    "PrizeBoss",
    (TaskInitFunc)PrizeBoss_0,
    (TaskUpdateFunc)PrizeBoss_1,
    (TaskDrawFunc)PrizeBoss_2,
    (TaskDestroyFunc)PrizeBoss_3,
    sizeof(BossPrizeWork),
};

TaskDesc gTaskDescCardEFFECT = {
    "Card_EFFECT",
    (TaskInitFunc)Card_EFFECT_0,
    (TaskUpdateFunc)Card_EFFECT_1,
    (TaskDrawFunc)Card_EFFECT_2,
    (TaskDestroyFunc)Card_EFFECT_3,
    sizeof(CardEffectWork),
};

TaskDesc gTaskDescScrollbar = {
    "scrollbar",
    (TaskInitFunc)scrollbar_0,
    (TaskUpdateFunc)scrollbar_1,
    (TaskDrawFunc)scrollbar_2,
    (TaskDestroyFunc)scrollbar_3,
    sizeof(ScrollBarWork),
};
