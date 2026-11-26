#ifndef PORT_OVERLAY_VERTICAL_BANDS_H
#define PORT_OVERLAY_VERTICAL_BANDS_H

#include <stdint.h>

/* Only carry a genuinely solid, opaque edge: never smear lettering, a
 * partially transparent fade or an incomplete sliding band into a margin. */
static int PortOverlaySolidEdge(const uint32_t* row, int width, uint32_t* color) {
    if (!row || !color || width <= 0 || (row[0] >> 24) != 255)
        return 0;
    for (int x = 1; x < width; x++)
        if (row[x] != row[0])
            return 0;
    *color = row[0];
    return 1;
}

#endif
