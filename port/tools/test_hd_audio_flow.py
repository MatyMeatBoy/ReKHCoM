#!/usr/bin/env python3
"""HD music flow in real time: stutters, jumps, overlaps and slow song changes.

Each scenario runs at the game's real speed (no KHCOM_FAST: a stutter only exists against the clock) with the HD
music on, SDL's "disk" audio driver (it takes the sound at a real device's pace and writes what would be heard to
out.raw, float32 stereo 48 kHz, nothing audible) and KHCOM_HDAUDIOLOG, one line per audio frame (port_hdmusic.c
HdLog). Reported per scenario:
  underruns  frames where the HD track plays with nothing queued (the device ran dry: a stutter)
  jumps      frames where the track's position moved backwards or by more than three frames' worth (a skip),
             outside its loop point
  overlaps   frames where the HD track plays and the cartridge's music was left audible
  loading    the longest run of frames a song change spent loading (silence)
  paused     frames on hold, and whether the position stayed put through them
  heard      with numpy/soundfile (run through: uv run --with numpy --with soundfile): the output is matched against
             each HD track it played, block by block; a change of the track's offset in the output is a jump heard

    python port/tools/test_hd_audio_flow.py --hd D:/01Claude/ReKHCoM-build/hd-completo [--build build/pc-eu] [--only title]
"""
import argparse
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOVES = ','.join(f'{f}-{f + 3}:{k}' for f, k in zip(range(200, 1700, 12), ['DOWN', 'RIGHT', 'UP', 'LEFT', 'DOWN'] * 30))
WALK = ','.join(f'{f}-{f + 40}:{k}' for f, k in zip(range(200, 1700, 50), ['RIGHT', 'DOWN', 'LEFT', 'UP'] * 10))
SCENARIOS = {
    'title': ({'KHCOM_MODE': '60:mode_title'}, 1800),
    'deckmenu': ({'KHCOM_MODE': '60:Mode_Deck', 'KHCOM_KEYS': MOVES}, 1800),
    'status': ({'KHCOM_MODE': '60:mode_status', 'KHCOM_KEYS': MOVES}, 1800),
    'worldselect': ({'KHCOM_MODE': '60:mode_worldselect', 'KHCOM_KEYS': MOVES}, 1800),
    'field': ({'KHCOM_MODE': '60:room:10901', 'KHCOM_KEYS': WALK}, 1800),
    'battle': ({'KHCOM_BATTLE': '0:1:0', 'KHCOM_KEYS': '150-153:A,' + MOVES.replace('DOWN', 'A')}, 1800),
    'boss': ({'KHCOM_BATTLE': '10:1:0', 'KHCOM_KEYS': '150-153:A'}, 1500),
    # our Options overlay over the field's song: open at 600, closed 300 frames later
    'options': ({'KHCOM_MODE': '60:room:10901', 'KHCOM_OPTIONS_AT': '600', 'KHCOM_OPTIONS_HOLD': '300'}, 1500),
    # the pause menus as a player walks them over the field's song: menu, deck, back, status, back, close (23/10)
    'pausemenus': ({'KHCOM_MODE': '60:room:10901',
                    'KHCOM_KEYS': '400-403:START,480-483:A,700-703:B,780-783:DOWN,800-803:DOWN,820-823:DOWN,'
                                  '840-843:DOWN,860-863:A,1080-1083:B,1160-1163:B'}, 1500),
}
LOOP_SLACK = 4  # frames of position step allowed beyond one frame's worth


def run(exe_dir, hd, name):
    env_extra, frames = SCENARIOS[name]
    d = ROOT / 'build/hdflow' / name
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(exe_dir / f, d / f)
    # graphics_mode set: without it the first start asks HD or GBA and waits (the title scenario timed out, 23/10)
    (d / 'kh_com.ini').write_text('config_version=2\ngraphics_mode=1\nview_mode=4\nhd_music=1\nhd_movies=0\n'
                                  'hd_scenes=0\n')
    shutil.copytree(hd / 'music', d / 'hd' / 'music')
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='disk', SDL_AUDIO_DISK_OUTPUT_FILE='out.raw',
               KHCOM_HDAUDIOLOG='hdaudio.log',
               KHCOM_QUIT=str(frames), **env_extra)
    env.pop('KHCOM_FAST', None)
    try:
        subprocess.run([str(d / 'khcom.exe'), '--language', 'es', '--rom', str(ROOT / 'roms/B8CP.gba')], cwd=d, env=env,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=frames / 50 + 120)
    except subprocess.TimeoutExpired:
        return {'error': 'timed out'}
    log = d / 'hdaudio.log'
    rows = [list(map(float, l.split())) for l in log.read_text().splitlines() if l.strip()] if log.exists() else []
    out = analyse(rows)
    out['heard'] = heard(d / 'out.raw', hd, rows)
    return out


def heard(raw, hd, rows):
    """Jumps heard: each HD track the scenario played is matched against the output block by block (0.25 s); its
    offset in the output must stay put. The first block is searched in the whole track, the next ones near the last
    offset. Needs numpy and soundfile."""
    try:
        import numpy as np
        import soundfile as sf
    except ImportError:
        return 'n/a'
    if not raw.exists():
        return 'no output'
    import re
    o = np.fromfile(raw, dtype=np.float32).reshape(-1, 2).mean(1)
    src = (ROOT / 'port/src/port_hdmusic.c').read_text(encoding='utf-8')
    table = src[src.index('kTracks[44] = {'):src.index('};', src.index('kTracks[44] = {'))]
    tracks = [None] + re.findall(r'"([^"]+)"', table)
    jumps, matched, blk = 0, 0, 12000
    for song in sorted({int(r[2]) for r in rows if r[3] == 1}):
        path = hd / 'music' / f'{tracks[song]}.ogg'
        if not path.exists():
            continue
        x, _ = sf.read(path, dtype='float32')
        x = x.mean(1) if x.ndim > 1 else x
        last = None
        for b in range(0, len(o) - blk, blk):
            seg = o[b:b + blk] - o[b:b + blk].mean()
            n = np.linalg.norm(seg)
            if n < 1e-3:
                continue
            lo = 0 if last is None else max(0, b + last - 24000)
            window = x[lo:(len(x) if last is None else lo + blk + 48000)]
            if len(window) < blk:
                continue
            c = np.correlate(window, seg, 'valid')
            k = int(np.argmax(c))
            conf = float(c[k] / (n * np.linalg.norm(window[k:k + blk]) + 1e-9))
            if conf < 0.95:
                continue
            offset = lo + k - b
            if last is not None and abs(offset - last) > 480:
                jumps += 1
            last = offset
            matched += 1
    return f'{jumps} jumps in {matched} matched blocks'


def analyse(rows):
    out = {'frames': len(rows), 'underruns': 0, 'jumps': 0, 'overlaps': 0, 'loading': 0, 'paused': 0,
           'paused_moved': 0, 'tracks': set()}
    step = 48000 / 59.7275
    run_loading = 0
    prev = None
    playing_for = 0
    for r in rows:
        frame, song, hdsong, state, queued, pos, gain = r
        if state == 1:
            out['tracks'].add(int(hdsong))
            playing_for += 1
            if playing_for > 8 and queued == 0:
                out['underruns'] += 1
            if gain > 0.0005:
                out['overlaps'] += 1
            if prev and prev[3] == 1 and prev[2] == hdsong and pos >= 0 and prev[5] >= 0:
                delta = pos - prev[5]
                # a backwards step is the loop point (to the loop start) unless the track was not near its end
                if delta > step * LOOP_SLACK or (delta < 0 and prev[5] - pos < step * 8):
                    out['jumps'] += 1
        else:
            playing_for = 0
        if state == 3:
            run_loading += 1
            out['loading'] = max(out['loading'], run_loading)
        else:
            run_loading = 0
        if state == 2:
            out['paused'] += 1
            if prev and prev[3] == 2 and pos != prev[5]:
                out['paused_moved'] += 1
        prev = r
    out['tracks'] = sorted(out['tracks'])
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--hd', required=True)
    ap.add_argument('--build', default='build/pc-eu')
    ap.add_argument('--only', default='')
    ap.add_argument('--jobs', type=int, default=3)
    args = ap.parse_args()
    names = [n for n in SCENARIOS if not args.only or n in args.only.split(',')]
    with ThreadPoolExecutor(args.jobs) as pool:
        results = dict(zip(names, pool.map(lambda n: run(ROOT / args.build, Path(args.hd), n), names)))
    bad = 0
    for n in names:
        r = results[n]
        if 'error' in r:
            print(f'{n:12s} {r["error"]}')
            bad += 1
            continue
        problem = r['underruns'] or r['overlaps'] or r['paused_moved'] or r['loading'] > 10 or not r['frames']
        bad += bool(problem)
        heard_bad = isinstance(r['heard'], str) and not r['heard'].startswith(('0 jumps', 'n/a'))
        problem = problem or heard_bad
        bad += bool(heard_bad and not (r['underruns'] or r['overlaps'] or r['paused_moved'] or r['loading'] > 10))
        print(f'{n:12s} {"PROBLEM" if problem else "ok":7s} frames {r["frames"]} underruns {r["underruns"]} '
              f'overlaps {r["overlaps"]} loading {r["loading"]} paused {r["paused"]}/{r["paused_moved"]} songs {r["tracks"]} '
              f'heard: {r["heard"]}')
    print(f'{bad} of {len(names)} scenarios with a problem')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
