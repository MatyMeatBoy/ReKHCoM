/**
 * mode_event.c
 * Event Mode
 */

#include "mode_test.h"
#include "card_ids.h"
#include "gba/keys.h"
#include "malloc.h"
#include "fade.h"
#include "player_progression.h"
#include "songs.h"
#include "card_api.h"
#include "display.h"
#include "event_background_types.h"
#include "event_index_data.h"
#include "evt_types.h"
#include "game_state.h"
#include "key.h"
#include "m4a_song.h"
#include "map_api.h"
#include "map_runtime.h"
#include "mode.h"
#include "mode_battle_data.h"
#include "mode_event.h"
#include "msg_types.h"
#include "player_progression_types.h"
#include "registration_data.h"
#include "save_api.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "msg_api.h"
#include "battle_ids.h"
#include "card_message_data.h"
#include "event_ids.h"
#include "jiminy_records_index_data.h"
#include "mode_movie.h"
#include "world_types.h"

static TaskPool sEventTaskPool;
static u8 sEventPaused;
static u32 sEventId;
static u8 sEventEndStep;

#ifdef PLATFORM_PC
/* Derived from game state, including after loading a checkpoint. */
int PortEventBackdropProfile(void) {
    return gEventState != NULL ? gEventState->eventId : -1;
}
/* The opening's first 256x256 panel contains Riku's one circle. The
   adjacent panels are other artwork/empty space, not a repeating room. */
int PortEventBackdropPanel(void) {
    return gEventState != NULL && gEventState->eventId == EVENT_149_RIKU_B12F_OPNING ? 3 : -1;
}
int PortEventTiledBackdrop(void) {
    return gEventState != NULL && (gGameState.flags & GAME_FLAG_RIKU) &&
        gEventState->eventId != EVENT_149_RIKU_B12F_OPNING ? 3 : -1;
}
#endif

enum EventEndStep {
    EVENT_END_STEP_SHOW_MESSAGE,
    EVENT_END_STEP_WAIT_MESSAGE
};

void Event_0(s32 arg) {
    EvtArg cfg;
    EventBackgroundDef* bg;

    gEventState = EwramAlloc(sizeof(EventState));
    bg = gEventBackgroundDefs[arg & 0x7FFF];
    gBldCnt = 0;
    gBldAlpha = 0;
    sEventId = arg;
    sEventPaused = FALSE;

    if (bg != NULL) {
        if (bg->isAffine != 0) {
            SetBgMode1();
            SetupBg(0, 3, 31, 14);
            SetupBg(1, 0, 16, 0);
            SetupBg(2, 0, 17, 0);
            SetupBg(3, 0, 18, 0);
            SetBgPriority(0, 0);
            SetBgPriority(1, 1);
            SetBgPriority(2, 2);
            SetBgPriority(3, 3);
        } else {
            SetBgMode0();
            SetupBg(0, 3, 31, 14);
            SetupBg(1, 0, 21, 0);
            SetupBg(2, 0, 22, 0);
            SetupBg(3, 0, 23, 0);
            SetBgPriority(0, 0);
            SetBgPriority(1, 1);
            SetBgPriority(2, 2);
            SetBgPriority(3, 3);
            DisableBg(0);
            DisableBg(1);
            DisableBg(2);
            DisableBg(3);
        }
    } else {
        SetBgMode0();
        SetBgMode0();
        SetupBg(0, 0, 22, 0);
        SetupBg(1, 0, 24, 0);
        SetupBg(2, 2, 28, 14);
        SetBgPriority(0, 3);
        SetBgPriority(1, 2);
        SetBgPriority(2, 1);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
    }

    cfg.eventId = arg;
    gEventState->eventId = arg & 0x7FFF;

    if (arg & 0x8000) {
        cfg.fromGame = FALSE;
    } else {
        cfg.fromGame = TRUE;
    }

    // @bug? Should mask with 0x7FFF.
#ifdef PLATFORM_PC
    /* From the debug event select (bit 0x8000) the original reads 0x8000 entries past the table: a ROM word that is
       not a pointer, which crashes the relocatable builds (Android 64, Switch, Vita). The game itself never sets it. */
    if (gEventSequenceDefs[sEventId & 0x7FFF]->keyframes->flags & CAMERA_KEYFRAME_FLAG_FADE_IN_WHITE) {
#else
    if (gEventSequenceDefs[sEventId & 0x8000]->keyframes->flags & CAMERA_KEYFRAME_FLAG_FADE_IN_WHITE) {
#endif
        FadeStartIn(FADE_MODE_WHITE, 999);
    }

    TaskPoolInit(&sEventTaskPool, 2);
    TaskCreate(&sEventTaskPool, &gTaskDescEventSeq, &cfg);
    ResetMessageWindowFlags();
    sEventEndStep = EVENT_END_STEP_SHOW_MESSAGE;
}

void EventDebugUpdate() {
    if (gEventState == NULL) {
        ModeRequest(&gModeEventselect, 0);
    }

    if (!sEventPaused) {
        TaskPoolUpdate(&sEventTaskPool);
    } else if (GetKeysRepeat() & SELECT_BUTTON) {
        TaskPoolUpdate(&sEventTaskPool);
    }

    TaskPoolDraw(&sEventTaskPool);

    if (!gEventState->running) {
        if (sEventEndStep == EVENT_END_STEP_SHOW_MESSAGE) {
            ShowEventEndMessage();
            sEventEndStep = EVENT_END_STEP_WAIT_MESSAGE;
        }

        if (sEventEndStep == EVENT_END_STEP_WAIT_MESSAGE) {
            if (!IsMessageWindowOpen()) {
                ModeRequest(&gModeEventselect, 0);
            }
        }
    }

    if (GetKeysPressed() & START_BUTTON) {
        sEventPaused = FALSE;
    }
}

void EventUpdate() {
    const EventSequenceDef* seqDef = gEventSequenceDefs[sEventId];
    UpdatePlayTime();
    TaskPoolUpdate(&sEventTaskPool);
    TaskPoolDraw(&sEventTaskPool);

    if (gEventState->running) {
        return;
    }

    if (sEventEndStep == EVENT_END_STEP_SHOW_MESSAGE) {
        ShowEventEndMessage();
        sEventEndStep = EVENT_END_STEP_WAIT_MESSAGE;
    }

    if (sEventEndStep != EVENT_END_STEP_WAIT_MESSAGE) {
        return;
    }

    if (IsMessageWindowOpen()) {
        return;
    }

    SetJiminyFlagsAfterEvent();

    if (IsMessageWindowAnswerYes() == TRUE) {
        GrantRewardsAfterEvent();
        SaveAfterEvent();
        return;
    }

    SetFriendsAfterEvent();
    GrantRewardsAfterEvent();
    UnlockCardKindsAfterEvent();
    EnterExitHallAfterEvent();

    if (gEventState->askedYesNo) {
        if (!gEventState->answerYes) {
            if (!HandleNoAnswerAfterEvent()) {
                AdvanceFloorStory();
                RequestMapMode();
            }
        } else {
            HandleYesAnswerAfterEvent();
        }

        return;
    }

    if (seqDef->toMap) {
        if (sEventId == EVENT_150_RIKU_B12F_ENTRANCE) {
            ModeRequest(&gModeWorldselect, 0);
        } else {
            AdvanceFloorStory();
            RequestMapMode();
        }

        return;
    }

    if (seqDef->nextEvent != EVENT_NONE) {
        switch (seqDef->nextEvent) {
        case EVENT_012_2F_ENTRANCE:
        case EVENT_014_2F_DEMO:
        case EVENT_017_3F_DEMO:
        case EVENT_020_4F_DEMO:
        case EVENT_023_5F_DEMO:
        case EVENT_028_6F_DEMO:
        case EVENT_032_8F_ENTRANCE:
        case EVENT_035_8F_DEMO:
        case EVENT_038_9F_DEMO:
        case EVENT_042_10F_DEMO:
        case EVENT_047_11F_DEMO_1:
        case EVENT_050_11F_GOAL_3:
        case EVENT_053_12F_DESTINY_ISLAND_E0:
        case EVENT_058_12F_GOAL:
        case EVENT_068_13F_CASTLE_OBLIVION_LAST3:
        case EVENT_157_RIKU_B12F_DEMO:
        case EVENT_165_RIKU_B8F_DEMO:
        case EVENT_168_RIKU_B6F_DEMO:
        case EVENT_169_RIKU_B5F_DEMO:
        case EVENT_175_RIKU_B4F_DEMO:
        case EVENT_184_RIKU_B3F_DEMO:
        case EVENT_191_RIKU_B1F_ENTRANCE:
            AdvanceFloorStory();
            RequestMapMode();
            break;
        case EVENT_061_13F_ENTRANCE:
            AdvanceToExitHall();
            RequestMapMode();
            break;
        default:
            ModeRequest(&gModeEvent, seqDef->nextEvent);
            break;
        }

        return;
    }

    if (seqDef->startsBattle) {
        if (seqDef->battleId == BATTLE_EVENT_HALLOWEEN_TOWN) {
            gGameState.battleStage = BATTLE_STAGE_HALLOWEEN_TOWN;
        } else if (seqDef->battleId == BATTLE_CARD_SOLDIERS) {
            gGameState.battleStage = BATTLE_STAGE_WONDERLAND;
        } else if (seqDef->battleId == BATTLE_SHADOW_100) {
            gGameState.battleStage = BATTLE_STAGE_MONSTRO;
        } else if (seqDef->battleId == BATTLE_EVENT_AGRABAH_1) {
            gGameState.battleStage = BATTLE_STAGE_AGRABAH;
        } else if (seqDef->battleId == BATTLE_EVENT_AGRABAH_2) {
            gGameState.battleStage = BATTLE_STAGE_AGRABAH;
        }

        ModeRequest(&gModeBattle, seqDef->battleId);
        return;
    }

    if (seqDef->toTitle) {
        FadeStartOut(FADE_MODE_BLACK, 16);
        ModeRequest(&gModeTitle, 0);
        return;
    }

    if (seqDef->toCopyright) {
        ModeRequest(&gModeCopyright1, 0);
        return;
    }

    if (seqDef->toMapFld) {
        AdvanceFloorStory();
        ModeRequest(&gModeMapFld, 0);
        return;
    }

    if (seqDef->exitCode != EVENT_EXIT_NONE) {
        switch (seqDef->exitCode) {
        case EVENT_EXIT_BATTLE_TUTORIAL_0:
            ModeRequest(&gModeBattle, BATTLE_TUTORIAL_0);
            break;
        case EVENT_EXIT_BATTLE_TUTORIAL_1:
            ModeRequest(&gModeBattle, BATTLE_TUTORIAL_1);
            break;
        case EVENT_EXIT_MOVIE_ENDING:
            ModeRequestHeapReset(&gModeMovie, MOVIE_ENDING);
            break;
        case EVENT_EXIT_WORLD_SELECT:
            gGameState.availableWorlds = 512;
            ModeRequest(&gModeWorldselect, 0);
            break;
        case EVENT_EXIT_MOVIE_6F_GOAL:
            ModeRequestHeapReset(&gModeMovie, MOVIE_6F_GOAL);
            break;
        case EVENT_EXIT_MOVIE_12F_E2:
            ModeRequestHeapReset(&gModeMovie, MOVIE_12F_E2);
            break;
        case EVENT_EXIT_MOVIE_RIKU_ENDING:
            ModeRequestHeapReset(&gModeMovie, MOVIE_RIKU_ENDING);
            break;
        case EVENT_EXIT_MAP_1F:
        case EVENT_EXIT_MAP_4F:
        case EVENT_EXIT_MAP_6F:
            AdvanceFloorStory();
            RequestMapMode();
            break;
        default:
            ModeRequest(&gModeDummy, seqDef->exitCode);
            break;
        }

        return;
    }

    if (seqDef->unk_2A != 0) {
        AdvanceFloorStory();
        RequestMapMode();
    } else if (seqDef->world != WORLD_NONE) {
        AdvanceFloorStory();
        RequestMapMode();
    } else if (seqDef->poohLevel != POOH_LEVEL_NONE) {
        if (seqDef->poohLevel == 0) {
            AdvanceFloorStory();
            ModeRequest(&gModePooh, 0);
        } else if (seqDef->poohLevel <= 6) {
            ModeRequest(&gModePooh, 1);
        }
    }
}

void Event_2() {
#ifdef PLATFORM_PC
    extern void PortHdSceneForget(void);
    PortHdSceneForget(); /* the next event offers its HD scene again, wherever its task lands */
#endif
    TaskPoolDestroy(&sEventTaskPool);
    EwramFree(gEventState);
    gEventState = NULL;
}

void RequestEventMode(u16 eventId) {
    ModeRequest(&gModeEvent, eventId);
}

void ShowEventEndMessage() {
    SetBackdropColor(0, 0, 0);

    switch (sEventId & 0x7FFF) {
    case EVENT_041_10F_GOAL_2:
    case EVENT_049_11F_GOAL_2:
    case EVENT_176_RIKU_B3F_ENTRANCE:
    case EVENT_185_RIKU_B2F_ENTRANCE:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_WORLD_CARD);
        break;
    case EVENT_034_8F_GOAL_2:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_LEARNED_WARPINATOR);
        break;
    case EVENT_088_HALLOWEEN_TOWN_E0_2:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_LEARNED_TERROR);
        break;
    case EVENT_136_100ACREWOOD_LV1:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_LEARNED_CONFUSE);
        break;
    case EVENT_137_100ACREWOOD_LV2:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_SPELLBINDER);
        break;
    case EVENT_139_100ACREWOOD_LV3:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_ELIXIR);
        break;
    case EVENT_140_100ACREWOOD_LV4:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_LEARNED_BIND);
        break;
    case EVENT_141_100ACREWOOD_LV5:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_LEARNED_IDYLL_ROMP);
        break;
    case EVENT_142_100ACREWOOD_LV6:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_LEARNED_SYNCHRO);
        break;
    case EVENT_061_13F_ENTRANCE:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        m4aSongNumStart(SONG_SYS_ITEMGET);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_LEARNED_TRINITY_LIMIT);
        break;
    case EVENT_126_COLISEUM_END:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_CLOUD);
        break;
    case EVENT_114_AGRABAH_END:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_GENIE);
        break;
    case EVENT_057_12F_DESTINY_ISLAND_E3:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_OATHKEEPER);
        break;
    case EVENT_143_100ACREWOOD_END_1ST_COMP:
    case EVENT_144_100ACREWOOD_END_1ST_NO:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_BAMBI);
        break;
    case EVENT_145_100ACREWOOD_END_COMP:
    case EVENT_146_100ACREWOOD_END_NO:
    case EVENT_147_100ACREWOOD_END_COMPCOMP:
    case EVENT_148_100ACREWOOD_END_SORAONLY:
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_EVENT_SAVE_CONFIRM);
        break;
    case EVENT_003_1F_TRAVERSE_TOWN_E0_2:
    case EVENT_044_11F_TWILIGHT_TOWN_E0:
    case EVENT_053_12F_DESTINY_ISLAND_E0:
    case EVENT_074_MONSTORO_E0:
    case EVENT_094_WONDERLAND_E0:
    case EVENT_101_ATLANTICA_E0:
    case EVENT_108_AGRABAH_E0_2:
    case EVENT_115_NEVERLAND_E0:
    case EVENT_120_COLISEUM_E0:
    case EVENT_129_HOLLOWBASTION_E0:
    case EVENT_151_RIKU_B12F_E0:
    case EVENT_177_RIKU_B3F_E0:
    case EVENT_186_RIKU_B2F_E0:
    case EVENT_192_RIKU_B1F_E0:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_KEY_OF_BEGINNINGS);
        break;
    case EVENT_054_12F_DESTINY_ISLAND_E1:
    case EVENT_075_MONSTORO_E1:
    case EVENT_089_HALLOWEEN_TOWN_E1:
    case EVENT_096_WONDERLAND_E1_2:
    case EVENT_102_ATLANTICA_E1:
    case EVENT_109_AGRABAH_E1:
    case EVENT_116_NEVERLAND_E1:
    case EVENT_121_COLISEUM_E1:
    case EVENT_130_HOLLOWBASTION_E1:
    case EVENT_152_RIKU_B12F_E1:
    case EVENT_188_RIKU_B2F_E1_2:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_KEY_OF_GUIDANCE);
        break;
    case EVENT_005_1F_TRAVERSE_TOWN_E1_2:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_SIMBA);
        break;
    case EVENT_006_1F_TRAVERSE_TOWN_E2:
    case EVENT_080_MONSTORO_E2_5:
    case EVENT_090_HALLOWEEN_TOWN_E2:
    case EVENT_097_WONDERLAND_E2:
    case EVENT_103_ATLANTICA_E2:
    case EVENT_111_AGRABAH_E2_2:
    case EVENT_117_NEVERLAND_E2:
    case EVENT_123_COLISEUM_E2_2:
    case EVENT_131_HOLLOWBASTION_E2:
    case EVENT_153_RIKU_B12F_E2:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_KEY_TO_TRUTH);
        break;
    case EVENT_119_NEVERLAND_END:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_TINKER_BELL);
        break;
    case EVENT_060_12F_GOAL_3:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_OBLIVION);
        break;
    case EVENT_027_6F_GOAL_3:
    case EVENT_156_RIKU_B12F_GOAL_2:
    case EVENT_166_RIKU_B7F_ENTRANCE:
        m4aSongNumStart(SONG_SYS_ITEMGET);
        DisableBg(0);
        DisableBg(1);
        DisableBg(2);
        DisableBg(3);
        FadeStartIn(FADE_MODE_BLACK, 1);
        CreateSysmsgwinTask(&sEventTaskPool, CARD_MSG_OBTAINED_WORLD_CARDS);
        break;
    }
}

void SetFriendsAfterEvent() {
    switch (sEventId) {
    case EVENT_000_1F_ENTRANCE_PART1:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_094_WONDERLAND_E0:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_074_MONSTORO_E0:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_088_HALLOWEEN_TOWN_E0_2:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK | FRIEND_FLAG_JACK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        LearnStock(LEARNED_STOCK_SURPRISE);
        break;
    case EVENT_093_HALLOWEEN_TOWN_END:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_107_AGRABAH_E0:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK | FRIEND_FLAG_ALADDIN);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        LearnStock(LEARNED_STOCK_SANDSTORM);
        break;
    case EVENT_114_AGRABAH_END:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_103_ATLANTICA_E2:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK | FRIEND_FLAG_ARIEL);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        LearnStock(LEARNED_STOCK_SPIRAL_WAVE);
        break;
    case EVENT_106_ATLANTICA_END:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_131_HOLLOWBASTION_E2:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK | FRIEND_FLAG_THE_BEAST);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        LearnStock(LEARNED_STOCK_FEROCIOUS_LUNGE);
        break;
    case EVENT_133_HOLLOWBASTION_END:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_117_NEVERLAND_E2:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_116_NEVERLAND_E1:
    case EVENT_118_NEVERLAND_E3:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK | FRIEND_FLAG_PETER_PAN);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        LearnStock(LEARNED_STOCK_HUMMINGBIRD);
        break;
    case EVENT_119_NEVERLAND_END:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_120_COLISEUM_E0:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_044_11F_TWILIGHT_TOWN_E0:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_059_12F_GOAL_2:
        gGameState.progression.friendFlags = (FRIEND_FLAG_GOOFY | FRIEND_FLAG_DONALD_DUCK);
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_052_12F_ENTRANCE:
    case EVENT_149_RIKU_B12F_OPNING:
        gGameState.progression.friendFlags = 0;
        LearnStock(LEARNED_STOCK_MAGIC);
        LearnStock(LEARNED_STOCK_GOOFY_CHARGE);
        LearnStock(LEARNED_STOCK_GOOFY_TORNADO);
        break;
    case EVENT_155_RIKU_B12F_GOAL:
        gGameState.progression.friendFlags = FRIEND_FLAG_THE_KING;
        LearnStock(LEARNED_STOCK_MM_MIRACLE);
        break;
    case EVENT_174_RIKU_B4F_GOAL_3:
    case EVENT_186_RIKU_B2F_E0:
        gGameState.progression.friendFlags = 0;
        break;
    case EVENT_185_RIKU_B2F_ENTRANCE:
    case EVENT_190_RIKU_B2F_GOAL:
        gGameState.progression.friendFlags = FRIEND_FLAG_THE_KING;
        break;
    case EVENT_156_RIKU_B12F_GOAL_2:
        gGameState.flags &= ~GAME_FLAG_DARK_POINTS_LOCKED;
        LearnStock(LEARNED_STOCK_DARK_BREAK);
        LearnStock(LEARNED_STOCK_DARK_FIRAGA);
        LearnStock(LEARNED_STOCK_DARK_AURA);
        break;
    }
}

void GrantRewardsAfterEvent() {
    switch (sEventId) {
    case EVENT_000_1F_ENTRANCE_PART1:
        gGameState.availableWorlds = 0x200;
        break;
    case EVENT_005_1F_TRAVERSE_TOWN_E1_2:
        ObtainCard(CARD_ID(CARD_SIMBA, 6));
        break;
    case EVENT_011_1F_GOAL_2:
        gGameState.availableWorlds = 61;
        break;
    case EVENT_027_6F_GOAL_3:
        gGameState.availableWorlds = 0x4C2;
        break;
    case EVENT_041_10F_GOAL_2:
        gGameState.availableWorlds = 0x800;
        break;
    case EVENT_049_11F_GOAL_2:
        gGameState.availableWorlds = 256;
        break;
    case EVENT_060_12F_GOAL_3:
        gGameState.availableWorlds = 0x1000;
        ObtainCard(CARD_ID(CARD_OBLIVION, 6));
        break;
    case EVENT_057_12F_DESTINY_ISLAND_E3:
        ObtainCard(CARD_ID(CARD_OATHKEEPER, 4));
        break;
    case EVENT_003_1F_TRAVERSE_TOWN_E0_2:
    case EVENT_044_11F_TWILIGHT_TOWN_E0:
    case EVENT_053_12F_DESTINY_ISLAND_E0:
    case EVENT_061_13F_ENTRANCE:
    case EVENT_074_MONSTORO_E0:
    case EVENT_088_HALLOWEEN_TOWN_E0_2:
    case EVENT_094_WONDERLAND_E0:
    case EVENT_101_ATLANTICA_E0:
    case EVENT_108_AGRABAH_E0_2:
    case EVENT_115_NEVERLAND_E0:
    case EVENT_120_COLISEUM_E0:
    case EVENT_129_HOLLOWBASTION_E0:
    case EVENT_151_RIKU_B12F_E0:
    case EVENT_177_RIKU_B3F_E0:
    case EVENT_186_RIKU_B2F_E0:
    case EVENT_192_RIKU_B1F_E0:
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_BEGINNINGS, 1));
        break;
    case EVENT_004_1F_TRAVERSE_TOWN_E1_1:
    case EVENT_054_12F_DESTINY_ISLAND_E1:
    case EVENT_075_MONSTORO_E1:
    case EVENT_089_HALLOWEEN_TOWN_E1:
    case EVENT_096_WONDERLAND_E1_2:
    case EVENT_102_ATLANTICA_E1:
    case EVENT_109_AGRABAH_E1:
    case EVENT_116_NEVERLAND_E1:
    case EVENT_121_COLISEUM_E1:
    case EVENT_130_HOLLOWBASTION_E1:
    case EVENT_152_RIKU_B12F_E1:
    case EVENT_188_RIKU_B2F_E1_2:
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_GUIDANCE, 1));
        break;
    case EVENT_006_1F_TRAVERSE_TOWN_E2:
    case EVENT_080_MONSTORO_E2_5:
    case EVENT_090_HALLOWEEN_TOWN_E2:
    case EVENT_097_WONDERLAND_E2:
    case EVENT_103_ATLANTICA_E2:
    case EVENT_111_AGRABAH_E2_2:
    case EVENT_117_NEVERLAND_E2:
    case EVENT_123_COLISEUM_E2_2:
    case EVENT_131_HOLLOWBASTION_E2:
    case EVENT_153_RIKU_B12F_E2:
        AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_TO_TRUTH, 1));
        break;
    case EVENT_114_AGRABAH_END:
        ObtainCard(CARD_ID(CARD_GENIE, 6));
        break;
    case EVENT_119_NEVERLAND_END:
        ObtainCard(CARD_ID(CARD_TINKER_BELL, 4));
        break;
    case EVENT_126_COLISEUM_END:
        ObtainCard(CARD_ID(CARD_CLOUD, 4));
        break;
    case EVENT_137_100ACREWOOD_LV2:
        ObtainCard(CARD_ID(CARD_SPELLBINDER, 4));
        break;
    case EVENT_139_100ACREWOOD_LV3:
        ObtainCard(CARD_ID(CARD_ELIXIR, 1));
        break;
    case EVENT_143_100ACREWOOD_END_1ST_COMP:
    case EVENT_144_100ACREWOOD_END_1ST_NO:
        ObtainCard(CARD_ID(CARD_BAMBI, 5));
        break;
    case EVENT_149_RIKU_B12F_OPNING:
        gGameState.availableWorlds = 128;
        break;
    case EVENT_156_RIKU_B12F_GOAL_2:
        gGameState.availableWorlds = 593;
        break;
    case EVENT_166_RIKU_B7F_ENTRANCE:
        gGameState.availableWorlds = 46;
        break;
    case EVENT_176_RIKU_B3F_ENTRANCE:
        gGameState.availableWorlds = 256;
        break;
    case EVENT_185_RIKU_B2F_ENTRANCE:
        gGameState.availableWorlds = 0x800;
        break;
    case EVENT_191_RIKU_B1F_ENTRANCE:
        gGameState.availableWorlds = 0x1000;
        break;
    }
}

void HandleYesAnswerAfterEvent() {
    const EventSequenceDef* seqDef = gEventSequenceDefs[sEventId];

    switch (sEventId) {
    case EVENT_068_13F_CASTLE_OBLIVION_LAST3:
        ModeRequest(&gModeEvent, EVENT_069_13F_CASTLE_OBLIVION_LAST4);
        break;
    case EVENT_083_MONSTORO_E3_FAILURE_1:
    case EVENT_084_MONSTORO_E3_FAILURE_2:
        gGameState.battleStage = BATTLE_STAGE_MONSTRO;
        ModeRequest(&gModeBattle, seqDef->battleId);
        break;
    }
}

u8 HandleNoAnswerAfterEvent() {
    switch (sEventId) {
    case EVENT_068_13F_CASTLE_OBLIVION_LAST3:
    case EVENT_083_MONSTORO_E3_FAILURE_1:
    case EVENT_084_MONSTORO_E3_FAILURE_2:
        RequestMapMode();
        return TRUE;
    }

    return FALSE;
}

void UnlockCardKindsAfterEvent() {
    switch (sEventId) {
    case EVENT_002_1F_TRAVERSE_TOWN_E0_1:
        SetCardKindObtained(OBTAINED_CARD_KINGDOM_KEY);
        break;
    case EVENT_136_100ACREWOOD_LV1:
        LearnStock(LEARNED_STOCK_CONFUSE);
        break;
    case EVENT_140_100ACREWOOD_LV4:
        LearnStock(LEARNED_STOCK_BIND);
        break;
    case EVENT_141_100ACREWOOD_LV5:
        LearnStock(LEARNED_STOCK_IDYLL_ROMP);
        break;
    case EVENT_142_100ACREWOOD_LV6:
        LearnStock(LEARNED_STOCK_SYNCHRO);
        break;
    case EVENT_034_8F_GOAL_2:
        LearnStock(LEARNED_STOCK_WARPINATOR);
        break;
    case EVENT_088_HALLOWEEN_TOWN_E0_2:
        LearnStock(LEARNED_STOCK_TERROR);
        break;
    case EVENT_108_AGRABAH_E0_2:
        SetCardKindObtained(OBTAINED_CARD_THREE_WISHES);
        break;
    case EVENT_074_MONSTORO_E0:
        SetCardKindObtained(OBTAINED_CARD_WISHING_STAR);
        break;
    case EVENT_120_COLISEUM_E0:
        SetCardKindObtained(OBTAINED_CARD_OLYMPIA);
        break;
    case EVENT_094_WONDERLAND_E0:
        SetCardKindObtained(OBTAINED_CARD_LADY_LUCK);
        break;
    case EVENT_101_ATLANTICA_E0:
        SetCardKindObtained(OBTAINED_CARD_CRABCLAW);
        break;
    case EVENT_115_NEVERLAND_E0:
        SetCardKindObtained(OBTAINED_CARD_FAIRY_HARP);
        break;
    case EVENT_129_HOLLOWBASTION_E0:
        SetCardKindObtained(OBTAINED_CARD_DIVINE_ROSE);
        break;
    case EVENT_087_HALLOWEEN_TOWN_E0:
        SetCardKindObtained(OBTAINED_CARD_PUMPKINHEAD);
        break;
    case EVENT_059_12F_GOAL_2:
        SetCardKindObtained(OBTAINED_CARD_OBLIVION);
        break;
    case EVENT_061_13F_ENTRANCE:
        LearnStock(LEARNED_STOCK_TRINITY_LIMIT);
        break;
    case EVENT_067_13F_CASTLE_OBLIVION_LAST2:
        SetCardKindObtained(OBTAINED_CARD_DIAMOND_DUST);
        SetCardKindObtained(OBTAINED_CARD_ONE_WINGED_ANGEL);
        break;
    }
}

void SaveAfterEvent() {
    switch (sEventId) {
    case EVENT_143_100ACREWOOD_END_1ST_COMP:
    case EVENT_144_100ACREWOOD_END_1ST_NO:
    case EVENT_145_100ACREWOOD_END_COMP:
    case EVENT_146_100ACREWOOD_END_NO:
    case EVENT_147_100ACREWOOD_END_COMPCOMP:
    case EVENT_148_100ACREWOOD_END_SORAONLY:
        if (gGameState.flags & GAME_FLAG_SECOND_FILE) {
            SaveWriteFileLarge(1);
        } else {
            SaveWriteFileLarge(0);
        }

        EnterExitHall();
        break;
    }
}

void EnterExitHallAfterEvent() {
    switch (sEventId) {
    case EVENT_143_100ACREWOOD_END_1ST_COMP:
    case EVENT_144_100ACREWOOD_END_1ST_NO:
    case EVENT_145_100ACREWOOD_END_COMP:
    case EVENT_146_100ACREWOOD_END_NO:
    case EVENT_147_100ACREWOOD_END_COMPCOMP:
    case EVENT_148_100ACREWOOD_END_SORAONLY:
        EnterExitHall();
        break;
    }
}

void SetJiminyFlagsAfterEvent() {
    switch (sEventId) {
    case EVENT_067_13F_CASTLE_OBLIVION_LAST2:
        SetJiminyFlag(JIMINY_RECORD_STORY_CASTLE_OBLIVION);
        break;
    case EVENT_063_13F_CASTLE_OBLIVION_E1:
        SetJiminyFlag(JIMINY_RECORD_CHARACTER_MARLUXIA);
        break;
    }
}

Mode gModeEventDebug = {
    "Event",
    Event_0,
    EventDebugUpdate,
    Event_2,
};

Mode gModeEvent = {
    "Event",
    Event_0,
    EventUpdate,
    Event_2,
};
