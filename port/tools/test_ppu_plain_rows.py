#!/usr/bin/env python3
"""KHCOM_PPU_BG_PLAIN_ROWS text rows against the general sampler, on the production PPU (host, ROM-free)."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
toolchain_root = Path(os.environ.get('KHCOM_TOOLCHAIN', ROOT.parent / 'toolchain'))
toolchain = sorted(toolchain_root.glob('llvm-mingw-*'))[-1]
with tempfile.TemporaryDirectory() as directory:
    exe = Path(directory) / 'ppu_plain_rows.exe'
    includes = [ROOT/'include'] + [p for p in (ROOT/'include').iterdir() if p.is_dir() and p.name != 'gba']
    cmd = [str(toolchain/'bin/i686-w64-mingw32-clang.exe'), '-O2', '-std=gnu11', '-funsigned-char',
           '-fno-strict-aliasing', '-fwrapv', '-mno-ms-bitfields', '-DPLATFORM_PC', '-DVERSION_EU',
           '-I' + str(ROOT/'port/include')]
    for p in includes:
        cmd += ['-iquote', str(p)]
    subprocess.run(cmd + [str(ROOT/'port/tools/ppu_plain_rows.c'), '-o', str(exe)], check=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith('KHCOM_PPU_')}
    out = subprocess.run([str(exe)], env=env, capture_output=True, text=True, timeout=300)
    assert out.returncode == 0, out.stdout + out.stderr
    rows, plain, fallback = [int(x) for x in out.stdout.split()[-3:]]
    assert plain > rows // 2 and fallback > 0, (rows, plain, fallback)
    print(f'PASS plain text rows: {rows} rows identical, {plain} drawn by the plain path, {fallback} text fallbacks')
