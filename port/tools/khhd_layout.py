#!/usr/bin/env python3
"""Re:CoM's 2D layouts (LAD) and sequences (SEQD), as OpenKh documents them for KH2 (docs/kh2/file/type/2ld.md,
OpenKh.Kh2/Layout.cs, Sequence.cs): the battle HUD of the player's Re:CoM (FORM/0002/FO0002.SPR holds gauge.lad,
Cockpit.lad and layout_card.lad; their textures are FO0060's gauge.imz and FO0002's Cockpit.imz). Nothing of it is
shipped: this reads the player's files to learn how Re:CoM composes its HUD (pieces, places, animations), so the
port can draw the same with its own game state.

    python port/tools/khhd_layout.py dump <spr file>                 # every layout, sequence, sprite and animation
    python port/tools/khhd_layout.py render <spr file> <lad name> <imz file> <out dir> [--frame N]
        # one PNG per animation group of each sequence, at frame N (0 by default)
"""
import json
import math
import struct
import sys
from pathlib import Path


def spr_entries(blob):
    """(name, offset, size) of each file in a .SPR container (0x20-byte entries: offset, name, size | 0x80000000)."""
    out = []
    first = struct.unpack_from('<I', blob, 0)[0]
    pos = 0
    while pos < first:
        off = struct.unpack_from('<I', blob, pos)[0]
        name = blob[pos + 4:pos + 0x1c].split(b'\0')[0].decode('latin1')
        size = struct.unpack_from('<I', blob, pos + 0x1c)[0] & 0x7FFFFFFF
        if not name:
            break
        out.append((name, off, size))
        pos += 0x20
    return out


ANIM = struct.Struct('<12i 6f 6f 4f 4f 2h i 2I')  # 144 bytes


def sequence(b):
    magic, unk, *sec, uv = struct.unpack_from('<2I 10i i', b, 0)
    assert magic == 0x44514553, hex(magic)
    (ns, os_), (np_, op), (ng, og), (na, oa), (nag, oag) = [sec[i:i + 2] for i in range(0, 10, 2)]
    sprites = [dict(zip(('palette', 'u0', 'v0', 'u1', 'v1', 'us', 'vs', 'cl', 'ct', 'cr', 'cb'),
                        struct.unpack_from('<5i 2f 4I', b, os_ + i * 44))) for i in range(ns)]
    parts = [dict(zip(('l', 't', 'r', 'b', 'sprite'), struct.unpack_from('<5i', b, op + i * 20))) for i in range(np_)]
    groups = []
    for i in range(ng):
        start, count = struct.unpack_from('<2h', b, og + i * 4)
        groups.append(parts[start:start + count])
    names = ('flags', 'group', 'f0', 'f1', 'tx0', 'tx1', 'ty0', 'ty1', 'px0', 'px1', 'py0', 'py1',
             'rx0', 'rx1', 'ry0', 'ry1', 'rz0', 'rz1', 's0', 's1', 'sx0', 'sx1', 'sy0', 'sy1',
             'cx0', 'cy0', 'cx1', 'cy1', 'bx0', 'bx1', 'by0', 'by1', 'bxn', 'byn', 'blend', 'c0', 'c1')
    anims = [dict(zip(names, ANIM.unpack_from(b, oa + i * ANIM.size))) for i in range(na)]
    agroups = []
    for i in range(nag):
        ai, cnt, loop, flags, fs, fe, x, y, size, u1c, u20 = struct.unpack_from('<4h 7i', b, oag + i * 36)
        agroups.append(dict(anims=anims[ai:ai + cnt], loop=loop, flags=flags, loop_start=fs, loop_end=fe,
                            x=x, y=y, size=size))
    return dict(sprites=sprites, groups=groups, agroups=agroups, uv=uv)


def layout(b):
    magic, ver, n1, o1, n2, o2, ns, os_ = struct.unpack_from('<2I 6i', b, 0)
    assert magic == 0x4459414C, hex(magic)
    props = [dict(zip(('texture', 'sequence', 'agroup', 'show_at', 'x', 'y'), struct.unpack_from('<6i', b, o1 + i * 24)))
             for i in range(n1)]
    sgroups = []
    for i in range(n2):
        idx, cnt, *unk = struct.unpack_from('<2h 4i', b, o2 + i * 20)
        sgroups.append(dict(props=props[idx:idx + cnt], unk=unk))
    offs = list(struct.unpack_from(f'<{ns}i', b, os_)) + [len(b)]
    seqs = [sequence(b[offs[i]:offs[i + 1]]) for i in range(ns)]
    return dict(groups=sgroups, sequences=seqs)


def lerp(a, b, t):
    return a + (b - a) * t


def color(c):
    return (c & 255, (c >> 8) & 255, (c >> 16) & 255, (c >> 24) & 255)


def draw_group(canvas, tex, seq, ag, frame, ox, oy, uvs=1.0):
    """Draw one animation group at a frame, the way OpenKh's SequenceRenderer does (translation, pivot, scale, colour;
    no rotation/bounce: enough to see the composition)."""
    from PIL import Image
    for a in ag['anims']:
        if not (a['f0'] <= frame <= max(a['f1'], a['f0'])):
            continue
        t = 0 if a['f1'] <= a['f0'] else (frame - a['f0']) / (a['f1'] - a['f0'])
        tx, ty = lerp(a['tx0'], a['tx1'], t), lerp(a['ty0'], a['ty1'], t)
        sc = lerp(a['s0'], a['s1'], t) if not a['flags'] & 0x40 else 1.0
        sx = sc * (lerp(a['sx0'], a['sx1'], t) if not a['flags'] & 0x40 else 1.0)
        sy = sc * (lerp(a['sy0'], a['sy1'], t) if not a['flags'] & 0x40 else 1.0)
        tint = color(a['c0']) if a['flags'] & 0x400 else None
        for part in seq['groups'][a['group']]:
            sp = seq['sprites'][part['sprite']]
            u0, v0, u1, v1 = sp['u0'], sp['v0'], sp['u1'], sp['v1']
            if u1 <= u0 or v1 <= v0:
                continue
            piece = tex.crop((round(u0 * uvs), round(v0 * uvs), round(u1 * uvs), round(v1 * uvs)))
            w, h = part['r'] - part['l'], part['b'] - part['t']
            if w == 0 or h == 0:
                continue
            fw, fh = abs(w * sx), abs(h * sy)
            if fw < 1 or fh < 1:
                continue
            piece = piece.resize((max(1, round(fw)), max(1, round(fh))))
            if w * sx < 0:
                piece = piece.transpose(Image.FLIP_LEFT_RIGHT)
            if h * sy < 0:
                piece = piece.transpose(Image.FLIP_TOP_BOTTOM)
            cols = [color(sp[k]) for k in ('cl', 'ct', 'cr', 'cb')]
            avg = [sum(c[i] for c in cols) / 4 for i in range(4)]
            if tint:
                avg = [avg[i] * tint[i] / 128 for i in range(4)]
            r, g, bb, al = piece.split()
            r = r.point(lambda v: min(255, int(v * avg[0] / 128)))
            g = g.point(lambda v: min(255, int(v * avg[1] / 128)))
            bb = bb.point(lambda v: min(255, int(v * avg[2] / 128)))
            al = al.point(lambda v: min(255, int(v * avg[3] / 128)))
            piece = Image.merge('RGBA', (r, g, bb, al))
            x = ox + tx + min(part['l'], part['r']) * sx
            y = oy + ty + min(part['t'], part['b']) * sy
            canvas.alpha_composite(piece, (int(round(x)), int(round(y))))


def main():
    cmd, spr = sys.argv[1], Path(sys.argv[2])
    blob = spr.read_bytes()
    files = {n: blob[o:o + s] for n, o, s in spr_entries(blob)}
    if cmd == 'dump':
        for n, b in files.items():
            lay = layout(b)
            print(f'== {n}: {len(lay["groups"])} layout groups, {len(lay["sequences"])} sequences')
            for gi, g in enumerate(lay['groups']):
                print(f'  group {gi}: {g["props"]}')
            for si, s in enumerate(lay['sequences']):
                print(f'  seq {si}: {len(s["sprites"])} sprites, {len(s["groups"])} sprite groups, '
                      f'{len(s["agroups"])} animation groups')
                for i, sp in enumerate(s['sprites']):
                    print(f'    sprite {i}: uv {sp["u0"]},{sp["v0"]}-{sp["u1"]},{sp["v1"]} colors '
                          f'{sp["cl"]:08x} {sp["ct"]:08x} {sp["cr"]:08x} {sp["cb"]:08x}')
                for i, g in enumerate(s['groups']):
                    print(f'    sgroup {i}: ' + '; '.join(f'{p["sprite"]}@{p["l"]},{p["t"]},{p["r"]},{p["b"]}' for p in g))
                for i, ag in enumerate(s['agroups']):
                    print(f'    agroup {i}: loop {ag["loop"]} {ag["loop_start"]}-{ag["loop_end"]}')
                    for a in ag['anims']:
                        print(f'      anim g{a["group"]} f{a["f0"]}-{a["f1"]} flags {a["flags"]:05x} t({a["tx0"]},{a["ty0"]})->'
                              f'({a["tx1"]},{a["ty1"]}) s{a["s0"]:.2f}->{a["s1"]:.2f} sx{a["sx0"]:.2f}->{a["sx1"]:.2f} '
                              f'sy{a["sy0"]:.2f}->{a["sy1"]:.2f} rz{a["rz0"]:.2f}->{a["rz1"]:.2f} '
                              f'c{a["c0"]:08x}->{a["c1"]:08x}')
        return
    if cmd == 'render':
        from PIL import Image
        sys.path.insert(0, str(Path(__file__).parent))
        from khhd_sprites import decode
        name, imz, out = sys.argv[3], Path(sys.argv[4]), Path(sys.argv[5])
        frame = int(sys.argv[sys.argv.index('--frame') + 1]) if '--frame' in sys.argv else 0
        out.mkdir(parents=True, exist_ok=True)
        lay = layout(files[name])
        texs = []
        raw = imz.read_bytes()
        for i in range(8):
            try:
                texs.append(Image.fromarray(decode(raw, i), 'RGBA'))
            except Exception:
                break
        # the sequences' UVs are the PS2 texture's; the remastered one is larger (--uv: HD / PS2 width, 2 by default)
        uvs = float(sys.argv[sys.argv.index('--uv') + 1]) if '--uv' in sys.argv else 2.0
        tiles = []
        for si, s in enumerate(lay['sequences']):
            for ti, tex in enumerate(texs):
                for ai, ag in enumerate(s['agroups']):
                    canvas = Image.new('RGBA', (400, 300), (40, 40, 60, 255))
                    draw_group(canvas, tex, s, ag, frame, 200, 150, uvs)
                    if canvas.getextrema()[0][1] > 60 or True:
                        tiles.append((f's{si} t{ti} a{ai}', canvas))
        from PIL import ImageDraw
        cols = 8
        sheet = Image.new('RGBA', (cols * 400, ((len(tiles) + cols - 1) // cols) * 300), (0, 0, 0, 255))
        for i, (label, c) in enumerate(tiles):
            ImageDraw.Draw(c).text((4, 4), label, fill='white')
            sheet.paste(c, ((i % cols) * 400, (i // cols) * 300))
        sheet.save(out / f'{Path(name).stem}_f{frame}.png')
        return


if __name__ == '__main__':
    main()
