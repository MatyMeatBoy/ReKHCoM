/* Maleficent's dragon arena in the wide view (Preliminar V2 of the boss arenas, waits for the user's approval).
 * BATTLE_SCENERY_AUDIT.md #5: the hall ends in cuts and the dragon's body shows again beside the picture.
 *
 * The hall (gBosMdBgMap0..3, a 512x512 BG) is drawn once, x 64..447 and y 120..511 (its stone curb goes on in the
 * map's top 16 rows); the rest of the map is empty. Its bays repeat every 72 pixels (an arch, a pillar with its
 * gargoyle), so beyond the drawn part each row goes on with a whole bay of itself; above it the plain night sky, below
 * it the stone curb. The dragon (gBosMdFrame*Map, the same BG's last block) is one creature: outside the picture only
 * the copy nearest the picture is drawn. Source tiles stay in the user's ROM; palette indices resolved live. */
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosMdBgMap0[], gBosMdBgMap1[], gBosMdBgMap2[], gBosMdBgMap3[];
extern const uint8_t gBosMdBgTiles[];
extern const uint16_t gDefaultBgMap[];

#define HALL_LEFT 64
#define HALL_RIGHT 448
#define HALL_TOP 120
#define BAY 72
#define CURB_TOP 496 /* the curb's 32 rows: 496..511 and the map's 0..15 */

static int Repeat(int position, int start, int width) {
    int offset = (position - start) % width;
    return start + (offset < 0 ? offset + width : offset);
}

static int FloorDiv(int a, int b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static int SourceIndex(int x, int y) {
    static const uint16_t* const maps[4] = { gBosMdBgMap0, gBosMdBgMap1, gBosMdBgMap2, gBosMdBgMap3 };
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
    px = (gBosMdBgTiles[(entry & 1023) * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15;
    return px ? (int)(entry >> 12) * 16 + px : 0;
}

/* 1 the hall, 2 the dragon, 0 neither */
int PortMaleficentLayer(const void* const* maps) {
    if (!maps)
        return 0;
    if (maps[0] == gBosMdBgMap0 && maps[1] == gBosMdBgMap1 && maps[2] == gBosMdBgMap2 && maps[3] == gBosMdBgMap3)
        return 1;
    return 0;
}

/* the dragon's layer: the three empty blocks and one of its frames (called only while the hall is on screen) */
int PortMaleficentDragonLayer(const void* const* maps) {
    return maps && maps[0] == gDefaultBgMap && maps[1] == gDefaultBgMap && maps[2] == gDefaultBgMap &&
           maps[3] != gDefaultBgMap;
}

/* x, y: the layer's world position; cx, cy: the world position of the picture's middle (its copy is the one drawn) */
int PortMaleficentHallPixel(int x, int y, int cx, int cy, uint16_t* color) {
    int mx = x - FloorDiv(cx, 512) * 512, my = y - FloorDiv(cy, 512) * 512, sx, sy, index;
    int inX = mx >= HALL_LEFT && mx < HALL_RIGHT, inY = my >= HALL_TOP && my < 512;
    if (inX && inY)
        return 0; /* the hall as the cartridge draws it */
    sx = inX ? mx : mx < HALL_LEFT ? Repeat(mx, HALL_LEFT + 12, BAY) : Repeat(mx, HALL_RIGHT - 12 - BAY, BAY);
    if (my < HALL_TOP)
        sy = HALL_TOP + 8; /* the plain sky */
    else if (my >= CURB_TOP)
        sy = CURB_TOP + Repeat(my, CURB_TOP, 32) - CURB_TOP; /* the curb, its 32 rows again (the last 16 wrap to 0) */
    else
        sy = my;
    index = SourceIndex(sx, sy);
    *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
    return 1;
}

/* the dragon: only its copy nearest the picture (no second dragon beside it) */
int PortMaleficentDragonVisible(int x, int y, int cx, int cy) {
    return FloorDiv(x - cx + 256, 512) == 0 && FloorDiv(y - cy + 256, 512) == 0;
}
