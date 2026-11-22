#ifndef BRIDGE_THREADS_H
#define BRIDGE_THREADS_H
/* Optional x32 low-stack/TLS entry support over the existing thread slot. */
#define BRIDGE_THREADS_PROBE 0x240
#define BRIDGE_THREADS_MAGIC 0x54485231
#define BRIDGE_X32_WORKERS 7
#define BRIDGE_X32_STACK_SHIFT 19
#define BRIDGE_X32_STACK_SIZE (1u << BRIDGE_X32_STACK_SHIFT)
#endif
