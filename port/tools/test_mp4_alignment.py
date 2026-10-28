"""Container bounds and observed legacy alignment, no codecs or game content.

--python-only avoids a compiler while the host is memory constrained.
--owned optionally checks an existing movie by seeking headers, never copying it.
"""
from pathlib import Path
import argparse
import importlib.util
import os
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def box(kind, body=b''):
    return struct.pack('>I4s', 8 + len(body), kind) + body


def fixtures():
    moov = box(b'moov')
    for n in range(32):
        movie = box(b'ftyp', b'isom') + moov + box(b'mdat', bytes(n))
        yield f'plain-{n}', movie, True
        yield f'aligned-cd-{n}', movie + b'\xcd' * (-len(movie) & 15), True
    movie = box(b'ftyp', b'isom') + moov + box(b'mdat')
    yield 'wrong-tail', movie + b'\xff' * 4, False
    yield 'mixed-tail', movie + b'\xcd\xcd\x00\xcd', False
    yield 'unaligned-tail', movie + b'\xcd' * 3, False
    yield 'oversized-tail', movie + b'\xcd' * 20, False
    yield 'missing-moov', box(b'mdat') + b'\xcd' * 8, False
    yield 'missing-mdat', moov + b'\xcd' * 8, False
    yield 'truncated-box', moov + struct.pack('>I4s', 40, b'mdat') + b'\xcd' * 16, False
    yield 'small-extended', moov + struct.pack('>I4sQ', 1, b'mdat', 15), False
    yield 'oversized-extended', moov + struct.pack('>I4sQ', 1, b'mdat', (1 << 64) - 1), False
    # The old C++ validator wraps at 8 + (2**64 - 8), then rereads moov forever.
    yield 'wrap-cycle', moov + struct.pack('>I4sQ', 1, b'mdat', (1 << 64) - 8), False
    yield 'eof-mdat', moov + struct.pack('>I4s', 0, b'mdat') + b'x', True
    for count in (4095, 4096, 4097):
        yield f'box-limit-{count}', moov + box(b'mdat') + box(b'free') * (count - 2), count <= 4096
    final = moov + box(b'mdat') + box(b'free') * 4093 + box(b'free', bytes(4))
    yield 'box-limit-4096-padded', final + b'\xcd' * (-len(final) & 15), True


def native_function():
    text = (ROOT / 'port/decompui/hd_import.inc').read_text(encoding='utf-8')
    start = text.index('static bool Mp4Ok(')
    while ';' in text[start:text.index('{', start)]:
        start = text.index('static bool Mp4Ok(', start + 1)
    at = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[at] == '{') - (text[at] == '}')
        at += 1
    return text[start:at]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--python-only', action='store_true')
    p.add_argument('--owned', type=Path)
    args = p.parse_args()
    spec = importlib.util.spec_from_file_location('mp4_alignment_launcher', ROOT / 'port/linux/launcher/rekhcom_launcher.py')
    launcher = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(launcher)
    with tempfile.TemporaryDirectory(prefix='rekhcom-mp4-bounds-') as tmp:
        out = Path(tmp)
        cases = []
        for name, data, expected in fixtures():
            path = out / (name + '.mp4')
            path.write_bytes(data)
            cases.append((path, expected))
        if args.owned:
            cases.append((args.owned.resolve(), True))
        for path, expected in cases:
            assert launcher.mp4(path) == expected, (path.name, expected)
        print(f'PASS Python container validator: {len(cases)} controls', flush=True)
        if args.python_only:
            return
        compiler = (next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang++.exe'))
                    if os.name == 'nt' else shutil.which('clang++') or shutil.which('c++'))
        assert compiler, 'C++ compiler required; use --python-only for a bounded Python pass'
        process_options = {'creationflags': 0x08004000} if os.name == 'nt' else {}
        source, exe = out / 'native.cpp', out / ('native.exe' if os.name == 'nt' else 'native')
        source.write_text('#include <filesystem>\n#include <fstream>\n#include <cstring>\n#include <iostream>\n#include <cstdint>\nnamespace fs=std::filesystem;\n' + native_function() + '\nint main(int n,char**v){for(int i=1;i<n;i++)std::cout<<Mp4Ok(fs::u8path(v[i]))<<std::endl;}\n', encoding='utf-8')
        compiled = subprocess.run([str(compiler), '-std=c++17', '-O1', *(['-static'] if os.name == 'nt' else []), str(source), '-o', str(exe)], stdin=subprocess.DEVNULL, capture_output=True, text=True, **process_options)
        if compiled.returncode:
            print(compiled.stdout + compiled.stderr, flush=True)
        compiled.check_returncode()
        result = subprocess.run([str(exe), *[str(path) for path, _ in cases]], stdin=subprocess.DEVNULL, check=True, capture_output=True, text=True, timeout=5, **process_options)
        assert result.stdout.splitlines() == [str(int(expected)) for _, expected in cases], result.stdout
        print(f'PASS C++ production container validator: {len(cases)} controls')


if __name__ == '__main__':
    main()
