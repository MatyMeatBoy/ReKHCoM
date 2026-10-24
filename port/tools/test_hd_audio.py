#!/usr/bin/env python3
"""The HD music replaces the music only: no sound effect may be taken for music.

The mixer silences the BGM player's channels when the HD music plays (m4a_port.c IsMusicChannel). With
KHCOM_AUDIOCHECK=1 every sounding channel's track is looked up in the player table with its real track counts, and a
channel taken for the wrong kind is logged ("audio check: ..."). Each scenario presses buttons where the game plays
sound effects (title, menus, field, battles, level-up, world select, moogle).

    python port/tools/test_hd_audio.py [--build build/pc-eu] [--only title,battle]
"""
import argparse
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PRESSES = ','.join(f'{f}-{f + 3}:{k}' for f, k in zip(range(150, 1400, 25),
                                                       ['A', 'DOWN', 'RIGHT', 'B', 'UP', 'LEFT', 'A', 'B'] * 7))
# no START: it opens menus whose frames the harness does not count, and KHCOM_QUIT would never come
SCENARIOS = {
    'title': {'KHCOM_MODE': '60:mode_title'},
    'deck': {'KHCOM_MODE': '60:Mode_Deck'},
    'status': {'KHCOM_MODE': '60:mode_status'},
    'worldselect': {'KHCOM_MODE': '60:mode_worldselect'},
    'field': {'KHCOM_MODE': '60:room:10901,120:fieldfreeze'},
    'moogle': {'KHCOM_MODE': '60:room:10901,200:moogle'},
    'battle': {'KHCOM_BATTLE': '0:1:0'},
    'boss': {'KHCOM_BATTLE': '10:1:0'},
    'levelup': {'KHCOM_BATTLE': '0:1:0', 'KHCOM_MODE': '400:levelup'},
    # issue #44: Sora's voices (field and battle attacks, with and without undub) and the save point's sound
    'savepoint': {'KHCOM_MODE': '60:room:10901,200:mapsave'},
    'fieldattack': {'KHCOM_MODE': '60:room:10901'},
    'undubbattle': {'KHCOM_BATTLE': '0:1:0', 'KHCOM_CONFIG': 'undub=1'},
    'undubfield': {'KHCOM_MODE': '60:room:10901', 'KHCOM_CONFIG': 'undub=1'},
}
# the HD music must really play in a case, or nothing was checked (22/10: the cases ran without it and passed, #44)
SILENT_OK = {'deck', 'status'}
HD_MUSIC = os.environ.get('KHCOM_HD_MUSIC', 'D:/01Claude/ReKHCoM-build/hd-completo/music')


def run(exe_dir, name):
    d = ROOT / 'build/hdaudio' / name
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(exe_dir / f, d / f)
    (d / 'kh_com.ini').write_text('config_version=2\nview_mode=4\nhd_music=1\n')
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_AUDIOCHECK='1',
               KHCOM_HD_MUSIC=HD_MUSIC, KHCOM_KEYS=PRESSES, KHCOM_QUIT='1400', **SCENARIOS[name])
    try:
        subprocess.run([str(d / 'khcom.exe'), '--language', 'es', '--rom', str(ROOT / 'roms/B8CP.gba')], cwd=d, env=env,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
    except subprocess.TimeoutExpired:
        return ['timed out'], True
    log = (d / 'kh_com.log').read_text(encoding='utf-8', errors='replace') if (d / 'kh_com.log').exists() else ''
    bad = [l for l in log.splitlines() if l.startswith('audio check:')]
    if log and 'hd music: song' not in log and name not in SILENT_OK:
        bad.append('the HD music never played: nothing was checked')
    shutil.rmtree(d, ignore_errors=True)
    return bad, bool(log)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build', default='build/pc-eu')
    ap.add_argument('--only', default='')
    ap.add_argument('--jobs', type=int, default=3)
    args = ap.parse_args()
    names = [n for n in SCENARIOS if not args.only or n in args.only.split(',')]
    with ThreadPoolExecutor(args.jobs) as pool:
        results = dict(zip(names, pool.map(lambda n: run(ROOT / args.build, n), names)))
    failed = 0
    for n in names:
        bad, ran = results[n]
        ok = ran and not bad
        failed += not ok
        print(f'{n:12s} {"ok" if ok else ("NO LOG" if not ran else "WRONG")} {"; ".join(bad)}')
    print(f'{failed} of {len(names)} scenarios with a problem')
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
