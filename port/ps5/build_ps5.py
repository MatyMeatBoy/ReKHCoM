#!/usr/bin/env python3
"""Build ReKHCoM for PS5.

  guest   compile the game as x32 (32-bit pointers) and link the flat image
  host    build the PS5 app that loads the images
  deploy  upload dist/PPSA98511 to a console over FTP (--ip)

The core (src/, port/src/) is compiled as it is; PS5-specific replacements are
in guest/src. Output goes to port/ps5/build and port/ps5/dist.
"""
import argparse
import concurrent.futures
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
BUILD = HERE / 'build'
DIST = HERE / 'dist'
CLANG = os.environ.get('CLANG', 'clang-18')
LLD = os.environ.get('LLD', 'ld.lld-18')
TARGET = '--target=x86_64-unknown-linux-gnux32'
# Changes the PS5 build makes to a copy of a core file (the file itself is not touched): (anchor, replacement).
PORT_PATCHES = {
    'port_ui.c': [('        case ITEM_INTZOOM:\n        case ITEM_SCALE: return 0;',
                   '        /* the console has no window or screen mode to choose */\n'
                   '        case ITEM_FULLSCREEN:\n        case ITEM_FS_RES:\n        case ITEM_VSYNC: return 0;\n'
                   '        case ITEM_INTZOOM:\n        case ITEM_SCALE: return 0;'),
                  # the Japanese cartridge's options menu is in English (no Japanese font is available here)
                  ('#elif defined(VERSION_JP)\n    return LANG_JA;', '#elif defined(VERSION_JP)\n    return LANG_EN;'),
                  # the title screen's hint names the DualSense's touchpad, which is Select here (every language)
                  ('static const char* const sTitleHint[LANG_COUNT]', '};', 'SELECT  ', 'TOUCH  '),
                  # the MMPX 3.2 filter: a row above Scanlines, off / on / on without edge blending (gPortMmpx, saved as mmpx=)
                  ('#include "port/port_ui.h"\n', '#include "port/port_ui.h"\nextern int gPortMmpx;\n'),
                  ('    ITEM_SCANLINES,\n    ITEM_LCD,', '    ITEM_MMPX,\n    ITEM_SCANLINES,\n    ITEM_LCD,'),
                  ('    { "Scanlines", "Lignes de balayage",',
                   '    { "MMPX 3.2 filter", "Filtre MMPX 3.2", "MMPX-3.2-Filter", "Filtro MMPX 3.2", "Filtro MMPX 3.2", "MMPX 3.2フィルター" },\n'
                   '    { "Scanlines", "Lignes de balayage",'),
                  ('        case ITEM_SCANLINES:\n            if (gPortConfig.scanlines == 0)',
                   '        case ITEM_MMPX: {\n'
                   '            static const char* const mmpx[3][LANG_COUNT] = {\n'
                   '                { "Off", "Non", "Aus", "No", "No", "オフ" },\n'
                   '                { "On", "Activé", "An", "Attivo", "Activo", "オン" },\n'
                   '                { "On without edge blending", "Activé sans fondu des bords", "An ohne Kantenmischung",\n'
                   '                  "Attivo senza fusione dei bordi", "Activo sin mezcla de bordes", "オン（エッジブレンドなし）" },\n'
                   '            };\n'
                   '            SDL_strlcpy(out, mmpx[SDL_clamp(gPortMmpx, 0, 2)][l], size);\n'
                   '            break;\n'
                   '        }\n'
                   '        case ITEM_SCANLINES:\n            if (gPortConfig.scanlines == 0)'),
                  ('        case ITEM_SCANLINES:\n            gPortConfig.scanlines = Wrap(',
                   '        case ITEM_MMPX:\n            gPortMmpx = Wrap(gPortMmpx + dir, 3);\n            break;\n'
                   '        case ITEM_SCANLINES:\n            gPortConfig.scanlines = Wrap(')],
    'port_config.c': [('#include "port/port.h"\n', '#include "port/port.h"\nextern int gPortMmpx;\n'),
                      ('    { "color_correction", &gPortConfig.colorCorrection, 0, 1 },',
                       '    { "color_correction", &gPortConfig.colorCorrection, 0, 1 },\n    { "mmpx", &gPortMmpx, 0, 2 },')],
}
PORT_EXCLUDED = {'port_memory', 'port_sections', 'port_state', 'port_hdtext', 'port_watchdog', 'port_biosfault'}


def patched_source(src, folder):
    """src with its PORT_PATCHES applied, written to folder (only when the text changes, so builds stay incremental).
    The console images (PS5, PS4, Switch, Vita, Android 64-bit) all compile these copies."""
    text = src.read_text(encoding='utf-8')
    for patch in PORT_PATCHES[src.name]:
        if len(patch) == 4:  # (start marker, end marker, old, new): replaced between the markers only
            start, end, old, new = patch
            a = text.find(start)
            b = text.find(end, a)
            if a < 0 or b < 0 or old not in text[a:b]:
                sys.exit(f'{src.name}: the PS5 patch no longer applies')
            text = text[:a] + text[a:b].replace(old, new) + text[b:]
            continue
        anchor, new = patch
        if text.count(anchor) != 1:
            sys.exit(f'{src.name}: the PS5 patch no longer applies')
        text = text.replace(anchor, new)
    patched = folder / src.name
    patched.parent.mkdir(parents=True, exist_ok=True)
    if not patched.exists() or patched.read_text(encoding='utf-8') != text:
        patched.write_text(text, encoding='utf-8', newline='\n')
    return patched


def run(cmd, **kw):
    r = subprocess.run([str(c) for c in cmd], **kw)
    if r.returncode:
        sys.exit(f'{Path(str(cmd[0])).name} failed ({r.returncode})')


def resource_dir():
    return subprocess.check_output([CLANG, '-print-resource-dir'], text=True).strip() + '/include'


def game_includes(region):
    quotes = [ROOT / 'include']
    quotes += sorted(d for d in (ROOT / 'include').iterdir() if d.is_dir() and d.name != 'gba')
    quotes += [BUILD.parent.parent.parent / 'build' / region / 'gen', ROOT / 'port/include']
    return [f'-iquote{d}' for d in quotes]


def common_flags():
    return [TARGET, '-ffreestanding', '-nostdinc', '-isystem', resource_dir(), f'-I{HERE / "guest/include"}',
            f'-I{HERE / "shared"}', '-O2', '-funsigned-char', '-fno-strict-aliasing', '-fwrapv', '-fno-pic', '-fno-pie',
            '-fno-stack-protector', '-fno-asynchronous-unwind-tables', '-fno-common', '-fno-omit-frame-pointer', '-msse4.1', '-fno-builtin-printf',
            '-fno-math-errno']


def compile_all(jobs, units):
    def one(unit):
        src, obj, extra = unit
        obj.parent.mkdir(parents=True, exist_ok=True)
        if obj.exists() and obj.stat().st_mtime >= src.stat().st_mtime and not os.environ.get('REBUILD'):
            dep = obj.with_suffix('.d')
            if dep.exists():
                # make syntax: "obj: dep dep \" lines, spaces inside a path escaped as "\ " (Windows: C:/Users/.../00\ Claude)
                text = dep.read_text().replace('\\\n', ' ')
                text = text[re.search(r':\s', text).end():] if re.search(r':\s', text) else ''
                paths = [p.replace('\\ ', ' ').strip() for p in re.findall(r'(?:\\ |\S)+', text)]
                newest = max((Path(p).stat().st_mtime for p in paths if Path(p).exists()), default=0)
                if newest <= obj.stat().st_mtime:
                    return None
        cmd = [CLANG, *extra, '-MD', '-MF', obj.with_suffix('.d'), '-c', src, '-o', obj]
        r = subprocess.run([str(c) for c in cmd], capture_output=True, text=True)
        if r.returncode:
            return f'{src}\n{r.stderr}'
        return None
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        errors = [e for e in pool.map(one, units) if e]
    if errors:
        print('\n'.join(errors[:5]), file=sys.stderr)
        sys.exit(f'{len(errors)} file(s) failed to compile')


def build_guest(region, jobs):
    upper = region.upper()
    pc = ROOT / 'build' / region / 'pc'
    if not (pc / 'rom_symbols.s').is_file():
        run([sys.executable, ROOT / 'port/tools/gen_pc_assets.py', '--version', region], cwd=ROOT)
    out = BUILD / f'guest-{region}'
    out.mkdir(parents=True, exist_ok=True)
    includes = game_includes(region)
    # The generated assembly is written for COFF (leading underscores, .rdata).
    for name, source in (('rom_symbols', pc / 'rom_symbols.s'), ('fixed_symbols', ROOT / 'port/src/port_symbols.s')):
        text = source.read_text()
        text = text.replace('.section .rdata,"dr"', '.section .rodata').replace('.globl _', '.globl ').replace('.set _', '.set ')
        (out / f'{name}.S').write_text(text)
    game_flags = [*common_flags(), '-w', '-std=gnu89', '-include', ROOT / 'port/include/port/game_prefix.h',
                  f'-DPLATFORM_PC', f'-DVERSION_{upper}', *includes]
    port_flags = [*common_flags(), '-Wall', '-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-but-set-variable',
                  '-Wno-unused-parameter', '-Wno-sign-compare', '-std=gnu11', '-DSTBI_NO_THREAD_LOCALS', f'-I{HERE / "third_party/stb"}', f'-I{ROOT / "port/src"}', '-DPLATFORM_PC', f'-DVERSION_{upper}', *includes]
    units = []
    for src in sorted((ROOT / 'src').rglob('*.c')):
        units.append((src, out / 'game' / src.relative_to(ROOT / 'src').with_suffix('.o'), game_flags))
    for src in sorted((ROOT / 'port/src').glob('*.c')):
        if src.stem in PORT_EXCLUDED:
            continue
        if src.name in PORT_PATCHES:
            src = patched_source(src, out / 'patched')
        units.append((src, out / 'port' / (src.stem + '.o'), port_flags))
    units.append((pc / 'rom_relocs.c', out / 'port/rom_relocs.o', port_flags))
    for src in sorted((HERE / 'guest/src').glob('*.c')):
        units.append((src, out / 'ps5' / (src.stem + '.o'), port_flags))
    for src in (out / 'rom_symbols.S', out / 'fixed_symbols.S', HERE / 'guest/src/crt0.S'):
        units.append((src, out / 'asm' / (src.stem + '.o'), [TARGET, '-c']))
    compile_all(jobs, units)
    objects = [u[1] for u in units]
    elf = out / 'guest.elf'
    # the object list goes in a response file: from a deep Windows folder it passes the 32 K command line limit
    rsp = out / 'guest_objects.rsp'
    rsp.write_text('\n'.join('"' + str(o).replace('\\', '/') + '"' for o in objects) + '\n', encoding='utf-8')
    run([LLD, '-m', 'elf32_x86_64', '-static', '--no-gc-sections', '-T', HERE / 'guest/guest.ld', '-Map', out / 'guest.map',
         '-o', elf, f'@{rsp}'])
    binary = out / 'guest.bin'
    run([os.environ.get('OBJCOPY', 'llvm-objcopy-18'), '-O', 'binary', elf, binary])
    print(f'guest {region}: {binary} ({binary.stat().st_size} bytes)')
    return binary


MESSAGES = {
    'need-rom': ('A compatible ROM is required',
                 ['Copy your Kingdom Hearts: Chain of Memories cartridge dump',
                  '(Europe, USA or Japan, a .gba file) to the folder',
                  '/data/ReKHCom/',
                  'The game starts by itself as soon as the file is there.']),
    'load-error': ('The game could not be started',
                   ['Something failed while loading the game.',
                    'The reason is written to /data/ReKHCom/logs/ReKHCoM.log']),
}


TOASTS = {
    'toast-states': ['Touchpad + L3: save state', 'Touchpad + R3: load state'],
    'toast-update-found': ['A new version is available', 'Downloading it now'],
    'toast-update-done': ['ReKHCoM was updated', 'Restart the app to use the new version'],
}


def render_toasts(out):
    """Small boxes the app draws over the game for two seconds: RGBA with a w, h header."""
    import struct
    from PIL import Image, ImageDraw, ImageFont
    font = ImageFont.truetype(str(ROOT / 'port/assets/fonts/KHMenu.otf'), 40)
    for name, lines in TOASTS.items():
        w, h = 760, 40 + 62 * len(lines)
        panel = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        d = ImageDraw.Draw(panel)
        d.rounded_rectangle((0, 0, w - 1, h - 1), 24, fill=(14, 16, 36, 225), outline=(130, 150, 255, 255), width=3)
        for i, line in enumerate(lines):
            d.text((w // 2, 20 + 62 * i + 31), line, font=font, fill=(235, 238, 255, 255), anchor='mm')
        (out / f'{name}.rgba').write_bytes(struct.pack('<II', w, h) + panel.tobytes())


def render_messages(out):
    """Full-screen notices drawn by the app when it has nothing to play: RGBA with a w, h header."""
    import struct
    from PIL import Image, ImageDraw, ImageFont
    font_path = ROOT / 'port/assets/fonts/KHMenu.otf'
    title_font, body_font = ImageFont.truetype(str(font_path), 64), ImageFont.truetype(str(font_path), 38)
    for name, (title, lines) in MESSAGES.items():
        w, h = 1500, 150 + 62 * len(lines)
        panel = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        d = ImageDraw.Draw(panel)
        d.rounded_rectangle((0, 0, w - 1, h - 1), 28, fill=(14, 16, 36, 235), outline=(130, 150, 255, 255), width=3)
        d.text((w // 2, 70), title, font=title_font, fill=(255, 255, 255, 255), anchor='mm')
        for i, line in enumerate(lines):
            colour = (255, 214, 102, 255) if line.startswith('/') else (214, 220, 255, 255)
            d.text((w // 2, 150 + 62 * i + 20), line, font=body_font, fill=colour, anchor='mm')
        (out / f'{name}.rgba').write_bytes(struct.pack('<II', w, h) + panel.tobytes())


def sync_content_version():
    """ReKHCoM_VERSION (app/ReKHCoM.h) into sce_sys/param.json as contentVersion: 1.0.3 -> 01.000.003."""
    m = re.search(r'#define\s+ReKHCoM_VERSION\s+"(\d+)\.(\d+)\.(\d+)"', (HERE / 'app/ReKHCoM.h').read_text())
    if not m:
        sys.exit('ReKHCoM_VERSION (X.Y.Z) not found in app/ReKHCoM.h')
    major, minor, patch = (int(g) for g in m.groups())
    if major > 99 or minor > 999 or patch > 999:
        sys.exit('ReKHCoM_VERSION does not fit contentVersion (00-99.000-999.000-999)')
    path = HERE / 'sce_sys/param.json'
    text = path.read_text()
    new = re.sub(r'("contentVersion"\s*:\s*)"[^"]*"', lambda mm: f'{mm.group(1)}"{major:02d}.{minor:03d}.{patch:03d}"', text, count=1)
    if new != text:
        path.write_text(new)


def build_host(regions):
    sync_content_version()
    assets = BUILD / 'assets'
    if assets.exists():
        shutil.rmtree(assets)
    assets.mkdir(parents=True)
    for region in regions:
        shutil.copy2(BUILD / f'guest-{region}/guest.bin', assets / f'guest-{region}.bin')
    render_messages(assets)
    render_toasts(assets)
    shutil.copytree(HERE / 'assets/fonts', assets / 'fonts')
    (assets / 'achievements').mkdir()
    shutil.copy2(ROOT / 'port/assets/achievements/badges.png', assets / 'achievements/badges.png')
    env = dict(os.environ,
               APP_VULKAN='1', APP_SOURCE_DIR='app', APP_PARAM='sce_sys/param.json', APP_SCE_SYS='sce_sys',
               APP_ASSETS='build/assets', APP_LAPY_HELPER='1', APP_ROOT_FILES='', APP_RUNTIME_MODULES='',
               APP_STATIC_ARCHIVES='', APP_DEFINITIONS='ReKHCoM_VULKAN=1',
               APP_INCLUDE_PATHS='app app/platform shared .deps/native/radv-release/include',
               APP_WRAP_SYMBOLS='fcntl malloc free calloc realloc reallocf memalign posix_memalign aligned_alloc malloc_usable_size',
               PACBREW_PACKAGES='sdl2 libcurl', PACBREW_INCLUDE_PATHS='', PACBREW_STATIC_ARCHIVES='')
    run(['bash', HERE / 'tools/fetch-radv.sh'], env=env)
    run(['bash', HERE / 'tools/build.sh', 'Folder'], cwd=HERE, env=env)
    print('app:', DIST / 'PPSA98511')


def package_host():
    app = DIST / 'PPSA98511'
    archive = DIST / 'PPSA98511.zip'
    if archive.exists():
        archive.unlink()
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for path in sorted(p for p in app.rglob('*') if p.is_file()):
            zf.write(path, Path('PPSA98511') / path.relative_to(app))
    print('package:', archive)


def deploy(host, port):
    from ftplib import FTP
    app = DIST / 'PPSA98511'
    files = sorted(p for p in app.rglob('*') if p.is_file())
    last = [app / 'eboot.bin', app / 'sce_sys/param.json']
    files = [p for p in files if p not in last] + last
    with FTP() as ftp:
        ftp.connect(host, port, timeout=15)
        ftp.login('anonymous', 'codex')
        made = set()

        def mkdirs(path):
            parts = ''
            for part in path.strip('/').split('/'):
                parts += '/' + part
                if parts not in made:
                    try:
                        ftp.mkd(parts)
                    except Exception:
                        pass
                    made.add(parts)
        for p in files:
            remote = '/data/homebrew/PPSA98511/' + p.relative_to(app).as_posix()
            mkdirs(os.path.dirname(remote))
            # To a temporary name, then renamed over the old file: a running eboot.bin can't be overwritten
            # ("Text file busy"), but it can be deleted and replaced by name; the new one runs from the next start.
            temporary = os.path.dirname(remote) + '/.' + os.path.basename(remote) + '.upload'
            for stale in (temporary,):
                try:
                    ftp.sendcmd('DELE ' + stale)
                except Exception:
                    pass
            with open(p, 'rb') as f:
                ftp.storbinary('STOR ' + temporary, f, blocksize=256 * 1024)
            try:
                ftp.sendcmd('DELE ' + remote)
            except Exception:
                pass
            ftp.rename(temporary, remote)
            print('uploaded', remote)
        ftp.quit()


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('step', nargs='?', default='guest', choices=['guest', 'host', 'linux', 'all', 'deploy'])
    p.add_argument('--region', choices=['eu', 'us', 'jp'], default='eu')
    p.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    p.add_argument('--ip', help='console address for deploy (FTP)')
    p.add_argument('--ftp-port', type=int, default=2121)
    p.add_argument('--jobs', type=int, default=os.cpu_count() or 2)
    p.add_argument('--version', help='set the PS5 app version for this build (X.Y.Z)')
    p.add_argument('--package', action='store_true', help='write dist/PPSA98511.zip after building the app')
    args = p.parse_args()
    if args.version and args.step not in ('host', 'all'):
        p.error('--version can be used with host or all')
    original_header = None
    original_param = None
    updated_header = None
    if args.version:
        if not re.fullmatch(r'\d+\.\d+\.\d+', args.version):
            p.error('--version must use X.Y.Z')
        header = HERE / 'app/ReKHCoM.h'
        original_header = header.read_text()
        param = HERE / 'sce_sys/param.json'
        original_param = param.read_text()
        updated_header = re.sub(r'(#define\s+ReKHCoM_VERSION\s+")\d+\.\d+\.\d+(" )?',
                                lambda m: m.group(1) + args.version + '"', original_header, count=1)
        if updated_header == original_header:
            p.error('ReKHCoM_VERSION (X.Y.Z) not found in app/ReKHCoM.h')
    if args.step in ('guest', 'all'):
        for region in (args.regions if args.step == 'all' else [args.region]):
            build_guest(region, args.jobs)
    if args.step in ('host', 'all'):
        try:
            if updated_header is not None:
                (HERE / 'app/ReKHCoM.h').write_text(updated_header)
            build_host(args.regions)
            if args.package:
                package_host()
        finally:
            if original_header is not None:
                (HERE / 'app/ReKHCoM.h').write_text(original_header)
                (HERE / 'sce_sys/param.json').write_text(original_param)
    if args.step == 'deploy':
        if not args.ip:
            sys.exit('deploy needs --ip')
        deploy(args.ip, args.ftp_port)


if __name__ == '__main__':
    main()
