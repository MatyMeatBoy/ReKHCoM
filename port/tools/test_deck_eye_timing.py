"""First deck eye tab: its left extension must begin/end its fade on the same frame.

Runs hidden against an original EU ROM, with one CPU worker. Optional --reference
also proves the native rectangle, lower panel fade and hand pixels stay identical.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]
FRAMES = list(range(208, 226)) + list(range(328, 341))


def capture(build, work, window, hd, sprites):
    work.mkdir(parents=True, exist_ok=True)
    for name in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / name, work / name)
    if (build / 'assets').is_dir():
        shutil.copytree(build / 'assets', work / 'assets', dirs_exist_ok=True)
    (work / 'kh_com.ini').write_text(f'config_version=11\ngraphics_mode=2\ngraphics_mode_applied=2\nview_mode=4\nzoom=6\nhd_text={hd}\nhd_cards={hd}\nhd_title={hd}\nhd_music=0\nhd_movies=0\nhd_scenes=0\nvsync=0\n')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
    for name in ('SDL_XINPUT_ENABLED', 'SDL_JOYSTICK_HIDAPI', 'SDL_JOYSTICK_RAWINPUT', 'SDL_JOYSTICK_DIRECTINPUT',
                 'SDL_JOYSTICK_WGI', 'SDL_JOYSTICK_GAMEINPUT', 'SDL_JOYSTICK_GAMEINPUT_RAW'):
        env[name] = '0'
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_PPU_THREADS='1',
               KHCOM_WINDOW=window, KHCOM_MODE='60:room:10901,120:fieldfreeze,180:Mode_Deck',
               KHCOM_KEYS='300-302:B', KHCOM_OUTPUT_SHOT=','.join(map(str, FRAMES)), KHCOM_QUIT='345')
    if sprites:
        env['KHCOM_HD_SPRITES'] = str(sprites)
    subprocess.run([str(build / 'khcom.exe'), '--data-dir', str(work), '--rom', str(ROOT / 'roms/B8CP.gba'),
                    '--language', 'es'], cwd=work, env=env, timeout=120, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    log = (work / 'kh_com.log').read_text(encoding='utf-8', errors='replace')
    assert 'mode: Mode_Deck' in log and not any(x in log for x in ('CRASH', 'FATAL', 'harness: no mode'))
    images = {}
    for frame in FRAMES:
        bmp = work / f'out_{frame:05d}.bmp'
        with Image.open(bmp) as im:
            image = im.convert('RGB')
            image.save(bmp.with_suffix('.png'))
            images[frame] = image
        # Keep lossless PNG evidence; these are generated files in this case only.
        assert bmp.resolve().is_relative_to(work.resolve())
        bmp.unlink()
    return images


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=Path, default=ROOT / 'build/pc-eu')
    ap.add_argument('--reference', type=Path)
    ap.add_argument('--out', type=Path, default=ROOT / 'build/deck-eye-timing')
    ap.add_argument('--window', default='1920x1080')
    ap.add_argument('--hd', type=int, choices=(0, 1), default=0)
    ap.add_argument('--sprites', type=Path)
    args = ap.parse_args()
    images = capture(args.build.resolve(), args.out.resolve() / 'after', args.window, args.hd, args.sprites)
    wrong = []
    for frame, image in images.items():
        w, h = image.size
        scale = h / 160
        edge = (w - 240 * scale) / 2
        y = int(18.5 * scale)
        if image.getpixel((int(edge - 8), y)) != image.getpixel((int(edge + 8), y)):
            wrong.append(frame)
    result = dict(desynced_frames=wrong, frames=FRAMES, window=args.window, hd=args.hd,
                  exe_sha256=hashlib.sha256((args.build / 'khcom.exe').read_bytes()).hexdigest())
    if args.reference:
        reference = capture(args.reference.resolve(), args.out.resolve() / 'before', args.window, args.hd, args.sprites)
        outside, changed = [], []
        for frame, image in images.items():
            diff = ImageChops.difference(image, reference[frame])
            box = diff.getbbox()
            if not box:
                continue
            changed.append(frame)
            w, h = image.size
            scale = h / 160
            edge = (w - 240 * scale) / 2
            if box[0] < 0 or box[2] > int(edge + 0.5) or box[1] < int(16 * scale) or box[3] > int(32 * scale + 0.5):
                outside.append((frame, box))
        result.update(changed_frames=changed, changes_outside_left_tab=outside)
        assert not outside, result
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / 'report.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))
    assert not wrong, result


if __name__ == '__main__':
    main()
