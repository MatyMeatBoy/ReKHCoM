"""Check the real doorpose:0 synthesis captures, including its world actors.

Use frames380/470 at1280x720 from test_door_views (normal and HD).
The orange door/Sora pixels above the panel distinguish actors from the white
spotlight/card; a coloured menu alone previously passed while both vanished.
"""
import argparse
from pathlib import Path
from PIL import Image


def actor_pixels(path, box):
    image = Image.open(path).convert('RGB')
    w, h = image.size
    assert (w, h) == (1280, 720), (path, image.size)
    return sum(r > 100 and r * 5 > g * 6 and g * 5 > b * 6
               for r, g, b in image.crop(box).getdata())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('captures', type=Path, nargs='+')
    parser.add_argument('--frames', type=int, nargs='+', default=[380, 470],
                        help='Include tutorial caption frames, e.g. 1100 1200 after Moment\'s Reprieve')
    parser.add_argument('--pattern', default='out_{frame:05d}.bmp',
                        help='Capture filename format, also accepts host_shot_{frame}.png')
    args = parser.parse_args()
    for folder in args.captures:
        for frame in args.frames:
            path = folder / args.pattern.format(frame=frame)
            for actor, box, minimum in (
                    ('Sora', (480, 290, 590, 405), 500),
                    ('door', (660, 140, 810, 405), 5000)):
                count = actor_pixels(path, box)
                assert count > minimum, f'{path}: missing {actor} above panel ({count} actor pixels)'
                print(f'PASS {path}: {actor} {count} pixels')


if __name__ == '__main__':
    main()
