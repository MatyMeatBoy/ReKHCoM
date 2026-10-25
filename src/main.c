/**
 * main.c
 * Main Loop and Interrupts
 */

#include "chara_api.h"
#include "intr.h"
#include "gba/syscall.h"
#include "key.h"
#include "malloc.h"
#include "m4a.h"
#include "field_state.h"
#include "map.h"
#include "map_tasks.h"
#include "battle_work.h"
#include "battle_actor.h"
#include "card.h"
#include "card_label_data.h"
#include "taskpool.h"
#include "listpool.h"
#include "pallet.h"
#include "sprite.h"
#include "sroll.h"
#include "key_state.h"
#include "sio.h"
#include "engine.h"
#include "main.h"
#include "mode.h"
#include "system_state.h"
#include "gba/io_reg.h"
#include "sroll_api.h"
#include "sio_api.h"
#include "save_api.h"
#include "mode_pooh_api.h"
#include <stddef.h>
#include <string.h>

extern const char* GetModeName(void);
#include "engine_math.h"
#include "gba/defines.h"
#include "gba/macro.h"
#include "types.h"
#ifdef PLATFORM_PC
#include "port/port.h"
#include "display.h"
#include "m4a_song.h"
#include "songs.h"
#include "card_worldselect.h"
#include "game_state.h"
#include "map_api.h"
#include "registration_data.h"
#include "mode_movie.h"
#include "mode_chkbtl.h"
#include "battle.h"
#include "mode_mapchk.h"
#include "fade.h"
#include "world_types.h"
#include "player_progression.h"
#include "battle_ids.h"
#include "map_runtime.h"
#include "card_api.h"
#include "player_progression_types.h"
#include "map_check_worlds.h"
#include "btl4.h"
#include "sprites_btl_hud.h"
#ifdef PLATFORM_PC
#include "event_ids.h"
#include "mode_mapchk.h"
#include "map_runtime.h"
#include "card_api.h"
#include "map_types.h"
#include "player_progression_types.h"
#include "map_check_worlds.h"
#endif
extern int gPortIntroRequested;
extern int gPortEventTest;
extern int gPortMovieTest;
extern int gPortBattleTest;
extern Mode gModeEventselect;
void PortEventSelectPreset(int list, int index);
extern int gPortStartupLanguage;
#endif

#ifdef PLATFORM_PC
/* Test harness (KHCOM_MODE): switch to one of these modes by name. */
extern TaskDesc gTaskDescMapGmk04, gTaskDescMapSave;
extern Mode gModeDeck, gModeStatus, gModeJiminy, gModeAllmap, gModeMapinspect, gModeWorldinspect,
    gModeWorldselect, gModeWORLDSELECT, gModeWorldwarp, gModeMsTop, gModePremire,
    gModeSioBtlOption, gModeDebug;

void PortHarnessSong(int song) {
    m4aSongNumStart((u16)song);
}

/* Options menu sounds: the debug menu's click (SONG_SYS_CLICK) when its light
   moves, a little quieter. The game is paused while the menu is open: the
   whole sound state (the m4a players, their tracks, the mixer, the sound
   registers and the port's PSG/FIFO) is copied when it opens, silenced for
   the clicks, and put back exactly when it closes, so the music and effects
   go on from the same note, at the same volume. */
#define MENU_CLICK_VOLUME 0xB0 /* of 0x100 */
#define MENU_SOUND_IO 0x060u   /* sound registers 0x060-0x0A7 */
#define MENU_SOUND_IO_SIZE 0x48u
extern u8 gMPlayMemAccArea[16];
extern CgbChannel gCgbChans[4];
extern SoundInfo gSoundInfo;
void SoundClear();
void PortAudioVisitState(void (*visit)(void* data, size_t size));
void PortAudioPsgOff(void);
void PortM4aMixHold(int save);
static struct {
    SoundInfo sound;
    CgbChannel cgb[4];
    u8 memAcc[16];
    MusicPlayerInfo info[26];
    MusicPlayerTrack tracks[64];
    u8 io[MENU_SOUND_IO_SIZE];
    u8 port[4096];
    u32 portSize;
    int held;
} sMenuSound;

static void MenuSoundSavePort(void* data, size_t size) {
    if (sMenuSound.portSize + size <= sizeof(sMenuSound.port))
        memcpy(sMenuSound.port + sMenuSound.portSize, data, size);
    sMenuSound.portSize += (u32)size;
}

static void MenuSoundLoadPort(void* data, size_t size) {
    if (sMenuSound.portSize + size <= sizeof(sMenuSound.port))
        memcpy(data, sMenuSound.port + sMenuSound.portSize, size);
    sMenuSound.portSize += (u32)size;
}

/* copy (save) or put back (load) every player's info and tracks */
static int MenuSoundPlayers(int save) {
    u32 i, t = 0;
    for (i = 0; i < NUM_MUSIC_PLAYERS && i < ARRAY_COUNT(sMenuSound.info); i++) {
        const MusicPlayer* mp = &gMPlayTable[i];
        if (t + mp->numTracks > ARRAY_COUNT(sMenuSound.tracks))
            return 0;
        if (mp->info == NULL || mp->track == NULL)
            continue; /* entries 13-15 of gMPlayTable have no player (m4a2.c) */
        if (save) {
            sMenuSound.info[i] = *mp->info;
            memcpy(&sMenuSound.tracks[t], mp->track, mp->numTracks * sizeof(MusicPlayerTrack));
        } else {
            *mp->info = sMenuSound.info[i];
            memcpy(mp->track, &sMenuSound.tracks[t], mp->numTracks * sizeof(MusicPlayerTrack));
        }
        t += mp->numTracks;
    }
    return 1;
}

static const MusicPlayer* MenuClickPlayer(void) {
    return &gMPlayTable[gSongTable[SONG_SYS_CLICK].ms];
}

void PortMenuSoundBegin(void) {
    sMenuSound.held = 0;
    if (!MenuSoundPlayers(1))
        return; /* larger than expected: leave the sound alone, no clicks */
    sMenuSound.sound = gSoundInfo;
    memcpy(sMenuSound.cgb, gCgbChans, sizeof(sMenuSound.cgb));
    memcpy(sMenuSound.memAcc, gMPlayMemAccArea, sizeof(sMenuSound.memAcc));
    memcpy(sMenuSound.io, (u8*)REG_BASE + MENU_SOUND_IO, MENU_SOUND_IO_SIZE);
    sMenuSound.portSize = 0;
    PortAudioVisitState(MenuSoundSavePort);
    if (sMenuSound.portSize > sizeof(sMenuSound.port))
        return;
    sMenuSound.held = 1;
    m4aMPlayAllStop();
    SoundClear(); /* PSG channels off too: no tone held under the menu */
    PortAudioPsgOff();
    PortM4aMixHold(1); /* and no reverb tail */
    if (gSoundInfoPtr != NULL)
        gSoundInfoPtr->reverb = 0; /* scenes run with echo: the click must sound once */
}

void PortMenuSoundClick(void) {
    if (!sMenuSound.held)
        return;
    m4aSongNumStart(SONG_SYS_CLICK);
    m4aMPlayVolumeControl(MenuClickPlayer()->info, 0xFFFF, MENU_CLICK_VOLUME);
}

/* confirm (1) and cancel (0) as the game's menus sound them */
void PortMenuSoundDecide(int confirm) {
    u16 song = confirm ? SONG_SYS_KETTEI : SONG_SYS_CANSEL;
    if (!sMenuSound.held)
        return;
    m4aSongNumStart(song);
    m4aMPlayVolumeControl(gMPlayTable[gSongTable[song].ms].info, 0xFFFF, MENU_CLICK_VOLUME);
}

/* the overlay has closed and the game's sound is back: SYS_CLOSE, as the
   game's field menu sounds closing (it plays on in the game's own mixer) */
void PortMenuSoundClose(void) {
    m4aSongNumStart(SONG_SYS_CLOSE);
}

/* the mixer for one menu frame (the port then sends it out) */
void PortMenuSoundFrame(void) {
    if (sMenuSound.held)
        m4aSoundMain();
}

void PortMenuSoundEnd(void) {
    if (!sMenuSound.held)
        return;
    MenuSoundPlayers(0);
    gSoundInfo = sMenuSound.sound;
    memcpy(gCgbChans, sMenuSound.cgb, sizeof(sMenuSound.cgb));
    memcpy(gMPlayMemAccArea, sMenuSound.memAcc, sizeof(sMenuSound.memAcc));
    memcpy((u8*)REG_BASE + MENU_SOUND_IO, sMenuSound.io, MENU_SOUND_IO_SIZE);
    sMenuSound.portSize = 0;
    PortAudioVisitState(MenuSoundLoadPort);
    PortM4aMixHold(0);
    sMenuSound.held = 0;
}

/* The original debug menu resets volatile game progress, so make it available
   through the cheat page only from the safe title screen. */
int PortDebugModeAvailable(void) {
    return strcmp(GetModeName(), "mode_title") == 0;
}

/* Options > Scenes: a scene seen before, replayed in the game's engine from the title (as the debug menu's event
   select does, without its end message); back to the title when it ends (mode_event.c). */
int gPortGalleryEvent;
extern Mode gModeEventDebug;
int PortSceneGalleryPlay(int eventId) {
    if (!PortDebugModeAvailable())
        return 0;
    gPortGalleryEvent = 1;
    ModeRequest(&gModeEventDebug, (eventId & 0x7FFF) | 0x8000);
    return 1;
}

void PortDebugModeEnter(void) {
    if (PortDebugModeAvailable()) {
        PortAchievementsInvalidate("Debug menu entered");
        ModeRequest(&gModeDebug, 0);
    }
}

extern Mode gModeBattle;

/* Boss rematches and the battle tutorials (Options -> Rematches), an idea of
   the Vita port: a boss of Sora's story, once beaten (its card obtained), or
   one of the two tutorials, fought again at its original strength. The port
   layer keeps the field in a state and restores it afterwards. */
static const struct {
    u16 battle;
    u8 world;
    s8 card; /* OBTAINED_CARD_*, -1 always available */
} sRematches[] = {
    { BATTLE_GUARD_ARMOR, WORLD_TRAVERSE_TOWN, OBTAINED_CARD_GUARD_ARMOR },
    { BATTLE_TRICKMASTER, WORLD_WONDERLAND, OBTAINED_CARD_TRICKMASTER },
    { BATTLE_JAFAR, WORLD_AGRABAH, OBTAINED_CARD_JAFAR },
    { BATTLE_HADES, WORLD_OLYMPUS_COLISEUM, OBTAINED_CARD_HADES },
    { BATTLE_URSULA, WORLD_ATLANTICA, OBTAINED_CARD_URSULA },
    { BATTLE_PARASITE_CAGE, WORLD_MONSTRO, OBTAINED_CARD_PARASITE_CAGE },
    { BATTLE_OOGIE_BOOGIE, WORLD_HALLOWEEN_TOWN, OBTAINED_CARD_OOGIE_BOOGIE },
    { BATTLE_HOOK, WORLD_NEVER_LAND, OBTAINED_CARD_HOOK },
    { BATTLE_DARKSIDE, WORLD_DESTINY_ISLANDS, OBTAINED_CARD_DARKSIDE },
    { BATTLE_DRAGON_MALEFICENT, WORLD_HOLLOW_BASTION, OBTAINED_CARD_DRAGON_MALEFICENT },
    { BATTLE_RIKU_1, WORLD_CASTLE_OBLIVION, OBTAINED_CARD_RIKU },
    { BATTLE_VEXEN_1, WORLD_CASTLE_OBLIVION, OBTAINED_CARD_VEXEN },
    { BATTLE_TUTORIAL_0, WORLD_TRAVERSE_TOWN, -1 },
    { BATTLE_TUTORIAL_1, WORLD_TRAVERSE_TOWN, -1 },
};

int PortRematchCount(void) {
    return ARRAY_COUNT(sRematches);
}

/* walking a room as Sora, and the boss beaten */
int PortRematchAvailable(int i) {
    const char* mode = GetModeName();
    if (i < 0 || i >= (int)ARRAY_COUNT(sRematches) || (gGameState.flags & GAME_FLAG_RIKU))
        return 0;
    if (strcmp(mode, "Mode_MapFld") != 0 && strcmp(mode, "Mode_MapFix") != 0)
        return 0;
    return sRematches[i].card < 0 || IsCardKindObtained(sRematches[i].card);
}

/* in the boss's own world and arena; everything is restored afterwards */
void PortRematchBegin(int i) {
    extern u8 gWorldBattleStages[];
    gGameState.world = sRematches[i].world;
    gGameState.battleStage = gWorldBattleStages[sRematches[i].world];
    ModeRequest(&gModeBattle, sRematches[i].battle);
}

const char* PortHarnessModeName(void) {
    extern Mode* gCurrentMode;
    return gCurrentMode != NULL ? gCurrentMode->name : "none";
}

u8 gPortHarnessTakePrize; /* takeprize below */

int PortHarnessMode(const char* name, int arg) {
    extern Mode gModeTitle, gModeMsShop, gModeMsCharge, gModeContinue, gModeWLogo, gModeEventselect,
        gModeRikuDeckTutorial, gModeVsbattle, gModeSioBattle;
    static Mode* const modes[] = {
        &gModeTitle, &gModeDeck, &gModeStatus, &gModeJiminy, &gModeAllmap, &gModeMapinspect, &gModeWorldinspect,
        &gModeWorldselect, &gModeWORLDSELECT, &gModeWorldwarp, &gModeMsTop, &gModePremire,
        &gModeSioBtlOption, &gModeDebug, &gModePooh, &gModeStaffRoll, &gModeMovie,
        /* the rest of test_android_scenarios.py's menu list (KHCOM_MODE) */
        &gModeMsShop, &gModeMsCharge, &gModeContinue, &gModeWLogo, &gModeEventselect, &gModeRikuDeckTutorial,
        &gModeVsbattle,
        &gModeSioBattle, /* the link battle's menus (the network cable's tests, ONLINE_ESTUDIO.md) */
    };
    u32 i;

    /* Isolated HUD regression: change actual actor HP, letting the original
       task perform damage/healing interpolation and expression changes. */
    if (strcmp(name, "hudhp") == 0 && gBtlWork != NULL && gBtlWork->actor != NULL) {
        extern void PortLog(const char* format, ...);
        int hp;
        if (arg < 0 || arg > 100) return 0;
        hp = gBtlWork->actor->maxHp * arg / 100;
        if (hp < 1) hp = 1; /* avoid ending the battle during the render probe */
        gBtlWork->actor->hp = hp;
        gBtlWork->actor->invincibleTimer = 3000;
        PortLog("harness: hudhp %d/%d", hp, gBtlWork->actor->maxHp);
        return 1;
    }

    /* Small numeric text fixture: use the existing battle task and its
       original DrawSprite path. This does not award experience or a level. */
    if ((strcmp(name, "textdigits") == 0 || strcmp(name, "textlevel") == 0) && gBtlWork != NULL) {
        Task* task = ListPoolFirst(&gBtlWork->taskPools[1].head);
        BtlExpWork* work;
        extern void PortLog(const char* format, ...);
        while (task != NULL && task->desc != &gTaskDescBtlExp)
            task = ListPoolNext(&task->node);
        if (task == NULL)
            task = TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBtlExp, NULL);
        if (task == NULL)
            return 0;
        work = task->work;
        work->level = gGameState.progression.level - (strcmp(name, "textlevel") == 0 ? 1 : 0);
        work->lastExp = gGameState.progression.exp;
        work->timer = 0;
        work->state = 1; /* BTL_EXP_STATE_GAIN, local to btl_exp.c */
#ifdef VERSION_JP
        work->gfx = gBtlExpNextFrame2;
#else
        work->gfx = gBtlExpNextLvFrame2;
#endif
        BtlExpSetNumber(work, (u32)arg % 100000);
        PortLog("harness: %s %d", name, arg % 100000);
        return 1;
    }

    /* Isolated status transition probe: populate the existing stock list
       and toggle its NEW badges. Never used outside KHCOM_MODE fixtures. */
    if (strcmp(name, "statusnew") == 0) {
        gGameState.progression.learnedStocks = ~(u64)0;
        gGameState.progression.learnedStocks2 = ~(u64)0;
        gGameState.progression.newStocks = arg ? ~(u64)0 : 0;
        gGameState.progression.newStocks2 = arg ? ~(u64)0 : 0;
        return 1;
    }

    if (strcmp(name, "progress") == 0) {
        extern void PortLog(const char* format, ...);
        PortLog("harness: progress flags=%u floor=%u world=%u room=%u story=%u level=%u mode=%s",
                (unsigned)gGameState.flags, (unsigned)gGameState.floor,
                (unsigned)gMapFloorState.world, (unsigned)gMapFloorState.room,
                (unsigned)gMapFloorState.progress, (unsigned)gGameState.progression.level,
                PortHarnessModeName());
        return 1;
    }
    if (strcmp(name, "roomgraph") == 0 || strcmp(name, "roomgoto") == 0) {
        u8 seen[32] = {0}, queue[32], sides[32] = {0};
        int head = 0, count = 1, side, room, neighbor;
        extern void PortLog(const char* format, ...);
        if (gFieldState == NULL || gGameState.floor >= ((gGameState.flags & GAME_FLAG_RIKU) ? 12 : 13))
            return 0;
        queue[0] = GetMapFloorDef(gGameState.floor)->entryRoom;
        if (queue[0] >= 32) return 0;
        seen[queue[0]] = 1;
        /* The final Riku boss room is disconnected from the ordinary links,
           but is explicitly part of the floor definition. Include its graph. */
        neighbor = GetMapFloorDef(gGameState.floor)->exitRoom;
        if (neighbor < 32 && !seen[neighbor]) {
            seen[neighbor] = 1;
            queue[count++] = neighbor;
        }
        while (head < count) {
            room = queue[head++];
            for (side = 0; side < 4; side++) {
                neighbor = GetMapRoomLink(room, side);
                if (neighbor < 32 && !seen[neighbor]) {
                    seen[neighbor] = 1;
                    sides[neighbor] = side;
                    queue[count++] = neighbor;
                }
            }
            PortLog("harness: roomgraph floor=%u world=%u room=%d", (unsigned)gGameState.floor,
                    (unsigned)gMapFloorState.world, room);
        }
        if (strcmp(name, "roomgoto") == 0) {
            if (arg < 0 || arg >= 32 || !seen[arg]) return 0;
            CreateMapRoom(arg, NULL);
            SetCurrentMapRoom(arg, sides[arg]);
            RequestMapMode();
        }
        return 1;
    }

    /* Rebuild a reported room exactly: its shape (roomType) and seed, read
       from the player's floor state. Applies to every room of the floor,
       without enemies, before roomgoto. */
    if (strcmp(name, "roomshape") == 0 || strcmp(name, "roomseed") == 0) {
        int i;
        for (i = 0; i < 32; i++) {
            if (name[4] == 's' && name[5] == 'h') {
                gMapFloorState.rooms[i].roomType = (u8)arg;
                gMapFloorState.rooms[i].flags |= FLOOR_ROOM_FLAG_CREATED;
            } else {
                gMapFloorState.rooms[i].seed = (u32)arg;
            }
            gMapFloorState.rooms[i].enemiesLeft = 0;
        }
        return 1;
    }

    /* the room's first field enemy to arg pixels right of the GBA screen, on
       Sora's row: the wide view still shows it (debug option illustration) */
    if (strcmp(name, "enmside") == 0 && gFieldState != NULL) {
        Task* task = ListPoolFirst(&gFieldState->tasks4.head);
        if (task == NULL)
            return 0;
        ((MapEnmWork*)task->work)->obj.fieldPosition.x = gFieldState->x + ((DISPLAY_WIDTH + arg) << 8);
        ((MapEnmWork*)task->work)->obj.fieldPosition.y = gFieldState->actor.fieldPosition.y;
        return 1;
    }
    /* field enemies left in every room of the floor (after roomshape) */
    if (strcmp(name, "roomenemies") == 0) {
        int i;
        for (i = 0; i < 32; i++)
            gMapFloorState.rooms[i].enemiesLeft = (u8)arg;
        return 1;
    }

    /* rematches: obtain:N marks OBTAINED_CARD_N; rematch:N starts row N as
       the options menu does */
    if (strcmp(name, "obtain") == 0) {
        SetCardKindObtained(arg);
        return 1;
    }
    /* deckuse:DECK|COUNT<<2|CARD<<9 - the collection holds COUNT copies of one
       card, all of them in deck DECK, which becomes the active deck (the
       official challenge achievements check the deck a battle is won with) */
    if (strcmp(name, "deckuse") == 0) {
        extern u16 gCardCount;
        extern Deck gDecks[3];
        extern void SetActiveDeckIndex(u8 index);
        int deck = arg & 3, count = (arg >> 2) & 0x7F, card = (arg >> 9) & CARD_ID_MASK, i;
        if (deck > 2 || count < 1 || count > DECK_SIZE)
            return 0;
        for (i = 0; i < count; i++) {
            gCardCollection[i] = (u16)(card | (CARD_FLAG_IN_DECK_1 << deck));
            gDecks[deck].cards[i] = (u16)i;
        }
        gCardCount = (u16)count;
        gDecks[deck].cardCount = (u16)count;
        SetActiveDeckIndex((u8)deck);
        return 1;
    }
    /* eventplay:N - a story event requested as the game does (RequestEventMode), not from the debug event menu: an
       event that starts a battle goes on into it and, once won, into the event after it (issue #46) */
    if (strcmp(name, "eventplay") == 0) {
        extern void RequestEventMode(u16 eventId);
        RequestEventMode((u16)arg);
        return 1;
    }
    /* worlds:MASK - the world cards Sora holds (gGameState.availableWorlds), for the world select */
    if (strcmp(name, "worlds") == 0) {
        gGameState.availableWorlds = (u16)arg;
        return 1;
    }
    /* movieend:ID - a movie as the final events hand over to it
       (mode_event.c EVENT_EXIT_MOVIE_ENDING: with the heap reset) */
    if (strcmp(name, "movieend") == 0) {
        ModeRequestHeapReset(&gModeMovie, arg);
        return 1;
    }
    /* jiminy:FLAG - a journal record, as the game sets it (SetJiminyFlag) */
    if (strcmp(name, "jiminy") == 0) {
        extern void SetJiminyFlag(u32 flag);
        SetJiminyFlag((u32)arg);
        return 1;
    }
    /* darkunlock - what EVENT_156_RIKU_B12F_GOAL_2 does early in Riku's story
       (mode_event.c): dark points count from then on */
    if (strcmp(name, "darkunlock") == 0) {
        extern void LearnStock(u32 stock);
        gGameState.flags &= ~GAME_FLAG_DARK_POINTS_LOCKED;
        LearnStock(LEARNED_STOCK_DARK_BREAK);
        LearnStock(LEARNED_STOCK_DARK_FIRAGA);
        LearnStock(LEARNED_STOCK_DARK_AURA);
        return 1;
    }
    /* darkpoints:N - dark points as won card breaks add them (AddDarkPoints);
       past 29, Riku's next single card starts his Dark Mode */
    if (strcmp(name, "darkpoints") == 0 && gBtlWork != NULL) {
        extern void AddDarkPoints(s16 points);
        AddDarkPoints((s16)arg);
        return 1;
    }
    /* fieldhp:N - the HP the player walks into the next battle with (HP is not
       restored between battles in this game) */
    if (strcmp(name, "fieldhp") == 0) {
        gGameState.hp = (s16)arg;
        return 1;
    }
    /* actorlog: where the player's battle object and its fields sit (to name
       the fields the official graphs read next to its HP) */
    if (strcmp(name, "actorlog") == 0 && gBtlWork != NULL && gBtlWork->actor != NULL) {
        extern void PortLog(const char* format, ...);
        PortLog("harness: actor %08x hp@%08x kind=%d size=%u btlwork=%08x", (unsigned)(uintptr_t)gBtlWork->actor,
                (unsigned)(uintptr_t)&gBtlWork->actor->hp, (int)gBtlWork->actor->kind, (unsigned)sizeof(*gBtlWork->actor),
                (unsigned)(uintptr_t)gBtlWork);
        return 1;
    }
    /* gsplace:FLOOR|STAGE<<8|WORLD<<16|FIRST<<24 - where the story stands when
       an event of the debug event list plays (it starts from a reset game
       state); FIRST is the first floor's world (10 Sora's game, 8 Riku's) */
    if (strcmp(name, "gsplace") == 0) {
        gGameState.floor = (s8)(arg & 0xFF);
        gGameState.battleStage = (u8)(arg >> 8);
        gGameState.world = (u8)(arg >> 16);
        gGameState.floors[0].world = (u8)((arg >> 24) & 0x7F);
        return 1;
    }
    /* noharm: the player cannot be hurt for the rest of the battle (the
       no-damage challenges), as the game's own invincibility frames do */
    if (strcmp(name, "noharm") == 0) {
        if (gBtlWork != NULL && gBtlWork->actor != NULL) /* outside a battle: nothing to do */
            gBtlWork->actor->invincibleTimer = 0x7FFF;
        return 1;
    }
    /* drawprizes: the orbs come to the player, as with the game's own Draw (attract) effect; a boss's orbs never
       time out and its battle closes only once they are taken, which the harness, standing still, never did
       (Guard Armor's chain seemed to hang) */
    if (strcmp(name, "drawprizes") == 0) {
        if (gBtlWork != NULL)
            gBtlWork->hcEffect = HC_EFFECT_DRAW;
        if (gRikuBtlWork != NULL)
            gRikuBtlWork->hcEffect = HC_EFFECT_DRAW;
        return 1;
    }
    /* takeprize: a boss's prize card goes to the player's feet (card_reload_children.c), who takes it as when
       walking up to it; the harness never walks, and the battle waits for the card */
    if (strcmp(name, "takeprize") == 0) {
        gPortHarnessTakePrize = 1;
        return 1;
    }
    /* battleid:ID|WORLD<<8 - any battle in a given world, as PortRematchBegin
       starts one (official achievement validation, ra_battle_dryrun.py) */
    if (strcmp(name, "battleid") == 0) {
        extern u8 gWorldBattleStages[];
        gGameState.world = (u8)(arg >> 8);
        gGameState.battleStage = gWorldBattleStages[gGameState.world];
        ModeRequest(&gModeBattle, arg & 0xFF);
        return 1;
    }
    if (strcmp(name, "rematch") == 0) {
        extern int PortRematchStart(int i);
        return PortRematchStart(arg);
    }

    /* moogle points picked up on the field, as map_tasks.c does */
    if (strcmp(name, "moogle") == 0) {
        extern void PortMoogleNotice(unsigned amount, unsigned total);
        gGameState.progression.mooglePoints += (u32)arg;
        PortMoogleNotice((unsigned)arg, gGameState.progression.mooglePoints);
        return 1;
    }

    /* the Cheats page's actions: 1 level 99, 2 map cards, 3 battle cards, 4 journal */
    if (strcmp(name, "cheat") == 0) {
        extern int PortCheatMaxLevel(void), PortCheatMapCards(void), PortCheatBattleCards(void), PortCheatJournal(void);
        extern void PortLog(const char* format, ...);
        extern u16 gCardCount;
        extern u8 gMapCardCounts[270];
        int ok = arg == 1 ? PortCheatMaxLevel() : arg == 2 ? PortCheatMapCards() : arg == 3 ? PortCheatBattleCards() : arg == 4 ? PortCheatJournal() : 0;
        int i, cards = 0, maps = 0;
        for (i = 0; i < 999; i++) cards += i < gCardCount && gCardCollection[i] != 0xFFF;
        for (i = 0; i < 270; i++) maps += gMapCardCounts[i];
        PortLog("harness: cheat mode=%s", PortHarnessModeName());
        PortLog("harness: cheat %d ok=%d level=%u hp=%d cp=%d ap=%d dp=%d stocks=%08x%08x milestone=%u cards=%d/%u maps=%d",
                arg, ok, (unsigned)gGameState.progression.level, gGameState.progression.maxHp, gGameState.progression.cp,
                gGameState.progression.ap, gGameState.progression.dp,
                (unsigned)(gGameState.progression.learnedStocks >> 32), (unsigned)gGameState.progression.learnedStocks,
                (unsigned)gGameState.progression.levelMilestone, cards, (unsigned)gCardCount, maps);
        return ok;
    }

    if (strcmp(name, "rikuopening") == 0) {
        extern void RequestEventMode(u16 eventId);
        ResetGameState();
        SetupRikuNewGame();
        RequestEventMode(EVENT_149_RIKU_B12F_OPNING);
        return 1;
    }

    /* Reproduce the MAP debug menu's entry, without bypassing map setup.
       arg = mode*10000 + world*100 + floor; roomriku selects Reverse/Rebirth.
       These are fixtures, not proof of a completed story playthrough. */
    if (strcmp(name, "room") == 0 || strcmp(name, "roomriku") == 0) {
        int mode = arg / 10000, world = (arg / 100) % 100, floor = arg % 100;
        int riku = strcmp(name, "roomriku") == 0;
        extern u8 gMapChkUseParams;
        extern void PortLog(const char* format, ...);
        if (arg < 0 || mode < 1 || mode > 3 || world > 12 || floor >= (riku ? 12 : 13))
            return 0;
        ResetGameState();
        if (riku) SetupRikuNewGame();
        else SetupSoraNewGame();
        InitMapCardInventory();
        gMapChkUseParams = 0;
        gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_MAP_SELECT | TUTORIAL_FLAG_SAVE_POINT |
            TUTORIAL_FLAG_MAP | TUTORIAL_FLAG_MAP_SELECT_EVENT_DOOR | TUTORIAL_FLAG_WARP_POINT |
            TUTORIAL_FLAG_ROOM_CREATE | TUTORIAL_FLAG_MAP_STAIR;
        gGameState.floors[floor].world = mode == 2 ? 0 : gMapChkWorldNames[world][0];
        MapChkSetFloorProgress(floor, mode == 3);
        GoToFloor(floor);
        if (mode == 1) {
            SetFloorWorld(gMapChkWorldNames[world][0]);
            gMapFloorState.flags |= FLOOR_FLAG_LOGO_SHOWN;
            EnterFloorWorld();
        } else {
            gMapFloorState.room = mode == 3 ? MAP_ROOM_EXIT_HALL : MAP_ROOM_ENTRANCE_HALL;
            gMapFloorState.entrySide = MAP_DOOR_SIDE_NONE;
        }
        PortLog("harness: room fixture actor=%s mode=%d world=%d floor=%d room=%d",
                riku ? "riku" : "sora", mode, world, floor, gMapFloorState.room);
        RequestMapMode();
        return 1;
    }

    if (strcmp(name, "castle") == 0) {
        ResetGameState();
        SetupSoraNewGame();
        gMapFloorState.room = MAP_ROOM_ENTRANCE_HALL;
        ModeRequestHeapReset(&gModeMapFix, 0);
        return 1;
    }
    /* Reproduce a white-fade castle exit through the real room-transition
       update, without depending on a previous executable's pointer state. */
    if (strcmp(name, "roomtransition") == 0 && gFieldState != NULL) {
        extern void MapFixSetUpdate(void (*update)(void));
        extern void MapFixLeaveEntranceHall(void);
        gMapRoomState->flags |= ROOM_FLAG_ENTER_WORLD;
        gMapFloorState.flags |= FLOOR_FLAG_ENTRY_EVENT_DONE;
        gMapFloorState.world = WORLD_TRAVERSE_TOWN;
        MapFixSetUpdate(MapFixLeaveEntranceHall);
        FadeStartOut(FADE_MODE_WHITE, 16);
        return 1;
    }
    if (strcmp(name, "fieldfreeze") == 0 && gFieldState != NULL) {
        gFieldState->flags |= FIELD_FLAG_FREEZE_ENEMIES | FIELD_FLAG_NO_ENEMY_SPAWN;
        PortLog("harness: fieldfreeze applied");
        return 1;
    }
    if (strcmp(name, "doortutorial") == 0) {
        gGameState.progression.tutorialFlags &= ~(TUTORIAL_FLAG_MAP_SELECT | TUTORIAL_FLAG_MAP_SELECT_EVENT_DOOR);
        return 1;
    }
    if (strcmp(name, "doorpose") == 0 && gFieldState != NULL) {
        extern int PortHarnessDoorPose(int eventDoor);
        return PortHarnessDoorPose(arg);
    }
    if (strcmp(name, "spotlight") == 0 && gFieldState != NULL) {
        SetBgPriority(0, 2);
        SetBgPriority(1, 2);
        TaskCreate(&gFieldState->tasks, &gTaskDescSpotLight, NULL);
        return 1;
    }
    if ((strcmp(name, "gmkcount") == 0 || strcmp(name, "crystalview") == 0) && gFieldState != NULL) {
        extern void PortLog(const char* format, ...);
        Task* task = ListPoolFirst(&gFieldState->tasks.head);
        int count = 0;
        while (task != NULL) {
            count += task->desc == &gTaskDescMapGmk04;
            if (task->desc == &gTaskDescMapGmk04)
                PortLog("harness: save crystal at %d,%d,%d room=%u seed=%08x", (int)((MapGmk04Work*)task->work)->obj.fieldPosition.x,
                        (int)((MapGmk04Work*)task->work)->obj.fieldPosition.y, (int)((MapGmk04Work*)task->work)->obj.fieldPosition.z,
                        (unsigned)gMapFloorState.room, (unsigned)GetMapFloorRoom(gMapFloorState.room)->seed);
            if (task->desc == &gTaskDescMapGmk04 && strcmp(name, "crystalview") == 0) {
                MapGmk04Work* crystal = task->work;
                gFieldState->actor.fieldPosition = crystal->obj.fieldPosition;
                /* arg 1: behind it, where issue 63's savestate stood */
                gFieldState->actor.fieldPosition.x -= arg == 1 ? 4457 : 0x1200;
                gFieldState->actor.fieldPosition.y += arg == 1 ? -2212 : 0x1C00;
                MapSetCameraTarget(gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
            }
            task = ListPoolNext(&task->node);
        }
        PortLog("harness: save crystals %d", count);
        return 1;
    }
    /* gmkall: every gimmick the room placed (crystal, lamps, crates...), to
       compare against the cartridge's placement list (crystal_parity.py) */
    if (strcmp(name, "gmkall") == 0 && gFieldState != NULL && gMapGmkPlacements != NULL) {
        extern void PortLog(const char* format, ...);
        extern u8 PortMapGmkCount(void);
        int i, count = PortMapGmkCount();
        for (i = 0; i < count; i++) {
            const MapGmkPlacement* p = &gMapGmkPlacements[i];
            PortLog("harness: gmk %d def=%08x flags=%04x at %d,%d,%d room=%u seed=%08x", i, (unsigned)(uintptr_t)p->def,
                    (unsigned)p->flags, (int)p->pos.x, (int)p->pos.y, (int)p->pos.z, (unsigned)gMapFloorState.room,
                    (unsigned)GetMapFloorRoom(gMapFloorState.room)->seed);
        }
        PortLog("harness: gmk count %d", count);
        return 1;
    }
    if (strcmp(name, "poslog") == 0 && gFieldState != NULL) {
        extern void PortLog(const char* format, ...);
        Task* enm = ListPoolFirst(&gFieldState->tasks4.head);
        int enemies = 0;
        for (; enm != NULL; enm = ListPoolNext(&enm->node)) {
            enemies++;
            PortLog("harness: enemy %s at %d (screen %d)", enm->desc->name,
                    (int)((MapEnmWork*)enm->work)->obj.fieldPosition.x >> 8,
                    (int)(((MapEnmWork*)enm->work)->obj.fieldPosition.x - gFieldState->x) >> 8);
        }
        PortLog("harness: sora at %d,%d,%d enemies=%d", (int)gFieldState->actor.fieldPosition.x,
                (int)gFieldState->actor.fieldPosition.y, (int)gFieldState->actor.fieldPosition.z, enemies);
        return 1;
    }
    if (strcmp(name, "doorselect") == 0 && gFieldState != NULL) {
        static u8 status;
        gFieldState->flags |= FIELD_FLAG_ROOM_CREATE | FIELD_FLAG_FREEZE_PLAYER;
        MapFreezeBg1();
        CreateMapCardSelection(&gFieldState->tasks, &status);
        SetBgPriority(1, 1);
        return 1;
    }

    /* the level-up screen, as the end of a battle opens it */
    /* the card-break mosaic over every layer (battle_runtime.c) */
    if (strcmp(name, "mosaic") == 0) {
        MosaicStartIn(16, 15);
        return 1;
    }
    /* btllog: every battle object (kind, HP, flags) and the enemy counts, to see
       what keeps a battle open (test_event_battles.py) */
    if (strcmp(name, "btllog") == 0 && gBtlWork != NULL) {
        BtlObj* obj = ListPoolFirst(&gBtlWork->pool);
        PortLog("btllog: enemies %d pending %d phase %d step %d flags %08x prizes %d bgfx %d", gBtlWork->enemyCount,
                gBtlWork->pendingEnemies, gBtlWork->phase, gBtlWork->phaseStep, (unsigned)gBtlWork->flags,
                gBtlWork->prizeCount, BgFxIsActive());
        while (obj != NULL) {
            PortLog("btllog: kind %d hp %d/%d flags %08x%08x kindFlags %04x self %d parent %d at %d,%d,%d", obj->kind,
                    obj->hp, obj->maxHp, (unsigned)(obj->flags >> 32), (unsigned)obj->flags, obj->kindFlags,
                    obj->self == obj, obj->parent != NULL, obj->x >> 8, obj->y >> 8, obj->z >> 8);
            obj = ListPoolNext(&obj->node);
        }
        return 1;
    }
    /* battlewin: every enemy leaves as Oogie Boogie's helpers do, so the game
       ends the battle by its own route (white fade, rewards, back to the room) */
    /* premirewin: as battlewin, with a premire prize collected: the game opens Bonus Elite at the rewards */
    if (strcmp(name, "premirewin") == 0 && gBtlWork != NULL && strcmp(PortHarnessModeName(), "mode_battle") == 0) {
        gBtlWork->flags |= BTL_FLAG_PREMIRE_COLLECTED;
        return PortHarnessMode("battlewin", 0);
    }
    if (strcmp(name, "battlewin") == 0 && gBtlWork != NULL
        && strcmp(PortHarnessModeName(), "mode_battle") == 0) {
        BtlObj* obj = ListPoolFirst(&gBtlWork->pool);
        gBtlWork->pendingEnemies = 0;
        while (obj != NULL) {
            obj->flags |= BTLOBJ_FLAG_WARP_PENDING;
            obj->hitFlags = 0;
            obj = ListPoolNext(&obj->node);
        }
        return 1;
    }
    /* bosskill: every enemy that can be hit takes a hit worth all its HP through
       the game's own damage route (ApplyBtlObjHit), so it is defeated as a real hit
       would; the unhittable parts wait, as for the player (Guard Armor's torso
       until its limbs are gone: hit all at once, its pieces were released after
       BeginBossDefeat zeroed the count, which wrapped to 250 and never closed) */
    if (strcmp(name, "bosskill") == 0) {
        /* outside a battle there is nothing to hit (a list of them spans a whole story chain) */
        BtlObj* obj = gBtlWork != NULL && strcmp(PortHarnessModeName(), "mode_battle") == 0
                          ? ListPoolFirst(&gBtlWork->pool) : NULL;
        while (obj != NULL) {
            if (obj != gBtlWork->actor && obj->hp > 0 && obj->maxHp > 0 && !(obj->flags & BTLOBJ_FLAG_UNHITTABLE)) {
                /* no knockback, as the game's delayed damage (battle_runtime.c): a stale
                   knockbackLift threw a Bandit up for thousands of frames */
                obj->damage = obj->hp;
                obj->knockbackSpeed = 0;
                obj->knockbackLift = 0;
                obj->flags |= BTLOBJ_FLAG_DAMAGE_PENDING;
            }
            obj = ListPoolNext(&obj->node);
        }
        return 1;
    }
    /* playerkill: the player takes a hit worth all its HP through the same
       damage route, so the battle is lost as a real hit would lose it */
    if (strcmp(name, "playerkill") == 0 && gBtlWork != NULL && gBtlWork->actor != NULL
        && strcmp(PortHarnessModeName(), "mode_battle") == 0) {
        BtlObj* actor = gBtlWork->actor;
        if (actor->hp > 0) {
            actor->invincibleTimer = 0;
            actor->damage = actor->hp;
            actor->flags |= BTLOBJ_FLAG_DAMAGE_PENDING;
        }
        return 1;
    }
    /* battleend: the same closing sequence without a level up, for the exit */
    if ((strcmp(name, "levelup") == 0 || strcmp(name, "battleend") == 0) && gBtlWork != NULL) {
        /* The debug battle menu maxes every stat; restore selectable reward
           options for this fixture instead of indexing a nonexistent option. */
        gGameState.progression.maxHp = 80;
        gGameState.progression.cp = 275;
        gGameState.progression.levelMilestone = 0;
        gBtlWork->pendingLevelUps = strcmp(name, "levelup") == 0;
        /* Use the battle's closing sequence, so cards/camera and effects
           stop before their tile allocations are reused by the menu. */
        gBtlWork->phase = BTL_PHASE_END;
        gBtlWork->phaseStep = 0; /* BTL_END_STEP_CLOSE_CARDS, local to battle_runtime */
        gBtlWork->prizeCount = 0;
        /* Debug encounters still have live actors; prevent their animation
           uploads from touching tile allocations released for the reward UI. */
        gBtlWork->hitStop = 32767;
        gBtlWork->flags |= BTL_FLAG_BGFX_PAUSED;
        return 1;
    }
    /* the save point's menu, opened as talking to it does */
    /* dockleft:BG: leave a layer docked as a closed portrait window used to
       (the user's 19/10 save window shifted by the margin) */
    if (strcmp(name, "dockleft") == 0) {
        extern int PortWideMarginX(void);
        PortDockBg(arg, -PortWideMarginX());
        return 1;
    }
    if (strcmp(name, "mapsave") == 0 && gFieldState != NULL) {
        gFieldState->flags |= FIELD_FLAG_FREEZE_PLAYER;
        TaskCreate(&gFieldState->tasks, &gTaskDescMapSave, NULL);
        return 1;
    }
    for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        if (strcmp(modes[i]->name, name) == 0) {
            /* the link battle comes from the title, which resets the game state first (ChkBtlReset sets the VS
               arena's bounds: without it the battle's camera had no room) */
            if (modes[i] == &gModeSioBattle || modes[i] == &gModeVsbattle)
                ResetGameState();
            ModeRequest(modes[i], arg);
            return 1;
        }
    }
    return 0;
}
#endif

vu16 gFrameSyncFlags;
u16 gVBlankEndVCount;
u32 gUnk_03006C04[3];
u32 gDebugFlags;
IntrFunc* gIntrTableSerial;
u32 gSoftResetMarker[2];
IntrFunc gIntrTable[INTR_COUNT];
IntrFunc* gIntrTableVCount;
IntrFunc* gIntrTableVBlank;
IntrFunc* gIntrTableTimer3;
IntrFunc gHBlankCallback;
vu32 gVBlankCounter;
IntrFunc gVCountCallback;
IntrFunc gVBlankCallback;
IntrFunc* gIntrTableHBlank;
vu16 gSystemFlags;
u8 gUnk_03006C7A[6];
u8 gIntrHandler[0x800];
vu32 gFrameCounter;
#ifdef VERSION_EU
u32 gLanguage;
#endif
IntrFunc gVBlankHandlerOverride;

void* GetEwramHeapStart() {
    return gEwramHeapStart;
}

u32 GetEwramHeapSize() {
    return EWRAM_HEAP_SIZE;
}

void* GetIwramHeapStart() {
    return gIwramHeapStart;
}

u32 GetIwramHeapSize() {
    return IWRAM_HEAP_SIZE;
}

void EnableVBlankIntr() {
    REG_IME = 0;
    REG_IE |= INTR_FLAG_VBLANK;
    REG_DISPSTAT |= DISPSTAT_VBLANK_INTR;
    m4aSoundVSyncOn();
    REG_IME = 1;
}

void DisableVBlankIntr() {
    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_VBLANK;
    REG_DISPSTAT &= ~DISPSTAT_VBLANK_INTR;
    m4aSoundVSyncOff();
    REG_IME = 1;
}

void EnableHBlankIntr() {
    REG_IME = 0;
    REG_IE |= INTR_FLAG_HBLANK;
    REG_DISPSTAT |= DISPSTAT_HBLANK_INTR;
    REG_IME = 1;
}

void DisableHBlankIntr() {
    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_HBLANK;
    REG_DISPSTAT &= ~DISPSTAT_HBLANK_INTR;
    REG_IME = 1;
}

#ifdef VERSION_EU
void ClearSystemMemory() {
    RegisterRamReset(RESET_ALL);
    REG_WAITCNT = WAITCNT_SRAM_2 | WAITCNT_WS0_N_3 | WAITCNT_WS0_S_1 | WAITCNT_WS1_N_3 | WAITCNT_WS1_S_1 | WAITCNT_WS2_N_3 | WAITCNT_WS2_S_1 | WAITCNT_PREFETCH_ENABLE;
    CpuFill32(0, (void*)EWRAM_START, EWRAM_SIZE);
    CpuFill32(0, (void*)IWRAM_START, IWRAM_SIZE - 0x200);
    CpuFill32(0, (void*)VRAM, VRAM_SIZE);
}
#endif

void InitSystem() {
#ifdef VERSION_EU
    u32 softReset;

    if (gSoftResetMarker[0] == SOFT_RESET_MAGIC) {
        ClearSystemMemory();
        softReset = TRUE;
    } else {
        ClearSystemMemory();
        softReset = FALSE;
    }
#else
    RegisterRamReset(RESET_ALL);
    REG_WAITCNT = WAITCNT_SRAM_2 | WAITCNT_WS0_N_3 | WAITCNT_WS0_S_1 | WAITCNT_WS1_N_3 | WAITCNT_WS1_S_1 | WAITCNT_WS2_N_3 | WAITCNT_WS2_S_1 | WAITCNT_PREFETCH_ENABLE;
    DmaFill32(3, 0, EWRAM_START, EWRAM_SIZE);
    DmaFill32(3, 0, IWRAM_START, IWRAM_SIZE - 0x200);
#endif
    gVBlankEndVCount = 0;
    gFrameSyncFlags = 0;
    gVBlankHandlerOverride = NULL;
#ifdef VERSION_EU
    gLanguage = LANGUAGE_ENGLISH;
#endif
    REG_IME = 0;
    DmaCopy32(3, IrqHandler, gIntrHandler, sizeof(gIntrHandler));
    INTR_VECTOR = gIntrHandler;
    REG_IE = INTR_FLAG_GAMEPAK;
    REG_IF = INTR_FLAG_GAMEPAK;
    REG_IME = 1;
    InitIntrTable();
    m4aSoundInit();
    m4aSoundVSyncOff();
    IwramHeapInit(GetIwramHeapStart(), GetIwramHeapSize());
    EwramHeapInit(GetEwramHeapStart(), GetEwramHeapSize());
    VTransInit();
    SpriteInit();
    BgInit();
    FadeInit();
    PalletInit();
    SioKeyInit();
    ResetPaletteEffect();
    ResetKeyState();
    SeedRandom(1234567);
    InitDisplayRegs();
    SaveInitSram();
    ScanlineDmaReset();
#ifdef VERSION_EU
    ModeInit(softReset);
#else
    ModeInit();
#endif
#ifdef PLATFORM_PC
#ifdef VERSION_EU
    if (gPortStartupLanguage >= 0)
        gLanguage = gPortStartupLanguage;
#endif
    if (gPortIntroRequested) {
        gPortIntroRequested = 0;
        ResetGameState(); /* as the title screen does before a new game */
        SetupSoraNewGame();
        ModeRequestHeapReset(&gModeMovie, MOVIE_OPENING);
    }
    /* Test harness: play one of the ROM's movies (then what follows it). */
    if (gPortMovieTest >= 0) {
        ResetGameState();
        SetupSoraNewGame();
        ModeRequestHeapReset(&gModeMovie, gPortMovieTest);
        gPortMovieTest = -1;
    }
    /* Test harness: the debug battle menu, preset to one battle and stage
       (A starts it). */
    if (gPortBattleTest >= 0) {
        ResetGameState();
        if (gPortBattleTest & 0x10000)
            SetupRikuNewGame();
        else
            SetupSoraNewGame();
        gChkBtlWork->enemy = gPortBattleTest & 0xFF;
        gChkBtlWork->bg = (gPortBattleTest >> 8) & 0xFF;
        gChkBtlWork->hp = 560; /* the menu's maximum: tests reach the end of the battle */
        ModeRequestHeapReset(&gModeChkbtl, 0);
        gPortBattleTest = -1;
    }
    /* Test harness: the debug event menu, preset to one event (A starts it). */
    if (gPortEventTest >= 0) {
        ResetGameState();
        PortEventSelectPreset(gPortEventTest >> 8, gPortEventTest & 0xFF);
        ModeRequestHeapReset(&gModeEventselect, 0);
        gPortEventTest = -1;
    }
#endif
}

void AgbMain() {
    gFrameCounter = 0;
    gVBlankCounter = 0;
    gSystemFlags = 0;
    gDebugFlags = 0;
    gSioPlayerId = 0;
    gSioStatus = 0;
    InitSystem();
    EnableVBlankIntr();

    for (;;) {
        UpdateKeyState();

        if (gSystemFlags & SYSTEM_FLAG_LINK_ACTIVE) {
            SioLinkUpdate();

            if (!(gSioStatus & SIO_STAT_RECV_EMPTY)) {
                if (!(gFrameSyncFlags & FRAME_SYNC_FRAME_READY)) {
                    ModeUpdate();
                    gFrameSyncFlags |= FRAME_SYNC_FRAME_READY;
                }
            }
        } else {
            if (!(gFrameSyncFlags & FRAME_SYNC_FRAME_READY)) {
                ModeUpdate();
                gFrameSyncFlags |= FRAME_SYNC_FRAME_READY;
            }
        }

        ApplyIntrCallbacks();
        VBlankIntrWait();
        gFrameCounter++;
    }
}

void VBlankIntr() {
    if (gVBlankHandlerOverride != NULL) {
        gVBlankHandlerOverride();
        return;
    }

    if (gFrameSyncFlags & FRAME_SYNC_IN_VBLANK) {
        gFrameSyncFlags |= FRAME_SYNC_VBLANK_OVERRUN;
        return;
    }

    gFrameSyncFlags |= FRAME_SYNC_IN_VBLANK;

    if (!(gFrameSyncFlags & FRAME_SYNC_SOUND_BUSY)) {
        m4aSoundVSync();
    }

    gIntrCheck |= INTR_FLAG_VBLANK;

    if (gFrameSyncFlags & FRAME_SYNC_FRAME_READY) {
        ModeFlushDisplay();
    }

    ModeRunVBlankCallbacks();
    ScanlineDmaUpdate();
    gVBlankEndVCount = REG_VCOUNT;
    gFrameSyncFlags &= ~FRAME_SYNC_FRAME_READY;

    if (!(gFrameSyncFlags & FRAME_SYNC_SOUND_BUSY)) {
        gFrameSyncFlags |= FRAME_SYNC_SOUND_BUSY;
        m4aSoundMain();
        gFrameSyncFlags &= ~FRAME_SYNC_SOUND_BUSY;
    }

    gFrameSyncFlags &= ~FRAME_SYNC_IN_VBLANK;
    gVBlankCounter++;
}

void HBlankIntrDummy() {
}

void VCountIntrDummy() {
}

void SerialIntrDummy() {
}

static IntrFunc sIntrTableTemplate[INTR_COUNT] = {
    SerialIntrDummy, VBlankIntr, HBlankIntrDummy, VCountIntrDummy, SerialIntrDummy, SerialIntrDummy, SerialIntrDummy,
    SerialIntrDummy, SerialIntrDummy, SerialIntrDummy, SerialIntrDummy, SerialIntrDummy, SerialIntrDummy, SerialIntrDummy,
};

void InitIntrTable() {
    s32 i;

    for (i = 0; i < INTR_COUNT; i++) {
        gIntrTable[i] = sIntrTableTemplate[i];
    }

    gIntrTableVBlank = &gIntrTable[1];
    gIntrTableVCount = &gIntrTable[3];
    gIntrTableHBlank = &gIntrTable[2];
    gIntrTableSerial = &gIntrTable[0];
    gIntrTableTimer3 = &gIntrTable[7];
    ResetVBlankCallback();
    ResetVCountCallback();
    ResetHBlankCallback();
    ResetSerialCallback();
    ResetTimer3Callback();
}

void ApplyIntrCallbacks() {
    *gIntrTableVBlank = gVBlankCallback;
    *gIntrTableVCount = gVCountCallback;
    *gIntrTableHBlank = gHBlankCallback;
}

void VBlankIntrSio() {
    if (gFrameSyncFlags & FRAME_SYNC_IN_VBLANK) {
        gFrameSyncFlags |= FRAME_SYNC_VBLANK_OVERRUN;
        return;
    }

    gFrameSyncFlags |= FRAME_SYNC_IN_VBLANK;
    SioVBlankUpdate();

    if (!(gFrameSyncFlags & FRAME_SYNC_SOUND_BUSY)) {
        m4aSoundVSync();
    }

    gIntrCheck |= INTR_FLAG_VBLANK;

    if (gFrameSyncFlags & FRAME_SYNC_FRAME_READY) {
        ModeFlushDisplay();
    }

    ModeRunVBlankCallbacks();
    gVBlankEndVCount = REG_VCOUNT;
    gFrameSyncFlags &= ~FRAME_SYNC_FRAME_READY;

    if (!(gFrameSyncFlags & FRAME_SYNC_SOUND_BUSY)) {
        gFrameSyncFlags |= FRAME_SYNC_SOUND_BUSY;
        m4aSoundMain();
        gFrameSyncFlags &= ~FRAME_SYNC_SOUND_BUSY;
    }

    gFrameSyncFlags &= ~FRAME_SYNC_IN_VBLANK;
    gVBlankCounter++;
}

void VBlankIntrBlockAudio() {
    if (gFrameSyncFlags & FRAME_SYNC_IN_VBLANK) {
        gFrameSyncFlags |= FRAME_SYNC_VBLANK_OVERRUN;
        return;
    }

    gFrameSyncFlags |= FRAME_SYNC_IN_VBLANK;
    REG_IME = 0;
    BlockAudioVBlank();
    REG_IME = 1;
    gIntrCheck |= INTR_FLAG_VBLANK;

    if (gFrameSyncFlags & FRAME_SYNC_FRAME_READY) {
        ModeFlushDisplay();
    }

    ModeRunVBlankCallbacks();
    ScanlineDmaUpdate();
    gVBlankEndVCount = REG_VCOUNT;
    gFrameSyncFlags &= ~FRAME_SYNC_FRAME_READY;
    gFrameSyncFlags &= ~FRAME_SYNC_IN_VBLANK;
    gVBlankCounter++;
}
