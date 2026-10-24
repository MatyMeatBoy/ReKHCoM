#!/usr/bin/env python3
"""Static checks of port/data/hd_sprites.tsv, each one a bug class found before (DEBUG_WORKFLOW.md):
  - names are unique (a second row of a name overwrote the first's picture: DIARIO / JOURNAL);
  - the tiles and frame symbols exist in every version the row is compiled for;
  - the box covers every OAM part of the frame in those versions (a part outside keeps the cartridge's pixels and
    logs "outside its box");
  - the tiles are ones the game's code names (src/): a frame of shared tiles named
    with another set never matches (the shop's gMsTopBarTiles).
Exit 1 with the problems listed. check_sources.py runs it.

    python port/tools/check_hd_sprites.py
"""
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SHAPES = {(0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64), (1, 0): (16, 8), (1, 1): (32, 8),
          (1, 2): (32, 16), (1, 3): (64, 32), (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64)}
ROMS = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}


def symbols(region):
    p = ROOT / 'build' / region / 'pc' / 'rom_symbols.s'
    return {k: int(v, 16) for k, v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)', p.read_text())} if p.exists() else None


def parts(data, syms, frame):
    fo = syms[frame] - 0x08000000
    n = struct.unpack_from('<H', data, fo)[0]
    out = []
    for k in range(min(n, 64)):
        a0, a1, _ = struct.unpack_from('<HHH', data, fo + 2 + k * 6)
        if (a0 >> 14, a1 >> 14) not in SHAPES:
            continue
        pw, ph = SHAPES[(a0 >> 14, a1 >> 14)]
        x = a1 & 0x1FF
        x -= 512 if x & 0x100 else 0
        y = a0 & 0xFF
        y -= 256 if y & 0x80 else 0
        out.append((x, y, pw, ph))
    return out


def main():
    problems = []
    rows = [l.split('\t') for l in (ROOT / 'port/data/hd_sprites.tsv').read_text(encoding='utf-8').splitlines()
            if l and not l.startswith('#')]
    seen = {}
    for r in rows:
        if r[0] in seen:
            problems.append(f'{r[0]}: name used twice (the second picture overwrites the first)')
        seen[r[0]] = r
    src = '\n'.join(p.read_text(encoding='utf-8', errors='replace') for p in (ROOT / 'src').rglob('*.c'))
    loaded = set(re.findall(r'\b(g\w+Tiles)\b', src))  # named by the game's code (loads, tables, catalogs)
    sym = {r: symbols(r) for r in ROMS}
    data = {r: (ROOT / 'roms' / f'{c}.gba').read_bytes() if (ROOT / 'roms' / f'{c}.gba').exists() else None
            for r, c in ROMS.items()}
    for r in rows:
        if len(r) < 10 or r[3] == '-':
            continue
        name, frame, tiles = r[0], r[2], r[3]
        x0, y0, w, h = map(int, r[4:8])
        regions = r[10].split(',') if len(r) > 10 and r[10] else list(ROMS)
        if tiles not in loaded:
            problems.append(f'{name}: {tiles} is never named by the game (shared tiles under another name?)')
        for reg in regions:
            s = sym.get(reg)
            if s is None:
                continue
            for need in (tiles, frame):
                if need != '-' and need not in s:
                    problems.append(f'{name} ({reg}): no symbol {need}')
            # a BG banner (hd_menu_titles.py BG_BANNERS) is named by its map: no OAM parts to check
            if frame == '-' or frame.endswith('Map') or frame not in s or data[reg] is None:
                continue
            for px, py, pw, ph in parts(data[reg], s, frame):
                if px < x0 or py < y0 or px + pw > x0 + w or py + ph > y0 + h:
                    problems.append(f'{name} ({reg}): part {px},{py} {pw}x{ph} outside the box {x0},{y0} {w}x{h}')
                    break
    for p in problems:
        print(p)
    print(f'check_hd_sprites: {len(rows)} rows, {len(problems)} problem(s)')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
