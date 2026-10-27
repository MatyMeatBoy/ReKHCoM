"""Compile the actual frame-key parser: a successful run must exercise full chords."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / 'port/src/port_main.c').read_text(encoding='utf8')
    start = source.index('static uint16_t HarnessKeys(uint32_t frame) {')
    body = source[start:source.index('\n}', start) + 2]
    code = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#define PORT_KEY_STOCK 0x400
#define PORT_KEY_DODGE 0x800
#define PORT_KEY_UNSTOCK 0x1000
#define SDL_atoi atoi
#define SDL_strchr strchr
#define SDL_strlen strlen
#define SDL_strncmp strncmp
#define SDL_strcmp strcmp
#define SDL_arraysize(a) (sizeof(a)/sizeof((a)[0]))
static const char* spec;
static int gPortGalleryEvent;
static const char* SDL_getenv(const char* name) {return !strcmp(name,"KHCOM_KEYS")?spec:NULL;}
static uint16_t MovieKeys(uint32_t frame) {return frame==2?128:0;}
static uint16_t MonkeyKeys(uint32_t frame) {return frame==3?64:0;}
static unsigned PortSceneGalleryAge(void) {return 0;}
''' + body + r'''
int main(void) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
    spec="10-12:L+R,20:A+B,30:START+SELECT,40:RIGHT+UNKNOWN+LEFT,50:L+L+R,60:STOCK+DODGE+UNSTOCK";
    assert(HarnessKeys(10)==768 && HarnessKeys(12)==768 && HarnessKeys(13)==0);
    assert(HarnessKeys(20)==3 && HarnessKeys(30)==12);
    assert(HarnessKeys(40)==48 && HarnessKeys(50)==768 && HarnessKeys(60)==0x1c00);
    spec="70:R+L,75:STARTED,80:A,broken,90:B";
    assert(HarnessKeys(70)==768 && HarnessKeys(75)==0 && HarnessKeys(80)==1 && HarnessKeys(90)==2);
    assert(!HarnessKeys(0)); /* A malformed entry must not borrow the next entry's colon. */
    spec=NULL;assert(HarnessKeys(2)==128 && HarnessKeys(3)==64 && !HarnessKeys(4));
    return 0;
}
'''
    cc = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-harness-chords-') as name:
        folder = Path(name); src = folder / 'keys.c'; exe = folder / 'keys.exe'
        src.write_text(code, encoding='utf8')
        subprocess.run([str(cc), '-std=c99', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(exe)], check=True)
        result = subprocess.run([str(exe)], timeout=10, creationflags=0x08004000)
        assert result.returncode == 0, f'frame key parser failed: {result.returncode:#x}'
    print('PASS: complete chords, frame boundaries, unknown tokens and other input producers')


if __name__ == '__main__':
    main()
