#!/usr/bin/env python3
"""Darkside's missing right half (port/src/port_darkside.c): pixel art from a reference picture, per pose.

The cartridge stores every body frame of Darkside (gBosDsdFrame*Map, BG1, palette bank 0) only up to its map block's
edge (x 256): the wide view showed him cut there. For each pose, a reference picture of the whole Darkside (the user
gives it, e.g. made with Gemini from the game's frame) is turned into the game's pixel art:

  1. aligned on the frame: the yellow eye anchors it, then scale and position are searched by silhouette over the
     head and chest the frame does have (x 150..247);
  2. its tones are mapped onto the body's ramp (palette indices 1-11) by matching their distribution where both
     exist, with a 2x2 ordered dither between neighbouring steps (the game's gradients);
  3. the game's outline (index 1) where the whole silhouette (frame + new piece) meets the outside;
  4. kept: everything right of the seam, and left of it only the reference's body that touches the seam (arms and
     fists the frame leaves out), drawn in the game only where the frame's own pixel is transparent.

Writes port/include/port/darkside_body_<pose>.inc (indices, two a byte) and a preview PNG. Source tiles and maps
are read from the user's ROM; the .inc holds only the new pixels.

    uv run --with numpy --with pillow python port/tools/gen_darkside_body.py --pose idle --ref <picture.png>
"""
import argparse
import re
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
# pose name -> animation slots (gBosDsdFrameMaps rows) that share its drawing
POSES = {
    "idle": [0, 1, 2, 3, 4],
    "lean": [8],
    "claw": [9],
    "reach": [10],
    "punch": [13, 21, 22, 23, 24, 25, 26, 27, 28],
    "crouch": [36, 37, 38, 39, 40],
    "raise": [41, 42, 43, 44, 45],
}
X0, W = 232, 72  # the new pixels: columns X0..X0+W-1 of the frame's block


class Rom:
    def __init__(self, region="eu"):
        txt = (ROOT / "build" / region / "pc" / "rom_symbols.s").read_text()
        self.syms = {m.group(1): int(m.group(2), 16) for m in re.finditer(r"\.set _(\w+), 0x([0-9a-f]+)", txt)}
        self.addrs = sorted(set(self.syms.values()))
        code = {"eu": "B8CP", "us": "B8CE", "jp": "B8CJ"}[region]
        self.rom = (ROOT / "roms" / (code + ".gba")).read_bytes()
        src = (ROOT / "src/bos/bos_dsd_main.c").read_text()
        body = re.search(r"gBosDsdFrameMaps\[47\]\[4\] = \{(.*?)\};", src, re.S).group(1)
        self.maps = [[t.strip() for t in row.split(",")][2] for row in re.findall(r"\{([^}]*)\}", body)]
        body = re.search(r"gBosDsdFrameTiles\[47\] = \{(.*?)\};", src, re.S).group(1)
        self.tiles = [t.strip() for t in body.split(",") if t.strip()]

    def get(self, name):
        a = self.syms[name]
        size = self.addrs[self.addrs.index(a) + 1] - a
        return self.rom[a - 0x08000000:a - 0x08000000 + size]

    def palette(self):
        pal = self.get("gBosDsdBgPalette")
        return [((c & 31) << 3, (c >> 5 & 31) << 3, (c >> 10 & 31) << 3)
                for c in (int.from_bytes(pal[i:i + 2], "little") for i in range(0, 32, 2))]

    def frame(self, slot):
        """palette indices of the slot's 256x256 block (0 transparent)"""
        m, t = self.get(self.maps[slot]), self.get(self.tiles[slot])
        out = np.zeros((256, 256), int)
        for ty in range(32):
            for tx in range(32):
                e = int.from_bytes(m[(ty * 32 + tx) * 2:(ty * 32 + tx) * 2 + 2], "little")
                n = e & 1023
                for py in range(8):
                    for px in range(8):
                        sx = 7 - px if e & 0x400 else px
                        sy = 7 - py if e & 0x800 else py
                        o = n * 32 + sy * 4 + sx // 2
                        out[ty * 8 + py, tx * 8 + px] = (t[o] >> ((sx & 1) * 4)) & 15 if o < len(t) else 0
        return out


def eye(F):
    ys, xs = np.nonzero((F == 12) | (F == 13))
    return xs.mean(), ys.mean()


def register(F, ref):
    """scale and offset placing the reference on the frame (search around the eyes, best silhouette overlap)"""
    g = np.array(ref).astype(int)
    lum = (g[..., 0] * 3 + g[..., 1] * 6 + g[..., 2]) / 10
    gm = (g[..., 3] > 128) & (lum < 120)
    yel = (g[..., 0] > 150) & (g[..., 1] > 120) & (g[..., 2] < 90)
    gy, gx = np.nonzero(yel)
    gex, gey = gx.mean(), gy.mean()
    fex, fey = eye(F)
    fm = F > 0
    region = np.zeros((256, 256), bool)
    region[0:200, 150:248] = True
    gim = Image.fromarray((gm * 255).astype(np.uint8))
    best = None
    for s in np.arange(0.40, 0.80, 0.005):
        w, h = int(round(gm.shape[1] * s)), int(round(gm.shape[0] * s))
        sm = np.array(gim.resize((w, h), Image.BILINEAR)) > 127
        ex, ey = fex - gex * s, fey - gey * s
        for dx in range(int(ex) - 6, int(ex) + 7):
            for dy in range(int(ey) - 6, int(ey) + 7):
                c = np.zeros((256, 256), bool)
                x0, y0, x1, y1 = max(dx, 0), max(dy, 0), min(dx + w, 256), min(dy + h, 256)
                if x1 <= x0 or y1 <= y0:
                    continue
                c[y0:y1, x0:x1] = sm[y0 - dy:y1 - dy, x0 - dx:x1 - dx]
                iou = ((c & fm) & region).sum() / max(1, ((c | fm) & region).sum())
                if best is None or iou > best[0]:
                    best = (iou, s, dx, dy)
    return best


def draw(F, ref, s, dx, dy):
    w, h = int(round(ref.width * s)), int(round(ref.height * s))
    gs = np.array(ref.resize((w, h), Image.LANCZOS)).astype(float)
    G = np.zeros((256, X0 + W + 64, 4))
    x0, y0, x1, y1 = max(dx, 0), max(dy, 0), min(dx + w, G.shape[1]), min(dy + h, 256)
    G[y0:y1, x0:x1] = gs[y0 - dy:y1 - dy, x0 - dx:x1 - dx]
    lum = G[..., 0] * 0.3 + G[..., 1] * 0.59 + G[..., 2] * 0.11
    body = (G[..., 3] > 140) & (lum < 140)
    eyes = (G[..., 0] > 140) & (G[..., 1] > 110) & (G[..., 2] < 100) & (G[..., 3] > 140)

    # the tones: the reference's luminance quantiles onto the frame's ramp indices, where both are body
    both = (F >= 1) & (F <= 11) & body[:, :256]
    both[:, :200] = False
    gl, gi = np.sort(lum[:, :256][both]), np.sort(F[both])

    def value(l):
        q = np.searchsorted(gl, l) / len(gl)
        pos = q * (len(gi) - 1)
        lo = gi[np.clip(pos.astype(int), 0, len(gi) - 1)]
        hi = gi[np.clip(pos.astype(int) + 1, 0, len(gi) - 1)]
        return lo + (hi - lo) * (pos - pos.astype(int))

    # what is kept: right of the seam all of it; left of it the reference's body connected to the seam where the
    # frame is empty (flood from the seam column through empty frame pixels)
    reg = body[:, X0:X0 + W].copy()
    seam = 256 - X0
    left = reg[:, :seam] & (F[:, X0:256] == 0)
    keep = np.zeros_like(left)
    keep[:, seam - 1] = left[:, seam - 1] & reg[:, seam]
    for _ in range(seam):
        grow = keep.copy()
        grow[:, :-1] |= keep[:, 1:]
        grow[1:, :] |= keep[:-1, :]
        grow[:-1, :] |= keep[1:, :]
        grow &= left
        if (grow == keep).all():
            break
        keep = grow
    reg[:, :seam] = keep

    out = np.zeros((256, W), int)
    val = np.zeros((256, W))
    val[reg] = value(lum[:, X0:X0 + W][reg])
    bayer = np.array([[0.125, 0.625], [0.875, 0.375]])
    yy, xx = np.mgrid[0:256, X0:X0 + W]
    idx = np.floor(val) + ((val - np.floor(val)) > bayer[yy % 2, xx % 2])
    out[reg] = np.clip(idx[reg], 2, 11).astype(int)
    out[eyes[:, X0:X0 + W] & reg] = 12
    sil = np.zeros((258, 258 + X0 + W - 256), bool)
    sil[1:257, 1:257] = F > 0
    sil[1:257, 1 + X0:1 + X0 + W] |= reg
    inner = sil[1:-1, 1:-1] & sil[:-2, 1:-1] & sil[2:, 1:-1] & sil[1:-1, :-2] & sil[1:-1, 2:]
    out[(sil[1:-1, 1:-1] & ~inner)[:, X0:X0 + W] & reg] = 1
    return out


def write_inc(pose, out, slots, rom, path):
    rows = np.nonzero(out.any(1))[0]
    r0, r1 = int(rows.min()), int(rows.max()) + 1
    lines = ["    { " + ", ".join(f"0x{(int(out[y, x]) | (int(out[y, x + 1]) << 4)):02x}" for x in range(0, W, 2)) + " },"
             for y in range(r0, r1)]
    maps = ", ".join(sorted({rom.maps[k] for k in slots}))
    text = (f"/* Darkside's right half, pose \"{pose}\" ({maps}): generated by port/tools/gen_darkside_body.py from a\n"
            f" * reference picture approved by the user. Body palette bank 0 indices, two pixels a byte (low nibble left),\n"
            f" * 0 transparent. Rows {r0}..{r1 - 1} of the frame's block, columns {X0}..{X0 + W - 1}. */\n"
            f"static const unsigned char kDarkside_{pose}[{r1 - r0}][{W // 2}] = {{\n" + "\n".join(lines) + "\n};\n"
            f"#define DARKSIDE_{pose.upper()}_TOP {r0}\n#define DARKSIDE_{pose.upper()}_ROWS {r1 - r0}\n")
    path.write_text(text, newline="\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pose", required=True, choices=sorted(POSES))
    ap.add_argument("--ref", required=True, help="the reference picture of the whole Darkside in this pose")
    ap.add_argument("--slot", type=int, help="the slot to align on (default: the pose's first)")
    args = ap.parse_args()
    rom = Rom()
    slots = POSES[args.pose]
    F = rom.frame(args.slot if args.slot is not None else slots[0])
    ref = Image.open(args.ref).convert("RGBA")
    iou, s, dx, dy = register(F, ref)
    print(f"aligned: overlap {iou:.3f}, scale {s:.3f}, at {dx},{dy}")
    out = draw(F, ref, s, dx, dy)
    write_inc(args.pose, out, slots, rom, ROOT / "port/include/port" / f"darkside_body_{args.pose}.inc")

    pal = rom.palette()
    full = np.zeros((256, X0 + W), int)
    full[:, :256] = F
    full[:, X0:] = np.where(out > 0, out, full[:, X0:])
    img = Image.new("RGB", (X0 + W, 256), (250, 240, 200))
    px = img.load()
    for y in range(256):
        for x in range(X0 + W):
            if full[y, x]:
                px[x, y] = pal[full[y, x]]
    prev = ROOT / "build/arena/body" / f"preview_{args.pose}.png"
    prev.parent.mkdir(parents=True, exist_ok=True)
    img.crop((120, 0, X0 + W, 256)).resize(((X0 + W - 120) * 4, 1024), Image.NEAREST).save(prev)
    print(prev)


if __name__ == "__main__":
    main()
