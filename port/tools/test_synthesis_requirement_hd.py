#!/usr/bin/env python3
"""Prepare an isolated serial synthesis capture. Default NEVER runs the game.

After the coordinator permits runtime testing, add --run. Optional --baseline
compares HD-off pixels against a previous executable directory. This smoke
fixture covers the real first door (1 >=); it does not claim all-rule coverage.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image
from synthesis_requirement_hd import ROOT, stage_assets

FRAMES = (300, 380, 470, 510)


def prepare(build, out, hd_source, enabled, fixture=None):
    out.mkdir(parents=True, exist_ok=False)
    for name in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / name, out / name)
    shutil.copytree(ROOT / 'port/assets/fonts', out / 'assets/fonts')
    sprites = out / 'hd/sprites'
    if hd_source:
        shutil.copytree(hd_source, sprites)
    else:
        sprites.mkdir(parents=True)
    stage_assets(sprites)
    (out / 'kh_com.ini').write_text(
        f'config_version=11\nview_mode=4\nzoom=8\nhd_text=1\nhd_cards={enabled}\nvsync=0\n',
        encoding='utf-8')
    env = dict(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
               SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT='0xffff/0xffff',
               KHCOM_PPU_THREADS='1',
               KHCOM_HD_SPRITES=str(sprites.resolve()), KHCOM_WINDOW='1920x1080',
               KHCOM_CONFIG=f'hd_cards={enabled};hd_text=1',
               KHCOM_MODE='60:room:10901,120:fieldfreeze,200:doorpose:0',
               KHCOM_KEYS='240-242:A', KHCOM_HDDRAWLOG='470,510',
               KHCOM_OUTPUT_SHOT=','.join(map(str, FRAMES)), KHCOM_QUIT='512')
    if fixture:
        env['KHCOM_SYNTHESIS_FIXTURE'] = fixture
    command = [str((out / 'khcom.exe').resolve()), '--language', 'es', '--rom',
               str((ROOT / 'roms/B8CP.gba').resolve())]
    (out / 'capture.json').write_text(json.dumps(dict(command=command, env=env), indent=2),
                                    encoding='utf-8')
    return command, env


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True, help='New directory; never overwritten')
    parser.add_argument('--hd', type=Path, help='Existing sprites to merge with synthesis pieces')
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--run', action='store_true', help='Explicitly launch serial captures')
    parser.add_argument('--fixture', choices=('red-at-most-7', 'green-exact-12', 'blue-total-10to9'),
                        help='Requires a PORT_SYNTHESIS_TEST_FIXTURE build; one HD-on run only')
    args = parser.parse_args()
    jobs = []
    cases = [('hd-on', args.build, 1)]
    if not args.fixture:
        cases.append(('hd-off', args.build, 0))
    for name, build, enabled in cases:
        folder = args.out / name
        jobs.append((name, folder, *prepare(build, folder, args.hd, enabled, args.fixture)))
    if args.baseline:
        folder = args.out / 'baseline-off'
        jobs.append(('baseline-off', folder, *prepare(args.baseline, folder, args.hd, 0)))
    if not args.run:
        print(f'Prepared {len(jobs)} isolated captures in {args.out}; NO game launched')
        return
    for name, folder, command, extra in jobs:
        env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
        env.update(extra)
        subprocess.run(command, cwd=folder, env=env, check=True, timeout=180,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        log = (folder / 'kh_com.log').read_text(errors='replace')
        assert 'CRASH' not in log and 'outside its box' not in log, name
        if name == 'hd-on':
            expected = {
                None: ('synthesis_body_none', 'synthesis_at_least_01'),
                'red-at-most-7': ('synthesis_body_red', 'synthesis_at_most_07'),
                'green-exact-12': ('synthesis_body_green', 'synthesis_exact_12'),
                'blue-total-10to9': ('synthesis_body_blue', 'synthesis_total_10'),
            }[args.fixture]
            for asset in expected:
                assert f'hd draw 470 {asset} ' in log, (name, asset)
            if args.fixture == 'blue-total-10to9':
                assert 'real UpdateEventKeyTotal 10->9 frame=490' in log
                assert 'hd draw 510 synthesis_total_09 ' in log
        else:
            assert 'hd draw 470 synthesis_' not in log, name
    if args.baseline:
        for frame in FRAMES:
            with Image.open(args.out / 'hd-off' / f'out_{frame:05d}.bmp') as actual, \
                 Image.open(args.out / 'baseline-off' / f'out_{frame:05d}.bmp') as before:
                assert actual.size == before.size and actual.tobytes() == before.tobytes(), frame
    print(f'PASS serial capture: {args.fixture or "natural first door + HD off"}; inspect before Atlas')


if __name__ == '__main__':
    main()
