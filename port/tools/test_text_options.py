#!/usr/bin/env python3
"""Exercise the real paused options menu; isolated copies, serial game runs.

Each row of the route is named (port_ui.c's ITEM_*), so the options can be reordered without touching the test. For
every language: a baseline run (the menu opened and closed: the settings the game saves untouched) and the route run
(each row changed once). A row passes when its text changed, or it is shown greyed (enabled=0: not available in that
state); its setting must then be saved changed in kh_com.ini.

The dummy video driver tests menu changes and rendering, not monitor mode switching or physical VSync (fullscreen and
its resolution are left out); reports keep that limitation.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]

# (row, presses, direction, the kh_com.ini key it saves to)
ROUTE = [('VIEW', 1, 1, 'view_mode'), ('ASPECT', 1, 1, None), ('ZOOM', 1, 1, 'zoom'), ('INTZOOM', 1, 1, None),
         ('SCALE', 1, 1, 'scale_mode'), ('HUD', 1, 1, 'hud_size'), ('NARROW', 1, 1, 'narrow_fit'),
         ('FILTER', 1, 1, 'filter'), ('VSYNC', 1, 1, 'vsync'), ('SCANLINES', 1, 1, 'scanlines'),
         ('LCD', 1, 1, 'color_correction'), ('CARDS', 1, 1, 'card_hud_raise'), ('FPS', 1, 1, 'show_fps'),
         ('DODGE', 1, 1, 'dodge_button'), ('STOCK', 1, 1, 'stock_button'), ('SAVEBANK', 1, 1, 'save_bank'),
         ('MUSIC', 1, -1, 'music_volume'), ('SFX', 1, -1, 'sfx_volume'), ('MASTER', 1, -1, 'master_volume')]


def item_numbers():
    src = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
    src = re.sub(r'/\*.*?\*/|//[^\n]*', '', src, flags=re.S)
    match = re.search(r'enum\s*\{\s*ITEM_', src)
    if not match:
        raise ValueError('The options ITEM_ enum was not found')
    body = src[src.index('{', match.start()) + 1:src.index('};', match.end())]
    numbers, value = {}, 0
    for entry in body.split(','):
        name, _, expression = entry.strip().partition('=')
        if not name:
            continue
        name = name.strip().removeprefix('ITEM_')
        if expression:
            base, _, add = expression.strip().partition('+')
            value = numbers[base.strip()[5:]] if base.strip().startswith('ITEM_') else int(base, 0)
            value += int(add or 0)
        numbers[name] = value
        value += 1
    return numbers


def actions():
    numbers = item_numbers()
    out = []
    for name, count, direction, _ in ROUTE:
        for _ in range(count):
            out.append((len(out) * 3 + 3, numbers[name], direction, name))
    return out


def run(work, exe_dir, version, lang, steps, args):
    for filename in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(exe_dir / filename, work / filename)
    if not (work / 'assets/fonts').exists():
        shutil.copytree(ROOT / 'port/assets/fonts', work / 'assets/fonts')
    (work / 'kh_com.ini').write_text('config_version=11\nview_mode=4\nhd_text=0\nwindow_scale=2\nfullscreen_width=0\nfullscreen_height=0\n')
    for old in work.glob('options_*.bmp'):
        old.unlink()
    env = {key: value for key, value in os.environ.items() if not key.startswith('KHCOM_')}
    hold = (steps[-1][0] if steps else 3) + 10
    env.update(SDL_VIDEO_DRIVER=args.video, SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
               KHCOM_OPTIONS_TOGGLE='100', KHCOM_OPTIONS_HOLD=str(hold),
               KHCOM_QUIT='180', KHCOM_TRACE_MODES='1', KHCOM_OUTPUT_SHOT='175')
    if steps:
        env['KHCOM_OPTIONS_ACTIONS'] = ','.join(f'{tick}:{row}:{direction}' for tick, row, direction, _ in steps)
    rom = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}[version]
    try:
        code = subprocess.run([str(work / 'khcom.exe'), '--language', lang, '--rom', str(ROOT / 'roms' / f'{rom}.gba')],
                              cwd=work, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                              timeout=args.timeout).returncode
    except subprocess.TimeoutExpired:
        code = 'timeout'
    log = (work / 'kh_com.log').read_text(errors='replace')
    config = dict(line.split('=', 1) for line in (work / 'kh_com.ini').read_text().splitlines()
                  if '=' in line and not line.startswith('#'))
    return code, log, config


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--versions', nargs='+', choices=('eu', 'us', 'jp'), default=['eu', 'us', 'jp'])
    parser.add_argument('--languages', nargs='+', default=['en', 'fr', 'de', 'it', 'es'])
    parser.add_argument('--build-prefix', help='builds at <prefix><version> (default: build/pc, build/pc-us, build/pc-jp)')
    parser.add_argument('--timeout', type=int, default=120)
    parser.add_argument('--video', choices=('dummy', 'windows'), default='dummy',
                        help='windows opens an isolated real game window')
    args = parser.parse_args()
    base = ROOT / 'build/text-options-results' / datetime.datetime.now().strftime('%Y%m%d_%H%M%S_%f')
    base.mkdir(parents=True)
    steps = actions()
    results = []
    for version in args.versions:
        exe_dir = ROOT / (args.build_prefix + version if args.build_prefix else 'build/pc' + ('' if version == 'eu' else '-' + version))
        for lang in args.languages if version == 'eu' else ['ja' if version == 'jp' else 'en']:
            work = base / f'{version}-{lang}'
            work.mkdir()
            _, _, baseline = run(work, exe_dir, version, lang, [], args)
            code, log, config = run(work, exe_dir, version, lang, steps, args)
            found = re.findall(r'options-test: tick=(\d+) item=(\d+) enabled=(\d+) before=\[(.*?)\] after=\[(.*?)\]', log)
            by_tick = {int(t): (int(e), b, a) for t, _, e, b, a in found}
            rows, problems = {}, []
            for tick, _, _, name in steps:
                enabled, before, after = by_tick.get(tick, (None, None, None))
                if enabled is None:
                    problems.append(f'{name}: not reached')
                    continue
                state = 'greyed' if not enabled else 'changed' if before != after else 'unchanged'
                rows[name] = state
                if state == 'unchanged':
                    problems.append(f'{name}: the text stayed [{before}]')
            for name, _, _, key in ROUTE:
                if key and rows.get(name) == 'changed' and config.get(key) == baseline.get(key):
                    problems.append(f'{name}: {key} not saved ({config.get(key)})')
            shots = len(list(work.glob('options_*.bmp')))
            if code != 0 or 'CRASH' in log:
                problems.append(f'exit {code}' + (' CRASH' if 'CRASH' in log else ''))
            if shots != len(steps):
                problems.append(f'{shots} menu shots for {len(steps)} changes')
            if not (work / 'out_00175.bmp').exists():
                problems.append('no game picture after closing the menu')
            result = dict(version=version, language=lang, video=args.video, exit=code, rows=rows, problems=problems,
                          passed=not problems, directory=str(work),
                          limitations=(['dummy driver: physical fullscreen/display modes and VSync not verified'] if args.video == 'dummy' else []) +
                                      ['menu mutation/persistence/rendering; gameplay effects require scene tests'])
            (work / 'result.json').write_text(json.dumps(result, indent=2))
            print(f"{version}-{lang}: {'PASS' if result['passed'] else 'FAIL'} " +
                  ' '.join(f'{k}={v}' for k, v in rows.items() if v != 'changed') + (' | ' + '; '.join(problems) if problems else ''),
                  flush=True)
            results.append(result)
    (base / 'results.json').write_text(json.dumps(results, indent=2))
    print(base)
    return 0 if all(result['passed'] for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
