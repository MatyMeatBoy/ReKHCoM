"""Actual shared list maps, canonical binding and unchanged detail recipe."""
import argparse,re
from pathlib import Path
import hd_menu_titles as h
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--rom',type=Path,required=True);p.add_argument('--symbols',type=Path,required=True)
a=p.parse_args();data=a.rom.read_bytes();syms={k:int(v,16) for k,v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)',a.symbols.read_text(encoding='utf8'))}
rows=h.jiminy_list_body_rows(data,syms);assert len(rows)==1
r=rows[0].split('\t');assert r[:8]==['gJiminyListBookBody','5','gJiminyCardListMap','gJiminyBgTiles','0','16','240','144'];assert r[-1]=='eu,us'
assert 'blit=384,396,447,422' in r[9] and r[9].endswith('scale=8')
committed=h.TSV.read_text(encoding='utf8').splitlines()
assert rows[0] in committed
assert h.jiminy_book_body_rows(data,syms)[0] in committed,'Existing detail geometry changed'
print('PASS actual three list bodies identical; one canonical recipe, native bounds/regions and remaster loops; existing detail recipe unchanged')
