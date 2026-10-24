#!/usr/bin/env python3
"""Every story event that starts a battle and goes on into another event once it is won (34 chains: Leon, Guard Armor,
the Organization, each world's bosses, Agrabah's and Halloween Town's event battles, Riku's...), played as the game
plays them: a room of the right side (Sora or Riku), the event requested as the field does (KHCOM_MODE eventplay),
its dialogues advanced, the battle won (bosskill) and the next event reached. A chain passes when the log shows
the battle and then an event, with no stray access recovered (bios:, which closes Android, the web and Linux ARM) and
no crash. Issue #46 (22/10): Android closed right after Agrabah's first event battle; the debug battle menu and the
debug event menu never take this road.

The chains come from the code (battle_runtime.c's RequestEventMode after a won battle, src/evt/events/*.inc's
battles). --android prints the cases for test_android_scenarios.py's 'story' group instead.

    python port/tools/test_event_battles.py [--build build/pc-eu] [--version eu] [--only 107,110] [--jobs 3]
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ROMS = {'eu': 'B8CP.gba', 'us': 'B8CE.gba', 'jp': 'B8CJ.gba'}
FRAMES = 9000


def chains():
    """(starting event number, battle, event after it) from the code."""
    starts = {}
    for f in glob.glob(str(ROOT / 'src/evt/events/*.inc')):
        for b in re.findall(r'\b(BATTLE_\w+)\s*,', Path(f).read_text(encoding='utf-8', errors='replace')):
            starts.setdefault(b, set()).add(int(os.path.basename(f)[:3]))
    src = (ROOT / 'src/btl/battle_runtime.c').read_text(encoding='utf-8')
    out = []
    for m in re.finditer(r'case (BATTLE_\w+):\s*(?:if[^\n]*\n\s*)?RequestEventMode\((EVENT_\w+)\)', src):
        for ev in sorted(starts.get(m.group(1), ())):
            out.append((ev, m.group(1), m.group(2)))
    return out


def env_for(event):
    """The harness settings of one chain: Riku's events (149..) from Riku's side."""
    room = 'roomriku:10001' if event >= 149 else 'room:10001'
    keys = ','.join(f'{f}-{f + 3}:A' for f in range(160, FRAMES, 40))
    # the event before the field's first random encounter (frame ~270); then, in whatever battle is on, every enemy
    # that can be hit falls to a real hit (bosskill: no battlewin, whose warp left boss parts behind), the player
    # unharmed
    mode = ','.join([f'60:{room}', f'120:eventplay:{event}'] + [f'{f}:bosskill' for f in range(1000, FRAMES, 150)] +
                    [f'{f}:noharm' for f in range(700, FRAMES, 300)])
    return {'KHCOM_MODE': mode, 'KHCOM_KEYS': keys}


def run(build, version, chain):
    event, battle, after = chain
    d = ROOT / 'build' / 'eventbattles' / version / f'{event:03d}'
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / f, d / f)
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_TRACE_MODES='1',
               KHCOM_QUIT=str(FRAMES), KHCOM_WATCHDOG='400', **env_for(event))
    try:
        code = subprocess.run([str(d / 'khcom.exe'), '--rom', str(ROOT / 'roms' / ROMS[version])], cwd=d, env=env,
                              timeout=900, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
    except subprocess.TimeoutExpired:
        code = 'timeout'
    log = (d / 'kh_com.log').read_text(errors='replace') if (d / 'kh_com.log').exists() else ''
    modes = re.findall(r'frame \d+ mode (\S+)', log)
    # the event, then its battle, then an event again
    reached = False
    seen_event = seen_battle = False
    for m in modes:
        if m == 'Event' and not seen_event:
            seen_event = True
        elif m == 'mode_battle' and seen_event and not seen_battle:
            seen_battle = True
        elif m == 'Event' and seen_battle:
            reached = True
            break
    bios = len(re.findall(r'bios:', log))
    crash = 'CRASH' in log
    ok = code == 0 and reached and not bios and not crash
    detail = '' if ok else f'code {code} reached {reached} bios {bios} crash {crash} modes {" ".join(modes[:8])}'
    return chain, ok, detail


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', default='build/pc-eu')
    ap.add_argument('--version', default='eu')
    ap.add_argument('--only')
    ap.add_argument('--jobs', type=int, default=3)
    ap.add_argument('--android', action='store_true')
    args = ap.parse_args()
    todo = [c for c in chains() if not args.only or str(c[0]) in args.only.split(',')]
    if args.android:
        for event, battle, after in todo:
            e = env_for(event)
            print(f'story-{event:03d}-{battle}\t{FRAMES}\tKHCOM_MODE={e["KHCOM_MODE"]}\nKHCOM_KEYS={e["KHCOM_KEYS"]}')
        return 0
    with ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(lambda c: run(ROOT / args.build, args.version, c), todo))
    bad = 0
    for (event, battle, after), ok, detail in results:
        bad += not ok
        print(f'{"ok  " if ok else "FAIL"} {event:03d} {battle:26} -> {after:36} {detail}')
    print(f'test_event_battles {args.version}: {len(results) - bad}/{len(results)} chains')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
