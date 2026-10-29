#!/usr/bin/env python3
"""Check every CardDef illustration producer, not merely existing recipe rows.

Local ROM/art is unnecessary. Explicit EU stock aliases count only when their
canonical (tiles, frame) is already a group6 recipe. This catches new missing
producers even when every previously registered PNG is current.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def coverage(source, rows, aliases, region):
    large = set(re.findall(r'^\s*(g\w+Frame0), (g\w+Tiles),', source, re.M))
    small = set(re.findall(r'(g\w+SmallCardFrame0), (g\w+SmallCardTiles)', source))
    applicable = [row for row in rows if len(row) < 11 or not row[10] or region.lower() in row[10].split(',')]
    registered_large = {(row[2], row[3]) for row in applicable if row[1] == '1'}
    registered_small = {(row[2], row[3]) for row in applicable if row[1] == '6'}
    if region == 'EU':
        for tiles, frame, target_tiles, target_frame in aliases:
            if (target_frame, target_tiles) in registered_small:
                registered_small.add((frame, tiles))
    missing = [(kind, frame, tiles) for kind, pairs, registered in
               [('large', large, registered_large), ('small', small, registered_small)]
               for frame, tiles in sorted(pairs)
               if (frame, tiles) not in registered and ('-', tiles) not in registered]
    return large, small, missing


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    args = ap.parse_args()
    source = (ROOT/'src/card/card_catalog.c').read_text(encoding='utf-8')
    # The real C preprocessor selects the actual regional CardDefs. Only
    # declarations from headers are unnecessary for this token-level audit.
    source = re.sub(r'^\s*#include[^\n]*\n', '', source, flags=re.M)
    rows = [line.split('\t') for line in (ROOT/'port/data/hd_sprites.tsv').read_text(
        encoding='utf-8').splitlines() if line and not line.startswith('#')]
    alias_source = (ROOT/'port/include/port/port_eu_boss_stock_alias.h').read_text(encoding='utf-8')
    aliases = re.findall(r'\{ (g\w+SmallCardTiles), (g\w+SmallCardFrame0), '
                        r'(g\w+SmallCardTiles), (g\w+SmallCardFrame0) \}', alias_source)
    with tempfile.TemporaryDirectory(prefix='card-producer-coverage-') as temp:
        path = Path(temp)/'catalog.c'; path.write_text(source, encoding='utf-8')
        for region in ['EU', 'US', 'JP']:
            actual = subprocess.check_output([args.cc, '-E', '-P', '-x', 'c',
                                              '-DVERSION_'+region, str(path)], text=True, encoding='utf-8')
            large, small, missing = coverage(actual, rows, aliases, region)
            for row in missing: print(region, 'MISSING:', *row)
            assert not missing, f'{region}: {len(missing)} actual illustration producers lack HD coverage'
            print(f'PASS {region}: {len(large)} large and {len(small)} small CardDef producers')
            # Guard against another false "all cached recipes current" pass:
            # missing producers must fail even when the remaining rows are valid.
            for absent in ['gCardBos03Tiles', 'gTrickmasterSmallCardHD']:
                reduced = [row for row in rows if row[0] != absent]
                assert coverage(actual, reduced, aliases, region)[2], f'{absent}: negative control did not fail'
            if region == 'EU':
                reduced = [alias for alias in aliases if alias[0] != 'gAxelASmallCardTiles']
                assert coverage(actual, rows, reduced, region)[2], 'Missing EU source alias was not detected'
    print('PASS: real regional producer coverage plus recipe/alias-removal negative controls')


if __name__ == '__main__': main()
