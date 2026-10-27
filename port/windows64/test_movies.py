"""Synthetic Windows64 Media Foundation regression suite; no game ROM required.
Run after build_windows64.py --host-only --test. ffmpeg only creates test inputs.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time
ROOT = Path(__file__).resolve().parents[2]
ap = argparse.ArgumentParser(description=__doc__)
ap.add_argument('--font', default='')
ap.add_argument('--ffmpeg', default=shutil.which('ffmpeg'))
args = ap.parse_args()
if not args.ffmpeg: ap.error('ffmpeg is needed only to generate synthetic test inputs')
base = ROOT / 'build/windows64/movie-tests'
base.mkdir(parents=True, exist_ok=True)
exe = ROOT / 'build/windows64/out/test_movie.exe'
fixture = base / 'fixture.mp4'
subprocess.check_call([args.ffmpeg, '-v', 'error', '-f', 'lavfi', '-i', 'testsrc2=size=320x180:rate=30:duration=3',
                       '-f', 'lavfi', '-i', 'sine=frequency=440:sample_rate=48000:duration=3',
                       '-c:v', 'libx264', '-threads', '1', '-pix_fmt', 'yuv420p', '-c:a', 'aac', '-y', str(fixture)],
                      creationflags=0x08004000)
subs = base / 'fixture.es.txt'
subs.write_text('0 89 Texto de prueba\\nSegunda linea\n', encoding='utf-8')
corrupt = base / 'corrupt.mp4'; corrupt.write_bytes(b'not a movie')
unicode_movie = base / 'fixture-\u00f1.mp4'; shutil.copyfile(fixture, unicode_movie)
cases = [
    ('normal', str(fixture), {'KHCOM_HDMOVIE_SHOT': '1'}, 1, '90 frames decoded'),
    ('offset', str(fixture) + '@2', {}, 1, 'frames decoded'),
    ('skip', str(fixture), {'KHCOM_HDMOVIE_PAUSETEST': '0'}, 1, 'skip confirmed'),
    ('cancel', str(fixture), {'KHCOM_HDMOVIE_PAUSETEST': '0', 'KHCOM_HDMOVIE_PAUSE_ACTION': 'cancel'}, 1, '90 frames decoded'),
    ('reset', str(fixture), {'TEST_MOVIE_RESET': '1'}, 0, 'generation=1'),
    ('unicode', str(unicode_movie), {}, 1, '90 frames decoded'),
    ('software', str(fixture), {'SDL_VIDEO_DRIVER': 'dummy'}, 1, ' RGB,'),
    ('corrupt', str(corrupt), {}, 0, 'does not open'),
    ('missing', str(base / 'missing.mp4'), {}, 0, 'movie result=0'),
]
report = []
for name, movie, extra, expected, evidence in cases:
    data = base / name; data.mkdir(exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_HDMOVIE_', 'TEST_MOVIE_', 'EXPECT_MOVIE_', 'SDL_VIDEO_'))}
    env.update(SDL_AUDIO_DRIVER='dummy', **extra)
    if not expected: env['EXPECT_MOVIE_FAILURE'] = '1'
    before = time.monotonic()
    process = subprocess.run([str(exe), movie, str(subs), args.font, str(data)], env=env,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20, creationflags=0x08004000)
    elapsed = time.monotonic() - before
    (data / 'host.log').write_bytes(process.stdout)
    text = process.stdout.decode(errors='replace')
    passed = process.returncode == 0 and evidence in text and f'movie result={expected}' in text
    report.append(dict(case=name, passed=passed, seconds=round(elapsed, 3), returncode=process.returncode, evidence=evidence))
    print(report[-1], flush=True)
(base / 'report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
assert all(item['passed'] for item in report), 'see per-case host.log'
assert report[1]['seconds'] < report[0]['seconds'] - 0.5, 'seek did not shorten playback'
print('Windows64 MF normal/seek/pause/skip/cancel/reset/Unicode/software/failure tests passed')
