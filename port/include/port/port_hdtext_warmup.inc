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

/* Gameplay's HD text before the game starts (OptiPort 2.2: entering the map cards rendered 12 new glyphs of the
   menu face at the menus' screen-layer scale, 4-9 ms, OPTIPORT_STUTTER_VIEW.md): the dialogue/menu face's Latin
   set at the world's and the menus' scales (its two slots) and the cells' digits at their default height. A later
   scale still renders on use. */
void PortHdTextWarmGame(SDL_Renderer* renderer, float world, float menu) {
    static const char cells[] = "0123456789/:+-x ";
    const float scales[2] = { world, menu };
    HdFont* font;
    int code, k;
    const char* c;
    if (!gPortConfig.hdText || !renderer || world <= 0) return;
    for (k = 0; k < 2; k++) {
        if (scales[k] <= 0 || (k && scales[k] == world)) continue;
        font = FontForScale(scales[k], 0);
        if (font->font)
            for (code = 32; code < 256; code++) Glyph(renderer, font, code);
    }
    font = FontForScale(world * (5.0f / HD_TEXT_CAP), 2);
    if (font->font)
        for (c = cells; *c; c++) Glyph(renderer, font, (unsigned char)*c);
}
