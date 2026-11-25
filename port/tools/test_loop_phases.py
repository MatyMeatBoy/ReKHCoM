#!/usr/bin/env python3
"""Host check of KHCOM_LOOP_PHASES (port/loop_phases.h): off by default, phases add up, only slow frames report."""
from pathlib import Path
import subprocess

r = Path(__file__).resolve().parents[2]
cc = next((r.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'),
          next(r.parents[2].glob('toolchain/llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'), None))
out = r / 'build/loop-phase-tests'
out.mkdir(parents=True, exist_ok=True)
(out / 'loop.c').write_text(r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "port/loop_phases.h"
#define MARK(phase, at) PortLoopPhasesMark(&p, PORT_LOOP_##phase, (at))
static double Sum(const PortLoopPhases* p) {
    uint64_t s = 0;
    for (int i = 0; i < PORT_LOOP_COUNT; i++) s += p->ticks[i];
    return (double)s;
}
int main(void) {
    PortLoopPhases p;
    char line[640], small[24];
    const char* off[] = { 0, "", "0", "-5", "x", "nan", "1e30" };
    for (int i = 0; i < 7; i++) {
        PortLoopPhasesInit(&p, off[i], 1000000);
        assert(!p.threshold);
    }
    PortLoopPhasesInit(&p, "20", 0);
    assert(!p.threshold);
    PortLoopPhasesInit(&p, "20", 1000000); /* microsecond ticks */
    assert(p.threshold == 20000);
    PortLoopPhasesWait(&p, 50, 5); /* before the first mark: ignored */
    assert(!p.ticks[PORT_LOOP_WAIT] && !p.late);
    MARK(FRAMELOG, 1000);
    assert(!PortLoopPhasesEnd(&p, line, sizeof(line)));

    /* exactly the threshold: not slow */
    MARK(LOGIC, 1200); MARK(PPU, 21000);
    assert(Sum(&p) == 20000);
    line[0] = 0;
    assert(!PortLoopPhasesEnd(&p, line, sizeof(line)) && !line[0] && Sum(&p) == 0);

    /* a slow frame with a wait inside the draw phase */
    MARK(LOGIC, 21300); MARK(PPU, 27800); MARK(COPYWAIT, 27900);
    PortLoopPhasesWait(&p, 4000, 700); /* taken between 29900 and 33900 */
    MARK(DRAW, 34000); MARK(FLIP, 37000); MARK(AUDIO, 43500); MARK(SRAM, 43500);
    assert(p.ticks[PORT_LOOP_DRAW] == 2100 && p.ticks[PORT_LOOP_WAIT] == 4000);
    assert(Sum(&p) == 43500 - 21000 && p.last - p.start == 43500 - 21000);
    assert(PortLoopPhasesEnd(&p, line, sizeof(line)));
    puts(line);
    assert(strstr(line, "22.50 ms (late 0.70):") == line);
    assert(strstr(line, " logic 0.30 ") && strstr(line, " ppu 6.50 ") && strstr(line, " draw 2.10 "));
    assert(strstr(line, " flip 3.00 ") && strstr(line, " audio 6.50 ") && strstr(line, " wait 4.00 "));
    assert(strstr(line, " report 0.00"));
    assert(!p.late && Sum(&p) == 0 && p.start == 43500);

    /* the line's own cost belongs to the next frame; a short buffer stays terminated */
    MARK(REPORT, 44000); MARK(LOGIC, 70000);
    assert(p.ticks[PORT_LOOP_REPORT] == 500);
    memset(small, 'x', sizeof(small));
    assert(PortLoopPhasesEnd(&p, small, sizeof(small)) && strlen(small) == sizeof(small) - 1);

    /* every phase at an absurd length still fits the game's buffer */
    uint64_t t = p.last;
    for (int i = 0; i < PORT_LOOP_COUNT; i++) PortLoopPhasesMark(&p, i, t += 99999000);
    assert(PortLoopPhasesEnd(&p, line, sizeof(line)) && strstr(line, " report 99999.00"));
    return 0;
}
""")
exe = out / 'loop.exe'
subprocess.run([str(cc), '-O2', '-std=gnu11', '-Wall', '-Werror', '-I' + str(r / 'port/include'), str(out / 'loop.c'),
                '-o', str(exe)], check=True)
subprocess.run([str(exe)], check=True)
print('PASS loop phases')
