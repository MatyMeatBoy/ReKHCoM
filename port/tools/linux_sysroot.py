"""Build a Debian sysroot for cross-compiling the Linux port from Windows.

Downloads the Packages index of one Debian release/architecture, resolves the
dependencies of the requested packages and extracts their data archives into
<out>/sysroot. Nothing is executed from the packages.

    python port/tools/linux_sysroot.py --arch i386 --out D:/01Claude/ReKHCoM-build/linux/i386
"""
import argparse
import hashlib
import io
import lzma
import re
import tarfile
import urllib.request
from pathlib import Path

MIRROR = 'https://deb.debian.org/debian'

# Headers and link libraries for the game and SDL3 (X11, ALSA, PulseAudio,
# PipeWire, udev, D-Bus, DRM/GBM/EGL headers for the KMS backend).
DEV = [
    'libc6-dev', 'linux-libc-dev', 'libgcc-12-dev',
    'libx11-dev', 'libxext-dev', 'libxrandr-dev', 'libxrender-dev', 'libxcursor-dev',
    'libxi-dev', 'libxfixes-dev', 'libxss-dev', 'libxtst-dev', 'libxkbcommon-dev',
    'libasound2-dev', 'libpulse-dev', 'libpipewire-0.3-dev', 'libudev-dev',
    'libdbus-1-dev', 'libdrm-dev', 'libgbm-dev', 'libegl-dev', 'libgles-dev', 'libgl-dev',
]

SKIP = {'debconf', 'debconf-2.0', 'dpkg', 'perl-base', 'install-info', 'init-system-helpers',
        'adduser', 'lsb-base', 'sysvinit-utils', 'cdebconf', 'libpam-runtime', 'passwd',
        'x11-common', 'pkg-config', 'pkgconf', 'python3', 'mesa-common-dev'}


def fetch(url):
    with urllib.request.urlopen(url) as r:
        return r.read()


def parse_packages(text):
    pkgs, provides = {}, {}
    for block in text.split('\n\n'):
        fields, last = {}, None
        for line in block.split('\n'):
            if line.startswith(' ') and last:
                fields[last] += '\n' + line
            elif ':' in line:
                last, value = line.split(':', 1)
                fields[last] = value.strip()
        if 'Package' in fields:
            pkgs[fields['Package']] = fields
            for p in fields.get('Provides', '').split(','):
                name = p.strip().split(' ')[0]
                if name:
                    provides.setdefault(name, fields['Package'])
    return pkgs, provides


def deps(fields):
    out = []
    for key in ('Pre-Depends', 'Depends'):
        for alt in fields.get(key, '').split(','):
            alt = alt.strip()
            if alt:
                out.append([re.sub(r'[:\s(].*', '', a.strip()) for a in alt.split('|')])
    return out


def ar_members(data):
    assert data[:8] == b'!<arch>\n'
    pos = 8
    while pos < len(data):
        name = data[pos:pos + 16].decode().strip().rstrip('/')
        size = int(data[pos + 48:pos + 58].decode().strip())
        yield name, data[pos + 60:pos + 60 + size]
        pos += 60 + size + (size & 1)


def extract(deb, root):
    for name, body in ar_members(deb):
        if name.startswith('data.tar'):
            if name.endswith('.xz'):
                body = lzma.decompress(body)
            elif name.endswith('.zst'):
                raise SystemExit('zstd data archives are not supported')
            with tarfile.open(fileobj=io.BytesIO(body)) as t:
                for m in t.getmembers():
                    target = root / m.name
                    if not target.resolve().is_relative_to(root.resolve()):
                        raise SystemExit('unsafe path in package: ' + m.name)
                    if m.isdir():
                        target.mkdir(parents=True, exist_ok=True)
                    elif m.isfile():
                        target.parent.mkdir(parents=True, exist_ok=True)
                        target.write_bytes(t.extractfile(m).read())
                    elif m.issym() or m.islnk():
                        target.parent.mkdir(parents=True, exist_ok=True)
                        (root / 'symlinks.txt').open('a').write(f'{m.name}\t{m.linkname}\t{int(m.islnk())}\n')


def materialize_links(root):
    """Windows cannot rely on symlinks: copy link targets in place."""
    lst = root / 'symlinks.txt'
    if not lst.exists():
        return
    lines = [l.split('\t') for l in lst.read_text().splitlines() if l]
    for _ in range(4):
        for name, target, hard in lines:
            dst = root / name
            src = (root / target.lstrip('/')) if (target.startswith('/') or hard == '1') else dst.parent / target
            if src.is_file() and not dst.exists():
                dst.parent.mkdir(parents=True, exist_ok=True)
                dst.write_bytes(src.read_bytes())


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--arch', required=True)
    ap.add_argument('--release', default='bookworm')
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('packages', nargs='*')
    args = ap.parse_args()
    out = args.out
    cache = out / 'debs'
    root = out / 'sysroot'
    cache.mkdir(parents=True, exist_ok=True)
    root.mkdir(parents=True, exist_ok=True)
    index = cache / f'Packages-{args.release}-{args.arch}'
    if not index.exists():
        index.write_bytes(lzma.decompress(fetch(f'{MIRROR}/dists/{args.release}/main/binary-{args.arch}/Packages.xz')))
    pkgs, provides = parse_packages(index.read_text(encoding='utf-8'))
    want, seen = list(args.packages or DEV), set()
    while want:
        name = want.pop()
        name = name if name in pkgs else provides.get(name, name)
        if name in seen or name in SKIP:
            continue
        if name not in pkgs:
            print('missing', name)
            continue
        seen.add(name)
        for alt in deps(pkgs[name]):
            choice = next((a for a in alt if a in pkgs or a in provides), alt[0])
            want.append(choice)
    for name in sorted(seen):
        f = pkgs[name]
        deb = cache / Path(f['Filename']).name
        if not deb.exists():
            data = fetch(f"{MIRROR}/{f['Filename']}")
            if hashlib.sha256(data).hexdigest() != f['SHA256']:
                raise SystemExit('checksum mismatch: ' + name)
            deb.write_bytes(data)
        extract(deb.read_bytes(), root)
    materialize_links(root)
    (out / 'packages.txt').write_text('\n'.join(sorted(seen)) + '\n')
    print(len(seen), 'packages in', root)


if __name__ == '__main__':
    main()
