/* Queue-level control of the output stream (port_audio.c). Pure arithmetic, so
 * it can be checked on the host (port/tools/test_audio_refill.py). */
#ifndef PORT_AUDIO_RATE_H
#define PORT_AUDIO_RATE_H

/* Playback ratio for a smoothed queue level. Without refill only a drift
 * outside the wide dead band is corrected. With it, a level lost to late
 * frames (below 70% of the target) is rebuilt at 0.3% slower playback until it
 * is back at 85%; in between the previous choice holds. */
static inline float PortAudioQueueRatio(float avg, float target, int refill, int* refilling) {
    if (refill) {
        if (avg < target * 0.70f)
            *refilling = 1;
        else if (avg >= target * 0.85f)
            *refilling = 0;
        if (*refilling)
            return 0.9970f;
    }
    if (avg < target * 0.5f)
        return 0.9985f;
    if (avg > target * 3.0f)
        return 1.0015f;
    return 1.0f;
}
#endif
