"""Compile only the standard C++ source probe; synthetic data, no game/Steam build."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
    if not compiler:
        candidates = sorted((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/clang++.exe'))
        compiler = str(candidates[-1]) if candidates else None
    if not compiler:
        raise SystemExit('C++17 compiler required (set CXX)')
    with tempfile.TemporaryDirectory(prefix='khcom-hd-source-') as folder:
        exe = Path(folder) / ('probe.exe' if os.name == 'nt' else 'probe')
        subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-static',
                        str(ROOT / 'port/tools/test_hd_media_source.cpp'), '-o', str(exe)], check=True)
        subprocess.run([str(exe), str(Path(folder) / 'fixtures')], check=True, timeout=20)


if __name__ == '__main__':
    main()
