/* Ursula's arena in the wide view (Preliminar V2 of the boss arenas, waits for the user's approval).
 * BATTLE_SCENERY_AUDIT.md #6: the floor ends above the wide view's bottom, and Ursula shows twice beside the picture.
 *
 * The cave (gBosUrsulaBgMap0..3, 512x512) already spans the whole map's width, so sideways it goes on by itself. Below
 * its floor the map has its own continuation in its first rows (the floor's spiky edge, then the red glow, rows 0..63
 * of the map); further down that glow stays. Ursula (her four body maps, one block of her own layer) is one: outside
 * the picture only the copy nearest the picture is drawn. Source tiles stay in the user's ROM. */
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosUrsulaBgMap0[], gBosUrsulaBgMap1[], gBosUrsulaBgMap2[], gBosUrsulaBgMap3[];
extern const uint16_t gBosUrsulaLeftMap[], gBosUrsulaRightMap[], gBosUrsulaHurtLeftMap[], gBosUrsulaHurtRightMap[];
extern const uint8_t gBosUrsulaBgTiles[];

#define BELOW_ROWS 64 /* the map's rows 0..63: what lies under the floor */

static int FloorDiv(int a, int b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static int SourceIndex(int x, int y) {
    static const uint16_t* const maps[4] = { gBosUrsulaBgMap0, gBosUrsulaBgMap1, gBosUrsulaBgMap2, gBosUrsulaBgMap3 };
    const uint16_t* block;
    unsigned entry;
    int px, py;
    x &= 511;
    y &= 511;
    block = maps[(y >> 8) * 2 + (x >> 8)];
    entry = block[((y & 255) >> 3) * 32 + ((x & 255) >> 3)];
    px = x & 7;
    py = y & 7;
    if (entry & 0x400) px = 7 - px;
    if (entry & 0x800) py = 7 - py;
    px = (gBosUrsulaBgTiles[(entry & 1023) * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15;
    return px ? (int)(entry >> 12) * 16 + px : 0;
}

/* 1 the cave, 2 Ursula's own layer, 0 neither */
int PortUrsulaLayer(const void* const* maps) {
    int i;
    if (!maps)
        return 0;
    if (maps[0] == gBosUrsulaBgMap0 && maps[1] == gBosUrsulaBgMap1 && maps[2] == gBosUrsulaBgMap2 &&
        maps[3] == gBosUrsulaBgMap3)
        return 1;
    for (i = 0; i < 4; i++)
        if (maps[i] == gBosUrsulaLeftMap || maps[i] == gBosUrsulaRightMap || maps[i] == gBosUrsulaHurtLeftMap ||
            maps[i] == gBosUrsulaHurtRightMap)
            return 2;
    return 0;
}

/* below the picture: what the map keeps under its floor; x, y the layer's world position, cy the picture's middle */
int PortUrsulaFloorPixel(int x, int y, int cy, uint16_t* color) {
    int my = y - FloorDiv(cy, 512) * 512, sy, index;
    if (my < 512)
        return 0; /* the cave as the cartridge draws it */
    sy = my - 512 < BELOW_ROWS ? my - 512 : BELOW_ROWS - 8; /* the spiky edge and the glow, then the glow */
    index = SourceIndex(x, sy);
    *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
    return 1;
}
