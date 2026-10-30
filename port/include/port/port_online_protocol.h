#ifndef PORT_ONLINE_PROTOCOL_H
#define PORT_ONLINE_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>

/* Direct-input protocol. This module has no sockets, waits, pacing or game state.
   Keep a peer outside rollback snapshots; only the game's logical frame rewinds. */
#define MODERN_PROTOCOL 4
#define MODERN_PACKET_MAX 512
#define MODERN_HISTORY 256
#define MODERN_CONTROLS 32
#define MODERN_CONTROL_MAX 400
#define MODERN_HASH_SLOTS 16

enum { MODERN_HELLO=1, MODERN_ACK, MODERN_CONTROL, MODERN_INPUT, MODERN_HASH };
enum { MODERN_OK, MODERN_ERR_VERSION, MODERN_ERR_REGION, MODERN_ERR_CONFLICT,
       MODERN_ERR_HISTORY, MODERN_ERR_DESYNC, MODERN_ERR_STATE };

typedef struct {
    uint32_t frame;
    uint16_t local, remote, usedRemote;
    uint8_t valid, localValid, remoteValid, usedValid;
} ModernInputSlot;

typedef struct {
    uint32_t sequence, id, sentAt;
    uint16_t size;
    uint8_t kind, valid, sent;
    uint8_t data[MODERN_CONTROL_MAX];
} ModernControl;

typedef struct {
    uint32_t frame, local, remote;
    uint8_t valid, localValid, remoteValid, sent, compared;
} ModernHash;

typedef struct {
    uint64_t session;
    uint8_t role, region, ready, peerReady, error, helloSent, ackDirty, inputsStarted;
    uint32_t helloAt, lastPeerTick, rtt, lastReceive;
    int32_t peerClockOffset;
    uint32_t txNext, txAck, rxNext;
    unsigned txCount, rxCount, rxRead;
    ModernControl tx[MODERN_CONTROLS], pending[MODERN_CONTROLS], rx[MODERN_CONTROLS];
    uint32_t round, base, made, remoteNext, localAck, rollback;
    uint32_t inputSent, inputAckSent, inputAt, hashAt;
    uint8_t rollbackPending, inputPacketSent, hashesSent;
    ModernInputSlot inputs[MODERN_HISTORY];
    ModernHash hashes[MODERN_HASH_SLOTS];
    uint32_t packetsSent, packetsReceived, rejected, corrections, checks, mismatches;
} ModernPeer;

typedef void (*ModernSend)(void* context, const uint8_t* packet, size_t size);

void ModernInit(ModernPeer* peer, unsigned role, unsigned region, uint64_t hostSession);
void ModernReceive(ModernPeer* peer, const uint8_t* packet, size_t size, uint32_t now);
void ModernPump(ModernPeer* peer, uint32_t now, ModernSend send, void* context);
int ModernConnected(const ModernPeer* peer);
int ModernQueueControl(ModernPeer* peer, unsigned kind, uint32_t id,
                       const void* data, size_t size, uint32_t* sequence);
int ModernControlAcked(const ModernPeer* peer, uint32_t sequence);
int ModernReadControl(ModernPeer* peer, ModernControl* output);
void ModernStartInputs(ModernPeer* peer, uint32_t round, uint32_t firstFrame);
int ModernInput(ModernPeer* peer, uint32_t frame, uint16_t localKeys,
                uint16_t* player0, uint16_t* player1);
int ModernTakeRollback(ModernPeer* peer, uint32_t* firstFrame);
void ModernSetHash(ModernPeer* peer, uint32_t frame, uint32_t hash);

#endif
