#!/usr/bin/env python3
"""Code-native synthesis pieces, at 4x GBA resolution; no ROM or game process.

Generate committed assets/table: python port/tools/synthesis_requirement_hd.py
Stage only these PNGs for a future isolated capture:
  python port/tools/synthesis_requirement_hd.py --stage build/synthesis-review/hd/sprites
An explicit KHCOM_HD_SPRITES can point at that staged directory. Other HD assets
must also be staged there for a complete review. This never launches the game.
"""
import argparse
from functools import lru_cache
import hashlib
import json
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / 'port/assets/synthesis-requirement'
ENTRIES = ROOT / 'port/include/port/synthesis_requirement_hd_entries.inc'
SCALE = 4
SUPERSAMPLE = 4
RULES = ('at_least', 'at_most', 'exact', 'total')
COLORS = ('none', 'green', 'red', 'blue', 'gold')
# The condition lives on the outline. The GBA/old proposal's bottom is neutral;
# a solid condition-colored slab was rejected in the 20261025-125559 review.
CONDITION_COLORS = ('#ededf3', '#48945b', '#b94b4c', '#5078b2', '#ededf3')
NEUTRAL_BAND = '#dedee3'
REVIEW_PARENT = 'c08283ad450f4455937a3f1e1ead3302'
REVIEW_SUBMISSION = '20261025-125559-473c1c'


def specs():
    for color in range(5):
        yield color, f'synthesis_body_{COLORS[color]}', (-16, -20, 32, 39), None, color
    for rule in range(4):
        for value in range(100):
            if rule == 3:
                box = (-8, -5, 16, 16) if value < 10 else (-15, -5, 30, 16)
            else:
                box = (-12, -5, 28, 16) if value < 10 else (-19, -5, 42, 16)
            yield 5 + rule * 100 + value, f'synthesis_{RULES[rule]}_{value:02d}', box, rule, value


class Canvas:
    def __init__(self, box):
        self.x, self.y, self.w, self.h = box
        self.s = SCALE * SUPERSAMPLE
        self.image = Image.new('RGBA', (self.w * self.s, self.h * self.s))
        self.draw = ImageDraw.Draw(self.image)

    def points(self, points):
        return [((x - self.x) * self.s, (y - self.y) * self.s) for x, y in points]

    def line(self, points, fill, width):
        self.draw.line(self.points(points), fill=fill, width=round(width * self.s), joint='curve')

    def polygon(self, points, fill):
        self.draw.polygon(self.points(points), fill=fill)

    def finish(self):
        return self.image.resize((self.w * SCALE, self.h * SCALE), Image.Resampling.LANCZOS)


def body(box, color):
    c = Canvas(box)
    # Open body: alpha inside shows the actual spotlight, not a painted backdrop.
    path = [(-14, 17.5), (-14, -16.5), (-7, -12), (0, -19), (7, -12),
            (14, -16.5), (14, 17.5), (-14, 17.5)]
    c.polygon([(-14, 12), (14, 12), (14, 17.5), (-14, 17.5)], NEUTRAL_BAND)
    c.line(path, '#8d8d9b', 1.4)
    c.line(path, CONDITION_COLORS[color], .75)
    c.line([(-13.5, 12), (13.5, 12)], '#ededf3', .7)
    return c.finish()


def local_digits(folder):
    """Only use a complete, explicitly staged local mod set; never fetch it."""
    folder = Path(folder)
    return folder if all((folder / f'gCardValueDigitTiles_{i}.png').is_file()
                         for i in range(10)) else None


@lru_cache(maxsize=20)
def digit_image(value, scale, digits=None):
    if digits is not None:
        with Image.open(Path(digits) / f'gCardValueDigitTiles_{value}.png') as source:
            glyph = source.convert('RGBA')
    else:
        # Committed code-native fallback only. Staging prefers local mod glyphs.
        font = ImageFont.truetype(str(ROOT / 'port/assets/fonts/KHMenu.otf'), 24 * scale)
        glyph = Image.new('RGBA', (40 * scale, 40 * scale))
        ImageDraw.Draw(glyph).text((5 * scale, 5 * scale), str(value), font=font,
                                  fill='white', stroke_width=round(.7 * scale),
                                  stroke_fill='#121217')
    glyph = glyph.crop(glyph.getchannel('A').getbbox())
    target_h = 11 * scale
    target_w = max(1, round(glyph.width * target_h / glyph.height))
    return glyph.resize((target_w, target_h), Image.Resampling.LANCZOS)


def operator_image(rule):
    c = Canvas((0, 0, 6, 11))
    if rule == 2:
        for y in (3.5, 7):
            c.line([(1, y), (5, y)], '#858594', 1.8)
            c.line([(1, y), (5, y)], '#f5f5fa', 1)
    else:
        arrow = [(.5, 4), (3, .5), (5.5, 4), (4, 4), (4, 10), (2, 10), (2, 4)]
        if rule == 1:
            arrow = [(x, 10.5 - y) for x, y in arrow]
        c.polygon(arrow, '#f5f5fa')
        c.line(arrow + arrow[:1], '#858594', .5)
    return c.image


def requirement(box, rule, value, digits=None):
    c = Canvas(box)
    parts = [digit_image(int(character), c.s, str(digits) if digits else None)
             for character in str(value)]
    if rule != 3:
        parts.append(operator_image(rule))
    gap = c.s
    group = Image.new('RGBA', (sum(p.width for p in parts) + gap * (len(parts) - 1), 11 * c.s))
    x = 0
    for part in parts:
        group.alpha_composite(part, (x, (group.height - part.height) // 2))
        x += part.width + gap
    # OAM boxes stay intact for clipping/occlusion. Their visible ink is fitted
    # as one group, inside x=-11..11 with clearance from the x=+-14 silhouette.
    factor = min(1, 22 * c.s / group.width)
    if factor < 1:
        group = group.resize((round(group.width * factor), round(group.height * factor)),
                             Image.Resampling.LANCZOS)
    x = round((-group.width / c.s / 2 - c.x) * c.s)
    y = round((3 - group.height / c.s / 2 - c.y) * c.s)
    c.image.alpha_composite(group, (x, y))
    return c.finish()


def table_text():
    out = ['/* Generated by synthesis_requirement_hd.py; OAM frame coordinates. */\n']
    for index, name, box, _, _ in specs():
        out.append('    { gSynthesisHdIdentities + %d, NULL, "%s", %s, 1 },\n' %
                   (index, name, ', '.join(map(str, box))))
    return ''.join(out)


def check_oam_boxes():
    """Check every authored part, not just visible ink, against the HD bounds."""
    card = (ROOT / 'config/assets/sprites_card.yaml').read_text(encoding='utf-8')
    pictures = (ROOT / 'config/assets/sprites_card_pictures.yaml').read_text(encoding='utf-8')

    def parts(text, symbol):
        block = text.split('  - symbol: ' + symbol + '\n', 1)[1]
        block = block.split('  - symbol:', 1)[0]
        return [tuple(map(int, p)) for p in re.findall(
            r'- \[(\d+)x(\d+),\s*(-?\d+),\s*(-?\d+)', block)]

    for _, name, (x, y, w, h), rule, value in specs():
        if rule is None:
            geometry = parts(pictures, 'gCardOutlineWhiteFrame0')
        else:
            frame = (0 if value < 10 else 2) + (rule != 3)
            geometry = parts(card, f'gKeyValueFrame{frame}')
        assert geometry, name
        for pw, ph, px, py in geometry:
            assert x <= px and y <= py and px + pw <= x + w and py + ph <= y + h, name


def generate(out=ASSETS, digits=None, write_table=True):
    out.mkdir(parents=True, exist_ok=True)
    manifest = []
    for index, name, box, rule, value in specs():
        image = body(box, value) if rule is None else requirement(box, rule, value, digits)
        image.save(out / (name + '.png'))
        manifest.append(dict(id=index, file=name + '.png', box=box))
    if write_table:
        ENTRIES.write_text(table_text(), encoding='utf-8', newline='\n')
    provider = 'local gCardValueDigitTiles' if digits else 'KHMenu fallback; staging prefers local mod'
    source_hashes = {f'gCardValueDigitTiles_{i}.png':
                     hashlib.sha256((Path(digits) / f'gCardValueDigitTiles_{i}.png').read_bytes()).hexdigest()
                     for i in range(10)} if digits else {}
    filename = 'manifest.json' if write_table else 'synthesis_manifest.json'
    (out / filename).write_text(json.dumps(dict(
        reference=REVIEW_PARENT, submission=REVIEW_SUBMISSION,
        status='fix-prepared-awaiting-serial-capture', digit_provider=provider,
        local_digit_hashes=source_hashes, scale=SCALE, pieces=manifest), indent=2) + '\n', encoding='utf-8')
    print(f'Generated {len(manifest)} transparent pieces: {provider}; table updated={write_table}')


def stage_assets(folder):
    """Build into this capture's directory, using its authorized local HD set."""
    generate(Path(folder), digits=local_digits(folder), write_table=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path)
    parser.add_argument('--digits', type=Path, help='Authorized local gCardValueDigitTiles set')
    parser.add_argument('--out', type=Path, help='Private generated assets; does not change the committed table')
    parser.add_argument('--check', action='store_true', help='Static asset/table validation only')
    args = parser.parse_args()
    if args.check:
        check_oam_boxes()
        assert ENTRIES.read_text(encoding='utf-8') == table_text(), 'stale HD table'
        manifest = json.loads((ASSETS / 'manifest.json').read_text(encoding='utf-8'))
        assert len(manifest['pieces']) == 405
        for piece, spec in zip(manifest['pieces'], specs()):
            index, name, box, rule, value = spec
            assert piece == dict(id=index, file=name + '.png', box=list(box))
            with Image.open(ASSETS / piece['file']) as im:
                assert im.mode == 'RGBA' and im.size == (box[2] * SCALE, box[3] * SCALE)
                assert im.getchannel('A').getextrema() == (0, 255)
                if rule is not None:
                    bounds = im.getchannel('A').point(lambda a: 255 if a >= 16 else 0).getbbox()
                    assert bounds and box[0] + bounds[0] / SCALE >= -12, name
                    assert box[0] + bounds[2] / SCALE <= 12, name
        print('PASS: 405 identities, boxes, dimensions and transparent RGBA assets; no game run')
    elif args.stage:
        args.stage.mkdir(parents=True, exist_ok=True)
        stage_assets(args.stage)
        print(f'Staged 405 synthesis pieces in {args.stage}; no game launched')
    else:
        if args.digits and not local_digits(args.digits):
            parser.error('--digits requires all ten local gCardValueDigitTiles PNGs')
        if args.digits and not args.out:
            parser.error('Use --out for private mod-derived assets; do not publish them with fallback assets')
        generate(args.out or ASSETS, args.digits, write_table=args.out is None)


if __name__ == '__main__':
    main()
