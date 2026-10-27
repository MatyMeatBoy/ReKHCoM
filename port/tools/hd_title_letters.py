#!/usr/bin/env python3
"""Letters of Re:CoM's title words (FORM/0505/FO0505.RTM <lang>_title.imz #4, decoded by khhd_textures.py) for the
HD title's recipes (port/data/hd_sprites.tsv): each word's line is cut at its empty columns into letters, matched
to the word's text, so other words (PULSA START, BRILLO, SORA...) are composed with sblit= letter by letter.

    python port/tools/hd_title_letters.py <decoded textures dir>   (prints letter -> texture, rect)
"""
import sys
from pathlib import Path

from PIL import Image

# the grey words of each texture (top to bottom, right column) and their rows (y0, y1)
WORDS = {
    'SP': [('NUEVAPARTIDA', 30, 64), ('CARGARPARTIDA', 96, 124), ('ESCENAS', 162, 190), ('ATRAS', 228, 256)],
}


def letters(png, text, y0, y1, x0=500, x1=1024):
    a = Image.open(png).convert('RGBA').split()[3]
    cols = [any(a.getpixel((x, y)) > 40 for y in range(y0, y1)) for x in range(x0, x1)]
    runs, x = [], 0
    while x < len(cols):
        if cols[x]:
            s = x
            while x < len(cols) and cols[x]:
                x += 1
            runs.append((s + x0, x + x0))
        x += 1
    return list(zip(text, runs)), len(runs)


def main():
    folder = Path(sys.argv[1])
    for lang, words in WORDS.items():
        png = next(folder.glob(f'*{lang}_title_4.png'))
        for text, y0, y1 in words:
            pairs, n = letters(png, text, y0, y1)
            print(lang, text, 'runs', n, ' '.join(f'{c}:{a}-{b}' for c, (a, b) in pairs))


if __name__ == '__main__':
    main()
