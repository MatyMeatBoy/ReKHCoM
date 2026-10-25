"""Import the drawn completion into native palette indices; keep ROM pixels.
Local proposal builder, following the existing boss index-art pipeline.
"""
from pathlib import Path
import hashlib,json,re,struct,sys
from PIL import Image,ImageDraw
sys.path.insert(0,str(Path('port/tools/art_review').resolve()))
from boss_sprite_review import comparison
ROOT=Path(__file__).resolve().parents[3];out=ROOT/'build/maleficent-native-completion';out.mkdir(exist_ok=True)
audit=json.loads((ROOT/'build/maleficent-native-audit/manifest.json').read_text())
ref=Image.open(ROOT/'build/maleficent-khcom-lomo/maleficent-lomo-montado-propuesta.png').convert('RGB')
rom=(ROOT/'roms/B8CP.gba').read_bytes();syms=(ROOT/'build/eu/pc/rom_symbols.s').read_text();addr=int(re.search(r'\.set _gBosMdPalette, 0x([0-9a-fA-F]+)',syms)[1],16)-0x8000000
pal=[tuple(((w>>s)&31)*255//31 for s in (0,5,10)) for w in struct.unpack_from('<16H',rom,addr)]
# Reference grid: four reference pixels to one native pixel. Only the
# completed right-hand region is imported; the original remains ROM data.
cache={}
def sample(x,y):
 rx=x*4-144;ry=y*4-3
 if not(0<=rx<ref.width and 0<=ry<ref.height):return 0
 c=ref.getpixel((rx,ry))
 if min(c)>155:return 0
 if c not in cache:cache[c]=min(range(1,16),key=lambda i:sum((c[k]-pal[i][k])**2 for k in range(3)))
 return cache[c]
seam=[y for y in range(384) if sample(256,y)];st,sb=min(seam),max(seam)
frames=[];reviews=[]
for group in audit['cut_groups']:
 top,bottom=group['cut_bounds'][1],group['cut_bounds'][3]-1
 extension=bytearray(512*384)
 for y in range(384):
  srcy=round(st+(y-top)*(sb-st)/max(1,bottom-top))
  for x in range(256,512):extension[y*512+x]=sample(x,srcy)
 (out/(group['id']+'.indices')).write_bytes(extension)
 for slot in group['frames']:
  f=audit['frames'][slot];before=Image.open(f['mounted']).convert('RGBA');body=Image.open(f['body']).convert('RGBA')
  after=Image.new('RGBA',before.size);after.putdata([(*pal[v],255) if v else (0,0,0,0) for v in extension]);body_after=after.copy();body_after.alpha_composite(body);after.alpha_composite(before)
  assert all(a==b for a,b in zip(before.getdata(),after.getdata()) if a[3])
  after.save(out/f'frame-{slot:02d}-mounted.png')
  frames.append(dict(frame=slot,cut=group['id'],source=f['source'],native_pixels_changed=0))
  if slot==group['representative']:
   padded=Image.new('RGBA',before.size);padded.alpha_composite(body);body=padded
   board=Image.new('RGB',(2048,2112),'#d8d8df');d=ImageDraw.Draw(board)
   for row,(left,right,label) in enumerate(((body,body_after,'CUERPO'),(before,after,'MONTADO'))):
    d.text((12,row*704+8),f'Malefica {group["id"]} - {label} - ANTES / DESPUES',fill='#171725')
    for col,im in enumerate((left,right)):
     scaled=im.resize((1024,768),Image.Resampling.NEAREST)
     # A common 320px-high view contains body and both assembled legs.
     board.paste(scaled.crop((0,0,1024,640)),(col*1024,row*704+40),scaled.crop((0,0,1024,640)))
   provenance=Image.new('RGBA',before.size);provenance.putdata([(55,135,245,255) if a[3] else (245,145,35,255) if b[3] else (0,0,0,0) for a,b in zip(before.getdata(),after.getdata())]);provenance=provenance.resize((1024,768),Image.Resampling.NEAREST).crop((0,0,1024,640));d.text((12,1416),'PROCEDENCIA: AZUL = original intacto / NARANJA = solo pixeles nuevos. 0 originales modificados.',fill='#171725');board.paste(provenance,(512,1450),provenance)
   path=out/(group['id']+'-review.png');board.save(path)
   reviews.append(dict(cut=group['id'],representative=slot,frames=group['frames'],review=str(path)))
result=dict(frames=frames,reviews=reviews,native_pixels_changed=0,palette_colors=16,reference_source='drawn lomo correction',reference_native_grid=dict(scale=4,x_offset=-144,y_offset=-3),limitations=['Native-index art proposal only; not active in game or approved.'])
(out/'manifest.json').write_text(json.dumps(result,indent=2));print('Native proposals:',len(reviews),'covered slots:',len(frames),'original changes:0')
