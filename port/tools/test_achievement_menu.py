"""Render and navigate Options > Achievements using the production SDL handler.

Isolated synthetic progress (one unlock in a launcher-style account snapshot);
never reads or writes player saves. Sequential runs and the dummy SDL backend
keep this suitable while the user is playing.

Controls checked: up/down move one row, left/right jump five rows, L/R (Q/E on
the keyboard, shoulders on a pad) change the All/Locked/Unlocked tab, B goes back.

    python port/tools/test_achievement_menu.py
"""
from pathlib import Path
import json
import os
import shutil
import struct
import subprocess
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/achievement-menu'
OUT.mkdir(parents=True, exist_ok=True)
ATLAS = ROOT / 'port/assets/achievements/badges.png'
FIRST_ID = 15580  # first achievement of the official set
DOWN, UP, LEFT, RIGHT = 1073741905, 1073741906, 1073741904, 1073741903
KEYS = f'1:{DOWN},2:{RIGHT},3:{RIGHT},4:{LEFT},5:{UP},6:{UP},7:101,8:101,9:113,10:8'
# SDL gamepad buttons are passed as -(button + 1): EAST 1, LEFT_SHOULDER 9,
# RIGHT_SHOULDER 10, DPAD_UP 11, DPAD_DOWN 12, DPAD_LEFT 13, DPAD_RIGHT 14.
PAD = '1:-13,2:-15,3:-15,4:-14,5:-12,6:-12,7:-11,8:-11,9:-10,10:-2'
STATES = ['filter=0 choice=1', 'filter=0 choice=6', 'filter=0 choice=11', 'filter=0 choice=5',
          'filter=0 choice=4', 'filter=1 choice=0', 'filter=2 choice=0']

report = []
for region, width, height, controls in [('eu', 240, 160, 'keyboard'), ('eu', 640, 360, 'keyboard'),
        ('eu', 1920, 1080, 'keyboard'), ('us', 1920, 1080, 'keyboard'),
        ('jp', 1920, 1080, 'keyboard'), ('us', 640, 360, 'gamepad')]:
    work = OUT / (f'{region}-{width}x{height}' + ('-pad' if controls == 'gamepad' else ''))
    if work.exists():
        shutil.rmtree(work)
    (work / 'cache').mkdir(parents=True)
    (work / 'assets/achievements').mkdir(parents=True)
    shutil.copy(ATLAS, work / 'assets/achievements/badges.png')
    (work / 'cache/achievement-unlocks.dat').write_bytes(b'KHRAPRG1' + struct.pack('<III', 1, 1, FIRST_ID))
    (work / 'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\n'
        'integer_zoom=1\nwindow_scale=1\nfullscreen=0\nvsync=0\n')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
        KHCOM_WINDOW=f'{width}x{height}', KHCOM_OPTIONS_TOGGLE='100', KHCOM_OPTIONS_SHOT='999999',
        KHCOM_OPTIONS_HOLD='15', KHCOM_ACHIEVEMENTS_SHOT='1', KHCOM_QUIT='101',
        KHCOM_ACHIEVEMENT_KEYS=PAD if controls == 'gamepad' else KEYS)
    rom = ROOT / ('roms/' + {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}[region] + '.gba')
    subprocess.run([str(ROOT / f'build/pc-{region}/khcom.exe'), '--rom', str(rom), '--data-dir', str(work),
                    '--language', 'es'], cwd=work, env=env, check=True, timeout=60)
    log = (work / 'kh_com.log').read_text(encoding='utf-8', errors='replace')
    for state in STATES:
        assert state in log, (region, controls, state)
    shots = sorted(work.glob('options_*.bmp'))
    assert len(shots) >= 8, (region, len(shots))
    for shot in shots:
        with Image.open(shot) as im:
            im.save(shot.with_suffix('.png'))
    # Shot 8 (Unlocked tab) shows the earned first achievement: its badge in colour;
    # shot 7 (Locked tab) shows a locked one: its badge in grey.
    def colour_pixels(tick):
        # The suffix is the main Options cursor, which changes as rows are added.
        # The navigation tick identifies the state being checked.
        captures = list(work.glob(f'options_{tick:04d}_*.png'))
        assert len(captures) == 1, (region, tick, captures)
        with Image.open(captures[0]) as im:
            im = im.convert('RGB')
            corner = im.crop((im.width * 3 // 4, im.height // 2, im.width, im.height))
            return sum(1 for p in corner.get_flattened_data() if max(p) - min(p) > 40 and p != (255, 0, 0))
    assert colour_pixels(8) > 200, (region, 'earned badge is not drawn in colour')
    assert colour_pixels(7) < 50, (region, 'locked badge is not grey')
    assert not list(work.glob('achievements_*.dat')), 'Browsing must not change awards'
    report.append(dict(region=region, output=[width, height], controls=controls, navigation='PASS', readonly='PASS'))
(OUT / 'report.json').write_text(json.dumps(report, indent=2))
print(f'PASS read-only achievement menu: {len(report)} region/size/control pairs, rows, 5-row jumps, L/R tabs, badge and back')
