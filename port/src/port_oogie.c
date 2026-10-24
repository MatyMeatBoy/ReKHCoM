/* Oogie Boogie's hall in the wide view (Preliminar V2 of the boss arenas, waits for the user's approval).
 * BATTLE_SCENERY_AUDIT.md: a narrow room (exactly the GBA picture's 240 pixels) with black at its sides, and again
 * beside itself at 32:9.
 *
 * The room (gBosBoogieBgMaps[1] over [0], the right column of a 512x512 BG) is x 256..495, y 192..471: 30 x 35 tiles,
 * built symmetrically. Read as one bay of a long hall, it goes on with its own tiles:
 *  - beside it, more bays: the same pillars, bricks, garlands of lights, pumpkins and stage, without what belongs to
 *    the fight (the teeth grate in the middle, the three counters whose glow goes out with each die broken: there the
 *    ledge and panel of the gap between two counters);
 *  - above, the pillars and the two plain brick rows go on up;
 *  - in front, the spooky green field beyond the path: the room's outer green columns (29 joins 0 as in the map) in
 *    blocks of 8 x 7 tiles, every other block mirrored and each band of blocks staggered;
 *  - below, the path: its white lines go on along their slopes (measured on rows 408..470), the black teeth of the
 *    red follow their line, the skeleton's forearm goes on down.
 * Source tiles stay in the user's ROM; palette indices resolved live (the game's fades are kept). */
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosBoogieBgMaps[]; /* [0]: the room's lower block, [1] (0x400 entries on): its upper block */
extern const uint8_t gBosBoogieBgTiles[];
extern const uint16_t gDefaultBgMap[];

#define ROOM_X 256 /* tile column 0 */
#define ROOM_COLS 30
#define ROW_TOP 24 /* tile rows 24..58 */
#define ROW_BOTTOM 58

static const uint8_t kGreen[8] = { 0, 1, 2, 3, 26, 27, 28, 29 };

static int FloorDiv(int a, int b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static int Mod(int a, int b) {
    int m = a % b;
    return m < 0 ? m + b : m;
}

static unsigned RoomEntry(int c, int r) {
    const uint16_t* block = r < 32 ? gBosBoogieBgMaps + 0x400 : gBosBoogieBgMaps;
    return block[(r & 31) * 32 + c];
}

static int TileIndex(unsigned entry, int x, int y) {
    int px = x & 7, py = y & 7, v;
    if (entry & 0x400) px = 7 - px;
    if (entry & 0x800) py = 7 - py;
    v = (gBosBoogieBgTiles[(entry & 1023) * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15;
    return v ? (int)(entry >> 12) * 16 + v : 0;
}

static int RoomIndex(int x, int y) {
    return TileIndex(RoomEntry((x - ROOM_X) >> 3, y >> 3), x, y);
}

/* the plain bricks of the room's rows 232 (odd tile rows) and 256 (even) */
static unsigned Brick(int c, int r) {
    return RoomEntry(2 + Mod(c - 2, 4), (r & 1) ? 29 : 32);
}

static unsigned Green(int c, int r) {
    int u = c < 15 ? c : c - ROOM_COLS, by = r > ROW_BOTTOM ? (r - 52) / 7 : 0;
    int rr = r > ROW_BOTTOM ? 52 + (r - 52) % 7 : r, bx, k;
    u += 4 * by;
    bx = FloorDiv(u, 8);
    k = Mod(u, 8);
    return (bx & 1) ? RoomEntry(kGreen[7 - k], rr) ^ 0x400 : RoomEntry(kGreen[k], rr);
}

/* the path's white lines (outer and inner edge, left and right; mirrored about x 375.5) */
static void Lines(int y, float* oL, float* iL, float* iR, float* oR) {
    *oL = 319.0f - 0.484f * (float)(y - 408);
    *iL = 330.0f - 0.339f * (float)(y - 408);
    *iR = 751.0f - *iL;
    *oR = 751.0f - *oL;
}

/* below the room inside its columns: -1 = the green field */
static int PathIndex(int x, int y) {
    float oL, iL, iR, oR, soL, siL, siR, soR, xs;
    int ys = 456 + Mod(y - 456, 16), sx;
    Lines(y, &oL, &iL, &iR, &oR);
    Lines(ys, &soL, &siL, &siR, &soR);
    if ((float)x < oL - 3.0f || (float)x > oR + 3.0f)
        return -1;
    if ((float)x < iL + 8.0f)
        xs = (float)x - ((float)x < (oL + iL) * 0.5f ? oL - soL : iL - siL);
    else if ((float)x > iR - 8.0f)
        xs = (float)x - ((float)x > (oR + iR) * 0.5f ? oR - soR : iR - siR);
    else if (x >= 360 && x < 392)
        return RoomIndex(x, 471); /* the skeleton's forearm goes on down */
    else
        return RoomIndex(346, 470); /* the path's plain red */
    sx = (int)(xs + 0.5f);
    sx = sx < ROOM_X ? ROOM_X : sx > ROOM_X + ROOM_COLS * 8 - 1 ? ROOM_X + ROOM_COLS * 8 - 1 : sx;
    return RoomIndex(sx, ys);
}

static int HallIndex(int x, int y) {
    int c = FloorDiv(x - ROOM_X, 8), r = FloorDiv(y, 8), cc = Mod(c, ROOM_COLS), bay = FloorDiv(c, ROOM_COLS);
    unsigned e;
    if (r > ROW_BOTTOM && bay == 0) {
        int v = PathIndex(x, y);
        if (v >= 0)
            return v;
        e = Green(c, r);
    } else if (r < ROW_TOP) {
        e = cc < 2 || cc >= 28 ? RoomEntry(cc, ROW_TOP) : Brick(cc, r); /* the pillars and the bricks go on up */
    } else if (r > ROW_BOTTOM || r >= 50) { /* from the rail's white band down: the green field */
        e = Green(c, r);
    } else if (r >= 35 && r <= 38 && cc >= 12 && cc <= 17) {
        e = Brick(cc, r); /* the teeth grate is the fight's own */
    } else if (r >= 44 && r <= 46) {
        e = RoomEntry(9 + (cc & 1), r); /* the counters too: the ledge and panel of the gap between two */
    } else {
        e = RoomEntry(cc, r);
    }
    return TileIndex(e, x, y);
}

int PortOogieLayer(const void* const* maps) {
    return maps && maps[0] == gDefaultBgMap && maps[1] == gBosBoogieBgMaps + 0x400 && maps[2] == gDefaultBgMap &&
           maps[3] == gBosBoogieBgMaps;
}

/* x, y: the layer's world position; cx, cy: the picture's middle (its copy of the room is the one continued) */
int PortOogieHallPixel(int x, int y, int cx, int cy, uint16_t* color) {
    int mx = x - FloorDiv(cx, 512) * 512, my = y - FloorDiv(cy, 512) * 512, index;
    if (mx >= ROOM_X && mx < ROOM_X + ROOM_COLS * 8 && my >= ROW_TOP * 8 && my < (ROW_BOTTOM + 1) * 8)
        return 0; /* the room as the cartridge draws it */
    index = HallIndex(mx, my);
    *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
    return 1;
}
