/* Bounded SDL PCM underflow recovery. No engine calls, I/O or decoder work
 * on the consumer thread. All state is protected by the stream's recursive
 * lock (SDL holds it while invoking the get callback). Normal PCM is handed
 * to SDL unchanged; only missing audio and the first 5 ms of recovery fade. */
#ifndef PORT_AUDIO_CONTINUITY_H
#define PORT_AUDIO_CONTINUITY_H
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>

typedef struct PortAudioContinuity {
    int attached, starved, fadeOutAt, fadeOutFrames, fadeInAt, fadeInFrames;
    float tail[2];
    uint32_t underruns;
    union { float f32[2048 * 2]; Sint16 s16[2048 * 2]; } scratch;
} PortAudioContinuity;

static int PortAudioContinuitySpec(SDL_AudioStream* stream, SDL_AudioSpec* spec, int* fadeFrames) {
    float ratio;
    if (!SDL_GetAudioStreamFormat(stream, spec, NULL) || spec->channels != 2 || spec->freq < 1 ||
        spec->freq > 384000 || (spec->format != SDL_AUDIO_F32 && spec->format != SDL_AUDIO_S16))
        return 0;
    ratio = SDL_GetAudioStreamFrequencyRatio(stream);
    if (!(ratio > 0.0f && ratio <= 16.0f)) ratio = 1.0f;
    *fadeFrames = SDL_clamp((int)(spec->freq * (double)ratio / 200.0), 1, 2048);
    return spec->format == SDL_AUDIO_F32 ? 8 : 4;
}

static float PortAudioContinuityRead(const void* data, int sample, SDL_AudioFormat format) {
    return format == SDL_AUDIO_F32 ? ((const float*)data)[sample] : ((const Sint16*)data)[sample] / 32768.0f;
}

static void PortAudioContinuityWrite(PortAudioContinuity* state, int sample, SDL_AudioFormat format, float value) {
    if (format == SDL_AUDIO_F32) state->scratch.f32[sample] = value;
    else state->scratch.s16[sample] = (Sint16)SDL_clamp(value * 32768.0f, -32768.0f, 32767.0f);
}

static void PortAudioContinuityRamp(PortAudioContinuity* state, SDL_AudioFormat format, int count, int frames) {
    for (int i = 0; i < count; i++) {
        int at = state->fadeOutAt + i + 1;
        float level = at < frames ? 1.0f - at / (float)frames : 0.0f;
        for (int channel = 0; channel < 2; channel++)
            PortAudioContinuityWrite(state, i * 2 + channel, format, state->tail[channel] * level);
    }
}

static void SDLCALL PortAudioContinuityGet(void* userdata, SDL_AudioStream* stream, int additional, int total) {
    PortAudioContinuity* state = userdata;
    SDL_AudioSpec spec;
    int frames, frameBytes, count;
    (void)total;
    if (additional <= 0 || !(frameBytes = PortAudioContinuitySpec(stream, &spec, &frames))) return;
    if (!state->starved) {
        state->starved = 1; state->fadeOutAt = 0; state->fadeOutFrames = frames; state->fadeInFrames = 0;
        state->underruns++;
    }
    /* One bounded device request. No block repeats and no unbounded fill loop. */
    count = SDL_min(2048, additional / frameBytes + (additional % frameBytes != 0));
    PortAudioContinuityRamp(state, spec.format, count, state->fadeOutFrames);
    if (SDL_PutAudioStreamData(stream, &state->scratch, count * frameBytes))
        state->fadeOutAt = SDL_min(state->fadeOutFrames, state->fadeOutAt + count);
    else {
        /* SDL could not supply this request: the device falls back to silence.
         * Never bring its previous nonzero tail back on a later callback. */
        state->fadeOutAt = state->fadeOutFrames;
        state->tail[0] = state->tail[1] = 0.0f;
    }
}

static bool PortAudioContinuityInit(PortAudioContinuity* state, SDL_AudioStream* stream) {
    memset(state, 0, sizeof(*state));
    state->attached = SDL_SetAudioStreamGetCallback(stream, PortAudioContinuityGet, state);
    return state->attached != 0;
}

static bool PortAudioContinuityClear(PortAudioContinuity* state, SDL_AudioStream* stream) {
    bool ok;
    if (!state->attached) return SDL_ClearAudioStream(stream);
    if (!SDL_LockAudioStream(stream)) return false;
    ok = SDL_ClearAudioStream(stream);
    if (ok) {
        state->starved = state->fadeOutAt = state->fadeOutFrames = state->fadeInAt = state->fadeInFrames = 0;
        state->tail[0] = state->tail[1] = 0.0f;
    }
    SDL_UnlockAudioStream(stream);
    return ok;
}

static uint32_t PortAudioContinuityUnderruns(PortAudioContinuity* state, SDL_AudioStream* stream) {
    uint32_t count;
    if (!state->attached || !SDL_LockAudioStream(stream)) return 0;
    count = state->underruns;
    SDL_UnlockAudioStream(stream);
    return count;
}

static void PortAudioContinuityTail(PortAudioContinuity* state, const void* data, int count, SDL_AudioFormat format) {
    for (int channel = 0; channel < 2; channel++)
        state->tail[channel] = PortAudioContinuityRead(data, (count - 1) * 2 + channel, format);
}

static bool PortAudioContinuityPut(PortAudioContinuity* state, SDL_AudioStream* stream, const void* data, int bytes) {
    SDL_AudioSpec spec;
    int frameBytes, frames, count, prefix = 0;
    bool ok = false;
    if (!state->attached || bytes == 0) return SDL_PutAudioStreamData(stream, data, bytes);
    if (!SDL_LockAudioStream(stream)) return false;
    frameBytes = PortAudioContinuitySpec(stream, &spec, &frames);
    if (!frameBytes || bytes < 0 || bytes % frameBytes || !data) goto done;
    count = bytes / frameBytes;
    if (state->starved) {
        /* A producer can return midway through a short consumer ramp. Finish
         * that ramp before the new fade-in, rather than jumping to zero. */
        int remaining = state->fadeOutFrames - state->fadeOutAt;
        if (remaining > 0) {
            PortAudioContinuityRamp(state, spec.format, remaining, state->fadeOutFrames);
            if (!SDL_PutAudioStreamData(stream, &state->scratch, remaining * frameBytes)) goto done;
            state->fadeOutAt = state->fadeOutFrames;
        }
        state->fadeInAt = 0; state->fadeInFrames = frames;
    }
    if (state->fadeInAt < state->fadeInFrames) {
        prefix = SDL_min(count, state->fadeInFrames - state->fadeInAt);
        for (int i = 0; i < prefix; i++) {
            float level = (state->fadeInAt + i + 1) / (float)state->fadeInFrames;
            for (int channel = 0; channel < 2; channel++)
                PortAudioContinuityWrite(state, i * 2 + channel, spec.format,
                    PortAudioContinuityRead(data, i * 2 + channel, spec.format) * level);
        }
        if (!SDL_PutAudioStreamData(stream, &state->scratch, prefix * frameBytes)) goto done;
        PortAudioContinuityTail(state, &state->scratch, prefix, spec.format);
        state->fadeInAt += prefix; state->starved = 0;
    }
    if (count > prefix) {
        const void* rest = (const unsigned char*)data + prefix * frameBytes;
        if (!SDL_PutAudioStreamData(stream, rest, (count - prefix) * frameBytes)) goto done;
        PortAudioContinuityTail(state, rest, count - prefix, spec.format);
    }
    ok = true;
done:
    SDL_UnlockAudioStream(stream);
    return ok;
}
#endif
