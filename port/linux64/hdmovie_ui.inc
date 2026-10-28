/* Host-owned movie controls: the guest is suspended during H_hd_movie. */
static SDL_Keycode sMovieStartKeys[2];
static int sMovieFps, sMovieLanguage;
static int MovieLanguage(int setting, const char* commandLine, const char* primarySubs) {
    static const char* const languages[] = { "en", "fr", "de", "it", "es" };
    static const char* const suffixes[] = { ".uk.txt", ".fr.txt", ".de.txt", ".it.txt", ".es.txt" };
    if (primarySubs) for (int i = 0; i < 5; i++) {
        size_t a = strlen(primarySubs), b = strlen(suffixes[i]);
        if (a >= b && !strcmp(primarySubs + a - b, suffixes[i])) return i;
    }
    if (primarySubs) {
        size_t size = strlen(primarySubs);
        if (size >= 7 && (!strcmp(primarySubs + size - 7, ".us.txt") || !strcmp(primarySubs + size - 7, ".ja.txt"))) return 0;
    }
    if (commandLine) for (int i = 0; i < 5; i++) if (!strcmp(commandLine, languages[i])) return i;
    return setting >= 1 && setting <= 5 ? setting - 1 : 0;
}
static void MovieSettings(const char* primarySubs) {
    sMovieStartKeys[0] = SDLK_RETURN; sMovieStartKeys[1] = 0;
    sMovieFps = sMovieLanguage = 0;
    char path[1200], line[256]; unsigned key; int value;
    snprintf(path, sizeof(path), "%s/kh_com.ini", sData);
    FILE* file = fopen(path, "r");
    int language = 0;
    while (file && fgets(line, sizeof(line), file)) {
        if (sscanf(line, "kb_start=%u", &key) == 1) sMovieStartKeys[0] = key;
        else if (sscanf(line, "kb_start_2=%u", &key) == 1) sMovieStartKeys[1] = key;
        else if (sscanf(line, "show_fps=%d", &value) == 1) sMovieFps = value != 0;
        else if (sscanf(line, "game_language=%d", &value) == 1) language = value;
    }
    if (file) fclose(file);
    sMovieLanguage = MovieLanguage(language, getenv("KHCOM_LANGUAGE"), primarySubs);
}
/* Commands:1 up,2 down,3 back,4 accept. Result:0 pending,1 continue,2 skip. */
static int MoviePauseChoice(int* confirm, int* selected, int command) {
    if (command == 1 || command == 2) *selected ^= 1;
    else if (command == 3) {
        if (*confirm) { *confirm = 0; *selected = 1; } else return 1;
    } else if (command == 4) {
        if (*confirm) {
            if (!*selected) return 2;
            *confirm = 0; *selected = 1;
        } else if (!*selected) return 1;
        else { *confirm = 1; *selected = 1; }
    }
    return 0;
}
static int MoviePauseKey(const SDL_Event* e) {
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat) {
        if (e->key.key == SDLK_UP) return 1;
        if (e->key.key == SDLK_DOWN) return 2;
        if (e->key.key == SDLK_ESCAPE || e->key.key == SDLK_Z) return 3;
        if (e->key.key == SDLK_RETURN || e->key.key == SDLK_SPACE || e->key.key == SDLK_X) return 4;
        if (e->key.key == sMovieStartKeys[0] || (sMovieStartKeys[1] && e->key.key == sMovieStartKeys[1])) return 3;
    } else if (e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        switch (e->gbutton.button) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP: return 1;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return 2;
            case SDL_GAMEPAD_BUTTON_EAST: case SDL_GAMEPAD_BUTTON_START: return 3;
            case SDL_GAMEPAD_BUTTON_SOUTH: return 4;
            default: break;
        }
    }
    return 0;
}
static void MovieDropInput(void) {
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
    SDL_FlushEvents(SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_EVENT_GAMEPAD_BUTTON_UP);
    SDL_FlushEvents(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP);
}
/* Freeze the already presented image; audio and wall-clock time both wait. */
static int MoviePause(stbtt_fontinfo* font, int haveFont, SDL_AudioStream* audio,
                      int* rendererLost, double* began) {
    static const char* const labels[][6] = {
        { "Paused", "Continue", "Skip scene", "Skip this scene?", "Yes", "No" },
        { "En pause", "Continuer", "Passer la scene", "Passer cette scene ?", "Oui", "Non" },
        { "Pause", "Fortsetzen", "Szene uberspringen", "Diese Szene uberspringen?", "Ja", "Nein" },
        { "In pausa", "Continua", "Salta scena", "Saltare questa scena?", "Si", "No" },
        { "Pausa", "Continuar", "Omitir escena", "Omitir esta escena?", "Si", "No" }
    };
    SDL_Surface* shot = SDL_RenderReadPixels(sRenderer, NULL);
    SDL_Texture* frozen = shot ? SDL_CreateTextureFromSurface(sRenderer, shot) : NULL;
    if (shot) SDL_DestroySurface(shot);
    if (!frozen) { Log("hd movie pause: no frozen image; continuing"); SDL_RenderPresent(sRenderer); return 0; }
    double before = SDL_GetTicksNS() / 1e9;
    if (audio) SDL_PauseAudioStreamDevice(audio);
    int confirm = 0, selected = 0, result = 0, previous = -1, previousH = -1, tick = 0, step = 0;
    SDL_Texture* texts[3] = { NULL, NULL, NULL }; int widths[3] = {0}, heights[3] = {0};
    while (!result && !sQuit && !*rendererLost) {
        SDL_Event e;
        if (getenv("KHCOM_HDMOVIE_PAUSETEST") && ++tick % 10 == 0 && step < 4) {
            static const SDL_Keycode yes[] = { SDLK_DOWN, SDLK_RETURN, SDLK_UP, SDLK_RETURN };
            static const SDL_Keycode no[] = { SDLK_DOWN, SDLK_RETURN, SDLK_ESCAPE, SDLK_ESCAPE };
            const char* action = getenv("KHCOM_HDMOVIE_PAUSE_ACTION");
            SDL_zero(e); e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = action && !strcmp(action, "cancel") ? no[step++] : yes[step++];
            SDL_PushEvent(&e);
        }
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) { sQuit = 1; break; }
            if (e.type == SDL_EVENT_RENDER_DEVICE_RESET) { RendererDeviceReset(); *rendererLost = 1; break; }
            result = MoviePauseChoice(&confirm, &selected, MoviePauseKey(&e));
            if (result) break;
        }
        if (result || sQuit || *rendererLost) break;
        int ow = 0, oh = 0; SDL_GetCurrentRenderOutputSize(sRenderer, &ow, &oh);
        if (ow <= 0 || oh <= 0) { SDL_Delay(16); continue; }
        if (previous != confirm || previousH != oh) {
            for (int i = 0; i < 3; i++) {
                if (texts[i]) SDL_DestroyTexture(texts[i]);
                const char* label = labels[sMovieLanguage][i == 0 ? (confirm ? 3 : 0) : (confirm ? 3 + i : i)];
                texts[i] = haveFont ? SubTexture(font, label, oh / 24.f, &widths[i], &heights[i]) : NULL;
            }
            previous = confirm; previousH = oh;
        }
        SDL_SetRenderViewport(sRenderer, NULL); SDL_SetRenderClipRect(sRenderer, NULL);
        SDL_SetRenderScale(sRenderer, 1, 1); SDL_RenderTexture(sRenderer, frozen, NULL, NULL);
        SDL_SetRenderDrawBlendMode(sRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, 180); SDL_RenderFillRect(sRenderer, NULL);
        SDL_FRect box = { ow * .2f, oh * .3f, ow * .6f, oh * .4f };
        SDL_SetRenderDrawColor(sRenderer, 25, 30, 65, 245); SDL_RenderFillRect(sRenderer, &box);
        for (int i = 0; i < 3; i++) {
            float y = oh * (.34f + .105f * i);
            if (i && selected == i - 1) {
                SDL_FRect r = { ow * .25f, y - 4, ow * .5f, oh * .07f };
                SDL_SetRenderDrawColor(sRenderer, 80, 100, 180, 255); SDL_RenderFillRect(sRenderer, &r);
            }
            if (texts[i]) {
                SDL_FRect r = { (ow - widths[i]) / 2.f, y, (float)widths[i], (float)heights[i] };
                SDL_RenderTexture(sRenderer, texts[i], NULL, &r);
            } else {
                const char* label = labels[sMovieLanguage][i == 0 ? (confirm ? 3 : 0) : (confirm ? 3 + i : i)];
                SDL_SetRenderDrawColor(sRenderer, 255, 255, 255, 255);
                SDL_SetRenderScale(sRenderer, 2, 2);
                SDL_RenderDebugText(sRenderer, ow * .14f, y / 2, label);
                SDL_SetRenderScale(sRenderer, 1, 1);
            }
        }
        if (getenv("KHCOM_HDMOVIE_PAUSETEST") && (tick == 5 || tick == 25)) {
            SDL_Surface* capture = SDL_RenderReadPixels(sRenderer, NULL); char path[1200];
            snprintf(path, sizeof(path), "%s/%s", sData, tick == 5 ? "hdmovie_pause.bmp" : "hdmovie_skip_confirm.bmp");
            if (capture) { SDL_SaveBMP(capture, path); SDL_DestroySurface(capture); }
        }
        SDL_RenderPresent(sRenderer); SDL_Delay(16);
    }
    for (int i = 0; i < 3; i++) if (texts[i]) SDL_DestroyTexture(texts[i]);
    SDL_DestroyTexture(frozen);
    if (audio && !sQuit && !*rendererLost) SDL_ResumeAudioStreamDevice(audio);
    *began += SDL_GetTicksNS() / 1e9 - before;
    MovieDropInput();
    Log("hd movie pause: %s", sQuit ? "quit" : *rendererLost ? "device reset" : result == 2 ? "skip confirmed" : "continue");
    return result == 2;
}
