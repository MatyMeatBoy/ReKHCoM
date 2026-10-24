#!/usr/bin/env python3
"""Draw the GBA card sprites straight from the ROM (frame layout + raw 4bpp tiles + palette, by symbol), in each
sprite's own local box: the 99 battle illustrations of gCardDefs, the category borders, outlines and backs, and the
value digits (normal and modified). The reference the HD cards are fitted to (hdcards_fit.py).

    python port/tools/hdcards_gba.py <repo> <out dir>      -> <out>/<tiles symbol>.png + sprites.tsv
"""
import re
import struct
import sys
from pathlib import Path

from PIL import Image

SHAPES = {(0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64), (1, 0): (16, 8), (1, 1): (32, 8),
          (1, 2): (32, 16), (1, 3): (64, 32), (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64)}


class Rom:
    def __init__(self, repo, region='eu', rom='B8CP.gba'):
        self.rom = (repo / 'roms' / rom).read_bytes()
        text = (repo / 'build' / region / 'pc' / 'rom_symbols.s').read_text()
        self.syms = {k: int(v, 16) for k, v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)', text)}

    def read(self, sym, n, skip=0):
        o = self.syms[sym] - 0x08000000 + skip
        return self.rom[o:o + n]

    def pieces(self, frame, index=0):
        """OAM pieces of a frame: (x, y, w, h, tile, palette bank, hflip, vflip)."""
        head = self.read(frame, 2 + 6 * 64)
        n = struct.unpack_from('<H', head)[0]
        out = []
        for k in range(n):
            a0, a1, a2 = struct.unpack_from('<HHH', head, 2 + k * 6)
            w, h = SHAPES[(a0 >> 14, a1 >> 14)]
            x = a1 & 0x1FF
            x -= 512 if x & 0x100 else 0
            y = a0 & 0xFF
            y -= 256 if y & 0x80 else 0
            out.append((x, y, w, h, a2 & 0x3FF, a2 >> 12, (a1 >> 12) & 1, (a1 >> 13) & 1))
        return out

    def render(self, frame, tiles, palette):
        ps = self.pieces(frame)
        x0, y0 = min(p[0] for p in ps), min(p[1] for p in ps)
        w = max(p[0] + p[2] for p in ps) - x0
        h = max(p[1] + p[3] for p in ps) - y0
        img = Image.new('RGBA', (w, h))
        px = img.load()
        for x, y, pw, ph, t, bank, hf, vf in ps:
            raw = self.read(palette, 32, bank * 32) if palette else None
            cols = []
            for i in range(16):
                c = struct.unpack_from('<H', raw, i * 2)[0]
                cols.append(((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31,
                             0 if i == 0 else 255))
            data = self.read(tiles, (t + (pw // 8) * (ph // 8)) * 32)
            for ty in range(ph // 8):
                for tx in range(pw // 8):
                    tile = data[(t + ty * (pw // 8) + tx) * 32:][:32]
                    for yy in range(8):
                        for xx in range(8):
                            b = tile[yy * 4 + xx // 2]
                            v = (b >> 4) if xx & 1 else b & 15
                            sx, sy = tx * 8 + xx, ty * 8 + yy
                            if hf:
                                sx = pw - 1 - sx
                            if vf:
                                sy = ph - 1 - sy
                            if v:
                                px[x - x0 + sx, y - y0 + sy] = cols[v]
        return img, (x0, y0)


def catalog(repo):
    """(frame, tiles, palette) of every battle illustration and small card in gCardDefs."""
    text = (repo / 'src/card/card_catalog.c').read_text(encoding='utf-8')
    seen = []
    for m in re.finditer(r'^\s+(g\w+Frame0), (g\w+Tiles), (g\w+Palette), LOCALIZED\(\w+\), (g\w+Frame0), (g\w+Tiles), '
                         r'(g\w+Palette),', text, re.M):
        for trip in (m.group(1, 2, 3), m.group(4, 5, 6)):
            if trip not in seen:
                seen.append(trip)
    return seen


def main():
    repo, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    rom = Rom(repo)
    sprites = catalog(repo)
    for colour in ('Red', 'Blue', 'Green', 'Black', 'White'):
        for part in ('Border', 'Outline', 'Back'):
            sprites.append((f'gCard{part}{colour}Frame0', f'gCard{part}{colour}Tiles', 'gCard00Palette'))
    sprites.append(('gEnemyCardBackFrame0', 'gEnemyCardBackTiles', 'gCard00Palette'))
    for d in range(10):
        sprites.append((f'gCardValueDigitFrame{d}', 'gCardValueDigitTiles', 'gCard00Palette'))
        sprites.append((f'gCardValueDigitFrame{d}', 'gCardModifiedValueDigitTiles', 'gBStatesPalette'))
    rows = []
    for frame, tiles, pal in sprites:
        if not all(s in rom.syms for s in (frame, tiles, pal)):
            print('missing', frame, tiles, pal)
            continue
        img, (x0, y0) = rom.render(frame, tiles, pal)
        name = tiles if not frame.startswith('gCardValueDigitFrame') else f'{tiles}_{frame[-1]}'
        img.save(out / f'{name}.png')
        rows.append(f'{name}\t{frame}\t{tiles}\t{pal}\t{x0}\t{y0}\t{img.width}\t{img.height}\n')
    (out / 'sprites.tsv').write_text(''.join(rows), encoding='utf-8')
    print(len(rows), 'sprites ->', out)


if __name__ == '__main__':
    main()
