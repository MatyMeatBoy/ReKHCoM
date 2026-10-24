#!/usr/bin/env python3
"""experimental_frame_skip keeps the game's logic exact: every story event runs twice, as always and with
KHCOM_SKIPTEST=1 (two frames of three are not drawn), and the shots, taken on frames that are drawn (multiples of
3), must match pixel for pixel. A difference means the game read something only drawing produces.

    python port/tools/test_frameskip.py [--version eu] [--jobs 2] [--only 0:8,1:3] [--build build/pc-eu]

Cases: build/frameskip/<version>/{normal,skip}/<list>_<index>/shot_*.bmp; report: build/frameskip/<version>/report.txt
"""
import argparse
import hashlib
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FRAMES = 3600  # shots at 900, 1800, 2700, 3600: all drawn under KHCOM_SKIPTEST


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--version', default='eu', choices=['eu', 'us', 'jp'])
    ap.add_argument('--jobs', type=int, default=2, help='per pass; the two passes run side by side')
    ap.add_argument('--only', default='')
    ap.add_argument('--build')
    args = ap.parse_args()

    work = ROOT / 'build/frameskip' / args.version
    common = [sys.executable, '-u', str(ROOT / 'port/tools/test_events.py'), '--version', args.version,
              '--frames', str(FRAMES), '--jobs', str(args.jobs), '--resume']
    if args.build:
        common += ['--build', args.build]
    if args.only:
        common += ['--only', args.only]

    def run(name, extra_env):
        log = open(work / f'{name}.log', 'w', encoding='utf-8')
        return subprocess.run(common + ['--work', str(work / name)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                              env=dict(os.environ, **extra_env)).returncode

    work.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(2) as pool:
        codes = list(pool.map(lambda a: run(*a), [('normal', {}), ('skip', {'KHCOM_SKIPTEST': '1'})]))

    lines, differ = [], 0
    for case in sorted(p for p in (work / 'normal').iterdir() if p.is_dir() and (p / 'result.json').exists()):
        other = work / 'skip' / case.name
        bad = []
        for shot in sorted(case.glob('shot_*.bmp')):
            twin = other / shot.name
            if not twin.exists() or hashlib.sha1(shot.read_bytes()).digest() != hashlib.sha1(twin.read_bytes()).digest():
                bad.append(shot.stem)
        if bad:
            differ += 1
        lines.append(f'{case.name}: {"DIFFERS " + " ".join(bad) if bad else "same"}')
    lines.append(f'{differ} of {len(lines)} events differ (passes exit {codes})')
    (work / 'report.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print('\n'.join(lines[-15:]))
    sys.exit(1 if differ or any(codes) else 0)


if __name__ == '__main__':
    main()
