"""ROM-free checks for cartridge i32 arithmetic lowering before LLVM optimization."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

HERE=Path(__file__).resolve().parent
def load(name):
    spec=importlib.util.spec_from_file_location(name,HERE/(name+'.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

class LoweringTests(unittest.TestCase):
    def test_scalar_i32_and_metadata(self):
        lower=load('cartridge_division_ir').lower_cartridge_division
        source='define i32 @f(i32 %a,i32 %b) {\n %v = sdiv exact i32 %a, %b, !dbg !4\n ret i32 %v\n}\n'
        result=lower(source)
        self.assertIn('%v = call i32 @__rekhcom_gba_sdiv32(i32 %a, i32 %b), !dbg !4',result)
        self.assertIn('alwaysinline',result)
    def test_other_contracts_untouched(self):
        lower=load('cartridge_division_ir').lower_cartridge_division
        source=' %a = fdiv float %n, %d\n %b = udiv i64 %x, %y\n %c = sdiv <2 x i32> %v, %w\n'
        self.assertEqual(lower(source),source)
    def test_unknown_scalar_form_fails_visibly(self):
        lower=load('cartridge_division_ir').lower_cartridge_division
        with self.assertRaisesRegex(ValueError,'i32 division'):
            lower(' %a = udiv i32 unsupported_operand, %d\n')
    def test_builder_scope(self):
        builder=load('build_web')
        source='define i32 @f(i32 %a,i32 %b) {\n %v = urem i32 %a, %b\n ret i32 %v\n}\n'
        self.assertEqual(builder.rewrite_ir(source,{}),source)
        self.assertIn('@__rekhcom_gba_urem32',builder.rewrite_ir(source,{},cartridge_arithmetic=True))
        self.assertIn('-disable-llvm-passes',builder.frontend_ir_flags(['-O2'],True))
        self.assertEqual(builder.frontend_ir_flags(['-O2'],False),['-O2'])

def execute_wasm(clang,node):
    lower=load('cartridge_division_ir').lower_cartridge_division
    with tempfile.TemporaryDirectory(prefix='cartridge-division-') as temp:
        folder=Path(temp);source=folder/'probe.c';ir=folder/'probe.ll';wasm=folder/'probe.wasm'
        source.write_text('''int probe_sdiv(int a,int b){return a/b;}
int probe_srem(int a,int b){return a%b;}
unsigned probe_udiv(unsigned a,unsigned b){return a/b;}
unsigned probe_urem(unsigned a,unsigned b){return a%b;}
int probe_branch(int a,int b){return (a/b)<0;}
''')
        command=[str(clang),'--target=wasm32','-O2','-fwrapv','-funsigned-char','-fno-strict-aliasing',
                 '-Xclang','-disable-llvm-passes','-S','-emit-llvm',str(source),'-o',str(ir)]
        subprocess.run(command,check=True)
        original=ir.read_text();assert 'optnone' not in original
        ir.write_text(lower(original))
        subprocess.run([str(clang),'--target=wasm32','-O2','-nostdlib',str(ir),'-Wl,--no-entry','-Wl,--export-all','-o',str(wasm)],check=True)
        driver=folder/'probe.cjs'
        driver.write_text('''const fs=require('fs');
const e=new WebAssembly.Instance(new WebAssembly.Module(fs.readFileSync(process.argv[2]))).exports;
let count=0;
for(const op of ['sdiv','srem','udiv','urem'])for(const a of [0,1,7,0x7fffffff,0x80000000,0xffffffff])
for(const b of [0,1,2,3,7,0x7fffffff,0x80000000,0xffffffff]){
 let x=BigInt(a),y=BigInt(b);if(op[0]==='s'){x=BigInt.asIntN(32,x);y=BigInt.asIntN(32,y)}
 let expected=y===0n?0n:op.endsWith('div')?x/y:x%y;
 let got=e['probe_'+op](a,b)>>>0;
 if(BigInt(got)!==BigInt.asUintN(32,expected))throw Error(JSON.stringify({op,a,b,got,expected:String(expected)}));
 count++;
}
if(e.probe_branch(0x80000000,-1)!==1||e.probe_branch(7,0)!==0)throw Error('post-division branch changed');
console.log(JSON.stringify({cases:count,branchControls:2,result:'PASS'}));
''')
        completed=subprocess.run([str(node),str(driver),str(wasm)],capture_output=True,text=True,check=True)
        print(completed.stdout.strip())

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--wasm-clang',type=Path);parser.add_argument('--node',type=Path)
    args,remaining=parser.parse_known_args()
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(LoweringTests)
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():raise SystemExit(1)
    if args.wasm_clang or args.node:
        parser.error('--wasm-clang and --node must be given together') if not(args.wasm_clang and args.node) else None
        execute_wasm(args.wasm_clang,args.node)
