/* The network cable through Steam (alpha, port/progress/ONLINE_ESTUDIO.md): a lobby to find the other player and
 * Steam's P2P messages, carried by Valve's relays (no ports to open, no address to type). port_netlink.c moves the
 * same datagrams through here instead of its UDP socket.
 *
 * steam_api.dll (Valve's redistributable, next to the game) is loaded when the link starts; without it, or without a
 * running Steam signed in, the link reports why and stays off. The app is Valve's test app, Spacewar (480), as source
 * ports do while they have no AppID of their own: everyone with Steam can use it.
 *
 * Host: a public lobby with rekhcom=<protocol>; the first member to enter is the other player.
 * Join: the ReKHCoM lobbies, the one of a Steam friend first (or the first one); its owner is the other player.
 * A friend's "Join game" from Steam's friends list enters the lobby too.
 */
#include "port/port.h"
#include "port/port_netlink.h"
#include "port/port_ui.h"

#if defined(_WIN32)
#include <SDL3/SDL.h>
#include <windows.h>
#include <string.h>

#define S_CALL __cdecl
typedef uint64_t SteamId;
typedef uint64_t ApiCall;
typedef int HPipe;
typedef unsigned char SBool;

/* steam_api.h's structures as Windows lays them out (VALVE_CALLBACK_PACK_LARGE: 8) */
#pragma pack(push, 8)
typedef struct {
    int user;
    int id;
    unsigned char* param;
    int size;
} CallbackMsg;
typedef struct {
    ApiCall call;
    int id;
    uint32_t size;
} CallCompleted; /* 703 */
typedef struct {
    int result;
    SteamId lobby;
} LobbyCreated; /* 513 */
typedef struct {
    SteamId lobby;
    uint32_t permissions;
    SBool locked;
    uint32_t response;
} LobbyEnter; /* 504 */
typedef struct {
    SteamId lobby, changed, maker;
    uint32_t state;
} LobbyChatUpdate; /* 506 */
typedef struct {
    uint32_t count;
} LobbyMatchList; /* 510 */
typedef struct {
    SteamId lobby, friendId;
} JoinRequested; /* 333 */
typedef struct {
    void* data;
    int size;
} NetMessage; /* the head of SteamNetworkingMessage_t */
#pragma pack(pop)

#define CB_JOIN_REQUESTED 333
#define CB_LOBBY_ENTER 504
#define CB_LOBBY_CHAT_UPDATE 506
#define CB_LOBBY_MATCH_LIST 510
#define CB_LOBBY_CREATED 513
#define CB_CALL_COMPLETED 703
#define CB_SESSION_REQUEST 1251
#define PROTOCOL "link-3" /* the slots' rules (NET_VERSION in src/sio/sio.c): other versions never meet */

static struct {
    HMODULE dll;
    SBool(S_CALL* Init)(void);
    HPipe(S_CALL* GetPipe)(void);
    void(S_CALL* DispatchInit)(void);
    void(S_CALL* RunFrame)(HPipe);
    SBool(S_CALL* NextCallback)(HPipe, CallbackMsg*);
    void(S_CALL* FreeCallback)(HPipe);
    SBool(S_CALL* CallResult)(HPipe, ApiCall, void*, int, int, SBool*);
    void*(S_CALL* Matchmaking)(void);
    void*(S_CALL* User)(void);
    void*(S_CALL* Friends)(void);
    void*(S_CALL* Messages)(void);
    void*(S_CALL* Utils)(void);
    ApiCall(S_CALL* CreateLobby)(void*, int, int);
    ApiCall(S_CALL* JoinLobby)(void*, SteamId);
    SteamId(S_CALL* LobbyOwner)(void*, SteamId);
    SBool(S_CALL* SetLobbyData)(void*, SteamId, const char*, const char*);
    ApiCall(S_CALL* RequestLobbyList)(void*);
    void(S_CALL* ListStringFilter)(void*, const char*, const char*, int);
    void(S_CALL* ListDistanceFilter)(void*, int);
    SteamId(S_CALL* LobbyByIndex)(void*, int);
    SteamId(S_CALL* MySteamId)(void*);
    SBool(S_CALL* HasFriend)(void*, SteamId, int);
    const char*(S_CALL* FriendName)(void*, SteamId);
    int(S_CALL* SendMessage)(void*, const void*, const void*, uint32_t, int, int);
    int(S_CALL* ReceiveMessages)(void*, int, NetMessage**, int);
    SBool(S_CALL* AcceptSession)(void*, const void*);
    void(S_CALL* SetIdentity)(void*, SteamId);
    void(S_CALL* ReleaseMessage)(NetMessage*);
    void(S_CALL* InitRelay)(void*);
    const char*(S_CALL* LobbyData)(void*, SteamId, const char*);
    void(S_CALL* ListSlotsFilter)(void*, int);
    SBool(S_CALL* LoggedOn)(void*);
    const char*(S_CALL* PersonaName)(void*);
} S;

static int sState; /* 0 not started, 1 ready, -1 failed */
static int sRole;
static HPipe sPipe;
static SteamId sMe, sLobby, sPeer;
static unsigned char sPeerIdentity[136]; /* SteamNetworkingIdentity */
static ApiCall sPending;
static uint64_t sNextSearch;
static char sStatus[160];
/* the lobby (port_lobby.c): the public rooms found, the one to enter, what the room we open says */
#define ROOMS_MAX 20
static int sListing, sListDone;
static int sRoomCount;
static SteamId sRoomIds[ROOMS_MAX], sTarget;
static char sRoomNames[ROOMS_MAX][25];
static int sRoomRegions[ROOMS_MAX];
static int sRegion;

/* the link's state, in the log and on screen (English, Spanish) */
static void Status2(const char* en, const char* es, const char* name) {
    char text[160];
    SDL_snprintf(text, sizeof(text), en, name ? name : "");
    if (SDL_strcmp(text, sStatus) != 0) {
        SDL_strlcpy(sStatus, text, sizeof(sStatus));
        PortLog("steamlink: %s", sStatus);
        PortUiLinkNotice(en, es, name);
    }
}

const char* PortSteamlinkStatus(void) {
    return sStatus;
}

static int Load(void) {
    char path[1024];
    SDL_snprintf(path, sizeof(path), "%ssteam_api.dll", SDL_GetBasePath() ? SDL_GetBasePath() : "");
    S.dll = LoadLibraryA(path);
    if (!S.dll)
        S.dll = LoadLibraryA("steam_api.dll");
    if (!S.dll) {
        Status2("steam_api.dll not found next to the game%s", "Falta steam_api.dll junto al juego%s", NULL);
        return 0;
    }
#define F(field, name)                                                                                                  \
    if ((*(FARPROC*)&S.field = GetProcAddress(S.dll, name)) == NULL) {                                                  \
        Status2("steam_api.dll lacks %s", "A steam_api.dll le falta %s", name);                                                                         \
        return 0;                                                                                                       \
    }
    F(Init, "SteamAPI_InitSafe");
    F(GetPipe, "SteamAPI_GetHSteamPipe");
    F(DispatchInit, "SteamAPI_ManualDispatch_Init");
    F(RunFrame, "SteamAPI_ManualDispatch_RunFrame");
    F(NextCallback, "SteamAPI_ManualDispatch_GetNextCallback");
    F(FreeCallback, "SteamAPI_ManualDispatch_FreeLastCallback");
    F(CallResult, "SteamAPI_ManualDispatch_GetAPICallResult");
    F(Matchmaking, "SteamAPI_SteamMatchmaking_v009");
    F(User, "SteamAPI_SteamUser_v023");
    F(Friends, "SteamAPI_SteamFriends_v018");
    F(Messages, "SteamAPI_SteamNetworkingMessages_SteamAPI_v002");
    F(Utils, "SteamAPI_SteamNetworkingUtils_SteamAPI_v004");
    F(CreateLobby, "SteamAPI_ISteamMatchmaking_CreateLobby");
    F(JoinLobby, "SteamAPI_ISteamMatchmaking_JoinLobby");
    F(LobbyOwner, "SteamAPI_ISteamMatchmaking_GetLobbyOwner");
    F(SetLobbyData, "SteamAPI_ISteamMatchmaking_SetLobbyData");
    F(RequestLobbyList, "SteamAPI_ISteamMatchmaking_RequestLobbyList");
    F(ListStringFilter, "SteamAPI_ISteamMatchmaking_AddRequestLobbyListStringFilter");
    F(ListDistanceFilter, "SteamAPI_ISteamMatchmaking_AddRequestLobbyListDistanceFilter");
    F(LobbyByIndex, "SteamAPI_ISteamMatchmaking_GetLobbyByIndex");
    F(MySteamId, "SteamAPI_ISteamUser_GetSteamID");
    F(HasFriend, "SteamAPI_ISteamFriends_HasFriend");
    F(FriendName, "SteamAPI_ISteamFriends_GetFriendPersonaName");
    F(SendMessage, "SteamAPI_ISteamNetworkingMessages_SendMessageToUser");
    F(ReceiveMessages, "SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel");
    F(AcceptSession, "SteamAPI_ISteamNetworkingMessages_AcceptSessionWithUser");
    F(SetIdentity, "SteamAPI_SteamNetworkingIdentity_SetSteamID64");
    F(ReleaseMessage, "SteamAPI_SteamNetworkingMessage_t_Release");
    F(InitRelay, "SteamAPI_ISteamNetworkingUtils_InitRelayNetworkAccess");
    F(LobbyData, "SteamAPI_ISteamMatchmaking_GetLobbyData");
    F(ListSlotsFilter, "SteamAPI_ISteamMatchmaking_AddRequestLobbyListFilterSlotsAvailable");
    F(LoggedOn, "SteamAPI_ISteamUser_BLoggedOn");
    F(PersonaName, "SteamAPI_ISteamFriends_GetPersonaName");
#undef F
    return 1;
}

static void Search(void) {
    void* mm = S.Matchmaking();
    S.ListStringFilter(mm, "rekhcom", PROTOCOL, 0 /* equal */);
    S.ListSlotsFilter(mm, 1); /* rooms still waiting for their second player */
    S.ListDistanceFilter(mm, 3 /* worldwide */);
    sPending = S.RequestLobbyList(mm);
    sNextSearch = SDL_GetTicks() + 3000;
    if (!sListing) /* the lobby's own list says what it does */
        Status2("Steam: looking for your friend's room%s", "Steam: buscando la sala de tu amigo%s", NULL);
}

static void SetPeer(SteamId peer) {
    if (peer == 0 || peer == sMe || peer == sPeer)
        return;
    sPeer = peer;
    SDL_memset(sPeerIdentity, 0, sizeof(sPeerIdentity));
    S.SetIdentity(sPeerIdentity, peer);
    Status2("Steam: linked with %s", "Steam: enlazado con %s", S.FriendName(S.Friends(), peer));
}

static void OnResult(int id, const unsigned char* p) {
    if (id == CB_LOBBY_CREATED) {
        const LobbyCreated* c = (const LobbyCreated*)p;
        if (c->result != 1 /* k_EResultOK */) {
            Status2("Steam: the room could not be created%s", "Steam: no se pudo crear la sala%s", NULL);
            return;
        }
        sLobby = c->lobby;
        S.SetLobbyData(S.Matchmaking(), sLobby, "rekhcom", PROTOCOL);
        S.SetLobbyData(S.Matchmaking(), sLobby, "name", S.PersonaName(S.Friends()));
        {
            char region[4];
            SDL_snprintf(region, sizeof(region), "%d", sRegion);
            S.SetLobbyData(S.Matchmaking(), sLobby, "region", region);
        }
        Status2("Steam: room open, waiting for your friend%s", "Steam: sala abierta, esperando a tu amigo%s", NULL);
    } else if (id == CB_LOBBY_MATCH_LIST) {
        const LobbyMatchList* m = (const LobbyMatchList*)p;
        SteamId pick = 0;
        uint32_t i;
        if (sListing) { /* the lobby's list: kept for the screen, nothing entered */
            sRoomCount = 0;
            for (i = 0; i < m->count && sRoomCount < ROOMS_MAX; i++) {
                SteamId lobby = S.LobbyByIndex(S.Matchmaking(), (int)i);
                const char* name = S.LobbyData(S.Matchmaking(), lobby, "name");
                const char* region = S.LobbyData(S.Matchmaking(), lobby, "region");
                if (S.LobbyOwner(S.Matchmaking(), lobby) == sMe)
                    continue;
                sRoomIds[sRoomCount] = lobby;
                SDL_strlcpy(sRoomNames[sRoomCount], name && *name ? name : "?", sizeof(sRoomNames[0]));
                sRoomRegions[sRoomCount] = region ? SDL_atoi(region) : 0;
                sRoomCount++;
            }
            sListing = 0;
            sListDone = 1;
            return;
        }
        for (i = 0; i < m->count; i++) {
            SteamId lobby = S.LobbyByIndex(S.Matchmaking(), (int)i);
            SteamId owner = S.LobbyOwner(S.Matchmaking(), lobby);
            if (owner != sMe && S.HasFriend(S.Friends(), owner, 4 /* immediate */)) {
                pick = lobby;
                break;
            }
            if (!pick && owner != sMe)
                pick = lobby;
        }
        if (pick) {
            sPending = S.JoinLobby(S.Matchmaking(), pick);
            Status2("Steam: entering the room%s", "Steam: entrando a la sala%s", NULL);
        }
    } else if (id == CB_LOBBY_ENTER) {
        const LobbyEnter* e = (const LobbyEnter*)p;
        if (e->response != 1 /* k_EChatRoomEnterResponseSuccess */)
            return;
        sLobby = e->lobby;
        if (sRole == PORT_NETLINK_JOIN)
            SetPeer(S.LobbyOwner(S.Matchmaking(), sLobby));
    }
}

static void Pump(void) {
    CallbackMsg msg;
    S.RunFrame(sPipe);
    while (S.NextCallback(sPipe, &msg)) {
        if (msg.id == CB_CALL_COMPLETED) {
            const CallCompleted* c = (const CallCompleted*)msg.param;
            static unsigned char buf[512];
            SBool failed = 0;
            if (c->size <= sizeof(buf) && S.CallResult(sPipe, c->call, buf, (int)c->size, c->id, &failed) && !failed)
                OnResult(c->id, buf);
            if (c->call == sPending)
                sPending = 0;
        } else if (msg.id == CB_LOBBY_ENTER) {
            OnResult(CB_LOBBY_ENTER, msg.param);
        } else if (msg.id == CB_LOBBY_CHAT_UPDATE) {
            const LobbyChatUpdate* u = (const LobbyChatUpdate*)msg.param;
            if (sRole == PORT_NETLINK_HOST && u->lobby == sLobby && (u->state & 1 /* entered */))
                SetPeer(u->changed);
        } else if (msg.id == CB_JOIN_REQUESTED) {
            const JoinRequested* j = (const JoinRequested*)msg.param;
            if (sRole == PORT_NETLINK_JOIN && !sPeer)
                S.JoinLobby(S.Matchmaking(), j->lobby);
        } else if (msg.id == CB_SESSION_REQUEST) {
            S.AcceptSession(S.Messages(), msg.param); /* the identity is the callback's first field */
        }
        S.FreeCallback(sPipe);
    }
    if (sRole == PORT_NETLINK_JOIN && !sPeer && !sPending && !sTarget && SDL_GetTicks() >= sNextSearch)
        Search();
}

/* Steam once: 1 ready and signed in, 0 not (Steam closed, no session, no steam_api.dll) */
static int Init(void) {
    static int loaded;
    static uint64_t retryAt;
    if (sState > 0)
        return 1;
    if (sState < 0 && SDL_GetTicks() < retryAt)
        return 0; /* the lobby's "try again": Steam may have been opened or signed in since */
    sState = -1;
    retryAt = SDL_GetTicks() + 2000;
    SDL_setenv_unsafe("SteamAppId", "480", 1); /* Spacewar, Valve's test app */
    SDL_setenv_unsafe("SteamGameId", "480", 1);
    if (!loaded && !(loaded = Load()))
        return 0;
    if (!S.Init()) {
        Status2("Open Steam and sign in to link%s", "Abre Steam e inicia sesion para enlazar%s", NULL);
        return 0;
    }
    S.DispatchInit();
    sPipe = S.GetPipe();
    sMe = S.MySteamId(S.User());
    S.InitRelay(S.Utils());
    sState = 1;
    return 1;
}

/* the lobby (port_lobby.c): 1 signed in to Steam, -1 not (it tells the player to open Steam and sign in) */
int PortSteamlinkLobbyReady(void) {
    if (!Init())
        return -1;
    return S.LoggedOn(S.User()) ? 1 : -1;
}

const char* PortSteamlinkUserName(void) {
    return sState > 0 ? S.PersonaName(S.Friends()) : "";
}

/* the public rooms: asked, then read once the answer came (-1 until then) */
void PortSteamlinkLobbyList(void) {
    if (!Init() || sPending)
        return;
    sListing = 1;
    sListDone = 0;
    Search();
}

int PortSteamlinkLobbyRooms(uint64_t* ids, char (*names)[25], int* regions, int max) {
    int i;
    if (sState > 0)
        Pump();
    if (!sListDone)
        return -1;
    for (i = 0; i < sRoomCount && i < max; i++) {
        ids[i] = sRoomIds[i];
        SDL_strlcpy(names[i], sRoomNames[i], sizeof(names[0]));
        regions[i] = sRoomRegions[i];
    }
    return i;
}

/* the room picked in the lobby: this one to enter (0: the first one found, as before); the cartridge region shown */
void PortSteamlinkTarget(uint64_t lobby, int region) {
    sTarget = lobby;
    sRegion = region;
}

int PortSteamlinkStart(int role) {
    if (!Init())
        return 0;
    sRole = role;
    sPeer = 0;
    sListing = 0;
    if (role == PORT_NETLINK_HOST) {
        sPending = S.CreateLobby(S.Matchmaking(), 2 /* public */, 2);
        Status2("Steam: opening a room%s", "Steam: abriendo una sala%s", NULL);
    } else if (sTarget) {
        sPending = S.JoinLobby(S.Matchmaking(), sTarget);
        Status2("Steam: entering the room%s", "Steam: entrando a la sala%s", NULL);
    } else {
        Search();
    }
    return 1;
}

void PortSteamlinkFrame(void) {
    if (sState > 0)
        Pump();
}

int PortSteamlinkHasPeer(void) {
    return sState > 0 && sPeer != 0;
}

void PortSteamlinkSend(const uint8_t* data, int size) {
    if (sState > 0 && sPeer)
        S.SendMessage(S.Messages(), sPeerIdentity, data, (uint32_t)size, 1 | 32 /* unreliable, no Nagle, restart a broken session */, 0);
}

int PortSteamlinkRecv(uint8_t* data, int cap) {
    NetMessage* m = NULL;
    int n = 0;
    if (sState <= 0)
        return 0;
    if (S.ReceiveMessages(S.Messages(), 0, &m, 1) == 1 && m) {
        n = m->size < cap ? m->size : cap;
        SDL_memcpy(data, m->data, (size_t)n);
        S.ReleaseMessage(m);
    }
    return n;
}
#else
int PortSteamlinkLobbyReady(void) {
    return 0;
}
const char* PortSteamlinkUserName(void) {
    return "";
}
void PortSteamlinkLobbyList(void) {
}
int PortSteamlinkLobbyRooms(uint64_t* ids, char (*names)[25], int* regions, int max) {
    (void)ids;
    (void)names;
    (void)regions;
    (void)max;
    return -1;
}
void PortSteamlinkTarget(uint64_t lobby, int region) {
    (void)lobby;
    (void)region;
}
int PortSteamlinkStart(int role) {
    (void)role;
    return 0;
}
void PortSteamlinkFrame(void) {
}
int PortSteamlinkHasPeer(void) {
    return 0;
}
void PortSteamlinkSend(const uint8_t* data, int size) {
    (void)data;
    (void)size;
}
int PortSteamlinkRecv(uint8_t* data, int cap) {
    (void)data;
    (void)cap;
    return 0;
}
const char* PortSteamlinkStatus(void) {
    return "";
}
#endif
