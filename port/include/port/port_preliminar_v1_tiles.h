#ifndef PORT_PRELIMINAR_V1_TILES_H
#define PORT_PRELIMINAR_V1_TILES_H
#include <stdint.h>
#include "port_scene_wide_tiles.h"

/* Every profile is attached to map content, including decompressed EU maps.
 * Source rectangles never contain copied ROM pixels or fixed RGB colours. */
typedef struct PortPreliminarRecipe {
    uint32_t hash;
    /* scene, layer, block dimensions, occupied dimensions, left/right
     * source bands, slopes, upper/lower rectangles, repeat, ready. */
    int16_t v[22];
} PortPreliminarRecipe;
static const PortPreliminarRecipe kPreliminarRecipes[] = {
#include "port_preliminar_v1_data.inc"
};

static inline const PortPreliminarRecipe* PortPreliminarFind(uint32_t hash, int bg, int w, int h) {
    unsigned i;
    for (i = 0; i < sizeof(kPreliminarRecipes)/sizeof(kPreliminarRecipes[0]); i++) {
        const PortPreliminarRecipe* p = &kPreliminarRecipes[i];
        if (p->hash == hash && p->v[1] == bg && p->v[2] == w && p->v[3] == h && p->v[21]) return p;
    }
    return 0;
}

static inline int PortPreliminarTileSource(const PortPreliminarRecipe* p, int* x, int* y) {
    const int16_t* v = p->v;
    int ox=*x, oy=*y, sx=ox, sy=oy, w=v[4], h=v[5];
    if (v[0]==7 && (ox<0 || ox>=64 || oy<0 || oy>=64)) {
        if (v[1]==2) {
            if (ox>=64) {
                if (oy<12 || oy>=20) return 0;
                *x=57+PortSceneTileMod(ox-57,7); *y=oy; return 1;
            }
            if (ox<0) {
                *x=PortSceneTileMod(ox,6); *y=oy+(ox-*x)/2;
                return *y>=0 && *y<64;
            }
            return 0;
        }
        if (ox<0) {
            if (oy<0) { *x=4; *y=0; return 1; }
            /* The first house is cropped 32 px into a complete 104 px bay.
             * Align the intact bay at x=104, including its preceding gap. */
            if (oy>=0 && oy<16) { *x=9+PortSceneTileMod(ox-9,17); *y=oy; return 1; }
            if (2*oy+ox<56) { *x=4; *y=20; return 1; }
            sx=PortSceneTileMod(ox,12); sy=oy+(ox-sx)/2;
            if (sy>=0 && sy<64) { *x=sx; *y=sy; return 1; }
        }
    }
    if (v[0]==44) {
        int diagonal=2*oy+ox;
        if (v[1]==3 && oy<21 && (ox<11 || ox>=56 || oy<0)) {
            *x=11+PortSceneTileMod(ox-11,8); *y=oy<0?0:oy; return 1;
        }
        if (v[1]==3 && ((ox>=8 && ox<64 && oy>=64) ||
            (ox<8 && diagonal>=116) || (ox>=64 && oy>=48))) {
            /* Sunlight crosses the ground in the opposite diagonal from
             * the yellow facade. Continue that band independently. */
            int light=2*oy-ox;
            if (light<64 || light>114) {
                *x=48+PortSceneTileMod(ox-48,8); *y=48+PortSceneTileMod(oy-48,4); return 1;
            }
            sx=24+PortSceneTileMod(ox-24,4); sy=oy+(sx-ox)/2;
            while (sy>=64) { sx-=4; sy-=2; }
            if (2*sy+sx<124) { sx+=8; sy+=4; }
            *x=sx; *y=sy; return sy>=0 && sy<64;
        }
        if (ox<8) {
            if (v[1]==2) {
                if (diagonal>=68 && diagonal<80) sx=8+PortSceneTileMod(ox-8,4);
                else return 0;
                *x=sx; *y=oy+(ox-sx)/2;
                return *y>=0 && *y<64;
            }
            if (oy>=21 && diagonal<64) { *x=0; *y=22; return 1; }
            sx=12+PortSceneTileMod(ox-12,4); sy=oy+(ox-sx)/2;
            if (sy>=0 && sy<64) { *x=sx; *y=sy; return 1; }
            *x=PortSceneTileMod(ox,8); *y=56+PortSceneTileMod(sy-56,8); return 1;
        }
        if (ox>=64 && oy>=21 && oy<24 && v[1]==3) { *x=0; *y=22; return 1; }
    }
    if (v[0] == 97) {
        if (ox >= 8 && ox < 48 && oy >= 0 && oy < 40) return 1;
        if (!v[20]) return 0;
        *x=8+PortSceneTileMod(ox-8,40); *y=PortSceneTileMod(oy,40); return 1;
    }
    if (ox >= 0 && ox < w && oy >= 0 && oy < h) return 1;
    if (!v[20]) return 0; /* characters, furniture and unique landmarks */
    if (v[0]==112 && v[1]==2 && ox<0) return 0;
    if (ox < 0 || ox >= w) {
        int start=ox<0?v[6]:v[8], period=ox<0?v[7]:v[9];
        sx=start+PortSceneTileMod(ox-start,period);
        sy=oy+(ox<0?v[10]:v[11])*(ox-sx)/2;
    }
    if (sy >= 0 && sy < h) { *x=sx; *y=sy; return 1; }
    if (v[0]==53 && v[1]==2 && sy>=h) {
        *x=PortSceneTileMod(ox,8); *y=56+PortSceneTileMod(oy,8); return 1;
    }
    {
        int offset=sy<0?12:16;
        if (!v[offset+2] || !v[offset+3]) return 0;
        *x=v[offset]+PortSceneTileMod(ox-v[offset],v[offset+2]);
        *y=v[offset+1]+PortSceneTileMod(sy-v[offset+1],v[offset+3]);
        return 1;
    }
}

static inline int PortPreliminarPixelSource(const PortPreliminarRecipe* p, int wx, int wy, int* sx, int* sy) {
    if (p && p->v[0]==44 && p->v[1]==2 && wx<64 &&
        2*wy+wx>=114*8 && 2*wy+wx<124*8) {
        /* Clean curb at x=120..127, between the original flower boxes.
         * Alternating columns need a FOUR PIXEL vertical offset: retaining
         * the pixel phase is essential, rather than clipping half a tile. */
        *sx=120+PortSceneTileMod(wx-120,8); *sy=wy+(wx-*sx)/2;
        return *sy>=0 && *sy<512;
    }
    return 0;
}
#endif
