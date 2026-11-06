/* Two one-dimensional masks. Geometry changes upload O(width + height)
 * pixels once; steady frames use two existing SDL texture draws only. */
static SDL_Texture* LcdGridTexture(const uint32_t* pixels, int width, int height) {
    SDL_Texture* texture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STATIC, width, height);
    if (texture && (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) ||
                    !SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) ||
                    !SDL_UpdateTexture(texture, NULL, pixels, width * 4))) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    return texture;
}
static int LcdGridReady(const PortLcdGridKey* key) {
    SDL_Texture *horizontal, *vertical;
    uint32_t* pixels;
    int i, count;
    if (sLcdGridReady && sLcdGridXTex && sLcdGridYTex && PortLcdGridSame(key, &sLcdGridKey)) {
        sLcdGridAttempted = 0;
        return 1;
    }
    if (sLcdGridAttempted && PortLcdGridSame(key, &sLcdGridAttemptKey)) return 0;
    sLcdGridAttemptKey = *key;
    sLcdGridAttempted = 1;
    count = key->width > key->height ? key->width : key->height;
    pixels = malloc((size_t)count * sizeof(*pixels));
    if (!pixels) return 0;
    for (i = 0; i < key->width; i++) pixels[i] = (uint32_t)PortLcdGridAlpha(i, key->phaseX, key->pitchX) << 24;
    horizontal = LcdGridTexture(pixels, key->width, 1);
    for (i = 0; i < key->height; i++) pixels[i] = (uint32_t)PortLcdGridAlpha(i, key->phaseY, key->pitchY) << 24;
    vertical = LcdGridTexture(pixels, 1, key->height);
    free(pixels);
    if (!horizontal || !vertical) {
        if (horizontal) SDL_DestroyTexture(horizontal);
        if (vertical) SDL_DestroyTexture(vertical);
        return 0; /* Never hide the game or render a stale grid after failure. */
    }
    if (sLcdGridXTex) SDL_DestroyTexture(sLcdGridXTex);
    if (sLcdGridYTex) SDL_DestroyTexture(sLcdGridYTex);
    sLcdGridXTex = horizontal; sLcdGridYTex = vertical;
    sLcdGridKey = *key;
    sLcdGridReady = 1;
    sLcdGridAttempted = 0;
    return 1;
}
static void DrawLcdGrid(int width, int height, float originX, float originY, float pitchX, float pitchY) {
    PortLcdGridKey key;
    SDL_FRect destination;
    int intensity = gPortConfig.lcdGrid;
    Uint8 alpha;
    if (intensity <= 0) { sLcdGridAttempted = 0; return; }
    if (!sRenderer || !PortLcdGridMakeKey(&key, width, height, originX, originY, pitchX, pitchY) ||
        !LcdGridReady(&key)) return;
    if (intensity > 100) intensity = 100;
    alpha = (Uint8)((intensity * 255 + 50) / 100);
    destination = (SDL_FRect){0, 0, (float)width, (float)height};
    if (!SDL_SetTextureAlphaMod(sLcdGridXTex, alpha) || !SDL_SetTextureAlphaMod(sLcdGridYTex, alpha) ||
        !SDL_RenderTexture(sRenderer, sLcdGridXTex, NULL, &destination) ||
        !SDL_RenderTexture(sRenderer, sLcdGridYTex, NULL, &destination))
        sRendererResetPending = 1;
}
