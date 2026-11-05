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
/* Pixel-phase continuation of the three foreground roofs. The last face
 * leaves the map below y512; its complete counterpart starts at (200,368),
 * hence 2*y-x==536. Switching donors earlier samples the street instead. */
static inline int PortAgrabahPixelSource(int bg, int wx, int wy, int* sx, int* sy) {
    int diagonal=2*wy-wx, donor;
    /* The last two-by-two BG2 tiles are transparent padding. Complete
     * their missing facade overlay as well; BG3 and authored ink stay put. */
    if (bg==2 && wx>=496 && wx<512 && wy>=496 && wy<512) {
        *sx=wx-32; *sy=wy-16; return 1;
    }
    if (wx<0 || (wx<512 && wy<512) || diagonal<56 || diagonal>=576) return 0;
    /* The same facade continues under the bottom/right corner. Painting
     * roof below y512 but wall right of x512 leaves a rectangular bite. */
    donor=diagonal<536 ? 448 : 256;
    *sx=donor+PortSceneTileMod(wx-donor,32);
    *sy=wy+(*sx-wx)/2;
    return 1;
}

/* Preserve the tile-run fast path unless any pixel in its next run needs
 * the facade resolver. The invariant decreases by one as X advances. */
static inline int PortAgrabahPixelRunNeeded(int bg, int wx, int wy) {
    int diagonal=2*wy-wx, run=8-PortSceneTileMod(wx,8);
    if (bg==2 && wx>=496 && wx<512 && wy>=496 && wy<512) return 1;
    return wx>=0 && (wx>=512 || wy>=512) && diagonal>=56 && diagonal-run+1<576;
}
#endif
