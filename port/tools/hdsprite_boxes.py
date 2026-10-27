#!/usr/bin/env python3
"""The box an HD sprite row needs (port/data/hd_sprites.tsv x0 y0 w h): the union of a frame's OAM parts, from the
frame's pieces in config/assets/*.yaml ("- [WxH, x, y]"). PortHdSpriteQueue refuses a part outside the row's box
("part ... outside its box" in kh_com.log).

    python port/tools/hdsprite_boxes.py gTitleMenuCursorFrame0 gTitleSoraLumiSpanishFrame0 ...
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def pieces():
    found, current = {}, None
    for path in sorted((ROOT / 'config/assets').glob('*.yaml')):
        for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
            m = re.match(r'\s*- symbol:\s*(\w+)', line)
            if m:
                current = found.setdefault(m.group(1), [])
                continue
            m = re.match(r'\s*- \[(\d+)x(\d+),\s*(-?\d+),\s*(-?\d+)', line)
            if m and current is not None:
                current.append(tuple(int(v) for v in m.groups()))
            elif re.match(r'\s*- name:', line):
                current = None
    return found


def main():
    table = pieces()
    for name in sys.argv[1:]:
        parts = table.get(name)
        if not parts:
            print(name, 'no pieces in config/assets')
            continue
        x0 = min(x for w, h, x, y in parts)
        y0 = min(y for w, h, x, y in parts)
        x1 = max(x + w for w, h, x, y in parts)
        y1 = max(y + h for w, h, x, y in parts)
        print(f'{name}\t{x0}\t{y0}\t{x1 - x0}\t{y1 - y0}\t({len(parts)} parts)')


if __name__ == '__main__':
    main()
