#!/usr/bin/env python3
"""Square Enix SCD (KH 1.5+2.5 PC music) to a plain Ogg Vorbis file and its loop points, for the HD music mod.

Layout seen in Re:CoM's STREAM/0001/*.win32.scd (one stream each, codec 6 = Ogg Vorbis):
- "SEDBSSCF" header; the sound entry: stream size, channels, rate, codec, loop start/end (byte offsets in the
  Ogg data), extradata size, aux chunk count (0x20 bytes);
- the Ogg extradata: version (2), header size (0x20), XOR byte, ..., seek table size, Vorbis header size; then the
  seek table, then the Vorbis header pages XORed with that byte, then the audio pages as they are.
The loop points become sample positions: the granule of the last page that ends before each byte offset.

    python port/tools/khhd_scd.py in.scd out.ogg      # also writes out.loop: "<start sample> <end sample>"
"""
import struct
import sys
from pathlib import Path


def convert(scd):
    if scd[:8] != b'SEDBSSCF':
        raise ValueError('not an SCD file')
    # the sound entry table: header offset 0x30 holds the table counts, the entry offsets follow at 0x60+
    table_off = struct.unpack_from('<I', scd, 0x3C)[0]  # sound entry offset table
    entry = struct.unpack_from('<I', scd, table_off)[0]
    size, channels, rate, codec, loop_start, loop_end, extra, aux = struct.unpack_from('<8i', scd, entry)
    if codec != 6:
        raise ValueError(f'codec {codec}: only Ogg Vorbis (6) is handled')
    pos = entry + 0x20
    for _ in range(aux):  # auxiliary chunks (markers): skip by their own size
        pos += struct.unpack_from('<I', scd, pos + 4)[0]
    version, header_size, xor = scd[pos], scd[pos + 1], scd[pos + 2]
    seek_size, vorbis_size = struct.unpack_from('<II', scd, pos + 0x10)
    head_at = pos + header_size + seek_size
    vorbis = bytearray(scd[head_at:head_at + vorbis_size])
    if version == 2 and xor:
        for i in range(len(vorbis)):
            vorbis[i] ^= xor
    audio = scd[head_at + vorbis_size:head_at + vorbis_size + size]
    ogg = bytes(vorbis) + audio
    if ogg[:4] != b'OggS':
        raise ValueError('the Vorbis header did not decode')

    def sample_at(offset):
        """Granule of the last complete page before the byte offset (offsets count from the audio data)."""
        best, p = 0, 0
        target = len(vorbis) + offset
        while p + 27 <= len(ogg) and ogg[p:p + 4] == b'OggS':
            granule = struct.unpack_from('<q', ogg, p + 6)[0]
            segs = ogg[p + 26]
            body = sum(ogg[p + 27:p + 27 + segs])
            end = p + 27 + segs + body
            if end > target:
                break
            if granule > 0:
                best = granule
            p = end
        return best

    total = sample_at(len(audio))
    loop = (sample_at(loop_start), sample_at(loop_end) if loop_end > loop_start else total)
    return ogg, rate, channels, loop


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    ogg, rate, channels, loop = convert(Path(sys.argv[1]).read_bytes())
    out = Path(sys.argv[2])
    out.write_bytes(ogg)
    out.with_suffix('.loop').write_text(f'{loop[0]} {loop[1]}\n')
    print(f'{out}: {len(ogg)} bytes, {channels} ch, {rate} Hz, loop {loop[0]}-{loop[1]}')


if __name__ == '__main__':
    main()
