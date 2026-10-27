/* Included after host_texture_slots.h; only real SDL GPU hosts opt in. */
#ifndef HOST_GPU_TARGETS_H
#define HOST_GPU_TARGETS_H
#include "bridge_gpu_targets.h"
#ifndef HOST_GPU_TARGET_BUDGET
#if defined(__ANDROID__) || defined(HOST_GPU_CONSTRAINED)
#define HOST_GPU_TARGET_BUDGET (12u * 1024u * 1024u)
#else
#define HOST_GPU_TARGET_BUDGET (32u * 1024u * 1024u)
#endif
#endif
static size_t sTargetBytes;
static int HostGpuMaxSize(void) {
    Sint64 size = SDL_GetNumberProperty(SDL_GetRendererProperties(sRenderer), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0);
    return size > 0 && size <= 0x7fffffff ? (int)size : 0;
}
static int HostGpuAvailable(void) {
    static uint32_t epoch = ~0u;
    static int available;
    if (epoch != sRendererGeneration) {
        SDL_Texture* probe;
        epoch = sRendererGeneration;
        available = 0;
        if (!sRenderer || !HostGpuMaxSize() || !strcmp(SDL_GetRendererName(sRenderer), "software")) return 0;
        probe = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, 1, 1);
        if (probe) {
            available = SDL_SetTextureBlendMode(probe, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
            SDL_DestroyTexture(probe);
        }
    }
    return available;
}
static void HostGpuBeforeFree(int slot) {
    if (!sTexTarget[slot]) return;
    if (SDL_GetRenderTarget(sRenderer) == sTex[slot]) SDL_SetRenderTarget(sRenderer, NULL);
    sTargetBytes -= sTexTargetBytes[slot];
    sTexTargetBytes[slot] = 0;
    sTexTarget[slot] = 0;
}
static int HostGpuCreate(int w, int h) {
    uint64_t bytes;
    if (!HostGpuAvailable() || w <= 0 || h <= 0 || w > HostGpuMaxSize() || h > HostGpuMaxSize()) return -1;
    bytes = (uint64_t)w * (uint64_t)h * 4;
    if (bytes > HOST_GPU_TARGET_BUDGET - sTargetBytes) return -1;
    for (int i = 0; i < HOST_TEXTURE_SLOTS; i++) {
        SDL_Texture* texture;
        if (sTex[i] || sTexSerial[i] == HOST_TEXTURE_MAX_SERIAL) continue;
        texture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if (!texture) return -1;
        sTex[i] = texture; sTexW[i] = w; sTexSmooth[i] = 1;
        sTexTarget[i] = 1; sTexTargetBytes[i] = (size_t)bytes; sTargetBytes += (size_t)bytes;
        sTexSerial[i]++;
        return (int)((sTexSerial[i] << 12) | (unsigned)i);
    }
    return -1;
}
static int HostGpuQuery(const char* name, char* out, int cap) {
    if (!strcmp(name, "KHCOM_GPU_RENDERER")) {
        const char* renderer = sRenderer ? SDL_GetRendererName(sRenderer) : "bridge-unavailable";
        return cap > 0 ? snprintf(out, (size_t)cap, "%s", renderer) : -1;
    }
    if (!strcmp(name, "KHCOM_GPU_TARGETS"))
        return cap > 0 ? snprintf(out, (size_t)cap, "gpu-targets-v1:%d:%u", HostGpuAvailable() ? HostGpuMaxSize() : 0,
                                  HOST_GPU_TARGET_BUDGET) : -1;
    return -2;
}
/* -2 means not our command. Legacy thread commands retain their own return values. */
static int HostGpuCommand(int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    int slot;
    (void)c; (void)d;
    switch (op) {
    case BRIDGE_GPU_PROTOCOL: return BRIDGE_GPU_MAGIC;
    case BRIDGE_GPU_PROBE: return HostGpuAvailable() ? BRIDGE_GPU_MAGIC : 0;
    case BRIDGE_GPU_MAX_SIZE: return HostGpuAvailable() ? HostGpuMaxSize() : 0;
    case BRIDGE_GPU_BUDGET: return HostGpuAvailable() ? HOST_GPU_TARGET_BUDGET : 0;
    case BRIDGE_GPU_CREATE: return HostGpuCreate((int)a, (int)b);
    case BRIDGE_GPU_BIND:
        slot = HostTextureSlot((int)a);
        if ((int)a != -1 && (slot < 0 || !sTexTarget[slot])) return 0;
        return SDL_SetRenderTarget(sRenderer, (int)a == -1 ? NULL : sTex[slot]);
    case BRIDGE_GPU_CLEAR:
        SDL_SetRenderDrawColor(sRenderer, (a >> 16) & 255, (a >> 8) & 255, a & 255, a >> 24);
        return SDL_RenderClear(sRenderer);
    case BRIDGE_GPU_BLEND:
        if (a != SDL_BLENDMODE_NONE && a != SDL_BLENDMODE_BLEND && a != SDL_BLENDMODE_ADD &&
            a != SDL_BLENDMODE_BLEND_PREMULTIPLIED) return 0;
        sBlend = (int)a; return 1;
    case BRIDGE_GPU_SMOOTH:
        slot = HostTextureSlot((int)a);
        if (slot < 0) return 0;
        sTexSmooth[slot] = b != 0; return 1;
    }
    return -2;
}
#endif
