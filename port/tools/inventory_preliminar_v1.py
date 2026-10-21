from pathlib import Path
import argparse,sys,re,json,struct
from PIL import Image,ImageDraw
parser=argparse.ArgumentParser(description='Inventory event backgrounds from one regional ROM.')
parser.add_argument('--region',choices=('eu','us','jp'),default='eu')
region=parser.parse_args().region
root=Path.cwd();sys.path.insert(0,'port/tools');import scene_tile_data as s
s.ROM=(root/'roms'/{'eu':'B8CP.gba','us':'B8CE.gba','jp':'B8CJ.gba'}[region]).read_bytes()
out=root/'build/preliminar-v1/inventory'/region;out.mkdir(parents=True,exist_ok=True)
src=(root/'src/evt/event_index_data.c').read_text();body=src.split('gEventBackgroundDefs[] = {',1)[1].split('};',1)[0];names=re.findall(r'(NULL|&gEventBg\w+)',body)
groups={}
for i,n in enumerate(names):
 if n!='NULL':groups.setdefault(n[1:],[]).append(i)
syms=(root/f'build/{region}/pc/rom_symbols.s').read_text();rows=[];thumbs=[]
for n,events in groups.items():
 try:
  address=int(re.search(r'\.set _'+n+r', (0x[0-9a-fA-F]+)',syms)[1],16);d=s.at(address,48)
  if d[36]:rows.append(dict(symbol=n,events=events,kind='affine',address=address));continue
  b=s.load_bg(address);w=b['w']*256;h=b['h']*256
  if w>2048 or h>2048:continue
  im=Image.new('RGB',(w,h),b['colors'][0]);cache={}
  hashes={};bounds={}
  for layer in ('bg3','bg2','bg1'):
   g=b[layer]
   if not g:continue
   hs=2166136261;occupied=[]
   for y,row in enumerate(g):
    for x,e in enumerate(row):
     hs=((hs^e)*16777619)&0xffffffff
     key=(layer,e)
     if key not in cache:
      td=b['tiles2'] if layer=='bg1' and b['tiles2'] else b['tiles'];t=Image.new('RGBA',(8,8));p=t.load()
      for yy in range(8):
       for xx in range(8):
        fx=7-xx if e&1024 else xx;fy=7-yy if e&2048 else yy;o=(e&1023)*32+fy*4+fx//2;ci=(td[o]>>(fx%2*4))&15 if o<len(td) else 0;p[xx,yy]=(*b['colors'][(e>>12)*16+ci],255 if ci else 0)
      cache[key]=t
     t=cache[key]
     if t.getbbox():occupied.append((x,y));im.paste(t,(x*8,y*8),t)
   hashes[layer]=hs
   bounds[layer]=[min(x for x,y in occupied),min(y for x,y in occupied),max(x for x,y in occupied)+1,max(y for x,y in occupied)+1] if occupied else None
  im.save(out/(n+'.png'));thumb=im.copy();thumb.thumbnail((192,192),Image.Resampling.NEAREST);thumbs.append((n,events,thumb))
  rows.append(dict(symbol=n,events=events,kind='text',address=address,width=w,height=h,hashes=hashes,bounds=bounds))
 except Exception as ex:rows.append(dict(symbol=n,events=events,error=str(ex)))
cols=6;sheet=Image.new('RGB',(cols*208,((len(thumbs)+cols-1)//cols)*228),(30,32,38));dr=ImageDraw.Draw(sheet)
for i,(n,events,im) in enumerate(thumbs):
 x=i%cols*208;y=i//cols*228;dr.text((x+4,y+4),n,fill='white');dr.text((x+4,y+17),','.join(map(str,events)),fill='white');sheet.paste(im,(x+4,y+32))
sheet.save(out/'contact.png');(out/'scenes.json').write_text(json.dumps(rows,indent=2));print('unique',len(rows),'text',len(thumbs));print([(r['symbol'],r.get('error')) for r in rows if 'error'in r])
