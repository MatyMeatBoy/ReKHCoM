#ifndef PORT_STARTUP_DEADLINE_H
#define PORT_STARTUP_DEADLINE_H

#include <SDL3/SDL.h>

/* Only the native bootstrap is timed. Enter and expiry compete for one atomic
 * state, so a deadline can never terminate a game that crossed the entry gate.
 * All video operations stay on the main thread. The callback must not call SDL
 * video, stdio, or other potentially locked driver services when terminating.
 * Begin requires a zero-initialized or previously joined object. */
typedef struct PortStartupDeadline {
    SDL_AtomicInt state; /* 0 active, 1 entered, 2 expired, 3 cancelled */
    SDL_Semaphore *wake;
    SDL_Thread *thread;
    Uint32 milliseconds;
    void (*expired)(void);
} PortStartupDeadline;

static int SDLCALL PortStartupDeadlineWait(void *opaque) {
    PortStartupDeadline *d = (PortStartupDeadline *)opaque;
    Uint64 began = SDL_GetTicks();
    for (;;) {
        Uint64 elapsed = SDL_GetTicks() - began;
        if (elapsed >= d->milliseconds) break;
        if (SDL_WaitSemaphoreTimeout(d->wake, (Sint32)(d->milliseconds - elapsed))) return 0;
    }
    if (SDL_CompareAndSwapAtomicInt(&d->state, 0, 2)) d->expired();
    return 0;
}

static inline bool PortStartupDeadlineBegin(PortStartupDeadline *d, Uint32 ms, void (*expired)(void)) {
    SDL_zero(*d);
    if (!ms || ms > 0x7fffffffu || !expired) return false;
    d->milliseconds = ms;
    d->expired = expired;
    d->wake = SDL_CreateSemaphore(0);
    if (!d->wake) return false;
    d->thread = SDL_CreateThread(PortStartupDeadlineWait, "native-startup", d);
    if (!d->thread) {
        SDL_DestroySemaphore(d->wake);
        d->wake = NULL;
        return false;
    }
    return true;
}

static inline void PortStartupDeadlineJoin(PortStartupDeadline *d) {
    if (d->thread) {
        SDL_SignalSemaphore(d->wake);
        SDL_WaitThread(d->thread, NULL);
        d->thread = NULL;
    }
    if (d->wake) SDL_DestroySemaphore(d->wake);
    d->wake = NULL;
}

static inline bool PortStartupDeadlineEnter(PortStartupDeadline *d) {
    bool entered = SDL_CompareAndSwapAtomicInt(&d->state, 0, 1);
    PortStartupDeadlineJoin(d);
    return entered;
}

static inline void PortStartupDeadlineCancel(PortStartupDeadline *d) {
    SDL_CompareAndSwapAtomicInt(&d->state, 0, 3);
    PortStartupDeadlineJoin(d);
}

#endif
