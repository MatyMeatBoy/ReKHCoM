#!/usr/bin/env python3
"""Recipe lines (port/data/hd_sprites.tsv) for the HD card borders (com_card01 cells) and value digits (card_no) of
Re:CoM: each HD frame's alpha box lands on the GBA border's ink box (the GBA card is stubbier: the fit stretches a
little), each white digit glyph on the GBA digit's ink box (modified values tinted the GBA's blue). Writes the lines
and a review sheet (GBA above, HD below).

    python port/tools/hdcards_parts.py <gba sprites dir> <Re:CoM cards_png dir> <out dir>
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

# HD cells: (texture, x0, y0, x1, y1, rotate) — rotate 270 turns the sideways grey/gold cells crown-up
CELLS = {
    'Red': ('com_card01', 2, 6, 187, 273, 0),
    'Green': ('com_card01', 2, 278, 187, 545, 0),
    'Blue': ('com_card01', 2, 550, 187, 821, 0),
    'Black': ('com_card01', 750, 187, 1017, 373, 270),
}
# card_no: the ten white glyphs on top (italic, so their boxes touch; found as connected alpha components)
GLYPHS = [(0, 0, 64, 64), (78, 0, 119, 64), (126, 0, 190, 64), (191, 0, 253, 64), (260, 0, 318, 64),
          (319, 0, 381, 64), (382, 0, 447, 64), (450, 0, 506, 64), (510, 0, 573, 64), (574, 0, 637, 64)]


def main():
    gdir, hdir, out = map(Path, sys.argv[1:4])
    out.mkdir(parents=True, exist_ok=True)
    rows = {r.split('\t')[0]: r.split('\t') for r in (gdir / 'sprites.tsv').read_text(encoding='utf-8').splitlines()}
    lines, cells = [], []
    for colour, (tex, x0, y0, x1, y1, rot) in CELLS.items():
        name = f'gCardBorder{colour}Tiles'
        r = rows[name]
        g = Image.open(gdir / f'{name}.png')
        hd = Image.open(hdir / f'{tex}.png').crop((x0, y0, x1, y1))
        if rot:
            hd = hd.rotate(rot, expand=True)
        # the whole HD frame (its alpha box) on the GBA frame's ink box: same crown, window and value circle
        b = hd.getchannel('A').point(lambda v: 255 if v > 40 else 0).getbbox()
        ix0, iy0, ix1, iy1 = g.getbbox()
        cost = 0
        lines.append(f'{name}\t1\t-\t{name}\t{r[4]}\t{r[5]}\t{r[6]}\t{r[7]}\tCARD/{"1000/CA1000" if tex == "com_card01" else "1001/CA1001"}'
                     f'.RTM:{tex}_tm2.dds\tcrop={x0},{y0},{x1},{y1};rot={rot};crop={b[0]},{b[1]},{b[2]},{b[3]};'
                     f'place={ix0},{iy0},{ix1},{iy1};scale=8\n')
        cells.append((name, g, hd.crop(tuple(b)), cost))
    glyphs = Image.open(hdir / 'card_no.png')
    for d in range(10):
        # the white glyph of digit d, its ink box; placed on the GBA digit's own ink box (the GBA's sprite is 8x8)
        gx0, gy0, gx1, gy1 = GLYPHS[d]
        for kind in ('gCardValueDigitTiles', 'gCardModifiedValueDigitTiles'):
            name = f'{kind}_{d}'
            r = rows[name]
            g = Image.open(gdir / f'{name}.png')
            ix0, iy0, ix1, iy1 = g.getbbox()
            tint = 'tint=297BFF;' if 'Modified' in kind else ''
            lines.append(f'{name}\t1\t{r[1]}\t{kind}\t{r[4]}\t{r[5]}\t{r[6]}\t{r[7]}\tCARD/9002/CA9002.RTM:card_no_tm2.dds'
                         f'\tcrop={gx0},{gy0},{gx1},{gy1};solo;{tint}place={ix0},{iy0},{ix1},{iy1};scale=8\n')
            canvas = Image.new('RGBA', (g.width * 8, g.height * 8))
            canvas.alpha_composite(glyphs.crop((gx0, gy0, gx1, gy1)).resize(((ix1 - ix0) * 8, (iy1 - iy0) * 8), Image.LANCZOS),
                                   (ix0 * 8, iy0 * 8))
            cells.append((name, g, canvas, 0))
    (out / 'parts_recipe.tsv').write_text(''.join(lines), encoding='utf-8')
    S = 8
    sheet = Image.new('RGBA', (len(cells) * 2 * 40 * S // 4 + 40, 50 * S // 2 + 40), (60, 60, 60, 255))
    x = 0
    d = ImageDraw.Draw(sheet)
    for name, g, crop, cost in cells:
        gw, gh = g.width * S // 2, g.height * S // 2
        sheet.alpha_composite(g.resize((gw, gh), Image.NEAREST), (x, 20))
        sheet.alpha_composite(crop.resize((gw, gh), Image.LANCZOS), (x, 30 + gh))
        x += gw + 6
    sheet.save(out / 'parts_fit.png')
    print(''.join(lines))


if __name__ == '__main__':
    main()
