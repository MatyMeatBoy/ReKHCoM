"""Exercise the production Android decoder choice with a fake MediaCodec."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "port/portable/hdmovie_android.c"
PROLOGUE = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

typedef int AMediaCodec;
typedef int AMediaFormat;
typedef int ANativeWindow;
#define AMEDIA_OK 0
#define SDL_strcmp strcmp

enum { C2, OMX, AUTO, FORCED };
enum { READY, CREATE_FAIL, CONFIGURE_FAIL, START_FAIL };
static const char* override;
static int policy[4], codec[4] = {C2, OMX, AUTO, FORCED};
static int calls[4], attempts, deleted[4];

static AMediaCodec *Create(int id) {
    assert(attempts < 4);
    calls[attempts++] = id;
    return policy[id] == CREATE_FAIL ? NULL : &codec[id];
}

static AMediaCodec *ByName(const char *name) {
    if (strcmp(name, "c2.android.avc.decoder") == 0) return Create(C2);
    if (strcmp(name, "OMX.google.h264.decoder") == 0) return Create(OMX);
    assert(override && strcmp(name, override) == 0);
    return Create(FORCED);
}

static AMediaCodec *ByType(const char *mime) {
    assert(mime && mime[0]);
    return Create(AUTO);
}

static int Configure(AMediaCodec *c, const AMediaFormat *format, ANativeWindow *window,
                     void *crypto, unsigned flags) {
    assert(format && !crypto && !flags);
    (void)window;
    return policy[*c] == CONFIGURE_FAIL ? -1 : AMEDIA_OK;
}
static int Start(AMediaCodec *c) {
    return policy[*c] == START_FAIL ? -1 : AMEDIA_OK;
}
static void Delete(AMediaCodec *c) { deleted[*c]++; }
static const char *SDL_getenv(const char *key) {
    assert(strcmp(key, "KHCOM_HDMOVIE_DECODER") == 0);
    return override;
}
static void HdMovieLog(const char *format, ...) { (void)format; }

static struct {
    AMediaCodec *(*codecByName)(const char *);
    AMediaCodec *(*codecNew)(const char *);
    int (*codecConfigure)(AMediaCodec *, const AMediaFormat *, ANativeWindow *, void *, unsigned);
    int (*codecStart)(AMediaCodec *);
    void (*codecDelete)(AMediaCodec *);
} M = {ByName, ByType, Configure, Start, Delete};
"""
MAIN = r"""
static void Clear(void) {
    override = NULL;
    attempts = 0;
    memset(policy, 0, sizeof(policy));
    memset(deleted, 0, sizeof(deleted));
}

int main(void) {
    AMediaFormat format = 0;
    ANativeWindow videoWindow = 0;
    Clear();
    assert(OpenDecoder("video/avc", &format, &videoWindow, false) == &codec[AUTO]);
    assert(attempts == 1 && calls[0] == AUTO);

    Clear();
    assert(OpenDecoder("video/avc", &format, &videoWindow, true) == &codec[C2]);
    assert(attempts == 1 && calls[0] == C2 && deleted[C2] == 0);

    for (int failure = CREATE_FAIL; failure <= START_FAIL; failure++) {
        Clear();
        policy[C2] = failure;
        assert(OpenDecoder("video/avc", &format, &videoWindow, true) == &codec[OMX]);
        assert(attempts == 2 && calls[0] == C2 && calls[1] == OMX);
        assert(deleted[C2] == (failure != CREATE_FAIL) && deleted[OMX] == 0);
    }

    Clear();
    policy[C2] = CONFIGURE_FAIL;
    policy[OMX] = START_FAIL;
    assert(OpenDecoder("video/avc", &format, &videoWindow, true) == &codec[AUTO]);
    assert(attempts == 3 && calls[0] == C2 && calls[1] == OMX && calls[2] == AUTO);
    assert(deleted[C2] == 1 && deleted[OMX] == 1 && deleted[AUTO] == 0);

    Clear();
    policy[C2] = policy[OMX] = CREATE_FAIL;
    policy[AUTO] = START_FAIL;
    assert(OpenDecoder("video/avc", &format, &videoWindow, true) == NULL);
    assert(attempts == 3 && deleted[AUTO] == 1);

    Clear();
    assert(OpenDecoder("video/hevc", &format, &videoWindow, true) == &codec[AUTO]);
    assert(attempts == 1 && calls[0] == AUTO);

    Clear();
    override = "test.codec";
    assert(OpenDecoder("video/avc", &format, &videoWindow, true) == &codec[FORCED]);
    assert(attempts == 1 && calls[0] == FORCED);
    policy[FORCED] = START_FAIL;
    attempts = 0;
    assert(OpenDecoder("video/avc", &format, &videoWindow, true) == NULL);
    assert(attempts == 1 && calls[0] == FORCED && deleted[FORCED] == 1);

    Clear();
    override = "test.codec";
    assert(OpenDecoder("audio/mp4a-latm", &format, NULL, true) == &codec[AUTO]);
    assert(attempts == 1 && calls[0] == AUTO);
    return 0;
}
"""


class AndroidMovieDecoderTest(unittest.TestCase):
    def test_codec_choice_and_cleanup(self):
        source = SOURCE.read_text(encoding="utf-8")
        start = source.find("static AMediaCodec* StartDecoder(")
        self.assertNotEqual(start, -1, "Missing Android decoder selection")
        stop = source.index("/* the first track", start)
        if os.name == "nt":
            toolchain = next(parent / "toolchain" for parent in ROOT.parents
                             if list((parent / "toolchain").glob("llvm-mingw-*")))
            binary = sorted(toolchain.glob("llvm-mingw-*"))[-1] / "bin/clang.exe"
        else:
            binary = os.environ.get("CC", "cc")
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "decoder.c"
            exe = Path(tmp) / "decoder.exe"
            program.write_text(PROLOGUE + source[start:stop] + MAIN, encoding="utf-8")
            subprocess.run([str(binary), "-std=c11", "-Wall", "-Wextra", "-Werror",
                            str(program), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
