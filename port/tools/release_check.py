"""Release checklist: run it before and after publishing; any FAIL blocks the release.

    python port/tools/stage_release.py v1.9.9
    python port/tools/release_check.py v1.9.9            # before: versions, staged files, Quiver
    python port/tools/release_check.py v1.9.9 --post     # after: GitHub releases and Pages

Before: the version is the same in the launcher (REKHCOM_VERSION), the web page
and the top CHANGELOG section; the tree is committed; every staged file
(build/publish/<v>) is newer than HEAD, the Windows zip carries the version,
and Quiver Launcher finds exactly one file for Windows, Linux x64 and Android
(its rule for automatic installs and updates). After: the main release is the
stable Latest with every staged file (PPSA98511.zip, the PS5 package, included),
and GitHub Pages serves the same version.
Never edit or re-upload a published stable release: Quiver pins each file's
SHA-256 and verifies a release only after it stays unchanged (about 48 h).
"""
import argparse
import json
import re
import subprocess
import sys
import time
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPO = 'MatyMeatBoy/ReKHCoM'
PAGES = 'https://matymeatboy.github.io/ReKHCoM/'
ap = argparse.ArgumentParser()
ap.add_argument('version')
ap.add_argument('--post', action='store_true')
ap.add_argument('--prerelease', action='store_true', help='expect a prerelease instead of the stable Latest')
ap.add_argument('--no-ps5', action='store_true')
args = ap.parse_args()
V = args.version
STAGE = ROOT / 'build/publish' / V
fails = []


def check(ok, text):
    print(('PASS ' if ok else 'FAIL ') + text)
    if not ok:
        fails.append(text)


# --- Quiver Launcher's platform rules (lib/QuiverLauncher.Core/Services/PlatformAssetMatcher.cs) ---
NON_WIN = ['linux', 'macos', 'osx', 'darwin', 'apple', '.deb', '.rpm', 'appimage', 'flatpak', '.dmg', '.pkg',
           'android', 'arm64-v8a', '.apk', 'switch']


def any_of(s, *subs):
    return any(x in s for x in subs)


def auxiliary(s):
    return (s.endswith('.json') or re.search(r'\.(?:ya?ml|blockmap|zsync)\Z', s) or
            re.search(r'(?:^|[._\s-])(?:notices?|sources?|source[._\s-]code|src)\.(?:zip|7z|rar|tar(?:\.(?:gz|xz|bz2|zst))?|tgz|tbz2|txz)\Z', s) or
            re.search(r'(?:\.(?:sha(?:1|224|256|384|512)?|md5|sig|asc|minisig|signature|debug)(?:\.txt)?$)|(?:^|[._-])(?:checksums?|sha(?:1|224|256|384|512)?sums?|md5sums?|pdb|dsym|symbols|debugsymbols|debug[._-]symbols)(?:$|[._-])', s))


def excluded(s):
    return (re.search(r'(?:^|[^a-z0-9])(?:ios|ipados|iphone|ipad|iphoneos|iphonesimulator)(?:$|[^a-z0-9])|\.ipa(?:$|[._-])', s) or
            re.search(r'(?:^|[^a-z0-9])(?:xbox|portmaster|stockos\d*|rg\d{2,3}[a-z0-9]*)(?:$|[^a-z0-9])', s) or auxiliary(s) or
            re.search(r'(?:^|[._-])(?:pdb|symbols|debugsymbols|debug[._-]symbols)(?:$|[._-])', s))


def windows(s):
    if excluded(s) or any_of(s, *NON_WIN) or re.search(r'(?:^|[^a-z0-9])mac(?:$|[^a-z0-9])', s):
        return False
    return (any_of(s, 'windows', 'win64', 'win32', 'win-x64', 'win-x86', '-win.', '_win.', '.exe', '.msi', 'msvc', 'mingw')
            or re.search(r'[_-]win[_-]|[_-]win\d|^win[_-]', s) or s.endswith(('.zip', '.7z', '.rar')))


def linux_x64(s):
    if excluded(s) or windows(s) or any_of(s, 'windows', 'win32', 'win64', 'macos', 'osx', 'darwin', '.exe', '.msi',
                                            '.dmg', 'switch', 'android', '.apk'):
        return False
    if not any_of(s, 'linux', 'appimage', 'flatpak', '.deb', '.rpm', 'tar.gz', 'tar.xz'):
        return False
    return not any_of(s, 'i686', 'i386', 'i586', 'x86-linux', '-i686-', 'arm64', 'aarch64', 'armv7', 'armhf', 'arm-')


def android(s):
    if excluded(s) or any_of(s, 'windows', 'win32', 'win64', 'linux', 'macos', 'osx', 'darwin', '.exe', '.msi',
                             'appimage', '.dmg', '.deb', '.rpm', 'switch'):
        return False
    return any_of(s, 'android', 'arm64-v8a') or s.endswith('.apk')


# The catalog entry's releaseAssetFilter narrows the Windows pick (the PS5 .zip
# shares the release; docs/quiver/submission-draft.json).
WIN_FILTER = json.loads((ROOT / 'docs/quiver/submission-draft.json').read_text(encoding='utf-8'))     .get('launcher', {}).get('releaseAssetFilter') if (ROOT / 'docs/quiver/submission-draft.json').is_file() else None


def quiver(names):
    for platform, rule in (('Windows', windows), ('Linux-X64', linux_x64), ('Android', android)):
        hits = [n for n in names if rule(n.lower())]
        if platform == 'Windows' and WIN_FILTER:
            hits = [n for n in hits if WIN_FILTER.lower() in n.lower()]
        check(len(hits) == 1, f'Quiver {platform}: exactly one automatic download ({", ".join(hits) or "none"})')


if not args.post:
    launcher = re.search(r'REKHCOM_VERSION\s+"([^"]+)"', (ROOT / 'port/include/port/port_version.h').read_text(encoding='utf-8')).group(1)
    check(launcher == V, f'launcher version {launcher}')
    rc = re.search(r'REKHCOM_RC_VERSION_TEXT\s+"([^"]+)"', (ROOT / 'port/decompui/rekhcom.rc').read_text()).group(1)
    check('v' + rc == V, f'executable file details (rekhcom.rc) {rc}')
    web = (ROOT / 'port/web/index.html').read_text(encoding='utf-8')
    check(f'Web v{V[1:]}' in web or f'WEB {V}' in web, 'web page label')
    top = re.search(r'^## (v[\d.]+)', (ROOT / 'CHANGELOG.md').read_text(encoding='utf-8'), re.M).group(1)
    check(top == V, f'CHANGELOG top section {top}')
    badges = subprocess.run([sys.executable, str(ROOT / 'port/tools/gen_badges_inc.py'), '--check'],
                            capture_output=True).returncode == 0
    check(badges, 'built-in badge atlas matches port/assets/achievements/badges.png (gen_badges_inc.py)')
    sources = subprocess.run([sys.executable, str(ROOT / 'port/tools/check_sources.py')], capture_output=True, text=True)
    check(sources.returncode == 0, 'console SDL shim and Java literals (check_sources.py)' +
          (': ' + sources.stdout.strip().splitlines()[0] if sources.returncode else ''))
    head = int(subprocess.check_output(['git', 'log', '-1', '--format=%ct'], cwd=ROOT, text=True))
    dirty = subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=no'], cwd=ROOT, text=True)
    check(not dirty.strip(), 'working tree committed (packages come from HEAD)')
    main = sorted(p.name for p in (STAGE / 'main').glob('*'))
    for kind in ('windows-x86.zip', 'linux-x86.tar.gz', 'steamdeck-i386.tar.gz', 'anbernic-beta.zip', 'android-beta.apk'):
        hit = [n for n in main if n.endswith(kind)]
        check(bool(hit) and all((STAGE / 'main' / n).stat().st_mtime >= head - 60 for n in hit),
              f'staged {kind} built after HEAD')
    if not args.no_ps5:
        check((STAGE / 'main/PPSA98511.zip').is_file(), 'staged PS5 PPSA98511.zip (same release)')
    win = next((STAGE / 'main' / n for n in main if n.endswith('windows-x86.zip')), None)
    if win:
        with zipfile.ZipFile(win) as z:
            names = z.namelist()
            check('ReKHCoM.exe' in names and not any(n.startswith('ReKHCoM-') for n in names),
                  'Windows zip has no top folder (updates extract over the old install)')
            check('ReKHCoM.exe' in names and V.encode() in z.read('ReKHCoM.exe'),
                  'Windows launcher inside the zip shows ' + V)
    quiver(main)
else:
    def view(tag):
        return json.loads(subprocess.check_output(['gh', 'release', 'view', tag, '--repo', REPO, '--json',
                                                   'isPrerelease,isDraft,assets'], text=True))
    info = view(V)
    check(not info['isDraft'], 'release is public (not a draft)')
    check(info['isPrerelease'] == args.prerelease, 'release marked ' + ('prerelease' if args.prerelease else 'stable'))
    assets = {a['name'] for a in info['assets']}
    for name in (p.name for p in (STAGE / 'main').glob('*')):
        check(name in assets, f'{name} uploaded')
    quiver(sorted(assets))
    latest = subprocess.run(['gh', 'api', f'repos/{REPO}/releases/latest', '-q', '.tag_name'], text=True,
                            capture_output=True).stdout.strip()
    check(args.prerelease or latest == V, f'GitHub Latest is {latest or "none"}')
    if not args.no_ps5:
        check('PPSA98511.zip' in assets, 'PS5 PPSA98511.zip in the release (its updater needs a stable vX.Y.Z)')
    page = ''
    for attempt in range(10):
        try:
            page = urllib.request.urlopen(PAGES + f'?check={int(time.time())}', timeout=30).read().decode('utf-8', 'replace')
        except Exception as error:  # noqa: BLE001
            page = str(error)
        if V[1:] in page:
            break
        time.sleep(30)
    check(V[1:] in page, f'GitHub Pages serves {V}')
print('\n%s: %d problem(s)' % ('BLOCKED' if fails else 'OK', len(fails)))
sys.exit(1 if fails else 0)
