#!/usr/bin/env python3
"""Spotlight margin-row skip against full rows, on the production PPU (host)."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
toolchain_root = Path(os.environ.get('KHCOM_TOOLCHAIN', ROOT.parent / 'toolchain'))
toolchain = sorted(toolchain_root.glob('llvm-mingw-*'))[-1]
FLAG = 'KHCOM_PPU_SPOTLIGHT_SKIP_MARGINS'
with tempfile.TemporaryDirectory() as directory:
    exe = Path(directory) / 'spotlight_margins.exe'
    includes = [ROOT/'include'] + [p for p in (ROOT/'include').iterdir() if p.is_dir() and p.name != 'gba']
    cmd = [str(toolchain/'bin/i686-w64-mingw32-clang.exe'), '-O2', '-std=gnu11', '-funsigned-char',
           '-fno-strict-aliasing', '-fwrapv', '-mno-ms-bitfields', '-DPLATFORM_PC', '-DVERSION_EU',
           '-I' + str(ROOT/'port/include')]
    for p in includes:
        cmd += ['-iquote', str(p)]
    subprocess.run(cmd + [str(ROOT/'port/tools/spotlight_margins.c'), '-o', str(exe)], check=True)
    results = {}
    for on in (False, True):
        env = {k: v for k, v in os.environ.items() if not k.startswith('KHCOM_PPU_')}
        env['KHCOM_PPU_SPOTLIGHT_CLEAR_ROWS'] = '1'
        if on:
            env[FLAG] = '1'
        raw = Path(directory) / f'{on}.bin'
        out = subprocess.run([str(exe), str(raw)], env=env, check=True, capture_output=True, text=True, timeout=60)
        results[on] = (raw.read_bytes(), [int(x) for x in out.stdout.split()[-3:]])
    off, on = results[False], results[True]
    assert off[1][0] == 0 and on[1][0] > 60, (off[1], on[1])
    assert on[1][2] > 0, 'no margin-row BG text exercised'
    assert off[0] == on[0], 'outputs differ'
    print(f'PASS spotlight margin skip: {on[1][0]} skipping frames, {on[1][1]} glyphs ({on[1][2]} in margins), identical outputs')
