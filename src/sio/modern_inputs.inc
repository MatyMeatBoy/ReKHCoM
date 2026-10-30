/* Included by chara.c only. These counters are game memory and rewind with
   snapshots; the heap-owned ModernPeer and immutable published keys do not. */
static u32 sModernFrame;
static u8 sModernBattle;

int PortRollbackReserve(void);
u32 VsBattleStateHash(void);

int ModernGameBeginInputs(u32 round) {
    ModernPeer* p = PortOnlinePeer();
    if (!p || !ModernConnected(p) || !PortRollbackReserve()) {
        PortOnlineFail(MODERN_ERR_STATE);
        return 0;
    }
    ModernStartInputs(p, round, 0);
    sModernFrame = 0;
    sModernBattle = 1;
    return 1;
}

void ModernGameEndInputs(void) { sModernBattle = 0; }
u32 ModernGameFrame(void) { return sModernFrame; }

int ModernGameRollbackSession(void) {
    return sModernBattle && !PortOnlineError();
}

u32 ModernGameTakeRollback(void) {
    u32 frame;
    ModernPeer* p = PortOnlinePeer();
    if (!p || !ModernTakeRollback(p, &frame)) return 0xffffffffu;
    return frame;
}

int ModernGameInputUpdate(void) {
    u16 a, b;
    u32 hash;
    ModernPeer* p;
    if (!sModernBattle) return 0;
    if (!PortRollbackResimulating()) {
        PortNetlinkFrame();
        PortOnlinePump();
    }
    p = PortOnlinePeer();
    if (!p || PortOnlineError()) return 0;
    if (sModernFrame % 120 == 0) {
        hash = VsBattleStateHash();
        if (hash) {
            if (PortNetlinkFakeDesync() && sModernFrame >= 1200) hash ^= 1;
            ModernSetHash(p, sModernFrame, hash);
        }
    }
    if (!ModernInput(p, sModernFrame, GetKeysHeld(), &a, &b)) return 0;
    SioKeyStateUpdateA(a);
    SioKeyStateUpdateB(b);
    gSioStatus &= ~SIO_STAT_RECV_EMPTY;
    sModernFrame++;
    if (PortRollbackResimulating() && sModernFrame == p->made)
        ModernFinishRollback(p);
    return 1;
}
