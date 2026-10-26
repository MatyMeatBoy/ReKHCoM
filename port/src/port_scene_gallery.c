/* The scene gallery (Options > Scenes, user 25/10): the story scenes already seen, to watch again in their cartridge
 * version or their Re:CoM HD one. What was seen is the whole ReKHCoM install's (any save, any bank): the furthest any
 * game got. scenes_seen.dat holds one bit per event id. Cheats > Unlock all scenes fills it. */
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>

#include "port/port.h"
#include "game_state.h"
#include "map_api.h"
#include "port/port_state.h"
#include "evt/event_ids.h"
#include "evt/evt_types.h"
#include "port/port_scene_gallery.h"
#include "port/port_hd_poster.h"

extern void PortReplayVisitState(PortStateVisitor visit);
extern int PortSceneGalleryBegin(int eventId);
extern void PortUiGalleryResume(void);

enum { GALLERY_IDLE, GALLERY_SAVE, GALLERY_PLAY, GALLERY_RETURN };
static int sReplay, sReplayEvent, sReplayFrames;
static int sReplayEntered;
static unsigned sReplayStartFrame;
static int sReplayFallback;
static unsigned char *sReplayMemory, *sReplayCursor;
static size_t sReplayBytes;
static char sReplayMode[64];

typedef struct GalleryAudit { uint32_t game, map, config, files; int recom, rank; } GalleryAudit;
static GalleryAudit sAudit;
static uint32_t AuditHash(const void* data, size_t size) {
    const unsigned char* p = data;
    uint32_t hash = 2166136261u;
    while (size--) hash = (hash ^ *p++) * 16777619u;
    return hash;
}
static GalleryAudit AuditStamp(void) {
    static const char* const names[] = { "scenes_seen.dat", "kh_com_recom.ini", "rebel_mode.txt",
        "achievements_official_eu.dat", "achievements_official_us.dat", "achievements_official_jp.dat",
        "achievements_rebel_eu.dat", "achievements_recom_eu.dat", "achievements_recom_extra.dat",
        "achievements_rebel_recom_eu.dat", "achievements_rebel_recom_extra.dat" };
    GalleryAudit stamp = {0};
    unsigned i;
    stamp.game = AuditHash(&gGameState, sizeof(gGameState));
    stamp.map = AuditHash(&gMapFloorState, sizeof(gMapFloorState));
    stamp.config = AuditHash(&gPortConfig, sizeof(gPortConfig));
    stamp.recom = PortRecomActive(); stamp.rank = PortRecomRunRank();
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        FILE* f = fopen(PortConfigPath(names[i]), "rb");
        int c;
        stamp.files = (stamp.files ^ i) * 16777619u;
        if (f) { while ((c = fgetc(f)) != EOF) stamp.files = (stamp.files ^ c) * 16777619u; fclose(f); }
    }
    return stamp;
}
void PortSceneGalleryAuditBegin(void) { if (SDL_getenv("KHCOM_GALLERY_AUDIT")) sAudit = AuditStamp(); }
void PortSceneGalleryAuditEnd(void) {
    if (SDL_getenv("KHCOM_GALLERY_AUDIT")) {
        GalleryAudit now = AuditStamp();
        PortLog("scene gallery audit: game=%d map=%d config=%d files=%d recom=%d rank=%d",
            sAudit.game == now.game, sAudit.map == now.map, sAudit.config == now.config, sAudit.files == now.files,
            sAudit.recom == now.recom, sAudit.rank == now.rank);
    }
}

/* One selected CPU poster and one GPU texture. A single cold HD job owns only
 * its temporary decode buffer; it never touches renderer/config/global path rings. */
#define PREVIEW_W 640
#define PREVIEW_H 360
static uint32_t sPreview[PREVIEW_W * PREVIEW_H];
static int sPreviewEvent = -1, sPreviewHd = -1, sPreviewReady, sPreviewSerial, sPreviewW, sPreviewH, sPreviewAttempted;
static Uint64 sPreviewWaitAt;
static int sHdReplayEvent = -1;
static SDL_Texture* sPreviewTexture;
static SDL_Renderer* sPreviewRenderer;
static int sTextureSerial = -1, sTextureW, sTextureH;
#if defined(_WIN32) || defined(PORT_LINUX)
typedef struct PreviewJob {
    SDL_Thread* thread;
    SDL_AtomicInt done;
    uint32_t* pixels;
    int event, serial, second, ok;
    char path[1024], cache[1200];
} PreviewJob;
static PreviewJob sPreviewJob;
static int PreviewDecodeJob(void* data) {
    PreviewJob* job = data;
    job->ok = PortHdScenePreviewFile(job->path, job->pixels, PREVIEW_W, PREVIEW_H, job->second);
    if (job->ok) {
        SDL_Surface* surface = SDL_CreateSurfaceFrom(PREVIEW_W, PREVIEW_H, SDL_PIXELFORMAT_ARGB8888, job->pixels, PREVIEW_W * 4);
        if (surface) SDL_SavePNG(surface, job->cache);
        SDL_DestroySurface(surface);
    }
    SDL_SetAtomicInt(&job->done, 1);
    return 0;
}
#endif
void PortSceneGalleryHdBegin(int eventId) { sHdReplayEvent = eventId; }
void PortSceneGalleryHdEnd(void) { sHdReplayEvent = -1; }
void PortSceneGalleryPreviewPoll(void) {
#if defined(_WIN32) || defined(PORT_LINUX)
    PreviewJob* job = &sPreviewJob;
    if (job->thread && SDL_GetAtomicInt(&job->done)) {
        SDL_WaitThread(job->thread, NULL);
        job->thread = NULL;
        if (job->ok && job->serial == sPreviewSerial && job->event == sPreviewEvent && sPreviewHd) {
            SDL_memcpy(sPreview, job->pixels, sizeof(sPreview));
            sPreviewW = PREVIEW_W; sPreviewH = PREVIEW_H; sPreviewReady = 1;
            sTextureSerial = -1;
            PortLog("scene gallery preview: HD event %d from bounded async poster", job->event);
        }
        SDL_free(job->pixels); job->pixels = NULL;
    }
#endif
}
void PortSceneGalleryPreviewCancel(void) {
    ++sPreviewSerial;
    sPreviewEvent = sPreviewHd = -1; sPreviewReady = 0;
    SDL_DestroyTexture(sPreviewTexture); sPreviewTexture = NULL; sPreviewRenderer = NULL;
    sTextureSerial = -1;
    PortSceneGalleryPreviewPoll(); /* never wait for a cold decoder when closing a menu */
}
void PortSceneGalleryPreviewSize(int* width, int* height) { *width = sPreviewW; *height = sPreviewH; }
const uint32_t* PortSceneGalleryPreview(int eventId, int hd) {
    extern int PortScenePosterPreview(int, uint32_t*, int*, int*);
    extern int PortScenePosterDecode(const void*, size_t, uint32_t*, int*, int*);
    PortSceneGalleryPreviewPoll();
    if (eventId != sPreviewEvent || hd != sPreviewHd) {
        const char* movie;
        char path[1024], sidecar[1200];
        void* data = NULL;
        size_t size = 0;
        ++sPreviewSerial; sPreviewEvent = eventId; sPreviewHd = hd;
        sPreviewReady = 0; sPreviewAttempted = 0; sPreviewWaitAt = SDL_GetTicks(); sTextureSerial = -1;
        if (!hd) {
            sPreviewReady = PortScenePosterPreview(eventId, sPreview, &sPreviewW, &sPreviewH);
            if (sPreviewReady) PortLog("scene gallery preview: GBA event %d from clean engine poster", eventId);
            else {
                extern int PortSceneBackgroundPreview(int, uint32_t*, int, int);
                sPreviewW = 360; sPreviewH = 240;
                sPreviewReady = PortSceneBackgroundPreview(eventId, sPreview, sPreviewW, sPreviewH);
            }
        } else if ((movie = PortHdGalleryMovieName(eventId)) != NULL) {
#if defined(_WIN32) || defined(PORT_LINUX)
            if (!PortHdScenePreviewPath(movie, path, sizeof(path))) path[0] = 0;
#else
            FILE* f;
            SDL_snprintf(path, sizeof(path), "%s%s.mp4", PortConfigPath("hd/scenes/"), movie);
            f = fopen(path, "rb");
            if (f) fclose(f);
            else SDL_snprintf(path, sizeof(path), "%s%s.mp4", PortConfigPath("hd/movies/"), movie);
#endif
            if (PortHdPosterPath(path, PortHdGalleryMovieOffset(eventId) + 1, sidecar, sizeof(sidecar))) {
#if defined(_WIN32) || defined(PORT_LINUX)
                SDL_PathInfo mediaInfo, imageInfo;
                if (SDL_GetPathInfo(path, &mediaInfo) && SDL_GetPathInfo(sidecar, &imageInfo) &&
                    imageInfo.modify_time >= mediaInfo.modify_time && imageInfo.size <= 2u * 1024u * 1024u)
#endif
                    data = SDL_LoadFile(sidecar, &size);
                sPreviewReady = PortScenePosterDecode(data, size, sPreview, &sPreviewW, &sPreviewH);
                SDL_free(data);
                if (sPreviewReady && (sPreviewW != PREVIEW_W || sPreviewH != PREVIEW_H)) sPreviewReady = 0;
            }
        }
    }
#if defined(_WIN32) || defined(PORT_LINUX)
    if (!sPreviewReady && hd && !sPreviewAttempted && !sPreviewJob.thread && SDL_GetTicks() - sPreviewWaitAt >= 75) {
        const char* movie = PortHdGalleryMovieName(eventId);
        PreviewJob* job = &sPreviewJob;
        if (movie && PortHdScenePreviewPath(movie, job->path, sizeof(job->path))) {
            job->event = eventId; job->serial = sPreviewSerial;
            job->second = PortHdGalleryMovieOffset(eventId) + 1;
            job->ok = 0; SDL_SetAtomicInt(&job->done, 0);
            PortHdPosterPath(job->path, job->second, job->cache, sizeof(job->cache));
            job->pixels = SDL_malloc(sizeof(sPreview));
            if (job->pixels) {
                job->thread = SDL_CreateThread(PreviewDecodeJob, "selected HD poster", job);
                if (!job->thread) { SDL_free(job->pixels); job->pixels = NULL; }
            }
        }
        sPreviewAttempted = 1; /* one attempt per selection, including corrupt media */
    }
#endif
    return sPreviewReady ? sPreview : NULL;
}
/* Output-resolution poster rendering avoids shrinking a 360p still to the menu's
 * ~90px logical card and enlarging those few pixels again on an HD display. */
int PortSceneGalleryPreviewDraw(SDL_Renderer* renderer, float x, float y, float w, float h, uint32_t face, uint32_t shadow) {
    SDL_FRect dest = {x,y,w,h};
    int alpha = 255 - SDL_clamp((int)(face & 255) - (int)(shadow & 255), 0, 255);
    if (!sPreviewReady || !renderer || w <= 0 || h <= 0) return 0;
    if (sPreviewRenderer != renderer || sTextureW != sPreviewW || sTextureH != sPreviewH) {
        SDL_DestroyTexture(sPreviewTexture); sPreviewTexture = NULL;
    }
    if (!sPreviewTexture) {
        sPreviewTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, sPreviewW, sPreviewH);
        if (!sPreviewTexture) return 0;
        sPreviewRenderer = renderer; sTextureW = sPreviewW; sTextureH = sPreviewH; sTextureSerial = -1;
        SDL_SetTextureBlendMode(sPreviewTexture, SDL_BLENDMODE_NONE);
        SDL_SetTextureScaleMode(sPreviewTexture, sPreviewHd ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
    }
    if (sTextureSerial != sPreviewSerial) {
        SDL_UpdateTexture(sPreviewTexture, NULL, sPreview, sPreviewW * 4);
        sTextureSerial = sPreviewSerial;
    }
    SDL_RenderTexture(renderer, sPreviewTexture, NULL, &dest);
    if (alpha > 0) {
        int r = SDL_min(255, (int)((shadow >> 16) & 255) * 255 / alpha);
        int g = SDL_min(255, (int)((shadow >> 8) & 255) * 255 / alpha);
        int b = SDL_min(255, (int)(shadow & 255) * 255 / alpha);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, r,g,b,alpha);
        SDL_RenderFillRect(renderer, &dest);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    return 1;
}
/* Offline only: the preparation tool supplies a private --data-dir. Never launched by the gallery. */
#if defined(_WIN32) || defined(PORT_LINUX)
static int sPosterEvent = -1, sPosterDone, sPosterStable, sPosterForce, sPosterFrame = 90;
static int sPosterBestScore = -1;
static uint32_t sPosterPixels[360 * 240];
static char sPosterOutput[1024];
int PortScenePosterRendering(void) { return sPosterEvent >= 0; }
int PortScenePosterFinished(void) { return sPosterDone; }
int PortScenePosterConfigure(int argc, char** argv) {
    int i, eventId = -1, isolated = 0;
    const char* output = NULL;
    char play[64];
    SDL_Environment* env;
    for (i = 1; i < argc; i++) {
        if (!SDL_strcmp(argv[i], "--scene-poster") && i + 1 < argc) eventId = SDL_atoi(argv[++i]);
        else if (!SDL_strcmp(argv[i], "--poster-frame") && i + 1 < argc) { int frame = SDL_atoi(argv[++i]); sPosterFrame = SDL_clamp(frame, 45, 1000); }
        else if (!SDL_strcmp(argv[i], "--poster-output") && i + 1 < argc) output = argv[++i];
        else if (!SDL_strcmp(argv[i], "--data-dir") && i + 1 < argc) { isolated = 1; i++; }
    }
    if (eventId < 0) return 0;
    if (!isolated || !output || (eventId >= EVENT_COUNT && !PORT_SCENE_IS_MOVIE(eventId))) return -1;
    sPosterEvent = eventId;
    SDL_strlcpy(sPosterOutput, output, sizeof(sPosterOutput));
    env = SDL_GetEnvironment();
    SDL_snprintf(play, sizeof(play), "20:%d", eventId);
    SDL_SetEnvironmentVariable(env, "KHCOM_GALLERY_PLAY", play, true);
    SDL_SetEnvironmentVariable(env, "KHCOM_MODE", "1:mode_title", true);
    SDL_SetEnvironmentVariable(env, "KHCOM_CONFIG", "graphics_mode=3;graphics_mode_applied=3;view_mode=0;zoom=8;hd_title=0;hd_text=0;hd_hud=0;hd_scenes=0;hd_movies=0;hd_music=0;frame_skip=0;vsync=0;game_language=1", true);
    SDL_SetEnvironmentVariable(env, "KHCOM_WINDOW", "360x240", true);
    SDL_SetEnvironmentVariable(env, "KHCOM_FAST", "1", true);
    SDL_SetEnvironmentVariable(env, "KHCOM_BACKGROUND_TEST", "1", true);
    SDL_SetEnvironmentVariable(env, "KHCOM_PPU_THREADS", "1", true);
    SDL_SetEnvironmentVariable(env, "KHCOM_HIDDEN", "1", true);
    SDL_SetEnvironmentVariable(env, "SDL_VIDEO_DRIVER", "dummy", true);
    SDL_SetEnvironmentVariable(env, "SDL_AUDIO_DRIVER", "dummy", true);
    return 1;
}
void PortSceneGalleryCapture(void) {
    extern EventState* gEventState;
    unsigned age;
    int actors = 0, i, dialogue, score, light = 0, x, y;
    if (sPosterEvent < 0 || sPosterDone || sReplay != GALLERY_PLAY) return;
    age = PortSceneGalleryAge();
    dialogue = !PORT_SCENE_IS_MOVIE(sPosterEvent) && gEventState && gEventState->msgWinOpen && !gEventState->fadedOut;
    if (gEventState && !PORT_SCENE_IS_MOVIE(sPosterEvent))
        for (i = 0; i < 16; i++) actors += gEventState->charaObjs[i] != NULL;
    sPosterStable = dialogue ? sPosterStable + 1 : 0;
    for (i = 0; i < gPortScreen.width * gPortScreen.height; i += 97)
        light += (gPortFrame[i] & 0xFFFFFFu) != 0;
    score = light > 8 ? actors * 100 + (dialogue ? 500 : 0) + SDL_min((int)age, 100) : 0;
    if (score > sPosterBestScore) {
        for (y = 0; y < 240; y++) for (x = 0; x < 360; x++)
            sPosterPixels[y * 360 + x] = gPortFrame[(y * gPortScreen.height / 240) * gPortScreen.width + x * gPortScreen.width / 360] | 0xFF000000u;
        sPosterBestScore = score;
    }
    if ((actors && sPosterStable >= 8 && age >= 45) ||
        (PORT_SCENE_IS_MOVIE(sPosterEvent) && age >= (unsigned)sPosterFrame) || age >= 1100 || sPosterForce) {
        SDL_Surface* surface;
        if (PORT_SCENE_IS_MOVIE(sPosterEvent))
            for (y = 0; y < 240; y++) for (x = 0; x < 360; x++)
                sPosterPixels[y * 360 + x] = gPortFrame[(y * gPortScreen.height / 240) * gPortScreen.width + x * gPortScreen.width / 360] | 0xFF000000u;
        surface = SDL_CreateSurfaceFrom(360, 240, SDL_PIXELFORMAT_XRGB8888, sPosterPixels, 360 * 4);
        if (surface && SDL_SavePNG(surface, sPosterOutput)) {
            PortLog("scene poster: saved event=%d actors=%d dialogue=%d age=%u clean-ui=1 %s", sPosterEvent, actors, dialogue, age, sPosterOutput);
            sPosterDone = 1;
        }
        SDL_DestroySurface(surface);
    }
}
void PortScenePosterForceCapture(void) {
    if (sPosterEvent >= 0 && !sPosterDone && sReplay == GALLERY_PLAY) { sPosterForce = 1; PortSceneGalleryCapture(); }
}
#else
int PortScenePosterRendering(void) { return 0; }
int PortScenePosterFinished(void) { return 0; }
int PortScenePosterConfigure(int argc, char** argv) { (void)argc; (void)argv; return 0; }
void PortScenePosterForceCapture(void) {}
void PortSceneGalleryCapture(void) {}
#endif
/* The video's decoded poster has no subtitle/pause overlay. Do not replace it with a presented frame. */
void PortSceneGalleryMovieCapture(SDL_Renderer* renderer) { (void)renderer; }

int PortSceneGalleryActive(void) { return sReplay != GALLERY_IDLE || sHdReplayEvent >= 0; }
int PortSceneGalleryAvailable(void) {
    char *data, *bss;
    size_t dataSize, bssSize;
    extern int PortDebugModeAvailable(void);
    extern int NetRollbackSession(void);
    PortGameSections(&data, &dataSize, &bss, &bssSize);
    return !NetRollbackSession() && ((dataSize && bssSize) || PortDebugModeAvailable());
}
unsigned PortSceneGalleryAge(void) { return PortFrameCount() - sReplayStartFrame; }
static void ReplaySize(void* p, size_t n) { (void)p; sReplayBytes += n; }
static void ReplaySave(void* p, size_t n) { memcpy(sReplayCursor, p, n); sReplayCursor += n; }
static void ReplayLoad(void* p, size_t n) { memcpy(p, sReplayCursor, n); sReplayCursor += n; }
static void ReplayVisit(PortStateVisitor visit) {
    PortReplayVisitState(visit);
    visit((void*)(uintptr_t)GBA_SRAM, GBA_SRAM_SIZE); /* a replay cannot change the player's cartridge save */
}

int PortSceneGalleryPlay(int eventId) {
    char *data, *bss;
    size_t dataSize, bssSize;
    extern int NetRollbackSession(void);
    if (sReplay || NetRollbackSession() || eventId < 0 || (eventId >= EVENT_COUNT && !PORT_SCENE_IS_MOVIE(eventId)))
        return 0;
    PortGameSections(&data, &dataSize, &bss, &bssSize);
    if (!dataSize || !bssSize) {
        extern int PortDebugModeAvailable(void);
        if (!PortDebugModeAvailable()) return 0; /* no running-game snapshot on this backend */
        PortSramSyncIfDirty();
        sReplayEvent = eventId;
        sReplayFallback = 1;
        sReplay = GALLERY_PLAY;
        sReplayFrames = 0;
        sReplayEntered = 0;
        sReplayStartFrame = PortFrameCount();
        PortSceneGalleryBegin(eventId);
        return 1;
    }
    PortSramSyncIfDirty(); /* commit any already completed user save before temporarily blocking replay writes */
    sReplayFallback = 0;
    sReplayEvent = eventId;
    sReplay = GALLERY_SAVE;
    return 1;
}

void PortSceneGalleryReturn(void) {
    PortScenePosterForceCapture();
    if (sReplay == GALLERY_PLAY)
        sReplay = GALLERY_RETURN;
}

void PortSceneGalleryShutdown(void) {
    PortSceneGalleryPreviewCancel();
    SDL_free(sReplayMemory);
    sReplayMemory = NULL;
    sReplay = GALLERY_IDLE;
    sHdReplayEvent = -1;
    sReplayFallback = 0;
}

void PortSceneGalleryFinalShutdown(void) {
    PortSceneGalleryShutdown();
#if defined(_WIN32) || defined(PORT_LINUX)
    if (sPreviewJob.thread) {
        SDL_WaitThread(sPreviewJob.thread, NULL);
        sPreviewJob.thread = NULL;
        SDL_free(sPreviewJob.pixels); sPreviewJob.pixels = NULL;
    }
#endif
}

/* Same native call stack as quick states/rollback; never restore from an event task's own wait. */
void PortSceneGalleryFrame(void) {
    PortSceneGalleryPreviewPoll();
    extern const char* PortHarnessModeName(void);
    if (sReplayFallback) {
        if (sReplay == GALLERY_RETURN) {
            extern void PortSceneGalleryGoTitle(void);
            PortSceneGalleryGoTitle();
            sReplay = GALLERY_SAVE;
        } else if (sReplay == GALLERY_SAVE && !strcmp(PortHarnessModeName(), "mode_title")) {
            PortSceneGalleryShutdown();
            PortUiGalleryResume();
        }
        return;
    }
    if (!gPortInMainLoop)
        return;
    if (sReplay == GALLERY_SAVE) {
        sReplayBytes = 0;
        ReplayVisit(ReplaySize);
        if (sReplayBytes > 64u * 1024u * 1024u || !(sReplayMemory = SDL_malloc(sReplayBytes))) {
            PortLog("scene gallery: checkpoint unavailable; playback cancelled");
            sReplay = GALLERY_IDLE;
            PortUiGalleryResume();
            return;
        }
        sReplayCursor = sReplayMemory;
        ReplayVisit(ReplaySave);
        PortSceneGalleryAuditBegin();
        SDL_strlcpy(sReplayMode, PortHarnessModeName(), sizeof(sReplayMode));
        sReplayFrames = 0;
        sReplayEntered = 0;
        sReplayStartFrame = PortFrameCount();
        sReplay = GALLERY_PLAY;
        PortLog("scene gallery: kept %u bytes from %s; playing %d", (unsigned)sReplayBytes, sReplayMode, sReplayEvent);
        PortSceneGalleryBegin(sReplayEvent);
    } else if (sReplay == GALLERY_RETURN) {
        sReplayCursor = sReplayMemory;
        ReplayVisit(ReplayLoad);
        PortSceneGalleryAuditEnd();
        PortAudioReset();
        PortSceneGalleryShutdown();
        PortUiGalleryResume();
        PortLog("scene gallery: restored original state; returning to the list (%s)", PortHarnessModeName());
    } else if (sReplay == GALLERY_PLAY) {
        /* A scene which never entered its mode returns safely rather than stranding the player. */
        ++sReplayFrames;
        const char* expected = PORT_SCENE_IS_MOVIE(sReplayEvent) ? "mode_movie" : "Event";
        if (!strcmp(PortHarnessModeName(), expected)) sReplayEntered = 1;
        if ((sReplayEntered || sReplayFrames > 600) && strcmp(PortHarnessModeName(), expected))
            sReplay = GALLERY_RETURN;
    }
}

#define SCENE_BITS 1024

static unsigned char sSeen[SCENE_BITS / 8];
static int sLoaded;

static void Save(void);

/* The scenes a game already went through, from its progress: the gallery's scenes are the story's (the floors' halls),
   each floor's run of event ids in order (event_ids.h): Sora's 13 floors, Riku's 12 (B12F first). Saves from before
   the gallery (scenes_seen.dat only counts scenes played since) get theirs back this way, with nothing past where
   they are. floor: the floor the game is on; flags: that floor's (GameFloor), -1 unknown (a file's summary). */
static const unsigned char kSoraFloors[14] = { 0, 12, 15, 18, 21, 24, 29, 32, 36, 39, 43, 52, 61, 74 };
static const unsigned char kRikuFloors[13] = { EVENT_149_RIKU_B12F_OPNING, EVENT_158_RIKU_B11F_ENTRANCE,
    EVENT_160_RIKU_B10F_GOAL, EVENT_162_RIKU_B9F_DEMO, EVENT_163_RIKU_B8F_GOAL, EVENT_166_RIKU_B7F_ENTRANCE,
    EVENT_168_RIKU_B6F_DEMO, EVENT_169_RIKU_B5F_DEMO, EVENT_170_RIKU_B4F_ENTRANCE, EVENT_176_RIKU_B3F_ENTRANCE,
    EVENT_185_RIKU_B2F_ENTRANCE, EVENT_191_RIKU_B1F_ENTRANCE, EVENT_COUNT };

static int Mark(int from, int to) {
    int e, added = 0;
    for (e = from; e < to && e < SCENE_BITS; e++)
        if (!(sSeen[e >> 3] & (1 << (e & 7)))) {
            sSeen[e >> 3] |= (unsigned char)(1 << (e & 7));
            added = 1;
        }
    return added;
}

static int FromProgress(int riku, int cleared, int floor, int flags) {
    const unsigned char* at = riku ? kRikuFloors : kSoraFloors;
    int floors = riku ? 12 : 13, f, added = 0;
    if (!riku && (floor >= 0 || cleared)) added |= Mark(PORT_SCENE_MOVIE(1), PORT_SCENE_MOVIE(1) + 1);
    if (!riku && (floor >= 6 || cleared)) added |= Mark(PORT_SCENE_MOVIE(2), PORT_SCENE_MOVIE(2) + 1);
    if (!riku && (floor >= 12 || cleared)) added |= Mark(PORT_SCENE_MOVIE(3), PORT_SCENE_MOVIE(3) + 1);
    if (cleared) added |= Mark(PORT_SCENE_MOVIE(riku ? 5 : 4), PORT_SCENE_MOVIE(riku ? 5 : 4) + 1);
    if (cleared)
        return added | Mark(at[0], at[floors]);
    if (floor < 0)
        return added;
    for (f = 0; f < floor && f < floors; f++)
        added |= Mark(at[f], at[f + 1]);
    if (floor < floors && flags >= 0) {
        if (flags & (FLOOR_FLAG_CLEARED | FLOOR_FLAG_EXIT_EVENT_DONE))
            added |= Mark(at[floor], at[floor + 1]);
        else if (flags & FLOOR_FLAG_ENTRY_EVENT_DONE)
            added |= Mark(at[floor], at[floor] + 1); /* the floor's entrance */
    }
    return added;
}

static void Load(void) {
    FILE* f;
    if (sLoaded)
        return;
    sLoaded = 1;
    f = fopen(PortConfigPath("scenes_seen.dat"), "rb");
    if (f) {
        if (fread(sSeen, 1, sizeof(sSeen), f) != sizeof(sSeen))
            memset(sSeen, 0, sizeof(sSeen)); /* an older or broken file: start again */
        fclose(f);
    }
}

/* the saves' own progress, as the title read their summaries (they change as the player saves: looked at whenever the
   gallery asks): files 1-2 Sora's, 3-4 Riku's; a cleared story, all of it */
static void FromSaves(void) {
    int i, added = 0;
    added |= FromProgress(0, (gGameState.flags & GAME_FLAG_SORA_CLEAR) != 0, -1, -1);
    added |= FromProgress(1, (gGameState.flags & GAME_FLAG_RIKU_CLEAR) != 0, -1, -1);
    for (i = 0; i < 4; i++)
        if (gGameState.fileSummaries[i].playTime)
            added |= FromProgress(i >= 2, 0, gGameState.fileSummaries[i].floor, -1);
    if (added) {
        PortLog("scene gallery: scenes seen added from the saves' progress");
        Save();
    }
}

/* save.c: a game was loaded: its floors' own marks too (the current floor's entrance, its end) */
void PortScenesFromGame(void) {
    int floor = gGameState.floor;
    if (PortSceneGalleryActive()) return;
    Load();
    /* the floor being played keeps its marks in the map's state (saved with it); floors[] has them once left */
    if (FromProgress((gGameState.flags & GAME_FLAG_RIKU) != 0, 0, floor,
                     floor >= 0 && floor < 13 ? gMapFloorState.flags | gGameState.floors[floor].flags : -1))
        Save();
}

static void Save(void) {
    FILE* f = fopen(PortConfigPath("scenes_seen.dat"), "wb");
    if (!f)
        return;
    fwrite(sSeen, 1, sizeof(sSeen), f);
    fclose(f);
}

void PortSceneSeen(int eventId) {
    if (PortSceneGalleryActive()) return;
    eventId &= 0x7FFF;
    if (eventId < 0 || eventId >= SCENE_BITS)
        return;
    Load();
    if (sSeen[eventId >> 3] & (1 << (eventId & 7)))
        return;
    sSeen[eventId >> 3] |= (unsigned char)(1 << (eventId & 7));
    Save();
}

int PortSceneIsSeen(int eventId) {
    eventId &= 0x7FFF;
    if (eventId < 0 || eventId >= SCENE_BITS)
        return 0;
    Load();
    FromSaves();
    return (sSeen[eventId >> 3] >> (eventId & 7)) & 1;
}

void PortScenesUnlockAll(void) {
    Load();
    memset(sSeen, 0xFF, sizeof(sSeen));
    Save();
}
