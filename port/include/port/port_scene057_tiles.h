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
    if (bg == 3 && *x >= 0 && *x <= 4 && *y >= 18 && *y <= 22) {
        /* Complete the half-grass crossing into the original map, under BG2. */
        *x += 8;
        *y -= 4;
        return 1;
    }
    if (bg == 3 && *x >= 31 && *x <= 38 && *y >= 5 && *y <= 11) {
        /* Whole eight-column grass counterpart, including the old map edge. */
        *x -= 15;
        *y += 4;
        return 1;
    }
    if (bg == 3 && (*x < 0 || *x >= 32 || *y < 0 || *y >= 32)) {
        /* A fixed eight-column donor keeps the entire tuft together;
         * nearest-column selection switches donors halfway through it. */
        int sx = 4 + PortSceneTileMod(*x - 4, 8);
        int sy = *y + (*x - sx) / 2;
        if (sy >= 0 && sy < 32) {
            *x = sx; *y = sy;
            return 1;
        }
    }
    if (*x < 0 || *x >= 32 || *y < 0 || *y >= 32)
        return PortSceneWideSource(57, bg, x, y);
    return 1;
}

#endif
