#!/usr/bin/env python3
"""Updating from older stables: each old Windows ZIP installed and played once (ROM, a save, a changed setting, the HD
mod in its old layout), then the new package unpacked the way a player does it (over the same folder; for the old
ZIPs with a versioned top folder, next to it), the new launcher and game run hidden, and the player's things checked:
save unchanged, setting kept, ROMs found, HD scenes where the new version looks for them, the game runs.

    py -3 port/tools/test_update.py --new <unpacked new package> [--olds <folder with ReKHCoM-vX-windows-x86.zip>]
"""
import argparse
import os
import shutil
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
KHCOMPC = ROOT.parent
OLDS = KHCOMPC / 'respaldos/releases-estables'
WORK = Path('D:/01Claude/ReKHCoM-build/updtest')
HIDDEN = dict(os.environ, SDL_VIDEO_DRIVER='offscreen', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1')


def game(root, frames=240):
    rom = root / 'roms/B8CP.gba'
    exe = root / 'bin/khcom_eu.exe'
    p = subprocess.run([str(exe), '--data-dir', str(root), '--rom', str(rom)], cwd=exe.parent,
                       env=dict(HIDDEN, KHCOM_QUIT=str(frames)), timeout=300,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return p.returncode


def launcher(root):
    shots = WORK / 'shots'
    shutil.rmtree(shots, ignore_errors=True)
    shots.mkdir(parents=True)
    subprocess.run([str(root / 'ReKHCoM.exe')], cwd=root, env=dict(HIDDEN, DECOMPUI_SHOT=str(shots)), timeout=300,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--new', required=True)
    ap.add_argument('--olds', default=str(OLDS))
    args = ap.parse_args()
    new = Path(args.new)
    save = (KHCOMPC / 'jugar/ReKHCoM-v2.0-pre4/kh_com.sav').read_bytes()
    rom = (ROOT / 'roms/B8CP.gba').read_bytes()
    bad = 0
    for z in sorted(Path(args.olds).glob('ReKHCoM-v*-windows-x86.zip')):
        version = z.name.split('-')[1]
        base = WORK / version
        shutil.rmtree(base, ignore_errors=True)
        base.mkdir(parents=True)
        with zipfile.ZipFile(z) as f:
            f.extractall(base)
            top = {n.split('/')[0] for n in f.namelist()}
        old = base / top.pop() if len(top) == 1 and (base / list(top)[0]).is_dir() else base
        nested = old != base
        # the player's install: ROM, save, a game run (its own settings file), one setting changed, the HD mod
        (old / 'roms').mkdir(exist_ok=True)
        (old / 'roms/B8CP.gba').write_bytes(rom)
        (old / 'kh_com.sav').write_bytes(save)
        game(old, 120)
        ini = old / 'kh_com.ini'
        text = ini.read_text() if ini.exists() else ''
        lines = [l for l in text.splitlines() if not l.startswith('master_volume=')] + ['master_volume=37']
        ini.write_text('\n'.join(lines) + '\n')
        (old / 'hd/movies').mkdir(parents=True)
        (old / 'hd/movies/ROP.mp4').write_bytes(b'old layout opening')
        (old / 'hd/movies/zz_event_151.mp4').write_bytes(b'old layout scene')
        # the update: over the same folder, or (versioned top folder) next to it
        target = base if nested else old
        shutil.copytree(new, target, dirs_exist_ok=True)
        launcher(target)
        code = game(target)
        checks = {
            'save': (target / 'kh_com.sav').exists() and (target / 'kh_com.sav').read_bytes() == save,
            'setting': 'master_volume=37' in ((target / 'kh_com.ini').read_text() if (target / 'kh_com.ini').exists() else ''),
            'rom': (target / 'roms/B8CP.gba').exists(),
            'scenes': (target / 'hd/scenes/zz_event_151.mp4').exists(),
            'opening': (target / 'hd/movies/ROP.mp4').exists(),
            'game': code == 0,
        }
        ok = all(checks.values())
        bad += not ok
        print(f"{version:8} {'nested' if nested else 'flat  '} {'ok' if ok else 'FAIL'} " +
              ' '.join(f"{k}={'ok' if v else 'NO'}" for k, v in checks.items()))
    print('all ok' if not bad else f'{bad} failed')
    return 1 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main())
