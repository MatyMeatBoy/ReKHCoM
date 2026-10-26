#include "port/port_hdtext_warmup.inc"

/* Port-owned settings and new-game panels: the same embedded KHMenu and
   portable glyph cache as the game's HD text, independent of hd_text. */
void PortHdTextUiGlyph(SDL_Renderer* renderer, int code, float x, float y, float scale, float cell,
                       uint32_t face, uint32_t shadow) {
    HdFont* font;
    HdGlyph* glyph;
    PortTextGlyph unit = {0};
    float offset = SDL_max(1.0f, scale * 0.5f);
    if (!gPortConfig.hdTitle || scale <= 0) return;
    font = FontForScale(scale, 3);
    if (!font->font) return;
    glyph = code > 255 ? GlyphJp(renderer, font, (uint16_t)code) : Glyph(renderer, font, code);
    if (code > 255 && glyph) /* full-width (arrows, kana): centred in the cell the text gives it */
        x += (cell - glyph->advance) * 0.5f;
#ifdef PORT_HD_TEXT_SIMPLE_DRAW
    /* The console rasterizer has no game-cell clipping; UI glyphs have none. */
    DrawGlyph(renderer, glyph, x + offset, y + 8 * scale + offset, shadow, 255);
    DrawGlyph(renderer, glyph, x, y + 8 * scale, face, 255);
#else
    DrawGlyph(renderer, glyph, x + offset, y + 8 * scale + offset, shadow, 255, &unit, 0, 0, scale);
    DrawGlyph(renderer, glyph, x, y + 8 * scale, face, 255, &unit, 0, 0, scale);
#endif
}

/* A glyph's advance (output pixels) in the port panels' HD face, for laying their texts out (port_ui.c). */
float PortHdTextUiAdvance(SDL_Renderer* renderer, int code, float scale) {
    HdFont* font;
    HdGlyph* glyph;
    if (!gPortConfig.hdTitle || scale <= 0) return 0;
    font = FontForScale(scale, 3);
    if (!font->font) return 0;
    glyph = code > 255 ? GlyphJp(renderer, font, (uint16_t)code) : Glyph(renderer, font, code);
    return glyph ? (float)glyph->advance : 0;
}
