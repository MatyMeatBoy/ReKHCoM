"""Replay a normal gallery scene, skip it, and require the same gallery back.

Pass a built EU exe; all files and the ROM copy stay in an isolated work folder.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--kind', choices=('basic','gba','hd','hd-error'), default='basic')
    p.add_argument('--from-game', action='store_true')
    p.add_argument('--hd-clip', type=Path)
    p.add_argument('--pause', action='store_true')
    p.add_argument('--save', type=Path)
    a = p.parse_args()
    a.work.mkdir(parents=True, exist_ok=True)
    exe = a.work / 'khcom.exe'
    shutil.copy2(a.exe, exe)
    shutil.copy2(a.exe.parent / 'SDL3.dll', a.work / 'SDL3.dll')
    shutil.copy2(a.rom, a.work / 'baserom.gba')
    if (a.exe.parent / 'assets').is_dir():
        shutil.copytree(a.exe.parent / 'assets', a.work / 'assets', dirs_exist_ok=True)
    if a.save:
        shutil.copy2(a.save, a.work / 'kh_com.sav')
    before = hashlib.sha256((a.work / 'kh_com.sav').read_bytes()).hexdigest() if a.save else None
    (a.work / 'scenes_seen.dat').write_bytes(b'\xff' * 128)
    (a.work / 'kh_com.ini').write_text('config_version=11\ngraphics_mode=0\ngraphics_mode_applied=0\n'
                                    'game_language=5\nfullscreen=0\nview_mode=4\nzoom=8\nvsync=0\nhd_title=1\nhd_text=1\n')
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
               KHCOM_BACKGROUND_TEST='1', KHCOM_WINDOW='640x360', KHCOM_GALLERY_PLAY='650:10',
               KHCOM_KEYS='800-1100:START', KHCOM_QUIT='1500', KHCOM_OPTIONS_HOLD='5')
    if a.kind != 'basic':
        env.pop('KHCOM_GALLERY_PLAY', None)
        env.pop('KHCOM_OPTIONS_HOLD', None)
        env.update(KHCOM_OPTIONS_AT='750', KHCOM_SCENES_SHOT='1' if a.kind == 'gba' else '2',
                   KHCOM_GALLERY_EVENT='10', KHCOM_GALLERY_KEYS='30:120,70:120,110:-1',
                   KHCOM_GALLERY_SHOTS='5,25,65,105', KHCOM_GALLERY_PREVIEWTEST='1', KHCOM_KEYS='850-950:START,1150-1250:START,1450-1550:START', KHCOM_QUIT='1800')
        if a.from_game: env['KHCOM_MODE'] = '610:room:20000'
        if a.pause: env['KHCOM_HDMOVIE_PAUSETEST'] = '1'
        if a.kind in ('hd','hd-error'):
            movie = a.work / 'hd/scenes/zz_event_012.mp4'
            movie.parent.mkdir(parents=True, exist_ok=True)
            if a.kind == 'hd-error': movie.write_bytes(b'invalid movie fixture')
            elif a.hd_clip: shutil.copy2(a.hd_clip, movie)
            else: raise ValueError('--hd-clip is required for the HD test')
    with (a.work / 'run.log').open('w') as log:
        result = subprocess.run([str(exe.resolve())], env=env, cwd=a.work, stdout=log,
                                stderr=subprocess.STDOUT, timeout=240)
    text = (a.work / 'run.log').read_text(errors='replace')
    assert result.returncode == 0, text[-3000:]
    marker = 'scene gallery: restored original state; returning to the list' if a.kind in ('basic','gba') else 'returning to the list choice='
    assert text.count(marker) >= (1 if a.kind == 'basic' else 2), text[-4000:]
    if a.from_game and a.kind == 'gba':
        assert text.count('returning to the list (Mode_MapFix)') == 2, text[-3000:]
    if a.kind == 'hd-error':
        assert text.count('HD zz_event_012 -> 0') == 2, text[-3000:]
    elif a.kind == 'hd':
        assert text.count('HD zz_event_012 -> 1') == 2, text[-3000:]
    if before:
        assert hashlib.sha256((a.work / 'kh_com.sav').read_bytes()).hexdigest() == before, 'SRAM changed during visual replay'
    assert 'bios:' not in text, 'Invalid memory access during scene playback: ' + text[-4000:]
    if a.kind != 'basic':
        assert 'scene gallery preview check: 199/199' in text, text[-4000:]
    if a.kind != 'basic':
        assert 'scene gallery engine posters: 199/199 portable PNGs' in text, text[-4000:]
    # Imported/async PNG posters have dedicated cache/lifecycle probes. Immediate playback
    # must not force a low-resolution UI/subtitle capture into the poster cache.
    assert 'Save state loaded' not in text, 'Gallery playback must preserve official achievement eligibility'
    print('gallery replay: scene returned to its list without invalidating achievements')


if __name__ == '__main__':
    main()
