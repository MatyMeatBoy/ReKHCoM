"""Private, native-pixel wide inspection of original ROM scene profiles.

No actors or hardware blending: this is a tilemap diagnostic, not a game shot.
The yellow rectangle marks the previously reviewed 320x180 world area.
"""
import json
from functools import lru_cache
from PIL import Image, ImageDraw
import scene_tile_data as sc
import scene061_tile_preview as base

ROOT = base.ROOT
OUT = ROOT / 'build/scene-review/wide-review'
VIEW = (960, 540)
CAMERAS = {57: (7, 43), 61: (14, 55), 74: (0, 0)}


def source(event, layer, x, y):
    if event == 57:
        if layer == 'bg3' and 0 <= x <= 4 and 18 <= y <= 22:
            return x+8, y-4
        if layer == 'bg3' and 32 <= x <= 35 and 5 <= y <= 11:
            return x-15, y+4
        if layer == 'bg2' and x >= 32:
            return 0, 0
        steps = ((8,-4),(-8,4),(8,4),(-8,-4))
    elif event == 61:
        # as port_scene061_tiles.h (PortScene061TileSource): the completed floor diamond over the old map edge
        if -5 <= x <= 3 and 3 <= y <= 7:
            return (x+12,y+6) if layer == 'bg3' else (0,3)
        if layer == 'bg2' and 32 <= x <= 36 and y == 5:
            return (16,17,2,3,16)[x-32], 0
        steps = ((4,2),(-4,2),(-4,2),(-4,-2)) if layer == 'bg3' else ((12,6),(-12,-6),(12,6),(-4,-2))
    else:
        if x < 0 or y < 0:
            return None
        if layer == 'bg2' and 32 <= x <= 34 and 17 <= y <= 21:
            return x-20,y+1
        bottom = (4,-2) if layer == 'bg3' else (-2,-1) if layer == 'bg2' else (-4,-2)
        steps = ((2,1),(-4,-2),(2,1),bottom)
    for _ in range(64):
        if 0 <= x < 32:
            break
        dx,dy = steps[0 if x < 0 else 1]
        x,y=x+dx,y+dy
    for _ in range(64):
        if 0 <= y < 32:
            break
        dx,dy = steps[2 if y < 0 else 3]
        if not 0 <= x+dx < 32:
            dx=-dx
        x,y=x+dx,y+dy
    return (x,y) if 0 <= x < 32 and 0 <= y < 32 else None


def corrected_source(event, layer, x, y):
    if event == 57:
        if 0 <= x < 32 and 0 <= y < 32:
            return source(event,layer,x,y)
        if layer == 'bg3' and 32 <= x <= 35 and 5 <= y <= 11:
            return source(event,layer,x,y)
        # Repeat the diagonal seam, not the entire rectangular map. Windows
        # and the unique vine are overlays, not vertical wall textures.
        candidates=[]
        for sx in range(x%8,32,8):
            sy=y-(sx-x)//2
            if 0 <= sy < 32:
                candidates.append((sx if layer=='bg2' else abs(sx-12),sx,sy))
        if candidates:
            _,sx,sy=min(candidates)
            return sx,sy
        if layer == 'bg2':
            return 0,0
        # Plain wooden boards above, intact sandy texture below the wall.
        return (x%8, (y%4)) if y+x/2 < 15 else (8+x%8,28+y%4)
    if event == 74:
        if x<0 or y<0:
            return None
        if x<32 and y<32:
            return x,y
        if layer=='bg2' and 32<=x<=34 and 17<=y<=21:
            return x-20,y+1
        if layer=='bg2' and x>=35:
            sx=7+(x-27)%20
            repeat=(x-27)//20
            sy=y-repeat*10-4
            if repeat>=1 and sx<=14 and 18<=sy<=21:
                return sx,sy
            if y-x/2>=1:
                return 0,0
        period=4
        candidates=[]
        for sx in range(x%period,32,period):
            sy=y+(sx-x)//2
            if 0<=sy<32:
                candidates.append((-sx,sx,sy))
        if candidates:
            _,sx,sy=min(candidates)
            return sx,sy
        return None
    if 0 <= x < 32 and 0 <= y < 32:
        return source(event, layer, x, y)
    if layer == 'bg2' and 32 <= x <= 36 and y == 5:
        return source(event,layer,x,y)
    if -5 <= x <= 3 and 3 <= y <= 7:
        return source(event,layer,x,y)
    # Keep the same diagonal invariant while entering the source map. Crossing
    # the bottom must not bend the floor up into the lower vertical facade.
    period = 4 if layer == 'bg3' else 12
    candidates=[]
    for sx in range(x % period,32,period):
        sy=y+(sx-x)//2
        if 0 <= sy < 32:
            candidates.append((abs(sx-16),sx,sy))
    if candidates:
        _,sx,sy=min(candidates)
        return sx,sy
    # Past the source's vertical coverage, use plain original vertical panels.
    # Their four-column phase follows the source columns, without floor motifs.
    if y-x/2 > 16:
        return x%4,31
    if layer=='bg3':
        return 24+x%4,0
    sx=20+x%12
    sy=y+(sx-x)//2
    return sx,2+(sy-2)%8


def render(event, extended, corrected=False):
    _,address=sc.bg_symbol(event)
    bg=sc.load_bg(address)
    palette = {57:'camera057/st.khst',74:'palette074/frame99.khst'}
    if event in palette:
        bg['colors']=sc.active_bg_palette(ROOT/'build/scene-review'/palette[event])
    camera=CAMERAS[event]
    origin=(camera[0]-(VIEW[0]-240)//2,camera[1]-(VIEW[1]-160)//2)
    image=Image.new('RGB',VIEW,bg['colors'][0])

    @lru_cache(None)
    def tile(layer,entry):
        data=bg['tiles2'] if layer=='bg1' else bg['tiles']
        t=Image.new('RGBA',(8,8))
        for yy in range(8):
            for xx in range(8):
                sx=7-xx if entry&1024 else xx
                sy=7-yy if entry&2048 else yy
                offset=(entry&1023)*32+sy*4+sx//2
                c=(data[offset]>>(4*(sx%2)))&15 if offset<len(data) else 0
                rgb=bg['colors'][(entry>>12)*16+c]
                t.putpixel((xx,yy),(*rgb,255 if c or layer=='bg3' else 0))
        return t

    if event==74:
        # The outer void belongs to the foreground mask, not palette entry 0.
        image.paste(tile('bg1',bg['bg1'][0][0]).getpixel((0,0))[:3],(0,0,*VIEW))
    for layer in ('bg3','bg2','bg1'):
        grid=bg[layer]
        if grid is None:
            continue
        for y in range(origin[1]//8,(origin[1]+VIEW[1]+7)//8):
            for x in range(origin[0]//8,(origin[0]+VIEW[0]+7)//8):
                lookup=corrected_source if corrected else source
                pos=lookup(event,layer,x,y) if extended else ((x,y) if 0<=x<32 and 0<=y<32 else None)
                if pos is None:
                    continue
                sx,sy=pos
                t=tile(layer,grid[sy][sx])
                image.paste(t,(x*8-origin[0],y*8-origin[1]),t)
    return image


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    for event in (57,61,74):
        for extended in (False,True):
            image=render(event,extended)
            label='current' if extended else 'original'
            image.save(OUT/f'{event:03d}-{label}.png')
        marked=image.copy()
        draw=ImageDraw.Draw(marked)
        draw.rectangle((320,180,639,359),outline='#ffff00',width=1)
        draw.text((320,166),'ENCUADRE ANTERIOR',fill='#ffff00')
        marked.save(OUT/f'{event:03d}-scope.png')
        render(event,True,True).save(OUT/f'{event:03d}-correction.png')
    (OUT/'scope.json').write_text(json.dumps({'view':VIEW,'cameras':CAMERAS,'status':'diagnostic; broader approval pending','actors':False},indent=2))
    print(OUT)


if __name__=='__main__':
    main()
