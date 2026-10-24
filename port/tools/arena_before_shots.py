#!/usr/bin/env python3
"""Preliminar V2 (boss arenas in the wide view): the arenas as they are today, at the art tests' x3 (zoom=6) in 16:9
(1920x1080) and 32:9 (3840x1080), a few frames into each fight, hidden and silent. A contact sheet per arena goes to
build/arena/v2-before/<arena>.png (WORKFLOW_ARTE_AMPLIO.md: compare before/after at the same sizes).

    py -3 port/tools/arena_before_shots.py [--build build/pc-eu] [--only trickmaster,jafar]
"""
import argparse
import os
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
# the debug battle menu's entries (src/debug/mode_chkbtl.c sChkBtlEntries)
ARENAS = {'darkside': 198, 'trickmaster': 200, 'oogie': 201, 'maleficent': 202, 'ursula': 203, 'jafar': 204,
          'marluxia': 205}
SIZES = {'16x9': (1920, 1080), '32x9': (3840, 1080)}
FRAMES = (240, 900, 1500)


def run(build, out, arena, aspect, v2=False):
    d = ROOT / 'build' / 'arena' / ('v2-runs-after' if v2 else 'v2-runs') / f'{arena}-{aspect}'
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / f, d / f)
    (d / 'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=0\nvsync=0\n')
    w, h = SIZES[aspect]
    keys = '60-63:A,' + ','.join(f'{f}-{f + 20}:RIGHT' for f in range(300, 1600, 200)) + ',' + \
        ','.join(f'{f}-{f + 20}:LEFT' for f in range(400, 1600, 200))
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
               KHCOM_WINDOW=f'{w}x{h}', KHCOM_BATTLE=f'{ARENAS[arena]}:1:0', KHCOM_KEYS=keys,
               KHCOM_OUTPUT_SHOT=','.join(map(str, FRAMES)), KHCOM_QUIT=str(FRAMES[-1] + 10))
    if v2:
        env['KHCOM_ARENAS_V2'] = '1'  # the Preliminar V2's work, off until approved
    subprocess.run([str(d / 'khcom.exe'), '--rom', str(ROOT / 'roms/B8CP.gba')], cwd=d, env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
    return [d / f'out_{f:05d}.bmp' for f in FRAMES]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', default=str(ROOT / 'build/pc-eu'))
    ap.add_argument('--only')
    ap.add_argument('--v2', action='store_true', help='with the Preliminar V2 (KHCOM_ARENAS_V2=1): the after shots')
    args = ap.parse_args()
    build = Path(args.build)
    out = ROOT / ('build/arena/v2-after' if args.v2 else 'build/arena/v2-before')
    out.mkdir(parents=True, exist_ok=True)
    arenas = args.only.split(',') if args.only else list(ARENAS)
    jobs = [(a, s) for a in arenas for s in SIZES]
    with ThreadPoolExecutor(4) as pool:
        shots = dict(zip(jobs, pool.map(lambda j: run(build, out, *j, v2=args.v2), jobs)))
    from PIL import Image
    for a in arenas:
        rows = []
        for s in SIZES:
            for p in shots[(a, s)]:
                if p.exists():
                    im = Image.open(p).convert('RGB')
                    im.thumbnail((1920, 540))
                    rows.append(im)
        if not rows:
            print(a, 'no shots')
            continue
        sheet = Image.new('RGB', (max(r.size[0] for r in rows), sum(r.size[1] + 6 for r in rows)), (40, 40, 40))
        y = 0
        for r in rows:
            sheet.paste(r, (0, y))
            y += r.size[1] + 6
        sheet.save(out / f'{a}.png')
        print(a, len(rows), 'shots ->', out / f'{a}.png')


if __name__ == '__main__':
    main()
