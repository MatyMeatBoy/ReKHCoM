"""Every distinct GBA card illustration (cardDef->gfx: frame + raw 4bpp tiles + palette from the ROM) next to its name."""
import re, struct, sys
from pathlib import Path
from PIL import Image, ImageDraw
REPO = Path(sys.argv[1]); OUT = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("D:/01Claude/ReKHCoM-build/hdhud")
rom = (REPO / "roms/B8CP.gba").read_bytes()
syms = dict(re.findall(r"\.set _(\w+), (0x[0-9a-f]+)", (REPO / "build/eu/pc/rom_symbols.s").read_text()))
SHAPES = {(0,0):(8,8),(0,1):(16,16),(0,2):(32,32),(0,3):(64,64),(1,0):(16,8),(1,1):(32,8),(1,2):(32,16),(1,3):(64,32),(2,0):(8,16),(2,1):(8,32),(2,2):(16,32),(2,3):(32,64)}
def rd(a, n): o = int(a, 16) - 0x08000000; return rom[o:o + n]
def render(base):
    fr, ti, pa = syms.get(base + "Frame0"), syms.get(base + "Tiles"), syms.get(base + "Palette")
    if not (fr and ti and pa): return None
    f = rd(fr, 64); n = struct.unpack_from("<H", f)[0]
    pal = [struct.unpack_from("<H", rd(pa, 32), i * 2)[0] for i in range(16)]
    cols = [((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31, 0 if i == 0 else 255) for i, c in enumerate(pal)]
    pieces = []
    for k in range(n):
        a0, a1, a2 = struct.unpack_from("<HHH", f, 2 + k * 6)
        w, h = SHAPES[(a0 >> 14, a1 >> 14)]
        x = a1 & 0x1FF; x -= 512 if x & 0x100 else 0
        y = a0 & 0xFF; y -= 256 if y & 0x80 else 0
        pieces.append((x, y, w, h, a2 & 0x3FF))
    x0 = min(p[0] for p in pieces); y0 = min(p[1] for p in pieces)
    W = max(p[0] + p[2] for p in pieces) - x0; H = max(p[1] + p[3] for p in pieces) - y0
    img = Image.new("RGBA", (W, H))
    tiles = rd(ti, 32 * 256)
    for x, y, w, h, t in pieces:
        for ty in range(h // 8):
            for tx in range(w // 8):
                tile = tiles[(t + ty * (w // 8) + tx) * 32:][:32]
                for py in range(8):
                    for px in range(8):
                        b = tile[py * 4 + px // 2]; v = (b >> 4) if px & 1 else b & 15
                        img.putpixel((x - x0 + tx * 8 + px, y - y0 + ty * 8 + py), cols[v])
    return img
rows = [l.split("\t") for l in (OUT / "gba_cards.tsv").read_text().splitlines()]
cells = []
for r in rows:
    base = r[1][:-len("Frame0")]
    im = render(base)
    if im: cells.append((r[4].replace("CARD_KIND_", "")[:16] + " " + r[5].replace("CARD_CATEGORY_", "")[:3], im))
    else: print("no gfx", r)
S = 4; C = 10; cw = 34 * S
sheet = Image.new("RGBA", (C * cw, ((len(cells) + C - 1) // C) * (cw + 14)), (50, 50, 50, 255))
d = ImageDraw.Draw(sheet)
for i, (t, im) in enumerate(cells):
    x, y = (i % C) * cw, (i // C) * (cw + 14)
    sheet.alpha_composite(im.resize((im.width * S, im.height * S), Image.NEAREST), (x, y + 14))
    d.text((x + 2, y + 1), t, fill=(255, 255, 0, 255))
sheet.save(OUT / "gba_cards.png"); print(len(cells), "cards", cells[0][1].size)
