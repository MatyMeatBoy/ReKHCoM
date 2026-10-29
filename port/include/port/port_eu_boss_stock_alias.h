#ifndef PORT_EU_BOSS_STOCK_ALIAS_H
#define PORT_EU_BOSS_STOCK_ALIAS_H

/* EU has separate small-card sources for boss attack variants. Their OAM
 * layouts match the collectible portrait; preserve each native coloured
 * crown/border/value while sharing only the existing HD portrait window.
 * Exact (tiles, frame) pairs only: never reinterpret an unknown frame. */
#ifdef VERSION_EU
extern const unsigned char gAxelSmallCardTiles[], gAxelSmallCardFrame0[];
extern const unsigned char gAxelASmallCardTiles[], gAxelASmallCardFrame0[];
extern const unsigned char gAxelBSmallCardTiles[], gAxelBSmallCardFrame0[];
extern const unsigned char gHadesSmallCardTiles[], gHadesSmallCardFrame0[];
extern const unsigned char gHadesASmallCardTiles[], gHadesASmallCardFrame0[];
extern const unsigned char gHadesBSmallCardTiles[], gHadesBSmallCardFrame0[];
extern const unsigned char gLarxeneSmallCardTiles[], gLarxeneSmallCardFrame0[];
extern const unsigned char gLarxeneASmallCardTiles[], gLarxeneASmallCardFrame0[];
extern const unsigned char gLarxeneBSmallCardTiles[], gLarxeneBSmallCardFrame0[];
extern const unsigned char gLarxeneCSmallCardTiles[], gLarxeneCSmallCardFrame0[];
extern const unsigned char gRikuSmallCardTiles[], gRikuSmallCardFrame0[];
extern const unsigned char gRikuASmallCardTiles[], gRikuASmallCardFrame0[];
extern const unsigned char gRikuBSmallCardTiles[], gRikuBSmallCardFrame0[];
extern const unsigned char gRikuCSmallCardTiles[], gRikuCSmallCardFrame0[];
extern const unsigned char gRikuDSmallCardTiles[], gRikuDSmallCardFrame0[];
#endif

static inline void PortHdNormalizeEuBossStock(const void** tiles, const void** frame) {
#ifdef VERSION_EU
    static const struct { const void *tiles, *frame, *portraitTiles, *portraitFrame; } aliases[] = {
        { gAxelASmallCardTiles, gAxelASmallCardFrame0, gAxelSmallCardTiles, gAxelSmallCardFrame0 },
        { gAxelBSmallCardTiles, gAxelBSmallCardFrame0, gAxelSmallCardTiles, gAxelSmallCardFrame0 },
        { gHadesASmallCardTiles, gHadesASmallCardFrame0, gHadesSmallCardTiles, gHadesSmallCardFrame0 },
        { gHadesBSmallCardTiles, gHadesBSmallCardFrame0, gHadesSmallCardTiles, gHadesSmallCardFrame0 },
        { gLarxeneASmallCardTiles, gLarxeneASmallCardFrame0, gLarxeneSmallCardTiles, gLarxeneSmallCardFrame0 },
        { gLarxeneBSmallCardTiles, gLarxeneBSmallCardFrame0, gLarxeneSmallCardTiles, gLarxeneSmallCardFrame0 },
        { gLarxeneCSmallCardTiles, gLarxeneCSmallCardFrame0, gLarxeneSmallCardTiles, gLarxeneSmallCardFrame0 },
        { gRikuASmallCardTiles, gRikuASmallCardFrame0, gRikuSmallCardTiles, gRikuSmallCardFrame0 },
        { gRikuBSmallCardTiles, gRikuBSmallCardFrame0, gRikuSmallCardTiles, gRikuSmallCardFrame0 },
        { gRikuCSmallCardTiles, gRikuCSmallCardFrame0, gRikuSmallCardTiles, gRikuSmallCardFrame0 },
        { gRikuDSmallCardTiles, gRikuDSmallCardFrame0, gRikuSmallCardTiles, gRikuSmallCardFrame0 },
    };
    unsigned i;
    for (i = 0; i < sizeof(aliases) / sizeof(aliases[0]); ++i) {
        if (*tiles == aliases[i].tiles && *frame == aliases[i].frame) {
            *tiles = aliases[i].portraitTiles;
            *frame = aliases[i].portraitFrame;
            return;
        }
    }
#else
    (void)tiles; (void)frame;
#endif
}

#endif
