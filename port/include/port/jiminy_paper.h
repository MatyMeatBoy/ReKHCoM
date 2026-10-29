#ifndef PORT_JIMINY_PAPER_H
#define PORT_JIMINY_PAPER_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Header labels differ, but these three detail layouts share the body. A
   pending mode/style change must not repaint the previous root/list frame. */
static inline int PortJiminyDetailMap(const uint16_t* shown, const uint16_t* detail) {
    return shown && detail && !memcmp(shown + 2 * 32, detail + 2 * 32, 18 * 32 * sizeof(uint16_t));
}

/* Only the actual opaque BG1 winner may give way to the new book. Other
   backgrounds, objects, windows and alpha combinations retain their image. */
static inline int PortJiminyPaperPixel(int topLayer, int blend, int x, int y) {
    return topLayer == 1 && !blend && x >= 0 && x < 240 && y >= 16 && y < 160;
}

static inline int PortJiminyForeground(uint32_t* out, const uint32_t* composite,
        const uint32_t* native, const uint8_t* paper, int width, int height, int originX, int originY) {
    int x, y;
    if (!out || !composite || !native || !paper || width < 240 || height < 160 ||
        originX < 0 || originY < 0 || originX > width - 240 || originY > height - 160) return 0;
    /* Preserve the complete frame transform when scaling the foreground.
       Cropping away the header changes pixel-art sampling at noninteger zoom. */
    memset(out, 0, 16 * 240 * sizeof(*out));
    for (y = 16; y < 160; y++)
        for (x = 0; x < 240; x++) {
            size_t source = (size_t)(originY + y) * width + originX + x;
            int target = y * 240 + x;
            out[target] = paper[y * 240 + x] && composite[source] == native[source] ? 0 :
                          composite[source] | 0xFF000000u;
        }
    return 1;
}
#endif
