#!/usr/bin/env python3
"""Audio through the reported transitions, at real speed and silent (v23-p3-audio-transitions).

SDL's "disk" audio driver consumes the mix at the device's rate without a speaker, so the port's own continuity
counter (KHCOM_AUDIOLOG: "consumer underruns N") says whether the game starved the device. Two scenes:

  intro   the intro skipped with START to the title, its menu, the opening movie and the first event
  moogle  the Moogle shop: its tutorial pages, then its menu

    python port/tools/test_audio_transitions.py --exe build/pc/khcom.exe --rom roms/B8CP.gba [--assets DIR]
        [--hd-sprites DIR] [--out DIR]

Windows hides the window (KHCOM_HOST_HIDDEN); KHCOM_BOT skips the first start's mode choice without running
unpaced. Fails on any underrun or a scene that never reached its last mode.
"""
import argparse, os, re, subprocess, sys, tempfile
from pathlib import Path

SCENES = {
    'intro': dict(keys='60-62:START,120-122:START,200-202:START,300-302:START', quit=900,
                  modes=('mode_title', 'Mode_MenuNew', 'mode_movie')),
    'moogle': dict(mode='60:room:10901,120:mode_ms_top',
                   keys=','.join(f'{f}-{f + 2}:A' for f in range(300, 1000, 50)), quit=1300,
                   modes=('mode_ms_top',)),
}
INI = 'config_version=11\nview_mode=4\nhd_text=1\nhd_cards=1\nhd_hud=1\nhd_title=1\n'


def run(name, scene, args, out):
    d = out / name
    d.mkdir(parents=True, exist_ok=True)
    (d / 'kh_com.ini').write_text(INI)
    env = dict(os.environ, SDL_VIDEO_DRIVER='offscreen', SDL_RENDER_DRIVER='software', SDL_AUDIO_DRIVER='disk',
               SDL_AUDIO_DISK_OUTPUT_FILE=str(d / 'audio.raw'), KHCOM_BOT='1', KHCOM_HOST_HIDDEN='1',
               KHCOM_WINDOW='1280x720', KHCOM_AUDIOLOG='1', KHCOM_KEYS=scene['keys'], KHCOM_QUIT=str(scene['quit']))
    if scene.get('mode'):
        env['KHCOM_MODE'] = scene['mode']
    if args.assets:
        env['KHCOM_ASSETS'] = args.assets
    if args.hd_sprites:
        env['KHCOM_HD_SPRITES'] = args.hd_sprites
    subprocess.run([str(Path(args.exe).resolve()), '--data-dir', str(d), '--rom', args.rom, '--language', 'es'], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=scene['quit'] / 60 + 60)
    log = (d / 'kh_com.log').read_text(errors='replace')
    underruns = max([int(n) for n in re.findall(r'consumer underruns (\d+)', log)] or [-1])
    missing = [m for m in scene['modes'] if f'mode: {m} ' not in log]
    ok = underruns == 0 and not missing
    print(f"{'PASS' if ok else 'FAIL'} {name}: underruns {underruns}" + (f", never reached {missing}" if missing else ''))
    return ok


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    p.add_argument('--rom', required=True)
    p.add_argument('--assets')
    p.add_argument('--hd-sprites')
    p.add_argument('--out')
    args = p.parse_args()
    out = Path(args.out or tempfile.mkdtemp(prefix='audio-transitions-'))
    results = [run(name, scene, args, out) for name, scene in SCENES.items()]
    sys.exit(0 if all(results) else 1)


if __name__ == '__main__':
    main()
