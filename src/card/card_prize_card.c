/**
 * card_prize_card.c
 * Field Prize Card
 */

#include "registration_data.h"
#include "map_api.h"
#include "m4a_song.h"
#include "game_state.h"
#include "monsgage.h"
#include "fade.h"
#include "btl_collision.h"
#include "obj_api.h"
#include "engine_math.h"
#include "obj.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_btl.h"
#include "sprites_card_pictures.h"
#include "songs.h"
#include "battle_actor_types.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "field_state.h"
#include "fld_types.h"
#include "map_runtime.h"
#include <stddef.h>
#include "types.h"
#include "lockon.h"
#include "card_prize_card.h"
#include "sprite_palettes.h"
#include "card_ids.h"

const u8 gUnk_090359E8[8] = { 1, 1, 4, 2, 5, 3, 3, 2 };

static void PrizeCard_0(PrizeCardWork* work, PrizeCardTaskArgs* arg) {
    PrizeCardTaskArgs args;
    const CardDef* def;
    const CardBack* back;
    Collider* collider;

    args = *arg;
    work->cardId = args.cardId;
    def = &gCardDefs[args.cardId];
    work->tiles = LoadObjTiles(def->tiles, 0x300);
    work->palette = LoadObjPalette(def->palette, 32);
    work->stat = *(CardStat*)&def->kind;

    if (gCardDefs[work->cardId].flags & (CARD_DEF_FLAG_SUMMON | CARD_DEF_FLAG_FRIEND)) {
        back = &gCardBacks[CARD_CATEGORY_ENEMY];
    } else {
        back = &gCardBacks[work->stat.category];
    }

    work->tiles2 = LoadObjTiles(back->tiles, 0x280);
    work->tiles3 = LoadObjTiles(back->tiles3, 0x600);
    work->palette2 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles4 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    work->tiles5 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette3 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->pos.x = args.x;
    work->pos.y = args.y;
    work->pos.z = args.z;
    work->pos.ground = 0;
    work->rotation = 24;
    FldPosInitGround(&work->pos);
    work->vz = -(GetRandom() % 129 + 0x300);
    work->speed = GetRandom() % 129 + 0x80;
    work->moveAngle = GetRandom() % 256;
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->scale = Q_8_8(0.5);
    work->flipAngleY = 0;
    work->flipAngleX = 0;
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_PICKUP, 30, 10);
    ColliderSetDisabled(collider, TRUE);
    ColliderSetPosition(collider, work->pos.x, work->pos.y, work->pos.z);
    work->timer = 0;
    work->collected[0] = 0;
    work->steps = 0;
    work->holdTimer = 0;
    TaskPoolInit(&work->tasks, 1);
}

static u8 PrizeCard_1(PrizeCardWork* work, void* task) {
    s32 k = 112;
    s16 x;
    s16 y;

    work->prevPos = work->pos;
    work->vz += 0x38;
    work->pos.z += work->vz;
    x = gSineTable[(u8)work->moveAngle];
    work->pos.x += (x * work->speed) >> 8;
    work->pos.y += (-gSineTable[(u8)work->moveAngle + 0x40] * work->speed) >> 8;

    if (IsFldPosBlocked(&work->pos) != 0) {
        work->moveAngle = work->moveAngle + k + GetRandom() % 33;

        work->pos.x = work->prevPos.x;
        work->pos.y = work->prevPos.y;
    } else {
        work->pos.ground = GetFldPosGround(&work->pos);
    }

    if (work->pos.z - 0x800 > work->pos.ground) {
        work->pos.z = work->pos.ground - 0x800;
        work->vz = -((work->vz * 217) >> 8);

        if (work->vz > -0x200) {
            work->vz = -0x200;
        }
    }

    if (work->collider.colliding) {
        work->collected[0] = 1;
        m4aSongNumStart(SONG_SYS_ITEMGET);

        if (work->cardId <= CARD_ID_LAST_VALUED) {
            ObtainCard(work->cardId);
        }

        SetTaskUpdate(task, (TaskUpdateFunc)UpdateFieldPrizeCardFlight);
        x = (work->pos.x >> 8) - (gFieldState->x >> 8);
        y = (work->pos.y >> 8) + (work->pos.z >> 8) - (gFieldState->y >> 8);
        work->pos.x = x << 8;
        work->pos.y = y << 8;
        ColliderSetDisabled(&work->collider, TRUE);
        work->steps = 16;
        work->priority = 50;
        AimFieldPrizeCardAtCenter(work);
        return 1;
    }

    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
    work->x = (work->pos.x >> 8) - (gFieldState->x >> 8);
    work->y = (work->pos.y >> 8) + (work->pos.z >> 8) - (gFieldState->y >> 8);
    work->shadowX = (work->pos.x >> 8) - (gFieldState->x >> 8);
    work->shadowY = (work->pos.y >> 8) + (work->pos.ground >> 8) - (gFieldState->y >> 8);
    work->priority = -0x1004 - (work->pos.y >> 8) * 4;
    UpdateFieldPrizeCardScale(work);
    work->flipAngleX += 2;

    if (work->timer == 20) {
        ColliderSetDisabled(&work->collider, FALSE);
    }

    if (work->timer <= 59) {
        work->timer++;
    }

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        return 0;
    }

    return 1;
}

void AimFieldPrizeCardAtCenter(PrizeCardWork* work) {
    s32 cx = 0x7800;
    s32 cy = 0x5000;
    s32 toCenter[2];

    toCenter[0] = cx - work->pos.x;
    toCenter[1] = cy - work->pos.y;
    work->distance = NormalizeVector2D8(&toCenter[0], &toCenter[1]);
    work->dirX = -toCenter[0];
    work->dirY = -toCenter[1];
    work->speed = 0x300;
    work->vz = 2;
}

u8 UpdateFieldPrizeCardFlight(PrizeCardWork* work, void* task) {
    s32 toCenter[2];

    if (work->speed < 0) {
        toCenter[0] = 0x7800 - work->pos.x;
        toCenter[1] = 0x5000 - work->pos.y;
        NormalizeVector2D8(&toCenter[0], &toCenter[1]);
        work->dirX = -toCenter[0];
        work->dirY = -toCenter[1];

        if (work->distance <= 0x7FF) {
            work->steps = 0;
            work->rotation = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateFieldPrizeCardShow);
#ifdef VERSION_EU
            CreateCardNameDisplay(&work->tasks, GetLocalizedString(gCardDefs[work->cardId].name));
#else
            CreateCardNameDisplay(&work->tasks, gCardDefs[work->cardId].name);
#endif
        }
    }

    work->pos.x += (work->dirX * work->speed) >> 8;
    work->pos.y += (work->dirY * work->speed) >> 8;
    work->rotation += 32;
    work->flipAngleY += (64 - work->flipAngleY) >> 4;
    work->flipAngleX = 0;
    work->distance = VectorLength2D(0x7800 - work->pos.x, 0x5000 - work->pos.y);
    work->speed -= work->vz;
    work->vz += 2;

    if (work->scale <= 255) {
        work->scale += 3;
    }

    work->x = work->pos.x >> 8;
    work->y = work->pos.y >> 8;
    UpdateFieldPrizeCardScale(work);

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        return 0;
    }

    return 1;
}

u8 UpdateFieldPrizeCardShow(PrizeCardWork* work, void* task) {
    s32 rotation;

    rotation = work->rotation << 8;
    ApproachValue((s32*)&work->flipAngleY, 0, work->steps);
    ApproachValue(&rotation, 0, work->steps);
    ApproachValue(&work->pos.x, 0x7800, work->steps);
    ApproachValue(&work->pos.y, 0x5800, work->steps);
    work->rotation = rotation >> 8;

    if (work->steps != 0) {
        work->steps--;
    }

    if (work->scale <= 255) {
        work->scale += 2;
    } else {
        work->scale = Q_8_8(1);
    }

    work->x = work->pos.x >> 8;
    work->y = work->pos.y >> 8;
    UpdateFieldPrizeCardScale(work);
    work->holdTimer++;

    // @bug Should be >=, so card 450 (the first enemy card) is missed (never called).
    if (work->cardId > CARD_ID_FIRST_ENEMY) {
        if (work->holdTimer == 120) {
            work->holdTimer = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdateFieldPrizeCardShrink);
        }
    } else if (work->holdTimer == 30) {
        work->holdTimer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateFieldPrizeCardShrink);
    }

    TaskPoolUpdate(&work->tasks);

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        return 0;
    }

    return 1;
}

u8 UpdateFieldPrizeCardShrink(PrizeCardWork* work) {
    work->rotation += 32;
    work->targetX = (gFieldState->actor.fieldPosition.x >> 8) - (gFieldState->x >> 8);
    work->targetY = (gFieldState->actor.fieldPosition.y >> 8) + (gFieldState->actor.fieldPosition.z >> 8) -
                (gFieldState->y >> 8);
    work->x += (work->targetX - work->x) >> 3;
    work->y += (work->targetY - work->y) >> 3;
    work->scaleX -= 10;
    work->scaleY -= 10;

    if (work->scaleX > 10 && !(gFieldState->flags & FIELD_FLAG_ROOM_CREATE)) {
        return 1;
    }

    return 0;
}

static void PrizeCard_2(PrizeCardWork* work) {
    u16 pal;
    ObjAffine* affine;
    void* gfx;
    const CardBack* back;
    const CardDef* def;
    s16 shadowScale;
    s32 collected;

    collected = work->collected[0];
    pal = 0;

    if (collected == 0) {
        pal = SPRITE_PRIORITY(2);
    }

    if (work->scaleX == Q_8_8(1) && work->rotation == 0) {
        affine = NULL;
    } else {
        affine = AllocObjAffine(work->rotation, work->scaleX, work->scaleY, TRUE);
    }

    def = &gCardDefs[work->cardId];
    DrawSprite(work->x, (u16)work->y - 8, def->gfx, work->tiles, work->palette,
               affine, pal, work->priority + 1);
    back = &gCardBacks[work->stat.category];
    DrawSprite(work->x, (u16)work->y - 8, back->gfx, work->tiles2, work->palette2,
               affine, pal, work->priority);
    gfx = gCardValueDigitFrames[work->stat.value];
    DrawSprite(work->x, (u16)work->y - 8, gfx, work->tiles4, work->palette2, affine,
               pal, work->priority - 1);

    if (work->collected[0] == 0) {
        shadowScale = Q_8_8(0.8) - ((work->pos.ground - work->pos.z) >> 7);

        if (shadowScale <= 2) {
            shadowScale = 2;
        }

        DrawSprite(work->shadowX, work->shadowY, gBtlShadowFrames[0],
                   work->tiles5, work->palette3, AllocObjAffine(0, shadowScale, shadowScale, FALSE), pal,
                   work->priority + 2);
    }

    TaskPoolDraw(&work->tasks);
}

static void PrizeCard_3(PrizeCardWork* work) {
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
}

void UpdateFieldPrizeCardScale(PrizeCardWork* work) {
    work->scaleX = (-COS(work->flipAngleX + 0x80) * work->scale) >> 8;
    work->scaleY = (-COS(work->flipAngleY + 0x80) * work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }
}

void SpawnRandomFieldPrizeCard(TaskPool* pool, s32 x, s32 y, s32 z) {
    PrizeCardTaskArgs args;

    args.x = x;
    args.y = y;
    args.z = z;
    args.cardId = PickPrizeMapCardForWorld(gGameState.world, 0);
    CreateFieldPrizeCardTask(pool, &args);
}

void SpawnFieldPrizeCard(TaskPool* pool, s32 x, s32 y, s32 z, s32 cardId) {
    PrizeCardTaskArgs args;

    args.x = x;
    args.y = y;
    args.z = z;
    args.cardId = cardId;
    CreateFieldPrizeCardTask(pool, &args);
}

void CreateFieldPrizeCardTask(TaskPool* pool, PrizeCardTaskArgs* args) {
    TaskCreate(pool, &gTaskDescFieldPrizeCard, args);
}

TaskDesc gTaskDescFieldPrizeCard = {
    "PrizeCard",
    (TaskInitFunc)PrizeCard_0,
    (TaskUpdateFunc)PrizeCard_1,
    (TaskDrawFunc)PrizeCard_2,
    (TaskDestroyFunc)PrizeCard_3,
    sizeof(PrizeCardWork),
};
