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
static unsigned sPreviewFrame;
#ifdef KH_OFFLINE_EVAL
static KhOfflineState sOffline;
static char sOfflinePath[1200];
static rc_runtime_t sOfflineRuntime;
static int sOfflineReady;
static uint64_t sPendingNotices[(KH_OFFLINE_MAX + 63) / 64];

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

/* The session is the unit: a cheat (a toggle turned on, or an action), quick states, a rematch or an edited save
   pause achievements until the game is restarted, even if the toggle is turned off again (a toggled run cannot be
   told apart from a clean one afterwards). The texts in Options and in the launcher say so. */
void PortAchievementsInvalidate(const char* reason) {
    if (sInvalidated) return;
    sInvalidated = 1;
    PortLog("achievements: session paused (%s)", reason);
#ifdef PORT_RETROACHIEVEMENTS
    KhRaInvalidate(sRa, reason);
#endif
}

/* ---- edited saves ---------------------------------------------------------------------------------------------
   The launcher's save editor leaves <save>.edited beside the save it changed ("files=<mask>": bit 0 the resume
   save, bit f + 1 save file f, Sora's 0-1 and Riku's 2-3, as save.c numbers them). Loading a marked file pauses
   achievements, as a cheat does; a file saved from a game that came from a marked file, or where a cheat action
   (max level, cards, journal) changed the progress, is marked too, and one saved from a clean game is cleared. */
static int sTainted; /* the game being played: from a marked file, or changed by a cheat action */
static Mode* sLastMode;

static const char* EditedMarkPath(void) {
    char name[96];
    SDL_snprintf(name, sizeof(name), "%s.edited", PortSaveName());
    return PortConfigPath(name);
}

static unsigned EditedMarkRead(void) {
    unsigned mask = 0, v;
    char line[96];
    FILE* f = fopen(EditedMarkPath(), "r");
    if (f == NULL)
        return 0;
    while (fgets(line, sizeof(line), f))
        if (SDL_sscanf(line, "files=%u", &v) == 1)
            mask = v;
    fclose(f);
    return mask & 31u;
}

static void EditedMarkWrite(unsigned mask) {
    FILE* f;
    if (mask == 0) {
        remove(EditedMarkPath());
        return;
    }
    f = fopen(EditedMarkPath(), "w");
    if (f == NULL)
        return;
    fprintf(f, "; ReKHCoM: save files changed by the launcher's save editor or a cheat; achievements pause with them\n"
               "; bit 1 the resume save, 2 Sora file 1, 4 Sora file 2, 8 Riku file 1, 16 Riku file 2\n"
               "files=%u\n", mask);
    fclose(f);
}

void PortAchievementsTaintSession(void) {
    sTainted = 1;
}

/* save.c: the game loaded save file `file` (-1: the resume save) */
void PortAchievementsFileLoaded(int file) {
    sTainted = (EditedMarkRead() >> (file + 1)) & 1u;
    if (sTainted)
        PortAchievementsInvalidate("Edited save loaded");
}

/* save.c: the game wrote save file `file` (-1: the resume save) */
void PortAchievementsFileSaved(int file) {
    unsigned mask = EditedMarkRead(), bit = 1u << (file + 1);
    unsigned next = sTainted ? mask | bit : mask & ~bit;
    if (next != mask)
        EditedMarkWrite(next);
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
    int evaluate;
#ifdef PORT_RETROACHIEVEMENTS
    DryRunFrame();
#endif
    ++sPreviewFrame;
    if (SDL_getenv("KHCOM_ACHIEVEMENT_PREVIEW") &&
        sPreviewFrame == (unsigned)SDL_atoi(SDL_getenv("KHCOM_ACHIEVEMENT_PREVIEW")))
        PortAchievementToast("Heartless Hunter", "Defeat your first Heartless in Castle Oblivion.", "VISTA PREVIA - LOGRO", SDL_atoi(SDL_getenv("KHCOM_ACHIEVEMENT_BADGE") ? SDL_getenv("KHCOM_ACHIEVEMENT_BADGE") : "0"));
    /* Invalidate before this frame can evaluate or submit an official award. */
    if (gPortConfig.cheatOneHit || gPortConfig.cheatInfiniteHp || gPortConfig.cheatUnlockRiku || gPortConfig.cheatSaveStates ||
        gPortConfig.ps2Mode)
        PortAchievementsInvalidate("Gameplay modifiers enabled");
    if (gCurrentMode == &gModeTitle) sStarted = 0;
    /* back at the title: the next game is a loaded file (its own mark) or a new one; only on arriving, since the
       resume save is loaded while the title is still the mode */
    if (gCurrentMode == &gModeTitle && sLastMode != &gModeTitle) sTainted = 0;
    sLastMode = gCurrentMode;
    if (gCurrentMode == &gModeMapFld || gCurrentMode == &gModeBattle) sStarted = 1;
    evaluate = !sInvalidated && (gCurrentMode == &gModeMapFld || gCurrentMode == &gModeBattle ||
                                 gCurrentMode == &gModeTitle || sStarted);
#ifdef PORT_RETROACHIEVEMENTS
    KhRaPump(sRa, evaluate);
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
    if (evaluate && sOfflineReady)
        rc_runtime_do_frame(&sOfflineRuntime, OfflineEvent, OfflinePeek, NULL, NULL);
    if (!PortUiActive()) {
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
