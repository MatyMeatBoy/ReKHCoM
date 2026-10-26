"""Optional static 360p gallery posters beside imported plain MP4s (one decoded frame per segment)."""
import argparse
import os
from pathlib import Path
import struct
import subprocess

WIDTH, HEIGHT = 640, 360


def poster_seconds(movie):
    return {'zz_event_123': (1, 171), 'zz_event_151': (1, 223)}.get(Path(movie).stem, (1,))


def poster_path(movie, second):
    return Path(movie).with_suffix(f'.poster-v1-t{second}-640x360.png')


def current_poster(movie, poster):
    try:
        if poster.stat().st_mtime_ns < movie.stat().st_mtime_ns or poster.stat().st_size > 2 * 1024 * 1024:
            return False
        with poster.open('rb') as file:
            header = file.read(24)
        return (header[:8] == b'\x89PNG\r\n\x1a\n' and header[12:16] == b'IHDR'
                and len(header) == 24 and struct.unpack('>II', header[16:24]) == (WIDTH, HEIGHT))
    except OSError:
        return False


def generate_poster(movie, second, ffmpeg='ffmpeg'):
    movie = Path(movie)
    poster = poster_path(movie, second)
    if current_poster(movie, poster):
        return True
    partial = poster.with_suffix('.part.png')
    command = [str(ffmpeg), '-v', 'error', '-nostdin', '-threads', '1', '-ss', str(second), '-i', str(movie),
               '-frames:v', '1', '-vf', 'scale=640:360:flags=area', '-an', '-y', str(partial)]
    try:
        subprocess.run(command, check=True, timeout=30, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
        if not current_poster(movie, partial):
            # A preserved/future source timestamp must not make a newly generated image stale.
            if partial.is_file():
                stamp = max(partial.stat().st_mtime_ns, movie.stat().st_mtime_ns)
                os.utime(partial, ns=(stamp, stamp))
            if not current_poster(movie, partial):
                return False
        os.replace(partial, poster)
        return True
    except (OSError, subprocess.SubprocessError):
        return False  # no ffmpeg/codec: movies still import and play as before
    finally:
        partial.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('hd', type=Path, help='existing imported hd folder; source movies are never changed')
    parser.add_argument('--ffmpeg', default='ffmpeg')
    args = parser.parse_args()
    total = ok = 0
    for folder in ('movies', 'scenes'):
        for movie in sorted((args.hd / folder).glob('*.mp4')):
            for second in poster_seconds(movie):
                total += 1
                ok += generate_poster(movie, second, args.ffmpeg)
    print(f'{ok}/{total} static posters ready; original movies unchanged')
    return 0 if ok == total else 1


if __name__ == '__main__':
    raise SystemExit(main())
