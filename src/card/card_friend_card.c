/**
 * card_friend_card.c
 * Card Pickups, Sleight Names and Premium Bonus
 */

#ifdef PLATFORM_PC
#include <stdlib.h>
void PortLog(const char* fmt, ...);
#endif
#include "macros.h"
#include "registration_data.h"
#include "system_state.h"
#include "card_battle.h"
#include "player_progression.h"
#include "m4a_song.h"
#include "monsgage.h"
#include "fade.h"
#include "btl_collision.h"
#include "obj_api.h"
#include "battle_actor.h"
#include "display.h"
#include "engine_math.h"
#include "listpool.h"
#include "anim.h"
#include "obj.h"
#include "taskpool.h"
#include "key.h"
#include "malloc.h"
#include "card.h"
#include "sprites_btl.h"
#include "sprites_card.h"
#include "sprites_premire_chance.h"
#include "sprites_card_pictures.h"
#include "gba/io_reg.h"
#include "card_ids.h"
#include "gba/keys.h"
#include "songs.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "bg_animation_data.h"
#include "boss_card_data.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_label_data.h"
#include "card_types.h"
#include "card_ui_types.h"
#include "types.h"
#include "engine.h"
#include "gba/macro.h"
#include "sprite_palettes.h"
#include <stddef.h>
#include "lockon.h"
#include "card_friend_card.h"
#include "jiminy_records_index_data.h"

struct CardListWork* gCardListWork EWRAM_COMMON(4);

static const u32 sFriendCardIds[8] = {
    CARD_ID(CARD_GOOFY, 0),
    CARD_ID(CARD_DONALD_DUCK, 0),
    CARD_ID(CARD_ALADDIN, 0),
    CARD_ID(CARD_ARIEL, 0),
    CARD_ID(CARD_JACK, 0),
    CARD_ID(CARD_PETER_PAN, 0),
    CARD_ID(CARD_THE_BEAST, 0),
    CARD_ID(CARD_THE_KING, 0),
};

void Friend_card_0(PickupCardWork* work, s32* args) {
    Collider* collider;

    work->cardId = args[3];
    work->posX = args[0];
    work->posY = args[1];
    work->posZ = args[2];
    work->floor = 0;
    work->moveAngle = GetRandom();
    work->vz = -(GetRandom() % 129 + 0x300);
    work->speed = GetRandom() % 129 + 0x80;
    work->flipAngleX = 0;
    work->flipAngleY = 0;
    work->angle = 24;
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->scale = Q_8_8(0.5);
    work->screenSpace = FALSE;
    work->unk_1CB = 0;
    work->unk_1CC = 0;
    work->timer = 0;
    work->visible = 1;
    work->cardDef = &gCardDefs[args[3]];

    if (work->cardDef->flags & CARD_DEF_FLAG_FRIEND) {
        work->backCategory = CARD_CATEGORY_ENEMY;
    } else {
        work->backCategory = work->cardDef->category;
    }

    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles3 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    work->tiles2 = LoadObjTiles(work->cardDef->tiles, 0x300);
    work->palette2 = LoadObjPalette(work->cardDef->palette, 32);
    work->tiles4 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette3 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_PICKUP, 8, 10);
    ColliderSetPosition(collider, work->posX, work->posY, work->posZ);
    TaskPoolInit(&work->tasks, 1);
    gBtlWork->prizeCount++;
}

void Heartless_card_0(PickupCardWork* work, s32* args) {
    Collider* collider;

    work->cardId = args[3];
    work->posX = args[0];
    work->posY = args[1];
    work->posZ = args[2];
    work->floor = 0;
    work->moveAngle = GetRandom();
    work->vz = -(GetRandom() % 129 + 0x300);
    work->speed = GetRandom() % 129 + 0x80;
    work->flipAngleX = 0;
    work->flipAngleY = 0;
    work->angle = 24;
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->scale = Q_8_8(0.5);
    work->screenSpace = FALSE;
    work->unk_1CB = 0;
    work->unk_1CC = 0;
    work->timer = 0;
    work->visible = 1;
    work->cardDef = &gCardDefs[args[3]];

    if (work->cardDef->flags & CARD_DEF_FLAG_FRIEND) {
        work->backCategory = CARD_CATEGORY_ENEMY;
    } else {
        work->backCategory = work->cardDef->category;
    }

    work->tiles = LoadObjTiles(gCardBacks[work->backCategory].tiles, 0x280);
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles3 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    work->tiles2 = LoadObjTiles(work->cardDef->tiles, 0x300);
    work->palette2 = LoadObjPalette(work->cardDef->palette, 32);
    work->tiles4 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette3 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_PICKUP, 8, 10);
    ColliderSetPosition(collider, work->posX, work->posY, work->posZ);
    TaskPoolInit(&work->tasks, 1);
    gBtlWork->prizeCount++;
}

void Gimmick_card_0(PickupCardWork* work, GimmickCardArgs* args) {
    Collider* collider;

    work->cardId = args->cardId;
    work->posX = args->x;
    work->posY = args->y;
    work->posZ = args->z;
    work->floor = 0;
    work->moveAngle = GetRandom();
    work->vz = -((u16)(GetRandom() % 129) + 0x300);
    work->speed = (u16)(GetRandom() % 129) + 0x80;
    work->flipAngleX = 0;
    work->flipAngleY = 0;
    work->angle = 24;
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->scale = Q_8_8(0.5);
    work->screenSpace = FALSE;
    work->unk_1CB = 0;
    work->unk_1CC = 0;
    work->timer = 0;
    work->visible = 1;
    work->cardDef = &gCardDefs[args->cardId];
    work->backCategory = work->cardDef->category;
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->tiles3 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    work->tiles2 = LoadObjTiles(work->cardDef->tiles, 0x300);
    work->palette2 = LoadObjPalette(work->cardDef->palette, 32);
    work->tiles4 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette3 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_PICKUP, 8, 10);
    ColliderSetPosition(collider, work->posX, work->posY, work->posZ);
    TaskPoolInit(&work->tasks, 1);
    gBtlWork->prizeCount++;
}

enum PickupCardFlight {
    PICKUP_CARD_FLIGHT_TO_DECK,
    PICKUP_CARD_FLIGHT_TO_CENTER
};

void StartPickupCardFlight(PickupCardWork* work, u8 target) {
    s32 dx;
    s32 dy;
    s32 tx;
    s32 ty;

    if (target == PICKUP_CARD_FLIGHT_TO_CENTER) {
        tx = 0x7800;
        ty = 0x5000;
    } else {
        tx = 0;
        ty = 0xA000;
    }

    dx = tx - work->posX;
    dy = ty - work->posY;
    work->distance = NormalizeVector2D8(&dx, &dy);
    work->dirX = -dx;
    work->dirY = -dy;
    work->speed = 0x300;
    work->vz = 2;
}

s32 Friend_card_1(PickupCardWork* work, void* task) {
    s16 sx;
    s32 nextTimer;
    u16 timer;
    s16 sy;

    if (gBtlWork->phase == BTL_PHASE_END) {
        return 0;
    }

    work->vz += gBtlWork->gravity;
    work->posZ += work->vz;
    work->posX += (gSineTable[work->moveAngle] * work->speed) >> 8;
    work->posY += (-gSineTable[work->moveAngle + 64] * work->speed) >> 8;

    if (ClampBattlePosition(&work->posX, &work->posY, -10, -10) != 0) {
        work->moveAngle = (u8)(work->moveAngle + 112) + GetRandom() % 33;
    }

    if (work->posZ - 0x800 > work->floor) {
        work->posZ = work->floor - 0x800;
        work->vz = -((204 * work->vz) >> 8);
        work->moveAngle = GetAngle(work->posX, work->posY,
                               gBtlWork->actor->x,
                               gBtlWork->actor->y);
        work->moveAngle = (u8)(work->moveAngle + 224) + GetRandom() % 65;

        if (work->vz > -0x200) {
            work->vz = -0x200;
        }
    }

    if (gBtlWork->hcEffect == HC_EFFECT_DRAW) {
        ColliderSetRadius(&work->collider, 50);
    } else {
        ColliderSetRadius(&work->collider, 10);
    }

    if (ApplyBattleBounds(&work->posX, &work->posY, &work->posZ,
                          &work->floor) != 0) {
        work->moveAngle = (u8)(work->moveAngle + 112) + GetRandom() % 33;
    }

    if (work->collider.colliding) {
        m4aSongNumStart(SONG_SYS_ITEMGET);

#ifdef VERSION_EU
        work->priority = 10;
#endif

        if (gCardBattleState != NULL) {
            WorldToScreen(&sx, &sy, work->posX, work->posY,
                          work->posZ);
            work->posX = sx << 8;
            work->posY = sy << 8;
            work->screenSpace = TRUE;
            ColliderSetDisabled(&work->collider, TRUE);
            StartPickupCardFlight(work, PICKUP_CARD_FLIGHT_TO_DECK);
#ifdef VERSION_EU
            work->visible = 1;
#endif
            SetTaskUpdate(task, (TaskUpdateFunc)FlyPickupCardToDeck);
        }

        return 1;
    }

    ColliderSetPosition(&work->collider, work->posX, work->posY,
                  work->posZ);
    work->scaleX =
        (-COS(work->flipAngleX + 128) *
         work->scale) >> 8;
    work->scaleY =
        (-COS(work->flipAngleY + 128) *
         work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }

    work->flipAngleX += 2;
    work->priority = -0x1004 - (work->posY >> 8) * 4;
    TaskPoolUpdate(&work->tasks);

    if (gBtlWork->flags & BTL_FLAG_TUTORIAL) {
        return 1;
    }

    timer = work->timer;

    if (timer > 359) {
        return 0;
    }

    nextTimer = timer + 1;
    work->timer = nextTimer;

    if (work->timer > 279 && nextTimer % 4 == 0) {
        work->visible ^= 1;
    }

    return 1;
}

s32 Gimmick_card_1(PickupCardWork* work, void* task) {
    s16 sx;
    s32 nextTimer;
    u16 timer;
    s16 sy;

    if (gBtlWork->phase == BTL_PHASE_END) {
        return 0;
    }

    work->vz += 56;
    work->posZ += work->vz;
    work->posX += (gSineTable[work->moveAngle] * work->speed) >> 8;
    work->posY += (-gSineTable[work->moveAngle + 64] * work->speed) >> 8;

    if (ClampBattlePosition(&work->posX, &work->posY, -10, -10) != 0) {
        work->moveAngle = (u8)(work->moveAngle + 112) + GetRandom() % 33;
    }

    if (work->posZ - 0x800 > work->floor) {
        work->posZ = work->floor - 0x800;
        work->vz = -((204 * work->vz) >> 8);
        work->moveAngle = GetAngle(work->posX, work->posY,
                               gBtlWork->actor->x,
                               gBtlWork->actor->y);
        work->moveAngle = (u8)(work->moveAngle + 224) + GetRandom() % 65;

        if (work->vz > -0x200) {
            work->vz = -0x200;
        }
    }

    if (gBtlWork->hcEffect == HC_EFFECT_DRAW) {
        ColliderSetRadius(&work->collider, 50);
    } else {
        ColliderSetRadius(&work->collider, 10);
    }

    if (ApplyBattleBounds(&work->posX, &work->posY, &work->posZ,
                          &work->floor) != 0) {
        work->moveAngle = (u8)(work->moveAngle + 112) + GetRandom() % 33;
    }

#ifdef PLATFORM_PC
    {
        /* harness (issue #83): KHCOM_GIMMICK_PICKUP=1 picks a dropped gimmick card up as soon as it lands */
        static int sAuto = -1;
        if (sAuto < 0) sAuto = getenv("KHCOM_GIMMICK_PICKUP") != NULL;
        if (sAuto && work->vz > -0x300 && work->vz < 0)
            work->collider.colliding = TRUE;
    }
#endif
    if (work->collider.colliding) {
        m4aSongNumStart(SONG_SYS_ITEMGET);
#ifdef PLATFORM_PC
        PortLog("battle: gimmick card collected");
#endif

#ifdef VERSION_EU
        work->priority = 10;
#endif

        if (gCardBattleState != NULL) {
            WorldToScreen(&sx, &sy, work->posX, work->posY,
                          work->posZ);
            work->posX = sx << 8;
            work->posY = sy << 8;
            work->screenSpace = TRUE;
            ColliderSetDisabled(&work->collider, TRUE);
            StartPickupCardFlight(work, PICKUP_CARD_FLIGHT_TO_DECK);
#ifdef VERSION_EU
            work->visible = 1;
#endif
            SetTaskUpdate(task, (TaskUpdateFunc)FlyPickupCardToDeck);
        }

        return 1;
    }

    ColliderSetPosition(&work->collider, work->posX, work->posY,
                  work->posZ);
    work->scaleX =
        (-COS(work->flipAngleX + 128) *
         work->scale) >> 8;
    work->scaleY =
        (-COS(work->flipAngleY + 128) *
         work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }

    work->flipAngleX += 2;
    work->priority = -0x1004 - (work->posY >> 8) * 4;
    TaskPoolUpdate(&work->tasks);

    timer = work->timer;

    if (timer > 359) {
        gCardBattleState->gimmickCardCount -= 1;
        return 0;
    }

    nextTimer = timer + 1;
    work->timer = nextTimer;

    if (work->timer > 279 && nextTimer % 4 == 0) {
        work->visible ^= 1;
    }

    return 1;
}

u8 FlyPickupCardToDeck(PickupCardWork* work) {
    s32 dx;
    s32 dy;
    u16 scale;

    if (gBtlWork->phase == BTL_PHASE_END) {
        return 0;
    }

    if (work->speed < 0) {
        dx = -work->posX;
        dy = 0xA000 - work->posY;
        NormalizeVector2D8(&dx, &dy);
        work->dirX = -dx;
        work->dirY = -dy;

        if (work->distance < 0x800) {
            if (work->cardId >= CARD_GIMMICK_0 && work->cardId <= CARD_GIMMICK_4) {
                gCardBattleState->pickedGimmickCardId = work->cardId;
            } else {
                gCardBattleState->pickedFriendCardId = work->cardId;
            }

            return 0;
        }
    }

    work->posX += (work->dirX * work->speed) >> 8;
    work->posY += (work->dirY * work->speed) >> 8;
    work->angle += 32;
    work->flipAngleY += (64 - work->flipAngleY) >> 4;
    work->flipAngleX = 0;
    work->distance = VectorLength2D(-work->posX, 0xA000 - work->posY);
    work->speed -= work->vz;
    work->vz += 2;
    scale = work->scale;

    if ((s16)scale <= 255) {
        work->scale = scale + 3;
    }

    work->scaleX = (-COS(work->flipAngleX + 128) * work->scale) >> 8;
    work->scaleY = (-COS(work->flipAngleY + 128) * work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 FlyHeartlessCardToPlayer(PickupCardWork* work) {
    s16 x;
    s16 y;

    work->angle += 32;
    WorldToScreen(&x, &y, gBtlWork->actor->x, gBtlWork->actor->y, gBtlWork->actor->z);
    work->posX += ((x << 8) - work->posX) >> 3;
    work->posY += ((y << 8) - work->posY) >> 3;
    work->scaleX -= 10;
    work->scaleY -= 10;

    if (work->scaleX <= 10) {
        return 0;
    }

    return 1;
}

s32 WaitHeartlessCardName(PickupCardWork* work, void* task) {
    work->timer += 1;

    if (work->timer == 60) {
        SetTaskUpdate(task, (TaskUpdateFunc)FlyHeartlessCardToPlayer);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

s32 FlyHeartlessCardToCenter(PickupCardWork* work, void* task) {
    s32 dx = 0;
    s32 dy = 0;
    u16 scale;

    if (work->speed < 0) {
        dx = 0x7800 - work->posX;
        dy = 0x5000 - work->posY;
        NormalizeVector2D8(&dx, &dy);
        work->dirX = -dx;
        work->dirY = -dy;

        if (work->distance < 0x800) {
            work->flipAngleY = 0;
            work->angle = 0;
            work->flipAngleX = 0;
            work->posX = 0x7800;
            work->posY = 0x5000;
            work->scaleX = Q_8_8(1);
            work->scaleY = Q_8_8(1);
#ifdef VERSION_EU
            CreateCardNameDisplay(&work->tasks, GetLocalizedString(gCardDefs[work->cardId].name));
#else
            CreateCardNameDisplay(&work->tasks, gCardDefs[work->cardId].name);
#endif
            SetTaskUpdate(task, (TaskUpdateFunc)WaitHeartlessCardName);
            work->timer = 0;
            work->priority = 50;
            return 1;
        }
    }

    work->posX += (work->dirX * work->speed) >> 8;
    work->posY += (work->dirY * work->speed) >> 8;
    work->angle += 32;
    work->flipAngleY += (64 - work->flipAngleY) >> 4;
    work->flipAngleX = 0;
    work->distance = VectorLength2D(0x7800 - work->posX, 0x5000 - work->posY);
    work->speed -= work->vz;
    work->vz += 2;
    scale = work->scale;

    if ((s16)scale <= 255) {
        work->scale = scale + 3;
    }

    work->scaleX = (-COS(work->flipAngleX + 128) * work->scale) >> 8;
    work->scaleY = (-COS(work->flipAngleY + 128) * work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

s32 Heartless_card_1(PickupCardWork* work, void* task) {
    s16 x;
    s16 y;

    work->vz += 0x38;
    work->posZ += work->vz;
    work->posX += (gSineTable[work->moveAngle] * work->speed) >> 8;
    work->posY += (-gSineTable[work->moveAngle + 64] * work->speed) >> 8;

    if (ClampBattlePosition(&work->posX, &work->posY, -10, -10) != 0) {
        work->moveAngle = (u8)(work->moveAngle + 0x70) + GetRandom() % 33;
    }

    if (work->posZ - 0x800 > work->floor) {
        work->posZ = work->floor - 0x800;
        work->vz = -((work->vz * 204) >> 8);
        work->moveAngle = GetAngle(work->posX, work->posY, gBtlWork->actor->x, gBtlWork->actor->y);
        work->moveAngle = (u8)(work->moveAngle + 0xE0) + GetRandom() % 65;

        if (work->vz > -0x200) {
            work->vz = -0x200;
        }
    }

    if (gBtlWork->hcEffect == HC_EFFECT_DRAW) {
        ColliderSetRadius(&work->collider, 50);
    } else {
        ColliderSetRadius(&work->collider, 10);
    }

    if (ApplyBattleBounds(&work->posX, &work->posY, &work->posZ, &work->floor)) {
        work->moveAngle = (u8)(work->moveAngle + 0x70) + GetRandom() % 33;
    }

    if (work->collider.colliding) {
        m4aSongNumStart(SONG_SYS_ITEMGET);
#ifdef VERSION_EU
        work->priority = 10;
#endif
        ObtainCard(work->cardId);
        WorldToScreen(&x, &y, work->posX, work->posY, work->posZ);
        work->posX = x << 8;
        work->posY = y << 8;
        work->screenSpace = TRUE;
        ColliderSetDisabled(&work->collider, TRUE);
        StartPickupCardFlight(work, PICKUP_CARD_FLIGHT_TO_CENTER);
        SetTaskUpdate(task, (TaskUpdateFunc)FlyHeartlessCardToCenter);
        return 1;
    }

    ColliderSetPosition(&work->collider, work->posX, work->posY, work->posZ);
    work->scaleX = (-COS(work->flipAngleX + 128) * work->scale) >> 8;
    work->scaleY = (-COS(work->flipAngleY + 128) * work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }

    work->flipAngleX += 2;
    work->priority = -0x1004 - ((work->posY >> 8) << 2);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void PickupCardDraw(PickupCardWork* work) {
    s16 x;
    u16 y;
    ObjAffine* affine;
    s16 shadowScale;
    u8 kind;

    if (work->visible != 0) {
        if (!work->screenSpace) {
            work->spriteFlags = GetBattleSpritePriorityFlags(work->posY);
            WorldToScreen(&x, &y, work->posX, work->posY,
                          work->posZ);
        } else {
            work->spriteFlags = 0;
            x = work->posX >> 8;
            y = work->posY >> 8;
        }

        affine = AllocObjAffine(work->angle, work->scaleX,
                                work->scaleY, FALSE);
        // @bug Leaving a battle frees gCardBattleState before this card is destroyed (NULL read).
#ifdef PLATFORM_PC
        /* the cartridge draws the back from stray tiles for that frame; the port skips it */
        if (gCardBattleState != NULL)
#endif
        DrawSprite(x, y - 8,
                   gCardBacks[work->cardDef->category].gfx,
                   gCardBattleState->tiles[work->cardDef->category],
                   work->palette, affine,
                   work->spriteFlags, work->priority);
        DrawSprite(x, y - 8, work->cardDef->gfx,
                   work->tiles2, work->palette2, affine,
                   work->spriteFlags, work->priority + 1);
        kind = work->cardDef->value;
        DrawSprite(x, y - 8, gCardValueDigitFrames[kind],
                   work->tiles3, work->palette, affine,
                   work->spriteFlags, work->priority - 2);
        shadowScale = Q_8_8(0.8) - ((work->floor - work->posZ) >> 7);

        if (shadowScale <= 2) {
            shadowScale = 2;
        }

        if (!work->screenSpace) {
            WorldToScreen(&work->x, &work->y,
                          work->posX, work->posY,
                          work->floor);
            DrawSprite(work->x, work->y,
                       gBtlShadowFrames[0], work->tiles4,
                       work->palette3, AllocObjAffine(0, shadowScale, shadowScale, FALSE),
                       work->spriteFlags, work->priority + 2);
        }

        TaskPoolDraw(&work->tasks);
    }
}

void Heartless_card_2(PickupCardWork* work) {
    s16 x;
    u16 y;
    ObjAffine* affine;
    s16 shadowScale;

    if (work->visible != 0) {
        if (!work->screenSpace) {
            work->spriteFlags = GetBattleSpritePriorityFlags(work->posY);
            WorldToScreen(&x, &y, work->posX, work->posY,
                          work->posZ);
        } else {
            work->spriteFlags = 0;
            x = work->posX >> 8;
            y = work->posY >> 8;
        }

        affine = AllocObjAffine(work->angle, work->scaleX,
                                work->scaleY, FALSE);
        DrawSprite(x, y - 8,
                   gCardBacks[work->cardDef->category].gfx,
                   work->tiles, work->palette, affine,
                   work->spriteFlags, work->priority);
        DrawSprite(x, y - 8, work->cardDef->gfx,
                   work->tiles2, work->palette2, affine,
                   work->spriteFlags, work->priority + 1);
        shadowScale = Q_8_8(0.8) - ((work->floor - work->posZ) >> 7);

        if (shadowScale <= 2) {
            shadowScale = 2;
        }

        if (!work->screenSpace) {
            WorldToScreen(&work->x, &work->y,
                          work->posX, work->posY,
                          work->floor);
            DrawSprite(work->x, work->y,
                       gBtlShadowFrames[0], work->tiles4,
                       work->palette3, AllocObjAffine(0, shadowScale, shadowScale, FALSE),
                       work->spriteFlags, work->priority + 2);
        }

        TaskPoolDraw(&work->tasks);
    }
}

void PickupCardDestroy(PickupCardWork* work) {
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette3);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
    // @bug Leaving a battle frees gCardBattleState before this card is destroyed (NULL write).
    BIOS_PTR(gCardBattleState)->friendCardCount = 0;
    gBtlWork->prizeCount--;
}

void Heartless_card_3(PickupCardWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjPalette(work->palette3);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
    gBtlWork->prizeCount--;
}

void CreateFriendCardTask(void* pool, s16 x, s16 y, s16 z, u8 idx) {
    s32 args[4];

    if (gCardBattleState != NULL) {
        if (gCardBattleState->addedFriendCards[0] <= 4) {
            if (gCardBattleState->friendCardCount == 0) {
                args[0] = x << 8;
                args[1] = y << 8;
                args[2] = z << 8;
                args[3] = sFriendCardIds[idx] + GetRandom() % 9;
                TaskCreate(pool, &gTaskDescFriendCard, args);
                gCardBattleState->friendCardCount++;
            }
        }
    }
}

void CreateHeartlessCardTask(void* pool, s16 x, s16 y, s16 z, u16 kind) {
    s32 args[4];
    const s32* cardIds;

    cardIds = gEnemyCardIds[kind];
    args[0] = x << 8;
    args[1] = y << 8;
    args[2] = z << 8;
    args[3] = cardIds[0];
    TaskCreate(pool, &gTaskDescHeartlessCard, args);
}

void CreateGimmickCardTask(void* pool, s16 x, s16 y, s16 z, u16 cardId) {
    s32 args[4];

    if (gCardBattleState != NULL && gCardBattleState->gimmickCardCount == 0) {
        gCardBattleState->gimmickCardCount++;
        args[0] = x << 8;
        args[1] = y << 8;
        args[2] = z << 8;
        args[3] = cardId;
        TaskCreate(pool, &gTaskDescGimmickCard, args);
    }
}

void StockNameSora_0(StockNameWork* work, const s32* src) {
    u8 i;
    s32* dst;
    s32* name;
    const s32* names;
    s32 z;
    ObjTiles* obj;

    if (src != NULL) {
        names = src;

        for (i = 0; i < 6; i++) {
            dst = work->stockNames;
            name = &dst[i];
            *name = names[i];
        }

        work->cycling = TRUE;
    } else {
        work->cycling = FALSE;
    }

    z = 0;
    work->stockNameIndex = z;
    work->unk_04 = z;
    obj = AllocSpriteFrameTiles(0x3C0);
    work->tiles = obj;

    if (!work->cycling) {
#ifdef VERSION_EU
        void** sprites;
        void* srcTiles;

        sprites = (void**)LANGSTR(gStockNameSprites[gCardBattleState->soraStockName].sprites);
        srcTiles = LANGSTR(gStockNameSprites[gCardBattleState->soraStockName].tiles);
        UpdateSpriteFrameTiles(obj, sprites[gStockNameSprites[gCardBattleState->soraStockName].spriteIndex], srcTiles);
#else
        UpdateSpriteFrameTiles(obj, gStockNameSprites[gCardBattleState->soraStockName].sprites[gStockNameSprites[gCardBattleState->soraStockName].spriteIndex], gStockNameSprites[gCardBattleState->soraStockName].tiles);
#endif
    } else {
#ifdef VERSION_EU
        void** sprites;
        void* srcTiles;

        sprites = (void**)LANGSTR(gStockNameSprites[work->stockNames[0]].sprites);
        srcTiles = LANGSTR(gStockNameSprites[work->stockNames[0]].tiles);
        UpdateSpriteFrameTiles(obj, sprites[gStockNameSprites[work->stockNames[0]].spriteIndex], srcTiles);
#else
        UpdateSpriteFrameTiles(obj, gStockNameSprites[work->stockNames[0]].sprites[gStockNameSprites[work->stockNames[0]].spriteIndex], gStockNameSprites[work->stockNames[0]].tiles);
#endif
    }

    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->visible = TRUE;
    work->stockName = gCardBattleState->soraStockName;
}

u8 StockNameSora_1(StockNameWork* work) {
#ifdef VERSION_EU
#ifdef PLATFORM_PC
    void** sprites;
    void* srcTiles;
#else
    void** sprites = (void**)LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].sprites);
    void* srcTiles = LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].tiles);
#endif
#endif

    if (!gCardBattleState->soraStockNameShown || gCardBattleState->soraStockName != work->stockName) {
        return 0;
    }

    if ((gFrameCounter >> 5) & 1) {
        work->visible = TRUE;
    } else {
        work->visible = FALSE;
    }

    if (work->cycling == TRUE && work->visible) {
        work->stockNameIndex++;

        // @bug A full list has no -1 after its six names (reads past stockNames).
#ifdef PLATFORM_PC
        if (work->stockNameIndex >= ARRAY_COUNT(work->stockNames) || work->stockNames[work->stockNameIndex] == -1) {
#else
        if (work->stockNames[work->stockNameIndex] == -1) {
#endif
            work->stockNameIndex = 0;
        }

#ifdef VERSION_EU
#ifdef PLATFORM_PC
        sprites = (void**)LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].sprites);
        srcTiles = LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].tiles);
#else
        // @bug Uses the previous name's sprites and tiles with this name's sprite index.
#endif
        UpdateSpriteFrameTiles(work->tiles, sprites[gStockNameSprites[work->stockNames[work->stockNameIndex]].spriteIndex], srcTiles);
#else
        UpdateSpriteFrameTiles(work->tiles, gStockNameSprites[work->stockNames[work->stockNameIndex]].sprites[gStockNameSprites[work->stockNames[work->stockNameIndex]].spriteIndex], gStockNameSprites[work->stockNames[work->stockNameIndex]].tiles);
#endif
    }

    return 1;
}

#ifdef PLATFORM_PC
static void StockNameSora_2_Body(StockNameWork* work);

/* Sleight name: top-left, under the HP gauge. */
void StockNameSora_2(StockNameWork* work) {
    PortHudBegin(-1, -1);
    StockNameSora_2_Body(work);
    PortHudEnd();
}

static void StockNameSora_2_Body(StockNameWork* work) {
#else
void StockNameSora_2(StockNameWork* work) {
#endif
    if (work->visible) {
        DrawSprite(64, 14, NULL, work->tiles, work->palette, NULL,
#ifdef VERSION_EU
                   SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC,
#else
                   0,
#endif
                   10);
    }
}

void StockNameRiku_0(StockNameWork* work, const s32* src) {
    u8 i;
    s32* dst;
    s32* name;
    const s32* names;
    s32 z;
    ObjTiles* obj;

    if (src != NULL) {
        names = src;

        for (i = 0; i < 6; i++) {
            dst = work->stockNames;
            name = &dst[i];
            *name = names[i];
        }

        work->cycling = TRUE;
    } else {
        work->cycling = FALSE;
    }

    z = 0;
    work->stockNameIndex = z;
    work->unk_04 = z;
    obj = AllocSpriteFrameTiles(0x3C0);
    work->tiles = obj;

    if (!work->cycling) {
#ifdef VERSION_EU
        void** sprites;
        void* srcTiles;

        sprites = (void**)LANGSTR(gStockNameSprites[gCardBattleState->rikuStockName].sprites);
        srcTiles = LANGSTR(gStockNameSprites[gCardBattleState->rikuStockName].tiles);
        UpdateSpriteFrameTiles(obj, sprites[gStockNameSprites[gCardBattleState->rikuStockName].spriteIndex], srcTiles);
#else
        UpdateSpriteFrameTiles(obj, gStockNameSprites[gCardBattleState->rikuStockName].sprites[gStockNameSprites[gCardBattleState->rikuStockName].spriteIndex], gStockNameSprites[gCardBattleState->rikuStockName].tiles);
#endif
    } else {
#ifdef VERSION_EU
        void** sprites;
        void* srcTiles;

        sprites = (void**)LANGSTR(gStockNameSprites[work->stockNames[0]].sprites);
        srcTiles = LANGSTR(gStockNameSprites[work->stockNames[0]].tiles);
        UpdateSpriteFrameTiles(obj, sprites[gStockNameSprites[work->stockNames[0]].spriteIndex], srcTiles);
#else
        UpdateSpriteFrameTiles(obj, gStockNameSprites[work->stockNames[0]].sprites[gStockNameSprites[work->stockNames[0]].spriteIndex], gStockNameSprites[work->stockNames[0]].tiles);
#endif
    }

    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->visible = TRUE;
    work->stockName = gCardBattleState->rikuStockName;
}

u8 StockNameRiku_1(StockNameWork* work) {
#ifdef VERSION_EU
#ifdef PLATFORM_PC
    void** sprites;
    void* srcTiles;
#else
    void** sprites = (void**)LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].sprites);
    void* srcTiles = LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].tiles);
#endif
#endif

    if (!gCardBattleState->rikuStockNameShown || gCardBattleState->rikuStockName != work->stockName) {
        return 0;
    }

    if ((gFrameCounter >> 5) & 1) {
        work->visible = FALSE;
    } else {
        work->visible = TRUE;
    }

    if (work->cycling == TRUE && work->visible) {
        work->stockNameIndex++;

        // @bug A full list has no -1 after its six names (reads past stockNames).
#ifdef PLATFORM_PC
        if (work->stockNameIndex >= ARRAY_COUNT(work->stockNames) || work->stockNames[work->stockNameIndex] == -1) {
#else
        if (work->stockNames[work->stockNameIndex] == -1) {
#endif
            work->stockNameIndex = 0;
        }

#ifdef VERSION_EU
#ifdef PLATFORM_PC
        sprites = (void**)LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].sprites);
        srcTiles = LANGSTR(gStockNameSprites[work->stockNames[work->stockNameIndex]].tiles);
#else
        // @bug Uses the previous name's sprites and tiles with this name's sprite index.
#endif
        UpdateSpriteFrameTiles(work->tiles, sprites[gStockNameSprites[work->stockNames[work->stockNameIndex]].spriteIndex], srcTiles);
#else
        UpdateSpriteFrameTiles(work->tiles, gStockNameSprites[work->stockNames[work->stockNameIndex]].sprites[gStockNameSprites[work->stockNames[work->stockNameIndex]].spriteIndex], gStockNameSprites[work->stockNames[work->stockNameIndex]].tiles);
#endif
    }

    return 1;
}

#ifdef PLATFORM_PC
static void StockNameRiku_2_Body(StockNameWork* work);

/* Sleight name: top centre. */
void StockNameRiku_2(StockNameWork* work) {
    PortHudBegin(0, -1);
    StockNameRiku_2_Body(work);
    PortHudEnd();
}

static void StockNameRiku_2_Body(StockNameWork* work) {
#else
void StockNameRiku_2(StockNameWork* work) {
#endif
    if (gRikuBtlWork->hcEffect != HC_EFFECT_SLEIGHTBLIND && work->visible) {
#ifdef PLATFORM_PC
        int PortHdHudStockNameX(int x, const u16* frame, int stockCount);
        DrawSprite(PortHdHudStockNameX(120, work->tiles->sprite, gCardBattleState->rikuStockCount), 14, NULL, work->tiles,
                   work->palette, NULL, 0, 10);
#else
        DrawSprite(120, 14, NULL, work->tiles, work->palette, NULL, 0, 10);
#endif
    }
}

void StockNameRiku_3(StockNameWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    // @bug Leaving a battle frees gCardBattleState before this name is destroyed (NULL write).
    BIOS_PTR(gCardBattleState)->unk_0D9 = 0;
    work->visible = FALSE;
    BIOS_PTR(gCardBattleState)->unk_0CA = 256;
}

void StockNameSora_3(StockNameWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    gCardBattleState->unk_0D8 = 0;
    work->visible = FALSE;
    gCardBattleState->unk_0C8 = 256;
}

void Premire_Chance_0(PremireChanceWork* work) {
    u8 n = 0;
    u8 i;
    s32 j;
    u16* cards;

    CpuFill32(0, work, sizeof(PremireChanceWork));
    gCardListWork = EwramAlloc(sizeof(CardListWork));
    work->slots = EwramAlloc(sizeof(CardSlot) * 100);
    CpuFill32(0, gCardListWork, sizeof(CardListWork));
    CpuFill32(0, work->slots, sizeof(CardSlot) * 100);
    cards = GetActiveDeck()->cards;
    work->tiles2 = AllocObjTiles(0x120, NULL);
    work->palette2 = LoadObjPalette(gSmallHandCursorPalette, sizeof(gSmallHandCursorPalette));
    SetObjTileSource(work->tiles2, gSmallHandCursorTiles);
    AnimInit(&work->anim, gSmallHandCursorAnims, gSmallHandCursorFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_FRENCH:
        work->tiles = LoadObjTiles(gPremireChanceTitleFrenchTiles, sizeof(gPremireChanceTitleFrenchTiles));
        break;
    case LANGUAGE_GERMAN:
        work->tiles = LoadObjTiles(gPremireChanceTitleGermanTiles, sizeof(gPremireChanceTitleGermanTiles));
        break;
    case LANGUAGE_ITALIAN:
        work->tiles = LoadObjTiles(gPremireChanceTitleItalianTiles, sizeof(gPremireChanceTitleItalianTiles));
        break;
    case LANGUAGE_SPANISH:
        work->tiles = LoadObjTiles(gPremireChanceTitleSpanishTiles, sizeof(gPremireChanceTitleSpanishTiles));
        break;
    case LANGUAGE_ENGLISH:
        work->tiles = LoadObjTiles(gPremireChanceTitleTiles, sizeof(gPremireChanceTitleTiles));
        break;
    default:
        work->tiles = LoadObjTiles(gPremireChanceTitleTiles, sizeof(gPremireChanceTitleTiles));
        break;
    }
#else
    work->tiles = LoadObjTiles(gPremireChanceTitleTiles, sizeof(gPremireChanceTitleTiles));
#endif
    work->palette = LoadObjPalette(gPremireChancePalette, sizeof(gPremireChancePalette));
    work->tiles5 = LoadObjTiles(gPremireChanceBarTiles, sizeof(gPremireChanceBarTiles));
    work->tiles3 = AllocObjTiles(0x3C0, NULL);
    work->palette3 = LoadObjPalette(gCardSelectBoxPalette, sizeof(gCardSelectBoxPalette));
    SetObjTileSource(work->tiles3, gCardSelectBoxTiles);
    AnimInit(&work->anim2, gCardSelectBoxAnims, gCardSelectBoxFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->gfx2 = AnimGetGfx(&work->anim2);
    work->tiles4 = LoadObjTiles(gAButtonIconTiles, sizeof(gAButtonIconTiles));
    work->palette4 = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    FadeToAmount(FADE_MODE_BLACK, 16, 16);
    FadeSetPaletteExcluded(work->palette->index + 16, TRUE);
    FadeSetPaletteExcluded(work->palette3->index + 16, TRUE);
    FadeSetPaletteExcluded(work->palette2->index + 16, TRUE);
    FadeSetPaletteExcluded(work->palette4->index + 16, TRUE);

#ifdef VERSION_EU
    for (i = 0, n = 0; i < DECK_SIZE; i++) {
#else
    for (i = 0; i < DECK_SIZE; i++) {
#endif
        if (cards[i] != CARD_NONE) {
            if (!(gCardCollection[cards[i]] & CARD_FLAG_PREMIUM)) {
                if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category != CARD_CATEGORY_ENEMY) {
                    if (gCardDefs[gCardCollection[cards[i]] & CARD_ID_MASK].category != CARD_CATEGORY_ITEM) {
                        work->slots[n].cardId = gCardCollection[cards[i]] & CARD_ID_MASK;
                        work->slots[n].index = i;
                        work->slots[n].unk_06 = n;
                        work->slots[n].stocked = FALSE;
                        n++;
                    }
                }
            }
        }
    }

    if (n < 10) {
        j = 0;

        do {
            if (cards[j] != CARD_NONE) {
                if (!(gCardCollection[cards[j]] & CARD_FLAG_PREMIUM)) {
                    if (gCardDefs[gCardCollection[cards[j]] & CARD_ID_MASK].category != CARD_CATEGORY_ENEMY) {
                        if (gCardDefs[gCardCollection[cards[j]] & CARD_ID_MASK].category != CARD_CATEGORY_ITEM) {
                            work->slots[n].cardId = gCardCollection[cards[j]] & CARD_ID_MASK;
                            work->slots[n].index = j;
                            work->slots[n].unk_06 = n;
                            work->slots[n].stocked = FALSE;
                            n++;
                        }
                    }
                }
            }

            j++;

            if (j > 98) {
                j = 0;
            }
        } while (n < 10);
    }

    work->cardCount = n;
    TaskPoolInit(&work->tasks, work->cardCount + 1);
    ListPoolInit(&gCardListWork->cards);
    gCardListWork->selectedCard = NULL;
    gCardListWork->effectCount = 0;
    TaskPoolInit(&gCardListWork->effectTasks, 24);
    CreatePremireChanceCardTasks(work);
    work->spinDelay = 10;
    work->advanced = FALSE;
    work->stopped = FALSE;
    work->inputEnabled = TRUE;
    work->resultPending = TRUE;
    work->cursorHidden = 0;
    work->stopTimer = 0;
    work->titleSteps = 16;
    work->slideSteps = 16;
    work->titleX = -0x80;
    work->topY = -0x800;
    work->bottomY = 0xA000;
}

u8 UpdatePremireChanceSpin(PremireChanceWork* work, void* task) {
    PremireChanceCardWork* card;
    s32 steps;
    s32 z;

    if (work->inputEnabled) {
        if ((GetKeysPressed() & A_BUTTON) && !work->stopped) {
            work->stopped = TRUE;
            m4aSongNumStart(SONG_SYS_ITEMGET);
        }

        if ((GetKeysPressed() & B_BUTTON) && work->stopped != TRUE) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            work->titleSteps = 16;
            work->slideSteps = 16;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceClose);
            card = ListPoolFirst(&gCardListWork->cards);

            while (card != NULL) {
                card->state |= PREMIRE_CHANCE_CARD_STATE_HIDDEN;
                work->resultPending = FALSE;
                card = ListPoolNext(&card->node);
            }

            TaskPoolUpdate(&work->tasks);
            TaskPoolUpdate(&gCardListWork->effectTasks);
            z = 0;
            work->cursorHidden = 1;
            work->inputEnabled = z;
            return 1;
        }
    }

    card = ListPoolFirst(&gCardListWork->cards);

    if (!work->stopped) {
        while (card != NULL) {
            steps = card->steps;

            if (steps == 0) {
                m4aSongNumStart(SONG_SYS_CLICK);

                if (card->position < work->cardCount - 1) {
                    card->position++;
                } else {
                    card->position = steps;
                }

                card->steps = work->spinDelay;
                work->advanced = TRUE;
            }

            card = ListPoolNext(&card->node);
        }
    } else {
        work->cursorHidden = 1;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceStop);
    }

    if (work->spinDelay == 4 || work->spinDelay == 10) {
        work->inputEnabled = TRUE;
    } else {
        work->inputEnabled = FALSE;
    }

    if (work->advanced) {
        if (work->spinDelay > 4) {
            work->spinDelay--;
        }

        work->advanced = FALSE;
    }

    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
    TaskPoolUpdate(&work->tasks);
    TaskPoolUpdate(&gCardListWork->effectTasks);
    return 1;
}

u8 Premire_Chance_1(PremireChanceWork* work, void* task) {
    s32 titleX;
    u8* steps;
    PremireChanceCardWork* card;

    card = ListPoolFirst(&gCardListWork->cards);

    if (card != NULL && card->state == PREMIRE_CHANCE_CARD_STATE_SPIN) {
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceSpin);
    }

    steps = &work->slideSteps;

    if (*steps != 0) {
        ApproachValue(&work->topY, 0, *steps);
        ApproachValue(&work->bottomY, 0x9800, *steps);
    } else {
        titleX = work->titleX << 8;
        steps = &work->titleSteps;
        ApproachValue(&titleX, 0, *steps);
        work->titleX = titleX >> 8;
    }

    (*steps)--;
    TaskPoolUpdate(&work->tasks);
    TaskPoolUpdate(&gCardListWork->effectTasks);
    return 1;
}

#ifdef PLATFORM_PC
/* the title banner starts at the window's left edge in the wide view, as the other menus' banners do */
extern int PortScreenMarginX(void);
#define PREMIRE_TITLE_SHIFT PortScreenMarginX()
#else
#define PREMIRE_TITLE_SHIFT 0
#endif
void Premire_Chance_2(PremireChanceWork* work) {
#ifdef PLATFORM_PC
    PortScreenBegin(); /* menu: drawn on the screen layer, at the window's height (as Level Up) */
#endif
    if (work->cursorHidden == 0) {
        DrawSprite(62, 50, work->gfx, work->tiles2, work->palette2, NULL, 0, 0);
        DrawSprite(53, 64, work->gfx2, work->tiles3, work->palette3, NULL, 0, 0);
    }

    if (work->inputEnabled) {
        DrawSprite(88, 70, gAButtonIconFrames[0], work->tiles4, work->palette4, NULL, 0, 0);
    }

#ifdef VERSION_EU
    DrawSprite(work->titleX - PREMIRE_TITLE_SHIFT, 0, gPremireChanceTitles[gLanguage][0], work->tiles, work->palette, NULL, 0, 0);
#else
    DrawSprite(work->titleX - PREMIRE_TITLE_SHIFT, 0, gPremireChanceTitleFrames[0], work->tiles, work->palette, NULL, 0, 0);
#endif
    DrawSprite(120, work->topY >> 8, gPremireChanceBarFrames[0], work->tiles5, work->palette, NULL, 0, 60);
    DrawSprite(120, work->bottomY >> 8, gPremireChanceBarFrames[1], work->tiles5, work->palette, NULL, 0, 60);
#ifdef PLATFORM_PC
    {
        /* the wide view: the bars go on to the window's edges (copies a screen to each side, the window's margin at most) */
        int k, margin = PortScreenMarginX();
        for (k = 1; k * 240 - 120 < margin + 240 && k <= 4; k++) {
            DrawSprite(120 - k * 240, work->topY >> 8, gPremireChanceBarFrames[0], work->tiles5, work->palette, NULL, 0, 60);
            DrawSprite(120 + k * 240, work->topY >> 8, gPremireChanceBarFrames[0], work->tiles5, work->palette, NULL, 0, 60);
            DrawSprite(120 - k * 240, work->bottomY >> 8, gPremireChanceBarFrames[1], work->tiles5, work->palette, NULL, 0, 60);
            DrawSprite(120 + k * 240, work->bottomY >> 8, gPremireChanceBarFrames[1], work->tiles5, work->palette, NULL, 0, 60);
        }
    }
#endif
    TaskPoolDraw(&work->tasks);
    TaskPoolDraw(&gCardListWork->effectTasks);
#ifdef PLATFORM_PC
    PortScreenEnd();
#endif
}

void Premire_Chance_3(PremireChanceWork* work) {
    TaskPoolDestroy(&gCardListWork->effectTasks);
    EwramFree(work->slots);
    EwramFree(gCardListWork);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles5);
    FadeSetPaletteExcluded(work->palette->index + 16, FALSE);
    FadeSetPaletteExcluded(work->palette3->index + 16, FALSE);
    FadeSetPaletteExcluded(work->palette2->index + 16, FALSE);
    FadeSetPaletteExcluded(work->palette4->index + 16, FALSE);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette3);
    ReleaseObjPalette(work->palette4);
    ReleaseObjPalette(work->palette);
    SetJiminyFlag(JIMINY_RECORD_PREMIUM_CARDS);
    TaskPoolDestroy(&work->tasks);
}

void CreatePremireChanceCardTasks(PremireChanceWork* work) {
    u8 i;

    for (i = 0; i < work->cardCount; i++) {
        work->slots[i].unk_06 = i;
        TaskCreate(&work->tasks, &gTaskDescPremireChanceCard, &work->slots[i]);
    }
}

u8 UpdatePremireChanceStop(PremireChanceWork* work, void* task) {
    PremireChanceCardWork* card;
    TaskPool* pool;
    u8 z;
    u8 stopTimer;
    u8* cursorHidden;

    card = ListPoolFirst(&gCardListWork->cards);
    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
    work->spinDelay = 0;
    work->stopTimer++;
    work->inputEnabled = FALSE;

    if (work->resultPending) {
        while (card != NULL) {
            if (card->steps == 0) {
                if (card->position == 3) {
                    card->state = PREMIRE_CHANCE_CARD_STATE_TO_CENTER;
                    ConvertActiveDeckCardToPremium(card->deckIndex);
                } else {
                    card->state = PREMIRE_CHANCE_CARD_STATE_MOVE_AWAY;
                }

                work->resultPending = FALSE;
            }

            card = ListPoolNext(&card->node);
        }
    }

    stopTimer = work->stopTimer;
    pool = &work->tasks;

    if (stopTimer == 30) {
        cursorHidden = &work->cursorHidden;
        z = 0;
        *cursorHidden = 1;
        SetBgPriority(2, 0);
        BgAnimInit(2, BGCNT_AFF512x512, BGCNT_256COLOR);
        BgAnimStart(&gBgAnimDefPremireChance, 120, 60);
        work->bgAnimDuration = BgAnimGetDuration(&gBgAnimDefPremireChance);
        work->resultTimer = z;
        gBldCnt = (BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
        gBldAlpha = BLDALPHA_BLEND(16, 16);
        BgAnimUpdate();
        FadeSetPaletteExcluded(10, TRUE);
        FadeSetPaletteExcluded(11, TRUE);
        FadeSetPaletteExcluded(12, TRUE);
        FadeSetPaletteExcluded(13, TRUE);
        FadeSetPaletteExcluded(14, TRUE);
        FadeSetPaletteExcluded(15, TRUE);
        TaskCreate(pool, &gTaskDescCardName, NULL);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceResult);
    }

    TaskPoolUpdate(pool);
    TaskPoolUpdate(&gCardListWork->effectTasks);
    return 1;
}

u8 UpdatePremireChanceResult(PremireChanceWork* work, void* task) {
    ListPoolFirst(&gCardListWork->cards);
    BgAnimUpdate();
    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
    TaskPoolUpdate(&work->tasks);
    TaskPoolUpdate(&gCardListWork->effectTasks);

    if ((GetKeysPressed() & A_BUTTON)
#ifdef VERSION_EU
        && work->resultTimer > 8
#endif
    ) {
        work->titleSteps = 16;
        work->slideSteps = 16;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePremireChanceClose);
    }

#ifdef VERSION_EU
    if (work->resultTimer <= 254) {
        work->resultTimer++;
    }
#endif

    return 1;
}

u8 UpdatePremireChanceClose(PremireChanceWork* work, void* task) {
    s32 titleX;
    u8* steps;

    steps = &work->titleSteps;

    if (*steps != 0) {
        titleX = work->titleX << 8;
        ApproachValue(&titleX, -0x8000, *steps);
        work->titleX = titleX >> 8;
    } else {
        steps = &work->slideSteps;

        if (*steps == 0) {
            return 0;
        }

        ApproachValue(&work->topY, -0x800, *steps);
        ApproachValue(&work->bottomY, 0xA000, *steps);
    }

    (*steps)--;
    TaskPoolUpdate(&work->tasks);
    TaskPoolUpdate(&gCardListWork->effectTasks);
    return 1;
}

TaskDesc gTaskDescFriendCard = {
    "Friend card",
    (TaskInitFunc)Friend_card_0,
    (TaskUpdateFunc)Friend_card_1,
    (TaskDrawFunc)PickupCardDraw,
    (TaskDestroyFunc)PickupCardDestroy,
    sizeof(PickupCardWork),
};

TaskDesc gTaskDescHeartlessCard = {
    "Heartless card",
    (TaskInitFunc)Heartless_card_0,
    (TaskUpdateFunc)Heartless_card_1,
    (TaskDrawFunc)Heartless_card_2,
    (TaskDestroyFunc)Heartless_card_3,
    sizeof(PickupCardWork),
};

TaskDesc gTaskDescGimmickCard = {
    "Gimmick card",
    (TaskInitFunc)Gimmick_card_0,
    (TaskUpdateFunc)Gimmick_card_1,
    (TaskDrawFunc)PickupCardDraw,
    (TaskDestroyFunc)PickupCardDestroy,
    sizeof(PickupCardWork),
};

TaskDesc gTaskDescStockNameSora = {
    "StockName",
    (TaskInitFunc)StockNameSora_0,
    (TaskUpdateFunc)StockNameSora_1,
    (TaskDrawFunc)StockNameSora_2,
    (TaskDestroyFunc)StockNameSora_3,
    sizeof(StockNameWork),
};

TaskDesc gTaskDescStockNameRiku = {
    "StockName",
    (TaskInitFunc)StockNameRiku_0,
    (TaskUpdateFunc)StockNameRiku_1,
    (TaskDrawFunc)StockNameRiku_2,
    (TaskDestroyFunc)StockNameRiku_3,
    sizeof(StockNameWork),
};

#ifdef VERSION_EU
void** gPremireChanceTitles[5] = { gPremireChanceTitleFrames, gPremireChanceTitleFrenchFrames, gPremireChanceTitleGermanFrames, gPremireChanceTitleItalianFrames, gPremireChanceTitleSpanishFrames };
#endif

TaskDesc gTaskDescPremireChance = {
    "Premire Chance",
    (TaskInitFunc)Premire_Chance_0,
    (TaskUpdateFunc)Premire_Chance_1,
    (TaskDrawFunc)Premire_Chance_2,
    (TaskDestroyFunc)Premire_Chance_3,
    sizeof(PremireChanceWork),
};
