#!/usr/bin/env python3
"""Start every story event from the game's debug event menu and report crashes.

Each event runs in its own hidden process (dummy SDL drivers, isolated folder
under build/eventtest/<version>) for a number of frames while A is pressed
regularly to advance text and battles. Results: build/eventtest/<version>/report.txt
and a contact sheet of the last screenshot of every event.

    python port/tools/test_events.py [--version eu|us|jp] [--wide] [--frames 4000] [--jobs 8] [--only 0:8,1:3]
"""
import argparse
import os
import json
import re
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import case_bin

ROOT = Path(__file__).resolve().parents[2]


def event_lists():
    """(list, index, name) for the Sora and Riku menus, from mode_eventselect.c."""
    src = (ROOT / "src" / "evt" / "mode_eventselect.c").read_text(encoding="utf-8")
    out = []
    for list_id, name in ((0, "sSoraEventIds"), (1, "sRikuEventIds")):
        body = re.search(name + r"\[\d+\] = \{(.*?)\};", src, re.S).group(1)
        ids = [t.strip() for t in body.split(",") if t.strip() and t.strip() != "EVENT_END"]
        out += [(list_id, i, ev) for i, ev in enumerate(ids)]
    return out


def prune_shots(d, wide):
    """A clean case keeps its log and result.json; its screenshots go (--prune), leaving the last one's hash and a
    thumbnail for the report's checks, so long batteries do not fill the disk."""
    import hashlib
    shots = sorted(d.glob("shot_*.bmp"))
    if not shots:
        return
    (d / "last_shot.sha1").write_text(hashlib.sha1(shots[-1].read_bytes()).hexdigest())
    try:
        from PIL import Image
        Image.open(shots[-1]).convert("RGB").resize((160, 90) if wide else (120, 80)).save(d / "thumb.png")
    except Exception:
        pass
    for shot in shots:
        shot.unlink()
    # the exe copy only serves to symbolize fault sites, which a clean case has none of (a full copy across drives)
    for name in ("khcom.exe", "SDL3.dll"):
        (d / name).unlink(missing_ok=True)


def run(exe_dir, work, version, list_id, index, frames, timeout, wide=False, resume=False, prune=False):
    d = work / f"{list_id}_{index:03d}"
    # each finished case leaves result.json: --resume keeps it and goes on from the first case without one
    if resume and (d / "result.json").exists():
        try:
            code, crash, bios = json.loads((d / "result.json").read_text())
            return code, crash, bios
        except (ValueError, OSError):
            pass
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    case_bin.place(exe_dir, work, d)
    (d / "kh_com.ini").write_text(f"config_version=2\nview_mode={4 if wide else 0}\n")
    keys = ",".join(f"{f}-{f + 3}:A" for f in range(30, frames, 40))
    shots = ",".join(str(f) for f in (frames // 4, frames // 2, frames * 3 // 4, frames))
    env = {"SDL_VIDEO_DRIVER": "dummy", "SDL_AUDIO_DRIVER": "dummy", "KHCOM_FAST": "1",
           "KHCOM_EVENT": f"{list_id}:{index}", "KHCOM_KEYS": keys, "KHCOM_SHOTS": shots,
           "KHCOM_QUIT": str(frames + 5)}
    full_env = dict(os.environ, **env)
    try:
        rom = ROOT / "roms" / {"eu": "B8CP", "us": "B8CE", "jp": "B8CJ"}[version]
        p = subprocess.run([str(d / "khcom.exe"), "--language", "es", "--rom", str(rom) + ".gba"], cwd=d, env=full_env,
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           timeout=timeout)
        code = p.returncode
    except subprocess.TimeoutExpired:
        code = "timeout"
    log = (d / "kh_com.log").read_text(errors="replace") if (d / "kh_com.log").exists() else ""
    crash = "CRASH" in log
    bios = sorted(set(re.findall(r"bios: \w+ \w+ at eip (\w+)", log)))
    (d / "result.tmp").write_text(json.dumps([code, crash, bios]))
    os.replace(d / "result.tmp", d / "result.json")
    if prune and code == 0 and not crash and not bios and "MISMATCH" not in log:
        prune_shots(d, wide)
    return code, crash, bios


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default="eu", choices=("eu", "us", "jp"))
    ap.add_argument("--frames", type=int, default=4000)
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--only", help="comma list of list:index")
    ap.add_argument("--build", help="folder with khcom.exe (default build/pc or build/pc-<version>)")
    ap.add_argument("--wide", action="store_true", help="16:9 auto view instead of the GBA picture")
    ap.add_argument("--resume", action="store_true", help="keep the cases that already finished (their result.json)")
    ap.add_argument("--prune", action="store_true",
                    help="delete a clean case's screenshots once it finishes (keeps log, result.json, hash, thumbnail)")
    ap.add_argument("--work", help="folder for the cases (default build/eventtest/<version>): keeps a side run apart")
    args = ap.parse_args()

    exe_dir = ROOT / (args.build or ("build/pc" if args.version == "eu" else f"build/pc-{args.version}"))
    work = Path(args.work) if args.work else ROOT / "build" / "eventtest" / (args.version + ("_wide" if args.wide else ""))
    work.mkdir(parents=True, exist_ok=True)
    events = event_lists()
    if args.only:
        wanted = {tuple(int(x) for x in s.split(":")) for s in args.only.split(",")}
        events = [e for e in events if (e[0], e[1]) in wanted]

    results = {}
    with ThreadPoolExecutor(args.jobs) as pool:
        futures = {pool.submit(run, exe_dir, work, args.version, l, i, args.frames, args.timeout, args.wide,
                               args.resume, args.prune): (l, i, n) for l, i, n in events}
        for k, fut in enumerate(as_completed(futures), 1):
            l, i, n = futures[fut]
            results[(l, i)] = (n,) + fut.result()
            print(f"[{k}/{len(futures)}] {l}:{i} {n} {'ok' if results[(l, i)][1] == 0 and not results[(l, i)][2] else 'BAD'}",
                  flush=True)

    lines = []
    bad = 0
    for (l, i), (name, code, crash, bios) in sorted(results.items()):
        status = "CRASH" if crash else ("TIMEOUT" if code == "timeout" else ("EXIT %s" % code if code else "ok"))
        if status != "ok":
            bad += 1
        lines.append(f"{'Sora' if l == 0 else 'Riku'} {i:3d} {name:40s} {status:10s} "
                     + (f"bios sites: {' '.join(bios)}" if bios else ""))
    lines.append(f"{len(results) - bad}/{len(results)} events ok")
    # Sanity check: distinct events end on distinct pictures. Many identical
    # last screenshots mean the harness did not start the events at all.
    import hashlib
    hashes = {}
    for (l, i) in results:
        shots = sorted((work / f"{l}_{i:03d}").glob("shot_*.bmp"))
        pruned = work / f"{l}_{i:03d}" / "last_shot.sha1"
        h = hashlib.sha1(shots[-1].read_bytes()).hexdigest() if shots else (pruned.read_text() if pruned.exists() else None)
        if h:
            hashes[h] = hashes.get(h, 0) + 1
    if hashes and max(hashes.values()) > len(results) // 4:
        lines.append(f"WARNING: {max(hashes.values())} events end on the same picture; were they started?")
    (work / "report.txt").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))

    try:
        from PIL import Image
        cells = []
        for (l, i) in sorted(results):
            d = work / f"{l}_{i:03d}"
            shots = sorted(d.glob("shot_*.bmp")) or sorted(d.glob("thumb.png"))
            if shots:
                cells.append(Image.open(shots[-1]).convert("RGB").resize((160, 90) if args.wide else (120, 80)))
        cols = 16
        cw, ch = (164, 94) if args.wide else (124, 84)
        sheet = Image.new("RGB", (cols * cw, ((len(cells) + cols - 1) // cols) * ch), (30, 30, 30))
        for n, im in enumerate(cells):
            sheet.paste(im, ((n % cols) * cw, (n // cols) * ch))
        if cells:
            sheet.save(work / "events.png")
    except ImportError:
        pass


if __name__ == "__main__":
    main()
