/* Jafar's lava room in the wide view (Preliminar V2 of the boss arenas; approved by the user 24/10: on for everyone,
 * KHCOM_JAFAR_WIDE=0 turns it off for before/after tests).
 * BATTLE_SCENERY_AUDIT.md #4: walls and lava end in a box, black around it; the room showed in the Original view
 * because its pillars' block is a work RAM buffer (a native global in the PC build) and the wide view never took it.
 *
 * The room (gBosJfBgMap0/1, the pillars' buffer, gBosJfBgMap3: a 512x512 BG) is 48 x 48 tiles, map x 448..831
 * (wrapping) and y 160..543; the rest of the map is empty. It is built from short periods, so it goes on with them:
 *  - the dark band above (one tile) and the brick wall (2 tiles);
 *  - its two columns, 31 tiles apart, mirror images of each other: beside the room they alternate every 31 tiles;
 *  - the lava's shore (7 tiles) and the lava, a 7 x 6 tile pattern whose next 6 rows are the same shifted 2 tiles.
 * The three moving pillars (map x 48..215, y 328..511) are the fight's: they stay in the room; no source tile is ever
 * read from them. The genie (his own layer, gBosJfMajinMapBuffer) is one: only the copy nearest the picture.
 * Source tiles stay in the user's ROM; palette indices resolved live (the lava's palette cycle is kept). */
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosJfBgMap0[], gBosJfBgMap1[], gBosJfBgMap2[], gBosJfBgMap3[];
extern const uint8_t gBosJfBgTiles[];
extern uint8_t gBosJfMapBuffer[], gBosJfMajinMapBuffer[];
extern const uint16_t gDefaultBgMap[];

#define ROOM_X 448 /* room column 0 = map tile column 56 */
#define ROOM_COLS 48
#define ROW_TOP 20 /* map tile rows 20..67 (y 160..543) */
#define ROW_BOTTOM 63 /* rows 64..67 wrap to the map's top: drawn here (the same lava), the wide view wraps sideways only */
#define COLUMN_GAP 31

static int FloorDiv(int a, int b) {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static int Mod(int a, int b) {
    int m = a % b;
    return m < 0 ? m + b : m;
}

/* the room's tile at room column i (0..47), map row r, as the ROM has it */
static unsigned RoomEntry(int i, int r) {
    static const uint16_t* const maps[4] = { gBosJfBgMap0, gBosJfBgMap1, gBosJfBgMap2, gBosJfBgMap3 };
    int tx = (56 + i) & 63, ty = r & 63;
    return maps[(ty >> 5) * 2 + (tx >> 5)][(ty & 31) * 32 + (tx & 31)];
}

/* a period-7 row's tile for column i, read where no pillar ever stands (room columns 0..4, 12, 13) */
static int Seven(int i) {
    int r7 = Mod(i, 7);
    return r7 <= 4 ? r7 : r7 + 7;
}

static unsigned HallEntry(int i, int r) {
    int k = FloorDiv(i + 8, COLUMN_GAP), d = i - COLUMN_GAP * k, lo = 6, hi = 10;
    if (r < ROW_TOP)
        return RoomEntry(12, ROW_TOP); /* the plain dark band */
    if (r >= 53) { /* the lava */
        int k6 = FloorDiv(r - 57, 6);
        return RoomEntry(Seven(i - 2 * k6), r - 6 * k6);
    }
    if (r >= 45 && r <= 47) { /* the columns' shadowed foot reaches one tile further out */
        if (k & 1) hi = 11;
        else lo = 5;
    }
    if (d >= lo && d <= hi)
        return RoomEntry((k & 1) ? d + COLUMN_GAP : d, r); /* a column, alternating its mirror image */
    if (r >= 50)
        return RoomEntry(Seven(i), r); /* the shore */
    return RoomEntry(12 + (i & 1), r); /* the wall */
}

/* approved art: on unless KHCOM_JAFAR_WIDE=0 (read once) */
int PortJafarWide(void) {
    const char* v;
    PORT_ENV_ONCE(v, getenv, "KHCOM_JAFAR_WIDE");
    return v == NULL || atoi(v) != 0;
}

/* 1 the room, 2 the genie's layer, 0 neither */
int PortJafarLayer(const void* const* maps) {
    if (!maps)
        return 0;
    if (maps[0] == gBosJfBgMap0 && maps[1] == gBosJfBgMap1 && maps[2] == (const void*)gBosJfMapBuffer &&
        maps[3] == gBosJfBgMap3)
        return 1;
    if (maps[0] == gDefaultBgMap && maps[1] == gDefaultBgMap && maps[2] == gDefaultBgMap &&
        maps[3] == (const void*)gBosJfMajinMapBuffer)
        return 2;
    return 0;
}

/* the game's own map buffers that live outside the GBA space in the PC build (a world map all the same) */
int PortJafarMapBuffer(const void* block) {
    return block == (const void*)gBosJfMapBuffer || block == (const void*)gBosJfMajinMapBuffer;
}

/* read once: getenv on every pixel slowed the 32:9 view to a frame a second */
static int NoHall(void) {
    const char* v;
    PORT_ENV_ONCE(v, getenv, "KHCOM_NO_JAFAR_HALL");
    return v != NULL;
}

/* x, y: the layer's world position; cx, cy: the picture's middle (its copy of the room is the one continued) */
int PortJafarHallPixel(int x, int y, int cx, int cy, uint16_t* color) {
    int u = x - (ROOM_X + FloorDiv(cx - ROOM_X, 512) * 512), my = y - FloorDiv(cy - ROW_TOP * 8, 512) * 512;
    int i = FloorDiv(u, 8), r = FloorDiv(my, 8), px = u & 7, py = my & 7, v;
    unsigned e;
    if (i >= 0 && i < ROOM_COLS && r >= ROW_TOP && r <= ROW_BOTTOM)
        return 0; /* the room as the cartridge draws it, its pillars included */
    if (NoHall())
        return 0; /* isolated before/after harness */
    e = HallEntry(i, r);
    if (e & 0x400) px = 7 - px;
    if (e & 0x800) py = 7 - py;
    v = (gBosJfBgTiles[(e & 1023) * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15;
    *color = v ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[(e >> 12) * 16 + v] & 0x7FFF : 0x8000;
    return 1;
}

/* the genie's rows the game clears as he sinks into the lava (entry 0, an opaque tile): on the GBA they fall below the
   screen; beside or below the picture they are nothing */
int PortJafarGenieCleared(int x, int y) {
    int mx = x & 511, my = y & 511;
    if (mx < 256 || my < 256)
        return 0;
    return ((const uint16_t*)(const void*)gBosJfMajinMapBuffer)[((my - 256) >> 3) * 32 + ((mx - 256) >> 3)] == 0;
}
