"""Regenerate the authored Oogie continuation masks; contains no cartridge pixels."""
from pathlib import Path
from PIL import Image, ImageDraw
# Hand-drawn continuation contours. No reflected source pixels or scanline width fitting.
left_polys=[[(255,416),(249,415),(246,412),(245,417),(247,423),(251,428),(255,429)],[(255,436),(250,436),(246,433),(244,437),(242,437),(245,443),(249,446),(255,447)]]
left_holes=[[(249,436),(251,440),(253,436)],[(249,447),(250,443),(252,447)]]
right_polys=[[(496,420),(500,418),(504,420),(506,425),(506,431),(509,433),(510,439),(513,441),(513,446),(511,448),(512,453),(509,458),(503,462),(496,460)]]
right_holes=[] # Both native eyes and mouth already exist; complete only the cut body contour.
left_polys += [[(257,472),(266,472),(268,474),(276,474),(281,472),(288,472),(283,477),(277,482),(270,483),(263,480),(257,475)]]
left_holes += [[(266,472),(281,472),(277,476),(272,478),(269,477)]]
right_polys += [[(464,472),(470,472),(472,474),(472,476),(470,476),(467,474)],[(483,472),(485,472),(486,474),(484,476),(483,475)]]
mask=Image.new('1',(576,96));draw=ImageDraw.Draw(mask)
for poly in left_polys+right_polys:draw.polygon([(x,y-400)for x,y in poly],fill=1)
for poly in left_holes+right_holes:draw.polygon([(x,y-400)for x,y in poly],fill=0)
def art(x,y):
 if not 408<=y<488:return 0
 if y<472 and not(224<=x<256 or 496<=x<528):return 0
 if y>=472 and not(224<=x<296 or 456<=x<528):return 0
 yy=y-400
 if mask.getpixel((x,yy)):return 146
 for radius,index in ((1,157),(2,158)):
  for dx in range(-radius,radius+1):
   for dy in range(-radius,radius+1):
    if abs(dx)+abs(dy)>radius:continue
    if 0<=yy+dy<96 and mask.getpixel((x+dx,yy+dy)):return index
 return 0


def generate():
 lines=['/* Authored continuations of the two clipped floor figures. Native palette offsets:', ' * 0 = no overwrite, 2 = black146, 13/14 = green157/158. No cartridge pixels. */', '#ifndef PORT_OOGIE_MOTIFS_H', '#define PORT_OOGIE_MOTIFS_H']
 for side,x0 in [('Left',224),('Right',456)]:
  values=[art(x,y) for y in range(408,488) for x in range(x0,x0+72)]
  values=[v-144 if v else 0 for v in values]
  packed=[values[i]|values[i+1]<<4 for i in range(0,len(values),2)]
  lines+=['static const unsigned char kOogie'+side+'Motif[2880] = {']
  lines+=['    '+','.join('0x%02x'%v for v in packed[i:i+24])+',' for i in range(0,len(packed),24)]
  lines+=['};']
 lines+=['#endif','']
 return '\n'.join(lines)

if __name__=='__main__':
 (Path(__file__).resolve().parents[1]/'include/port/port_oogie_motifs.h').write_text(generate())
