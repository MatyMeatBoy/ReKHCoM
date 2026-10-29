/* Included after the UI's text helpers. No game state or navigation changes. */
static uint32_t JiminyFooterFade(uint32_t color, int dark, int white) {
    unsigned result = 0xFF000000u;
    int shift;
    dark = SDL_clamp(dark, 0, 255); white = SDL_clamp(white, 0, 255);
    for (shift = 0; shift <= 16; shift += 8) {
        int channel = (color >> shift) & 255;
        channel = channel * (255 - dark) / 255;
        channel += (255 - channel) * white / 255;
        result |= (unsigned)channel << shift;
    }
    return result;
}

void PortUiJournalFooter(SDL_Renderer* renderer, int width, float y, float height,
                         float scale, int detail, int dark, int white) {
    static const char* const open[LANG_COUNT] = { "Open", "Ouvrir", "Öffnen", "Apri", "Abrir", "開く" };
    static const char* const entry[LANG_COUNT] = { "Entry", "Entrée", "Eintrag", "Voce", "Entrada", "項目" };
    static const char* const back[LANG_COUNT] = { "Back", "Retour", "Zurück", "Indietro", "Volver", "戻る" };
    static const char* const finish[LANG_COUNT] = { "Close", "Fermer", "Schließen", "Chiudi", "Cerrar", "閉じる" };
    static uint32_t pixels[512 * 14];
    char a[32] = "A", b[32] = "B", start[32] = "START", left[32] = "L", right[32] = "R", text[384];
    int codes[192], count = 0, i, l = Lang(), nativeWidth, ready = 1;
    float natural = 0, pen, fontScale, fit;
    const char* p;
    SDL_FRect area = { 0, y, (float)width, height };
    uint32_t ground = JiminyFooterFade(0xFF9CC65Au, dark, white);
    uint32_t face = JiminyFooterFade(0xFF354C31u, dark, white);
    uint32_t shadow = JiminyFooterFade(0xFFD8E6CBu, dark, white);
    extern int PortHdTextUiGlyphReady(SDL_Renderer*, int, float);
    extern float PortHdTextUiAdvance(SDL_Renderer*, int, float);
    extern void PortHdTextUiGlyph(SDL_Renderer*, int, float, float, float, float, uint32_t, uint32_t);
    if (!renderer || width <= 0 || height <= 0 || scale <= 0) return;
    PortControllerActionLabel(PORT_ACT_A, a, sizeof(a));
    PortControllerActionLabel(PORT_ACT_B, b, sizeof(b));
    PortControllerActionLabel(PORT_ACT_START, start, sizeof(start));
    PortControllerActionLabel(PORT_ACT_L, left, sizeof(left));
    PortControllerActionLabel(PORT_ACT_R, right, sizeof(right));
    if (detail)
        SDL_snprintf(text, sizeof(text), "%s/%s %s   %s %s   %s %s", left, right, entry[l], b, back[l], start, finish[l]);
    else
        SDL_snprintf(text, sizeof(text), "%s %s   %s %s   %s %s", a, open[l], b, back[l], start, finish[l]);
    /* Parse with the same Unicode/SJIS conversion as the existing panels. */
    p = text;
    while (*p && count < (int)SDL_arraysize(codes)) {
        int code = NextChar(&p);
#ifdef VERSION_JP
        code = code & JP_CHAR ? PortCodepointToSjis((unsigned)(code & 0xFFFF)) : code & 0xFFFF;
#endif
        codes[count++] = code;
    }
    fontScale = scale * 0.85f;
    for (i = 0; i < count; i++) {
        if (!PortHdTextUiGlyphReady(renderer, codes[i], fontScale)) ready = 0;
        natural += PortHdTextUiAdvance(renderer, codes[i], fontScale);
    }
    nativeWidth = PortUiTextWidth(text);
    fit = ready && natural > 0 ? ((width - 8 * scale) / natural) :
          nativeWidth > 0 ? ((width - 8 * scale) / (nativeWidth * fontScale)) : 1;
    if (fit < 1) {
        fontScale *= SDL_max(fit, 0.01f);
        for (i = 0; i < count; i++)
            if (!PortHdTextUiGlyphReady(renderer, codes[i], fontScale)) ready = 0;
    }
    if (!ready && nativeWidth > 0 && nativeWidth * fontScale > width - 8 * scale)
        fontScale = SDL_max((width - 8 * scale) / nativeWidth, 0.01f);
    SDL_SetRenderDrawColor(renderer, (Uint8)(ground >> 16), (Uint8)(ground >> 8), (Uint8)ground, 255);
    SDL_RenderFillRect(renderer, &area);
    pen = 4 * scale;
    if (ready) {
        for (i = 0; i < count; i++) {
            float advance = PortHdTextUiAdvance(renderer, codes[i], fontScale);
            PortHdTextUiGlyph(renderer, codes[i], pen, y + (height - 10 * fontScale) * 0.5f,
                              fontScale, advance, face, shadow);
            pen += advance;
        }
    } else {
        int row, x;
        /* Texture-free fallback: bounded runs of native pixels, never hide
           the help row because a font allocation/upload was rejected. */
        PortUiRenderTextLine(pixels, 512, 14, text, face);
        for (row = 0; row < 14; row++) {
            for (x = 0; x < SDL_min(nativeWidth, 512);) {
                uint32_t color = pixels[row * 512 + x];
                int end = x + 1;
                SDL_FRect run;
                if (!color) { x++; continue; }
                while (end < SDL_min(nativeWidth, 512) && pixels[row * 512 + end] == color) end++;
                color = (color == face) ? face : shadow;
                SDL_SetRenderDrawColor(renderer, (Uint8)(color >> 16), (Uint8)(color >> 8), (Uint8)color, 255);
                run.x = pen + x * fontScale; run.y = y + (height - 10 * fontScale) * 0.5f + row * fontScale;
                run.w = (end - x) * fontScale; run.h = fontScale;
                SDL_RenderFillRect(renderer, &run);
                x = end;
            }
        }
    }
}
