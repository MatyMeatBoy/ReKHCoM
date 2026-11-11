#ifndef PORT_GAMEPAD_MAPPINGS_H
#define PORT_GAMEPAD_MAPPINGS_H
#include <SDL3/SDL.h>

/* SDL3 accepts the mapping format exported by SDL2 Gamepad Tool. Keep this
   on the real SDL host: the x64 guest bridge has no mapping database API. */
static inline void KhLoadGamepadMappings(void) {
    char path[1024];
    const char* base = SDL_GetBasePath();
    if (base) {
        SDL_snprintf(path, sizeof(path), "%sgamecontrollerdb.txt", base);
        if (SDL_GetPathInfo(path, NULL)) SDL_AddGamepadMappingsFromFile(path);
    }
    if (SDL_GetPathInfo("gamecontrollerdb.txt", NULL)) SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
    /* SDL reads this hint during init at the same priority as local files.
       Reapply it last so an explicit user-selected file wins for a GUID. */
    {
        const char* file = SDL_GetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE);
        if (file && *file) SDL_AddGamepadMappingsFromFile(file);
    }
}
#endif
