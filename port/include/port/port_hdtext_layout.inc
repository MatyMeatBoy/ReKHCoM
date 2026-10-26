/* Shared HD layout: keep Windows and browser spacing/occlusion identical. */
static int CompareGlyphs(const void* a, const void* b) {
    const PortTextGlyph* p = a;
    const PortTextGlyph* q = b;

    if (p->y != q->y)
        return p->y - q->y;
    return p->x - q->x;
}

static void DrawGlyph(SDL_Renderer* r, HdGlyph* g, float x, float y, uint32_t color, int alpha,
                      const PortTextGlyph* cell, float originX, float originY, float scale) {
    SDL_FRect d;

    if (g->tex == NULL)
        return;
    d.x = SDL_floorf(x + 0.5f) + g->left;
    d.y = SDL_floorf(y + 0.5f) - g->top;
    d.w = (float)g->w;
    d.h = (float)g->h;
    SDL_SetTextureColorMod(g->tex, (color >> 16) & 255, (color >> 8) & 255, color & 255);
    SDL_SetTextureAlphaMod(g->tex, (Uint8)alpha);
    if (!cell->coverage) {
        SDL_RenderTexture(r, g->tex, NULL, &d);
    } else {
        SDL_Rect saved;
        int hadClip=SDL_RenderClipEnabled(r), row=0;
        SDL_GetRenderClipRect(r, &saved);
        /* Merge equal rows and contiguous columns. This clips the actual
         * glyph texture (including its shadow) behind the game's selector;
         * its alpha and reveal timing still come from the original frame. */
        while (row<16) {
            int end=row+1, col=0;
            uint16_t bits=cell->visible[row];
            while (end<16 && cell->visible[end]==bits) end++;
            while (col<16) {
                int first;
                SDL_Rect clip, intersection;
                while (col<16 && !(bits&(1u<<col))) col++;
                first=col;
                while (col<16 && (bits&(1u<<col))) col++;
                if (first==col) break;
                clip.x=(int)SDL_floorf(originX+(cell->x+first-4)*scale);
                clip.y=(int)SDL_floorf(originY+(cell->y+row-4)*scale);
                clip.w=(int)SDL_floorf(originX+(cell->x+col-4)*scale)-clip.x;
                clip.h=(int)SDL_floorf(originY+(cell->y+end-4)*scale)-clip.y;
                if (clip.w<=0 || clip.h<=0) continue;
                if (hadClip) {
                    if (!SDL_GetRectIntersection(&clip,&saved,&intersection)) continue;
                    clip=intersection;
                }
                SDL_SetRenderClipRect(r,&clip);
                SDL_RenderTexture(r,g->tex,NULL,&d);
            }
            row=end;
        }
        SDL_SetRenderClipRect(r,hadClip?&saved:NULL);
    }
}

/* Experimental card digits: outline glyphs fitted to their native ink box,
   including the card's affine animation. Values still come from game code. */
void PortHdTextHudDigits(SDL_Renderer* renderer, float originX, float originY, float scale) {
    PortHudDigit digits[128];
    HdFont* font;
    int i, n;
    if (!gPortConfig.hdHud) return;
    n = PortPpuHudDigits(digits, 128);
    if (!n) return;
    font = FontForScale(8.0f, 2);
    if (!font->font) return;
    for (i = 0; i < n; i++) {
        HdGlyph* g = Glyph(renderer, font, digits[i].code);
        SDL_FRect box = {originX + digits[i].x * scale, originY + digits[i].y * scale,
                         digits[i].w * scale, digits[i].h * scale};
        uint32_t color = digits[i].color;
        if (!g->tex || !digits[i].w || !digits[i].h) continue;
        if (SDL_getenv("KHCOM_HDHUDLOG")) PortLog("HD HUD digit %c: slot %d box %d,%d %dx%d", digits[i].code,
                                                digits[i].slot, digits[i].x, digits[i].y, digits[i].w, digits[i].h);
        SDL_SetTextureScaleMode(g->tex, SDL_strcmp(SDL_GetRendererName(renderer), "software") == 0 ?
                               SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
        SDL_SetTextureColorMod(g->tex, color >> 16, color >> 8, color);
        SDL_SetTextureAlphaMod(g->tex, 255);
        {
            int x, y;
            for (y = 0; y < digits[i].h; y++) for (x = 0; x < digits[i].w; x++) {
                SDL_FRect source = {x * g->w / (float)digits[i].w, y * g->h / (float)digits[i].h,
                                    g->w / (float)digits[i].w, g->h / (float)digits[i].h};
                SDL_FRect cell = {box.x + x * scale, box.y + y * scale, scale, scale};
                if (PortPpuHudDigitVisible(i, digits[i].x + x, digits[i].y + y))
                    SDL_RenderTexture(renderer, g->tex, &source, &cell);
            }
        }
    }
}

static void DrawFlowing(SDL_Renderer* renderer, PortTextGlyph* glyphs, int n, float originX, float originY, float scale) {
    const uint16_t* widths = PortLatinGlyphWidths();
    HdFont* font;
    float shadow = SDL_max(1.0f, SDL_floorf(scale * 0.5f));
    int i, j, pass, lineY, lineGameEnd = 0;
    float space, lineEnd;

    font = FontForScale(scale, 0);
    if (font->font == NULL)
        return;

    /* Japanese glyphs keep their cells on the game's grid; the rest flow. */
    for (i = j = 0; i < n; i++) {
        if (glyphs[i].code > 0xFF) {
            HdGlyph* g = GlyphJp(renderer, font, glyphs[i].code);
            float x = originX + glyphs[i].x * scale + (HD_TEXT_JP_CELL * scale - g->advance) * 0.5f;
            float base = originY + (glyphs[i].y + HD_TEXT_BASELINE) * scale;
            int alpha = glyphs[i].semi ? glyphs[i].semi : 0xFF;
            DrawGlyph(renderer, g, x + shadow, base + shadow, glyphs[i].shadow, alpha, &glyphs[i], originX, originY, scale);
            DrawGlyph(renderer, g, x, base, glyphs[i].face, alpha, &glyphs[i], originX, originY, scale);
        } else {
            glyphs[j++] = glyphs[i];
        }
    }
    n = j;
    if (n == 0)
        return;

    /* Words: glyphs on one line, each where the previous one ends. */
    space = (float)Glyph(renderer, font, ' ')->advance;
    lineY = -1;
    lineEnd = 0;
    for (i = 0; i < n; i = j) {
        float start, pen, baseline;

        for (j = i + 1; j < n; j++) {
            const PortTextGlyph* p = &glyphs[j - 1];
            if (glyphs[j].y != p->y || glyphs[j].x != p->x + widths[p->code & 255])
                break;
        }
        start = originX + glyphs[i].x * scale;
        if (glyphs[i].y == lineY) {
            /* Words of a sentence follow each other with the font's own
               space; a wider gap is a column and keeps its place. */
            int gap = glyphs[i].x - lineGameEnd, gameSpace = SDL_max(1, widths[' ']);
            if (gap <= gameSpace * 2)
                start = lineEnd + space * SDL_max(1, (gap + gameSpace / 2) / gameSpace);
            else if (start < lineEnd + space)
                start = lineEnd + space;
        }
        baseline = originY + (glyphs[i].y + HD_TEXT_BASELINE) * scale;
        for (pass = 0; pass < 2; pass++) {
            int t;
            pen = start;
            for (t = i; t < j; t++) {
                HdGlyph* g = Glyph(renderer, font, glyphs[t].code);
                int alpha = glyphs[t].semi ? glyphs[t].semi : 0xFF;
                if (pass == 0)
                    DrawGlyph(renderer, g, pen + shadow, baseline + shadow, glyphs[t].shadow, alpha, &glyphs[t], originX, originY, scale);
                else
                    DrawGlyph(renderer, g, pen, baseline, glyphs[t].face, alpha, &glyphs[t], originX, originY, scale);
                pen += g->advance;
            }
        }
        lineY = glyphs[i].y;
        lineEnd = pen;
        lineGameEnd = glyphs[j - 1].x + widths[glyphs[j - 1].code & 255];
    }
}

/* Movie subtitles: each line is centred on the game's own line, so the
   outline font's other widths keep it where the cartridge puts it. */
static void DrawSubtitles(SDL_Renderer* renderer, PortTextGlyph* glyphs, int n, float originX, float originY,
                          float scale) {
    const uint16_t* widths = PortLatinGlyphWidths();
    HdFont* font = FontForScale(scale, 1);
    float shadow = SDL_max(1.0f, SDL_floorf(scale * 0.5f));
    float space;
    int i, j, gameSpace = SDL_max(1, widths[' ']);

    if (font->font == NULL)
        return;
    space = (float)Glyph(renderer, font, ' ')->advance;
    for (i = 0; i < n; i = j) {
        float hdWidth = 0, pen, baseline;
        int t, prevEnd = glyphs[i].x, gameStart = glyphs[i].x, gameEnd;

        for (j = i; j < n && glyphs[j].y == glyphs[i].y; j++) {
            int jp = glyphs[j].code > 0xFF, gap = glyphs[j].x - prevEnd;
            if (j > i && gap > 0)
                hdWidth += space * SDL_max(1, (gap + gameSpace / 2) / gameSpace);
            hdWidth += jp ? HD_TEXT_JP_CELL * scale : (float)Glyph(renderer, font, glyphs[j].code)->advance;
            prevEnd = glyphs[j].x + (jp ? HD_TEXT_JP_CELL : widths[glyphs[j].code == 0x97 ? 25 : glyphs[j].code & 255]);
        }
        gameEnd = prevEnd;
        pen = originX + ((gameStart + gameEnd) * 0.5f) * scale - hdWidth * 0.5f;
        baseline = originY + (glyphs[i].y + HD_TEXT_BASELINE) * scale;
        prevEnd = gameStart;
        for (t = i; t < j; t++) {
            int jp = glyphs[t].code > 0xFF, gap = glyphs[t].x - prevEnd, alpha = glyphs[t].semi ? glyphs[t].semi : 0xFF;
            if (t > i && gap > 0)
                pen += space * SDL_max(1, (gap + gameSpace / 2) / gameSpace);
            if (jp) {
                HdGlyph* g = GlyphJp(renderer, font, glyphs[t].code);
                float x = pen + (HD_TEXT_JP_CELL * scale - g->advance) * 0.5f;
                DrawGlyph(renderer, g, x + shadow, baseline + shadow, glyphs[t].shadow, alpha, &glyphs[t], originX, originY, scale);
                DrawGlyph(renderer, g, x, baseline, glyphs[t].face, alpha, &glyphs[t], originX, originY, scale);
                pen += HD_TEXT_JP_CELL * scale;
                prevEnd = glyphs[t].x + HD_TEXT_JP_CELL;
            } else {
                HdGlyph* g = Glyph(renderer, font, glyphs[t].code);
                DrawGlyph(renderer, g, pen + shadow, baseline + shadow, glyphs[t].shadow, alpha, &glyphs[t], originX, originY, scale);
                DrawGlyph(renderer, g, pen, baseline, glyphs[t].face, alpha, &glyphs[t], originX, originY, scale);
                pen += g->advance;
                prevEnd = glyphs[t].x + widths[glyphs[t].code == 0x97 ? 25 : glyphs[t].code & 255];
            }
        }
    }
}

/* Small numeric HUD cells (EXP and next-level counters). Keep each cell in
   place instead of applying dialogue word spacing to tabular numbers. */
static void DrawSmallNumbers(SDL_Renderer* renderer, PortTextGlyph* glyphs, int n,
                             float originX, float originY, float scale) {
    float shadow = SDL_max(1.0f, SDL_floorf(scale * 0.5f));
    int i;
    for (i = 0; i < n; i++) {
        /* each number at the height and baseline of its own pixel digit */
        int cap = glyphs[i].cap ? glyphs[i].cap : 5, base = glyphs[i].base ? glyphs[i].base : 7;
        HdFont* font = FontForScale(scale * (cap / HD_TEXT_CAP), 2);
        HdGlyph* glyph;
        float x, baseline;
        if (font->font == NULL)
            return;
        glyph = Glyph(renderer, font, glyphs[i].code);
        x = originX + glyphs[i].x * scale + (8.0f * scale - glyph->advance) * 0.5f;
        baseline = originY + (glyphs[i].y + base) * scale;
        int alpha = glyphs[i].semi ? glyphs[i].semi : 0xFF;
        DrawGlyph(renderer, glyph, x + shadow, baseline + shadow, glyphs[i].shadow, alpha, &glyphs[i], originX, originY, scale);
        DrawGlyph(renderer, glyph, x, baseline, glyphs[i].face, alpha, &glyphs[i], originX, originY, scale);
    }
}

void PortHdTextDraw(SDL_Renderer* renderer, int hudLayer, float originX, float originY, float scale) {
    static PortTextGlyph glyphs[HD_TEXT_MAX_GLYPHS];
    int n, i, flowing = 0, subs, smallCount;

    if (!gPortConfig.hdText || scale <= 0.0f)
        return;
    n = PortPpuTextGlyphs(glyphs, HD_TEXT_MAX_GLYPHS, hudLayer);
    if (n == 0)
        return;
    SDL_qsort(glyphs, n, sizeof(glyphs[0]), CompareGlyphs);
    /* style 0 first (in place), then the subtitles after them */
    {
        static PortTextGlyph sub[HD_TEXT_MAX_GLYPHS];
        static PortTextGlyph small[HD_TEXT_MAX_GLYPHS];
        for (i = subs = smallCount = 0; i < n; i++) {
            if (glyphs[i].style == 2)
                small[smallCount++] = glyphs[i];
            else if (glyphs[i].style)
                sub[subs++] = glyphs[i];
            else
                glyphs[flowing++] = glyphs[i];
        }
        /* Opt-in evidence of the glyph classes actually emitted by the PPU. */
        {
            const char* trace = SDL_getenv("KHCOM_HD_TEXT_TRACE");
            while (trace && *trace) {
                const char* comma = SDL_strchr(trace, ',');
                if ((uint32_t)SDL_atoi(trace) == PortFrameCount()) {
                    PortLog("hd-text: frame=%u layer=%d flowing=%d subtitles=%d small=%d",
                            (unsigned)PortFrameCount(), hudLayer, flowing, subs, smallCount);
                    break;
                }
                trace = comma ? comma + 1 : NULL;
            }
        }
        if (flowing > 0)
            DrawFlowing(renderer, glyphs, flowing, originX, originY, scale);
        if (subs > 0)
            DrawSubtitles(renderer, sub, subs, originX, originY, scale);
        if (smallCount > 0)
            DrawSmallNumbers(renderer, small, smallCount, originX, originY, scale);
    }
}

/* Shift-JIS code of a Unicode character (two-byte codes only), 0 if none:
   the options overlay draws Japanese with the cartridge's own font. */

#include "port/port_hdtext_ui.inc"
