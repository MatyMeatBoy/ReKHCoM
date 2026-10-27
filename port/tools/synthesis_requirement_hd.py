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


# ---- Re:CoM 1.5's own pieces (user 27/10: as close to 1.5HD as possible) -------------------------------------------
# Importer recipes (port/data/hd_sprites.tsv, tiles '-'): each player's import composes the 405 pieces from the room
# synthesis menu's texture (FO0102 UK_room01.imz; SP_/FR_/GR_/IT_ hold the same pieces): its crowned card tinted to the
# condition as glass, its bevelled outline in white, its italic digits in white over a dark drop shadow and an arrow /
# equals drawn to match them. Nothing of Re:CoM is committed; the PNGs above stay the code-native fallback.
RECOM_SOURCE = 'FORM/0102/FO0102.RTM:UK_room01.imz#0'
RECOM_CARD = (376, 5, 434, 92)        # the plain crowned card (grey, tinted)
RECOM_OUTLINE = (444, 5, 505, 95)     # its bevel: light edges and shadow
RECOM_DIGITS = [(5, 26, 35, 54), (47, 26, 64, 54), (78, 26, 108, 54), (6, 62, 36, 90), (46, 62, 73, 90),
                (80, 62, 110, 90), (5, 98, 35, 126), (43, 98, 70, 126), (79, 98, 110, 126), (5, 134, 33, 162)]
RECOM_GLASS = ('EDEDF3', '48A85C', 'C44648', '5078C8', 'EDEDF3')
TSV_BEGIN = '# Room synthesis requirements (generated by synthesis_requirement_hd.py --tsv): Re:CoM room01 pieces.'
TSV_END = '# End of the room synthesis requirements.'
K = 8  # the recipes' scale


def recom_body(box, color):
    x, y, w, h = box
    # the cartridge's silhouette spans x -14..14, y -19..17.5: the card fills it (a 7% wider card than Re:CoM's)
    cx0, cy0, cx1, cy1 = (-13 - x) * K, (-19 - y) * K, (13 - x) * K, round((17.5 - y) * K)
    sx = (cx1 - cx0) / (RECOM_CARD[2] - RECOM_CARD[0])
    sy = (cy1 - cy0) / (RECOM_CARD[3] - RECOM_CARD[1])
    ow = round((RECOM_OUTLINE[2] - RECOM_OUTLINE[0]) * sx)
    oh = round((RECOM_OUTLINE[3] - RECOM_OUTLINE[1]) * sy)
    return (f'canvas={w * K},{h * K};ink={RECOM_GLASS[color]};fade=35;'
            f'blit={",".join(map(str, RECOM_CARD))},{cx0},{cy0},{cx1},{cy1};fade=100;ink=FFFFFF,E6E6EE;'
            f'blit={",".join(map(str, RECOM_OUTLINE))},{cx0},{cy0},{cx0 + ow},{cy0 + oh};scale={K}')


def recom_requirement(box, rule, value):
    x, y, w, h = box
    glyph_h = 11 * K
    parts = []
    for ch in str(value):
        d = RECOM_DIGITS[int(ch)]
        parts.append(('digit', d, round((d[2] - d[0]) * glyph_h / (d[3] - d[1]))))
    if rule != 3:
        parts.append(('op', rule, 6 * K))
    gap = K
    width = sum(p[2] for p in parts) + gap * (len(parts) - 1)
    f = min(1.0, 22 * K / width)
    gh = round(glyph_h * f)
    left = round((0 - x) * K - width * f / 2)
    top = round((3 - y) * K - gh / 2)
    shadow = max(2, round(K * f / 2))
    ops = [f'canvas={w * K},{h * K}']
    for layer in ('shadow', 'face'):
        ink = '14142A' if layer == 'shadow' else 'FFFFFF,E8E8F0'
        dx = shadow if layer == 'shadow' else 0
        ops.append(f'ink={ink}')
        cur = left
        for kind, what, pw in parts:
            pw = round(pw * f)
            if kind == 'digit':
                ops.append(f'blit={",".join(map(str, what))},{cur + dx},{top + dx},{cur + dx + pw},{top + dx + gh}')
            else:
                colour = '14142AFF' if layer == 'shadow' else 'F5F5FAFF'
                ux, uy = pw / 6, gh / 11  # operator_image's 6x11 grid
                if what == 2:
                    for yy in (2.6, 6.4):
                        pts = [(.5, yy), (5.5, yy), (5.5, yy + 1.8), (.5, yy + 1.8)]
                        ops.append('poly=' + colour + ',' + ','.join(
                            f'{round(cur + dx + px * ux)},{round(top + dx + py * uy)}' for px, py in pts))
                else:
                    arrow = [(.5, 4.5), (3, .5), (5.5, 4.5), (4, 4.5), (4, 10.5), (2, 10.5), (2, 4.5)]
                    if what == 1:
                        arrow = [(px, 11 - py) for px, py in arrow]
                    ops.append('poly=' + colour + ',' + ','.join(
                        f'{round(cur + dx + px * ux)},{round(top + dx + py * uy)}' for px, py in arrow))
            cur += pw + round(gap * f)
    ops.append(f'scale={K}')
    return ';'.join(ops)


def recipe_rows():
    rows = [TSV_BEGIN]
    for _, name, box, rule, value in specs():
        recipe = recom_body(box, value) if rule is None else recom_requirement(box, rule, value)
        rows.append('\t'.join([name, '1', '-', '-', *map(str, box), RECOM_SOURCE, recipe]))
    rows.append(TSV_END)
    return rows


def write_tsv():
    tsv = ROOT / 'port/data/hd_sprites.tsv'
    lines = tsv.read_text(encoding='utf-8').splitlines()
    if TSV_BEGIN in lines:
        lines = lines[:lines.index(TSV_BEGIN)] + lines[lines.index(TSV_END) + 1:]
    tsv.write_text('\n'.join(lines + recipe_rows()) + '\n', encoding='utf-8', newline='\n')
    print(f'{len(recipe_rows()) - 2} synthesis recipes -> {tsv}')


def stage_assets(folder):
    """Build into this capture's directory, using its authorized local HD set."""
    generate(Path(folder), digits=local_digits(folder), write_table=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path)
    parser.add_argument('--digits', type=Path, help='Authorized local gCardValueDigitTiles set')
    parser.add_argument('--out', type=Path, help='Private generated assets; does not change the committed table')
    parser.add_argument('--check', action='store_true', help='Static asset/table validation only')
    parser.add_argument('--tsv', action='store_true', help="Write the importer's Re:CoM recipes into hd_sprites.tsv")
    args = parser.parse_args()
    if args.tsv:
        write_tsv()
        return
    if args.check:
        assert all(l in (ROOT / 'port/data/hd_sprites.tsv').read_text(encoding='utf-8').splitlines()
                   for l in recipe_rows()), 'stale synthesis recipes: run --tsv'
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
