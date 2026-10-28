/**
 * card_enemy.c
 * Enemy Battle Cards
 */

#include "registration_data.h"
#include "card_battle.h"
#include "m4a_song.h"
#include "game_state.h"
#include "obj_api.h"
#include "engine_math.h"
#include "listpool.h"
#include "obj.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_card_pictures.h"
#include "songs.h"
#include "battle_work.h"
#include "boss_card_data.h"
#include "card_def_data.h"
#include "card_types.h"
#include "types.h"
#include <stddef.h>
#include "card_enemy.h"
#include "battle.h"
#include "card_label_data.h"
#include "map_types.h"
#include "card_ids.h"
#ifdef PLATFORM_PC
#include "engine.h"
#endif

static s16 sBossCardValue;


static const s32 sEnemyCardLayout[10] = {
    0x11000, 0xBC00, 0xDC00, 0x5800, 0xDC00, 0x4400, 0xDC00, 0x3000, 0x10400, 0xB800,
};

void LookupEnemyCardDef(CardDisplayArgs* args, const CardDef** out, u8 index) {
    CardSlot* slot;
    s32 variant;
    s32 id;

    slot = args->slot;
    variant = args->variant;

    if (variant != -1) {
        ((CardDisplayWork*)((u8*)out - offsetof(CardDisplayWork, cardDef)))->enemyKind = variant;
    }

    if (slot != NULL) {
        id = slot[index].cardId;

        if (id != CARD_ID_NONE) {
            *out = &gCardDefs[id];
        }
    }
}

void LinkEnemyCardDisplay(CardDisplayWork* work) {
    ListNodeInit(&work->node, work->args.pool, work);
    ListPoolAppend(&work->node, work->args.pool);
}

void card_enemy_0(CardDisplayWork* work, CardDisplayArgs* args) {
    work->tiles = NULL;
    work->tiles2 = NULL;
    work->tiles3 = NULL;
    work->palette = NULL;
    work->command = CARD_DISP_COMMAND_NONE;
    work->args = *args;
    work->flags = 0;
    work->priority = 0x50;
    work->timer = 0;
    LookupEnemyCardDef(&work->args, &work->cardDef, work->args.index);
    work->scaleX = Q_8_8(1);
    work->scaleY = Q_8_8(1);
    work->bobAngle = GetRandom();
    work->angle = 0;
    work->ringRadius = 0;
    work->ringRadiusTarget = 0x2400;
    work->ringCenterX = sEnemyCardLayout[0];
    work->ringCenterY = sEnemyCardLayout[1];
    work->x = 0xDC00;
    work->y = 0x8400;
    work->value = work->cardDef->value;
    LinkEnemyCardDisplay(work);
}

u8 card_enemy_1(CardDisplayWork* work, void* task) {
    if (!(work->flags & CARD_DISP_FLAG_VISIBLE)) {
        if (work->flags & CARD_DISP_FLAG_GFX_LOADED) {
            ReleaseCardDisplayGfx(work);
            work->flags &= ~CARD_DISP_FLAG_GFX_LOADED;
            work->flags |= CARD_DISP_FLAG_FACE_DOWN;
        }
    }

    UpdateCardDisplayFlip(work);

    if (work->flags & CARD_DISP_FLAG_DEALING) {
        work->timer = 8;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardDeal);
    } else if (!(work->flags & CARD_DISP_FLAG_FROZEN)) {
        UpdateEnemyCardRingPosition(work);
        work->bobAngle += 4;
        DispatchEnemyCardCommand(work, task);

        if (!(work->flags & CARD_DISP_FLAG_OPEN)) {
            work->flags &= ~CARD_DISP_FLAG_SETTLED;
            SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardClosed);
        }
    }

    return 1;
}

void EnemyCardDraw(CardDisplayWork* work) {
    void* gfx;
    ObjAffine* affine;
    u16 flags;

    gfx = work->cardDef->gfx;

#ifdef PLATFORM_PC
    /* HUD-sized like Sora's cards, on the bottom row, over its enemy */
    PortHudBegin(2, 1);
#endif
    if (work->flags & CARD_DISP_FLAG_VISIBLE) {
        if (!(work->flags & CARD_DISP_FLAG_FACE_DOWN)) {
            if (work->flags & CARD_DISP_FLAG_GFX_LOADED) {
                if ((work->flags & CARD_DISP_FLAG_DOUBLE_SIZE) == 0) {
                    affine = AllocObjAffine(work->angle, work->scaleX, work->scaleY, FALSE);
                } else {
                    affine = AllocObjAffine(work->angle, work->scaleX, work->scaleY, TRUE);
                }

                flags = SPRITE_PRIORITY(1) | SPRITE_FLAG_NO_MOSAIC;
                DrawSprite(work->x >> 8, (work->y >> 8) + (gSineTable[work->bobAngle] >> 8),
                           gEnemyCardBacks[0].gfx, gCardBattleState->tiles[work->cardDef->category],
                           gCardBattleState->palette, affine, flags, work->priority - 1);
                DrawSprite(work->x >> 8, (work->y >> 8) + (gSineTable[work->bobAngle] >> 8),
                           gfx, work->tiles, work->palette, affine, flags, work->priority);

                if (work->valueModified) {
                    DrawSprite(work->x >> 8, (work->y >> 8) + (gSineTable[work->bobAngle] >> 8),
                               gCardValueDigitFrames[work->value], gCardBattleState->tiles7,
                               gCardBattleState->palette2, affine, flags, work->priority - 2);
                } else {
                    DrawSprite(work->x >> 8, (work->y >> 8) + (gSineTable[work->bobAngle] >> 8),
                               gCardValueDigitFrames[work->value], gCardBattleState->tiles5,
                               gCardBattleState->palette, affine, flags, work->priority - 2);
                }
            }
        }
    }
#ifdef PLATFORM_PC
    PortHudEnd();
#endif
}

void EnemyCardDestroy(CardDisplayWork* work) {
    if (work->tiles != NULL) {
        ReleaseCardDisplayGfx(work);
    }

    if (work->palette2 != NULL) {
        ReleaseObjPalette(work->palette2);
    }
}

u8 EnemyCardWaitPlayEnd(CardDisplayWork* work, void* task) {
    if (gBtlWork->flags & BTL_FLAG_CARD_PLAY_ENDED) {
        work->timer = 8;
        work->spinSpeed = 8;
        gCardBattleState->activeCardCount = 0;
        gCardBattleState->activeValue = 0;
        gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_ENDED;
        gBtlWork->flags &= ~BTL_FLAG_CARD_ACTIVE;
        gBtlWork->flags &= ~BTL_FLAG_OPPONENT_CARD_BUSY;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardShrinkAway);
    } else if (work->flags & CARD_DISP_FLAG_BROKEN) {
        work->priority -= 4;
        work->ringRadius = 0x500;
        work->timer = 0x100;
        work->ringAngle = (u16)(GetRandom() % 33) - 16;
        work->spinSpeed = GetRandom() % 5 + 254;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardBreakFall);
    }

    return 1;
}

u8 EnemyUsecard_1(CardDisplayWork* work, void* task) {
    work->priority = 80;
    ApproachValue(&work->x, 0x7800, work->timer);
    ApproachValue(&work->y, 0x8400, work->timer);

    if ((s16)work->timer > 0) {
        work->timer--;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_ACTIVE) {
        if (work->flags & CARD_DISP_FLAG_IN_PLAY) {
            if ((s16)work->timer == 0) {
                SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardWaitPlayEnd);
            }
        } else if ((s16)work->timer <= 2) {
            work->priority -= 4;
            work->ringRadius = 0x500;
            work->timer = 0x100;
            work->ringAngle = (u16)(GetRandom() % 33) - 16;
            work->spinSpeed = GetRandom() % 5 + 254;
            SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardFlyOff);
            return 1;
        }
    } else if ((s16)work->timer <= 2) {
        work->priority -= 4;
        work->ringRadius = 0x500;
        work->timer = 0x100;
        work->ringAngle = (u16)(GetRandom() % 33) - 16;
        work->spinSpeed = GetRandom() % 5 + 254;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardFlyOff);
    }

    return 1;
}

u8 EnemyCardDeal(CardDisplayWork* work, void* task) {
    ApproachValue(&work->x, SIN((work->ringAngle >> 8) - 32) * (work->ringRadius >> 8) + sEnemyCardLayout[0],
                  work->timer);
    ApproachValue(&work->y, -COS((work->ringAngle >> 8) - 32) * (work->ringRadius >> 8) + sEnemyCardLayout[1],
                  work->timer);
    work->timer--;

    if ((s16)work->timer <= 1) {
        work->timer = 0;
        work->flags &= ~CARD_DISP_FLAG_DEALING;
        SetTaskUpdate(task, (TaskUpdateFunc)card_enemy_1);
    }

    return 1;
}

u8 EnemyCardClosed(CardDisplayWork* work, void* task) {
    if (work->command == CARD_DISP_COMMAND_REMOVE) {
        return 0;
    }

    work->ringRadius += -work->ringRadius >> 1;
    work->x += (sEnemyCardLayout[8] - work->x) >> 1;
    work->y += (sEnemyCardLayout[9] - work->y) >> 1;

    if (work->flags & CARD_DISP_FLAG_OPEN) {
        SetTaskUpdate(task, (TaskUpdateFunc)card_enemy_1);
    }

    return 1;
}

void UpdateEnemyCardRingPosition(CardDisplayWork* work) {
    s32 wrappedAngle;

    if (work->ringAngleTarget - work->ringAngle > 0x7F00) {
        work->ringAngle += 0x10000;
    }

    wrappedAngle = work->ringAngle - 0x10000;

    if (work->ringAngleTarget - wrappedAngle < work->ringAngle - work->ringAngleTarget) {
        work->ringAngle = wrappedAngle;
    }

    work->swingAngle += (work->swingAngleTarget - work->swingAngle) >> 2;
    work->ringRadius += (work->ringRadiusTarget - work->ringRadius) >> 1;
    ApproachValue(&work->ringAngle, work->ringAngleTarget, work->timer);
    work->timer--;

    if ((s16)work->timer <= 1) {
        work->timer = 0;
        work->flags |= CARD_DISP_FLAG_SETTLED;
    } else {
        work->flags &= ~CARD_DISP_FLAG_SETTLED;
    }

    work->x = SIN((work->ringAngle >> 8) - 32) * (work->ringRadius >> 8) + work->ringCenterX;
    work->y = -COS((work->ringAngle >> 8) - 32) * (work->ringRadius >> 8) + work->ringCenterY;
}

u8 EnemyCardShrinkAway(CardDisplayWork* work) {
    ApproachValue(&work->y, 0x8200, work->timer);

    if ((s16)work->timer > 0) {
        work->timer--;
    } else {
        work->timer = 0;
    }

    if ((s16)work->timer == 0) {
        work->timer = 0;
        work->angle += work->spinSpeed;
        work->spinSpeed++;

        if (work->scaleX <= Q_8_8(0.1)) {
            return 0;
        }

        work->scaleX -= Q_8_8(0.1);
        work->scaleY -= Q_8_8(0.1);
    }

    return 1;
}

u8 EnemyCardFlyOff(CardDisplayWork* work) {
    work->command = CARD_DISP_COMMAND_NONE;
    work->y -= work->ringRadius;
    work->ringRadius -= (s16)work->timer;
    work->timer++;
    work->x -= COS(work->ringAngle);
    work->angle += work->spinSpeed;
    work->scaleX -= 5;
    work->scaleY -= 5;

    if (IsCardDisplayOffScreen(work)) {
        work->flags &= ~CARD_DISP_FLAG_VISIBLE;
        ReleaseCardDisplayGfx(work);
        work->flags &= ~CARD_DISP_FLAG_GFX_LOADED;
        gBtlWork->flags &= ~BTL_FLAG_OPPONENT_CARD_BUSY;
        return 0;
    }

    return 1;
}

void EnemyCardSlideBack(CardDisplayWork* work, void* task) {
    work->x -= gSineTable[work->spinSpeed] * 3;
    UpdateCardDisplayFlip(work);

    if (work->spinSpeed != 0) {
        work->spinSpeed -= 8;
    } else {
        work->spinSpeed = 0;
        work->flags &= ~CARD_DISP_FLAG_VISIBLE;
        SetTaskUpdate(task, (TaskUpdateFunc)card_enemy_1);
    }

    if (!(work->flags & CARD_DISP_FLAG_OPEN)) {
        work->flags &= ~CARD_DISP_FLAG_SETTLED;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardClosed);
    }
}

void EnemyCardSlideOut(CardDisplayWork* work, void* task) {
    work->x += gSineTable[work->spinSpeed] * 3;
    UpdateCardDisplayFlip(work);

    if ((s8)work->spinSpeed >= 0) {
        work->spinSpeed += 8;
    } else {
        work->spinSpeed = 0x80;
        work->flags &= ~CARD_DISP_FLAG_SELECTED;
        work->priority = 100;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardSlideBack);
    }

    if (!(work->flags & CARD_DISP_FLAG_OPEN)) {
        work->flags &= ~CARD_DISP_FLAG_SETTLED;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardClosed);
    }
}

void DispatchEnemyCardCommand(CardDisplayWork* work, void* task) {
    switch (work->command) {
    case CARD_DISP_COMMAND_PLAY:
        work->timer = 16;
        work->priority -= 4;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyUsecard_1);
        break;
    case CARD_DISP_COMMAND_STOCK:
        work->timer = 8;
        work->priority -= 4;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyStockMoveToSlot);
        break;
    case CARD_DISP_COMMAND_FLY_OFF:
        work->priority -= 4;
        work->ringRadius = 0x500;
        work->timer = 0x100;
        work->ringAngle = (u16)(GetRandom() % 33) - 16;
        work->spinSpeed = GetRandom() % 5 + 254;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardFlyOff);
        break;
    case CARD_DISP_COMMAND_REMOVE:
        work->ringRadius = 0x500;
        work->timer = 0x100;
        work->ringAngle = (u16)(GetRandom() % 33) - 16;
        work->spinSpeed = GetRandom() % 5 + 254;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardFlyOff);
        break;
    case CARD_DISP_COMMAND_SLIDE_OUT:
        work->spinSpeed = 0;
        work->priority -= 4;
        SetTaskUpdate(task, (TaskUpdateFunc)EnemyCardSlideOut);
        work->command = CARD_DISP_COMMAND_NONE;
        break;
    }
}

u8 EnemyStockMoveToSlot(CardDisplayWork* work, void* task) {
    s32 (*layout)[2]; s32* px;

    if (gBtlWork->paused == TRUE) {
        return 1;
    }

    UpdateCardDisplayFlip(work);

    if (work->flags & CARD_DISP_FLAG_OPEN) {
        px = &work->x; layout = (s32 (*)[2])sEnemyCardLayout; ApproachValue(px, layout[3 - work->stockIndex][0], work->timer); ApproachValue(&work->y, ((s32 (*)[2])sEnemyCardLayout)[3 - work->stockIndex][1], work->timer);
    } else {
        ApproachValue(&work->x, sEnemyCardLayout[8], work->timer);
        ApproachValue(&work->y, sEnemyCardLayout[9], work->timer);
    }

    if ((s16)work->timer > 0) {
        work->timer--;
        work->flags &= ~CARD_DISP_FLAG_SETTLED;
    } else {
        work->timer = 0;
        work->flags |= CARD_DISP_FLAG_SETTLED;
    }

    if (work->command == CARD_DISP_COMMAND_PLAY) {
        if (!(gBtlWork->flags & BTL_FLAG_CARD_ACTIVE) && work->stockIndex == 0) {
            gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
        }

        if (work->flags & CARD_DISP_FLAG_UNOPPOSED) {
            SetTaskUpdate(task, (TaskUpdateFunc)SoraStockStartUnopposedPlay);
        } else {
            work->timer = 15;
            work->ringRadiusTarget = 0x800;
            work->ringRadius = 0;
            work->ringAngleTarget = gPlayedCardAngles[work->stockIndex] * 2;
            work->ringAngle = 0;
            work->ringCenterX = work->x;
            work->ringCenterY = work->y;
            SetTaskUpdate(task, (TaskUpdateFunc)SoraStockMoveToPlay);
        }
    }

    work->bobAngle += 4;
    return 1;
}

u8 EnemyCardBreakFall(CardDisplayWork* work, void* task) {
    work->command = CARD_DISP_COMMAND_NONE;
    work->y -= work->ringRadius;
    work->ringRadius -= (s16)work->timer >> 1;
    work->timer++;
    work->x += 0x200;
    work->angle += 16;

    if (!(work->flags & CARD_DISP_FLAG_SPIN_MIRRORED)) {
        work->scaleX -= 20;

        if (work->scaleX >= -2 && work->scaleX <= 2) {
            work->scaleX = -20;
        }

        if (work->scaleX <= Q_8_8(-1)) {
            work->scaleX = Q_8_8(-1);
            work->flags |= CARD_DISP_FLAG_SPIN_MIRRORED;
        }
    } else {
        work->scaleX -= 20;

        if (work->scaleX >= -2 && work->scaleX <= 2) {
            work->scaleX = 20;
        }

        if (work->scaleX >= Q_8_8(1)) {
            work->scaleX = Q_8_8(1);
            work->flags &= ~CARD_DISP_FLAG_SPIN_MIRRORED;
        }
    }

    if (IsCardDisplayOffScreen(work)) {
        work->flags &= ~CARD_DISP_FLAG_VISIBLE;
        ReleaseCardDisplayGfx(work);
        work->flags &= ~CARD_DISP_FLAG_GFX_LOADED;
        gBtlWork->flags &= ~BTL_FLAG_OPPONENT_CARD_BUSY;
        return 0;
    }

    return 1;
}

void EnemyUsecard_0(CardDisplayWork* work, CardDisplayArgs* args) {
    const s32* cardIds;
    u8 n;
    s32 id;

    work->tiles = NULL;
    work->tiles2 = NULL;
    work->tiles3 = NULL;
    work->tiles4 = NULL;
    work->tiles5 = NULL;
    work->palette2 = NULL;
    work->palette = NULL;
    work->children = NULL;
    work->command = CARD_DISP_COMMAND_NONE;
    work->args = *args;
    work->flags = 0;
    work->priority = 50;
    work->timer = 0;
    cardIds = gEnemyCardIds[work->args.variant];
    n = gEnemyCardCounts[work->args.variant];
    work->enemyKind = work->args.variant;

    if (n == 1) {
        id = cardIds[0];
    } else if ((s16)work->args.index != -1) {
        if ((s16)work->args.index > n) {
            id = cardIds[GetRandom() % n];
        } else {
            id = cardIds[(s16)work->args.index - 1];
        }
    } else {
        if (gCardBattleState->nextEnemyCardIndex > n) {
            gCardBattleState->nextEnemyCardIndex = n;
        }

        id = cardIds[gCardBattleState->nextEnemyCardIndex];
        gCardBattleState->nextEnemyCardIndex = GetRandom() % n;
    }

    work->cardDef = &gCardDefs[id];
    work->scaleX = Q_8_8(1);
    work->scaleY = Q_8_8(1);
    work->bobAngle = GetRandom();
    work->angle = 0;
    work->ringRadius = 0;
    work->ringRadiusTarget = 0x2400;
    work->flags |= (CARD_DISP_FLAG_SELECTED | CARD_DISP_FLAG_VISIBLE);
    work->ringCenterX = 0xDC00;
    work->ringCenterY = 0x8800;
    work->x = 0xDC00;
    work->y = 0x8800;
    work->timer = 10;
    work->priority -= 4;
    LoadCardDisplayGfx(work);
    work->value = work->cardDef->value;

    switch (gGameState.roomEffect) {
    case ROOM_EFFECT_ALMIGHTY_DARKNESS:
        work->value += 2;

        if (work->value > 9) {
            work->value = 9;
        }

        work->valueModified = TRUE;
        break;
    case ROOM_EFFECT_FEEBLE_DARKNESS:
        if (work->value > 2) {
            work->value -= 2;
        } else {
            work->value = 1;
        }

        work->valueModified = TRUE;
        break;
    default:
        work->valueModified = FALSE;
        break;
    }

    work->flags |= CARD_DISP_FLAG_GFX_LOADED;
}

void EnemyUsecardByIndexInit(CardDisplayWork* work, CardDisplayArgs* args) {
    const s32* cardIds;
    u8 n;
    s32 id;

    work->tiles = NULL;
    work->tiles2 = NULL;
    work->tiles3 = NULL;
    work->palette = NULL;
    work->command = CARD_DISP_COMMAND_NONE;
    work->args = *args;
    work->flags = 0;
    work->priority = 50;
    work->timer = 0;
    cardIds = gEnemyCardIds[work->args.variant];
    n = gEnemyCardCounts[work->args.variant];
    work->enemyKind = work->args.variant;

    if (n == 1) {
        id = cardIds[0];
    } else if ((s16)work->args.index < n) {
        id = cardIds[(s16)work->args.index];
    } else {
        id = cardIds[GetRandom() % n];
    }

    work->cardDef = &gCardDefs[id];
    work->scaleX = Q_8_8(1);
    work->scaleY = Q_8_8(1);
    work->bobAngle = GetRandom();
    work->angle = 0;
    work->ringRadius = 0;
    work->ringRadiusTarget = 0x2400;
    work->flags |= (CARD_DISP_FLAG_FACE_DOWN | CARD_DISP_FLAG_VISIBLE);
    work->ringCenterX = 0x10000;
    work->ringCenterY = 0x8800;
    work->x = 0x10000;
    work->y = 0x8800;
    work->timer = 0x10;
    work->priority -= 4;
    work->value = work->cardDef->value;
}

void EnemyUsecardRandomInit(CardDisplayWork* work, CardDisplayArgs* args) {
    const s32* cardIds;
    u8 n;
    s32 id;

    work->tiles = NULL;
    work->tiles2 = NULL;
    work->tiles3 = NULL;
    work->palette = NULL;
    work->command = CARD_DISP_COMMAND_NONE;
    work->args = *args;
    work->flags = 0;
    work->priority = 50;
    work->timer = 0;
    cardIds = gEnemyCardIds[work->args.variant];
    n = gEnemyCardCounts[work->args.variant];
    work->enemyKind = work->args.variant;

    if (n == 1) {
        id = cardIds[0];
    } else if ((s16)work->args.index < n) {
        /* Index zero selects the first card on GBA, while still advancing RNG. */
        id = cardIds[MOD0(GetRandom(), (s16)work->args.index)];
    } else {
        id = cardIds[GetRandom() % n];
    }

    work->cardDef = &gCardDefs[id];
    work->scaleX = Q_8_8(1);
    work->scaleY = Q_8_8(1);
    work->bobAngle = GetRandom();
    work->angle = 0;
    work->ringRadius = 0;
    work->ringRadiusTarget = 0x2400;
    work->flags |= (CARD_DISP_FLAG_FACE_DOWN | CARD_DISP_FLAG_VISIBLE);
    work->ringCenterX = 0x10000;
    work->ringCenterY = 0x8800;
    work->x = 0x10000;
    work->y = 0x8800;
    work->timer = 0x10;
    work->priority -= 4;
    work->value = work->cardDef->value;
}

void UseEnemyCard(u16 arg) {
    CardDisplayArgs args;
    CardDisplayWork* card;
    u8 i;
    u8 flag;
    u8 found;
#ifdef VERSION_EU
    s32 j;
    s32 k;
#endif

    args.pool = NULL;
    args.slot = NULL;
    args.variant = arg;
    args.index = sBossCardValue;
    args.listIndex = CARD_LIST_MAIN;
    card = TaskCreate(&gCardBattleState->tasks, &gTaskDescEnemyUsecard, &args)->work;
    gBtlWork->flags |= BTL_FLAG_OPPONENT_CARD_BUSY;
    gCardBattleState->enemyCardUsed = TRUE;

    if ((gBtlWork->flags & BTL_FLAG_CARD_ACTIVE) == 0) {
        card->flags |= CARD_DISP_FLAG_IN_PLAY;
        gCardBattleState->activeCards[0] = card;
        gCardBattleState->activeValue = card->value;
        gCardBattleState->activeCardCount = 1;
        gBtlWork->soraOwnsPlay = FALSE;
        gBtlWork->flags |= BTL_FLAG_CARD_PLAY_START;
        gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
    } else if ((gBtlWork->flags & BTL_FLAG_CARD_PLAY_ENDED) == 0) {
#ifdef VERSION_EU
        if (gCardBattleState->activeValue <= card->value || card->value == 0) {
#else
        if (gCardBattleState->activeValue <= card->value) {
#endif
            found = FALSE;

            if (gBtlWork->hcEffect == HC_EFFECT_ATTACK_BRACER) {
#ifdef VERSION_EU
                if (gCardBattleState->activeCards[0]->cardDef->category == CARD_CATEGORY_ATTACK && !gCardBattleState->soraStockActive) {
                    found = TRUE;
                }
#else
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    if (gCardBattleState->activeCards[i]->cardDef->category == CARD_CATEGORY_ATTACK) {
                        found = TRUE;
                        break;
                    }
                }
#endif
            }

            if (gBtlWork->hcEffect == HC_EFFECT_LEAF_BRACER) {
#ifdef VERSION_EU
                for (j = 0; j < gCardBattleState->activeCardCount; j++) {
                    if (gCardBattleState->activeCards[j]->cardDef->move == MOVE_CURE) {
                        found = TRUE;
                    }
                }
#else
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    if (gCardBattleState->activeCards[i]->cardDef->move == MOVE_CURE) {
                        found = TRUE;
                        break;
                    }
                }
#endif
            }

            if (gBtlWork->hcEffect == HC_EFFECT_ITEM_BRACER) {
#ifdef VERSION_EU
                for (k = 0; k < gCardBattleState->activeCardCount; k++) {
                    if (gCardBattleState->activeCards[k]->cardDef->category == CARD_CATEGORY_ITEM && !(gCardBattleState->activeCards[k]->cardDef->flags & CARD_DEF_FLAG_FRIEND)) {
                        found = TRUE;
                    }
                }
#else
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    if (gCardBattleState->activeCards[i]->cardDef->category == CARD_CATEGORY_ITEM) {
                        found = TRUE;
                        break;
                    }
                }
#endif
            }

            if (!found) {
                gBtlWork->flags |= BTL_FLAG_CARD_BREAK;

                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    gCardBattleState->activeCards[i]->flags |= CARD_DISP_FLAG_BROKEN;
                }

                if (gCardBattleState->activeValue != card->value) {
                    if (card->value == 0) {
                        gBtlWork->breakDifference = -(s8)gCardBattleState->activeValue;
                    } else {
                        gBtlWork->breakDifference = gCardBattleState->activeValue - card->value;
                    }

                    m4aSongNumStart(SONG_SYS_CARDLOSE);
                    gBtlWork->flags |= BTL_FLAG_CARD_PLAY_START;
                    gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
                    gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_ENDED;
                    gCardBattleState->activeCards[0] = card;

                    if (gBtlWork->hcEffect == HC_EFFECT_VALUE_BREAK) {
                        if (card->value != 0) {
                            gCardBattleState->activeValue = card->value - gCardBattleState->activeValue;
                            card->value = gCardBattleState->activeValue;
                        } else {
                            gCardBattleState->activeValue = 0;
                        }

                        if (gCardBattleState->activeValue < 0) {
                            gCardBattleState->activeValue = 0;
                        }

                        gBtlWork->hcEffectCount--;
                    } else {
                        gCardBattleState->activeValue = card->value;
                    }

                    gCardBattleState->activeCardCount = 1;
                    gBtlWork->soraOwnsPlay = FALSE;
                    card->flags |= CARD_DISP_FLAG_IN_PLAY;
                    AddBreakDarkPoints();
                } else {
                    m4aSongNumStart(SONG_SYS_DROW);
                    gBtlWork->flags &= ~BTL_FLAG_CARD_ACTIVE;
                    gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_ENDED;
                    gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_START;
                    gBtlWork->soraOwnsPlay = FALSE;
                }
            }
        }
    } else {
#ifdef VERSION_EU
        if (gCardBattleState->activeValue <= card->value || card->value == 0) {
#else
        if (gCardBattleState->activeValue <= card->value) {
#endif
            flag = FALSE;

            if (gBtlWork->hcEffect == HC_EFFECT_ATTACK_BRACER) {
#ifdef VERSION_EU
                if (gCardBattleState->activeCards[0]->cardDef->category == CARD_CATEGORY_ATTACK && !gCardBattleState->soraStockActive) {
                    flag = TRUE;
                }
#else
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    if (gCardBattleState->activeCards[i]->cardDef->category == CARD_CATEGORY_ATTACK) {
                        flag = TRUE;
                        break;
                    }
                }
#endif
            }

#ifndef VERSION_EU
            if (gBtlWork->hcEffect == HC_EFFECT_INCREMENTOR_2) {
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    if (gCardBattleState->activeCards[i]->cardDef->category == CARD_CATEGORY_MAGIC) {
                        flag = TRUE;
                        break;
                    }
                }
            }
#endif

            if (gBtlWork->hcEffect == HC_EFFECT_LEAF_BRACER) {
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    if (gCardBattleState->activeCards[i]->cardDef->move == MOVE_CURE) {
                        flag = TRUE;
                        break;
                    }
                }
            }

            if (gBtlWork->hcEffect == HC_EFFECT_ITEM_BRACER) {
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
#ifdef VERSION_EU
                    if (gCardBattleState->activeCards[i]->cardDef->category == CARD_CATEGORY_ITEM && !(gCardBattleState->activeCards[i]->cardDef->flags & CARD_DEF_FLAG_FRIEND)) {
#else
                    if (gCardBattleState->activeCards[i]->cardDef->category == CARD_CATEGORY_ITEM) {
#endif
                        flag = TRUE;
                        break;
                    }
                }
            }

            if (!flag) {
                gBtlWork->flags |= BTL_FLAG_CARD_BREAK;

                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    gCardBattleState->activeCards[i]->flags |= CARD_DISP_FLAG_BROKEN;
                }

                if (gCardBattleState->activeValue != card->value) {
                    if (card->value == 0) {
                        gBtlWork->breakDifference = -(s8)gCardBattleState->activeValue;
                    } else {
                        gBtlWork->breakDifference = gCardBattleState->activeValue - card->value;
                    }

                    m4aSongNumStart(SONG_SYS_CARDLOSE);
                    gBtlWork->flags |= BTL_FLAG_CARD_PLAY_START;
                    gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
                    gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_ENDED;
                    gCardBattleState->activeCards[0] = card;

#ifdef VERSION_EU
                    if (gBtlWork->hcEffect == HC_EFFECT_VALUE_BREAK) {
                        if (card->value != 0) {
                            gCardBattleState->activeValue = card->value - gCardBattleState->activeValue;
                            card->value = gCardBattleState->activeValue;
                        } else {
                            gCardBattleState->activeValue = 0;
                        }

                        if (gCardBattleState->activeValue < 0) {
                            gCardBattleState->activeValue = 0;
                        }

                        gBtlWork->hcEffectCount--;
                    } else {
                        gCardBattleState->activeValue = card->value;
                    }

#else
                    gCardBattleState->activeValue = card->value;
#endif
                    gCardBattleState->activeCardCount = 1;
                    gBtlWork->soraOwnsPlay = FALSE;
                    card->flags |= CARD_DISP_FLAG_IN_PLAY;
                    AddBreakDarkPoints();
                } else {
                    m4aSongNumStart(SONG_SYS_DROW);
                    gBtlWork->flags &= ~BTL_FLAG_CARD_ACTIVE;
                    gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_ENDED;
                    gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_START;
                    gBtlWork->soraOwnsPlay = FALSE;
                }
            }
        }
    }

    card->flags = (card->flags | CARD_DISP_FLAG_SELECTED) & ~CARD_DISP_FLAG_SETTLED;
}

void UseEnemyCardByIndex(u16 variant, u8 index) {
    CardDisplayArgs arg;
    CardDisplayWork* card;
    u8 i;

    arg.pool = NULL;
    arg.slot = NULL;
    arg.variant = variant;
    arg.index = index;
    arg.listIndex = CARD_LIST_MAIN;
    card = TaskCreate(&gCardBattleState->tasks, &gTaskDescEnemyUsecardByIndex, &arg)->work;
    gBtlWork->flags |= BTL_FLAG_OPPONENT_CARD_BUSY;

    if ((gBtlWork->flags & BTL_FLAG_CARD_ACTIVE) == 0) {
        card->flags |= CARD_DISP_FLAG_IN_PLAY;
        gCardBattleState->activeCards[0] = card;
        gCardBattleState->activeValue = card->cardDef->value;
        gCardBattleState->activeCardCount = 1;
        gBtlWork->soraOwnsPlay = FALSE;
        gBtlWork->flags |= BTL_FLAG_CARD_PLAY_START;
        gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
    } else if ((gBtlWork->flags & BTL_FLAG_CARD_PLAY_ENDED) == 0) {
        if (gCardBattleState->soraHcEffect != HC_EFFECT_ATTACK_BRACER) {
            if (gCardBattleState->activeValue < card->cardDef->value) {
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    gCardBattleState->activeCards[i]->flags |= CARD_DISP_FLAG_BROKEN;
                }

                m4aSongNumStart(SONG_SYS_CARDLOSE);
                gBtlWork->flags |= BTL_FLAG_CARD_BREAK;
                gBtlWork->flags |= BTL_FLAG_CARD_PLAY_START;
                gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
                gCardBattleState->activeCards[0] = card;
                gCardBattleState->activeValue = card->cardDef->value;
                gCardBattleState->activeCardCount = 1;
                gBtlWork->soraOwnsPlay = FALSE;
                card->flags |= CARD_DISP_FLAG_IN_PLAY;
            }
        }
    }

    card->flags |= CARD_DISP_FLAG_SELECTED;
    card->flags &= ~CARD_DISP_FLAG_SETTLED;
}

void UseRandomEnemyCard(u16 variant, u8 index) {
    CardDisplayArgs arg;
    CardDisplayWork* card;
    u8 i;

    arg.pool = NULL;
    arg.slot = NULL;
    arg.variant = variant;
    arg.index = index;
    arg.listIndex = CARD_LIST_MAIN;
    card = TaskCreate(&gCardBattleState->tasks, &gTaskDescEnemyUsecardRandom, &arg)->work;
    gBtlWork->flags |= BTL_FLAG_OPPONENT_CARD_BUSY;

    if ((gBtlWork->flags & BTL_FLAG_CARD_ACTIVE) == 0) {
        card->flags |= CARD_DISP_FLAG_IN_PLAY;
        gCardBattleState->activeCards[0] = card;
        gCardBattleState->activeValue = card->cardDef->value;
        gCardBattleState->activeCardCount = 1;
        gBtlWork->soraOwnsPlay = FALSE;
        gBtlWork->flags |= BTL_FLAG_CARD_PLAY_START;
        gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
    } else if (gBtlWork->flags & BTL_FLAG_CARD_PLAY_ENDED) {
        if (gBtlWork->hcEffect == HC_EFFECT_ATTACK_BRACER) {
            if (gCardBattleState->activeValue < card->cardDef->value) {
                for (i = 0; i < gCardBattleState->activeCardCount; i++) {
                    gCardBattleState->activeCards[i]->flags |= CARD_DISP_FLAG_BROKEN;
                }

                m4aSongNumStart(SONG_SYS_CARDLOSE);
                gBtlWork->flags |= BTL_FLAG_CARD_BREAK;
                gBtlWork->flags |= BTL_FLAG_CARD_PLAY_START;
                gBtlWork->flags |= BTL_FLAG_CARD_ACTIVE;
                gCardBattleState->activeCards[0] = card;
                gCardBattleState->activeValue = card->cardDef->value;
                gCardBattleState->activeCardCount = 1;
                gBtlWork->soraOwnsPlay = FALSE;
                card->flags |= CARD_DISP_FLAG_IN_PLAY;
            }
        }
    }

    card->flags |= CARD_DISP_FLAG_SELECTED;
    card->flags &= ~CARD_DISP_FLAG_SETTLED;
}

void ResetBossCardValue() {
    sBossCardValue = -1;
}

void SetBossCardValue(u16 value) {
    sBossCardValue = value;
}

u16 GetBossCardValue() {
    if (sBossCardValue != -1) {
        return sBossCardValue;
    }

    // @bug Oogie Boogie's intro polls this before the card battle state exists (NULL read).
    return BIOS_PTR(gCardBattleState)->nextEnemyCardIndex;
}

TaskDesc gTaskDescCardEnemy = {
    "card_enemy",
    (TaskInitFunc)card_enemy_0,
    (TaskUpdateFunc)card_enemy_1,
    (TaskDrawFunc)EnemyCardDraw,
    (TaskDestroyFunc)EnemyCardDestroy,
    sizeof(CardDisplayWork),
};

TaskDesc gTaskDescEnemyUsecard = {
    "EnemyUsecard",
    (TaskInitFunc)EnemyUsecard_0,
    (TaskUpdateFunc)EnemyUsecard_1,
    (TaskDrawFunc)EnemyCardDraw,
    (TaskDestroyFunc)EnemyCardDestroy,
    sizeof(CardDisplayWork),
};

TaskDesc gTaskDescEnemyUsecardByIndex = {
    "EnemyUsecard",
    (TaskInitFunc)EnemyUsecardByIndexInit,
    (TaskUpdateFunc)EnemyUsecard_1,
    (TaskDrawFunc)EnemyCardDraw,
    (TaskDestroyFunc)EnemyCardDestroy,
    sizeof(CardDisplayWork),
};

TaskDesc gTaskDescEnemyUsecardRandom = {
    "EnemyUsecard",
    (TaskInitFunc)EnemyUsecardRandomInit,
    (TaskUpdateFunc)EnemyUsecard_1,
    (TaskDrawFunc)EnemyCardDraw,
    (TaskDestroyFunc)EnemyCardDestroy,
    sizeof(CardDisplayWork),
};
