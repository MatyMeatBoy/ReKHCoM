"""Execute the production variant import coordinator with a synthetic file copier.

Real source fingerprint, transaction, cache and resolver; no MF decode, ROM or assets.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
    if not compiler:
        compiler = str(sorted((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/clang++.exe'))[-1])
    source = (ROOT / 'port/decompui/hd_import.inc').read_text(encoding='utf-8')
    begin = source.index('static bool ImportMediaVariant(')
    end = source.index('\nstatic void Run(', begin)
    bases = source[source.index('static const char* const kBaseMovies[]'):]
    bases = bases[:bases.index(';') + 1]
    unit = bases + '\n' + (ROOT / 'port/decompui/hd_media_scenes.inc').read_text() + '\n' + source[begin:end]
    with tempfile.TemporaryDirectory(prefix='khcom-media-import-') as folder:
        Path(folder, 'hd_media_import.inc').write_text(unit, encoding='utf-8')
        exe = Path(folder) / ('test.exe' if os.name == 'nt' else 'test')
        subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-static', '-I' + folder,
                        str(ROOT / 'port/tools/test_hd_media_import.cpp'), '-o', str(exe)], check=True)
        subprocess.run([str(exe), str(Path(folder) / 'fixtures')], check=True, timeout=20)


if __name__ == '__main__':
    main()
