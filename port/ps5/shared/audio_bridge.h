/* Optional lifecycle commands through the existing HF_thread slot. Older
 * hosts return zero for unknown commands; the fixed bridge ABI is unchanged. */
#ifndef AUDIO_BRIDGE_H
#define AUDIO_BRIDGE_H
#define BRIDGE_AUDIO_MAGIC 0x41554431
#define BRIDGE_AUDIO_PROBE 0x200
#define BRIDGE_AUDIO_PAUSE 0x201
#define BRIDGE_AUDIO_RESUME 0x202
#define BRIDGE_AUDIO_GAIN 0x203
#endif
