#!/usr/bin/env python3
"""Build the PS Vita alpha: ReKHCoM.vpk.

The Vita is a 32-bit ARM console, so the game and the port layer run as ordinary code of the process, but its user
memory starts at 0x81000000: the GBA's low addresses cannot be mapped. So, as on the Switch (port/switch), the GBA
space is relocatable (PORT_GBA_RELOC, include/gba/gba_space.h) and the game talks to the console through the same
bridge (port/ps5/shared/bridge_list.h), served by the same host source (port/switch/host/switch_host.c, __vita__):

- guest: the game and the port compiled here by clang for the Cortex-A9 (ARMv7, Thumb-2, hard float), one archive
  per region (build/vita/guest-<region>.a);
- host and package, in the consoles VM (VitaSDK in /root/consoles/vitasdk, port/progress/TOOLKIT_CONSOLAS.md): each
  archive becomes one object whose symbols are local except guest_main and gHostTable (the game keeps its own libc),
  linked with the host into one executable per region: eboot.bin (Europe), eboot_us.self, eboot_jp.self. The one
  that starts hands over to the one that matches the cartridge (ux0:data/ReKHCoM/*.gba).

    python port/vita/build_vita.py            # guest + package: build/vita/ReKHCoM.vpk
    python port/vita/build_vita.py guest      # only the archives

Needs the generated assets of each region (build/<region>) and the ROM pointer lists (port/data/rom_pointers_*.inc).
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
PS5 = ROOT / 'port/ps5'
SWITCH = ROOT / 'port/switch'
BUILD = ROOT / 'build/vita'
TARGET = ['--target=armv7a-none-eabihf', '-mcpu=cortex-a9', '-mfpu=neon', '-mthumb']
TITLE_ID = 'RKHC00001'
sys.path.insert(0, str(PS5))
sys.path.insert(0, str(SWITCH))
import build_ps5  # noqa: E402  (PORT_PATCHES, PORT_EXCLUDED, the notices)
import build_switch  # noqa: E402  (compile_all, the toolchain, the VM)

CLANG = build_switch.CLANG
AR = str(Path(CLANG).with_name('llvm-ar' + ('.exe' if build_switch.WINDOWS else '')))
GUEST_SKIP = {'crt0', 'stubs'}  # x86-64 assembly: port/vita/guest has the ARM ones

MESSAGES = {
    'need-rom': ('A compatible ROM is required',
                 ['Copy your Kingdom Hearts: Chain of Memories cartridge dump',
                  '(Europe, USA or Japan, a .gba file) to the folder',
                  'ux0:data/ReKHCoM/',
                  'The game starts by itself as soon as the file is there.']),
    'load-error': ('The game could not be started',
                   ['Something failed while loading the game.',
                    'The reason is written to ux0:data/ReKHCoM/logs/ReKHCoM.log']),
}
TOASTS = {
    'toast-states': ['Select + L: save state', 'Select + R: load state'],
    'toast-update-found': ['A new version is available', 'Downloading it now'],
    'toast-update-done': ['ReKHCoM was updated', 'Restart the app to use the new version'],
}


def common_flags():
    resource = subprocess.check_output([CLANG, '-print-resource-dir'], text=True).strip() + '/include'
    return [*TARGET, '-ffreestanding', '-nostdinc', '-isystem', resource, f'-I{PS5 / "guest/include"}', f'-I{PS5 / "shared"}',
            '-O2', '-funsigned-char', '-fno-strict-aliasing', '-fwrapv', '-fno-stack-protector', '-fno-common',
            '-fno-short-enums', '-fno-builtin-printf', '-fno-math-errno', '-DPORT_GBA_RELOC', '-DPORT_VITA']


def elf_symbols(source, out):
    """rom_symbols_reloc.s (COFF syntax for the Windows check build) as ELF: no leading underscore, the regions as
    zero-filled space."""
    text = source.read_text()
    text = text.replace('.section .bss$gbaspace,"bw"', '.section .bss.gbaspace,"aw",%nobits')
    text = re.sub(r'(?<![\w.$])_(\w+)', r'\1', text)
    out.write_text(text)


def build_guest(region, jobs):
    upper = region.upper()
    pc = ROOT / 'build' / region / 'pc'
    if not (pc / 'rom_symbols_reloc.s').is_file():
        build_switch.run([sys.executable, ROOT / 'port/tools/gen_pc_assets.py', '--version', region], cwd=ROOT,
                         env=dict(os.environ, PYTHONUTF8='1'))
    out = BUILD / f'guest-{region}'
    out.mkdir(parents=True, exist_ok=True)
    includes = build_ps5.game_includes(region)
    game_flags = [*common_flags(), '-w', '-std=gnu89', '-include', ROOT / 'port/include/port/game_prefix.h', '-DPLATFORM_PC',
                  f'-DVERSION_{upper}', *includes]
    port_flags = [*common_flags(), '-w', '-std=gnu11', '-DSTBI_NO_THREAD_LOCALS', f'-I{PS5 / "third_party/stb"}',
                  f'-I{ROOT / "port/src"}', '-DPLATFORM_PC', f'-DVERSION_{upper}', *includes]
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
    units.append((HERE / 'guest/stubs_arm.c', out / 'vita/stubs_arm.o', port_flags))
    elf_symbols(pc / 'rom_symbols_reloc.s', out / 'rom_symbols.s')
    for src in (out / 'rom_symbols.s', HERE / 'guest/crt0_arm.S'):
        units.append((src, out / 'asm' / (src.stem + '.o'), [*TARGET, '-c']))
    build_switch.compile_all(jobs, units)
    archive = BUILD / f'guest-{region}.a'
    archive.unlink(missing_ok=True)
    build_switch.run([AR, 'rcs', archive, *[u[1] for u in units]])
    print(f'guest {region}: {archive} ({archive.stat().st_size} bytes)')
    return archive


# The partial link: the guest's text and its game globals (port/game_prefix.h) get the bounds the port reads
# (port_sections_ps5.c, state_ps5.c).
GUEST_LD = '''SECTIONS {
    .text : { __text_start = .; *(.text .text.*) __text_end = .; }
    khdata : { __start_khdata = .; *(khdata) __stop_khdata = .; }
    khbss (NOLOAD) : { __start_khbss = .; *(khbss) __stop_khbss = .; }
}
'''

VM_SCRIPT = r'''set -e
export VITASDK=/root/consoles/vitasdk PATH=/root/consoles/vitasdk/bin:$PATH
cd /root/share
FLAGS="-mcpu=cortex-a9 -mfpu=neon -mthumb -O2 -DREKHCOM_VITA_GPU"
# the picture on the GPU: SDL2's GXM renderer (VitaSDK's own package, installed once into the VM's SDK)
# a run killed while installing leaves the package database locked
[ -f $VITASDK/arm-vita-eabi/lib/libSDL2.a ] || { find $VITASDK -name db.lck -delete; yes | vdpm sdl2; }
[ -f $VITASDK/arm-vita-eabi/lib/libSDL2.a ]
LIBS="-lSDL2 -lSceGxm_stub -lSceDisplay_stub -lSceCtrl_stub -lSceAudio_stub -lSceAudioIn_stub -lScePower_stub
 -lSceAppMgr_stub -lSceAppUtil_stub -lSceSysmem_stub -lSceSysmodule_stub -lSceIme_stub -lSceHid_stub -lSceTouch_stub
 -lSceMotion_stub -lSceCommonDialog_stub -lSceKernelDmacMgr_stub -lm"
LIBS=$(echo $LIBS)
for r in REGIONS; do
  arm-vita-eabi-ld -r -T guest.ld --whole-archive guest-$r.a -o guest-$r-r.o
  arm-vita-eabi-objcopy -G guest_main -G gHostTable guest-$r-r.o guest-$r.o
  arm-vita-eabi-gcc $FLAGS -Wall -DREKHCOM_VITA_REGION=\"$r\" -Isrc -c src/switch_host.c -o host-$r.o
  arm-vita-eabi-gcc $FLAGS -Wl,-q host-$r.o guest-$r.o $LIBS -o eboot-$r.elf
  # vita-elf-create appends its SCE data to the code segment: when the code ends right below the data segment there
  # is no room ("segment 1 overlaps"); 8 KiB of padding moves the end and leaves it
  if ! vita-elf-create eboot-$r.elf eboot-$r.velf; then
    echo 'const unsigned char rekhcom_vita_pad[8192] __attribute__((used)) = { 1 };' > pad.c
    arm-vita-eabi-gcc $FLAGS -c pad.c -o pad.o
    arm-vita-eabi-gcc $FLAGS -Wl,-q host-$r.o guest-$r.o pad.o $LIBS -o eboot-$r.elf
    vita-elf-create eboot-$r.elf eboot-$r.velf
  fi
  vita-make-fself -s -c eboot-$r.velf eboot-$r.self
done
mv eboot-eu.self eboot.bin
vita-mksfoex -s TITLE_ID=TITLEID -d ATTRIBUTE2=12 "ReKHCoM" param.sfo
ASSETS=$(cd pkg && find . -type f | sed 's|^\./||' | while read f; do printf -- '-a pkg/%s=%s ' "$f" "$f"; done)
SELFS=$(for r in REGIONS; do [ $r = eu ] || printf -- '-a eboot-%s.self=eboot_%s.self ' $r $r; done)
vita-pack-vpk -s param.sfo -b eboot.bin $SELFS $ASSETS ReKHCoM.vpk
ls -l ReKHCoM.vpk
'''


def livearea(sce_sys):
    """The icon and the LiveArea screen, from the PS5 package art (8-bit palette PNGs, as the Vita wants them)."""
    from PIL import Image
    art = Image.open(PS5 / 'sce_sys/icon0.png').convert('RGB')
    back = Image.open(PS5 / 'sce_sys/back.png').convert('RGB') if (PS5 / 'sce_sys/back.png').is_file() else art
    contents = sce_sys / 'livearea/contents'
    contents.mkdir(parents=True)

    def save(image, size, path):
        image.resize(size, Image.LANCZOS).quantize(256).save(path)

    save(art, (128, 128), sce_sys / 'icon0.png')
    save(back, (840, 500), contents / 'bg.png')
    save(art, (280, 158), contents / 'startup.png')
    (contents / 'template.xml').write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n<livearea style="a1" format-ver="01.00" content-rev="1">\n'
        '  <livearea-background><image>bg.png</image></livearea-background>\n'
        '  <gate><startup-image>startup.png</startup-image></gate>\n</livearea>\n')


def package(regions):
    share = BUILD / 'vm-share'
    if share.exists():
        shutil.rmtree(share)
    pkg = share / 'pkg'
    assets = pkg / 'assets'
    assets.mkdir(parents=True)
    for region in regions:
        shutil.copy2(BUILD / f'guest-{region}.a', share / f'guest-{region}.a')
    build_ps5.MESSAGES, build_ps5.TOASTS = MESSAGES, TOASTS
    build_ps5.render_messages(assets)
    build_ps5.render_toasts(assets)
    shutil.copytree(PS5 / 'assets/fonts', assets / 'fonts')
    (assets / 'achievements').mkdir()
    shutil.copy2(ROOT / 'port/assets/achievements/badges.png', assets / 'achievements/badges.png')
    livearea(pkg / 'sce_sys')
    (share / 'src').mkdir()
    for src in (SWITCH / 'host/switch_host.c', PS5 / 'shared/bridge.h', PS5 / 'shared/bridge_list.h', PS5 / 'shared/hd_music_host.h', PS5 / 'shared/udp_host.h',
                ROOT / 'port/third_party/stb/stb_vorbis.c'):
        shutil.copy2(src, share / 'src' / src.name)
    (share / 'guest.ld').write_text(GUEST_LD)
    script = VM_SCRIPT.replace('REGIONS', ' '.join(regions)).replace('TITLEID', TITLE_ID)
    (share / 'build.sh').write_bytes(script.encode())
    out = BUILD / 'vm-out'
    shutil.rmtree(out, ignore_errors=True)  # a package left by an earlier run must never pass for this one
    command = [sys.executable, ROOT / 'port/tools/linux_vm_test.py', '--share', share, '--disk', build_switch.VM_DISK, '--fresh-share',
               '--net', '--mem', '4096', '--cpus', '4', '--run', 'bash /root/share/build.sh', '--run-timeout', '1500',
               '--fetch', '/root/share/ReKHCoM.vpk', '--out', out]
    build_switch.run(command, cwd=ROOT, env=dict(os.environ, MSYS_NO_PATHCONV='1', PYTHONUTF8='1'))
    vpk = out / 'ReKHCoM.vpk'
    if not vpk.is_file():
        sys.exit('the VM did not return ReKHCoM.vpk: see the log above')
    shutil.copy2(vpk, BUILD / 'ReKHCoM.vpk')
    print(f'package: {BUILD / "ReKHCoM.vpk"} ({vpk.stat().st_size} bytes)')


TEST_SCRIPT = r'''cd /root/share
command -v arm-linux-gnueabihf-gcc >/dev/null || (apt-get update -qq &&
  DEBIAN_FRONTEND=noninteractive apt-get install -y -qq gcc-arm-linux-gnueabihf libc6-dev-armhf-cross >/dev/null)
arm-linux-gnueabihf-ld -r -T guest.ld --whole-archive guest-@REGION@.a -o guest-r.o 2>/dev/null
arm-linux-gnueabihf-objcopy -G guest_main -G gHostTable guest-r.o guest.o
arm-linux-gnueabihf-gcc -O2 -g -mfpu=neon -static -DREKHCOM_LINKED_GUEST -DREKHCOM_VITA_REGION=\"@REGION@\" -Isrc \
  src/switch_host.c guest.o -lm -o vita_test 2>&1 | grep -v "enums\|GNU-stack" || true
mkdir -p /root/d /root/shots && cp data/*.gba /root/d/
start=$(date +%s)
@ENV@ timeout 3000 ./vita_test --data /root/d --assets /root/share/assets @SHOTS@ > /root/run.log 2>&1
echo rc=$? seconds=$(($(date +%s)-start))
grep -v "^\[ReKHCoM\] $" /root/run.log | tail -30
cd /root && tar -cf shots.tar shots
'''


def test(region, env, shots):
    """The Vita's game image on 32-bit ARM Linux (the ARM64 VM runs it): the host's test mode, headless, with the
    game's harness variables (env) and screenshots at the given frames (build/vita/test-out/shots.tar)."""
    share = BUILD / 'test-share'
    if share.exists():
        shutil.rmtree(share)
    (share / 'assets').mkdir(parents=True)
    (share / 'data').mkdir()
    (share / 'src').mkdir()
    shutil.copy2(BUILD / f'guest-{region}.a', share)
    shutil.copy2(ROOT / 'roms' / ({'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}[region] + '.gba'), share / 'data')
    shutil.copytree(PS5 / 'assets/fonts', share / 'assets/fonts')
    for src in (SWITCH / 'host/switch_host.c', PS5 / 'shared/bridge.h', PS5 / 'shared/bridge_list.h', PS5 / 'shared/hd_music_host.h', PS5 / 'shared/udp_host.h',
                ROOT / 'port/third_party/stb/stb_vorbis.c'):
        shutil.copy2(src, share / 'src' / src.name)
    (share / 'guest.ld').write_text(GUEST_LD)
    script = (TEST_SCRIPT.replace('@REGION@', region).replace('@ENV@', ' '.join(f"{k}='{v}'" for k, v in env.items()))
              .replace('@SHOTS@', ' '.join(f'--shot {f} /root/shots/f{f}.ppm' for f in shots)))
    (share / 'run.sh').write_bytes(script.encode())
    out = BUILD / 'test-out'
    command = [sys.executable, ROOT / 'port/tools/linux_vm_test.py', '--machine', 'aarch64', '--net', '--share', share,
               '--run', 'bash /root/share/run.sh', '--run-timeout', '3200', '--fetch', '/root/run.log',
               '--fetch', '/root/shots.tar', '--out', out, '--mem', '3072', '--cpus', '4']
    build_switch.run(command, cwd=ROOT, env=dict(os.environ, MSYS_NO_PATHCONV='1', PYTHONUTF8='1'))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('step', nargs='?', default='all', choices=['guest', 'package', 'all', 'test'])
    p.add_argument('--env', action='append', default=[], metavar='NAME=VALUE', help='test: harness variables')
    p.add_argument('--shots', type=int, nargs='*', default=[700], help='test: frames to save')
    p.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    p.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2))
    args = p.parse_args()
    if args.step == 'test':
        test(args.regions[0], dict(e.split('=', 1) for e in args.env), args.shots)
        return
    if 'eu' not in args.regions:
        sys.exit('the European executable is eboot.bin: --regions must include eu')
    if args.step in ('guest', 'all'):
        for region in args.regions:
            build_guest(region, args.jobs)
    if args.step in ('package', 'all'):
        package(args.regions)


if __name__ == '__main__':
    main()
