#!/usr/bin/env python3
"""Make the HD text font's lowercase a and r easier to tell apart (user request, v1.9.21).

KHMenu's single-storey a is a rounded box with a small spur: at small sizes it reads as an o. This redraws it as a
two-storey a in the font's own strokes (the right stem and spur kept, a top bar, a lower bowl open on the left), and
lengthens the r's arm so it does not read as an i or a short n. Everything else in the font is untouched.

    uv run --with fonttools python port/tools/font_kh_legible.py port/assets/fonts/KHMenu.otf
"""
import sys

from fontTools.pens.recordingPen import RecordingPen
from fontTools.pens.t2CharStringPen import T2CharStringPen
from fontTools.ttLib import TTFont


def draw_a(pen):
    # outer: spur and right stem as the original (x 489..633, spur to 673), top bar from x 110, bowl 0..300
    pen.moveTo((173, 0))
    pen.lineTo((377, 0))
    pen.curveTo((428, 0), (461, 21), (477, 64))
    pen.curveTo((488, 22), (521, 0), (577, 0))
    pen.lineTo((673, 0))
    pen.lineTo((673, 96))
    pen.lineTo((633, 96))
    pen.lineTo((633, 504))
    pen.lineTo((150, 504))
    pen.curveTo((126, 504), (110, 488), (110, 464))
    pen.lineTo((110, 404))
    pen.lineTo((489, 404))
    pen.lineTo((489, 300))
    pen.lineTo((173, 300))
    pen.curveTo((90, 300), (50, 262), (50, 196))
    pen.lineTo((50, 112))
    pen.curveTo((50, 37), (91, 0), (173, 0))
    pen.closePath()
    # the bowl's counter
    pen.moveTo((181, 100))
    pen.lineTo((181, 200))
    pen.lineTo((489, 200))
    pen.lineTo((489, 100))
    pen.closePath()


def draw_r(pen):
    # the original r with its arm 70 units longer and its terminal reaching lower (y 230 instead of 280)
    pen.moveTo((50, 0))
    pen.lineTo((186, 0))
    pen.lineTo((186, 400))
    pen.lineTo((500, 400))
    pen.lineTo((500, 230))
    pen.lineTo((620, 230))
    pen.lineTo((620, 408))
    pen.curveTo((620, 445), (613, 469), (600, 480))
    pen.curveTo((587, 496), (563, 504), (528, 504))
    pen.lineTo((318, 504))
    pen.curveTo((291, 504), (271, 501), (258, 496))
    pen.curveTo((239, 488), (226, 476), (218, 460))
    pen.lineTo((186, 472))
    pen.lineTo((186, 504))
    pen.lineTo((50, 504))
    pen.closePath()


def accent_contours(gs, name):
    """The contours of an accented glyph that lie above the x-height (its accent), as recorded pen calls."""
    rec = RecordingPen()
    gs[name].draw(rec)
    contours, current = [], []
    for op in rec.value:
        current.append(op)
        if op[0] in ('closePath', 'endPath'):
            contours.append(current)
            current = []
    keep = []
    for c in contours:
        ys = [p[1] for op in c for p in op[1]]
        if ys and min(ys) >= 520:
            keep.append(c)
    return keep


def with_accent(contours):
    def draw(pen):
        draw_a(pen)
        for c in contours:
            for op, args in c:
                getattr(pen, op)(*args)
    return draw


def main():
    path = sys.argv[1]
    font = TTFont(path)
    cff = font['CFF '].cff
    top = cff[cff.fontNames[0]]
    strings = top.CharStrings
    private = top.Private
    gs = font.getGlyphSet()
    cmap = font.getBestCmap()
    accented = []
    for ch in 'áàâäãå':  # the a's with an accent are outlines of their own: the new a under each original accent
        name = cmap.get(ord(ch))
        if name:
            accented.append((name, with_accent(accent_contours(gs, name)), font['hmtx'][name][0]))
    for name, draw, advance in [('a', draw_a, 698), ('r', draw_r, 670)] + accented:
        pen = T2CharStringPen(advance, gs)
        draw(pen)
        cs = pen.getCharString(private=private, globalSubrs=cff.GlobalSubrs)
        strings[name] = cs
        lsb = font['hmtx'][name][1]
        font['hmtx'][name] = (advance, lsb)
    font.save(path)
    print(f'{path}: a, r and {len(accented)} accented a redrawn')


if __name__ == '__main__':
    main()
