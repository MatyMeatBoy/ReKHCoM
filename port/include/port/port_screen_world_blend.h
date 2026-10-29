#ifndef PORT_SCREEN_WORLD_BLEND_H
#define PORT_SCREEN_WORLD_BLEND_H

/* Renderer-owned, bounded scratch for a screen's 16/16 additive light.
 * The PPU keeps an independent native result until this pass succeeds. */
typedef struct PortScreenWorldBlend {
    SDL_Texture* texture;
    int width;
    int height;
    int blockedWidth, blockedHeight;
    int failed;
} PortScreenWorldBlend;

static void PortScreenWorldBlendIdle(PortScreenWorldBlend* state) {
    state->blockedWidth = state->blockedHeight = state->failed = 0;
}

static void PortScreenWorldBlendFailed(PortScreenWorldBlend* state, const char* reason) {
    if (!state->failed)
        PortLog("HD world light: native illustration/light fallback (%s)", reason);
    state->failed = 1;
}

static int PortScreenWorldBlendPrepareSize(PortScreenWorldBlend* state, SDL_Renderer* renderer,
                                          const uint32_t* light, int width, int height) {
    if (!light || width < GBA_SCREEN_W || width > PORT_MAX_W || height < GBA_SCREEN_H ||
        (size_t)width * height > PORT_MAX_W * GBA_SCREEN_H) return 0;
    if (!state->texture && state->blockedWidth == width && state->blockedHeight == height) return 0;
#ifdef PORT_GBA_RELOC
    /* Old hosts reduce every blend mode to a boolean. Their explicit GPU
     * protocol is required before asking for addition instead of alpha. */
    if (SDL_GetNumberProperty(SDL_GetRendererProperties(renderer),
                              SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0) <= 0) {
        PortScreenWorldBlendFailed(state, "host has no additive-blend protocol");
        return 0;
    }
#endif
    if (state->texture && (state->width != width || state->height != height)) {
        SDL_DestroyTexture(state->texture);
        state->texture = NULL;
        state->width = state->height = 0;
    }
    if (!state->texture) {
        state->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!state->texture) {
            state->blockedWidth = width;
            state->blockedHeight = height;
            PortScreenWorldBlendFailed(state, "texture allocation");
            return 0;
        }
        state->width = width;
        state->height = height;
        state->blockedWidth = state->blockedHeight = 0;
    }
    if (!SDL_SetTextureBlendMode(state->texture, SDL_BLENDMODE_ADD) ||
        !SDL_SetTextureScaleMode(state->texture, SDL_SCALEMODE_PIXELART) ||
        !SDL_UpdateTexture(state->texture, NULL, light, width * sizeof(*light))) {
        PortScreenWorldBlendFailed(state, "blend setup or upload");
        return 0;
    }
    return 1;
}

static int PortScreenWorldBlendPrepare(PortScreenWorldBlend* state, SDL_Renderer* renderer,
                                      const uint32_t* light, int width) {
    return PortScreenWorldBlendPrepareSize(state, renderer, light, width, GBA_SCREEN_H);
}

static int PortScreenWorldBlendDraw(PortScreenWorldBlend* state, SDL_Renderer* renderer,
                                   const SDL_FRect* destination) {
    if (!state->texture || !SDL_RenderTexture(renderer, state->texture, NULL, destination)) {
        PortScreenWorldBlendFailed(state, "light draw");
        return 0;
    }
    state->failed = 0;
    return 1;
}
#endif
