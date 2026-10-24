/**
 * mode_vsbattle.c
 * Link Battle Mode
 */
#ifdef PLATFORM_PC
#include "engine.h"
#endif

#include "macros.h"
#include "mode_vsbattle.h"
#include "chara_types.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include "mode_sio_api.h"
#include "sio_api.h"
#include "battle_actor.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "display.h"
#include "engine_math.h"
#include "game_state.h"
#include "m4a_song.h"
#include "mode.h"
#include <stddef.h>
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include "battle.h"
#include "gba/io_reg.h"

u8 gUnk_02039B98 EWRAM_COMMON(4);

void mode_vsbattle_0(u32 playerId) {
    VsTaskArg arg;
    VsTaskArg arg2;
    BtlWork** rikuBtl;

#ifdef PLATFORM_PC
    PortSetBattleView(1); /* as mode_battle.c: the stage repeats sideways in the wide margins */
#endif
    gBtlWork = EwramAlloc(sizeof(BtlWork));
    rikuBtl = &gRikuBtlWork;
    *rikuBtl = EwramAlloc(sizeof(BtlWork));

    if (gSioPlayerId == SIO_PLAYER_PARENT) {
        SeedRandom(gCharaLinkSend.seed);
    } else {
        SeedRandom(gCharaLinkRecv.seed);
    }
#ifdef PLATFORM_PC
    {
        extern int PortNetlinkTrace(void);
        extern void PortLog(const char* format, ...);
        if (PortNetlinkTrace()) /* the battle's seed: the parent's, sent in the link exchange (chara.c) */
            PortLog("netlink: vs seed sent %04x received %04x, player %d", gCharaLinkSend.seed, gCharaLinkRecv.seed,
                    gSioPlayerId);
    }
#endif

    VsBtlWorkInit();
    AllocBattleTiles();
    PlayVsBattleBgm();
    SetBgMode2();
    gBtlWork->bg = 2;
    gBtlWork->mapBg = 3;
    SetupBg(3, 0, 12, 0);
    SetupBg(2, 2, 28, 10);
    SetBgPriority(3, 2);
    SetBgPriority(2, 0);
    SetBgOverflow(3, TRUE);
    SetBgOverflow(2, FALSE);
    TaskPoolInit(&gBtlWork->taskPools[0], 32);
    TaskPoolInit(&gBtlWork->taskPools[1], 32);
    BgFxInit(BGCNT_256COLOR, gBtlWork->bg);
    ColliderPoolsInit();

    if (playerId == SIO_PLAYER_PARENT) {
        arg.mainSide = TRUE;
        arg.side = SIO_PLAYER_PARENT;
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlSora, &arg);
        arg.mainSide = FALSE;
        arg.side = SIO_PLAYER_CHILD;
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlSora, &arg);
        gBtlWork->flags |= BTL_FLAG_VS_LINK_PARENT;
    } else {
        arg2.mainSide = FALSE;
        arg2.side = SIO_PLAYER_PARENT;
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlSora, &arg2);
        arg2.mainSide = TRUE;
        arg2.side = SIO_PLAYER_CHILD;
        TaskCreate(&gBtlWork->taskPools[0], &gTaskDescBtlSora, &arg2);
    }

    SetBattleBounds((s16)(0x100 - gVsBattleHalfWidth), (s16)(gVsBattleHalfWidth + 0x100),
                  (s16)gVsBattleMinY, (s16)gVsBattleMaxY);
    TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlMap, NULL);
    gUnk_02039B98 = 0;
    FadeStartIn(FADE_MODE_BLACK, 60);
}

void mode_vsbattle_1() {
    if (!gBtlWork->paused) {
        VsBattleUpdate();

        if (gBtlWork->hitStop <= 0) {
            TaskPoolUpdate(&gBtlWork->taskPools[0]);
        } else {
            gBtlWork->hitStop--;
        }

        BgFxUpdate();
        ColliderUpdateAll();
        TaskPoolDraw(&gBtlWork->taskPools[1]);

        if (gBtlWork->flags & BTL_FLAG_CARD_BREAK) {
            gBtlWork->flags &= ~BTL_FLAG_CARD_BREAK;
        }
    }

    TaskPoolDraw(&gBtlWork->taskPools[0]);
}

void mode_vsbattle_2() {
#ifdef PLATFORM_PC
    PortSetBattleView(0);
#endif
    BgFxFree();
    TaskPoolDestroy(&gBtlWork->taskPools[1]);
    TaskPoolDestroy(&gBtlWork->taskPools[0]);
    ReleaseBattleTiles();
    EwramFree(gRikuBtlWork);
    EwramFree(gBtlWork);
}

void func_0800C6B0() {
}

void func_0800C6B4() {
}

void PlayVsBattleBgm() {
    switch (gGameState.battleStage) {
    case BATTLE_STAGE_WONDERLAND:
        m4aSongNumStart(SONG_BGM_ALICE_BTL);
        break;
    case BATTLE_STAGE_AGRABAH:
        m4aSongNumStart(SONG_BGM_ALADDIN_BATTLE);
        break;
    case BATTLE_STAGE_ATLANTICA:
        m4aSongNumStart(SONG_BGM_MARMAID_BATTLE);
        break;
    case BATTLE_STAGE_MONSTRO:
        m4aSongNumStart(SONG_BGM_PINOCCHIO_BTL);
        break;
    case BATTLE_STAGE_OLYMPUS_COLISEUM:
        m4aSongNumStart(SONG_BGM_HERCULES_BATTLE);
        break;
    case BATTLE_STAGE_HALLOWEEN_TOWN:
        m4aSongNumStart(SONG_BGM_HALLOWEEN_BTL);
        break;
    case BATTLE_STAGE_NEVER_LAND:
        m4aSongNumStart(SONG_BGM_PETERPAN_BTL);
        break;
    case BATTLE_STAGE_HOLLOW_BASTION:
        m4aSongNumStart(SONG_BGM_HOLLOW_BATTLE);
        break;
    case BATTLE_STAGE_TRAVERSE_TOWN:
        m4aSongNumStart(SONG_BGM_TOWN_BTL);
        break;
    case BATTLE_STAGE_DESTINY_ISLANDS:
        m4aSongNumStart(SONG_BGM_DESTINY_BATTLE);
        break;
    case BATTLE_STAGE_CASTLE_OBLIVION:
        m4aSongNumStart(SONG_BGM_F13F_FORGET_BATTLE);
        break;
    case BATTLE_STAGE_TWILIGHT_TOWN:
        m4aSongNumStart(SONG_BGM_TWILIGHT_BATTLE);
        break;
    default:
        m4aSongNumStart(SONG_BGM_BOSS1_WORLD);
        break;
    }
}

Mode gModeVsbattle = { "mode_vsbattle", (ModeInitFunc)mode_vsbattle_0, mode_vsbattle_1, mode_vsbattle_2 };

#ifdef PLATFORM_PC
/* The network link (sio.c): the link battle is running, the one part of the link that plays on guessed packets */
int VsBattleRunning(void) {
    extern Mode* gCurrentMode;
    return gCurrentMode == &gModeVsbattle;
}

/* The network link's desync check (sio.c): a hash of what both games must agree on during a link battle, in
   player order (the cable's parent first): each fighter's position and HP, and the random generators. 0 when no
   link battle is running. */
u32 VsBattleStateHash(void) {
    extern u32 PortRandomState(void);
    BtlWork* bySide[2];
    u32 h = 2166136261u;
    int i;
    if (gBtlWork == NULL || gRikuBtlWork == NULL || !(gBtlWork->flags & BTL_FLAG_VS_BATTLE))
        return 0;
    bySide[gSioPlayerId == SIO_PLAYER_PARENT ? 0 : 1] = gBtlWork;
    bySide[gSioPlayerId == SIO_PLAYER_PARENT ? 1 : 0] = gRikuBtlWork;
    for (i = 0; i < 2; i++) {
        const BtlObj* a = bySide[i]->actor;
        if (a == NULL)
            continue;
        h = (h ^ (u32)a->x) * 16777619u;
        h = (h ^ (u32)a->y) * 16777619u;
        h = (h ^ (u32)a->z) * 16777619u;
        h = (h ^ (u32)(u16)a->hp) * 16777619u;
    }
    h = (h ^ PortRandomState()) * 16777619u;
    {
        /* KHCOM_NETLINK_TRACE: the parts, to find what two games disagree on */
        extern int PortNetlinkTrace(void);
        extern void PortLog(const char* format, ...);
        if (PortNetlinkTrace()) {
            const BtlObj* a = bySide[0]->actor;
            const BtlObj* b = bySide[1]->actor;
            PortLog("netlink: hash %08x p1 %d,%d,%d hp %d p2 %d,%d,%d hp %d rng %08x", (unsigned)h, a ? (int)a->x : 0,
                    a ? (int)a->y : 0, a ? (int)a->z : 0, a ? (int)a->hp : 0, b ? (int)b->x : 0, b ? (int)b->y : 0,
                    b ? (int)b->z : 0, b ? (int)b->hp : 0, (unsigned)PortRandomState());
        }
    }
    return h ? h : 1;
}
#endif
