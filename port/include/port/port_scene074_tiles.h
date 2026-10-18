#ifndef PORT_SCENE074_TILES_H
#define PORT_SCENE074_TILES_H

#include "port_scene_wide_tiles.h"

/* Event 074: retained layout and whole lower ornament; wider continuation under review.
 * Apply to BG3, BG2 AND BG1, with the current hardware palette/effects.
 * The declared 64x64 map has only one occupied 32x32 block: call this lookup
 * also for coordinates inside default blocks, not only outside the declaration.
 * The authored void inside the occupied block is part of the scene and stays.
 * A successful lookup returns ORIGINAL source coordinates, never mutated tiles.
 */
static inline int PortScene074TileSource(int bg, int* x, int* y) {
    if ((bg < 1 || bg > 3) || *x < 0 || *y < 0)
        return 0;
    if (bg == 2 && *x >= 32 && *x <= 34 && *y >= 17 && *y <= 21) {
        /* Nineteen existing entries match the original left counterpart.
         * Continue its tail instead of repeating its beginning diagonally. */
        *x -= 20;
        *y += 1;
        return 1;
    }
    if (*x >= 32 || *y >= 32)
        return PortSceneWideSource(74, bg, x, y);
    return 1;
}

#endif
