#ifndef PORT_SCENE120_ENGRAVING_H
#define PORT_SCENE120_ENGRAVING_H

/* The two mural heads are already in the loaded character bank, directly
 * above its visible tiles692..699. Only their missing upper ink is used;
 * the atlas's other ornament fragments and masonry remain untouched. */
static inline int PortScene120EngravingEntry(int x, int y) {
    if (x < 96 || x >= 160 || y < -48 || y >= 0 || (x < 128 && y < -32)) return -1;
    return 0x4000 | (((y + 168) >> 3) * 32 + ((x + 64) >> 3));
}

static inline int PortScene120EngravingInk(int index) {
    return index >= 0 && index < 16 && (0x1D00u & (1u << index));
}
#endif
