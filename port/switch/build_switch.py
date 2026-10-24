#!/usr/bin/env python3
"""Build the Switch alpha: the game image (AArch64 with 32-bit pointers) and the libnx host (.nro).

The game core needs 32-bit pointers, and the Switch runs only 64-bit programs that cannot map the GBA's low
addresses. So, as on the PS5 (port/ps5/README.md), the game and the port layer are one image the host loads and
calls through the bridge (port/ps5/shared/bridge_list.h), with two differences:

- the image is AArch64 code with 32-bit pointers: clang's arm64_32 target (Mach-O objects, linked by lld's Mach-O
  driver at 0x10000000, above the code region's start);
- the GBA space is relocatable (PORT_GBA_RELOC, include/gba/gba_space.h): every region is an array of the image.

    python port/switch/build_switch.py guest            # build/switch/guest-<region>.macho, on Windows or Linux
    python port/switch/build_switch.py host             # build/switch/ReKHCoM.nro (devkitPro, in the consoles VM)

Needs the generated assets of each region (build/<region>) and the ROM pointer lists (port/data/rom_pointers_*.inc).
"""
import argparse
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PS5 = ROOT / 'port/ps5'
BUILD = ROOT / 'build/switch'
WINDOWS = os.name == 'nt'
LLVM = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin'
CLANG = os.environ.get('CLANG', str(LLVM / 'clang.exe') if WINDOWS else 'clang-18')
LLD = os.environ.get('LLD', str(LLVM / 'ld.lld.exe') if WINDOWS else 'ld.lld-18')
TARGET = '--target=arm64_32-apple-watchos'
CPU = '-mcpu=cortex-a57'  # the Switch's CPU (ARMv8.0): without it clang targets Apple's and emits ARMv8.3 (xpaci)
BASE = 0x10000000
VM_DISK = Path('D:/01Claude/ReKHCoM-build/ps5/ps5env.qcow2')  # the consoles build VM (port/progress/TOOLKIT_CONSOLAS.md)
sys.path.insert(0, str(PS5))
import build_ps5  # noqa: E402  (its PORT_PATCHES and PORT_EXCLUDED are shared)

GUEST_SKIP = {'crt0', 'stubs'}  # x86-64 assembly: port/switch/guest has the AArch64 ones


def run(cmd, **kw):
    r = subprocess.run([str(c) for c in cmd], **kw)
    if r.returncode:
        sys.exit(f'{Path(str(cmd[0])).name} failed ({r.returncode})')


def common_flags():
    resource = subprocess.check_output([CLANG, '-print-resource-dir'], text=True).strip() + '/include'
    return [TARGET, CPU, '-ffreestanding', '-nostdinc', '-isystem', resource, f'-I{PS5 / "guest/include"}', f'-I{PS5 / "shared"}',
            '-O2', '-funsigned-char', '-fno-strict-aliasing', '-fwrapv', '-fno-stack-protector', '-fno-common',
            '-fno-omit-frame-pointer', '-fno-builtin-printf', '-fno-math-errno', '-DPORT_GBA_RELOC', '-DPORT_SWITCH']


def macho_symbols(source, out):
    """rom_symbols_reloc.s (COFF syntax for the Windows check build) as Mach-O: the regions as zero-filled space."""
    lines = []
    for line in source.read_text().splitlines():
        s = line.strip()
        if s.startswith('.section') or s.startswith('.balign'):
            continue
        m = re.match(r'_(\w+):$', s)
        if m:
            pending = m.group(1)
            continue
        m = re.match(r'\.space\s+(\S+)', s)
        if m:
            lines.append(f'\t.zerofill __GBA,__space,_{pending},{m.group(1)},12')
            continue
        lines.append(line)
    out.write_text('\n'.join(lines) + '\n')


def compile_all(jobs, units):
    def one(unit):
        src, obj, extra = unit
        obj.parent.mkdir(parents=True, exist_ok=True)
        if obj.exists() and obj.stat().st_mtime >= src.stat().st_mtime and not os.environ.get('REBUILD'):
            dep = obj.with_suffix('.d')
            if dep.exists():
                text = dep.read_text().replace('\\\n', ' ')
                # 'target: dep dep ...', with the spaces inside paths escaped ('00\ Claude')
                body = re.split(r':\s', text, maxsplit=1)[-1]
                deps = [Path(d.replace('\\ ', ' ')) for d in re.split(r'(?<!\\)\s+', body) if d]
                newest = max((d.stat().st_mtime for d in deps if d.exists()), default=0)
                if newest <= obj.stat().st_mtime:
                    return None
        cmd = [CLANG, *extra, '-MD', '-MF', obj.with_suffix('.d'), '-c', src, '-o', obj]
        r = subprocess.run([str(c) for c in cmd], capture_output=True, text=True)
        return f'{src}\n{r.stderr}' if r.returncode else None
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        errors = [e for e in pool.map(one, units) if e]
    if errors:
        print('\n'.join(errors[:5]), file=sys.stderr)
        sys.exit(f'{len(errors)} file(s) failed to compile')


def build_guest(region, jobs):
    upper = region.upper()
    pc = ROOT / 'build' / region / 'pc'
    if not (pc / 'rom_symbols_reloc.s').is_file():
        run([sys.executable, ROOT / 'port/tools/gen_pc_assets.py', '--version', region], cwd=ROOT,
            env=dict(os.environ, PYTHONUTF8='1'))
    out = BUILD / f'guest-{region}'
    out.mkdir(parents=True, exist_ok=True)
    includes = build_ps5.game_includes(region)
    game_flags = [*common_flags(), '-w', '-std=gnu89', '-include', ROOT / 'port/include/port/game_prefix.h', '-DPLATFORM_PC',
                  f'-DVERSION_{upper}', *includes]
    port_flags = [*common_flags(), '-w', '-std=gnu11', '-DSTBI_NO_THREAD_LOCALS', f'-I{PS5 / "third_party/stb"}',
                  f'-I{ROOT / "port/src"}', f'-I{ROOT / "port/android/native"}', '-DPLATFORM_PC', f'-DVERSION_{upper}',
                  '-DPORT_TOUCH', *includes]
    units = []
    for src in sorted((ROOT / 'src').rglob('*.c')):
        units.append((src, out / 'game' / src.relative_to(ROOT / 'src').with_suffix('.o'), game_flags))
    for src in sorted((ROOT / 'port/src').glob('*.c')):
        if src.stem in build_ps5.PORT_EXCLUDED:
            continue
        if src.name in build_ps5.PORT_PATCHES:
            src = build_ps5.patched_source(src, out / 'patched')
        units.append((src, out / 'port' / (src.stem + '.o'), port_flags))
    units.append((pc / 'rom_relocs.c', out / 'port/rom_relocs.o', port_flags))
    for src in sorted((PS5 / 'guest/src').glob('*.c')):
        if src.stem not in GUEST_SKIP:
            units.append((src, out / 'ps5' / (src.stem + '.o'), port_flags))
    units.append((HERE / 'guest/stubs_a64.c', out / 'switch/stubs_a64.o', port_flags))
    # the touch controls (the options button on the handheld screen, issue #35; the Android 64-bit build's controls)
    units.append((ROOT / 'port/android/native/touch_android.c', out / 'port/touch_android.o', port_flags))
    macho_symbols(pc / 'rom_symbols_reloc.s', out / 'rom_symbols.s')
    for src in (out / 'rom_symbols.s', HERE / 'guest/crt0_a64.S'):
        units.append((src, out / 'asm' / (src.stem + '.o'), [TARGET, CPU, '-c']))
    compile_all(jobs, units)
    image = BUILD / f'guest-{region}.macho'
    objects = out / 'objects.rsp'  # the object list as a response file: too long for a Windows command line
    objects.write_text('\n'.join('"' + u[1].as_posix() + '"' for u in units) + '\n')
    run([LLD, '-flavor', 'darwin', '-arch', 'arm64_32', '-platform_version', 'watchos', '9.0', '9.0', '-e', '_switch_boot',
         '-pagezero_size', hex(BASE), '-map', out / 'guest.map', '-o', image, f'@{objects}'])
    print(f'guest {region}: {image} ({image.stat().st_size} bytes)')
    return image


MESSAGES = {
    'need-rom': ('A compatible ROM is required',
                 ['Copy your Kingdom Hearts: Chain of Memories cartridge dump',
                  '(Europe, USA or Japan, a .gba file) to the SD card folder',
                  '/switch/ReKHCoM/',
                  'The game starts by itself as soon as the file is there.']),
    'load-error': ('The game could not be started',
                   ['Something failed while loading the game.',
                    'The reason is written to /switch/ReKHCoM/logs/ReKHCoM.log']),
}
TOASTS = {
    'toast-states': ['Minus + L3: save state', 'Minus + R3: load state'],
    'toast-update-found': ['A new version is available', 'Downloading it now'],
    'toast-update-done': ['ReKHCoM was updated', 'Restart the app to use the new version'],
}
VM_SCRIPT = r'''set -e
export DEVKITPRO=/opt/devkitpro
cd /root/share
P=$DEVKITPRO/portlibs/switch
CC=$DEVKITPRO/devkitA64/bin/aarch64-none-elf-gcc
ARCH="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE"
$CC $ARCH -O2 -Wall -D__SWITCH__ -Isrc -I$DEVKITPRO/libnx/include -I$P/include -c src/switch_host.c -o host.o
$CC $ARCH -specs=$DEVKITPRO/libnx/switch.specs host.o -L$DEVKITPRO/libnx/lib -L$P/lib     $($P/bin/sdl2-config --libs) -lnx -lm -o ReKHCoM.elf
$DEVKITPRO/tools/bin/nacptool --create "ReKHCoM" "ReKHCoM project" "VERSION" ReKHCoM.nacp
$DEVKITPRO/tools/bin/elf2nro ReKHCoM.elf ReKHCoM.nro --icon=icon.jpg --nacp=ReKHCoM.nacp --romfsdir=romfs
ls -l ReKHCoM.nro
'''


def version():
    text = (ROOT / 'port/include/port/port_version.h').read_text(encoding='utf-8')
    return re.search(r'#define REKHCOM_VERSION\s+"v([^"]+)"', text).group(1)


def build_host(regions):
    """The .nro: the libnx host with the game images, fonts and notices in its romfs. Compiled in the consoles VM
    (port/progress/TOOLKIT_CONSOLAS.md), which has devkitPro in /opt/devkitpro."""
    from PIL import Image
    share = BUILD / 'host-share'
    if share.exists():
        shutil.rmtree(share)
    assets = share / 'romfs/assets'
    assets.mkdir(parents=True)
    for region in regions:
        image = BUILD / f'guest-{region}.macho'
        if not image.is_file():
            sys.exit(f'{image} missing: build the guest first')
        shutil.copy2(image, assets / image.name)
    build_ps5.MESSAGES, build_ps5.TOASTS = MESSAGES, TOASTS
    build_ps5.render_messages(assets)
    build_ps5.render_toasts(assets)
    shutil.copytree(PS5 / 'assets/fonts', assets / 'fonts')
    (assets / 'achievements').mkdir()
    shutil.copy2(ROOT / 'port/assets/achievements/badges.png', assets / 'achievements/badges.png')
    (share / 'src').mkdir()
    for src in (HERE / 'host/switch_host.c', PS5 / 'shared/bridge.h', PS5 / 'shared/bridge_list.h', PS5 / 'shared/hd_music_host.h', PS5 / 'shared/udp_host.h',
                ROOT / 'port/third_party/stb/stb_vorbis.c'):
        shutil.copy2(src, share / 'src' / src.name)
    Image.open(PS5 / 'sce_sys/icon0.png').convert('RGB').resize((256, 256), Image.LANCZOS).save(share / 'icon.jpg', quality=92)
    (share / 'build.sh').write_bytes(VM_SCRIPT.replace('VERSION', version()).encode())
    out = BUILD / 'host-out'
    shutil.rmtree(out, ignore_errors=True)  # a .nro left by an earlier run must never pass for this one
    command = [sys.executable, ROOT / 'port/tools/linux_vm_test.py', '--share', share, '--disk', VM_DISK, '--fresh-share', '--mem', '4096',
               '--cpus', '4', '--run', 'bash /root/share/build.sh', '--fetch', '/root/share/ReKHCoM.nro', '--out', out]
    run(command, cwd=ROOT, env=dict(os.environ, MSYS_NO_PATHCONV='1', PYTHONUTF8='1'))
    nro = out / 'ReKHCoM.nro'
    if not nro.is_file():
        sys.exit('the VM did not return ReKHCoM.nro: see the log above')
    shutil.copy2(nro, BUILD / 'ReKHCoM.nro')
    print(f'host: {BUILD / "ReKHCoM.nro"} ({nro.stat().st_size} bytes)')


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('step', nargs='?', default='guest', choices=['guest', 'host', 'all'])
    p.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    p.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2))
    args = p.parse_args()
    if args.step in ('guest', 'all'):
        for region in args.regions:
            build_guest(region, args.jobs)
    if args.step in ('host', 'all'):
        build_host(args.regions)


if __name__ == '__main__':
    main()
