/* SDL hosts queue input S16 frames. The optional protocol advertises those
 * units and provides the lifecycle of SDL_OpenAudioDeviceStream to the guest.
 * Non-SDL/ring hosts retain their existing device-frame bridge contract. */
#ifndef AUDIO_BRIDGE_SDL_H
#define AUDIO_BRIDGE_SDL_H
#include "audio_bridge.h"
static int sAudioBridgeControls;
static PortMovieVolumeState sMovieVolume;
static void HostAudioLogUnderruns(void) {
    static int enabled = -1;
    static uint32_t previous;
    if (enabled < 0) enabled = getenv("KHCOM_AUDIOLOG") != NULL;
    if (enabled && sAudio) {
        uint32_t count = PortAudioContinuityUnderruns(&sAudioContinuity, sAudio);
        if (count != previous) {
            Log("audio continuity: consumer underruns %u", (unsigned)count);
            previous = count;
        }
    }
}
static int HostAudioCommand(int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    (void)b; (void)c; (void)d;
    int movie = PortMovieVolumeCommand(&sMovieVolume, op, a);
    if (movie != -2) return movie;
    switch (op) {
    case BRIDGE_AUDIO_PROBE: sAudioBridgeControls = 1; return BRIDGE_AUDIO_MAGIC;
    case BRIDGE_AUDIO_PAUSE: return sAudio && SDL_PauseAudioStreamDevice(sAudio);
    case BRIDGE_AUDIO_RESUME: return sAudio && SDL_ResumeAudioStreamDevice(sAudio);
    case BRIDGE_AUDIO_GAIN: {
        float gain;
        memcpy(&gain, &a, sizeof(gain));
        if (!(gain >= 0.0f && gain <= 3.402823466e38f)) return 0;
        return sAudio && SDL_SetAudioStreamGain(sAudio, gain);
    }
    default: return -2;
    }
}
#endif
