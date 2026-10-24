/* HD music on the bridge hosts (port/progress/HD_MOD.md): the game image has no C library for stb_vorbis, so it only
 * says which track plays (bridge entry hd_music, port/src/port_hdmusic.c keeps the game's logic) and the host decodes
 * hd/music/<track>.ogg here and mixes it into the game's own sound in H_audio_push: the music follows the game's audio
 * clock, needs no thread and pauses with it.
 *
 * Included once by each host after its H_fopen/H_fread/H_fseek/H_ftell/H_fclose (the files open as the game's do).
 * The host calls HdMusicMix(frames, count, rate) on every block the game pushes, before sending it on. */
#ifndef HD_MUSIC_HOST_H
#define HD_MUSIC_HOST_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HD_MUSIC_LOG
#define HD_MUSIC_LOG(...) ((void)0) /* a host with a log defines it before including this file */
#endif

#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_INTEGER_CONVERSION
#define STB_VORBIS_NO_STDIO
#if __has_include("stb_vorbis.c") /* the Switch/Vita VM builds copy it next to the host source */
#include "stb_vorbis.c"
#else
#include "../../third_party/stb/stb_vorbis.c"
#endif

enum { HD_MUSIC_OPEN, HD_MUSIC_PLAY, HD_MUSIC_PAUSE, HD_MUSIC_CLOSE };

static struct {
    stb_vorbis* vorbis;
    unsigned char* file;
    int playing, gain;                      /* gain: thousandths */
    unsigned rate, loopStart, loopEnd, pos; /* samples of the track */
    float buf[1024 * 2];
    int bufLen, bufAt;
    float a[2], b[2]; /* the two source frames around the output position */
    double frac;
} sHd;

static unsigned char* HdLoad(const char* path, int* size) {
    int h = H_fopen(path, 0), n;
    unsigned char* data = NULL;
    if (h < 0)
        return NULL;
    if (H_fseek(h, 0, 2) == 0 && (n = H_ftell(h)) > 0 && H_fseek(h, 0, 0) == 0 && (data = malloc((size_t)n + 1))) {
        if (H_fread(h, data, (uint32_t)n) != n) {
            free(data);
            data = NULL;
        } else {
            data[n] = 0;
            *size = n;
        }
    }
    H_fclose(h);
    return data;
}

static void HdMusicClose(void) {
    if (sHd.vorbis)
        stb_vorbis_close(sHd.vorbis);
    free(sHd.file);
    memset(&sHd, 0, sizeof(sHd));
}

/* Next frame of the track, looping at its loop points. */
static void HdNext(float* out) {
    if (sHd.bufAt >= sHd.bufLen) {
        int want = 1024, got = 0;
        if (sHd.pos + (unsigned)want > sHd.loopEnd)
            want = (int)(sHd.loopEnd - sHd.pos);
        if (want > 0)
            got = stb_vorbis_get_samples_float_interleaved(sHd.vorbis, 2, sHd.buf, want * 2);
        if (got <= 0) {
            stb_vorbis_seek(sHd.vorbis, sHd.loopStart);
            sHd.pos = sHd.loopStart;
            got = stb_vorbis_get_samples_float_interleaved(sHd.vorbis, 2, sHd.buf, 2048);
            if (got <= 0) {
                out[0] = out[1] = 0.0f;
                return;
            }
        }
        sHd.pos += (unsigned)got;
        sHd.bufLen = got;
        sHd.bufAt = 0;
    }
    out[0] = sHd.buf[sHd.bufAt * 2];
    out[1] = sHd.buf[sHd.bufAt * 2 + 1];
    sHd.bufAt++;
}

static int HdMusicOpen(const char* path) {
    char loopPath[512];
    int size = 0, error = 0;
    size_t len = strlen(path);

    HdMusicClose();
    if (len < 4 || len >= sizeof(loopPath) || !(sHd.file = HdLoad(path, &size))) {
        HD_MUSIC_LOG("hd music: %s does not open", path);
        return 0;
    }
    sHd.vorbis = stb_vorbis_open_memory(sHd.file, size, &error, NULL);
    if (!sHd.vorbis) {
        HD_MUSIC_LOG("hd music: %s does not decode (%d)", path, error);
        HdMusicClose();
        return 0;
    }
    HD_MUSIC_LOG("hd music: %s, %u Hz", path, stb_vorbis_get_info(sHd.vorbis).sample_rate);
    sHd.rate = stb_vorbis_get_info(sHd.vorbis).sample_rate;
    sHd.loopEnd = stb_vorbis_stream_length_in_samples(sHd.vorbis);
    memcpy(loopPath, path, len - 4); /* <track>.ogg → <track>.loop: start and end sample */
    strcpy(loopPath + len - 4, ".loop");
    {
        int loopSize = 0;
        unsigned char* loop = HdLoad(loopPath, &loopSize);
        unsigned s, e;
        if (loop && sscanf((const char*)loop, "%u %u", &s, &e) == 2 && s < e && e <= sHd.loopEnd) {
            sHd.loopStart = s;
            sHd.loopEnd = e;
        }
        free(loop);
    }
    HdNext(sHd.a);
    HdNext(sHd.b);
    return 1;
}

static int H_hd_music(int op, const char* path, int gain) {
    switch (op) {
        case HD_MUSIC_OPEN: return HdMusicOpen(path);
        case HD_MUSIC_PLAY:
            sHd.playing = sHd.vorbis != NULL;
            sHd.gain = gain;
            return sHd.playing;
        case HD_MUSIC_PAUSE: sHd.playing = 0; return 1;
        default: HdMusicClose(); return 1;
    }
}

/* Adds the track to a block of the game's stereo frames at its rate (linear resampling from the track's 48 kHz). */
static void HdMusicMix(int16_t* frames, uint32_t count, int rate) {
    double step;
    float gain;
    uint32_t i;
    if (!sHd.playing || !sHd.vorbis || rate <= 0)
        return;
    step = (double)sHd.rate / rate;
    gain = sHd.gain * (32767.0f / 1000.0f);
    for (i = 0; i < count; i++) {
        int c;
        for (c = 0; c < 2; c++) {
            float s = sHd.a[c] + (sHd.b[c] - sHd.a[c]) * (float)sHd.frac;
            int v = frames[i * 2 + c] + (int)(s * gain);
            frames[i * 2 + c] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        }
        sHd.frac += step;
        while (sHd.frac >= 1.0) {
            sHd.a[0] = sHd.b[0];
            sHd.a[1] = sHd.b[1];
            HdNext(sHd.b);
            sHd.frac -= 1.0;
        }
    }
}

/* The host's H_audio_push: the game's block with the track mixed in, handed to the host's own push (rate: the one
   the game opened the audio at). */
static void HdMusicPush(const int16_t* frames, uint32_t count, int rate, void (*push)(const int16_t*, uint32_t)) {
    static int16_t mix[1024 * 2];
    if (!sHd.playing) {
        push(frames, count);
        return;
    }
    while (count) {
        uint32_t n = count < 1024 ? count : 1024;
        memcpy(mix, frames, n * 4);
        HdMusicMix(mix, n, rate);
        push(mix, n);
        frames += n * 2;
        count -= n;
    }
}

#endif
