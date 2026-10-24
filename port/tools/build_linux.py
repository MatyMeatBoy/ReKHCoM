"""Cross-compile the Linux port from Windows (clang + lld, Debian sysroot).

The game core needs 32-bit pointers (ROM structures embed them), so Linux
builds are i386 (PCs, Steam Deck) or armhf. Prepare once:

    python port/tools/linux_sysroot.py --arch i386 --out D:/01Claude/ReKHCoM-build/linux/i386
    (then SDL3 and FreeType, see port/progress/LINUX.md)

    python port/tools/build_linux.py --arch i386 --versions eu us jp
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import ninja_syntax  # noqa: E402

# clang/lld/llvm-readelf from llvm-mingw and ninja from ../toolchain.
_TC = ROOT.parent / 'toolchain'
os.environ['PATH'] = os.pathsep.join([str(p / 'bin') for p in sorted(_TC.glob('llvm-mingw-*'))[-1:]]
                                     + [str(_TC / 'bin'), os.environ.get('PATH', '')])

ARCHES = {
    'i386': {'triple': 'i686-linux-gnu', 'flags': '-march=i686 -msse2 -mfpmath=sse'},
    'armhf': {'triple': 'arm-linux-gnueabihf', 'flags': '-march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb'},
}
# Windows-only platform files; Linux replacements live in port/linux and the
# POSIX memory backend shared with Android.
WINDOWS_ONLY = {'port_memory', 'port_sections', 'port_hdtext', 'port_watchdog'}


def rel(p):
    return os.path.relpath(p, ROOT).replace('\\', '/')


def sjis_table(path):
    rows = []
    for code in range(0x8140, 0xFD00):
        hi, lo = code >> 8, code & 0xFF
        if lo < 0x40 or lo == 0x7F or lo > 0xFC or not (0x81 <= hi <= 0x9F or 0xE0 <= hi <= 0xFC):
            continue
        try:
            ch = bytes([hi, lo]).decode('shift_jis')
        except UnicodeDecodeError:
            continue
        rows.append((code, ord(ch)))
    for code in range(0xA1, 0xE0):
        rows.append((code, ord(bytes([code]).decode('shift_jis'))))
    rows.sort()
    text = 'static const unsigned gLinuxSjis[][2] = {' + ','.join(f'{{{a},{b}}}' for a, b in rows) + '};\n'
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def symbols(src, dst):
    """Mach-O/PE style symbol table -> ELF: no leading underscore, .rodata."""
    text = src.read_text()
    text = text.replace('.section .rdata,"dr"', '.section .rodata')
    text = text.replace('.globl _', '.globl ').replace('.set _', '.set ')
    if not dst.exists() or dst.read_text() != text:
        dst.write_text(text)


def configure(version, arch, base, out):
    a = ARCHES[arch]
    sysroot = base / 'sysroot'
    sdl = base / 'sdl'
    ft = base / 'ft'
    gen = ROOT / 'build' / version / 'gen'
    glue = ROOT / 'build' / version / 'pc'
    if not (glue / 'rom_symbols.s').exists():
        subprocess.check_call([sys.executable, str(ROOT / 'port/tools/gen_pc_assets.py'), '--version', version],
                              env=dict(os.environ, PYTHONUTF8='1'))
    out.mkdir(parents=True, exist_ok=True)
    symbols(glue / 'rom_symbols.s', out / 'rom_symbols.s')
    symbols(ROOT / 'port/src/port_symbols.s', out / 'port_symbols.s')
    sjis_table(out / 'linux_sjis.h')

    target = f'--target={a["triple"]} --sysroot={sysroot.as_posix()} {a["flags"]}'
    include_dirs = ['include'] + sorted(rel(p) for p in (ROOT / 'include').iterdir() if p.is_dir() and p.name != 'gba')
    includes = ' '.join(f'-I{d}' for d in include_dirs + [rel(gen), 'port/include'])
    quoted = ' '.join(f'-iquote {d}' for d in include_dirs + [rel(gen)])
    common = (f'{target} -O2 -g -fno-pie -funsigned-char -fno-strict-aliasing -fwrapv -fno-common '
              f'-DVERSION_{version.upper()} -DPLATFORM_PC -DPORT_LINUX')
    game_cflags = f'{common} -std=gnu89 -w -include port/game_prefix.h {includes}'
    port_cflags = (f'{common} -std=gnu11 -Wall -Wno-unused-function -Wno-missing-braces {quoted} -Iport/include '
                   f'-I{rel(out)} -I{(sdl / "include").as_posix()} -I{(ft / "include/freetype2").as_posix()}')
    # Fixed image base above the GBA space (0x02000000-0x0E010000): quick
    # states hold raw pointers, as on Windows.
    ldflags = (f'{target} -fuse-ld=lld -no-pie -Wl,--image-base=0x10000000 -Wl,-z,nostart-stop-gc '
               # The bundled loader, relative to the game folder (the launcher
               # changes into it): run by the kernel as the interpreter, the
               # heap follows the executable instead of landing in the GBA
               # range (ARM64 kernels put a directly-run loader's brk there).
               f"-Wl,--dynamic-linker=lib/rekhcom -Wl,-rpath,'$$ORIGIN/../lib' -L{(sdl / 'lib').as_posix()} -lSDL3 {(ft / 'lib/libfreetype.a').as_posix()} "
               f'-lm -lpthread -ldl -Wl,--build-id')
    with open(out / 'build.ninja', 'w') as f:
        n = ninja_syntax.Writer(f)
        n.variable('builddir', rel(out))
        n.variable('cc', 'clang')
        n.variable('game_cflags', game_cflags)
        n.variable('port_cflags', port_cflags)
        n.variable('ldflags', ldflags)
        n.rule('cc_game', '$cc $game_cflags -MD -MF $out.d -c $in -o $out', description='CC $in', depfile='$out.d', deps='gcc')
        n.rule('cc_port', '$cc $port_cflags -MD -MF $out.d -c $in -o $out', description='CC $in', depfile='$out.d', deps='gcc')
        n.rule('as', f'$cc {target} -c $in -o $out', description='AS $in')
        n.rule('link', '$cc @$out.rsp -o $out $ldflags', rspfile='$out.rsp', rspfile_content='$in', description='LINK $out')
        objs = []
        # Game code first: the quick-state build id hashes it up to the platform layer.
        for src in sorted((ROOT / 'src').rglob('*.c')):
            obj = f'$builddir/game/{rel(src)[4:-2]}.o'
            n.build(obj, 'cc_game', rel(src))
            objs.append(obj)
        skip = WINDOWS_ONLY | ({'port_biosfault'} if arch != 'i386' else set())  # x86 decoder
        port_sources = [p for p in sorted((ROOT / 'port/src').glob('*.c')) if p.stem not in skip]
        port_sources += [ROOT / 'port/linux/platform_linux.c', ROOT / 'port/linux/fbdev_linux.c', ROOT / 'port/android/native/port_memory_android.c',
                         ROOT / 'port/portable/hdtext_freetype.c', glue / 'rom_relocs.c']
        for src in port_sources:
            obj = f'$builddir/port/{src.stem}.o'
            n.build(obj, 'cc_port', rel(src))
            objs.append(obj)
        for name in ('port_symbols', 'rom_symbols'):
            obj = f'$builddir/port/{name}_s.o'
            n.build(obj, 'as', f'$builddir/{name}.s')
            objs.append(obj)
        n.build(f'$builddir/khcom_{version}', 'link', objs)
        n.default(f'$builddir/khcom_{version}')


# Libraries SDL3 opens at run time (dlopen), besides the linked ones. GPU
# drivers are left out on purpose: they must match the system's kernel
# driver, so the bundle renders with SDL's software renderer.
DLOPEN = ['libX11.so.6', 'libXext.so.6', 'libXcursor.so.1', 'libXi.so.6', 'libXfixes.so.3', 'libXrandr.so.2',
          'libXss.so.1', 'libXrender.so.1', 'libXtst.so.6', 'libasound.so.2', 'libpulse.so.0', 'libudev.so.1',
          'libdbus-1.so.3', 'libdrm.so.2']
LIBDIRS = {'i386': 'i386-linux-gnu', 'armhf': 'arm-linux-gnueabihf'}
LOADER = {'i386': 'ld-linux.so.2', 'armhf': 'ld-linux-armhf.so.3'}


def needed(path):
    out = subprocess.check_output(['llvm-readelf', '-d', str(path)], text=True)
    return [m.split('[')[1].rstrip(']') for m in out.splitlines() if '(NEEDED)' in m]


def bundle(arch, base, versions, dest):
    """Self-contained folder: bin/, lib/ (loader, glibc, SDL3, client libs)."""
    import shutil
    sysroot = base / 'sysroot'
    triple = LIBDIRS[arch]
    search = [sysroot / 'lib' / triple, sysroot / 'usr/lib' / triple, sysroot / 'usr/lib' / triple / 'pulseaudio',
              base / 'sdl/lib']
    if dest.exists():
        shutil.rmtree(dest)
    (dest / 'bin').mkdir(parents=True)
    (dest / 'lib').mkdir()
    (dest / 'roms').mkdir()
    todo = list(DLOPEN) + ['libSDL3.so.0', LOADER[arch]]
    for version in versions:
        exe = ROOT / 'build' / f'linux-{arch}-{version}' / f'khcom_{version}'
        subprocess.check_call(['llvm-strip', '--strip-debug', '-o', str(dest / 'bin' / exe.name), str(exe)])
        todo += needed(exe)
    done = set()
    while todo:
        name = todo.pop()
        if name in done:
            continue
        src = next((d / name for d in search if (d / name).is_file()), None)
        if src is None:
            raise SystemExit(f'missing runtime library {name}')
        done.add(name)
        shutil.copyfile(src, dest / 'lib' / name)
        todo += needed(src)
    subprocess.check_call(['llvm-strip', '--strip-unneeded', str(dest / 'lib' / 'libSDL3.so.0')])
    # SDL3 opens these by their unversioned names (libasound.so, libX11.so ...),
    # which runtime packages do not ship. Without a copy under that name the
    # system's library is loaded instead and may need a newer glibc than the
    # bundled one (issue #19: libasound.so wanting GLIBC_2.43).
    for name in DLOPEN:
        plain = name.split('.so')[0] + '.so'
        if plain != name and (dest / 'lib' / name).is_file():
            shutil.copyfile(dest / 'lib' / name, dest / 'lib' / plain)
    # The loader runs the game, so the process is named after its file.
    (dest / 'lib' / LOADER[arch]).rename(dest / 'lib' / 'rekhcom')
    (dest / 'ReKHCoM.sh').write_bytes((ROOT / 'port/linux/ReKHCoM.sh').read_bytes().replace(b'\r\n', b'\n'))
    for name in ('KHMenu.otf', 'ComicHearts.otf', 'NOTICE.md'):
        (dest / 'assets/fonts').mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / 'port/assets/fonts' / name, dest / 'assets/fonts' / name)
    (dest / 'assets/achievements').mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / 'port/assets/achievements/badges.png', dest / 'assets/achievements/badges.png')
    # Window icon (set by the game) and a launcher entry for desktops/Steam Deck.
    shutil.copyfile(ROOT / 'port/assets/icon.bmp', dest / 'assets/icon.bmp')
    shutil.copyfile(ROOT / 'port/assets/icon.png', dest / 'ReKHCoM.png')
    desktop = ['[Desktop Entry]', 'Type=Application', 'Name=ReKHCoM',
               'Comment=Kingdom Hearts: Chain of Memories (PC port)', 'Exec=sh ReKHCoM.sh',
               'Icon=ReKHCoM.png', 'Terminal=false', 'Categories=Game;']
    (dest / 'ReKHCoM.desktop').write_bytes((chr(10).join(desktop) + chr(10)).encode())
    for name in ('README.md', 'CHANGELOG.md', 'LICENSE.md'):
        shutil.copyfile(ROOT / name, dest / name)
    (dest / 'licenses').mkdir()
    shutil.copyfile(base / 'sdl-src-LICENSE.txt', dest / 'licenses/SDL3-LICENSE.txt')
    (dest / 'licenses/RUNTIME.md').write_text(
        'lib/ holds unmodified Debian 12 (bookworm) ' + arch + ' runtime libraries: GNU C Library (LGPL-2.1+),\n'
        'libX11 and friends (MIT/X11), ALSA lib and PulseAudio client (LGPL-2.1+), systemd libudev (LGPL-2.1+),\n'
        'D-Bus (AFL-2.1/GPL-2+), libdrm (MIT). Sources: https://sources.debian.org/ (packages listed in\n'
        'packages.txt). SDL3 is zlib-licensed (SDL3-LICENSE.txt); FreeType is linked statically (FTL).\n')
    shutil.copyfile(base / 'packages.txt', dest / 'licenses/packages.txt')
    return sorted(done)


def executable(name):
    return name.endswith('.sh') or '/bin/' in name or '/lib/' in name


def make_tar(src, archive):
    """No versioned top folder: an update extracts over the old install."""
    import tarfile
    with tarfile.open(archive, 'w:gz', compresslevel=9) as t:
        for f in sorted(src.rglob('*')):
            name = f.relative_to(src).as_posix()
            info = t.gettarinfo(f, name)
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mode = 0o755 if f.is_dir() or executable('/' + name) else 0o644
            if f.is_file():
                with open(f, 'rb') as fh:
                    t.addfile(info, fh)
            else:
                t.addfile(info)


def make_zip(src, archive):
    import zipfile
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for f in sorted(src.rglob('*')):
            name = f.relative_to(src).as_posix()
            if f.is_dir():
                continue
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = ((0o100755 if executable('/' + name) else 0o100644) << 16)
            info.create_system = 3  # Unix: keep the permission bits
            z.writestr(info, f.read_bytes())


def package(version, base_i386, base_armhf):
    """One file per Linux platform in build/releases."""
    import shutil
    import re
    out = ROOT / 'build/releases'
    out.mkdir(parents=True, exist_ok=True)
    stage = ROOT / 'build/linux-stage'
    if stage.exists():
        shutil.rmtree(stage)
    cfg = re.search(r'PORT_ZOOM_CONFIG_VERSION\s+(\d+)', (ROOT / 'port/include/port/port_zoom.h').read_text()).group(1)
    made = []
    # Linux PC: self-contained x86 folder.
    pc = stage / 'pc'
    bundle('i386', base_i386, ['eu', 'us', 'jp'], pc)
    shutil.copyfile(ROOT / 'port/linux/LINUX.md', pc / 'LINUX.md')
    archive = out / f'ReKHCoM-{version}-linux-x86.tar.gz'
    make_tar(pc, archive)
    made.append(archive)
    # Steam Deck: the same build, fullscreen from the first launch.
    deck = stage / 'deck'
    shutil.copytree(pc, deck)
    (deck / 'kh_com.default.ini').write_text(f'# Steam Deck preset\nconfig_version={cfg}\nfullscreen=1\n')
    archive = out / f'ReKHCoM-{version}-steamdeck.tar.gz'
    make_tar(deck, archive)
    made.append(archive)
    # Anbernic (PortMaster layout): launcher + rekhcom/ with the armhf build.
    arm = stage / 'anbernic'
    bundle('armhf', base_armhf, ['eu', 'us', 'jp'], arm / 'rekhcom')
    shutil.copyfile(ROOT / 'port/linux/LINUX.md', arm / 'rekhcom' / 'LINUX.md')
    (arm / 'ReKHCoM.sh').write_bytes((ROOT / 'port/linux/anbernic/ReKHCoM.sh').read_bytes().replace(b'\r\n', b'\n'))
    archive = out / f'ReKHCoM-{version}-anbernic-alpha.zip'
    make_zip(arm, archive)
    made.append(archive)
    for a in made:
        print('package', rel(a), a.stat().st_size)
    return made


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--arch', choices=ARCHES, default='i386')
    ap.add_argument('--versions', nargs='+', default=['eu', 'us', 'jp'])
    ap.add_argument('--base', type=Path, help='prepared sysroot/SDL/FreeType folder (default D:/01Claude/ReKHCoM-build/linux/<arch>)')
    ap.add_argument('--jobs', type=int, default=os.cpu_count())
    ap.add_argument('--bundle', type=Path, help='also assemble a self-contained folder here')
    ap.add_argument('--package', metavar='VERSION', help='build i386 and armhf, all regions, and write '
                    'build/releases/ReKHCoM-VERSION-{linux-x86.tar.gz,steamdeck.tar.gz,anbernic-alpha.zip}')
    args = ap.parse_args()
    if args.package:
        bases = {a: Path('D:/01Claude/ReKHCoM-build/linux') / a for a in ARCHES}
        for arch in ARCHES:
            for version in ('eu', 'us', 'jp'):
                out = ROOT / 'build' / f'linux-{arch}-{version}'
                configure(version, arch, bases[arch], out)
                subprocess.check_call(['ninja', '-f', str(out / 'build.ninja'), '-j', str(args.jobs)], cwd=ROOT)
        package(args.package, bases['i386'], bases['armhf'])
        return
    base = args.base or Path('D:/01Claude/ReKHCoM-build/linux') / args.arch
    for version in args.versions:
        out = ROOT / 'build' / f'linux-{args.arch}-{version}'
        configure(version, args.arch, base, out)
        subprocess.check_call(['ninja', '-f', str(out / 'build.ninja'), '-j', str(args.jobs)], cwd=ROOT)
        print('built', rel(out / f'khcom_{version}'))
    if args.bundle:
        libs = bundle(args.arch, base, args.versions, args.bundle)
        print('bundled', args.bundle, len(libs), 'libraries')


if __name__ == '__main__':
    main()
