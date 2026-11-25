#!/usr/bin/env python3
"""Host check of the audio queue control (port_audio_rate.h): default identical, refill rebuilds the level."""
from pathlib import Path
import subprocess
r = Path(__file__).resolve().parents[2]
cc = next((r.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'),
          next(r.parents[2].glob('toolchain/llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'), None))
out = r / 'build/audio-tests'
out.mkdir(parents=True, exist_ok=True)
(out / 'refill.c').write_text(r"""
#include <assert.h>
#include <stdio.h>
#include "port/port_audio_rate.h"
static float Reference(float avg, float target) {
    if (avg < target * 0.5f) return 0.9985f;
    if (avg > target * 3.0f) return 1.0015f;
    return 1.0f;
}
/* 63072 Hz in, 1056 samples a frame, the device taking 1024 frames at 48000 Hz;
   starts at the level the GA36 log stayed at after its late frames. */
static float Run(int refill, int frames, int* changes) {
    float target = 63072.0f / 59.7275f * 4.0f, queued = 1100, avg = -1, last = 1;
    double chunk = 1024 * 63072.0 / 48000, due = 0;
    int state = 0;
    *changes = 0;
    for (int i = 0; i < frames; i++) {
        queued += 63072.0f / 59.7275f;
        if (avg < 0) avg = queued;
        avg += (queued - avg) * 0.02f;
        float ratio = PortAudioQueueRatio(avg, target, refill, &state);
        if (ratio != last) (*changes)++;
        last = ratio;
        due += 63072.0 * ratio / 59.7275;
        while (due >= chunk) { due -= chunk; queued -= (float)chunk; }
        assert(queued > 0 && queued < target * 3);
    }
    return avg;
}
int main(void) {
    float target = 4224;
    for (int t = 1; t < 12000; t++) for (int v = 0; v < 40000; v += 7) {
        int state = 0;
        if (t % 211) break;
        assert(PortAudioQueueRatio((float)v, (float)t, 0, &state) == Reference((float)v, (float)t) && !state);
    }
    for (int v = 0; v < 30000; v++) {
        int state = 0;
        assert(PortAudioQueueRatio((float)v, target, 0, &state) == Reference((float)v, target) && !state);
    }
    int s = 0;
    assert(PortAudioQueueRatio(target * .70f, target, 1, &s) == 1.0f && !s);
    assert(PortAudioQueueRatio(target * .70f - 1, target, 1, &s) == .9970f && s);
    assert(PortAudioQueueRatio(target * .84f, target, 1, &s) == .9970f && s);
    assert(PortAudioQueueRatio(target * .85f, target, 1, &s) == 1.0f && !s);
    assert(PortAudioQueueRatio(target * .84f, target, 1, &s) == 1.0f && !s);
    assert(PortAudioQueueRatio(target * 3 + 1, target, 1, &s) == 1.0015f && !s);
    int offChanges, onChanges;
    float off = Run(0, 3600, &offChanges), on = Run(1, 3600, &onChanges);
    printf("after 60 s: off avg %.0f, on avg %.0f (target %.0f), ratio changes %d/%d\n", off, on, target,
           offChanges, onChanges);
    assert(offChanges == 0 && off < target * .70f);
    assert(on > target * .75f && onChanges <= 2);
    return 0;
}
""")
exe = out / 'refill.exe'
subprocess.run([str(cc), '-O2', '-std=gnu11', '-I' + str(r / 'port/include'), str(out / 'refill.c'), '-o', str(exe)], check=True)
subprocess.run([str(exe)], check=True)
print('PASS audio refill')
