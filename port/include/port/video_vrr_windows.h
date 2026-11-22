#ifndef PORT_VIDEO_VRR_WINDOWS_H
#define PORT_VIDEO_VRR_WINDOWS_H
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <dxgi1_5.h>

/* Presentation eligibility only, not monitor/driver FreeSync configuration.
 * Shared by the native32 process and the real Windows64 SDL host. */
static int PortWindowsVrrEligible(SDL_Renderer* renderer) {
    SDL_PropertiesID properties;
    IDXGISwapChain* chain;
    DXGI_SWAP_CHAIN_DESC desc;
    if (!renderer) return 0;
    properties=SDL_GetRendererProperties(renderer);
    chain=(IDXGISwapChain*)SDL_GetPointerProperty(properties,
        SDL_PROP_RENDERER_D3D11_SWAPCHAIN_POINTER,NULL);
    if (!chain) chain=(IDXGISwapChain*)SDL_GetPointerProperty(properties,
        SDL_PROP_RENDERER_D3D12_SWAPCHAIN_POINTER,NULL);
    if (chain && SUCCEEDED(IDXGISwapChain_GetDesc(chain,&desc)))
        return desc.Windowed && (desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)!=0;
    return 0;
}
#endif
