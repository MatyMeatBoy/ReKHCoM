"""Pinned GBA arrays must not acquire SysV's assumed 16-byte global alignment.

Compile a declaration produced by the real generator to x32 LLVM IR. The old
declaration emits aligned SIMD loads from gMapSelectGridPalettes+32, although
the verified ROM places it at an address ending in 0x84 (only four aligned).
"""
import importlib.util
from pathlib import Path
import re
import subprocess
import tempfile

R=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('gen_pc_assets',R/'port/tools/gen_pc_assets.py')
gen=importlib.util.module_from_spec(spec); spec.loader.exec_module(gen)


def main():
    cc=next((R.parent/'toolchain').glob('llvm-mingw-*/bin/clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-rom-align-') as name:
        folder=Path(name)
        instance=object.__new__(gen.Gen)
        instance.gen=folder
        instance.rom_symbols={'gMapSelectGridPalettes':0x0954bf84,'oddAsset':0x08000003,
                              'halfAsset':0x08000002,'nativeAligned':0x08000010}
        header=folder/'palettes.h'
        header.write_text('extern unsigned char gMapSelectGridPalettes[192];\n'
                          'extern unsigned char oddAsset[32];\nextern unsigned short halfAsset[16];\n'
                          'extern unsigned char nativeAligned[32];\nextern unsigned char untouchedNative[32];\n')
        if hasattr(instance,'write_rom_alignment'):
            instance.write_rom_alignment()
        code='#include "palettes.h"\nvoid copy(unsigned char* restrict out) {'
        code+='for(int i=0;i<22;i++)out[i]=gMapSelectGridPalettes[i+32];}\n'
        src=folder/'copy.c'; src.write_text(code)
        ir=subprocess.check_output([str(cc),'--target=x86_64-linux-gnux32','-O2','-S','-emit-llvm',str(src),'-o','-'],text=True)
        alignment=re.search(r'@gMapSelectGridPalettes = external.*align (\d+)',ir)
        assert alignment and int(alignment.group(1))<=4, 'ROM palette acquired unsafe 16-byte alignment:\n'+ir
        text=header.read_text()
        assert 'oddAsset[32] __attribute__((aligned(1)))' in text
        assert 'halfAsset[16] __attribute__((aligned(2)))' in text
        assert 'nativeAligned[32] __attribute__((aligned(16)))' in text
        assert 'extern unsigned char untouchedNative[32];' in text
        instance.write_rom_alignment()
        assert header.read_text()==text, 'alignment rewrite must be idempotent'
        # Updating an existing build must upgrade raw headers without the ROM,
        # then preserve mtimes so incremental builds stay incremental.
        cached=folder/'build/eu/gen'; cached.mkdir(parents=True)
        glue=folder/'build/eu/pc'; glue.mkdir()
        cached_header=cached/'old.h'
        cached_header.write_text('extern unsigned char gMapSelectGridPalettes[192];\n')
        (glue/'rom_symbols.s').write_text('\t.set _gMapSelectGridPalettes, 0x0954bf84\n')
        (glue/'rom_symbols_reloc.s').write_text('cached relocations\n')
        old_root=gen.ROOT
        gen.ROOT=folder
        try:
            assert gen.refresh_generated_alignment('eu')
            assert 'aligned(4)' in cached_header.read_text()
            modified=cached_header.stat().st_mtime_ns
            assert gen.refresh_generated_alignment('eu')
            assert cached_header.stat().st_mtime_ns==modified
        finally:
            gen.ROOT=old_root
    print('PASS: ROM declarations preserve actual alignment; x32 cannot emit aligned loads on 4-byte palette')


if __name__=='__main__':
    main()
