#ifndef PORT_SCENE130_LIBRARY_H
#define PORT_SCENE130_LIBRARY_H
#include "port_scene_wide_tiles.h"

/* The library's source coordinates only. Bookcases translate by (64,+/-32)
 * and the complete carpet diamond repeats on (96,48)/(96,-48). No furniture
 * belongs to the donor rectangles. Authored pixels are retained by the caller. */
static inline int PortScene130LibrarySource(int x, int y, int* sx, int* sy) {
    int distance = x < 240 ? 240-x : x-240;
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
