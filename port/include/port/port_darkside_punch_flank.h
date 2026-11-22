#ifndef PORT_DARKSIDE_PUNCH_FLANK_H
#define PORT_DARKSIDE_PUNCH_FLANK_H
#include "darkside_body_punch_flank.inc"

/* Only this pending shadow follows the nine native body-frame seams.
 * The approved arm and every other pose keep their own existing placement. */
static inline int PortDarksidePunchFlankIndex(int slot, int x, int y) {
    int offset;
    if (slot==13) offset=0;
    else if (slot>=21 && slot<=28) offset=(slot-20<6 ? slot-20 : 6)*8;
    else return 0;
    x-=DARKSIDE_FLANK_X;
    y-=DARKSIDE_FLANK_TOP+offset;
    if (x<0 || x>=DARKSIDE_FLANK_WIDTH || y<0 || y>=DARKSIDE_FLANK_ROWS) return 0;
    return (kDarksideFlank[y][x>>1]>>((x&1)*4))&15;
}
#endif
