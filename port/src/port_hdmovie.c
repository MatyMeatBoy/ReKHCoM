/*
 * HD movies (port/progress/HD_MOD.md): the Re:Chain of Memories movies of the player's own KINGDOM HEARTS -HD
 * 1.5+2.5 ReMIX-, imported by the launcher as hd/movies/<name>.mp4 (the story scenes: hd/scenes), play in place of the cartridge's movies.
 *
 * mode_movie.c calls PortHdMoviePlay when a movie starts: if the movie has an imported file and hd_movies is on, the
 * whole movie plays here (the window's renderer, an audio stream of its own) and the game goes on as when its own
 * movie ends; Start, A, Enter, Escape or a tap skip it. Otherwise the cartridge's movie plays as always.
 *
 * Subtitles (hd_subtitles): hd/subs/<name>.<lang>.txt, Re:CoM's own (the launcher takes them from Recom.pkg, every
 * language it has: port/tools/khhd_subs.py), in the game's language, drawn under the picture as Re:CoM does.
 *
 * Windows: Media Foundation (part of Windows: H.264 and AAC need nothing else). Android, 32 and 64 bits: the NDK's
 * MediaCodec (port/portable/hdmovie_android.c; the 64-bit image asks its host through the hd_movie bridge entry).
 * Linux: the system's ffmpeg (and ffprobe), run as two processes whose raw frames and 48 kHz PCM come through
 * pipes: nothing is linked, so it works whatever the system's ffmpeg version and the game's architecture (the
 * i386 package on a 64-bit system). Without ffmpeg installed the cartridge's movie plays. Other systems: not yet.
 * The video comes out as the decoder's own NV12 and the GPU turns it into colour (an NV12 texture): asking Media
 * Foundation for RGB put a software colour conversion of every 1080p frame on the CPU and the movies stuttered.
 * Frames already late when decoded are not uploaded.
 */
#include "port/port.h"
#include "port/port_scene_gallery.h"
#include "port/port_hd_poster.h"

#if defined(_WIN32) || defined(__ANDROID__) || defined(PORT_ANDROID64) || defined(PORT_LINUX) || defined(PORT_LINUX64_GUEST)
#include <stdio.h>
#include <string.h>
static int PlayFile(const char* name);
/* A Re:CoM scene that holds two of the cartridge's (duartepedro, #163): its HD movie starts where the cartridge's
   own movie begins, as the GBA's flow has the first part as an event. Seconds; the file is never cut. */
static int sStartAt;

/* Re:CoM's story scenes (zz_event_*) live in hd/scenes, the opening, credits and endings in hd/movies; a scene
   imported before 23/10 sits in hd/movies and is still found there. */
static void MoviePath(const char* name, char* out, size_t size) {
    if (strncmp(name, "zz_event_", 9) == 0) {
        FILE* f;
        snprintf(out, size, "%s%s.mp4", PortConfigPath("hd/scenes/"), name);
        f = fopen(out, "rb");
        if (f) {
            fclose(f);
            return;
        }
    }
    snprintf(out, size, "%s%s.mp4", PortConfigPath("hd/movies/"), name);
}

/* include/evt/mode_movie.h: MOVIE_OPENING 1, MOVIE_6F_GOAL, MOVIE_12F_E2, MOVIE_ENDING, MOVIE_RIKU_ENDING */
/* HD movies replace only the cartridge's pre-rendered movies, each with its whole Re:CoM scene; the story's other
   scenes are HD cutscenes' (#163, duartepedro). Re:CoM's credits (RSE, RRE) are not this version's: the cartridge's
   play. */
static const char* MovieFile(int movieId, int* startAt) {
    *startAt = 0;
    switch (movieId) {
        case 1: return "ROP";          /* the opening: the hooded man and the castle */
        case 3: *startAt = 170; return "zz_event_123"; /* 12th floor: Namine's charm becomes Kairi's (from 2:50) */
        case 4: *startAt = 222; return "zz_event_151"; /* Sora's ending: his friends' memories (from 3:42) */
        case 5: return "zz_event_631"; /* Riku's ending: the road to dawn */
        default: return NULL;          /* the 6th floor's: no Re:CoM scene matched */
    }
}

#ifdef VERSION_EU
extern uint32_t gLanguage;
#endif

/* the subtitle languages to try, from the game's: EU English (uk), French, German, Italian, Spanish; US English;
   Japanese (us if none) */
static void SubsOrder(const char* order[2]) {
#ifdef VERSION_EU
    static const char* const langs[] = { "uk", "fr", "de", "it", "es" };
    order[0] = gLanguage < 5 ? langs[gLanguage] : "uk";
    order[1] = "us";
#elif defined(VERSION_JP)
    order[0] = "ja";
    order[1] = "us";
#else
    order[0] = "us";
    order[1] = "uk";
#endif
}

#include "port_hdscenes.inc"

/* the endings' scene already played as its event's (only the debug menu's events 073 / 195 reach them): the credits
   do not show it again */
static int sEndingScenePlayed;
static const char* sLastScene; /* the event scene last played whole (a cartridge movie after it skips the same file) */

/* The full mod: the story event's Re:CoM scene, if imported and hd_scenes is on (event_message.c then ends the event
   through the game's own scene skip, so the event's results stay the game's). */
/* Re:CoM's own scenes, which have no event on the cartridge: in the Re:CoM mode they play next to their neighbour
   (before = 1: ahead of it), in their order in Re:CoM; the original mode stays as the cartridge. */
static const struct { unsigned short eventId; const char* movie; unsigned char before; } kHdSceneExtras[] = {
    { EVENT_000_1F_ENTRANCE_PART1, "zz_event_001", 1 }, /* the hooded man's prologue, then the castle */
    { EVENT_193_RIKU_B1F_LAST1, "zz_event_614", 0 },
};

static void PlayExtras(int eventId, int before) {
    size_t i;
    if (!PortRecomActive())
        return;
    for (i = 0; i < sizeof(kHdSceneExtras) / sizeof(kHdSceneExtras[0]); i++)
        if (kHdSceneExtras[i].eventId == eventId && kHdSceneExtras[i].before == before)
            PlayFile(kHdSceneExtras[i].movie);
}

/* The endings' scenes come with the HD movies (the importer copies them with ROP / RSE / RRE): they lead into the
   credits, so with hd_movies on and hd_scenes off the endings still match the opening (issue #42). */
static int IsEnding(int eventId) {
    return eventId == EVENT_073_13F_CAPSULE_ROOM_ENDING || eventId == EVENT_195_RIKU_ENDING;
}

/* The scene gallery (port_scene_gallery.c): the scenes it lists (the story's, with their Re:CoM scene), and the HD
   one played by itself. While the gallery replays a scene in the game's engine (gPortGalleryEvent), it stays the
   cartridge's. */
extern int gPortGalleryEvent;
void PortSceneSeen(int eventId);
int PortHdSceneCount(void) {
    return (int)(sizeof(kHdScenes) / sizeof(kHdScenes[0]));
}
int PortHdSceneAt(int i, const char** movie) {
    if (i < 0 || i >= PortHdSceneCount())
        return -1;
    if (movie)
        *movie = kHdScenes[i].movie;
    return kHdScenes[i].eventId;
}
int PortHdSceneImported(const char* movie) {
    char path[1024];
    FILE* f;
    MoviePath(movie, path, sizeof(path));
    f = fopen(path, "rb");
    if (f)
        fclose(f);
    return f != NULL;
}
int PortHdMoviePlayName(const char* movie) {
    return PlayFile(movie);
}

/* The gallery uses the same mapping and movie offsets as live gameplay; never guess a file from an event number. */
const char* PortHdGalleryMovieName(int sceneId) {
    int i, startAt;
    if (PORT_SCENE_IS_MOVIE(sceneId)) return MovieFile(sceneId - PORT_SCENE_MOVIE_BASE, &startAt);
    for (i = 0; i < PortHdSceneCount(); i++)
        if (kHdScenes[i].eventId == sceneId) return kHdScenes[i].movie;
    return NULL;
}
int PortHdGalleryMovieOffset(int sceneId) {
    int startAt = 0;
    if (PORT_SCENE_IS_MOVIE(sceneId)) MovieFile(sceneId - PORT_SCENE_MOVIE_BASE, &startAt);
    return startAt;
}

int PortHdGalleryPlayScene(int sceneId, int hybrid) {
    const char* movie = PortHdGalleryMovieName(sceneId);
    int oldStart = sStartAt, played;
    if (!movie || !PortHdSceneImported(movie)) return 0;
    if (PORT_SCENE_IS_MOVIE(sceneId)) MovieFile(sceneId - PORT_SCENE_MOVIE_BASE, &sStartAt);
    if (hybrid && !PORT_SCENE_IS_MOVIE(sceneId)) PlayExtras(sceneId, 1);
    played = PlayFile(movie);
    if (hybrid && played && !PORT_SCENE_IS_MOVIE(sceneId)) PlayExtras(sceneId, 0);
    sStartAt = oldStart;
    return played;
}

int PortHdScenePlay(int eventId) {
    size_t i;
    eventId &= 0x7FFF; /* the debug menu's events carry 0x8000 (mode_event.c) */
    PortLog("hd scene: event %d", eventId);
    PortSceneSeen(eventId);
    if (gPortGalleryEvent)
        return 0;
    if (!gPortConfig.hdScenes) /* the endings' event scenes too: HD movies are the cartridge's movies only (#163) */
        return 0;
    for (i = 0; i < sizeof(kHdScenes) / sizeof(kHdScenes[0]); i++)
        if (kHdScenes[i].eventId == eventId) {
            int played;
            PlayExtras(eventId, 1);
            played = PlayFile(kHdScenes[i].movie);
            sLastScene = played ? kHdScenes[i].movie : NULL;
            if (played)
                PlayExtras(eventId, 0);
            if (played && IsEnding(eventId))
                sEndingScenePlayed = 1;
            return played;
        }
    return 0;
}

/* The cartridge movie movieId, if its HD movie is imported and hd_movies is on. The story reaches the credits straight
   from the last event (072 for Sora, mode_event.c EVENT_EXIT_MOVIE_ENDING): Re:CoM's ending scene, which has no event
   of its own on that road, plays first (issue #42). */
int PortHdMoviePlay(int movieId) {
    int startAt, played;
    const char* name = MovieFile(movieId, &startAt);
    if (gPortGalleryEvent) return 0; /* the GBA tab always replays the cartridge's own cinematic */
    if (!gPortConfig.hdMovies || name == NULL)
        return 0;
    if ((movieId == 4 && sEndingScenePlayed) || (sLastScene && strcmp(sLastScene, name) == 0)) {
        /* HD cutscenes just played this whole file as the event's scene (151, 123): not twice */
        sLastScene = NULL;
        sEndingScenePlayed = 0;
        return 1;
    }
    sEndingScenePlayed = 0;
    sStartAt = startAt;
    played = PlayFile(name);
    sStartAt = 0;
    return played;
}


#endif

#if defined(_WIN32) || defined(PORT_LINUX)
#ifdef _WIN32
#define COBJMACROS
#endif
#include <SDL3/SDL.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
typedef WCHAR SubChar;
#else
#include <stdlib.h>
#include <unistd.h>
typedef long long LONGLONG; /* the movie clock in 100 ns units, as Media Foundation counts it */
typedef char SubChar;
unsigned char* PortPortableHdTextCover(const char* text, int px, int pad, int* w, int* h);
#endif

SDL_Renderer* PortRenderer(void);

/* Imported stills avoid opening a video decoder at all. Reject stale or
   oversized images before loading; a poster never needs a full movie frame. */
static int ReadImportedPoster(const char* movie, uint32_t* out, int ow, int oh, int second) {
    char path[1200];
    SDL_PathInfo media, image;
    SDL_Surface *source, *target;
    unsigned char header[24];
    SDL_IOStream* file;
    unsigned w, h;
    int ok;
    if (!PortHdPosterPath(movie, second, path, sizeof(path)) ||
        !SDL_GetPathInfo(movie, &media) || !SDL_GetPathInfo(path, &image) ||
        image.modify_time < media.modify_time || image.size > 2u * 1024u * 1024u) return 0;
    file = SDL_IOFromFile(path, "rb");
    if (!file) return 0;
    ok = SDL_ReadIO(file, header, sizeof(header)) == sizeof(header); SDL_CloseIO(file);
    if (!ok || memcmp(header, "\x89PNG\r\n\x1a\n", 8) || memcmp(header + 12, "IHDR", 4)) return 0;
    w = (unsigned)header[16] << 24 | (unsigned)header[17] << 16 | (unsigned)header[18] << 8 | header[19];
    h = (unsigned)header[20] << 24 | (unsigned)header[21] << 16 | (unsigned)header[22] << 8 | header[23];
    if (w != PORT_HD_POSTER_W || h != PORT_HD_POSTER_H) return 0;
    source = SDL_LoadPNG(path);
    if (!source) return 0;
    target = SDL_CreateSurfaceFrom(ow, oh, SDL_PIXELFORMAT_ARGB8888, out, ow * 4);
    ok = target && SDL_BlitSurfaceScaled(source, NULL, target, NULL, SDL_SCALEMODE_LINEAR) &&
        PortHdPosterHasPicture(out, ow, oh, ow * 4);
    SDL_DestroySurface(target); SDL_DestroySurface(source);
    return ok;
}

int PortHdScenePreviewPath(const char* name, char* out, size_t size) {
    char path[1024];
    SDL_PathInfo info;
    int n;
    if (!name || !*name || !out || !size) return 0;
    /* SDL's path API uses UTF-8 on Windows too; stdio's ANSI probe would
       miss a scene in a Unicode data directory and choose the legacy folder. */
    SDL_snprintf(path, sizeof(path), "%s%s.mp4", PortConfigPath("hd/scenes/"), name);
    if (strncmp(name, "zz_event_", 9) || !SDL_GetPathInfo(path, &info))
        SDL_snprintf(path, sizeof(path), "%s%s.mp4", PortConfigPath("hd/movies/"), name);
    n = SDL_snprintf(out, size, "%s", path);
    return n >= 0 && (size_t)n < size;
}

int PortHdScenePreview(const char* name, uint32_t* out, int ow, int oh, int second) {
    char path[1024];
    return PortHdScenePreviewPath(name, path, sizeof(path)) && PortHdScenePreviewFile(path, out, ow, oh, second);
}

#ifdef _WIN32
/* One selected gallery still, with no audio stream, textures or background decoder retained. */
int PortHdScenePreviewFile(const char* path, uint32_t* out, int ow, int oh, int second) {
    WCHAR wpath[1024];
    IMFAttributes* attrs = NULL;
    IMFSourceReader* reader = NULL;
    IMFMediaType* type = NULL;
    IMFSample* sample = NULL;
    IMFMediaBuffer* buffer = NULL;
    IMF2DBuffer* two = NULL;
    UINT64 size = 0;
    UINT32 width = 0, height = 0;
    int ok = 0, i, x, y;
    BYTE* pixels = NULL;
    LONG pitch = 0;
    DWORD length = 0;
    HRESULT com, format;
    if (!path || !*path || !out || ow <= 0 || oh <= 0 || ow > 854 || oh > 480 || second < 0) return 0;
    if (ReadImportedPoster(path, out, ow, oh, second)) return 1;
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024)) return 0;
    if (GetFileAttributesW(wpath) == INVALID_FILE_ATTRIBUTES) return 0;
    com = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) return 0;
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
        if (SUCCEEDED(com)) CoUninitialize();
        return 0;
    }
    if (FAILED(MFCreateAttributes(&attrs, 1))) goto done;
    IMFAttributes_SetUINT32(attrs, &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    if (FAILED(MFCreateSourceReaderFromURL(wpath, attrs, &reader))) goto done;
    IMFSourceReader_SetStreamSelection(reader, (DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
    IMFSourceReader_SetStreamSelection(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    if (FAILED(MFCreateMediaType(&type))) goto done;
    IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, (UINT64)(unsigned)ow << 32 | (unsigned)oh);
    /* Some older video processors only accept their native output size. */
    format = IMFSourceReader_SetCurrentMediaType(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, type);
    if (FAILED(format)) {
        IMFMediaType_DeleteItem(type, &MF_MT_FRAME_SIZE);
        format = IMFSourceReader_SetCurrentMediaType(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, type);
    }
    if (FAILED(format)) goto done;
    IMFMediaType_Release(type); type = NULL;
    if (FAILED(IMFSourceReader_GetCurrentMediaType(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &type))) goto done;
    if (FAILED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &size))) goto done;
    width = (UINT32)(size >> 32); height = (UINT32)size;
    if (!width || !height || width > 4096 || height > 2160) goto done;
    {
        static const GUID timeFormat = {0};
        PROPVARIANT at;
        PropVariantInit(&at); at.vt = VT_I8; at.hVal.QuadPart = (LONGLONG)second * 10000000;
        IMFSourceReader_SetCurrentPosition(reader, &timeFormat, &at);
        PropVariantClear(&at);
    }
    /* MF seeks to a preceding keyframe. Decode its preroll instead of caching
       that often-black first frame forever as the requested gallery still. */
    for (i = 0; i < 900 && !sample; i++) {
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        if (FAILED(IMFSourceReader_ReadSample(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &flags, &timestamp, &sample))) goto done;
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) goto done;
        if (sample && timestamp < (LONGLONG)second * 10000000) {
            IMFSample_Release(sample); sample = NULL;
        }
    }
    if (!sample || FAILED(IMFSample_ConvertToContiguousBuffer(sample, &buffer))) goto done;
    if (SUCCEEDED(IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void**)&two)) &&
        SUCCEEDED(IMF2DBuffer_Lock2D(two, &pixels, &pitch))) {
        for (y = 0; y < oh; y++) for (x = 0; x < ow; x++)
            out[y * ow + x] = ((const uint32_t*)(pixels + (LONG)(y * height / oh) * pitch))[x * width / ow] | 0xFF000000u;
        IMF2DBuffer_Unlock2D(two);
        ok = 1;
    } else if (SUCCEEDED(IMFMediaBuffer_Lock(buffer, &pixels, NULL, &length))) {
        if (length >= (size_t)width * height * 4) {
            for (y = 0; y < oh; y++) for (x = 0; x < ow; x++)
                out[y * ow + x] = ((const uint32_t*)pixels)[(y * height / oh) * width + x * width / ow] | 0xFF000000u;
            ok = 1;
        }
        IMFMediaBuffer_Unlock(buffer);
    }
done:
    if (two) IMF2DBuffer_Release(two);
    if (buffer) IMFMediaBuffer_Release(buffer);
    if (sample) IMFSample_Release(sample);
    if (type) IMFMediaType_Release(type);
    if (reader) IMFSourceReader_Release(reader);
    if (attrs) IMFAttributes_Release(attrs);
    MFShutdown();
    if (SUCCEEDED(com)) CoUninitialize();
    return ok;
}
#endif

/* START (the pad's or the keyboard's), Escape or a tap pause the movie under the game's scene pause (back, or skip
   after asking); A and B do nothing, so a press meant for the dialogue before never ends a movie. 1: pause wanted. */
static int sMovieDeviceReset;
static int MovieResetEvent(const SDL_Event* event) {
#if __STDC_HOSTED__ && !defined(__EMSCRIPTEN__)
    if (PortRendererHandleReset(event->type)) {
        if (event->type == SDL_EVENT_RENDER_DEVICE_RESET) sMovieDeviceReset = 1;
        return 1;
    }
#else
    (void)event;
#endif
    return 0;
}

static int PauseWanted(int* quit) {
    SDL_Event e;
    int pause = 0;
    while (SDL_PollEvent(&e)) {
        if (MovieResetEvent(&e)) { if (sMovieDeviceReset) return 0; continue; }
        switch (e.type) {
            case SDL_EVENT_QUIT:
                *quit = 1;
                return 0;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat) {
                    int i;
                    pause |= e.key.key == SDLK_ESCAPE;
                    for (i = 0; i < 2; i++)
                        pause |= gPortConfig.keys[PORT_ACT_START][i] != 0 &&
                                 e.key.key == (SDL_Keycode)gPortConfig.keys[PORT_ACT_START][i];
                }
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                pause |= e.gbutton.button == SDL_GAMEPAD_BUTTON_START;
                break;
            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                pause = 1;
                break;
            default:
                break;
        }
    }
    return pause;
}

/* the presses that started the movie (the dialogue's A) are not the movie's */
static void DropPendingInput(void) {
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP);
    SDL_FlushEvents(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_EVENT_MOUSE_BUTTON_UP);
    SDL_FlushEvents(SDL_EVENT_GAMEPAD_BUTTON_DOWN, SDL_EVENT_GAMEPAD_BUTTON_UP);
    SDL_FlushEvents(SDL_EVENT_FINGER_DOWN, SDL_EVENT_FINGER_UP);
}

/* The scene pause over the frozen picture: the frame as shown (shot), brought to the game's canvas, with the pause
   window drawn on it by port_ui.c as over any scene. The sound waits paused. 1: skip the movie. */
static int MoviePause(SDL_Renderer* renderer, SDL_Surface* shot, SDL_AudioStream* audio, int* quit) {
    extern void PortUiOpenHdMoviePause(void);
    extern void PortUiCancelHdMoviePause(void);
    extern int PortUiHdMovieSkipTaken(void);
    extern int PortUiHandleEvent(const void* event);
    extern void PortUiDraw(uint32_t* frame, int width, int height);
    extern void PortUiHdDraw(SDL_Renderer* renderer, float x, float y, float scale);
    extern int PortUiActive(void);
    extern void PortMenuSoundBegin(void);
    extern void PortMenuSoundDecide(int confirm);
    extern void PortMenuSoundFrame(void);
    extern void PortMenuSoundEnd(void);
    extern void PortAudioMenuFrame(void);
    extern int PortUiTakeMenuSound(int open);
    int cw = gPortScreen.width, ch = gPortScreen.height, x, y;
    uint32_t *base, *work;
    SDL_Surface* argb = shot ? SDL_ConvertSurface(shot, SDL_PIXELFORMAT_ARGB8888) : NULL;
    SDL_Texture* layer;
    if (!argb || cw <= 0 || ch <= 0) {
        SDL_DestroySurface(argb);
        return 0;
    }
    base = SDL_malloc((size_t)cw * ch * 4);
    work = SDL_malloc((size_t)cw * ch * 4);
    layer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, cw, ch);
    if (!base || !work || !layer) {
        SDL_free(base);
        SDL_free(work);
        SDL_DestroyTexture(layer);
        SDL_DestroySurface(argb);
        return 0;
    }
    for (y = 0; y < ch; y++) {
        const uint32_t* row = (const uint32_t*)((const uint8_t*)argb->pixels + (size_t)(y * argb->h / ch) * argb->pitch);
        for (x = 0; x < cw; x++)
            base[y * cw + x] = row[x * argb->w / cw];
    }
    SDL_DestroySurface(argb);
    SDL_SetTextureScaleMode(layer, SDL_SCALEMODE_NEAREST);
    if (audio)
        SDL_PauseAudioStreamDevice(audio);
    PortUiOpenHdMoviePause();
    /* the pause sounds as the game's menus do (its cursor and choices, as over the game's own scenes) */
    PortAudioMovieHold(0);
    PortMenuSoundBegin();
    if (PortUiTakeMenuSound(1))
        PortMenuSoundDecide(0);
    {
        /* test harness (KHCOM_HDMOVIE_PAUSETEST): the pause is walked to "skip" and "yes" by key presses */
        static const SDL_Keycode walk[] = { SDLK_DOWN, SDLK_RETURN, SDLK_UP, SDLK_RETURN };
        static const SDL_Keycode cancelWalk[] = { SDLK_DOWN, SDLK_RETURN, SDLK_ESCAPE, SDLK_ESCAPE };
        int step = 0, tick = 0, test = SDL_getenv("KHCOM_HDMOVIE_PAUSETEST") != NULL;
        const char* pauseAction = SDL_getenv("KHCOM_HDMOVIE_PAUSE_ACTION");
    while (PortUiActive() && !*quit) {
        SDL_Event e;
        if (test && ++tick % 10 == 0 && step < 4) {
            SDL_Event k;
            SDL_zero(k);
            k.type = SDL_EVENT_KEY_DOWN;
            k.key.key = pauseAction && !SDL_strcmp(pauseAction, "cancel") ? cancelWalk[step++] : walk[step++];
            k.key.down = true;
            SDL_PushEvent(&k);
        }
        while (SDL_PollEvent(&e)) {
            if (MovieResetEvent(&e)) { if (sMovieDeviceReset) break; continue; }
            if (e.type == SDL_EVENT_QUIT)
                *quit = 1;
            else
                PortUiHandleEvent(&e);
        }
        if (sMovieDeviceReset) break;
        PortMenuSoundFrame();
        PortAudioMenuFrame();
        SDL_memcpy(work, base, (size_t)cw * ch * 4);
        PortUiDraw(work, cw, ch);
        SDL_UpdateTexture(layer, NULL, work, cw * 4);
        SDL_SetRenderViewport(renderer, NULL);
        SDL_SetRenderClipRect(renderer, NULL);
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        SDL_RenderTexture(renderer, layer, NULL, NULL);
        {
            int ow, oh;
            SDL_GetCurrentRenderOutputSize(renderer, &ow, &oh);
            if (ow > 0 && oh > 0) {
                /* PortUiDraw defers HD glyphs to this renderer pass. Match the canvas stretched above,
                   keeping the font at the output's vertical resolution even in a resized window. */
                float scale = (float)oh / ch;
                SDL_SetRenderScale(renderer, (float)ow / (cw * scale), 1.0f);
                PortUiHdDraw(renderer, 0, 0, scale);
                SDL_SetRenderScale(renderer, 1.0f, 1.0f);
            }
        }
        if (test && (tick == 5 || tick == 25)) {
            SDL_Surface* s = SDL_RenderReadPixels(renderer, NULL); /* include the deferred text, as presented */
            if (s) {
                SDL_SaveBMP(s, PortConfigPath(tick == 5 ? "hdmovie_pause.bmp" : "hdmovie_skip_confirm.bmp"));
                SDL_DestroySurface(s);
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    }
    if (sMovieDeviceReset) PortUiCancelHdMoviePause();
    PortMenuSoundEnd();
    (void)PortUiTakeMenuSound(0);
    PortAudioMovieHold(1); /* the pause's last sound fades, the movie has the sound again */
    SDL_DestroyTexture(layer);
    SDL_free(base);
    SDL_free(work);
    if (audio)
        SDL_ResumeAudioStreamDevice(audio);
    DropPendingInput();
    return PortUiHdMovieSkipTaken() || *quit;
}

static int PlayFile(const char* name);

/* ---- subtitles ------------------------------------------------------------------------------------------------ */

#define SUB_MAX 256
typedef struct {
    int count, shown; /* shown: the line its texture holds, -1 none */
    unsigned short first[SUB_MAX], last[SUB_MAX];
    SubChar* text[SUB_MAX];
    SDL_Texture* texture;
    int w, h, height; /* texture size; the output height it was made for */
} Subs;

static int LoadSubs(Subs* s, const char* name) {
    const char* order[2];
    char path[1024], line[1024];
    FILE* f = NULL;
    int i;

    SDL_zerop(s);
    s->shown = -1;
    if (!gPortConfig.hdSubtitles)
        return 0;
    SubsOrder(order);
    for (i = 0; i < 2 && !f; i++) {
        SDL_snprintf(path, sizeof(path), "%s%s.%s.txt", PortConfigPath("hd/subs/"), name, order[i]);
        f = fopen(path, "rb");
    }
    if (!f)
        return 0;
    while (s->count < SUB_MAX && fgets(line, sizeof(line), f)) {
        unsigned first, last;
        int used = 0, n;
        char* text;
        char* w;
        if (sscanf(line, "%u %u %n", &first, &last, &used) < 2 || used == 0)
            continue;
        text = line + used;
        for (w = text; *text; text++) { /* "\n" breaks the line; the line's own end goes */
            if (text[0] == '\\' && text[1] == 'n') {
                *w++ = '\n';
                text++;
            } else if (*text != '\r' && *text != '\n') {
                *w++ = *text;
            }
        }
        *w = 0;
#ifdef _WIN32
        n = MultiByteToWideChar(CP_UTF8, 0, line + used, -1, NULL, 0);
        if (n <= 0)
            continue;
        s->text[s->count] = SDL_malloc(sizeof(WCHAR) * (size_t)n);
        if (!s->text[s->count])
            break;
        MultiByteToWideChar(CP_UTF8, 0, line + used, -1, s->text[s->count], n);
#else
        (void)n;
        s->text[s->count] = SDL_strdup(line + used);
        if (!s->text[s->count])
            break;
#endif
        s->first[s->count] = (unsigned short)first;
        s->last[s->count] = (unsigned short)last;
        s->count++;
    }
    fclose(f);
    PortLog("hd movie: %d subtitles (%s)", s->count, path);
    return s->count;
}

static void FreeSubs(Subs* s) {
    int i;
    for (i = 0; i < s->count; i++)
        SDL_free(s->text[i]);
    if (s->texture)
        SDL_DestroyTexture(s->texture);
    SDL_zerop(s);
}

/* the letters' coverage (s->w x s->h) as white letters with a dark outline spread over a disc of radius pad */
static void SubTextureFromCover(Subs* s, SDL_Renderer* renderer, const Uint8* cover, int pad) {
    int x, y, dx, dy;
    Uint32* rgba = SDL_malloc((size_t)s->w * s->h * 4);
    if (!rgba)
        return;
    for (y = 0; y < s->h; y++)
        for (x = 0; x < s->w; x++) {
            int a = cover[y * s->w + x], o = a;
            for (dy = -pad; dy <= pad && o < 255; dy++)
                for (dx = -pad; dx <= pad; dx++) {
                    int yy = y + dy, xx = x + dx;
                    if (dx * dx + dy * dy > pad * pad + pad || yy < 0 || xx < 0 || yy >= s->h || xx >= s->w)
                        continue;
                    o = SDL_max(o, cover[yy * s->w + xx]);
                }
            rgba[y * s->w + x] = (Uint32)(o * 230 / 255 > a ? o * 230 / 255 : a) << 24 | (Uint32)a << 16 |
                                 (Uint32)a << 8 | (Uint32)a;
        }
    s->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, s->w, s->h);
    if (s->texture) {
        SDL_UpdateTexture(s->texture, NULL, rgba, s->w * 4);
        SDL_SetTextureBlendMode(s->texture, SDL_BLENDMODE_BLEND);
    }
    SDL_free(rgba);
}

#ifdef _WIN32
/* the line as white letters with a dark outline, at a size for the output height */
static void MakeSubTexture(Subs* s, SDL_Renderer* renderer, int line, int outH) {
    static int fontTried;
    HDC dc = CreateCompatibleDC(NULL);
    int px = SDL_max(12, outH / 22), pad = SDL_max(2, px / 9), y;
    HFONT font, old;
    RECT r = { 0, 0, 0, 0 };
    BITMAPINFO bi;
    void* bits = NULL;
    HBITMAP bmp;
    const WCHAR* text = s->text[line];

    if (!fontTried) { /* the KHMenu face the HD text uses (port_hdtext.c), in the data folder or next to the game */
        const char* base = SDL_GetBasePath();
        char path[1024];
        WCHAR wpath[1024];
        fontTried = 1;
        SDL_strlcpy(path, PortConfigPath("assets/fonts/KHMenu.otf"), sizeof(path));
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
            SDL_snprintf(path, sizeof(path), "%sassets/fonts/KHMenu.otf", base ? base : "");
        if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024) > 0)
            AddFontResourceExW(wpath, FR_PRIVATE, NULL);
    }
    if (s->texture)
        SDL_DestroyTexture(s->texture);
    s->texture = NULL;
    s->shown = line;
    s->height = outH;
    font = CreateFontW(-px, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS,
                       CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH | FF_SWISS, L"KHMenu");
    old = SelectObject(dc, font);
    DrawTextW(dc, text, -1, &r, DT_CALCRECT | DT_CENTER | DT_NOPREFIX);
    s->w = r.right + pad * 2 + 2;
    s->h = r.bottom + pad * 2 + 2;
    SDL_zero(bi);
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = s->w;
    bi.bmiHeader.biHeight = -s->h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (bmp && bits) {
        HGDIOBJ oldBmp = SelectObject(dc, bmp);
        const Uint32* src = bits;
        Uint8* cover = SDL_malloc((size_t)s->w * s->h);
        RECT box = { pad + 1, pad + 1, s->w - pad - 1, s->h - pad - 1 };
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkMode(dc, TRANSPARENT);
        DrawTextW(dc, text, -1, &box, DT_CENTER | DT_NOPREFIX);
        GdiFlush();
        if (cover) {
            for (y = 0; y < s->h * s->w; y++)
                cover[y] = (Uint8)((src[y] >> 8) & 255);
            SubTextureFromCover(s, renderer, cover, pad);
        }
        SDL_free(cover);
        SelectObject(dc, oldBmp);
        DeleteObject(bmp);
    }
    SelectObject(dc, old);
    DeleteObject(font);
    DeleteDC(dc);
}
#else
/* the line in KHMenu by FreeType (the HD text's, port/portable/hdtext_freetype.c), outlined as on Windows */
static void MakeSubTexture(Subs* s, SDL_Renderer* renderer, int line, int outH) {
    int px = SDL_max(12, outH / 22), pad = SDL_max(2, px / 9);
    Uint8* cover;
    if (s->texture)
        SDL_DestroyTexture(s->texture);
    s->texture = NULL;
    s->shown = line;
    s->height = outH;
    cover = PortPortableHdTextCover(s->text[line], px, pad, &s->w, &s->h);
    if (cover)
        SubTextureFromCover(s, renderer, cover, pad);
    SDL_free(cover);
}
#endif

/* the subtitle due at clock (100 ns units; the subtitles count the movie's frames at 30 a second) */
static void DrawSubs(Subs* s, SDL_Renderer* renderer, LONGLONG clock, const SDL_FRect* picture, int outH) {
    int frame = (int)(clock * 30 / 10000000), i, line = -1;
    SDL_FRect dst;
    for (i = 0; i < s->count; i++)
        if (frame >= s->first[i] && frame <= s->last[i])
            line = i;
    if (line < 0)
        return;
    if (line != s->shown || outH != s->height || !s->texture)
        MakeSubTexture(s, renderer, line, outH);
    if (!s->texture)
        return;
    dst.w = (float)s->w;
    dst.h = (float)s->h;
    dst.x = SDL_roundf(picture->x + (picture->w - dst.w) / 2);
    dst.y = SDL_roundf(SDL_min(picture->y + picture->h, (float)outH) - dst.h - outH / 18.0f);
    SDL_RenderTexture(renderer, s->texture, NULL, &dst);
}

/* Plays hd/movies/<name>.mp4 (a scene: hd/scenes, MoviePath) to the end (or until skipped). 0: nothing imported or it could not be opened. */
#ifdef _WIN32
static int PlayFile(const char* name) {
    sMovieDeviceReset = 0;
    char path[1024];
    WCHAR wpath[1024];
    IMFSourceReader* reader = NULL;
    IMFMediaType* type = NULL;
    IMFAttributes* attrs = NULL;
    SDL_Renderer* renderer = PortRenderer();
    SDL_Texture* texture = NULL;
    SDL_AudioStream* audio = NULL;
    UINT32 width = 0, height = 0, rate = 48000, channels = 2;
    UINT32 bufferHeight = 0; /* NV12: the decoder's rows (1088 for a 1080p H.264), the colour plane after them */
    LONGLONG frameTime = 333333; /* 100 ns units per frame, from the frame rate */
    LONGLONG duration = 0;       /* 100 ns units, from the file (0: unknown) */
    LONGLONG skipEnd = -1;       /* skipped: the movie fades out until this clock, then ends */
    int nv12 = 0, dropped = 0;
    LONGLONG audioPushed = 0; /* 100 ns units of audio handed to the stream */
    LONGLONG nextVideo = -1;
    IMFSample* pending = NULL;
    int quit = 0, videoDone = 0, audioDone = 0, played = 0, shown = 0, tested = 0;
    Subs subs;
    HRESULT hr;

    if (name == NULL || renderer == NULL)
        return 0;
    MoviePath(name, path, sizeof(path));
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
        return 0;
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024);

    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE)))
        return 0;
    hr = MFCreateAttributes(&attrs, 1);
    if (FAILED(hr))
        goto done;
    IMFAttributes_SetUINT32(attrs, &MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    hr = MFCreateSourceReaderFromURL(wpath, attrs, &reader);
    IMFAttributes_Release(attrs);
    attrs = NULL;
    if (FAILED(hr)) {
        PortLog("hd movie: %s does not open (0x%08lx)", path, (unsigned long)hr);
        MFShutdown();
        return 0;
    }
    /* video as the decoder's NV12 (32-bit BGRX if it cannot, or for SDL's software renderer: its NV12 textures with
       the BT.709 colour space crashed the game), audio as 16-bit PCM */
    for (nv12 = SDL_strcmp(SDL_GetRendererName(renderer), "software") != 0; nv12 >= 0; nv12--) {
        hr = MFCreateMediaType(&type);
        if (FAILED(hr))
            goto done;
        IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
        IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, nv12 ? &MFVideoFormat_NV12 : &MFVideoFormat_RGB32);
        hr = IMFSourceReader_SetCurrentMediaType(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, type);
        IMFMediaType_Release(type);
        type = NULL;
        if (SUCCEEDED(hr))
            break;
    }
    if (SUCCEEDED(hr)) {
        IMFMediaType* actual = NULL;
        if (SUCCEEDED(IMFSourceReader_GetCurrentMediaType(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &actual))) {
            UINT64 size = 0, fps = 0;
            MFVideoArea area;
            IMFMediaType_GetUINT64(actual, &MF_MT_FRAME_SIZE, &size);
            width = (UINT32)(size >> 32);
            height = bufferHeight = (UINT32)size;
            /* the picture inside the decoder's padded frame */
            if (SUCCEEDED(IMFMediaType_GetBlob(actual, &MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8*)&area, sizeof(area), NULL)) &&
                area.Area.cx > 0 && area.Area.cy > 0 && (UINT32)area.Area.cy <= height) {
                width = (UINT32)area.Area.cx;
                height = (UINT32)area.Area.cy;
            }
            if (SUCCEEDED(IMFMediaType_GetUINT64(actual, &MF_MT_FRAME_RATE, &fps)) && (UINT32)(fps >> 32) > 0 && (UINT32)fps > 0)
                frameTime = (LONGLONG)10000000 * (UINT32)fps / (UINT32)(fps >> 32);
            IMFMediaType_Release(actual);
        }
    }
    {
        PROPVARIANT var;
        PropVariantInit(&var);
        if (SUCCEEDED(IMFSourceReader_GetPresentationAttribute(reader, (DWORD)MF_SOURCE_READER_MEDIASOURCE, &MF_PD_DURATION,
                                                               &var)) && var.vt == VT_UI8)
            duration = (LONGLONG)var.uhVal.QuadPart;
        PropVariantClear(&var);
    }
    hr = MFCreateMediaType(&type);
    if (FAILED(hr))
        goto done;
    IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    if (FAILED(IMFSourceReader_SetCurrentMediaType(reader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, type)))
        audioDone = 1;
    IMFMediaType_Release(type);
    type = NULL;
    if (!audioDone) {
        IMFMediaType* actual = NULL;
        if (SUCCEEDED(IMFSourceReader_GetCurrentMediaType(reader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actual))) {
            IMFMediaType_GetUINT32(actual, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
            IMFMediaType_GetUINT32(actual, &MF_MT_AUDIO_NUM_CHANNELS, &channels);
            IMFMediaType_Release(actual);
        }
        {
            SDL_AudioSpec spec = { SDL_AUDIO_S16, (int)channels, (int)rate };
            audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
            if (audio)
                SDL_ResumeAudioStreamDevice(audio);
            else
                audioDone = 1;
        }
    }
    if (width == 0 || height == 0) {
        PortLog("hd movie: %s has no usable video", path);
        goto done;
    }
    {
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER,
                              nv12 ? SDL_PIXELFORMAT_NV12 : SDL_PIXELFORMAT_XRGB8888);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, width);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, height);
        if (nv12) /* HD video: BT.709, studio range */
            SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_BT709_LIMITED);
        texture = SDL_CreateTextureWithProperties(renderer, props);
        SDL_DestroyProperties(props);
    }
    if (!texture) {
        PortLog("hd movie: no %s texture (%s)", nv12 ? "NV12" : "RGB", SDL_GetError());
        goto done;
    }
    SDL_SetTextureScaleMode(texture, (SDL_ScaleMode)PortSmoothScaleMode()); /* LINEAR, but not on SDL's software renderer */
    if (sStartAt > 0) {
        static const GUID kTimeFormat = { 0 }; /* GUID_NULL: 100 ns units (not linked from uuid.lib here) */
        PROPVARIANT at;
        PropVariantInit(&at);
        at.vt = VT_I8;
        at.hVal.QuadPart = (LONGLONG)sStartAt * 10000000;
        if (SUCCEEDED(IMFSourceReader_SetCurrentPosition(reader, &kTimeFormat, &at)))
            audioPushed = at.hVal.QuadPart;
        PropVariantClear(&at);
    }
    /* the first picture before anything is shown: a system whose Media Foundation opens the file but cannot decode
       its H.264 (Wine/Proton without the codec) keeps the cartridge's scene instead of a black movie */
    {
        int tries;
        for (tries = 0; tries < 64 && !pending; tries++) {
            DWORD flags = 0;
            LONGLONG ts = 0;
            hr = IMFSourceReader_ReadSample(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &flags, &ts, &pending);
            if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM))
                break;
            if (pending)
                nextVideo = ts;
        }
        if (!pending) {
            PortLog("hd movie: %s gives no picture (0x%08lx): the cartridge's instead", path, (unsigned long)hr);
            goto done;
        }
    }
    PortLog("hd movie: %s (%ux%u%s, %u Hz)", name, width, height, nv12 ? " NV12" : " RGB", rate);
    LoadSubs(&subs, name);
    played = 1;
    PortAudioMovieHold(1);
    DropPendingInput();

    while (!quit && !(videoDone && audioDone)) {
        Uint64 frameStart = SDL_GetTicksNS();
        LONGLONG clock;
        int pause = PauseWanted(&quit);
        {
            const char* at = SDL_getenv("KHCOM_HDMOVIE_PAUSETEST");
            if (at && !tested && nextVideo >= (LONGLONG)SDL_atoi(at) * 10000000) {
                tested = 1;
                pause = 1;
            }
        }
        if (quit || sMovieDeviceReset)
            break;
        /* keep about half a second of audio queued: it is the clock */
        while (!audioDone && audio && SDL_GetAudioStreamQueued(audio) < (int)(rate * channels * 2 / 2)) {
            DWORD flags = 0;
            LONGLONG ts = 0;
            IMFSample* sample = NULL;
            hr = IMFSourceReader_ReadSample(reader, (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &flags, &ts, &sample);
            if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
                audioDone = 1;
                if (sample)
                    IMFSample_Release(sample);
                break;
            }
            if (sample) {
                IMFMediaBuffer* buffer = NULL;
                if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(sample, &buffer))) {
                    BYTE* data;
                    DWORD length = 0;
                    if (SUCCEEDED(IMFMediaBuffer_Lock(buffer, &data, NULL, &length))) {
                        DWORD trim = 0;
                        /* MF seeks to a keyframe, not necessarily the requested second. Its PCM can start
                           earlier too (#42): never play that preroll under video already at sStartAt. */
                        LONGLONG first = (LONGLONG)sStartAt * 10000000;
                        if (ts < first) {
                            LONGLONG frames = (first - ts) * rate / 10000000;
                            trim = frames >= length / (channels * 2) ? length : (DWORD)frames * channels * 2;
                        }
                        if (trim < length) {
                            SDL_PutAudioStreamData(audio, data + trim, (int)(length - trim));
                            /* The queue's end is in the file's timeline, just like the video/subtitles.
                               Counting bytes from the requested seek time shifts it by the preroll. */
                            audioPushed = ts + (LONGLONG)length * 10000000 / (rate * channels * 2);
                        }
                        IMFMediaBuffer_Unlock(buffer);
                    }
                    IMFMediaBuffer_Release(buffer);
                }
                IMFSample_Release(sample);
            }
        }
        /* the presentation time: audio played so far, or the wall clock without audio */
        if (audio && !audioDone)
            clock = audioPushed - (LONGLONG)SDL_GetAudioStreamQueued(audio) * 10000000 / (rate * channels * 2);
        else
            clock = nextVideo >= 0 ? nextVideo : 0;
        if (skipEnd >= 0 && clock >= skipEnd)
            break;
        {
            /* test harness (test_hd_scenes.py): KHCOM_HDMOVIE_SECONDS=<n> skips every movie after n seconds */
            const char* limit = SDL_getenv("KHCOM_HDMOVIE_SECONDS");
            if (limit && skipEnd < 0 && clock >= (LONGLONG)SDL_atoi(limit) * 10000000) {
                PortLog("hd movie: test stop at %d s", SDL_atoi(limit));
                skipEnd = clock + 3000000;
            }
        }
        /* show the latest video frame due by now */
        while (!videoDone) {
            if (!pending) {
                DWORD flags = 0;
                LONGLONG ts = 0;
                hr = IMFSourceReader_ReadSample(reader, (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &flags, &ts, &pending);
                if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
                    videoDone = 1;
                    break;
                }
                if (!pending)
                    continue;
                nextVideo = ts;
            }
            if (nextVideo > clock + 100000) /* more than 10 ms ahead: later */
                break;
            if (audio && !audioDone && nextVideo + frameTime <= clock) {
                dropped++; /* its time is over already: the next frame is due, skip the upload */
            } else if (nv12) {
                IMFMediaBuffer* buffer = NULL;
                IMF2DBuffer* buffer2d = NULL;
                if (SUCCEEDED(IMFSample_GetBufferByIndex(pending, 0, &buffer))) {
                    BYTE* scan0;
                    LONG pitch;
                    if (SUCCEEDED(IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void**)&buffer2d)) &&
                        SUCCEEDED(IMF2DBuffer_Lock2D(buffer2d, &scan0, &pitch))) {
                        /* the colour plane follows the decoder's rows, which can be more than the type says (1080p
                           H.264 decodes as 1088 rows): count them from the buffer, or the colour sits rows off */
                        DWORD length = 0;
                        UINT32 rows = bufferHeight;
                        if (pitch > 0 && SUCCEEDED(IMFMediaBuffer_GetCurrentLength(buffer, &length)) &&
                            length / 3 * 2 / (DWORD)pitch > rows)
                            rows = length / 3 * 2 / (DWORD)pitch;
                        if (pitch > 0)
                            SDL_UpdateNVTexture(texture, NULL, scan0, pitch, scan0 + (size_t)pitch * rows, pitch);
                        IMF2DBuffer_Unlock2D(buffer2d);
                    } else {
                        BYTE* data;
                        DWORD length = 0;
                        if (SUCCEEDED(IMFMediaBuffer_Lock(buffer, &data, NULL, &length))) {
                            /* packed rows of the frame's width, the colour plane after all of them */
                            UINT32 rows = length / 3 * 2 / width;
                            if (rows >= height)
                                SDL_UpdateNVTexture(texture, NULL, data, (int)width, data + (size_t)width * rows, (int)width);
                            IMFMediaBuffer_Unlock(buffer);
                        }
                    }
                    if (buffer2d)
                        IMF2DBuffer_Release(buffer2d);
                    IMFMediaBuffer_Release(buffer);
                }
            } else {
                IMFMediaBuffer* buffer = NULL;
                IMF2DBuffer* buffer2d = NULL;
                if (SUCCEEDED(IMFSample_GetBufferByIndex(pending, 0, &buffer))) {
                    BYTE* scan0;
                    LONG pitch;
                    if (SUCCEEDED(IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void**)&buffer2d)) &&
                        SUCCEEDED(IMF2DBuffer_Lock2D(buffer2d, &scan0, &pitch))) {
                        if (pitch >= 0) {
                            SDL_UpdateTexture(texture, NULL, scan0, pitch);
                        } else { /* bottom-up rows */
                            void* pixels;
                            int texPitch;
                            if (SDL_LockTexture(texture, NULL, &pixels, &texPitch)) {
                                UINT32 y;
                                for (y = 0; y < height; y++)
                                    memcpy((BYTE*)pixels + (size_t)y * texPitch, scan0 + (LONG)y * pitch, width * 4);
                                SDL_UnlockTexture(texture);
                            }
                        }
                        IMF2DBuffer_Unlock2D(buffer2d);
                    } else {
                        /* a plain buffer (the video processor's output): top-down rows of width * 4 bytes */
                        BYTE* data;
                        DWORD length = 0;
                        if (SUCCEEDED(IMFMediaBuffer_Lock(buffer, &data, NULL, &length))) {
                            if (length >= width * height * 4)
                                SDL_UpdateTexture(texture, NULL, data, (int)(width * 4));
                            IMFMediaBuffer_Unlock(buffer);
                        }
                    }
                    if (buffer2d)
                        IMF2DBuffer_Release(buffer2d);
                    IMFMediaBuffer_Release(buffer);
                }
            }
            IMFSample_Release(pending);
            pending = NULL;
            shown++;
            if (!audio || audioDone)
                break; /* without audio: one frame per loop, paced below */
        }
        {
            int ow = 0, oh = 0;
            SDL_FRect dst;
            float scale;
            SDL_GetCurrentRenderOutputSize(renderer, &ow, &oh);
            scale = SDL_min((float)ow / width, (float)oh / height);
            dst.w = width * scale;
            dst.h = height * scale;
            dst.x = (ow - dst.w) / 2;
            dst.y = (oh - dst.h) / 2;
            SDL_SetRenderViewport(renderer, NULL);
            SDL_SetRenderClipRect(renderer, NULL);
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, &dst);
            DrawSubs(&subs, renderer, clock, &dst, oh);
            {
                /* the movie fades in from black and out to black (0.3 s each), as the game's own scenes do: no hard
                   cut from the field into the movie and back */
                const LONGLONG fade = 3000000;
                LONGLONG end = skipEnd >= 0 ? skipEnd : duration;
                LONGLONG edge = clock < fade ? clock : (end > fade && clock > end - fade ? end - clock : fade);
                /* the sound fades with the picture: a skip never cuts it with a pop */
                if (audio)
                    SDL_SetAudioStreamGain(audio, edge <= 0 ? 0.0f : (float)edge / fade);
                if (edge < fade) {
                    Uint8 a = (Uint8)(255 - (edge < 0 ? 0 : edge) * 255 / fade);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, a);
                    SDL_RenderFillRect(renderer, NULL);
                }
            }
            {
                /* test harness: KHCOM_HDMOVIE_SHOT=<seconds> saves that moment as hdmovie.bmp */
                static int shot;
                const char* at = SDL_getenv("KHCOM_HDMOVIE_SHOT");
                if (at && !shot && clock >= (LONGLONG)SDL_atoi(at) * 10000000) {
                    SDL_Surface* s = SDL_RenderReadPixels(renderer, NULL);
                    if (s) {
                        SDL_SaveBMP(s, PortConfigPath("hdmovie.bmp"));
                        SDL_DestroySurface(s);
                    }
                    shot = 1;
                }
            }
            if (pause) {
                /* the frame just drawn is the one that stays behind the pause window */
                SDL_Surface* frozen = SDL_RenderReadPixels(renderer, NULL);
                int skip = MoviePause(renderer, frozen, audio, &quit);
                SDL_DestroySurface(frozen);
                if (quit || sMovieDeviceReset)
                    break;
                if (skip && skipEnd < 0) /* out to black (and silence) in 0.3 s from here, as at the movie's end */
                    skipEnd = clock + 3000000;
                continue;
            }
            if (shown >= 10) {
                extern void PortSceneGalleryMovieCapture(SDL_Renderer* renderer);
                PortSceneGalleryMovieCapture(renderer);
            }
            SDL_RenderPresent(renderer);
        }
        {
            Uint64 spent = SDL_GetTicksNS() - frameStart;
            if (spent < 16000000)
                SDL_DelayNS(16000000 - spent);
        }
        if (audioDone && audio && SDL_GetAudioStreamQueued(audio) > 0 && videoDone)
            while (SDL_GetAudioStreamQueued(audio) > 0 && !quit && !sMovieDeviceReset && !PauseWanted(&quit))
                SDL_Delay(10);
    }

done:
    PortLog("hd movie: %d frames decoded, %d late ones not shown%s", shown, dropped, quit ? ", window closed" : "");
    if (attrs)
        IMFAttributes_Release(attrs);
    if (type)
        IMFMediaType_Release(type);
    if (pending)
        IMFSample_Release(pending);
    if (played) {
        FreeSubs(&subs);
        PortAudioMovieHold(0);
    }
    if (texture)
        SDL_DestroyTexture(texture);
    if (audio)
        SDL_DestroyAudioStream(audio);
    if (reader)
        IMFSourceReader_Release(reader);
    MFShutdown();
    if (quit) {
        SDL_Event q;
        SDL_zero(q);
        q.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&q); /* the main loop closes the game as usual */
    }
    return played && !sMovieDeviceReset;
}
#else
/* The path in single quotes for the shell ('\'' for a quote inside it). */
static void ShellQuote(const char* in, char* out, size_t size) {
    size_t n = 0;
    if (size < 3)
        return;
    out[n++] = '\'';
    for (; *in && n + 5 < size; in++) {
        if (*in == '\'') {
            SDL_memcpy(out + n, "'\\''", 4);
            n += 4;
        } else {
            out[n++] = *in;
        }
    }
    out[n++] = '\'';
    out[n] = 0;
}

int PortHdScenePreviewFile(const char* path, uint32_t* out, int ow, int oh, int second) {
    char quoted[2200], command[2600];
    FILE* decoder;
    int ok;
    if (!path || !*path || !out || ow <= 0 || oh <= 0 || ow > 854 || oh > 480 || second < 0) return 0;
    if (ReadImportedPoster(path, out, ow, oh, second)) return 1;
    if (access(path, R_OK)) return 0;
    ShellQuote(path, quoted, sizeof(quoted));
    SDL_snprintf(command, sizeof(command), "ffmpeg -v error -nostdin -threads 1 -ss %d -i %s -frames:v 1 -vf scale=%d:%d:flags=area "
                 "-an -f rawvideo -pix_fmt bgra - 2>/dev/null", second, quoted, ow, oh);
    decoder = popen(command, "r");
    if (!decoder) return 0;
    ok = fread(out, 4, (size_t)ow * oh, decoder) == (size_t)ow * oh;
    if (pclose(decoder) != 0) ok = 0; /* wait/reap: never leave a decoder behind when changing selection */
    return ok;
}

static int PlayFile(const char* name) {
    sMovieDeviceReset = 0;
    char path[1024], quoted[2200], command[2600], line[256];
    SDL_Renderer* renderer = PortRenderer();
    SDL_Texture* texture = NULL;
    SDL_AudioStream* audio = NULL;
    FILE *video = NULL, *sound = NULL, *probe;
    Uint8* frame = NULL;
    size_t frameBytes = 0;
    int width = 0, height = 0, outW = 0, outH = 0, rate = 48000, channels = 2;
    LONGLONG frameTime = 333333, duration = 0, skipEnd = -1, audioPushed = 0, nextVideo = 0;
    int quit = 0, videoDone = 0, audioDone = 0, played = 0, shown = 0, dropped = 0, havePending = 0, tested = 0;
    Subs subs;

    if (name == NULL || renderer == NULL)
        return 0;
    MoviePath(name, path, sizeof(path));
    if (access(path, R_OK) != 0)
        return 0;
    if (system("command -v ffmpeg >/dev/null 2>&1 && command -v ffprobe >/dev/null 2>&1") != 0) {
        static int told;
        if (!told++)
            PortLog("hd movie: no ffmpeg/ffprobe on this system: the cartridge's movies play");
        return 0;
    }
    ShellQuote(path, quoted, sizeof(quoted));
    /* the picture's size, frame rate and the movie's length */
    SDL_snprintf(command, sizeof(command), "ffprobe -v error -select_streams v:0 -show_entries "
                 "stream=width,height,avg_frame_rate:format=duration -of default=nw=1 %s 2>/dev/null", quoted);
    probe = popen(command, "r");
    if (!probe)
        return 0;
    while (fgets(line, sizeof(line), probe)) {
        unsigned num = 0, den = 0;
        double seconds = 0;
        if (SDL_sscanf(line, "width=%d", &width) == 1 || SDL_sscanf(line, "height=%d", &height) == 1)
            continue;
        if (SDL_sscanf(line, "avg_frame_rate=%u/%u", &num, &den) == 2 && num > 0 && den > 0)
            frameTime = (LONGLONG)10000000 * den / num;
        else if (SDL_sscanf(line, "duration=%lf", &seconds) == 1 && seconds > 0)
            duration = (LONGLONG)(seconds * 10000000);
    }
    pclose(probe);
    if (width <= 0 || height <= 0) {
        PortLog("hd movie: %s has no usable video", path);
        return 0;
    }
    /* frames come scaled to the window (never larger than the file): a 1080p movie in a 720p window moves less */
    SDL_GetCurrentRenderOutputSize(renderer, &outW, &outH);
    if (outW > 0 && outH > 0 && (outW < width || outH < height)) {
        float scale = SDL_min((float)outW / width, (float)outH / height);
        width = SDL_max(2, (int)(width * scale) & ~1);
        height = SDL_max(2, (int)(height * scale) & ~1);
    }
    frameBytes = (size_t)width * height * 4;
    frame = SDL_malloc(frameBytes);
    SDL_snprintf(command, sizeof(command), "ffmpeg -v error -nostdin -ss %d -i %s -an -sn -vf scale=%d:%d "
                 "-f rawvideo -pix_fmt bgr0 - 2>/dev/null", sStartAt, quoted, width, height);
    nextVideo = audioPushed = (LONGLONG)sStartAt * 10000000;
    video = frame ? popen(command, "r") : NULL;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!video || !texture) {
        PortLog("hd movie: %s does not open (%s)", path, texture ? "ffmpeg" : SDL_GetError());
        goto done;
    }
    SDL_SetTextureScaleMode(texture, (SDL_ScaleMode)PortSmoothScaleMode());
    /* the first picture before anything is shown: a file ffmpeg cannot decode keeps the cartridge's scene */
    if (fread(frame, 1, frameBytes, video) != frameBytes) {
        PortLog("hd movie: %s gives no picture: the cartridge's instead", path);
        goto done;
    }
    havePending = 1;
    SDL_snprintf(command, sizeof(command), "ffmpeg -v error -nostdin -ss %d -i %s -vn -sn -ac 2 -ar 48000 -f s16le - 2>/dev/null",
                 sStartAt, quoted);
    sound = popen(command, "r");
    {
        SDL_AudioSpec spec = { SDL_AUDIO_S16, channels, rate };
        audio = sound ? SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL) : NULL;
        if (audio)
            SDL_ResumeAudioStreamDevice(audio);
        else
            audioDone = 1;
    }
    PortLog("hd movie: %s (%dx%d RGB by ffmpeg, %d Hz)", name, width, height, rate);
    LoadSubs(&subs, name);
    played = 1;
    PortAudioMovieHold(1);
    DropPendingInput();

    while (!quit && !(videoDone && audioDone)) {
        Uint64 frameStart = SDL_GetTicksNS();
        LONGLONG clock;
        int pause = PauseWanted(&quit);
        {
            const char* at = SDL_getenv("KHCOM_HDMOVIE_PAUSETEST");
            if (at && !tested && nextVideo >= (LONGLONG)SDL_atoi(at) * 10000000) {
                tested = 1;
                pause = 1;
            }
        }
        if (quit || sMovieDeviceReset)
            break;
        /* keep about half a second of audio queued: it is the clock */
        while (!audioDone && SDL_GetAudioStreamQueued(audio) < rate * channels * 2 / 2) {
            Uint8 chunk[9600];
            size_t got = fread(chunk, 1, sizeof(chunk), sound);
            if (got == 0) {
                audioDone = 1;
                break;
            }
            SDL_PutAudioStreamData(audio, chunk, (int)got);
            audioPushed += (LONGLONG)got * 10000000 / (rate * channels * 2);
        }
        if (audio && !audioDone)
            clock = audioPushed - (LONGLONG)SDL_GetAudioStreamQueued(audio) * 10000000 / (rate * channels * 2);
        else
            clock = nextVideo;
        if (skipEnd >= 0 && clock >= skipEnd)
            break;
        {
            const char* limit = SDL_getenv("KHCOM_HDMOVIE_SECONDS");
            if (limit && skipEnd < 0 && clock >= (LONGLONG)SDL_atoi(limit) * 10000000) {
                PortLog("hd movie: test stop at %d s", SDL_atoi(limit));
                skipEnd = clock + 3000000;
            }
        }
        /* show the latest frame due by now; frames come in order, one frameTime apart */
        while (!videoDone) {
            if (!havePending) {
                if (fread(frame, 1, frameBytes, video) != frameBytes) {
                    videoDone = 1;
                    break;
                }
                havePending = 1;
                nextVideo += frameTime;
            }
            if (nextVideo > clock + 100000)
                break;
            if (audio && !audioDone && nextVideo + frameTime <= clock)
                dropped++;
            else
                SDL_UpdateTexture(texture, NULL, frame, width * 4);
            havePending = 0;
            shown++;
            if (!audio || audioDone)
                break;
        }
        {
            int ow = 0, oh = 0;
            SDL_FRect dst;
            float scale;
            SDL_GetCurrentRenderOutputSize(renderer, &ow, &oh);
            scale = SDL_min((float)ow / width, (float)oh / height);
            dst.w = width * scale;
            dst.h = height * scale;
            dst.x = (ow - dst.w) / 2;
            dst.y = (oh - dst.h) / 2;
            SDL_SetRenderViewport(renderer, NULL);
            SDL_SetRenderClipRect(renderer, NULL);
            SDL_SetRenderScale(renderer, 1.0f, 1.0f);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, &dst);
            DrawSubs(&subs, renderer, clock, &dst, oh);
            {
                const LONGLONG fade = 3000000;
                LONGLONG end = skipEnd >= 0 ? skipEnd : duration;
                LONGLONG edge = clock < fade ? clock : (end > fade && clock > end - fade ? end - clock : fade);
                if (audio)
                    SDL_SetAudioStreamGain(audio, edge <= 0 ? 0.0f : (float)edge / fade);
                if (edge < fade) {
                    Uint8 a = (Uint8)(255 - (edge < 0 ? 0 : edge) * 255 / fade);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, a);
                    SDL_RenderFillRect(renderer, NULL);
                }
            }
            {
                static int shot;
                const char* at = SDL_getenv("KHCOM_HDMOVIE_SHOT");
                if (at && !shot && clock >= (LONGLONG)SDL_atoi(at) * 10000000) {
                    SDL_Surface* s = SDL_RenderReadPixels(renderer, NULL);
                    if (s) {
                        SDL_SaveBMP(s, PortConfigPath("hdmovie.bmp"));
                        SDL_DestroySurface(s);
                    }
                    shot = 1;
                }
            }
            if (pause) {
                SDL_Surface* frozen = SDL_RenderReadPixels(renderer, NULL);
                int skip = MoviePause(renderer, frozen, audio, &quit);
                SDL_DestroySurface(frozen);
                if (quit || sMovieDeviceReset)
                    break;
                if (skip && skipEnd < 0)
                    skipEnd = clock + 3000000;
                continue;
            }
            if (shown >= 10) {
                extern void PortSceneGalleryMovieCapture(SDL_Renderer* renderer);
                PortSceneGalleryMovieCapture(renderer);
            }
            SDL_RenderPresent(renderer);
        }
        {
            Uint64 spent = SDL_GetTicksNS() - frameStart;
            if (spent < 16000000)
                SDL_DelayNS(16000000 - spent);
        }
        if (audioDone && audio && SDL_GetAudioStreamQueued(audio) > 0 && videoDone)
            while (SDL_GetAudioStreamQueued(audio) > 0 && !quit && !sMovieDeviceReset && !PauseWanted(&quit))
                SDL_Delay(10);
    }

done:
    PortLog("hd movie: %d frames decoded, %d late ones not shown%s", shown, dropped, quit ? ", window closed" : "");
    if (played) {
        FreeSubs(&subs);
        PortAudioMovieHold(0);
    }
    /* closing our ends first: an ffmpeg still writing gets a broken pipe and exits, so pclose does not wait */
    if (sound)
        pclose(sound);
    if (video)
        pclose(video);
    SDL_free(frame);
    if (texture)
        SDL_DestroyTexture(texture);
    if (audio)
        SDL_DestroyAudioStream(audio);
    if (quit) {
        SDL_Event q;
        SDL_zero(q);
        q.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&q);
    }
    return played && !sMovieDeviceReset;
}
#endif
#elif defined(__ANDROID__) || defined(PORT_ANDROID64) || defined(PORT_LINUX64_GUEST) /* the image asks its host */
#include <SDL3/SDL.h>
#if defined(PORT_ANDROID64) || defined(PORT_LINUX64_GUEST)
int h_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* font);
#else
#include <stdarg.h>
SDL_Renderer* PortRenderer(void);
int HdMovieAndroidPlay(SDL_Renderer* renderer, const char* path, const char* subs1, const char* subs2, const char* font);
void HdMovieLog(const char* fmt, ...) {
    char text[1024];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    PortLog("%s", text);
}
#endif

/* Plays hd/movies/<name>.mp4 (a scene: hd/scenes, MoviePath) to the end (or until skipped). 0: nothing imported or it could not be opened. */
static int PlayFile(const char* name) {
    char path[512], subs[2][512], font[512];
    const char* order[2];
    int i, played;
    MoviePath(name, path, sizeof(path));
    SubsOrder(order);
    for (i = 0; i < 2; i++)
        SDL_snprintf(subs[i], sizeof(subs[i]), "%s%s.%s.txt", PortConfigPath("hd/subs/"), name, order[i]);
    SDL_strlcpy(font, PortConfigPath("assets/fonts/KHMenu.otf"), sizeof(font));
#if defined(PORT_ANDROID64) || defined(PORT_LINUX64_GUEST)
#ifdef PORT_LINUX64_GUEST
    if (sStartAt > 0) /* the 64-bit Linux host reads where to start after an @ (port/linux64) */
        SDL_snprintf(path + strlen(path), sizeof(path) - strlen(path), "@%d", sStartAt);
#endif
    played = h_hd_movie(path, gPortConfig.hdSubtitles ? subs[0] : NULL, gPortConfig.hdSubtitles ? subs[1] : NULL, font);
#else
    if (!SDL_GetPathInfo(path, NULL))
        return 0;
    PortAudioMovieHold(1);
    played = HdMovieAndroidPlay(PortRenderer(), path, gPortConfig.hdSubtitles ? subs[0] : NULL,
                                gPortConfig.hdSubtitles ? subs[1] : NULL, font);
    PortAudioMovieHold(0);
#endif
    if (played == 2) { /* the app was closed during the movie: the main loop closes the game as usual */
#if defined(PORT_ANDROID64) || defined(PORT_LINUX64_GUEST)
        /* the host's quit_requested says so */
#else
        SDL_Event q;
        SDL_zero(q);
        q.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&q);
#endif
    }
    return played != 0;
}
#else
int PortHdSceneCount(void) {
    return 0;
}
int PortHdSceneAt(int i, const char** movie) {
    (void)i;
    (void)movie;
    return -1;
}
int PortHdSceneImported(const char* movie) {
    (void)movie;
    return 0;
}
int PortHdMoviePlayName(const char* movie) {
    (void)movie;
    return 0;
}
const char* PortHdGalleryMovieName(int sceneId) { (void)sceneId; return NULL; }
int PortHdGalleryMovieOffset(int sceneId) { (void)sceneId; return 0; }
int PortHdGalleryPlayScene(int sceneId, int hybrid) { (void)sceneId; (void)hybrid; return 0; }
int PortHdMoviePlay(int movieId) {
    (void)movieId;
    return 0;
}
int PortHdScenePlay(int eventId) {
    (void)eventId;
    return 0;
}
#endif
