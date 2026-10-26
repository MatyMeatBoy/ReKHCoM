"""Real hidden SDL menu input: remapped axes, held controls, keyboard binding and rapid overlays.

No player data is opened: every case starts with its own settings and blank save.
    python port/tools/test_ui_input.py --build build/pc-eu [--cycles 100]
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

from test_text_options import item_numbers

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--rom', type=Path, default=ROOT / 'roms/B8CP.gba')
    parser.add_argument('--cycles', type=int, default=100)
    parser.add_argument('--timeout', type=int, default=180)
    args = parser.parse_args()
    build = args.build.resolve()
    source = ROOT / 'build/ui-input' / time.strftime('%Y%m%d_%H%M%S')
    source.mkdir(parents=True)
    items = item_numbers()
    cases = [
        ('axis-confirm', 'pad_a=108\n', {'KHCOM_VPAD': '140-145:LT,180-185:START'}, {'dodge_button': '0'}),
        ('held-axis', 'pad_a=108\n', {'KHCOM_VPAD': '80-145:LT,180-185:START'}, {'dodge_button': '1'}),
        ('held-release-press', 'pad_a=108\n', {'KHCOM_VPAD': '80-145:LT,160-165:LT,200-205:START'}, {'dodge_button': '0'}),
        ('axis-back', 'pad_b=110\n', {'KHCOM_VPAD': '140-145:RT'}, {}),
        ('keyboard-confirm', 'kb_a=102\n', {'KHCOM_OPTIONS_KEYS': '3:102,6:27'}, {'dodge_button': '0'}),
        ('keyboard-back', 'kb_b=103\n', {'KHCOM_OPTIONS_KEYS': '3:103'}, {}),
        ('keyboard-capture', '', {'KHCOM_OPTIONS_CURSOR': str(items['CONTROL_A']),
             'KHCOM_OPTIONS_KEYS': '3:13,6:102,9:27,12:27'}, {'kb_a': '102'}),
        ('axis-capture', '', {'KHCOM_OPTIONS_CURSOR': str(items['CONTROL_A']),
             'KHCOM_VPAD': '144-147:RIGHT,160-163:SOUTH,180-185:LT,210-215:START'}, {'pad_a': '108'}),
        ('restore-controls', 'kb_a=113\npad_a=1\npad_layout=2\n',
             {'KHCOM_OPTIONS_CURSOR': str(items['CONTROL_RESET']), 'KHCOM_OPTIONS_KEYS': '3:13,6:27,9:27'},
             {'kb_a': '120', 'kb_a_2': '107', 'pad_a': '0', 'pad_layout': '0'}),
    ]
    if args.cycles:
        if args.cycles < 1:
            parser.error('--cycles must be zero (skip stress) or positive')
        pad = []
        for i in range(args.cycles):
            frame = 180 + i * 60
            pad += [f'{frame}-{frame+2}:START', f'{frame}-{frame+2}:BACK', f'{frame+25}-{frame+28}:EAST']
        end = 240 + args.cycles * 60
        for context in ['field', 'event']:
            extra = {'KHCOM_VPAD': ','.join(pad), 'KHCOM_QUIT': str(end), 'KHCOM_FRAMELOG': 'logic.csv'}
            if context == 'field':
                extra['KHCOM_MODE'] = '30:room:10901,60:fieldfreeze'
            else:
                extra.update(KHCOM_EVENT='0:35', KHCOM_KEYS='60-63:A')
            cases.append(('rapid-' + context, '', extra, {}))
    results = []
    digest = hashlib.sha256((build / 'khcom.exe').read_bytes()).hexdigest()
    for name, ini, extra, expected in cases:
        work = source / name
        work.mkdir()
        for file in ['khcom.exe', 'SDL3.dll']:
            shutil.copy2(build / file, work / file)
        shutil.copytree(ROOT / 'port/assets/fonts', work / 'assets/fonts')
        (work / 'kh_com.ini').write_text('config_version=11\ngraphics_mode=2\ngraphics_mode_applied=2\n'
            'view_mode=4\nwindow_scale=2\nfullscreen=0\nvsync=0\ndodge_button=1\nassets_seen=-1\n' + ini)
        env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
        env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_OPTIONS_AUDIT='1',
            KHCOM_OPTIONS_TOGGLE='100', KHCOM_OPTIONS_HOLD='120', KHCOM_QUIT='260', KHCOM_TRACE_MODES='1')
        if name.startswith('rapid-'):
            env.pop('KHCOM_OPTIONS_TOGGLE')
            env.pop('KHCOM_OPTIONS_HOLD')
        env.update(extra)
        try:
            code = subprocess.run([str(work / 'khcom.exe'), '--rom', str(args.rom.resolve()), '--language', 'es'],
                cwd=work, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
        log = (work / 'kh_com.log').read_text(encoding='utf-8', errors='replace')
        settings = dict(line.split('=', 1) for line in (work / 'kh_com.ini').read_text().splitlines()
                        if '=' in line and not line.startswith('#'))
        problems = []
        if code != 0 or re.search(r'bios:|CRASH|FATAL', log):
            problems.append(f'exit/fault: {code}')
        for key, value in expected.items():
            if settings.get(key) != value:
                problems.append(f'{key}={settings.get(key)}, expected {value}')
        opens = re.findall(r'options-lifecycle: open serial=(\d+) frame=(\d+)', log)
        closes = re.findall(r'options-lifecycle: close serial=(\d+) frame=(\d+) tick=(\d+)', log)
        if not opens or not closes:
            problems.append('missing open/close lifecycle evidence (build lacks audit or menu did not run)')
        if name == 'axis-back' and closes and int(closes[0][1]) >= 190:
            problems.append('RT failed to close before automatic timeout')
        if name == 'keyboard-back' and closes and int(closes[0][2]) >= 10:
            problems.append('remapped back failed to close before automatic timeout')
        if name.startswith('rapid-'):
            if len(opens) != args.cycles or len(closes) != args.cycles:
                problems.append(f'{len(opens)} opens/{len(closes)} closes for {args.cycles} requested')
            if [x[0] for x in opens] != [x[0] for x in closes]:
                problems.append('unmatched overlay lifecycle serials')
            if not (work / 'logic.csv').exists():
                problems.append('no game-frame progression evidence')
            else:
                frames = (work / 'logic.csv').read_text().splitlines()
                if len(frames) < 20:
                    problems.append('game did not keep advancing outside the overlays')
        result = dict(name=name, passed=not problems, exit=code, problems=problems,
                      opens=len(opens), closes=len(closes), folder=str(work))
        results.append(result)
        print(f'{name}: {"PASS" if result["passed"] else "FAIL"} ({len(opens)}/{len(closes)} open/close) ' + '; '.join(problems), flush=True)
        (source / 'report.json').write_text(json.dumps(dict(build_sha256=digest, cases=results), indent=2))
    print(source)
    return 0 if all(r['passed'] for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
