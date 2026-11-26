#ifndef PORT_LOOP_PHASES_H
#define PORT_LOOP_PHASES_H

/* KHCOM_LOOP_PHASES=<ms>: where the main loop's time went, for the frames over that many milliseconds only.
   Each mark charges the time since the previous one to a phase, so the phases of a frame add up to its interval
   (end of one cycle to the end of the next). No allocation; the caller writes a line only for a slow frame.
   Off (the variable unset or not above 0) no clock is read. It measures, it changes nothing. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* logic: the game's code between two cycles; wait: asleep until the frame's deadline (late: how far past it the
   sleep returned); pace/sram/framelog: after the picture, the latter KHCOM_FRAMELOG's own cost; report: the
   previous slow frame's line. */
#define PORT_LOOP_PHASE_LIST(X) \
    X(LOGIC, "logic") X(SERVICES, "services") X(SETUP, "setup") X(PPU, "ppu") X(DMA, "dma") X(IRQ, "irq") \
    X(VBLINES, "vblines") X(AUDIO, "audio") X(COPYWAIT, "copywait") X(DRAW, "draw") X(FLIP, "flip") \
    X(PRESENT_END, "present_end") X(EVENTS, "events") X(UI, "ui") X(INPUT, "input") X(HARNESS, "harness") \
    X(ROLLBACK, "rollback") X(TIMING, "timing") X(PACE, "pace") X(WAIT, "wait") X(SRAM, "sram") \
    X(FRAMELOG, "framelog") X(REPORT, "report")

enum {
#define PORT_LOOP_ENUM(id, name) PORT_LOOP_##id,
    PORT_LOOP_PHASE_LIST(PORT_LOOP_ENUM)
#undef PORT_LOOP_ENUM
    PORT_LOOP_COUNT
};

typedef struct PortLoopPhases {
    double msPerTick;
    uint64_t threshold; /* ticks; 0: off */
    uint64_t start, last, late;
    uint64_t ticks[PORT_LOOP_COUNT];
} PortLoopPhases;

static inline void PortLoopPhasesInit(PortLoopPhases* p, const char* value, uint64_t frequency) {
    double ms = value ? atof(value) : 0;

    memset(p, 0, sizeof(*p));
    if (ms > 0 && ms < 1e9 && frequency) {
        p->msPerTick = 1000.0 / (double)frequency;
        p->threshold = (uint64_t)(ms / p->msPerTick);
        if (!p->threshold)
            p->threshold = 1;
    }
}

/* The first mark only sets the origin. */
static inline void PortLoopPhasesMark(PortLoopPhases* p, int phase, uint64_t now) {
    if (p->last)
        p->ticks[phase] += now - p->last;
    else
        p->start = now;
    p->last = now;
}

/* A wait measured inside a phase still running: counted apart, and left out of that phase. */
static inline void PortLoopPhasesWait(PortLoopPhases* p, uint64_t waited, uint64_t late) {
    if (!p->last)
        return;
    p->ticks[PORT_LOOP_WAIT] += waited;
    p->last += waited;
    p->late += late;
}

/* Closes the interval at the last mark. Returns 1 and fills line when it was over the threshold. */
static inline int PortLoopPhasesEnd(PortLoopPhases* p, char* line, size_t size) {
    uint64_t interval = p->last - p->start;
    int slow = p->threshold && interval > p->threshold;

    if (slow && size) {
        static const char* const names[PORT_LOOP_COUNT] = {
#define PORT_LOOP_NAME(id, name) name,
            PORT_LOOP_PHASE_LIST(PORT_LOOP_NAME)
#undef PORT_LOOP_NAME
        };
        size_t used;
        int i, n = snprintf(line, size, "%.2f ms (late %.2f):", interval * p->msPerTick, p->late * p->msPerTick);

        used = n < 0 ? 0 : (size_t)n;
        for (i = 0; i < PORT_LOOP_COUNT && used < size; i++) {
            n = snprintf(line + used, size - used, " %s %.2f", names[i], p->ticks[i] * p->msPerTick);
            used += n < 0 ? 0 : (size_t)n;
        }
    }
    p->start = p->last;
    p->late = 0;
    memset(p->ticks, 0, sizeof(p->ticks));
    return slow;
}

#endif
