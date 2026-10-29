"""Production PPU additive-light regression; synthetic VRAM, no ROM/SDL/GPU."""
from pathlib import Path
import argparse,subprocess,tempfile
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
inc=[R/'include']+[x for x in (R/'include').iterdir() if x.is_dir() and x.name!='gba']
flags=[str(a.cc.resolve()),'-O1','-std=gnu11','-funsigned-char','-fno-strict-aliasing','-fwrapv','-mno-ms-bitfields',
 '-DPLATFORM_PC','-DVERSION_EU','-I'+str(R/'port/include')]
for x in inc:flags+=['-iquote',str(x)]
with tempfile.TemporaryDirectory(prefix='world-card-blend-') as d:
 exe=Path(d)/'test.exe'
 subprocess.run(flags+[str(R/'port/tools/world_card_blend_conformance.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True,timeout=30)
