/* Trickmaster's room (Wonderland's boss, issue: the board ends above the wide view's bottom and leaves a black band).
 * The user's rule (Atlas 24/10): one fireplace, like the event room. Keep the original room, extend only clean
 * wallpaper sideways, and continue the chessboard with one perspective. Source tiles stay in the user's ROM.
 *
 * The board in gBosTmBgMap (256x256, the same map in all four blocks) is a perspective floor measured from the
 * cartridge's own pixels: every column widens linearly with y and meets the others at y = 33.3 (vanishing point),
 * a column centred on x = 128; the rows are evenly spaced in depth (1 / (y - 33.3) falls by 0.000564 a row) and the
 * next row starts exactly at the map's bottom (y = 256). Below it the board goes on with that geometry, its two
 * squares' palette indices read from the last authored row (resolved with the live palette: fades stay the game's). */
#include <math.h>
#include "port/port.h"
#include "port/port_battle_backdrop.h"

extern const uint16_t gBosTmBgMap[];
extern const uint8_t gBosTmBgTiles[];

#define VANISH_Y 33.3f
#define COLUMN_SLOPE 0.127f /* a square's width per pixel of depth below the vanishing point */
#define ROW_STEP 0.000564f  /* 1 / (y - VANISH_Y) between two row borders */
#define BOARD_END 256       /* the map's last row + 1: a row border */

static int sIndex[2] = { -1, -1 }; /* the square on the centre column of the last row, and its neighbour */

static int PositiveMod(int x, int n) {
    int r = x % n;
    return r < 0 ? r + n : r;
}

static int SourceIndex(int x, int y) {
    unsigned entry = gBosTmBgMap[((y & 255) >> 3) * 32 + ((x & 255) >> 3)];
    int px = x & 7, py = y & 7, index;
    if (entry & 0x400) px = 7 - px;
    if (entry & 0x800) py = 7 - py;
    index = (gBosTmBgTiles[(entry & 1023) * 32 + py * 4 + px / 2] >> ((px & 1) * 4)) & 15;
    return index ? (int)(entry >> 12) * 16 + index : 0;
}

int PortTrickmasterBackdropMatches(const void* const* maps) {
    return maps && maps[0] == gBosTmBgMap && maps[1] == gBosTmBgMap && maps[2] == gBosTmBgMap &&
           maps[3] == gBosTmBgMap;
}

int PortTrickmasterBackdropPixel(int x, int y, int cx, int cy, uint16_t* color) {
    float depth, width;
    int row, column, index;
    /* The hardware scroll can sit in any of the map's identical blocks.
     * Keep the one under the native viewport, rather than the first block. */
    x -= cx - PositiveMod(cx, 256);
    y -= cy - PositiveMod(cy, 256);
    if (y < 24) {
        int sx = PositiveMod(x, 56), sy = 24 + PositiveMod(y - 24, 48);
        /* Atlas: continue the pink wallpaper upwards, not the beige ceiling
         * strip. Keep the actual timber posts beside the chimney aligned
         * with their original x, excluding the horizontal rail at y=80.
         * The plain right panel avoids copying the clock and little window.
         * Keep only the existing chimney's stone shaft through it. */
        if ((x >= 56 && x < 120) || (x >= 184 && x < 208))
            sx = x;
        else if (x >= 144 && x < 184)
            sx = 72 + PositiveMod(x - 144, 40);
        /* Extend the top edge's uninterrupted timber columns. Repeating a
         * taller panel brings back the diagonal ends of the crosspieces. */
        if ((x >= 56 && x < 120) || (x >= 144 && x < 208))
            sy = 24;
        if (x >= 120 && x < 144) {
            sx = x;
            sy = 24 + PositiveMod(y - 24, 16);
        }
        index = SourceIndex(sx, sy);
        *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
        return 1;
    }
    if (y < BOARD_END) {
        int sx, sy;
        if (x >= 0 && x < 256 && y >= 0)
            return 0;
        /* Only the side wallpaper repeats, never the fireplace, lamp or vase.
         * The leftmost seven tile columns form a complete clean wall module. */
        sx = PositiveMod(x, 56);
        sy = y < 0 ? PositiveMod(y, 24) : y;
        /* Straight upper/lower silhouette of the long horizontal rail;
         * retain the authored grain between those two clean edge rows. */
        if (sy >= 108 && sy < 128) {
            if (sy < 112) { sx = 32; sy = 108; }
            else if (sy < 114 || sy >= 126) sx = 32;
        }
        if (sy >= 176) {
            /* Continue the authored checkerboard analytically below its wall;
             * all columns converge on the original room's vanishing point. */
            depth = (float)sy - VANISH_Y;
            width = COLUMN_SLOPE * depth;
            column = (int)floorf(((float)x - 128.0f + width * 0.5f) / width);
            /* Read the row's centre colour and alternate the two actual
             * floor inks. Sampling a moving donor near its diagonal edge
             * would introduce little teeth in every projected column. */
            index = SourceIndex(128, sy);
            if (column & 1) {
                int a = SourceIndex(128, BOARD_END - 4);
                int b = SourceIndex(156, BOARD_END - 4);
                index = index == a ? b : a;
            }
            *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
            return 1;
        }
        index = SourceIndex(sx, sy);
        *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
        return 1;
    }
    if (sIndex[0] < 0) {
        float w = COLUMN_SLOPE * (BOARD_END - 4 - VANISH_Y);
        sIndex[0] = SourceIndex(128, BOARD_END - 4);
        sIndex[1] = SourceIndex(128 + (int)w, BOARD_END - 4);
    }
    depth = 1.0f / ((float)y - VANISH_Y);
    row = (int)((1.0f / ((float)BOARD_END - VANISH_Y) - depth) / ROW_STEP); /* 0: the first row below the map */
    width = COLUMN_SLOPE * ((float)y - VANISH_Y);
    column = (int)floorf(((float)x - 128.0f + width * 0.5f) / width);
    /* row 0 alternates with the last authored row: on the centre column it takes the other square */
    index = sIndex[((row + column + 1) & 1)];
    *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
    return 1;
}
