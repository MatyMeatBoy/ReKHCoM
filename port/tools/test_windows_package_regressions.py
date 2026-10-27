"""Run bounded, isolated x86/x64 scenarios against an extracted Windows package.

Private SRAM is optional and never modified. Success requires postconditions in
the game log, not just exit zero. Headless runs establish logic, not visual QA.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import time


def scenario(name):
    if name == 'saved-door':
        return dict(quit=1000, mode='900:progress', bot='1', keys='',
                    expected=['load-menu: loaded slot=0',
                              'floor=3 world=5 room=0 story=6 level=17 mode=Mode_MapFld'])
    if name == 'event-door':
        keys = '240-243:A,420-423:A,620-623:A,820-823:A,1200-1203:RIGHT,1300-1303:RIGHT,'
        keys += ','.join(f'{f}-{f+3}:A' for f in range(1500, 2990, 120))
        return dict(quit=3000, mode='60:room:10901,120:fieldfreeze,200:doorpose:3,2800:progress',
                    bot='', keys=keys, expected=['mode_battle', 'world=10 room=0'])
    commands = []
    fixtures = [('room', 0, 0), ('room', 5, 3), ('room', 9, 1), ('room', 12, 12),
                ('roomriku', 0, 0), ('roomriku', 5, 3), ('roomriku', 9, 1), ('roomriku', 12, 11)]
    for i, (actor, world, floor) in enumerate(fixtures):
        frame = 60 + i * 180
        commands += [f'{frame}:{actor}:{10000+world*100+floor}', f'{frame+140}:progress']
    return dict(quit=60+len(fixtures)*180, mode=','.join(commands), bot='', keys='',
                expected=['room fixture actor=sora', 'room fixture actor=riku'], progress_count=len(fixtures))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--package', type=Path, required=True)
    ap.add_argument('--rom', type=Path, required=True)
    ap.add_argument('--save', type=Path, help='Private EU regression SRAM; used only for saved-door')
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--arch', choices=['32', '64', 'both'], default='both')
    ap.add_argument('--case', choices=['saved-door', 'event-door', 'room-sweep', 'all'], default='all')
    args = ap.parse_args()
    package, rom, out = args.package.resolve(), args.rom.resolve(), args.out.resolve()
    if package == out or package in out.parents:
        ap.error('output must be outside the package')
    cases = ['saved-door', 'room-sweep', 'event-door'] if args.case == 'all' else [args.case]
    if 'saved-door' in cases and not args.save:
        ap.error('saved-door requires --save (the documented private regression fixture)')
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    arches = ['32', '64'] if args.arch == 'both' else [args.arch]
    for arch in arches:
        for case in cases:
            folder = out / f'{arch}-{case}'
            folder.mkdir(exist_ok=False)  # Never overwrite previous diagnostic evidence.
            if case == 'saved-door':
                shutil.copyfile(args.save, folder / 'kh_com.sav')
            (folder / 'kh_com.ini').write_text(
                'config_version=11\ngraphics_mode=1\ngraphics_mode_applied=1\n'
                'fullscreen=0\nvsync=0\nexperimental_frame_skip=0\n'
                'hd_movies=0\nhd_music=0\nhd_scenes=0\nhd_hud=0\nhd_text=0\nhd_cards=0\nhd_title=0\n'
                'discord_presence=0\nundub=0\n', encoding='utf8')
            spec = scenario(case)
            env = {k: v for k, v in os.environ.items() if not k.startswith(('SDL_', 'KHCOM_', 'REKHCOM_'))}
            env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_HOST_HIDDEN='1',
                       KHCOM_HIDDEN='1', KHCOM_FAST='1', KHCOM_BACKGROUND_TEST='1', KHCOM_PPU_THREADS='1',
                       KHCOM_TRACE_MODES='1', KHCOM_ASSETS=str(package / 'assets'),
                       KHCOM_QUIT=str(spec['quit']), KHCOM_MODE=spec['mode'])
            if spec['bot']: env['KHCOM_BOT'] = spec['bot']
            if spec['keys']: env['KHCOM_KEYS'] = spec['keys']
            exe = package / ('bin64/rekhcom64.exe' if arch == '64' else 'bin/khcom_eu.exe')
            cmd = [str(exe)]
            if arch == '64': cmd.append(str(package / 'bin64/guest-eu.bin'))
            cmd += ['--rom', str(rom), '--data-dir', str(folder), '--language', 'es']
            began = time.monotonic()
            with (folder / 'output.txt').open('w', encoding='utf8') as output:
                try:
                    result = subprocess.run(cmd, cwd=folder, env=env, stdout=output, stderr=output,
                                            timeout=150, creationflags=0x08004000)
                    rc = result.returncode
                except subprocess.TimeoutExpired:
                    rc = 'timeout'
            logfile = folder / 'kh_com.log'
            log = logfile.read_text(encoding='utf8', errors='replace') if logfile.exists() else ''
            missing = [value for value in spec['expected'] if value not in log]
            if spec.get('progress_count') and log.count('harness: progress ') != spec['progress_count']:
                missing.append(f"expected {spec['progress_count']} progress checkpoints")
            if spec.get('progress_count'):
                checkpoints = re.findall(r'harness: progress [^\n]+', log)
                if any('mode=Mode_MapFld' not in line for line in checkpoints):
                    missing.append('a room did not reach the field before its checkpoint')
                if log.count('harness: room fixture actor=') != spec['progress_count']:
                    missing.append('a room fixture did not execute')
            if 'harness: no mode ' in log: missing.append('a fixture command was rejected')
            row = dict(arch=arch, case=case, exit=rc, seconds=round(time.monotonic()-began, 2),
                       passed=rc == 0 and not missing, missing=missing,
                       executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest())
            if arch == '64': row['guest_sha256'] = hashlib.sha256((package/'bin64/guest-eu.bin').read_bytes()).hexdigest()
            rows.append(row)
            (out / 'report.json').write_text(json.dumps(rows, indent=2), encoding='utf8')
            print(json.dumps(row), flush=True)
            if not row['passed']: raise SystemExit(1)


if __name__ == '__main__':
    main()
