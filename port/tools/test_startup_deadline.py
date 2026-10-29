"""Real SDL threads: a pre-guest timeout must never kill an entered game."""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PROBE = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
static int failSemaphore, failThread;
static SDL_Semaphore *ProbeSemaphore(Uint32 n) {
    return failSemaphore ? NULL : SDL_CreateSemaphore(n);
}
static SDL_Thread *ProbeThread(SDL_ThreadFunction f, const char *n, void *p) {
    return failThread ? NULL : SDL_CreateThread(f, n, p);
}
#define SDL_CreateSemaphore ProbeSemaphore
#undef SDL_CreateThread
#define SDL_CreateThread ProbeThread
#include "port/startup_deadline.h"
#undef SDL_CreateSemaphore
#undef SDL_CreateThread
static void expired(void) { fputs("TIMEOUT_BEFORE_GUEST\n", stderr); fflush(stderr); _Exit(75); }
static SDL_AtomicInt expiredCount;
static void countedExpiry(void) { SDL_AddAtomicInt(&expiredCount, 1); }
int main(int argc, char **argv) {
    PortStartupDeadline d = {0};
    const char *mode = argc > 1 ? argv[1] : "success";
    if (!strcmp(mode, "semaphore-fail")) failSemaphore = 1;
    if (!strcmp(mode, "thread-fail")) failThread = 1;
    if (failSemaphore || failThread) {
        assert(!PortStartupDeadlineBegin(&d, 20, expired));
        assert(!d.thread && !d.wake);
        return 0;
    }
    if (!strcmp(mode, "race")) {
        for (int i = 0; i < 100; i++) {
            SDL_SetAtomicInt(&expiredCount, 0);
            assert(PortStartupDeadlineBegin(&d, 1, countedExpiry));
            if (i & 1) SDL_Delay(3);
            bool entered = PortStartupDeadlineEnter(&d);
            assert(SDL_GetAtomicInt(&expiredCount) == !entered);
            assert(SDL_GetAtomicInt(&d.state) == (entered ? 1 : 2));
        }
        SDL_Delay(120);
        puts("ENTERED_GAME_SURVIVED"); return 0;
    }
    assert(PortStartupDeadlineBegin(&d, 40, expired));
    if (!strcmp(mode, "stall")) { SDL_Delay(1000); return 99; }
    if (!strcmp(mode, "cancel")) PortStartupDeadlineCancel(&d);
    else assert(PortStartupDeadlineEnter(&d));
    assert(!d.thread && !d.wake);
    SDL_Delay(100);
    puts("ENTERED_GAME_SURVIVED");
    return 0;
}
'''


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=Path, required=True)
    args = ap.parse_args()
    out = args.work.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out / 'probe.c').write_text(PROBE, encoding='utf8', newline='\n')
    tc = ROOT.parent / 'toolchain'
    sdl = tc / 'SDL3-3.4.18/x86_64-w64-mingw32'
    cc = tc / 'llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe'
    subprocess.run([str(cc), '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'port/include'), '-I' + str(sdl / 'include'),
                    str(out / 'probe.c'), '-L' + str(sdl / 'lib'), '-lSDL3',
                    '-o', str(out / 'probe.exe')], check=True)
    shutil.copyfile(sdl / 'bin/SDL3.dll', out / 'SDL3.dll')
    rows = []
    for mode, expected in [('success', 0), ('stall', 75), ('cancel', 0),
                           ('race', 0), ('semaphore-fail', 0), ('thread-fail', 0)]:
        p = subprocess.run([str(out / 'probe.exe'), mode], capture_output=True,
                           text=True, timeout=5, env=os.environ)
        assert p.returncode == expected, (mode, p.returncode, p.stdout, p.stderr)
        assert ('TIMEOUT_BEFORE_GUEST' in p.stderr) == (mode == 'stall')
        rows.append(dict(case=mode, exit=p.returncode, stdout=p.stdout, stderr=p.stderr))
    (out / 'RESULT.json').write_text(json.dumps(dict(result='PASS', cases=rows,
        scope='Real SDL semaphore/thread/atomics on Windows64. No physical SteamOS claim.'), indent=2),
        encoding='utf8', newline='\n')
    print('PASS: pre-guest stall exits75; entered/cancelled game survives; allocation failures clean')


if __name__ == '__main__':
    main()
