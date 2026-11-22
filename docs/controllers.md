# Generic USB controllers

ReKHCoM uses SDL3. Controllers are not inherently "SDL2 controllers" or
"SDL3 controllers": recognition depends on the operating system's driver and
SDL's mapping from the device's physical inputs to gamepad controls.

## Import a mapping

Export a mapping with SDL2 Gamepad Tool or another SDL-compatible mapper and
save the mapping line in `gamecontrollerdb.txt` next to the game executable.
Keep its GUID and platform field for the computer/device where it was made.
Restart the game after changing the file. The working directory is also
searched, after the executable directory. SDL's environment overrides
`SDL_GAMECONTROLLERCONFIG` and `SDL_GAMECONTROLLERCONFIG_FILE` remain available.

This path works in the native SDL game and in the Windows/Linux 64-bit SDL
hosts. Once SDL recognizes the device, configure its game actions normally
in Options > Controls. There is no vendor whitelist in ReKHCoM.

## Use a joystick without a mapping

The direct SDL build (Windows x86) also accepts unrecognized joysticks.
Use **F1**, open Controls, choose gamepad bindings, and assign each action
with the physical control you want. Buttons, positive/negative axis directions
and hat directions can be captured. Hat diagonals activate both directions.
The same bindings navigate the options menu once assigned.

Bindings are saved in `kh_com.ini`. Raw bindings describe physical indices
and are shared by unmapped devices; reassign them when changing to a different
layout. Recognized gamepads keep their normalized SDL bindings.

The 64-bit guest bridge currently exposes normalized gamepads only. For an
unknown controller on those builds, import a mapping with the first method;
raw joystick capture is not exposed through that bridge by this change.

**Select + Start** (as assigned, on the same controller) opens the port menu;
Select alone remains a game input during play. **F1** remains available even
before a controller has been configured. The existing Escape/Guide shortcuts
and title-screen options entry remain available.

## Hardware limits

A software mapping translates reported input; it cannot restore analog
precision if hardware reports only digital endpoints. A controller recovering
after a fall does not establish an SDL fault or a confirmed hardware diagnosis.
ReKHCoM's GBA game actions themselves use digital directions.

## Verification

`python port/tools/test_controller_bindings.py` runs ROM-free tests with real
SDL virtual devices. Set `KHCOM_TOOLCHAIN` to the toolchain directory when it
is not available at `../toolchain`.

Coverage includes existing normalized gamepads, unmapped buttons/axes/hats,
diagonals, capture filtering, raw menu navigation, the Select + Start chord,
held-input suppression, startup/hotplug ownership, disconnect cleanup, and
SDL2-format mapping import. Physical clone hardware still needs a playtest.
