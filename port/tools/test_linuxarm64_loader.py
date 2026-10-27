"""Compile/run portable loader logic only; never execute an ARM game image."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]

def main():
    compiler = os.environ.get('CC') or shutil.which('clang') or shutil.which('gcc')
    if not compiler:
        tools = next(p / 'toolchain' for p in ROOT.parents if (p / 'toolchain').is_dir())
        compiler = str(sorted(tools.glob('llvm-mingw-*/bin/clang.exe'))[-1])
    with tempfile.TemporaryDirectory(prefix='linux-arm64-loader-') as directory:
        exe = Path(directory) / ('probe.exe' if os.name == 'nt' else 'probe')
        subprocess.run([compiler, '-std=c11', '-O1', str(ROOT / 'port/tools/test_linuxarm64_loader.c'),
                        '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
        if os.name == 'nt':
            subprocess.run([compiler, '-std=c11', '-O1', '-I'+str(ROOT/'port/ps5/shared'),
                            '-I'+str(ROOT/'port/linuxarm64'), str(ROOT/'port/tools/test_linuxarm64_image_win.c'),
                            '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
        else:
            print('Whole-image Windows mapping adapter skipped; use real Linux mapping QA separately.')
if __name__ == '__main__': main()
