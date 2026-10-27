/* Host adapter for the native Windows decoder/subtitle source extracted at build time.
   No guest function pointers are called while the host is playing a movie. */
#define COBJMACROS
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include "../include/port/port_hd_media.h"
typedef WCHAR SubChar;
#define SDL_getenv SDL_getenv_unsafe
static int sStartAt, sMovieDeviceReset;
static const char *sMovieSubs[2], *sMovieFont;
static SDL_Keycode sMovieStartKeys[2] = { SDLK_RETURN, 0 };
static struct { int hdSubtitles; } gPortConfig;
static void RendererDeviceReset(void);
#define PortLog Log
static SDL_Renderer* PortRenderer(void) { return sRenderer; }
static int PortSmoothScaleMode(void) { return SDL_SCALEMODE_LINEAR; }
static const char* PortConfigPath(const char* name) {
    static char paths[4][1200]; static unsigned next;
    if (sMovieFont && !strcmp(name, "assets/fonts/KHMenu.otf")) return sMovieFont;
    char* path = paths[next++ & 3]; snprintf(path, 1200, "%s/%s", sData, name); return path;
}
static void MoviePath(const char* name, char* out, size_t cap) { snprintf(out, cap, "%s", name); }
static void MovieSubtitlePath(const char* name, int i, char* out, size_t cap) {
    (void)name; snprintf(out, cap, "%s", sMovieSubs[i] ? sMovieSubs[i] : "");
}
static void PortAudioMovieHold(int on) {
    if (!sAudio) return;
    H_audio_clear();
    if (on || sAudioBridgeControls) SDL_PauseAudioStreamDevice(sAudio);
    else SDL_ResumeAudioStreamDevice(sAudio); /* legacy guests start the device themselves */
}
static void PortSceneGalleryMovieCapture(SDL_Renderer* renderer) { (void)renderer; }
static void DropPendingInput(void) {
    sHostKeyRead = sHostKeyWrite = 0;
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
    SDL_FlushEvents(SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_EVENT_GAMEPAD_BUTTON_UP);
    SDL_FlushEvents(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP);
    /* The movie may consume the release of a key already held by the guest.
       Queue a logical focus loss after clearing input so all guest actions reset. */
    SDL_Event clear = {0}; clear.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    QueueKey(&clear);
}
static int PauseWanted(int* quit) {
    SDL_Event event; int pause = 0;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_RENDER_DEVICE_RESET) {
            RendererDeviceReset(); sMovieDeviceReset = 1;
        } else if (event.type == SDL_EVENT_QUIT) { *quit = 1; return 0; }
        else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
            pause |= event.key.key == SDLK_ESCAPE || event.key.key == sMovieStartKeys[0] ||
                     (sMovieStartKeys[1] && event.key.key == sMovieStartKeys[1]);
        else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            pause |= event.gbutton.button == SDL_GAMEPAD_BUTTON_START;
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_FINGER_DOWN) pause = 1;
    }
    return pause;
}
#include "windows_movie_subtitles.inc"

static int MoviePause(SDL_Renderer* renderer, SDL_Surface* shot, SDL_AudioStream* audio, int* quit) {
    /* Continue / Skip, then an explicit Yes / No confirmation. A/B outside this UI never skip. */
    static const char* const labels[][5] = {
        { "Paused", "Continue", "Skip scene", "Skip this scene?", "Yes" },
        { "En pause", "Continuer", "Passer la scene", "Passer cette scene ?", "Oui" },
        { "Pause", "Fortsetzen", "Szene uberspringen", "Diese Szene uberspringen?", "Ja" },
        { "In pausa", "Continua", "Salta scena", "Saltare questa scena?", "Si" },
        { "Pausa", "Continuar", "Omitir escena", "Omitir esta escena?", "Si" }
    };
    const char* no[] = { "No", "Non", "Nein", "No", "No" };
    int lang = 0, confirm = 0, selected = 0, result = 0, done = 0, previous = -1, tick = 0, step = 0;
    const char* first = sMovieSubs[0] ? sMovieSubs[0] : "";
    if (strstr(first, ".fr.txt")) lang = 1; else if (strstr(first, ".de.txt")) lang = 2;
    else if (strstr(first, ".it.txt")) lang = 3; else if (strstr(first, ".es.txt")) lang = 4;
    SDL_Texture* frozen = shot ? SDL_CreateTextureFromSurface(renderer, shot) : NULL;
    if (!frozen) return 0;
    Subs text[3]; memset(text, 0, sizeof(text));
    if (audio) SDL_PauseAudioStreamDevice(audio);
    while (!done && !*quit && !sMovieDeviceReset) {
        SDL_Event event;
        if (getenv("KHCOM_HDMOVIE_PAUSETEST") && ++tick % 10 == 0 && step < 4) {
            static const SDL_Keycode walk[] = { SDLK_DOWN, SDLK_RETURN, SDLK_UP, SDLK_RETURN };
            static const SDL_Keycode cancel[] = { SDLK_DOWN, SDLK_RETURN, SDLK_ESCAPE, SDLK_ESCAPE };
            SDL_zero(event); event.type = SDL_EVENT_KEY_DOWN;
            const char* action = getenv("KHCOM_HDMOVIE_PAUSE_ACTION");
            event.key.key = action && !strcmp(action, "cancel") ? cancel[step++] : walk[step++];
            SDL_PushEvent(&event);
        }
        while (SDL_PollEvent(&event)) {
            int key = 0;
            if (event.type == SDL_EVENT_RENDER_DEVICE_RESET) { RendererDeviceReset(); sMovieDeviceReset = 1; break; }
            if (event.type == SDL_EVENT_QUIT) { *quit = 1; break; }
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) key = event.key.key;
            else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
                switch (event.gbutton.button) {
                    case SDL_GAMEPAD_BUTTON_DPAD_UP: key = SDLK_UP; break;
                    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: key = SDLK_DOWN; break;
                    case SDL_GAMEPAD_BUTTON_SOUTH: key = SDLK_RETURN; break;
                    case SDL_GAMEPAD_BUTTON_EAST: key = SDLK_ESCAPE; break;
                    case SDL_GAMEPAD_BUTTON_START: key = SDLK_ESCAPE; break;
                    default: break;
                }
            }
            if (key == SDLK_UP || key == SDLK_DOWN) selected ^= 1;
            else if (key == SDLK_ESCAPE) {
                if (confirm) { confirm = 0; selected = 1; } else done = 1;
            } else if (key == SDLK_RETURN || key == SDLK_SPACE) {
                if (confirm) { if (!selected) { result = 1; done = 1; } else { confirm = 0; selected = 1; } }
                else if (!selected) done = 1;
                else { confirm = 1; selected = 1; }
            }
        }
        if (*quit || sMovieDeviceReset) break;
        int ow = 0, oh = 0; SDL_GetCurrentRenderOutputSize(renderer, &ow, &oh);
        if (ow <= 0 || oh <= 0) { SDL_Delay(16); continue; }
        if (previous != confirm || text[0].height != oh) {
            for (int i = 0; i < 3; i++) {
                FreeSubs(&text[i]);
                const char* label = i == 0 ? labels[lang][confirm ? 3 : 0] :
                    i == 1 ? labels[lang][confirm ? 4 : 1] : confirm ? no[lang] : labels[lang][2];
                int len = MultiByteToWideChar(CP_UTF8, 0, label, -1, NULL, 0);
                text[i].text[0] = SDL_malloc((size_t)len * sizeof(WCHAR));
                if (text[i].text[0]) {
                    MultiByteToWideChar(CP_UTF8, 0, label, -1, text[i].text[0], len);
                    text[i].count = 1; MakeSubTexture(&text[i], renderer, 0, oh);
                }
            }
            previous = confirm;
        }
        SDL_SetRenderViewport(renderer, NULL); SDL_SetRenderClipRect(renderer, NULL);
        SDL_SetRenderScale(renderer, 1, 1); SDL_RenderTexture(renderer, frozen, NULL, NULL);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180); SDL_RenderFillRect(renderer, NULL);
        SDL_FRect box = { ow * .2f, oh * .3f, ow * .6f, oh * .4f };
        SDL_SetRenderDrawColor(renderer, 25, 30, 65, 245); SDL_RenderFillRect(renderer, &box);
        for (int i = 0; i < 3; i++) {
            float y = oh * (.34f + .105f * i);
            if (i > 0 && selected == i - 1) {
                SDL_FRect choice = { ow * .25f, y - 4, ow * .5f, oh * .07f };
                SDL_SetRenderDrawColor(renderer, 80, 100, 180, 255); SDL_RenderFillRect(renderer, &choice);
            }
            SDL_FRect dst = { (ow - text[i].w) / 2.f, y, (float)text[i].w, (float)text[i].h };
            if (text[i].texture) SDL_RenderTexture(renderer, text[i].texture, NULL, &dst);
        }
        if (getenv("KHCOM_HDMOVIE_PAUSETEST") && (tick == 5 || tick == 25)) {
            SDL_Surface* cap = SDL_RenderReadPixels(renderer, NULL);
            if (cap) { SDL_SaveBMP(cap, PortConfigPath(tick == 5 ? "hdmovie_pause.bmp" : "hdmovie_skip_confirm.bmp")); SDL_DestroySurface(cap); }
        }
        SDL_RenderPresent(renderer); SDL_Delay(16);
    }
    for (int i = 0; i < 3; i++) FreeSubs(&text[i]);
    SDL_DestroyTexture(frozen);
    if (audio) SDL_ResumeAudioStreamDevice(audio);
    DropPendingInput();
    Log("hd movie pause: %s", *quit ? "quit" : sMovieDeviceReset ? "device reset" : result ? "skip confirmed" : "continue");
    return result;
}
#include "windows_movie_decoder.inc"
#undef SDL_getenv

static int H_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* font) {
    char path[2048];
    if (!movie || snprintf(path, sizeof(path), "%s", movie) >= (int)sizeof(path)) return 0;
    sStartAt = 0; sMovieDeviceReset = 0;
    char* at = strrchr(path, '@');
    if (at) { char* end; long second = strtol(at + 1, &end, 10); if (end != at + 1 && !*end && second >= 0 && second <= 86400) { sStartAt = (int)second; *at = 0; } }
    sMovieSubs[0] = subs1; sMovieSubs[1] = subs2; sMovieFont = font;
    gPortConfig.hdSubtitles = (subs1 && *subs1) || (subs2 && *subs2);
    sMovieStartKeys[0] = SDLK_RETURN; sMovieStartKeys[1] = 0;
    FILE* config = fopen(PortConfigPath("kh_com.ini"), "r");
    if (config) {
        char line[256]; unsigned key;
        while (fgets(line, sizeof(line), config)) {
            if (sscanf(line, "kb_start=%u", &key) == 1) sMovieStartKeys[0] = key;
            else if (sscanf(line, "kb_start_2=%u", &key) == 1) sMovieStartKeys[1] = key;
        }
        fclose(config);
    }
    HRESULT com = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) return 0;
    SDL_Rect viewport, clip;
    float scaleX = 1, scaleY = 1;
    SDL_GetRenderViewport(sRenderer, &viewport);
    bool clipped = SDL_RenderClipEnabled(sRenderer);
    SDL_GetRenderClipRect(sRenderer, &clip);
    SDL_GetRenderScale(sRenderer, &scaleX, &scaleY);
    int played = PlayFile(path);
    SDL_SetRenderViewport(sRenderer, &viewport);
    SDL_SetRenderScale(sRenderer, scaleX, scaleY);
    SDL_SetRenderClipRect(sRenderer, clipped ? &clip : NULL);
    if (sMovieDeviceReset) Log("hd movie: renderer reset; returning to guest generation %u", sRendererGeneration);
    DropPendingInput();
    if (SUCCEEDED(com)) CoUninitialize();
    return played;
}
#undef PortLog
