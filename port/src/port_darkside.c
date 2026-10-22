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
#include <math.h>
#include <stdlib.h>
#include <string.h>
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
    /* KHCOM_DARKSIDE_BEACH=1 (or the Preliminar V1 review copy) shows it until the user approves it; then on by
       default (=0 turns it off) */
    const char* v = getenv("KHCOM_DARKSIDE_BEACH");
    if (v != NULL)
        return atoi(v) != 0;
    return getenv("KHCOM_PRELIMINAR_V1") != NULL;
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

/* The other poses have no drawing: their right half is closed with the frame's own pixels, flipped (the rule for any
   cut edge, LOGICA_APROBADOS.md). A pose facing the camera (the eye near the cut) mirrors its left half over the
   body's axis; the axis is the one whose mirror best continues the columns at the cut. A pose in profile, and any row
   the mirror leaves cut, closes each cut run with the pixels just left of the cut, flipped, as a rounded edge (half
   an ellipse, 0.6 of the run's half height deep). The game's outline (index 1) where the new piece meets the
   outside. Read from VRAM as the game loaded it (some poses change the map and keep the previous frame's tiles), and
   rebuilt when the map or those tiles change. */
#define CLOSE_W 96
static const void* sCloseMap;
static uint32_t sCloseSum;
static int sCloseOn;
static unsigned char sFrame[256][256]; /* the frame's palette indices (bank * 16 + index, 0 transparent) */
static unsigned char sClose[256][CLOSE_W];

static void BuildClose(void) {
    static unsigned char edge[256][CLOSE_W];
    long ex = 0, n = 0;
    int x, y, j;
    for (y = 0; y < 256; y++)
        for (x = 0; x < 256; x++)
            if ((sFrame[y][x] & 15) == 12 || (sFrame[y][x] & 15) == 13) { /* the yellow eye */
                ex += x;
                n++;
            }
    memset(sClose, 0, sizeof(sClose));
    if (n && ex * 2 >= 400 * n) { /* the eye at x >= 200: facing the camera */
        int center = (int)((ex * 2 + n / 2) / n), best = -1, a, a2 = center;
        for (a = center - 24; a <= center + 24; a++) {
            int score = 0;
            for (j = 0; j < 4; j++) {
                int sx = a - 256 - j;
                if (sx < 0 || sx >= 256)
                    continue;
                for (y = 0; y < 256; y++)
                    score += ((sFrame[y][255 - j] != 0) == (sFrame[y][sx] != 0)) + (sFrame[y][255 - j] == sFrame[y][sx]);
            }
            if (score > best) {
                best = score;
                a2 = a;
            }
        }
        for (y = 0; y < 256; y++)
            for (j = 0; j < CLOSE_W; j++) {
                int sx = a2 - 256 - j;
                if (sx >= 0 && sx < 256)
                    sClose[y][j] = sFrame[y][sx];
            }
    }
    for (y = 0; y < 256;) {
        int y0 = y, len;
        if (!sFrame[y][255] || sClose[y][0]) {
            y++;
            continue;
        }
        while (y < 256 && sFrame[y][255] && !sClose[y][0])
            y++;
        len = y - y0;
        for (x = y0; x < y; x++) {
            double t = (x - y0 + 0.5) / len * 2 - 1, r = 1 - t * t;
            int depth = (int)(len / 2.0 * 0.6 * (r > 0 ? sqrt(r) : 0) + 0.5);
            for (j = 0; j < depth && j < CLOSE_W; j++)
                sClose[x][j] = sFrame[x][255 - j] ? sFrame[x][255 - j] : 1;
        }
    }
    /* the outline: new pixels with the outside next to them */
    for (y = 0; y < 256; y++)
        for (j = 0; j < CLOSE_W; j++) {
            int l, r, u, d;
            edge[y][j] = 0;
            if (!sClose[y][j])
                continue;
            l = j ? sClose[y][j - 1] != 0 : sFrame[y][255] != 0;
            r = j + 1 < CLOSE_W && sClose[y][j + 1];
            u = y > 0 && sClose[y - 1][j];
            d = y < 255 && sClose[y + 1][j];
            edge[y][j] = !(l && r && u && d);
        }
    for (y = 0; y < 256; y++)
        for (j = 0; j < CLOSE_W; j++)
            if (edge[y][j])
                sClose[y][j] = 1;
}

static const DarksidePose* FindPose(const uint16_t* frame) {
    size_t k;
    for (k = 0; k < sizeof(kPoses) / sizeof(kPoses[0]); k++)
        if (kPoses[k].map == frame)
            return &kPoses[k];
    return NULL;
}

void PortDarksideBeginFrame(int bg, const void* const* maps) {
    const uint16_t* frame = (const uint16_t*)maps[2];
    unsigned cnt = *(const uint16_t*)(uintptr_t)(GBA_IO + 8 + bg * 2);
    const uint8_t* chars = (const uint8_t*)(uintptr_t)GBA_VRAM + ((cnt >> 2) & 3) * 0x4000;
    uint32_t sum = 2166136261u;
    int i, x, y;
    sCloseOn = !FindPose(frame);
    if (!sCloseOn)
        return;
    for (i = 0; i < 0x4000; i += 4)
        sum = (sum ^ (chars[i] | chars[i + 1] << 8 | chars[i + 2] << 16 | (uint32_t)chars[i + 3] << 24)) * 16777619u;
    if (frame == sCloseMap && sum == sCloseSum)
        return;
    for (y = 0; y < 256; y++)
        for (x = 0; x < 256; x++) {
            unsigned entry = frame[(y >> 3) * 32 + (x >> 3)];
            int px = x & 7, py = y & 7, v;
            if (entry & 0x400) px = 7 - px;
            if (entry & 0x800) py = 7 - py;
            v = (chars[((entry & 1023) * 32 + py * 4 + px / 2) & 0x3FFF] >> ((px & 1) * 4)) & 15;
            sFrame[y][x] = (unsigned char)(v ? (entry >> 12) * 16 + v : 0);
        }
    BuildClose();
    sCloseMap = frame;
    sCloseSum = sum;
}

/* The block's own coordinates, in the copy of the map nearest the original picture (cx, cy: its middle, in
   pixels). 1: drawn here. */
int PortDarksideBodyPixel(const void* const* maps, int x, int y, int cx, int cy, uint16_t* color) {
    const uint16_t* frame = (const uint16_t*)maps[2];
    const DarksidePose* pose = FindPose(frame);
    const uint16_t* pltt = (const uint16_t*)(uintptr_t)GBA_PLTT;
    int bx = x & 511, by = (y & 511) - 256, v = 0;
    if (2 * (x - cx) < -512 || 2 * (x - cx) >= 512 || 2 * (y - cy) < -512 || 2 * (y - cy) >= 512)
        return 0;
    if (by < 0 || bx < (pose ? DARKSIDE_ART_LEFT : BEACH_W) || bx >= BEACH_W + CLOSE_W)
        return 0;
    /* every pixel of this strip is decided here: the caller's tile runs would carry a transparent pixel over the
       rest of its tile */
    if (!pose) {
        if (sCloseOn && frame == sCloseMap)
            v = sClose[by][bx - BEACH_W];
    } else if (bx < BEACH_W && (v = FramePixel(frame, pose->tiles, bx, by)) != 0) {
        v |= (frame[(by >> 3) * 32 + (bx >> 3)] >> 12) * 16; /* the cartridge's own pixel stays */
    } else {
        int mx = bx - DARKSIDE_ART_LEFT, my = by - pose->top;
        if (mx < DARKSIDE_ART_W && my >= 0 && my < pose->rows)
            v = (pose->art[my][mx >> 1] >> ((mx & 1) * 4)) & 15; /* the body's bank 0 */
    }
    *color = v ? pltt[v] & 0x7FFF : 0x8000; /* live */
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
