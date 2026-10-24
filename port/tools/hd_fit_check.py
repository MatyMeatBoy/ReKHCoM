#!/usr/bin/env python3
"""How well each HD sprite (port/data/hd_sprites.tsv, pictures in <hd sprites dir>) sits on its cartridge sprite: the
cartridge's silhouette (from the ROM: frame, tiles) against the HD picture's alpha brought to the sprite's box, as
overlap (IoU), and the shift (in GBA pixels, eighths) that would line their centres and edges up best.

    python port/tools/hd_fit_check.py <hd sprites dir> [--region eu] [--only name,...] [--worst 30]
"""
import argparse
import re
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SHAPES = {(0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64), (1, 0): (16, 8), (1, 1): (32, 8),
          (1, 2): (32, 16), (1, 3): (64, 32), (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64)}
SCALE = 8


def rom(region):
    code = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}[region]
    data = (ROOT / 'roms' / f'{code}.gba').read_bytes()
    syms = {k: int(v, 16) for k, v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)',
                                                 (ROOT / 'build' / region / 'pc' / 'rom_symbols.s').read_text())}
    return data, syms


def silhouette(data, syms, frame, tiles, box):
    """The cartridge sprite's opaque pixels in its box (x0, y0, w, h), at SCALE."""
    x0, y0, w, h = box
    m = np.zeros((h, w), bool)
    fo = syms[frame] - 0x08000000
    to = syms[tiles] - 0x08000000
    n = struct.unpack_from('<H', data, fo)[0]
    for k in range(min(n, 64)):
        a0, a1, a2 = struct.unpack_from('<HHH', data, fo + 2 + k * 6)
        if (a0 >> 14, a1 >> 14) not in SHAPES:
            continue
        pw, ph = SHAPES[(a0 >> 14, a1 >> 14)]
        x = a1 & 0x1FF
        x -= 512 if x & 0x100 else 0
        y = a0 & 0xFF
        y -= 256 if y & 0x80 else 0
        for ty in range(ph):
            for tx in range(pw):
                sx = pw - 1 - tx if (a1 >> 12) & 1 else tx
                sy = ph - 1 - ty if (a1 >> 13) & 1 else ty
                t = (a2 & 0x3FF) + (sy >> 3) * (pw >> 3) + (sx >> 3)
                b = data[to + t * 32 + (sy & 7) * 4 + ((sx & 7) >> 1)]
                v = b >> 4 if sx & 1 else b & 15
                mx, my = x + tx - x0, y + ty - y0
                if v and 0 <= mx < w and 0 <= my < h:
                    m[my, mx] = True
    return np.kron(m, np.ones((SCALE, SCALE), bool))


def ink_box(data, syms, frame, tiles, box):
    ys, xs = np.nonzero(silhouette(data, syms, frame, tiles, box))
    return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1  # in eighths of a GBA pixel


def fix(data, syms):
    p = ROOT / 'port/data/hd_sprites.tsv'
    out, changed = [], 0
    for line in p.read_text(encoding='utf-8').split(chr(10)):
        f = line.split(chr(9))
        if len(f) > 9 and not line.startswith('#'):
            name, frame, tiles, recipe = f[0], f[2], f[3], f[9]
            if frame == '-':
                frame = tiles[:-5] + 'Frame0'
            box = tuple(int(v) for v in f[4:8])
            if (name.startswith('gBtlPop') or name.startswith('gLvupLogo')) and re.search(r';place=[^;]+;scale=8$', recipe):
                x0, y0, x1, y1 = ink_box(data, syms, frame, tiles, box)
                # whole GBA pixels (place takes integers): the ink box rounded outwards
                pl = f'{x0 // SCALE},{y0 // SCALE},{-(-x1 // SCALE)},{-(-y1 // SCALE)}'
                f[9] = re.sub(r';place=[^;]+;', f';place={pl};', recipe)
            elif re.match(r'gCard(Modified)?ValueDigitTiles_\d$', name) and recipe.startswith('canvas='):
                x0, y0, x1, y1 = ink_box(data, syms, frame, tiles, box)
                m = re.search(r'blit=(\d+),(\d+),(\d+),(\d+),', recipe)
                sx0, sy0, sx1, sy1 = map(int, m.groups())
                h = y1 - y0
                w = round((sx1 - sx0) / (sy1 - sy0) * h)
                cx = (x0 + x1) / 2
                dx0 = max(0, round(cx - w / 2))
                dst = f'{dx0},{y0},{min(box[2] * SCALE, dx0 + w)},{y1}'
                f[9] = re.sub(r'(blit=\d+,\d+,\d+,\d+,)[^;]+', lambda mm: mm.group(1) + dst, recipe)
            if f[9] != recipe:
                changed += 1
            line = chr(9).join(f)
        out.append(line)
    p.write_text(chr(10).join(out), encoding='utf-8', newline='')
    print(f'{changed} recipes fitted')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('hd')
    ap.add_argument('--region', default='eu')
    ap.add_argument('--only')
    ap.add_argument('--worst', type=int, default=40)
    ap.add_argument('--fix', action='store_true',
                    help="rewrite the recipe: battle words (gBtlPop*, gLvupLogo*) placed on the cartridge's ink box, "
                         "card value digits centred on it at its height")
    args = ap.parse_args()
    data, syms = rom(args.region)
    only = set(args.only.split(',')) if args.only else None
    rows = []
    for line in (ROOT / 'port/data/hd_sprites.tsv').read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        f = line.split('\t')
        name, frame, tiles = f[0], f[2], f[3]
        if tiles == '-' or (only and name not in only):
            continue
        if frame == '-':
            frame = tiles[:-5] + 'Frame0' if tiles.endswith('Tiles') else None
        png = Path(args.hd) / f'{name}.png'
        if not frame or frame not in syms or tiles not in syms or not png.exists():
            continue
        box = tuple(int(v) for v in f[4:8])
        gba = silhouette(data, syms, frame, tiles, box)
        hd = np.array(Image.open(png).convert('RGBA').resize((gba.shape[1], gba.shape[0])))[:, :, 3] > 96
        inter, union = (gba & hd).sum(), (gba | hd).sum()
        iou = inter / union if union else 1.0
        def edges(m):
            ys, xs = np.nonzero(m)
            return (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1) if len(xs) else (0, 0, 0, 0)
        ge, he = edges(gba), edges(hd)
        dx = ((ge[0] + ge[2]) - (he[0] + he[2])) / 2 / SCALE
        dy = ((ge[1] + ge[3]) - (he[1] + he[3])) / 2 / SCALE
        sw = (ge[2] - ge[0]) / max(1, he[2] - he[0])
        sh = (ge[3] - ge[1]) / max(1, he[3] - he[1])
        rows.append((iou, name, dx, dy, sw, sh))
    rows.sort()
    if args.fix:
        fix(data, syms)
    print(f'{len(rows)} sprites; IoU median {np.median([r[0] for r in rows]):.3f}')
    print(f'{"IoU":>6} {"dx":>6} {"dy":>6} {"sw":>5} {"sh":>5}  name')
    for iou, name, dx, dy, sw, sh in rows[:args.worst]:
        print(f'{iou:6.3f} {dx:6.2f} {dy:6.2f} {sw:5.2f} {sh:5.2f}  {name}')


if __name__ == '__main__' and '--fit-words' not in sys.argv:
    main()


def fit_words(hd_dir, region='eu'):
    """Battle words: the HD picture's ink on the cartridge's ink box, to the eighth of a pixel (canvas + blit)."""
    data, syms = rom(region)
    p = ROOT / 'port/data/hd_sprites.tsv'
    out, changed = [], 0
    for line in p.read_text(encoding='utf-8').split(chr(10)):
        f = line.split(chr(9))
        m = re.match(r'crop=(\d+),(\d+),(\d+),(\d+);place=(\d+),(\d+),(\d+),(\d+);scale=8$', f[9]) if len(f) > 9 else None
        if m and (f[0].startswith('gBtlPop') or f[0].startswith('gLvupLogo')):
            png = Path(hd_dir) / f'{f[0]}.png'
            frame = f[2] if f[2] != '-' else f[3][:-5] + 'Frame0'
            box = tuple(int(v) for v in f[4:8])
            if png.exists():
                cx0, cy0, cx1, cy1, px0, py0, px1, py1 = map(int, m.groups())
                gx0, gy0, gx1, gy1 = ink_box(data, syms, frame, f[3], box)
                a = np.array(Image.open(png).convert('RGBA'))[:, :, 3] > 96
                ys, xs = np.nonzero(a)
                hx0, hy0, hx1, hy1 = xs.min(), ys.min(), xs.max() + 1, ys.max() + 1
                # the picture's place spans (px0..px1)*8; its ink spans hx0..hx1 of that: map the ink onto the GBA's
                sx = (gx1 - gx0) / (hx1 - hx0)
                sy = (gy1 - gy0) / (hy1 - hy0)
                d0x = gx0 - (hx0 - px0 * SCALE) * sx
                d0y = gy0 - (hy0 - py0 * SCALE) * sy
                d1x = d0x + (px1 - px0) * SCALE * sx
                d1y = d0y + (py1 - py0) * SCALE * sy
                f[9] = (f'canvas={box[2] * SCALE},{box[3] * SCALE};blit={cx0},{cy0},{cx1},{cy1},'
                        f'{round(d0x)},{round(d0y)},{round(d1x)},{round(d1y)};scale=8')
                changed += 1
            line = chr(9).join(f)
        out.append(line)
    p.write_text(chr(10).join(out), encoding='utf-8', newline='')
    print(f'{changed} words fitted')


if __name__ == '__main__' and '--fit-words' in sys.argv:
    fit_words(sys.argv[sys.argv.index('--fit-words') + 1])
