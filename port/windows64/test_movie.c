#define SDL_MAIN_HANDLED
#define main host_main
#include "windows64_host.c"
#undef main
#include <assert.h>
static DWORD WINAPI InjectReset(void* ignored) {
    (void)ignored; Sleep(600);
    SDL_Event event = {0}; event.type = SDL_EVENT_RENDER_DEVICE_RESET;
    SDL_PushEvent(&event); return 0;
}
static int MovieTestMain(int argc, char** argv) {
    assert(argc >= 5);
    snprintf(sData, sizeof(sData), "%s", argv[4]);
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD));
    sWindow = SDL_CreateWindow("Windows64 MF test", 640, 360, SDL_WINDOW_HIDDEN);
    assert(sWindow);
    sRenderer = SDL_CreateRenderer(sWindow, NULL); assert(sRenderer);
    fprintf(stderr, "movie test renderer: %s\n", SDL_GetRendererName(sRenderer));
    assert(H_audio_open(48000) == 0);
    HANDLE reset = getenv("TEST_MOVIE_RESET") ? CreateThread(NULL, 0, InjectReset, NULL, 0, NULL) : NULL;
    SDL_Rect savedClip = { 10, 20, 200, 100 };
    SDL_SetRenderClipRect(sRenderer, &savedClip);
    int result = H_hd_movie(argv[1], argv[2], NULL, argv[3]);
    fprintf(stderr, "movie result=%d generation=%u\n", result, sRendererGeneration);
    if (reset) { WaitForSingleObject(reset, INFINITE); CloseHandle(reset); assert(sRendererGeneration == 1); }
    SDL_Rect restoredClip; SDL_GetRenderClipRect(sRenderer, &restoredClip);
    assert(SDL_RenderClipEnabled(sRenderer));
    assert(!memcmp(&savedClip, &restoredClip, sizeof(savedClip)));
    uint32_t pixels[4] = {0xffffffff, 0xff000000, 0xffffffff, 0xff000000};
    int texture = B_tex_create(pixels, 2, 2, 0); assert(texture >= 0);
    B_gfx_begin(0xff000000); B_tex_free(texture); B_gfx_end();
    assert(result == (getenv("EXPECT_MOVIE_FAILURE") ? 0 : 1));
    if (sAudio) SDL_DestroyAudioStream(sAudio);
    SDL_DestroyRenderer(sRenderer); SDL_DestroyWindow(sWindow); SDL_Quit();
    return 0;
}

int main(int argc, char** argv) { (void)argc; (void)argv; return SDL_RunApp(0, NULL, MovieTestMain, NULL); }
