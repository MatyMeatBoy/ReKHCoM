"""Check actual Riku banner recipes against owned native frames and old rows."""
import argparse
from pathlib import Path
import re
import hd_menu_titles as h

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True)
p.add_argument('--symbols',type=Path,required=True)
a=p.parse_args()
data=a.rom.read_bytes()
syms={k:int(v,16) for k,v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)',a.symbols.read_text(encoding='utf8'))}
assert hasattr(h,'jiminy_riku_rows'),'Riku journal has no HD banner recipes'
rows=[r.split('\t') for r in h.jiminy_riku_rows(data,syms)]
assert len(rows)==5 and len({r[0] for r in rows})==5
expected=[('English','gJiminyTitleFrame3','gJiminyTitleTiles','eu,us'),
          ('French','gJiminyTitleLocalizedFrame3','gJiminyTitleLocalizedTiles','eu'),
          ('Spanish','gJiminyTitleLocalizedFrame5','gJiminyTitleLocalizedTiles','eu'),
          ('Italian','gJiminyTitleLocalizedFrame7','gJiminyTitleLocalizedTiles','eu'),
          ('German','gJiminyTitleLocalizedFrame9','gJiminyTitleLocalizedTiles','eu')]
for r,(lang,frame,tiles,regions) in zip(rows,expected):
    assert r[:4]==['gJiminyRiku'+lang+'Banner','5',frame,tiles],r[:4]
    assert tuple(map(int,r[4:8]))==h.frame_pixels(data,syms,frame,tiles)[1]
    assert r[10]==regions and r[9].endswith('scale=8')
    assert 'sblit=' in r[9] and r[9].startswith('canvas=')
old={r.split('\t')[0]:r for r in h.TSV.read_text(encoding='utf8').splitlines()}
for spec in h.BANNERS:
    r=h.row(spec,data,syms)
    fields=r.split('\t')
    existing=old[fields[0]].split('\t')
    assert existing[:9]==fields[:9] and existing[10:]==fields[10:],'Existing binding changed or was renamed'
print('PASS five Riku native frames/regions/boxes; existing banner names/bindings preserved')
