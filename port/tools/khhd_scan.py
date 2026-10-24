#!/usr/bin/env python3
"""List every asset of the player's Recom.hed/.pkg that carries remastered HD textures (the 1.5+2.5's additions),
without decompressing anything: one TSV row per HD texture (asset name, texture name, size in bytes). The base for
the HD inventory in HUD_HD_RECOM.md. Output stays outside the repo (it names the player's game files).

    python port/tools/khhd_scan.py "<install>/Image/dt/Recom.hed" recom.txt D:/01Claude/ReKHCoM-build/hdhud/scan.tsv
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import khhd_pkg as k  # noqa: E402

hed, names_path, out = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
names = k.names_by_md5(names_path)
rows = []
with open(hed.with_suffix('.pkg'), 'rb') as pkg:
    for md5, offset, stored, real in k.entries(hed):
        pkg.seek(offset)
        dlen, rcount, clen, _ = struct.unpack('<iiii', pkg.read(0x10))
        for _ in range(rcount):
            name, roff, orig_off, rdlen, rclen = struct.unpack('<32siiii', pkg.read(0x30))
            rows.append((names.get(md5, md5), name.split(b'\0')[0].decode('latin1'), rdlen))
out.write_text(''.join(f'{a}\t{t}\t{n}\n' for a, t, n in rows), encoding='utf-8')
assets = {a for a, _, _ in rows}
print(f'{len(rows)} HD textures in {len(assets)} assets -> {out}')
