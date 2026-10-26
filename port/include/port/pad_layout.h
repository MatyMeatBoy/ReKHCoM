#ifndef PORT_PAD_LAYOUT_H
#define PORT_PAD_LAYOUT_H
/* Nintendo pads label their face buttons by position the other way round from
   Xbox ones (A is on the right, B at the bottom). SDL reports positions, so on a
   Nintendo-layout pad the four face buttons are exchanged (south<->east,
   west<->north) before anything reads them: "A" then means the button labelled A.
   layout: 0 detect from the pad, 1 Xbox positions, 2 Nintendo. */
#include <SDL3/SDL.h>

/* Hysteresis for remapped axes and menu triggers: a slow squeeze must cross
   the press threshold before becoming held, then fully release to rearm. */
static inline int KhPadAxisHeld(int value, int negative, int held) {
    if (negative) value = -value;
    return value > (held ? 8000 : 16000);
}

static inline int KhPadIsNintendo(SDL_Gamepad* pad, int layout) {
    if (layout == 1) return 0;
    if (layout == 2) return 1;
    switch (pad ? SDL_GetGamepadType(pad) : SDL_GAMEPAD_TYPE_UNKNOWN) {
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return 1;
        default:
            return 0;
    }
}

static inline int KhPadSwapButton(int button) {
    switch (button) {
        case SDL_GAMEPAD_BUTTON_SOUTH: return SDL_GAMEPAD_BUTTON_EAST;
        case SDL_GAMEPAD_BUTTON_EAST: return SDL_GAMEPAD_BUTTON_SOUTH;
        case SDL_GAMEPAD_BUTTON_WEST: return SDL_GAMEPAD_BUTTON_NORTH;
        case SDL_GAMEPAD_BUTTON_NORTH: return SDL_GAMEPAD_BUTTON_WEST;
        default: return button;
    }
}

/* The button as the game means it, for a button event or a polled read. */
static inline void KhPadNormalizeEvent(SDL_Event* ev, int layout) {
    if (ev->type != SDL_EVENT_GAMEPAD_BUTTON_DOWN && ev->type != SDL_EVENT_GAMEPAD_BUTTON_UP) return;
    if (KhPadIsNintendo(SDL_GetGamepadFromID(ev->gbutton.which), layout))
        ev->gbutton.button = (Uint8)KhPadSwapButton(ev->gbutton.button);
}

static inline int KhPadButtonDown(SDL_Gamepad* pad, int button, int layout) {
    if (KhPadIsNintendo(pad, layout)) button = KhPadSwapButton(button);
    return SDL_GetGamepadButton(pad, (SDL_GamepadButton)button);
}
#endif
