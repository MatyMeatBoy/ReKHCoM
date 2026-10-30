#include "port/port_online_codec.h"
#include "../../include/sio/chara_types.h"
#include "../../include/card/card_types.h"
#include <string.h>

static void W16(uint8_t* d, uint16_t v) { d[0]=(uint8_t)v; d[1]=(uint8_t)(v>>8); }
static uint16_t R16(const uint8_t* d) { return (uint16_t)(d[0]|(uint16_t)d[1]<<8); }
static void W64(uint8_t* d, uint64_t v) { for(unsigned i=0;i<8;i++)d[i]=(uint8_t)(v>>(8*i)); }
static uint64_t R64(const uint8_t* d) { uint64_t v=0;for(unsigned i=0;i<8;i++)v|=(uint64_t)d[i]<<(8*i);return v; }

static int ValidProfile(const CharaLinkData* p) {
    return p->level>0 && p->level<=99 && p->hp<=p->maxHp && p->maxHp>0;
}

size_t ModernEncodeProfile(uint8_t* d, size_t cap, const CharaLinkData* p) {
    if(!d || !p || cap<32 || !ValidProfile(p))return 0;
    W16(d,p->hp); W16(d+2,p->maxHp); W16(d+4,p->level);
    W16(d+6,p->winCount); W16(d+8,p->loseCount); W16(d+10,p->ap);
    W64(d+12,p->learnedStocks); W64(d+20,p->learnedStocks2);
    W16(d+28,p->worldFlags); W16(d+30,p->seed);
    return 32;
}

int ModernDecodeProfile(CharaLinkData* out, const uint8_t* d, size_t size) {
    CharaLinkData p={0};
    if(!out || !d || size!=32)return 0;
    p.hp=R16(d); p.maxHp=R16(d+2); p.level=R16(d+4);
    p.winCount=R16(d+6); p.loseCount=R16(d+8); p.ap=R16(d+10);
    p.learnedStocks=R64(d+12); p.learnedStocks2=R64(d+20);
    p.worldFlags=R16(d+28); p.seed=R16(d+30);
    if(!ValidProfile(&p))return 0;
    *out=p; return 1;
}

size_t ModernEncodeDeck(uint8_t* d, size_t cap, const Deck* p) {
    if(!d || !p || cap<224 || p->cardCount>DECK_SIZE)return 0;
    for(unsigned i=0;i<DECK_SIZE;i++)W16(d+i*2,p->cards[i]);
    memcpy(d+198,p->name,20); W16(d+218,p->cpCost); W16(d+220,p->cardCount); W16(d+222,p->unk_DE);
    return 224;
}

int ModernDecodeDeck(Deck* out, const uint8_t* d, size_t size) {
    Deck p={0};
    if(!out || !d || size!=224 || R16(d+220)>DECK_SIZE)return 0;
    for(unsigned i=0;i<DECK_SIZE;i++)p.cards[i]=R16(d+i*2);
    memcpy(p.name,d+198,20); p.cpCost=R16(d+218); p.cardCount=R16(d+220); p.unk_DE=R16(d+222);
    *out=p; return 1;
}
