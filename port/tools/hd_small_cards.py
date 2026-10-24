#!/usr/bin/env python3
"""Rows of port/data/hd_sprites.tsv for the menus' small cards (group 6: the deck, the map cards, the status' allies):
each cartridge small card (16x32: crown, border, a 14x15 picture window, the black notch of its value at the bottom
right) keeps its own pixels, and Re:CoM's picture of the same card (the big card's texture) covers only the window,
without the notch. The small card shows a part of the big card's drawing at its own zoom: the crop is the one whose
colours, brought down to the window's 14x15 pixels, are nearest to the cartridge's (cards with the same picture, the
attack and its sleights, share it).

    python port/tools/hd_small_cards.py <Recom.hed> [--write] [--only gDonaldDuckSmallCard,...] [--sheet out.png]
"""
import re
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import khhd_pkg  # noqa: E402
from hd_menu_titles import frame_pixels  # noqa: E402
from khhd_sprites import decode  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
TSV = ROOT / 'port/data/hd_sprites.tsv'
CATALOG = ROOT / 'src/card/card_catalog.c'
NAMES = 'D:/01Claude/ReKHCoM-build/refs/OpenKh/OpenKh.Egs/resources/recom.txt'
K = 8
WIN = (1, 16, 15, 31)  # the picture window in the sprite's box (corners, GBA pixels)
# the value's black notch, in window pixels (x0, y0, x1, y1), stepped as the cartridge draws it
NOTCH = [(10, 8, 14, 9), (9, 9, 14, 10), (8, 10, 14, 14), (9, 14, 14, 15)]


def pairs():
    """{small card name: (big card's tiles base, small frame, small tiles, small palette)} from the card catalogue."""
    text = CATALOG.read_text(encoding='utf-8')
    found = re.findall(r'g\w+Frame0, (g\w+)Tiles, g\w+Palette, LOCALIZED\(\w+\), ((g\w+)SmallCardFrame0), (g\w+), (g\w+),',
                       text)
    return {small: (big, frame, tiles, palette) for big, frame, small, tiles, palette in found}


def window(data, syms, frame, tiles, palette):
    """The cartridge window as RGB float (15, 14, 3) and its mask (the notch and empty pixels left out)."""
    px, box = frame_pixels(data, syms, frame, tiles)
    po = syms[palette] - 0x08000000
    pal = [struct.unpack_from('<H', data, po + 2 * i)[0] for i in range(16)]
    w, h = WIN[2] - WIN[0], WIN[3] - WIN[1]
    rgb, mask = np.zeros((h, w, 3)), np.ones((h, w), bool)
    for y in range(h):
        for x in range(w):
            v = px.get((box[0] + WIN[0] + x, box[1] + WIN[1] + y), 0)
            c = pal[v]
            rgb[y, x] = ((c & 31) * 8, ((c >> 5) & 31) * 8, ((c >> 10) & 31) * 8)
            mask[y, x] = v != 0
    for x0, y0, x1, y1 in NOTCH:
        mask[y0:y1, x0:x1] = False
    return rgb, mask, box


def best_crop(tex, rgb, mask):
    """(x0, y0, x1, y1) of the texture whose colours fit the window best."""
    h, w = rgb.shape[:2]
    a = tex[:, :, 3:4] / 255.0
    flat = tex[:, :, :3] * a  # over black, as the cartridge's dark edges
    th, tw = flat.shape[:2]
    best = (1e18, None)
    for k in np.arange(9.0, 18.5, 0.25):  # the small cards zoom the drawing 11-16 texture pixels a GBA pixel
        for phase in (0.0, 0.5):
            off = int(round(phase * k))
            src = flat[off:, off:]
            dw, dh = int(src.shape[1] // k), int(src.shape[0] // k)
            if dw < w or dh < h:
                continue
            small = np.asarray(Image.fromarray(src[:int(dh * k), :int(dw * k)].astype(np.uint8)).resize(
                (dw, dh), Image.BOX), dtype=float)
            win = np.lib.stride_tricks.sliding_window_view(small, (h, w, 3))[:, :, 0]
            d = ((win - rgb) ** 2).sum(-1)
            score = (d * mask).sum((-1, -2)) / mask.sum()
            j = np.unravel_index(np.argmin(score), score.shape)
            if score[j] < best[0]:
                x0, y0 = off + j[1] * k, off + j[0] * k
                best = (score[j], (int(round(x0)), int(round(y0)), int(round(x0 + w * k)), int(round(y0 + h * k))))
    return best


def main():
    hed = Path(sys.argv[1])
    only = None
    if '--only' in sys.argv:
        only = set(sys.argv[sys.argv.index('--only') + 1].split(','))
    data = (ROOT / 'roms/B8CP.gba').read_bytes()
    syms = {k: int(v, 16) for k, v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)',
                                                 (ROOT / 'build/eu/pc/rom_symbols.s').read_text())}
    rows_by_tiles = {}
    for line in TSV.read_text(encoding='utf-8').splitlines():
        f = line.split('\t')
        if len(f) > 9 and f[1] == '1':
            rows_by_tiles.setdefault(f[3], f)
    names = khhd_pkg.names_by_md5(NAMES)
    where = {names.get(m, m): o for m, o, _, _ in khhd_pkg.entries(hed)}
    textures = {}
    rows, sheet = [], []
    with open(hed.with_suffix('.pkg'), 'rb') as pkg:
        for small, (big, frame, tiles, palette) in sorted(pairs().items()):
            if only and small not in only:
                continue
            big_row = rows_by_tiles.get(big + 'Tiles')
            if big_row is None:
                print('no HD big card for', small, file=sys.stderr)
                continue
            asset, texname = big_row[8].split(':')
            index = 0
            if '#' in texname:
                texname, index = texname.split('#')
                index = int(index)
            if big_row[8] not in textures:
                textures[big_row[8]] = decode(khhd_pkg.read_asset(pkg, where[asset], with_remastered=True)[2][texname],
                                              index)
            tex = textures[big_row[8]].astype(float)
            rgb, mask, box = window(data, syms, frame, tiles, palette)
            score, crop = best_crop(tex, rgb, mask)
            k = (crop[2] - crop[0]) / (WIN[2] - WIN[0])
            erase = ';'.join('erase=%d,%d,%d,%d' % (round(x0 * k), round(y0 * k), round(x1 * k), round(y1 * k))
                             for x0, y0, x1, y1 in NOTCH)
            ops = f'crop={",".join(map(str, crop))};{erase};place={",".join(map(str, WIN))};scale={K}'
            rows.append('\t'.join([small + 'SmallCardHD', '6', frame, tiles,
                                   str(box[0]), str(box[1]), str(box[2]), str(box[3]), big_row[8], ops]))
            print(f'{small:28s} score {score:8.0f} crop {crop}', file=sys.stderr)
            if '--sheet' in sys.argv:
                cut = np.clip(tex[max(crop[1], 0):crop[3], max(crop[0], 0):crop[2]], 0, 255).astype(np.uint8)
                hd = Image.fromarray(cut).resize((14 * 6, 15 * 6))
                gba = Image.fromarray(rgb.astype(np.uint8)).resize((14 * 6, 15 * 6), Image.NEAREST)
                pair = Image.new('RGB', (14 * 12 + 4, 15 * 6))
                pair.paste(gba, (0, 0))
                pair.paste(hd.convert('RGB'), (14 * 6 + 4, 0))
                sheet.append(pair)
    if sheet:
        cols = 8
        out = Image.new('RGB', (cols * (sheet[0].width + 6), ((len(sheet) + cols - 1) // cols) * (sheet[0].height + 6)))
        for i, s in enumerate(sheet):
            out.paste(s, ((i % cols) * (s.width + 6), (i // cols) * (s.height + 6)))
        out.save(sys.argv[sys.argv.index('--sheet') + 1])
    if '--write' not in sys.argv:
        print('\n'.join(rows))
        return
    keep = [l for l in TSV.read_text(encoding='utf-8').splitlines() if not re.match(r'g\w+SmallCardHD\t', l)]
    TSV.write_text('\n'.join(keep + rows) + '\n', encoding='utf-8')
    print(f'{len(rows)} small card rows in {TSV.relative_to(ROOT)}')


if __name__ == '__main__':
    main()
