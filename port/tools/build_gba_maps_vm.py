#!/usr/bin/env python3
"""Build the original GBA ROMs with the decomp inside the Debian VM and fetch their link maps.

The maps give the cartridge address of every IWRAM/EWRAM variable, which the
RetroAchievements layer uses (with the ELF symbol tables, which also list
the static variables the map leaves out) to project the port's globals onto the GBA memory
map (port/progress/LOGROS_71_VIA_RAPIDA.md). Each ROM is checked against its
sha1 by the decomp's own build; only the maps leave the VM.

    python port/tools/build_gba_maps_vm.py            # -> build/gba-maps/com_{eu,us,jp}.{map,elf}

Needs: a clean tree (the source is HEAD), roms/B8CP.gba, B8CE.gba and B8CJ.gba,
the VM image D:/01Claude/ReKHCoM-build/ps5/ps5env.qcow2 (network for the first run:
apt packages, agbcc, binutils 2.10, gbagfx; they persist on the VM disk).
"""
import argparse
import os
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
IMAGE = Path('D:/01Claude/ReKHCoM-build/ps5/ps5env.qcow2')
SHARE = Path('D:/01Claude/ReKHCoM-build/ps5/share-gba')
AGBCC_REVISION = 'da598c1d918402c42c0c0d7128ba14567f3175e9'

SCRIPT = r'''#!/bin/bash
set -ex
export DEBIAN_FRONTEND=noninteractive
command -v arm-none-eabi-as >/dev/null && command -v arm-none-eabi-cpp >/dev/null && command -v bison >/dev/null && python3 -c 'import yaml' && command -v ninja >/dev/null \
  || { apt-get update -qq && apt-get install -y -qq build-essential bison flex binutils-arm-none-eabi gcc-arm-none-eabi python3-yaml ninja-build libpng-dev git curl ca-certificates; }
mkdir -p /root/gba /root/gba-cache
cd /root/gba
find . -mindepth 1 -maxdepth 1 ! -name tools ! -name build -exec rm -rf {} +
tar -xf /root/share/source.tar -C /root/gba
mkdir -p roms && cp /root/share/roms/B8C?.gba roms/
# agbcc (old_agbcc) into tools/agbcc, kept on the VM disk between runs
if [ ! -x /root/gba-cache/agbcc/bin/old_agbcc ]; then
  if [ ! -x /root/gba-cache/agbcc-src/old_agbcc ]; then
    rm -rf /root/gba-cache/agbcc-src
    git clone --quiet https://github.com/pret/agbcc /root/gba-cache/agbcc-src
    git -C /root/gba-cache/agbcc-src checkout --quiet AGBCC_REVISION
    (cd /root/gba-cache/agbcc-src && ./build.sh)
  fi
  rm -rf /root/gba-cache/install && mkdir -p /root/gba-cache/install
  (cd /root/gba-cache/agbcc-src && ./install.sh /root/gba-cache/install)
  rm -rf /root/gba-cache/agbcc && cp -a /root/gba-cache/install/tools/agbcc /root/gba-cache/agbcc
fi
rm -rf tools/agbcc && cp -a /root/gba-cache/agbcc tools/agbcc
if [ ! -x /root/gba-cache/legacy/bin/arm-elf-as ] || [ ! -f /root/gba-cache/legacy/lib/libc.a ] || [ ! -f /root/gba-cache/legacy/lib/libgcc.a ]; then
  python3 tools/setup_legacy_toolchain.py --prefix /root/gba-cache/legacy
fi
rm -rf tools/legacy && cp -a /root/gba-cache/legacy tools/legacy
if [ ! -x /root/gba-cache/gbagfx ]; then sh tools/fetch_gbagfx.sh && cp tools/gbagfx/gbagfx /root/gba-cache/gbagfx; fi
mkdir -p tools/gbagfx && cp /root/gba-cache/gbagfx tools/gbagfx/gbagfx
rm -rf /root/gba-maps && mkdir -p /root/gba-maps
for v in us eu jp; do
  python3 tools/extract_assets.py $v
  python3 configure.py --version $v > /dev/null
  ninja -j"$(nproc)"
  cp build/$v/com_$v.map build/$v/com_$v.elf /root/gba-maps/
  sha1sum build/$v/com_$v.gba | tee -a /root/gba-maps/sha1.txt
  # The same link again with a current ld and --emit-relocs: every pointer word of the ROM shows up as a
  # relocation (the relocatable GBA space of the Vita and Switch builds needs that list). The result must be
  # the same ROM, byte for byte.
  link=$(ninja -t commands build/$v/com_$v.elf | tail -1)
  link=${link/tools\/legacy\/bin\/arm-elf-ld/arm-none-eabi-ld -q --no-warn-rwx-segments}
  link=${link/-o build\/$v\/com_$v.elf/-o build\/$v\/com_$v.relocs.elf}
  link=${link/-Map build\/$v\/com_$v.map/-Map build\/$v\/com_$v.relocs.map}
  eval "$link"
  arm-none-eabi-objcopy -O binary build/$v/com_$v.relocs.elf build/$v/relocs.bin
  arm-none-eabi-objcopy -O binary build/$v/com_$v.elf build/$v/orig.bin
  cmp build/$v/relocs.bin build/$v/orig.bin && echo "relink $v identical"
  arm-none-eabi-readelf -rW build/$v/com_$v.relocs.elf | gzip > /root/gba-maps/com_$v.relocs.txt.gz
done
tar -cf /root/gba-maps.tar -C /root gba-maps
echo MAPS-OK
'''.replace('AGBCC_REVISION', AGBCC_REVISION)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--out', type=Path, default=ROOT / 'build/gba-maps')
    ap.add_argument('--cpus', default='8')
    ap.add_argument('--mem', default='6144')
    args = ap.parse_args()
    if subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=no'], cwd=ROOT, text=True).strip():
        sys.exit('Commit your changes first: the ROMs are built from HEAD')
    for code in ('B8CP', 'B8CE', 'B8CJ'):
        if not (ROOT / 'roms' / f'{code}.gba').is_file():
            sys.exit(f'Missing roms/{code}.gba')
    if SHARE.exists():
        shutil.rmtree(SHARE)
    (SHARE / 'roms').mkdir(parents=True)
    subprocess.check_call(['git', 'archive', 'HEAD', '-o', str(SHARE / 'source.tar')], cwd=ROOT)
    for code in ('B8CP', 'B8CE', 'B8CJ'):
        shutil.copyfile(ROOT / 'roms' / f'{code}.gba', SHARE / 'roms' / f'{code}.gba')
    (SHARE / 'build-gba.sh').write_bytes(SCRIPT.encode())
    fetched = args.out / 'fetch'
    command = [sys.executable, str(ROOT / 'port/tools/linux_vm_test.py'), '--share', str(SHARE),
               '--disk', str(IMAGE), '--net', '--mem', args.mem, '--cpus', args.cpus,
               '--bg-run', 'bash /root/share/build-gba.sh', '--poll', '30', '--stall', '1800',
               '--fetch', '/root/gba-maps.tar', '--out', str(fetched)]
    subprocess.run(command, cwd=ROOT, check=True, env=dict(os.environ, MSYS_NO_PATHCONV='1', PYTHONUTF8='1'))
    package = fetched / 'gba-maps.tar'
    if not package.is_file():
        sys.exit('The VM did not return the maps: see the log above')
    with tarfile.open(package) as t:
        t.extractall(args.out, filter='data')
    shutil.rmtree(fetched)
    for v in ('eu', 'us', 'jp'):
        m = args.out / 'gba-maps' / f'com_{v}.map'
        print(v, m, m.stat().st_size if m.exists() else 'MISSING')


if __name__ == '__main__':
    main()
