/* Optional read-only SDL host metadata. No HostPad/table layout changes.
 * The SDL hosts expose one physical controller as bridge ID 1, never its SDL ID.
 * Include after the host's sPad declaration. */
#ifndef HOST_GAMEPAD_INFO_H
#define HOST_GAMEPAD_INFO_H
static int HostGamepadInfoQuery(const char* name, char* out, int cap) {
    int connected, type = 0, n;
    if (strcmp(name, "KHCOM_GAMEPAD_INFO_V1")) return -2;
    if (!out || cap <= 0) return -1;
    connected = sPad && SDL_GamepadConnected(sPad);
    if (connected) {
        /* Wire values are explicit, so future SDL types remain unknown. */
        switch (SDL_GetGamepadType(sPad)) {
        case SDL_GAMEPAD_TYPE_STANDARD: type = 1; break;
        case SDL_GAMEPAD_TYPE_XBOX360: type = 2; break;
        case SDL_GAMEPAD_TYPE_XBOXONE: type = 3; break;
        case SDL_GAMEPAD_TYPE_PS3: type = 4; break;
        case SDL_GAMEPAD_TYPE_PS4: type = 5; break;
        case SDL_GAMEPAD_TYPE_PS5: type = 6; break;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO: type = 7; break;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT: type = 8; break;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT: type = 9; break;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: type = 10; break;
        default: break;
        }
    }
    n = snprintf(out, (size_t)cap, "bridge-pad-v1:1:%d:%d", connected, type);
    return n >= 0 && n < cap ? n : -1;
}
#endif
