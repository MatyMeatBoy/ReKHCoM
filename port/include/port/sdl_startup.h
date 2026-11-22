#ifndef REKHCOM_SDL_STARTUP_H
#define REKHCOM_SDL_STARTUP_H

#include <SDL3/SDL.h>

/* Start the required SDL services before audio. Callers already handle an
 * unavailable output stream and can keep simulating and rendering without it. */
static inline bool PortSdlInit(SDL_InitFlags required, void (*log)(const char*, ...)) {
    if (!SDL_Init(required))
        return false;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        log("SDL audio unavailable, continuing without sound: %s", SDL_GetError());
        SDL_ClearError();
    }
    return true;
}

#endif
