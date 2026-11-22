#ifndef PORT_SCENE057_TILES_H
#define PORT_SCENE057_TILES_H

#include "port_scene_wide_tiles.h"

/* Event 057: layout retained; wider continuation under review. Use original source maps
 * and live palette. Farther continuation separates the diagonal wall band from vertical padding;
 * do not repeat the unique tree/vines as a rectangular wallpaper.
 * Coordinates and dimensions are in 8x8 tiles, occupied map is 32x32.
 */
static inline int PortScene057TileSource(int bg, int* x, int* y) {
    if (bg != 2 && bg != 3)
        return 0;
    /* The unique vine reaches the top of the native map. Do not copy a
     * partial window/cap into the padding immediately above it: keep BG3's
     * complete wood planks visible. This sampler runs outside GBA only. */
    if (bg == 2 && *x >= 20 && *x < 28 && *y >= -8 && *y < 0) {
        *x = 0; *y = 0;
        return 1;
    }
    /* Counterpart to the authored left-hand window step. Keep the short
     * panel beside the vine, then raise its cap toward the right. These
     * strips share 2*y+x with their donors; no global slope is reversed.
     * ExtendedWorldEntry applies this only to the extra viewport pixels. */
    if (bg == 2 && *x >= 28 && *y >= -8 && *y < 8) {
        int phase=PortSceneTileMod(*x-28,12);
        int sx=phase<4 ? 8+phase : phase-4;
        int sy=*y+(*x-sx)/2;
        if (sy<0 || sy>=32 || (sx>=8 && sy<8)) { *x=0; *y=0; }
        else { *x=sx; *y=sy; }
        return 1;
    }
    if (*y < 0) {
        if (bg == 2 && 2 * *y + *x < 16) {
            /* Upper padding is clear: never repeat a partial window/vine. */
            *x = 0; *y = 0;
            return 1;
        }
        if (bg == 3 && 2 * *y + *x < 30) {
            /* The original plank module is eight rows, not four. */
            *x = PortSceneTileMod(*x, 8);
            *y = PortSceneTileMod(*y, 8);
            return 1;
        }
    }
    if (bg == 3 && *x >= 31 && *x < 35 && *y >= 5 && *y < 12) {
        /* The right-edge fragment matches this complete native tuft exactly. */
        *x -= 3; *y += 10;
        return 1;
    }
    if (bg == 3 && (*x < 0 || *x >= 32 || *y < 0 || *y >= 32)) {
        int sx=4+PortSceneTileMod(*x-4,8);
        int sy=*y+(*x-sx)/2;
        if (2 * *y + *x >= 32) {
            int lx=PortSceneTileMod(*x,16), ly=PortSceneTileMod(*y-23,12);
            int cx=*x-lx, cy=*y-ly;
            /* Keep each complete96x64 native tuft inside a128x96 cell.
             * Skip a tuft whose core would cross the diagonal wall edge;
             * the remaining pixels use a clean native floor16x16 module. */
            if (lx<12 && ly<8 && 2*cy+cx+6>=32) {
                sx=lx; sy=23+ly;
            } else {
                sx=PortSceneTileMod(*x,2); sy=30+PortSceneTileMod(*y,2);
            }
        }
        if (sy>=0 && sy<32) { *x=sx; *y=sy; return 1; }
    }
    if (*x < 0 || *x >= 32 || *y < 0 || *y >= 32)
        return PortSceneWideSource(57, bg, x, y);
    return 1;
}

#endif
