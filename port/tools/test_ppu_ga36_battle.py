"""Host check of the opt-in GA36 battle/field routes of port_ppu.c.

ppu_ga36_battle.c draws 600 random 320x240 frames with the production
renderer (NEON through SIMDe's host header). The run with every route off is
the scalar reference; the run with them on must give the same pixels, margins
and occlusion buffers.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
tools = Path(os.environ.get('KHCOM_TOOLCHAIN', 'D:/01Claude/ReKHCoM/toolchain'))
clang = sorted(tools.glob('llvm-mingw-*'))[-1] / 'bin/i686-w64-mingw32-clang.exe'
sdl = sorted(tools.glob('SDL3-*'))[-1] / 'i686-w64-mingw32/include'
simde = tools / 'emsdk/upstream/emscripten/system/include/compat/arm_neon.h'
includes = [root / 'include'] + [p for p in (root / 'include').iterdir() if p.is_dir() and p.name != 'gba']
with tempfile.TemporaryDirectory() as directory:
    out = Path(directory)
    shutil.copyfile(simde, out / 'arm_neon.h')  # alone: its neighbours replace the C library
    exe = out / 'battle.exe'
    command = [str(clang), '-O2', '-w', '-std=gnu11', '-funsigned-char', '-fno-strict-aliasing', '-fwrapv',
               '-mno-ms-bitfields', '-DPLATFORM_PC', '-DVERSION_EU', '-I' + str(out), '-I' + str(root / 'port/include'),
               '-I' + str(sdl), '-include', 'SDL3/SDL.h']
    for path in includes:
        command += ['-iquote', str(path)]
    subprocess.run(command + [str(root / 'port/tools/ppu_ga36_battle.c'), '-o', str(exe)], check=True)
    runs = []
    for fast in (False, True):
        env = dict(os.environ)
        env.pop('GA36_FAST', None)
        env.pop('KHCOM_PPU_HEADER_GONE_PARALLEL', None)
        if fast:
            env['GA36_FAST'] = '1'
            env['KHCOM_PPU_HEADER_GONE_PARALLEL'] = '1'
        runs.append(subprocess.run([str(exe)], env=env, check=True, capture_output=True, text=True, timeout=300))
    reference, fast = (run.stdout.splitlines() for run in runs)
    assert len(reference) == 600 == len(fast)
    different = [(a, b) for a, b in zip(reference, fast) if a != b]
    assert not different, (len(different), different[:5])
    assert len({line.split()[-1] for line in reference}) > 500
    assert ' 0' not in runs[1].stderr, runs[1].stderr
    print('PASS GA36 battle/field routes: 600 frames identical;', runs[1].stderr.strip())
