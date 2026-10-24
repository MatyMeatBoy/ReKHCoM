/**
 * sio.c
 * Serial Communication Driver
 */

#include "macros.h"
#include "intr.h"
#include "sio.h"
#include "gba/io_reg.h"
#include "system_state.h"
#include "gba/macro.h"
#include "sio_types.h"
#include <stddef.h>
#include "types.h"
#include "main.h"
#include "sio_api.h"

u8 gSioLastSendCount EWRAM_COMMON(4);
s16 gSioErrorFrameCount EWRAM_COMMON(4);
u16 gSioRecvFrame[4][2] EWRAM_COMMON(16);
u32 gSioErrorStatus EWRAM_COMMON(4);
s32 (*gSioLinkRecvCallback)() EWRAM_COMMON(8);
u8 gSioPlayerCount EWRAM_COMMON(4);
u8 gSioLastRecvCount EWRAM_COMMON(4);
s32 (*gSioLinkSendCallback)() EWRAM_COMMON(4);
u16 gSioCommandRecv[4][2] EWRAM_COMMON(16);
u32 gSioStatus EWRAM_COMMON(4);
u8 gSioHandshake EWRAM_COMMON(4);
u32 gSioPlayerId EWRAM_COMMON(4);
u8 gSioHandshakeRequest EWRAM_COMMON(4);
SioWork gSioWork EWRAM_COMMON(16);
u16 gSioCommandSend[4] EWRAM_COMMON(8);
u8 gSioLinkResult EWRAM_COMMON(4);
u16 gSioSendFrame[4] EWRAM_COMMON(8);

static u8 sSioChecksumReady;
static u16 sSioSavedIme;
static u8 sSioIdleVBlanks;
static u8 sSioSendEmpty;
static u8 sSioPauseTimer;
static u8 sSioPrevPlayerCount;
static s8 sSioAutoStartDone;
static u16 sSioSendNonzero;
static u16 sSioRecvNonzero;

enum SioState {
    SIO_STATE_STOP,
    SIO_STATE_INIT,
    SIO_STATE_HANDSHAKE,
    SIO_STATE_INIT_TIMER,
    SIO_STATE_CONNECTED
};

enum SioRequest {
    SIO_REQUEST_NONE,
    SIO_REQUEST_START,
    SIO_REQUEST_RESET
};

enum SioRole {
    SIO_ROLE_CHILD,
    SIO_ROLE_PARENT = 8
};

enum SioQueueFull {
    SIO_QUEUE_FULL_SEND = 1,
    SIO_QUEUE_FULL_RECV = 2
};

enum SioTimeout {
    SIO_TIMEOUT_PARENT = 1,
    SIO_TIMEOUT_CHILD = 2
};

enum SioHandshakeWord {
    SIO_HANDSHAKE_START = 0x8FFF,
    SIO_HANDSHAKE_IDLE = 0xD5E0
};

#define SIO_MULTI_EMPTY 0xFFFF
#define SIO_PAUSE_REQUEST 0x1000

#ifdef PLATFORM_PC
/* The cable over the network (port/src/port_netlink.c, port/progress/ONLINE_ESTUDIO.md): the game's packet queue
   stays as it is; instead of the registers, each game frame is a slot both players fill with one packet (or none)
   and send by UDP; slot n of player 1 and slot n of player 2 make the pair the cable would have carried, given to
   both games in the same order. The game's own empty state and pause request absorb the network's delay. */
#include "port/port.h"
#include "port/port_netlink.h"
#include <stdlib.h>
#include <string.h>
#include "port/port_ui.h"

#define NET_RING 64
#define NET_RESEND 24 /* slots sent again in every datagram, until the other side has them */
#define NET_VERSION 3 /* the slots' rules: both games must play by the same ones (HELLO carries it) */
#define NET_WINDOW 12 /* link frames the game may run ahead on predicted packets before it waits */
enum { NET_HELLO = 1, NET_START, NET_SLOTS, NET_HASH };
#define NET_HASH_EVERY 120 /* link frames between desync checks during a battle */
typedef struct {
    u16 words[4];
    u8 pause;
} NetSlot;
/* The network's state lives outside the game's memory (allocated once): rolling the game back (port_rollback.c) must
   not take back what was sent or received. */
typedef struct {
    u32 mine;   /* slots of ours made: the link frames this game has run since the connection */
    u32 acked;  /* ours the other side has */
    u32 theirs; /* theirs we have, all of them below */
    NetSlot my[NET_RING], peer[NET_RING], guess[NET_RING];
    u32 peerIdx[NET_RING];  /* slot + 1 of the peer's slot held there */
    u32 guessIdx[NET_RING]; /* slot + 1 when that slot was played on a guess */
    u32 rollbackTo;         /* the earliest link frame to play again (a wrong guess), ~0 none */
    /* a slot of ours can change when its frame is played again (the cartridge's packets carry what it received:
       the parent relays the keys, the menus answer): the new one is sent again from `resendFrom` for a while, and
       the other side takes back the frames that used the old one */
    u32 resendFrom, resendUntil, revisions; /* resendUntil: a clock time (PortNetlinkMs) */
    u8 pauseFrames;
    u8 startFrames; /* the parent repeats START this many frames */
    /* link frame n of both games plays the pair of slot n - delay (the input delay, 1-2 frames); a slot of the
       other player not here yet is guessed (its last one again) and the game goes on; when the real one differs,
       the game is rolled back to that frame and played again to now, in the same real frame (port_rollback.c).
       It only waits when the other player is more than NET_WINDOW frames behind. */
    u8 delay, peerAdvantage;
    u32 sentAt[NET_RING]; /* when each slot of ours was made: an ack of it gives the round trip */
    u32 rttMs;            /* smoothed */
    /* the player gave up waiting (B held after 10 s): from then on the link reports the cable's timeout, and the
       cartridge's own error screen ends it as when a cable is pulled */
    u8 dropped;
    u16 giveUpHold;
    u32 frames, waited, waitStart, rollbacks, guessed;
    /* the desync check, on confirmed link frames only: ours by frame, sent once final; theirs as received */
    u32 hashAt[8], hash[8], hashSent[8], peerHashAt[8], peerHash[8], checks, mismatches;
} NetState;
static NetState* sNetState;
#define sNet (*sNetState)
/* the handshake's measures */
static u32 sNetPeerTicks, sNetRttMin;
static u8 sNetPeerVersion, sNetVersionWarned;
/* the game's link frame: part of the game's memory, so a rollback takes it back with everything else */
static u32 sLinkFrame;
static u32 sNetLastFirst, sNetLastCount, sNetSlotsIn; /* KHCOM_NETLINK_TRACE: what came last */

static void NetReset(void) {
    if (sNetState == NULL)
        sNetState = (NetState*)malloc(sizeof(NetState));
    CpuFill32(0, sNetState, sizeof(NetState));
    sNet.rollbackTo = 0xFFFFFFFF;
    sNet.resendFrom = 0xFFFFFFFF;
    sLinkFrame = 0;
}

static int NetPeerKnown(u32 slot) {
    return sNet.peerIdx[slot % NET_RING] == slot + 1;
}

/* both hashes of one link frame known: compare */
static void NetHashCompare(u32 at) {
    int i, j;
    for (i = 0; i < 8; i++)
        if (sNet.hashSent[i] == 1 && sNet.hashAt[i] == at)
            for (j = 0; j < 8; j++)
                if (sNet.peerHash[j] && sNet.peerHashAt[j] == at) {
                    sNet.checks++;
                    if (sNet.hash[i] != sNet.peerHash[j]) {
                        if (sNet.mismatches++ == 0)
                            PortUiLinkNotice("Link: the games differ (desync); reported in the log",
                                             "Enlace: las partidas difieren (desincronizacion); queda en el registro", NULL);
                        PortLog("netlink: DESYNC at link frame %u: %08x here, %08x there", (unsigned)at,
                                (unsigned)sNet.hash[i], (unsigned)sNet.peerHash[j]);
                    } else if (sNet.checks % 10 == 0) {
                        PortLog("netlink: %u state checks, %u desync", (unsigned)sNet.checks, (unsigned)sNet.mismatches);
                    }
                    sNet.hashSent[i] = 2; /* compared */
                    sNet.peerHash[j] = 0;
                    return;
                }
}

static int NetActive(void) {
    return PortNetlinkRole() != 0;
}

static void NetPut16(u8* p, u16 v) {
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}
static u16 NetGet16(const u8* p) {
    return (u16)(p[0] | p[1] << 8);
}
static void NetPut32(u8* p, u32 v) {
    NetPut16(p, (u16)v);
    NetPut16(p + 2, (u16)(v >> 16));
}
static u32 NetGet32(const u8* p) {
    return NetGet16(p) | (u32)NetGet16(p + 2) << 16;
}

/* HELLO: role, version, our clock and the other's last one echoed (the round trip); START: role, the delay */
static void NetSendControl(u8 type) {
    u8 d[11] = { type, (u8)PortNetlinkRole(), NET_VERSION };
    if (type == NET_START)
        d[2] = sNet.delay;
    NetPut32(d + 3, PortNetlinkMs() | 1);
    NetPut32(d + 7, sNetPeerTicks);
    PortNetlinkSend(d, type == NET_HELLO ? 11 : 3);
}

/* how many link frames this game runs ahead of the other player's slots it has */
static u32 NetAdvantage(void) {
    u32 ahead = sNet.mine > sNet.theirs + sNet.delay ? sNet.mine - sNet.theirs - sNet.delay : 0;
    return ahead > 255 ? 255 : ahead;
}

/* ours from the oldest the other side lacks: type, first slot, count, what we have of theirs, the delay, how far
   ahead we run, then 9 bytes a slot */
static void NetSendSlots(void) {
    u8 d[12 + NET_RESEND * 9];
    u32 first = sNet.acked, i, n;
    if (sNet.resendFrom != 0xFFFFFFFF) { /* revised slots: in a datagram of their own, for a while (by the clock: a
                                            game that waits makes no frames) */
        if ((int)(PortNetlinkMs() - sNet.resendUntil) >= 0 || sNet.resendFrom >= first) {
            sNet.resendFrom = 0xFFFFFFFF;
        } else {
            u32 keep = sNet.acked;
            sNet.acked = sNet.resendFrom;
            sNet.resendFrom = 0xFFFFFFFF;
            NetSendSlots();
            sNet.resendFrom = sNet.acked;
            sNet.acked = keep;
        }
    }
    n = sNet.mine - first; /* from the oldest the other side lacks: after an outage nothing is skipped */
    if (n > NET_RESEND)
        n = NET_RESEND;
    d[0] = NET_SLOTS;
    NetPut32(d + 1, first);
    d[5] = (u8)n;
    NetPut32(d + 6, sNet.theirs);
    d[10] = sNet.delay;
    d[11] = (u8)NetAdvantage();
    for (i = 0; i < n; i++) {
        const NetSlot* s = &sNet.my[(first + i) % NET_RING];
        u8* p = d + 12 + i * 9;
        NetPut16(p, s->words[0]);
        NetPut16(p + 2, s->words[1]);
        NetPut16(p + 4, s->words[2]);
        NetPut16(p + 6, s->words[3]);
        p[8] = s->pause;
    }
    PortNetlinkSend(d, (int)(12 + n * 9));
}

/* every datagram waiting: their slots into the ring (a slot that was guessed wrong asks for a rollback); START (or
   their first slots) moves a waiting child on */
static void NetReceive(void) {
    u8 d[PORT_NETLINK_MAX];
    int n;
    while ((n = PortNetlinkRecv(d, sizeof(d))) > 0) {
        if (d[0] == NET_HELLO) {
            sNetPeerVersion = n >= 11 ? d[2] : 1;
            if (n >= 11) {
                u32 echo = NetGet32(d + 7);
                sNetPeerTicks = NetGet32(d + 3);
                if (echo) {
                    u32 rtt = (PortNetlinkMs() | 1) - echo;
                    if (rtt < 5000 && (sNetRttMin == 0 || rtt < sNetRttMin))
                        sNetRttMin = rtt ? rtt : 1;
                }
            }
            if (sNetPeerVersion != NET_VERSION && !sNetVersionWarned) {
                sNetVersionWarned = 1;
                PortLog("netlink: the other game plays link version %d, this one %d: update both", sNetPeerVersion,
                        NET_VERSION);
                PortUiLinkNotice("Link: the other player's ReKHCoM is a different version",
                                 "Enlace: el ReKHCoM del otro jugador es de otra version", NULL);
            }
            continue;
        }
        if (((d[0] == NET_START && n >= 3) || (d[0] == NET_SLOTS && n >= 12)) && gSioWork.state == SIO_STATE_HANDSHAKE &&
            !gSioWork.isParent && sNetPeerVersion == NET_VERSION) {
            gSioWork.state = SIO_STATE_CONNECTED;
            gSioWork.playerCount = 2; /* the parent was heard: two on the cable */
            gSioWork.handshake = 1;
            NetReset();
            sNet.delay = d[0] == NET_START ? d[2] : d[10];
            PortLog("netlink: connected (player 2), %d frames of input delay", sNet.delay);
        }
        if (sNetState == NULL)
            continue; /* nothing else before the connection */
        if (d[0] == NET_HASH && n >= 9) {
            u32 at = NetGet32(d + 1), slot = (at / NET_HASH_EVERY) % 8;
            sNet.peerHashAt[slot] = at;
            sNet.peerHash[slot] = NetGet32(d + 5);
            NetHashCompare(at);
            continue;
        }
        if (d[0] == NET_SLOTS && n >= 12 && gSioWork.state == SIO_STATE_CONNECTED) {
            u32 first = NetGet32(d + 1), ack = NetGet32(d + 6), i, count = d[5];
            sNetLastFirst = first;
            sNetLastCount = count;
            sNetSlotsIn++;
            if (ack > sNet.acked && ack <= sNet.mine) {
                u32 rtt = PortNetlinkMs() - sNet.sentAt[(ack - 1) % NET_RING];
                if (rtt < 5000)
                    sNet.rttMs = sNet.rttMs ? (sNet.rttMs * 7 + rtt) / 8 : rtt;
                sNet.acked = ack;
            }
            sNet.peerAdvantage = d[11];
            for (i = 0; i < count && 12 + (int)(i + 1) * 9 <= n; i++) {
                u32 slot = first + i;
                const u8* p = d + 12 + i * 9;
                NetSlot* s;
                if (slot + NET_RING / 2 < sNet.theirs || slot >= sNet.theirs + NET_RING / 2)
                    continue;
                s = &sNet.peer[slot % NET_RING];
                if (NetPeerKnown(slot)) { /* a revision: the frames that used the old one are played again */
                    if (s->words[0] == NetGet16(p) && s->words[1] == NetGet16(p + 2) && s->words[2] == NetGet16(p + 4) &&
                        s->words[3] == NetGet16(p + 6))
                        continue;
                    if (slot + sNet.delay < sLinkFrame && slot + sNet.delay < sNet.rollbackTo)
                        sNet.rollbackTo = slot + sNet.delay;
                    sNet.revisions++;
                }
                s->words[0] = NetGet16(p);
                s->words[1] = NetGet16(p + 2);
                s->words[2] = NetGet16(p + 4);
                s->words[3] = NetGet16(p + 6);
                s->pause = p[8];
                sNet.peerIdx[slot % NET_RING] = slot + 1;
                if (sNet.guessIdx[slot % NET_RING] == slot + 1) { /* played on a guess: was it right? */
                    const NetSlot* g = &sNet.guess[slot % NET_RING];
                    sNet.guessIdx[slot % NET_RING] = 0;
                    if (memcmp(g->words, s->words, sizeof(s->words)) != 0 && slot + sNet.delay < sNet.rollbackTo)
                        sNet.rollbackTo = slot + sNet.delay;
                }
            }
            while (NetPeerKnown(sNet.theirs))
                sNet.theirs++;
        }
    }
}

/* waiting for the other player inside a link frame: our slots out again, the picture and the window alive */
static void NetWait(void) {
    static u32 noticed;
    u32 now = PortNetlinkMs();
    if (!sNet.waitStart)
        sNet.waitStart = now | 1;
    else if (now - sNet.waitStart > 1000 && now - noticed > 500) {
        noticed = now;
        PortUiLinkWaitingNotice((int)((now - sNet.waitStart) / 1000), now - sNet.waitStart > 10000);
        if ((now - sNet.waitStart) / 500 % 4 == 2)
            PortLog("netlink: waiting at link frame %u: %u of ours made, %u acked, %u of theirs; last in %u+%u (%u)",
                    (unsigned)sLinkFrame, (unsigned)sNet.mine, (unsigned)sNet.acked, (unsigned)sNet.theirs,
                    (unsigned)sNetLastFirst, (unsigned)sNetLastCount, (unsigned)sNetSlotsIn);
    }
    NetSendSlots();
    PortLinkIdle();
    NetReceive();
    if (sNet.waitStart && now - sNet.waitStart > 10000) { /* after 10 s, B held a moment gives up */
        int autoGiveUp = PortNetlinkGiveUpSeconds(); /* tests: KHCOM_NETLINK_GIVEUP=<seconds> */
        sNet.giveUpHold = (PortInputKeys() & 0x0002 /* B */) ? sNet.giveUpHold + 1 : 0;
        if (sNet.giveUpHold > 20 || (autoGiveUp > 0 && now - sNet.waitStart > (u32)autoGiveUp * 1000)) {
            sNet.dropped = 1;
            PortLog("netlink: the player gave up waiting: the link ends as a pulled cable");
        }
    }
}

/* the battle's desync check: the state before link frame `f`, kept until every slot it depends on is confirmed */
static void NetHashFrame(u32 f) {
    extern u32 VsBattleStateHash(void);
    u32 h, slot;
    if (f % NET_HASH_EVERY != 0)
        return;
    h = VsBattleStateHash();
    if (h && PortNetlinkFakeDesync() && f >= 1200) /* tests: the check must catch a difference */
        h ^= 1;
    if (!h)
        return;
    slot = (f / NET_HASH_EVERY) % 8;
    if (sNet.hashAt[slot] == f && sNet.hashSent[slot])
        return; /* already sent: final (it is sent 48 frames later, past any rollback) */
    sNet.hashAt[slot] = f;
    sNet.hash[slot] = h; /* a rollback that plays this frame again writes it again */
    sNet.hashSent[slot] = 0;
}

static void NetHashSend(void) {
    int i;
    for (i = 0; i < 8; i++) {
        u32 at = sNet.hashAt[i];
        if (sNet.hash[i] && !sNet.hashSent[i] && at + 48 <= sLinkFrame && at + 48 <= sNet.theirs &&
            sNet.rollbackTo == 0xFFFFFFFF && sNet.resendFrom == 0xFFFFFFFF) {
            u8 d[9];
            d[0] = NET_HASH;
            NetPut32(d + 1, at);
            NetPut32(d + 5, sNet.hash[i]);
            PortNetlinkSend(d, 9);
            PortNetlinkSend(d, 9); /* twice: a lost check is only a skipped check */
            sNet.hashSent[i] = 1;
            NetHashCompare(at);
        }
    }
}

/* the link frame: one packet of the game's send queue is our slot (made once; a frame played again after a
   rollback makes the same one); the pair of `delay` frames ago into the receive queue, the other player's half
   guessed if it is not here yet */
static void NetSlotsFrame(u16* sendFrame, u16 (*recvFrame)[2]) {
    u32 i, f = sLinkFrame++;
    int waited = 0;
    u16 words[4];
    /* the network never cuts the link: while the other player is away the game waits (its packets stop being
       taken once the queue is nearly full, instead of the cable's "queue full" error), no timeout is reported */
    if (!gSioWork.paused && gSioWork.sendCount < 28)
        SioQueueSendFrame(sendFrame);
    NetReceive();
    if (gSioWork.sendCount > 0) {
        for (i = 0; i < 4; i++)
            words[i] = gSioWork.sendBuf[i][gSioWork.sendReadIdx];
        gSioWork.sendCount--;
        if (++gSioWork.sendReadIdx > 31)
            gSioWork.sendReadIdx = 0;
    } else {
        CpuFill16(0, words, sizeof(words));
    }
    if (f == sNet.mine) { /* a new frame: our slot for it */
        NetSlot* s = &sNet.my[f % NET_RING];
        while (sNet.mine - sNet.acked >= NET_RING - 4 && gSioWork.state == SIO_STATE_CONNECTED && !sNet.dropped) { /* ring full */
            NetWait();
            waited = 1;
        }
        CpuCopy16(words, s->words, sizeof(words));
        s->pause = 0;
        sNet.sentAt[f % NET_RING] = PortNetlinkMs();
        sNet.mine++;
        NetSendSlots();
    } else if (f < sNet.mine && memcmp(words, sNet.my[f % NET_RING].words, sizeof(words)) != 0) {
        /* played again with what really came, the frame made another packet: it replaces the one sent */
        CpuCopy16(words, sNet.my[f % NET_RING].words, sizeof(words));
        if (f < sNet.resendFrom)
            sNet.resendFrom = f;
        sNet.resendUntil = PortNetlinkMs() + 500;
        sNet.revisions++;
        if (PortNetlinkTrace())
            PortLog("netlink: link frame %u revised", (unsigned)f);
    }
    if (f >= sNet.delay) {
        u32 p = f - sNet.delay;
        const NetSlot* by[2];
        NetSlot* theirs;
        u16 nonzero = 0;
        /* guesses only in the battle (its packets are keys); the link's menus change screens and load on what they
           receive, so there the frame waits for the real packet (lockstep). In the battle it waits only when too far
           ahead of the other player (an outage, or a much slower machine). */
        extern int VsBattleRunning(void);
        while (!NetPeerKnown(p) && (p >= sNet.theirs + NET_WINDOW || !VsBattleRunning()) &&
               gSioWork.state == SIO_STATE_CONNECTED && !sNet.dropped) {
            NetWait();
            waited = 1;
        }
        if (NetPeerKnown(p)) {
            theirs = &sNet.peer[p % NET_RING];
        } else { /* not here yet: their last slot again */
            theirs = &sNet.guess[p % NET_RING];
            if (sNet.theirs > 0)
                *theirs = sNet.peer[(sNet.theirs - 1) % NET_RING];
            else
                CpuFill16(0, theirs, sizeof(*theirs));
            sNet.guessIdx[p % NET_RING] = p + 1;
            sNet.guessed++;
        }
        by[gSioWork.playerId] = &sNet.my[p % NET_RING];
        by[gSioWork.playerId ^ 1] = theirs;
        for (i = 0; i < 4; i++)
            nonzero |= by[0]->words[i] | by[1]->words[i];
        if (nonzero && PortNetlinkTrace())
            PortLog("netlink: pair %u  %04x %04x %04x %04x | %04x %04x %04x %04x", (unsigned)p, by[0]->words[0],
                    by[0]->words[1], by[0]->words[2], by[0]->words[3], by[1]->words[0], by[1]->words[1],
                    by[1]->words[2], by[1]->words[3]);
        if (nonzero && gSioWork.recvCount < 32) {
            u8 idx = gSioWork.recvReadIdx + gSioWork.recvCount;
            if (idx > 31)
                idx -= 32;
            for (i = 0; i < 4; i++) {
                gSioWork.recvBuf[0][i][idx] = by[0]->words[i];
                gSioWork.recvBuf[1][i][idx] = by[1]->words[i];
            }
            gSioWork.recvCount++;
        }
    }
    /* the machine running ahead gives a frame of time back now and then, so neither keeps playing on guesses: both
       end up as far from the other, half the round trip each */
    if (f + 1 == sNet.mine && f % 8 == 0 && NetAdvantage() > (u32)sNet.peerAdvantage + 1)
        PortLinkYieldFrame();
    sNet.waitStart = 0;
    if (f + 1 == sNet.mine && sNet.mine % 600 == 0)
        PortLog("netlink: slot %u, %u of theirs, %u guessed, %u rollbacks, %u revisions", (unsigned)sNet.mine,
                (unsigned)sNet.theirs, (unsigned)sNet.guessed, (unsigned)sNet.rollbacks, (unsigned)sNet.revisions);
    gSioWork.paused = sNet.pauseFrames > 0;
    if (sNet.pauseFrames > 0)
        sNet.pauseFrames--;
    if (sNet.startFrames > 0) {
        sNet.startFrames--;
        NetSendControl(NET_START);
    }
    gSioWork.queueFull = 0;
    gSioWork.timeout = sNet.dropped; /* given up: the cable's timeout, every frame, until the game ends the link */
    gSioWork.hardwareError = 0;
    gSioWork.checksumError = 0;
    SioReadRecvFrame(recvFrame);
    if (gSystemFlags & SYSTEM_FLAG_LINK_ACTIVE) {
        NetHashFrame(f);
        extern int VsBattleRunning(void);
        if (f + 1 == sNet.mine && VsBattleRunning()) { /* the battle: how often a frame had to wait for the other player */
            sNet.frames++;
            sNet.waited += waited;
            if (sNet.frames % 600 == 0)
                PortLog("netlink: battle %u frames, %u waited, %u rollbacks, %d frames of input delay",
                        (unsigned)sNet.frames, (unsigned)sNet.waited, (unsigned)sNet.rollbacks, sNet.delay);
        }
    }
    NetHashSend();
    if (PortNetlinkTrace() && !gSioWork.recvEmpty)
        PortLog("netlink: read %04x %04x | %04x %04x", recvFrame[0][0], recvFrame[1][0], recvFrame[0][1], recvFrame[1][1]);
}

/* mode_sio.c: a link is going (the lobby is not opened again) */
int NetLinkConnected(void) {
    return sNetState != NULL && NetActive() && gSioWork.state == SIO_STATE_CONNECTED && !sNet.dropped;
}

/* port_ui.c: the network line on screen (net_stats): round trip, input delay; 0 when no link session */
int NetLinkStats(int* rttMs, int* delay) {
    if (!NetLinkConnected())
        return 0;
    *rttMs = (int)(sNet.rttMs ? sNet.rttMs : sNetRttMin);
    *delay = sNet.delay;
    return 1;
}

/* port_rollback.c: the link session's frames */
int NetRollbackSession(void) {
    return sNetState != NULL && NetActive() && gSioWork.state == SIO_STATE_CONNECTED && !sNet.dropped;
}
u32 NetLinkFrame(void) {
    return sLinkFrame;
}
u32 NetLinkMade(void) {
    return sNetState != NULL ? sNet.mine : 0;
}
/* the earliest link frame a wrong guess spoiled, taken once; ~0 none */
u32 NetTakeRollback(void) {
    u32 to;
    if (sNetState == NULL)
        return 0xFFFFFFFF;
    to = sNet.rollbackTo;
    sNet.rollbackTo = 0xFFFFFFFF;
    if (to != 0xFFFFFFFF)
        sNet.rollbacks++;
    return to;
}

/* the input delay the parent picks: 1 frame on a near connection, 2 beyond about 2 frames one way (fewer guesses to
   take back); KHCOM_NETLINK_DELAY fixes it (tests) */
static u8 NetPickDelay(void) {
    int d = PortNetlinkDelayOverride();
    if (d <= 0)
        d = sNetRttMin / 2 > 34 ? 2 : 1;
    return (u8)(d > 8 ? 8 : d);
}

/* the handshake: the host is the parent (player 1), whoever joins the child; both see two players once the other
   is heard; the parent's start request starts the slots on both sides */
static void NetHandshake(u8* request) {
    int peer;
    gSioWork.isParent = PortNetlinkRole() == PORT_NETLINK_HOST ? 8 : 0;
    gSioWork.playerId = gSioWork.isParent ? 0 : 1;
    NetSendControl(NET_HELLO);
    NetReceive();
    if (gSioWork.state != SIO_STATE_HANDSHAKE)
        return; /* START arrived */
    peer = PortNetlinkSilentMs() < 1000 && sNetPeerVersion == NET_VERSION;
    gSioWork.playerCount = peer ? 2 : 1;
    gSioWork.handshake = peer ? 1 : 0;
    if ((*request == 1 || (sSioAutoStartDone == 0 && peer)) && gSioWork.isParent && peer) {
        u8 delay;
        if (!sNetRttMin)
            return; /* the round trip is measured in a frame or two: the start waits for it */
        delay = NetPickDelay();
        gSioWork.state = SIO_STATE_CONNECTED;
        NetReset();
        sNet.delay = delay;
        sNet.startFrames = 30;
        sSioAutoStartDone = -1;
        PortLog("netlink: connected (player 1), round trip %u ms, %d frames of input delay", (unsigned)sNetRttMin, delay);
    } else if (*request == 1) {
        sSioAutoStartDone = -1;
    }
}
#endif

u16 IsVBlankIntrLive() {
    if (REG_IME & 1) {
        if (REG_DISPSTAT & DISPSTAT_VBLANK_INTR) {
            if (REG_IE & INTR_FLAG_VBLANK) {
                if (!(REG_DISPCNT & DISPCNT_FORCED_BLANK)) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void SetVBlankCallback(IntrFunc fn) {
    gVBlankCallback = fn;

    if (!IsVBlankIntrLive()) {
        *gIntrTableVBlank = fn;
    }
}

void ResetVBlankCallback() {
    *gIntrTableVBlank = VBlankIntr;
    gVBlankCallback = VBlankIntr;
}

void SetVCountCallback(IntrFunc fn) {
    gVCountCallback = fn;

    if (!IsVBlankIntrLive()) {
        *gIntrTableVCount = fn;
    }
}

void ResetVCountCallback() {
    *gIntrTableVCount = VCountIntrDummy;
    gVCountCallback = VCountIntrDummy;
}

void SetHBlankCallback(IntrFunc fn) {
    gHBlankCallback = fn;

    if (!IsVBlankIntrLive()) {
        *gIntrTableHBlank = fn;
    }
}

void ResetHBlankCallback() {
    *gIntrTableHBlank = HBlankIntrDummy;
    gHBlankCallback = HBlankIntrDummy;
}

void SetSerialCallback(IntrFunc fn) {
    *gIntrTableSerial = fn;
}

void ResetSerialCallback() {
    *gIntrTableSerial = SerialIntrDummy;
}

void SetTimer3Callback(IntrFunc fn) {
    *gIntrTableTimer3 = fn;
}

void ResetTimer3Callback() {
    *gIntrTableTimer3 = SerialIntrDummy;
}

void SioInit() {
    u16* savedIme;
    u16 ime;

    savedIme = &sSioSavedIme;
    ime = REG_IME;
    REG_IME = 0;
    REG_IE &= ~(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_IME = ime;
    REG_RCNT = 0;
    REG_SIOCNT = SIO_MULTI_MODE;
    REG_SIOCNT |= (SIO_INTR_ENABLE | SIO_115200_BPS);
    *savedIme = REG_IME;
    SetVBlankCallback(VBlankIntrSio);
    SetSerialCallback(SioSerialIntr);
    SetTimer3Callback(SioTimer3Intr);
    REG_IME = 0;
    REG_IE |= INTR_FLAG_SERIAL;
    REG_IME = *savedIme;
    REG_SIOMLT_SEND = 0;
    *(u64*)REG_ADDR_SIOMULTI0 = 0;
    CpuFill32(0, &gSioWork, sizeof(SioWork));
    sSioIdleVBlanks = 0;
    sSioSendEmpty = FALSE;
    sSioPrevPlayerCount = 0;
    gSioLastSendCount = 0;
    gSioLastRecvCount = 0;
    gSioStatus = 0;
    gSioErrorFrameCount = 0;
    gSioErrorStatus = 0;
    gSioPlayerId = 0;
    gSioPlayerCount = 0;
    gSioHandshake = 0;
    gSioHandshakeRequest = SIO_REQUEST_NONE;
    gSioLinkResult = SIO_LINK_RESULT_NONE;
    sSioAutoStartDone = 0;
    sSioChecksumReady = FALSE;
    sSioSendNonzero = 0;
    sSioRecvNonzero = 0;
    gSioLinkSendCallback = NULL;
    gSioLinkRecvCallback = NULL;
}

void SioReset() {
    SioInit();
    SioStop();
}

void func_08006E70() {
}

void SioStop() {
    sSioSavedIme = REG_IME;
    REG_IME = 0;
    REG_IE &= ~(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_IME = sSioSavedIme;
    REG_SIOCNT = 0;
    REG_TM3CNT_H = 0;
    REG_IF = (INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    CpuFill32(0, &gSioWork, sizeof(SioWork));
}

u32 SioRunStateMachine(u8* request, u16* sendFrame, u16 (*recvFrame)[2]) {
    u32 playerBits;
    u32 status;
    u32 result;
    u32 recvEmpty, handshake, hardwareError, checksumError, queueFull, timeout;

#ifdef PLATFORM_PC
    PortNetlinkFrame();
    if (NetActive() && gSioWork.state == SIO_STATE_HANDSHAKE && *request != SIO_REQUEST_RESET) {
        NetHandshake(request);
    } else if (NetActive() && (gSioWork.state == SIO_STATE_CONNECTED || gSioWork.state == SIO_STATE_INIT_TIMER)) {
        gSioWork.state = SIO_STATE_CONNECTED;
        NetSlotsFrame(sendFrame, recvFrame);
    } else
#endif
    switch (gSioWork.state) {
    case SIO_STATE_STOP:
        SioStop();
        gSioWork.state = SIO_STATE_INIT;
        break;
    case SIO_STATE_INIT:
        SioInit();
        gSioWork.state = SIO_STATE_HANDSHAKE;
        break;
    case SIO_STATE_HANDSHAKE:
        switch (*request) {
        default:
            SioCheckParent();

            if (sSioAutoStartDone == 0) {
                if (gSioWork.isParent != 0 && gSioWork.playerCount == 2) {
                    gSioWork.startPending = TRUE;
                    sSioAutoStartDone = -1;
                }
            }

            break;
        case SIO_REQUEST_START:
            if (gSioWork.isParent != 0 && gSioWork.playerCount == 2) {
                gSioWork.startPending = TRUE;
            }

            sSioAutoStartDone = -1;
            break;
        case SIO_REQUEST_RESET:
            gSioWork.state = SIO_STATE_STOP;
            REG_SIOMLT_SEND = 0;
            break;
        }

        break;
    case SIO_STATE_INIT_TIMER:
        SioInitTimer();
        gSioWork.state = SIO_STATE_CONNECTED;
    case SIO_STATE_CONNECTED:
        if (!gSioWork.paused) {
            SioQueueSendFrame(sendFrame);
        }

        SioReadRecvFrame(recvFrame);
        break;
    }

    *request = SIO_REQUEST_NONE;
    playerBits = gSioWork.playerId | (gSioWork.playerCount << SIO_STAT_PLAYER_COUNT_SHIFT);

    if (gSioWork.isParent == SIO_ROLE_PARENT) {
        playerBits |= SIO_STAT_PARENT;
    }

    recvEmpty = gSioWork.recvEmpty << SIO_STAT_RECV_EMPTY_SHIFT;
    handshake = gSioWork.handshake << SIO_STAT_HANDSHAKE_SHIFT;
    hardwareError = gSioWork.hardwareError << SIO_STAT_ERROR_HARDWARE_SHIFT;
    checksumError = gSioWork.checksumError << SIO_STAT_ERROR_CHECKSUM_SHIFT;
    queueFull = gSioWork.queueFull << SIO_STAT_ERROR_QUEUE_FULL_SHIFT;
    timeout = gSioWork.timeout << SIO_STAT_ERROR_TIMEOUT_SHIFT;

    if (gSioWork.state == SIO_STATE_CONNECTED) {
        status = playerBits | SIO_STAT_CONNECTED | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    } else {
        status = playerBits | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    }

    result = status;

    if (gSioWork.playerId > 1) {
        result |= SIO_STAT_ERROR_INVALID_ID;
    }

    return result;
}

u32 SioTransferFrames(u8* request, u16* sendFrame, u16 (*recvFrame)[2]) {
    u32 playerBits;
    u32 status;
    u32 result;
    u32 recvEmpty, handshake, hardwareError, checksumError, queueFull, timeout;

#ifdef PLATFORM_PC
    PortNetlinkFrame();
    if (NetActive() && gSioWork.state == SIO_STATE_CONNECTED)
        NetSlotsFrame(sendFrame, recvFrame);
    else
#endif
    if (gSioWork.state == SIO_STATE_CONNECTED) {
        if (!gSioWork.paused) {
            SioQueueSendFrame(sendFrame);
        }

        SioReadRecvFrame(recvFrame);
    }

    playerBits = gSioWork.playerId | (gSioWork.playerCount << SIO_STAT_PLAYER_COUNT_SHIFT);

    if (gSioWork.isParent == SIO_ROLE_PARENT) {
        playerBits |= SIO_STAT_PARENT;
    }

    recvEmpty = gSioWork.recvEmpty << SIO_STAT_RECV_EMPTY_SHIFT;
    handshake = gSioWork.handshake << SIO_STAT_HANDSHAKE_SHIFT;
    hardwareError = gSioWork.hardwareError << SIO_STAT_ERROR_HARDWARE_SHIFT;
    checksumError = gSioWork.checksumError << SIO_STAT_ERROR_CHECKSUM_SHIFT;
    queueFull = gSioWork.queueFull << SIO_STAT_ERROR_QUEUE_FULL_SHIFT;
    timeout = gSioWork.timeout << SIO_STAT_ERROR_TIMEOUT_SHIFT;

    if (gSioWork.state == SIO_STATE_CONNECTED) {
        status = playerBits | SIO_STAT_CONNECTED | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    } else {
        status = playerBits | recvEmpty | handshake | hardwareError | checksumError | queueFull | timeout;
    }

    result = status;

    if (gSioWork.playerId > 1) {
        result |= SIO_STAT_ERROR_INVALID_ID;
    }

    return result;
}

void SioCheckParent() {
    if (((*(vu32*)REG_ADDR_SIOCNT) & (SIO_MULTI_SI | SIO_MULTI_SD)) == SIO_MULTI_SD && gSioWork.playerId == 0) {
        gSioWork.isParent = SIO_ROLE_PARENT;
    } else {
        gSioWork.isParent = SIO_ROLE_CHILD;
    }
}

void SioInitTimer() {
    if (gSioWork.isParent != 0) {
        REG_TM3CNT_L = 0xFF2D;
        REG_TM3CNT_H = (TIMER_INTR_ENABLE | TIMER_64CLK);
        sSioSavedIme = REG_IME;
        REG_IME = 0;
        REG_IE |= INTR_FLAG_TIMER3;
        REG_IME = sSioSavedIme;
    }
}

void SioQueueSendFrame(u16* frame) {
    u8 idx;
    u8 i;

    sSioSavedIme = REG_IME;
    REG_IME = 0;

    if (gSioWork.sendCount < 32) {
        idx = gSioWork.sendCount + gSioWork.sendReadIdx;

        if (idx > 31) {
            idx -= 32;
        }

        for (i = 0; i < ARRAY_COUNT(gSioWork.sendBuf); i++) {
            sSioSendNonzero |= *frame;
            gSioWork.sendBuf[i][idx] = *frame;
            *frame = 0;
            frame++;
        }
    } else {
        gSioWork.queueFull |= SIO_QUEUE_FULL_SEND;
    }

    if (sSioSendNonzero != 0) {
        gSioWork.sendCount++;
        sSioSendNonzero = 0;
    }

    REG_IME = sSioSavedIme;
    gSioLastSendCount = gSioWork.sendCount;
}

void SioReadRecvFrame(u16 (*frame)[2]) {
    u8 i;
    u8 j;

    sSioSavedIme = REG_IME;
    REG_IME = 0;

    if (gSioWork.recvCount == 0) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < gSioWork.playerCount; j++) {
                frame[i][j] = 0;
            }
        }

        gSioWork.recvEmpty = 1;
    } else {
        for (i = 0; i < ARRAY_COUNT(gSioWork.recvBuf[0]); i++) {
            for (j = 0; j < gSioWork.playerCount; j++) {
                frame[i][j] = gSioWork.recvBuf[j][i][gSioWork.recvReadIdx];
            }
        }

        gSioWork.recvCount--;
        gSioWork.recvReadIdx++;

        if (gSioWork.recvReadIdx > 31) {
            gSioWork.recvReadIdx = 0;
        }

        gSioWork.recvEmpty = 0;
    }

    REG_IME = sSioSavedIme;
}

void SioVBlankUpdate() {
#ifdef PLATFORM_PC
    if (NetActive())
        return; /* no transfers to count: the network's timeout and pause are NetSlotsFrame's */
#endif
    if (gSioWork.paused) {
        sSioPauseTimer--;

        if (sSioPauseTimer != 0) {
            return;
        }

        gSioWork.paused = FALSE;
    }

    if (gSioWork.isParent != 0) {
        if (gSioWork.state == SIO_STATE_HANDSHAKE) {
            SioStartTransfer();
        } else if (gSioWork.state == SIO_STATE_CONNECTED) {
            if (gSioWork.transferCount <= 4) {
                if (gSioWork.hardwareError != 0) {
                    SioStartTransfer();
                } else {
                    gSioWork.timeout = SIO_TIMEOUT_PARENT;
                }
            } else if (gSioWork.timeout == 0) {
                gSioWork.transferCount = 0;
                SioStartTransfer();
            }
        }
    } else if (gSioWork.state == SIO_STATE_CONNECTED || gSioWork.state == SIO_STATE_HANDSHAKE) {
        sSioIdleVBlanks++;

        if (sSioIdleVBlanks > 6) {
            if (gSioWork.state == SIO_STATE_CONNECTED) {
                gSioWork.timeout = SIO_TIMEOUT_CHILD;
            }

            if (gSioWork.state == SIO_STATE_HANDSHAKE) {
                gSioWork.playerId = 0;
                gSioWork.playerCount = 0;
                gSioWork.handshake = 0;
            }
        }
    }
}

void SioTimer3Intr() {
    SioStopTimer();
    SioStartTransfer();
}

void SioSerialIntr() {
    u32 cnt;

    cnt = (*(vu32*)REG_ADDR_SIOCNT);
    gSioWork.playerId = (cnt << 26) >> 30;

    switch (gSioWork.state) {
    case SIO_STATE_CONNECTED:
        if (cnt & SIO_ERROR) {
            gSioWork.hardwareError = 1;
        }

        SioRecvWord();
        SioSendWord();
        SioFinishTransfer();
        break;
    case SIO_STATE_HANDSHAKE:
        if (SioHandshake()) {
            if (gSioWork.isParent != 0) {
                gSioWork.state = SIO_STATE_INIT_TIMER;
                gSioWork.transferCount = 4;
            } else {
                gSioWork.state = SIO_STATE_CONNECTED;
            }
        }

        break;
    }

    gSioWork.transferCount++;
    sSioIdleVBlanks = 0;

    if (gSioWork.transferCount == 4) {
        gSioLastRecvCount = gSioWork.recvCount;
    }
}

void SioStartTransfer() {
    REG_SIOCNT |= SIO_START;
}

u8 SioHandshake() {
    u8 count;
    u16 min;
    u8 i;

    count = 0;
    min = 0xFFFF;

    if (gSioWork.startPending == TRUE) {
        REG_SIOMLT_SEND = SIO_HANDSHAKE_START;
    } else {
        REG_SIOMLT_SEND = SIO_HANDSHAKE_IDLE;
    }

    gSioWork.startPending = FALSE;
    *(u64*)gSioWork.recv = *(u64*)REG_ADDR_SIOMULTI0;

    for (i = 0; i < 2; i++) {
        if ((gSioWork.recv[i] & ~3) == SIO_HANDSHAKE_IDLE || gSioWork.recv[i] == SIO_HANDSHAKE_START) {
            count++;

            if (min > gSioWork.recv[i] && gSioWork.recv[i] != 0) {
                min = gSioWork.recv[i];
            }
        } else if (gSioWork.recv[i] == SIO_MULTI_EMPTY) {
            if (i == gSioWork.playerId) {
                count = 0;
            }
        } else {
            count = 0;
            break;
        }
    }

    gSioWork.playerCount = count;

    if (gSioWork.playerCount == 2) {
        if (gSioWork.playerCount == sSioPrevPlayerCount && gSioWork.recv[0] == SIO_HANDSHAKE_START) {
            return TRUE;
        }

        if (gSioWork.playerCount == 2) {
            gSioWork.handshake = (min & 3) + 1;
        } else {
            gSioWork.handshake = 0;
        }
    } else {
        gSioWork.handshake = 0;
    }

    sSioPrevPlayerCount = gSioWork.playerCount;
    return FALSE;
}

void SioRecvWord() {
    u16 recv[4];
    u8 i;
    u8 idx;

    *(u64*)recv = *(u64*)REG_ADDR_SIOMULTI0;

    if (gSioWork.sendWordIdx == 0) {
        for (i = 0; i < gSioWork.playerCount; i++) {
            if (gSioWork.checksum != recv[i] && sSioChecksumReady) {
                gSioWork.checksumError = 1;
            }
        }

        gSioWork.checksum = 0;
        sSioChecksumReady = TRUE;
    } else {
        idx = gSioWork.recvReadIdx + gSioWork.recvCount;

        if (idx > 31) {
            idx -= 32;
        }

        if (gSioWork.recvCount < 32) {
            for (i = 0; i < gSioWork.playerCount; i++) {
                gSioWork.checksum += recv[i];
                sSioRecvNonzero |= recv[i];
                gSioWork.recvBuf[i][gSioWork.recvWordIdx][idx] = recv[i];

                if (gSioWork.sendWordIdx == 1 && !gSioWork.paused && (recv[i] & SIO_PAUSE_REQUEST)) {
                    gSioWork.paused = TRUE;
                    sSioPauseTimer = 5;
                }
            }
        } else {
            gSioWork.queueFull |= SIO_QUEUE_FULL_RECV;
        }

        gSioWork.recvWordIdx++;

        if (gSioWork.recvWordIdx == 4 && sSioRecvNonzero != 0) {
            gSioWork.recvCount++;
            sSioRecvNonzero = 0;
        }
    }
}

void SioSendWord() {
    if (gSioWork.sendWordIdx == 4) {
        REG_SIOMLT_SEND = gSioWork.checksum;

        if (!sSioSendEmpty) {
            gSioWork.sendCount--;
            gSioWork.sendReadIdx++;

            if (gSioWork.sendReadIdx > 31) {
                gSioWork.sendReadIdx = 0;
            }
        } else {
            sSioSendEmpty = FALSE;
        }
    } else {
        if (gSioWork.sendWordIdx == 0 && gSioWork.sendCount == 0) {
            sSioSendEmpty = TRUE;
        }

        if (sSioSendEmpty) {
            REG_SIOMLT_SEND = 0;
        } else {
            REG_SIOMLT_SEND = gSioWork.sendBuf[gSioWork.sendWordIdx][gSioWork.sendReadIdx];
        }

        if (!gSioWork.paused && gSioWork.sendWordIdx == 0 && gSioWork.recvCount > 3) {
            REG_SIOMLT_SEND |= SIO_PAUSE_REQUEST;
        }

        gSioWork.sendWordIdx++;
    }
}

void SioStopTimer() {
    if (gSioWork.isParent != 0) {
        REG_TM3CNT_H &= ~TIMER_ENABLE;
        REG_TM3CNT_L = 0xFF2D;
    }
}

void SioFinishTransfer() {
    if (gSioWork.recvWordIdx == 4) {
        gSioWork.sendWordIdx = 0;
        gSioWork.recvWordIdx = 0;
    } else if (gSioWork.isParent != 0) {
        REG_TM3CNT_H |= TIMER_ENABLE;
    }
}

void SioResetSendQueue() {
    u8 i;
    u8 j;

    gSioWork.sendCount = 0;
    gSioWork.sendReadIdx = 0;

    for (i = 0; i < ARRAY_COUNT(gSioWork.sendBuf); i++) {
        for (j = 0; j < ARRAY_COUNT(gSioWork.sendBuf[0]); j++) {
            gSioWork.sendBuf[i][j] = 0xEFFF;
        }
    }
}

void SioResetRecvQueue() {
    u8 i;
    u8 j;
    u8 k;

    gSioWork.recvCount = 0;
    gSioWork.recvReadIdx = 0;

    for (i = 0; i < ARRAY_COUNT(gSioWork.recvBuf); i++) {
        for (j = 0; j < ARRAY_COUNT(gSioWork.recvBuf[0]); j++) {
            for (k = 0; k < ARRAY_COUNT(gSioWork.recvBuf[0][0]); k++) {
                gSioWork.recvBuf[i][j][k] = 0xEFFF;
            }
        }
    }
}

void SioClearRegs() {
    REG_RCNT = 0;
    REG_SIOCNT = 0;
    REG_SIODATA8 = 0;
    *(vu16*)REG_ADDR_SIODATA32 = 0;
    REG_SIOMLT_SEND = 0;
    REG_SIOMLT_RECV = 0;
    REG_SIOMULTI0 = 0;
    REG_SIOMULTI1 = 0;
    REG_SIOMULTI2 = 0;
    REG_SIOMULTI3 = 0;
}

void SioShutdown() {
    SioClearRegs();
    REG_IME = 0;
    ResetVBlankCallback();
    ResetSerialCallback();
    ResetTimer3Callback();
    REG_IE = (INTR_FLAG_VBLANK | INTR_FLAG_GAMEPAK);
    REG_DISPSTAT = DISPSTAT_VBLANK_INTR;
    REG_IME = 1;
    SioStop();
}

u8 SioIsConnected() {
    if (gSioWork.state == SIO_STATE_CONNECTED) {
        return TRUE;
    }

    return FALSE;
}
