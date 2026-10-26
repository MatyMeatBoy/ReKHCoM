/* Shared by GDI, FreeType, browser and console glyph backends. Prepare only
   a bounded two-slot UI cache, independent of gameplay's font churn. */
int PortCodepointToSjis(unsigned cp);
void PortHdTextWarmUi(SDL_Renderer* renderer, float scale) {
    static const unsigned arrows[] = {0x2190, 0x2191, 0x2192, 0x2193};
    HdFont* font;
    int code;
    if (!gPortConfig.hdTitle || !renderer || scale <= 0) return;
    font = FontForScale(scale, 3);
    if (!font->font) return;
    for (code = 32; code < 256; code++) Glyph(renderer, font, code);
    for (code = 0; code < 4; code++) {
        int sjis = PortCodepointToSjis(arrows[code]);
        if (sjis > 255) GlyphJp(renderer, font, (uint16_t)sjis);
    }
}
