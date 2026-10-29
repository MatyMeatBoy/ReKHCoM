/* One bounded native foreground texture, owned by the HD renderer lifecycle.
   The caller still has the complete native picture and restores it on false. */
static SDL_Texture* sJiminyForeground;

int PortHdJiminyBookDraw(SDL_Renderer* renderer, int id, const uint32_t* foreground,
                         float ox, float oy, float scale, int dark, int white) {
    const HdSpriteDef* def;
    HdSpriteState* state;
    HdSpriteDraw q = {0};
    SDL_FRect dst = { ox, oy, 240 * scale, 160 * scale };
    float shade, light;
    if (!renderer || !foreground || scale <= 0 || id < 1 || id > HD_SPRITE_COUNT) return 0;
    sRenderer = renderer;
    def = &kHdSprites[id - 1]; state = &sState[id - 1];
    if (def->group != 5 || def->x0 != 0 || def->y0 != 16 || def->w != 240 || def->h != 144 ||
        !GroupOn(def->group) || !SpriteReady(id - 1)) return 0;
    dark = SDL_clamp(dark, 0, 255); white = SDL_clamp(white, 0, 255);
    if (white && !SpriteTextureReady(state->white)) return 0;
    if (!sJiminyForeground) {
        sJiminyForeground = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                               SDL_TEXTUREACCESS_STREAMING, 240, 160);
        if (!sJiminyForeground) return 0;
        if (!SDL_SetTextureBlendMode(sJiminyForeground, SDL_BLENDMODE_BLEND)) {
            SDL_DestroyTexture(sJiminyForeground); sJiminyForeground = NULL; return 0;
        }
    }
    /* Options may change while this cached texture remains alive. Keep the
       foreground's filter identical to the native picture's selected filter. */
    if (!SDL_SetTextureScaleMode(sJiminyForeground, gPortConfig.filter == PORT_FILTER_LINEAR ?
            (SDL_ScaleMode)PortSmoothScaleMode() : SDL_SCALEMODE_PIXELART) ||
        !SDL_UpdateTexture(sJiminyForeground, NULL, foreground, 240 * 4) ||
        !SpriteTextureReady(sJiminyForeground)) return 0;
    q.id = id - 1; q.alpha = 255;
    q.x[1] = q.x[2] = 240;
    q.y[0] = q.y[1] = 16; q.y[2] = q.y[3] = 160;
    q.u1 = q.v1 = 1;
    shade = 1.0f - dark / 255.0f; light = white / 255.0f;
    if (light < 1 && !Quad(state->image, &q, ox, oy, scale, (SDL_FColor){shade,shade,shade,1})) return 0;
    if (light > 0 && !Quad(state->white, &q, ox, oy, scale, (SDL_FColor){1,1,1,light})) return 0;
    return SDL_RenderTexture(renderer, sJiminyForeground, NULL, &dst);
}
