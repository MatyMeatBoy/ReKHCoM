#!/usr/bin/env python3
"""Start every battle of the game's debug battle menu and report crashes.

Each battle runs in its own hidden process (dummy SDL drivers, isolated folder
under build/battletest/<version>) for a number of frames while A (attack) is
pressed regularly and the stick moves around. Results:
build/battletest/<version>/report.txt and a contact sheet of the last
screenshot of every battle.

    python port/tools/test_battles.py [--version eu|us|jp] [--riku] [--frames 3000] [--jobs 8] [--only 0,10,200]
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

def battle_entries():
    """(index, world, battle id) of the debug battle menu, from mode_chkbtl.c."""
    src = (ROOT / "src" / "debug" / "mode_chkbtl.c").read_text(encoding="utf-8")
    body = re.search(r"sChkBtlEntries\[\d+\] = \{(.*?)\n\};", src, re.S).group(1)
    rows = re.findall(r"\{\s*(\w+),\s*\d+,\s*(\w+),", body)
    return [(i, w, b) for i, (w, b) in enumerate(rows)]


def stage_table():
    src = (ROOT / "src" / "debug" / "mode_chkbtl.c").read_text(encoding="utf-8")
    body = re.search(r"sChkBtlWorlds\[\d*\] = \{(.*?)\n\};", src, re.S).group(1)
    return re.findall(r"\{\s*(\w+),", body)


def run(exe_dir, work, version, index, stage, frames, timeout, riku=False, stock=False):
    d = work / f"{index:03d}"
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    case_bin.place(exe_dir, work, d)
    (d / "kh_com.ini").write_text("config_version=2\n")
    keys = ["60-63:A"]
    dirs = ("RIGHT", "LEFT", "UP", "DOWN")
    for n, f in enumerate(range(300, frames, 24)):
        if stock:  # stocking cards (L+R) over and over, standing still while the enemies hit (KH_COM_VITA #12)
            keys.append(f"{f}-{f + 3}:{'L+R' if n % 3 else 'R'}")
            if n % 6 == 5:
                keys.append(f"{f + 8}-{f + 11}:A")
            continue
        keys.append(f"{f}-{f + 3}:A")
        if n % 4 == 0:
            keys.append(f"{f + 6}-{f + 20}:{dirs[(n // 4) % 4]}")
    shots = ",".join(str(f) for f in (frames // 3, frames * 2 // 3, frames))
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1",
               KHCOM_BATTLE=f"{index}:{stage}:{int(riku)}", KHCOM_KEYS=",".join(keys), KHCOM_SHOTS=shots,
               KHCOM_QUIT=str(frames + 5))
    rom = ROOT / "roms" / ({"eu": "B8CP", "us": "B8CE", "jp": "B8CJ"}[version] + ".gba")
    try:
        p = subprocess.run([str(d / "khcom.exe"), "--language", "es", "--rom", str(rom)], cwd=d, env=env,
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           timeout=timeout)
        code = p.returncode
    except subprocess.TimeoutExpired:
        code = "timeout"
    log = (d / "kh_com.log").read_text(errors="replace") if (d / "kh_com.log").exists() else ""
    crash = "CRASH" in log
    bios = sorted(set(re.findall(r"bios: \w+ \w+ at eip (\w+)", log)))
    m4a = "m4a:" in log
    return code, crash, bios, m4a


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default="eu", choices=("eu", "us", "jp"))
    ap.add_argument("--frames", type=int, default=3000)
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--only", help="comma list of entry indices")
    ap.add_argument("--riku", action="store_true", help="fight as Riku (Reverse/Rebirth)")
    ap.add_argument("--stock", action="store_true", help="stock cards (L+R) all along while taking hits")
    ap.add_argument("--build", help="folder with khcom.exe (default build/pc or build/pc-<version>)")
    args = ap.parse_args()

    exe_dir = ROOT / (args.build or ("build/pc" if args.version == "eu" else f"build/pc-{args.version}"))
    work = ROOT / "build" / "battletest" / (args.version + ("_riku" if args.riku else "") + ("_stock" if args.stock else ""))
    work.mkdir(parents=True, exist_ok=True)
    stages = stage_table()
    entries = battle_entries()
    if args.only:
        wanted = {int(x) for x in args.only.split(",")}
        entries = [e for e in entries if e[0] in wanted]

    def stage_for(world):
        """The debug menu's stage row for the entry's own world."""
        for i, s in enumerate(stages):
            if s.replace("BATTLE_STAGE_", "WORLD_") == world:
                return i
        return 0

    results = {}
    with ThreadPoolExecutor(args.jobs) as pool:
        futures = {pool.submit(run, exe_dir, work, args.version, i, stage_for(w), args.frames, args.timeout, args.riku, args.stock): (i, w, b)
                   for i, w, b in entries}
        for fut, (i, w, b) in futures.items():
            results[i] = (w, b) + fut.result()

    lines = []
    bad = 0
    for i, (world, battle, code, crash, bios, m4a) in sorted(results.items()):
        status = "CRASH" if crash else ("TIMEOUT" if code == "timeout" else ("EXIT %s" % code if code else "ok"))
        if status != "ok":
            bad += 1
        lines.append(f"{i:3d} {battle:32s} {world:24s} {status:10s} "
                     + (f"bios sites: {' '.join(bios)} " if bios else "") + ("m4a pointer left the ROM" if m4a else ""))
    lines.append(f"{len(results) - bad}/{len(results)} battles ok")
    hashes = {}
    for i in results:
        shots = sorted((work / f"{i:03d}").glob("shot_*.bmp"))
        if shots:
            h = hashlib.sha1(shots[-1].read_bytes()).hexdigest()
            hashes[h] = hashes.get(h, 0) + 1
    if hashes and max(hashes.values()) > len(results) // 4:
        lines.append(f"WARNING: {max(hashes.values())} battles end on the same picture; were they started?")
    (work / "report.txt").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))

    try:
        from PIL import Image
        cells = []
        for i in sorted(results):
            shots = sorted((work / f"{i:03d}").glob("shot_*.bmp"))
            if shots:
                cells.append(Image.open(shots[-1]).convert("RGB").resize((160, 90)))
        cols = 16
        sheet = Image.new("RGB", (cols * 164, ((len(cells) + cols - 1) // cols) * 94), (30, 30, 30))
        for n, im in enumerate(cells):
            sheet.paste(im, ((n % cols) * 164, (n // cols) * 94))
        if cells:
            sheet.save(work / "battles.png")
    except ImportError:
        pass


if __name__ == "__main__":
    main()
