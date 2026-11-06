"""Compile actual border code with SDL mocks and extracted production OutputRect.
Explicit compiler/output required; run only during an authorized CPU window.
"""
from pathlib import Path
import argparse,ctypes,hashlib,json,os,re,subprocess
ROOT=Path(__file__).resolve().parents[2]
def function(s,name):
    m=re.search(r'(?:static )?(?:void|int|SDL_Rect) '+name+r'\([^;{}]*\)\s*\{',s);assert m,name
    a=m.start();b=m.end();depth=1
    while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
    return s[a:b]
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--cc',required=True);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    s=(ROOT/'port/src/port_main.c').read_text(encoding='utf8')
    baseline=subprocess.check_output(['git','show','8f04d2b9:port/src/port_main.c'],cwd=ROOT).decode('utf8')
    config=(ROOT/'port/src/port_config.c').read_text(encoding='utf8')
    code=function(config,'PortConfigView')+'\n'+function(s,'AspectRect')+'\n'+function(s,'ContentArea')+'\n'+function(s,'CrtScale')+'\n'+function(s,'OutputRect')+'\n'
    code+=function(baseline,'ClassicBorder').replace('ClassicBorder','OldClassicBorder')+'\n'
    code+=function(baseline,'OutputRect').replace('OutputRect','OldOutputRect').replace('ClassicBorder','OldClassicBorder')+'\n'
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'border_output_fixture.inc').write_text(code)
    exe=args.output/'gba_border_conformance.exe'
    command=[args.cc,'-std=gnu11','-O2','-I'+str(ROOT/'port/include'),'-I'+str(args.output),str(ROOT/'port/tools/gba_border_conformance.c'),'-o',str(exe)]
    if os.name=='nt':ctypes.windll.kernel32.SetErrorMode(0x0001|0x0002|0x8000)
    flags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0)|getattr(subprocess,'CREATE_NO_WINDOW',0)
    c=subprocess.run(command,cwd=ROOT,capture_output=True,creationflags=flags)
    (args.output/'compile.stdout').write_bytes(c.stdout);(args.output/'compile.stderr').write_bytes(c.stderr)
    assert c.returncode==0,c.stderr.decode(errors='replace')
    p=subprocess.run([str(exe)],capture_output=True,creationflags=flags)
    (args.output/'stdout.log').write_bytes(p.stdout);(args.output/'stderr.log').write_bytes(p.stderr)
    inputs={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('port/src/port_gba_border.inc','port/include/port/gba_border_layout.h','port/tools/gba_border_conformance.c')}
    result={'source_sha256':hashlib.sha256(s.encode()).hexdigest(),'input_sha256':inputs,'extracted_sha256':hashlib.sha256(code.encode()).hexdigest(),'compile_command':command,'compile_exit':c.returncode,'exit':p.returncode,'stdout':p.stdout.decode(errors='replace'),'scope':'actual production geometry/cache/output code with mock SDL, no GPU/game/ROM runtime'}
    (args.output/'RESULT.json').write_text(json.dumps(result,indent=2));print(json.dumps(result))
    assert p.returncode==0
if __name__=='__main__':main()
