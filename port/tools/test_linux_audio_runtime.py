"""Check the actual native64 runtime writer against existing SDK ELF files.

Requires the prepared Linux amd64 sysroot and LLVM; no download/build/VM.
An optional published archive checks the already complete i386 private client.
"""
import argparse
import ast
import importlib.util
import json
from pathlib import Path
import subprocess
import shutil
import sys
import tarfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('audio_linux64', ROOT / 'port/linux64/build_linux64.py')
BUILD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILD)


def needed(path):
    text = subprocess.check_output([str(BUILD.TC / 'llvm-readelf.exe'), '-d', str(path)], text=True)
    return [line.split('[')[1].split(']')[0] for line in text.splitlines() if '(NEEDED)' in line]


def produce(dest):
    dest.mkdir(parents=True, exist_ok=True)
    if hasattr(BUILD, 'copy_runtime_libraries'):
        BUILD.copy_runtime_libraries(dest)
        return
    # Authoritative RED: execute the old writer's actual loop, not a copy.
    tree = ast.parse((ROOT / 'port/linux64/build_linux64.py').read_text(encoding='utf-8'))
    main = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'main')
    loop = next(n for n in main.body if isinstance(n, ast.For) and isinstance(n.iter, ast.Name) and n.iter.id == 'LIBS')
    (dest / 'lib64').mkdir()
    exec(compile(ast.Module(body=[loop], type_ignores=[]), '<actual-old-runtime-writer>', 'exec'),
         dict(BUILD.__dict__, OUT=dest))


def run(out, archive):
    pulse = BUILD.find('libpulse.so.0')
    if pulse is None:
        raise RuntimeError('Existing Linux amd64 sysroot with PulseAudio is required')
    dependencies = [name for name in needed(pulse) if name.startswith('libpulsecommon-')]
    if not dependencies:
        raise RuntimeError('Expected version-private Pulse client dependency in the SDK ELF')
    stage = out / 'stage'
    produce(stage)
    private = []
    for name in dependencies:
        target = stage / 'lib64' / name
        if not target.is_file():
            raise RuntimeError(f'Actual runtime writer omitted required ELF dependency: {name}')
        if target.read_bytes()[4] != 2:
            raise RuntimeError('A 32-bit private Pulse library cannot satisfy the 64-bit client')
        private.append(dict(name=name, bytes=target.stat().st_size))
    forbidden = ['libc.so.6', 'libm.so.6', 'libX11.so.6', 'libxcb.so.1', 'libsystemd.so.0', 'libdbus-1.so.3']
    if any((stage / 'lib64' / name).exists() for name in forbidden):
        raise RuntimeError('Do not inject versioned system/GPU dependencies into the native64 library path')
    incomplete = out / 'incomplete-sdk'
    incomplete.mkdir()
    shutil.copyfile(pulse, incomplete / 'libpulse.so.0')
    with patch.object(BUILD, 'runtime_search', return_value=[incomplete]), patch.object(BUILD, 'LIBS', ['libpulse.so.0']):
        try:
            produce(out / 'missing-private')
        except SystemExit as error:
            if 'private runtime library' not in str(error):
                raise
        else:
            raise RuntimeError('Missing private client dependency did not stop packaging')
    mismatched = incomplete / dependencies[0]
    data = bytearray((stage / 'lib64' / dependencies[0]).read_bytes())
    data[4] = 1  # An i386 private library cannot satisfy an ELF64 client.
    mismatched.write_bytes(data)
    with patch.object(BUILD, 'runtime_search', return_value=[incomplete]), patch.object(BUILD, 'LIBS', ['libpulse.so.0']):
        try:
            produce(out / 'wrong-architecture')
        except SystemExit as error:
            if 'wrong ELF architecture' not in str(error):
                raise
        else:
            raise RuntimeError('Incompatible private library architecture accepted')
    # Tiny real ELF DSOs exercise the shared writer on all supported ABI
    # families without requiring an ARM64 SDK/sysroot or running native code.
    helper = sys.modules['linux_runtime_clients']
    abi_results = []
    for triple in ('i686-linux-gnu', 'x86_64-linux-gnu', 'aarch64-linux-gnu'):
        folder = out / triple
        folder.mkdir()
        (folder / 'private.c').write_text('int pulse_fixture(void){return 7;}')
        (folder / 'client.c').write_text('extern int pulse_fixture(void);int client(void){return pulse_fixture();}')
        common = [str(BUILD.TC / 'clang.exe'), '--target=' + triple, '-fuse-ld=lld', '-fPIC', '-shared', '-nostdlib']
        subprocess.run(common + [str(folder / 'private.c'), '-Wl,-soname,libpulsecommon-fixture.so', '-o', str(folder / 'libpulsecommon-fixture.so')], check=True)
        subprocess.run(common + [str(folder / 'client.c'), '-L' + str(folder), '-l:libpulsecommon-fixture.so', '-Wl,-soname,libpulse.so.0', '-o', str(folder / 'libpulse.so.0')], check=True)
        helper.copy_client_libraries(['libpulse.so.0'], [folder], folder / 'package', BUILD.TC / 'llvm-readelf.exe')
        copied = folder / 'package/libpulsecommon-fixture.so'
        if not copied.is_file() or helper.elf_identity(copied) != helper.elf_identity(folder / 'libpulse.so.0'):
            raise RuntimeError('Private closure failed for ' + triple)
        abi_results.append(triple)
    i386 = 'NOT_REQUESTED'
    if archive:
        with tarfile.open(archive) as package:
            data = package.extractfile('lib/libpulsecommon-16.1.so').read()
            if data[:5] != b'\x7fELF\x01':
                raise RuntimeError('Published i386 private Pulse library is not ELF32')
            i386 = 'PRESENT_ELF32'
    result = dict(result='PASS', private=private, host_system_libraries='not overridden',
                  missing_private='REJECTED', wrong_architecture='REJECTED', elf_abis=abi_results, published_i386=i386,
                  limits='ELF packaging contract only; no audio server/device or Bazzite runtime test')
    (out / 'RESULTS.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--archive', type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    run(args.out, args.archive)
