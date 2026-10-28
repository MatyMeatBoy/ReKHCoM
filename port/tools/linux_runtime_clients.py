"""Native-host client libraries without replacing the host's versioned GPU ABI.

SDL opens unversioned client names. PulseAudio additionally needs a private,
version-specific libpulsecommon SONAME, which another distro cannot supply.
Stable system SONAMEs (glibc, X11/xcb, systemd, etc.) remain host-resolved.
"""
from pathlib import Path
import re
import shutil
import subprocess

PRIVATE_PULSE = re.compile(r'libpulsecommon-[A-Za-z0-9_.+-]+\.so')


def elf_identity(path):
    with Path(path).open('rb') as stream:
        header = stream.read(20)
    if len(header) != 20 or header[:4] != b'\x7fELF':
        raise SystemExit(f'not an ELF runtime library: {path}')
    return header[4:6], header[18:20]  # ELF class/endianness and machine.


def needed(path, readelf):
    text = subprocess.check_output([str(readelf), '-d', str(path)], text=True)
    return [line.split('[')[1].split(']')[0] for line in text.splitlines() if '(NEEDED)' in line]


def copy_client_libraries(names, search, destination, readelf):
    destination = Path(destination)
    destination.mkdir(parents=True, exist_ok=True)

    def find(name):
        return next((folder / name for folder in search if (folder / name).is_file()), None)

    for name in names:
        source = find(name)
        if source is None:
            print('optional system library missing:', name)
            continue
        # Only the unversioned name SDL dlopens; versioned host libraries are
        # deliberately not shadowed for the system's GPU driver.
        shutil.copyfile(source, destination / (name.split('.so')[0] + '.so'))
        if not name.startswith('libpulse'):
            continue
        identity = elf_identity(source)
        todo, copied = [source], set()
        while todo:
            for dependency in needed(todo.pop(), readelf):
                if not dependency.startswith('libpulsecommon-'):
                    continue
                if not PRIVATE_PULSE.fullmatch(dependency):
                    raise SystemExit(f'invalid private runtime library: {dependency}')
                if dependency in copied:
                    continue
                private = find(dependency)
                if private is None:
                    raise SystemExit(f'missing private runtime library {dependency} required by {name}')
                if elf_identity(private) != identity:
                    raise SystemExit(f'wrong ELF architecture for private runtime library {dependency}')
                shutil.copyfile(private, destination / dependency)
                copied.add(dependency)
                todo.append(private)
