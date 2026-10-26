/* Real options reset and SDL controller layout/axis rules, without a cartridge. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/pad_layout.h"
#include "port/menu_input.h"
PortConfig gPortConfig;
static int saved, inputReset;
static int sControlCapture;
void PortConfigSave(void) { ++saved; }
void PortInputResetBindings(void) { ++inputReset; }
#include "reset_control_bindings.inc"
#include "ui_mapped_key.inc"

int main(void) {
    memset(&gPortConfig, 0x7f, sizeof(gPortConfig));
    gPortConfig.padLayout = 2;
    sControlCapture = 1;
    ResetControlBindings();
    assert(gPortConfig.keys[PORT_ACT_A][0] == SDLK_X);
    assert(gPortConfig.keys[PORT_ACT_A][1] == SDLK_K);
    assert(gPortConfig.keys[PORT_ACT_SELECT][1] == SDLK_RSHIFT);
    assert(gPortConfig.pad[PORT_ACT_A] == SDL_GAMEPAD_BUTTON_SOUTH);
    assert(gPortConfig.pad[PORT_ACT_L] == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    assert(gPortConfig.padLayout == 0);
    assert(!sControlCapture && saved == 1 && inputReset == 1);

    /* Auto does not infer Nintendo from an absent/ambiguous controller. */
    assert(!KhPadIsNintendo(NULL, 0));
    assert(!KhPadIsNintendo(NULL, 1));
    assert(KhPadIsNintendo(NULL, 2));

    /* A slow squeeze crosses the intermediate release band before pressing. */
    int held = 0, presses = 0;
    const int samples[] = {0, 4000, 9000, 16000, 17000, 15000, 9000, 8000, 9000, 17000, 0};
    for (unsigned i = 0; i < sizeof(samples) / sizeof(*samples); ++i) {
        int was = held;
        held = KhPadAxisHeld(samples[i], 0, held);
        presses += held && !was;
    }
    assert(presses == 2 && !held);
    assert(!KhPadAxisHeld(16000, 0, 0));
    assert(KhPadAxisHeld(16001, 0, 0));
    assert(KhPadAxisHeld(8001, 0, 1));
    assert(!KhPadAxisHeld(8000, 0, 1));
    assert(KhPadAxisHeld(-17000, 1, 0));
    assert(!KhPadAxisHeld(-8000, 1, 1));

    SDL_Event event = {0}, mapped;
    unsigned char axes[2] = {0, 0};
    const int bindings[2] = {PORT_PAD_AXIS_BASE + SDL_GAMEPAD_AXIS_LEFT_TRIGGER * 2,
                             PORT_PAD_AXIS_BASE + SDL_GAMEPAD_AXIS_RIGHT_TRIGGER * 2};
    event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.which = 42;
    event.gaxis.axis = SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    event.gaxis.value = 9000; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 1);
    event.gaxis.value = 17000; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 2);
    assert(mapped.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && mapped.gbutton.which == 42 && mapped.gbutton.button == bindings[0]);
    event.gaxis.value = 18000; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 1);
    event.gaxis.value = 10000; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 1);
    event.gaxis.value = 17000; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 1);
    event.gaxis.value = 0; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 1);
    event.gaxis.value = 17000; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 2);
    event.gaxis.axis = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    event.gaxis.value = 17000; assert(KhPadMenuAxis(&event, bindings, axes, &mapped) == 2);
    assert(mapped.gbutton.button == bindings[1]);
    const int duplicated[2] = {bindings[0], bindings[0]};
    axes[0] = axes[1] = 0;
    event.gaxis.axis = SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
    assert(KhPadMenuAxis(&event, duplicated, axes, &mapped) == 2);
    assert(KhPadMenuAxis(&event, duplicated, axes, &mapped) == 1);
    gPortConfig.keys[PORT_ACT_A][0] = SDLK_F;
    gPortConfig.keys[PORT_ACT_B][0] = SDLK_G;
    assert(UiMappedKey(SDLK_F) == SDLK_X && UiMappedKey(SDLK_G) == SDLK_Z);
    gPortConfig.keys[PORT_ACT_A][0] = SDLK_ESCAPE;
    gPortConfig.keys[PORT_ACT_B][0] = SDLK_UP;
    assert(UiMappedKey(SDLK_ESCAPE) == SDLK_ESCAPE && UiMappedKey(SDLK_UP) == SDLK_UP);

    puts("Controls: actual reset restores both key slots, pad bindings, automatic layout and capture/input; gradual trigger press, jitter and release PASS");
    return 0;
}
