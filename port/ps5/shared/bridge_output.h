/* Optional OUT1 controls over HF_thread. Unknown operations return zero on old
 * hosts; the fixed bridge table and its calling convention are unchanged. */
#ifndef BRIDGE_OUTPUT_H
#define BRIDGE_OUTPUT_H
#define BRIDGE_OUTPUT_MAGIC 0x4f555431
#define BRIDGE_OUTPUT_PROBE 0x230
#define BRIDGE_OUTPUT_CAPS 0x231
#define BRIDGE_OUTPUT_WINDOW_SET 0x232
#define BRIDGE_OUTPUT_VSYNC_SET 0x233
/* VSync query: zero means unavailable, otherwise actual interval + 2. */
#define BRIDGE_OUTPUT_VSYNC_GET 0x234
#define BRIDGE_OUTPUT_PIXEL_WIDTH 0x235
#define BRIDGE_OUTPUT_PIXEL_HEIGHT 0x236
#define BRIDGE_OUTPUT_WINDOW_SYNC 0x237
#define BRIDGE_OUTPUT_WINDOW_CAP 1
#define BRIDGE_OUTPUT_VSYNC_CAP 2
#endif
