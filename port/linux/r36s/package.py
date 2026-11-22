"""Build a manual-install R36S ZIP from the audited AArch64 build output."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / 'port/tools'))
from build_linux import install_common_assets, install_runtime_version, make_zip


HOST = 'binarm64/rekhcom-arm64'
SDL = 'libarm64/libSDL3.so.0'
GUESTS = tuple(f'binarm64/guest-{region}.macho' for region in ('eu', 'us', 'jp'))
RUNTIME_FILES = (HOST, SDL, *GUESTS)
SYSTEM_LIBS = {
    'ld-linux-aarch64.so.1', 'libc.so.6', 'libm.so.6', 'libdl.so.2',
    'libpthread.so.0', 'librt.so.1', 'libgcc_s.so.1',
}


def audit_elf(path, readelf, *, host):
    report = subprocess.check_output([str(readelf), '-h', '-l', '-d', '-V', str(path)], text=True)
    if (not re.search(r'Class:\s+ELF64', report) or not re.search(r'Machine:\s+AArch64', report) or
            not re.search(r"Data:.*little endian", report)):
        raise ValueError(f'{path}: expected a little-endian AArch64 ELF64 binary')
    dependencies = set(re.findall(r'\(NEEDED\).*\[([^]\n]+)\]', report))
    allowed = SYSTEM_LIBS | {'libSDL3.so.0'} if host else SYSTEM_LIBS
    if unexpected := dependencies - allowed:
        raise ValueError(f'{path}: unexpected runtime libraries: {", ".join(sorted(unexpected))}')
    if host and ('[Requesting program interpreter: /lib/ld-linux-aarch64.so.1]' not in report or
                 'libSDL3.so.0' not in dependencies):
        raise ValueError(f'{path}: expected the ARM64 system loader and libSDL3.so.0')
    if not host and not re.search(r'\(SONAME\).*\[libSDL3\.so\.0\]', report):
        raise ValueError(f'{path}: expected libSDL3.so.0 SONAME')
    versions = [tuple(map(int, version.split('.'))) for version in re.findall(r'\bGLIBC_(\d+(?:\.\d+)+)', report)]
    if max(versions, default=(0,)) > (2, 31) or re.search(r'\b(?:GLIBC_PRIVATE|GLIBCXX_|CXXABI_)', report):
        raise ValueError(f'{path}: runtime requirements exceed the R36S Ubuntu Focal ABI')
    return report


def audit_guest(path):
    with path.open('rb') as stream:
        header = stream.read(28)
    if len(header) != 28:
        raise ValueError(f'{path}: truncated Mach-O guest')
    magic, cpu, _, kind, _, _, flags = struct.unpack('<7I', header)
    if magic != 0xFEEDFACE or cpu != 0x0200000C or kind != 2 or not flags & 0x200000:
        raise ValueError(f'{path}: expected an ARM64 ILP32 relocatable Mach-O executable')


def create_package(runtime, archive, sdl_license, readelf):
    if archive.exists():
        raise FileExistsError(archive)
    for path in (*(runtime / name for name in RUNTIME_FILES), sdl_license):
        if not path.is_file():
            raise FileNotFoundError(path)
    for name in (HOST, SDL):
        print(audit_elf(runtime / name, readelf, host=name == HOST))
    for name in GUESTS:
        audit_guest(runtime / name)

    with tempfile.TemporaryDirectory(prefix='rekhcom-r36s-') as folder:
        stage = Path(folder)
        game = stage / 'rekhcomr36s'
        for name in RUNTIME_FILES:
            target = game / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(runtime / name, target)
        for source, target in ((HERE / 'ReKHCoM.sh', stage / 'ReKHCoM R36S.sh'),
                               (HERE.parent / 'ReKHCoM.sh', game / 'ReKHCoM.sh')):
            target.write_bytes(source.read_bytes().replace(b'\r\n', b'\n'))
        install_runtime_version(game)
        install_common_assets(game)
        shutil.copyfile(HERE / 'README.md', game / 'R36S.md')
        (game / 'licenses').mkdir()
        shutil.copyfile(sdl_license, game / 'licenses/SDL3-LICENSE.txt')
        (game / 'roms').mkdir()
        (game / 'roms/README.txt').write_text('Copy your own EU, US or JP GBA cartridge dump here.\n', encoding='ascii')
        archive.parent.mkdir(parents=True, exist_ok=True)
        with archive.open('xb') as stream:
            try:
                make_zip(stage, stream)
            except BaseException:
                stream.close()
                archive.unlink()
                raise
    with archive.open('rb') as stream:
        print(hashlib.file_digest(stream, 'sha256').hexdigest(), archive)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'build/r36s/out')
    parser.add_argument('--sdl-license', type=Path, required=True)
    compiler = next((p / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-readelf.exe'
                     for p in ROOT.parents if (p / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-readelf.exe').is_file()),
                    'llvm-readelf')
    parser.add_argument('--readelf', default=compiler)
    args = parser.parse_args()
    try:
        create_package(args.runtime, args.archive, args.sdl_license, args.readelf)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
