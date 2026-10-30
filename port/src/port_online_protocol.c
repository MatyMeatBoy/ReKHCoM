#include "port/port_online_protocol.h"
#include <string.h>

#define HEADER 24
#define TAIL 32

static uint16_t Read16(const uint8_t* p) { return (uint16_t)(p[0] | (uint16_t)p[1]<<8); }
static uint32_t Read32(const uint8_t* p) { return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static uint64_t Read64(const uint8_t* p) { return Read32(p) | (uint64_t)Read32(p+4)<<32; }
static void Write16(uint8_t* p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void Write32(uint8_t* p, uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8)); }
static void Write64(uint8_t* p, uint64_t v) { Write32(p,(uint32_t)v); Write32(p+4,(uint32_t)(v>>32)); }

void ModernInit(ModernPeer* p, unsigned role, unsigned region, uint64_t session) {
    memset(p,0,sizeof(*p)); p->role=(uint8_t)role; p->region=(uint8_t)region;
    p->session=role==0?session:0; p->txNext=p->rxNext=1;
    if(role>1 || region>2 || (role==0 && !session))p->error=MODERN_ERR_STATE;
}

int ModernConnected(const ModernPeer* p) { return p->ready && p->peerReady && !p->error; }

static void DrainControls(ModernPeer* p) {
    while(p->rxCount<MODERN_CONTROLS) {
        ModernControl* c=&p->pending[(p->rxNext-1)%MODERN_CONTROLS];
        if(!c->valid || c->sequence!=p->rxNext)break;
        p->rx[(p->rxRead+p->rxCount)%MODERN_CONTROLS]=*c;
        c->valid=0; p->rxCount++; p->rxNext++; p->ackDirty=1;
    }
}

int ModernQueueControl(ModernPeer* p, unsigned kind, uint32_t id,
                       const void* data, size_t size, uint32_t* sequence) {
    ModernControl* c;
    if(p->error || !kind || kind>255 || size>MODERN_CONTROL_MAX || (size&&!data) || p->txCount==MODERN_CONTROLS)return 0;
    c=&p->tx[(p->txNext-1)%MODERN_CONTROLS];
    if(c->valid)return 0;
    memset(c,0,sizeof(*c)); c->valid=1; c->sequence=p->txNext++; c->kind=(uint8_t)kind;
    c->id=id; c->size=(uint16_t)size; if(size)memcpy(c->data,data,size);
    if(sequence)*sequence=c->sequence;
    p->txCount++; return 1;
}

int ModernControlAcked(const ModernPeer* p, uint32_t sequence) { return (int32_t)(p->txAck-sequence)>=0; }

int ModernReadControl(ModernPeer* p, ModernControl* output) {
    if(!p->rxCount)return 0;
    *output=p->rx[p->rxRead]; p->rxRead=(p->rxRead+1)%MODERN_CONTROLS; p->rxCount--;
    DrainControls(p); return 1;
}

static void AckControls(ModernPeer* p, uint32_t ack) {
    if((int32_t)(ack-p->txAck)<=0 || (int32_t)(p->txNext-ack)<=0)return;
    p->txAck=ack;
    for(unsigned i=0;i<MODERN_CONTROLS;i++) {
        ModernControl* c=&p->tx[i];
        if(c->valid && (int32_t)(ack-c->sequence)>=0) { c->valid=0; p->txCount--; }
    }
}

void ModernStartInputs(ModernPeer* p, uint32_t round, uint32_t first) {
    memset(p->inputs,0,sizeof(p->inputs)); memset(p->hashes,0,sizeof(p->hashes));
    p->round=round; p->base=p->made=p->remoteNext=p->localAck=first;
    p->rollbackPending=p->inputPacketSent=p->hashesSent=0; p->inputsStarted=1;
    p->corrections=p->checks=p->mismatches=0;
}

static ModernInputSlot* Slot(ModernPeer* p, uint32_t frame) {
    ModernInputSlot* s=&p->inputs[frame%MODERN_HISTORY];
    if(!s->valid || s->frame!=frame) { memset(s,0,sizeof(*s)); s->valid=1; s->frame=frame; }
    return s;
}

int ModernInput(ModernPeer* p, uint32_t frame, uint16_t local, uint16_t* a, uint16_t* b) {
    ModernInputSlot* s;
    if(!ModernConnected(p) || !p->inputsStarted)return 0;
    if(frame==p->made) {
        if(p->made-p->localAck>=MODERN_HISTORY ||
           ((int32_t)(p->made-p->remoteNext)>=0 && p->made-p->remoteNext>=MODERN_HISTORY)) {
            p->error=MODERN_ERR_HISTORY; return 0;
        }
        s=Slot(p,frame); s->local=local&0x3ff; s->localValid=1; p->made++;
    } else {
        if((int32_t)(frame-p->made)>=0 || p->made-frame>MODERN_HISTORY)return 0;
        s=&p->inputs[frame%MODERN_HISTORY];
        if(!s->valid || s->frame!=frame || !s->localValid) { p->error=MODERN_ERR_HISTORY; return 0; }
    }
    if(s->remoteValid)s->usedRemote=s->remote;
    else {
        const ModernInputSlot* prior=&p->inputs[(frame-1)%MODERN_HISTORY];
        s->usedRemote=prior->valid && prior->frame==frame-1 ?
            (prior->remoteValid?prior->remote:(prior->usedValid?prior->usedRemote:0)):0;
    }
    s->usedValid=1;
    *a=p->role?s->usedRemote:s->local; *b=p->role?s->local:s->usedRemote;
    return 1;
}

int ModernTakeRollback(ModernPeer* p, uint32_t* frame) {
    if(!p->rollbackPending)return 0;
    *frame=p->rollback; p->rollbackPending=0; return 1;
}

static void RemoteInput(ModernPeer* p, uint32_t frame, uint16_t keys) {
    ModernInputSlot* s;
    /* A peer cannot allocate history with an arbitrary future frame. */
    if((int32_t)(frame-p->made)>=MODERN_HISTORY/2 ||
       ((int32_t)(frame-p->made)<0 && p->made-frame>MODERN_HISTORY))return;
    s=&p->inputs[frame%MODERN_HISTORY];
    if(s->valid && s->frame!=frame && s->localValid && p->made-s->frame<=MODERN_HISTORY) {
        p->rejected++; return;
    }
    s=Slot(p,frame);
    if(s->remoteValid) {
        if(s->remote!=keys)p->error=MODERN_ERR_CONFLICT;
        return;
    }
    s->remote=keys; s->remoteValid=1;
    if(s->usedValid && s->usedRemote!=keys) {
        if(!p->rollbackPending || (int32_t)(frame-p->rollback)<0)p->rollback=frame;
        p->rollbackPending=1; p->corrections++;
    }
}

static int Stable(const ModernPeer* p, uint32_t frame) {
    return (int32_t)(p->remoteNext-frame)>=0 && (int32_t)(p->localAck-frame)>=0;
}

static ModernHash* HashSlot(ModernPeer* p, uint32_t frame) {
    ModernHash* h=&p->hashes[(frame/120)%MODERN_HASH_SLOTS];
    if(h->valid && h->frame!=frame && (int32_t)(frame-h->frame)<0)return 0;
    if(!h->valid || h->frame!=frame) { memset(h,0,sizeof(*h)); h->valid=1; h->frame=frame; }
    return h;
}

static void CompareHashes(ModernPeer* p) {
    for(unsigned i=0;i<MODERN_HASH_SLOTS;i++) {
        ModernHash* h=&p->hashes[i];
        if(h->valid && h->localValid && h->remoteValid && !h->compared && Stable(p,h->frame)) {
            h->compared=1; p->checks++;
            if(h->local!=h->remote) { p->mismatches++; p->error=MODERN_ERR_DESYNC; }
        }
    }
}

void ModernSetHash(ModernPeer* p, uint32_t frame, uint32_t hash) {
    ModernHash* h=HashSlot(p,frame); if(!h)return;
    if(h->localValid && h->sent && h->local!=hash) { p->error=MODERN_ERR_STATE; return; }
    h->local=hash; h->localValid=1; CompareHashes(p);
}

static int PayloadValid(unsigned type, const uint8_t* d, size_t size) {
    if(type==MODERN_HELLO)return size==12;
    if(type==MODERN_ACK)return size==0;
    if(type==MODERN_HASH)return size==12;
    if(type==MODERN_CONTROL)return size>=7 && d[0] && Read16(d+5)<=MODERN_CONTROL_MAX && size==7u+Read16(d+5);
    if(type==MODERN_INPUT) {
        size_t at=13;
        if(size<13 || d[12]>2)return 0;
        for(unsigned n=0;n<d[12];n++) {
            unsigned count;
            if(at+5>size)return 0;
            count=d[at+4]; at+=5;
            if(count>TAIL || at+count*2>size)return 0;
            for(unsigned i=0;i<count;i++)if(Read16(d+at+i*2)&~0x3ff)return 0;
            at+=count*2;
        }
        return at==size;
    }
    return 0;
}

void ModernReceive(ModernPeer* p, const uint8_t* data, size_t size, uint32_t now) {
    unsigned type; const uint8_t* d; size_t len; uint64_t session;
    if(size<HEADER || size>MODERN_PACKET_MAX || !data || memcmp(data,"KHM2",4)) { p->rejected++; return; }
    if(data[4]!=MODERN_PROTOCOL) { p->error=MODERN_ERR_VERSION; return; }
    if(data[6]>1 || data[6]==p->role) { p->rejected++; return; }
    if(data[7]!=p->region) { p->error=MODERN_ERR_REGION; return; }
    type=data[5]; d=data+HEADER; len=size-HEADER; session=Read64(data+8);
    if(!PayloadValid(type,d,len)) { p->rejected++; return; }
    if(type==MODERN_HELLO) {
        if(p->role==1 && !p->session && session)p->session=session;
        if(session!=p->session && !(p->role==0 && !session && !p->peerReady)) { p->rejected++; return; }
        if(session && session==p->session) {
            p->ready=1; if(d[8])p->peerReady=1;
        }
        p->lastPeerTick=Read32(d);
        if(Read32(d+4)) {
            uint32_t rtt=now-Read32(d+4);
            if(rtt<10000) {
                if(!p->rtt || rtt<p->rtt)p->rtt=rtt?rtt:1;
                p->peerClockOffset=(int32_t)(Read32(d)-(now-p->rtt/2));
            }
        }
    } else if(!p->session || session!=p->session || !p->ready) { p->rejected++; return; }
    p->lastReceive=now; p->packetsReceived++; AckControls(p,Read32(data+20));
    if(type==MODERN_CONTROL) {
        uint32_t seq=Read32(data+16);
        p->ackDirty=1;
        if((int32_t)(seq-p->rxNext)<0 || seq-p->rxNext>=MODERN_CONTROLS)return;
        ModernControl* c=&p->pending[(seq-1)%MODERN_CONTROLS];
        if(!c->valid) {
            memset(c,0,sizeof(*c)); c->sequence=seq; c->kind=d[0]; c->id=Read32(d+1); c->size=Read16(d+5);
            memcpy(c->data,d+7,c->size); c->valid=1;
        } else if(c->sequence!=seq || c->kind!=d[0] || c->id!=Read32(d+1) ||
                  c->size!=Read16(d+5) || memcmp(c->data,d+7,c->size))p->error=MODERN_ERR_CONFLICT;
        DrainControls(p);
    } else if(type==MODERN_INPUT && p->inputsStarted && Read32(d)==p->round) {
        uint32_t ack=Read32(d+4); size_t at=13;
        if((int32_t)(ack-p->localAck)>0 && (int32_t)(p->made-ack)>=0)p->localAck=ack;
        for(unsigned n=0;n<d[12];n++) {
            uint32_t first=Read32(d+at); unsigned count=d[at+4]; at+=5;
            for(unsigned i=0;i<count;i++)RemoteInput(p,first+i,Read16(d+at+i*2));
            at+=count*2;
        }
        for(unsigned i=0;i<MODERN_HISTORY;i++) {
            ModernInputSlot* s=&p->inputs[p->remoteNext%MODERN_HISTORY];
            if(!s->valid || s->frame!=p->remoteNext || !s->remoteValid)break;
            p->remoteNext++;
        }
        CompareHashes(p);
    } else if(type==MODERN_HASH && p->inputsStarted && Read32(d)==p->round) {
        uint32_t f=Read32(d+4); ModernHash* h;
        if((int32_t)(f-p->made)>0)return;
        h=HashSlot(p,f); if(!h)return;
        if(h->remoteValid && h->remote!=Read32(d+8)) { p->error=MODERN_ERR_CONFLICT; return; }
        h->remote=Read32(d+8); h->remoteValid=1; CompareHashes(p);
    }
}

static void Send(ModernPeer* p, unsigned type, uint32_t seq, const uint8_t* payload,
                 size_t size, ModernSend send, void* context) {
    uint8_t data[MODERN_PACKET_MAX];
    if(size>MODERN_PACKET_MAX-HEADER)return;
    memcpy(data,"KHM2",4); data[4]=MODERN_PROTOCOL; data[5]=(uint8_t)type; data[6]=p->role; data[7]=p->region;
    Write64(data+8,p->session); Write32(data+16,seq); Write32(data+20,p->rxNext-1);
    if(size)memcpy(data+HEADER,payload,size);
    send(context,data,HEADER+size); p->packetsSent++; p->ackDirty=0;
}

static size_t InputRange(ModernPeer* p, uint8_t* data, uint32_t first, unsigned count) {
    Write32(data,first); data[4]=(uint8_t)count;
    for(unsigned i=0;i<count;i++)Write16(data+5+i*2,p->inputs[(first+i)%MODERN_HISTORY].local);
    return 5+count*2;
}

void ModernPump(ModernPeer* p, uint32_t now, ModernSend send, void* context) {
    uint8_t data[MODERN_CONTROL_MAX+16];
    if(p->error || !send)return;
    if(!p->helloSent || now-p->helloAt>=250) {
        memset(data,0,12); Write32(data,now); Write32(data+4,p->lastPeerTick); data[8]=p->ready;
        Send(p,MODERN_HELLO,0,data,12,send,context); p->helloAt=now; p->helloSent=1;
    }
    if(!ModernConnected(p))return;
    for(unsigned i=0, sent=0;i<MODERN_CONTROLS && sent<4;i++) {
        ModernControl* c=&p->tx[i];
        uint32_t rto=p->rtt>100?p->rtt*2:250;
        if(c->valid && (!c->sent || now-c->sentAt>=rto)) {
            data[0]=c->kind; Write32(data+1,c->id); Write16(data+5,c->size); memcpy(data+7,c->data,c->size);
            Send(p,MODERN_CONTROL,c->sequence,data,7+c->size,send,context); c->sent=1; c->sentAt=now; sent++;
        }
    }
    if(p->inputsStarted && (!p->inputPacketSent || p->inputSent!=p->made ||
       p->inputAckSent!=p->remoteNext || now-p->inputAt>=100)) {
        uint32_t n=p->made-p->base, first; unsigned count=n>TAIL?TAIL:n; size_t at=13;
        Write32(data,p->round); Write32(data+4,p->remoteNext); Write32(data+8,p->made); data[12]=0;
        first=p->made-count;
        if(count) { at+=InputRange(p,data+at,first,count); data[12]++; }
        if((int32_t)(first-p->localAck)>0) {
            unsigned repair=first-p->localAck; if(repair>TAIL)repair=TAIL;
            at+=InputRange(p,data+at,p->localAck,repair); data[12]++;
        }
        Send(p,MODERN_INPUT,0,data,at,send,context); p->inputPacketSent=1;
        p->inputSent=p->made; p->inputAckSent=p->remoteNext; p->inputAt=now;
    }
    if(p->inputsStarted && (!p->hashesSent || now-p->hashAt>=250)) {
        for(unsigned i=0;i<MODERN_HASH_SLOTS;i++) {
            ModernHash* h=&p->hashes[i];
            if(h->valid && h->localValid && Stable(p,h->frame)) {
                Write32(data,p->round); Write32(data+4,h->frame); Write32(data+8,h->local);
                Send(p,MODERN_HASH,0,data,12,send,context); h->sent=1;
            }
        }
        p->hashAt=now; p->hashesSent=1;
    }
    if(p->ackDirty)Send(p,MODERN_ACK,0,0,0,send,context);
}
