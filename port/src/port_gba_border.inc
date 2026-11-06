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

static int GbaBorderReady(void) {
    int selected = gPortConfig.gbaBorder;
    int width, height, channels, y;
    unsigned char material[BORDER_H * 4];
    unsigned char* pixels;
    SDL_Texture *art, *background;
    if (selected < 1 || selected > 3) {
        sBorderAttempted = 0;
        return 0;
    }
    if (!sRenderer) return 0;
    if (sBorderShown == selected && sBorderTex && sBorderMaterialTex) {
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
    background = BorderTexture(material, 1, BORDER_H);
    if (selected == 1) BorderMapCards(pixels, width, height);
    art = BorderTexture(pixels, width, height);
    stbi_image_free(pixels);
    if (!background || !art) {
        if (background) SDL_DestroyTexture(background);
        if (art) SDL_DestroyTexture(art);
        return 0; /* A missing replacement never shrinks/hides the game. */
    }
    if (sBorderTex) SDL_DestroyTexture(sBorderTex);
    if (sBorderMaterialTex) SDL_DestroyTexture(sBorderMaterialTex);
    sBorderTex = art;
    sBorderMaterialTex = background;
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
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, 0, BORDER_W, 28 },
               (SDL_FRect){ cx - 192 * s, y - 28 * s, 384 * s, 28 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, 188, BORDER_W, 28 },
               (SDL_FRect){ cx - 192 * s, bottom, 384 * s, 28 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 0, 28, 72, 160 },
               (SDL_FRect){ x - 72 * s, cy - 80 * s, 72 * s, 160 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 312, 28, 72, 160 },
               (SDL_FRect){ right, cy - 80 * s, 72 * s, 160 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 72, 24, 240, 4 },
               (SDL_FRect){ x, y - 4 * s, w, 4 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 72, 188, 240, 4 },
               (SDL_FRect){ x, bottom, w, 4 * s }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 68, 28, 4, 160 },
               (SDL_FRect){ x - 4 * s, y, 4 * s, h }, BORDER_W, BORDER_H);
    BorderQuad(vertices, indices, &count, (SDL_FRect){ 312, 28, 4, 160 },
               (SDL_FRect){ right, y, 4 * s, h }, BORDER_W, BORDER_H);
    if (count && !SDL_RenderGeometry(sRenderer, sBorderTex, vertices, count * 4, indices, count * 6))
        sRendererResetPending = 1;
}
