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

static inline int PortJiminyBookPixel(int list, int topLayer, int blend, int x, int y) {
    return (!list || x >= 91) && PortJiminyPaperPixel(topLayer, blend, x, y);
}

/* The last binder loop is an object, unlike the fourteen BG1 loops. Only
   that exact authored object gives way; cursors, portraits and NEW stay. */
static inline int PortJiminyBookObject(int list, int topLayer, int blend, int binder, int x, int y) {
    return list && topLayer == 4 && !blend && binder && x >= 0 && x < 240 && y >= 16 && y < 160;
}

/* Name plates occupy three BG0 tile rows. Capture final colours rather than
   infer an extension from the width of a translated title or live palettes. */
static inline void PortJiminyRecordNameEdge(uint32_t* edges, int layer, int blend,
        int x, int y, uint32_t pixel) {
    if (!edges || (x != 0 && x != 239) || y < 16 || y >= 40) return;
    edges[(x == 239 ? 24 : 0) + y - 16] = layer == 0 && !blend ? pixel : 0;
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
