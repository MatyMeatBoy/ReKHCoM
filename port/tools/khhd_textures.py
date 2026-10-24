#!/usr/bin/env python3
"""Pull named assets from the player's Recom.hed/.pkg (KINGDOM HEARTS -HD 1.5+2.5 ReMIX-) with their remastered HD
textures (DDS/PNG, the "remastered entries" of each asset), for the HD HUD / HD cards research (HUD_HD_RECOM.md).
Names: OpenKh's OpenKh.Egs/resources/recom.txt. Output stays outside the repo (never committed or shipped).

    python port/tools/khhd_textures.py "<install>/Image/dt/Recom.hed" D:/01Claude/ReKHCoM-build/hdhud/cards CARD/0001/CA0001.RTM ...
"""
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import khhd_pkg as k  # noqa: E402

hed, out, want = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3:]
out.mkdir(parents=True, exist_ok=True)
idx = {m: o for m, o, s, r in k.entries(hed)}
with open(hed.with_suffix('.pkg'), 'rb') as pkg:
    for name in want:
        m = hashlib.md5(name.encode()).hexdigest()
        if m not in idx:
            print(name, 'MISSING')
            continue
        data, rem, extra = k.read_asset(pkg, idx[m], with_remastered=True)
        base = name.replace('/', '_')
        (out / base).write_bytes(data)
        print(name, len(data), 'remastered:', [(n, len(extra[n]), k.kind(extra[n])) for n in extra])
        for n, b in extra.items():
            (out / (base + '__' + n.replace('/', '_'))).write_bytes(b)
