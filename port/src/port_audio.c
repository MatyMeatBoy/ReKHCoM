/*
 * Sound output.
 *
 * Each frame combines the three GBA sound sources and queues the result on
 * an SDL audio stream:
 *   - DirectSound driven by the m4a mixer (read from gPortMix),
 *   - DirectSound fed by plain FIFO streaming (movies, staff roll),
 *   - the four PSG channels, emulated from the sound registers.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#include "m4a.h"
#include "port/port.h"

static int AudioLog(void) {
    const char* v;
    PORT_ENV_ONCE(v, SDL_getenv, "KHCOM_AUDIOLOG");
    return v != NULL;
}
#include "port/port_audio.h"
#include "port/port_state.h"

#define OUT_RATE 48000
#define FRAME_SAMPLES_MAX PORT_MIX_MAX

static SDL_AudioStream* sStream;
static int sStreamRate;
static int sStreamPriming = 1;
static float sQueuedAvg = -1.0f;
static float sOut[FRAME_SAMPLES_MAX * 2];
static float sPsgL[FRAME_SAMPLES_MAX], sPsgR[FRAME_SAMPLES_MAX];
static float sDsAR[FRAME_SAMPLES_MAX];

int PortPacedByVsync(void);

/* ---- PSG ------------------------------------------------------------------ */

typedef struct PsgChannel {
    int on;
    int length;
    int lengthEnable;
    int volume;
    int envDir;
    int envPeriod;
    int envTimer;
    double phase;
    uint16_t freq;
    uint16_t lastRegFreq;
    /* channel 1 sweep */
    int sweepTimer;
    int sweepEnabled;
    uint16_t sweepShadow;
    /* channel 4 */
    uint16_t lfsr;
} PsgChannel;

static PsgChannel sPsg[4];
static double sSeqPhase;
static int sSeqStep;

static const uint8_t sDuty[4][8] = {
    { 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 1, 1 },
    { 0, 1, 1, 1, 1, 1, 1, 0 },
};

static const uint8_t sNrx4[4] = { 0x65, 0x6D, 0x75, 0x7D };

/* Mean of a square channel's duty pattern (+1/-1 per eighth) between
   phase p and p + inc, in eighths: the box-filtered waveform, so the
   output does not alias the way point samples do. */
static double DutyIntegral(int duty, double p) {
    static const int8_t prefix[4][9] = {
        {0,-1,-2,-3,-4,-5,-6,-7,-6},
        {0, 1, 0,-1,-2,-3,-4,-5,-4},
        {0, 1, 0,-1,-2,-3,-2,-1, 0},
        {0,-1, 0, 1, 2, 3, 4, 5, 4},
    };
    /* Oscillator phases are small and non-negative. A prefix sum avoids
       scanning all eight duty steps twice for every output sample. */
    double periods = p >= 0 ? (double)(int)(p * 0.125) : SDL_floor(p * 0.125);
    double r = p - periods * 8.0;
    int k = (int)r;
    if (k > 7) k = 7;
    return periods * prefix[duty][8] + prefix[duty][k] +
           (sDuty[duty][k] ? r - k : k - r);
}

static uint16_t RegFreq(int ch) {
    static const uint8_t lo[3] = { 0x64, 0x6C, 0x74 };
    return (uint16_t)(IO8(lo[ch]) | ((IO8(sNrx4[ch]) & 7) << 8));
}

/* Channel 1's next sweep frequency; above 2047 the channel turns off. */
static int SweepNext(PsgChannel* c, uint8_t nr10) {
    int delta = c->sweepShadow >> (nr10 & 7);
    return (nr10 & 8) ? c->sweepShadow - delta : c->sweepShadow + delta;
}

static void LoadEnvelope(PsgChannel* c, uint8_t nrx2) {
    c->volume = nrx2 >> 4;
    c->envDir = (nrx2 >> 3) & 1;
    c->envPeriod = nrx2 & 7;
    c->envTimer = c->envPeriod ? c->envPeriod : 8;
}

static void Trigger(int ch) {
    PsgChannel* c = &sPsg[ch];

    c->on = 1;
    switch (ch) {
        case 0:
        case 1: {
            uint8_t nrx1 = IO8(ch == 0 ? 0x62 : 0x68);
            LoadEnvelope(c, IO8(ch == 0 ? 0x63 : 0x69));
            c->length = 64 - (nrx1 & 63);
            c->freq = c->lastRegFreq = RegFreq(ch);
            if (ch == 0) {
                uint8_t nr10 = IO8(0x60);
                int time = (nr10 >> 4) & 7;
                c->sweepShadow = c->freq;
                c->sweepTimer = time ? time : 8;
                c->sweepEnabled = time != 0 || (nr10 & 7) != 0;
                /* with a shift, the hardware checks the next step at once */
                if ((nr10 & 7) && SweepNext(c, nr10) > 2047)
                    c->on = 0;
            }
            if ((c->volume == 0 && !c->envDir))
                c->on = 0;
            break;
        }
        case 2:
            c->length = 256 - IO8(0x72);
            c->freq = c->lastRegFreq = RegFreq(2);
            c->phase = 0;
            if (!(IO8(0x70) & 0x80))
                c->on = 0;
            break;
        case 3:
            LoadEnvelope(c, IO8(0x79));
            c->length = 64 - (IO8(0x78) & 63);
            c->lfsr = 0x7FFF;
            if ((c->volume == 0 && !c->envDir))
                c->on = 0;
            break;
    }
    c->lengthEnable = (IO8(sNrx4[ch]) >> 6) & 1;
}

static void PsgFrameRegisters(void) {
    int ch;

    for (ch = 0; ch < 4; ch++) {
        uint8_t nrx4 = IO8(sNrx4[ch]);

        if (nrx4 & 0x80) {
            Trigger(ch);
            IO8(sNrx4[ch]) = nrx4 & 0x7F;
        } else {
            sPsg[ch].lengthEnable = (nrx4 >> 6) & 1;
        }
        if (ch < 3) {
            uint16_t f = RegFreq(ch);
            if (f != sPsg[ch].lastRegFreq) {
                sPsg[ch].lastRegFreq = f;
                sPsg[ch].freq = f;
                if (ch == 0)
                    sPsg[0].sweepShadow = f;
            }
        }
    }
    if (!(IO8(0x70) & 0x80))
        sPsg[2].on = 0;
}

static void SequencerStep(void) {
    int ch;

    if ((sSeqStep & 1) == 0) {
        for (ch = 0; ch < 4; ch++) {
            PsgChannel* c = &sPsg[ch];
            if (c->lengthEnable && c->length > 0 && --c->length == 0)
                c->on = 0;
        }
    }
    if (sSeqStep == 2 || sSeqStep == 6) {
        PsgChannel* c = &sPsg[0];
        uint8_t nr10 = IO8(0x60);
        int time = (nr10 >> 4) & 7;

        if (--c->sweepTimer <= 0) {
            c->sweepTimer = time ? time : 8;
            if (c->sweepEnabled && time) {
                int nf = SweepNext(c, nr10);
                if (nf > 2047) {
                    c->on = 0;
                } else if (nr10 & 7) {
                    c->sweepShadow = (uint16_t)nf;
                    c->freq = (uint16_t)nf;
                    /* and the step after it, again */
                    if (SweepNext(c, nr10) > 2047)
                        c->on = 0;
                }
            }
        }
    }
    if (sSeqStep == 7) {
        static const int envCh[3] = { 0, 1, 3 };
        int i;
        for (i = 0; i < 3; i++) {
            PsgChannel* c = &sPsg[envCh[i]];
            if (c->envPeriod == 0)
                continue;
            if (--c->envTimer <= 0) {
                c->envTimer = c->envPeriod;
                if (c->envDir && c->volume < 15)
                    c->volume++;
                else if (!c->envDir && c->volume > 0)
                    c->volume--;
            }
        }
    }
    sSeqStep = (sSeqStep + 1) & 7;
}

static void PsgRender(int n, int rate) {
    uint8_t nr50 = IO8(0x80), nr51 = IO8(0x81);
    uint16_t cnth = IO16(0x82);
    int masterOn = (IO8(0x84) & 0x80) != 0;
    float ratio = (cnth & 3) == 2 ? 1.0f : ((cnth & 3) == 1 ? 0.5f : 0.25f);
    float volR = (float)((nr50 & 7) + 1);
    float volL = (float)(((nr50 >> 4) & 7) + 1);
    float gain[4];
    const uint8_t* wave = (const uint8_t*)(uintptr_t)(GBA_IO + 0x90);
    int i, ch;
    SoundInfo* info = gSoundInfoPtr;
    extern MusicPlayerInfo gMPlayInfo_BGM;

    for (ch = 0; ch < 4; ch++) {
        gain[ch] = gPortMix.sfxGain;
        if (info != NULL && info->cgbChans != NULL) {
            const MusicPlayerTrack* t = info->cgbChans[ch].track;
            if (t != NULL && gMPlayInfo_BGM.tracks != NULL && t >= gMPlayInfo_BGM.tracks
                && t < gMPlayInfo_BGM.tracks + gMPlayInfo_BGM.trackCount) /* its own tracks only (m4a_port.c) */
                gain[ch] = gPortMix.musicGain;
        }
    }

    PsgFrameRegisters();
    for (i = 0; i < n; i++) {
        float l = 0, r = 0;

        sSeqPhase += 512.0 / rate;
        while (sSeqPhase >= 1.0) {
            sSeqPhase -= 1.0;
            SequencerStep();
        }
        if (!masterOn) {
            sPsgL[i] = sPsgR[i] = 0;
            continue;
        }
        for (ch = 0; ch < 4; ch++) {
            PsgChannel* c = &sPsg[ch];
            float s = 0;

            if (!c->on)
                continue;
            switch (ch) {
                case 0:
                case 1: {
                    int duty = IO8(ch == 0 ? 0x62 : 0x68) >> 6;
                    double hz = 131072.0 / (2048 - c->freq);
                    double inc = hz * 8.0 / rate, p0 = c->phase;
                    c->phase += inc;
                    s = (float)((DutyIntegral(duty, c->phase) - DutyIntegral(duty, p0)) / inc) * (float)c->volume;
                    if (c->phase >= 8.0)
                        c->phase -= 8.0 * (int)(c->phase / 8.0);
                    break;
                }
                case 2: {
                    uint8_t nr32 = IO8(0x73);
                    int idx, nib, shift;
                    double hz = 2097152.0 / (2048 - c->freq);
                    c->phase += hz / rate;
                    if (c->phase >= 32.0)
                        c->phase -= 32.0 * (int)(c->phase / 32.0);
                    idx = (int)c->phase;
                    nib = (idx & 1) ? (wave[idx >> 1] & 15) : (wave[idx >> 1] >> 4);
                    s = (float)(nib * 2 - 15);
                    if (nr32 & 0x80) {
                        s *= 0.75f;
                    } else {
                        shift = (nr32 >> 5) & 3;
                        s = shift == 0 ? 0 : s / (float)(1 << (shift - 1));
                    }
                    break;
                }
                case 3: {
                    uint8_t nr43 = IO8(0x7C);
                    double div = (nr43 & 7) ? (double)(nr43 & 7) : 0.5;
                    double hz = 524288.0 / div / (double)(2 << (nr43 >> 4));
                    /* shifts 14 and 15 stop the generator: the output holds */
                    if ((nr43 >> 4) < 14)
                        c->phase += hz / rate;
                    while (c->phase >= 1.0) {
                        int bit = (c->lfsr ^ (c->lfsr >> 1)) & 1;
                        c->phase -= 1.0;
                        c->lfsr = (uint16_t)((c->lfsr >> 1) | (bit << 14));
                        if (nr43 & 8)
                            c->lfsr = (uint16_t)((c->lfsr & ~0x40) | (bit << 6));
                    }
                    s = (c->lfsr & 1) ? -(float)c->volume : (float)c->volume;
                    break;
                }
            }
            s *= gain[ch];
            if (nr51 & (0x10 << ch))
                l += s;
            if (nr51 & (0x01 << ch))
                r += s;
        }
        sPsgL[i] = l * volL * ratio;
        sPsgR[i] = r * volR * ratio;
    }

    /* Channel status bits read back by the game. */
    {
        uint8_t nr52 = IO8(0x84) & 0x80;
        for (ch = 0; ch < 4; ch++)
            if (sPsg[ch].on)
                nr52 |= 1 << ch;
        IO8(0x84) = nr52;
        const char* psgLog;
        PORT_ENV_ONCE(psgLog, SDL_getenv, "KHCOM_PSGLOG");
        if ((nr51 & ((nr52 & 15) * 0x11)) && psgLog) {
            static int sLogged;
            if (sLogged++ < 20)
                PortLog("psg: channels %x nr50 %02x nr51 %02x", nr52 & 15, nr50, nr51);
        }
    }
}

/* ---- FIFO streaming ---------------------------------------------------------- */

typedef struct Fifo {
    uint32_t lastSad;
    uint32_t pos;
    double frac;
} Fifo;

static Fifo sFifo[2];
static double sStreamFrac;

/* The DMA channel (1 or 2) feeding FIFO A (0) or B (1), 0 if none. */
static int FifoDma(int fifo) {
    int dma;

    for (dma = 1; dma <= 2; dma++) {
        uint32_t cnt = IO32(0xB8 + dma * 12);
        if ((cnt & 0x80000000u) && ((cnt >> 28) & 3) == 3 && IO32(0xB4 + dma * 12) == (uint32_t)(GBA_IO + 0xA0 + fifo * 4))
            return dma;
    }
    return 0;
}

/* Sample rate of a FIFO: the overflow rate of its timer (SOUNDCNT_H bit 10 / 14). */
static double FifoRate(int fifo) {
    int timer = (IO16(0x82) >> (fifo ? 14 : 10)) & 1;

    if (!(IO16(0x102 + timer * 4) & 0x80))
        return 0;
    return 16777216.0 / (double)(0x10000 - IO16(0x100 + timer * 4));
}

/* n output samples from a FIFO's stream, `step` stream samples apiece. */
static void FifoRead(int fifo, float* out, int n, float scale, double step) {
    Fifo* f = &sFifo[fifo];
    int dma = FifoDma(fifo);
    uint32_t sad = IO32(0xB0 + dma * 12);
    int i;

    if (sad != f->lastSad) {
        f->lastSad = sad;
        f->pos = sad;
        f->frac = 0;
    }
    for (i = 0; i < n; i++) {
        out[i] = (float)*(const int8_t*)(uintptr_t)f->pos * scale;
        f->frac += step;
        while (f->frac >= 1.0) {
            f->frac -= 1.0;
            f->pos++;
        }
    }
}

static int InM4aBuffer(uint32_t addr) {
    SoundInfo* info = gSoundInfoPtr;
    uint32_t base;

    if (info == NULL)
        return 0;
    base = (uint32_t)(uintptr_t)info->pcmBuffer;
    return addr >= base && addr < base + sizeof(info->pcmBuffer);
}

/* ---- output -------------------------------------------------------------------- */

void PortAudioVisitState(PortStateVisitor visit) {
    visit(sPsg, sizeof(sPsg));
    visit(&sSeqPhase, sizeof(sSeqPhase));
    visit(&sSeqStep, sizeof(sSeqStep));
    visit(sFifo, sizeof(sFifo));
    visit(&sStreamFrac, sizeof(sStreamFrac));
}

/* Options menu: the PSG channels stop sounding (a note held at volume 0
   still leaves its level); the menu puts the saved state back on closing. */
void PortAudioPsgOff(void) {
    int ch;
    for (ch = 0; ch < 4; ch++)
        sPsg[ch].on = 0;
}

void PortAudioReset(void) {
    if (sStream != NULL) {
        SDL_PauseAudioStreamDevice(sStream);
        SDL_ClearAudioStream(sStream);
        SDL_SetAudioStreamFrequencyRatio(sStream, 1.0f);
    }
    sStreamPriming = 1;
    sQueuedAvg = -1.0f;
}

/* An HD movie takes the sound over (port_hdmovie.c): the game's output and the HD track fade out in 150 ms, so
   nothing of the field's music plays under the movie and nothing stops with a pop; after it they are back at full
   level and the game's output primes again as after a pause. */
void PortAudioMovieHold(int hold) {
    extern void PortHdMusicGain(float gain);
    extern void PortHdMusicHold(void);
    int i;
    if (hold) {
        for (i = 9; i >= 0; i--) {
            if (sStream != NULL)
                SDL_SetAudioStreamGain(sStream, i / 10.0f);
            PortHdMusicGain(i / 10.0f);
            SDL_Delay(15);
        }
        PortHdMusicHold();
        PortAudioReset();
    } else {
        if (sStream != NULL)
            SDL_SetAudioStreamGain(sStream, 1.0f);
        PortHdMusicGain(1.0f);
    }
}

int PortAudioInit(void) {
    SDL_AudioSpec out = { SDL_AUDIO_F32, 2, OUT_RATE };

    gPortMix.oversample = 4;
    gPortMix.musicGain = 1.0f;
    gPortMix.sfxGain = 1.0f;
    sStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &out, NULL, NULL);
    if (sStream == NULL) {
        PortLog("Audio: could not open the output device: %s", SDL_GetError());
        return 0;
    }
    /* The convenience API opens paused. Fill the intended safety buffer
       before starting playback, rather than racing the first audio callback. */
    sStreamPriming = 1;
    sQueuedAvg = -1.0f;
    return 1;
}

void PortAudioShutdown(void) {
    if (sStream != NULL)
        SDL_DestroyAudioStream(sStream);
    sStream = NULL;
}

static void Queue(int n, int rate) {
    if (sStream == NULL)
        return;
    if (rate != sStreamRate) {
        SDL_AudioSpec in = { SDL_AUDIO_F32, 2, rate };
        SDL_SetAudioStreamFormat(sStream, &in, NULL);
        sStreamRate = rate;
    }
    SDL_PutAudioStreamData(sStream, sOut, n * 2 * (int)sizeof(float));

    /* Keep ~4 frames queued. The fill level is smoothed and the playback rate
       corrected by at most 0.3% (about 5 cents), which is inaudible; only a
       large backlog (e.g. after a pause) is dropped outright. */
    {
        float queued = (float)(SDL_GetAudioStreamQueued(sStream) / (2 * (int)sizeof(float)));
        float target = rate / (float)GBA_FPS * 4.0f;
        float ratio;

        if (queued > target * 6) {
            PortAudioReset();
            queued = 0;
        }
        if (sStreamPriming) {
            if (queued + 1.0f < target) /* allow fractional samples/frame */
                return;
            SDL_ResumeAudioStreamDevice(sStream);
            sStreamPriming = 0;
            if (AudioLog())
                PortLog("audio: playback primed with %.0f samples at %d Hz", queued, rate);
        }
        if (sQueuedAvg < 0)
            sQueuedAvg = queued;
        sQueuedAvg += (queued - sQueuedAvg) * 0.02f;
        /* Production already matches consumption (59.7275 frames x samples per
           frame = sample rate), so normally play at exactly 1.0. Only a real
           drift outside the dead band is corrected, by at most 0.15%. */
        if (sQueuedAvg < target * 0.5f)
            ratio = 0.9985f;
        else if (sQueuedAvg > target * 3.0f)
            ratio = 1.0015f;
        else
            ratio = 1.0f;
        SDL_SetAudioStreamFrequencyRatio(sStream, ratio);
        if (AudioLog()) {
            static int sTick;
            if (++sTick % 60 == 0) {
                SDL_AudioSpec src, dst;
                SDL_GetAudioStreamFormat(sStream, &src, &dst);
                PortLog("audio: queued %.0f (avg %.0f, target %.0f) ratio %.4f in %d Hz out %d Hz", queued, sQueuedAvg,
                        target, ratio, src.freq, dst.freq);
            }
        }
    }
}

/* Experimental smooth audio: a 2-pole Butterworth low-pass at 9 kHz on the
   final mix, so what the GBA's low sample rates leave above it (images of
   the 13-31 kHz samples) is softened. One filter per channel, coefficients
   per output rate. */
static void SmoothFilter(int n, int rate) {
    static float z[2][2];
    static int sRate;
    static float b0, b1, b2, a1, a2;
    int i, c;

    if (rate < 20000)
        return;
    if (rate != sRate) {
        float w = 2.0f * SDL_PI_F * 9000.0f / rate, cs = SDL_cosf(w), al = SDL_sinf(w) / (2.0f * 0.7071f);
        float a0 = 1.0f + al;
        sRate = rate;
        b0 = (1.0f - cs) * 0.5f / a0;
        b1 = (1.0f - cs) / a0;
        b2 = b0;
        a1 = -2.0f * cs / a0;
        a2 = (1.0f - al) / a0;
    }
    for (c = 0; c < 2; c++) {
        for (i = 0; i < n; i++) {
            float x = sOut[i * 2 + c];
            float y = b0 * x + z[c][0];
            z[c][0] = b1 * x - a1 * y + z[c][1];
            z[c][1] = b2 * x - a2 * y;
            sOut[i * 2 + c] = y;
        }
    }
}

/* While the options menu pauses the game: send the m4a mix only (its click);
   a movie or stream must not advance behind the menu. */
void PortAudioMenuFrame(void) {
    extern void PortHdMusicHold(void);
    if (gPortMix.ready && InM4aBuffer(IO32(0xBC))) {
        PortAudioFrame();
    } else {
        gPortMix.ready = 0;
        PortHdMusicHold(); /* the cartridge is silent: the HD track waits with it */
    }
}

void PortAudioFrame(void) {
    uint16_t cnth = IO16(0x82);
    float master = gPortConfig.masterVolume / 100.0f;
    float aScale = (cnth & 4) ? 4.0f : 2.0f;
    float bScale = (cnth & 8) ? 4.0f : 2.0f;
    int n, rate, i;
    int haveB = 0;
    static float sDsBL[FRAME_SAMPLES_MAX];

    gPortMix.musicGain = gPortConfig.musicVolume / 100.0f;
    gPortMix.sfxGain = gPortConfig.sfxVolume / 100.0f;
    PortHdMusicFrame(); /* the imported HD track, if any, silences the cartridge's BGM channels */

    if (gPortMix.ready && InM4aBuffer(IO32(0xBC))) {
        /* m4a: FIFO A carries the right mix, FIFO B the left one. */
        n = gPortMix.count;
        rate = gPortMix.rate;
        for (i = 0; i < n; i++) {
            sDsAR[i] = gPortMix.right[i] / 256.0f * aScale;
            sDsBL[i] = gPortMix.left[i] / 256.0f * bScale;
        }
        haveB = 1;
    } else if ((FifoDma(0) && FifoRate(0) > 0) || (FifoDma(1) && FifoRate(1) > 0)) {
        /* Streams: the output runs at FIFO A's rate (B's when A is idle);
           the other FIFO is stepped at its own timer's rate. */
        int lead = FifoDma(0) && FifoRate(0) > 0 ? 0 : 1;
        double hz = FifoRate(lead);
        double want = hz / GBA_FPS + sStreamFrac;

        n = (int)want;
        sStreamFrac = want - n;
        if (n > FRAME_SAMPLES_MAX)
            n = FRAME_SAMPLES_MAX;
        rate = (int)(hz + 0.5);
        if (lead == 0) {
            FifoRead(0, sDsAR, n, aScale, 1.0);
        } else {
            memset(sDsAR, 0, n * sizeof(float));
        }
        if (FifoDma(1) && FifoRate(1) > 0) {
            FifoRead(1, sDsBL, n, bScale, FifoRate(1) / hz);
            haveB = 1;
        }
    } else {
        rate = 32768 * 2;
        n = (int)(rate / GBA_FPS);
        memset(sDsAR, 0, n * sizeof(float));
    }
    gPortMix.ready = 0;

    PsgRender(n, rate);

    for (i = 0; i < n; i++) {
        float a = sDsAR[i], b = haveB ? sDsBL[i] : 0;
        float l = sPsgL[i], r = sPsgR[i];

        if (cnth & 0x100)
            r += a;
        if (cnth & 0x200)
            l += a;
        if (cnth & 0x1000)
            r += b;
        if (cnth & 0x2000)
            l += b;
        sOut[i * 2 + 0] = SDL_clamp(l / 1024.0f * master, -1.0f, 1.0f);
        sOut[i * 2 + 1] = SDL_clamp(r / 1024.0f * master, -1.0f, 1.0f);
    }
    if (gPortConfig.smoothAudio)
        SmoothFilter(n, rate);
    Queue(n, rate);

    {
        /* KHCOM_WAVDUMP=file: raw float32 stereo of everything queued, with
           the rate of each block in a side file (development aid). */
        static FILE* sDump;
        static FILE* sDumpRates;
        const char* path = SDL_getenv("KHCOM_WAVDUMP");
        if (path != NULL) {
            if (sDump == NULL) {
                char rates[512];
                sDump = fopen(path, "wb");
                SDL_snprintf(rates, sizeof(rates), "%s.rates", path);
                sDumpRates = fopen(rates, "w");
            }
            if (sDump != NULL) {
                fwrite(sOut, sizeof(float), (size_t)n * 2, sDump);
                fprintf(sDumpRates, "%d %d\n", n, rate);
            }
        }
    }
}
