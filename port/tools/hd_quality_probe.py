#!/usr/bin/env python3
"""HD movie compression probe: a few real movies re-encoded by the launcher (--hd-compress) under several encoder
settings, then each one's size and VMAF against its source (ffmpeg with libvmaf). For choosing the presets' settings
(HD_MOD.md "Calidad de los videos").

    py -3 port/tools/hd_quality_probe.py --source D:/01Claude/ReKHCoM-build/hd-completo/movies
"""
import argparse
import json
import os
import re
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORK = Path('D:/01Claude/ReKHCoM-build/hdprobe')
# name: (quality preset, extra environment)
CONFIGS = {
    'actual-buena': (1, {}),
    'cq70-sw': (1, {'KHCOM_ENC_RC': '3', 'KHCOM_ENC_Q': '70', 'KHCOM_ENC_HW': '0'}),
    'cq80-sw': (1, {'KHCOM_ENC_RC': '3', 'KHCOM_ENC_Q': '80', 'KHCOM_ENC_HW': '0'}),
    'cq70-hw': (1, {'KHCOM_ENC_RC': '3', 'KHCOM_ENC_Q': '70'}),
    'actual-ahorro': (2, {}),
}


def vmaf(dist, ref):
    out = subprocess.run(['ffmpeg', '-hide_banner', '-i', str(dist), '-i', str(ref), '-lavfi',
                          '[0:v][1:v]scale2ref=flags=bicubic[d][r];[d][r]libvmaf=n_subsample=4:n_threads=4',
                          '-f', 'null', '-'], capture_output=True, text=True)
    m = re.search(r'VMAF score: ([\d.]+)', out.stderr)
    return float(m.group(1)) if m else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--source', required=True)
    ap.add_argument('--only')
    args = ap.parse_args()
    src = Path(args.source)
    scenes = sorted(src.glob('zz_event_*.mp4'), key=lambda p: p.stat().st_size)
    picks = [src / 'ROP.mp4', scenes[0], scenes[len(scenes) // 2], scenes[-1], scenes[len(scenes) * 3 // 4]]
    exe = ROOT / 'build/pc-eu/ReKHCoM.exe'
    results = {}
    for name, (quality, extra) in CONFIGS.items():
        if args.only and name not in args.only.split(','):
            continue
        d = WORK / name
        shutil.rmtree(d, ignore_errors=True)
        (d / 'hd/movies').mkdir(parents=True)
        for p in picks:
            shutil.copy2(p, d / 'hd/movies' / p.name)
        shutil.copy2(exe, d / 'ReKHCoM.exe')
        shutil.copy2(ROOT / 'build/pc-eu/SDL3.dll', d / 'SDL3.dll')
        env = dict(os.environ, SDL_VIDEO_DRIVER='offscreen', **extra)
        subprocess.run([str(d / 'ReKHCoM.exe'), '--hd-compress', '--quality', str(quality)], cwd=d, env=env,
                       capture_output=True, timeout=3600)
        rows = {}
        for p in picks:
            out = d / 'hd/movies' / p.name
            rows[p.name] = {'source_MB': round(p.stat().st_size / 1e6, 1), 'MB': round(out.stat().st_size / 1e6, 1),
                            'vmaf': vmaf(out, p)}
        results[name] = rows
        total = sum(r['MB'] for r in rows.values())
        mean = sum(r['vmaf'] or 0 for r in rows.values()) / len(rows)
        print(f"{name:14} total {total:7.1f} MB  vmaf mean {mean:5.1f}  " +
              ' '.join(f"{k.replace('zz_event_', 'e').replace('.mp4', '')}={v['MB']}MB/{v['vmaf']:.1f}" if v['vmaf'] else f"{k}=?"
                       for k, v in rows.items()), flush=True)
    (WORK / 'results.json').write_text(json.dumps(results, indent=1))


if __name__ == '__main__':
    main()
