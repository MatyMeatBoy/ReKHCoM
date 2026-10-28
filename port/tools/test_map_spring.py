#!/usr/bin/env python3
"""Directed spring transition using real field/collision tasks and ordinary B.

Requires a build with springpose/springlog and an owned ROM; creates a fresh
private data directory. A different generated room is not a reporter reproduction.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--guest', type=Path, help='image for a 64-bit host')
    ap.add_argument('--rom', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--world', type=int, default=5, help='MAP-check world index; 5 is Halloween Town')
    ap.add_argument('--floor', type=int, default=3)
    ap.add_argument('--room', type=int, default=3)
    ap.add_argument('--shape', type=int, default=1)
    ap.add_argument('--seed', type=int, default=11)
    ap.add_argument('--spring', type=int, default=0)
    ap.add_argument('--press-frame', type=int, default=340)
    ap.add_argument('--timeout', type=float, default=120)
    a = ap.parse_args()
    if not (0 <= a.world < 12 and 0 <= a.floor <= 12 and 0 <= a.room < 32 and
            0 <= a.shape < 24 and 0 <= a.seed < 2**31 and a.spring >= 0 and a.press_frame > 235):
        ap.error('Invalid fixture parameters')
    exe, rom, guest, out = a.exe.resolve(), a.rom.resolve(), a.guest.resolve() if a.guest else None, a.out.resolve()
    for path in (exe, rom, *([guest] if guest else [])):
        if not path.is_file():
            ap.error(f'Input not found: {path}')
    # No existing save/settings folder is reused or rewritten.
    out.mkdir(parents=True, exist_ok=False)
    (out / 'kh_com.ini').write_text(
        'config_version=11\ngraphics_mode=1\ngraphics_mode_applied=1\nview_mode=4\nfullscreen=0\n'
        'vsync=0\nexperimental_frame_skip=0\nhd_movies=0\nhd_music=0\nhd_scenes=0\nhd_hud=0\n'
        'hd_text=0\nhd_cards=0\nhd_title=0\ndiscord_presence=0\nundub=0\n', encoding='utf-8')
    commands = [f'60:room:{10000+a.world*100+a.floor}', f'140:roomshape:{a.shape}',
                f'141:roomseed:{a.seed}', f'142:roomgoto:{a.room}', '220:fieldfreeze',
                '221:progress', '222:springlog', f'230:springpose:{a.spring}']
    commands += [f'{a.press_frame+delta}:springlog' for delta in (-6, 6, 20, 35, 60, 90, 130)]
    env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'REKHCOM_', 'SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIO_DRIVER='dummy',
               KHCOM_HOST_HIDDEN='1', KHCOM_HIDDEN='1', KHCOM_FAST='1', KHCOM_BACKGROUND_TEST='1',
               KHCOM_PPU_THREADS='1', KHCOM_TRACE_MODES='1', KHCOM_WINDOW='480x272',
               KHCOM_MODE=','.join(commands), KHCOM_KEYS=f'{a.press_frame}-{a.press_frame+6}:B',
               KHCOM_QUIT=str(a.press_frame+170))
    command = [str(exe)] + ([str(guest)] if guest else []) + ['--rom', str(rom), '--data-dir', str(out), '--language', 'en']
    with (out / 'console.txt').open('w', encoding='utf-8') as stream:
        child = subprocess.Popen(command, cwd=out, env=env, stdout=stream, stderr=subprocess.STDOUT,
                                 creationflags=0x08004000 if os.name == 'nt' else 0)
        (out / 'OWNER.json').write_text(json.dumps(dict(pid=child.pid, command=command)), encoding='utf-8')
        try:
            code = child.wait(timeout=a.timeout)
        except subprocess.TimeoutExpired:
            child.terminate()
            child.wait(timeout=10)
            code = 'timeout'
    log = (out / 'kh_com.log').read_text(encoding='utf-8', errors='replace') if (out / 'kh_com.log').exists() else ''
    pattern = r'spring actor at=(-?\d+),(-?\d+),(-?\d+) ground=(-?\d+) angle=(-?\d+) state=(-?\d+) vz=(-?\d+) target=(-?\d+),(-?\d+),(-?\d+) jump=(-?\d+) springs=(\d+)'
    samples = [tuple(map(int, x)) for x in re.findall(pattern, log)]
    springs = [tuple(map(int, x)) for x in re.findall(r'spring (\d+) at=(-?\d+),(-?\d+),(-?\d+) ground=(-?\d+) height=(-?\d+) angle=(\d+)', log)]
    selected = next((x for x in springs if x[0] == a.spring), None)
    target = selected[4]-selected[5] if selected else None
    ready = len(samples) == 9 and samples[2][-2] > 0
    jumped = any(x[5] == 14 for x in samples)
    landed = bool(samples and selected and samples[-1][2] == samples[-1][3] == target and samples[-1][5] == 0)
    fixture_valid = bool(code == 0 and selected and ready and 'harness: no mode ' not in log)
    passed = fixture_valid and jumped and landed
    result = dict(status='PASS' if passed else 'FAIL' if fixture_valid else 'INVALID_FIXTURE', exit=code,
                  executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  guest_sha256=hashlib.sha256(guest.read_bytes()).hexdigest() if guest else None,
                  rom_sha256=hashlib.sha256(rom.read_bytes()).hexdigest(),
                  parameters={k: v for k, v in vars(a).items() if k not in ('exe', 'guest', 'rom', 'out')},
                  spring_ready_before_input=ready, spring_jump_observed=jumped, upper_ground=target,
                  landed_upper=landed, samples=samples,
                  scope='Directed generated-room transition. No reporter-layout, physical-device, HD or visual certification.')
    (out / 'RESULT.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(result))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
