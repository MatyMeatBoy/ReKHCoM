#ifndef PORT_SCENE061_TILES_H
#define PORT_SCENE061_TILES_H

#include "port_scene_wide_tiles.h"

/* Scene 061: retained layout, wider diagonal correction under review (2026-10-18).
 * Coordinates and steps are in original 8x8 tiles. Call only for event 061,
 * on BG3 (floor) or BG2 (wall), including original coordinates: the completed
 * left floor ornament spans the old map edge. Do not pass the result
 * through the generic decoration skipping algorithm: this distribution was
 * approved with direct source-tile lookup. See ESCENA061_RECUPERADA.md.
 */
static inline int PortScene061TileSource(int bg, int width, int height, int* x, int* y) {
    static const int floorSteps[4][2] = {{4, 2}, {-4, 2}, {-4, 2}, {-4, -2}};
    static const int wallSteps[4][2] = {{12, 6}, {-12, -6}, {12, 6}, {-4, -2}};
    const int (*steps)[2];
    int i, edge, dx, dy;
    if ((bg != 2 && bg != 3) || width <= 0 || height <= 0)
        return 0;
    if (width == 32 && height == 32) {
        if (bg == 3 && *x >= -5 && *x <= 3 && *y >= 3 && *y <= 7) {
            *x += 12;
            *y += 6;
            return 1;
        }
        if (bg == 2 && *x >= -5 && *x <= 3 && *y >= 3 && *y <= 7) {
            /* Original transparent entry: keep the old extension ornament
             * from being drawn over the newly completed floor diamond. */
            *x = 0;
            *y = 3;
            return 1;
        }
        if (bg == 2 && *x >= 32 && *x <= 36 && *y == 5) {
            static const int sourceX[5] = {16, 17, 2, 3, 16};
            *x = sourceX[*x - 32];
            *y = 0;
            return 1;
        }
    }
    if (width == 32 && height == 32 && bg == 3 &&
        (*x < 0 || *x >= 32 || *y < 0 || *y >= 32)) {
        /* Keep the whole 16-column floor module together. Picking the
         * closest four-column donor independently breaks large ornaments
         * into unrelated checkerboard fragments at the extended corridor. */
        int sx = 4 + PortSceneTileMod(*x - 4, 16);
        int sy = *y + (sx - *x) / 2;
        if (sy >= 0 && sy < 32) {
            *x = sx;
            *y = sy;
            return 1;
        }
    }
    if (width == 32 && height == 32 &&
        (*x < 0 || *x >= 32 || *y < 0 || *y >= 32))
        return PortSceneWideSource(61, bg, x, y);
    steps = bg == 3 ? floorSteps : wallSteps;
    for (i = 0; i < 64 && (*x < 0 || *x >= width); ++i) {
        edge = *x < 0 ? 0 : 1;
        *x += steps[edge][0];
        *y += steps[edge][1];
    }
    for (i = 0; i < 64 && (*y < 0 || *y >= height); ++i) {
        edge = *y < 0 ? 2 : 3;
        dx = steps[edge][0];
        dy = steps[edge][1];
        if (*x + dx < 0 || *x + dx >= width)
            dx = -dx;
        *x += dx;
        *y += dy;
    }
    return *x >= 0 && *x < width && *y >= 0 && *y < height;
}

#endif
