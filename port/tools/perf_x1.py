#!/usr/bin/env python3
"""Frame work at 1920x1080 (hidden D3D11 window, real time, silent): field, event scene and battle at a given zoom.

Prints per scenario the work per frame (logic + PPU + present; budget 16.74 ms at 59.73 Hz), the late frames and the
game's own profile line (PPU and present means). RENDIMIENTO.md keeps the results.

    python port/tools/perf_x1.py [--build build/pc-eu] [--zoom 2] [--only field,event,battle] [--threads N]
"""
import argparse
import csv
import os
import shutil
import statistics
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WALK = ','.join(f'{f}-{f + 40}:{k}' for f, k in zip(range(200, 1500, 50), ['RIGHT', 'DOWN', 'LEFT', 'UP'] * 10))
SCENARIOS = {
    'field': {'KHCOM_MODE': '60:room:10901', 'KHCOM_KEYS': WALK},
    'event': {'KHCOM_EVENT': '0:16', 'KHCOM_KEYS': ','.join(f'{f}-{f + 3}:A' for f in range(30, 1500, 40))},
    'iso': {'KHCOM_EVENT': '0:4', 'KHCOM_KEYS': ','.join(f'{f}-{f + 3}:A' for f in range(30, 1500, 40))},  # 004: iso floor
    'battle': {'KHCOM_BATTLE': '0:1:0', 'KHCOM_KEYS': '150-153:A,' + ','.join(f'{f}-{f + 3}:A' for f in range(200, 1500, 15))},
}


def run(exe_dir, name, zoom, threads, frames=1500):
    d = ROOT / 'build' / 'perf' / f'{name}_z{zoom}'
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(exe_dir / f, d / f)
    (d / 'kh_com.ini').write_text(f'config_version=11\nview_mode=4\nzoom={zoom}\nhd_text=0\nvsync=0\n'
                                  'experimental_frame_skip=0\n')
    env = dict(os.environ, SDL_AUDIO_DRIVER='dummy', KHCOM_HIDDEN='1', KHCOM_WINDOW='1920x1080', KHCOM_PROFILE='1',
               KHCOM_FRAMELOG='frames.csv', KHCOM_QUIT=str(frames), **SCENARIOS[name])
    if threads:
        env['KHCOM_PPU_THREADS'] = str(threads)
    env.pop('KHCOM_FAST', None)
    subprocess.run([str(d / 'khcom.exe'), '--rom', str(ROOT / 'roms/B8CP.gba')], cwd=d, env=env, timeout=300,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    rows = list(csv.DictReader(open(d / 'frames.csv')))[200:]
    work = sorted(float(r['render_work_ms']) for r in rows)
    late = sum(1 for r in rows if float(r['interval_ms']) > 20)
    prof = [l.strip() for l in (d / 'kh_com.log').read_text(errors='replace').splitlines() if 'profile:' in l]
    return (f'{name:7s} x{int(zoom) / 2:g}  work mean {statistics.mean(work):5.2f} p50 {work[len(work) // 2]:5.2f} '
            f'p99 {work[int(len(work) * .99)]:5.2f} ms | late {late}/{len(rows)} | {prof[len(prof) // 2] if prof else ""}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', default='build/pc-eu')
    ap.add_argument('--zoom', default='2', help='kh_com.ini zoom id: 2 = x1, 6 = x3, 8 = x4')
    ap.add_argument('--only', default='field,event,iso,battle')
    ap.add_argument('--threads', type=int, help='KHCOM_PPU_THREADS (default: the game picks)')
    args = ap.parse_args()
    for name in args.only.split(','):
        print(run(ROOT / args.build, name, args.zoom, args.threads), flush=True)


if __name__ == '__main__':
    main()
