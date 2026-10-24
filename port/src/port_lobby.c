/*
 * The online lobby (port/progress/ONLINE_ESTUDIO.md): before the link battle's menu, a screen of the port (port_ui.c,
 * PortUiOpenLobby) lists the public rooms, opens one or enters one, or goes back to the cable.
 *
 * One provider of rooms, two backends behind it, chosen by netlink.cfg (the launcher writes it):
 *   lobby:steam                  Steam's lobbies through Valve's test app (Spacewar, 480): free, the player signs in
 *                                to Steam (port_steamlink.c). PC only for now.
 *   lobby:<server>:<port>        a relay server (port/tools/netlink_relay.py, a VPS): any platform with UDP,
 *                                crossplay; no account yet (the name shown is the player's, from kh_com.ini).
 * KHCOM_LOBBY=<the same> overrides it (tests). Whatever the backend, picking a room ends in an ordinary link spec
 * (steam:host|join, relay:<server>:<port>:<code>:host|join) handed to port_netlink.c, so the link itself does not
 * know there was a lobby.
 */
#include "port/port.h"
#include "port/port_netlink.h"

#include <SDL3/SDL.h>
#include <string.h>

#define LOBBY_VERSION 3 /* NET_VERSION in src/sio/sio.c: only rooms that play by the same rules are listed */
#define LOBBY_ROOMS 20

static int sBackend; /* 0 none, 1 Steam, 2 a relay server */
static char sServer[128];
static int sServerPort;
static int sCount = -1; /* -1 asking */
static char sCodes[LOBBY_ROOMS][17], sNames[LOBBY_ROOMS][25];
static uint64_t sIds[LOBBY_ROOMS];
static int sRegions[LOBBY_ROOMS];
static uint32_t sAskedAt;
static char sMyCode[17];

static const char* Setting(void) {
    const char* env = SDL_getenv("KHCOM_LOBBY");
    const char* cfg = env ? env : PortNetlinkConfig();
    return cfg && SDL_strncmp(cfg, "lobby", 5) == 0 ? cfg : NULL;
}

/* 1: the link battle opens the lobby first */
int PortLobbyWanted(void) {
    const char* s = Setting();
    if (!s)
        return 0;
    if (SDL_strcmp(s, "lobby:steam") == 0 || SDL_strcmp(s, "lobby") == 0) {
        sBackend = 1;
        return 1;
    }
    if (SDL_strncmp(s, "lobby:", 6) == 0) { /* lobby:<server>:<port> */
        const char* colon = SDL_strrchr(s + 6, ':');
        if (!colon || colon - (s + 6) >= (int)sizeof(sServer))
            return 0;
        SDL_memcpy(sServer, s + 6, (size_t)(colon - (s + 6)));
        sServer[colon - (s + 6)] = 0;
        sServerPort = SDL_atoi(colon + 1);
        sBackend = sServerPort > 0 ? 2 : 0;
        return sBackend != 0;
    }
    return 0;
}

int PortLobbyBackend(void) {
    return sBackend;
}

/* the cartridge's region (EU 0, US 1, JP 2): a room shows it, and only the same region can link */
static int Region(void) {
#if defined(VERSION_US)
    return 1;
#elif defined(VERSION_JP)
    return 2;
#else
    return 0;
#endif
}

int PortLobbyRegion(void) {
    return Region();
}

/* the player's name on the room: Steam's, or kh_com.ini's online_name */
const char* PortLobbyPlayerName(void) {
    static char name[25];
    if (sBackend == 1 && PortSteamlinkUserName()[0])
        return PortSteamlinkUserName();
    SDL_strlcpy(name, gPortConfig.onlineName[0] ? gPortConfig.onlineName : "ReKHCoM", sizeof(name));
    return name;
}

/* 1 ready, -1 the player must sign in (Steam), -2 the server does not answer, 0 still asking */
int PortLobbyReady(void) {
    if (sBackend == 1)
        return PortSteamlinkLobbyReady();
    if (sBackend == 2)
        return sCount >= 0 ? 1 : (sAskedAt && SDL_GetTicks() - sAskedAt > 3000 ? -2 : 0);
    return -2;
}

void PortLobbyRefresh(void) {
    sCount = -1;
    sAskedAt = (uint32_t)SDL_GetTicks() | 1;
    if (sBackend == 1)
        PortSteamlinkLobbyList();
    else if (sBackend == 2)
        PortNetlinkLobbyAsk(sServer, sServerPort, LOBBY_VERSION);
}

/* once a frame while the lobby is open: the answer read, the list asked again every 4 s */
void PortLobbyFrame(void) {
    int n = -1;
    if (sBackend == 1)
        n = PortSteamlinkLobbyRooms(sIds, sNames, sRegions, LOBBY_ROOMS);
    else if (sBackend == 2)
        n = PortNetlinkLobbyRead(sCodes, sNames, sRegions, LOBBY_ROOMS);
    if (n >= 0)
        sCount = n;
    if (sAskedAt && SDL_GetTicks() - sAskedAt > 4000 && PortLobbyReady() != -1)
        PortLobbyRefresh();
}

int PortLobbyRoomCount(void) {
    return sCount;
}

/* a room's line: its name, and its region when it is not this cartridge's (it cannot be entered) */
const char* PortLobbyRoomName(int i) {
    return i >= 0 && i < sCount ? sNames[i] : "";
}

int PortLobbyRoomRegion(int i) {
    return i >= 0 && i < sCount ? sRegions[i] : -1;
}

/* the code of a room opened on a relay server (shown so a friend can be told) */
const char* PortLobbyMyCode(void) {
    return sMyCode;
}

void PortLobbyCreate(void) {
    char spec[200];
    if (sBackend == 1) {
        PortSteamlinkTarget(0, Region());
        PortNetlinkUse("steam:host");
        return;
    }
    {
        static const char digits[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; /* no 0/O, 1/I */
        uint32_t seed = (uint32_t)SDL_GetPerformanceCounter() ^ (uint32_t)SDL_GetTicks() * 2654435761u;
        int i;
        for (i = 0; i < 6; i++) {
            seed = seed * 1664525u + 1013904223u;
            sMyCode[i] = digits[(seed >> 16) % 32];
        }
        sMyCode[6] = 0;
    }
    SDL_snprintf(spec, sizeof(spec), "relay:%s:%d:%s:host", sServer, sServerPort, sMyCode);
    PortNetlinkUse(spec);
    PortNetlinkAnnounce(LOBBY_VERSION, Region(), PortLobbyPlayerName());
}

void PortLobbyJoin(int i) {
    char spec[200];
    if (i < 0 || i >= sCount)
        return;
    if (sBackend == 1) {
        PortSteamlinkTarget(sIds[i], Region());
        PortNetlinkUse("steam:join");
        return;
    }
    SDL_snprintf(spec, sizeof(spec), "relay:%s:%d:%s:join", sServer, sServerPort, sCodes[i]);
    PortNetlinkUse(spec);
}

void PortLobbyCable(void) {
    PortNetlinkUse(NULL);
}
