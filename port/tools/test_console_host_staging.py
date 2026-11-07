"""Exercise real Switch/Vita host staging with a mock VM and fixture assets.
No compiler, SDK execution, ROM, import or VM. Verify flat quoted-include closure.
"""
from contextlib import ExitStack
from pathlib import Path
import importlib.util,re,shutil,tempfile,unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
switch=load('switch_staging',ROOT/'port/switch/build_switch.py')
vita=load('vita_staging',ROOT/'port/vita/build_vita.py')
class FixtureImage:
 def convert(self,*args):return self
 def resize(self,*args):return self
 def save(self,path,**kwargs):Path(path).parent.mkdir(parents=True,exist_ok=True);Path(path).write_bytes(b'fixture-image')
class ConsoleStaging(unittest.TestCase):
 def check_staging(self,module,is_vita):
  from PIL import Image
  import core_assets
  copy=shutil.copy2
  with tempfile.TemporaryDirectory(prefix='console-staging-') as td,ExitStack() as stack:
   build=Path(td)/'build';build.mkdir();regions=['eu','us','jp']
   for region in regions:(build/('guest-'+region+('.a' if is_vita else '.macho'))).write_bytes(b'guest-fixture')
   def copy_asset(source,target,*args,**kwargs):
    target=Path(target);target.parent.mkdir(parents=True,exist_ok=True)
    if Path(source).suffix in ('.h','.c') or Path(source).parent==build:return copy(source,target,*args,**kwargs)
    target.write_bytes(b'asset-fixture');return str(target)
   def vm(command,**kwargs):
    args=list(map(str,command));self.assertIn('--fresh-share',args);share=Path(args[args.index('--share')+1]);src=share/'src';pending=[src/'switch_host.c'];visited=set()
    while pending:
     path=pending.pop()
     if path in visited:continue
     visited.add(path);self.assertTrue(path.is_file(),'missing staged dependency: '+path.name)
     for name in re.findall(r'^\s*#\s*include\s*"([^"/]+)"',path.read_text(encoding='utf8'),re.M):pending.append(src/name)
    for name in ('bridge_threads.h','thread_host.h','audio_bridge.h'):self.assertEqual((src/name).read_bytes(),(ROOT/'port/ps5/shared'/name).read_bytes())
    out=Path(args[args.index('--out')+1]);out.mkdir(parents=True,exist_ok=True);(out/('ReKHCoM.vpk' if is_vita else 'ReKHCoM.nro')).write_bytes(b'VM-result-fixture')
   stack.enter_context(patch.object(module,'BUILD',build));stack.enter_context(patch.object(shutil,'copy2',side_effect=copy_asset));stack.enter_context(patch.object(shutil,'copytree',side_effect=lambda source,target,**kwargs:Path(target).mkdir(parents=True,exist_ok=True)))
   stack.enter_context(patch.object(module.build_ps5,'render_messages'));stack.enter_context(patch.object(module.build_ps5,'render_toasts'));stack.enter_context(patch.object(core_assets,'gallery_posters'));stack.enter_context(patch.object(Image,'open',return_value=FixtureImage()))
   if is_vita:stack.enter_context(patch.object(module,'livearea',side_effect=lambda p:p.mkdir(parents=True,exist_ok=True)))
   stack.enter_context(patch.object(module.build_switch if is_vita else module,'run',side_effect=vm))
   (module.package if is_vita else module.build_host)(regions)
 def test_switch(self):self.check_staging(switch,False)
 def test_vita(self):self.check_staging(vita,True)
if __name__=='__main__':unittest.main()
