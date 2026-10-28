"""Actual setter/four caller expressions; direct Wasm tools avoid shared SDK cache."""
import argparse,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--emsdk',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();sdk=a.emsdk.resolve();a.out.mkdir(parents=True,exist_ok=True)
cc=sdk/'upstream/bin/clang.exe';ld=sdk/'upstream/bin/wasm-ld.exe';node=next((sdk/'node').glob('*/node.exe'))
caller=(ROOT/'src/bos/bos_pc.c').read_text(encoding='utf-8');engine=(ROOT/'include/engine.h').read_text(encoding='utf-8');ppu=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
calls=re.findall(r'PortSetBgWorld\([^;]+;',caller)
if len(calls)!=4:raise RuntimeError('Expected four audited boss call sites')
prototype=re.search(r'typedef int \(\*PortWorldTileFunc\)[^;]+;\s*void PortSetBgWorld[^;]+;',engine).group(0)
start=ppu.index('void PortSetBgWorld(');end=ppu.index('\n}',start)+2
prefix='typedef int (*PortWorldTileFunc)(int,int,int);\nstatic struct {PortWorldTileFunc fn;int x,y;} sBgWorld[4];\n'
callee=a.out/'callee.c';callee.write_text(prefix+ppu[start:end]+'''\nint Check(int active){
 if(!active)return sBgWorld[1].fn==0 && sBgWorld[1].x==0 && sBgWorld[1].y==0;
 return sBgWorld[1].fn && sBgWorld[1].fn(1,2,3)==123 && sBgWorld[1].x==40 && sBgWorld[1].y==8;
}\n''',encoding='utf-8')
subprocess.run([str(cc),'--target=wasm32-unknown-unknown','-O2','-c',str(callee),'-o',str(a.out/'callee.o')],check=True)
has_header=bool(re.search(r'#ifdef PLATFORM_PC\s+#include "engine.h"\s+#endif',caller))
rows=[]
for name,declared in [('baseline',False),('candidate',has_header)]:
 unit=('#define NULL ((void*)0)\n'+(prototype if declared else '')+'\nint Check(int);\nstatic int BosPcWorldTile(int b,int x,int y){return b*100+x*10+y;}\nint run(void){int sx=40,sy=0;\n')
 for i,call in enumerate(calls):unit+=call+'\nif(!Check('+('1' if 'BosPcWorldTile' in call else '0')+'))return '+str(i+10)+';\n'
 unit+='return 0;}\n';path=a.out/(name+'.c');path.write_text(unit,encoding='utf-8')
 subprocess.run([str(cc),'--target=wasm32-unknown-unknown','-O2','-std=gnu89','-w','-c',str(path),'-o',str(a.out/(name+'.o'))],check=True)
 for fatal in [False,True]:
  wasm=a.out/(name+('-fatal' if fatal else '')+'.wasm')
  link=subprocess.run([str(ld),'--no-entry','--export=run',*(['--fatal-warnings'] if fatal else []),str(a.out/(name+'.o')),str(a.out/'callee.o'),'-o',str(wasm)],capture_output=True,text=True)
  row=dict(case=name,fatal_warnings=fatal,link_exit=link.returncode,stderr=link.stderr)
  if link.returncode==0:
   runner=a.out/'run.cjs';runner.write_text("const fs=require('fs');WebAssembly.instantiate(fs.readFileSync(process.argv[2])).then(x=>process.exitCode=x.instance.exports.run());",encoding='utf-8')
   run=subprocess.run([str(node),str(runner),str(wasm)],capture_output=True,text=True);row.update(runtime_exit=run.returncode,trap='unreachable' in run.stderr)
   (a.out/(name+('-fatal' if fatal else '')+'-runtime.txt')).write_text(run.stdout+run.stderr,encoding='utf-8')
  rows.append(row)
(a.out/'RESULTS.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
print(json.dumps(rows,indent=2))
if not(rows[0]['link_exit']==0 and rows[0].get('trap') and rows[1]['link_exit']!=0 and rows[2].get('runtime_exit')==0 and rows[3].get('runtime_exit')==0):raise SystemExit(1)
if "'-Wl,--fatal-warnings'" not in (ROOT/'port/tools/build_web.py').read_text(encoding='utf-8'):raise RuntimeError('Production Web linker must reject warnings')
