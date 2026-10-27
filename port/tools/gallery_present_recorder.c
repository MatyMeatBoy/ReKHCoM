/* Instrumented QA only: bounded 32-frame ring, read back before EVERY present.
 * Readback perturbs timing. These captures must never be used for performance.
 * The forced include is applied to every production TU calling RenderPresent.
 */
#define GALLERY_QA_IMPLEMENTATION
#include "gallery_present_recorder.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
extern int PortSceneGalleryActive(void);
extern int PortUiActive(void);
extern int PortUiMenuCanvas(void);
extern int PortUiSceneFade(void);
extern void GalleryQaSelection(int values[8]);
enum { RING = 32, AFTER = 8 };
typedef struct { SDL_Surface* image; unsigned id; } Frame;
static Frame ring[RING];
static unsigned serial;
static int previous, tail, returned, initialized;
static FILE* trace;

static void Save(Frame* frame, const char* label) {
    char path[128];
    if (!frame->image) return;
    SDL_snprintf(path, sizeof(path), "qa-%s-%06u.bmp", label, frame->id);
    if (!SDL_SaveBMP(frame->image, path)) {
        fprintf(stderr, "QA capture failed: %s\n", SDL_GetError());
        exit(91);
    }
}
static void Cleanup(void) {
    unsigned i;
    for (i = 0; i < RING; ++i) SDL_DestroySurface(ring[i].image);
    if (trace) fclose(trace);
}
bool GalleryQaPresent(SDL_Renderer* renderer, const char* source, int line) {
    int state[8];
    int active = PortSceneGalleryActive();
    Frame* frame = &ring[serial % RING];
    if (!initialized) {
        initialized = 1;
        trace = fopen("qa-frames.csv", "w");
        if (!trace) exit(90);
        fprintf(trace, "present,caller,line,gallery,ui,canvas,fade,renderer,readback,choice,scroll,tab,character,event,tick,pause,step\n");
        atexit(Cleanup);
    }
    /* Save the list immediately before entering any replay. */
    if (active && !previous && serial) Save(&ring[(serial - 1) % RING], "before");
    SDL_DestroySurface(frame->image);
    frame->image = returned && !tail ? NULL : SDL_RenderReadPixels(renderer, NULL);
    frame->id = serial;
    GalleryQaSelection(state);
    fprintf(trace, "%u,%s,%d,%d,%d,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", serial, source, line,
            active, PortUiActive(), PortUiMenuCanvas(), PortUiSceneFade(),
            SDL_GetRendererName(renderer), frame->image != NULL,
            state[0],state[1],state[2],state[3],state[4],state[5],state[6],state[7]);
    fflush(trace);
    if ((!returned || tail) && !frame->image) {
        fprintf(stderr, "QA readback failed: %s\n", SDL_GetError());
        exit(92);
    }
    if (previous && !active && !returned) {
        unsigned first = serial >= RING - 1 ? serial - RING + 1 : 0, i;
        for (i = first; i <= serial; ++i) Save(&ring[i % RING], "return");
        returned = 1;
        tail = AFTER;
    } else if (tail) {
        Save(frame, "return");
        --tail;
    }
    previous = active;
    ++serial;
    return SDL_RenderPresent(renderer);
}
