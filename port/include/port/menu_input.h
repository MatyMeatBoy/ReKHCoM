#ifndef PORT_MENU_INPUT_H
#define PORT_MENU_INPUT_H
#include "port/port.h"
#include "port/pad_layout.h"

/* 0: unrelated axis, 1: a mapped axis held/released, 2: one new A/B press.
   Leave captures raw: the caller invokes this only outside binding capture. */
static inline int KhPadMenuAxis(const SDL_Event* event, const int bindings[2], unsigned char held[2], SDL_Event* mapped) {
    int action, used = 0, pressed = -1;
    if (event->type != SDL_EVENT_GAMEPAD_AXIS_MOTION) return 0;
    for (action = 0; action < 2; action++) {
        int encoded = bindings[action] - PORT_PAD_AXIS_BASE, was;
        if (encoded < 0 || encoded / 2 >= SDL_GAMEPAD_AXIS_COUNT || encoded / 2 != event->gaxis.axis) continue;
        used = 1;
        was = held[action];
        held[action] = (unsigned char)KhPadAxisHeld(event->gaxis.value, encoded & 1, was);
        if (held[action] && !was && pressed < 0) pressed = action;
    }
    if (pressed >= 0) {
        SDL_memset(mapped, 0, sizeof(*mapped));
        mapped->type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
        mapped->gbutton.which = event->gaxis.which;
        mapped->gbutton.button = (Uint8)bindings[pressed];
        return 2;
    }
    return used;
}
#endif
