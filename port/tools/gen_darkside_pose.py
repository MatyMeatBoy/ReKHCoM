#!/usr/bin/env python3
"""Darkside's right half for the poses other than standing (port/src/port_darkside.c), drawn from the finished model.

The cartridge cuts every body frame of Darkside at its block's edge (x 256). The standing pose has its right half
drawn from the user's reference (gen_darkside_body.py, darkside_body_idle.inc); together with the cartridge's frame 0
that is the finished model. Each other pose takes its pieces from that model, already in the game's pixels and
palette, placed on the pose's own body:

  - "model": the model's right side (shoulder, arm, fist, the hair beside the head) moved so the eyes meet, plus a
    per-pose nudge (and an optional flip);
  - "self": the pose's own left limb, mirrored over the body's axis (a claw that should be a claw, not a fist).

Only pixels where the cartridge's frame is transparent are added, right of DRAW_LEFT. Of what a piece adds, only the
part joined to the body (and the hair above the eye) stays: loose bits of the model's waist do not. The game's
outline (index 1) closes the new silhouette.

Writes port/include/port/darkside_body_<pose>.inc (indices, two a byte) and build/arena/body/pose_<pose>.png
(lighter: the new pixels).

    uv run --with numpy --with pillow python port/tools/gen_darkside_pose.py [pose ...]
"""
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gen_darkside_body import POSES as SLOTS, ROOT, Rom, eye  # noqa: E402

DRAW_LEFT, DRAW_W = 232, 128
CLOSE_RATIO, CLOSE_DEPTH = 0.5, 20  # the new pixels: columns DRAW_LEFT.. of the frame's block

# frames that only swap the map and keep the pose's tiles (the punch's eye animates): drawn with those tiles
EXTRA = {"punch": [14, 15, 16, 17, 29, 30, 31, 32]}
EXTRA_TILES = "gBosDsdFrame8Tiles"

# pose -> pieces. model: nudge (dx, dy) added to the eye-to-eye offset, or at (dx, dy) when the pose shows no eye; the model's columns from x0, or its box
# (x0, y0, x1, y1); flip: 'h' mirrors the piece about its own middle. self: the pose's own pixels mirrored over x =
# axis2 / 2, in rows (y0, y1).
POSES = {
    # facing the camera, the free arm out: the model's shoulder, arm and fist where the eyes meet
    "crouch": [dict(kind="model", nudge=(0, 0), x0=236)],
    "raise": [dict(kind="model", nudge=(0, 0), x0=236)],
    # both hands are claws: the model's hair, then the pose's own shoulder and claws mirrored below the eye
    "claw": [dict(kind="model", nudge=(0, 0), box=(236, 0, 320, 64)), dict(kind="self", axis2=510, rows=(46, 256))],
    # in profile, the back to the cut: the model's shoulder over the back and the arm hanging behind
    "lean": [dict(kind="model", nudge=(35, -5), x0=244)],
    "punch": [dict(kind="model", nudge=(55, -25), x0=244)],
    # only the reaching arm is in the frame: the whole model (head, torso, fist) at its root
    "reach": [dict(kind="model", at=(62, 20), x0=150)],
}


def model(rom):
    """the finished standing model: frame 0 and the approved drawing"""
    txt = (ROOT / "port/include/port/darkside_body_idle.inc").read_text()
    top = int(re.search(r"IDLE_TOP (\d+)", txt).group(1))
    left = int(re.search(r"columns (\d+)\.\.", txt).group(1))
    m = np.zeros((256, 320), int)
    m[:, :256] = rom.frame(0)
    for y, row in enumerate(re.findall(r"\{ (0x[^}]*) \}", txt)):
        for i, v in enumerate(int(b, 16) for b in row.split(", ")):
            for k, p in enumerate((v & 15, v >> 4)):
                x = left + i * 2 + k
                if p and (x >= 256 or m[top + y, x] == 0):
                    m[top + y, x] = p
    return m


def components(mask):
    lab = np.zeros(mask.shape, int)
    out = []
    h, w = mask.shape
    for y0 in range(h):
        for x0 in range(w):
            if mask[y0, x0] and not lab[y0, x0]:
                n = len(out) + 1
                stack, pts = [(y0, x0)], []
                lab[y0, x0] = n
                while stack:
                    y, x = stack.pop()
                    pts.append((y, x))
                    for p, q in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
                        if 0 <= p < h and 0 <= q < w and mask[p, q] and not lab[p, q]:
                            lab[p, q] = n
                            stack.append((p, q))
                out.append(pts)
    return out


def build(rom, M, pose):
    slot = SLOTS[pose][0]
    F = rom.frame(slot)
    slots = SLOTS[pose][1:] + EXTRA.get(pose, [])
    # cover the gaps of every frame of the pose (its hair moves; the punch sinks): each frame aligned on its eye,
    # only the pixels all of them have count as the body's
    first = F
    for k in slots:
        G = rom.frame(k)
        if ((G == 12) | (G == 13)).any():
            G = np.roll(G, -int(round(eye(G)[1] - eye(first)[1])), axis=0)
        F = np.where(G > 0, F, 0)
    W = 256 + DRAW_W
    A = np.zeros((256, W), int)
    A[:, :256] = F
    ex, ey = eye(first) if ((first == 12) | (first == 13)).any() else (np.nan, np.nan)
    mx, my = eye(M[:, :256])
    added = np.zeros((256, W), int)
    for piece in POSES[pose]:
        src = np.zeros((256, W), int)
        if piece["kind"] == "model":
            if "at" in piece:  # the pose has no eye (the body is beyond the cut): where the model goes
                dx, dy = piece["at"]
            else:
                dx = int(round(ex - mx)) + piece["nudge"][0]
                dy = int(round(ey - my)) + piece["nudge"][1]
            part = M.copy()
            x0, y0, x1, y1 = piece.get("box", (piece.get("x0", 0), 0, 320, 256))
            part[:, :x0] = 0
            part[:, x1:] = 0
            part[:y0] = 0
            part[y1:] = 0
            if piece.get("flip") == "h":
                cols = np.nonzero(part.any(0))[0]
                part[:, cols[0]:cols[-1] + 1] = part[:, cols[0]:cols[-1] + 1][:, ::-1]
            for y in range(256):
                sy = y - dy
                if 0 <= sy < 256:
                    for x in range(W):
                        sx = x - dx
                        if 0 <= sx < 320:
                            src[y, x] = part[sy, sx]
        else:  # self
            a2 = piece["axis2"]
            y0, y1 = piece.get("rows", (0, 256))
            for y in range(y0, y1):
                for x in range(256, W):
                    sx = a2 - x
                    if 0 <= sx < 256:
                        src[y, x] = F[y, sx]
        put = (src > 0) & (A == 0) & (added == 0)
        put[:, :DRAW_LEFT] = False
        added[put] = src[put]
    # keep what joins the body (the largest pieces) and the hair above the eye
    comps = components(added > 0)
    comps.sort(key=len, reverse=True)
    keep = np.zeros(added.shape, bool)
    for i, pts in enumerate(comps):
        ys = [p[0] for p in pts]
        if i == 0 or len(pts) >= 400 or (ey == ey and max(ys) < ey):
            for p in pts:
                keep[p] = True
    added[~keep] = 0
    A[added > 0] = added[added > 0]
    # what is still cut at the block's edge (a back, a waist the model has no piece for): closed with the body's own
    # border, flipped, as a rounded edge (half an ellipse, at most CLOSE_DEPTH deep)
    y = 0
    while y < 256:
        if not (A[y, 255] and not A[y, 256]):
            y += 1
            continue
        y0 = y
        while y < 256 and A[y, 255] and not A[y, 256]:
            y += 1
        n = y - y0
        for yy in range(y0, y):
            t = (yy - y0 + 0.5) / n * 2 - 1
            depth = int(round(min(n / 2 * CLOSE_RATIO, CLOSE_DEPTH) * np.sqrt(max(0.0, 1 - t * t))))
            for j in range(depth):
                added[yy, 256 + j] = A[yy, 255 - j] or 1
                A[yy, 256 + j] = added[yy, 256 + j]
    # the outline where the new silhouette meets the outside
    sil = np.zeros((258, W + 2), bool)
    sil[1:-1, 1:-1] = A > 0
    inner = sil[1:-1, 1:-1] & sil[:-2, 1:-1] & sil[2:, 1:-1] & sil[1:-1, :-2] & sil[1:-1, 2:]
    edge = (A > 0) & ~inner & (added > 0)
    added[edge] = 1
    return F, added[:, DRAW_LEFT:DRAW_LEFT + DRAW_W]


def write_inc(pose, out, rom, path):
    rows = np.nonzero(out.any(1))[0]
    r0, r1 = int(rows.min()), int(rows.max()) + 1
    lines = ["    { " + ", ".join(f"0x{(int(out[y, x]) | (int(out[y, x + 1]) << 4)):02x}" for x in range(0, DRAW_W, 2)) +
             " }," for y in range(r0, r1)]
    maps = ", ".join(sorted({rom.maps[k] for k in SLOTS[pose]}))
    text = (f"/* Darkside's right half, pose \"{pose}\" ({maps}): generated by port/tools/gen_darkside_pose.py from the\n"
            f" * finished standing model (the cartridge's frame 0 and the user's approved drawing). Body palette bank 0\n"
            f" * indices, two pixels a byte (low nibble left), 0 transparent. Rows {r0}..{r1 - 1} of the frame's block,\n"
            f" * columns {DRAW_LEFT}..{DRAW_LEFT + DRAW_W - 1}. */\n"
            f"static const unsigned char kDarkside_{pose}[{r1 - r0}][{DRAW_W // 2}] = {{\n" + "\n".join(lines) + "\n};\n"
            f"#define DARKSIDE_{pose.upper()}_TOP {r0}\n#define DARKSIDE_{pose.upper()}_ROWS {r1 - r0}\n")
    path.write_text(text, newline="\n")


def preview(rom, F, out, path):
    pal = rom.palette()
    W = DRAW_LEFT + DRAW_W
    img = Image.new("RGB", (W, 256), (250, 240, 200))
    px = img.load()
    for y in range(256):
        for x in range(W):
            n = out[y, x - DRAW_LEFT] if x >= DRAW_LEFT else 0
            v = n or (F[y, x] if x < 256 else 0)
            if v:
                c = pal[v]
                px[x, y] = tuple(min(255, k + 45) for k in c) if n else c
    img = img.crop((100, 0, W, 256))
    img.resize((img.width * 3, img.height * 3), Image.NEAREST).save(path)


def main():
    rom = Rom()
    for k in sum(EXTRA.values(), []):
        rom.tiles[k] = EXTRA_TILES
    M = model(rom)
    for pose in sys.argv[1:] or sorted(POSES):
        F, out = build(rom, M, pose)
        write_inc(pose, out, rom, ROOT / "port/include/port" / f"darkside_body_{pose}.inc")
        p = ROOT / "build/arena/body" / f"pose_{pose}.png"
        p.parent.mkdir(parents=True, exist_ok=True)
        preview(rom, F, out, p)
        print(pose, p)


if __name__ == "__main__":
    main()
