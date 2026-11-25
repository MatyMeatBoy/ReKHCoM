#!/usr/bin/env python3
"""Host check of KHCOM_AUDIO_RESCUE_SKIP (port_audio_rate.h) on a model of the pacer and the audio queue.

    python port/tools/test_audio_rescue.py [frames.csv]

Without an argument: synthetic stretches of late frames. With a KHCOM_FRAMELOG capture (rescue and frame skip off)
its measured costs are replayed; a model, not a device measurement."""
from pathlib import Path
import csv
import subprocess
import sys
r = Path(__file__).resolve().parents[2]
cc = next((r.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'),
          next(r.parents[2].glob('toolchain/llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'), None))
out = r / 'build/audio-tests'
out.mkdir(parents=True, exist_ok=True)
(out / 'rescue.c').write_text(r"""
#include <assert.h>
#include <stdio.h>
#include "port/port_audio_rate.h"
#define PERIOD_US 16743
#define SPF 1056.0
#define CHUNK (1024 * 63072.0 / 48000)
typedef struct { double busy, drawn, tail; } Cost; /* us: to the deadline wait, its PPU + present, after it */
typedef struct { int frames, skips, underruns, pairs, first, last; double seconds, low, end; } Result;
static Cost cost[40000];
/* rescue: threshold in frames, 0 off */
static Result Run(int n, int rescue) {
    Result res = {0};
    double t = 0, next = 0, queued = 5 * SPF, avg = -1, last = 0, due = 0, ratio = 1;
    int refilling = 0, skippedLast = 0;
    res.low = 1e9;
    for (int i = 0; i < n; i++) {
        double late = next && t > next ? t - next : 0;
        int skip = 0, prev = skippedLast;
        if (rescue && next)
            skip = PortAudioRescueDue(queued < SPF * rescue, (uint64_t)late, PERIOD_US, &skippedLast);
        if (skip) {
            res.pairs += prev;
            if (!res.skips++) res.first = i;
            res.last = i;
        }
        double busy = cost[i].busy - (skip ? cost[i].drawn : 0);
        double put = t + (skip ? 0 : cost[i].drawn * 0.5); /* the sound is queued between PPU and present */
        for (int step = 0; step < 2; step++) { /* the device drains up to the put, then up to the next cycle */
            double to = put;
            if (step) {
                double ready = t + busy;
                if (!next) next = ready;
                else if (ready > next + PERIOD_US * 4) next = ready - PERIOD_US;
                next += PERIOD_US;
                to = (ready > next ? ready : next) + cost[i].tail;
            }
            due += (to - last) * 1e-6 * 63072.0 * ratio;
            last = to;
            while (due >= CHUNK) {
                due -= CHUNK;
                if (queued < CHUNK) { res.underruns++; queued = 0; }
                else queued -= CHUNK;
            }
            if (!step) {
                if (queued < res.low) res.low = queued;
                queued += SPF;
                if (avg < 0) avg = queued;
                avg += (queued - avg) * 0.02;
                ratio = PortAudioQueueRatio((float)avg, (float)(SPF * 6), 1, &refilling);
            }
        }
        t = last;
        res.frames++;
    }
    res.seconds = t * 1e-6;
    res.end = queued;
    return res;
}
static void Print(const char* name, Result v) {
    printf("%s: %d frames %.2f s (%.3f/s), skipped %d (frames %d-%d), model underruns %d, low %.0f, end %.0f\n", name,
           v.frames, v.seconds, v.frames / v.seconds, v.skips, v.first, v.last, v.underruns, v.low, v.end);
    fflush(stdout);
}
int main(int argc, char** argv) {
    (void)argv;
    assert(!PortAudioRescueFrames(0) && !PortAudioRescueFrames("") && !PortAudioRescueFrames("0"));
    assert(!PortAudioRescueFrames("x") && !PortAudioRescueFrames("-3"));
    assert(PortAudioRescueFrames("1") == 4 && PortAudioRescueFrames("2") == 2 && PortAudioRescueFrames("4") == 4);
    assert(PortAudioRescueFrames("8") == 8 && PortAudioRescueFrames("99") == 8);
    int s = 0;
    assert(!PortAudioRescueDue(0, 999999, PERIOD_US, &s) && !s);
    assert(!PortAudioRescueDue(1, PERIOD_US / 4, PERIOD_US, &s) && !s);
    assert(PortAudioRescueDue(1, PERIOD_US / 4 + 1, PERIOD_US, &s) && s);
    assert(!PortAudioRescueDue(1, 999999, PERIOD_US, &s) && !s); /* never two in a row */
    assert(PortAudioRescueDue(1, 999999, PERIOD_US, &s) && s);
    int n = 0;
    if (argc > 1) {
        while (n < 40000 && scanf("%lf %lf %lf", &cost[n].busy, &cost[n].drawn, &cost[n].tail) == 3) n++;
        Print("off", Run(n, 0));
        for (int f = 2; f <= 6; f++) {
            char name[16];
            sprintf(name, "rescue %d", f);
            Result v = Run(n, f);
            assert(!v.pairs);
            Print(name, v);
        }
        return 0;
    }
    /* on time, 35 s of frames slightly over budget (the GA36 stretch), on time again */
    for (n = 0; n < 6000; n++) {
        int slow = n >= 600 && n < 2700;
        cost[n].busy = slow ? 16900 + (n % 7) * 40 : 12200;
        cost[n].drawn = slow ? 14900 : 10500;
        cost[n].tail = 170;
    }
    Result off = Run(n, 0), on = Run(n, 4);
    Print("off", off);
    Print("rescue 4", on);
    assert(!off.skips && off.underruns > 10);
    assert(!on.underruns);
    assert(on.skips > 0 && on.skips < 2100 / 10 && !on.pairs && on.first >= 600 && on.last < 2700);
    assert(on.seconds <= off.seconds && on.frames == off.frames);
    /* every frame late by far: still every other frame drawn */
    for (n = 0; n < 600; n++) cost[n] = (Cost){40000, 15000, 170};
    on = Run(n, 4);
    assert(on.skips <= n / 2 && !on.pairs);
    /* on time: never acts, whatever the threshold */
    for (n = 0; n < 600; n++) cost[n] = (Cost){12200, 10500, 170};
    assert(!Run(n, 8).skips);
    return 0;
}
""")
exe = out / 'rescue.exe'
subprocess.run([str(cc), '-O2', '-std=gnu11', '-I' + str(r / 'port/include'), str(out / 'rescue.c'), '-o', str(exe)], check=True)
subprocess.run([str(exe)], check=True)
if len(sys.argv) > 1:
    rows = list(csv.DictReader(open(sys.argv[1], newline='', encoding='utf-8-sig')))
    data = ''.join('%f %f %f\n' % (float(x['render_work_ms']) * 1000,
                                   (float(x['ppu_ms']) + float(x['present_ms'])) * 1000,
                                   (float(x['logic_ms']) + 0.17) * 1000) for x in rows)
    subprocess.run([str(exe), 'replay'], input=data, text=True, check=True)
print('PASS audio rescue')
