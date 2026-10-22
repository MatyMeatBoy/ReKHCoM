"""Darkside's beach (port_darkside.c): native pixels, original view and unrelated arenas against the same binary.

Runs isolated, hidden fixtures. The before/after switch (KHCOM_DARKSIDE_BEACH) turns only the beach, the single
Darkside and the single smoke layer on or off; it does not change the game state or executable.

    python port/tools/test_darkside_backdrop.py [--build-prefix build/pc-] [--out build/arena/darkside-tests]
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import subprocess
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]
FRAMES = (180, 380, 600)
# a fight where Darkside attacks: punch, crouch, claw and the raised arms (each pose drawn from the finished model)
ATTACK_FRAMES = (300, 780, 1020, 1620)
ATTACK_KEYS = '60-63:A,200-1700:A'


def run(case, args, root):
    region, actor, aspect, battle, stage, original = case[:6]
    attacks = len(case) > 6
    frames = ATTACK_FRAMES if attacks else FRAMES
    label = f'{region}-{actor}-{aspect}-{battle}-{int(original)}' + ('-attacks' if attacks else '')
    images = []
    for enabled in (False, True):
        d = root / label / ('after' if enabled else 'before')
        d.mkdir(parents=True)
        for name in ('khcom.exe', 'SDL3.dll'):
            shutil.copy2(ROOT / (args.build_prefix + region) / name, d / name)
        width, height, zoom = (960, 540, 4) if aspect == '16x9' else (960, 270, 2)
        (d / 'kh_com.ini').write_text(f'config_version=11\nview_mode={0 if original else 4}\nzoom={zoom}\nhd_text=0\nvsync=0\n')
        env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
                   KHCOM_WINDOW=f'{width}x{height}', KHCOM_BATTLE=f'{battle}:{stage}:{int(actor == "riku")}',
                   KHCOM_KEYS=ATTACK_KEYS if attacks else '60-63:A,200-380:LEFT,400-600:RIGHT',
                   KHCOM_QUIT=str(max(frames) + 10),
                   KHCOM_SHOTS=','.join(map(str, frames)), KHCOM_OUTPUT_SHOT=','.join(map(str, frames)))
        env['KHCOM_DARKSIDE_BEACH'] = '1' if enabled else '0'
        code = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}[region]
        p = subprocess.run([str(d / 'khcom.exe'), '--rom', str(ROOT / 'roms' / (code + '.gba'))],
                           cwd=d, env=env, timeout=180, capture_output=True)
        if p.returncode or 'CRASH' in (d / 'kh_com.log').read_text(errors='replace'):
            raise RuntimeError(f'{label}: process failed: {p.returncode}')
        internal = []
        for frame in frames:
            with Image.open(d / f'shot_{frame:05d}.bmp') as image:
                internal.append(image.convert('RGB'))
            with Image.open(d / f'out_{frame:05d}.bmp') as image:
                image.save(d / f'out_{frame:05d}.png')
        images.append(internal)
    changed, native_changes, black_reduction = 0, 0, []
    for before, after in zip(*images):
        if before.size != after.size:
            raise RuntimeError(label + ': canvas geometry changed')
        w, h = before.size
        box = ((w - 240) // 2, (h - 160) // 2, (w + 240) // 2, (h + 160) // 2)
        native_changes += ImageChops.difference(before.crop(box), after.crop(box)).getbbox() is not None
        changed += ImageChops.difference(before, after).getbbox() is not None
        black_reduction.append(sum(n for n, c in before.getcolors(w * h) if c == (0, 0, 0)) -
                               sum(n for n, c in after.getcolors(w * h) if c == (0, 0, 0)))
    should_change = battle == 198 and not original
    passed = native_changes == 0 and ((changed == len(frames) and min(black_reduction) > 0)
                                    if should_change else changed == 0)
    result = dict(case=label, passed=passed, changed_frames=changed, native_changed_frames=native_changes,
                  removed_black_pixels=black_reduction)
    print(('PASS ' if passed else 'FAIL ') + json.dumps(result), flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-prefix', default='build/pc-')
    parser.add_argument('--out', default='build/arena/darkside-tests')
    args = parser.parse_args()
    root = (ROOT / args.out).resolve()
    if not root.is_relative_to(ROOT / 'build'):
        parser.error('output must be under build/')
    root.mkdir(parents=True, exist_ok=False)
    cases = [(region, 'sora', aspect, 198, 11, False)
             for region in ('eu', 'us', 'jp') for aspect in ('16x9', '32x9')]
    cases += [('eu', 'riku', aspect, 198, 11, False) for aspect in ('16x9', '32x9')]
    cases += [(region, 'sora', '16x9', 198, 11, True) for region in ('eu', 'us', 'jp')]
    cases += [('eu', 'sora', '16x9', 198, 11, False, 'attacks'), ('jp', 'riku', '32x9', 198, 11, False, 'attacks')]
    # unrelated arenas stay identical: Guard Armor, Hook, a world field
    cases += [('eu', 'sora', '32x9', 199, 1, False), ('eu', 'sora', '32x9', 177, 6, False),
              ('eu', 'sora', '32x9', 0, 5, False)]
    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(lambda case: run(case, args, root), cases))
    (root / 'report.json').write_text(json.dumps(results, indent=2) + '\n')
    print(f'{sum(r["passed"] for r in results)}/{len(results)} passed', flush=True)
    return 0 if all(r['passed'] for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
