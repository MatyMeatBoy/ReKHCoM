#ifndef PORT_CURSOR_IDLE_H
#define PORT_CURSOR_IDLE_H

#include <SDL3/SDL.h>

typedef struct PortCursorIdle {
    Uint64 lastActivity;
    bool overGame;
    bool hidden;
} PortCursorIdle;

static void PortCursorIdleRestore(PortCursorIdle* state) {
    if (state->hidden && SDL_ShowCursor()) state->hidden = false;
}

static void PortCursorIdleEvent(PortCursorIdle* state, SDL_Window* window,
                                const SDL_Event* event) {
    SDL_WindowID id = window ? SDL_GetWindowID(window) : 0;
    bool activity = false;
    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION:
            activity = event->motion.windowID == id &&
                       (event->motion.xrel != 0 || event->motion.yrel != 0);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            activity = event->button.windowID == id;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            activity = event->wheel.windowID == id;
            break;
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
            activity = event->window.windowID == id;
            if (activity) state->overGame = false;
            break;
        default: break;
    }
    if (activity) {
        state->lastActivity = SDL_GetTicks();
        PortCursorIdleRestore(state);
    }
}

/* Call after draining events so movement at the deadline cannot hide/show in
 * the same pump. Only hide over the focused game, never over another window. */
static void PortCursorIdlePoll(PortCursorIdle* state, SDL_Window* window) {
    Uint64 now = SDL_GetTicks();
    bool over = window && SDL_GetMouseFocus() == window &&
                SDL_GetKeyboardFocus() == window &&
                !(SDL_GetWindowFlags(window) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED));
    if (!over || !state->overGame) state->lastActivity = now;
    state->overGame = over;
    if (over && now - state->lastActivity >= 2000) {
        if (!state->hidden && SDL_HideCursor()) state->hidden = true;
    } else {
        PortCursorIdleRestore(state);
    }
}

#endif
