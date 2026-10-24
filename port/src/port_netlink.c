/* The link cable over the network (alpha, port/progress/ONLINE_ESTUDIO.md): a UDP socket to the other player.
 * src/sio/sio.c keeps the game's own packet queue and asks here for the datagrams; this file only moves bytes.
 *
 * KHCOM_NETLINK=steam:host         open a Steam room (port_steamlink.c): the cable's parent, player 1
 * KHCOM_NETLINK=steam:join         enter a friend's Steam room: the child, player 2
 * KHCOM_NETLINK=host:<port>        wait for the other player on that UDP port (player 1)
 * KHCOM_NETLINK=join:<ip>:<port>   connect to a host by address or name (player 2)
 * KHCOM_NETLINK=relay:<server>:<port>:<code>:host|join   meet through a relay (port/tools/netlink_relay.py on a
 *                                  VPS) by a room code: no ports to open, any platform with UDP (crossplay)
 * Without the variable, netlink.cfg in the data folder (the launcher writes it) holds the same text.
 * KHCOM_NETLINK_LAG=<frames>       tests: hold every datagram that long (in 16.7 ms frames of time, so a game that
 *                                  waits for the other one keeps its datagrams going) before it goes out
 * KHCOM_NETLINK_JITTER=<frames>    tests: and up to that much more, at random (an uneven connection)
 * KHCOM_NETLINK_LOSS=<percent>     tests: drop that share of the datagrams sent
 * KHCOM_NETLINK_DROP=<from>-<to>   tests: an outage, nothing sent between those game frames
 *
 * The socket itself is four calls (UdpOpen/UdpResolve/UdpSend/UdpRecv): BSD sockets where the game is hosted (PC,
 * Android 32-bit), the host's through the bridge where it is a guest image (Android 64-bit, consoles: udp_* in
 * port/ps5/shared/bridge_list.h; a host without a network answers 0 and the link stays the cable).
 */
#include "port/port.h"
#include "port/port_netlink.h"

#if !defined(__EMSCRIPTEN__)
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* an IPv4 address and port, in network order */
typedef struct {
    uint32_t ip;
    uint16_t port;
} NetAddr;

#if __STDC_HOSTED__
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Sock;
#define BAD_SOCK INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int Sock;
#define BAD_SOCK (-1)
#define closesocket close
#endif
static Sock sSock = BAD_SOCK;

/* a non-blocking UDP socket on that port (0: any) */
static int UdpOpen(int bindPort) {
    struct sockaddr_in me;
#ifdef _WIN32
    WSADATA wsa;
    u_long nb = 1;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 0;
#endif
    if (sSock != BAD_SOCK)
        closesocket(sSock); /* the lobby opens it again for the room it picked */
    sSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sSock == BAD_SOCK)
        return 0;
#ifdef _WIN32
    ioctlsocket(sSock, FIONBIO, &nb);
#else
    fcntl(sSock, F_SETFL, fcntl(sSock, F_GETFL, 0) | O_NONBLOCK);
#endif
    SDL_memset(&me, 0, sizeof(me));
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    me.sin_port = htons((unsigned short)bindPort);
    if (bind(sSock, (struct sockaddr*)&me, sizeof(me)) != 0) {
        closesocket(sSock);
        sSock = BAD_SOCK;
        return 0;
    }
    return 1;
}

/* host (an address or a name) and port */
static int UdpResolve(const char* host, int port, NetAddr* to) {
    struct addrinfo hints, *res = NULL;
    SDL_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0 || !res)
        return 0;
    to->ip = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
    to->port = htons((unsigned short)port);
    freeaddrinfo(res);
    return 1;
}

static void UdpSend(const NetAddr* to, const void* data, int size) {
    struct sockaddr_in a;
    SDL_memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = to->ip;
    a.sin_port = to->port;
    sendto(sSock, (const char*)data, size, 0, (struct sockaddr*)&a, sizeof(a));
}

static int UdpRecv(void* data, int cap, NetAddr* from) {
    struct sockaddr_in a;
#ifdef _WIN32
    int len = sizeof(a);
#else
    socklen_t len = sizeof(a);
#endif
    int n = (int)recvfrom(sSock, (char*)data, cap, 0, (struct sockaddr*)&a, &len);
    if (n <= 0)
        return 0;
    from->ip = a.sin_addr.s_addr;
    from->port = a.sin_port;
    return n;
}

static int UdpReady(void) {
    return sSock != BAD_SOCK;
}
#else
#include "bridge_calls.h"
static int sSockOpen;

static int UdpOpen(int bindPort) {
    sSockOpen = h_udp_open(bindPort);
    return sSockOpen;
}

static int UdpResolve(const char* host, int port, NetAddr* to) {
    uint8_t a[6];
    if (!h_udp_resolve(host, port, a))
        return 0;
    SDL_memcpy(&to->ip, a, 4);
    SDL_memcpy(&to->port, a + 4, 2);
    return 1;
}

static void UdpSend(const NetAddr* to, const void* data, int size) {
    uint8_t a[6];
    SDL_memcpy(a, &to->ip, 4);
    SDL_memcpy(a + 4, &to->port, 2);
    h_udp_send(a, data, size);
}

static int UdpRecv(void* data, int cap, NetAddr* from) {
    uint8_t a[6];
    int n = h_udp_recv(data, cap, a);
    if (n <= 0)
        return 0;
    SDL_memcpy(&from->ip, a, 4);
    SDL_memcpy(&from->port, a + 4, 2);
    return n;
}

static int UdpReady(void) {
    return sSockOpen;
}
#endif

static int SameAddr(const NetAddr* a, const NetAddr* b) {
    return a->ip == b->ip && a->port == b->port;
}

/* "a.b.c.d:port" for the log */
static const char* AddrText(const NetAddr* a) {
    static char text[24];
    const uint8_t* ip = (const uint8_t*)&a->ip;
    const uint8_t* port = (const uint8_t*)&a->port;
    SDL_snprintf(text, sizeof(text), "%u.%u.%u.%u:%u", ip[0], ip[1], ip[2], ip[3], (unsigned)(port[0] << 8 | port[1]));
    return text;
}

/* the tests' random loss and jitter (SDL_rand is not in the consoles' shim) */
static uint32_t TestRand(uint32_t n) {
    static uint32_t seed = 0x2545F491;
    seed = seed * 1664525u + 1013904223u;
    return n ? (seed >> 8) % n : 0;
}

static int sChecked, sRole, sSteam;
static NetAddr sPeer;
static int sHavePeer;
static uint64_t sLastHeard;
static int sLag, sLoss, sJitter;
static uint32_t sDropFrom, sDropTo;
static int sRelay, sRelayPlayers;
static char sRoom[17];
/* the lobby (port_lobby.c): the link it picked, replacing netlink.cfg; a public room's announce */
static char sUse[160];
static int sUseSet;
static int sAnnounce, sAnnounceVersion, sAnnounceRegion;
static char sAnnounceName[25];

/* datagrams held back by KHCOM_NETLINK_LAG */
typedef struct {
    uint32_t due;
    int size;
    uint8_t data[PORT_NETLINK_MAX];
} Held;
static Held sHeld[256];
static int sHeldCount;
static uint32_t sFrame;

static void RelayAnnounce(void) {
    uint8_t d[19 + 24];
    size_t n = SDL_strlen(sAnnounceName), c = SDL_strlen(sRoom);
    if (n > 24)
        n = 24;
    SDL_memset(d, 0, sizeof(d));
    d[0] = 'A';
    d[1] = (uint8_t)sAnnounceVersion;
    d[2] = (uint8_t)sAnnounceRegion;
    SDL_memcpy(d + 3, sRoom, c > 16 ? 16 : c);
    SDL_memcpy(d + 19, sAnnounceName, n);
    UdpSend(&sPeer, d, (int)(19 + n));
}

static void RelayHello(void) {
    uint8_t d[2 + sizeof(sRoom)];
    size_t n = SDL_strlen(sRoom);
    d[0] = 'J';
    d[1] = (uint8_t)sRole;
    SDL_memcpy(d + 2, sRoom, n);
    UdpSend(&sPeer, d, (int)(2 + n));
}

static int Open(const char* spec) {
    char host[128] = "";
    int port = 0;
    if (SDL_strncmp(spec, "host:", 5) == 0) {
        port = SDL_atoi(spec + 5);
        sRole = PORT_NETLINK_HOST;
    } else if (SDL_strncmp(spec, "join:", 5) == 0) {
        const char* colon = SDL_strrchr(spec + 5, ':');
        if (!colon || colon - (spec + 5) >= (int)sizeof(host))
            return 0;
        SDL_memcpy(host, spec + 5, (size_t)(colon - (spec + 5)));
        host[colon - (spec + 5)] = 0;
        port = SDL_atoi(colon + 1);
        sRole = PORT_NETLINK_JOIN;
    } else if (SDL_strncmp(spec, "relay:", 6) == 0) { /* relay:<server>:<port>:<code>:host|join */
        char rest[160], *f[4];
        int i;
        SDL_strlcpy(rest, spec + 6, sizeof(rest));
        f[0] = rest;
        for (i = 1; i < 4; i++) {
            f[i] = f[i - 1] ? SDL_strchr(f[i - 1], ':') : NULL;
            if (f[i])
                *f[i]++ = 0;
        }
        if (!f[3])
            return 0;
        SDL_strlcpy(host, f[0], sizeof(host));
        port = SDL_atoi(f[1]);
        SDL_strlcpy(sRoom, f[2], sizeof(sRoom));
        sRole = SDL_strcmp(f[3], "host") == 0 ? PORT_NETLINK_HOST : PORT_NETLINK_JOIN;
        sRelay = 1;
    }
    if (port <= 0 || port > 65535 || !sRole)
        return 0;
    if (!UdpOpen(sRole == PORT_NETLINK_HOST && !sRelay ? port : 0)) {
        PortLog("netlink: could not bind port %d", port);
        return 0;
    }
    if (sRole == PORT_NETLINK_JOIN || sRelay) { /* the other player, or the relay, by address or name */
        if (!UdpResolve(host, port, &sPeer)) {
            PortLog("netlink: %s not found", host);
            return 0;
        }
        sHavePeer = 1;
    }
    if (SDL_getenv("KHCOM_NETLINK_LAG"))
        sLag = SDL_atoi(SDL_getenv("KHCOM_NETLINK_LAG"));
    if (SDL_getenv("KHCOM_NETLINK_DROP")) {
        unsigned a = 0, b = 0;
        if (SDL_sscanf(SDL_getenv("KHCOM_NETLINK_DROP"), "%u-%u", &a, &b) == 2) {
            sDropFrom = a;
            sDropTo = b;
        }
    }
    if (SDL_getenv("KHCOM_NETLINK_JITTER"))
        sJitter = SDL_atoi(SDL_getenv("KHCOM_NETLINK_JITTER"));
    if (SDL_getenv("KHCOM_NETLINK_LOSS"))
        sLoss = SDL_atoi(SDL_getenv("KHCOM_NETLINK_LOSS"));
    PortLog("netlink: %s, port %d%s%s%s%s", sRole == PORT_NETLINK_HOST ? "hosting" : "joining", port,
            sRole == PORT_NETLINK_JOIN || sRelay ? " at " : "", host, sRelay ? ", room " : "", sRelay ? sRoom : "");
    if (sRelay)
        RelayHello();
    return 1;
}

/* the link's setting: KHCOM_NETLINK, else the first line of netlink.cfg */
/* netlink.cfg's line (or KHCOM_NETLINK), as written: "lobby:..." is the lobby's (port_lobby.c), not a link */
const char* PortNetlinkConfig(void) {
    static char line[160];
    const char* env = SDL_getenv("KHCOM_NETLINK");
    FILE* f;
    int i;
    if (env)
        return env;
    f = fopen(PortConfigPath("netlink.cfg"), "r");
    if (!f)
        return NULL;
    if (!fgets(line, sizeof(line), f))
        line[0] = 0;
    fclose(f);
    for (i = 0; line[i] && line[i] != '\r' && line[i] != '\n'; i++)
        ;
    line[i] = 0;
    return line[0] && SDL_strcmp(line, "off") != 0 ? line : NULL;
}

static const char* Spec(void) {
    const char* spec = sUseSet ? (sUse[0] ? sUse : NULL) : PortNetlinkConfig();
    return spec && SDL_strncmp(spec, "lobby", 5) != 0 ? spec : NULL;
}

/* The lobby's choice (NULL: the cable): the link starts again with it at its next use */
void PortNetlinkUse(const char* spec) {
    SDL_strlcpy(sUse, spec ? spec : "", sizeof(sUse));
    sUseSet = 1;
    sChecked = 0;
    sRole = 0;
    sSteam = 0;
    sRelay = 0;
    sRelayPlayers = 0;
    sHavePeer = 0;
    sLastHeard = 0;
    sAnnounce = 0;
    SDL_memset(&sPeer, 0, sizeof(sPeer));
    PortLog("netlink: the lobby picked %s", spec ? spec : "the cable");
}

/* the lobby's host: its room is listed publicly while it waits (relay servers; Steam lists its own lobbies) */
void PortNetlinkAnnounce(int version, int region, const char* name) {
    sAnnounce = 1;
    sAnnounceVersion = version;
    sAnnounceRegion = region;
    SDL_strlcpy(sAnnounceName, name ? name : "", sizeof(sAnnounceName));
}

/* The lobby's list on a relay server: 'L' asked (the socket opened for it), 'R' read back (-1 while none came) */
static NetAddr sLobbyServer;
int PortNetlinkLobbyAsk(const char* server, int port, int version) {
    uint8_t d[2] = { 'L', (uint8_t)version };
    if (!UdpReady() && !UdpOpen(0))
        return 0;
    if (!UdpResolve(server, port, &sLobbyServer))
        return 0;
    UdpSend(&sLobbyServer, d, 2);
    return 1;
}

int PortNetlinkLobbyRead(char (*codes)[17], char (*names)[25], int* regions, int max) {
    uint8_t d[PORT_NETLINK_MAX];
    NetAddr from;
    int n, count = -1;
    if (!UdpReady())
        return -1;
    while ((n = UdpRecv(d, sizeof(d), &from)) > 0) {
        int at = 2, i;
        if (d[0] != 'R' || n < 2 || !SameAddr(&from, &sLobbyServer))
            continue;
        count = 0;
        for (i = 0; i < d[1] && count < max && at + 18 <= n; i++) {
            int len = d[at + 17];
            if (at + 18 + len > n)
                break;
            regions[count] = d[at];
            SDL_memcpy(codes[count], d + at + 1, 16);
            codes[count][16] = 0;
            SDL_memcpy(names[count], d + at + 18, (size_t)(len > 24 ? 24 : len));
            names[count][len > 24 ? 24 : len] = 0;
            at += 18 + len;
            count++;
        }
    }
    return count;
}

int PortNetlinkRole(void) {
    if (!sChecked) {
        const char* spec = Spec();
        sChecked = 1;
        if (spec && SDL_strncmp(spec, "steam:", 6) == 0) {
            sRole = SDL_strcmp(spec + 6, "host") == 0 ? PORT_NETLINK_HOST : PORT_NETLINK_JOIN;
            sSteam = PortSteamlinkStart(sRole);
            if (!sSteam)
                sRole = 0;
            else
                PortLog("netlink: through Steam, %s", sRole == PORT_NETLINK_HOST ? "hosting" : "joining");
        } else if (spec && !Open(spec)) {
            sRole = 0;
        }
    }
    return UdpReady() || sSteam ? sRole : 0;
}

static void SendHeld(void);

static void SendNow(const uint8_t* data, int size) {
    if (sDropTo && sFrame >= sDropFrom && sFrame < sDropTo)
        return;
    if (sLoss > 0 && (int)TestRand(100) < sLoss)
        return;
    if (sSteam) {
        PortSteamlinkSend(data, size);
        return;
    }
    if (sRelay) {
        uint8_t d[PORT_NETLINK_MAX + 1];
        d[0] = 'D';
        SDL_memcpy(d + 1, data, (size_t)size);
        UdpSend(&sPeer, d, size + 1);
        return;
    }
    UdpSend(&sPeer, data, size);
}

void PortNetlinkSend(const uint8_t* data, int size) {
    if (!PortNetlinkRole() || !(sHavePeer || sSteam) || size <= 0 || size > PORT_NETLINK_MAX)
        return;
    if ((sLag > 0 || sJitter > 0) && sHeldCount < (int)SDL_arraysize(sHeld)) {
        Held* h = &sHeld[sHeldCount++];
        h->due = (uint32_t)SDL_GetTicks() + ((uint32_t)sLag + (sJitter > 0 ? TestRand((uint32_t)sJitter + 1) : 0)) * 50 / 3;
        h->size = size;
        SDL_memcpy(h->data, data, (size_t)size);
        return;
    }
    SendNow(data, size);
}

int PortNetlinkRecv(uint8_t* data, int cap) {
    NetAddr from;
    int n;
    if (!PortNetlinkRole())
        return 0;
    if (sHeldCount)
        SendHeld(); /* a game waiting for the other one runs no frames: its held datagrams still go */
    if (sSteam) {
        n = PortSteamlinkRecv(data, cap);
        if (n > 0)
            sLastHeard = SDL_GetTicks();
        return n;
    }
    n = UdpRecv(data, cap, &from);
    if (n <= 0)
        return 0;
    if (sRelay) { /* only the relay writes: 'O' its room count, 'D' the other player's datagram */
        if (!SameAddr(&from, &sPeer))
            return 0;
        if (data[0] == 'O' && n >= 2) {
            if (data[1] != sRelayPlayers)
                PortLog("netlink: relay room %s, %d player(s)", sRoom, data[1]);
            sRelayPlayers = data[1];
            return 0;
        }
        if (data[0] != 'D')
            return 0;
        SDL_memmove(data, data + 1, (size_t)--n);
    } else if (sRole == PORT_NETLINK_HOST && !sHavePeer) { /* the first player to write is the other end of the cable */
        sPeer = from;
        sHavePeer = 1;
        PortLog("netlink: player 2 at %s", AddrText(&from));
    } else if (!SameAddr(&from, &sPeer)) {
        /* player 2 back from another address (a new connection after a cut): taken once the old one went quiet */
        if (sRole != PORT_NETLINK_HOST || PortNetlinkSilentMs() < 2000)
            return 0;
        sPeer = from;
        PortLog("netlink: player 2 back at %s", AddrText(&from));
    }
    sLastHeard = SDL_GetTicks();
    return n;
}

/* the tests' held datagrams whose time has come */
static void SendHeld(void) {
    int i, kept = 0;
    uint32_t now = (uint32_t)SDL_GetTicks();
    for (i = 0; i < sHeldCount; i++) {
        if ((int32_t)(now - sHeld[i].due) >= 0)
            SendNow(sHeld[i].data, sHeld[i].size);
        else
            sHeld[kept++] = sHeld[i];
    }
    sHeldCount = kept;
}

void PortNetlinkFrame(void) {
    sFrame++;
    if (sSteam)
        PortSteamlinkFrame();
    if (sRelay && UdpReady() && sFrame % 60 == 0) { /* the room again: the relay finds a changed address */
        RelayHello();
        if (sAnnounce && sRelayPlayers < 2 && sRole == PORT_NETLINK_HOST)
            RelayAnnounce();
    }
    SendHeld();
}

int PortNetlinkSilentMs(void) {
    return sLastHeard ? (int)(SDL_GetTicks() - sLastHeard) : 1 << 30;
}

uint32_t PortNetlinkMs(void) {
    return (uint32_t)SDL_GetTicks();
}

int PortNetlinkDelayOverride(void) {
    const char* d = SDL_getenv("KHCOM_NETLINK_DELAY");
    return d ? SDL_atoi(d) : gPortConfig.netDelay; /* the launcher's choice; 0 automatic */
}

int PortNetlinkGiveUpSeconds(void) {
    const char* d = SDL_getenv("KHCOM_NETLINK_GIVEUP");
    return d ? SDL_atoi(d) : 0;
}

int PortNetlinkFakeDesync(void) { /* KHCOM_NETLINK_FAKEDESYNC: tests of the desync check */
    static int fake = -1;
    if (fake < 0)
        fake = SDL_getenv("KHCOM_NETLINK_FAKEDESYNC") && *SDL_getenv("KHCOM_NETLINK_FAKEDESYNC");
    return fake;
}

int PortBtlCamTrace(void) {
    static int trace = -1;
    if (trace < 0)
        trace = SDL_getenv("KHCOM_BTLCAM") != NULL;
    return trace;
}

int PortNetlinkTrace(void) {
    static int trace = -1;
    if (trace < 0)
        trace = SDL_getenv("KHCOM_NETLINK_TRACE") != NULL;
    return trace;
}
#else
const char* PortNetlinkConfig(void) {
    return NULL;
}
void PortNetlinkUse(const char* spec) {
    (void)spec;
}
void PortNetlinkAnnounce(int version, int region, const char* name) {
    (void)version;
    (void)region;
    (void)name;
}
int PortNetlinkLobbyAsk(const char* server, int port, int version) {
    (void)server;
    (void)port;
    (void)version;
    return 0;
}
int PortNetlinkLobbyRead(char (*codes)[17], char (*names)[25], int* regions, int max) {
    (void)codes;
    (void)names;
    (void)regions;
    (void)max;
    return -1;
}
uint32_t PortNetlinkMs(void) {
    return 0;
}
int PortNetlinkDelayOverride(void) {
    return 0;
}
int PortNetlinkGiveUpSeconds(void) {
    return 0;
}
int PortNetlinkTrace(void) {
    return 0;
}
int PortBtlCamTrace(void) {
    return 0;
}
int PortNetlinkFakeDesync(void) {
    return 0;
}
int PortNetlinkRole(void) {
    return 0;
}
void PortNetlinkSend(const uint8_t* data, int size) {
    (void)data;
    (void)size;
}
int PortNetlinkRecv(uint8_t* data, int cap) {
    (void)data;
    (void)cap;
    return 0;
}
void PortNetlinkFrame(void) {
}
int PortNetlinkSilentMs(void) {
    return 1 << 30;
}
#endif
