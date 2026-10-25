#ifndef PORT_AGRABAH_TILES_H
#define PORT_AGRABAH_TILES_H
#include "port_scene_wide_tiles.h"

/* Street shared by events 107/108. Coordinates are original 8x8 tiles.
 * Side donors contain no windows, pots, market stalls or building corners.
 * Keep the two independent diagonals: street NE/SW, roof fronts NW/SE. */
static inline int PortAgrabahTileSource(int bg, int* x, int* y, int topPreview) {
    int ox = *x, oy = *y, sx, sy;
    /* The Genie pans the shared 107/108 backdrop above its authored top.
     * Continue each existing pilaster in its own column, not as plain yellow
     * facade. Complete the one window cropped at the source top, using the
     * intact matching window eight columns to its left and four rows below.
     * This sampler is called only outside the native GBA viewport. */
    if (topPreview && oy < 0 && ox >= 0 && ox < 64) {
        if (bg == 2) {
            if ((ox >= 16 && ox < 20) || (ox >= 40 && ox < 44)) {
                *y = 4 + PortSceneTileMod(oy, 4); return 1;
            }
            if (ox >= 52 && ox < 56 && oy >= -4) {
                *x = ox - 8; *y = oy + 4; return 1;
            }
            return 0;
        }
        if (bg == 3 && ox >= 16 && ox < 20) {
            *y = 4 + PortSceneTileMod(oy, 4); return 1;
        }
    }
    if (ox >= 0 && ox < 64 && oy >= 0 && oy < 64) return 1;
    if (ox < 0) {
        sx = PortSceneTileMod(ox, 4);
        sy = oy + (ox - sx) / 2;
    } else if (ox >= 64) {
        sx = 60 + PortSceneTileMod(ox, 4);
        if (2 * oy + ox >= 118 && 2 * oy - ox < 7) {
            if (bg == 2) return 0;
            *x = sx; *y = 30 + PortSceneTileMod(oy, 2);
            return 1; /* open roof between two independently sloping curbs */
        }
        sy = oy + (2 * oy - ox >= 7 ? sx - ox : ox - sx) / 2;
    } else {
        sx = ox;
        sy = oy;
    }
    if (sy >= 0 && sy < 64) { *x = sx; *y = sy; return 1; }
    if (bg == 2) return 0;
    /* Plain upper facade or foreground roof, from their own texture bands. */
    *x = PortSceneTileMod(ox, 4);
    *y = sy < 0 ? 12 + PortSceneTileMod(oy, 4) : 60 + PortSceneTileMod(oy, 4);
    return 1;
}
/* Pixel phase matters on a 2:1 curb: a four-tile donor translation
 * changes y by sixteen pixels, not by a rounded whole source tile. */
static inline int PortAgrabahPixelSource(int bg, int wx, int wy, int* sx, int* sy) {
    if (wx >= 0 && wx < 512 && wy >= 0 && wy < 512) return 0;
    if (wx < 0) {
        *sx = PortSceneTileMod(wx, 32);
        *sy = wy + (wx - *sx) / 2;
    } else if (wx >= 512) {
        *sx = 480 + PortSceneTileMod(wx - 480, 32);
        if (2 * wy + wx >= 944 && 2 * wy - wx < 56) {
            if (bg == 2) { *sx = 0; *sy = 0; return 1; }
            *sy = 240 + PortSceneTileMod(wy, 16);
            return 1;
        }
        if (2 * wy - wx >= 56) {
            /* The rightmost donor cuts the foreground wall at y=512.
             * The complete parallel face at x=256..287 includes its foot. */
            *sx = 256 + PortSceneTileMod(wx - 256, 32);
            *sy = wy + (*sx - wx) / 2;
        } else *sy = wy + (wx - *sx) / 2;
    } else return 0; /* retain the top window and pilasters' layer recipe */
    if (*sy >= 0 && *sy < 512) return 1;
    if (bg == 2) { *sx = 0; *sy = 0; return 1; }
    *sx = PortSceneTileMod(wx, 32);
    *sy = *sy < 0 ? 96 + PortSceneTileMod(wy, 32) : 480 + PortSceneTileMod(wy, 32);
    return 1;
}
#endif
