/* Trickmaster's room (Wonderland's boss, issue: the board ends above the wide view's bottom and leaves a black band).
 * The user's rule (18/10, BATTLE_SCENERY_AUDIT.md #8): keep the room as it is and only continue the chessboard
 * downwards, with its perspective and alternation. Source tiles stay in the user's ROM; nothing is shipped.
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

int PortTrickmasterBackdropPixel(int x, int y, uint16_t* color) {
    float depth, width;
    int row, column, index;
    if (y < BOARD_END)
        return 0; /* the room and the board as the cartridge draws them */
    if (sIndex[0] < 0) {
        float w = COLUMN_SLOPE * (BOARD_END - 4 - VANISH_Y);
        sIndex[0] = SourceIndex(128, BOARD_END - 4);
        sIndex[1] = SourceIndex(128 + (int)w, BOARD_END - 4);
    }
    x &= 255; /* the stage repeats sideways every 256 pixels, as the map above it does */
    depth = 1.0f / ((float)y - VANISH_Y);
    row = (int)((1.0f / ((float)BOARD_END - VANISH_Y) - depth) / ROW_STEP); /* 0: the first row below the map */
    width = COLUMN_SLOPE * ((float)y - VANISH_Y);
    column = (int)floorf(((float)x - 128.0f + width * 0.5f) / width);
    /* row 0 alternates with the last authored row: on the centre column it takes the other square */
    index = sIndex[((row + column + 1) & 1)];
    *color = index ? ((const uint16_t*)(uintptr_t)GBA_PLTT)[index] & 0x7FFF : 0x8000;
    return 1;
}
