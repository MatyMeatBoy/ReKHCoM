/* Included by SDL GPU hosts after sWindow/sRenderer exist. Commands run on the
 * caller's renderer thread; no shared pointers or new bridge slots are used. */
#ifndef HOST_OUTPUT_SDL_H
#define HOST_OUTPUT_SDL_H
#include <limits.h>
#include "bridge_output.h"
#ifndef HOST_OUTPUT_WINDOW
#define HOST_OUTPUT_WINDOW 1
#endif
static int HostOutputCommand(int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    int actual = 0, width = 0, height = 0;
    (void)c; (void)d;
    switch (op) {
    case BRIDGE_OUTPUT_PROBE:
        return sWindow && sRenderer ? BRIDGE_OUTPUT_MAGIC : 0;
    case BRIDGE_OUTPUT_CAPS:
        return sWindow && sRenderer ? BRIDGE_OUTPUT_VSYNC_CAP |
            (HOST_OUTPUT_WINDOW ? BRIDGE_OUTPUT_WINDOW_CAP : 0) : 0;
    case BRIDGE_OUTPUT_WINDOW_SET:
        if (!HOST_OUTPUT_WINDOW || !sWindow || !a || !b || a > INT_MAX || b > INT_MAX)
            return 0;
        if (!SDL_SetWindowSize(sWindow, (int)a, (int)b) || !SDL_SyncWindow(sWindow) ||
            !SDL_GetWindowSize(sWindow, &width, &height)) {
            Log("output window: request failed: %s", SDL_GetError());
            return 0;
        }
        Log("output window: requested=%ux%u actual=%dx%d", a, b, width, height);
        return width == (int)a && height == (int)b;
    case BRIDGE_OUTPUT_VSYNC_SET:
        if (!sRenderer || (int32_t)a < -1) return 0;
        if (!SDL_SetRenderVSync(sRenderer, (int32_t)a) ||
            !SDL_GetRenderVSync(sRenderer, &actual)) {
            Log("output vsync: request failed: %s", SDL_GetError());
            return 0;
        }
        Log("output vsync: requested=%d actual=%d", (int32_t)a, actual);
        return actual == (int32_t)a;
    case BRIDGE_OUTPUT_VSYNC_GET:
        if (!sRenderer || !SDL_GetRenderVSync(sRenderer, &actual) || actual < -1 ||
            actual > INT_MAX - 2) return 0;
        return actual + 2;
    case BRIDGE_OUTPUT_PIXEL_WIDTH:
    case BRIDGE_OUTPUT_PIXEL_HEIGHT:
        if (!sWindow || !SDL_GetWindowSizeInPixels(sWindow, &width, &height)) return 0;
        return op == BRIDGE_OUTPUT_PIXEL_WIDTH ? width : height;
    case BRIDGE_OUTPUT_WINDOW_SYNC:
        return HOST_OUTPUT_WINDOW && sWindow && SDL_SyncWindow(sWindow);
    default: return -2;
    }
}
#endif
