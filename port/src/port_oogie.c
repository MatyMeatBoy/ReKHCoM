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
 *  - in front, complete the missing ghost contours independently over two
 *    plain green carpet fields, with white outer edges and a wall into the abyss;
 *  - below, the path: its white lines go on along their slopes (measured on rows 408..470), the black teeth of the
 *    red follow their line, the carpet continues in plain red, with green flanks in perspective.
 * Source tiles stay in the user's ROM; palette indices resolved live (the game's fades are kept). */
#include <stdlib.h>
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosBoogieBgMaps[]; /* [0]: the room's lower block, [1] (0x400 entries on): its upper block */
extern const uint8_t gBosBoogieBgTiles[];
extern const uint16_t gDefaultBgMap[];

#define ROOM_X 256 /* tile column 0 */
#define ROOM_COLS 30
#define ROW_TOP 24 /* tile rows 24..58 */
#define ROW_BOTTOM 58


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
    else
        return RoomIndex(346, 470); /* the path's plain red */
    sx = (int)(xs + 0.5f);
    sx = sx < ROOM_X ? ROOM_X : sx > ROOM_X + ROOM_COLS * 8 - 1 ? ROOM_X + ROOM_COLS * 8 - 1 : sx;
    return RoomIndex(sx, ys);
}

/* Continue the two floor fields in perspective. The missing ghost contours
 * are drawn independently; no reflected source pixels or repeated motifs. */
static int sFloorGreen=-1, sGhostInk=-1;
static int FloorMotifIndex(int x, int y) {
    static const unsigned char leftReach[] ={0,4,9,11,16,13,18,10,6,0};
    static const unsigned char rightReach[]={0,8,12,16,14,20,17,12,5,0};
    int left=218, right=533, depth=y>472?y-472:0;
    int edge, k, reach, t, white=RoomIndex(319,408);
    left-=depth*3/4;right+=depth*3/4;
    if(x<left-5 || x>right+5)return 0;
    if(x<left || x>right)return RoomIndex(256,399);
    if(x<left+3 || x>right-3)return white;
    if(sFloorGreen<0)return 0;
    if(y>=404 && y<476 && (x<256 || x>=496)) {
        const unsigned char* shape=x<256?leftReach:rightReach;
        k=(y-404)/8;t=(y-404)%8;
        reach=(shape[k]*(8-t)+shape[k+1]*t)/8;
        edge=x<256?256-x:x-495;
        if(edge<=reach)return sGhostInk;
    }
    return sFloorGreen;
}

static int HallIndex(int x, int y) {
    int c = FloorDiv(x - ROOM_X, 8), r = FloorDiv(y, 8), cc = Mod(c, ROOM_COLS);
    unsigned e;
    if (r > ROW_BOTTOM) {
        int v = PathIndex(x, y);
        if (v >= 0)
            return v;
        return FloorMotifIndex(x, y);
    } else if (r < ROW_TOP) {
        e = cc < 2 || cc >= 28 ? RoomEntry(cc, ROW_TOP) : Brick(cc, r); /* the pillars and the bricks go on up */
    } else if (r > ROW_BOTTOM || r >= 50) { /* from the rail's white band down: the green field */
        return FloorMotifIndex(x, y);
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
    int active=maps && maps[0] == gDefaultBgMap && maps[1] == gBosBoogieBgMaps + 0x400 && maps[2] == gDefaultBgMap &&
           maps[3] == gBosBoogieBgMaps;
    if(active) {
    if(sFloorGreen<0) {
        int count[256]={0},a,b;
        for(b=408;b<472;b++) for(a=256;a<288;a++)count[RoomIndex(a,b)&255]++;
        sGhostInk=1;for(a=2;a<256;a++)if(count[a]>count[sGhostInk])sGhostInk=a;
        for(a=1;a<256;a++) {
            unsigned c=((const uint16_t*)(uintptr_t)GBA_PLTT)[a];
            int r=c&31,g=(c>>5)&31,b=(c>>10)&31;
            if(g>r && g>b && (sFloorGreen<0 || count[a]>count[sFloorGreen]))sFloorGreen=a;
        }
        /* Selection happens before the parallel scanline jobs. */
    }
    }
    return active;
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
