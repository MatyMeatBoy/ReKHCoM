#!/usr/bin/env python3
"""Captures of one boss arena at the test widths (WORKFLOW_ARTE_AMPLIO.md section 4), before and after its
extension, from the same executable: x3 integer (zoom=6) at 1920x1080, 2560x1080, 3440x1440 and 3840x1080, plus x1
at 1920x1080. The camera walks left then right so both ends of the arena show. Silent, hidden processes.

    python port/tools/capture_arena.py --battle 198:11 --off KHCOM_NO_DARKSIDE_BEACH [--build build/pc-eu]
        [--region eu] [--riku] [--frames 180,380,600] [--out build/arena/darkside]

Writes <out>/<size>_z<zoom>/{before,after}/out_<frame>.png and a contact sheet <out>/sheet.png (before | after).
"""
import argparse
import os
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SIZES = [("1920x1080", 6), ("2560x1080", 6), ("3440x1440", 6), ("3840x1080", 6), ("1920x1080", 2)]


def run(args, size, zoom, enabled):
    d = Path(args.out) / f"{size}_z{zoom}" / ("after" if enabled else "before")
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    for name in ("khcom.exe", "SDL3.dll"):
        shutil.copy2(ROOT / args.build / name, d / name)
    (d / "kh_com.ini").write_text(f"config_version=11\nview_mode=4\nzoom={zoom}\nhd_text=0\nvsync=0\n")
    frames = [int(f) for f in args.frames.split(",")]
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1", KHCOM_WINDOW=size,
               KHCOM_KEYS=args.keys,
               KHCOM_OUTPUT_SHOT=args.frames, KHCOM_QUIT=str(max(frames) + 10))
    if args.event is not None:
        from test_events import event_lists
        env["KHCOM_EVENT"] = next(f"{l}:{i}" for l, i, n in event_lists() if n.startswith(f"EVENT_{args.event:03d}_"))
    else:
        env["KHCOM_BATTLE"] = f"{args.battle}:{int(args.riku)}"
    if args.off:
        env[args.off] = "0" if enabled else "1"
    if args.on:
        env[args.on] = "1" if enabled else "0"
    rom = {"eu": "B8CP", "us": "B8CE", "jp": "B8CJ"}[args.region]
    subprocess.run([str(d / "khcom.exe"), "--rom", str(ROOT / "roms" / (rom + ".gba"))], cwd=d, env=env,
                   timeout=300, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for f in frames:
        bmp = d / f"out_{f:05d}.bmp"
        if bmp.exists():
            Image.open(bmp).save(bmp.with_suffix(".png"))
            bmp.unlink()
    for name in ("khcom.exe", "SDL3.dll"):
        (d / name).unlink(missing_ok=True)
    log = (d / "kh_com.log").read_text(errors="replace") if (d / "kh_com.log").exists() else ""
    return d, "CRASH" in log or "bios:" in log


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--battle", help="debug battle entry:stage (audit_battle_scenery.py cases)")
    ap.add_argument("--event", type=int, help="a story event number instead (the same background before the fight)")
    ap.add_argument("--off", help="the extension's variable: =1 turns it off (before), =0 on (after)")
    ap.add_argument("--on", help="the extension's variable while it waits for approval: =1 on (after), =0 off")
    ap.add_argument("--build", default="build/pc-eu")
    ap.add_argument("--region", default="eu", choices=("eu", "us", "jp"))
    ap.add_argument("--riku", action="store_true")
    ap.add_argument("--frames", default="180,380,600")
    ap.add_argument("--keys", default="60-63:A,200-380:LEFT,400-600:RIGHT")
    ap.add_argument("--out", required=True)
    ap.add_argument("--jobs", type=int, default=4)
    args = ap.parse_args()

    jobs = [(s, z, e) for s, z in SIZES for e in ((False, True) if args.off or args.on else (False,))]
    with ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(lambda j: run(args, *j), jobs))
    for d, bad in results:
        if bad:
            print(f"CRASH or bios: in {d}")

    rows = []
    frames = args.frames.split(",")
    for s, z in SIZES:
        for f in frames:
            cells = []
            for e in ((False, True) if args.off or args.on else (False,)):
                p = Path(args.out) / f"{s}_z{z}" / ("after" if e else "before") / f"out_{int(f):05d}.png"
                if p.exists():
                    im = Image.open(p).convert("RGB")
                    cells.append(im.resize((640, int(640 * im.height / im.width))))
            if cells:
                rows.append(cells)
    if rows:
        w = 650 * max(len(r) for r in rows)
        h = sum(max(c.height for c in r) + 10 for r in rows)
        sheet = Image.new("RGB", (w, h), (255, 0, 255))
        y = 0
        for r in rows:
            for i, c in enumerate(r):
                sheet.paste(c, (i * 650, y))
            y += max(c.height for c in r) + 10
        sheet.save(Path(args.out) / "sheet.png")
    print(Path(args.out) / "sheet.png")


if __name__ == "__main__":
    main()
