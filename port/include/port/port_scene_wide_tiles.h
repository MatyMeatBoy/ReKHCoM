#ifndef PORT_SCENE_WIDE_TILES_H
#define PORT_SCENE_WIDE_TILES_H

/* Source coordinates for the wider inspection, independent of viewport size.
 * All returned entries belong to the original occupied 32x32 map. */
static inline int PortSceneTileMod(int value, int period) {
    int r = value % period;
    return r < 0 ? r + period : r;
}

static inline int PortSceneWideSource(int event, int bg, int* x, int* y) {
    int ox = *x, oy = *y, sx, sy, period, score, best = 100000;
    int found = 0, bx = 0, by = 0;
    if (event == 74 && (ox < 0 || oy < 0)) return 0;
    if (event == 74 && bg == 2 && ox >= 35) {
        int repeat = (ox - 27) / 20;
        sx = 7 + (ox - 27) % 20;
        sy = oy - repeat * 10 - 4;
        if (repeat >= 1 && sx <= 14 && sy >= 18 && sy <= 21) {
            *x = sx; *y = sy; return 1;
        }
        if (2 * oy - ox >= 2) { *x = 0; *y = 0; return 1; }
    }
    period = event == 57 ? 8 : (event == 61 && bg == 2 ? 12 : 4);
    for (sx = PortSceneTileMod(ox, period); sx < 32; sx += period) {
        sy = oy + (event == 57 ? (ox - sx) / 2 : (sx - ox) / 2);
        if (sy < 0 || sy >= 32) continue;
        if (event == 74) score = -sx;
        else {
            int center = event == 57 ? 12 : 16;
            score = sx > center ? sx - center : center - sx;
            if (event == 57 && bg == 2) score = sx;
        }
        if (score < best) { found = 1; best = score; bx = sx; by = sy; }
    }
    if (found) { *x = bx; *y = by; return 1; }
    if (event == 74) return 0;
    if (event == 57) {
        if (bg == 2) { *x = 0; *y = 0; }
        else if (2 * oy + ox < 30) {
            *x = PortSceneTileMod(ox,8); *y = PortSceneTileMod(oy,4);
        } else {
            *x = 8 + PortSceneTileMod(ox,8); *y = 28 + PortSceneTileMod(oy,4);
        }
    } else if (2 * oy - ox > 32) {
        *x = PortSceneTileMod(ox,4); *y = 31;
    } else if (bg == 3) {
        *x = 24 + PortSceneTileMod(ox,4); *y = 0;
    } else {
        *x = 20 + PortSceneTileMod(ox,12);
        sy = oy + (*x - ox) / 2;
        *y = 2 + PortSceneTileMod(sy-2,8);
    }
    return 1;
}
#endif
