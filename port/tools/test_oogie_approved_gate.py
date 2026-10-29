#!/usr/bin/env python3
"""The approved Oogie arena must run by default, retain Off, and match its map.

Evaluate the actual DrawTextBg gate as C, with only the latch and map matcher
stubbed. No SDL, ROM or art inputs are needed. The preview gate is intentionally
separate so approving Oogie cannot promote another pending arena.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    args = ap.parse_args()
    source = (ROOT / 'port/src/port_ppu.c').read_text(encoding='utf-8')
    gate = re.search(r'int oogieHall\s*=\s*(.*?);', source, re.S).group(1)
    functions = []
    for name in ['PortArenasV2', 'PortArenasApproved']:
        functions.append(re.search(r'int '+name+r'\(void\)\s*\{.*?\n\}', source, re.S).group())
    c = r'''
#include <stdlib.h>
#include <stdio.h>
struct { int valid; const void* const* map; } sBgLatch[4];
static int matched;
static int PortOogieLayer(const void* const* maps) { (void)maps; return matched; }
''' + '\n'.join(functions) + '\nint main(int argc, char** argv) {\n' + r'''
    int bg = 0, expected;
    if (argc != 4) return 2;
    sBgLatch[0].valid = atoi(argv[1]); matched = atoi(argv[2]); expected = atoi(argv[3]);
''' + '    int oogieHall = ' + gate + ';\n' + r'''
    if (!!oogieHall != expected) {
        fprintf(stderr, "gate=%d expected=%d\n", !!oogieHall, expected); return 1;
    }
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='oogie-approval-contract-') as temp:
        path = Path(temp); (path/'test.c').write_text(c, encoding='utf-8')
        exe = path/('test.exe' if os.name == 'nt' else 'test')
        subprocess.run([args.cc, '-std=c11', str(path/'test.c'), '-o', str(exe)], check=True)
        count = 0
        for switch in [None, '1', '0']:
            env = dict(os.environ); env.pop('KHCOM_ARENAS_V2', None)
            if switch is not None: env['KHCOM_ARENAS_V2'] = switch
            for valid in [0, 1]:
                for match in [0, 1]:
                    expected = int(switch != '0' and valid and match)
                    subprocess.run([str(exe), str(valid), str(match), str(expected)], env=env, check=True)
                    count += 1
        print(f'PASS: {count} actual Oogie gate cases: default/on/off, valid/invalid, matching/unrelated maps')


if __name__ == '__main__':
    main()
