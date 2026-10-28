/* Optional preview observation. Disabled: no clocks, buffer writes or file I/O.
 * Main/render thread only; CPU-ready is observed when the worker is polled,
 * not at the decoder's completion instant. Present return is not scanout. */
#ifndef PORT_PREVIEW_TRACE_H
#define PORT_PREVIEW_TRACE_H
#define PREVIEW_TRACE_CAPACITY 256
typedef struct PreviewTraceRecord {
    uint64_t counter;
    const char* stage;
    int serial, event, hd, fallback;
    unsigned generation, frame;
} PreviewTraceRecord;
static PreviewTraceRecord sPreviewTrace[PREVIEW_TRACE_CAPACITY];
static unsigned sPreviewTraceCount, sPreviewTraceDropped;
static int sPreviewTraceEnabled = -1, sPreviewTracePending;
static int sPreviewTraceDrawSerial = -1, sPreviewTraceDrawFallback = -1;

static int PreviewTraceEnabled(void) {
    if (sPreviewTraceEnabled < 0) sPreviewTraceEnabled = SDL_getenv("KHCOM_PREVIEW_TRACE") != NULL;
    return sPreviewTraceEnabled;
}
static void PreviewTrace(const char* stage) {
    if (!PreviewTraceEnabled()) return;
    if (sPreviewTraceCount >= PREVIEW_TRACE_CAPACITY) { ++sPreviewTraceDropped; return; }
    PreviewTraceRecord* r = &sPreviewTrace[sPreviewTraceCount++];
    r->counter = SDL_GetPerformanceCounter(); r->stage = stage;
    r->serial = sPreviewSerial; r->event = sPreviewEvent;
    r->hd = sPreviewHd; r->fallback = sPreviewFallback;
    r->frame = PortFrameCount();
    /* Generation identity without recording a player's absolute media path. */
    r->generation = 2166136261u;
    for (const unsigned char* p = (const unsigned char*)sPreviewMediaPath; *p; ++p)
        r->generation = (r->generation ^ *p) * 16777619u;
}
static void PreviewTraceDraw(int accepted) {
    if (!PreviewTraceEnabled()) return;
    if (!accepted) { PreviewTrace("draw-rejected"); return; }
    if (sPreviewTraceDrawSerial != sPreviewSerial || sPreviewTraceDrawFallback != sPreviewFallback) {
        PreviewTrace("draw-submitted"); sPreviewTracePending = 1;
        sPreviewTraceDrawSerial = sPreviewSerial; sPreviewTraceDrawFallback = sPreviewFallback;
    }
}
void PortSceneGalleryPreviewPresented(int accepted) {
    if (!PreviewTraceEnabled() || !sPreviewTracePending) return;
    PreviewTrace(accepted ? "present-return" : "present-rejected");
    sPreviewTracePending = 0;
}
static void PreviewTraceFlush(void) {
    if (!PreviewTraceEnabled() || !sPreviewTraceCount) return;
    FILE* f = fopen(PortConfigPath("preview_trace.csv"), "w");
    if (!f) return;
    fprintf(f, "# frequency=%llu dropped=%u; CPUready=polled; bridge returns are submissions, not GPU completion\n",
        (unsigned long long)SDL_GetPerformanceFrequency(), sPreviewTraceDropped);
    fputs("counter,stage,serial,event,hd,fallback,generation,guest_frame\n", f);
    for (unsigned i = 0; i < sPreviewTraceCount; ++i) {
        PreviewTraceRecord* r = &sPreviewTrace[i];
        fprintf(f, "%llu,%s,%d,%d,%d,%d,%u,%u\n", (unsigned long long)r->counter,
            r->stage, r->serial, r->event, r->hd, r->fallback, r->generation, r->frame);
    }
    fclose(f);
}
#endif
