/* Real options reset and SDL controller layout/axis rules, without a cartridge. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/pad_layout.h"
PortConfig gPortConfig;
static int saved, inputReset;
static int sControlCapture;
void PortConfigSave(void) { ++saved; }
void PortInputResetBindings(void) { ++inputReset; }
#include "reset_control_bindings.inc"

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

    puts("Controls: actual reset restores both key slots, pad bindings, automatic layout and capture/input; gradual trigger press, jitter and release PASS");
    return 0;
}
