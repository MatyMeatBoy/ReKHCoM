#!/usr/bin/env python3
"""Undub against the Japanese cartridge itself: every voice song (SONG_VO_*) is played alone (KHCOM_SONG) with its
output dumped (KHCOM_WAVDUMP) on the EU and US builds with Undub on, and on the JP build; Undub passes a song when
both regional dumps are identical to the Japanese one (same samples, same sequence). With Undub off a regional dump
must stay its own. Issue #45 (22/10): the old check only asked that the two tested voices change, and passed
SONG_VO_SR_ATTACK00 still on its English sample.

Uses only the locally installed original ROMs; dumps stay under build/undub/.

    python port/tools/test_undub.py [--build-eu build/pc-eu] [--build-us build/pc-us] [--build-jp build/pc-jp]
                                    [--only SR_ATTACK00,...] [--jobs 4]
"""
import argparse
import hashlib
import os
import re
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build' / 'undub'
ROMS = {'eu': 'B8CP.gba', 'us': 'B8CE.gba', 'jp': 'B8CJ.gba'}


def voice_songs():
    """(name, id) of every SONG_VO_* in include/songs.h's enum."""
    names = re.findall(r'^\s+(SONG_\w+)', (ROOT / 'include/songs.h').read_text(), re.M)
    return [(n[8:], i) for i, n in enumerate(names) if n.startswith('SONG_VO_')]


def run(build, region, song, undub):
    d = OUT / f'{region}-{song}-{int(undub)}'
    shutil.rmtree(d, ignore_errors=True)
    (d / 'roms').mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / f, d / f)
    shutil.copy2(ROOT / 'roms' / 'B8CJ.gba', d / 'roms' / 'B8CJ.gba')
    (d / 'kh_com.ini').write_text(f'config_version=2\nundub={int(undub)}\n')
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
               KHCOM_SONG=f'30:{song}', KHCOM_WAVDUMP='voice.f32', KHCOM_QUIT='240')
    r = subprocess.run([str(d / 'khcom.exe'), '--data-dir', str(d), '--rom', str(ROOT / 'roms' / ROMS[region])],
                       cwd=d, env=env, timeout=120, capture_output=True)
    log = (d / 'kh_com.log').read_text(errors='replace') if (d / 'kh_com.log').exists() else ''
    dump = (d / 'voice.f32').read_bytes() if (d / 'voice.f32').exists() else b''
    problem = ''
    if r.returncode:
        problem = f'exit {r.returncode}'
    elif len(dump) < 4096:
        problem = 'no audio dump'
    elif 'm4a:' in log or 'fault' in log.lower():
        problem = 'sequencer fault'
    elif undub and region != 'jp' and 'voice tracks loaded' not in log:
        problem = 'Undub did not apply'
    shutil.rmtree(d, ignore_errors=True)
    return hashlib.sha256(dump).hexdigest(), problem


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-eu', default='build/pc-eu')
    ap.add_argument('--build-us', default='build/pc-us')
    ap.add_argument('--build-jp', default='build/pc-jp')
    ap.add_argument('--only')
    ap.add_argument('--jobs', type=int, default=4)
    args = ap.parse_args()
    builds = {'eu': ROOT / args.build_eu, 'us': ROOT / args.build_us, 'jp': ROOT / args.build_jp}
    songs = [(n, i) for n, i in voice_songs() if not args.only or n in args.only.split(',')]
    jobs = [(r, n, i, u) for n, i in songs for r, u in (('jp', False), ('eu', True), ('us', True), ('eu', False))]
    with ThreadPoolExecutor(args.jobs) as pool:
        res = dict(zip([(r, n, u) for r, n, i, u in jobs], pool.map(lambda j: run(builds[j[0]], j[0], j[2], j[3]), jobs)))
    bad = 0
    for n, i in songs:
        jp, eu, us, off = res[('jp', n, False)], res[('eu', n, True)], res[('eu', n, False)], res[('eu', n, False)]
        us = res[('us', n, True)]
        problems = [p for _, p in (jp, eu, us, off) if p]
        if problems:
            verdict = 'FAIL ' + '; '.join(problems)
        elif eu[0] == jp[0] and us[0] == jp[0]:
            verdict = 'ok'
        else:
            verdict = 'FAIL differs from the Japanese: ' + ' '.join(r for r, h in (('eu', eu), ('us', us)) if h[0] != jp[0])
        bad += verdict != 'ok'
        print(f'{verdict[:4]:4} {i:3} {n:16} {verdict if verdict != "ok" else ""}')
    print(f'test_undub: {len(songs) - bad}/{len(songs)} voices as the Japanese cartridge')
    return 1 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main())
