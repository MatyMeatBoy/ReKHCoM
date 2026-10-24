"""Build every cartridge version with one command; never uploads a release.

python port/tools/build_all.py --dry-run
python port/tools/build_all.py --package --release-version v1.8.3
python port/tools/build_all.py --platform all --package --release-version v1.9.9

--platform all runs windows, linux, android and web in turn (web uses the
D:/01Claude/ReKHCoM-build folders unless --emsdk/--web-sdl-build/--web-work are given).
Linux needs the sysroots prepared once (port/progress/LINUX.md). PS5 requires
a native Linux host and the three original ROMs; run --platform ps5 there.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--platform', choices=['windows', 'linux', 'android', 'web', 'ps5', 'all'], default='windows')
    p.add_argument('--versions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    p.add_argument('--jobs', type=int, default=3, help='Total compiler workers; region builds run sequentially')
    p.add_argument('--out-prefix', default='build/rel-')
    p.add_argument('--toolchain', type=Path)
    p.add_argument('--sdl', type=Path)
    p.add_argument('--ninja', type=Path)
    p.add_argument('--emsdk', type=Path)
    p.add_argument('--web-sdl-build', type=Path)
    p.add_argument('--web-work', type=Path)
    p.add_argument('--package', action='store_true')
    p.add_argument('--release-version')
    p.add_argument('--dry-run', action='store_true')
    args = p.parse_args()
    if args.jobs < 1:
        p.error('--jobs must be positive')
    if args.platform == 'all':
        rest = [a for a in sys.argv[1:] if a not in ('all',)]
        rest = [a for i, a in enumerate(rest) if not (a == '--platform')]
        cache = Path('D:/01Claude/ReKHCoM-build')
        for plat in ('windows', 'linux', 'android', 'web'):
            extra = []
            if plat == 'web':
                for flag, value in (('--emsdk', cache / 'toolchain/emsdk'), ('--web-sdl-build', cache / 'web-sdl'),
                                    ('--web-work', cache / 'web'),
                                    ('--sdl', ROOT.parent / 'toolchain/android/SDL3-3.4.18')):
                    if flag not in rest:
                        extra += [flag, str(value)]
            command = [sys.executable, __file__, '--platform', plat, *rest, *extra]
            if plat in ('android', 'web'):
                command = [c for c in command if c != '--package']
                if '--release-version' in command:
                    i = command.index('--release-version')
                    del command[i:i + 2]
            print('==', plat, flush=True)
            subprocess.run(command, cwd=ROOT, check=True)
        if sys.platform.startswith('linux'):
            command = [sys.executable, __file__, '--platform', 'ps5', *rest]
            print('== ps5', flush=True)
            subprocess.run(command, cwd=ROOT, check=True)
        elif Path('D:/01Claude/ReKHCoM-build/ps5/ps5env.qcow2').is_file() and '--package' in rest and '--dry-run' not in rest:
            print('== ps5 (Debian VM)', flush=True)
            subprocess.run([sys.executable, 'port/tools/build_ps5_vm.py'], cwd=ROOT, check=True)
        else:
            print('PS5 skipped: needs a native Linux host, or the prepared VM image (D:/01Claude/ReKHCoM-build/ps5) with --package.', flush=True)
        return
    if args.platform == 'ps5':
        command = [sys.executable, 'port/ps5/build_ps5.py', 'all', '--jobs', str(args.jobs)]
        if args.package:
            if not args.release_version:
                p.error('PS5 --package requires --release-version')
            version = re.search(r'#define REKHCOM_VERSION\s+"([^"]+)"',
                                (ROOT / 'port/include/port/port_version.h').read_text(encoding='utf-8'))
            if not version or args.release_version != version[1]:
                p.error('--release-version must match REKHCOM_VERSION in the launcher')
            numeric_version = re.match(r'v?(\d+)\.(\d+)(?:\.(\d+))?', args.release_version)
            if not numeric_version:
                p.error('--release-version must start with vX.Y or vX.Y.Z for the PS5 package')
            # v2.0 is 2.0.0 for the PS5 package (contentVersion and the in-app updater compare three numbers)
            command += ['--version', '.'.join(g or '0' for g in numeric_version.groups()), '--package']
        elif args.release_version:
            command += ['--version', args.release_version]
        if args.dry_run:
            print(json.dumps({'platform': 'ps5', 'commands': [command]}, indent=2))
        else:
            if not sys.platform.startswith('linux'):
                if args.package:
                    subprocess.run([sys.executable, 'port/tools/build_ps5_vm.py', '--version', args.release_version], cwd=ROOT, check=True)
                    return
                p.error('PS5 builds require native Linux; use the prepared Linux VM (--package builds it there) or a Linux host')
            subprocess.run(command, cwd=ROOT, check=True, env=dict(os.environ, PYTHONUTF8='1'))
        return
    if args.platform == 'linux':
        if args.package:
            commands = [[sys.executable, 'port/tools/build_linux.py', '--package', args.release_version,
                         '--jobs', str(args.jobs)]]
        else:
            commands = [[sys.executable, 'port/tools/build_linux.py', '--arch', arch, '--versions', *args.versions,
                         '--jobs', str(args.jobs)] for arch in ('i386', 'armhf')]
        if args.dry_run:
            print(json.dumps({'platform': 'linux', 'commands': commands}, indent=2))
        else:
            for command in commands:
                subprocess.run(command, cwd=ROOT, check=True, env=dict(os.environ, PYTHONUTF8='1'))
        return
    if args.platform == 'web':
        if not all([args.emsdk, args.sdl, args.web_sdl_build, args.web_work]):
            p.error('Web requires --emsdk, --sdl (source), --web-sdl-build and --web-work')
        commands = [[sys.executable, 'port/tools/build_web.py', '--emsdk', str(args.emsdk),
                     '--sdl-source', str(args.sdl), '--sdl-build', str(args.web_sdl_build),
                     '--work', str(args.web_work), '--jobs', str(args.jobs), '--region', region]
                    for region in dict.fromkeys(args.versions)]
        if args.dry_run:
            print(json.dumps({'platform':'web', 'commands':commands}, indent=2))
        else:
            for command in commands:
                subprocess.run(command, cwd=ROOT, check=True, env=dict(os.environ, PYTHONUTF8='1'))
        return
    if args.platform == 'android':
        command=[sys.executable,'port/tools/build_android.py','--regions',*args.versions,'--jobs',str(args.jobs)]
        if args.dry_run:
            print(json.dumps({'platform':'android','commands':[command]},indent=2))
        else:
            subprocess.run(command,cwd=ROOT,check=True)
        return
    versions = list(dict.fromkeys(args.versions))
    if args.package and set(versions) != {'eu', 'us', 'jp'}:
        p.error('A public package requires EU, US and JP; partial packages are not supported')
    version = re.search(r'#define REKHCOM_VERSION\s+"([^"]+)"',
                        (ROOT / 'port/include/port/port_version.h').read_text(encoding='utf-8'))
    if args.package and (not version or args.release_version != version[1]):
        p.error('--release-version must match REKHCOM_VERSION in the launcher; this tool does not change versions')
    base = ROOT.parent / 'toolchain'
    def dependency(explicit, pattern):
        hits = sorted(base.glob(pattern))
        path = explicit or (hits[-1] if hits else None)
        if not path:
            p.error('Missing dependency: ' + pattern)
        return Path(path).resolve()
    tc = dependency(args.toolchain, 'llvm-mingw-*')
    sdl = dependency(args.sdl, 'SDL3-*')
    ninja = args.ninja or shutil.which('ninja') or base / 'bin/ninja.exe'
    if not Path(ninja).is_file():
        p.error('Ninja is missing; pass --ninja PATH')
    outputs = {v: (ROOT / (args.out_prefix + v)).resolve() for v in versions}
    for out in outputs.values():
        if not out.is_relative_to(ROOT / 'build'):
            p.error('Build outputs must stay inside this repository/build')
    commands = []
    for v, out in outputs.items():
        commands += [[sys.executable, 'port/configure_pc.py', '--version', v,
                      '--out', str(out.relative_to(ROOT)), '--toolchain', str(tc), '--sdl', str(sdl)],
                     [str(ninja), '-f', str(out / 'build.ninja'), '-j', str(args.jobs)]]
    if args.package:
        commands.append([sys.executable, 'port/tools/package_release.py', '--version', args.release_version,
                         '--build', str(outputs['eu']), '--build-us', str(outputs['us']),
                         '--build-jp', str(outputs['jp']), '--sdl', str(sdl)])
    if args.dry_run:
        print(json.dumps({'platform': args.platform, 'commands': commands}, indent=2))
        return
    env = dict(os.environ, PYTHONUTF8='1')
    for cmd in commands:
        subprocess.run(cmd, cwd=ROOT, env=env, check=True)
    manifest = {'platform': 'windows', 'commit': subprocess.check_output(
        ['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(), 'builds': {}}
    for v, out in outputs.items():
        exe = out / 'khcom.exe'
        manifest['builds'][v] = {'exe': str(exe.relative_to(ROOT)),
                                'sha256': hashlib.sha256(exe.read_bytes()).hexdigest()}
    report = ROOT / 'build/build-all-manifest.json'
    report.write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print('Built:', ', '.join(versions), '| manifest:', report)


if __name__ == '__main__':
    main()
