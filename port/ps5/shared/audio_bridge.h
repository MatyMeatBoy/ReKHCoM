/* Optional lifecycle commands through the existing HF_thread slot. Older
 * hosts return zero for unknown commands; the fixed bridge ABI is unchanged. */
#ifndef AUDIO_BRIDGE_H
#define AUDIO_BRIDGE_H
#define BRIDGE_AUDIO_MAGIC 0x41554431
#define BRIDGE_AUDIO_PROBE 0x200
#define BRIDGE_AUDIO_PAUSE 0x201
#define BRIDGE_AUDIO_RESUME 0x202
#define BRIDGE_AUDIO_GAIN 0x203
/* Independent of AUD1 lifecycle support: ring hosts also need the input rate
 * for HD mixing, while retaining their device-frame queued-count contract. */
#define BRIDGE_AUDIO_INPUT_RATE 0x204

/* Movie streams are independent of the game stream. Keep hd_movie's four-path
 * ABI unchanged: an exact capability probe precedes a one-shot percent value. */
#define BRIDGE_MOVIE_MAGIC 0x4D4F5631
#define BRIDGE_MOVIE_PROBE 0x210
#define BRIDGE_MOVIE_MASTER 0x211
typedef struct { unsigned percent; int pending; } PortMovieVolumeState;
static inline float PortMovieGain(int percent, float fade) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (!(fade > 0.0f)) fade = 0.0f;
    if (fade > 1.0f) fade = 1.0f;
    return percent * 0.01f * fade;
}
static inline int PortMovieVolumeCommand(PortMovieVolumeState* state, int op, unsigned percent) {
    if (op == BRIDGE_MOVIE_PROBE) return BRIDGE_MOVIE_MAGIC;
    if (op != BRIDGE_MOVIE_MASTER) return -2;
    state->pending = 0;
    if (percent > 100) return 0;
    state->percent = percent;
    state->pending = 1;
    return 1;
}
static inline int PortMovieVolumeTake(PortMovieVolumeState* state) {
    int percent = state->pending ? (int)state->percent : 100;
    state->pending = 0;
    return percent;
}
#endif
