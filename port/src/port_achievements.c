#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#if __has_include(<time.h>)
#include <time.h>
#define KH_WALL_CLOCK 1
#endif
#include "port/port.h"
#include "port/port_ui.h"
#include "game_state.h"
#include "mode.h"
#include "../achievements/offline_achievements.c"
/* the language of the official set's text (index into KhOfficialAchievement.text) */
#ifdef VERSION_EU
extern uint32_t gLanguage;
#define ACHIEVEMENT_TEXT_LANGUAGE ((int)gLanguage)
#elif defined(VERSION_JP)
#define ACHIEVEMENT_TEXT_LANGUAGE 5 /* Japanese */
#else
#define ACHIEVEMENT_TEXT_LANGUAGE 0 /* English */
#endif
/* the official set is evaluated by the game itself (rcheevos' runtime, no
   account): the European cartridge, wherever rcheevos is built in */
#if defined(VERSION_EU) && (defined(PORT_RETROACHIEVEMENTS) || defined(PORT_OFFLINE_ACHIEVEMENTS))
#define KH_OFFLINE_EVAL 1
#include "rc_runtime.h"
int PortRaReadMemory(uint32_t address, uint8_t* buffer, uint32_t size);
#endif
#ifdef PORT_RETROACHIEVEMENTS
int PortRaReadMemory(uint32_t address, uint8_t* buffer, uint32_t size);
static uint32_t PortRaRead(uint32_t address, uint8_t* out, uint32_t size, void* userdata) {
    (void)userdata;
    return PortRaReadMemory(address, out, size) ? size : 0;
}
#ifdef VERSION_EU
/* Official achievements whose reads are all verified (build/ra-reference-audit/enable.json;
   the battle ones by ra_battle_dryrun.py: won triggers, lost never; the
   challenge ones also never with any of their rules broken; the endings by
   ra_ending_dryrun.py). */
static const uint32_t sOfficialVerified[] = { 15580u, 15582u, 15583u, 15584u, 15585u, 15587u, 15588u, 15590u, 15591u, 15592u, 15594u, 15595u, 15596u, 15597u, 15598u, 15599u, 15600u, 15601u, 15602u, 15603u, 15604u, 15605u, 15611u, 15612u, 15619u, 15651u, 15652u, 15653u, 15654u, 15655u, 15656u, 15657u, 15658u, 15659u, 15660u, 15661u, 15662u, 15663u, 15664u, 15665u, 15666u, 41892u, 156793u, 160363u, 160364u, 160365u, 160366u, 160367u, 160368u, 160369u, 160370u, 160371u, 160372u, 160373u, 160374u, 160375u, 160376u, 160377u, 160378u, 160379u, 160380u, 160381u, 160382u, 160383u, 160384u, 160385u, 160386u, 160387u, 160388u, 160389u, 160390u, 160391u, 160392u, 160393u, 160394u, 160395u, 160396u, 160397u, 160398u, 160399u, 160400u, 160401u, 160402u, 160403u, 160404u, 160405u, 160406u, 160407u, 160408u, 160409u };
#endif
#endif

#ifdef PORT_RETROACHIEVEMENTS
#include "../achievements/ra_client.h"
static KhRa* sRa;
#endif

extern Mode* gCurrentMode;
extern Mode gModeMapFld, gModeBattle, gModeTitle;
static int sInvalidated, sStarted;
/* Rebel mode (user 25/10): a session paused by cheats (not by a test run) still earns the official set, in a list of
   its own kept on this computer only (achievements_rebel_eu.dat), never sent to RetroAchievements. The first time
   it happens rebel_mode.txt is written and a sign says what it is; from then on Cheats shows its section. */
static int sRebel;
static unsigned sPreviewFrame;
#ifdef KH_OFFLINE_EVAL
static KhOfflineState sOffline;
static char sOfflinePath[1200];
static rc_runtime_t sOfflineRuntime;
static int sOfflineReady;
static uint64_t sPendingNotices[(KH_OFFLINE_MAX + 63) / 64];
static KhOfflineState sRebelState;
static char sRebelPath[1200];
static rc_runtime_t sRebelRuntime;
static int sRebelReady;

static uint32_t OfflinePeek(uint32_t address, uint32_t bytes, void* ud) {
    uint8_t b[4] = { 0, 0, 0, 0 };
    (void)ud;
    if (bytes > 4 || !PortRaReadMemory(address, b, bytes))
        return 0;
    return b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

static void OfflineEvent(const rc_runtime_event_t* event) {
    int index;
    uint64_t now = 1; /* a guest without a wall clock stores an unknown time */
    if (event->type != RC_RUNTIME_EVENT_ACHIEVEMENT_TRIGGERED)
        return;
    index = KhOfficialIndex(event->id);
    rc_runtime_deactivate_achievement(&sOfflineRuntime, event->id);
    if (index < 0 || sOffline.when[index])
        return;
#ifdef KH_WALL_CLOCK
    now = (uint64_t)time(NULL);
#endif
    sOffline.when[index] = now ? now : 1;
    PortLog("achievement: %u unlocked", (unsigned)event->id);
    if (!KhOfflineSave(&sOffline, sOfflinePath)) PortLog("achievements: could not save progress");
#ifdef __EMSCRIPTEN__
    else EM_ASM({ if (Module.persistSoon) Module.persistSoon();
                  if (Module.onAchievementsChanged) Module.onAchievementsChanged(); });
#endif
    sPendingNotices[index / 64] |= 1ull << (index % 64);
}
#endif

#ifdef KH_OFFLINE_EVAL
static void RebelEvent(const rc_runtime_event_t* event) {
    int index;
    if (event->type != RC_RUNTIME_EVENT_ACHIEVEMENT_TRIGGERED)
        return;
    index = KhOfficialIndex(event->id);
    rc_runtime_deactivate_achievement(&sRebelRuntime, event->id);
    if (index < 0 || sRebelState.when[index])
        return;
#ifdef KH_WALL_CLOCK
    sRebelState.when[index] = (uint64_t)time(NULL);
#endif
    if (!sRebelState.when[index])
        sRebelState.when[index] = 1;
    PortLog("achievement: rebel %u unlocked", (unsigned)event->id);
    KhOfflineSave(&sRebelState, sRebelPath);
    {
        static const char* const kLabel[6] = { "REBEL MODE", "MODE REBELLE", "REBELLEN-MODUS", "MODALITÀ RIBELLE",
            "MODO REBELDE", "反逆モード" };
        int l = ACHIEVEMENT_TEXT_LANGUAGE;
        if (l < 0 || l > 5) l = 0;
        PortAchievementToast(KhOfficialText(KhOfficialAt(index), ACHIEVEMENT_TEXT_LANGUAGE, 0),
                             KhOfficialText(KhOfficialAt(index), ACHIEVEMENT_TEXT_LANGUAGE, 1), kLabel[l], index);
    }
}
#endif
/* a save edited in the launcher (its .rebel mark, port_memory*.c): Rebel mode from its load on */
static void RebelStart(void);
void PortRebelEditedSave(void) {
    PortAchievementsInvalidate("Save edited in the launcher");
    RebelStart();
}
int PortRebelActive(void) {
    return sRebel;
}
/* rebel mode was ever reached here (Cheats shows its section) */
int PortRebelUnlocked(void) {
    FILE* f;
    if (sRebel)
        return 1;
    f = fopen(PortConfigPath("rebel_mode.txt"), "rb");
    if (f)
        fclose(f);
    return f != NULL;
}
int PortRebelEarned(int index) {
#ifdef KH_OFFLINE_EVAL
    return index >= 0 && index < KH_OFFLINE_MAX && sRebelState.when[index] != 0;
#else
    (void)index;
    return 0;
#endif
}
static void RebelStart(void) {
    static const char* const kTitle[6] = { "Rebel mode", "Mode rebelle", "Rebellen-Modus", "Modalità ribelle",
        "Modo rebelde", "反逆モード" };
    static const char* const kText[6] = {
        "Cheats are fine with us! These achievements stay on this computer, in their own list, not on RetroAchievements.",
        "Les astuces, ça nous va ! Ces succès restent sur cet ordinateur, dans leur liste, hors RetroAchievements.",
        "Cheats sind okay! Diese Erfolge bleiben auf diesem PC, in eigener Liste, nicht bei RetroAchievements.",
        "I trucchi ci vanno bene! Questi obiettivi restano su questo PC, in una lista a parte, non su RetroAchievements.",
        "¡Los trucos nos parecen bien! Estos logros quedan en este equipo, en su propia lista, no en RetroAchievements.",
        "チートもOK! これらの実績はこのPCだけの別リストで、RetroAchievementsには送られません。" };
    int l = ACHIEVEMENT_TEXT_LANGUAGE;
    FILE* f;
    if (sRebel)
        return;
    if (!PortRebelUnlocked() && (f = fopen(PortConfigPath("rebel_mode.txt"), "wb")) != NULL) {
        fputs("unlocked\n", f);
        fclose(f);
        if (l < 0 || l > 5) l = 0;
        PortAchievementToast(kTitle[l], kText[l], "", -1);
    }
    sRebel = 1;
}

int PortEventId(void); /* mode_event.c */
#include "port_achievements_recom.inc"
#include "port_achievement_list.inc"

void PortAchievementsInit(void) {
#ifdef KH_OFFLINE_EVAL
    int i;
    snprintf(sOfflinePath, sizeof(sOfflinePath), "%s", PortConfigPath("achievements_official_eu.dat"));
    KhOfflineLoad(&sOffline, sOfflinePath);
    rc_runtime_init(&sOfflineRuntime);
    for (i = 0; i < KhOfficialCount(); ++i)
        if (!sOffline.when[i] &&
            rc_runtime_activate_achievement(&sOfflineRuntime, KhOfficialAt(i)->id, KhOfficialAt(i)->definition, NULL, 0) == RC_OK)
            sOfflineReady++;
    PortLog("achievements: %d of %d official achievements to earn", sOfflineReady, KhOfficialCount());
    snprintf(sRebelPath, sizeof(sRebelPath), "%s", PortConfigPath("achievements_rebel_eu.dat"));
    KhOfflineLoad(&sRebelState, sRebelPath);
    rc_runtime_init(&sRebelRuntime);
    for (i = 0; i < KhOfficialCount(); ++i)
        if (!sRebelState.when[i] &&
            rc_runtime_activate_achievement(&sRebelRuntime, KhOfficialAt(i)->id, KhOfficialAt(i)->definition, NULL, 0) == RC_OK)
            sRebelReady++;
    RecomInit();
#endif
#ifdef PORT_RETROACHIEVEMENTS
    /* Official set (EU cartridge): evaluated from the port's memory as the
     * cartridge lays it out (port_ra_memory.c / port_ra_state.c). Only the
     * achievements whose every read is verified against the ROM are active;
     * the rest stay listed but disabled until their memory is verified
     * (queue item 71). */
    sRa = KhRaCreate(PortConfigPath("."), PortRaRead, NULL, NULL);
#ifdef VERSION_EU
    KhRaStartOfficial(sRa, sOfficialVerified, sizeof(sOfficialVerified) / sizeof(sOfficialVerified[0]));
#endif
#endif
    /* harness runs earn nothing, unless a test checks the unlocks themselves
       (KHCOM_ACHIEVEMENTS_LIVE=1, with its own data folder) */
    if ((SDL_getenv("KHCOM_MODE") || SDL_getenv("KHCOM_BOT") || SDL_getenv("KHCOM_BATTLE") ||
         SDL_getenv("KHCOM_EVENT") || SDL_getenv("KHCOM_KEYS")) && !SDL_getenv("KHCOM_ACHIEVEMENTS_LIVE")) sInvalidated = 1;
}

void PortAchievementsRomLoaded(const uint8_t* rom, size_t size) {
#ifdef PORT_RETROACHIEVEMENTS
    KhRaSetRom(sRa, rom, size);
#else
    (void)rom; (void)size;
#endif
}

/* Issue #78: a paused session was only in kh_com.log, so players in Re:CoM mode thought achievements were
   broken. The first time play starts paused, a toast says why (game language: en fr de it es ja). */
static int sPauseNotice; /* 0 none, 1 Re:CoM mode, 2 cheats or other modifiers; -1 shown */

static void PauseNoticeFrame(void) {
    static const char* const kTitle[6] = { "Achievements paused", "Succès en pause", "Erfolge pausiert",
        "Obiettivi in pausa", "Logros en pausa", "実績は一時停止中" };
    static const char* const kRecomTitle[6] = { "Re:CoM achievements", "Succès Re:CoM", "Re:CoM-Erfolge",
        "Obiettivi Re:CoM", "Logros Re:CoM", "Re:CoM 実績" };
    static const char* const kRecom[6] = { "Their own list, kept offline.",
        "Leur propre liste, hors ligne.",
        "Eigene Liste, nur offline.",
        "Lista propria, solo offline.",
        "Su propia lista, sin conexión.",
        "専用リスト(オフライン)。" };
    static const char* const kCheat[6] = { "Cheats or debug tools were used.",
        "Astuces ou débogage utilisés.",
        "Cheats oder Debug benutzt.",
        "Usati trucchi o debug.",
        "Se usaron trucos o depuración.",
        "チートかデバッグが使われました。" };
    int l = ACHIEVEMENT_TEXT_LANGUAGE;

    if (sPauseNotice <= 0 || (gCurrentMode != &gModeMapFld && gCurrentMode != &gModeBattle)) return;
    if (l < 0 || l > 5) l = 0;
    PortAchievementToast(sPauseNotice == 1 ? kRecomTitle[l] : kTitle[l], sPauseNotice == 1 ? kRecom[l] : kCheat[l], "", -1);
    sPauseNotice = -1;
}

void PortAchievementsInvalidate(const char* reason) {
    if (sInvalidated) return;
    sInvalidated = 1;
    if (sPauseNotice == 0)
        sPauseNotice = 2;
    PortLog("achievements: session paused (%s)", reason);
#ifdef PORT_RETROACHIEVEMENTS
    KhRaInvalidate(sRa, reason);
#endif
}

#ifdef PORT_RETROACHIEVEMENTS
/* Test harness (KHCOM_RA_DRYRUN=definitions.json, the catalog audit_ra_references.py
   saved): every official achievement is evaluated each frame against the port's
   memory with rcheevos' own runtime, offline, and each trigger is logged
   ("ra-dryrun: trigger <id> frame <n>"). Nothing is submitted; no account. */
#include "rc_runtime.h"
static rc_runtime_t sDryRuntime;
static int sDryReady = -1;
static unsigned sDryFrame;

static uint32_t DryPeek(uint32_t address, uint32_t bytes, void* ud) {
    uint8_t b[4] = { 0, 0, 0, 0 };
    (void)ud;
    if (bytes > 4 || !PortRaReadMemory(address, b, bytes))
        return 0;
    return b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

static void DryEvent(const rc_runtime_event_t* event) {
    if (event->type == RC_RUNTIME_EVENT_ACHIEVEMENT_TRIGGERED)
        PortLog("ra-dryrun: trigger %u frame %u", (unsigned)event->id, sDryFrame);
}

static void DryRunFrame(void) {
    const char* path = SDL_getenv("KHCOM_RA_DRYRUN");
    if (!path)
        return;
    if (sDryReady < 0) {
        size_t size = 0;
        char* json = (char*)SDL_LoadFile(path, &size);
        char* p = json;
        int count = 0;
        sDryReady = 0;
        if (!json)
            return;
        rc_runtime_init(&sDryRuntime);
        /* [{"id": N, "definition": "..."}, ...] */
        while (p && (p = SDL_strstr(p, "\"id\":")) != NULL) {
            unsigned id;
            char* d;
            char* e;
            p += 5;
            while (*p == ' ') p++;
            id = (unsigned)SDL_strtoul(p, NULL, 10);
            d = SDL_strstr(p, "\"definition\":");
            if (!d)
                break;
            d += 13;
            while (*d == ' ') d++;
            if (*d++ != '"')
                break;
            e = SDL_strchr(d, '"');
            if (!e)
                break;
            *e = 0;
            if (id != 101000001u && rc_runtime_activate_achievement(&sDryRuntime, id, d, NULL, 0) == RC_OK)
                count++;
            p = e + 1;
        }
        PortLog("ra-dryrun: %d official achievements active", count);
        SDL_free(json);
        sDryReady = 1;
    }
    if (sDryReady == 1) {
        ++sDryFrame;
        rc_runtime_do_frame(&sDryRuntime, DryEvent, DryPeek, NULL, NULL);
        /* KHCOM_RA_WATCH=1: the battle graphs' operands, logged when they change
           (mode argument, world, card battle root and its +0xCC, the effect
           root's -0x330 and -0x174 neighbours as the official AddAddress reads) */
        if (SDL_getenv("KHCOM_RA_WATCH")) {
            static char last[240];
            char line[240];
            uint32_t fx = DryPeek(0x3CA28, 3, NULL), cbs = DryPeek(0x423E4, 3, NULL);
            SDL_snprintf(line, sizeof(line),
                         "arg=%u world=%u cbs=%06x cbs+cc=%u fx=%06x fx-330=%u fx-174=%u fx-1a8=%u fx-364=%u gs=%u/%u/%u x3bbc8=%u",
                         (unsigned)DryPeek(0x749C, 2, NULL), (unsigned)DryPeek(0x42346, 1, NULL), (unsigned)cbs,
                         (unsigned)(cbs ? DryPeek(cbs + 0x80CC, 2, NULL) : 0), (unsigned)fx,
                         (unsigned)(fx ? DryPeek(fx + 0x7CD0, 2, NULL) : 0), (unsigned)(fx ? DryPeek(fx + 0x7E8C, 2, NULL) : 0),
                         (unsigned)(fx ? DryPeek(fx + 0x7E58, 2, NULL) : 0), (unsigned)(fx ? DryPeek(fx + 0x7C9C, 2, NULL) : 0),
                         (unsigned)DryPeek(0x421CC, 1, NULL), (unsigned)DryPeek(0x421CD, 1, NULL),
                         (unsigned)DryPeek(0x421CE, 1, NULL), (unsigned)DryPeek(0x3BBC8, 4, NULL));
            if (SDL_strcmp(line, last) != 0) {
                SDL_strlcpy(last, line, sizeof(last));
                PortLog("ra-watch: frame %u %s", sDryFrame, line);
            }
        }
    }
}
#endif

void PortAchievementsFrame(void) {
    int evaluate, recom;
#ifdef PORT_RETROACHIEVEMENTS
    DryRunFrame();
#endif
    ++sPreviewFrame;
    if (SDL_getenv("KHCOM_ACHIEVEMENT_PREVIEW") &&
        sPreviewFrame == (unsigned)SDL_atoi(SDL_getenv("KHCOM_ACHIEVEMENT_PREVIEW")))
        PortAchievementToast("Heartless Hunter", "Defeat your first Heartless in Castle Oblivion.", "VISTA PREVIA - LOGRO", SDL_atoi(SDL_getenv("KHCOM_ACHIEVEMENT_BADGE") ? SDL_getenv("KHCOM_ACHIEVEMENT_BADGE") : "0"));
    /* Invalidate before this frame can evaluate or submit an official award. */
    if (gPortConfig.cheatOneHit || gPortConfig.cheatInfiniteHp || gPortConfig.cheatUnlockRiku || gPortConfig.cheatSaveStates) {
        PortAchievementsInvalidate("Gameplay modifiers enabled");
        RebelStart(); /* not a test run: those never reach this (sInvalidated from the start) */
    }
    /* a game of the Re:CoM version earns the Re:CoM list instead; the official one (and the account) waits */
    recom = gPortConfig.ps2Mode && PortRecomActive();
    if (recom && !sInvalidated && sPauseNotice == 0 && sStarted)
        sPauseNotice = 1;
    PauseNoticeFrame();
    if (gCurrentMode == &gModeTitle) sStarted = 0;
    if (gCurrentMode == &gModeMapFld || gCurrentMode == &gModeBattle) sStarted = 1;
    evaluate = !sInvalidated && (gCurrentMode == &gModeMapFld || gCurrentMode == &gModeBattle ||
                                 gCurrentMode == &gModeTitle || sStarted);
#ifdef PORT_RETROACHIEVEMENTS
    KhRaPump(sRa, evaluate && !recom);
    {
        const char* notice = KhRaTakeNotice(sRa);
        if (notice) {
            char title[256];
            const char* nl = SDL_strchr(notice, '\n');
            SDL_strlcpy(title, notice, nl ? (size_t)(nl - notice + 1) : sizeof(title));
            int index = KhOfficialIndex(KhRaNoticeId(sRa));
            if (index >= 0) /* the set's own text in the game's language */
                PortAchievementToast(KhOfficialText(KhOfficialAt(index), ACHIEVEMENT_TEXT_LANGUAGE, 0),
                                     KhOfficialText(KhOfficialAt(index), ACHIEVEMENT_TEXT_LANGUAGE, 1), "", index);
            else
                PortAchievementToast(title, nl ? nl + 1 : "", "", -1);
        }
    }
#endif
#ifdef KH_OFFLINE_EVAL
    if (evaluate && !recom && sOfflineReady)
        rc_runtime_do_frame(&sOfflineRuntime, OfflineEvent, OfflinePeek, NULL, NULL);
    if (sRebel && sRebelReady && !PortUiActive() && /* Re:CoM games with cheats too: one Rebel list */
        (gCurrentMode == &gModeMapFld || gCurrentMode == &gModeBattle || sStarted))
        rc_runtime_do_frame(&sRebelRuntime, RebelEvent, OfflinePeek, NULL, NULL);
    {
        /* harness: KHCOM_CREDITS=frame:movie reaches a story's credits (with KHCOM_ACHIEVEMENTS_LIVE) */
        const char* c = SDL_getenv("KHCOM_CREDITS");
        unsigned f; int m;
        if (c && SDL_getenv("KHCOM_ACHIEVEMENTS_LIVE") && SDL_sscanf(c, "%u:%d", &f, &m) == 2 && f == sPreviewFrame)
            PortAchievementsMovie(m);
    }
    RecomFrame(evaluate && recom);
    if (!PortUiActive()) {
        RecomNoticesFrame();
        int i, account = 0, spanish;
#ifdef PORT_RETROACHIEVEMENTS
        /* signed in: RetroAchievements' own notice announces it; here it is only kept */
        {
            rc_client_t* client = KhRaClient(sRa);
            account = client && rc_client_get_user_info(client) && rc_client_is_game_loaded(client);
        }
#endif
        spanish = ACHIEVEMENT_TEXT_LANGUAGE == 4;
        for (i = 0; i < KhOfficialCount(); ++i)
            if (sPendingNotices[i / 64] & (1ull << (i % 64))) {
                sPendingNotices[i / 64] &= ~(1ull << (i % 64));
                if (!account)
                    PortAchievementToast(KhOfficialText(KhOfficialAt(i), ACHIEVEMENT_TEXT_LANGUAGE, 0), KhOfficialText(KhOfficialAt(i), ACHIEVEMENT_TEXT_LANGUAGE, 1),
                                         spanish ? "LOGRO DESBLOQUEADO" : "ACHIEVEMENT UNLOCKED", i);
            }
    }
#endif
}

void PortAchievementsShutdown(void) {
    PortAchievementToastShutdown();
#ifdef PORT_RETROACHIEVEMENTS
    KhRaDestroy(sRa); sRa = NULL;
#endif
}
