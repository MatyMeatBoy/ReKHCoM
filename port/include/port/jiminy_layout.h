#ifndef PORT_JIMINY_LAYOUT_H
#define PORT_JIMINY_LAYOUT_H

/* Presentation only: all 240x160 game pixels remain visible above the help row. */
typedef struct PortJiminyLayout {
    float scale, x, y, width, height, footerY, footerHeight;
} PortJiminyLayout;

static inline int PortJiminyFit(int width, int height, PortJiminyLayout* out) {
    float horizontal, vertical, scale;
    if (!out || width <= 0 || height <= 0) return 0;
    horizontal = width / 240.0f;
    vertical = height / 174.0f;
    scale = horizontal < vertical ? horizontal : vertical;
    out->scale = scale;
    out->width = 240 * scale;
    out->height = 160 * scale;
    out->x = (width - out->width) * 0.5f;
    out->y = (height - 174 * scale) * 0.5f;
    out->footerY = out->y + out->height;
    out->footerHeight = 14 * scale;
    return 1;
}

#endif
