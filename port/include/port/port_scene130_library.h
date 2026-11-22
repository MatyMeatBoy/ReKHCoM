#ifndef PORT_SCENE130_LIBRARY_H
#define PORT_SCENE130_LIBRARY_H
#include "port_scene_wide_tiles.h"

/* The library's source coordinates only. Bookcases translate by (64,+/-32)
 * and the complete carpet diamond repeats on (96,48)/(96,-48). No furniture
 * belongs to the donor rectangles. Authored pixels are retained by the caller. */
/* The 05/11 review circled this 476-pixel flat-green interruption of the
 * left baseboard. Only its empty BG2/flat-floor pixels may be resampled. */
static inline int PortScene130LibraryRemate(int x,int y) {
    return x>=24 && x<48 && y>=272 && y<300 &&
        (x>=32 || y>=280) && x+2*y<624;
}

static inline int PortScene130LibrarySource(int x, int y, int* sx, int* sy) {
    int distance = x < 240 ? 240-x : x-240;
    if (PortScene130LibraryRemate(x,y)) {
        *sx=x+64; *sy=y-32; return 0; /* complete original baseboard band */
    }
    /* Continue the same full baseboard below the repaired notch. It occupies
     * diagonal592..623, although the carpet starts already at592. Resolve
     * this band before carpet so the left extension keeps its ornament. */
    if (x<24 && 2*y+x>=592 && 2*y+x<624) {
        *sx=64+PortSceneTileMod(x-64,64);
        *sy=y+(x-*sx)/2;
        return 0;
    }
    /* The matching right wall has its own diagonal,112..143. Keep its
     * complete native ornament ahead of the carpet here too. The caller
     * retains authored pixels, so original shelves and furniture stay put. */
    if (x>456 && 2*y-x>=112 && 2*y-x<144) {
        *sx=400+PortSceneTileMod(x-400,64);
        *sy=y-(x-*sx)/2;
        return 0;
    }
    if (y >= 176 + distance/2) {
        int u=PortSceneTileMod(x+2*y-832,192);
        int v=PortSceneTileMod(x-2*y+448,192);
        int d=u-v;
        *sx=192+(u+v)/2;
        *sy=320+(d-PortSceneTileMod(d,4))/4;
        return 1;
    } else {
        if (x<240) {
            *sx=48+PortSceneTileMod(x-48,64);
            *sy=y+(x-*sx)/2;
        } else {
            *sx=432+PortSceneTileMod(x-432,64);
            *sy=y-(x-*sx)/2;
        }
        if (*sy<56 || *sy>=512) { *sx=x<240?64:480; *sy=64; }
        return 0;
    }
}
#endif
