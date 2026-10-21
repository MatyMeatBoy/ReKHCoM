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
    if (v[0]==104 && v[1]==3) {
        /* Atlantica's Ursula chamber continues along a 32px/16px diagonal
         * rhythm. Sample the left four-tile rib/floor strip in its original
         * orientation so the wall and floor remain continuous and the
         * central mouth is never copied. Above the opaque source, use the
         * clean right-hand wall stripes; below it, continue the solid purple
         * backing instead of reflecting blank row 31 or inventing floor. */
        if (ox>=0 && ox<w && oy>=0 && oy<h-1) return 1;
        sx=PortSceneTileMod(ox,4);
        sy=oy-(ox-sx)/2;
        if (sy<0) {
            sx=40+PortSceneTileMod(ox,4);
            sy=0;
        } else if (sy>=h-1) {
            sx=0;
            sy=30;
        }
        if (sx<0 || sx>=w || sy<0 || sy>=h-1) return 0;
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==112 && v[1]==3 && !(ox>=0 && ox<w && oy>=0 && oy<h) &&
        ((ox>=w && 4*oy-ox>=16) || oy>=h)) {
        /* Continue the open floor beyond the right wall's diagonal and below
         * the room from a fully painted patch. The wall band remains visible
         * above that line; BG2's props stay unique. */
        *x=16+PortSceneTileMod(ox,8);
        *y=36+PortSceneTileMod(oy,8);
        return 1;
    }
    if (v[0]==45) {
        if (v[1]==2) {
            if (ox>=0 && ox<64 && oy>=0 && oy<64) return 1;
            if (oy>=22 && oy<27) { *x=10+PortSceneTileMod(ox-10,4); *y=oy; return 1; }
            return 0;
        }
        if (oy<0) { *x=24; *y=0; return 1; }
        if (oy<27 && (ox<10 || ox>=54)) {
            *x=oy<15?24:10+PortSceneTileMod(ox-10,12); *y=oy; return 1;
        }
        if (ox>=0 && ox<64 && oy<64) return 1;
        if (oy>=64) {
            sy=62+PortSceneTileMod(oy-62,2); sx=ox+oy-sy;
            if (sx>=0 && sx<64) { *x=sx; *y=sy; return 1; }
        }
        if (oy<43) { *x=0; *y=oy; return 1; }
        *x=56+PortSceneTileMod(ox,8); *y=56+PortSceneTileMod(oy,8); return 1;
    }
    if (v[0]==54) {
        /* Keep the island and pier unique. BG3's leftmost 128px is clear
         * sky/sea and safe as a water donor at horizontal map edges. */
        if (v[1]==2) return ox>=0 && ox<64 && oy>=0 && oy<64;
        if (ox>=0 && ox<64 && oy>=0 && oy<64) return 1;
        if (oy<0) {
            *x=PortSceneTileMod(ox,8); *y=PortSceneTileMod(oy,8); return 1;
        }
        if (oy>=64) {
            *x=PortSceneTileMod(ox,16); *y=48+PortSceneTileMod(oy-48,8); return 1;
        }
        /* Above the island, the full row carries the clouds and horizon;
         * below it, restrict donors to the eight-tile open-water margin. */
        *x=PortSceneTileMod(ox,oy<24?64:16); *y=oy; return 1;
    }
    if (v[0]==55) {
        /* The palm, island and debris live on BG2. Keep them unique; continue
         * BG3's cloud band at the sides and use its palette background as sea. */
        if (v[1]==2) return ox>=0 && ox<64 && oy>=0 && oy<64;
        if (oy<0) { *x=PortSceneTileMod(ox,64); *y=PortSceneTileMod(oy,4); return 1; }
        if (ox>=0 && ox<64 && oy>=0 && oy<32) return 1;
        if (oy>=0 && oy<8) { *x=PortSceneTileMod(ox,64); *y=oy; return 1; }
        return 0;
    }
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
    if (v[0]==87 && v[1]==3 &&
        (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* The chamber floor is repeatable, but BG3 also contains both room
         * walls. Extend only its clean interior swath; keep BG2's statues and
         * the authored perimeter strictly inside the original room. */
        int chunk=ox>=0?ox/w:(ox-(w-1))/w;
        int phase=PortSceneTileMod(chunk*3,8);
        *x=PortSceneTileMod(ox,w);
        *y=8+PortSceneTileMod(oy-8+phase,8);
        return 1;
    }
    if (v[0]==90 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* BG3 merges scenery with unique props. Continue skyline, wall and
         * floor as separate diagonal bands. The wall donor is the prop-free
         * stretch x8..23, avoiding the central gate and the graves; BG1
         * overlays remain unique. */
        if (v[1]!=3) return 0;
        {
            int diagonal=2*oy+ox;
            if (oy<24 || (ox<0 && diagonal<72)) {
                if (ox<0) {
                    int phase=PortSceneTileMod(-ox-1,52);
                    sx=phase<26?phase:51-phase;
                } else if (ox>=w) {
                    int phase=PortSceneTileMod(ox-w,16);
                    sx=56+(phase<8?7-phase:phase-8);
                }
                sy=oy<0?PortSceneTileMod(oy,8):oy;
                if (oy>=24) {
                    sy=(diagonal-sx)/2;
                    if (sy<0) sy=0;
                    if (sy>=24) sy=23;
                }
            } else if (diagonal>=84 && diagonal<104) {
                sx=8+PortSceneTileMod(ox-8,16);
                sy=(diagonal-sx)/2;
                if (sy<0 || sy>=h) return 0;
            } else if (diagonal>=104) {
                if (ox<0) sx=PortSceneTileMod(ox,16);
                else if (ox>=w) sx=48+PortSceneTileMod(ox-48,16);
                else sx=48+PortSceneTileMod(ox-48,16);
                sy=48+PortSceneTileMod((diagonal-sx)/2-48,16);
            } else {
                /* Clear night backdrop below the distant hills, before the
                 * diagonal masonry starts; this eight-tile donor has no fence. */
                sx=PortSceneTileMod(ox,8);
                sy=24+PortSceneTileMod(oy-24,8);
            }
        }
        if (sx<0 || sx>=w || sy<0 || sy>=h) return 0;
        *x=sx; *y=sy;
        return 1;
    }
    if (v[0]==95 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* BG3 merges the maze, throne and garden. Continue a throne-free
         * skyline, a short flower-hedge module, then open lawn. The red path,
         * throne and BG1 trees/arch/stairs/chairs remain unique. */
        if (v[1]!=3) return 0;
        if (oy<8) {
            *x=PortSceneTileMod(ox,64); *y=PortSceneTileMod(oy,6); return 1;
        }
        if (oy<12) {
            int phase=PortSceneTileMod(ox,64);
            *x=phase<32?phase:63-phase; *y=8+PortSceneTileMod(oy-8,4); return 1;
        }
        /* BG3 is transparent below y=44. Fill the distant park with a broad,
         * mirrored sample of its clean grass margin, never the maze or path. */
        {
            int phase=PortSceneTileMod(ox,32);
            *x=phase<16?48+phase:63-(phase-16);
            *y=40+PortSceneTileMod(oy-40,4);
        }
        return 1;
    }
    if (v[0]==98 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* Queen of Hearts room: its wallpaper and checker floor are one
         * isometric room separated by the back-wall diagonal. Repeat only
         * clean texture cells; BG1's table and BG3's fireplace/door stay put. */
        if (v[1]!=3) return 0;
        {
            int diagonal=oy<0?0:oy>=h?128:ox<0?2*oy+ox:2*oy-(ox-w);
            if (diagonal<80) {
                /* Wide clean wallpaper panel above the red cornice. */
                sx=32+PortSceneTileMod(ox-32,16);
                sy=PortSceneTileMod(oy,5);
            } else {
                sx=40+PortSceneTileMod(ox-40,8);
                sy=48+PortSceneTileMod(oy-48,8);
            }
        }
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==117 && v[1]==3 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* Captain Hook's treasure room BG3 has a large dark empty triangle
         * outside the clean planked center. Extend only an intact floor patch;
         * BG2's chests, coils and crates stay unique. */
        sx=8+PortSceneTileMod(ox-8,8);
        sy=20+PortSceneTileMod(oy-20,8);
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==116 && v[1]==3 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* Captain Hook's cabin is a single isometric room: continue only the
         * brick wall above its diagonal floor line and the plank floor below.
         * Sampling the whole map would repeat windows/props and create extra
         * perspective corners in ultrawide margins. BG2 remains unique. */
        if (2*oy+ox<32) {
            sx=PortSceneTileMod(ox,8);
            sy=4;
        } else {
            sx=8+PortSceneTileMod(ox-8,8);
            sy=22+PortSceneTileMod(oy-22,4);
        }
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==99 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* Queen's chamber: mirror only the side wallpaper modules and the
         * checker floor. The centered fireplace, lamp, arch and vase stay
         * unique; the clean panel supplies the unseen top/bottom rows. */
        if (v[1]!=3) return 0;
        if (ox<0 || ox>=w) {
            int distance=ox<0?-ox-1:ox-w;
            if (oy<3) {
                sx=PortSceneTileMod(ox,w);
            } else if (distance<8) {
                sx=ox<0?distance:w-1-distance;
            } else {
                /* Past one edge module, continue with the clean pink panel. */
                sx=11;
            }
        }
        if ((ox<0 || ox>=w) && oy>=16 && oy<21) {
            /* Continue clean wall trim without the candle motifs or vase. */
            sx=ox<0?4+PortSceneTileMod(-ox-1,3):29+PortSceneTileMod(ox-w,3);
        } else if ((ox<0 || ox>=w) && oy>=21) {
            /* Beyond the authored edges, continue the floor with solid
             * palette-matched checker cells; repeating the perspective map
             * would introduce extra vanishing points in the wide margins. */
            int distance=ox<0?-ox-1:ox-w;
            int cellWidth=oy<24?2:oy<26?3:oy<31?6:8;
            int baseDark;
            int dark;
            if (oy<h) baseDark=oy<24 || (oy>=29 && oy<31);
            else baseDark=PortSceneTileMod((oy-h)/2,2)!=0;
            dark=baseDark ^ ((distance/cellWidth)&1);
            if (dark) { sx=5; sy=31; }
            else if (oy>=24 && oy<26) { sx=5; sy=25; }
            else { sx=2; sy=31; }
            if (oy>=h) sy=h-1;
        }
        if (oy<0) {
            sy=PortSceneTileMod(oy,3);
        } else if (oy>=h && ox>=0 && ox<w) {
            /* Hold the nearest clean checker row past the room map. */
            sy=31;
        }
        *x=sx; *y=sy; return 1;
    }
    if (v[0] == 102) {
        /* The authored BG3 already contains its diagonal corridor. Do not
         * repeat that silhouette into the wide margins; the renderer fills
         * them from floor/water swatches outside the unchanged GBA viewport. */
        return ox >= 0 && ox < w && oy >= 0 && oy < h;
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
