"""Audit original Maleficent parts and group shared body cuts for Atlas.

Frame0 initially loads the common tile bank. Later frames replace its prefix,
so decoding a later frame in isolation loses the shared torso. This tool
renders local ROM originals only; it never invents a completed sprite.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

import yaml
from PIL import Image
from boss_sprite_review import group_cut_regions

ROOT = Path(__file__).resolve().parents[3]
SIZE = (512, 384)


def records(source):
    body = source.split('static const MdFrameDef sMdFrameDefs[41] = {', 1)[1].split('\n};', 1)[0]
    depth = 0
    start = 0
    result = []
    for i, c in enumerate(body):
        if c == '{':
            if depth == 0:
                start = i
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                result.append(body[start:i + 1])
    assert len(result) == 41
    return result


def signed(n):
    n = int(n, 0)
    return n - 65536 if n >= 32768 else n


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT/'build/maleficent-native-audit')
    a = parser.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    rom = (ROOT/'roms/B8CP.gba').read_bytes()
    symbols = {m[1]:int(m[2],16) for m in re.finditer(r'\.set _(\w+), 0x([0-9a-fA-F]+)',
               (ROOT/'build/eu/pc/rom_symbols.s').read_text())}
    def data(name, size):
        at = symbols[name] - 0x8000000
        end=min((v for v in symbols.values() if v>symbols[name]),default=symbols[name]+size)-0x8000000
        return rom[at:min(at+size,end)]
    words = struct.unpack('<16H', data('gBosMdPalette',32))
    pal = [tuple(((v>>s)&31)*255//31 for s in (0,5,10)) for v in words]
    source = (ROOT/'src/bos/bos_md.c').read_text()
    animations = {}
    for name, body in re.findall(r'static const MdAnimFrame (\w+)\[\d+\] = \{(.*?)\};', source, re.S):
        for slot, duration in re.findall(r'\{\s*(\d+),\s*(\d+)\s*\}', body):
            animations.setdefault(int(slot), []).append({'animation':name,'duration':int(duration)})
    sheets = yaml.safe_load((ROOT/'config/assets/sprites_worldinspect.yaml').read_text())
    if isinstance(sheets, dict):
        sheets = next(v for v in sheets.values() if isinstance(v,list))
    objframes = {}
    for sheet in sheets:
        if isinstance(sheet,dict) and sheet.get('format') == 'sprite_sheet':
            for f in sheet.get('frames',[]):
                objframes[f['symbol']] = f.get('pieces',[])
    def rgba(indices):
        im=Image.new('RGBA',indices.size)
        im.putdata([(*pal[v&15],255) if v else (0,0,0,0) for v in indices.tobytes()])
        return im
    # The runtime first loads the full frame0 bank (including shared torso
    # tiles), then updates only each subsequent frame's prefix.
    base_tiles=data('gBosMdFrame0Tiles',32768)
    frames=[]
    for slot, record in enumerate(records(source)):
        bx,by,tiles,mapname = re.search(r'\{\s*(\d+),\s*(\d+),\s*(\w+Tiles),.*?\{[^}]*\b(gBosMdFrame\d+Map)',record,re.S).groups()
        bx,by=signed(bx),signed(by)
        rawtiles=data(tiles,32768);rawtiles+=base_tiles[len(rawtiles):];rawmap=data(mapname,2048)
        native=Image.new('L',(256,256))
        for y in range(256):
            for x in range(256):
                e=struct.unpack_from('<H',rawmap,((y//8)*32+x//8)*2)[0]
                xx=7-x%8 if e&1024 else x%8;yy=7-y%8 if e&2048 else y%8
                at=(e&1023)*32+yy*4+xx//2
                if at<len(rawtiles):native.putpixel((x,y),(rawtiles[at]>>((xx&1)*4))&15)
        before=Image.new('L',SIZE);before.paste(native,(0,0))
        arms=re.findall(r'\{\s*(0x[\da-fA-F]+|\d+),\s*(0x[\da-fA-F]+|\d+),\s*(0x[\da-fA-F]+|\d+),\s*(\w+Tiles),\s*[^,]+,\s*(\w+Frame\d+)\s*\}',record)
        assert len(arms)==2,(slot,arms)
        for ax,ay,az,art,frame in arms:
            raw=data(art,32768);ox=160+signed(ax)-bx;oy=128+signed(ay)+signed(az)-by
            for shape,px,py,*options in objframes[frame]:
                opt=options[0] if options else {};w,h=map(int,shape.split('x'));tile=opt.get('tile',0)
                for y in range(h):
                    for x in range(w):
                        xx=w-1-x if opt.get('hflip') else x;yy=h-1-y if opt.get('vflip') else y
                        at=(tile+(yy//8)*(w//8)+xx//8)*32+(yy%8)*4+(xx%8)//2
                        v=(raw[at]>>((xx&1)*4))&15 if at<len(raw) else 0;dx=ox+px+x;dy=oy+py+y
                        if v and 0<=dx<SIZE[0] and 0<=dy<SIZE[1]:before.putpixel((dx,dy),v)
        stem=f'maleficent-{slot:02d}'
        mounted=a.out/(stem+'-mounted-original.png')
        body=a.out/(stem+'-body-original.png')
        rgba(before).save(mounted);rgba(native).save(body)
        edge=[y for y in range(256) if native.getpixel((255,y))]
        frames.append(dict(frame=slot,source=mapname,tiles=tiles,
            objects=[v[-1] for v in arms],animations=animations.get(slot,[]),
            body=str(body),mounted=str(mounted),mode='sprite',
            body_sha=hashlib.sha256(native.tobytes()).hexdigest(),
            seam_hash=hashlib.sha256(native.crop((248,0,256,256)).tobytes()).hexdigest() if edge else None,
            cut_bounds=[255,min(edge),256,max(edge)+1] if edge else None))
    cuts=group_cut_regions(frames)
    result=dict(boss='Maleficent',mode='sprite',frames=frames,cut_groups=cuts,
        coverage=dict(slots=len(frames),body_drawings=len({f['body_sha'] for f in frames}),cut_groups=len(cuts)),
        limitations=['Original ROM renders, not completed or approved art.',
                     'A shared cut is reviewed once with original body and assembled pose; other poses are coverage evidence.'])
    (a.out/'manifest.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result['coverage']))


if __name__=='__main__':
    main()
