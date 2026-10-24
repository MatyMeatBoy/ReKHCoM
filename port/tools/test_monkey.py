#!/usr/bin/env python3
"""Play the game unattended with random, reproducible input and report crashes.

Each case starts at a point of the story (an event of the debug event menu,
which leaves the player in that floor's field afterwards) or from a save
file, then plays with KHCOM_MONKEY input: walking, attacking, rolling,
changing cards, opening and leaving the pause menu, going through doors and
fighting whatever it meets. Every case runs in its own hidden process under
build/monkeytest/<version>.

A case fails when the game crashes, exits with an error, stops responding
(timeout), or shows the same picture for the whole second half of the run
(stuck while input keeps coming).

    python port/tools/test_monkey.py [--version eu|us|jp] [--minutes 10] [--jobs 4]
                                     [--cases 0:12,1:20] [--seeds 2] [--save path/to/kh_com.sav]
"""
import argparse
import hashlib
import os
import re
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import case_bin

ROOT = Path(__file__).resolve().parents[2]
FPS = 59.7275
ROMS = {"eu": "B8CP", "us": "B8CE", "jp": "B8CJ"}


def event_lists():
    """(list, index, name) for the Sora and Riku menus, from mode_eventselect.c."""
    src = (ROOT / "src" / "evt" / "mode_eventselect.c").read_text(encoding="utf-8")
    out = []
    for list_id, name in ((0, "sSoraEventIds"), (1, "sRikuEventIds")):
        body = re.search(name + r"\[\d+\] = \{(.*?)\};", src, re.S).group(1)
        ids = [t.strip() for t in body.split(",") if t.strip() and t.strip() != "EVENT_END"]
        out += [(list_id, i, ev) for i, ev in enumerate(ids)]
    return out


def default_cases():
    """One field entry per floor: the events whose name marks a floor's entrance."""
    cases = []
    for list_id, index, name in event_lists():
        if "ENTRANCE" in name:
            cases.append((list_id, index, name))
    return cases


def run(exe_dir, work, version, case, seed, frames, timeout, save):
    tag = case if isinstance(case, str) else f"{case[0]}_{case[1]:03d}"
    d = work / f"{tag}_s{seed}"
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    case_bin.place(exe_dir, work, d)
    (d / "kh_com.ini").write_text("config_version=2\n")
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1")
    step = max(600, frames // 20)
    shots = list(range(step, frames + 1, step))
    if save:
        shutil.copy(save, d / ("kh_com.sav" if version == "eu" else f"kh_com_{version}.sav"))
        # title: A through the screens and the file select into the save
        title = ",".join(f"{f}-{f + 3}:A" for f in range(200, 1400, 50))
        env.update(KHCOM_KEYS=title, KHCOM_MONKEY=f"{seed}:1500")
    else:
        # the event itself: A every 40 frames to advance it, then the monkey
        env.update(KHCOM_EVENT=f"{case[0]}:{case[1]}",
                   KHCOM_KEYS=",".join(f"{f}-{f + 3}:A" for f in range(30, 3000, 40)),
                   KHCOM_MONKEY=f"{seed}:3000")
    env.update(KHCOM_SHOTS=",".join(map(str, shots)), KHCOM_QUIT=str(frames + 5))
    rom = ROOT / "roms" / (ROMS[version] + ".gba")
    try:
        p = subprocess.run([str(d / "khcom.exe"), "--language", "es", "--rom", str(rom)], cwd=d, env=env,
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           timeout=timeout)
        code = p.returncode
    except subprocess.TimeoutExpired:
        code = "timeout"
    log = (d / "kh_com.log").read_text(errors="replace") if (d / "kh_com.log").exists() else ""
    crash = "CRASH" in log
    taken = sorted(d.glob("shot_*.bmp"))
    half = taken[len(taken) // 2:]
    stuck = len(half) >= 4 and len({hashlib.sha1(f.read_bytes()).hexdigest() for f in half}) == 1
    distinct = len({hashlib.sha1(f.read_bytes()).hexdigest() for f in taken})
    return tag, seed, code, crash, stuck, distinct, len(taken), len(shots)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default="eu", choices=tuple(ROMS))
    ap.add_argument("--minutes", type=float, default=10, help="game minutes per case")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--seeds", type=int, default=1, help="seeds per case")
    ap.add_argument("--cases", help="comma list of list:index (default: every floor entrance)")
    ap.add_argument("--save", help="start from this save file instead of events")
    ap.add_argument("--build", help="folder with khcom.exe (default build/pc or build/pc-<version>)")
    args = ap.parse_args()

    exe_dir = ROOT / (args.build or ("build/pc" if args.version == "eu" else f"build/pc-{args.version}"))
    work = ROOT / "build" / "monkeytest" / args.version
    work.mkdir(parents=True, exist_ok=True)
    frames = int(args.minutes * 60 * FPS)
    if args.save:
        cases = ["save"]
    elif args.cases:
        names = {(l, i): n for l, i, n in event_lists()}
        cases = [(int(a), int(b), names.get((int(a), int(b)), "?"))
                 for a, b in (c.split(":") for c in args.cases.split(","))]
    else:
        cases = default_cases()
    timeout = max(600, frames // 20)

    jobs = [(c, s) for c in cases for s in range(1, args.seeds + 1)]
    results = []
    with ThreadPoolExecutor(args.jobs) as pool:
        futures = [pool.submit(run, exe_dir, work, args.version, c, s, frames, timeout, args.save) for c, s in jobs]
        for (c, _), fut in zip(jobs, futures):
            results.append((c,) + fut.result())

    lines, bad = [], 0
    for case, tag, seed, code, crash, stuck, distinct, taken, wanted in results:
        if crash:
            status = "CRASH"
        elif code == "timeout":
            status = "TIMEOUT"
        elif code:
            status = f"EXIT {code}"
        elif taken < wanted:
            status = "SHORT"
        elif stuck:
            status = "STUCK"
        else:
            status = "ok"
        bad += status != "ok"
        name = case if isinstance(case, str) else case[2]
        lines.append(f"{tag:8s} seed {seed}  {name:36s} {status:8s} {distinct:2d}/{taken} distinct pictures")
    lines.append(f"{len(results) - bad}/{len(results)} cases ok, {args.minutes:g} game minutes each "
                 f"({len(results) * args.minutes:g} minutes in all)")
    (work / "report.txt").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))

    try:
        from PIL import Image
        cells = []
        for case, tag, seed, *_ in results:
            taken = sorted((work / f"{tag}_s{seed}").glob("shot_*.bmp"))
            for f in taken[-4:]:
                cells.append(Image.open(f).convert("RGB").resize((160, 90)))
        cols = 8
        sheet = Image.new("RGB", (cols * 164, ((len(cells) + cols - 1) // cols) * 94), (30, 30, 30))
        for n, im in enumerate(cells):
            sheet.paste(im, ((n % cols) * 164, (n // cols) * 94))
        if cells:
            sheet.save(work / "monkey.png")
    except ImportError:
        pass


if __name__ == "__main__":
    main()
