"""The actual native Sora/Riku root frames must all have regional HD rows."""
import argparse
from pathlib import Path
import re
import hd_menu_titles as h

ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--rom',type=Path,required=True)
ap.add_argument('--symbols',type=Path,required=True)
a=ap.parse_args()
data=a.rom.read_bytes()
symbols={k:int(v,16) for k,v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)',a.symbols.read_text(encoding='utf8'))}
rows=[line.split('\t') for line in h.TSV.read_text(encoding='utf8').splitlines() if line and not line.startswith('#')]
frames=[('gJiminyTitleFrame0','gJiminyTitleTiles','eu,us'),
        ('gJiminyTitleFrame3','gJiminyTitleTiles','eu,us')]
frames += [('gJiminyTitleLocalizedFrame'+str(n),'gJiminyTitleLocalizedTiles','eu') for n in (0,3,4,5,6,7,8,9)]
for frame,tiles,regions in frames:
    matching=[r for r in rows if r[2:4]==[frame,tiles]]
    assert len(matching)==1,(frame,'missing or duplicate HD binding',len(matching))
    r=matching[0]
    assert r[1]=='5' and r[10]==regions,(frame,r[:4],r[10])
    assert tuple(map(int,r[4:8]))==h.frame_pixels(data,symbols,frame,tiles)[1]
print('PASS all ten Sora/Riku root/list header frames, exact native bounds and regional guards')
