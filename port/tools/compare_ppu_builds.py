#!/usr/bin/env python3
"""A PPU change must not move a pixel: the same events at x1 1920x1080 (four screenshots each) with an old and a new
build, the new one under KHCOM_PPU_VERIFY (several cores against one). Prints identical/DIFFERENT and MISMATCH count.

    python port/tools/compare_ppu_builds.py --old build/pc-eu --new build/pc-test 0:4 0:16 0:57
"""
import os, subprocess, shutil, sys, hashlib
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
def run(build, tag, ev, extra={}):
    d = ROOT / "build/isocmp" / f"{ev.replace(':','_')}_{tag}"
    if d.exists(): shutil.rmtree(d)
    d.mkdir(parents=True)
    for f in ("khcom.exe", "SDL3.dll"): shutil.copy2(ROOT / build / f, d / f)
    (d / "kh_com.ini").write_text("config_version=11\nview_mode=4\nzoom=2\nhd_text=0\nvsync=0\n")
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1", KHCOM_WINDOW="1920x1080",
               KHCOM_EVENT=ev, KHCOM_KEYS=",".join(f"{f}-{f+3}:A" for f in range(30, 700, 40)),
               KHCOM_SHOTS="150,300,450,600", KHCOM_QUIT="610", **extra)
    subprocess.run([str(d / "khcom.exe"), "--rom", str(ROOT / "roms/B8CP.gba")], cwd=d, env=env, timeout=900,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    log = (d / "kh_com.log").read_text(errors="replace")
    return {f.name: hashlib.sha1(f.read_bytes()).hexdigest() for f in sorted(d.glob("shot_*.bmp"))}, log.count("MISMATCH"), "CRASH" in log
import argparse
ap = argparse.ArgumentParser()
ap.add_argument("--old", required=True)
ap.add_argument("--new", required=True)
ap.add_argument("events", nargs="+", help="debug menu list:index")
args = ap.parse_args()
for ev in args.events:
    old = run(args.old, "old", ev)
    new = run(args.new, "new", ev, {"KHCOM_PPU_VERIFY": "1"})
    same = old[0] == new[0] and len(old[0]) == 4
    print(ev, "identical" if same else f"DIFFERENT {old[0]} {new[0]}", "mismatch", new[1], "crash", old[2] or new[2], flush=True)
