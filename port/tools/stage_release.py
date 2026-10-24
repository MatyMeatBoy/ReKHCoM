"""Stage a release's files under the names Quiver Launcher can pick automatically.

Quiver installs and updates a catalog app on its own only when exactly one
release file matches the player's platform (DownloadAssetPolicy.Select in
tgeorgiadis/quiver-launcher): on Windows any .zip/.7z/.rar without another
platform's marker counts, on Linux any tar.gz/AppImage/.deb with no i386 or ARM
marker. So the files for other systems carry their platform in the name, the
web player is not attached (it is served by GitHub Pages). The PS5 package
sits in the same release, next to the others (user, 19/10): its updater wants the
exact name PPSA98511.zip on a stable vX.Y.Z release, so the name stays and the
upload gives it a visible label (gh: "PPSA98511.zip#ReKHCoM-vX-ps5-beta (PPSA98511.zip)").
From v1.9.14 (user, 21/10) PC files keep their plain names and every other platform is published as -beta
(still being debugged); the build outputs keep their -alpha names.
Quiver's catalog entry picks the Windows file by its releaseAssetFilter
"windows-x86.zip" (docs/quiver/submission-draft.json), so the second .zip does not
compete with it. Never edit a published stable's existing files: Quiver pins them.

    python port/tools/stage_release.py v1.9.12    # -> build/publish/v1.9.12/main
"""
import hashlib
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
V = sys.argv[1]
SRC = ROOT / 'build/releases'
OUT = ROOT / 'build/publish' / V
MAIN = {
    f'ReKHCoM-{V}-windows-x86.zip': f'ReKHCoM-{V}-windows-x86.zip',
    f'ReKHCoM-{V}-linux-x86.tar.gz': f'ReKHCoM-{V}-linux-x86.tar.gz',
    f'ReKHCoM-{V}-steamdeck.tar.gz': f'ReKHCoM-{V}-steamdeck-i386.tar.gz',
    f'ReKHCoM-{V}-anbernic-alpha.zip': f'ReKHCoM-{V}-linux-armhf-anbernic-beta.zip',
    f'ReKHCoM-{V}-android-alpha.apk': f'ReKHCoM-{V}-android-beta.apk',
    'PPSA98511.zip': 'PPSA98511.zip',
    f'ReKHCoM-{V}-ps4-alpha.pkg': f'ReKHCoM-{V}-ps4-beta.pkg',
    f'ReKHCoM-{V}-switch-alpha.nro': f'ReKHCoM-{V}-switch-beta.nro',  # build/switch/ReKHCoM.nro (build_switch.py)
    f'ReKHCoM-{V}-vita-alpha.vpk': f'ReKHCoM-{V}-vita-beta.vpk',  # build/vita/ReKHCoM.vpk (build_vita.py)
}


def stage(mapping, folder):
    folder.mkdir(parents=True, exist_ok=True)
    sums = []
    for source, name in mapping.items():
        src = SRC / source
        if not src.is_file():
            print('missing', src.name)
            continue
        shutil.copy2(src, folder / name)
        sums.append(f'{hashlib.sha256((folder / name).read_bytes()).hexdigest()}  {name}')
    (folder / f'SHA256SUMS-{V}{"-ps5" if folder.name == "ps5" else ""}.txt').write_text('\n'.join(sums) + '\n')
    return sums


if OUT.exists():
    shutil.rmtree(OUT)
print('\n'.join(stage(MAIN, OUT / 'main')))
