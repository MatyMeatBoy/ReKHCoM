/* Source-pixel LCD gutters integrated over physical output pixels.
 * Pure bounded arithmetic: no SDL, allocations or inter-frame state. */
#ifndef PORT_LCD_GRID_H
#define PORT_LCD_GRID_H

typedef struct PortLcdGridKey {
    int width, height;
    double pitchX, pitchY, phaseX, phaseY;
} PortLcdGridKey;

static double PortLcdGridFloor(double value) {
    long long integral = (long long)value;
    return (double)(integral - (value < (double)integral));
}
static double PortLcdGridPhase(double origin, double pitch) {
    return origin - PortLcdGridFloor(origin / pitch) * pitch;
}
static int PortLcdGridMakeKey(PortLcdGridKey* key, int width, int height,
                             double originX, double originY, double pitchX, double pitchY) {
    if (width < 1 || height < 1 || width > 32768 || height > 32768 ||
        !(pitchX >= 1.0 / 1024 && pitchX <= 32768) ||
        !(pitchY >= 1.0 / 1024 && pitchY <= 32768) ||
        !(originX >= -1000000000 && originX <= 1000000000) ||
        !(originY >= -1000000000 && originY <= 1000000000)) return 0;
    key->width = width; key->height = height;
    key->pitchX = pitchX; key->pitchY = pitchY;
    key->phaseX = PortLcdGridPhase(originX, pitchX);
    key->phaseY = PortLcdGridPhase(originY, pitchY);
    return 1;
}
static int PortLcdGridSame(const PortLcdGridKey* a, const PortLcdGridKey* b) {
    return a->width == b->width && a->height == b->height &&
        a->pitchX == b->pitchX && a->pitchY == b->pitchY &&
        a->phaseX == b->phaseX && a->phaseY == b->phaseY;
}
static double PortLcdGridIntegral(double position, double pitch) {
    double cells = PortLcdGridFloor(position / pitch);
    double gap = pitch * 0.125;
    double inside;
    if (gap > 1) gap = 1; /* Never obscure more than one physical pixel. */
    inside = position - cells * pitch - (pitch - gap);
    if (inside < 0) inside = 0;
    if (inside > gap) inside = gap;
    return cells * gap + inside;
}
static int PortLcdGridAlpha(int pixel, double phase, double pitch) {
    double coverage = PortLcdGridIntegral(pixel + 1.0 - phase, pitch) -
                      PortLcdGridIntegral(pixel - phase, pitch);
    if (coverage <= 0) return 0;
    if (coverage >= 1) return 255;
    return (int)(coverage * 255 + 0.5);
}
#endif
