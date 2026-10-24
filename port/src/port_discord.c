/*
 * Discord Rich Presence: the player's Discord profile shows the game, the world and floor, the character and the
 * level ("Playing Kingdom Hearts: Chain of Memories" comes from the Discord application's name).
 *
 * It speaks Discord's local IPC directly (no SDK): a named pipe on Windows (\\.\pipe\discord-ipc-N), a Unix socket
 * on Linux ($XDG_RUNTIME_DIR/discord-ipc-N and the Flatpak and Snap folders). Each message is an opcode, a length and
 * JSON: a handshake with the application id, then SET_ACTIVITY. Nothing happens when Discord is not running; the
 * game retries every few seconds. Off with discord_presence=0 (Options, launcher).
 *
 * Desktop builds only (Windows, Linux): Android, the web and the console images get the empty functions below.
 */
#include "port/port.h"

/* The Discord application (discord.com/developers): its name is the game shown in the profile. KHCOM_DISCORD_APP_ID
   replaces it for tests. */
#ifndef REKHCOM_DISCORD_APP_ID
#define REKHCOM_DISCORD_APP_ID "1562303132728426516"
#endif
/* Pictures by URL (Discord fetches them): the repository's port/assets/discord. */
#define DISCORD_ART "https://raw.githubusercontent.com/MatyMeatBoy/ReKHCom/main/port/assets/discord/"

#if (defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))) && __STDC_HOSTED__
#define DISCORD_IPC 1
#endif

#ifdef DISCORD_IPC
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "game_state.h"
#include "menu/world_types.h"
#include "mode.h"
#include "registration_data.h"
#include "btl/mode_battle_data.h"
#include "evt/mode_event.h"
#include "btl/battle_ids.h"
#include "btl/battle_work.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static HANDLE sPipe = INVALID_HANDLE_VALUE;
#else
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
static int sSock = -1;
#endif

extern Mode* gCurrentMode;

static int sConnected;
static uint64_t sNextTry, sNextSend;
static int64_t sStart;
static char sSent[512], sWanted[512];
static unsigned sNonce;

static int Write(const void* data, uint32_t size) {
#ifdef _WIN32
    DWORD done;
    return WriteFile(sPipe, data, size, &done, NULL) && done == size;
#else
    return send(sSock, data, size, MSG_NOSIGNAL) == (ssize_t)size;
#endif
}

static void Close(void) {
#ifdef _WIN32
    if (sPipe != INVALID_HANDLE_VALUE)
        CloseHandle(sPipe);
    sPipe = INVALID_HANDLE_VALUE;
#else
    if (sSock >= 0)
        close(sSock);
    sSock = -1;
#endif
    sConnected = 0;
    sSent[0] = 0;
}

/* Discord's answers are not needed: read them away so the pipe never fills. A closed pipe ends the connection. */
static void Drain(void) {
    char buf[1024];
#ifdef _WIN32
    DWORD avail = 0, got;
    if (!PeekNamedPipe(sPipe, NULL, 0, NULL, &avail, NULL)) {
        Close();
        return;
    }
    while (avail > 0 && ReadFile(sPipe, buf, avail < sizeof(buf) ? avail : sizeof(buf), &got, NULL) && got > 0)
        avail -= got < avail ? got : avail;
#else
    for (;;) {
        ssize_t n = recv(sSock, buf, sizeof(buf), 0);
        if (n > 0)
            continue;
        if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
            Close();
        break;
    }
#endif
}

static int Frame(uint32_t op, const char* json) {
    uint32_t len = (uint32_t)strlen(json), head[2] = { op, len };
    if (!Write(head, sizeof(head)) || !Write(json, len)) {
        Close();
        return 0;
    }
    return 1;
}

static int Open(void) {
    const char* test = SDL_getenv("KHCOM_DISCORD_IPC"); /* tests: this pipe or socket instead of Discord's */
    int i;
#ifdef _WIN32
    if (test != NULL) {
        sPipe = CreateFileA(test, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        return sPipe != INVALID_HANDLE_VALUE;
    }
    for (i = 0; i < 10; i++) {
        char name[64];
        SDL_snprintf(name, sizeof(name), "\\\\.\\pipe\\discord-ipc-%d", i);
        sPipe = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (sPipe != INVALID_HANDLE_VALUE)
            return 1;
    }
#else
    static const char* const vars[] = { "XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP" };
    static const char* const subdirs[] = { "", "/app/com.discordapp.Discord", "/snap.discord" };
    size_t v, s;
    if (test != NULL) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        SDL_strlcpy(addr.sun_path, test, sizeof(addr.sun_path));
        sSock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sSock >= 0 && connect(sSock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            fcntl(sSock, F_SETFL, fcntl(sSock, F_GETFL) | O_NONBLOCK);
            return 1;
        }
        if (sSock >= 0)
            close(sSock);
        sSock = -1;
        return 0;
    }
    for (v = 0; v <= sizeof(vars) / sizeof(vars[0]); v++) {
        const char* dir = v < sizeof(vars) / sizeof(vars[0]) ? getenv(vars[v]) : "/tmp";
        if (dir == NULL || !*dir)
            continue;
        for (s = 0; s < sizeof(subdirs) / sizeof(subdirs[0]); s++)
            for (i = 0; i < 10; i++) {
                struct sockaddr_un addr;
                memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_UNIX;
                SDL_snprintf(addr.sun_path, sizeof(addr.sun_path), "%s%s/discord-ipc-%d", dir, subdirs[s], i);
                sSock = socket(AF_UNIX, SOCK_STREAM, 0);
                if (sSock < 0)
                    return 0;
                if (connect(sSock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                    fcntl(sSock, F_SETFL, fcntl(sSock, F_GETFL) | O_NONBLOCK);
                    return 1;
                }
                close(sSock);
                sSock = -1;
            }
    }
#endif
    return 0;
}

/* A test run (hidden window or a scheduled quit) never shows itself in the player's Discord profile: the batteries
   start hundreds of games while the player plays. KHCOM_DISCORD_IPC (the presence's own tests) still connects. */
static int HarnessRun(void) {
    static int known = -1;
    if (known < 0) {
        const char* video = SDL_getenv("SDL_VIDEO_DRIVER");
        known = SDL_getenv("KHCOM_DISCORD_IPC") == NULL
                && (SDL_getenv("KHCOM_QUIT") != NULL
                    || (video != NULL && (strcmp(video, "dummy") == 0 || strcmp(video, "offscreen") == 0)));
    }
    return known;
}

static const char* AppId(void) {
    const char* id = SDL_getenv("KHCOM_DISCORD_APP_ID");
    return id != NULL && *id ? id : REKHCOM_DISCORD_APP_ID;
}

/* The game's language: English, French, German, Italian, Spanish, Japanese (as port_ui.c's Lang()). */
#ifdef VERSION_EU
extern uint32_t gLanguage;
#endif
enum { DL_EN, DL_FR, DL_DE, DL_IT, DL_ES, DL_JA, DL_COUNT };
static int Lang(void) {
#ifdef VERSION_EU
    return gLanguage < DL_JA ? (int)gLanguage : DL_EN;
#elif defined(VERSION_JP)
    return DL_JA;
#else
    return DL_EN;
#endif
}

/* World names as in the translated achievements (port/achievements/official_set_eu.inc). */
static const char* WorldName(int world) {
    static const char* const names[][DL_COUNT] = {
        { NULL, NULL, NULL, NULL, NULL, NULL },
        { "Agrabah", "Agrabah", "Agrabah", "Agrabah", "Agrabah", "アグラバー" },
        { "Atlantica", "Atlantica", "Atlantica", "Atlantica", "Atlántica", "アトランティカ" },
        { "Olympus Coliseum", "Colisée de l'Olympe", "Arena des Olymps", "Monte Olimpo", "Coliseo del Olimpo",
          "オリンポスコロシアム" },
        { "Wonderland", "Pays des Merveilles", "Wunderland", "Paese delle Meraviglie", "País de las Maravillas",
          "ワンダーランド" },
        { "Monstro", "Monstro", "Monstro", "Monstro", "Monstruo", "モンストロ" },
        { "Halloween Town", "Halloween Town", "Halloween Town", "Halloween Town", "Ciudad de Halloween",
          "ハロウィンタウン" },
        { "Never Land", "Pays Imaginaire", "Nimmerland", "L'Isola che non c'è", "País de Nunca Jamás", "ネバーランド" },
        { "Hollow Bastion", "Forteresse oubliée", "Hollow Bastion", "Fortezza Oscura", "Bastión Hueco",
          "ホロウバスティオン" },
        { "Destiny Islands", "Île du Destin", "Insel des Schicksals", "Isola del destino", "Islas del Destino",
          "デスティニーアイランド" },
        { "Traverse Town", "Ville de Traverse", "Traverse Town", "Traverse Town", "Ciudad de Paso", "トラヴァースタウン" },
        { "Twilight Town", "Cité du crépuscule", "Dimmerstadt", "Crepuscopoli", "Villa Crepúsculo", "トワイライトタウン" },
        { "Castle Oblivion", "Manoir Oblivion", "Schloss des Entfallens", "Castello dell'Oblio", "Castillo del Olvido",
          "忘却の城" },
        { "100 Acre Wood", "Forêt des Rêves Bleus", "Hundertmorgenwald", "Bosco dei Cento Acri",
          "Bosque de los Cien Acres", "100エーカーの森" },
    };
    return world > 0 && world < (int)(sizeof(names) / sizeof(names[0])) ? names[world][Lang()] : NULL;
}

/* The activity's words, in the game's language. */
enum { T_TITLE, T_FLOOR, T_BASEMENT, T_CHOOSING, T_LEVEL, T_BATTLE, T_SCENE, T_VERSUS, T_SORA, T_RIKU, T_COUNT };
static const char* Text(int id) {
    static const char* const text[T_COUNT][DL_COUNT] = {
        { "Title screen", "Écran titre", "Titelbildschirm", "Schermata del titolo", "Pantalla de título", "タイトル画面" },
        { "Floor %d", "Étage %d", "Ebene %d", "Piano %d", "Piso %d", "%d階" },
        { "Floor B%d", "Étage B%d", "Ebene B%d", "Piano B%d", "Piso B%d", "地下%d階" },
        { "choosing a world", "choix du monde", "Weltwahl", "scelta del mondo", "eligiendo mundo", "ワールド選択" },
        { "Lv", "Niv.", "Lv", "Liv.", "Nv.", "Lv" },
        { "In battle", "En combat", "Im Kampf", "In battaglia", "En combate", "バトル中" },
        { "In a scene", "En cinématique", "In einer Szene", "In una scena", "En una escena", "イベント中" },
        { "vs %s", "contre %s", "gegen %s", "contro %s", "contra %s", "VS %s" },
        { "Sora", "Sora", "Sora", "Sora", "Sora", "ソラ" },
        { "Riku", "Riku", "Riku", "Riku", "Riku", "リク" },
    };
    return text[id][Lang()];
}

/* Boss battles: the boss's name and its achievement badge (port/assets/discord/badge<n>.png, the atlas index of
   port/assets/achievements/badges.png), Sora's story first, then Reverse/Rebirth's; 0 for the world's badge. */
static const struct {
    u16 battle;
    u8 sora, riku;
    const char* name;
} kBosses[] = {
    { BATTLE_GUARD_ARMOR, 42, 31, "Guard Armor" },   { BATTLE_JAFAR, 44, 27, "Jafar" },
    { BATTLE_TRICKMASTER, 49, 36, "Trickmaster" },   { BATTLE_URSULA, 55, 33, "Ursula" },
    { BATTLE_PARASITE_CAGE, 46, 28, "Parasite Cage" }, { BATTLE_DRAGON_MALEFICENT, 54, 25, "Dragon Maleficent" },
    { BATTLE_DARKSIDE, 60, 38, "Darkside" },         { BATTLE_OOGIE_BOOGIE, 45, 34, "Oogie Boogie" },
    { BATTLE_MARLUXIA_2, 65, 65, "Marluxia" },       { BATTLE_LEON, 0, 0, "Leon" },
    { BATTLE_HOOK, 56, 30, "Captain Hook" },         { BATTLE_CLOUD, 0, 0, "Cloud" },
    { BATTLE_HADES, 48, 35, "Hades" },               { BATTLE_RIKU_1, 10, 39, "Riku" },
    { BATTLE_AXEL_1, 2, 2, "Axel" },                 { BATTLE_LARXENE_1, 8, 8, "Larxene" },
    { BATTLE_VEXEN_1, 22, 29, "Vexen" },             { BATTLE_MARLUXIA, 20, 20, "Marluxia" },
    { BATTLE_ANSEM_1, 26, 26, "Ansem" },             { BATTLE_LEXAEUS, 37, 37, "Lexaeus" },
    { BATTLE_RIKU_2, 12, 32, "Riku" },               { BATTLE_RIKU_3, 24, 32, "Riku" },
    { BATTLE_RIKU_4, 18, 32, "Riku" },               { BATTLE_RIKU_5, 18, 32, "Riku" },
    { BATTLE_RIKU_6, 18, 32, "Riku" },               { BATTLE_AXEL_2, 19, 19, "Axel" },
    { BATTLE_LARXENE_2, 17, 17, "Larxene" },         { BATTLE_VEXEN_2, 22, 29, "Vexen" },
    { BATTLE_VEXEN_3, 22, 29, "Vexen" },             { BATTLE_ANSEM_2, 26, 26, "Ansem" },
};

static int BossIndex(void) {
    size_t i;
    if (gCurrentMode != &gModeBattle || gBtlWork == NULL)
        return -1;
    for (i = 0; i < sizeof(kBosses) / sizeof(kBosses[0]); i++)
        if (kBosses[i].battle == gBtlWork->battleId)
            return (int)i;
    return -1;
}

/* The screens before a game is loaded (logos, language, title, file menus): no level yet, or one of those modes. */
static int OnTitle(void) {
    Mode* m = gCurrentMode;
    return m == NULL || gGameState.progression.level == 0 || m == &gModeTitle || m == &gModeWLogo ||
           m == &gModeMenuNew || m == &gModeMenuLoad || m == &gModeMenuMsg;
}

/* gGameState.world is the game's own (UpdateGameWorld): 0 in the castle's halls, the floor's world elsewhere. A new
   game's opening scenes come before the field sets it, so until the field has run it is the castle. */
static int sFieldSeen;

/* The activity's JSON for what is on screen now. */
static void Describe(char* out, size_t size) {
    char details[128], state[128], large[64], largeText[96], small[256] = "";
    int riku = (gGameState.flags & GAME_FLAG_RIKU) != 0, floor = gGameState.floor + 1;
    const char* world;
    Mode* m = gCurrentMode;
    int boss = BossIndex(), badge;
    int known = (sFieldSeen || m == &gModeBattle) && gGameState.world != 0; /* a battle always has its world */

    world = WorldName(known ? gGameState.world : WORLD_CASTLE_OBLIVION);
    if (OnTitle()) {
        SDL_strlcpy(details, Text(T_TITLE), sizeof(details));
        state[0] = 0;
        SDL_strlcpy(large, "logo", sizeof(large));
        SDL_strlcpy(largeText, "ReKHCoM", sizeof(largeText));
    } else {
        char where[48], extra[64] = "";
        SDL_snprintf(where, sizeof(where), Text(riku ? T_BASEMENT : T_FLOOR), floor);
        if (m == &gModeWorldselect || m == &gModeWORLDSELECT || world == NULL)
            SDL_snprintf(details, sizeof(details), "%s - %s", where, Text(T_CHOOSING));
        else
            SDL_snprintf(details, sizeof(details), "%s - %s", world, where);
        if (boss >= 0) {
            SDL_strlcpy(extra, " - ", sizeof(extra));
            SDL_snprintf(extra + 3, sizeof(extra) - 3, Text(T_VERSUS), kBosses[boss].name);
        } else if (m == &gModeBattle || m == &gModeEvent) {
            SDL_snprintf(extra, sizeof(extra), " - %s", Text(m == &gModeBattle ? T_BATTLE : T_SCENE));
        }
        SDL_snprintf(state, sizeof(state), "%s - %s %d%s", Text(riku ? T_RIKU : T_SORA), Text(T_LEVEL),
                     gGameState.progression.level, extra);
        /* the large picture: the boss's achievement badge (port/assets/discord/badge<n>.png), else the world's
           (world<id>.png), the logo in the castle and while choosing a world; the small one: the character */
        badge = boss < 0 ? 0 : riku ? kBosses[boss].riku : kBosses[boss].sora;
        if (badge) {
            SDL_snprintf(large, sizeof(large), "badge%d", badge);
            SDL_strlcpy(largeText, kBosses[boss].name, sizeof(largeText));
        } else if (known && m != &gModeWorldselect && m != &gModeWORLDSELECT && gGameState.world >= 1 &&
                   gGameState.world <= WORLD_100_ACRE_WOOD && gGameState.world != WORLD_CASTLE_OBLIVION) {
            SDL_snprintf(large, sizeof(large), "world%d", gGameState.world);
            SDL_strlcpy(largeText, world, sizeof(largeText));
        } else {
            SDL_strlcpy(large, "logo", sizeof(large));
            SDL_strlcpy(largeText, world != NULL ? world : "ReKHCoM", sizeof(largeText));
        }
        SDL_snprintf(small, sizeof(small), ",\"small_image\":\"" DISCORD_ART "%s.png\",\"small_text\":\"%s\"",
                     riku ? "riku" : "sora", Text(riku ? T_RIKU : T_SORA));
    }
    SDL_snprintf(out, size,
                 "\"details\":\"%s\",%s%s%s\"timestamps\":{\"start\":%lld},"
                 "\"assets\":{\"large_image\":\"" DISCORD_ART "%s.png\",\"large_text\":\"%s\"%s}",
                 details, state[0] ? "\"state\":\"" : "", state, state[0] ? "\"," : "", (long long)sStart, large,
                 largeText, small);
}

static long Pid(void) {
#ifdef _WIN32
    return (long)GetCurrentProcessId();
#else
    return (long)getpid();
#endif
}

void PortDiscordFrame(void) {
    uint64_t now = SDL_GetTicks();
    char json[1024];

    if (OnTitle())
        sFieldSeen = 0;
    else if (gCurrentMode == &gModeMapFld)
        sFieldSeen = 1;
    if (!gPortConfig.discordPresence || !*AppId() || HarnessRun()) {
        if (sConnected)
            Close();
        return;
    }
    if (sStart == 0)
        sStart = (int64_t)time(NULL);
    if (!sConnected) {
        if (now < sNextTry)
            return;
        sNextTry = now + 10000;
        if (!Open())
            return;
        SDL_snprintf(json, sizeof(json), "{\"v\":1,\"client_id\":\"%s\"}", AppId());
        if (!Frame(0, json))
            return;
        sConnected = 1;
        sNextSend = now + 1000; /* after Discord's READY */
        PortLog("discord: connected");
        return;
    }
    Drain();
    if (!sConnected || now < sNextSend)
        return;
    Describe(sWanted, sizeof(sWanted));
    if (strcmp(sWanted, sSent) == 0) {
        sNextSend = now + 1000;
        return;
    }
    SDL_snprintf(json, sizeof(json), "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":%ld,\"activity\":{%s}},\"nonce\":\"%u\"}",
                 Pid(), sWanted, ++sNonce);
    if (Frame(1, json)) {
        SDL_strlcpy(sSent, sWanted, sizeof(sSent));
        sNextSend = now + 5000; /* Discord takes five updates every 20 seconds */
    }
}

void PortDiscordShutdown(void) {
    if (sConnected)
        Close();
}

int PortDiscordAvailable(void) {
    return *AppId() != 0;
}
#else
void PortDiscordFrame(void) {
}
void PortDiscordShutdown(void) {
}
int PortDiscordAvailable(void) {
    return 0;
}
#endif
