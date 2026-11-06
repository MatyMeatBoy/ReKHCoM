"""Compile/link actual event owner caller fragments and the production signature.
Explicit SDK paths; no ROM, imports or game execution.
"""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def run(args,log):
 p=subprocess.run([str(x) for x in args],capture_output=True,creationflags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0)|getattr(subprocess,'CREATE_NO_WINDOW',0));log.write_bytes(p.stdout+p.stderr);return p

def main():
 a=argparse.ArgumentParser();a.add_argument('--wasm-cc',required=True);a.add_argument('--wasm-ld',required=True);a.add_argument('--output',type=Path,required=True);a.add_argument('--expect-red',action='store_true');a.add_argument('--baseline');a.add_argument('--native-cc');a.add_argument('--x32-cc');o=a.parse_args();o.output.mkdir(parents=True,exist_ok=True)
 current=(ROOT/'src/evt/evt_obj.c').read_text(encoding='utf8');ppu=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf8');signature=re.search(r'void PortSpriteEventShadowOwner\(int role\) \{[^}]+\}',ppu)[0]
 def wasm(caller,label):
  d=o.output/label;d.mkdir(exist_ok=True);includes=re.search(r'#ifdef PLATFORM_PC\n(.*?)#endif',caller,re.S)[1];calls=re.findall(r'    PortSpriteEventShadowOwner\([\s\S]*?\);',caller);assert len(calls)==2
  (d/'caller.c').write_text(includes+'\nstruct Event{int eventId;void*charaObjs[5];};struct Event state,*gEventState=&state;void*obj;\nvoid call_owner(void){\n'+'\n'.join(calls)+'\n}',encoding='utf8')
  (d/'producer.c').write_text('#include "port/port_oam.h"\nstatic uint8_t sEventShadowActor;\n'+signature+'\nint read_owner(void){return sEventShadowActor;}\n',encoding='utf8')
  for name in ('caller','producer'):
   p=run([o.wasm_cc,'--target=wasm32-unknown-unknown','-ffreestanding','-std=gnu89','-O2','-Wno-implicit-function-declaration','-DPLATFORM_PC','-I'+str(ROOT/'port/include'),'-I'+str(ROOT/'include/evt'),'-c',d/(name+'.c'),'-o',d/(name+'.o')],d/(name+'.log'));assert not p.returncode,p.stderr.decode(errors='replace')
  return run([o.wasm_ld,'--fatal-warnings','--no-entry','--export=call_owner','--export=read_owner',d/'caller.o',d/'producer.o','-o',d/'contract.wasm'],d/'link.log')
 p=wasm(current,'current');v={'current_link_exit':p.returncode,'current_link_log':p.stderr.decode(errors='replace')}
 if o.expect_red:
  assert p.returncode and b'signature mismatch' in p.stderr;print(json.dumps(v));return
 assert not p.returncode,v
 if o.baseline:
  before=subprocess.check_output(['git','show',o.baseline+':src/evt/evt_obj.c'],cwd=ROOT).decode('utf8');red=wasm(before,'baseline');assert red.returncode and b'signature mismatch' in red.stderr;v['baseline_link_exit']=red.returncode;v['baseline_link_log']=red.stderr.decode(errors='replace')
  inc=['-iquote'+str(ROOT/'include')]+['-iquote'+str(x) for x in sorted((ROOT/'include').iterdir()) if x.is_dir() and x.name!='gba']+['-iquote'+str(ROOT/'build/eu/gen'),'-I'+str(ROOT/'port/include')];v['objects']={}
  for arch,cc,target in [('native32',o.native_cc,[]),('x32',o.x32_cc,['--target=x86_64-unknown-linux-gnux32','-ffreestanding','-fno-pic','-fno-pie'])]:
   if not cc:continue
   objects=[]
   for label,text in [('before',before),('after',current)]:
    d=o.output/(arch+'-'+label);d.mkdir(exist_ok=True);source=d/'evt_obj.c';source.write_text(text,encoding='utf8');obj=d/'evt_obj.o';flags=[*target,'-O2','-std=gnu89','-w','-DPLATFORM_PC','-DVERSION_EU','-funsigned-char','-fno-strict-aliasing','-fwrapv','-fno-asynchronous-unwind-tables','-ffunction-sections','-fdata-sections','-include',ROOT/'port/include/port/game_prefix.h',*inc];p=run([cc,*flags,'-c',source,'-o',obj],d/'compile.log');assert not p.returncode,p.stderr.decode(errors='replace');objects.append(obj.read_bytes())
   v['objects'][arch]={'before':hashlib.sha256(objects[0]).hexdigest(),'after':hashlib.sha256(objects[1]).hexdigest(),'byte_identical':objects[0]==objects[1]} # Diagnostic only: prototypes may change unprototyped-call codegen.
  if o.native_cc:
   p=run([o.native_cc,'-O2','-std=gnu89','-Werror=implicit-function-declaration','-DPLATFORM_PC','-DVERSION_EU',*inc,'-fsyntax-only',ROOT/'src/evt/evt_obj.c'],o.output/'strict-caller.log');v['actual_caller_strict_exit']=p.returncode;assert not p.returncode,p.stderr.decode(errors='replace')
 (o.output/'RESULT.json').write_text(json.dumps(v,indent=2));print(json.dumps(v))
if __name__=='__main__':main()
