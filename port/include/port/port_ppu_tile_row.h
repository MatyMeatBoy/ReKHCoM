#ifndef PORT_PPU_TILE_ROW_H
#define PORT_PPU_TILE_ROW_H
#include <stdint.h>

/* Only call after the complete4bpp row passed its VRAM bounds check.
 * Byte reads need no alignment and palette slot0 is always transparent. */
static inline int PortPpu4bppRowEmpty(const uint8_t* pixels) {
    return !(pixels[0] | pixels[1] | pixels[2] | pixels[3]);
}
#endif
