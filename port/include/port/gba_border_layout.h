#ifndef PORT_GBA_BORDER_LAYOUT_H
#define PORT_GBA_BORDER_LAYOUT_H

typedef struct PortGbaBorderArea { int x, y, w, h; } PortGbaBorderArea;
typedef struct PortGbaBorderRect { float x, y, w, h; } PortGbaBorderRect;

/* Reserve the same fraction on both axes: keep the selected aspect ratio.
 * Dividing before multiplying also permits large validated output sizes. */
static inline PortGbaBorderArea PortGbaBorderViewport(int width, int height, int enabled) {
    PortGbaBorderArea r = { 0, 0, 0, 0 };
    if (width <= 0 || height <= 0) return r;
    r.w = width;
    r.h = height;
    if (!enabled) return r;
    r.w = width / 27 * 20 + width % 27 * 20 / 27;
    r.h = height / 27 * 20 + height % 27 * 20 / 27;
    if (r.w < 1) r.w = 1;
    if (r.h < 1) r.h = 1;
    r.x = (width - r.w) / 2;
    r.y = (height - r.h) / 2;
    return r;
}

static inline float PortGbaBorderMin(float a, float b) { return a < b ? a : b; }
static inline int PortGbaBorderClose(float a, float b) { return a - b < .01f && b - a < .01f; }

static inline int PortGbaBorderCeilBound(float value, int limit) {
    int n;
    if (value >= (float)limit) return limit;
    n = (int)value;
    return n + (n < value);
}

/* Fixed views keep their nominal canvas and chosen integer/fit scale. Give
 * every overlay the real picture viewport, rather than its old letterbox. */
static inline PortGbaBorderArea PortGbaBorderPictureArea(int width, int height, int canvasW, int canvasH, int integer) {
    PortGbaBorderArea r = { 0, 0, 0, 0 };
    float scale;
    if (width <= 0 || height <= 0 || canvasW <= 0 || canvasH <= 0) return r;
    scale = PortGbaBorderMin((float)width / canvasW, (float)height / canvasH);
    if (integer && scale >= 1) scale = (float)(int)scale;
    r.w = PortGbaBorderCeilBound(canvasW * scale, width);
    r.h = PortGbaBorderCeilBound(canvasH * scale, height);
    r.x = (width - r.w) / 2;
    r.y = (height - r.h) / 2;
    return r;
}

static inline int PortGbaBorderLegacy(int width, int height, const PortGbaBorderRect* ap) {
    float s = ap->w / 240.0f;
    return s > 0 && PortGbaBorderClose(width, 384 * s) && PortGbaBorderClose(height, 216 * s) &&
        PortGbaBorderClose(ap->x, 72 * s) && PortGbaBorderClose(ap->y, 28 * s) &&
        PortGbaBorderClose(ap->h, 160 * s);
}

/* A native ornament uses this same scale in X/Y. Never elongate map cards
 * to fill a taller side panel. Plain material and bevels fill the rest. */
static inline float PortGbaBorderDecorationScale(int width, int height, const PortGbaBorderRect* ap) {
    float cx = ap->x + ap->w / 2;
    float s = PortGbaBorderMin(cx / 192, (width - cx) / 192);
    s = PortGbaBorderMin(s, ap->x / 72);
    s = PortGbaBorderMin(s, (width - ap->x - ap->w) / 72);
    s = PortGbaBorderMin(s, ap->y / 28);
    s = PortGbaBorderMin(s, (height - ap->y - ap->h) / 28);
    s = PortGbaBorderMin(s, ap->w / 240);
    s = PortGbaBorderMin(s, ap->h / 160);
    if (s <= 0) return 0;
    if (s >= 1) s = (float)(int)s;
    return s;
}

#endif
