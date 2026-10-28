/* The image's other threads (bridge entry thread; the guest side is port/switch/guest/stubs_a64.c): the PPU's cores
 * (port_ppu.c, SWITCH_MOTOR.md). THREAD_HOST_LIBNX (the Switch) and THREAD_HOST_POSIX (the Linux AArch64 test host)
 * start them; any other host tells the image it has one core, and the image draws on its own thread.
 *
 * A thread enters the image through the guest's switch_thread(fn, arg, stackTop), which runs fn(arg) on a stack of
 * the image and comes back here when fn returns (the PPU's never do). */
#ifndef THREAD_HOST_H
#define THREAD_HOST_H
#include "audio_bridge.h"

#if defined(THREAD_HOST_LIBNX) || defined(THREAD_HOST_POSIX)
#define THREAD_HOST_MAX 2  /* with the game's thread, the three cores an application has on the Switch */
#define THREAD_HOST_SEMS 16

typedef int (*ThreadHostEntry)(uint32_t fn, uint32_t arg, uint32_t stackTop);
typedef struct {
    uint32_t entry, fn, arg, stackTop;
} ThreadHostStart;
static ThreadHostStart sThreadStarts[THREAD_HOST_MAX];
static int sThreadCount, sSemCount;

#ifdef THREAD_HOST_LIBNX
static Thread sThreads[THREAD_HOST_MAX];
static Semaphore sSems[THREAD_HOST_SEMS];
static void ThreadHostMain(void* p) {
    const ThreadHostStart* s = (const ThreadHostStart*)p;
    ((ThreadHostEntry)(uintptr_t)s->entry)(s->fn, s->arg, s->stackTop);
}
#else
#include <pthread.h>
#include <semaphore.h>
static sem_t sSems[THREAD_HOST_SEMS];
static void* ThreadHostMain(void* p) {
    const ThreadHostStart* s = (const ThreadHostStart*)p;
    ((ThreadHostEntry)(uintptr_t)s->entry)(s->fn, s->arg, s->stackTop);
    return NULL;
}
#endif

static int H_thread(int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
#ifdef THREAD_HOST_AUDIO_RATE
    if (op == BRIDGE_AUDIO_INPUT_RATE)
        return THREAD_HOST_AUDIO_RATE(a);
#endif
    switch (op) {
        case 0: { /* the cores: KHCOM_PPU_THREADS=1 keeps the image on its own thread */
            const char* env = getenv("KHCOM_PPU_THREADS");
            int n = env ? atoi(env) : 1 + THREAD_HOST_MAX;
            return n < 1 ? 1 : n > 1 + THREAD_HOST_MAX ? 1 + THREAD_HOST_MAX : n;
        }
        case 1: {
            ThreadHostStart* s;
            if (sThreadCount >= THREAD_HOST_MAX)
                return 0;
            s = &sThreadStarts[sThreadCount];
            s->entry = a;
            s->fn = b;
            s->arg = c;
            s->stackTop = d;
#ifdef THREAD_HOST_LIBNX
            /* cores 1 and 2 (the game's thread is on core 0), the game's priority */
            if (R_FAILED(threadCreate(&sThreads[sThreadCount], ThreadHostMain, s, NULL, 0x10000, 0x2C, 1 + sThreadCount)) ||
                R_FAILED(threadStart(&sThreads[sThreadCount]))) {
                Log("ppu: thread %d could not start", sThreadCount + 1);
                return 0;
            }
#else
            {
                pthread_t t;
                if (pthread_create(&t, NULL, ThreadHostMain, s))
                    return 0;
                pthread_detach(t);
            }
#endif
            sThreadCount++;
            return 1;
        }
        case 2:
            if (sSemCount >= THREAD_HOST_SEMS)
                return 0;
#ifdef THREAD_HOST_LIBNX
            semaphoreInit(&sSems[sSemCount], a);
#else
            sem_init(&sSems[sSemCount], 0, a);
#endif
            return ++sSemCount;
        case 3:
            if (a >= 1 && (int)a <= sSemCount) {
#ifdef THREAD_HOST_LIBNX
                semaphoreWait(&sSems[a - 1]);
#else
                while (sem_wait(&sSems[a - 1]) && errno == EINTR) {}
#endif
            }
            return 0;
        case 4:
            if (a >= 1 && (int)a <= sSemCount) {
#ifdef THREAD_HOST_LIBNX
                semaphoreSignal(&sSems[a - 1]);
#else
                sem_post(&sSems[a - 1]);
#endif
            }
            return 0;
    }
    return 0;
}
#else
static int H_thread(int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
#ifdef THREAD_HOST_AUDIO_RATE
    if (op == BRIDGE_AUDIO_INPUT_RATE)
        return THREAD_HOST_AUDIO_RATE(a);
#endif
    (void)a; (void)b; (void)c; (void)d;
    return op == 0; /* one core: the image draws on its own thread */
}
#endif

#endif
