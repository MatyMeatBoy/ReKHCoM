"""Repair SDL's X11 runtime-name discovery for materialized Windows sysroots.

Upstream derives these from symlink REALPATH. Our Windows sysroot stores
copies, so use the ELF DT_SONAME instead, without changing upstream sources.
"""
from pathlib import Path
import re
import subprocess

ENTRY = re.compile(r'^(#define SDL_VIDEO_DRIVER_X11_DYNAMIC(?:_[A-Z0-9]+)? )"([^"]+)"', re.M)
RUNTIME_NAME = re.compile(r'lib[A-Za-z0-9_+.-]+\.so(?:\.[0-9]+)+\Z')


def normalize_x11_sonames(config, sysroot, readelf):
    config, sysroot = Path(config), Path(sysroot).resolve()
    original = config.read_text(encoding='utf-8')
    rows = []
    replacements = {}
    for match in ENTRY.finditer(original):
        name = match[2]
        if Path(name).name != name or '/' in name or '\\' in name:
            raise ValueError(f'invalid X11 library name: {name}')
        candidates = sorted(sysroot.glob('usr/lib/*/' + name)) + sorted(sysroot.glob('lib/*/' + name))
        source = next((p for p in candidates if p.is_file()), None)
        if source is None:
            raise FileNotFoundError(f'X11 runtime source missing in sysroot: {name}')
        if not source.resolve().is_relative_to(sysroot):
            raise ValueError(f'X11 runtime source outside sysroot: {source}')
        result = subprocess.run([str(readelf), '-d', str(source)], check=True, capture_output=True, text=True)
        names = re.findall(r'\(SONAME\).*?\[([^\]]+)\]', result.stdout)
        if len(names) != 1 or not RUNTIME_NAME.fullmatch(names[0]):
            raise ValueError(f'missing or invalid runtime SONAME in {source}')
        soname = names[0]
        replacements[match[0]] = match[1] + '"' + soname + '"'
        rows.append(dict(macro=match[1].strip().split()[-1], source=str(source),
                         previous=name, soname=soname, changed=name != soname))
    text = ENTRY.sub(lambda m: replacements[m[0]], original)
    if text != original:
        pending = config.with_name(config.name + '.sonames.tmp')
        with pending.open('x', encoding='utf-8', newline='\n') as stream:
            stream.write(text)
        pending.replace(config)
    return rows
