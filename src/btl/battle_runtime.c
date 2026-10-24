/**
 * battle_runtime.c
 * Battle Core Logic
 */

#include "macros.h"
#include "mode_continue.h"
#include "registration_data.h"
#include "map_api.h"
#include "msg_api.h"
#include "display.h"
#include "key.h"
#include "m4a_song.h"
#include "obj_api.h"
#include "battle.h"
#include "battle_actor.h"
#include "world_types.h"
#include "gba/keys.h"
#include "fade.h"
#include "player_progression.h"
#include <stdlib.h>
#include "enemy_tile_counts.h"
#include <string.h>
#include "anim.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "card_battle.h"
#include "engine_math.h"
#include "game_state.h"
#include "listpool.h"
#include "m4a.h"
#include "map_runtime.h"
#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "mode.h"
#include "obj.h"
#include "player_progression_types.h"
#include "save_api.h"
#include "system_state.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include "gba/macro.h"
#include <stddef.h>
#include "hum_common.h"
#include "card_deckmenu2.h"
#include "battle_ids.h"
#include "card_label_data.h"
#include "enemy_ids.h"
#include "event_ids.h"
#include "jiminy_records_index_data.h"
#include "songs.h"
#include "gba/defines.h"
#include "card.h"
#include "card_ids.h"
#include "map_types.h"

s32 gUnk_02039DC0 EWRAM_COMMON(4);
s32* gLockonDoorPosition EWRAM_COMMON(4);

void SetBattleZoom(u16 steps, s32 scale, s32 x, s32 y) {
    gBtlWork->zoomScale = scale;
    gBtlWork->zoomSteps = steps;
    gBtlWork->zoomX = x;
    gBtlWork->zoomY = y;
}

void AnimChangeWithDef(const AnimDef* defs, void* anim, u16 index, u16 flags, void* tiles) {
    const AnimDef* def = &defs[index];
    AnimChangeWithTables(anim, def->animId, flags, def->anims, def->gfxTable);
    SetObjTileSource(tiles, def->tiles);
}

void WorldToScreen(s16* outX, s16* outY, s32 px, s32 py, s32 pz) {
    s16 x;
    s16 y;
    u8 angle;
    s32 cosIndex;
    s16 idx;
    const s16* sine;
    s32 rotX;
    s32 rotY;

    if (gBtlWork->scale == Q_8_8(1)) {
        x = (px >> 8) - (gBtlWork->viewX >> 8);
        y = ((py >> 8) + (pz >> 8)) - (gBtlWork->viewY >> 8);
    } else {
        x = (((px >> 8) - (gBtlWork->viewX >> 8)) * gBtlWork->scale) >> 8;
        y = ((((py >> 8) + (pz >> 8)) - (gBtlWork->viewY >> 8)) * gBtlWork->scale) >> 8;
    }

    if (gBtlWork->rotation == 0) {
        *outX = x + DISPLAY_WIDTH / 2;
        *outY = y + DISPLAY_HEIGHT / 2;
    } else {
        angle = -gBtlWork->rotation;
        sine = gSineTable;
        cosIndex = angle + 64;
        idx = cosIndex & 255;
        rotX = sine[idx] * x;
        rotY = sine[idx += 64] * x;
        rotX += sine[angle] * y;
        rotY += sine[cosIndex] * y;
        *outX = (rotX >> 8) + DISPLAY_WIDTH / 2;
        *outY = (rotY >> 8) + DISPLAY_HEIGHT / 2;
    }
}

void CreateBtlPopTask(BtlObj* obj, s16 kind) {
    BtlPrizeSrc src;
    s16* cooldown;

    if (kind != BTL_POP_CARD_BREAK) {
        if (obj->parent != NULL) {
            cooldown = &obj->parent->popCooldown;
        } else {
            cooldown = &obj->popCooldown;
        }

        if (*cooldown > 0) {
            return;
        }

        *cooldown = 50;
    }

    src.x = obj->x;
    src.y = obj->y;
    src.z = obj->z - ((obj->height / 2) << 8);

    if (gGameState.flags & GAME_FLAG_RIKU) {
        if (kind == BTL_POP_CARD_BREAK) {
            src.kind = abs(gBtlWork->breakDifference);
            TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPopCb, &src);
            return;
        }

        src.kind = kind;
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPop, &src);
        return;
    }

    src.kind = kind;

    if (kind == BTL_POP_CARD_BREAK) {
        if (obj->flags & BTLOBJ_FLAG_NO_BREAK_POP) {
            return;
        }

        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPop, &src);
        return;
    }

    TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPop, &src);
}

void BtlWorkInit() {
    u8* dest;
    u8* src;
    CpuFill32(0, gBtlWork, sizeof(BtlWork));
    gBtlWork->phase = BTL_PHASE_START;
    gBtlWork->fadeExcludedPalettes = 0xFFFF0000;
    gBtlWork->gravity = 0x42;
    gBtlWork->fadeAmount = 10;
    dest = gBtlWork->savedProgression;
    src = (u8*)&gGameState;
    src += offsetof(GameState, progression);
    memcpy(dest, src, 0x88);
    ListPoolInit(&gBtlWork->pool);
    ListPoolInit(&gBtlWork->pool2);
}

void UpdateEnemyCardUse() {
    BtlObj* enemy;

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_BUSY) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
        return;
    }

    enemy = gBtlWork->actor4;

    if (enemy == NULL) {
        return;
    }

    if (enemy->flags & BTLOBJ_FLAGS_NO_CARD_USE) {
        return;
    }

    gBtlWork->actor3 = enemy;
    UseEnemyCard(enemy->kind);
}

void HandleSoraCardInput() {
    BtlObj* player;
    u16 timer;
    u16 chord;

    if (gBtlWork->flags & BTL_FLAG_GIMMICK_CARD_ACTIVE) {
        return;
    }

    timer = gBtlWork->listSwitchTimer;

    if ((s16)timer > 0) {
        gBtlWork->listSwitchTimer = timer - 1;

        if (gBtlWork->listSwitchTimer == 0) {
            RequestSwitchSoraCardList();
        }

        return;
    }

    if (gBtlWork->flags & BTL_FLAG_CAN_CHARGE_RELOAD) {
        if (!(gGameState.flags & GAME_FLAG_RIKU)) {
            if (GetKeysHeld() & A_BUTTON) {
                if (!(GetKeysHeld() & (L_BUTTON | R_BUTTON))) {
                    SetSoraReloadCharging();
                }
            }
        }
    }

    if (gBtlWork->flags & BTL_FLAG_RELOAD_CHARGING) {
        return;
    }

#ifdef PLATFORM_PC
    if (PortTakeUnstock()) {
        RequestSoraUnstock();
    }
#endif
    chord = ReadKeyChord(L_BUTTON, R_BUTTON);

    switch (chord) {
    case L_BUTTON:
        RequestSoraNextCard();
        break;
    case R_BUTTON:
        RequestSoraPrevCard();
        break;
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        RequestSwitchSoraCardList();
    }

    if (IsSoraReloadCardSelected()) {
        gBtlWork->lHeldFrames = 0;
        gBtlWork->rHeldFrames = 0;
    } else {
        if ((GetKeysHeld() & L_BUTTON) && !(GetKeysHeld() & R_BUTTON)) {
            if (gBtlWork->lHeldFrames < 255) {
                gBtlWork->lHeldFrames++;
            }
        } else {
            gBtlWork->lHeldFrames = 0;
        }

        if ((GetKeysHeld() & R_BUTTON) && !(GetKeysHeld() & L_BUTTON)) {
            if (gBtlWork->rHeldFrames < 255) {
                gBtlWork->rHeldFrames++;
            }
        } else {
            gBtlWork->rHeldFrames = 0;
        }
    }

    if (gBtlWork->lHeldFrames > 32) {
        RequestSoraNextCard();
    }

    if (gBtlWork->rHeldFrames > 32) {
        RequestSoraPrevCard();
    }

    player = gBtlWork->actor;

    if (player->flags & BTLOBJ_FLAG_CARD_USE_BLOCKED) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_BUSY) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
        return;
    }

    if (player->flags & BTLOBJ_FLAG_DAMAGE_PENDING) {
        return;
    }

    if (chord == (L_BUTTON | R_BUTTON)) {
        if (GetSoraStockCount() > 2) {
            RequestSoraStockUse();
        } else {
            RequestSoraCardStock();
        }
    }

    if (GetKeysPressed() & A_BUTTON) {
        RequestSoraCardUse();

        if (GetSoraCardListIndex() == CARD_LIST_ENEMY) {
            if (IsSoraSelectionEmpty() == 0) {
                gBtlWork->listSwitchTimer = 15;
            }
        }
    }
}

void HandleRikuCardInput() {
    BtlObj* riku;
    u16 timer;
    u16 chord;

    if (gRikuBtlWork->flags & BTL_FLAG_RELOAD_CHARGING) {
        return;
    }

    timer = gRikuBtlWork->listSwitchTimer;

    if ((s16)timer > 0) {
        gRikuBtlWork->listSwitchTimer = timer - 1;

        if (gRikuBtlWork->listSwitchTimer == 0) {
            RequestSwitchRikuCardList();
        }

        return;
    }

    chord = ReadKeyChord(L_BUTTON, R_BUTTON);

    switch (chord) {
    case L_BUTTON:
        RequestRikuNextCard();
        break;
    case R_BUTTON:
        RequestRikuPrevCard();
        break;
    }

    if (GetKeysPressed() & SELECT_BUTTON) {
        RequestSwitchRikuCardList();
    }

    if (IsRikuReloadCardSelected()) {
        gBtlWork->lHeldFrames = 0;
        gBtlWork->rHeldFrames = 0;
    } else {
        if ((GetKeysHeld() & L_BUTTON) && !(GetKeysHeld() & R_BUTTON)) {
            if (gBtlWork->lHeldFrames < 255) {
                gBtlWork->lHeldFrames++;
            }
        } else {
            gBtlWork->lHeldFrames = 0;
        }

        if ((GetKeysHeld() & R_BUTTON) && !(GetKeysHeld() & L_BUTTON)) {
            if (gBtlWork->rHeldFrames < 255) {
                gBtlWork->rHeldFrames++;
            }
        } else {
            gBtlWork->rHeldFrames = 0;
        }
    }

    if (gBtlWork->lHeldFrames > 32) {
        RequestRikuNextCard();
    }

    if (gBtlWork->rHeldFrames > 32) {
        RequestRikuPrevCard();
    }

    riku = gRikuBtlWork->actor;

    if (riku->flags & BTLOBJ_FLAG_CARD_USE_BLOCKED) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_BUSY) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
        return;
    }

    if (riku->flags & BTLOBJ_FLAG_DAMAGE_PENDING) {
        return;
    }

    if (chord == (L_BUTTON | R_BUTTON)) {
        if (GetRikuStockCount() > 2) {
            RequestRikuStockUse();
        } else {
            RequestRikuCardStock();
        }
    }

    if (GetKeysPressed() & A_BUTTON) {
        RequestRikuCardUse();

        if (GetRikuCardListIndex() == CARD_LIST_ENEMY) {
            if (IsRikuSelectionEmpty() == 0) {
                gRikuBtlWork->listSwitchTimer = 15;
            }
        }
    }
}

void HandleTutorialCardInput() {
    BtlObj* player;
    u16 chord;
    u16 pressed;
    u16 held;

    if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL_NO_CARD_USE)) {
        if (gBtlWork->flags & BTL_FLAG_CAN_CHARGE_RELOAD) {
            if (!(gGameState.flags & GAME_FLAG_RIKU)) {
                if (GetKeysHeld() & A_BUTTON) {
                    if (!(GetKeysHeld() & (L_BUTTON | R_BUTTON))) {
                        SetSoraReloadCharging();
                    }
                }
            }
        }
    }

    if (gBtlWork->flags & BTL_FLAG_RELOAD_CHARGING) {
        return;
    }

#ifdef PLATFORM_PC
    /* No returning stocked cards in the tutorials: Leon's waits for the
       three stocked cards to be used as a sleight. Taken so it does not
       fire later. */
    PortTakeUnstock();
#endif
    chord = ReadKeyChord(L_BUTTON, R_BUTTON);
    pressed = GetKeysPressed();
    held = GetKeysHeld();

    if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL_NO_CARD_SELECT)) {
        switch (chord) {
        case L_BUTTON:
            RequestSoraNextCard();
            break;
        case R_BUTTON:
            RequestSoraPrevCard();
            break;
        }
    }

    if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL_NO_LIST_SWITCH)) {
        if (pressed & SELECT_BUTTON) {
            RequestSwitchSoraCardList();
        }
    }

    if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL_NO_CARD_SELECT)) {
        if (IsSoraReloadCardSelected()) {
            gBtlWork->lHeldFrames = 0;
            gBtlWork->rHeldFrames = 0;
        } else {
            if ((held & L_BUTTON) && !(held & R_BUTTON)) {
                if (gBtlWork->lHeldFrames < 255) {
                    gBtlWork->lHeldFrames++;
                }
            } else {
                gBtlWork->lHeldFrames = 0;
            }

            if ((held & R_BUTTON) && !(held & L_BUTTON)) {
                if (gBtlWork->rHeldFrames < 255) {
                    gBtlWork->rHeldFrames++;
                }
            } else {
                gBtlWork->rHeldFrames = 0;
            }
        }
    }

    if (gBtlWork->lHeldFrames > 32) {
        RequestSoraNextCard();
    }

    if (gBtlWork->rHeldFrames > 32) {
        RequestSoraPrevCard();
    }

    player = gBtlWork->actor;

    if (player->flags & BTLOBJ_FLAG_CARD_USE_BLOCKED) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_BUSY) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
        return;
    }

    if (player->flags & BTLOBJ_FLAG_DAMAGE_PENDING) {
        return;
    }

    if (chord == (L_BUTTON | R_BUTTON)) {
        if (GetSoraStockCount() > 2) {
            if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL_NO_STOCK_USE)) {
                RequestSoraStockUse();
            }
        } else {
            if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL_NO_STOCK)) {
                RequestSoraCardStock();
            }
        }
    }

    if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL_NO_CARD_USE)) {
        if (pressed & A_BUTTON) {
            RequestSoraCardUse();
        }
    }
}

void MakeOpponentsHittable() {
    BtlWork* btl = gBtlWork;
    BtlObj* opponent;

    if (btl->flags & BTL_FLAG_VS_BATTLE) {
        if (btl->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            opponent = gRikuBtlWork->actor;
            opponent->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
            return;
        }
    } else if (btl->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
        opponent = ListPoolFirst(&btl->pool);

        while (opponent != NULL) {
            opponent->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
            opponent->invincibleTimer = 0;
            opponent = ListPoolNext(&opponent->node);
        }

        return;
    }

    opponent = btl->actor;
    opponent->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
}

void DropFriendCard(s32 x, s32 y, s32 z) {
    u16 flags;
    s16 friendIndex;

    flags = gGameState.progression.friendFlags;

    if (gBtlWork->flags & BTL_FLAG_TUTORIAL) {
        return;
    }

    if (gGameState.flags & GAME_FLAG_RIKU) {
        if (flags & FRIEND_FLAG_THE_KING) {
            SetJiminyFlag(JIMINY_RECORD_RIKU_CARD_KING);
            CreateFriendCardTask(gBtlWork->taskPools, x >> 8, y >> 8, z >> 8, FRIEND_CARD_THE_KING);
        }

        return;
    }

    friendIndex = -1;

    if (GetRandom() % 4 != 0) {
        switch (gGameState.world) {
        case WORLD_AGRABAH:
            if (flags & FRIEND_FLAG_ALADDIN) {
                friendIndex = FRIEND_CARD_ALADDIN;
                SetJiminyFlag(JIMINY_RECORD_FRIEND_CARD_ALADDIN);
            }

            break;
        case WORLD_ATLANTICA:
            if (flags & FRIEND_FLAG_ARIEL) {
                friendIndex = FRIEND_CARD_ARIEL;
                SetJiminyFlag(JIMINY_RECORD_FRIEND_CARD_ARIEL);
            }

            break;
        case WORLD_HALLOWEEN_TOWN:
            if (flags & FRIEND_FLAG_JACK) {
                friendIndex = FRIEND_CARD_JACK;
                SetJiminyFlag(JIMINY_RECORD_FRIEND_CARD_JACK);
            }

            break;
        case WORLD_NEVER_LAND:
            if (flags & FRIEND_FLAG_PETER_PAN) {
                friendIndex = FRIEND_CARD_PETER_PAN;
                SetJiminyFlag(JIMINY_RECORD_FRIEND_CARD_PETER_PAN);
            }

            break;
        case WORLD_HOLLOW_BASTION:
            if (flags & FRIEND_FLAG_THE_BEAST) {
                friendIndex = FRIEND_CARD_THE_BEAST;
                SetJiminyFlag(JIMINY_RECORD_FRIEND_CARD_BEAST);
            }

            break;
        }
    }

    if (friendIndex == -1) {
        if (GetRandom() % 2 != 0) {
            if (flags & FRIEND_FLAG_GOOFY) {
                friendIndex = FRIEND_CARD_GOOFY;
                SetJiminyFlag(JIMINY_RECORD_FRIEND_CARD_GOOFY);
            }
        } else {
            if (flags & FRIEND_FLAG_DONALD_DUCK) {
                friendIndex = FRIEND_CARD_DONALD_DUCK;
                SetJiminyFlag(JIMINY_RECORD_FRIEND_CARD_DONALD_DUCK);
            }
        }
    }

    if (friendIndex != -1) {
        CreateFriendCardTask(gBtlWork->taskPools, x >> 8, y >> 8, z >> 8, friendIndex);
    }
}

void EndCardPlay() {
    gBtlWork->phase = BTL_PHASE_IDLE;

    if (!(gBtlWork->flags & BTL_FLAG_CARD_BREAK)) {
        gBtlWork->flags |= BTL_FLAG_CARD_PLAY_ENDED;
    }

    gBtlWork->flags &= ~BTL_FLAG_OPPONENT_CARD_ACTION;
    gBtlWork->flags &= ~BTL_FLAG_PLAYER_CARD_ACTION;
}

enum BtlEndStep {
    BTL_END_STEP_REWARD_SCREEN = -1,
    BTL_END_STEP_CLOSE_CARDS,
    BTL_END_STEP_WAIT_FRAME,
    BTL_END_STEP_REWARDS,
    BTL_END_STEP_FADE_OUT,
    BTL_END_STEP_EXIT
};

void UpdateBattleState() {
    BtlObj* player;
    s32 i;
    s32 changed;
    s32 busy;
    s8 count;
    BtlPrizeArgs pos;

    player = gBtlWork->actor;

    if (gBtlWork->hcEffect == HC_EFFECT_FLOAT) {
        gBtlWork->gravity = 38;
    } else {
        gBtlWork->gravity = 66;
    }

    if (gBtlWork->hcEffect == HC_EFFECT_VANISH) {
        if (!(gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) && gFrameCounter % 30 == 0) {
            gBtlWork->targetX = (gBtlWork->xMin + GetRandom() % (gBtlWork->xMax - gBtlWork->xMin + 1)) * 256;
            gBtlWork->targetY = (gBtlWork->yMin + GetRandom() % (gBtlWork->yMax - gBtlWork->yMin + 1)) * 256;
            gBtlWork->targetZ = 0;
        }
    } else {
        gBtlWork->targetX = gBtlWork->actor->x;
        gBtlWork->targetY = gBtlWork->actor->y;
        gBtlWork->targetZ = gBtlWork->actor->z;
    }

    gBtlWork->flags &= ~BTL_FLAG_ENEMY_FRAME_CHANGED;

    switch ((u32)gBtlWork->phase) {
    case BTL_PHASE_IDLE:
    case BTL_PHASE_CARD_PLAY:
        if (gBtlWork->flags & BTL_FLAG_TUTORIAL) {
            HandleTutorialCardInput();
#ifdef VERSION_EU
            HandleRikuTutorialCardInput();
#else
            HandleRikuAiCardInput();
#endif
        } else if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
            HandleSoraCardInput();
            HandleRikuAiCardInput();
        } else {
            UpdateEnemyCardUse();
            HandleSoraCardInput();
        }

        break;
    }

    gBtlWork->actor4 = NULL;
    TaskPoolUpdate(&gBtlWork->taskPools[1]);

    if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
        gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
        gBtlWork->flags &= ~BTL_FLAG_STOCK_SEQUENCE;

        if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
            gRikuBtlWork->flags &= ~BTL_FLAG_STOCK_SEQUENCE;
        }

        gBtlWork->phase = BTL_PHASE_IDLE;

        if (gBtlWork->soraOwnsPlay) {
            BtlObj* obj;
            gBtlWork->flags &= ~BTL_FLAG_OPPONENT_CARD_ACTION;
            obj = gBtlWork->actor3;

            if (obj != NULL) {
                obj->flags |= BTLOBJ_FLAG_CARD_BREAK_PENDING;
            }

            FadeFromAmount(FADE_MODE_ADD_WHITE, 10, 4);
        } else {
            gBtlWork->flags &= ~BTL_FLAG_PLAYER_CARD_ACTION;
            player->flags |= BTLOBJ_FLAG_CARD_BREAK_PENDING;
            FadeFromAmount(FADE_MODE_RED, 10, 4);
        }

        MosaicStartIn(16, 15);
        SetBattleZoom(1, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
        gBtlWork->phaseStep = 0;
    }

    if (gBtlWork->flags & BTL_FLAG_CARD_PLAY_START) {
        changed = TRUE;
        gBtlWork->flags &= ~BTL_FLAG_CARD_PLAY_START;

        if (gBtlWork->soraOwnsPlay) {
            gBtlWork->flags |= BTL_FLAG_PLAYER_CARD_ACTION;
            player->flags |= BTLOBJ_FLAG_CARD_ACTION_PENDING;
        } else {
            BtlObj* obj;
            gBtlWork->flags |= BTL_FLAG_OPPONENT_CARD_ACTION;
            obj = gBtlWork->actor3;

            if (obj != NULL) {
                obj->flags |= BTLOBJ_FLAG_CARD_ACTION_PENDING;
            }
        }

        gBtlWork->phase = BTL_PHASE_CARD_PLAY;
        gBtlWork->phaseStep = 0;
    } else {
        changed = FALSE;
    }

    if (gBtlWork->flags & BTL_FLAG_PLAYER_DEFEATED) {
        if (gBtlWork->phase != BTL_PHASE_GAME_OVER) {
            gBtlWork->phase = BTL_PHASE_GAME_OVER;
            gBtlWork->phaseStep = 0;
        }
    } else if (gBtlWork->flags & BTL_FLAG_BATTLE_OVER) {
        if (gBtlWork->phase != BTL_PHASE_END) {
            gBtlWork->phase = BTL_PHASE_END;
            gBtlWork->phaseStep = BTL_END_STEP_CLOSE_CARDS;

            switch (gBtlWork->battleId) {
            case BATTLE_CARD_SOLDIERS:
            case BATTLE_EVENT_AGRABAH_2:
                pos.x = 0x10000;
                pos.y = (gBtlWork->yMin + gBtlWork->yMax) * 128;
                pos.z = -0x4600;
                CreateBossPrizeCardTask(gBtlWork->taskPools, &pos);
                break;
            case BATTLE_SHADOW_100:
                if (gBtlWork->flags & 0x100000) {
                    pos.x = 0x10000;
                    pos.y = (gBtlWork->yMin + gBtlWork->yMax) * 128;
                    pos.z = -0x4600;
                    CreateBossPrizeCardTask(gBtlWork->taskPools, &pos);
                }

                break;
            }
        }
    }

    switch ((u32)gBtlWork->phase) {
    case BTL_PHASE_START:
        if (gBtlWork->phaseStep == BTL_START_STEP_INTRO) {
            if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL)) {
                gBtlWork->task = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlStart, NULL);
            }

            gBtlWork->phaseStep = BTL_START_STEP_EXCLUDE_PALETTES;
        }

        if (FadeIsActive()) {
            break;
        }

        if (gBtlWork->phaseStep == BTL_START_STEP_EXCLUDE_PALETTES) {
            for (i = 0; i < 32; i++) {
                if (gBtlWork->fadeExcludedPalettes & (s32)(1U << i)) {
                    FadeSetPaletteExcluded(i, TRUE);
                }
            }

            gBtlWork->phaseStep = BTL_START_STEP_CREATE_TASKS;
        }

        if (IsTaskActiveNamed(gBtlWork->task, gTaskDescBtlStart.name)) {
            break;
        }

        if (gBtlWork->phaseStep == BTL_START_STEP_CREATE_TASKS) {
            gBtlWork->flags |= BTL_FLAG_ENEMY_MOVE_ENABLED;
            gBtlWork->flags &= ~BTL_FLAG_PAUSE_DISABLED;
            TaskCreate(gBtlWork->taskPools, &gTaskDescBtlLockon, NULL);
            TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlHpply, NULL);

            if (!(gBtlWork->flags & BTL_FLAG_TUTORIAL)) {
                TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlHpenm, NULL);
            }

            TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlExp, NULL);

            if (!(gBtlWork->flags & BTL_FLAG_BOSS_BATTLE) && !(gBtlWork->flags & BTL_FLAG_HUM_BATTLE)) {
                switch (gBtlWork->battleId) {
                case BATTLE_CARD_SOLDIERS:
                case BATTLE_EVENT_HALLOWEEN_TOWN:
                case BATTLE_EVENT_AGRABAH_1:
                case BATTLE_EVENT_AGRABAH_2:
                case BATTLE_TUTORIAL_0:
                case BATTLE_TUTORIAL_1:
                case BATTLE_TUTORIAL_2:
                case BATTLE_TUTORIAL_3:
                case BATTLE_TUTORIAL_4:
                case BATTLE_TUTORIAL_5:
                case BATTLE_TUTORIAL_6:
                    break;
                default:
                    if (gDebugFlags & DEBUG_FLAG_CHKBTL) {
                        TaskCreate(gBtlWork->taskPools, &gTaskDescBtlEscape, NULL);
                    } else if (gGameState.progression.tutorialFlags & TUTORIAL_FLAG_MAP) {
                        TaskCreate(gBtlWork->taskPools, &gTaskDescBtlEscape, NULL);
                    }

                    break;
                }
            }

            TaskCreate(&gBtlWork->taskPools[1], &gTaskDescCardBattleSora, NULL);

            if (gBtlWork->flags & BTL_FLAG_TUTORIAL) {
                if (gBtlWork->battleId == BATTLE_TUTORIAL_1) {
                    TaskCreate(&gBtlWork->taskPools[1], &gTaskDescCardBattleRiku, NULL);
                }
            } else if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
                TaskCreate(&gBtlWork->taskPools[1], &gTaskDescCardBattleRiku, NULL);
            }

            RequestOpenCards();
            RequestBossCardOpen();
            gBtlWork->phaseStep = BTL_START_STEP_FINISH;
        } else if (gBtlWork->phaseStep == BTL_START_STEP_FINISH) {
            gBtlWork->phase = BTL_PHASE_IDLE;
            gBtlWork->phaseStep = 0;

            if (gGameState.roomEffect == ROOM_EFFECT_MEETING_GROUND) {
                DropFriendCard(0x10000, gBtlWork->actor->y, gBtlWork->actor->z - 0x7800);
            }
        }

        break;
    case BTL_PHASE_IDLE:
        break;
    case BTL_PHASE_END:
        if (gBtlWork->phaseStep == BTL_END_STEP_CLOSE_CARDS) {
            RequestCloseCards();
            RequestBossCardClose();
            gBtlWork->flags &= ~BTL_FLAG_ENEMY_MOVE_ENABLED;

            if (gBtlWork->flags & BTL_FLAG_SUMMON_ACTIVE) {
                gBtlWork->flags |= BTL_FLAG_DISMISS_SUMMONS;
                FadeToOriginal(FADE_MODE_BLACK, 8);
            }

            SetBattleZoom(8, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            gBtlWork->flags |= BTL_FLAG_CARD_PLAY_ENDED;
            gBtlWork->actor2 = NULL;
            gBtlWork->phaseStep = BTL_END_STEP_WAIT_FRAME;
            gBtlWork->hcEffect = HC_EFFECT_NONE;
            gBtlWork->flags |= BTL_FLAG_STOP_SPAWNING;
        } else if (gBtlWork->phaseStep == BTL_END_STEP_WAIT_FRAME) {
            gBtlWork->phaseStep = BTL_END_STEP_REWARDS;
        } else {
            if (BgFxIsActive()) {
                break;
            }

            if (gBtlWork->flags & BTL_FLAG_BOSS_DEFEATING) {
                break;
            }

            if (gBtlWork->prizeCount != 0 && !(gBtlWork->flags & BTL_FLAG_ESCAPED)) {
                break;
            }

            if (gBtlWork->flags & BTL_FLAG_PREMIRE_COLLECTED) {
                if (gBtlWork->phaseStep == BTL_END_STEP_REWARDS) {
                    ReleaseBattleTiles();
                    gBtlWork->task = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescPremireChance, NULL);
                    gBtlWork->flags |= BTL_FLAG_PAUSE_DISABLED;
                    gBtlWork->flags |= BTL_FLAG_FIELD_HIDDEN;
                    gBtlWork->phaseStep = BTL_END_STEP_REWARD_SCREEN;
                } else {
                    if (IsTaskActiveNamed(gBtlWork->task, gTaskDescPremireChance.name)) {
                        break;
                    }

                    gBtlWork->flags &= ~BTL_FLAG_PREMIRE_COLLECTED;
                    gBtlWork->phaseStep = BTL_END_STEP_REWARDS;
                }

                break;
            }

            if (gBtlWork->pendingLevelUps != 0) {
                if (gBtlWork->phaseStep == BTL_END_STEP_REWARDS) {
                    ReleaseBattleTiles();
                    gBtlWork->task = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescLevelUp, NULL);
                    gBtlWork->flags |= BTL_FLAG_PAUSE_DISABLED;
                    gBtlWork->flags |= BTL_FLAG_FIELD_HIDDEN;
                    gBtlWork->phaseStep = BTL_END_STEP_REWARD_SCREEN;
                }

                break;
            }

            if (gBtlWork->phaseStep == BTL_END_STEP_REWARD_SCREEN) {
                if (!IsTaskActive(gBtlWork->task)) {
                    BtlObj* healed;
                    gBtlWork->phaseStep = BTL_END_STEP_REWARDS;
                    healed = gBtlWork->actor;
                    healed->hp = gGameState.progression.maxHp;
                    healed->maxHp = gGameState.progression.maxHp;
                }

                break;
            }

            if (gBtlWork->phaseStep == BTL_END_STEP_REWARDS && !FadeIsActive()) {
                gBtlWork->flags |= BTL_FLAG_PAUSE_DISABLED;
                gBtlWork->hitStop = 99;
                gBtlWork->phaseStep = BTL_END_STEP_FADE_OUT;
            } else if (gBtlWork->phaseStep == BTL_END_STEP_FADE_OUT) {
                SetBackdropColor(0, 0, 0);
                FadeStartOut(FADE_MODE_BLACK, 15);
                FadeLock();
                gBtlWork->phaseStep = BTL_END_STEP_EXIT;
            } else if (!FadeIsActive()) {
                ExitBattle();
            }
        }

        break;
    case BTL_PHASE_GAME_OVER:
        if (gBtlWork->phaseStep == 0) {
            RequestCloseCards();
            RequestBossCardClose();
            gBtlWork->flags &= ~BTL_FLAG_ENEMY_MOVE_ENABLED;
            gBtlWork->flags |= BTL_FLAG_CARD_PLAY_ENDED;
            gBtlWork->actor2 = NULL;
            gBtlWork->pendingEnemies = 0;
        }

        if (gBtlWork->phaseStep == 140) {
            FadeStartOut(FADE_MODE_WHITE, 100);
            FadeLock();
            gBtlWork->flags |= BTL_FLAG_PAUSE_DISABLED;
            gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
            gBtlWork->hitStop = 100;
        } else if (gBtlWork->phaseStep > 140 && !FadeIsActive()) {
            m4aMPlayAllStop();

            if (gDebugFlags & DEBUG_FLAG_CHKBTL) {
                ModeRequest(&gModeChkbtl, 0);
            } else {
                GameState* state = &gGameState;
                memcpy(&state->progression.maxHp, gBtlWork->savedProgression, sizeof(gBtlWork->savedProgression));
                state->flags |= GAME_FLAG_BATTLE_NOT_WON;

                switch (gBtlWork->battleId) {
                case BATTLE_ANSEM_1:
                    state->progression.friendFlags = 0;
                    break;
                case BATTLE_LARXENE_2:
                    state->progression.friendFlags &= ~(FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
                    break;
#ifdef VERSION_EU
                case BATTLE_HOOK:
                    state->progression.friendFlags &= ~FRIEND_FLAG_PETER_PAN;
                    break;
#endif
                }

                ModeRequest(&gModeContinue, 0);
            }

            break;
        }

        gBtlWork->phaseStep++;
        break;
    case BTL_PHASE_CARD_PLAY: {
        BtlObj* obj;

        if (changed) {
            break;
        }

        busy = FALSE;

        if (player->flags & BTLOBJ_FLAG_IN_CARD_ACTION) {
            busy = TRUE;
        }

        if (busy) {
            break;
        }

        obj = ListPoolFirst(&gBtlWork->pool);

        while (obj != NULL) {
            if (obj->flags & BTLOBJ_FLAG_IN_CARD_ACTION) {
                busy = TRUE;
                break;
            }

            obj = ListPoolNext(&obj->node);
        }

        if (busy) {
            break;
        }

        count = GetStockMoveCount();
        gBtlWork->phaseStep = 0;

        if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
            if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
                if (gBtlWork->stockMove >= count) {
                    gBtlWork->flags &= ~BTL_FLAG_STOCK_SEQUENCE;
                }

                if (gBtlWork->flags & BTL_FLAG_STOCK_SEQUENCE) {
                    player->flags |= BTLOBJ_FLAG_CARD_ACTION_PENDING;
                } else {
                    EndCardPlay();
                }
            } else {
                if (gRikuBtlWork->stockMove >= count) {
                    gRikuBtlWork->flags &= ~BTL_FLAG_STOCK_SEQUENCE;
                }

                if (gRikuBtlWork->flags & BTL_FLAG_STOCK_SEQUENCE) {
                    gRikuBtlWork->actor->flags |= BTLOBJ_FLAG_CARD_ACTION_PENDING;
                } else {
                    EndCardPlay();
                }
            }
        } else {
            if (gBtlWork->stockMove >= count) {
                gBtlWork->flags &= ~BTL_FLAG_STOCK_SEQUENCE;
            }

            if (gBtlWork->flags & BTL_FLAG_STOCK_SEQUENCE) {
                player->flags |= BTLOBJ_FLAG_CARD_ACTION_PENDING;
            } else {
                EndCardPlay();
            }
        }

        break;
    }
    }
}

u32 ClampBattlePosition(s32* px, s32* py, s32 radiusX, s32 radiusY) {
    s16 rx = radiusX;
    s16 ry = radiusY;
    u8 edge = BATTLE_EDGE_NONE;

    if (*py < (gBtlWork->yMin - ry) << 8) {
        *py = (gBtlWork->yMin - ry) << 8;
        edge = BATTLE_EDGE_TOP;
    }

    if (*py > (gBtlWork->yMax + ry) << 8) {
        *py = (gBtlWork->yMax + ry) << 8;
        edge = BATTLE_EDGE_BOTTOM;
    }

    if (*px < (gBtlWork->xMin - rx) << 8) {
        *px = (gBtlWork->xMin - rx) << 8;
        edge = BATTLE_EDGE_LEFT;
    }

    if (*px > (gBtlWork->xMax + rx) << 8) {
        *px = (gBtlWork->xMax + rx) << 8;
        edge = BATTLE_EDGE_RIGHT;
    }

    return edge;
}

void SetBattleBounds(s32 xMin, s32 xMax, s32 yMin, s32 yMax) {
    u16 x0 = xMin;
    u16 x1 = xMax;
    u16 y0 = yMin;
    u16 y1 = yMax;

    gBtlWork->xMin = x0;
    gBtlWork->xMax = x1;
    gBtlWork->yMin = y0;
    gBtlWork->yMax = y1;
    SetGimmickTarget(((s16)x0 + (s16)x1) << 7, ((s16)y0 + (s16)y1) << 7, -0x2000);
}

s32 ApplyBtlObjHit(BtlObj* obj) {
    if (obj->flags & BTLOBJ_FLAG_WARP_PENDING) {
        obj->flags &= ~BTLOBJ_FLAG_WARP_PENDING;

        if (obj->hitFlags & ATTACK_FLAG_NO_DEATH_EFFECT) {
            obj->flags |= BTLOBJ_FLAG_NO_DEATH_FX;
        }

        return BTL_REACTION_WARPED;
    }

    if (obj->flags & BTLOBJ_FLAG_DAMAGE_PENDING) {
        obj->flags &= ~(BTLOBJ_FLAG_CARD_ACTION_PENDING | BTLOBJ_FLAG_DAMAGE_PENDING);
#ifdef PLATFORM_PC
        /* experimental Re:Chain of Memories difficulty (100 % when off) */
        if (obj->damage > 0)
            obj->damage = PortScaleDamage(obj->damage, obj == gBtlWork->actor);
#endif
        obj->hp -= obj->damage;

        if (obj->hp < 0) {
            obj->hp = 0;
        }

        obj->flags &= ~BTLOBJ_FLAG_CARD_ACTION_PENDING;
        gBtlWork->hitStop = gBtlWork->pendingHitStop;
        obj->flags |= (BTLOBJ_FLAG_HIT_LOCKED | BTLOBJ_FLAG_CARD_USE_BLOCKED | BTLOBJ_FLAG_HURT);
        obj->hitFlashFrames = 0;

        if (obj->hp <= 0) {
            obj->flags |= BTLOBJ_FLAG_INTANGIBLE;
            obj->flags &= ~(BTLOBJ_FLAG_HEAL_PENDING | BTLOBJ_FLAG_STUN_PENDING);
            obj->flags |= BTLOBJ_FLAG_DEFEATED;
            obj->badStatus = BAD_STATUS_NONE;
            obj->badStatusTimer = 0;

            if (obj->hitFlags & ATTACK_FLAG_NO_DEATH_EFFECT) {
                obj->flags |= BTLOBJ_FLAG_NO_DEATH_FX;
            }

            if (obj->flags & BTLOBJ_FLAG_GRAVITY_PENDING) {
                obj->flags &= ~BTLOBJ_FLAG_GRAVITY_PENDING;
                return BTL_REACTION_GRAVITY_DEFEATED;
            }

            return BTL_REACTION_DEFEATED;
        }

        if (obj->kind != BTLOBJ_KIND_PLAYER && GetRandom() % 8 == 0) {
            DropFriendCard(obj->x, obj->y, obj->z - 0x7800);
        }

        if (obj->flags & BTLOBJ_FLAG_GRAVITY_PENDING) {
            obj->flags &= ~BTLOBJ_FLAG_GRAVITY_PENDING;
            obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;
            obj->badStatus = BAD_STATUS_NONE;
            obj->badStatusTimer = 0;
            return BTL_REACTION_GRAVITY;
        }

        if (obj->flags & BTLOBJ_FLAG_STUN_PENDING) {
            obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;

            if (obj->badStatus != BAD_STATUS_STUN) {
                obj->badStatus = BAD_STATUS_STUN;
                obj->badStatusTimer = 240;
            }

            return BTL_REACTION_STUNNED;
        }

        if (obj->flags & BTLOBJ_FLAG_TERROR_PENDING) {
            obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;
            obj->badStatus = BAD_STATUS_TERROR;
            obj->badStatusTimer = 300;
            return BTL_REACTION_TERRIFIED;
        }

        if (obj->flags & BTLOBJ_FLAG_CONFUSE_PENDING) {
            obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;
            obj->badStatus = BAD_STATUS_CONFUSE;
            obj->badStatusTimer = 300;
            return BTL_REACTION_HURT;
        }

        if (obj->flags & BTLOBJ_FLAG_BIND_PENDING) {
            obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;

            if (obj->badStatus != BAD_STATUS_BIND) {
                obj->badStatus = BAD_STATUS_BIND;
                obj->badStatusTimer = 600;
            }

            return BTL_REACTION_HURT;
        }

        obj->badStatus = BAD_STATUS_NONE;
        obj->badStatusTimer = 0;
        return BTL_REACTION_HURT;
    }

    if (obj->flags & BTLOBJ_FLAG_HEAL_PENDING) {
        obj->flags &= ~(BTLOBJ_FLAG_CARD_ACTION_PENDING | BTLOBJ_FLAG_HEAL_PENDING);
        obj->flags |= (BTLOBJ_FLAG_HIT_LOCKED | BTLOBJ_FLAG_CARD_USE_BLOCKED);
        return BTL_REACTION_HEALED;
    }

    if (obj->flags & BTLOBJ_FLAG_HAZARD_PENDING) {
        obj->flags &= ~BTLOBJ_FLAG_HAZARD_PENDING;

        if (obj->hp > 0) {
            ClearBtlObjActionFlags(obj);
            return BTL_REACTION_HAZARD;
        }

        return BTL_REACTION_NONE;
    }

    if (obj->flags & BTLOBJ_FLAG_STOP_PENDING) {
        obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;
        gBtlWork->hitStop = gBtlWork->pendingHitStop;
        ClearBtlObjActionFlags(obj);
        obj->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;

        if (obj->badStatus != BAD_STATUS_STOP) {
            obj->badStatus = BAD_STATUS_STOP;
            obj->badStatusTimer = obj->damage;
        }

        return BTL_REACTION_STOPPED;
    }

    return BTL_REACTION_NONE;
}

u8 TryStartCardAction(BtlObj* obj) {
    u64 flags = obj->flags;

    if (flags & BTLOBJ_FLAG_CARD_ACTION_PENDING) {
        obj->flags &= ~(BTLOBJ_FLAG_CARD_ACTION_PENDING | BTLOBJ_FLAG_DAMAGE_PENDING | BTLOBJ_FLAG_HEAL_PENDING | BTLOBJ_FLAG_HURT | BTLOBJ_FLAG_STUN_PENDING | BTLOBJ_FLAG_GRAVITY_PENDING);
        obj->flags |= (BTLOBJ_FLAG_IN_CARD_ACTION | BTLOBJ_FLAG_HIT_LOCKED);
        obj->originX = obj->x;
        obj->originY = obj->y;
        obj->originZ = obj->z;
        return TRUE;
    }

    return FALSE;
}

s32 UpdateBtlObjReaction(BtlObj* obj) {
    u16 delayed;
    u16 invincible;
    u16 cooldown;

    if (obj->badStatus == BAD_STATUS_STOP) {
        if (obj->flags & BTLOBJ_FLAG_DAMAGE_PENDING) {
            obj->flags &= ~(BTLOBJ_FLAG_DAMAGE_PENDING | BTLOBJ_FLAG_GRAVITY_PENDING);
            obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;
            gBtlWork->hitStop = gBtlWork->pendingHitStop;
            obj->invincibleTimer = 30;
            obj->delayedDamage += obj->damage;
        }
    } else {
        delayed = obj->delayedDamage;

        if ((s16)obj->delayedDamage > 0) {
            obj->damage = delayed;
            obj->delayedDamage = 0;
            gBtlWork->pendingHitStop = 0;
            obj->flags &= ~BTLOBJ_FLAGS_STATUS_PENDING;
            obj->flags |= BTLOBJ_FLAG_DAMAGE_PENDING;
            obj->hitFlags = 0;
            obj->knockbackSpeed = 0;
            obj->knockbackLift = 0;
        }
    }

    invincible = obj->invincibleTimer;

    if (obj->invincibleTimer > 0) {
        obj->invincibleTimer = invincible - 1;
    }

    cooldown = obj->popCooldown;

    if (obj->popCooldown > 0) {
        obj->popCooldown = cooldown - 1;
    }

    if (obj->flags & BTLOBJ_FLAG_CARD_BREAK_PENDING) {
        obj->flags &= ~(BTLOBJ_FLAG_CARD_ACTION_PENDING | BTLOBJ_FLAG_DAMAGE_PENDING | BTLOBJ_FLAG_CARD_BREAK_PENDING);
        ClearBtlObjActionFlags(obj);
        obj->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
        CreateBtlPopTask(obj, BTL_POP_CARD_BREAK);
        gBtlWork->hitStop = 12;
        return BTL_REACTION_CARD_BROKEN;
    }

    if (TryStartCardAction(obj)) {
        return BTL_REACTION_CARD_ACTION;
    }

    return ApplyBtlObjHit(obj);
}

void ClearBtlObjActionFlags(BtlObj* obj) {
    obj->flags &= ~(BTLOBJ_FLAG_IN_CARD_ACTION | BTLOBJ_FLAG_HIT_LOCKED | BTLOBJ_FLAG_CARD_USE_BLOCKED | BTLOBJ_FLAG_HURT);
}

u16 GetBattleSpritePriorityFlags(s32 y) {
    if (y < gBtlWork->bossY + (gBtlWork->bossPriorityOffset << 8)) {
        return SPRITE_PRIORITY(2);
    }

    return SPRITE_PRIORITY(1);
}

void BeginBossDefeat(BtlObj* actor) {
    BtlObj* enemy;

    gBtlWork->flags |= BTL_FLAG_BOSS_DEFEATING;
    gBtlWork->flags |= BTL_FLAG_DISMISS_SUMMONS;
    gBtlWork->flags |= BTL_FLAG_BATTLE_OVER;
    SetEnemyJiminyFlag(actor);
    m4aMPlayFadeOut(gMPlayTable[gSongTable[SONG_BGM_BOSS1_WORLD].ms].info, 12);
    FadeStartIn(FADE_MODE_ADD_WHITE, 20);
    FadeLock();
    enemy = ListPoolFirst(&gBtlWork->pool);

    while (enemy != NULL) {
        enemy->node.flags |= LIST_NODE_FLAG_SKIP;
        enemy = ListPoolNext(&enemy->node);
    }

    gBtlWork->enemyCount = 0;
}

void EndBossDefeat() {
    gBtlWork->flags &= ~BTL_FLAG_BOSS_DEFEATING;
}

void SetEnemyKindFlags(BtlObj* obj) {
    switch (obj->kind) {
    case ENEMY_RED_NOCTURNE:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_FIRE | BTLOBJ_FLAG_WEAK_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    case ENEMY_BLUE_RHAPSODY:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_BLIZZARD | BTLOBJ_FLAG_WEAK_FIRE | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    case ENEMY_YELLOW_OPERA:
        obj->flags |= BTLOBJ_FLAG_ABSORB_THUNDER;
        break;
    case ENEMY_GREEN_REQUIEM:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_FIRE | BTLOBJ_FLAG_ABSORB_BLIZZARD | BTLOBJ_FLAG_ABSORB_THUNDER);
        break;
    case ENEMY_SEA_NEON:
        obj->flags |= BTLOBJ_FLAG_WEAK_THUNDER;
        break;
    case ENEMY_BLACK_FUNGUS:
        obj->flags |= (BTLOBJ_FLAG_IMMUNE_TERROR | BTLOBJ_FLAG_IMMUNE_WARP | BTLOBJ_FLAG_IMMUNE_CONFUSE | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_PHYSICAL);
        break;
    case ENEMY_SCREWDIVER:
        obj->flags |= BTLOBJ_FLAG_ABSORB_THUNDER;
        break;
    case ENEMY_WIZARD:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_FIRE | BTLOBJ_FLAG_ABSORB_BLIZZARD | BTLOBJ_FLAG_ABSORB_THUNDER | BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY);
        break;
    case ENEMY_AQUATANK:
        obj->flags |= BTLOBJ_FLAG_ABSORB_THUNDER;
        break;
    case ENEMY_GUARD_ARMOR:
        obj->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_JAFAR:
        obj->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_TRICKMASTER:
        obj->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_URSULA:
        obj->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_PARASITE_CAGE:
        obj->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_DRAGON_MALEFICENT:
        obj->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_DARKSIDE:
        obj->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_OOGIE_BOOGIE:
        obj->flags |= (BTLOBJ_FLAG_WEAK_PHYSICAL | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_MARLUXIA_2:
        obj->flags |= (BTLOBJ_FLAG_WEAK_PHYSICAL | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_HOOK:
        obj->flags |= (BTLOBJ_FLAG_IMMUNE_THUNDER | BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_CLOUD:
        obj->flags |= (BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_HADES:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_FIRE | BTLOBJ_FLAG_IMMUNE_THUNDER | BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_RIKU_REPLICA:
        obj->flags |= (BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    case ENEMY_CARD_SOLDIER_SPADE:
        obj->flags |= BTLOBJ_FLAG_WEAK_FIRE;
        break;
    case ENEMY_CARD_SOLDIER_HEART:
        obj->flags |= BTLOBJ_FLAG_WEAK_FIRE;
        break;
    case ENEMY_AXEL:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_FIRE | BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_LARXENE:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_THUNDER | BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD);
        break;
    case ENEMY_VEXEN:
        obj->flags |= (BTLOBJ_FLAG_ABSORB_BLIZZARD | BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_MARLUXIA:
        obj->flags |= (BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_WEAK_PHYSICAL | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_ANSEM:
        obj->flags |= (BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case ENEMY_LEXAEUS:
        obj->flags |= (BTLOBJ_FLAG_IMMUNE_BLIZZARD | BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_PHYSICAL);
        break;
    case ENEMY_SHADOW:
    default:
        break;
    }
}

void InitEnemyBtlObj(BtlObj* obj, const EmyKind* kind, s32 x, s32 y, s32 z) {
    const EnemyBaseStats* stats;
    enum EmyId {
        EMY_ID_32 = 32,
        EMY_ID_33 = 33,
        EMY_ID_34 = 34,
        EMY_ID_35 = 35,
        EMY_ID_36 = 36,
        EMY_ID_37 = 37,
        EMY_ID_38 = 38,
        EMY_ID_39 = 39,
        EMY_ID_40 = 40,
        EMY_ID_45 = 45,
        EMY_ID_48 = 48,
        EMY_ID_49 = 49,
        EMY_ID_50 = 50,
        EMY_ID_51 = 51,
        EMY_ID_52 = 52,
        EMY_ID_53 = 53
    } id;
    s32 hpRate;
    s32 attackRate;
    s32 expRate;

    stats = GetEnemyBaseStats(kind->id);

    if (stats != NULL) {
        id = kind->id;

        switch (id) {
        case EMY_ID_45:
        case EMY_ID_48:
        case EMY_ID_49:
        case EMY_ID_50:
        case EMY_ID_51:
        case EMY_ID_52:
        case EMY_ID_53:
            switch (gBtlWork->battleId) {
            case BATTLE_RIKU_1:
                obj->maxHp = 1120;
                obj->attack = 5;
                obj->exp = 2775;
                break;
            case BATTLE_RIKU_2:
                obj->maxHp = 1120;
                obj->attack = 5;
                obj->exp = 3225;
                break;
            case BATTLE_RIKU_3:
                obj->maxHp = 1120;
                obj->attack = 8;
                obj->exp = 5700;
                break;
            case BATTLE_RIKU_4:
                obj->maxHp = 1680;
                obj->attack = 10;
                obj->exp = 6825;
                break;
            case BATTLE_RIKU_5:
                obj->maxHp = 1120;
                obj->attack = 5;
                obj->exp = 1875;
                break;
            case BATTLE_RIKU_6:
                obj->maxHp = 1680;
                obj->attack = 10;
                obj->exp = 5700;
                break;
            case BATTLE_AXEL_1:
                obj->maxHp = 320;
                obj->attack = 2;
                obj->exp = 75;
                break;
            case BATTLE_AXEL_2:
                obj->maxHp = 1680;
                obj->attack = 15;
                obj->exp = 6825;
                break;
            case BATTLE_LARXENE_1:
                obj->maxHp = 1120;
                obj->attack = 5;
                obj->exp = 2325;
                break;
            case BATTLE_LARXENE_2:
                obj->maxHp = 1680;
                obj->attack = 15;
                obj->exp = 6263;
                break;
            case BATTLE_VEXEN_1:
                obj->maxHp = 1120;
                obj->attack = 15;
                obj->exp = 4125;
                break;
            case BATTLE_VEXEN_2:
                obj->maxHp = 1120;
                obj->attack = 20;
                obj->exp = 5700;
                break;
            case BATTLE_VEXEN_3:
                obj->maxHp = 1120;
                obj->attack = 3;
                obj->exp = 975;
                break;
            case BATTLE_ANSEM_1:
                obj->maxHp = 400;
                obj->attack = 3;
                obj->exp = 133;
                break;
            case BATTLE_ANSEM_2:
                obj->maxHp = 2240;
                obj->attack = 25;
                obj->exp = 0;
                break;
            case BATTLE_LEXAEUS:
                obj->maxHp = 1680;
                obj->attack = 15;
                obj->exp = 6517;
                break;
            default:
                obj->maxHp = 2240;
                obj->attack = 27;
                obj->exp = 13131;
                break;
            }

            break;
        case EMY_ID_37:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                obj->maxHp = 300;
                obj->attack = 4;
                obj->exp = 150;
                break;
            }
        default:
            if (gGameState.floor <= 9) {
                hpRate = 25;
                attackRate = 102;
                expRate = 384;
            } else {
                hpRate = 51;
                attackRate = 76;
                expRate = 640;
            }

            obj->maxHp = ((gGameState.floor * hpRate + 256) * stats->hp) >> 8;
            obj->attack = ((gGameState.floor * attackRate + 256) * stats->attack) >> 8;
            obj->exp = ((expRate * gGameState.floor + 256) * (u16)stats->exp) >> 8;
            break;
        }
    } else {
        obj->maxHp = kind->maxHp;
        obj->attack = 0;
        obj->exp = 1;
        id = kind->id;
    }

    obj->attackOffset = 80;
    obj->attackRangeX = 32;
    obj->attackRangeY = 32;
    obj->cardInterval = 100;
    obj->hp = obj->maxHp;
    obj->self = obj;
    obj->x = x;
    obj->y = y;
    obj->z = z;
    obj->groundZ = 0;
    obj->flags = 0;
    obj->kindFlags = kind->flags;
    obj->height = kind->height;
    obj->centerHeight = kind->centerHeight;
    obj->centerOffsetX = 0;
    obj->radiusX = kind->radius;
    obj->radiusY = kind->radius >> 1;
    obj->kind = id;
    obj->damage = 0;
    obj->floorZ = 0;
    obj->parent = NULL;
    obj->invincibleTimer = 0;
    obj->delayedDamage = 0;
    obj->shadowPriority = 0xFFF1;
    obj->btl = NULL;
    obj->badStatus = BAD_STATUS_NONE;
    obj->badStatusTimer = 0;
    /* @bug: in the scene before Riku's B3 boss the enemies are created before the battle actor (NULL read) */
    obj->confuseTargetX = BIOS_PTR(gBtlWork->actor)->x;
    obj->confuseTargetY = BIOS_PTR(gBtlWork->actor)->y;
    obj->confuseTargetZ = BIOS_PTR(gBtlWork->actor)->z;
    obj->vx = 0;
    obj->vy = 0;
    obj->popCooldown = 0;

    switch (id) {
    case EMY_ID_32:
    case EMY_ID_33:
    case EMY_ID_34:
    case EMY_ID_35:
    case EMY_ID_36:
    case EMY_ID_37:
    case EMY_ID_38:
    case EMY_ID_39:
    case EMY_ID_40:
        if (!(kind->flags & EMY_KIND_FLAG_NO_COLLIDER)) {
            ColliderInit(&obj->collider, COLLIDER_TYPE_BOSS, kind->radius, kind->height);
        }

        obj->flags |= (BTLOBJ_FLAG_IMMUNE_STOP | BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_IMMUNE_TERROR | BTLOBJ_FLAG_IMMUNE_WARP | BTLOBJ_FLAG_IMMUNE_CONFUSE | BTLOBJ_FLAG_IMMUNE_BIND);
        obj->flags |= BTLOBJ_FLAG_BOSS;
        break;
    default:
        if (!(kind->flags & EMY_KIND_FLAG_NO_COLLIDER)) {
            if (kind->flags & EMY_KIND_FLAG_NO_ENEMY_COLLISION) {
                ColliderInit(&obj->collider, COLLIDER_TYPE_ENEMY_NO_ENEMY_COLLISION, kind->radius, kind->height);
            } else {
                ColliderInit(&obj->collider, COLLIDER_TYPE_ENEMY, kind->radius, kind->height);
            }
        }
    }

    SetEnemyKindFlags(obj);

    if (kind->flags & EMY_KIND_FLAG_LARGE_BODY) {
        obj->flags |= BTLOBJ_FLAG_LARGE_SHADOW;
    }

    ListNodeInit(&obj->node, &gBtlWork->pool, obj);
    ListPoolAppend(&obj->node, &gBtlWork->pool);
    gBtlWork->enemyCount++;
}

void ReleaseEnemyBtlObj(BtlObj* obj) {
    BtlObj* self = obj->self;

    if (self == obj) {
        ListPoolRemove(&self->node, &gBtlWork->pool);

        if (!(self->kindFlags & EMY_KIND_FLAG_NO_COLLIDER)) {
            ColliderUnregister(&self->collider);
        }

        gBtlWork->enemyCount--;
    }
}

u8 CreateBtlPrizeTasksCapped(BtlPrizeSrc* src, u16 kind, s16 value, s16* remaining, s16* cnt) {
    s16 i;
    s16 count;

    count = *remaining / value;
    src->kind = kind;

    for (i = 0; i < count; i++) {
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPrize, src);

        if (++(*cnt) > 2) {
            return TRUE;
        }
    }

    *remaining = *remaining % value;
    return FALSE;
}

void CreateBtlPrizeTasks(BtlPrizeSrc* src, u16 kind, s16 value, s16* remaining) {
    s16 i;
    s16 count;
    count = *remaining / value;
    src->kind = kind;

    for (i = 0; i < count; i++) {
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPrize, src);
    }

    *remaining = *remaining % value;
}

void DropBossPrizes(BtlObj* obj) {
    BtlPrizeSrc src;
    s16 remaining;
    src.x = obj->x;
    src.y = obj->y;
    src.z = obj->z;
    src.noTimeout = TRUE;
    remaining = obj->exp;
    CreateBtlPrizeTasks(&src, BTL_PRIZE_EXP_1400, 0x578, &remaining);
    CreateBtlPrizeTasks(&src, BTL_PRIZE_EXP_199, 199, &remaining);
    CreateBtlPrizeTasks(&src, BTL_PRIZE_EXP_60, 60, &remaining);
    CreateBtlPrizeTasks(&src, BTL_PRIZE_EXP_30, 30, &remaining);
    CreateBtlPrizeTasks(&src, BTL_PRIZE_EXP_10, 10, &remaining);
    CreateBtlPrizeTasks(&src, BTL_PRIZE_EXP_5, 5, &remaining);
    CreateBtlPrizeTasks(&src, BTL_PRIZE_EXP_1, 1, &remaining);
}

void DropEnemyPrizes(BtlObj* obj) {
    BtlPrizeSrc src;
    BtlPrizeSrc cardSrc;
    s16 remaining;
    s16 cnt;
    s32 enemyCard;
    s32 hit;
    s32 chance;
    s32 roll;

    if (gBtlWork->flags & BTL_FLAG_NO_ENEMY_DROPS) {
        return;
    }

    src.x = obj->x;
    src.y = obj->y;
    src.z = obj->z;
    src.noTimeout = FALSE;
    remaining = obj->exp;
    cnt = 0;

    if (gBtlWork->enemyCount == 1 && gBtlWork->pendingEnemies <= 0) {
        if (CountRegularMapCards() <= 4) {
            enemyCard = FALSE;
        } else {
            switch (obj->kind) {
            case ENEMY_POWERWILD:
            case ENEMY_SEARCH_GHOST:
            case ENEMY_LARGE_BODY:
            case ENEMY_FAT_BANDIT:
            case ENEMY_AQUATANK:
            case ENEMY_CREEPER_PLANT:
                chance = 2;
                break;
            case ENEMY_SHADOW:
            case ENEMY_SEA_NEON:
            case ENEMY_WHITE_MUSHROOM:
            case ENEMY_SOLDIER:
            case ENEMY_BOUNCYWILD:
            case ENEMY_BARREL_SPIDER:
            case ENEMY_SCREWDIVER:
            case ENEMY_GARGOYLE:
            case ENEMY_AIR_PIRATE:
            case ENEMY_DARKBALL:
            case ENEMY_WYVERN:
            case ENEMY_DEFENDER:
            case ENEMY_TORNADO_STEP:
                chance = 4;
                break;
            case ENEMY_RED_NOCTURNE:
            case ENEMY_BLUE_RHAPSODY:
            case ENEMY_YELLOW_OPERA:
            case ENEMY_GREEN_REQUIEM:
            case ENEMY_BLACK_FUNGUS:
            case ENEMY_AIR_SOLDIER:
            case ENEMY_BANDIT:
            case ENEMY_WIGHT_KNIGHT:
            case ENEMY_PIRATE:
            case ENEMY_WIZARD:
            case ENEMY_NEOSHADOW:
            case ENEMY_CRESCENDO:
                chance = 3;
                break;
            default:
                chance = 0;
                break;
            }

            if (chance > 99) {
                enemyCard = TRUE;
            } else if (chance == 0) {
                enemyCard = FALSE;
            } else {
                if (gGameState.roomEffect == ROOM_EFFECT_ALMIGHTY_DARKNESS ||
                    gGameState.roomEffect == ROOM_EFFECT_ENEMY_CARD_DROPS) {
                    chance = (chance * 5 * 128) >> 8;
                }

                chance = 100 / chance;
                roll = GetRandom();
                hit = FALSE;

                if ((u16)roll % chance == 0) {
                    hit = TRUE;
                }

                enemyCard = hit;
            }
        }

        if (gGameState.flags & GAME_FLAG_RIKU) {
            enemyCard = FALSE;
        }

        if (gBtlWork->battleId != BATTLE_CARD_SOLDIERS && gBtlWork->battleId != BATTLE_EVENT_AGRABAH_2) {
            if (enemyCard) {
                CreateHeartlessCardTask(&gBtlWork->taskPools[0], obj->x >> 8, obj->y >> 8, obj->z >> 8, obj->kind);
            } else {
                cardSrc.x = obj->x;
                cardSrc.y = obj->y;
                cardSrc.z = obj->z;
                CreatePrizeCardTask(&gBtlWork->taskPools[0], &cardSrc);
            }
        }
    }

    if (CreateBtlPrizeTasksCapped(&src, BTL_PRIZE_EXP_1400, 0x578, &remaining, &cnt)) {
        return;
    }

    if (CreateBtlPrizeTasksCapped(&src, BTL_PRIZE_EXP_199, 199, &remaining, &cnt)) {
        return;
    }

    if (CreateBtlPrizeTasksCapped(&src, BTL_PRIZE_EXP_60, 60, &remaining, &cnt)) {
        return;
    }

    if (CreateBtlPrizeTasksCapped(&src, BTL_PRIZE_EXP_30, 30, &remaining, &cnt)) {
        return;
    }

    if (CreateBtlPrizeTasksCapped(&src, BTL_PRIZE_EXP_10, 10, &remaining, &cnt)) {
        return;
    }

    if (CreateBtlPrizeTasksCapped(&src, BTL_PRIZE_EXP_5, 5, &remaining, &cnt)) {
        return;
    }

    CreateBtlPrizeTasksCapped(&src, BTL_PRIZE_EXP_1, 1, &remaining, &cnt);
}

void TryDropPremireCard(BtlObj* obj) {
    BtlPrizeSrc src;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        return;
    }

    if (IsActiveDeckAllPremium()) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_NO_ENEMY_DROPS) {
        return;
    }

    if (gBtlWork->flags & BTL_FLAG_PREMIRE_DROPPED) {
        return;
    }

    if (!HasNonPremiumCardsInActiveDeck()) {
        return;
    }

    if (GetDeckCardCount(GetActiveDeckIndex()) <= 9) {
        return;
    }

    gBtlWork->flags |= BTL_FLAG_PREMIRE_DROPPED;
    src.x = obj->x;
    src.y = obj->y;
    src.z = obj->z;
    src.noTimeout = FALSE;
    TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlPremire, &src);
}

u8 IsPlayerOnPlatform(Collider* platform) {
    if (platform == gBtlWork->platform) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void SetBattleActorPosition(s32 x, s32 y, s32 z) {
    gBtlWork->actor->x = x;
    gBtlWork->actor->y = y;
    gBtlWork->actor->z = z;
}

void RequestEnemyCardUse(BtlObj* obj) {
    if (!(obj->flags & BTLOBJ_FLAGS_NO_CARD_USE)) {
        gBtlWork->actor4 = obj;
    }
}

void TryEnemyCardUse(BtlObj* obj) {
    s32 x;
    s32 y;
    s32 cx;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;

    if (obj->flags & BTLOBJ_FLAGS_NO_CARD_USE) {
        return;
    }

    if (GetRandom() % (obj->cardInterval * gBtlWork->enemyCount) != 0) {
        return;
    }

    GetEnemyTargetPosition(obj, &x, &y, NULL);

    if (obj->attackRangeX == 0) {
        gBtlWork->actor4 = obj;
        return;
    }

    if (obj->flags & BTLOBJ_FLAG_FACING_LEFT) {
        cx = obj->x - ((s16)(x1 = obj->attackOffset) * 256);
        x1 = obj->attackRangeX;
    } else {
        x1 = obj->attackOffset;
        cx = obj->x + (s16)x1 * 256;
        x1 = obj->attackRangeX;
    }

    x1 -= 4;
    x0 = cx - (x1 *= 256);
    y0 = obj->y - (obj->attackRangeY * 256);
    x1 = x0 + ((obj->attackRangeX + 4) * 512);
    y1 = y0 + ((obj->attackRangeY + 4) * 512);

    if (x0 > x) {
        return;
    }

    if (x1 < x) {
        return;
    }

    if (y0 > y) {
        return;
    }

    if (y1 < y) {
        return;
    }

    gBtlWork->actor4 = obj;
}

void SetBtlObjParent(BtlObj* obj, BtlObj* parent) {
    obj->parent = parent;
}

u8 SpawnEnemy(s32 id, s32 x, s32 y, s32 z) {
    EnemySpawnRequest request;
    s32 born;

    born = TRUE;
    request.flags = 0;

    switch (id) {
    case ENEMY_SHADOW:
        request.desc = &gTaskDescEmy00;
        born = FALSE;
        break;
    case ENEMY_RED_NOCTURNE:
        request.desc = &gTaskDescEmy01;
        break;
    case ENEMY_BLUE_RHAPSODY:
        request.desc = &gTaskDescEmy02;
        break;
    case ENEMY_YELLOW_OPERA:
        request.desc = &gTaskDescEmy03;
        break;
    case ENEMY_GREEN_REQUIEM:
        request.desc = &gTaskDescEmy04;
        break;
    case ENEMY_SEA_NEON:
        request.desc = &gTaskDescEmy06;
        break;
    case ENEMY_WHITE_MUSHROOM:
        request.desc = &gTaskDescEmy07;
        break;
    case ENEMY_BLACK_FUNGUS:
        request.desc = &gTaskDescEmy08;
        break;
    case ENEMY_SOLDIER:
        request.desc = &gTaskDescEmy14;
        break;
    case ENEMY_POWERWILD:
        request.desc = &gTaskDescEmy15;
        break;
    case ENEMY_BOUNCYWILD:
        request.desc = &gTaskDescEmy16;
        break;
    case ENEMY_AIR_SOLDIER:
        request.desc = &gTaskDescEmy18;
        break;
    case ENEMY_BANDIT:
        request.desc = &gTaskDescEmy19;
        break;
    case ENEMY_BARREL_SPIDER:
        request.desc = &gTaskDescEmy21;
        break;
    case ENEMY_SEARCH_GHOST:
        request.desc = &gTaskDescEmy22;
        break;
    case ENEMY_SCREWDIVER:
        request.desc = &gTaskDescEmy23;
        break;
    case ENEMY_WIGHT_KNIGHT:
        request.desc = &gTaskDescEmy25;
        break;
    case ENEMY_GARGOYLE:
        request.desc = &gTaskDescEmy26;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_PIRATE:
        request.desc = &gTaskDescEmy27;
        break;
    case ENEMY_AIR_PIRATE:
        request.desc = &gTaskDescEmy28;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_DARKBALL:
        request.desc = &gTaskDescEmy29;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_WYVERN:
        request.desc = &gTaskDescEmy30;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_WIZARD:
        request.desc = &gTaskDescEmy31;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_NEOSHADOW:
        request.desc = &gTaskDescEmy37;
        born = FALSE;
        break;
    case ENEMY_LARGE_BODY:
        request.desc = &gTaskDescEmy38;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_FAT_BANDIT:
        request.desc = &gTaskDescEmy39;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_AQUATANK:
        request.desc = &gTaskDescEmy41;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_DEFENDER:
        request.desc = &gTaskDescEmy44;
        request.flags |= SPAWN_FLAG_LARGE_EFFECT;
        break;
    case ENEMY_TORNADO_STEP:
        request.desc = &gTaskDescEmy81;
        break;
    case ENEMY_CRESCENDO:
        request.desc = &gTaskDescEmy82;
        break;
    case ENEMY_CREEPER_PLANT:
        request.desc = &gTaskDescEmy83;
        break;
    case ENEMY_CARD_SOLDIER_HEART:
        request.desc = &gTaskDescEmyTrumpH;
        born = FALSE;
        break;
    case ENEMY_CARD_SOLDIER_SPADE:
        request.desc = &gTaskDescEmyTrumpS;
        born = FALSE;
        break;
    default:
        request.desc = &gTaskDescEmy00;
        break;
    }

    request.x = x;
    request.y = y;
    request.z = z;
    request.tileCount = gEnemyTileCounts[id];

    if (born) {
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlBorn, &request);
    } else {
        if (!CanAllocObjTiles(request.tileCount) || !CanAllocObjPalette(1)) {
            gBtlWork->pendingEnemies--;
            return 0;
        }

        TaskCreate(&gBtlWork->taskPools[0], request.desc, &request.x);
    }

    return 1;
}

void AllocBattleTiles() {
    if (gGameState.flags & GAME_FLAG_RIKU) {
        gBtlWork->tiles = AllocObjTiles(0x840, NULL);
    } else {
        gBtlWork->tiles = AllocObjTiles(0xC80, NULL);
        gBtlWork->tiles2 = AllocObjTiles(0xA00, NULL);

        if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
            gRikuBtlWork->tiles = AllocObjTiles(0xC80, NULL);
        }
    }

    gBtlWork->flags |= BTL_FLAG_TILES_ALLOCATED;
}

void ReleaseBattleTiles() {
    if (gBtlWork->flags & BTL_FLAG_TILES_ALLOCATED) {
        if (gGameState.flags & GAME_FLAG_RIKU) {
            ReleaseObjTiles(gBtlWork->tiles);
        } else {
            ReleaseObjTiles(gBtlWork->tiles);
            ReleaseObjTiles(gBtlWork->tiles2);

            if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
                ReleaseObjTiles(gRikuBtlWork->tiles);
            }
        }

        gBtlWork->flags &= ~BTL_FLAG_TILES_ALLOCATED;
    }
}

void SetGimmickFlag(u8 index) {
    if (index <= 4) {
        gBtlWork->gimmickFlags |= 1 << index;
    }
}

u8 ConsumeGimmickFlag(u8 index) {
    u8 mask;

    if (index > 4) {
        return FALSE;
    }

    mask = 1 << index;

    if (gBtlWork->gimmickFlags & mask) {
        gBtlWork->gimmickFlags &= ~mask;
        return TRUE;
    }

    return FALSE;
}

void DropGimmickCard(u8 index, s32 x, s32 y, s32 z) {
    u16 id;

    switch (index) {
    case 0:
        id = CARD_GIMMICK_0;
        break;
    case 1:
        id = CARD_GIMMICK_1;
        break;
    case 2:
        id = CARD_GIMMICK_2;
        break;
    case 3:
        id = CARD_GIMMICK_3;
        break;
    case 4:
        id = CARD_GIMMICK_4;
        break;
    default:
        return;
    }

    CreateGimmickCardTask(&gBtlWork->taskPools[0], x >> 8, y >> 8, z >> 8, id);
}

void SetGimmickTarget(s32 x, s32 y, s32 z) {
    gBtlWork->gimmickX = x;
    gBtlWork->gimmickY = y;
    gBtlWork->gimmickZ = z;
}

void SetBtlPaletteFadeExcluded(u8 index, u8 on) {
    if (index <= 0x1F) {
        if (on) {
            gBtlWork->fadeExcludedPalettes |= 1 << index;
        } else {
            gBtlWork->fadeExcludedPalettes &= ~(1 << index);
        }
    }
}

void SetBtlObjUnhittable(BtlObj* obj, u8 on) {
    if (on) {
        obj->flags |= BTLOBJ_FLAG_UNHITTABLE;
    } else {
        obj->flags &= ~BTLOBJ_FLAG_UNHITTABLE;
    }
}

void ExitBattle() {
    m4aMPlayAllStop();

    if (gDebugFlags & DEBUG_FLAG_CHKBTL) {
        ModeRequest(&gModeChkbtl, 0);
        return;
    }

    if (gGameState.flags & GAME_FLAG_RIKU) {
        switch (gBtlWork->battleId) {
        case BATTLE_ANSEM_1:
            RequestEventMode(EVENT_156_RIKU_B12F_GOAL_2);
            return;
        case BATTLE_VEXEN_3:
            RequestEventMode(EVENT_161_RIKU_B10F_GOAL_2);
            return;
        case BATTLE_RIKU_5:
            RequestEventMode(EVENT_164_RIKU_B8F_GOAL_2);
            return;
        case BATTLE_LEXAEUS:
            RequestEventMode(EVENT_172_RIKU_B4F_GOAL_2);
            return;
        case BATTLE_DARKSIDE:
            RequestEventMode(EVENT_181_RIKU_B3F_E2_2);
            return;
        case BATTLE_RIKU_6:
            RequestEventMode(EVENT_188_RIKU_B2F_E1_2);
            return;
        case BATTLE_ANSEM_2:
            gGameState.flags |= GAME_FLAG_RIKU_CLEAR;
            SaveWriteHeader(-1);
            RequestEventMode(EVENT_194_RIKU_B1F_LAST2);
            return;
        default:
            if (gBtlWork->flags & BTL_FLAG_BOSS_BATTLE) {
                AdvanceFloorStory();
                RequestMapMode();
            } else if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
                AdvanceFloorStory();
                RequestMapMode();
            } else {
                RequestMapMode();
            }

            return;
        }
    } else {
        switch (gBtlWork->battleId) {
        case BATTLE_CARD_SOLDIERS:
            RequestEventMode(EVENT_096_WONDERLAND_E1_2);
            return;
        case BATTLE_SHADOW_100:
            if (gBtlWork->flags & BTL_FLAG_ESCAPED) {
                RequestEventMode(EVENT_084_MONSTORO_E3_FAILURE_2);
            } else if (gBtlWork->flags & 0x100000) {
                RequestEventMode(EVENT_082_MONSTORO_E3_SUCCESS);
            } else {
                RequestEventMode(EVENT_083_MONSTORO_E3_FAILURE_1);
            }

            return;
        case BATTLE_EVENT_HALLOWEEN_TOWN:
            RequestEventMode(EVENT_088_HALLOWEEN_TOWN_E0_2);
            return;
        case BATTLE_EVENT_AGRABAH_1:
            RequestEventMode(EVENT_108_AGRABAH_E0_2);
            return;
        case BATTLE_EVENT_AGRABAH_2:
            RequestEventMode(EVENT_111_AGRABAH_E2_2);
            return;
        case BATTLE_GUARD_ARMOR:
            RequestEventMode(EVENT_009_1F_TRAVERSE_TOWN_E4);
            return;
        case BATTLE_JAFAR:
            RequestEventMode(EVENT_114_AGRABAH_END);
            return;
        case BATTLE_TRICKMASTER:
            RequestEventMode(EVENT_100_WONDERLAND_END);
            return;
        case BATTLE_URSULA:
            RequestEventMode(EVENT_106_ATLANTICA_END);
            return;
        case BATTLE_PARASITE_CAGE:
            RequestEventMode(EVENT_078_MONSTORO_E2_3);
            return;
        case BATTLE_DRAGON_MALEFICENT:
            RequestEventMode(EVENT_133_HOLLOWBASTION_END);
            return;
        case BATTLE_DARKSIDE:
            RequestEventMode(EVENT_056_12F_DESTINY_ISLAND_E2_2);
            return;
        case BATTLE_OOGIE_BOOGIE:
            RequestEventMode(EVENT_093_HALLOWEEN_TOWN_END);
            return;
        case BATTLE_MARLUXIA_2:
            gGameState.flags |= GAME_FLAG_SORA_CLEAR;
            SaveWriteHeader(-1);
            RequestEventMode(EVENT_071_13F_CASTLE_OBLIVION_LAST6);
            return;
        case BATTLE_LEON:
            RequestEventMode(EVENT_006_1F_TRAVERSE_TOWN_E2);
            return;
        case BATTLE_HOOK:
            RequestEventMode(EVENT_119_NEVERLAND_END);
            return;
        case BATTLE_CLOUD:
            RequestEventMode(EVENT_123_COLISEUM_E2_2);
            return;
        case BATTLE_HADES:
            RequestEventMode(EVENT_126_COLISEUM_END);
            return;
        case BATTLE_RIKU_1:
            RequestEventMode(EVENT_031_7F_GOAL_2);
            return;
        case BATTLE_AXEL_1:
            RequestEventMode(EVENT_011_1F_GOAL_2);
            return;
        case BATTLE_LARXENE_1:
            RequestEventMode(EVENT_027_6F_GOAL_3);
            return;
        case BATTLE_VEXEN_1:
            RequestEventMode(EVENT_041_10F_GOAL_2);
            return;
        case BATTLE_MARLUXIA:
            RequestEventMode(EVENT_067_13F_CASTLE_OBLIVION_LAST2);
            return;
        case BATTLE_RIKU_2:
            RequestEventMode(EVENT_034_8F_GOAL_2);
            return;
        case BATTLE_RIKU_3:
            RequestEventMode(EVENT_049_11F_GOAL_2);
            return;
        case BATTLE_AXEL_2:
            RequestEventMode(EVENT_065_13F_CASTLE_OBLIVION_E1_3);
            return;
        case BATTLE_LARXENE_2:
            RequestEventMode(EVENT_060_12F_GOAL_3);
            return;
        case BATTLE_VEXEN_2:
            RequestEventMode(EVENT_046_11F_TWILIGHT_TOWN_E1_2);
            return;
        case BATTLE_TUTORIAL_0:
            RequestEventMode(EVENT_003_1F_TRAVERSE_TOWN_E0_2);
            return;
        case BATTLE_TUTORIAL_1:
            RequestEventMode(EVENT_005_1F_TRAVERSE_TOWN_E1_2);
            return;
        case BATTLE_RIKU_4:
            AdvanceFloorStory();
            RequestMapMode();
            return;
        default:
            if (gBtlWork->flags & BTL_FLAG_BOSS_BATTLE) {
                AdvanceFloorStory();
                RequestMapMode();
            } else if (gBtlWork->flags & BTL_FLAG_HUM_BATTLE) {
                AdvanceFloorStory();
                RequestMapMode();
            } else {
                RequestMapMode();
            }

            return;
        }
    }
}

u8 ApplyBattleBounds(s32* x, s32* y, s32* z, s32* floor) {
    if (gBtlWork->boundsCallback != NULL) {
        return gBtlWork->boundsCallback(x, y, z, floor);
    }

    return 0;
}

void GetEnemyTargetPosition(BtlObj* obj, s32* x, s32* y, s32* z) {
    u16 roll;

    if (obj->badStatus == BAD_STATUS_CONFUSE) {
        if (x != NULL) {
            *x = obj->confuseTargetX;
        }

        if (y != NULL) {
            *y = obj->confuseTargetY;
        }

        if (z != NULL) {
            *z = obj->confuseTargetZ;
        }

        roll = GetRandom() % 6;

        if (roll == 0) {
            if (x != NULL) {
                *x = (gBtlWork->xMin + GetRandom() % (gBtlWork->xMax - gBtlWork->xMin + 1)) << 8;
            }

            if (y != NULL) {
                *y = (gBtlWork->yMin + GetRandom() % (gBtlWork->yMax - gBtlWork->yMin + 1)) << 8;
            }

            if (z != NULL) {
                *z = roll;
            }
        }
    } else {
        if (x != NULL) {
            *x = gBtlWork->targetX;
        }

        if (y != NULL) {
            *y = gBtlWork->targetY;
        }

        if (z != NULL) {
            *z = gBtlWork->targetZ;
        }
    }
}

void SetEnemyHpFromStats(BtlObj* obj, s32 id, s32 hpScale) {
    u16 enemyId = id;
    const EnemyBaseStats* stats = GetEnemyBaseStats(enemyId);

    if (stats != NULL) {
        obj->maxHp = (stats->hp * hpScale) >> 8;

        if (obj->maxHp <= 0) {
            obj->maxHp = 1;
        }

        obj->hp = obj->maxHp;
    }
}

void SetEnemyJiminyFlag(BtlObj* obj) {
    switch (obj->kind) {
    case ENEMY_SHADOW:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_SHADOW);
        break;
    case ENEMY_RED_NOCTURNE:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_RED_NOCTURNE);
        break;
    case ENEMY_BLUE_RHAPSODY:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_BLUE_RHAPSODY);
        break;
    case ENEMY_YELLOW_OPERA:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_YELLOW_OPERA);
        break;
    case ENEMY_GREEN_REQUIEM:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_GREEN_REQUIEM);
        break;
    case ENEMY_SEA_NEON:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_SEA_NEON);
        break;
    case ENEMY_WHITE_MUSHROOM:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_WHITE_MUSHROOM);
        break;
    case ENEMY_BLACK_FUNGUS:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_BLACK_FUNGUS);
        break;
    case ENEMY_SOLDIER:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_SOLDIER);
        break;
    case ENEMY_POWERWILD:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_POWERWILD);
        break;
    case ENEMY_BOUNCYWILD:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_BOUNCYWILD);
        break;
    case ENEMY_AIR_SOLDIER:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_AIR_SOLDIER);
        break;
    case ENEMY_BANDIT:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_BANDIT);
        break;
    case ENEMY_BARREL_SPIDER:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_BARREL_SPIDER);
        break;
    case ENEMY_SEARCH_GHOST:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_SEARCH_GHOST);
        break;
    case ENEMY_SCREWDIVER:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_SCREWDIVER);
        break;
    case ENEMY_WIGHT_KNIGHT:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_WIGHT_KNIGHT);
        break;
    case ENEMY_GARGOYLE:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_GARGOYLE);
        break;
    case ENEMY_PIRATE:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_PIRATE);
        break;
    case ENEMY_AIR_PIRATE:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_AIR_PIRATE);
        break;
    case ENEMY_DARKBALL:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_DARKBALL);
        break;
    case ENEMY_WYVERN:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_WYVERN);
        break;
    case ENEMY_WIZARD:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_WIZARD);
        break;
    case ENEMY_NEOSHADOW:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_NEOSHADOW);
        break;
    case ENEMY_LARGE_BODY:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_LARGE_BODY);
        break;
    case ENEMY_FAT_BANDIT:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_FAT_BANDIT);
        break;
    case ENEMY_AQUATANK:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_AQUATANK);
        break;
    case ENEMY_DEFENDER:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_DEFENDER);
        break;
    case ENEMY_TORNADO_STEP:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_TORNADO_STEP);
        break;
    case ENEMY_CRESCENDO:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_CRESCENDO);
        break;
    case ENEMY_CREEPER_PLANT:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_CREEPER_PLANT);
        break;
    case ENEMY_GUARD_ARMOR:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_GUARD_ARMOR);
        break;
    case ENEMY_TRICKMASTER:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_TRICKMASTER);
        break;
    case ENEMY_PARASITE_CAGE:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_PARASITE_CAGE);
        break;
    case ENEMY_DARKSIDE:
        SetJiminyFlag(JIMINY_RECORD_HEARTLESS_DARKSIDE);
        break;
    }
}

u8 StepHitFlash(BtlObj* obj) {
    if (gBtlWork->paused == TRUE) {
        return FALSE;
    }

    if (!(obj->flags & BTLOBJ_FLAG_HURT)) {
        return FALSE;
    }

    if (obj->hitFlashFrames > 0x17) {
        return FALSE;
    }

    obj->hitFlashFrames++;

    if (obj->hitFlashFrames & 1) {
        return TRUE;
    }

    return FALSE;
}

u8 StepHitFlashSolid(BtlObj* obj) {
    if (gBtlWork->paused == TRUE) {
        return FALSE;
    }

    if (!(obj->flags & BTLOBJ_FLAG_HURT)) {
        return FALSE;
    }

    if (obj->hitFlashFrames > 0x17) {
        return FALSE;
    }

    obj->hitFlashFrames++;
    return TRUE;
}
