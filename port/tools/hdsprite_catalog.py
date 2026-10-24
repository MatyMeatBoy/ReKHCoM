#!/usr/bin/env python3
"""Name and lay out the sprites a KHCOM_HDSPRITE_EXPORT run saved (port_ppu.c ExportSprites): each
<tiles address>_<frame address>.bmp gets the ROM symbols of its tiles and frame (build/<region>/pc/rom_symbols.s),
is cropped to what it draws, and goes into catalog.tsv (tiles, frame, size, box in frame coordinates) and contact
sheets for matching against Re:CoM's HD textures (HUD_HD_RECOM.md). Tiles decompressed into RAM have no symbol.

    python port/tools/hdsprite_catalog.py <export dir> [--region eu]
"""
import argparse
import bisect
import re
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]


def symbols(region):
    text = (ROOT / 'build' / region / 'pc' / 'rom_symbols.s').read_text()
    pairs = sorted((int(v, 16), k) for k, v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)', text))
    return [a for a, _ in pairs], [n for _, n in pairs]


def name(addrs, names, a):
    i = bisect.bisect_right(addrs, a) - 1
    if i >= 0 and addrs[i] == a:
        return names[i]
    return f'{a:08x}' if i < 0 or a - addrs[i] > 0x40000 else f'{names[i]}+{a - addrs[i]:#x}'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('dir')
    ap.add_argument('--region', default='eu')
    args = ap.parse_args()
    src = Path(args.dir)
    addrs, names = symbols(args.region)
    rows, cells = [], []
    for f in sorted(src.glob('*.bmp')):
        t, fr = (int(x, 16) for x in f.stem.split('_'))
        im = Image.open(f).convert('RGBA')
        box = im.getbbox()
        if not box:
            continue
        crop = im.crop(box)
        tn, fn = name(addrs, names, t), name(addrs, names, fr)
        rows.append(f'{tn}\t{fn}\t{crop.width}\t{crop.height}\t{box[0] - 128}\t{box[1] - 128}\t{f.name}\n')
        cells.append((tn, crop))
    (src / 'catalog.tsv').write_text(''.join(sorted(rows)), encoding='utf-8')
    cells.sort(key=lambda c: c[0])
    W, S, page, x, y, rowh = 1900, 2, 0, 0, 0, 0
    sheet = Image.new('RGBA', (W, 1900), (50, 50, 80, 255))
    draw = ImageDraw.Draw(sheet)
    for label, im in cells:
        w, h = im.width * S, im.height * S
        cw = max(w, 120)
        if x + cw > W:
            x, y, rowh = 0, y + rowh + 14, 0
        if y + h + 14 > sheet.height:
            sheet.save(src / f'catalog_{page}.png')
            page += 1
            sheet = Image.new('RGBA', (W, 1900), (50, 50, 80, 255))
            draw = ImageDraw.Draw(sheet)
            x, y, rowh = 0, 0, 0
        sheet.alpha_composite(im.resize((w, h), Image.NEAREST), (x, y + 12))
        draw.text((x, y), label[:24], fill=(255, 255, 0, 255))
        x += cw + 6
        rowh = max(rowh, h)
    sheet.save(src / f'catalog_{page}.png')
    print(len(rows), 'sprites,', page + 1, 'sheets ->', src)


if __name__ == '__main__':
    main()
