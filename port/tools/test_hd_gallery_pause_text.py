"""The HD movie pause dialog opened from Options > Scenes (gallery) shows its text (#163 video, sibling of #274).

Silent and hidden, private folder. Needs a built exe, the ROM and the real zz_event_151.mp4 (HD import).
    python port/tools/test_hd_gallery_pause_text.py --exe build/pc/khcom.exe --rom roms/B8CP.gba --movies <hd>/movies --work build/gallerypause
"""
import argparse, os, shutil, subprocess, sys, tempfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from test_hd_movie_pause_text import verify


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'rom', 'movies', 'work'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    a.work.mkdir(parents=True, exist_ok=True)
    for context, hdtitle in (('title', 1), ('title', 0), ('game', 1)):
        run = Path(tempfile.mkdtemp(prefix=f'{context}-{hdtitle}-', dir=a.work))
        for name in ('khcom.exe', 'SDL3.dll'):
            shutil.copy2(a.exe.parent / name, run / name)
        if (a.exe.parent / 'assets').exists():
            shutil.copytree(a.exe.parent / 'assets', run / 'assets')
        (run / 'hd/scenes').mkdir(parents=True)
        shutil.copy2(a.movies / 'ROP.mp4', run / 'hd/scenes/ROP.mp4')
        (run / 'kh_com.sav').write_bytes(b'\xff' * 65536)
        (run / 'scenes_seen.dat').write_bytes(b'\xff' * 128)
        (run / 'kh_com.ini').write_text(
            f'config_version=11\ngraphics_mode={2 - hdtitle}\ngraphics_mode_applied={2 - hdtitle}\nhd_title={hdtitle}\n'
            'hd_text=1\nhd_scenes=1\nhd_movies=1\nhd_music=0\nassets_seen=255\nfullscreen=0\nview_mode=0\n'
            'game_language=5\nzoom=0\nvsync=0\nhd_subtitles=0\nsave_bank=1\n', encoding='utf-8')
        env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
        env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_BACKGROUND_TEST='1',
                   KHCOM_WINDOW='640x360', KHCOM_MODE='1:room:20000' if context == 'game' else '1:mode_title',
                   KHCOM_OPTIONS_AT='120', KHCOM_SCENES_SHOT='2', KHCOM_GALLERY_EVENT='513',
                   KHCOM_GALLERY_KEYS='30:120,70:-1', KHCOM_GALLERY_QUIT_AFTER='70', KHCOM_QUIT='8000',
                   KHCOM_HDMOVIE_PAUSETEST='1', KHCOM_HDMOVIE_PAUSE_ACTION='skip')
        r = subprocess.run([str((run / 'khcom.exe').resolve()), '--rom', str(a.rom.resolve())], cwd=run, env=env,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=180)
        assert (run / 'hdmovie_pause.bmp').exists(), (context, hdtitle, r.returncode, 'gallery HD movie never paused',
                                                      (run / 'kh_com.log').read_text(errors='replace')[-1500:])
        verify(run / 'hdmovie_pause.bmp')
    print('PASS: gallery HD movie pause readable from title (HD and GBA title) and in game')


if __name__ == '__main__':
    main()
