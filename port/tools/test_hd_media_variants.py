"""Standard C++ transaction/runtime resolver fixtures; no game/SDK/media build."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import re

ROOT = Path(__file__).resolve().parents[2]


def main():
    actual = set(re.findall(r'"(zz_event_\d+)"', (ROOT / 'port/decompui/hd_media_scenes.inc').read_text()))
    expected = set(re.findall(r'"(zz_event_\d+)"', (ROOT / 'port/src/port_hdscenes.inc').read_text())) | {'zz_event_001', 'zz_event_614', 'zz_event_631'}
    assert actual == expected, 'variant completeness inventory differs from game story/extras map'
    compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
    if not compiler:
        compiler = str(sorted((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/clang++.exe'))[-1])
    with tempfile.TemporaryDirectory(prefix='khcom-media-variants-') as folder:
        exe = Path(folder) / ('test.exe' if os.name == 'nt' else 'test')
        subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-static',
                        str(ROOT / 'port/tools/test_hd_media_variants.cpp'), '-o', str(exe)], check=True)
        subprocess.run([str(exe), str(Path(folder) / 'data')], check=True, timeout=20)


if __name__ == '__main__':
    main()
