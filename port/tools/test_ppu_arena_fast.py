"""Host check of KHCOM_PPU_ARENA_FAST in port_ppu.c.

ppu_arena_fast.c draws random battle-arena affine rows (reflected ornament
bands, extended floor, wrap and no wrap, extreme and random matrices, several
screen widths) with the reference per-pixel route and the fast row route;
every pixel and world flag must match.
"""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
tools = Path(os.environ.get('KHCOM_TOOLCHAIN', 'D:/01Claude/ReKHCoM/toolchain'))
clang = sorted(tools.glob('llvm-mingw-*'))[-1] / 'bin/i686-w64-mingw32-clang.exe'
sdl = sorted(tools.glob('SDL3-*'))[-1] / 'i686-w64-mingw32/include'
includes = [root / 'include'] + [p for p in (root / 'include').iterdir() if p.is_dir() and p.name != 'gba']
with tempfile.TemporaryDirectory() as directory:
    exe = Path(directory) / 'arena.exe'
    command = [str(clang), '-O2', '-w', '-std=gnu11', '-funsigned-char', '-fno-strict-aliasing', '-fwrapv',
               '-mno-ms-bitfields', '-DPLATFORM_PC', '-DVERSION_EU', '-I' + str(root / 'port/include'),
               '-I' + str(sdl), '-include', 'SDL3/SDL.h']
    for path in includes:
        command += ['-iquote', str(path)]
    subprocess.run(command + [str(root / 'port/tools/ppu_arena_fast.c'), '-o', str(exe)], check=True)
    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=300)
    assert run.returncode == 0, run.stdout + run.stderr
    counts = dict(zip(run.stdout.split()[0::2], map(int, run.stdout.split()[1::2])))
    assert counts['banded'] > 1000 and counts['floor'] > 100, counts
    print('PASS arena fast affine rows:', run.stdout.strip())
