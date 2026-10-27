"""Enter a real door's synthesis UI in each view; reject a text-only black frame.

Private cartridge dumps required. Runs hidden in fresh directories. Room setup
uses a fixture; striking the door, accepting its prompt and leaving use real keys.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import datetime
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-prefix', default='build/nightly3-')
    p.add_argument('--versions', nargs='+', choices=('eu', 'us', 'jp'), default=['eu', 'us', 'jp'])
    p.add_argument('--views', nargs='+', type=int, choices=range(5), default=list(range(5)))
    p.add_argument('--hd', nargs='+', type=int, choices=(0, 1), default=[1])
    p.add_argument('--jobs', type=int, default=2)
    p.add_argument('--baseline', type=Path, help='Optional previous EU captures: viewN/470.png')
    args = p.parse_args()
    out = ROOT/'build/door-view-regressions'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S_%f')
    out.mkdir(parents=True)

    def run(case):
        region, view, hd = case
        d = out/f'{region}-view{view}-hd{hd}'
        d.mkdir()
        for name in ('khcom.exe', 'SDL3.dll'):
            shutil.copy2(ROOT/(args.build_prefix+region)/name, d/name)
        shutil.copytree(ROOT/'port/assets/fonts', d/'assets/fonts')
        (d/'kh_com.ini').write_text(f'config_version=11\nview_mode={view}\nzoom=8\nhd_text={hd}\n')
        env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
        env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
                   KHCOM_WINDOW='1280x720', KHCOM_MODE='60:room:10901,120:fieldfreeze,200:doorpose:0',
                   KHCOM_KEYS='240-242:A,420-422:A,500-502:B,560-562:B',
                   KHCOM_OUTPUT_SHOT='280,380,470,650', KHCOM_QUIT='660', KHCOM_TRACE_MODES='1')
        rom = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}[region]
        result = subprocess.run([str(d/'khcom.exe'), '--rom', str(ROOT/'roms'/(rom+'.gba')),
                                 '--language', 'es'], cwd=d, env=env, timeout=180,
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        log = (d/'kh_com.log').read_text(errors='replace')
        assert result.returncode == 0 and 'CRASH' not in log, (case, result.returncode)
        assert 'harness: door pose' in log and 'door pose unavailable' not in log.lower(), (case, log[-2000:])
        for frame in (280, 380, 470, 650):
            im = Image.open(d/f'out_{frame:05d}.bmp').convert('RGB')
            im.save(d/f'{frame}.png')
        pic = Image.open(d/'470.png').convert('RGB')
        # HD letters are neutral white/gray. The original card, panel and
        # cursor have coloured pixels, which were all missing in issue #16.
        colored = sum(count for count, rgb in pic.getcolors(pic.width*pic.height)
                      if max(rgb)-min(rgb) > 20)
        assert colored > 10000, (case, 'synthesis graphics missing', colored)
        trace = [(int(f), mode) for f, mode in re.findall(r'harness: frame (\d+) mode (\S+)', log)]
        # Synthesis is a task inside Mode_MapFld, so it does not switch modes.
        assert trace and trace[-1][1] == 'Mode_MapFld', (case, trace)
        unchanged = None
        if args.baseline and region == 'eu' and hd == 1:
            returned = Image.open(d/'650.png').convert('RGB')
            before = Image.open(args.baseline/f'view{view}/650.png').convert('RGB')
            assert before.size == returned.size and before.tobytes() == returned.tobytes(), 'Field return changed'
            if view == 4:
                before = Image.open(args.baseline/'view4/470.png').convert('RGB')
                unchanged = before.size == pic.size and before.tobytes() == pic.tobytes()
                assert unchanged, 'Automatic view changed'
        row = dict(region=region, view=view, hd=hd, colored_pixels=colored,
                   field_mode=True, automatic_unchanged=unchanged, passed=True)
        print(row, flush=True)
        return row

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        rows = list(pool.map(run, [(r,v,h) for r in args.versions for v in args.views for h in args.hd]))
    (out/'report.json').write_text(json.dumps(rows, indent=2))
    print(out)
    print(f'{len(rows)}/{len(rows)} door view cases PASS')


if __name__ == '__main__':
    main()
