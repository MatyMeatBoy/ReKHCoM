#!/usr/bin/env python3
"""Build the PS4 alpha (a fake-signed .pkg for consoles with a homebrew enabler such as GoldHEN).

The game image is the PS5 build's guest (port/ps5/build_ps5.py guest): x32 code the PS4's CPU runs as it is. This
script builds those images, the PS4 host (port/ps4/ps4_host.c) with the OpenOrbis toolchain, and the package. It runs
on Windows (the toolchain ships Windows tools) or Linux.

    python port/ps4/build_ps4.py                       # all: guest images eu us jp, host, package
    python port/ps4/build_ps4.py host                  # only the host and the package (guest images already built)
    python port/ps4/build_ps4.py test-host             # the same host for Linux, headless (run it in the Linux VM)

Needs: OO_PS4_TOOLCHAIN (default D:/01Claude/ReKHCoM-build/ps4/toolchain, the OpenOrbis v0.5.4 release), clang and ld.lld
(the repository's llvm-mingw on Windows), .NET 6 or later for PkgTool, and the generated assets of each region
(build/<region>, as for the desktop build). Output: port/ps4/dist/ReKHCoM-ps4-alpha.pkg.
"""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
BUILD = HERE / 'build'
DIST = HERE / 'dist'
OO = Path(os.environ.get('OO_PS4_TOOLCHAIN', 'D:/01Claude/ReKHCoM-build/ps4/toolchain'))
WINDOWS = os.name == 'nt'
LLVM = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin'
CLANG = os.environ.get('CLANG', str(LLVM / 'clang.exe') if WINDOWS else 'clang-18')
LLD = os.environ.get('LLD', str(LLVM / 'ld.lld.exe') if WINDOWS else 'ld.lld-18')
OBJCOPY = os.environ.get('OBJCOPY', str(LLVM / 'llvm-objcopy.exe') if WINDOWS else 'llvm-objcopy-18')

TITLE = 'ReKHCoM'
TITLE_ID = 'RKHC00001'
CONTENT_ID = f'IV0000-{TITLE_ID}_00-REKHCOMPS4000000'
LIBS = ['-lc', '-lkernel', '-lSceUserService', '-lSceVideoOut', '-lSceAudioOut', '-lScePad', '-lSceSysmodule',
        '-lSceSystemService', '-lSDL2']


def run(cmd, **kw):
    r = subprocess.run([str(c) for c in cmd], **kw)
    if r.returncode:
        sys.exit(f'{Path(str(cmd[0])).name} failed ({r.returncode})')


def tool(name):
    return OO / 'bin' / ('windows' if WINDOWS else 'linux') / (name + ('.exe' if WINDOWS else ''))


def version():
    import re
    text = (ROOT / 'port/include/port/port_version.h').read_text(encoding='utf-8')
    major, minor = re.search(r'#define REKHCOM_VERSION\s+"v(\d+)\.(\d+)', text).groups()
    return f'{int(major):02d}.{int(minor):02d}'


def build_guests(regions, jobs):
    env = dict(os.environ, CLANG=CLANG, LLD=LLD, OBJCOPY=OBJCOPY, PYTHONUTF8='1')
    for region in regions:
        run([sys.executable, ROOT / 'port/ps5/build_ps5.py', 'guest', '--region', region, '--jobs', str(jobs)], cwd=ROOT, env=env)


def build_host():
    BUILD.mkdir(parents=True, exist_ok=True)
    obj, elf = BUILD / 'ps4_host.o', BUILD / 'ps4_host.elf'
    run([CLANG, '--target=x86_64-pc-freebsd12-elf', '-fPIC', '-funwind-tables', '-O2', '-Wall', '-Wno-unused-but-set-global',
         '-DREKHCOM_PS4', '-isysroot', OO, '-isystem', OO / 'include', f'-I{ROOT / "port/ps5/shared"}', '-c',
         HERE / 'ps4_host.c', '-o', obj])
    run([LLD, '-m', 'elf_x86_64', '-pie', '--script', OO / 'link.x', '--eh-frame-hdr', f'-L{OO / "lib"}', *LIBS, obj,
         OO / 'lib/crt1.o', '-o', elf])
    return elf


def build_test_host():
    """The host for Linux, headless: checks the bridge, the rasterizer and the fault handler without a console."""
    BUILD.mkdir(parents=True, exist_ok=True)
    out = BUILD / 'ps4_host_linux'
    run(['clang', '-O2', '-Wall', '-Wno-unused-but-set-global', f'-I{ROOT / "port/ps5/shared"}', HERE / 'ps4_host.c', '-o', out,
         '-lm', '-lpthread'])
    print('test host:', out)


def gp4(stage, files):
    """The package project: create-gp4 only knows the sample's folders, so it is written here."""
    import datetime
    import xml.etree.ElementTree as ET
    root = ET.Element('psproject', {'fmt': 'gp4', 'version': '1000'})
    volume = ET.SubElement(root, 'volume')
    ET.SubElement(volume, 'volume_type').text = 'pkg_ps4_app'
    ET.SubElement(volume, 'volume_id').text = 'PS4VOLUME'
    ET.SubElement(volume, 'volume_ts').text = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    ET.SubElement(volume, 'package', {'content_id': CONTENT_ID, 'passcode': '0' * 32, 'storage_type': 'digital50',
                                      'app_type': 'full'})
    info = ET.SubElement(volume, 'chunk_info', {'chunk_count': '1', 'scenario_count': '1'})
    ET.SubElement(ET.SubElement(info, 'chunks'), 'chunk', {'id': '0', 'layer_no': '0', 'label': 'Chunk #0'})
    scenarios = ET.SubElement(info, 'scenarios', {'default_id': '0'})
    ET.SubElement(scenarios, 'scenario', {'id': '0', 'type': 'sp', 'initial_chunk_count': '1', 'label': 'Scenario #0'}).text = '0'
    listing = ET.SubElement(root, 'files', {'img_no': '0'})
    for f in files:
        ET.SubElement(listing, 'file', {'targ_path': f, 'orig_path': f})

    def tree(parent, folder):
        for d in sorted(p for p in folder.iterdir() if p.is_dir()):
            tree(ET.SubElement(parent, 'dir', {'targ_name': d.name}), d)
    tree(ET.SubElement(root, 'rootdir'), stage)
    ET.indent(root, '\t')
    return '<?xml version="1.0"?>\n' + ET.tostring(root, encoding='unicode') + '\n'


def package(regions, elf):
    import struct
    sys.path.insert(0, str(ROOT / 'port/ps5'))
    import build_ps5
    stage = BUILD / 'pkg'
    if stage.exists():
        shutil.rmtree(stage)
    (stage / 'sce_sys/about').mkdir(parents=True)
    (stage / 'sce_module').mkdir()
    assets = stage / 'assets'
    assets.mkdir()
    for region in regions:
        shutil.copy2(ROOT / f'port/ps5/build/guest-{region}/guest.bin', assets / f'guest-{region}.bin')
    build_ps5.render_messages(assets)
    build_ps5.render_toasts(assets)
    shutil.copytree(ROOT / 'port/ps5/assets/fonts', assets / 'fonts')
    (assets / 'achievements').mkdir()
    shutil.copy2(ROOT / 'port/assets/achievements/badges.png', assets / 'achievements/badges.png')
    sample = OO / 'samples/SDL2'
    shutil.copy2(sample / 'sce_sys/about/right.sprx', stage / 'sce_sys/about/right.sprx')
    for prx in ('libc.prx', 'libSceFios2.prx'):
        shutil.copy2(sample / 'sce_module' / prx, stage / 'sce_module' / prx)
    from PIL import Image
    Image.open(ROOT / 'port/ps5/sce_sys/icon0.png').convert('RGB').resize((512, 512)).save(stage / 'sce_sys/icon0.png')
    Image.open(ROOT / 'port/ps5/sce_sys/back.png').convert('RGB').resize((1920, 1080)).save(stage / 'sce_sys/pic1.png')

    env = dict(os.environ, OO_PS4_TOOLCHAIN=str(OO), DOTNET_ROLL_FORWARD='Major')
    run([tool('create-fself'), f'-in={elf}', f'-out={BUILD / "ps4_host.oelf"}', '--eboot', stage / 'eboot.bin',
         '--paid', '0x3800000000000011'], env=env)
    sfo = stage / 'sce_sys/param.sfo'
    pkgtool = tool('PkgTool.Core')
    run([pkgtool, 'sfo_new', sfo], env=env)
    ver = version()
    for key, kind, size, value in (('APP_TYPE', 'Integer', 4, '1'), ('APP_VER', 'Utf8', 8, ver), ('ATTRIBUTE', 'Integer', 4, '0'),
                                   ('CATEGORY', 'Utf8', 4, 'gd'), ('CONTENT_ID', 'Utf8', 48, CONTENT_ID),
                                   ('DOWNLOAD_DATA_SIZE', 'Integer', 4, '0'), ('SYSTEM_VER', 'Integer', 4, '0'),
                                   ('TITLE', 'Utf8', 128, TITLE), ('TITLE_ID', 'Utf8', 12, TITLE_ID), ('VERSION', 'Utf8', 8, ver)):
        run([pkgtool, 'sfo_setentry', sfo, key, '--type', kind, '--maxsize', str(size), '--value', value], env=env)
    files = sorted(str(p.relative_to(stage)).replace('\\', '/') for p in stage.rglob('*') if p.is_file())
    (stage / 'pkg.gp4').write_text(gp4(stage, files), encoding='utf-8')
    DIST.mkdir(parents=True, exist_ok=True)
    run([pkgtool, 'pkg_build', 'pkg.gp4', DIST], cwd=stage, env=env)
    built = DIST / f'{CONTENT_ID}.pkg'
    final = DIST / 'ReKHCoM-ps4-alpha.pkg'
    if final.exists():
        final.unlink()
    built.rename(final)
    print('package:', final, f'({final.stat().st_size} bytes)')
    return final


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('step', nargs='?', default='all', choices=['all', 'guest', 'host', 'test-host'])
    p.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    p.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2))
    args = p.parse_args()
    if args.step == 'test-host':
        build_test_host()
        return
    if not (OO / 'link.x').is_file():
        sys.exit(f'OpenOrbis toolchain not found at {OO} (set OO_PS4_TOOLCHAIN)')
    if args.step in ('all', 'guest'):
        build_guests(args.regions, args.jobs)
    if args.step in ('all', 'host'):
        package(args.regions, build_host())


if __name__ == '__main__':
    main()
