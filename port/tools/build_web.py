"""Compile the native engine for wasm32; never package a cartridge or save.

Requires generated PC assets and a static Emscripten SDL3 build. Absolute GBA
symbols cannot be assembled in Wasm: rewrite their LLVM IR uses to constants.
"""
import argparse
import concurrent.futures
import hashlib
import json
import re
import subprocess
import shutil
from pathlib import Path
from cartridge_division_ir import lower_cartridge_division, fingerprint as division_fingerprint

ROOT = Path(__file__).resolve().parents[2]


def frontend_ir_flags(common, is_game):
    # Keep optimized frontend attributes, but establish defined cartridge
    # arithmetic before LLVM can exploit C's division UB. O2 still runs below.
    return [*common, '-Xclang', '-disable-llvm-passes'] if is_game else common


def rewrite_ir(source, symbols, *, cartridge_arithmetic=False):
    # Only imported globals are fixed-address cartridge aliases. A definition
    # with the same name indicates a generator error; do not silently remove it.
    imported = set()
    lines = []
    for line in source.splitlines(keepends=True):
        match = re.match(r'@([\w.$]+)\s*=\s*(.*)', line)
        if match and match[1] in symbols:
            if not match[2].startswith('external '):
                raise ValueError(f'Fixed symbol has a native definition: {match[1]}')
            imported.add(match[1])
        else:
            lines.append(line)
    result = re.sub(r'@([\w.$]+)', lambda m: f'inttoptr (i32 {symbols[m[1]]} to ptr)'
                    if m[1] in imported else m[0], ''.join(lines))
    return lower_cartridge_division(result) if cartridge_arithmetic else result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--emsdk', type=Path, required=True)
    parser.add_argument('--sdl-build', type=Path, required=True)
    parser.add_argument('--sdl-source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--region', choices=['eu', 'us', 'jp'], default='eu')
    parser.add_argument('--jobs', type=int, default=3)
    args = parser.parse_args()
    emcc = args.emsdk.resolve() / 'upstream/emscripten/emcc.exe'
    work = args.work.resolve() / args.region
    work.mkdir(parents=True, exist_ok=True)
    gen = ROOT / 'build' / args.region / 'gen'
    glue = ROOT / 'build' / args.region / 'pc'
    symbols = {}
    for p in [glue / 'rom_symbols.s', ROOT / 'port/src/port_symbols.s']:
        for name, value in re.findall(r'\.set\s+_(\w+),\s*(0x[0-9a-fA-F]+|\d+)', p.read_text()):
            symbols[name] = int(value, 0)
    if len(symbols) < 1000:
        raise RuntimeError('Generated fixed-address symbol table is missing')
    chars = set((ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8'))
    sjis = []
    for ch in sorted(chars):
        try:
            code = int.from_bytes(ch.encode('shift_jis'), 'big')
            sjis.append(f'{{{ord(ch)},{code}}}')
        except UnicodeEncodeError:
            pass
    (work / 'web_sjis.h').write_text('static const unsigned webSjis[][2]={' + ','.join(sjis) + '};\n')
    reloc = (glue / 'rom_relocs.c').read_text()
    reloc = re.sub(r'extern char (SrollBlit\w+)\[\];', r'extern void \1(void);', reloc)
    # ISO C permits casting a function pointer to an object pointer in this
    # implementation: LLVM emits the Wasm function-table relocation here.
    reloc = re.sub(r'(\{0x[\da-fA-F]+,\s*)(SrollBlit\w+)(\s*\})', r'\1(const void*)\2\3', reloc)
    reloc_path = work / 'rom_relocs_web.c'
    reloc_path.write_text(reloc)
    includes = [ROOT / 'include'] + [p for p in (ROOT / 'include').iterdir() if p.is_dir() and p.name != 'gba']
    includes += [gen, ROOT / 'port/include', work]
    common = ['-O2', '-funsigned-char', '-fno-strict-aliasing', '-fwrapv', '-fno-common',
              '-DPLATFORM_PC', '-DVERSION_' + args.region.upper(), '-w']
    for p in includes:
        common += ['-iquote', str(p)]
    common += ['-I', str(args.sdl_source.resolve() / 'include')]
    common += ['-DPORT_OFFLINE_ACHIEVEMENTS', '-I', str(ROOT / 'port/third_party/rcheevos/include')]
    game = list((ROOT / 'src').rglob('*.c'))
    excluded = {'port_memory', 'port_sections', 'port_state', 'port_hdtext', 'port_watchdog', 'port_biosfault'}
    port = [p for p in (ROOT / 'port/src').glob('*.c') if p.stem not in excluded]
    port += list((ROOT / 'port/web/native').glob('*.c'))
    port += [ROOT / 'port/android/native/touch_android.c', reloc_path]
    # rcheevos' runtime (no network): the official set evaluated offline
    rcheevos = ROOT / 'port/third_party/rcheevos/src'
    port += sorted((rcheevos / 'rcheevos').glob('*.c')) + [rcheevos / 'rc_compat.c', rcheevos / 'rc_util.c',
                                                             rcheevos / 'rhash' / 'md5.c']  # runtime.c checks memrefs with MD5
    jobs = [(p, True) for p in game] + [(p, False) for p in port]
    headers = sorted([*ROOT.joinpath('include').rglob('*.h'),
                      *ROOT.joinpath('port/include').rglob('*.h'), *gen.rglob('*.h'),
                      *ROOT.joinpath('port/include').rglob('*.inc'),
                      *ROOT.joinpath('port/src').glob('*.inc'),
                      *ROOT.joinpath('port/achievements').glob('*.h'),
                      ROOT / 'port/achievements/offline_achievements.c',
                      ROOT / 'port/achievements/official_set_eu.inc',
                      *ROOT.joinpath('port/android/native').glob('*.h'), work / 'web_sjis.h'])
    header_hash = hashlib.sha256()
    for path in headers:
        header_hash.update(path.read_bytes())
    compiler_version = subprocess.check_output([str(emcc), '--version'], text=True)
    configuration = (repr(common) + header_hash.hexdigest() + compiler_version +
                     json.dumps(symbols, sort_keys=True) + Path(__file__).read_text(encoding='utf-8') + division_fingerprint())

    def compile_unit(item):
        path, is_game = item
        key = path.stem + '-' + hashlib.sha256(str(path).encode()).hexdigest()[:10]
        ir, obj = work / (key + '.ll'), work / (key + '.o')
        stamp = work / (key + '.sha256')
        fingerprint = hashlib.sha256(path.read_bytes() + configuration.encode()).hexdigest()
        if obj.is_file() and stamp.is_file() and stamp.read_text() == fingerprint:
            return obj
        flags = common + ['-std=gnu89' if is_game else '-std=gnu11']
        # Game section pragmas are PE/ELF-specific; browser reset reloads instead.
        command = [str(emcc), str(path), *frontend_ir_flags(flags, is_game), '-S', '-emit-llvm', '-o', str(ir)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(str(path) + '\n' + result.stderr)
        ir.write_text(rewrite_ir(ir.read_text(), symbols, cartridge_arithmetic=is_game))
        result = subprocess.run([str(emcc), '-O2', '-c', str(ir), '-o', str(obj)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(str(path) + '\n' + result.stderr)
        stamp.write_text(fingerprint)
        return obj

    objects = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for n, obj in enumerate(pool.map(compile_unit, jobs), 1):
            objects.append(obj)
            if n % 25 == 0:
                print(f'Compiled {n}/{len(jobs)}', flush=True)
    out = ROOT / 'build/web'
    out.mkdir(parents=True, exist_ok=True)
    output = out / f'rekhcom-{args.region}.js'
    options = ['-O2', '-Wl,--fatal-warnings', '-sASYNCIFY=1', '-sASYNCIFY_STACK_SIZE=131072',
               '-sEMULATE_FUNCTION_POINTER_CASTS=1', '-sGLOBAL_BASE=234946560',
               '-sINITIAL_MEMORY=268435456', '-sALLOW_MEMORY_GROWTH=1', '-sMAXIMUM_MEMORY=536870912',
               '-sSTACK_SIZE=1048576', '-sMODULARIZE=1', '-sEXPORT_NAME=createReKHCoM',
               '-sENVIRONMENT=web', '-sFORCE_FILESYSTEM=1', '-lidbfs.js',
               '-sEXPORTED_FUNCTIONS=["_main","_PortWebRomReady","_PortWebSetVisible","_PortWebClearInput","_PortWebTouchEnable","_PortWebTouchPersist","_PortWebResize","_PortWebFlushSave","_PortWebActiveBank","_PortWebSetHdReady","_PortWebOption"]',
               '-sEXPORTED_RUNTIME_METHODS=["FS","IDBFS","HEAPU8","callMain","ccall"]',
               # achievement badges for the in-game popup and list (PortLoadAsset)
               '--embed-file', str(ROOT / 'port/assets/achievements/badges.png') + '@/achievements/badges.png',
               '--embed-file', str(ROOT / 'port/assets/gallery') + '@/gallery',
               '-o', str(output)]
    from core_assets import gallery_posters
    gallery_posters()
    # A response file avoids Windows' command-line length limit.
    response = work / 'link.rsp'
    response.write_text('\n'.join(json.dumps(str(p)) for p in objects) + '\n' +
                        json.dumps(str(args.sdl_build.resolve() / 'libSDL3.a')) + '\n' +
                        '\n'.join(json.dumps(p) for p in options))
    subprocess.run([str(emcc), '@' + str(response)], check=True)
    for name in ['index.html', 'web.css', 'web.js', 'runtime.js', 'i18n.js', 'settings.js', 'save-editor.js', 'editor-ui.js', 'achievement-data.js', 'achievements.js', 'cards.js', 'controller.png']:
        shutil.copy2(ROOT / 'port/web' / name, out / name)
    shutil.copytree(ROOT / 'port/assets/fonts', out / 'fonts', dirs_exist_ok=True)
    for name in ['logo.png', 'boxart.jpg']:  # the launcher's own art, already shipped with it
        shutil.copy2(ROOT / 'port/decompui/assets' / name, out / name)
    print(f'Built {output}', flush=True)


if __name__ == '__main__':
    main()
