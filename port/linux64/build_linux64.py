#!/usr/bin/env python3
"""The 64-bit Linux build (port/linux64): the x32 game images and the x86-64 host, into build/linux64/out
(bin64/rekhcom64, bin64/guest-<region>.bin, lib64/ with SDL3 and the client libraries SDL opens by their unversioned
names). build_linux.py --package adds that folder to the Linux and Steam Deck packages; ReKHCoM.sh runs it on x86-64.

    python port/tools/linux_prepare.py --arch amd64        (once: Debian sysroot, SDL3, FreeType)
    python port/linux64/build_linux64.py [--regions eu us jp]
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
from workdir import WORK_DIR  # noqa: E402

TC = sorted((ROOT.parent / 'toolchain').glob('llvm-mingw-*'))[-1] / 'bin'
BASE = WORK_DIR / 'linux/amd64'
OUT = ROOT / 'build/linux64/out'
# what SDL opens by itself (build_linux.py DLOPEN) and what those need, x86-64
LIBS = ['libX11.so.6', 'libXext.so.6', 'libXcursor.so.1', 'libXi.so.6', 'libXfixes.so.3', 'libXrandr.so.2',
        'libXss.so.1', 'libXrender.so.1', 'libXtst.so.6', 'libasound.so.2', 'libpulse.so.0', 'libudev.so.1']
# only under the unversioned name SDL opens: the system's own versioned libraries (and their libxcb...) stay the
# system's, so its newer Mesa never meets an older copy of them


def find(name):
    for d in (BASE / 'sysroot/usr/lib/x86_64-linux-gnu', BASE / 'sysroot/lib/x86_64-linux-gnu'):
        if (d / name).exists():
            return d / name
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    args = ap.parse_args()
    env = dict(os.environ, CLANG=str(TC / 'clang.exe'), LLD=str(TC / 'ld.lld.exe'), OBJCOPY=str(TC / 'llvm-objcopy.exe'),
               REKHCOM_GUEST_DUALSENSE='0')  # SELECT, not the DualSense's touchpad, in the hints
    shutil.rmtree(OUT, ignore_errors=True)
    (OUT / 'bin64').mkdir(parents=True)
    (OUT / 'lib64').mkdir()
    for region in args.regions:
        subprocess.check_call([sys.executable, str(ROOT / 'port/ps5/build_ps5.py'), 'guest', '--region', region],
                              cwd=ROOT, env=env)
        shutil.copyfile(ROOT / f'port/ps5/build/guest-{region}/guest.bin', OUT / f'bin64/guest-{region}.bin')
    subprocess.check_call([str(TC / 'clang.exe'), '--target=x86_64-linux-gnu', f'--sysroot={BASE / "sysroot"}',
                           '-fuse-ld=lld', '-O2', f'-I{ROOT / "port/ps5/shared"}', f'-I{ROOT / "port/ps5/third_party/stb"}',
                           f'-I{BASE / "sdl/include"}', str(HERE / 'linux64_host.c'), f'-L{BASE / "sdl/lib"}', '-lSDL3',
                           '-lm', '-Wl,-rpath,$ORIGIN/../lib64', '-o', str(OUT / 'bin64/rekhcom64')])
    shutil.copyfile(sorted((BASE / 'sdl/lib').glob('libSDL3.so.0*'))[0], OUT / 'lib64/libSDL3.so.0')
    for name in LIBS:
        src = find(name)
        if not src:
            print('not in the sysroot:', name)
            continue
        data = src.resolve().read_bytes()
        plain = name.split('.so')[0] + '.so'  # SDL opens libX11.so, libasound.so ...
        (OUT / 'lib64' / plain).write_bytes(data)
    print(OUT, sorted(p.name for p in (OUT / 'bin64').iterdir()), len(list((OUT / 'lib64').iterdir())), 'libraries')


if __name__ == '__main__':
    main()
