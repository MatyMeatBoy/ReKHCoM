"""Fault-inject the Android loader's image write loop without mapping memory."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PROLOGUE = r"""
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#undef SSIZE_MAX
#define SSIZE_MAX 7 /* Exercise the request bound with a small fixture. */

static const uint8_t image[] = "abcdefghijklmnopqrs";
static uint8_t written[sizeof(image)];
static size_t offset;
static unsigned calls;
static const int* steps;
static size_t stepCount;
static int trailing;

static ssize_t TestWrite(int fd, const void* data, size_t count) {
    assert(fd == 3 && count > 0 && count <= SSIZE_MAX);
    assert(data == image + offset);
    assert(calls < 64);
    int answer = calls < stepCount ? steps[calls] : trailing;
    calls++;
    if (answer < 0) {
        errno = -answer;
        return -1;
    }
    if (answer == 0) return 0;
    assert((size_t)answer <= count || answer == SSIZE_MAX);
    if ((size_t)answer > count) answer = (int)count;
    memcpy(written + offset, data, (size_t)answer);
    offset += (size_t)answer;
    return answer;
}
#define write TestWrite
"""
TEST = r"""
static void Begin(const int* results, size_t count, int after) {
    steps = results;
    stepCount = count;
    trailing = after;
    offset = calls = 0;
    errno = 0;
}

int main(void) {
    Begin(NULL, 0, SSIZE_MAX);
    assert(WriteMemfdImage(3, image, 5) && calls == 1 && offset == 5);
    assert(memcmp(image, written, 5) == 0);

    const int interrupted[] = {-EINTR, 2, -EINTR, 5, 1, 6};
    Begin(interrupted, sizeof(interrupted) / sizeof(interrupted[0]), SSIZE_MAX);
    assert(WriteMemfdImage(3, image, sizeof(image) - 1));
    assert(offset == sizeof(image) - 1 && calls == 7);
    assert(memcmp(image, written, offset) == 0);

    const int stalled[] = {2, 0};
    Begin(stalled, 2, SSIZE_MAX);
    assert(!WriteMemfdImage(3, image, 5) && calls == 2 && offset == 2 && errno == EIO);

    const int diskFull[] = {2, -ENOSPC};
    Begin(diskFull, 2, SSIZE_MAX);
    assert(!WriteMemfdImage(3, image, 5) && calls == 2 && offset == 2 && errno == ENOSPC);

    Begin(NULL, 0, -EINTR);
    assert(!WriteMemfdImage(3, image, 5) && calls == 16 && offset == 0 && errno == EINTR);

    int repeated[31];
    for (size_t i = 0; i < 31; i++) repeated[i] = i == 15 ? 2 : -EINTR;
    Begin(repeated, 31, SSIZE_MAX);
    assert(WriteMemfdImage(3, image, 5) && calls == 32 && offset == 5);
    assert(memcmp(image, written, offset) == 0);

    Begin(NULL, 0, SSIZE_MAX);
    assert(WriteMemfdImage(3, image, 0) && calls == 0 && offset == 0);
    return 0;
}
"""


def main():
    host = (ROOT / "port/android64/host/android64_host.c").read_text(encoding="utf-8")
    start = host.index("static int WriteMemfdImage(")
    loop = host[start:host.index("\n}", start) + 2]
    if os.name == "nt":
        compiler = next(p / "toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe"
                        for p in ROOT.parents
                        if (p / "toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe").is_file())
    else:
        compiler = os.environ.get("CC", "cc")
    with tempfile.TemporaryDirectory() as tmp:
        source = Path(tmp) / "write_loop.c"
        exe = Path(tmp) / "write_loop.exe"
        source.write_text(PROLOGUE + loop + TEST, encoding="utf-8")
        subprocess.run([str(compiler), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        str(source), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True, timeout=5)
    print("Android image write loop fault injection PASS")


if __name__ == "__main__":
    main()
