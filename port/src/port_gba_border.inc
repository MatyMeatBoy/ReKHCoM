/* The existing border art, cached once per selection/reset. No new art files.
 * Keep semantic decorations at native proportions; stretch only material/bevel. */
static SDL_Texture* BorderTexture(unsigned char* rgba, int width, int height) {
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, rgba, width * 4);
    SDL_Texture* texture = surface ? SDL_CreateTextureFromSurface(sRenderer, surface) : NULL;
    if (surface) SDL_DestroySurface(surface);
    if (texture && (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) ||
                    !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND))) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    return texture;
}

/* Alpha masks/positions are those of gen_gba_borders.py, not new artwork.
 * Copy the final native PNG pixels only where its existing motif was drawn.
 * This removes material/outer-frame fragments without recolouring the art. */
static const char* const kBorderHeart[] = {
    ".##...##.", "####.####", "#########", "#########", ".#######.", "..#####..", "...###...", "....#...."
};
static const char* const kBorderCrown[] = { "#...#...#", "#..###..#", "##.###.##", "#########", "#########", ".#######." };
static const char* const kBorderCard[] = {
    "..#.#.#..", ".#######.", "#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
    "#.......#", "#.....###", "#....####", "#########"
};
static void BorderCopyRect(unsigned char* dst, const unsigned char* src, int x, int y, int w, int h) {
    int row, col;
    for (row = y; row < y + h; row++) for (col = x; col < x + w; col++)
        if (col >= 0 && col < BORDER_W && row >= 0 && row < BORDER_H)
            memcpy(dst + (row * BORDER_W + col) * 4, src + (row * BORDER_W + col) * 4, 4);
}
static void BorderCopyPattern(unsigned char* dst, const unsigned char* src, const char* const* pattern,
                              int rows, int x, int y, int scale) {
    int row, col;
    for (row = 0; row < rows; row++) for (col = 0; col < 9; col++)
        if (pattern[row][col] == '#') BorderCopyRect(dst, src, x + col * scale, y + row * scale, scale, scale);
}
static void BorderAdaptivePixels(unsigned char* dst, const unsigned char* src, int selected) {
    int i, x;
    memset(dst, 0, BORDER_W * BORDER_H * 4);
    if (selected == 1) {
        for (i = 0; i < 4; i++) {
            int cx = i & 1 ? BORDER_W - 22 : 22, cy = i & 2 ? BORDER_H - 26 : 18;
            BorderCopyPattern(dst, src, kBorderHeart, 8, cx - 9, cy - 4, 2);
            BorderCopyPattern(dst, src, kBorderHeart, 8, cx - 8, cy - 3, 2);
            BorderCopyPattern(dst, src, kBorderCrown, 6, cx - 4, cy - 15, 1);
        }
        for (i = 0, x = 92; x < 302; i++, x += 24) {
            const char* const* motif = i & 1 ? kBorderCrown : kBorderHeart;
            int rows = i & 1 ? 6 : 8;
            BorderCopyPattern(dst, src, motif, rows, x, 9, 1);
            BorderCopyPattern(dst, src, motif, rows, x, BORDER_H - 18, 1);
        }
    } else if (selected == 2) {
        static const int spots[14][2] = {
            {8,10},{36,60},{12,112},{40,160},{14,190},{354,14},{326,66},{358,118},
            {328,168},{356,192},{100,4},{264,4},{150,194},{214,194}
        };
        for (i = 0; i < 14; i++) {
            int y = spots[i][1]; x = spots[i][0];
            BorderCopyPattern(dst, src, kBorderCard, 12, x + 1, y + 1, 2);
            BorderCopyPattern(dst, src, kBorderCard, 12, x, y, 2);
            BorderCopyRect(dst, src, x + 2, y + 6, 14, 12);
            BorderCopyPattern(dst, src, kBorderHeart, 8, x + 4, y + 8, 1);
        }
    } else {
        for (i = 0; i < 4; i++) {
            int cx = i & 1 ? BORDER_W - 24 : 24, cy = i & 2 ? BORDER_H - 22 : 16;
            BorderCopyPattern(dst, src, kBorderCrown, 6, cx - 9, cy - 6, 2);
            BorderCopyPattern(dst, src, kBorderCrown, 6, cx - 10, cy - 7, 2);
        }
        for (x = 80; x < 304; x += 16) {
            BorderCopyPattern(dst, src, kBorderHeart, 8, x, 12, 1);
            BorderCopyPattern(dst, src, kBorderHeart, 8, x, BORDER_H - 18, 1);
        }
    }
    BorderCopyRect(dst, src, 72, 24, 240, 4);
    BorderCopyRect(dst, src, 72, 188, 240, 4);
    BorderCopyRect(dst, src, 68, 28, 4, 160);
    BorderCopyRect(dst, src, 312, 28, 4, 160);
}

static int GbaBorderReady(void) {
    int selected = gPortConfig.gbaBorder;
    int width, height, channels, y;
    unsigned char material[BORDER_H * 4];
    unsigned char *pixels, *adaptive;
    SDL_Texture *art, *background, *ornaments;
    if (selected < 1 || selected > 3) {
        sBorderAttempted = 0;
        return 0;
    }
    if (!sRenderer) return 0;
    if (sBorderShown == selected && sBorderTex && sBorderMaterialTex && sBorderAdaptiveTex) {
        sBorderAttempted = selected;
        return 1;
    }
    if (sBorderAttempted == selected) return 0;
    sBorderAttempted = selected;
    pixels = stbi_load_from_memory(kGbaBorders[selected - 1], (int)kGbaBorderSizes[selected - 1],
                                  &width, &height, &channels, 4);
    if (!pixels) return 0;
    if (width != BORDER_W || height != BORDER_H) {
        stbi_image_free(pixels);
        return 0;
    }
    /* x52..67 is material beside the aperture, not semantic art. Select an
     * existing palette colour by row majority: isolated stars never become
     * stretched stripes, and no map-card image is used as a fill texture. */
    for (y = 0; y < BORDER_H; y++) {
        int x, best = 52, bestCount = 0;
        for (x = 52; x < 68; x++) {
            int j, count = 0;
            for (j = 52; j < 68; j++)
                if (memcmp(pixels + (y * width + x) * 4, pixels + (y * width + j) * 4, 4) == 0) count++;
            if (count > bestCount) { best = x; bestCount = count; }
        }
        memcpy(material + y * 4, pixels + (y * width + best) * 4, 4);
    }
    adaptive = malloc(BORDER_W * BORDER_H * 4);
    if (!adaptive) { stbi_image_free(pixels); return 0; }
    BorderAdaptivePixels(adaptive, pixels, selected);
    if (selected == 1) {
        BorderMapCards(pixels, width, height);
        BorderMapCards(adaptive, width, height);
    }
    background = BorderTexture(material, 1, BORDER_H);
    art = BorderTexture(pixels, width, height);
    ornaments = BorderTexture(adaptive, width, height);
    free(adaptive);
    stbi_image_free(pixels);
    if (!background || !art || !ornaments) {
        if (background) SDL_DestroyTexture(background);
        if (art) SDL_DestroyTexture(art);
        if (ornaments) SDL_DestroyTexture(ornaments);
        return 0; /* A missing replacement never shrinks/hides the game. */
    }
    if (sBorderTex) SDL_DestroyTexture(sBorderTex);
    if (sBorderMaterialTex) SDL_DestroyTexture(sBorderMaterialTex);
    if (sBorderAdaptiveTex) SDL_DestroyTexture(sBorderAdaptiveTex);
    sBorderTex = art;
    sBorderMaterialTex = background;
    sBorderAdaptiveTex = ornaments;
    sBorderShown = selected;
    return 1;
}

/* Bounded geometry: at most eight quads in one batch (48 emitted vertices
 * in the SDL guest bridge, below its300-vertex submission limit). */
static void BorderQuad(SDL_Vertex* vertices, int* indices, int* count,
                       SDL_FRect source, SDL_FRect destination, float textureW, float textureH) {
    int i, v = *count * 4, at = *count * 6;
    static const int corner[6] = { 0, 1, 2, 0, 2, 3 };
    if (destination.w <= 0 || destination.h <= 0) return;
    vertices[v] = (SDL_Vertex){ { destination.x, destination.y }, { 1, 1, 1, 1 },
                                { source.x / textureW, source.y / textureH } };
    vertices[v + 1] = (SDL_Vertex){ { destination.x + destination.w, destination.y }, { 1, 1, 1, 1 },
                                    { (source.x + source.w) / textureW, source.y / textureH } };
    vertices[v + 2] = (SDL_Vertex){ { destination.x + destination.w, destination.y + destination.h }, { 1, 1, 1, 1 },
                                    { (source.x + source.w) / textureW, (source.y + source.h) / textureH } };
    vertices[v + 3] = (SDL_Vertex){ { destination.x, destination.y + destination.h }, { 1, 1, 1, 1 },
                                    { source.x / textureW, (source.y + source.h) / textureH } };
    for (i = 0; i < 6; i++) indices[at + i] = v + corner[i];
    (*count)++;
}

static void DrawGbaBorder(int width, int height, const PortGbaBorderRect* aperture) {
    SDL_Vertex vertices[32];
    int indices[48], count = 0;
    float x = aperture->x, y = aperture->y, w = aperture->w, h = aperture->h;
    float right = x + w, bottom = y + h, s, cx = x + w / 2, cy = y + h / 2;
    if (!GbaBorderReady() || width <= 0 || height <= 0) return;
    if (PortGbaBorderLegacy(width, height, aperture)) {
        SDL_FRect full = { 0, 0, (float)width, (float)height };
        SDL_RenderTexture(sRenderer, sBorderTex, NULL, &full);
        return; /* Exact original texture and geometry, including six cards. */
    }
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, 0, 1, y * BORDER_H / height },
               (SDL_FRect){ 0, 0, (float)width, y }, 1, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, bottom * BORDER_H / height, 1, (height - bottom) * BORDER_H / height },
               (SDL_FRect){ 0, bottom, (float)width, height - bottom }, 1, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, y * BORDER_H / height, 1, h * BORDER_H / height },
               (SDL_FRect){ 0, y, x, h }, 1, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, y * BORDER_H / height, 1, h * BORDER_H / height },
               (SDL_FRect){ right, y, width - right, h }, 1, BORDER_H);
    if (count && !SDL_RenderGeometry(sRenderer, sBorderMaterialTex, vertices, count * 4, indices, count * 6)) {
        sRendererResetPending = 1;
        return;
    }
    s = PortGbaBorderDecorationScale(width, height, aperture);
    if (s <= 0) return;
    count = 0;
    /* Four semantic strips use the identical X/Y scale. They fit wholly
     * outside the aperture; only four simple bevels span the longer edges. */
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 72, 0, 240, 28 },
               (SDL_FRect){ cx - 120 * s, y - 28 * s, 240 * s, 28 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 72, 188, 240, 28 },
               (SDL_FRect){ cx - 120 * s, bottom, 240 * s, 28 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, 0, 72, 216 },
               (SDL_FRect){ x - 72 * s, cy - 108 * s, 72 * s, 216 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 312, 0, 72, 216 },
               (SDL_FRect){ right, cy - 108 * s, 72 * s, 216 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 72, 24, 240, 4 },
               (SDL_FRect){ x, y - 4 * s, w, 4 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 72, 188, 240, 4 },
               (SDL_FRect){ x, bottom, w, 4 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 68, 28, 4, 160 },
               (SDL_FRect){ x - 4 * s, y, 4 * s, h }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 312, 28, 4, 160 },
               (SDL_FRect){ right, y, 4 * s, h }, BORDER_W, BORDER_H);
    if (count && !SDL_RenderGeometry(sRenderer, sBorderAdaptiveTex, vertices, count * 4, indices, count * 6))
        sRendererResetPending = 1;
}
