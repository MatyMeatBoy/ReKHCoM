"""Brick Launcher's platinum trophy figure: Sora from the HD title art, cut out and reduced to pixel art.

Brick turns each pixel into a voxel, so the figure is kept small (64 px wide). Replace the output with a hand-made
sprite any time; Brick only needs a PNG with a transparent background.

    python port/tools/make_brick_platinum.py
"""
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'port/assets/hd/title/art/sora.png'
OUT = ROOT / 'port/assets/brick-link/platinum.png'
SORA = (170, 40, 780, 1180)  # Sora, in the title art's pixels
WIDTH = 64


def keep_largest(image):
    """Only the biggest connected shape stays (Sora), so no stray specks of the poster become floating voxels."""
    px = image.load()
    w, h = image.size
    seen, best = set(), []
    for sy in range(h):
        for sx in range(w):
            if (sx, sy) in seen or px[sx, sy][3] == 0:
                continue
            shape, stack = [], [(sx, sy)]
            seen.add((sx, sy))
            while stack:
                x, y = stack.pop()
                shape.append((x, y))
                for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                    if 0 <= nx < w and 0 <= ny < h and (nx, ny) not in seen and px[nx, ny][3]:
                        seen.add((nx, ny))
                        stack.append((nx, ny))
            if len(shape) > len(best):
                best = shape
    keep = set(best)
    for y in range(h):
        for x in range(w):
            if (x, y) not in keep:
                px[x, y] = (0, 0, 0, 0)


def main():
    art = Image.open(SOURCE).convert('RGBA').crop(SORA)
    # The poster's other characters sit behind Sora in greys and pale tones; Sora is saturated (red, orange,
    # yellow, brown) with dark outlines. Keep those, drop the rest.
    import colorsys
    px = art.load()
    for y in range(art.height):
        for x in range(art.width):
            r, g, b, a = px[x, y]
            if not a:
                continue
            h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
            if s < 0.35 and l > 0.18:
                px[x, y] = (0, 0, 0, 0)
    art = art.crop(art.getbbox())
    height = round(art.height * WIDTH / art.width)
    small = art.resize((WIDTH, height), Image.LANCZOS)
    # hard edges: a pixel is either there or not, so every voxel is a full block
    pixels = small.load()
    for y in range(small.height):
        for x in range(small.width):
            r, g, b, a = pixels[x, y]
            pixels[x, y] = (r, g, b, 255 if a >= 128 else 0)
    keep_largest(small)
    small = small.crop(small.getbbox())
    OUT.parent.mkdir(parents=True, exist_ok=True)
    small.save(OUT, optimize=True)
    print(OUT, small.size)


if __name__ == '__main__':
    main()
