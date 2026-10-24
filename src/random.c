/**
 * random.c
 * Random Number Generator
 */

#include "types.h"

static u32 sRandSeed;
static u32 sRandomState[4];

void SeedRand(u32 seed) {
    sRandSeed = seed;
}

u32 Rand() {
    sRandSeed = (sRandSeed * 0x41C64E6D + 12345) & 0x7FFF;
    return sRandSeed;
}

void SeedRandom(u32 seed) {
    SeedRand(seed);
    sRandomState[0] = Rand();
    sRandomState[1] = Rand();
    sRandomState[2] = Rand();
    sRandomState[3] = Rand();
}

u16 GetRandom() {
    u32 x;

    x = sRandomState[1];
    x <<= 1;

    if (sRandomState[0] & 0x80000000) {
        x++;
    }

    x <<= 1;

    if (sRandomState[0] & 0x40000000) {
        x++;
    }

    sRandomState[3] <<= 1;

    if (sRandomState[2] & 0x80000000) {
        sRandomState[3]++;
    }

    x ^= sRandomState[3];
    sRandomState[3] = sRandomState[2];
    sRandomState[2] = sRandomState[1];
    sRandomState[1] = sRandomState[0];
    sRandomState[0] = x;
    x &= 0x7FFF;
    return x;
}

#ifdef PLATFORM_PC
/* the random generators' state, for the network link's desync check (mode_vsbattle.c VsBattleStateHash) */
u32 PortRandomState(void) {
    return sRandSeed ^ sRandomState[0] ^ (sRandomState[1] << 7) ^ (sRandomState[2] << 13) ^ (sRandomState[3] << 21);
}
#endif
