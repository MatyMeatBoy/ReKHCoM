"""Reproducible native-data proposal: extensions only beyond ROM block x256.

Does not modify the source checkout, ROM, game renderer or original opaque pixels.
Reads an actual feedback submission and links a child review to its exact cloud ID.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from server import ROOT, Store, now


def frame(source, slot):
    rom = (source / 'roms/B8CP.gba').read_bytes()
    symbols = {m[1]: int(m[2],16) for m in re.finditer(r'\.set _(\w+), 0x([0-9a-f]+)',
                (source / 'build/eu/pc/rom_symbols.s').read_text())}
    addresses = sorted(set(symbols.values()))
    def get(name):
        address = symbols[name]
        return rom[address-0x08000000:addresses[addresses.index(address)+1]-0x08000000]
    code = (source/'src/bos/bos_dsd_main.c').read_text()
    maps = [row.split(',')[2].strip() for row in re.findall(r'\{([^}]*)\}',
        re.search(r'gBosDsdFrameMaps\[47\]\[4\] = \{(.*?)\};',code,re.S)[1])]
    tiles = [s.strip() for s in re.search(r'gBosDsdFrameTiles\[47\] = \{(.*?)\};',code,re.S)[1].split(',') if s.strip()]
    m,t = get(maps[slot]),get(tiles[slot])
    original = {}
    for y in range(256):
        for x in range(256):
            at=((y//8)*32+x//8)*2
            e=int.from_bytes(m[at:at+2],'little')
            px=7-x%8 if e&0x400 else x%8
            py=7-y%8 if e&0x800 else y%8
            offset=(e&1023)*32+py*4+px//2
            value=((t[offset]>>((px&1)*4))&15) if offset<len(t) else 0
            if value:original[x,y]=value
    raw=get('gBosDsdBgPalette')
    palette=[]
    for i in range(0,32,2):
        c=int.from_bytes(raw[i:i+2],'little')
        palette.append(((c&31)<<3,((c>>5)&31)<<3,((c>>10)&31)<<3))
    return original,palette


def extension(inc):
    top=int(re.search(r'#define DARKSIDE_\w+_TOP (\d+)',inc)[1])
    rows=re.findall(r'\{\s*(0x[^}]+)\}',inc)
    pixels={}
    for y,row in enumerate(rows):
        for j,b in enumerate(re.findall(r'0x([0-9a-fA-F]+)',row)):
            value=int(b,16)
            for dx,c in [(j*2,value&15),(j*2+1,value>>4)]:
                if c:pixels[232+dx,top+y]=c
    return pixels


def draw_review(path, original, added, palette, bbox, font_path):
    natural=Image.new('RGB',(360,256),'#e8e9df')
    provenance=natural.copy()
    visible={p:c for p,c in added.items() if p not in original}
    for (x,y),c in {**visible,**original}.items():
        natural.putpixel((x,y),palette[c])
        provenance.putpixel((x,y),(232,135,60) if (x,y) in visible else (54,126,175))
    w,h=(bbox[2]-bbox[0])*2,(bbox[3]-bbox[1])*2
    natural=natural.crop(bbox).resize((w,h),Image.Resampling.NEAREST)
    provenance=provenance.crop(bbox).resize((w,h),Image.Resampling.NEAREST)
    pane=max(420,w)
    figure=Image.new('RGB',(pane*2+72,h+154),'#fafbf5')
    d=ImageDraw.Draw(figure)
    font=ImageFont.truetype(str(font_path),22)
    small=ImageFont.truetype(str(font_path),16)
    d.text((24,17),'03  DARKSIDE / BRAZO EN ALTO / propuesta 2',font=font,fill='#274d43')
    d.text((24,53),'Cambio: añadidos solamente desde x256',font=small,fill='#526653')
    d.text((pane+48,53),'AZUL = original   NARANJA = añadido',font=small,fill='#526653')
    figure.paste(natural,(24,84));figure.paste(provenance,(pane+48,84))
    seam=pane+48+(256-bbox[0])*2
    for y in range(84,84+h,12):d.line((seam,y,seam,min(y+6,84+h)),fill='#bd512a',width=2)
    d.text((24,h+100),'Propuesta hija: original intacto; eliminados los añadidos que invadían el bloque izquierdo.',font=small,fill='#526653')
    d.text((24,h+124),'Pendiente de tu revisión estética. No aprobada ni integrada en la build.',font=small,fill='#526653')
    figure.save(path)


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--source',type=Path,required=True,help='Checkout containing the V2 native inc and user ROM')
    p.add_argument('--submission',type=Path,required=True,help='comentarios.json from actual human submission')
    p.add_argument('--cloud',required=True)
    p.add_argument('--data',type=Path,default=ROOT/'build/art-review')
    p.add_argument('--font',type=Path,default=Path('C:/Windows/Fonts/arial.ttf'))
    args=p.parse_args()
    payload=json.loads(args.submission.read_text(encoding='utf-8'))
    parent=next(a for a in payload['assets'] if any(n['id']==args.cloud for n in a['annotations']))
    snapshot=args.submission.parent/parent['snapshot']
    if hashlib.sha256(snapshot.read_bytes()).hexdigest()!=parent['sha256']:
        raise ValueError('Feedback snapshot SHA mismatch')
    inc_path=args.source/'port/include/port/darkside_body_raise.inc'
    inc=inc_path.read_text()
    source_hash=hashlib.sha256(inc.encode()).hexdigest()
    if source_hash not in parent['provenance']:
        raise ValueError('Source inc differs from the version reviewed; do not apply feedback to another version')
    old=extension(inc)
    new={point:color for point,color in old.items() if point[0]>=256}
    original,palette=frame(args.source,41)
    allpixels={**old,**original}
    bbox=(min(x for x,y in allpixels)-8,max(0,min(y for x,y in allpixels)-8),
          max(x for x,y in allpixels)+9,min(256,max(y for x,y in allpixels)+9))
    checks=[]
    for slot in range(41,46):
        base,_=frame(args.source,slot)
        combined={**{point:color for point,color in new.items() if point not in base},**base}
        assert all(combined[point]==color for point,color in base.items())
        assert all(point[0]>=256 for point in combined if point not in base)
        checks.append(dict(frame=slot,original_opaque_pixels_changed=0,added_pixels_left_of_256=0,
                           removed_visible_additions=sum(point[0]<256 and point not in base for point in old)))
    output=args.data/'proposals/darkside-03-boundary-v2'
    output.mkdir(parents=True,exist_ok=True)
    # Modify editable native data: 12 packed bytes encode x232..255 on every row.
    def strip_left(match):
        values=re.findall(r'0x[0-9a-fA-F]+',match[1])
        return '{ '+', '.join(['0x00']*12+values[12:])+' }'
    modified=re.sub(r'\{\s*(0x[^}]+)\}',strip_left,inc)
    (output/'darkside_body_raise_v2.inc').write_text(modified,encoding='utf-8')
    assert extension(modified)==new
    path=output/'darkside-03-v2.png'
    draw_review(path,original,new,palette,bbox,args.font)
    info=dict(created=now(),response_to=[dict(submission=payload['submission_id'],image_id=parent['id'],cloud_id=args.cloud,
               reviewed_image_sha256=parent['sha256'])],
        change='Eliminar únicamente los píxeles añadidos x232..255. Conservar todos los píxeles originales y los añadidos x>=256.',
        technique='Edición determinista de índices 4bpp nativos; 12 bytes iniciales a cero en cada fila. Prioridad del original intacta.',
        hypothesis='Cuando el humano restringe la extensión al exterior del bloque, la transparencia dentro del bloque no autoriza añadir arte.',
        checks=checks,limitations=['Esta iteración resuelve la invasión izquierda; el dibujo y sombreado del añadido derecho se mantienen para aislar el cambio.',
            'Revisión de sprite nativo; aún no validada en una build ni aprobada por el usuario.'],
        source_inc_normalized_sha256=source_hash,proposal_inc_sha256=hashlib.sha256((output/'darkside_body_raise_v2.inc').read_bytes()).hexdigest(),
        subsequent_human_verdict=None)
    (output/'revision.json').write_text(json.dumps(info,ensure_ascii=False,indent=2),encoding='utf-8')
    item=Store(args.data).add(path.read_bytes(),'Darkside 03 · Brazo en alto · v2','Propuesta 2 · límite derecho · pendiente',
        'Respuesta a la nube '+args.cloud+'. Azul: píxeles originales. Naranja: añadidos exclusivamente x>=256. Propuesta privada; no integrada.',
        parent_id=parent['id'],revision_info=info)
    (output/'review-id.txt').write_text(item['id'],encoding='utf-8')
    print(json.dumps(dict(id=item['id'],path=str(path),checks=checks),ensure_ascii=False,indent=2))


if __name__=='__main__':main()
