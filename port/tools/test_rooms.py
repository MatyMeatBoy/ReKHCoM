#!/usr/bin/env python3
"""Room battery: every world and every castle hall, entered as the debug
menu's MAP check does (KHCOM_MODE=<frame>:room:<mode*10000+world*100+floor>).

  mode 1  the floor's world (13 worlds, on an early and a late floor)
  mode 2  the castle's entrance hall of each floor
  mode 3  the castle's exit hall of each floor

In each run Sora walks in turn to each side and presses A now and then, so
rooms are crossed, door menus open and battles start. A case fails when the
process does not exit normally or the log reports a fault. Shots at the
middle and the end go to a contact sheet for the widescreen review.

    python port/tools/test_rooms.py [--version eu] [--build build/pc] [--jobs 4] [--frames 3000]
"""
import argparse
import json
import os
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from PIL import Image

import case_bin

ROOT = Path(__file__).resolve().parents[2]
WORLDS = 13
FLOORS = 13
START = 300


def cases():
    out = []
    for world in range(WORLDS):
        for floor in (0, 9):
            out.append((1, world, floor))
    for floor in range(FLOORS):
        out.append((2, 0, floor))
        out.append((3, 0, floor))
    return out


def keys(frames):
    dirs = ("RIGHT", "DOWN", "LEFT", "UP", "RIGHT+DOWN", "LEFT+UP")
    out = []
    f, n = START + 120, 0
    while f < frames:
        out.append(f"{f}-{f + 90}:{dirs[n % len(dirs)]}")
        out.append(f"{f + 95}-{f + 98}:A")
        f += 130
        n += 1
    return ",".join(out)


def run(exe_dir, work, case, frames, timeout, resume=False, version="eu"):
    mode, world, floor = case
    name = f"room_{mode}_{world:02d}_{floor:02d}"
    d = work / name
    mid = START + (frames - START) // 2
    shots = [d / f"shot_{mid:05d}.bmp", d / f"shot_{frames:05d}.bmp"]
    # each finished case leaves result.json: --resume keeps it and goes on from the first case without one
    if resume and (d / "result.json").exists():
        try:
            status, modes, faults = json.loads((d / "result.json").read_text())
            return name, status, modes, faults, [s for s in shots if s.exists()]
        except (ValueError, OSError):
            pass
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    case_bin.place(exe_dir, work, d)
    (d / "kh_com.ini").write_text("config_version=2\nview_mode=4\n")
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1",
               KHCOM_MODE=f"{START}:room:{mode * 10000 + world * 100 + floor}", KHCOM_KEYS=keys(frames),
               KHCOM_SHOTS=f"{mid},{frames}", KHCOM_QUIT=str(frames + 1), KHCOM_TRACE_MODES="1")
    try:
        rom = ROOT / "roms" / ({"eu": "B8CP", "us": "B8CE", "jp": "B8CJ"}[version] + ".gba")
        code = subprocess.run([str(d / "khcom.exe"), "--rom", str(rom)], cwd=str(d), env=env, timeout=timeout,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
    except subprocess.TimeoutExpired:
        code = "timeout"
    log = (d / "kh_com.log").read_text(errors="replace") if (d / "kh_com.log").exists() else ""
    faults = [l for l in log.splitlines() if "fatal" in l.lower() or "crash" in l.lower() or "exception" in l.lower()]
    modes = []
    for l in log.splitlines():
        if l.startswith("harness: frame") and " mode " in l:
            m = l.split(" mode ")[1]
            if m not in modes:
                modes.append(m)
    status = "ok" if code == 0 and not faults else (f"exit {code}" if code != 0 else "fault")
    (d / "result.tmp").write_text(json.dumps([status, modes, faults[:2]]))
    os.replace(d / "result.tmp", d / "result.json")
    return name, status, modes, faults[:2], [s for s in shots if s.exists()]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default="eu", choices=("eu", "us", "jp"))
    ap.add_argument("--build", help="folder with khcom.exe (default build/pc or build/pc-<version>)")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--frames", type=int, default=3000)
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--resume", action="store_true", help="keep the cases that already finished (their result.json)")
    args = ap.parse_args()
    exe_dir = ROOT / (args.build or ("build/pc" if args.version == "eu" else f"build/pc-{args.version}"))
    # one folder of cases per version (they used to share build/, so two versions could not run side by side)
    work = ROOT / "build" / f"roomtest_{args.version}"
    cases_dir = ROOT / "build" / f"roomcases_{args.version}"
    cases_dir.mkdir(parents=True, exist_ok=True)
    results = []
    with ThreadPoolExecutor(args.jobs) as pool:
        futures = [pool.submit(run, exe_dir, cases_dir, c, args.frames, args.timeout, args.resume,
                               args.version) for c in cases()]
        for k, fut in enumerate(as_completed(futures), 1):
            results.append(fut.result())
            print(f"[{k}/{len(futures)}] {results[-1][0]} {results[-1][1]}", flush=True)
    results.sort()
    lines, thumbs = [], []
    for name, status, modes, faults, shots in results:
        lines.append(f"{name:18s} {status:10s} {' > '.join(modes)}{'  ' + ' | '.join(faults) if faults else ''}")
        thumbs.append((name, shots))
    ok = sum(1 for r in results if r[1] == "ok")
    lines.append(f"{ok}/{len(results)} rooms ok")
    work.mkdir(parents=True, exist_ok=True)
    (work / "report.txt").write_text("\n".join(lines) + "\n")
    # contact sheet: two shots per case
    tw, th, cols = 160, 90, 6
    sheet = Image.new("RGB", (tw * cols, th * ((len(thumbs) * 2 + cols - 1) // cols)), "black")
    k = 0
    for name, shots in thumbs:
        for s in shots:
            sheet.paste(Image.open(s).convert("RGB").resize((tw, th)), ((k % cols) * tw, (k // cols) * th))
            k += 1
    sheet.save(work / "rooms.png")
    shutil.rmtree(cases_dir, ignore_errors=True)
    print("\n".join(l for l in lines if " ok " not in l))
    print(f"report: {work / 'report.txt'}")


if __name__ == "__main__":
    main()
