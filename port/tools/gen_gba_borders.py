#!/usr/bin/env python3
"""The GBA classic mode's decorative borders (gba_border 1..3), in the manner of the Super Game Boy's: 384x216 pixel
art at the GBA's pixel size with the 240x160 picture's hole in the middle (72 pixels at the sides, 28 above and
below), so a 1920x1080 window shows border and picture at x5, 3840x2160 at x10. Our own art (shapes of the game's
world: cards with their crown, hearts, Castle Oblivion's marble), in GBA colours (5 bits a channel).

Writes port/assets/borders/gba_border_<n>.png and port/include/port/gba_borders_png.inc (embedded in the game, as
the achievement badges: every platform has them).

    python port/tools/gen_gba_borders.py [--preview <game shot.bmp> <out.png>]
"""
import math
import random
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'port' / 'assets' / 'borders'
INC = ROOT / 'port' / 'include' / 'port' / 'gba_borders_png.inc'
W, H, HX, HY = 384, 216, 72, 28


def c5(r, g, b, a=255):
    """A colour the GBA can show: 5 bits a channel."""
    q = lambda v: (max(0, min(255, int(v))) >> 3) * 255 // 31
    return (q(r), q(g), q(b), a)


def lerp(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


HEART = ['.##...##.', '####.####', '#########', '#########', '.#######.', '..#####..', '...###...', '....#....']
CROWN = ['#...#...#', '#..###..#', '##.###.##', '#########', '#########', '.#######.']
CARD = ['..#.#.#..', '.#######.', '#########', '#.......#', '#.......#', '#.......#', '#.......#', '#.......#',
        '#.......#', '#.....###', '#....####', '#########']


def stamp(d, pattern, x, y, col, scale=1):
    for j, row in enumerate(pattern):
        for i, ch in enumerate(row):
            if ch == '#':
                d.rectangle([x + i * scale, y + j * scale, x + (i + 1) * scale - 1, y + (j + 1) * scale - 1], fill=col)


def hole_frame(d, outer, inner, light, dark):
    """Bevelled frame around the picture's hole."""
    d.rectangle([HX - 4, HY - 4, HX + 240 + 3, HY + 160 + 3], fill=outer)
    d.rectangle([HX - 3, HY - 3, HX + 240 + 2, HY + 160 + 2], fill=light)
    d.rectangle([HX - 2, HY - 2, HX + 240 + 2, HY + 160 + 2], fill=dark)
    d.rectangle([HX - 2, HY - 2, HX + 240 + 1, HY + 160 + 1], fill=inner)


def border_oblivion():
    """Castle Oblivion: white marble, silver veins, hearts and crowns in the corners, a row of card suits."""
    img = Image.new('RGBA', (W, H))
    px = img.load()
    rnd = random.Random(7)
    top, bottom = (232, 232, 240), (196, 200, 214)
    for y in range(H):
        for x in range(W):
            t = y / (H - 1)
            base = lerp(top, bottom, t)
            vein = 0.5 + 0.5 * math.sin(x * 0.045 + y * 0.09 + 3 * math.sin(y * 0.05 + x * 0.013))
            v = -14 * (vein > 0.94) - 6 * (vein > 0.86)
            dither = ((x + y) & 1) * 3
            px[x, y] = c5(base[0] + v + dither, base[1] + v + dither, base[2] + v + dither)
    d = ImageDraw.Draw(img)
    silver, dark, gold = c5(150, 156, 176), c5(72, 76, 100), c5(208, 176, 88)
    d.rectangle([2, 2, W - 3, H - 3], outline=silver)
    d.rectangle([4, 4, W - 5, H - 5], outline=c5(176, 182, 200))
    hole_frame(d, dark, c5(0, 0, 0), c5(236, 236, 248), silver)
    for cx, cy in ((22, 18), (W - 22, 18), (22, H - 26), (W - 22, H - 26)):
        stamp(d, HEART, cx - 9, cy - 4, c5(120, 128, 156), 2)
        stamp(d, HEART, cx - 9 + 1, cy - 4 + 1, silver, 2)
        stamp(d, CROWN, cx - 4, cy - 15, gold, 1)
    for i, x in enumerate(range(HX + 20, HX + 240 - 10, 24)):
        stamp(d, HEART if i % 2 else CROWN, x, H - 18, c5(168, 172, 192), 1)
        stamp(d, HEART if i % 2 else CROWN, x, 9, c5(168, 172, 192), 1)
    # the sides get three of the game's own map cards each, drawn from the player's ROM when the game makes the
    # border (port_main.c, BorderMapCards): nothing of the game ships here
    return img


def border_cards():
    """Cards in the night: a deep blue sky, stars, and the battle cards' colours scattered around the picture."""
    img = Image.new('RGBA', (W, H))
    px = img.load()
    top, bottom = (16, 16, 56), (48, 32, 96)
    for y in range(H):
        for x in range(W):
            base = lerp(top, bottom, y / (H - 1))
            px[x, y] = c5(*base)
    d = ImageDraw.Draw(img)
    rnd = random.Random(3)
    for _ in range(140):
        x, y = rnd.randrange(W), rnd.randrange(H)
        if HX - 6 <= x <= HX + 246 and HY - 6 <= y <= HY + 166:
            continue
        b = rnd.choice((120, 176, 232))
        d.point((x, y), fill=c5(b, b, min(255, b + 24)))
    colours = [c5(232, 72, 64), c5(64, 192, 96), c5(80, 128, 240), c5(168, 168, 176)]
    spots = [(8, 10), (36, 60), (12, 112), (40, 160), (14, 190), (W - 30, 14), (W - 58, 66), (W - 26, 118),
             (W - 56, 168), (W - 28, 192), (100, 4), (W - 120, 4), (150, H - 22), (W - 170, H - 22)]
    for i, (x, y) in enumerate(spots):
        col = colours[i % 4]
        stamp(d, CARD, x + 1, y + 1, c5(8, 8, 24), 2)
        stamp(d, CARD, x, y, col, 2)
        d.rectangle([x + 2, y + 6, x + 15, y + 17], fill=c5(24, 40, 136))
        stamp(d, HEART, x + 4, y + 8, c5(200, 208, 232), 1)
    hole_frame(d, c5(8, 8, 24), c5(0, 0, 0), c5(200, 176, 96), c5(120, 96, 40))
    return img


def border_classic():
    """The Super Game Boy's own manner: a frame of horizontal bands, bevels, crowns in the corners."""
    img = Image.new('RGBA', (W, H))
    d = ImageDraw.Draw(img)
    bands = [c5(64, 56, 96), c5(72, 64, 108)]
    for y in range(H):
        d.line([(0, y), (W - 1, y)], fill=bands[(y // 4) % 2])
    d.rectangle([0, 0, W - 1, H - 1], outline=c5(32, 24, 56))
    d.rectangle([6, 6, W - 7, H - 7], outline=c5(144, 136, 184))
    d.rectangle([7, 7, W - 8, H - 8], outline=c5(40, 32, 72))
    hole_frame(d, c5(24, 16, 48), c5(0, 0, 0), c5(176, 168, 208), c5(48, 40, 80))
    gold = c5(216, 184, 88)
    for cx, cy in ((24, 16), (W - 24, 16), (24, H - 22), (W - 24, H - 22)):
        stamp(d, CROWN, cx - 9, cy - 6, c5(32, 24, 56), 2)
        stamp(d, CROWN, cx - 9 - 1, cy - 6 - 1, gold, 2)
    for x in range(HX + 8, HX + 240 - 8, 16):
        stamp(d, HEART, x, 12, c5(112, 104, 152), 1)
        stamp(d, HEART, x, H - 18, c5(112, 104, 152), 1)
    return img


BORDERS = [border_oblivion, border_cards, border_classic]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    blobs = []
    for i, make in enumerate(BORDERS, 1):
        img = make()
        # the hole stays transparent: the game's picture shows through
        ImageDraw.Draw(img).rectangle([HX, HY, HX + 239, HY + 159], fill=(0, 0, 0, 0))
        path = OUT / f'gba_border_{i}.png'
        img.save(path, optimize=True)
        blobs.append(path.read_bytes())
    lines = ['/* Generated by port/tools/gen_gba_borders.py: the GBA classic borders (PNG), do not edit. */\n']
    for i, b in enumerate(blobs, 1):
        lines.append(f'static const unsigned char kGbaBorder{i}[{len(b)}] = {{\n')
        for k in range(0, len(b), 24):
            lines.append('    ' + ', '.join(str(v) for v in b[k:k + 24]) + ',\n')
        lines.append('};\n')
    lines.append('static const unsigned char* const kGbaBorders[] = { ' + ', '.join(f'kGbaBorder{i}' for i in range(1, len(blobs) + 1)) + ' };\n')
    lines.append('static const unsigned kGbaBorderSizes[] = { ' + ', '.join(str(len(b)) for b in blobs) + ' };\n')
    INC.write_text(''.join(lines), encoding='utf-8', newline='\n')
    print(INC, [len(b) for b in blobs])
    if len(sys.argv) > 3 and sys.argv[1] == '--preview':
        shot = Image.open(sys.argv[2]).convert('RGBA')
        game = shot.crop(shot.getbbox()).resize((240, 160), Image.NEAREST) if shot.size != (240, 160) else shot
        sheet = Image.new('RGBA', (W * 2 + 8, H * 2 * 3 + 16), (0, 0, 0, 255))
        for i in range(len(BORDERS)):
            b = Image.open(OUT / f'gba_border_{i + 1}.png').convert('RGBA')
            full = Image.new('RGBA', (W, H), (0, 0, 0, 255))
            full.paste(game, (HX, HY))
            full.alpha_composite(b)
            sheet.paste(full.resize((W * 2, H * 2), Image.NEAREST), (0, i * (H * 2 + 8)))
        sheet.save(sys.argv[3])


if __name__ == '__main__':
    main()
