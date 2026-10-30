"""Compile the shared engine setting parser used by the game and launcher."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', required=True)
    args = parser.parse_args()
    code = r'''
#include <assert.h>
#include <string.h>
#include "port/port_online_config.h"
int main(void) {
    assert(!PortOnlineParseEngine(0));
    assert(!PortOnlineParseEngine(""));
    assert(!PortOnlineParseEngine("classic\n"));
    assert(!PortOnlineParseEngine("modern-invalid"));
    assert(!PortOnlineParseEngine("modern\nclassic"));
    assert(PortOnlineParseEngine("modern"));
    assert(PortOnlineParseEngine("modern\r\n"));
    assert(!strcmp(PortOnlineEngineName(0), "classic"));
    assert(!strcmp(PortOnlineEngineName(1), "modern"));
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='online-config-') as tmp:
        src, exe = Path(tmp)/'test.c', Path(tmp)/'test.exe'
        src.write_text(code)
        subprocess.run([args.cc, '-std=c99', '-Wall', '-Wextra', '-Werror', '-static',
                        '-I', str(ROOT/'port/include'), str(src), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print('PASS shared online engine parser: classic default, strict modern, roundtrip')


if __name__ == '__main__':
    main()
