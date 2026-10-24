#!/usr/bin/env python3
"""Build the game images of the 64-bit Android app (port/progress/ANDROID_64.md).

The 32-bit app maps the GBA space at its hardware addresses, which fails where something already sits there (issues
#22, #34) and cannot run at all on 64-bit-only phones. The 64-bit app is the Switch build's idea (port/switch): the game
and the port layer are one AArch64 image with 32-bit pointers (clang arm64_32, Mach-O), with the relocatable GBA space,
loaded and served by a 64-bit host (port/android64/host). The differences:

- the image is linked at 0x01000000: in a 64-bit Android process ART keeps everything below its heap (0x12c00000)
  free, and the image (about 150 MB) ends well before it;
- PORT_ANDROID64 turns on the port's Android paths (touch controls, background pause), served by the host through
  the touch_poll and app_state bridge entries.

    python port/android64/build_android64.py              # build/android64/guest-<region>.macho for eu us jp
"""
import argparse
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / 'port/switch'))
import build_switch  # noqa: E402

BASE = 0x01000000


def configure():
    build_switch.BASE = BASE
    build_switch.BUILD = ROOT / 'build/android64'
    switch_flags = build_switch.common_flags

    def flags():
        return [f for f in switch_flags() if f != '-DPORT_SWITCH'] + [
            '-DPORT_ANDROID64', f'-I{ROOT / "port/android/native"}']
    build_switch.common_flags = flags


def build_guest(region, jobs):
    """The Switch guest build (it carries the touch controls, port/android/native/touch_android.c)."""
    configure()
    return build_switch.build_guest(region, jobs)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    p.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2))
    args = p.parse_args()
    for region in args.regions:
        build_guest(region, args.jobs)


if __name__ == '__main__':
    main()
