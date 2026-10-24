#!/usr/bin/env python3
"""Re:CoM movie subtitles from the player's own KINGDOM HEARTS -HD 1.5+2.5 ReMIX- (HD mod, port/progress/HD_MOD.md).

Recom.pkg keeps them as FORM/5xxx/FO5xxx.BIN: a 0x40-byte header, then an MVS table (the US English text) with one
remastered copy per language (UK_, FR_, GR_ (German), IT_, SP_MVS000.binl):
- "MVS\\0", u16 version, u16 count, u32 0x30; at 0x10 count entries of u16 first frame, u16 last frame, u32 text
  offset (frames at 30 per second of the movie); the texts are NUL-terminated, '\\n' breaks the line;
- letters beyond ASCII are two bytes: 0x99 then a code of the game's own table (KANA below), or Shift-JIS punctuation.
FO5000 is the opening (ROP), FO5200 / FO5699 the endings (RSE / RRE), FO5abc the scene zz_event_abc.

Each becomes hd/subs/<movie>.<lang>.txt, UTF-8, one line per subtitle: "<first frame> <last frame> <text>" with the
line breaks written as \\n. <lang>: us, uk, fr, de, it, es (ja: the one US table left in Japanese).

    python port/tools/khhd_subs.py "<install folder>" <data folder>
"""
import hashlib
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import khhd_pkg  # noqa: E402

# 0x99 xx: capitals from 0x80, small letters from 0x9A (the order the six languages' texts show), then punctuation
KANA = dict(zip(range(0x80, 0x9A), 'ÀÁÂÄÃÇÈÉÊËÌÍÎÏÑÒÓÔÕÖŒÙÚÛÜß'))
KANA.update(zip(range(0x9A, 0xBA), 'àáâäãçèéêëìíîïñòóôõöùúûüœ¿¡«„»ºª'))
KANA[0xCF] = '°'
SJIS = {0x815C: '—', 0x81F4: '', 0x8167: '“', 0x8168: '”', 0x83C3: 'é', 0x8140: ' '}
LANGS = {'': 'us', 'UK': 'uk', 'FR': 'fr', 'GR': 'de', 'IT': 'it', 'SP': 'es'}


def movie_name(sid):
    return {5000: 'ROP', 5200: 'RSE', 5699: 'RRE'}.get(sid, f'zz_event_{sid - 5000:03d}')


def decode(raw):
    out, i = [], 0
    while i < len(raw):
        b = raw[i]
        if b < 0x80:
            out.append(chr(b))
            i += 1
            continue
        pair = raw[i:i + 2]
        code = int.from_bytes(pair, 'big')
        if b == 0x99 and len(pair) == 2 and pair[1] in KANA:
            out.append(KANA[pair[1]])
        elif code in SJIS:
            out.append(SJIS[code])
        else:
            out.append(pair.decode('cp932', errors='replace'))
        i += 2
    return ''.join(out)


def parse(table):
    """[(first frame, last frame, text)] of one MVS table."""
    if table[:4] != b'MVS\0':
        return []
    count = struct.unpack_from('<H', table, 6)[0]
    lines = []
    for i in range(count):
        first, last, off = struct.unpack_from('<HHI', table, 0x10 + i * 8)
        lines.append((first, last, decode(table[off:table.index(b'\0', off)])))
    return lines


def extract(install, data):
    hed = Path(install) / 'Image/dt/Recom.hed'
    out = Path(data) / 'hd/subs'
    out.mkdir(parents=True, exist_ok=True)
    want = {}
    for sid in range(5000, 5700):
        name = f'FORM/{sid}/FO{sid}.BIN'
        want[hashlib.md5(name.encode()).hexdigest()] = sid
    written = 0
    with open(hed.with_suffix('.pkg'), 'rb') as pkg:
        for md5, offset, _stored, _real in khhd_pkg.entries(hed):
            sid = want.get(md5)
            if sid is None:
                continue
            base, _, extra = khhd_pkg.read_asset(pkg, offset, with_remastered=True)
            tables = {'': base[0x40:]}
            tables.update({n[:2]: d for n, d in extra.items() if re.match(r'[A-Z]{2}_MVS', n)})
            for prefix, table in tables.items():
                lines = parse(table)
                if prefix in LANGS and lines:
                    text = ''.join(f'{a} {b} {t}'.replace('\n', '\\n') + '\n' for a, b, t in lines)
                    lang = 'ja' if any(ord(c) >= 0x3000 for c in text) else LANGS[prefix]  # a Japanese leftover
                    (out / f'{movie_name(sid)}.{lang}.txt').write_text(text, encoding='utf-8', newline='\n')
                    written += 1
    return written


if __name__ == '__main__':
    print(extract(sys.argv[1], sys.argv[2]), 'subtitle files')
