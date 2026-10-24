#!/usr/bin/env python3
"""Import the HD mod from the player's own KINGDOM HEARTS -HD 1.5+2.5 ReMIX- (reference for the launcher's importer).

Writes, under <data folder>/hd:
- movies/ROP.mp4, RSE.mp4, RRE.mp4 and scenes/ with the endings' zz_event_151/621/631 (and with --scenes every zz_event_###.mp4): the Re:CoM movies with their first
  0x100 bytes un-XORed (fixed 16-byte key), i.e. plain MP4 files;
- subs/<movie>.<lang>.txt: the movies' subtitles in every language the game has (khhd_subs.py);
- music/<track>.ogg + .loop: the Re:CoM soundtrack (Recom.pkg STREAM/0001/*.win32.scd → Ogg Vorbis, khhd_scd.py).

    python port/tools/khhd_import.py "<install folder>" <data folder> [--scenes] [--no-movies] [--no-music]
"""
import argparse
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import khhd_pkg  # noqa: E402
import khhd_scd  # noqa: E402
import khhd_subs  # noqa: E402

MOVIE_KEY = bytes.fromhex('3f1ec193264258b8aa9d5375bf629a97')
# with the movies, the endings' scenes too (Sora's capsule room, Riku's ending and crossroads): they lead into RSE / RRE
BASE_MOVIES = ('ROP', 'RSE', 'RRE', 'zz_event_151', 'zz_event_621', 'zz_event_631')
TRACKS = ['01F_Town_B', '01F_Town_F', '03F_Hercul', '03F_Hercules_B', '12_Event_Yuttari', '13_Event_Namine',
          '14_Event_Odayaka', '15_Event_XIII', '16_XIIIBoss1', '17_Halloween_F', '18_Helloween_B', '19_Alice_Field',
          '20_Event_13thFloor', '21_Alice_Battle', '22_PeterPan_Field', '23_PeterPan_Battle', '24_Pinocchio_Field',
          '25_Aladdin_F', '26_Aladdin_B', '27_Pinocchio_B', '28_Forget_F', '29_Forget_B', '30_Twilight_F',
          '31_Twilight_B', '32_Destiny_F', '33_Destiny_B', '34_Boss_RikuAnsem', '35_Boss_NiseRiku', '36_WinnieThePooh',
          '37_LastBoss1', '38_UnderTheSea', '39_LittleMermaid_B', '40_LastBoss2', '41_LastBoss3', '42_Hollow_F',
          '43_Hollow_B', '44_PooGame1', '45_PoohGame2', 'Boss1', 'Boss2_World', 'Event_Unrest', 'Event1_Kinpak',
          'Event2_', 'Event4', 'Title']


def copy_movie(src, dst):
    with open(src, 'rb') as f, open(dst, 'wb') as out:
        head = bytearray(f.read(0x100))
        for i in range(len(head)):
            head[i] ^= MOVIE_KEY[i % 16]
        out.write(head)
        while True:
            chunk = f.read(1 << 22)
            if not chunk:
                break
            out.write(chunk)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('install')
    ap.add_argument('data')
    ap.add_argument('--scenes', action='store_true')
    ap.add_argument('--no-movies', action='store_true')
    ap.add_argument('--no-music', action='store_true')
    args = ap.parse_args()
    root = Path(args.install)
    hd = Path(args.data) / 'hd'
    if not args.no_movies:
        movies = root / 'STEAM/dt/KHCReSource/BIN/movie'
        (hd / 'movies').mkdir(parents=True, exist_ok=True)
        names = list(BASE_MOVIES) + (sorted(p.stem for p in movies.glob('zz_event_*.mp4')
                                            if p.stem not in BASE_MOVIES) if args.scenes else [])
        # the story scenes in hd/scenes (23/10; an older import's move over), the rest in hd/movies
        (hd / 'scenes').mkdir(parents=True, exist_ok=True)
        for old in (hd / 'movies').glob('zz_event_*.mp4'):
            if not (hd / 'scenes' / old.name).exists():
                old.rename(hd / 'scenes' / old.name)
        for name in names:
            folder = 'scenes' if name.startswith('zz_event_') else 'movies'
            copy_movie(movies / f'{name}.mp4', hd / folder / f'{name}.mp4')
            print('movie', name)
        print(khhd_subs.extract(root, args.data), 'subtitle files')
    if not args.no_music:
        hed = root / 'Image/dt/Recom.hed'
        (hd / 'music').mkdir(parents=True, exist_ok=True)
        want = {hashlib.md5(f'STREAM/0001/{t}.win32.scd'.encode()).hexdigest(): t for t in TRACKS}
        with open(hed.with_suffix('.pkg'), 'rb') as pkg:
            for md5, offset, _stored, _real in khhd_pkg.entries(hed):
                if md5 in want:
                    data, _, _ = khhd_pkg.read_asset(pkg, offset)
                    ogg, _rate, _ch, loop = khhd_scd.convert(data)
                    out = hd / 'music' / f'{want[md5]}.ogg'
                    out.write_bytes(ogg)
                    out.with_suffix('.loop').write_text(f'{loop[0]} {loop[1]}\n')
                    print('music', want[md5], loop)


if __name__ == '__main__':
    main()
