"""Offline donor/phase evidence. No compilation or game execution.

ROM palette only: preview is a tilemap diagnostic, not a runtime capture.
"""
import json
from pathlib import Path

import scene_tile_data as sc
import scene_wide_review as review
from PIL import Image, ImageDraw


def main():
    root = Path(__file__).resolve().parents[2]
    out = root / 'build/atlas074'
    out.mkdir(parents=True, exist_ok=True)
    _, address = sc.bg_symbol(74)
    bg = sc.load_bg(address)
    grid = bg['bg2']
    matches = sum(grid[y][x] == grid[y + 1][x - 20]
                  for y in range(17, 21) for x in range(27, 32)
                  if grid[y][x])
    assert matches == 19, matches
    # Every pixel tile of each full ornament must use the same donor.
    checked = 0
    for repeat in range(1, 16):
        for sy in range(18, 22):
            for sx in range(7, 15):
                x = sx + 20 + repeat * 20
                y = sy - 1 + repeat * 10
                assert review.corrected_source(74, 'bg2', x, y) == (sx, sy)
                checked += 1
    for layer in ('bg1', 'bg2', 'bg3'):
        for y in range(32):
            for x in range(32):
                assert review.corrected_source(74, layer, x, y) == (x, y)
    # Reuse the actual offline decoder/compositor. Never substitute this for
    # the serial runtime capture with the active hardware palette and actors.
    sc.active_bg_palette = lambda _: bg['colors']
    new_source = review.corrected_source
    after = review.render(74, True, True)

    def old_source(event, layer, x, y):
        if event == 74 and layer == 'bg2' and x >= 35 and y >= 0:
            repeat = (x - 27) // 20
            sx = 7 + (x - 27) % 20
            sy = y - repeat * 10 - 4
            if repeat >= 1 and sx <= 14 and 18 <= sy <= 21:
                return sx, sy
            if 2 * y - x >= 2:
                return 0, 0
        return new_source(event, layer, x, y)

    review.corrected_source = old_source
    try:
        before = review.render(74, True, True)
    finally:
        review.corrected_source = new_source
    before.save(out / '074-before-offline.png')
    after.save(out / '074-after-offline.png')
    pair = Image.new('RGB', (960, 1120), '#202020')
    draw = ImageDraw.Draw(pair)
    draw.text((8, 4), 'ANTES - diagnostico offline, paleta ROM', fill='white')
    pair.paste(before, (0, 20))
    draw.text((8, 564), 'PROPUESTA - fase (20,10), modulo entero 8x4; sin actores/efectos', fill='white')
    pair.paste(after, (0, 580))
    pair.save(out / '074-comparison-offline.png')
    evidence = dict(original_matching_entries=matches, full_module_tiles_checked=checked,
                    native_coordinates_checked=3 * 32 * 32,
                    donor=[7, 18, 15, 22], first_anchor=[27, 17], period=[20, 10],
                    palette='ROM diagnostic only', runtime_test='waiting for serial slot')
    (out / 'offline-evidence.json').write_text(json.dumps(evidence, indent=2))
    print(json.dumps(evidence))


if __name__ == '__main__':
    main()
