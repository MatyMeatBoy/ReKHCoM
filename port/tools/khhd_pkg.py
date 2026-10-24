#!/usr/bin/env python3
"""Read the packages of the player's own KINGDOM HEARTS -HD 1.5+2.5 ReMIX- (PC: Steam or Epic), for the HD mod.

The format is the one OpenKh documents and implements (github.com/OpenKH/OpenKh, OpenKh.Egs, Apache-2.0):
- <name>.hed: 0x20-byte entries: MD5 of the file name (UTF-8), offset in <name>.pkg (int64), stored length, real length;
- each asset in the .pkg: a 16-byte header that is also the key seed (decompressed length, remastered count,
  compressed length: -2 plain, -1 not compressed, else zlib size; date), the remastered entries (0x30 bytes each),
  then the data: its first 0x100 bytes XORed with a key expanded from the seed, then zlib (2-byte header + deflate).
Nothing from the game is shipped with ReKHCoM: the launcher reads the player's installation.

    python port/tools/khhd_pkg.py list "<install>/Image/dt/Recom.hed" [--names names.txt]
    python port/tools/khhd_pkg.py get  "<install>/Image/dt/Recom.hed" <name or md5> out.bin
"""
import argparse
import hashlib
import struct
import sys
import zlib
from pathlib import Path

MASTER = bytes.fromhex(
    '7e8897550b06f108ebbb141cd87aec4134b2a346ef6bfee1cf53a50512d28e52'
    '4a80e981b0f0b49cff0f1513da734e77bed730e5f65a113767bc836f2776d0cd'
    '690d2e514290b8b64cadce5b1a1ff5af01f85e3a6e688be89fc9d9269229c833'
    '983254d4442566ac5f9921e48f1dc2d5a462f90261de59e7079afa2f953f86d3'
    '78a775edd62d6487bdc7c1aaf28c17cb318ac3cc04ee6aab5c2270ca9e716d85'
    '455db9a9a61047fb827d847bc6e238fc2b0e209dc5f339a8a06558437ce33618'
    '724979aed17440c4914f2463bfba239650b357df1e03487f354d3ee6a1dd093c'
    '3d3b568d932a9b4b0c28b1e0608919db2cf76cb51b94c0dceab70af416fda200')
SCRAMBLE = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36]
PASSES = 10


def make_key(seed):
    key = bytearray(0xB0)
    for i, b in enumerate(seed):
        key[i] = b if b else i
    for i in range(PASSES * 4):
        frame = key[0x0C + i * 4:0x10 + i * 4]
        if i % 4 == 0:
            sk = SCRAMBLE_BYTES[i:i + 4]  # OpenKh indexes ScrambleKey bytewise: (round constant, 0, 0, 0)
            frame = bytes([MASTER[frame[1]] ^ sk[0], MASTER[frame[2]] ^ sk[1], MASTER[frame[3]] ^ sk[2],
                           MASTER[frame[0]] ^ sk[3]])
        for k in range(4):
            key[0x10 + i * 4 + k] = key[i * 4 + k] ^ frame[k]
    return bytes(key)


# OpenKh's ScrambleKey as bytes (10 little-endian ints), indexed by i + k exactly as the C# does
SCRAMBLE_BYTES = b''.join(struct.pack('<I', s) for s in SCRAMBLE) + bytes(8)


def decrypt_head(key, data):
    data = bytearray(data)
    for off in range(0, min(len(data), 0x100), 0x10):
        n = min(len(data) - off, 0x10)
        for p in range(PASSES, -1, -1):
            for j in range(n):
                data[off + j] ^= key[0x10 * p + j]
    return bytes(data)


def entries(hed):
    raw = Path(hed).read_bytes()
    for i in range(len(raw) // 0x20):
        md5, offset, stored, real = struct.unpack_from('<16sqii', raw, i * 0x20)
        yield md5.hex(), offset, stored, real


def read_asset(pkg, offset, with_remastered=False):
    pkg.seek(offset)
    seed = pkg.read(0x10)
    dlen, rcount, clen, _date = struct.unpack('<iiii', seed)
    key = make_key(seed)
    remastered = []
    for _ in range(rcount):
        name, roff, orig_off, rdlen, rclen = struct.unpack('<32siiii', pkg.read(0x30))
        remastered.append((name.split(b'\0')[0].decode('latin1'), rdlen, rclen))
    size = clen if clen >= 0 else dlen
    packed = pkg.read(size)
    if clen > -2:
        packed = decrypt_head(key, packed)
    data = zlib.decompress(packed[2:], -15, dlen) if clen > -1 else packed
    extra = {}
    if with_remastered:
        for name, rdlen, rclen in remastered:
            n = rclen if rclen >= 0 else rdlen
            pos = pkg.tell()
            pkg.seek((pos + 15) & ~15)
            blob = pkg.read(n + (-n % 16))
            if rclen > -2:
                blob = decrypt_head(key, blob)
            extra[name] = zlib.decompress(blob[2:], -15, rdlen) if rclen > -1 else blob[:rdlen]
    return data, remastered, extra


def kind(data):
    head = data[:16]
    for magic, name in ((b'ftyp', 'mp4'), (b'CRID', 'usm'), (b'SEDBSSCF', 'scd'), (b'RIFF', 'riff'), (b'OggS', 'ogg'),
                        (b'BIKi', 'bik'), (b'\x89PNG', 'png'), (b'DDS ', 'dds')):
        if magic in head[:12]:
            return name
    return head[:4].hex()


def names_by_md5(path):
    out = {}
    if path:
        for line in Path(path).read_text(encoding='utf-8', errors='replace').splitlines():
            line = line.strip()
            if line:
                out[hashlib.md5(line.encode('utf-8')).hexdigest()] = line
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('command', choices=['list', 'get'])
    ap.add_argument('hed')
    ap.add_argument('name', nargs='?')
    ap.add_argument('out', nargs='?')
    ap.add_argument('--names')
    ap.add_argument('--min', type=int, default=0, help='list: only assets at least this large')
    args = ap.parse_args()
    names = names_by_md5(args.names)
    pkg_path = Path(args.hed).with_suffix('.pkg')
    with open(pkg_path, 'rb') as pkg:
        if args.command == 'list':
            for md5, offset, stored, real in entries(args.hed):
                if real < args.min:
                    continue
                pkg.seek(offset)
                seed = pkg.read(0x10)
                dlen, rcount, clen, _ = struct.unpack('<iiii', seed)
                pkg.seek(offset + 0x10 + rcount * 0x30)
                head = pkg.read(0x100 if clen != -1 else 16)
                if clen > -2:
                    head = decrypt_head(make_key(seed), head)
                k = kind(zlib.decompressobj(-15).decompress(head[2:], 16) if clen > -1 else head)
                print(f'{md5} {real:>11} {k:<8} {names.get(md5, "?")}')
        else:
            target = args.name.lower()
            md5 = target if len(target) == 32 and all(c in '0123456789abcdef' for c in target) else \
                hashlib.md5(args.name.encode('utf-8')).hexdigest()
            for m, offset, stored, real in entries(args.hed):
                if m == md5:
                    data, _, _ = read_asset(pkg, offset)
                    Path(args.out).write_bytes(data)
                    print(f'{args.out}: {len(data)} bytes ({kind(data)})')
                    return
            sys.exit(f'{args.name}: not in {args.hed}')


if __name__ == '__main__':
    main()
