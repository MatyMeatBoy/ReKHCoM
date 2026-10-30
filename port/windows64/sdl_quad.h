/* Axis-aligned bridge copies use SDL's rectangle path. Large textured
 * triangles in the software backend lose coverage at1080p; the ordinary
 * copy path also avoids triangulating every full-screen scene layer. */
static int HostDrawSdlQuad(SDL_Texture* texture, const HostVertex* v, int n, float shift) {
    SDL_FRect src, dst;
    SDL_FlipMode flip = SDL_FLIP_NONE;
    float tw, th, u0, u1, v0, v1;
    int i;
    if (!texture || n != 6 || v[1].y != v[0].y || v[2].x != v[0].x ||
        v[5].x != v[1].x || v[5].y != v[2].y ||
        v[3].x != v[1].x || v[3].y != v[1].y ||
        v[4].x != v[2].x || v[4].y != v[2].y ||
        v[1].v != v[0].v || v[2].u != v[0].u ||
        v[5].u != v[1].u || v[5].v != v[2].v ||
        v[3].u != v[1].u || v[3].v != v[1].v ||
        v[4].u != v[2].u || v[4].v != v[2].v ||
        v[1].x <= v[0].x || v[2].y <= v[0].y)
        return 0;
    for (i = 1; i < 6; i++) if (v[i].argb != v[0].argb) return 0;
    if (!SDL_GetTextureSize(texture, &tw, &th)) return 0;
    u0 = v[0].u - shift; u1 = v[1].u - shift;
    v0 = v[0].v; v1 = v[2].v;
    if (u0 < 0 || u0 > 1 || u1 < 0 || u1 > 1 || v0 < 0 || v0 > 1 || v1 < 0 || v1 > 1)
        return 0;
    if (u1 < u0) flip = (SDL_FlipMode)(flip | SDL_FLIP_HORIZONTAL);
    if (v1 < v0) flip = (SDL_FlipMode)(flip | SDL_FLIP_VERTICAL);
    src = (SDL_FRect){fminf(u0,u1)*tw, fminf(v0,v1)*th, fabsf(u1-u0)*tw, fabsf(v1-v0)*th};
    dst = (SDL_FRect){v[0].x, v[0].y, v[1].x-v[0].x, v[2].y-v[0].y};
    SDL_SetTextureColorMod(texture, (v[0].argb >> 16)&255, (v[0].argb >> 8)&255, v[0].argb&255);
    SDL_SetTextureAlphaMod(texture, v[0].argb >> 24);
    SDL_RenderTextureRotated(sRenderer, texture, &src, &dst, 0, NULL, flip);
    SDL_SetTextureColorMod(texture,255,255,255); SDL_SetTextureAlphaMod(texture,255);
    return 1;
}
