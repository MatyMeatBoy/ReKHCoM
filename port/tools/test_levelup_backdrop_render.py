"""Capture isolated real boss reward transitions, using an explicit private ROM.

No original save/config is opened. The report records exact binary/ROM identity;
visual comparisons and Original-view A/B remain separate from capture success.
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


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--rom', type=Path, required=True, help='Already isolated private EU fixture')
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--entries', default='197,198,199,200,201,202,203,204,205')
    p.add_argument('--view', type=int, default=4)
    p.add_argument('--zoom', type=int, default=6)
    p.add_argument('--hd', type=int, choices=(0, 1), default=0)
    p.add_argument('--riku', type=int, choices=(0, 1), default=0)
    p.add_argument('--size', default='320x180')
    p.add_argument('--at', type=int, default=1400, help='Closing injection frame; vary to avoid a live BG effect')
    p.add_argument('--natural', action='store_true', help='Private instrumented rewardready probe, then actual boss damage/defeat')
    args = p.parse_args()
    exe, rom, out = args.exe.resolve(), args.rom.resolve(), args.out.resolve()
    assert not out.exists(), 'Use a NEW evidence directory'
    out.mkdir(parents=True)
    source = (ROOT/'src/debug/mode_chkbtl.c').read_text()
    entries = re.findall(r'^\s*\{ WORLD_[^\n]+', source, re.M)
    ids = [i for i, row in enumerate(entries) if re.search(r',\s*2,\s*BATTLE_', row)] if args.entries == 'all-bosses' else [int(n) for n in args.entries.split(',')]
    rom_sha, exe_sha = digest(rom), digest(exe)
    results = []
    for entry in ids:
        case = out/f'boss-{entry}'
        case.mkdir()
        for src, name in ((exe, 'khcom.exe'), (exe.parent/'SDL3.dll', 'SDL3.dll')):
            os.link(src, case/name)  # immutable executables only; every writable file is private
        shutil.copytree(ROOT/'port/assets/fonts', case/'assets/fonts')
        (case/'kh_com.ini').write_text(f'config_version=11\nview_mode={args.view}\nzoom={args.zoom}\nhd_text={args.hd}\nvsync=0\n')
        env = {k: v for k, v in os.environ.items() if not k.startswith('KHCOM_')}
        frames = tuple(args.at+offset for offset in (-50, 40, 100, 190, 500))
        env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
                   KHCOM_WINDOW=args.size, KHCOM_BATTLE=f'{entry}:0:{args.riku}',
                   KHCOM_KEYS=f'60-63:A,{args.at+400}-{args.at+403}:A', KHCOM_MODE=f'{args.at}:levelup',
                   KHCOM_QUIT=str(args.at+550), KHCOM_OUTPUT_SHOT=','.join(map(str, frames)),
                   KHCOM_PPULOG='1', KHCOM_REGLOG=f'{args.at-10}-{args.at+101}', KHCOM_PPU_THREADS='1')
        if args.natural:
            frames = (250, 1000, 1500, 2000, 2500)
            modes = ['300:rewardready'] + [f'{f}:bosskill' for f in range(350, 1951, 100)]
            modes += [f'{f}:btllog' for f in frames]
            env.update(KHCOM_KEYS='60-63:A,3000-3003:A', KHCOM_MODE=','.join(modes),
                       KHCOM_QUIT='3200', KHCOM_OUTPUT_SHOT=','.join(map(str, frames)),
                       KHCOM_REGLOG='1490-1501')
        with (case/'stdout.txt').open('w') as log:
            run = subprocess.run([str(case/'khcom.exe'), '--rom', str(rom), '--language', 'es'],
                                 cwd=case, env=env, stdout=log, stderr=log, timeout=180,
                                 creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0) | getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0))
        log = (case/'kh_com.log').read_text(errors='replace')
        passed = run.returncode == 0 and 'harness: no mode levelup' not in log and 'CRASH' not in log and all((case/f'out_{frame:05}.bmp').is_file() for frame in frames)
        row = dict(entry=entry, name=re.search(r'BATTLE_[A-Z0-9_]+', entries[entry])[0],
                   exit=run.returncode, captures=passed, frames=frames)
        results.append(row)
        (out/'RESULTS.json').write_text(json.dumps(dict(executable_sha256=exe_sha, private_rom_sha256=rom_sha,
            view=args.view, zoom=args.zoom, hd=args.hd, riku=args.riku, size=args.size, injection_frame=args.at, natural_probe=args.natural, cases=results,
            scope='Real debug encounter closing/reward tasks; capture completion is not pixel certification or natural EXP/device proof'), indent=2))
        print(row['name'], 'capture PASS' if passed else 'FAIL', flush=True)
        assert passed, row
    assert digest(rom) == rom_sha, 'Private input changed'


if __name__ == '__main__':
    main()
