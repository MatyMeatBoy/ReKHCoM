#!/usr/bin/env python3
"""Exercise real desktop menu visibility, cancel and orderly exit81.

Use --ui-source for the exact compiled UI source (desktop64 builds patch its
enum). Tests use new private data directories, blank saves and dummy audio.
An exit81 proves the game's launcher-close contract; it does not run a launcher.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--guest', type=Path)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--ui-source', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    exe, rom, ui = args.exe.resolve(), args.rom.resolve(), args.ui_source.resolve()
    guest = args.guest.resolve() if args.guest else None
    for path in [exe, rom, ui] + ([guest] if guest else []):
        if not path.is_file():
            parser.error('Missing input: ' + str(path))
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', ui.read_text(encoding='utf-8'), flags=re.S)
    start = re.search(r'enum\s*\{\s*ITEM_', text).start()
    entries = text[text.index('{', start)+1:text.index('};', start)].split(',')
    quit_index = next(i for i, entry in enumerate(entries) if entry.strip() == 'ITEM_QUIT')
    # Quit precedes every explicit enum value in these native/guest sources.
    if any('=' in entry for entry in entries[:quit_index]):
        parser.error('Explicit enum value before Quit needs an updated observer')
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    rows = []
    for hd_text in (0, 1):
        for cancel in (True, False):
            case = out / ('hdtext' + str(hd_text) + ('-cancel' if cancel else '-quit'))
            case.mkdir()
            (case / 'kh_com.ini').write_text(
                'config_version=11\ngraphics_mode=1\ngraphics_mode_applied=1\n'
                'view_mode=1\nzoom=8\nvsync=0\ndiscord_presence=0\nhd_text=' + str(hd_text) + '\n')
            shutil.copytree(ROOT / 'port/assets/fonts', case / 'assets/fonts')
            env = {k: v for k, v in os.environ.items() if not k.startswith(('SDL_', 'KHCOM_', 'REKHCOM_'))}
            env.update(SDL_VIDEO_DRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIO_DRIVER='dummy',
                       KHCOM_HOST_HIDDEN='1', KHCOM_HIDDEN='1', KHCOM_FAST='1',
                       KHCOM_BACKGROUND_TEST='1', KHCOM_PPU_THREADS='1',
                       KHCOM_OPTIONS_AT='100', KHCOM_OPTIONS_CURSOR=str(quit_index),
                       KHCOM_OPTIONS_AUDIT='1', KHCOM_OPTIONS_HOLD='55', KHCOM_QUIT='220',
                       KHCOM_OPTIONS_KEYS='10:13,20:27' if cancel else '10:13,20:1073741905,30:13')
            command = [str(exe)] + ([str(guest)] if guest else []) + ['--rom', str(rom), '--data-dir', str(case), '--language', 'en']
            with (case / 'console.txt').open('w', encoding='utf-8') as stream:
                child = subprocess.Popen(command, cwd=case, env=env, stdout=stream, stderr=subprocess.STDOUT)
                (case / 'OWNER.json').write_text(json.dumps({'pid': child.pid, 'command': command}))
                try:
                    code = child.wait(timeout=180)
                except subprocess.TimeoutExpired:
                    child.terminate()
                    child.wait(timeout=10)
                    code = 'timeout'
            log = (case / 'kh_com.log').read_text(errors='replace') if (case / 'kh_com.log').exists() else ''
            audit = [line for line in log.splitlines() if 'options-audit:' in line and 'item=' + str(quit_index) + ' ' in line]
            visible = bool(audit) and all('visible=1' in line for line in audit)
            row = dict(case=case.name, exit=code, row_visible=visible, pass_=visible and code == (0 if cancel else 81), audit=audit)
            (case / 'RESULT.json').write_text(json.dumps(row, indent=2))
            rows.append(row)
    result = dict(pass_=all(row['pass_'] for row in rows), cases=rows,
                  exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  guest_sha256=hashlib.sha256(guest.read_bytes()).hexdigest() if guest else None,
                  compiled_ui_sha256=hashlib.sha256(ui.read_bytes()).hexdigest(),
                  scope='Real menu, original/HD text, dummy audio, blank private data; no launcher or physical Linux certification')
    (out / 'RESULT.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result))
    return 0 if result['pass_'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
