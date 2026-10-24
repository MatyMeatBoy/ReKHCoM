"""Japanese text of the port's own menus that the cartridge's font cannot draw.

The JP build draws our menus with the game's message font (port_ui.c DrawGlyph): kana by formula, punctuation by the
row 0x81 table, kanji only those GetSjisGlyph (src/msg/msg.c) knows. A character outside them is drawn as a gap.
Prints each string with a missing character; exit status 1 if any.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ['port/src/port_ui.c', 'port/src/port_hdtext.c', 'port/src/port_main.c', 'port/src/port_config.c']


def kanji():
    """SJIS codes GetSjisGlyph gives a glyph: inner `case 0xLL:` under outer `case 0xHH00:` (glyph != 0)."""
    s = (ROOT/'src/msg/msg.c').read_text(encoding='utf-8')
    body = s[s.index('void GetSjisGlyph('):]
    body = body[:body.index('\n}\n')]
    codes, high = set(), None
    for m in re.finditer(r'case 0x([0-9A-Fa-f]{2})00:|case 0x([0-9A-Fa-f]{2}):\s*\*glyph = (\d+);', body):
        if m.group(1):
            high = int(m.group(1), 16)
        elif high is not None and int(m.group(3)):
            codes.add(high << 8 | int(m.group(2), 16))
    return codes


ROW81 = {0x41, 0x42, 0x43, 0x44, 0x45, 0x48, 0x49, 0x5B, 0x5C, 0x5E, 0x60, 0x63, 0x66, 0x69, 0x6A, 0x75, 0x76, 0x77,
         0x78, 0x7B, 0x7C, 0x81, 0x93, 0x96}
PORT_MAPPED = {'↑', '↓', '←', '→', '○', '：'}  # NextChar maps them to Latin codes


def drawable(ch, known):
    if ord(ch) < 0x3000 or ch in PORT_MAPPED:
        return True
    try:
        b = ch.encode('cp932')
    except UnicodeEncodeError:
        return False
    if len(b) != 2:
        return True
    code, hi, lo = b[0] << 8 | b[1], b[0], b[1]
    if hi == 0x81:
        return lo in ROW81
    if hi == 0x82:  # GetSjisGlyph: digits, letters, hiragana
        return 79 <= lo <= 88 or 96 <= lo <= 121 or 129 <= lo <= 154 or 159 <= lo <= 241
    if hi == 0x83:  # katakana
        return 64 <= lo <= 126 or 128 <= lo <= 148
    return code in known


def main():
    known, bad = kanji(), 0
    for src in SOURCES:
        p = ROOT/src
        if not p.exists():
            continue
        for n, line in enumerate(p.read_text(encoding='utf-8').splitlines(), 1):
            for text in re.findall(r'"((?:[^"\\]|\\.)*)"', line):
                missing = sorted({c for c in text if not drawable(c, known)})
                if missing:
                    bad += 1
                    print(f'{src}:{n}: {"".join(missing)}  {text}')
    print('check_jp_glyphs:', 'ok' if not bad else f'{bad} string(s) with characters the JP font lacks')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
