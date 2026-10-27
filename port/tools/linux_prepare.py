"""One-time preparation of the Linux cross builds on Windows.

For each architecture (i386, amd64, armhf, arm64): Debian sysroot, a pkg-config stand-in,
a CMake toolchain file, then SDL3 (shared, X11/KMSDRM/ALSA/PulseAudio/
PipeWire loaded at run time, no Wayland) and FreeType (static).

    python port/tools/linux_prepare.py --arch arm64 --jobs 1
    python port/tools/linux_prepare.py --arch i386

Needs ../toolchain (llvm-mingw, ninja, SDL3 and FreeType sources) and the
Android SDK's CMake under ../toolchain/android/sdk/cmake.
"""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from workdir import WORK_DIR  # noqa: E402  the heavy work folder (REKHCOM_WORK)

ROOT = Path(__file__).resolve().parents[2]
TC = next((p / 'toolchain' for p in ROOT.parents if (p / 'toolchain').is_dir()), ROOT.parent / 'toolchain')
BASE = (WORK_DIR / 'linux')
TRIPLES = {'i386': ('i686', 'i686-linux-gnu', ''),
           'amd64': ('x86_64', 'x86_64-linux-gnu', ''),  # the 64-bit host (port/linux64, 26/10)
           'arm64': ('aarch64', 'aarch64-linux-gnu', ''),
           'armhf': ('armv7', 'arm-linux-gnueabihf', '-march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb')}

PKGCONFIG = r'''"""Minimal pkg-config for cross builds: reads .pc files under PKG_SYSROOT."""
import os, re, sys
root = os.environ['PKG_SYSROOT']
dirs = [os.path.join(root, d) for d in ('usr/lib/aarch64-linux-gnu/pkgconfig', 'usr/lib/x86_64-linux-gnu/pkgconfig', 'usr/lib/i386-linux-gnu/pkgconfig', 'usr/lib/arm-linux-gnueabihf/pkgconfig',
        'usr/share/pkgconfig', 'usr/lib/pkgconfig')]
def load(name):
    for d in dirs:
        p = os.path.join(d, name + '.pc')
        if os.path.exists(p):
            break
    else:
        return None
    v, f = {'pc_sysrootdir': root}, {}
    for line in open(p, encoding='utf-8', errors='replace'):
        line = line.strip()
        if not line or line.startswith('#'): continue
        m = re.match(r'^([A-Za-z0-9_.]+)\s*=\s*(.*)$', line)
        if m: v[m.group(1)] = m.group(2); continue
        m = re.match(r'^([A-Za-z.]+):\s*(.*)$', line)
        if m: f[m.group(1)] = m.group(2)
    def sub(s):
        for _ in range(10): s = re.sub(r'\$\{(\w+)\}', lambda m: v.get(m.group(1), ''), s)
        return s
    for k in list(v): v[k] = sub(v[k])
    return {k: sub(x) for k, x in f.items()}
def reqs(s):
    return [r for r in re.split(r'[,\s]+', re.sub(r'[<>=]+\s*[\w.~-]+', '', s or '')) if r]
args = sys.argv[1:]
mods = []
for a in args:
    if a.startswith('-'): continue
    for part in re.split(r'[<>=!]+', a):
        for name in part.split():
            if not re.match(r'^[\d.]+$', name): mods.append(name)
if any(load(m) is None for m in mods): sys.exit(1)
if '--exists' in args: sys.exit(0)
if '--modversion' in args:
    for m in mods: print(load(m).get('Version', '0'))
    sys.exit(0)
out, seen = [], set()
def walk(m):
    if m in seen: return
    seen.add(m); p = load(m)
    if not p: return
    if any(a.startswith('--cflags') for a in args): out.extend(p.get('Cflags', '').split())
    if any(a.startswith('--libs') for a in args): out.extend(p.get('Libs', '').split())
    for r in reqs(p.get('Requires')): walk(r)
for m in mods: walk(m)
res = []
for t in out:
    if t.startswith('-I/') or t.startswith('-L/'): t = t[:2] + root + t[2:]
    if '--cflags-only-I' in args and not t.startswith('-I'): continue
    if '--cflags-only-other' in args and t.startswith('-I'): continue
    if '--libs-only-l' in args and not t.startswith('-l'): continue
    if '--libs-only-L' in args and not t.startswith('-L'): continue
    if '--libs-only-other' in args and t.startswith(('-l', '-L')): continue
    if t not in res: res.append(t)
print(' '.join(res))
'''


def cmake():
    hits = sorted((TC / 'android/sdk/cmake').glob('*/bin/cmake.exe'))
    if not hits:
        raise SystemExit('CMake not found under toolchain/android/sdk/cmake')
    return str(hits[-1])


def prepare(arch, env, jobs=None):
    proc, triple, flags = TRIPLES[arch]
    base = BASE / arch
    tools = BASE / 'tools'
    tools.mkdir(parents=True, exist_ok=True)
    # An ABI-only sysroot may already exist without the SDL development packages.
    if not (base / 'sysroot/usr/include/X11/Xlib.h').exists():
        subprocess.check_call([sys.executable, str(ROOT / 'port/tools/linux_sysroot.py'), '--arch', arch, '--out', str(base)])
    (tools / 'pkgconfig.py').write_text(PKGCONFIG)
    (tools / 'pkg-config.bat').write_text('@echo off\r\npython -I "%~dp0pkgconfig.py" %*\r\n')
    sysroot = (base / 'sysroot').as_posix()
    toolchain = tools / f'{arch}-linux.cmake'
    toolchain.write_text(f'''set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR {proc})
set(CMAKE_SYSROOT {sysroot})
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET {triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET {triple})
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_ASM_COMPILER_TARGET {triple})
set(CMAKE_C_FLAGS_INIT "{flags}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_AR llvm-ar)
set(CMAKE_RANLIB llvm-ranlib)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(PKG_CONFIG_EXECUTABLE {(tools / "pkg-config.bat").as_posix()} CACHE FILEPATH "")
''')
    env = dict(env, PKG_SYSROOT=sysroot)
    sdl_src = sorted((TC / 'android').glob('SDL3-*'))
    sdl_src = [p for p in sdl_src if p.is_dir()][-1]
    ft_src = sorted(p for p in TC.glob('freetype-*') if p.is_dir())[-1]
    for name, src, opts in (
            ('sdl', sdl_src, ['-DSDL_SHARED=ON', '-DSDL_STATIC=OFF', '-DSDL_TESTS=OFF', '-DSDL_EXAMPLES=OFF',
                              '-DSDL_WAYLAND=OFF', '-DSDL_X11=ON', '-DSDL_KMSDRM=ON', '-DSDL_ALSA=ON',
                              '-DSDL_PULSEAUDIO=ON', '-DSDL_PIPEWIRE=ON', '-DSDL_JACK=OFF', '-DSDL_SNDIO=OFF',
                              '-DSDL_IBUS=OFF', '-DSDL_HIDAPI_LIBUSB=OFF', '-DSDL_DEPS_SHARED=ON', '-DSDL_VULKAN=OFF',
                              '-DSDL_OPENVR=OFF', '-DSDL_CAMERA=OFF']),
            ('ft', ft_src, ['-DCMAKE_POSITION_INDEPENDENT_CODE=ON', '-DBUILD_SHARED_LIBS=OFF', '-DFT_DISABLE_ZLIB=ON',
                            '-DFT_DISABLE_BZIP2=ON', '-DFT_DISABLE_PNG=ON', '-DFT_DISABLE_HARFBUZZ=ON',
                            '-DFT_DISABLE_BROTLI=ON'])):
        build = base / f'{name}-build'
        subprocess.check_call([cmake(), '-G', 'Ninja', '-S', str(src), '-B', str(build),
                               f'-DCMAKE_TOOLCHAIN_FILE={toolchain.as_posix()}', '-DCMAKE_BUILD_TYPE=Release',
                               f'-DCMAKE_INSTALL_PREFIX={(base / name).as_posix()}', *opts], env=env)
        subprocess.check_call([cmake(), '--build', str(build), '-j', str(jobs or os.cpu_count() or 1)], env=env)
        subprocess.check_call([cmake(), '--install', str(build)], env=env)
    shutil.copyfile(sdl_src / 'LICENSE.txt', base / 'sdl-src-LICENSE.txt')
    print('prepared', base)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--arch', choices=TRIPLES, action='append')
    ap.add_argument('--jobs', type=int, default=None, help='maximum parallel compilation jobs')
    args = ap.parse_args()
    if args.jobs is not None and args.jobs < 1:
        ap.error('--jobs must be positive')
    env = dict(os.environ)
    env['PATH'] = os.pathsep.join([str(p / 'bin') for p in sorted(TC.glob('llvm-mingw-*'))[-1:]]
                                  + [str(TC / 'bin'), env.get('PATH', '')])
    # Experimental ARM64 is opt-in; existing default preparation remains unchanged.
    for arch in args.arch or [name for name in TRIPLES if name != 'arm64']:
        prepare(arch, env, args.jobs)


if __name__ == '__main__':
    main()
