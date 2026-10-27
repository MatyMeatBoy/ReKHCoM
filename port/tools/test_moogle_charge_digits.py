"""Moogle shop "Change cards" counts: a dense HD digit (2) without a grey block behind it, grey zeros still grey.

LoadDecimalDigitTiles (mode_ms_top.c) registers each count digit as HD text. Its colours used to be read from the
digit's own tile, and a 2 has more face than panel pixels: the HD 2 was drawn over a block of the wrong colour
(user report 27/10). Every digit now takes the colours of its set's 0.

Silent and hidden, private folder; needs a save with cards in the deck (counts 3x04, 4x02, 5x02 ... as in the
report's save), passed with --save and only copied.
    python port/tools/test_moogle_charge_digits.py --exe build/pc/khcom.exe --rom roms/B8CP.gba --save <kh_com.sav>
        --sprites <hd>/sprites --work build/moogle-digits
"""
import argparse, os, shutil, subprocess, tempfile
from pathlib import Path
from PIL import Image


def grey(im, box):
    return sum(1 for y in range(box[1], box[3]) for x in range(box[0], box[2])
               if 90 < im.getpixel((x, y))[0] < 180 and abs(im.getpixel((x, y))[0] - im.getpixel((x, y))[1]) < 12
               and abs(im.getpixel((x, y))[1] - im.getpixel((x, y))[2]) < 12)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'rom', 'save', 'sprites', 'work'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    a.work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='moogle-', dir=a.work))
    for name in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(a.exe.parent / name, run / name)
    shutil.copy2(a.save, run / 'kh_com.sav')
    (run / 'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=8\nhd_text=1\nhd_cards=1\nhd_hud=1\n'
                                    'graphics_mode=1\ngraphics_mode_applied=1\nvsync=0\ngame_language=5\n')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_WINDOW='1920x1080',
               KHCOM_HD_SPRITES=str(a.sprites.resolve()), KHCOM_MODE='2200:mode_ms_charge', KHCOM_OUTPUT_SHOT='2600',
               KHCOM_KEYS='700-703:START,850-853:A,1000-1003:A,1150-1153:A,1300-1303:A,1450-1453:A', KHCOM_QUIT='2650')
    subprocess.run([str((run / 'khcom.exe').resolve()), '--rom', str(a.rom.resolve()), '--language', 'es'], cwd=run,
                   env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300, check=True)
    im = Image.open(run / 'out_02600.bmp').convert('RGB')
    # cells in the 1920x1080 shot: the 2 of "4 x 02" and of "5 x 02"; the grey "00" of "0 x 00"
    twos = [grey(im, (740, 700, 800, 755)), grey(im, (1065, 480, 1125, 535))]
    zeros = grey(im, (695, 480, 795, 535))
    assert all(n < 55 for n in twos), ('grey block behind an HD 2', twos)
    assert zeros > 1000, ('grey zero counts lost their colour', zeros)
    print('PASS: Moogle counts, HD 2 clean', twos, 'grey zeros', zeros)


if __name__ == '__main__':
    main()
