#!/usr/bin/env python3
"""Build Linux AArch64 LP64 host and relocatable AArch64 ILP32 Mach-O guests.

First: python port/tools/linux_prepare.py --arch arm64 --jobs 1
Then:  python port/linuxarm64/build_linuxarm64.py --jobs 1
The output can coexist with x86-64 bin64/lib64 in one Linux bundle.
"""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / 'port/tools'))
sys.path.insert(0, str(ROOT / 'port/switch'))
from workdir import WORK_DIR
from linux_runtime_clients import copy_client_libraries
import build_switch
BASE = WORK_DIR / 'linux/arm64'
BUILD = ROOT / 'build/linuxarm64'
OUT = BUILD / 'out'
TRIPLE = 'aarch64-linux-gnu'
_SWITCH_COMMON_FLAGS = build_switch.common_flags
LIBS = ['libX11.so.6', 'libXext.so.6', 'libXcursor.so.1', 'libXi.so.6', 'libXfixes.so.3', 'libXrandr.so.2',
        'libXss.so.1', 'libXrender.so.1', 'libXtst.so.6', 'libasound.so.2', 'libpulse.so.0', 'libudev.so.1']


def configure_guest():
    build_switch.BUILD = BUILD
    build_switch.BASE = 0x10000000
    build_switch.PPU_THREADS = False
    build_switch.TOUCH_CONTROLS = False
    build_switch.common_flags = lambda: [f for f in _SWITCH_COMMON_FLAGS() if f != '-DPORT_SWITCH'] + ['-DPORT_LINUX64_GUEST']


def build_host():
    llvm = build_switch.LLVM
    clang = os.environ.get('CLANG', str(llvm / 'clang.exe') if os.name == 'nt' else 'clang-18')
    sdl = BASE / 'sdl'
    libraries = sorted((sdl / 'lib').glob('libSDL3.so.0*'))
    if not libraries:
        raise SystemExit('ARM64 SDL3 missing: run python port/tools/linux_prepare.py --arch arm64 --jobs 1')
    (OUT / 'binarm64').mkdir(parents=True, exist_ok=True)
    (OUT / 'libarm64').mkdir(parents=True, exist_ok=True)
    subprocess.check_call([clang, f'--target={TRIPLE}', f'--sysroot={BASE / "sysroot"}', '-fuse-ld=lld', '-O2',
                           f'-I{ROOT / "port/ps5/shared"}', f'-I{ROOT / "port/ps5/third_party/stb"}', f'-I{sdl / "include"}',
                           str(HERE / 'linuxarm64_host.c'), f'-L{sdl / "lib"}', '-lSDL3', '-lm',
                           '-Wl,-rpath,$ORIGIN/../libarm64', '-o', str(OUT / 'binarm64/rekhcom-arm64')])
    shutil.copyfile(libraries[0], OUT / 'libarm64/libSDL3.so.0')
    search = [BASE / f'sysroot/{folder}/{TRIPLE}' for folder in ('usr/lib', 'lib')]
    search.append(BASE / f'sysroot/usr/lib/{TRIPLE}/pulseaudio')
    readelf = os.environ.get('READELF', str(llvm / 'llvm-readelf.exe') if os.name == 'nt' else 'llvm-readelf')
    copy_client_libraries(LIBS, search, OUT / 'libarm64', readelf)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    ap.add_argument('--jobs', type=int, default=1)
    ap.add_argument('--component', choices=['all', 'host', 'guest'], default='all')
    args = ap.parse_args()
    if args.jobs < 1:
        ap.error('--jobs must be positive')
    if args.component in ('all', 'host'):
        build_host()
    if args.component in ('all', 'guest'):
        configure_guest()
        (OUT / 'binarm64').mkdir(parents=True, exist_ok=True)
        for region in args.regions:
            image = build_switch.build_guest(region, args.jobs)
            shutil.copyfile(image, OUT / 'binarm64' / image.name)
    print(OUT)


if __name__ == '__main__':
    main()
