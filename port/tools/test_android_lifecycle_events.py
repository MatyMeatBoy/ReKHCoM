"""Verify Android app events from the SDL3 producer reach the game's poll queue."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = r"""
#include <SDL3/SDL.h>
#include <assert.h>

#include "android_lifecycle_events.h"

static unsigned observed;
static bool SDLCALL CountBackgroundEvents(void* userdata, SDL_Event* event) {
    (void)userdata;
    if (event->type == SDL_EVENT_DID_ENTER_BACKGROUND || event->type == SDL_EVENT_DID_ENTER_FOREGROUND)
        ++observed;
    return true;
}

static SDL_Gamepad* sPad;
static int sQuit, sBackground, sBackgroundPending, sFingerCount;
static void RendererDeviceReset(void) {}
static void HostTextQueue(const char* text) { (void)text; }
static void TouchEvent(const SDL_TouchFingerEvent* event, int down) { (void)event; (void)down; }

HOST_FUNCTIONS

int main(void) {
    assert(SDL_GetVersion() == SDL_VERSIONNUM(3, 4, 18));
    assert(SDL_Init(SDL_INIT_EVENTS));
    assert(SDL_AddEventWatch(CountBackgroundEvents, NULL));

    /* SDL sends both events through watchers, but leaves its queue empty. */
    SDL_Event event;
    SDL_OnApplicationDidEnterBackground();
    SDL_OnApplicationDidEnterForeground();
    assert(observed == 2);
    assert(!SDL_PollEvent(&event));

    assert(PortAndroidLifecycleStart());
    SDL_OnApplicationDidEnterForeground(); /* startup notification: already foreground */
    SDL_OnApplicationWillEnterBackground();
    SDL_OnApplicationDidEnterBackground();
    SDL_OnApplicationDidEnterBackground(); /* duplicate */
    SDL_OnApplicationWillEnterForeground();
    SDL_OnApplicationDidEnterForeground();
    SDL_OnApplicationDidEnterForeground(); /* duplicate */
    assert(observed == 7); /* enqueued copies must not reenter watchers */

    assert(SDL_PollEvent(&event) && event.type == SDL_EVENT_DID_ENTER_BACKGROUND);
    assert(SDL_PollEvent(&event) && event.type == SDL_EVENT_DID_ENTER_FOREGROUND);
    assert(!SDL_PollEvent(&event));

    /* One host drain sees both transitions. Deliver the pause to the guest
       first, clear old touch state and then expose the current foreground. */
    sFingerCount = 1;
    SDL_OnApplicationDidEnterBackground();
    SDL_OnApplicationDidEnterForeground();
    Pump();
    assert(!sFingerCount);
    assert(H_app_state() == 1);
    assert(H_app_state() == 0);
    assert(H_app_state() == 0);

    PortAndroidLifecycleStop();
    SDL_OnApplicationDidEnterBackground();
    assert(observed == 10);
    assert(!SDL_PollEvent(&event));

    event.type = SDL_EVENT_QUIT;
    assert(SDL_PushEvent(&event));
    Pump();
    assert(sQuit);

    SDL_RemoveEventWatch(CountBackgroundEvents, NULL);
    SDL_Quit();
    return 0;
}
"""


def main():
    if os.name == "nt":
        toolchain = next(p / "toolchain" for p in ROOT.parents
                         if (p / "toolchain/SDL3-3.4.18").is_dir())
        sdl = toolchain / "SDL3-3.4.18/x86_64-w64-mingw32"
        compiler = toolchain / "llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe"
        flags = ["-I" + str(sdl / "include"), "-L" + str(sdl / "lib"), "-lSDL3"]
        env = dict(os.environ, PATH=str(sdl / "bin") + os.pathsep + os.environ["PATH"])
    else:
        compiler = os.environ.get("CC", "cc")
        flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", "sdl3"], text=True).split()
        env = None
    with tempfile.TemporaryDirectory() as tmp:
        host = (ROOT / "port/android64/host/android64_host.c").read_text(encoding="utf-8")
        functions = []
        for name in ("static void Pump(void)", "static int H_app_state(void)"):
            start = host.index(name + " {")
            functions.append(host[start:host.index("\n}", start) + 2])
        code = Path(tmp) / "lifecycle.c"
        exe = Path(tmp) / "lifecycle.exe"
        code.write_text(SOURCE.replace("HOST_FUNCTIONS", "\n\n".join(functions)), encoding="utf-8")
        subprocess.run([str(compiler), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-I" + str(ROOT / "port/android/native"), str(code), *flags,
                        "-o", str(exe)], check=True)
        subprocess.run([str(exe)], env=env, check=True)
    print("SDL 3.4.18 lifecycle routing, deduplication, host state and removal PASS")


if __name__ == "__main__":
    main()
