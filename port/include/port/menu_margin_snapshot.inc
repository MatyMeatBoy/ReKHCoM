/* Capture presentation colours while the rendered frame's VRAM, palettes,
   registers and object slots are still live. VBlank may replace all of them.
   OAM x is -256..255 and its largest part is 64 pixels: 576 columns cover
   every possible authored header span, without heap or window-size buffers. */
#define MENU_HEADER_ROWS 16
#define MENU_HEADER_COLUMNS 576
static struct {
    uint32_t sprite[MENU_HEADER_ROWS][MENU_HEADER_COLUMNS];
    uint32_t band[MENU_HEADER_ROWS][2], still[MENU_HEADER_ROWS][2];
    uint32_t stripes[GBA_SCREEN_H][2], tabs[2][16];
    int x, left, width, drawn;
} sMenuPixels;

static void CaptureMenuMarginPixels(void) {
    int y, x, phase, i;
    int shown = sMenuHeaderFrame && HeaderOnScreen();
    sMenuPixels.drawn = sMenuHeaderFrame && sMenuShown.headerDrawn;
    sMenuPixels.x = sMenuShown.headerX;
    sMenuPixels.left = HeaderLeft();
    sMenuPixels.width = sMenuPixels.drawn ? HeaderWidth() : 0;
    for (y = 0; y < MENU_HEADER_ROWS; y++) {
        for (phase = 0; phase < 2; phase++) {
            uint32_t color = HeaderPixelRaw(y, phase);
            sMenuPixels.band[y][phase] = color;
            sMenuPixels.still[y][phase] = shown ? color : 0;
        }
        for (x = 0; x < sMenuPixels.width; x++)
            sMenuPixels.sprite[y][x] = MenuSpritePixel(sMenuHeaderFrame, sMenuHeaderTiles,
                                                       sMenuHeaderPalette, y, 0, x);
    }
    for (y = 0; y < GBA_SCREEN_H; y++)
        for (phase = 0; phase < 2; phase++) {
            uint32_t color = 0;
            for (i = 0; i < 2 && !color; i++) {
                const MenuBandSource* band = &sMenuBands[i];
                color = MenuSpritePixel(band->frame, band->tiles, band->palette, y - band->y, phase, 0);
            }
            sMenuPixels.stripes[y][phase] = color;
        }
    for (i = 0; i < 2; i++)
        for (y = 0; y < 16; y++)
            sMenuPixels.tabs[i][y] = MenuTabPixelRaw(y + 16, 0, i ? 1 : -1);
}

uint32_t PortMenuHeaderPixel(int y, int phase) {
    return (unsigned)y < MENU_HEADER_ROWS ? sMenuPixels.still[y][phase & 1] : 0;
}

uint32_t PortMenuHeaderPixelAt(int y, int distance) {
    int64_t source;
    if ((unsigned)y >= MENU_HEADER_ROWS || distance < 0) return 0;
    if (!sMenuPixels.drawn) return PortMenuHeaderPixel(y, distance);
    source = -1LL - distance - sMenuPixels.x - sMenuPixels.left;
    if (source < 0) return sMenuPixels.band[y][distance & 1];
    return source < sMenuPixels.width ? sMenuPixels.sprite[y][source] : 0;
}

uint32_t PortMenuBandPixel(int y, int phase) {
    return (unsigned)y < GBA_SCREEN_H ? sMenuPixels.stripes[y][phase & 1] : 0;
}

uint32_t PortMenuTabPixel(int y, int phase, int side) {
    (void)phase; /* the authored tab's clean edge has no alternating columns */
    return y >= 16 && y < 32 ? sMenuPixels.tabs[side > 0][y - 16] : 0;
}
