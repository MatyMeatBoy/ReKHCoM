#ifndef REKHCOM_ANDROID_LIFECYCLE_EVENTS_H
#define REKHCOM_ANDROID_LIFECYCLE_EVENTS_H

#include <SDL3/SDL.h>

static bool sPortAndroidBackground;

/* SDL3 sends app lifecycle events to watchers only. Relay one event per
 * transition to the existing render-thread consumers (game and movies).
 * SDL_ADDEVENT appends directly, without dispatching the watchers again. */
static bool SDLCALL PortAndroidLifecycleWatch(void* userdata, SDL_Event* event) {
    (void)userdata;
    if (event->type != SDL_EVENT_DID_ENTER_BACKGROUND &&
        event->type != SDL_EVENT_DID_ENTER_FOREGROUND)
        return true;

    const bool background = event->type == SDL_EVENT_DID_ENTER_BACKGROUND;
    if (background == sPortAndroidBackground)
        return true;

    SDL_Event queued = {0};
    queued.type = event->type;
    queued.common.timestamp = SDL_GetTicksNS();
    if (SDL_PeepEvents(&queued, 1, SDL_ADDEVENT, 0, 0) == 1)
        sPortAndroidBackground = background;
    else
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not queue Android lifecycle event: %s", SDL_GetError());
    return true;
}

static bool PortAndroidLifecycleStart(void) {
    sPortAndroidBackground = false;
    return SDL_AddEventWatch(PortAndroidLifecycleWatch, NULL);
}

static void PortAndroidLifecycleStop(void) {
    SDL_RemoveEventWatch(PortAndroidLifecycleWatch, NULL);
}

#endif
