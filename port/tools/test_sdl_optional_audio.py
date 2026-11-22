"""Exercise required startup and optional audio against real SDL."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = r"""
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "port/sdl_startup.h"

static int warnings;
static char warning[512];
static void Log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SDL_vsnprintf(warning, sizeof(warning), fmt, args);
    va_end(args);
    warnings++;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const SDL_InitFlags required = SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS;
    if (strcmp(argv[1], "bad-video") == 0) {
        assert(!PortSdlInit(required, Log));
        assert(warnings == 0 && SDL_WasInit(SDL_INIT_AUDIO) == 0);
        assert(strstr(SDL_GetError(), "rekhcom-unavailable"));
        SDL_Quit();
        return 0;
    }

    const bool audioExpected = strcmp(argv[1], "dummy") == 0;
    assert(PortSdlInit(required, Log));
    assert((SDL_WasInit(required) & required) == required);
    assert((SDL_WasInit(SDL_INIT_AUDIO) != 0) == audioExpected);
    assert(warnings == !audioExpected);
    if (!audioExpected) {
        assert(strstr(warning, "rekhcom-unavailable"));
        assert(!SDL_GetError()[0]);
    }

    SDL_Window* window = SDL_CreateWindow("startup probe", 32, 32, SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, "software");
    assert(renderer && SDL_RenderClear(renderer) && SDL_RenderPresent(renderer));
    SDL_Event event = {0};
    event.type = SDL_RegisterEvents(1);
    assert(event.type != 0 && SDL_PushEvent(&event));
    assert(SDL_PeepEvents(&event, 1, SDL_GETEVENT, event.type, event.type) == 1);

    SDL_AudioSpec spec = { SDL_AUDIO_F32, 2, 48000 };
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (audioExpected) {
        float samples[64] = {0};
        assert(stream);
        assert(SDL_PutAudioStreamData(stream, samples, sizeof(samples)));
        assert(SDL_GetAudioStreamQueued(stream) >= (int)sizeof(samples));
        SDL_DestroyAudioStream(stream);
    } else {
        assert(!stream);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
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
        env = dict(os.environ)
    with tempfile.TemporaryDirectory() as tmp:
        code = Path(tmp) / "sdl_startup.c"
        exe = Path(tmp) / "sdl_startup.exe"
        code.write_text(SOURCE, encoding="utf-8")
        subprocess.run([str(compiler), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-I" + str(ROOT / "port/include"), str(code), *flags,
                        "-o", str(exe)], check=True)
        for label, video, audio in (
            ("missing-audio", "dummy", "rekhcom-unavailable"),
            ("dummy", "dummy", "dummy"),
            ("bad-video", "rekhcom-unavailable", "dummy"),
        ):
            subprocess.run([str(exe), label], check=True, timeout=15,
                           env=dict(env, SDL_VIDEO_DRIVER=video, SDL_AUDIO_DRIVER=audio))
    print("SDL startup with missing audio, working audio, and mandatory video failure PASS")


if __name__ == "__main__":
    main()
