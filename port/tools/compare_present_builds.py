#!/usr/bin/env python3
"""A presentation change must not move a pixel of what the window shows: field, event, iso floor scene, battle, the
Options open over the field and the title, at x1 and x4 1920x1080, five KHCOM_OUTPUT_SHOT captures each, old build
against new.

    python port/tools/compare_present_builds.py build/pc-eu build/pc-test
"""
import os, subprocess, shutil, hashlib, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
A = ",".join(f"{f}-{f+3}:A" for f in range(30, 700, 40))
CASES = {
    "field": {"KHCOM_MODE": "60:room:10901", "KHCOM_KEYS": "200-260:RIGHT,300-360:DOWN"},
    "event": {"KHCOM_EVENT": "0:16", "KHCOM_KEYS": A},
    "iso": {"KHCOM_EVENT": "0:4", "KHCOM_KEYS": A},
    "battle": {"KHCOM_BATTLE": "0:1:0", "KHCOM_KEYS": "150-153:A"},
    "options": {"KHCOM_MODE": "60:room:10901", "KHCOM_OPTIONS_AT": "300", "KHCOM_OPTIONS_HOLD": "200"},
    "title": {},
}
def run(build, tag, name, zoom):
    d = ROOT / "build/presentcmp" / f"{name}_z{zoom}_{tag}"
    if d.exists(): shutil.rmtree(d)
    d.mkdir(parents=True)
    for f in ("khcom.exe", "SDL3.dll"): shutil.copy2(ROOT / build / f, d / f)
    (d / "kh_com.ini").write_text(f"config_version=11\nview_mode=4\nzoom={zoom}\nhd_text=0\nvsync=0\n")
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1", KHCOM_WINDOW="1920x1080",
               KHCOM_OUTPUT_SHOT="100,250,350,450,600", KHCOM_QUIT="610", **CASES[name])
    subprocess.run([str(d / "khcom.exe"), "--rom", str(ROOT / "roms/B8CP.gba")], cwd=d, env=env, timeout=900,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return {f.name: hashlib.sha1(f.read_bytes()).hexdigest() for f in sorted(d.glob("out_*.bmp"))}
for name in CASES:
    for zoom in (2, 8):
        o, n = run(sys.argv[1], "old", name, zoom), run(sys.argv[2], "new", name, zoom)
        print(name, f"z{zoom}", "identical" if o == n and o else f"DIFFERENT {sorted(k for k in o if o.get(k) != n.get(k))} ({len(o)} shots)", flush=True)
