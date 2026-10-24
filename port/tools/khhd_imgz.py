#!/usr/bin/env python3
"""Decode the remastered .imz textures of Re:CoM (1.5+2.5): an IMGZ is a list of IMGD images; in the remastered
entries every IMGD carries 32-bit RGBA pixels (its PS2 header is kept, the palette is empty). Writes one PNG per
image, for the HD inventory (HUD_HD_RECOM.md). Output stays outside the repo.

    python port/tools/khhd_imgz.py <file.imz>... --out D:/01Claude/ReKHCoM-build/hdhud/png
"""
import argparse
import struct
from pathlib import Path

from PIL import Image


def imgd_images(blob):
    """Yield (index, PIL.Image) for each IMGD of an IMGZ (or a bare IMGD)."""
    if blob[:4] == b'IMGD':
        yield 0, imgd(blob)
        return
    assert blob[:4] == b'IMGZ', blob[:4]
    count = struct.unpack_from('<i', blob, 12)[0]
    for i in range(count):
        off, size = struct.unpack_from('<ii', blob, 16 + i * 8)
        yield i, imgd(blob[off:off + size])


def imgd(b):
    assert b[:4] == b'IMGD', b[:4]
    boff, blen, poff, plen = struct.unpack_from('<iiii', b, 8)
    w, h = struct.unpack_from('<HH', b, 0x1C)
    px = b[boff:boff + blen]
    if blen == w * h * 4:
        img = Image.frombytes('RGBA', (w, h), px)
        # PS2 alpha runs 0..128; the remaster keeps it in some textures
        a = img.getchannel('A')
        if a.getextrema()[1] <= 128:
            img.putalpha(a.point(lambda v: min(255, v * 2)))
        return img
    raise ValueError(f'IMGD {w}x{h}: {blen} bytes, palette {plen}: not 32-bit')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('files', nargs='+')
    ap.add_argument('--out', required=True)
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    for f in map(Path, args.files):
        blob = f.read_bytes()
        for i, img in imgd_images(blob):
            dst = out / f'{f.stem}_{i}.png'
            img.save(dst, optimize=True)
            print(dst.name, img.size)


if __name__ == '__main__':
    main()
