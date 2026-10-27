#!/usr/bin/env python3
"""Build the native Windows x64 SDL host and x32 game images (32-bit pointers).
Use --host-only to reuse previously built guest images without compiling the game.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
OUT = ROOT / 'build/windows64/out'

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    ap.add_argument('--jobs', type=int, default=1)
    ap.add_argument('--host-only', action='store_true')
    ap.add_argument('--test', action='store_true', help='run the ABI and low mapping regression test')
    args = ap.parse_args()
    if args.jobs < 1: ap.error('--jobs must be positive')
    tc = sorted((ROOT.parent / 'toolchain').glob('llvm-mingw-*'))[-1] / 'bin'
    sdl = sorted((ROOT.parent / 'toolchain').glob('SDL3-*'))[-1] / 'x86_64-w64-mingw32'
    OUT.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, CLANG=str(tc / 'clang.exe'), LLD=str(tc / 'ld.lld.exe'),
               OBJCOPY=str(tc / 'llvm-objcopy.exe'), REKHCOM_GUEST_DUALSENSE='0', REKHCOM_GUEST_LINUX64='1')
    if not args.host_only:
        for region in args.regions:
            subprocess.check_call([sys.executable, str(ROOT / 'port/ps5/build_ps5.py'), 'guest',
                                   '--region', region, '--jobs', str(args.jobs)], cwd=ROOT, env=env)
            shutil.copyfile(ROOT / f'port/ps5/build/guest-{region}/guest.bin', OUT / f'guest-{region}.bin')
    flags = [str(tc / 'clang.exe'), '--target=x86_64-w64-mingw32', '-O2',
             f'-I{ROOT / "port/ps5/shared"}', f'-I{ROOT / "port/ps5/third_party/stb"}',
             f'-I{sdl / "include"}', f'-L{sdl / "lib"}']
    subprocess.check_call(flags + [str(HERE / 'windows64_host.c'), '-lSDL3', '-lm', '-o', str(OUT / 'rekhcom64.exe')])
    shutil.copyfile(sdl / 'bin/SDL3.dll', OUT / 'SDL3.dll')
    if args.test:
        test = OUT / 'test_windows64.exe'
        subprocess.check_call(flags + [str(HERE / 'test_windows64.c'), str(HERE / 'test_preserved.S'), '-lSDL3', '-lm', '-o', str(test)])
        subprocess.check_call([str(test)], cwd=OUT)
    print(OUT)
if __name__ == '__main__': main()
