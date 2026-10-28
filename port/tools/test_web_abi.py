"""Execute fixed-address and callback ABI probes in real WebAssembly, under Node."""
import argparse
import json
from pathlib import Path
import subprocess
from build_web import rewrite_ir, frontend_ir_flags, ROOT


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emsdk', type=Path, required=True)
    args = parser.parse_args()
    sdk = args.emsdk.resolve()
    emcc = sdk / 'upstream/emscripten/emcc.exe'
    nodes = list((sdk / 'node').glob('*/bin/node')) + list((sdk / 'node').glob('*/node.exe'))
    if not nodes:
        raise RuntimeError('Emsdk Node runtime not found')
    out = ROOT / 'build/web-abi'
    out.mkdir(parents=True, exist_ok=True)
    source = out / 'probe.c'
    source.write_text('''#include <emscripten.h>
#include <stdint.h>
extern char FixedCount[], Hardware[];
static volatile int nativeGlobal;
static volatile int numerator=7, denominator=0;
static int callback(int value) { nativeGlobal=value;return 91; }
static void (*volatile castCallback)(int)=(void(*)(int))callback;
int main(void) {
    if((uintptr_t)FixedCount!=26)return 1;
    if((uintptr_t)Hardware!=0x02000000)return 2;
    if((uintptr_t)&nativeGlobal<0x0E010000)return 3;
    *(volatile uint32_t*)Hardware=0x12345678;
    emscripten_sleep(1);
    if(*(volatile uint32_t*)Hardware!=0x12345678)return 4;
    castCallback(37);
    if(nativeGlobal!=37)return 5;
    if(numerator/denominator!=0 || numerator%denominator!=0)return 6;
    return 0;
}
''')
    ir = out / 'probe.ll'
    subprocess.run([str(emcc), str(source), *frontend_ir_flags(['-O2'], True), '-S', '-emit-llvm', '-o', str(ir)], check=True)
    ir.write_text(rewrite_ir(ir.read_text(), {'FixedCount':26, 'Hardware':0x02000000}, cartridge_arithmetic=True))
    js = out / 'probe.js'
    subprocess.run([str(emcc), str(ir), '-O2', '-sASYNCIFY=1', '-sEMULATE_FUNCTION_POINTER_CASTS=1',
                    '-sGLOBAL_BASE=234946560', '-sINITIAL_MEMORY=268435456', '-sENVIRONMENT=node',
                    '-o', str(js)], check=True)
    subprocess.run([str(nodes[0]), str(js)], check=True)
    report = {'passed':True, 'checks':['absolute count', 'fixed hardware address',
              'native data above hardware', 'memory survives Asyncify', 'callback return-type compatibility',
              'cartridge zero quotient/remainder through game IR pipeline']}
    (out / 'report.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report))


if __name__ == '__main__':
    main()
