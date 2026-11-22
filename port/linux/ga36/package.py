"""Pack the existing Linux armhf build for manual installation on GA36."""
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
from build_linux import WORK_DIR, bundle, install_armhf_layout, make_zip


REGIONS = ('eu', 'us', 'jp')
STOCK_KERNEL = (3, 4, 39)


def audit_elf(path, readelf, *, require_kernel_abi=False, interpreter=False):
    with path.open('rb') as stream:
        header = stream.read(52)
    if len(header) != 52 or header[:7] != b'\x7fELF\x01\x01\x01' or struct.unpack_from('<H', header, 18)[0] != 40:
        raise ValueError(f'{path}: expected a little-endian ARM ELF32 image')
    flags = struct.unpack_from('<I', header, 36)[0]
    if flags & 0xFF000600 != 0x05000400:
        raise ValueError(f'{path}: expected ARM EABI5 hard-float')
    if interpreter:
        report = subprocess.check_output([str(readelf), '-l', str(path)], text=True)
        if '[Requesting program interpreter: libarmhf/rekhcom]' not in report:
            raise ValueError(f'{path}: expected the bundled ARM interpreter at libarmhf/rekhcom')
    if require_kernel_abi:
        report = subprocess.check_output([str(readelf), '-n', str(path)], text=True)
        match = re.search(r'OS: Linux, ABI: (\d+)\.(\d+)\.(\d+)', report)
        if not match or tuple(map(int, match.groups())) > STOCK_KERNEL:
            raise ValueError(f'{path}: expected a Linux kernel ABI requirement at or below 3.4.39')
        print(f'{path.name}: Linux kernel ABI {match[0].rsplit(" ", 1)[1]}')


def create_package(base, archive, readelf):
    if archive.exists():
        raise FileExistsError(archive)
    with tempfile.TemporaryDirectory(prefix='rekhcom-ga36-') as folder:
        stage = Path(folder)
        game = stage / 'rekhcomga36'
        bundle('armhf', base, REGIONS, game)
        install_armhf_layout(game)
        for region in REGIONS:
            audit_elf(game / f'binarmhf/khcom_{region}', readelf, interpreter=True)
        audit_elf(game / 'libarmhf/rekhcom', readelf)
        audit_elf(game / 'libarmhf/libc.so.6', readelf, require_kernel_abi=True)
        audit_elf(game / 'libarmhf/sys/libSDL3.so.0', readelf)
        (stage / 'ReKHCoM GA36.sh').write_bytes((HERE / 'ReKHCoM.sh').read_bytes().replace(b'\r\n', b'\n'))
        shutil.copyfile(HERE / 'README.md', game / 'GA36.md')
        (game / 'roms/README.txt').write_text('Copy your own EU, US or JP GBA cartridge dump here.\n', encoding='ascii')
        archive.parent.mkdir(parents=True, exist_ok=True)
        with archive.open('xb') as output:
            try:
                make_zip(stage, output)
            except BaseException:
                output.close()
                archive.unlink()
                raise
    with archive.open('rb') as stream:
        print(hashlib.file_digest(stream, 'sha256').hexdigest(), archive)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--base', type=Path, default=WORK_DIR / 'linux/armhf')
    parser.add_argument('--readelf', default='llvm-readelf')
    args = parser.parse_args()
    try:
        create_package(args.base, args.archive, args.readelf)
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
