#ifndef PORT_HALLOWEEN_FLOOR_H
#define PORT_HALLOWEEN_FLOOR_H
#include <stdint.h>

/* Halloween's affine arena ends at texture row 512. The donor is its own
 * unobstructed ground (400..463), below every fence, pumpkin and tree.
 * Project a repeating ground module towards the viewer around (256,320):
 * its apparent width/depth grow together. No framebuffer colours, reflected
 * ornaments or native pixels participate. Coordinates remain in source VRAM
 * so the usual palette, fades, masks and sprite composition still apply. */
static int PortHalloweenFloorSource(int* x, int* y) {
    int depth, sx, distance;
    if (*y < 512) return 0;
    depth = *y - 320;
    sx = 256 + (int)(((int64_t)*x - 256) * 192 / depth);
    distance = (int)(((int64_t)*y - 512) * 192 / depth);
    sx = (sx - 64) % 384;
    if (sx < 0) sx += 384;
    *x = 64 + sx;
    *y = 400 + (48 + distance) % 64;
    return 1;
}
#endif
