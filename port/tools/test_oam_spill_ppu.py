"""Full renderer spill/state checks, optionally all independent native oracles."""
from pathlib import Path
from array import array
import argparse, subprocess, tempfile, json
from test_ppu_conformance import CASES,expected
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True)
p.add_argument('--native-oracle',action='store_true');p.add_argument('--report',type=Path);a=p.parse_args()
inc=[R/'include']+[x for x in (R/'include').iterdir() if x.is_dir() and x.name!='gba']
args=[str(a.cc.resolve()),'-O1','-std=gnu11','-funsigned-char','-fno-strict-aliasing','-fwrapv',
 '-mno-ms-bitfields','-DPLATFORM_PC','-DVERSION_EU','-I'+str(R/'port/include')]
for x in inc:args+=['-iquote',str(x)]
results=[]
with tempfile.TemporaryDirectory(prefix='oam-spill-ppu-') as d:
 exe=Path(d)/'test.exe';raw=Path(d)/'pixels.bin'
 subprocess.run(args+[str(R/'port/tools/oam_spill_conformance.c'),'-o',str(exe)],check=True)
 r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30);print(r.stdout+r.stderr,end='');r.check_returncode()
 if a.native_oracle:
  for name in CASES:
   r=subprocess.run([str(exe),name,str(raw)],capture_output=True,text=True,check=True,timeout=20)
   if name.removeprefix('fast_').startswith('semantic'):
    actual=json.loads(r.stdout);want=[0,0,0]
    if not any(s in name for s in ('cover_','window','stale','clear')):want[2 if 'screen' in name else 1 if 'hud' in name else 0]=1
    mismatch=int(actual['counts']!=want)
    if 'explicit' in name:mismatch+=int(actual['cap']!=5 or actual['base']!=6)
   else:
    actual=array('I');actual.frombytes(raw.read_bytes());want=expected(name.removeprefix('fast_'))
    assert len(actual)==len(want)==38400
    mismatch=sum(x!=y for x,y in zip(actual,want))
   results.append(dict(case=name,mismatches=mismatch))
   if mismatch:print('FAIL',name,mismatch,flush=True)
  assert all(x['mismatches']==0 for x in results),results
  print(f'PASS {len(results)} existing independent native-pixel/semantic cases')
if a.report:a.report.write_text(json.dumps(dict(spill='PASS',native_oracles=results),indent=2),encoding='utf8')
