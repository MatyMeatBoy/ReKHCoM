/*
 * HD music (port/progress/HD_MOD.md): the Re:Chain of Memories soundtrack of the player's own KINGDOM HEARTS -HD
 * 1.5+2.5 ReMIX-, imported by the launcher as hd/music/<track>.ogg (+ .loop: start and end sample), plays in place
 * of the cartridge's background music.
 *
 * The game keeps running its own song: this only watches the BGM player (which song, paused, stopped, fading),
 * silences its channels (gPortMix.musicGain) and streams the matching track at 48 kHz on an audio stream of its own,
 * so the track is never resampled down to the GBA's rate. Off with hd_music=0, or when no track was imported.
 *
 * Desktop and 32-bit Android (hosted C library) decode the track here. The bridge images (64-bit Android, Switch,
 * Vita, PS4, PS5) have no C library for it: they hand the track to their host (bridge entry hd_music,
 * port/ps5/shared/hd_music_host.h), which mixes it into the game's sound. The web gets the empty functions below.
 */
#include "port/port.h"
#include "port/port_ui.h"

#if !defined(__EMSCRIPTEN__)
#include "lib/m4a.h"
#include "port/port_audio.h"

extern MusicPlayerInfo gMPlayInfo_BGM;

/* Cartridge song (include/songs.h, 1..43) → Re:CoM track (STREAM/0001/<name>.win32.scd). */
static const char* const kTracks[44] = {
    NULL,
    "21_Alice_Battle", "19_Alice_Field", "Boss1", "Event1_Kinpak", "Event2_", "Title", "12_Event_Yuttari",
    "24_Pinocchio_Field", "27_Pinocchio_B", "22_PeterPan_Field", "23_PeterPan_Battle", "01F_Town_B", "01F_Town_F",
    "16_XIIIBoss1", "17_Halloween_F", "18_Helloween_B", "39_LittleMermaid_B", "15_Event_XIII", "13_Event_Namine",
    "03F_Hercules_B", "03F_Hercul", "38_UnderTheSea", "26_Aladdin_B", "Event4", "42_Hollow_F", "43_Hollow_B",
    "14_Event_Odayaka", "Event_Unrest", "34_Boss_RikuAnsem", "20_Event_13thFloor", "25_Aladdin_F",
    "36_WinnieThePooh", "32_Destiny_F", "44_PooGame1", "35_Boss_NiseRiku", "30_Twilight_F", "37_LastBoss1",
    "28_Forget_F", "29_Forget_B", "33_Destiny_B", "Boss2_World", "40_LastBoss2", "31_Twilight_B",
};

#define HD_MUSIC_LEVEL 0.33f
static int sSong = -1, sMissing = -1; /* sMissing: the song whose track did not open (tried once) */

const char* PortHdMusicTrack(int song) {
    return song > 0 && song < (int)(sizeof(kTracks) / sizeof(kTracks[0])) ? kTracks[song] : NULL;
}

/* The cartridge song the BGM player has, or -1. */
static int CurrentSong(void) {
    const MusicPlayerInfo* bgm = &gMPlayInfo_BGM;
    int i;
    if (bgm->songHeader == NULL)
        return -1;
    for (i = 1; i < (int)(sizeof(kTracks) / sizeof(kTracks[0])); i++)
        if (gSongTable[i].header == bgm->songHeader)
            return i;
    return -1;
}

#if __STDC_HOSTED__
/* ---- hosted: stb_vorbis on an SDL audio stream of its own --------------------------------------------------- */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_INTEGER_CONVERSION
#include "../third_party/stb/stb_vorbis.c"


/* hd/music/<track>.<ext> in the data folder (KHCOM_HD_MUSIC=<folder>: the audio tests' shared copy, test_hd_audio.py) */
static const char* MusicPath(char* out, size_t size, const char* track, const char* ext) {
    const char* dir = SDL_getenv("KHCOM_HD_MUSIC");
    char name[160];
    if (dir) {
        SDL_snprintf(out, size, "%s/%s.%s", dir, track, ext);
        return out;
    }
    SDL_snprintf(name, sizeof(name), "hd/music/%s.%s", track, ext);
    SDL_snprintf(out, size, "%s", PortConfigPath(name));
    return out;
}

static SDL_AudioStream* sStream;
static stb_vorbis* sVorbis;
static int sRate;
static unsigned sLoopStart, sLoopEnd, sPosition;
static unsigned char* sFile;

/* The track file is read on a thread of its own (a 7 MB read on the game's thread was a hitch at doors and battle
   starts): while it loads, the cartridge's song keeps playing. */
static SDL_Thread* sLoader;
static SDL_AtomicInt sLoaderDone;
static int sLoaderSong = -1;
static void* sLoaderData;
static size_t sLoaderSize;
static char sLoaderPath[512];

static int SDLCALL LoaderMain(void* unused) {
    (void)unused;
    sLoaderData = SDL_LoadFile(sLoaderPath, &sLoaderSize);
    SDL_SetAtomicInt(&sLoaderDone, 1);
    return 0;
}

/* The loaded file of song, or NULL with *pending set while it still loads (a load for another song is let finish
   and dropped). */
static void* TakeLoaded(int song, size_t* size, int* pending) {
    *pending = 0;
    if (sLoader != NULL) {
        void* data;
        if (!SDL_GetAtomicInt(&sLoaderDone)) {
            *pending = 1;
            return NULL;
        }
        SDL_WaitThread(sLoader, NULL);
        sLoader = NULL;
        data = sLoaderData;
        sLoaderData = NULL;
        if (sLoaderSong == song) {
            sLoaderSong = -1;
            *size = sLoaderSize;
            return data;
        }
        SDL_free(data);
        sLoaderSong = -1;
    }
    MusicPath(sLoaderPath, sizeof(sLoaderPath), kTracks[song], "ogg");
    SDL_SetAtomicInt(&sLoaderDone, 0);
    sLoaderSong = song;
    sLoader = SDL_CreateThread(LoaderMain, "hd music", NULL);
    if (sLoader == NULL) { /* no thread: read it here */
        sLoaderSong = -1;
        return SDL_LoadFile(sLoaderPath, size);
    }
    *pending = 1;
    return NULL;
}

static void Close(void) {
    if (sVorbis)
        stb_vorbis_close(sVorbis);
    sVorbis = NULL;
    SDL_free(sFile);
    sFile = NULL;
    if (sStream)
        SDL_ClearAudioStream(sStream);
    sSong = -1;
}

/* 1: the track plays, 0: no track, -1: still loading (the cartridge's song plays meanwhile) */
static int Open(int song) {
    char name[160];
    size_t size = 0;
    int error = 0, pending;
    FILE* loop;

    Close();
    SDL_snprintf(name, sizeof(name), "hd/music/%s.ogg", kTracks[song]);
    sFile = TakeLoaded(song, &size, &pending);
    if (pending)
        return -1;
    if (sFile == NULL)
        return 0;
    sVorbis = stb_vorbis_open_memory(sFile, (int)size, &error, NULL);
    if (sVorbis == NULL) {
        PortLog("hd music: %s does not decode (%d)", name, error);
        Close();
        return 0;
    }
    {
        stb_vorbis_info info = stb_vorbis_get_info(sVorbis);
        sRate = (int)info.sample_rate;
    }
    sLoopStart = 0;
    sLoopEnd = stb_vorbis_stream_length_in_samples(sVorbis);
    loop = fopen(MusicPath(name, sizeof(name), kTracks[song], "loop"), "r");
    if (loop != NULL) {
        unsigned a, b;
        if (fscanf(loop, "%u %u", &a, &b) == 2 && a < b && b <= sLoopEnd) {
            sLoopStart = a;
            sLoopEnd = b;
        }
        fclose(loop);
    }
    sPosition = 0;
    if (sStream == NULL) {
        SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, sRate };
        sStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (sStream)
            SDL_ResumeAudioStreamDevice(sStream);
    } else {
        SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, sRate };
        SDL_SetAudioStreamFormat(sStream, &spec, NULL);
    }
    sSong = song;
    PortLog("hd music: song %d -> %s", song, kTracks[song]);
    return sStream != NULL;
}

/* the device waits with what is queued, so the track resumes exactly where it stopped (clearing the stream dropped
   a fifth of a second and the track jumped on: the stutter after menus) */
static int sPaused;
static void Pause(void) {
    if (sStream && !sPaused) {
        SDL_PauseAudioStreamDevice(sStream);
        sPaused = 1;
    }
}
static void Resume(void) {
    if (sStream && sPaused) {
        SDL_ResumeAudioStreamDevice(sStream);
        sPaused = 0;
    }
}

/* The level is the stream's, applied as it plays: baked into the samples a fifth of a second ahead, the game's
   fades came late and in steps, heard as small jumps when menus changed (23/10). sHold: PortHdMusicGain's. */
static float sHold = 1.0f, sLevel = 1.0f;

/* Decodes until about a fifth of a second is queued, looping at the track's loop points. */
static void Feed(float gain) {
    float pcm[2048 * 2];
    sLevel = gain;
    SDL_SetAudioStreamGain(sStream, sLevel * sHold);
    gain = 1.0f;
    Resume();
    while (SDL_GetAudioStreamQueued(sStream) < (int)(sRate / 5 * 2 * sizeof(float))) {
        int want = 2048, got, i;
        if (sPosition + (unsigned)want > sLoopEnd)
            want = (int)(sLoopEnd - sPosition);
        if (want <= 0) {
            stb_vorbis_seek(sVorbis, sLoopStart);
            sPosition = sLoopStart;
            continue;
        }
        got = stb_vorbis_get_samples_float_interleaved(sVorbis, 2, pcm, want * 2);
        if (got <= 0) {
            stb_vorbis_seek(sVorbis, sLoopStart);
            sPosition = sLoopStart;
            continue;
        }
        for (i = 0; i < got * 2; i++)
            pcm[i] *= gain;
        SDL_PutAudioStreamData(sStream, pcm, got * 2 * (int)sizeof(float));
        sPosition += (unsigned)got;
    }
}

static int QueuedMs(void) {
    return sStream && sRate ? (int)((long long)SDL_GetAudioStreamQueued(sStream) / 8 * 1000 / sRate) : -1;
}
static long Position(void) {
    return sVorbis ? (long)sPosition : -1;
}

static void Shutdown(void) {
    Close();
    if (sLoader != NULL) {
        SDL_WaitThread(sLoader, NULL);
        sLoader = NULL;
        SDL_free(sLoaderData);
        sLoaderData = NULL;
    }
    if (sStream)
        SDL_DestroyAudioStream(sStream);
    sStream = NULL;
}
#else
/* ---- bridge images: the host decodes and mixes (port/ps5/shared/hd_music_host.h) ----------------------------- */
#include <stdio.h>

#include "bridge_calls.h"

enum { HD_MUSIC_OPEN, HD_MUSIC_PLAY, HD_MUSIC_PAUSE, HD_MUSIC_CLOSE };

static void Close(void) {
    if (sSong >= 0)
        h_hd_music(HD_MUSIC_CLOSE, "", 0);
    sSong = -1;
}

static int Open(int song) {
    char name[160];
    Close();
    snprintf(name, sizeof(name), "hd/music/%s.ogg", kTracks[song]);
    if (!h_hd_music(HD_MUSIC_OPEN, PortConfigPath(name), 0))
        return 0;
    sSong = song;
    PortLog("hd music: song %d -> %s", song, kTracks[song]);
    return 1;
}

static void Feed(float gain) {
    h_hd_music(HD_MUSIC_PLAY, "", (int)(gain * 1000.0f));
}

static void Pause(void) {
    if (sSong >= 0)
        h_hd_music(HD_MUSIC_PAUSE, "", 0);
}

static void Shutdown(void) {
    Close();
}
static int QueuedMs(void) {
    return -1;
}
static long Position(void) {
    return -1;
}
#endif

/* KHCOM_HDAUDIOLOG=file (test_hd_audio_flow.py): one line per audio frame - the game's song, the HD track's song,
   state (0 none, 1 plays, 2 paused, 3 loading), queued ms, position in samples, the cartridge music gain left */
static void HdLog(int song, int state) {
#if __STDC_HOSTED__
    static FILE* f;
    static int tried;
    static unsigned frame;
    const char* path;
    if (!tried) {
        tried = 1;
        path = SDL_getenv("KHCOM_HDAUDIOLOG");
        if (path)
            f = fopen(path, "w");
    }
    if (f)
        fprintf(f, "%u %d %d %d %d %ld %.3f\n", frame++, song, sSong, state, QueuedMs(), Position(),
                (double)gPortMix.musicGain);
#else
    (void)song, (void)state; /* the bridge images have no files of their own */
#endif
}

/* Once per frame, after the mixer gains are set (port_audio.c). */
void PortHdMusicFrame(void) {
    const MusicPlayerInfo* bgm = &gMPlayInfo_BGM;
    int song, playing;
    float gain;

    if (!gPortConfig.hdMusic) {
        if (sSong >= 0)
            Close();
        return;
    }
    song = CurrentSong();
    playing = song > 0 && (bgm->status & MUSICPLAYER_STATUS_TRACK) && !(bgm->status & MUSICPLAYER_STATUS_PAUSE);
    /* our Options overlay does not stop the song (the cartridge's keeps playing under it, so does the HD track) */
    if (!playing) {
        Pause(); /* stopped or paused: the track resumes from where it was */
        if (song != sSong && sSong >= 0 && !(song > 0 && (bgm->status & MUSICPLAYER_STATUS_PAUSE)))
            Close();
        HdLog(song, sSong >= 0 ? 2 : 0);
        return;
    }
    if (song != sSong) {
        int opened;
        if (song == sMissing) {
            HdLog(song, 0);
            return; /* not imported: the cartridge's song plays */
        }
        opened = Open(song);
        if (opened < 0) {
            /* loading (a few frames): silence, not a moment of the cartridge's song before the HD track */
            gPortMix.musicGain = 0.0f;
            HdLog(song, 3);
            return;
        }
        if (!opened) {
            PortLog("hd music: no track for song %d (%s)", song, kTracks[song]);
            sMissing = song;
            HdLog(song, 0);
            return;
        }
        sMissing = -1;
    }
    /* the Re:CoM tracks are mastered about 10 dB above the cartridge's mix (title -7, field -10, battle -11 dB,
       measured 22/10): this brings them to its loudness, so the sound effects keep their balance */
    gain = HD_MUSIC_LEVEL * gPortMix.musicGain * gPortConfig.masterVolume / 100.0f;
    if (bgm->fadeOI)
        gain *= (float)(bgm->fadeOV >> FADE_VOL_SHIFT) / FADE_VOL_MAX;
    gPortMix.musicGain = 0.0f; /* the cartridge's song keeps playing, silent */
    Feed(gain);
    HdLog(song, 1);
}

void PortHdMusicShutdown(void) {
    Shutdown();
}

/* The game makes no sound this frame (a menu holds it): the track waits instead of running dry. */
void PortHdMusicHold(void) {
    if (sSong >= 0)
        Pause();
}

/* The track's output level on top of its own (PortAudioMovieHold fades it out before an HD movie). */
void PortHdMusicGain(float gain) {
#if __STDC_HOSTED__
    sHold = gain;
    if (sStream)
        SDL_SetAudioStreamGain(sStream, sLevel * sHold);
#else
    (void)gain; /* the bridge's player has no output level */
#endif
}
#else
void PortHdMusicHold(void) {
}
void PortHdMusicGain(float gain) {
    (void)gain;
}
void PortHdMusicFrame(void) {
}
void PortHdMusicShutdown(void) {
}
const char* PortHdMusicTrack(int song) {
    (void)song;
    return NULL;
}
#endif
