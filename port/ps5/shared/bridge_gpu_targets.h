/* Optional graphics commands transported by the existing HF_thread slot.
 * Never append the host table: old hosts return zero for these unknown ops. */
#ifndef BRIDGE_GPU_TARGETS_H
#define BRIDGE_GPU_TARGETS_H
#define BRIDGE_GPU_MAGIC 0x47584631
#define BRIDGE_GPU_PROBE 0x100
#define BRIDGE_GPU_CREATE 0x101
#define BRIDGE_GPU_BIND 0x102
#define BRIDGE_GPU_CLEAR 0x103
#define BRIDGE_GPU_BLEND 0x104
#define BRIDGE_GPU_MAX_SIZE 0x105
#define BRIDGE_GPU_BUDGET 0x106
#define BRIDGE_GPU_SMOOTH 0x107
#define BRIDGE_GPU_PROTOCOL 0x108
#endif
