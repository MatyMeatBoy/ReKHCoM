/**
 * tutorial.c
 * Battle Tutorial Guide
 */

#include "tutorial.h"
#include "gba/keys.h"
#include "sprites_btl_hud.h"
#include "system_state.h"
#include "anim.h"
#include "battle_work.h"
#include "btl4.h"
#include "btl_effect.h"
#include "card_api.h"
#include "card_battle.h"
#include "display.h"
#include "engine_math.h"
#include "gba/io_reg.h"
#include "key.h"
#include "obj.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "engine.h"
#include "sprite_palettes.h"
#include "card_message_data.h"

void TutorialOpenMessage(u16 message) {
    gDispCnt = (gDispCnt & ~DISPCNT_MODE_MASK) | DISPCNT_MODE_1;
    CreateCardMessageTask(&gBtlWork->taskPools[1], 0, message);
}

void TutorialOpenPersistentMessage(u16 message) {
    CreatePersistentSysmsgwinTask(&gBtlWork->taskPools[1], message);
}

void TutorialRestoreBgMode() {
    gDispCnt = (gDispCnt & ~DISPCNT_MODE_MASK) | DISPCNT_MODE_2;
}

enum TutorialState {
    TUTORIAL_STATE_WAIT,
    TUTORIAL_STATE_MESSAGE_DELAY,
    TUTORIAL_STATE_MESSAGE,
    TUTORIAL_STATE_PERSISTENT_DELAY,
    TUTORIAL_STATE_PERSISTENT_MESSAGE,
    TUTORIAL_STATE_SAY_FRIENDS_ARE_CARDS,
    TUTORIAL_STATE_DROP_FRIEND_CARD,
    TUTORIAL_STATE_SAY_PICK_UP_FRIENDS,
    TUTORIAL_STATE_HELP_MOVE = 9,
    TUTORIAL_STATE_PICK_UP_FRIEND_CARD,
    TUTORIAL_STATE_SELECT_FRIEND_CARD,
    TUTORIAL_STATE_SAY_CARDS_ON_TOP,
    TUTORIAL_STATE_HELP_USE_FRIEND_CARD,
    TUTORIAL_STATE_USE_FRIEND_CARD,
    TUTORIAL_STATE_SAY_CARDS_REAPPEAR,
    TUTORIAL_STATE_SAY_CARDS_RULE,
    TUTORIAL_STATE_HELP_JUMP_AND_DODGE,
    TUTORIAL_STATE_JUMP_AND_DODGE,
    TUTORIAL_STATE_SAY_MOVE_THEN_USE,
    TUTORIAL_STATE_HELP_USE_THREE_CARDS,
    TUTORIAL_STATE_USE_THREE_CARDS,
    TUTORIAL_STATE_SAY_CARDS_DISAPPEAR,
    TUTORIAL_STATE_SAY_CARDS_RUN_OUT,
    TUTORIAL_STATE_SAY_USE_UP_CARDS = 25,
    TUTORIAL_STATE_USE_UP_CARDS,
    TUTORIAL_STATE_SAY_NO_CARDS_LEFT,
    TUTORIAL_STATE_SAY_FOCUS = 29,
    TUTORIAL_STATE_HELP_RELOAD,
    TUTORIAL_STATE_RELOAD,
    TUTORIAL_STATE_SAY_CARDS_RETURNED,
    TUTORIAL_STATE_SAY_CARDS_LIMITED,
    TUTORIAL_STATE_HELP_CYCLE_CARDS,
    TUTORIAL_STATE_CYCLE_CARDS,
    TUTORIAL_STATE_SAY_CARD_CATEGORIES,
    TUTORIAL_STATE_SAY_CATEGORY_CONTENTS,
    TUTORIAL_STATE_HELP_SWITCH_CATEGORY,
    TUTORIAL_STATE_SWITCH_CATEGORY,
    TUTORIAL_STATE_SAY_ATTACK_OR_DEFEND,
    TUTORIAL_STATE_ROBE_FINISH,
    TUTORIAL_STATE_ROBE_END,
    TUTORIAL_STATE_LEON_START,
    TUTORIAL_STATE_SAY_CARD_VALUES,
    TUTORIAL_STATE_PRACTICE_BREAKS,
    TUTORIAL_STATE_PRACTICE_END,
    TUTORIAL_STATE_SAY_CARD_BREAK,
    TUTORIAL_STATE_SAY_DEFLECTED,
    TUTORIAL_STATE_SAY_ZERO_SPECIAL,
    TUTORIAL_STATE_SAY_ZERO_BREAKS_ALL,
    TUTORIAL_STATE_SAY_ZERO_LAST,
    TUTORIAL_STATE_SAY_DECK_COST,
    TUTORIAL_STATE_SAY_STOCK_INTRO,
    TUTORIAL_STATE_SAY_STOCKING,
    TUTORIAL_STATE_SAY_STOCK_THREE,
    TUTORIAL_STATE_HELP_STOCK_CARDS,
    TUTORIAL_STATE_STOCK_CARDS,
    TUTORIAL_STATE_SAY_STOCK_SUM,
    TUTORIAL_STATE_HELP_USE_STOCK,
    TUTORIAL_STATE_USE_STOCK,
    TUTORIAL_STATE_SAY_SLEIGHTS,
    TUTORIAL_STATE_SAY_SLEIGHT_COST,
    TUTORIAL_STATE_LEON_FINISH,
    TUTORIAL_STATE_LEON_END
};

void TutorialQueueMessage(TutorialWork* work, u16 message, u32 nextState) {
    work->timer = 0;
    work->state = TUTORIAL_STATE_MESSAGE_DELAY;
    work->nextState = nextState;
    work->message = message;
}

void TutorialQueuePersistentMessage(TutorialWork* work, u16 message, u32 nextState) {
    work->timer = 0;
    work->state = TUTORIAL_STATE_PERSISTENT_DELAY;
    work->nextState = nextState;
    work->message = message;
}

void TutorialCloseMessage() {
    CloseMessageWindow();
}

void TutorialWait(TutorialWork* work, u16 count, u32 nextState) {
    work->timer = 0;
    work->state = TUTORIAL_STATE_WAIT;
    work->nextState = nextState;
    work->count = count;
}

void TutorialShowArrow(TutorialWork* work, u16 x, u16 y, u16 animId) {
    work->flags |= TUTORIAL_FLAG_SHOW_ARROW;
    work->arrowX = x;
    work->arrowY = y;
    AnimStart(&work->anim, animId, ANIM_FLAG_LOOP);
}

void TutorialHideArrow(TutorialWork* work) {
    work->flags &= ~TUTORIAL_FLAG_SHOW_ARROW;
}

void task_tutorial_0(TutorialWork* work, s32 kind) {
    gBg0Cnt = 0;
    SetupBg(0, 2, 28, 14);
    SetBgScroll(0, 0, 0);
    work->flags = 0;
    work->timer = 0;
    work->state = TUTORIAL_STATE_WAIT;
    work->count = 120;
    work->nextState = kind == 0 ? TUTORIAL_STATE_SAY_FRIENDS_ARE_CARDS : TUTORIAL_STATE_LEON_START;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CONTROL;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_JUMP;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_DODGE;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_USE;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_SELECT;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_STOCK;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_STOCK_USE;
    gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_LIST_SWITCH;
    work->tiles = AllocObjTiles(0x100, gTutorialArrowTiles);
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    AnimInit(&work->anim, gTutorialArrowAnims, gTutorialArrowFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    SeedRandom(2);
}

s32 task_tutorial_1(TutorialWork* work) {
    { /* TEMPDBG */
        extern void PortLog(const char* format, ...);
        static u32 last = 0xFFFF;
        if (work->state != last) { PortLog("tut state %u frame %u", (unsigned)work->state, (unsigned)gFrameCounter); last = work->state; }
    }
    switch (work->state) {
    case TUTORIAL_STATE_WAIT:
        if (work->timer > work->count) {
            work->state = work->nextState;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case TUTORIAL_STATE_MESSAGE_DELAY:
        gBtlWork->hitStop = 8;

        if (work->timer > 20) {
            work->state = TUTORIAL_STATE_MESSAGE;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case TUTORIAL_STATE_MESSAGE:
        gBtlWork->hitStop = 8;

        if (work->timer == 0) {
            if (BgFxIsActive()) {
                break;
            }

            TutorialOpenMessage(work->message);
            work->timer++;
            break;
        }

        if (IsMessageWindowOpen()) {
            break;
        }

        TutorialRestoreBgMode();
        work->state = work->nextState;
        work->timer = 0;
        break;
    case TUTORIAL_STATE_PERSISTENT_DELAY:
        gBtlWork->hitStop = 8;

        if (work->timer > 20) {
            work->state = TUTORIAL_STATE_PERSISTENT_MESSAGE;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case TUTORIAL_STATE_PERSISTENT_MESSAGE:
        gBtlWork->hitStop = 8;

        if (BgFxIsActive()) {
            break;
        }

        TutorialOpenPersistentMessage(work->message);
        work->state = work->nextState;
        work->timer = 0;
        break;
    case TUTORIAL_STATE_SAY_FRIENDS_ARE_CARDS:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_00, TUTORIAL_STATE_DROP_FRIEND_CARD);
        break;
    case TUTORIAL_STATE_DROP_FRIEND_CARD:
        if (work->timer == 0) {
            CreateFriendCardTask(&gBtlWork->taskPools[0], 320, 0x181, 0, 1);
        }

        if (work->timer > 120) {
            work->state = TUTORIAL_STATE_SAY_PICK_UP_FRIENDS;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case TUTORIAL_STATE_SAY_PICK_UP_FRIENDS:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_01, TUTORIAL_STATE_HELP_MOVE);
        break;
    case TUTORIAL_STATE_HELP_MOVE:
        TutorialQueuePersistentMessage(work, CARD_MSG_ROBE_TUTORIAL_02, TUTORIAL_STATE_PICK_UP_FRIEND_CARD);
        break;
    case TUTORIAL_STATE_PICK_UP_FRIEND_CARD:
        if (work->timer == 0) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CONTROL;
        }

        if (!(gBtlWork->flags & 0x20000000000ULL)) {
            work->timer++;
            break;
        }

        TutorialCloseMessage();
        gBtlWork->flags &= ~0x20000000000ULL;
        work->state = TUTORIAL_STATE_SELECT_FRIEND_CARD;
        work->timer = 0;
        break;
    case TUTORIAL_STATE_SELECT_FRIEND_CARD:
        if (work->timer > 20) {
            RequestSoraPrevCard();
            TutorialWait(work, 30, TUTORIAL_STATE_SAY_CARDS_ON_TOP);
        } else {
            work->timer++;
        }

        break;
    case TUTORIAL_STATE_SAY_CARDS_ON_TOP:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_03, TUTORIAL_STATE_HELP_USE_FRIEND_CARD);
        break;
    case TUTORIAL_STATE_HELP_USE_FRIEND_CARD:
        TutorialQueuePersistentMessage(work, CARD_MSG_ROBE_TUTORIAL_04, TUTORIAL_STATE_USE_FRIEND_CARD);
        break;
    case TUTORIAL_STATE_USE_FRIEND_CARD:
        if (work->timer == 10) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CARD_USE;
        }

        if (!(gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION)) {
            work->timer++;
            break;
        }

        TutorialCloseMessage();
        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_USE;
        work->state = TUTORIAL_STATE_SAY_CARDS_REAPPEAR;
        work->timer = 0;
        break;
    case TUTORIAL_STATE_SAY_CARDS_REAPPEAR:
        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            break;
        }

        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_05, TUTORIAL_STATE_SAY_CARDS_RULE);
        break;
    case TUTORIAL_STATE_SAY_CARDS_RULE:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_06, TUTORIAL_STATE_HELP_JUMP_AND_DODGE);
        break;
    case TUTORIAL_STATE_HELP_JUMP_AND_DODGE:
        TutorialQueuePersistentMessage(work, CARD_MSG_ROBE_TUTORIAL_07, TUTORIAL_STATE_JUMP_AND_DODGE);
        break;
    case TUTORIAL_STATE_JUMP_AND_DODGE:
        if (work->timer == 0) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_DODGE;
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_JUMP;
        }

        if (!(gBtlWork->flags & BTL_FLAG_DODGE_ROLL_DONE)) {
            work->timer++;
            break;
        }

        if (!(gBtlWork->flags & BTL_FLAG_JUMP_LANDED)) {
            work->timer++;
            break;
        }

        TutorialCloseMessage();
        gBtlWork->flags &= ~BTL_FLAG_DODGE_ROLL_DONE;
        gBtlWork->flags &= ~BTL_FLAG_JUMP_LANDED;
        TutorialWait(work, 15, TUTORIAL_STATE_SAY_MOVE_THEN_USE);
        break;
    case TUTORIAL_STATE_SAY_MOVE_THEN_USE:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_08, TUTORIAL_STATE_HELP_USE_THREE_CARDS);
        break;
    case TUTORIAL_STATE_HELP_USE_THREE_CARDS:
        TutorialQueuePersistentMessage(work, CARD_MSG_ROBE_TUTORIAL_09, TUTORIAL_STATE_USE_THREE_CARDS);
        break;
    case TUTORIAL_STATE_USE_THREE_CARDS:
        if (work->timer == 0) {
            work->count = 0;
            work->flags &= ~TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
        }

        if (work->timer == 10) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CARD_USE;
        }

        if (work->flags & TUTORIAL_FLAG_CARD_ACTION_ACTIVE) {
            if (!(gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION)) {
                if (work->count == 0) {
                    TutorialCloseMessage();
                }

                work->flags &= ~TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
                work->count++;
            }
        } else if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            work->flags |= TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
        }

        if (work->count <= 2) {
            work->timer++;
            break;
        }

        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_USE;
        TutorialWait(work, 30, TUTORIAL_STATE_SAY_CARDS_DISAPPEAR);
        break;
    case TUTORIAL_STATE_SAY_CARDS_DISAPPEAR:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_10, TUTORIAL_STATE_SAY_CARDS_RUN_OUT);
        break;
    case TUTORIAL_STATE_SAY_CARDS_RUN_OUT:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_11, TUTORIAL_STATE_SAY_USE_UP_CARDS);
        break;
    case TUTORIAL_STATE_SAY_USE_UP_CARDS:
        TutorialShowArrow(work, 14, 90, 0);
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_12, TUTORIAL_STATE_USE_UP_CARDS);
        break;
    case TUTORIAL_STATE_USE_UP_CARDS:
        if (work->timer == 0) {
            TutorialHideArrow(work);
        }

        if (work->timer == 10) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CARD_USE;
        }

        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            work->timer++;
            break;
        }

        if (!IsSoraReloadCardSelected()) {
            work->timer++;
            break;
        }

        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_USE;
        TutorialWait(work, 60, TUTORIAL_STATE_SAY_NO_CARDS_LEFT);
        break;
    case TUTORIAL_STATE_SAY_NO_CARDS_LEFT:
        TutorialShowArrow(work, 14, 90, 0);
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_13, TUTORIAL_STATE_SAY_FOCUS);
        break;
    case TUTORIAL_STATE_SAY_FOCUS:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_14, TUTORIAL_STATE_HELP_RELOAD);
        break;
    case TUTORIAL_STATE_HELP_RELOAD:
        TutorialHideArrow(work);
        TutorialQueuePersistentMessage(work, CARD_MSG_ROBE_TUTORIAL_15, TUTORIAL_STATE_RELOAD);
        break;
    case TUTORIAL_STATE_RELOAD:
        if (work->timer == 10) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CARD_USE;
        }

        if (gBtlWork->flags & BTL_FLAG_RELOADING) {
            work->timer++;
            break;
        }

        if (IsSoraReloadCardSelected()) {
            work->timer++;
            break;
        }

        TutorialCloseMessage();
        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_USE;
        TutorialWait(work, 30, TUTORIAL_STATE_SAY_CARDS_RETURNED);
        break;
    case TUTORIAL_STATE_SAY_CARDS_RETURNED:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_16, TUTORIAL_STATE_SAY_CARDS_LIMITED);
        break;
    case TUTORIAL_STATE_SAY_CARDS_LIMITED:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_17, TUTORIAL_STATE_HELP_CYCLE_CARDS);
        break;
    case TUTORIAL_STATE_HELP_CYCLE_CARDS:
        TutorialQueuePersistentMessage(work, CARD_MSG_ROBE_TUTORIAL_18, TUTORIAL_STATE_CYCLE_CARDS);
        break;
    case TUTORIAL_STATE_CYCLE_CARDS:
        if (work->timer == 0) {
            work->count = 0;
            work->inputCooldown = 0;
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CARD_SELECT;
        }

        if (work->inputCooldown <= 0) {
            if (GetKeysPressed() & (L_BUTTON | R_BUTTON)) {
                work->inputCooldown = 10;
                work->count++;
            }
        } else {
            work->inputCooldown--;
        }

        if (work->count <= 6) {
            work->timer++;
            break;
        }

        work->timer = 0;
        work->state = TUTORIAL_STATE_SAY_CARD_CATEGORIES;
        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_SELECT;
        TutorialCloseMessage();
        break;
    case TUTORIAL_STATE_SAY_CARD_CATEGORIES:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_19, TUTORIAL_STATE_SAY_CATEGORY_CONTENTS);
        break;
    case TUTORIAL_STATE_SAY_CATEGORY_CONTENTS:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_20, TUTORIAL_STATE_HELP_SWITCH_CATEGORY);
        break;
    case TUTORIAL_STATE_HELP_SWITCH_CATEGORY:
        TutorialQueuePersistentMessage(work, CARD_MSG_ROBE_TUTORIAL_21, TUTORIAL_STATE_SWITCH_CATEGORY);
        break;
    case TUTORIAL_STATE_SWITCH_CATEGORY:
        if (work->timer == 0) {
            work->count = 0;
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_LIST_SWITCH;
        }

        if (GetKeysPressed() & SELECT_BUTTON) {
            work->count++;
        }

        if (work->count <= 1) {
            work->timer++;
            break;
        }

        TutorialCloseMessage();
        work->timer = 0;
        work->state = TUTORIAL_STATE_SAY_ATTACK_OR_DEFEND;
        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_LIST_SWITCH;
        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_SELECT;
        break;
    case TUTORIAL_STATE_SAY_ATTACK_OR_DEFEND:
        TutorialQueueMessage(work, CARD_MSG_ROBE_TUTORIAL_22, TUTORIAL_STATE_ROBE_FINISH);
        break;
    case TUTORIAL_STATE_ROBE_FINISH:
        TutorialWait(work, 0x50, TUTORIAL_STATE_ROBE_END);
        break;
    case TUTORIAL_STATE_LEON_START:
        TutorialWait(work, 30, TUTORIAL_STATE_SAY_CARD_VALUES);
        break;
    case TUTORIAL_STATE_SAY_CARD_VALUES:
        TutorialShowArrow(work, 48, 144, 1);
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_00, TUTORIAL_STATE_PRACTICE_BREAKS);
        break;
    case TUTORIAL_STATE_PRACTICE_BREAKS:
        if (work->timer == 0) {
            TutorialHideArrow(work);
            work->flags &= ~TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
            work->count = 0;
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CONTROL;
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_JUMP;
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_DODGE;
            gBtlWork->flags |= 0x100000ULL;
            gBtlWork->flags |= 0x20000000000ULL;
        }

        if (work->timer == 10) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_CARD_USE;
        }

        if (work->flags & TUTORIAL_FLAG_CARD_ACTION_ACTIVE) {
            if (!(gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION)) {
                work->flags &= ~TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
                work->count++;
            }
        } else if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            work->flags |= TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
        }

        if (work->count <= 6) {
            work->timer++;
            break;
        }

        work->timer = 0;
        work->state = TUTORIAL_STATE_PRACTICE_END;
        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_CARD_USE;
        gBtlWork->flags &= ~0x100000ULL;
        gBtlWork->flags &= ~0x20000000000ULL;
        break;
    case TUTORIAL_STATE_PRACTICE_END:
        if (work->timer > 30) {
            work->timer = 0;
            work->state = TUTORIAL_STATE_SAY_CARD_BREAK;
        } else {
            work->timer++;
        }

        break;
    case TUTORIAL_STATE_SAY_CARD_BREAK:
        if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            break;
        }

        if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
            break;
        }

        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_01, TUTORIAL_STATE_SAY_DEFLECTED);
        break;
    case TUTORIAL_STATE_SAY_DEFLECTED:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_02, TUTORIAL_STATE_SAY_ZERO_SPECIAL);
        break;
    case TUTORIAL_STATE_SAY_ZERO_SPECIAL:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_03, TUTORIAL_STATE_SAY_ZERO_BREAKS_ALL);
        break;
    case TUTORIAL_STATE_SAY_ZERO_BREAKS_ALL:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_04, TUTORIAL_STATE_SAY_ZERO_LAST);
        break;
    case TUTORIAL_STATE_SAY_ZERO_LAST:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_05, TUTORIAL_STATE_SAY_DECK_COST);
        break;
    case TUTORIAL_STATE_SAY_DECK_COST:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_06, TUTORIAL_STATE_SAY_STOCK_INTRO);
        break;
    case TUTORIAL_STATE_SAY_STOCK_INTRO:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_07, TUTORIAL_STATE_SAY_STOCKING);
        break;
    case TUTORIAL_STATE_SAY_STOCKING:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_08, TUTORIAL_STATE_SAY_STOCK_THREE);
        break;
    case TUTORIAL_STATE_SAY_STOCK_THREE:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_09, TUTORIAL_STATE_HELP_STOCK_CARDS);
        break;
    case TUTORIAL_STATE_HELP_STOCK_CARDS:
        TutorialQueuePersistentMessage(work, CARD_MSG_LEON_TUTORIAL_10, TUTORIAL_STATE_STOCK_CARDS);
        break;
    case TUTORIAL_STATE_STOCK_CARDS:
        if (work->timer == 0) {
            work->count = 0;
        }

        if (work->timer == 10) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_STOCK;
        }

        if (work->count == 0 && GetSoraStockCount() != 0) {
            TutorialCloseMessage();
            work->count++;
        }

        if (GetSoraStockCount() <= 2) {
            work->timer++;
            break;
        }

        gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_STOCK;
        work->timer = 0;
        work->state = TUTORIAL_STATE_SAY_STOCK_SUM;
        break;
    case TUTORIAL_STATE_SAY_STOCK_SUM:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_11, TUTORIAL_STATE_HELP_USE_STOCK);
        break;
    case TUTORIAL_STATE_HELP_USE_STOCK:
        TutorialQueuePersistentMessage(work, CARD_MSG_LEON_TUTORIAL_12, TUTORIAL_STATE_USE_STOCK);
        break;
    case TUTORIAL_STATE_USE_STOCK:
        if (work->timer == 0) {
            gBtlWork->flags |= 0x20000000000ULL;
            work->flags &= ~TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
            work->count = 0;
        }

        if (work->timer == 10) {
            gBtlWork->flags &= ~BTL_FLAG_TUTORIAL_NO_STOCK_USE;
        }

        if (work->flags & TUTORIAL_FLAG_CARD_ACTION_ACTIVE) {
            if (!(gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION)) {
                work->flags &= ~TUTORIAL_FLAG_CARD_ACTION_ACTIVE;
                work->count++;
            }
        } else if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
            work->flags |= TUTORIAL_FLAG_CARD_ACTION_ACTIVE;

            if (work->count == 0) {
                TutorialCloseMessage();
            }
        }

        if (work->count > 0) {
            gBtlWork->flags |= BTL_FLAG_TUTORIAL_NO_STOCK_USE;
            gBtlWork->flags &= ~0x20000000000ULL;
            TutorialWait(work, 30, TUTORIAL_STATE_SAY_SLEIGHTS);
        } else {
            work->timer++;
        }

        break;
    case TUTORIAL_STATE_SAY_SLEIGHTS:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_13, TUTORIAL_STATE_SAY_SLEIGHT_COST);
        break;
    case TUTORIAL_STATE_SAY_SLEIGHT_COST:
        TutorialQueueMessage(work, CARD_MSG_LEON_TUTORIAL_14, TUTORIAL_STATE_LEON_FINISH);
        break;
    case TUTORIAL_STATE_LEON_FINISH:
        TutorialWait(work, 0x50, TUTORIAL_STATE_LEON_END);
        break;
    case TUTORIAL_STATE_ROBE_END:
    case TUTORIAL_STATE_LEON_END:
        gBtlWork->flags |= BTL_FLAG_BATTLE_OVER;
        break;
    }

    return 1;
}

void task_tutorial_2(TutorialWork* work) {
    void* gfx;
    u16 x;
    u16 y;
    s32 wave;

    if (work->flags & TUTORIAL_FLAG_SHOW_ARROW) {
        gfx = AnimUpdate(&work->anim);

        if (work->anim.animId == 0) {
            x = work->arrowX;
            wave = SIN(gFrameCounter << 3);
            y = (wave >> 7) + work->arrowY;
        } else {
            wave = SIN(gFrameCounter << 3);
            x = (wave >> 7) + work->arrowX;
            y = work->arrowY;
        }

#ifdef PLATFORM_PC
        /* The arrow points at HUD elements: follow their anchored corner. */
        PortHudBeginAt(work->arrowX, work->arrowY);
#endif
        DrawSprite(x, y, gfx, work->tiles, work->palette, NULL, SPRITE_FLAG_NO_MOSAIC, 0);
#ifdef PLATFORM_PC
        PortHudEnd();
#endif
    }
}

void task_tutorial_3(TutorialWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    SeedRandom(gFrameCounter);
}

TaskDesc gTaskDescTutorial = {
    "task_tutorial",
    (TaskInitFunc)task_tutorial_0,
    (TaskUpdateFunc)task_tutorial_1,
    (TaskDrawFunc)task_tutorial_2,
    (TaskDestroyFunc)task_tutorial_3,
    sizeof(TutorialWork),
};
