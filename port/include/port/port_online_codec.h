#ifndef PORT_ONLINE_CODEC_H
#define PORT_ONLINE_CODEC_H
#include <stddef.h>
#include <stdint.h>
struct CharaLinkData;
struct Deck;
size_t ModernEncodeProfile(uint8_t*, size_t, const struct CharaLinkData*);
int ModernDecodeProfile(struct CharaLinkData*, const uint8_t*, size_t);
size_t ModernEncodeDeck(uint8_t*, size_t, const struct Deck*);
int ModernDecodeDeck(struct Deck*, const uint8_t*, size_t);
#endif
