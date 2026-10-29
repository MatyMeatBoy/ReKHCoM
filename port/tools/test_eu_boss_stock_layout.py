#!/usr/bin/env python3
"""Verify all EU stock aliases against local native OAM data; writes no ROM art."""
import argparse
from pathlib import Path
import re
from hdcards_gba import Rom

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--rom', type=Path, required=True)
    ap.add_argument('--symbols', type=Path, required=True)
    args = ap.parse_args()
    rom = Rom.__new__(Rom)
    rom.rom = args.rom.read_bytes()
    rom.syms = {name: int(value, 16) for name, value in re.findall(
        r'\.set _(\w+), (0x[0-9a-fA-F]+)', args.symbols.read_text(encoding='utf-8'))}
    header = (ROOT/'port/include/port/port_eu_boss_stock_alias.h').read_text(encoding='utf-8')
    pairs = re.findall(r'\{ (g\w+SmallCardTiles), (g\w+SmallCardFrame0), '
                       r'(g\w+SmallCardTiles), (g\w+SmallCardFrame0) \}', header)
    assert len(pairs) == 11
    for tiles, frame, portrait, portrait_frame in pairs:
        assert rom.syms[tiles] != rom.syms[portrait], 'Alias must represent a distinct actual producer'
        actual, canonical = rom.pieces(frame), rom.pieces(portrait_frame)
        assert actual == canonical, f'{frame}: native OAM shape/offset/flip differs'
        assert min(p[0] for p in actual) == -8 and min(p[1] for p in actual) == -20
        assert max(p[0]+p[2] for p in actual) == 8 and max(p[1]+p[3] for p in actual) == 12
    print('PASS: all11 EU distinct native sources have identical 16x32 OAM layout at (-8,-20)')


if __name__ == '__main__': main()
