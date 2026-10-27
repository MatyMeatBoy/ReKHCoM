/* SDL bridge texture ownership, included by Linux64/Android64 after sRenderer.
 * Handles are opaque positive ints: 12-bit slot + 19-bit allocation serial.
 * An exhausted slot is retired, never wrapped back to a stale handle. */
#ifndef HOST_TEXTURE_SLOTS_H
#define HOST_TEXTURE_SLOTS_H
#define HOST_TEXTURE_SLOTS 4096
#define HOST_TEXTURE_MAX_SERIAL 0x7ffffu
static SDL_Texture* sTex[HOST_TEXTURE_SLOTS];
static int sTexW[HOST_TEXTURE_SLOTS], sTexSmooth[HOST_TEXTURE_SLOTS];
static unsigned sTexSerial[HOST_TEXTURE_SLOTS];
static int HostTextureSlot(int id) {
    if (id < 0) return -1;
    unsigned slot = (unsigned)id & 4095u;
    return sTex[slot] && ((unsigned)id >> 12) == sTexSerial[slot] ? (int)slot : -1;
}
static int H_tex_create(const void* rgba, int w, int h, int smooth) {
    if (!rgba || w <= 0 || h <= 0 || w > 0x1fffffff) return -1;
    for (int i = 0; i < HOST_TEXTURE_SLOTS; i++) {
        if (sTex[i] || sTexSerial[i] == HOST_TEXTURE_MAX_SERIAL) continue;
        SDL_Texture* texture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
        if (!texture) return -1;
        if (!SDL_UpdateTexture(texture, NULL, rgba, w * 4)) {
            SDL_DestroyTexture(texture);
            return -1;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        sTex[i] = texture;
        sTexW[i] = w;
        sTexSmooth[i] = smooth;
        sTexSerial[i]++;
        return (int)((sTexSerial[i] << 12) | (unsigned)i);
    }
    return -1;
}
static void H_tex_update(int id, const void* rgba) {
    int slot = HostTextureSlot(id);
    if (slot >= 0 && rgba) SDL_UpdateTexture(sTex[slot], NULL, rgba, sTexW[slot] * 4);
}
static void H_tex_free(int id) {
    int slot = HostTextureSlot(id);
    if (slot < 0) return;
    SDL_DestroyTexture(sTex[slot]);
    sTex[slot] = NULL;
}
static void HostTextureReset(void) {
    for (int i = 0; i < HOST_TEXTURE_SLOTS; i++) {
        if (sTex[i]) SDL_DestroyTexture(sTex[i]);
        sTex[i] = NULL;
    }
    /* Never wrap the query epoch. At exhaustion retire all remaining handles. */
    if (sRendererGeneration < 0xffffffffu) sRendererGeneration++;
    else for (int i = 0; i < HOST_TEXTURE_SLOTS; i++) sTexSerial[i] = HOST_TEXTURE_MAX_SERIAL;
}
#endif
