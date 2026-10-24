#!/usr/bin/env python3
"""Every story event mapped to a Re:CoM HD scene (port/src/port_hdscenes.inc) really plays its scene.

Each mapped event starts from the debug event menu (KHCOM_EVENT, as test_events.py) in its own silent process, with
hd_scenes on and the player's imported hd folder linked in. The case passes when the log shows the game asking for
the event's scene ("hd scene: event N") and the movie opening ("hd movie: <name>"); every movie is stopped after
KHCOM_HDMOVIE_SECONDS. With --recom the Re:CoM-only scenes (kHdSceneExtras) must play next to their neighbour, and
without it they must not. With --movies-only (hd_movies on, hd_scenes off) only the endings' scenes (IsEnding in
port_hdmovie.c, issue #42) may play, and they must.

    python port/tools/test_hd_scenes.py --hd D:/01Claude/ReKHCoM-build/hd-completo [--build build/pc-eu] [--recom] [--jobs 6]
"""
import argparse
import json
import os
import re
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import case_bin
from test_events import event_lists

ROOT = Path(__file__).resolve().parents[2]


def event_numbers(version):
    """The enum's values for this cartridge (EU has no EVENT_127/128_DAMI: #ifndef VERSION_EU)."""
    defined = {"VERSION_" + version.upper()}
    out, n, live = {}, 0, [True]
    for line in (ROOT / "include" / "evt" / "event_ids.h").read_text(encoding="utf-8").splitlines():
        s = line.strip()
        m = re.match(r"#if(n?)def\s+(\w+)", s)
        if m:
            live.append(live[-1] and ((m.group(2) in defined) != bool(m.group(1))))
            continue
        if s.startswith("#else"):
            live[-1] = live[-2] and not live[-1]
            continue
        if s.startswith("#endif"):
            live.pop()
            continue
        m = re.match(r"(EVENT_\w+)\s*(?:=\s*(\w+))?\s*,", s)
        if m and live[-1]:
            if m.group(2):
                n = int(m.group(2), 0)
            out[m.group(1)] = n
            n += 1
    return out


def scene_map():
    inc = (ROOT / "port" / "src" / "port_hdscenes.inc").read_text(encoding="utf-8")
    scenes = re.findall(r"\{\s*(EVENT_\w+),\s*\"(\w+)\"\s*\}", inc)
    src = (ROOT / "port" / "src" / "port_hdmovie.c").read_text(encoding="utf-8")
    extras = re.findall(r"\{\s*(EVENT_\w+),\s*\"(\w+)\",\s*(\d)\s*\}", src)
    endings = re.findall(r"eventId == (EVENT_\w+)", src[src.index("static int IsEnding"):src.index("int PortHdScenePlay")])
    return scenes, extras, endings


def link(target, link_path):
    if link_path.exists():
        return
    subprocess.run(["cmd", "/c", "mklink", "/J", str(link_path), str(target)], check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def run(exe_dir, work, hd, version, list_id, index, frames, timeout, recom, movies_only):
    d = work / f"{list_id}_{index:03d}"
    if d.exists():
        if (d / "hd").exists():
            os.rmdir(d / "hd")  # the junction only, never the player's files
        shutil.rmtree(d)
    d.mkdir(parents=True)
    case_bin.place(exe_dir, work, d)
    link(hd, d / "hd")
    (d / "kh_com.ini").write_text("config_version=2\nview_mode=0\nhd_movies=1\nhd_scenes=" + ("0" if movies_only else "1") +
                                     "\nhd_subtitles=1\n")
    keys = ",".join(f"{f}-{f + 3}:A" for f in range(30, frames, 40))
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1",
               KHCOM_EVENT=f"{list_id}:{index}", KHCOM_KEYS=keys, KHCOM_QUIT=str(frames))
    env.setdefault("KHCOM_HDMOVIE_SECONDS", "1")  # a longer stop is kept (e.g. to walk KHCOM_HDMOVIE_PAUSETEST)
    if recom:
        env["KHCOM_RECOM"] = "1"
    rom = ROOT / "roms" / ({"eu": "B8CP", "us": "B8CE", "jp": "B8CJ"}[version] + ".gba")
    try:
        p = subprocess.run([str(d / "khcom.exe"), "--language", "es", "--rom", str(rom)], cwd=d, env=env,
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           timeout=timeout)
        code = p.returncode
    except subprocess.TimeoutExpired:
        code = "timeout"
    log = (d / "kh_com.log").read_text(errors="replace") if (d / "kh_com.log").exists() else ""
    os.rmdir(d / "hd")
    # the first run of the event only: the keys go on, and the debug menu starts the event again once it ends (each
    # run rightly offers its scene again; until 23/10 a stale check hid that, and the ending of chained events too)
    first = re.search(r"hd scene: event \d+", log)
    if first:
        again = log.find(first.group(0), first.end())
        if again > 0:
            log = log[:again]
    asked = [int(x) for x in re.findall(r"hd scene: event (\d+)", log)]
    movies = re.findall(r"hd movie: (\w+) \(", log)
    return code, "CRASH" in log, asked, movies


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--hd", required=True, help="the imported hd folder (movies, music, subs)")
    ap.add_argument("--version", default="eu", choices=("eu", "us", "jp"))
    ap.add_argument("--build", help="folder with khcom.exe (default build/pc-<version>)")
    ap.add_argument("--recom", action="store_true", help="Re:CoM mode (KHCOM_RECOM=1): the extra scenes play too")
    ap.add_argument("--movies-only", action="store_true", help="hd_scenes off: only the endings' scenes play")
    ap.add_argument("--frames", type=int, default=900)
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--timeout", type=int, default=300)
    ap.add_argument("--only", help="comma list of event names (EVENT_016_3F_GOAL,...)")
    args = ap.parse_args()

    exe_dir = ROOT / (args.build or f"build/pc-{args.version}")
    work = ROOT / "build" / "hdscenes" / (args.version + ("_recom" if args.recom else "") + ("_movies" if args.movies_only else ""))
    work.mkdir(parents=True, exist_ok=True)
    numbers = event_numbers(args.version)
    scenes, extras, endings = scene_map()
    menu = {name: (l, i) for l, i, name in event_lists()}
    have = {p.stem for d in ("movies", "scenes") for p in Path(args.hd, d).glob("*.mp4")}

    lines, cases = [], []
    if args.only:
        scenes = [s for s in scenes if s[0] in args.only.split(",")]
    for ev, movie in scenes:
        if ev not in menu:
            lines.append(f"{ev:40s} {movie:14s} NOT IN THE DEBUG MENU (not tested)")
        elif movie not in have:
            lines.append(f"{ev:40s} {movie:14s} NOT IMPORTED in {args.hd}")
        else:
            cases.append((ev, movie))

    results = {}
    with ThreadPoolExecutor(args.jobs) as pool:
        futs = {pool.submit(run, exe_dir, work, Path(args.hd), args.version, *menu[ev], args.frames, args.timeout,
                            args.recom, args.movies_only): (ev, movie) for ev, movie in cases}
        for k, f in enumerate(as_completed(futs), 1):
            results[futs[f]] = f.result()
            print(f"[{k}/{len(futs)}] {futs[f][0]}", flush=True)

    bad = 0
    for (ev, movie), (code, crash, asked, movies) in sorted(results.items(), key=lambda r: numbers.get(r[0][0], 0)):
        want = [movie]
        for xev, xmovie, before in extras:
            if xev == ev and args.recom:
                want = [xmovie] + want if before == "1" else want + [xmovie]
        if args.movies_only and ev not in endings:
            want = []
        problems = []
        if numbers[ev] not in asked:
            problems.append(f"scene never asked (asked: {asked})")
        if movies != want:
            problems.append(f"played {movies}, wanted {want}")
        if crash or code not in (0, "timeout"):
            problems.append(f"exit {code}{' CRASH' if crash else ''}")
        bad += bool(problems)
        lines.append(f"{ev:40s} {movie:14s} {'ok' if not problems else 'BAD ' + '; '.join(problems)}")
    lines.append(f"{len(results) - bad}/{len(results)} scenes ok, {len(scenes) - len(cases)} not testable here")
    (work / "report.txt").write_text("\n".join(lines) + "\n")
    (work / "report.json").write_text(json.dumps({f"{e}|{m}": r for (e, m), r in results.items()}, indent=1))
    print("\n".join(lines))


if __name__ == "__main__":
    main()
