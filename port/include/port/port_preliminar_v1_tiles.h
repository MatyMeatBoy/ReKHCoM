#ifndef PORT_PRELIMINAR_V1_TILES_H
#define PORT_PRELIMINAR_V1_TILES_H
#include <stdint.h>
#include "port_scene_wide_tiles.h"
#include "port_scene053_plateau.inc"

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
    if (v[0]==116) {
        if (ox>=0 && ox<w && oy>=0 && oy<h) return 1;
        if (v[1]==2) {
            if (2*oy+ox>=60) {
                /* Front rail: a whole gold/lantern module, translated along
                 * the deck diagonal; never repeat it down the hull. */
                sx=16+PortSceneTileMod(ox-16,8);
                sy=oy+(ox-sx)/2;
            } else if (ox<8) {
                sx=PortSceneTileMod(ox,8); sy=oy+(ox-sx)/2;
            } else if (ox>=28) {
                /* The far wall turns behind the flat back wall: its right
                 * window module is eight rows above the left one's phase. */
                sx=PortSceneTileMod(ox,8); sy=oy+(ox-sx)/2-8;
            } else {
                return 0; /* no lamps/windows above the authored back wall */
            }
            if (sy<0 || sy>=32) return 0;
        } else {
            if (ox<8) {
                sx=PortSceneTileMod(ox,8); sy=oy+(ox-sx)/2;
            } else if (ox>=28) {
                sx=28+PortSceneTileMod(ox-28,4); sy=oy+(ox-sx)/2;
            }
            if (sy<0 && ox>=8 && ox<28) {
                sx=8+PortSceneTileMod(ox-8,8); sy=PortSceneTileMod(oy,2);
            } else if (sy<0 || sy>=32) {
                /* Clean diagonal red hull beyond either horizontal map end. */
                sx=24+PortSceneTileMod(ox-24,8);
                sy=24+PortSceneTileMod(oy+(ox-sx)/2-24,2);
            }
        }
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==115) {
        if (ox>=0 && ox<w && oy>=0 && oy<h) return 1;
        sx=PortSceneTileMod(ox,8);
        sy=oy+(ox-sx)/2;
        if (v[1]==3) {
            /* A plank has four rows, not the old two-row upper swatch.
             * Keep its diagonal phase and avoid copying wall pixels. */
            if (sy<0 || sy>=32) sy=20+PortSceneTileMod(sy-20,4);
        } else if (v[1]==2) {
            if (2*oy+ox>=32) {
                int cx=ox+4-PortSceneTileMod(ox+4,8);
                int base=22-cx/2;
                int cy=oy+1-PortSceneTileMod(oy+1-base,8);
                /* A candidate centred in an authored empty cell must not
                 * leave a half hatch outside that cell's map edge. */
                if (cx>=0 && cx<32 && cy>=0 && cy<32 &&
                    (cy>=29 || 2*cy+cx>=76)) return 0;
                /* Complete hatch diamonds on their staggered (16,-8),
                 * (0,8) lattice instead of wrapping clipped edge shards. */
                sx=8+PortSceneTileMod(ox-8,16);
                sy=16+PortSceneTileMod(oy+(ox-sx)/2-16,8);
            } else if (sy<0 || sy>=32) return 0;
        }
        *x=sx; *y=sy; return 1;
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
        if (ox>=54 && ox<64 && oy>=43 && oy<64) { /* the sunlit corner, shaded as the rest */
            *x=PortSceneTileMod(ox,8); *y=44+PortSceneTileMod(oy-44,4); return 1;
        }
        if (ox>=0 && ox<64 && oy<64) return 1;
        if (oy>=64) {
            sy=62+PortSceneTileMod(oy-62,2); sx=ox+oy-sy;
            if (sx>=0 && sx<64) { *x=sx; *y=sy; return 1; }
        }
        if (oy<43) { *x=0; *y=oy; return 1; }
        /* the dark green in the trees' shade, as inside (columns 0..7 under
         * the wall), not the sunlit corner's (user 24/10) */
        *x=PortSceneTileMod(ox,8); *y=44+PortSceneTileMod(oy-44,4); return 1;
    }
    if (v[0]==54) {
        /* Keep the island and pier unique. BG3's leftmost 128px is clear
         * sky/sea and safe as a water donor at horizontal map edges. */
        if (v[1]==2 && ox<0) {
            /* the pier goes on to the lower left along 2y+x, two posts
             * (eight columns) at a time (user 24/10) */
            sx=4+PortSceneTileMod(ox-4,8); sy=oy+(ox-sx)/2;
            if (sy<0 || sy>=64) return 0;
            *x=sx; *y=sy; return 1;
        }
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
    if (v[0]==4 && (ox<0 || ox>=64 || oy<0 || oy>=64)) {
        /* Alley with the crate carpet: only the walls and the plain floor
         * go on, along their own periods (wall 6 columns, shifting by
         * (6,-3) along its base; floor 8 columns by 4 rows). Props, carpet
         * and stains stay unique. */
        int diagonal=2*oy+ox;
        if (v[1]==2 && ox>=64) {
            /* The foreground right wall belongs to BG2. Its base runs
             * down-right, opposite to BG3's back wall. Continue the clean
             * last six columns, not the pillar's left edge or the crates. */
            sx=58+PortSceneTileMod(ox-58,6);
            sy=oy-(ox-sx)/2;
            *x=sx; *y=sy<0?0:sy;
            return *y<64;
        }
        if (v[1]!=3) return 0;
        if (ox>=64 && diagonal<97) {
            sx=58+PortSceneTileMod(ox-58,6); sy=oy+(ox-sx)/2;
            *x=sx; *y=sy<0?0:sy; return 1;
        }
        if (ox<0 && diagonal<72) {
            sx=6+PortSceneTileMod(ox-6,6); sy=oy-(sx-ox)/2;
            *x=sx; *y=sy<0?0:sy; return 1;
        }
        if (ox>=0 && ox<64 && oy<0) { *x=ox; *y=0; return 1; }
        *x=56+PortSceneTileMod(ox,8); *y=56+PortSceneTileMod(oy,4); return 1;
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
        if (v[1]==2) {
            /* the town behind (user 24/10): to the left its low houses
             * (columns 12..23), to the right the arcade (37..48) repeat;
             * the facade's top piece at 15..19 floated over the red wall */
            if (oy>=23 && oy<26 && ox>=12 && ox<20) return 0;
            if (oy>=16 && oy<23 && ox<12) { *x=12+PortSceneTileMod(ox-12,12); *y=oy; return 1; }
            if (oy>=16 && oy<24 && ox>=49) { *x=37+PortSceneTileMod(ox-37,12); *y=oy; return 1; }
        }
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
    if (v[0]==87 && v[1]==2 && (ox<0 || ox>=w)) {
        /* BG2 holds the stone step's lit edge (user 24/10 "falta seguir borde"):
         * right, the horizontal ledge (rows 15..17, columns 24..31) repeats by
         * its 2-tile zigzag; left, the diagonal ledge (one row per two
         * columns) repeats by its 4-column period down to the screen edge. */
        if (ox>=w) {
            if (oy<15 || oy>17) return 0;
            *x=24+PortSceneTileMod(ox-24,8); return 1;
        }
        sx=4+PortSceneTileMod(ox-4,4); sy=oy-(sx-ox)/2;
        if (sy<24 || sy>=h) return 0; /* ledge rows only; the pumpkins above stay unique */
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==87 && v[1]==3 &&
        (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* The chamber floor is repeatable, but BG3 also contains both room
         * walls. Extend only its clean interior swath; keep BG2's statues and
         * the authored perimeter strictly inside the original room. */
        int chunk=ox>=0?ox/w:(ox-(w-1))/w;
        int phase=PortSceneTileMod(chunk*3,8);
        /* the room's own structures go on too (user 24/10): the brick wall
         * on the right (columns 24..31 from row 18), the stone step at the
         * bottom along 2y+x=56, the stone landing at the top along 3y+x=12 */
        if (ox>=24 && oy>=18) {
            *x=24+PortSceneTileMod(ox-24,8); *y=oy<32?oy:22+PortSceneTileMod(oy-22,10); return 1;
        }
        if (ox<24 && 2*oy+ox>=56) {
            sx=8+PortSceneTileMod(ox-8,8); sy=oy+(ox-sx)/2;
            if (sy>=32) sy=28+PortSceneTileMod(sy-28,4);
            *x=sx; *y=sy; return 1;
        }
        if (3*oy+ox<12) { /* only the landing's stone (rows 0..1, columns 0..5) */
            *x=PortSceneTileMod(ox,6); *y=PortSceneTileMod(oy,2); return 1;
        }
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
            if (oy<20) {
                /* Three complete distant hills occupy 32 columns. Include
                 * all shadow rows: clipping at row 16 flattened the left
                 * skyline and substituted fence fragments on the right. */
                sx=PortSceneTileMod(ox,32);
                sy=oy<0?PortSceneTileMod(oy,8):oy;
                if (oy>=18 && sx>=30) { sx=0; sy=20; } /* no gate finial */
            } else if (ox>=w && oy<29) {
                /* A horizontal railing, not the diagonal corner repeated
                 * into saw teeth. Columns 40..43 hold a complete post, clear of the moon trail.
                 * Align their tips with the authored right corner (row 22),
                 * then use clean masonry without any grave or gate. */
                if (oy<22) { sx=0; sy=20; }
                else if (oy<25) {
                    sx=40+PortSceneTileMod(ox-64,4); sy=oy-4;
                } else {
                    sx=46+PortSceneTileMod(ox-64,2); sy=oy-4;
                }
            } else if (ox>=w) {
                sx=48+PortSceneTileMod(ox-48,16);
                sy=48+PortSceneTileMod(oy-48,16);
            } else if (ox<0 && diagonal<64) {
                sx=PortSceneTileMod(ox,8); sy=20;
            } else if (diagonal>=64 && diagonal<104) {
                /* the railing over the wall and the wall: the railing's posts
                 * reach up to 2y+x=64 (they were cut at 84, user 24/10) */
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
            *x=PortSceneTileMod(ox,16); *y=oy; return 1;
        }
        /* the maze behind each hedge and the hedge itself go on along their
         * diagonals (user 24/10): left hedge from 2y+x=54 to 72, right hedge
         * from 2y-x=-16 to 0; the lawn in front */
        if (ox<0 && 2*oy+ox<72) {
            if (2*oy+ox<54) {
                *x=PortSceneTileMod(ox,16);
                *y=12+PortSceneTileMod(oy-12,8); return 1;
            }
            else sx=8+PortSceneTileMod(ox-8,12);           /* the hedge and its topiaries */
            sy=oy+(ox-sx)/2;
            if (sy<12) sy=12+PortSceneTileMod(sy-12,8); /* the maze goes on upwards too */
            if (sy<64) { *x=sx; *y=sy; return 1; }
        }
        if (ox>=w && 2*oy-ox<0) {
            if (2*oy-ox<-16) {
                /* The maze is a horizontal plane; do not shear it with the
                 * foreground hedge or sample the throne at columns 56..57. */
                *x=PortSceneTileMod(ox,16);
                *y=12+PortSceneTileMod(oy-12,8); return 1;
            }
            sx=60+PortSceneTileMod(ox-60,4); /* clean hedge face */
            sy=oy+(sx-ox)/2;
            if (sy<12) sy=12+PortSceneTileMod(sy-12,8);
            if (sy<64) { *x=sx; *y=sy; return 1; }
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
        /* the side walls go on to the edges along their diagonals (user
         * 24/10): left, the carrot wallpaper and rail (columns 0..7, its
         * motif repeats every 8 columns); right, a red pillar and its yellow
         * lattice panel (columns 52..63); the floor below them as inside.
         * The slanted checker above repeats every 8 rows on both walls, and
         * both steps (8 and 12 columns) belong to its lattice. */
        if (ox<0 || (oy<0 && ox<32)) {
            sx=PortSceneTileMod(ox,8); sy=oy+(ox-sx)/2;
            if (sy<0) sy=PortSceneTileMod(sy,8);
            if (sy<h) { *x=sx; *y=sy; return 1; }
        } else if (ox>=w || oy<0) {
            sx=52+PortSceneTileMod(ox-52,12); sy=oy+(sx-ox)/2;
            if (sy<0) sy=PortSceneTileMod(sy,8);
            if (sy<h) { *x=sx; *y=sy; return 1; }
        }
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
    if (v[0]==99 && (ox<0 || ox>=w || oy<0 || oy>=h)) {
        /* Queen's chamber: mirror only the side wallpaper modules and the
         * checker floor. The centered fireplace, lamp, arch and vase stay
         * unique; the clean panel supplies the unseen top/bottom rows. */
        if (v[1]!=3) return 0;
        if (ox<0 || ox>=w) {
            int distance=ox<0?-ox-1:ox-w;
            if (oy<3) {
                /* the ceiling frieze's stripes come every 7 columns, the
                 * right half mirrored (user 24/10: seams every 32) */
                sx=ox<0?PortSceneTileMod(ox,7):25+PortSceneTileMod(ox-25,7);
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
    if (v[0] == 102 && v[1] == 2) {
        /* Three short shafts start below the map's top. Continue each in
         * its own column and five-row phase, including blank map cells.
         * Called only for the added viewport; cartridge pixels stay intact. */
        int start = -1;
        if (ox>=31 && ox<35) start=3;
        else if (ox>=40 && ox<44) start=1;
        else if (ox>=49 && ox<53) start=6;
        if (start>=0 && oy<start) {
            *y=start+PortSceneTileMod(oy-start,5); return 1;
        }
        if (oy<0) {
            if (!((ox>=22 && ox<30) || (ox>=35 && ox<39) || (ox>=45 && ox<49))) return 0;
            *y=PortSceneTileMod(oy,5); return 1;
        }
    }
    if (v[0] == 102) {
        /* The authored BG3 already contains its diagonal corridor. Do not
         * repeat that silhouette into the wide margins; the renderer fills
         * them from floor/water swatches outside the unchanged GBA viewport. */
        return ox >= 0 && ox < w && oy >= 0 && oy < h;
    }
    if (v[0]==104 && v[1]==3 && 2*oy-ox>=48) {
        /* Solid purple below the front edge, from an opaque wall tile. */
        *x=0; *y=25; return 1;
    }
    if (v[0]==104 && v[1]==3 && (ox<0 || ox>=w || oy<0) && 2*oy-ox<16) {
        /* the floor's back edge, its arcs whole and the stripes behind (user
         * 24/10): the floor repeats along (4,2); columns 40..43 hold whole
         * arcs (the vase hides the top of the arc at 36), the map's left
         * columns have theirs cut by its top */
        sx=40+PortSceneTileMod(ox-40,4); sy=oy+(sx-ox)/2;
        if (sy<0) sy=PortSceneTileMod(sy,4);
        *x=sx; *y=sy; return 1;
    }
    if (v[0]==104 && v[1]==3 && (ox<0 || ox>=w || oy<0 || oy>=31)) {
        /* Continue the floor and its front edge along (8,4). Row 31 is
         * already blank in this map; the old lower swatch crossed its edge. */
        sx=PortSceneTileMod(ox,8); sy=oy+(sx-ox)/2;
        if (sy>=32) sy=31;
        if (sy<0) sy=0;
        *x=sx; *y=sy; return 1;
    }
    if (v[0] == 97) {
        if (ox >= 8 && ox < 48 && oy >= 0 && oy < 40) return 1;
        if (!v[20]) return 0;
        *x=8+PortSceneTileMod(ox-8,40); *y=PortSceneTileMod(oy,40); return 1;
    }
    if (ox >= 0 && ox < w && oy >= 0 && oy < h) return 1;
    if (v[0]==53 && v[1]==3 && (ox<0 || ox>=w || oy<0) && 2*oy-ox<-1 &&
        ox>=SCENE053_PLATEAU_X0 && ox<SCENE053_PLATEAU_X0+SCENE053_PLATEAU_W &&
        oy>=SCENE053_PLATEAU_Y0 && oy<SCENE053_PLATEAU_Y0+SCENE053_PLATEAU_H) {
        /* the plateau laid out with whole tufts (gen_scene053_plateau.py, user 24/10) */
        const uint8_t* t=kScene053Plateau[oy-SCENE053_PLATEAU_Y0][ox-SCENE053_PLATEAU_X0];
        *x=t[0]; *y=t[1]; return 1;
    }
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

/* Scene 099's checker floor in one-point perspective (user 24/10: "as Guard
 * Armor"): measured on the map, its cells are w(y)=0.1269*(y-33.8) wide,
 * one centred on x=128, and its rows end where 1/(y-33.8) steps by 0.000567
 * (y=181,195,211,231,256). Each pixel takes the map pixel at the same place
 * in its cell pair: in its own row beside the map, in the nearest pair of
 * rows (211..255) below it, from the left of the row clear of the fireplace. */
static inline int PortPreliminar099Floor(int wx, int wy, int* sx, int* sy) {
    double w=0.1269*(wy-33.8), u=(wx-128)/w+0.5, y=wy, sw;
    int m;
    if (wy<160 || (wx>=0 && wx<256 && wy<256)) return 0;
    u-=2.0*(int)(u/2.0); if (u<0) u+=2.0; /* the place within the pair */
    if (wy>=256) {
        double v=2.0-(1.0/(wy-33.8)-0.005647)/0.000567;
        int iv=(int)v; double fv=v-iv;
        if (v<0) { iv-=1; fv+=1.0; }
        v=fv+((iv&1)?3.0:2.0);
        y=33.8+1.0/(0.005647-0.000567*(v-2.0));
        if (y>255.0) y=255.0;
    }
    sw=0.1269*(y-33.8);
    m=(int)((128.0/sw-0.5)/2.0);
    *sx=(int)(128.0+(u-2.0*m-0.5)*sw);
    *sy=(int)y;
    if (*sx<0) *sx=0;
    return 1;
}

static inline int PortPreliminarPixelSource(const PortPreliminarRecipe* p, int wx, int wy, int* sx, int* sy) {
    if (p && p->v[0]==112 && p->v[1]==2 && (wy<0 || wx>=512 || (wx>=416 && wx<456 && wy<24))) {
        if (wy<0 && wx>=192 && wx<208) {
            *sx=504; *sy=344; /* leave the corner shaft's gold face to BG3 */
        } else if (wx>=440 && wx<456 && wy<24) {
            *sx=wx; *sy=80; /* continue the existing outer jamb up with the tall door */
        } else if (wx>=352 && wx<448 && wy<24) {
            /* User 25/10: temporarily stretch the door's colour straight
             * up to the screen edge, as an unusually tall doorway. */
            /* Right leaf: one orange sample, not the purple divider repeated
             * across the panel (user's orange reference line, 25/10). */
            *sx=wx>=408?401:wx;
            *sy=8;
        } else if (wx>=176) {
            *sx=256+PortSceneTileMod(wx-256,64);
            *sy=wy-(wx-*sx)/2;
            if (*sy<0) *sy=PortSceneTileMod(*sy,32);
            if (*sy>=352) *sy=351;
        } else {
            *sx=504; *sy=344; /* no extra chairs on the left */
        }
        return 1;
    }
    if (p && p->v[0]==112 && p->v[1]==3 && wy<0 && wx>=368 && wx<384) {
        *sx=64+PortSceneTileMod(wx,16); *sy=PortSceneTileMod(wy,16);
        return 1; /* the existing shaft's light face, above the cut map */
    }
    if (p && p->v[0]==112 && p->v[1]==3 && (wy<0 || wx>=512) && wx>=352 && 2*wy-wx<-128) {
        /* BG2's wall texture has transparent pixels: its matching solid
         * pink undercoat must continue too, rather than exposing yellow. */
        *sx=352+PortSceneTileMod(wx-352,8); *sy=PortSceneTileMod(wy,16);
        return 1;
    }
    if (p && p->v[0]==120 && (wx<0 || wx>=256 || wy<0 || wy>=256)) {
        if (p->v[1]==1) {
            /* One foreground roof: continue its two slopes and the solid
             * ochre body below it, not a floating triangular cutout. */
            *sx=wx; *sy=wy;
            if (wx<0) { *sx=0; *sy=wy+wx/2; }
            else if (wx>=256) { *sx=255; *sy=wy-(wx-255)/2; }
            if (*sy<0) *sy=0;
            if (*sy>=256) *sy=255;
        } else if (p->v[1]==3) {
            if (wx>=0 && wx<32 && wy<0) {
                *sx=wx; *sy=PortSceneTileMod(wy,16); /* the original corner shaft, once */
            } else if (2*wy+wx<192) {
                *sx=32+PortSceneTileMod(wx-32,16);
                *sy=16+PortSceneTileMod(wy-16,16); /* plain masonry, no repeated shafts */
            } else {
                *sx=64+PortSceneTileMod(wx-64,32);
                *sy=160+PortSceneTileMod(wy-160,32); /* the room's clean ground */
            }
        } else if (p->v[1]==2) {
            int diagonal=2*wy+wx;
            *sx=255; *sy=255; /* unique lamps, sign and barrels remain in their authored positions */
            if (diagonal>=208 && diagonal<256) {
                *sx=64+PortSceneTileMod(wx-64,16);
                *sy=wy+(wx-*sx)/2; /* plain footing, along the wall's diagonal */
            }
        } else return 0;
        return 1;
    }
    if (p && p->v[0]==117 && p->v[1]==2 && wy<0 && wy>=-56 && wx>=40 && wx<152) {
        /* Complete the top-cut crate from the matching cap at (152,8).
         * Its last eight pixels cross the donor map's right edge: reflect
         * the cap's intact left tip, including the within-tile pixel phase. */
        *sx=wx+112; *sy=wy+56;
        if (*sx>=256) *sx=415-*sx;
        return 1;
    }
    if (p && p->v[0]==118 && (wx<0 || wx>=512 || wy<0 || wy>=512 ||
        (p->v[1]==2 && wx>=0 && wx<32 && 2*wy+wx>=96*8 && 2*wy+wx<128*8))) {
        int fore=2*wy+wx, beam=2*wy-wx;
        if (p->v[1]==3) {
            if (fore>=116*8) {
                /* The complete rail/scrollwork/hull module follows the
                 * opposite diagonal to the beams crossing the deck. */
                *sx=384+PortSceneTileMod(wx-384,128);
                *sy=wy+(wx-*sx)/2;
                if (*sy>=512) *sy=448+PortSceneTileMod(*sy-448,16);
            } else if ((beam>=8*8 && beam<16*8) || (beam>=64*8 && beam<72*8)) {
                /* Continue both authored beams, with their whole thickness. */
                *sx=PortSceneTileMod(wx,64); *sy=wy+(*sx-wx)/2;
            } else {
                *sx=128+PortSceneTileMod(wx-128,64);
                *sy=192+PortSceneTileMod(wy+(wx-*sx)/2-192,32);
            }
        } else if (p->v[1]==2) {
            *sx=0; *sy=0; /* empty authored cell */
            if (wy<0 && wx>=112 && wx<176) {
                *sx=wx; *sy=PortSceneTileMod(wy,64); /* the existing mast */
            } else if (fore>=96*8 && fore<128*8) {
                /* Whole cannon/rope bays along the rail, not clipped strips
                 * of their silhouettes at the old map boundary. */
                *sx=128+PortSceneTileMod(wx-128,128);
                *sy=wy+(wx-*sx)/2;
                /* Finish the last repeated rope's tip across the old map
                 * boundary too. This sampler is only used outside the GBA
                 * picture; a rectangular map cut must not cut its circle. */
                if (wx>=0 && wx<32) { *sx=wx+256; *sy=wy-128; }
                if (*sy<0 || *sy>=512) { *sx=0; *sy=0; }
            }
        } else return 0;
        return 1;
    }
    if (p && p->v[0]==95 && p->v[1]==3 && (wx<0 || wx>=512 || wy<0 || wy>=512)) {
        int tx=wx>>3, ty=wy>>3;
        if (wy>=48 && wy<96) {
            /* Only the mountain side of the skyline repeats; the castle
             * silhouette in the authored map must not gain extra copies. */
            int phase=PortSceneTileMod(wx,64);
            *sx=96+(phase<32?phase:63-phase); *sy=wy; return 1;
        }
        if (wx>=512 && 2*wy-wx>=-128 && 2*wy-wx<0) {
            /* Reflect the whole left hedge bay, including its flowers, rather
             * than chopping the right hedge into four-column stair steps. */
            int reflected=559-wx;
            *sx=64+PortSceneTileMod(reflected-64,96);
            *sy=wy+(reflected-*sx)/2;
            return 1;
        }
        if (!PortPreliminarTileSource(p,&tx,&ty)) return 0;
        *sx=tx*8+(wx&7); *sy=ty*8+(wy&7); return 1;
    }

    if (p && p->v[0]==110 && (wx<0 || wx>=512 || wy<0 || wy>=512)) {
        /* Agrabah's terrace turns at x=80 and x=208 inside the map. Beyond
         * the left edge it is horizontal (y=208..335), not a staircase of
         * eight-tile donor strips. Keep the authored corners and urns unique.
         * The two shafts cut by y=0 continue using their own plain tops. */
        if (p->v[1]==2) {
            *sx=504; *sy=504; /* empty BG2 cell */
            if (wy<0 && ((wx>=0 && wx<48) || (wx>=112 && wx<160))) {
                *sx=wx; *sy=PortSceneTileMod(wy,16);
            }
        } else if (p->v[1]==3) {
            if (wy>=512) {
                *sx=256+PortSceneTileMod(wx-256,128);
                *sy=384+PortSceneTileMod(wy-384,128);
            } else if (wx<0) {
                if (wy<208) {
                    *sx=48+PortSceneTileMod(wx-48,32);
                    *sy=80+PortSceneTileMod(wy-80,64);
                } else if (wy<336) {
                    *sx=PortSceneTileMod(wx,64); *sy=wy;
                } else {
                    *sx=256+PortSceneTileMod(wx-256,128);
                    *sy=384+PortSceneTileMod(wy-384,128);
                }
            } else if (wy<0) {
                if (wx<160) {
                    *sx=wx; *sy=PortSceneTileMod(wy,16);
                    /* Above the purple corner, sample the same wall plane. */
                    if (wx>=48 && wx<112) *sy=0;
                } else {
                    /* The large cracked swatch starts below its authored
                     * ledge. Above it use the intact quiet sand on the left,
                     * not repeated fragments of the bottom-born pattern. */
                    *sx=48+PortSceneTileMod(wx-48,32);
                    *sy=80+PortSceneTileMod(wy-80,64);
                }
            } else if (wy<112) {
                /* Atlas cloud 2 is the height reference, not a second flaw:
                 * retain the cracked donor below this level only. */
                *sx=48+PortSceneTileMod(wx-48,32);
                *sy=80+PortSceneTileMod(wy-80,64);
            } else {
                *sx=wy>=144 && wy<264?320+PortSceneTileMod(wx-320,160):320+PortSceneTileMod(wx-320,192);
                *sy=wy;
            }
        } else return 0;
        return 1;
    }

    if (p && p->v[0]==104 && p->v[1]==2 && wy<0) {
        /* the kelp cut by the map's top goes on up its 6-row cycle and ends
         * in its own bottom tip, reflected (user 24/10); each at its height */
        static const int8_t kKelp[][3]={{8,-4,12},{10,-2,9},{21,-3,9},{24,-5,13},
                                        {25,-3,10},{27,-4,11},{35,-2,11},{36,-4,15}};
        int col=wx>>3, row=wy>>3;
        unsigned i;
        for (i=0; i<sizeof(kKelp)/sizeof(kKelp[0]); i++) {
            if (kKelp[i][0]!=col || row<kKelp[i][1]) continue;
            *sx=wx;
            *sy=row==kKelp[i][1]?kKelp[i][2]*8+7-(wy&7):PortSceneTileMod(row,6)*8+(wy&7);
            return 1;
        }
        *sx=504; *sy=248; /* an empty cell of the map */
        return 1;
    }
    if (p && p->v[0]==99 && p->v[1]==3 && (wx<0 || wx>=256 || wy<24 || wy>=256)) {
        int tx=wx>>3, ty=wy>>3;
        if (wy<24) {
            /* Same room as Trickmaster: extend its wall, not the beige
             * checker ceiling. Posts and the single stone shaft retain
             * their x; the clean panel excludes the clock and window. */
            *sx=PortSceneTileMod(wx,56);
            *sy=24+PortSceneTileMod(wy-24,48);
            if ((wx>=56 && wx<120) || (wx>=184 && wx<208)) *sx=wx;
            else if (wx>=144 && wx<184) *sx=72+PortSceneTileMod(wx-144,40);
            if ((wx>=56 && wx<120) || (wx>=144 && wx<208)) *sy=24;
            if (wx>=120 && wx<144) {
                *sx=wx; *sy=24+PortSceneTileMod(wy-24,16);
            }
            return 1;
        }
        if ((wx<0 || wx>=256) && wy>=24 && wy<176) {
            /* Same intact wallpaper module as the arena: no mirrored prop
             * fragment beside the room, and no flat filler farther away. */
            *sx=PortSceneTileMod(wx,56); *sy=wy;
            if (wy>=108 && wy<128) {
                if (wy<112) { *sx=32; *sy=108; }
                else if (wy<114 || wy>=126) *sx=32;
            }
            return 1;
        }
        if (PortPreliminar099Floor(wx,wy,sx,sy)) return 1;
        if (!PortPreliminarTileSource(p,&tx,&ty)) return 0;
        *sx=tx*8+(wx&7); *sy=ty*8+(wy&7);
        return 1;
    }
    if (p && p->v[0]==45 && p->v[1]==2 && wx<0 && wx>=-24 && wy>=54*8 && wy<64*8) {
        /* the pine cut by the map's left edge (its axis at x=20): its missing
         * half is its right half reflected, below row 54 where the next pine
         * ends; its tip above is narrow enough to be whole (user 24/10) */
        *sx=39-wx; *sy=wy;
        return 1;
    }
    if (p && p->v[0]==124 && (wx<0 || wx>=480 || wy<0 || wy>=440)) {
        int L=2*wy+wx, R=2*wy-wx;
        if (p->v[1]==3) {
            /* The golden field surrounds finite platforms. The two arms
             * which cross the source edges keep their own borders; the
             * upper-left point is an authored corner, not a repeated bay. */
            *sx=320+PortSceneTileMod(wx-320,64);
            *sy=400+PortSceneTileMod(wy-400,32);
            if (L<496) {
                *sx=PortSceneTileMod(wx,64);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
            } else if (wx<0 && L>=600 && L<880) {
                *sx=8+PortSceneTileMod(wx-8,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && L>=600 && L<880) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && wx<520 && R>=16 && L<=1054) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            }
        } else if (p->v[1]==2) {
            *sx=504; *sy=504; /* no extra columns */
            if (wx<0 && L>=608 && L<656) {
                *sx=8+PortSceneTileMod(wx-8,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx<0 && L>=816 && L<896) {
                *sx=64+PortSceneTileMod(wx-64,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && ((L>=608 && L<656) || (L>=816 && L<896))) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            } else if (wx>=480 && wx<520 && R>=0 && R<48 && L<=1054) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy-(wx-*sx)/2;
            } else if (wx>=480 && wx<520 && L>=1040 && L<1120 && R>=16) {
                *sx=448+PortSceneTileMod(wx-448,16);
                *sy=wy+(wx-*sx)/2;
            }
        } else return 0;
        return 1;
    }
    if (p && p->v[0]==121 && (wx<0 || wx>=384 || wy<0 || wy>=256)) {
        if (p->v[1]==2) {
            *sx=504; *sy=248; /* empty map cell, not the emblem */
            if (wx<0) {
                int x=PortSceneTileMod(wx,16), y=wy+(wx-x)/2;
                if (y>=128 && y<160) { *sx=x; *sy=y; }
            } else if (wx>=224) {
                int x=240+PortSceneTileMod(wx-240,16), y=wy+(wx-x)/2;
                if (y>=40 && y<64) { *sx=x; *sy=y; }
            }
            /* The original corner shaft crosses y=0. Continue that shaft,
             * and complete the two existing windows from the intact first
             * window; do not place windows at any new positions. */
            if (wy<0 && wx>=214 && wx<234) {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
            } else if (wy<0 && wx>=284 && wx<292 && wy>=-10) {
                *sx=wx-20; *sy=wy+10;
            } else if (wy<0 && wx>=304 && wx<312 && wy>=-20) {
                *sx=wx-40; *sy=wy+20;
            }
        } else if (p->v[1]==3) {
            int floor;
            if (wx<192) {
                *sx=PortSceneTileMod(wx,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,64);
                floor=2*wy+wx>=272;
            } else if (wx>=224) {
                *sx=224+PortSceneTileMod(wx-224,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
                floor=2*wy+wx>=336;
            } else {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
                floor=2*wy-wx>=-112;
            }
            if (floor) {
                *sx=PortSceneTileMod(wx,128);
                *sy=160+PortSceneTileMod(wy-160,64);
            }
        } else return 0;
        return 1;
    }
    /* Coliseum 122, approved with honours (user 24/10, Codex c62c7d8ee) */
    if (p && p->v[0]==122 && (wx<0 || wx>=440 || wy<0 || wy>=336)) {
        /* Three original wall planes: the central face spans x=184..311.
         * Side walls follow 2*y-x, with their own intercepts. Translate in
         * pixels to retain the four-pixel phase of each 8-pixel column. */
        if (p->v[1]==2) {
            *sx=504; *sy=504; /* transparent: never clone the heraldic reliefs */
            if (wx<0 || (wx<184 && wy<0)) {
                int x=24+PortSceneTileMod(wx-24,16), y=wy+(x-wx)/2;
                if (y>=0 && y<32) { *sx=x; *sy=y; }
            } else if (wx>=440) {
                int x=384+PortSceneTileMod(wx-384,16), y=wy+(x-wx)/2;
                if (y>=112 && y<144) { *sx=x; *sy=y; }
            }
            /* Complete only the original panels cut by y=0 or x=440.
             * The intact diagonal panel at (312,16)..(376,112) is the donor;
             * no new heraldry is placed in the empty extended walls. */
            if (wy<0 && wx>=56 && wx<120) { *sx=wx+256; *sy=wy+64; }
            else if (wy<0 && wx>=120 && wx<184) { *sx=wx+192; *sy=wy+32; }
            else if (wx>=440 && wx<472 && wy-48>=16 && wy-48<112) {
                *sx=wx-96; *sy=wy-48;
            }
            if (*sy<0) { *sx=504; *sy=504; }
            if ((wx<184 && 2*wy-wx>=32) ||
                (wx>=184 && wx<312 && wy>=112) ||
                (wx>=312 && 2*wy-wx>=-96)) {
                *sx=PortSceneTileMod(wx,128);
                *sy=160+PortSceneTileMod(wy-160,128);
            }
        } else if (p->v[1]==3) {
            if (wx<184) {
                *sx=128+PortSceneTileMod(wx-128,32);
                *sy=wy+(*sx-wx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
            } else if (wx>=312) {
                *sx=320+PortSceneTileMod(wx-320,32);
                *sy=wy+(*sx-wx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
            } else {
                *sx=wx; *sy=PortSceneTileMod(wy,64);
            }
            /* Past the wall contact, extend an intact pavement module.
             * Original BG2 accents stay within the authored floor. */
            if ((wx<184 && 2*wy-wx>=-24) ||
                (wx>=184 && wx<312 && wy>=96) ||
                (wx>=312 && 2*wy-wx>=-152)) {
                *sx=PortSceneTileMod(wx,64);
                *sy=160+PortSceneTileMod(wy-160,64);
            }
        } else return 0;
        return 1;
    }
    /* Coliseum 121, approved with honours (user 24/10, Codex 1ee40e4e6): only this scene of Preliminar V1 */
    if (p && p->v[0]==121 && (wx<0 || wx>=384 || wy<0 || wy>=256)) {
        if (p->v[1]==2) {
            *sx=504; *sy=248; /* empty map cell, not the emblem */
            if (wx<0) {
                int x=PortSceneTileMod(wx,16), y=wy+(wx-x)/2;
                if (y>=128 && y<160) { *sx=x; *sy=y; }
            } else if (wx>=224) {
                int x=240+PortSceneTileMod(wx-240,16), y=wy+(wx-x)/2;
                if (y>=40 && y<64) { *sx=x; *sy=y; }
            }
            /* The original corner shaft crosses y=0. Continue that shaft,
             * and complete the two existing windows from the intact first
             * window; do not place windows at any new positions. */
            if (wy<0 && wx>=214 && wx<234) {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
            } else if (wy<0 && wx>=284 && wx<292 && wy>=-10) {
                *sx=wx-20; *sy=wy+10;
            } else if (wy<0 && wx>=304 && wx<312 && wy>=-20) {
                *sx=wx-40; *sy=wy+20;
            }
        } else if (p->v[1]==3) {
            int floor;
            if (wx<192) {
                *sx=PortSceneTileMod(wx,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,64);
                floor=2*wy+wx>=272;
            } else if (wx>=224) {
                *sx=224+PortSceneTileMod(wx-224,32);
                *sy=wy+(wx-*sx)/2;
                if (*sy<0) *sy=PortSceneTileMod(*sy,32);
                floor=2*wy+wx>=336;
            } else {
                *sx=wx; *sy=PortSceneTileMod(wy,32);
                floor=2*wy-wx>=-112;
            }
            if (floor) {
                *sx=PortSceneTileMod(wx,128);
                *sy=160+PortSceneTileMod(wy-160,64);
            }
        } else return 0;
        return 1;
    }
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
