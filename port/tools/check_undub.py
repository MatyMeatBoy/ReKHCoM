#!/usr/bin/env python3
"""What the Undub option leaves in English (issue #45): every voice song (SONG_VO_*) of a regional ROM is followed to
the samples its instruments play once port_undub.c's map is applied, and compared with the Japanese ROM's same song.
Lists the songs whose sample is still the regional (English) one, and those whose sequence differs from the Japanese
one without being replaced (the voice then plays with English timing, "sounds a little messed up").

    python port/tools/check_undub.py [--region eu|us]
"""
import argparse
import hashlib
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def syms(region):
    text = (ROOT / 'build' / region / 'pc' / 'rom_symbols.s').read_text()
    return {k: int(v, 16) for k, v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)', text)}


def u32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def undub_map(region):
    """{pointer offset: (dest, source, size)} and {song name: (dest, source)} of port_undub_map.h for a region."""
    text = (ROOT / 'port/include/port/port_undub_map.h').read_text(encoding='utf-8')
    part = text.split(f'defined(VERSION_{region.upper()})')[1].split('#e')[0]
    samples = {int(a, 16): (int(b, 16), int(c, 16), int(d)) for a, b, c, d in
               re.findall(r'\{ (0x[0-9a-f]+), (0x[0-9a-f]+), (0x[0-9a-f]+), (\d+) \}', part)}
    songs = set(re.findall(r'gSongVo(\w+)', part))
    return samples, songs


def instruments(rom, header, seq=None, tone=None):
    """(entry offset, sample offset) of the instruments a one-song header's tracks select (VOICE commands); seq/tone:
    the ROM and voice group the sequence really comes from (a song undub replaces plays the Japanese sequence on
    the regional copy of the Japanese voice group, port_undub.c groupDelta)."""
    seq = seq or rom
    count = seq[header]
    tone = (u32(seq, header + 4) - 0x08000000) if tone is None else tone
    used = set()
    for t in range(count):
        p = u32(seq, header + 8 + 4 * t) - 0x08000000
        for k in range(p, p + 512):
            if seq[k] == 0xBD:
                used.add(seq[k + 1])
            if seq[k] == 0xB1:  # FINE
                break
    out = []
    for i in sorted(used or {0}):
        e = tone + 12 * i
        if rom[e] & 0x07 == 0 and rom[e] < 0x40:  # a DirectSound instrument: its sample pointer at +4
            out.append((e, u32(rom, e + 4) - 0x08000000))
    return out


def sample_hash(rom, off):
    size = u32(rom, off + 12)
    return hashlib.md5(rom[off + 16:off + 16 + min(size, 0x40000)]).hexdigest()


def track_bytes(rom, header):
    p = u32(rom, header + 8) - 0x08000000
    end = rom.index(b'\xb1', p)
    return rom[p:end + 1]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--region', default='eu')
    args = ap.parse_args()
    code = {'eu': 'B8CP', 'us': 'B8CE'}[args.region]
    rom = (ROOT / 'roms' / f'{code}.gba').read_bytes()
    jp = (ROOT / 'roms' / 'B8CJ.gba').read_bytes()
    rs, js = syms(args.region), syms('jp')
    samples, songs = undub_map(args.region)
    names = [n for n in rs if n.startswith('gSongVo') and n in js and not n.endswith(('Frame0', 'Tiles'))]
    english, timing = [], []
    for n in sorted(names):
        rh, jh = rs[n] - 0x08000000, js[n] - 0x08000000
        jsamp = [sample_hash(jp, s) for _, s in instruments(jp, jh)]
        final = []
        if n[7:] in songs:  # replaced: the Japanese sequence on the regional copy of its voice group
            delta = {'eu': 0x96A20, 'us': 0x26AF4}[args.region]
            used = instruments(rom, jh, seq=jp, tone=u32(jp, jh + 4) - 0x08000000 + delta)
        else:
            used = instruments(rom, rh)
        for e, s in used:
            m = samples.get(e + 4)
            final.append(hashlib.md5(jp[m[1] + 16:m[1] + 16 + min(u32(jp, m[1] + 12), 0x40000)]).hexdigest()
                         if m and m[1] else sample_hash(rom, s))
        if final and jsamp and not set(final) <= set(jsamp):
            english.append(n[7:])
        if n[7:] not in songs and track_bytes(rom, rh) != track_bytes(jp, jh):
            timing.append(n[7:])
    print(f'{args.region}: {len(names)} voice songs')
    print(f'  still playing a regional sample ({len(english)}): {" ".join(english)}')
    print(f'  sequence differs from the Japanese, not replaced ({len(timing)}): {" ".join(timing)}')
    return 1 if english or timing else 0


if __name__ == '__main__':
    raise SystemExit(main())
