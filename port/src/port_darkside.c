/* Darkside's beach (battle 198, Sora and Riku): the wide view continues the authored 256x512 beach with its own
 * bands, and Darkside himself stays one. port/progress/BATTLE_SCENERY_TECHNIQUE.md is the method.
 *
 * The beach (BG0, gBosDsdMapBlocks: only the left column of the 512x512 map is drawn) is built as bands of a base
 * pattern with unique pieces painted over it: night sky (one tile), sea (8 tiles), dark water and sand (4 tiles a
 * row). Outside the drawn column each tile row repeats its base pattern, in the authored phase, so the palm, the
 * floating island, the driftwood and the rocks are never copied. The shoreline row has no pattern (every tile is
 * different): it continues pixel column by pixel column from its clean stretches, chaining columns that match.
 * Above the map the sky goes on, below it the sand (it wrapped to the black sky before).
 *
 * Darkside's body (BG1, one 256x256 frame of gBosDsdFrameMaps in the lower left block) is not repeated: outside
 * that block the layer is empty (it wrapped into a second and third Darkside at 32:9).
 *
 * Identified by the map blocks themselves, never by battle number or mode. Source tiles stay in the user's ROM;
 * colours come from the live palette, so the game's fades and flashes apply. */
#include <stdlib.h>
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosDsdBgMap0[], gBosDsdBgMap2[], gDefaultBgMap[];
extern const uint8_t gBosDsdBgTiles[];
extern const uint16_t* gBosDsdFrameMaps[47][4];

#define BEACH_W 256 /* the drawn column of the map */
#define BEACH_H 512
#define SHORE_RUN 2048 /* columns chained on each side before the walk repeats */

static signed char sPeriod[64]; /* tile period of each row's base pattern; -1: the shoreline */
static unsigned char sStart[64]; /* the first column of that pattern */
static int sShoreRow = -1;
static short sShoreRight[SHORE_RUN], sShoreLeft[SHORE_RUN];
static int sPrepared;

int PortDarksideBeachOn(void) {
    /* KHCOM_DARKSIDE_BEACH=1 shows it until the user approves it; then on by default (=0 turns it off) */
    const char* v;
    PORT_ENV_ONCE(v, getenv, "KHCOM_DARKSIDE_BEACH");
    return v != NULL && atoi(v) != 0;
}

static unsigned Entry(int tx, int ty) {
    return ty < 32 ? gBosDsdBgMap0[ty * 32 + tx] : gBosDsdBgMap2[(ty - 32) * 32 + tx];
}

static int Repeat(int position, int start, int width) {
    int offset = (position - start) % width;
    return start + (offset < 0 ? offset + width : offset);
}

static int SourceIndex(int x, int y) {
    unsigned entry = Entry(x >> 3, y >> 3);
    int px = x & 7, py = y & 7, index;
    unsigned offset;
    if (entry & 0x400) px = 7 - px;
    if (entry & 0x800) py = 7 - py;
    offset = (entry & 1023) * 32 + py * 4 + px / 2;
    if (offset >= 32768) return 0;
    index = (gBosDsdBgTiles[offset] >> ((px & 1) * 4)) & 15;
    return index ? (int)(entry >> 12) * 16 + index : 0;
}

/* How alike two pixel columns of the shoreline row are (equal palette indices). */
static int ColumnLike(int a, int b) {
    int y, same = 0;
    for (y = 0; y < 8; y++)
        same += SourceIndex(a, sShoreRow * 8 + y) == SourceIndex(b, sShoreRow * 8 + y);
    return same;
}

/* The base pattern of a row: the period (1, 2, 4 or 8 tiles) whose most frequent aligned group fills at least half
   of the row; failing that, the 4-tile group made of the row's most frequent tiles. */
static void PrepareRow(int ty) {
    static const int periods[] = { 1, 2, 4, 8 };
    unsigned e[32];
    int i, j, k, p, unique = 0;
    for (i = 0; i < 32; i++)
        e[i] = Entry(i, ty);
    for (i = 0; i < 32; i++) {
        for (j = 0; j < i && e[j] != e[i]; j++)
            ;
        unique += j == i;
    }
    if (unique == 32) {
        sPeriod[ty] = -1;
        return;
    }
    for (k = 0; k < 4; k++) {
        p = periods[k];
        for (i = 0; i < 32; i += p) {
            int count = 0, g;
            for (g = 0; g < 32; g += p) {
                for (j = 0; j < p && e[g + j] == e[i + j]; j++)
                    ;
                count += j == p;
            }
            if (count * p >= 16) {
                sPeriod[ty] = (signed char)p;
                sStart[ty] = (unsigned char)i;
                return;
            }
        }
    }
    {
        int best = -1, bestAt = 0;
        for (i = 0; i < 32; i += 4) {
            int score = 0;
            for (j = 0; j < 4; j++)
                for (k = 0; k < 32; k++)
                    score += e[k] == e[i + j];
            if (score > best) {
                best = score;
                bestAt = i;
            }
        }
        sPeriod[ty] = 4;
        sStart[ty] = (unsigned char)bestAt;
    }
}

/* A shoreline tile is clean when the dark water above it and the sand below it are their rows' base pattern:
   no palm, driftwood or rock reaches into it. */
static int ShoreClean(int x) {
    int tx = x >> 3, r;
    if (x < 0 || x >= BEACH_W)
        return 0;
    for (r = sShoreRow - 1; r <= sShoreRow + 1; r += 2) {
        int p = sPeriod[r], s = sStart[r];
        if (p <= 0 || Entry(tx, r) != Entry(Repeat(tx, s, p), r))
            return 0;
    }
    return 1;
}

/* Chains clean columns outward from the edge column: on along the current stretch; at its end, on from the clean
   column most like the current one (away from it, so the wave does not echo), in either direction. */
static void Walk(short* out, int edge, int dir) {
    int c = edge, d = dir, i;
    for (i = 0; i < SHORE_RUN; i++) {
        if (!ShoreClean(c + d) || (i > 0 && i % 97 == 0)) {
            int best = -1, bestC = c, bestD = d, cand, cd;
            for (cand = 0; cand < BEACH_W; cand++) {
                if (!ShoreClean(cand) || abs(cand - c) < 24)
                    continue;
                for (cd = -1; cd <= 1; cd += 2) {
                    int score;
                    if (!ShoreClean(cand + cd))
                        continue;
                    score = ColumnLike(cand, c) * 4 + ColumnLike(cand + cd, c + d < 0 || c + d >= BEACH_W ? c : c + d);
                    if (score > best) {
                        best = score;
                        bestC = cand;
                        bestD = cd;
                    }
                }
            }
            if (best < 0) { /* no clean stretch elsewhere: turn back */
                d = -d;
            } else {
                c = bestC - bestD; /* the step below lands on bestC */
                d = bestD;
            }
        }
        c += d;
        if (c < 0 || c >= BEACH_W)
            c = edge;
        out[i] = (short)c;
    }
}

static void Prepare(void) {
    int ty;
    if (sPrepared)
        return;
    for (ty = 0; ty < 64; ty++) {
        PrepareRow(ty);
        if (sPeriod[ty] < 0 && sShoreRow < 0)
            sShoreRow = ty;
    }
    if (sShoreRow > 0 && sShoreRow < 63) {
        Walk(sShoreRight, BEACH_W - 1, 1);
        Walk(sShoreLeft, 0, -1);
    }
    sPrepared = 1;
}

int PortDarksideLayer(const void* const* maps) {
    if (!maps || !PortDarksideBeachOn())
        return 0;
    if (maps[0] == gBosDsdBgMap0 && maps[1] == gDefaultBgMap && maps[2] == gBosDsdBgMap2 && maps[3] == gDefaultBgMap)
        return 1;
    if ((const void*)maps >= (const void*)gBosDsdFrameMaps[0] && (const void*)maps < (const void*)gBosDsdFrameMaps[47] &&
        maps[0] == gDefaultBgMap && maps[1] == gDefaultBgMap && maps[3] == gDefaultBgMap)
        return 2;
    return 0;
}

/* Darkside's right half, drawn for the wide view where the cartridge cut each body frame at its block's edge (x 256);
   left of it the arms and fists the frame leaves out, only where the frame's own pixel is transparent. One drawing a
   pose, generated by port/tools/gen_darkside_body.py from a reference the user approved. */
#include "port/darkside_body_idle.inc"
#define DARKSIDE_ART_LEFT 232
#define DARKSIDE_ART_W 72

extern const uint16_t gBosDsdFrame0Map[], gBosDsdFrame1Map[], gBosDsdFrame2Map[], gBosDsdFrame3Map[],
    gBosDsdFrame4Map[];
extern const uint8_t gBosDsdFrame1Tiles[], gBosDsdFrame2Tiles[], gBosDsdFrame3Tiles[], gBosDsdFrame4Tiles[];

typedef struct DarksidePose {
    const uint16_t* map;     /* the body frame (gBosDsdFrameMaps' third block) */
    const uint8_t* tiles;    /* its tiles, to keep every pixel the cartridge has */
    const unsigned char (*art)[DARKSIDE_ART_W / 2];
    int top, rows;           /* the drawing's rows in the frame's block */
} DarksidePose;

static const DarksidePose kPoses[] = {
    { gBosDsdFrame0Map, gBosDsdBgTiles, kDarkside_idle, DARKSIDE_IDLE_TOP, DARKSIDE_IDLE_ROWS },
    { gBosDsdFrame1Map, gBosDsdFrame1Tiles, kDarkside_idle, DARKSIDE_IDLE_TOP, DARKSIDE_IDLE_ROWS },
    { gBosDsdFrame2Map, gBosDsdFrame2Tiles, kDarkside_idle, DARKSIDE_IDLE_TOP, DARKSIDE_IDLE_ROWS },
    { gBosDsdFrame3Map, gBosDsdFrame3Tiles, kDarkside_idle, DARKSIDE_IDLE_TOP, DARKSIDE_IDLE_ROWS },
    { gBosDsdFrame4Map, gBosDsdFrame4Tiles, kDarkside_idle, DARKSIDE_IDLE_TOP, DARKSIDE_IDLE_ROWS },
};

/* The body frame's own pixel (palette index) in its block, x < 256. */
static int FramePixel(const uint16_t* frame, const uint8_t* tiles, int x, int y) {
    unsigned entry = frame[(y >> 3) * 32 + (x >> 3)];
    int px = x & 7, py = y & 7;
    if (entry & 0x400) px = 7 - px;
    if (entry & 0x800) py = 7 - py;
    return (tiles[(entry & 1023) * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15;
}

/* The block's own coordinates, in the copy of the map nearest the original picture (cx, cy: its middle, in
   pixels). 1: drawn here. */
int PortDarksideBodyPixel(const void* const* maps, int x, int y, int cx, int cy, uint16_t* color) {
    const uint16_t* frame = (const uint16_t*)maps[2];
    const DarksidePose* pose = NULL;
    int mx, my, v;
    size_t k;
    for (k = 0; k < sizeof(kPoses) / sizeof(kPoses[0]); k++)
        if (kPoses[k].map == frame)
            pose = &kPoses[k];
    if (!pose)
        return 0;
    if (2 * (x - cx) < -512 || 2 * (x - cx) >= 512 || 2 * (y - cy) < -512 || 2 * (y - cy) >= 512)
        return 0;
    mx = (x & 511) - DARKSIDE_ART_LEFT;
    my = (y & 511) - 256 - pose->top;
    if (mx < 0 || mx >= DARKSIDE_ART_W || my < 0 || my >= pose->rows)
        return 0;
    v = (pose->art[my][mx >> 1] >> ((mx & 1) * 4)) & 15;
    if (!v || ((x & 511) < BEACH_W && FramePixel(frame, pose->tiles, x & 511, (y & 511) - 256)))
        return 0; /* the cartridge's own pixel stays */
    *color = ((const uint16_t*)(uintptr_t)GBA_PLTT)[v] & 0x7FFF; /* the body's bank 0, live */
    return 1;
}

int PortDarksideTileSource(int layer, int* tx, int* ty, int cx, int cy) {
    int x = *tx, y = *ty;
    if (layer == 2) {
        /* the body: once, in its own block, in the copy of the map nearest the original picture (the event before
           Riku's fight scrolls him across the map's edge, where the GBA wraps him around) */
        int mx = x & 63, my = y & 63;
        if (2 * (x - cx) < -64 || 2 * (x - cx) >= 64 || 2 * (y - cy) < -64 || 2 * (y - cy) >= 64)
            return 0;
        *tx = mx;
        *ty = my;
        return mx < 32 && my >= 32;
    }
    Prepare();
    if (y < 0)
        y = 0;
    else if (y >= 64)
        y = 62 + ((y - 64) & 1); /* the sand's two alternating rows */
    if (x < 0 || x >= 32) {
        if (sPeriod[y] > 0)
            x = Repeat(x, sStart[y], sPeriod[y]);
        else
            x = x < 0 ? 0 : 31; /* the shoreline: drawn by PortDarksideShorePixel */
    }
    *tx = x;
    *ty = y;
    return 1;
}

int PortDarksideShorePixel(int x, int y, uint16_t* color) {
    int sx, index, i;
    Prepare();
    if (sShoreRow < 0 || (y >> 3) != sShoreRow || (x >= 0 && x < BEACH_W))
        return 0;
    i = x >= BEACH_W ? x - BEACH_W : -1 - x;
    i %= SHORE_RUN;
    sx = x >= BEACH_W ? sShoreRight[i] : sShoreLeft[i];
    index = SourceIndex(sx, y);
    *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
    return 1;
}
