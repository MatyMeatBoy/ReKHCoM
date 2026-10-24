#!/usr/bin/env python3
"""The sound around an HD scene: one story event, real time, SDL's "disk" audio driver (silent; out.raw is what
would be heard, float32 stereo 48 kHz). Prints the loudness every 50 ms and the largest sample steps, so a cut (a
pop), the field's music left under the movie or a skip without its fade shows up. Also saves the presented frames
(KHCOM_PRESENTSHOTS) to check the picture's fades.

    uv run --with numpy python port/tools/test_hd_transition_audio.py --hd D:/01Claude/ReKHCoM-build/hd-completo \
        [--build build/pc-eu] [--event 0:16] [--seconds 3] [--pause 2]
"""
import argparse
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--hd", required=True)
    ap.add_argument("--build", default="build/pc-eu")
    ap.add_argument("--event", default="0:16", help="debug menu list:index (KHCOM_EVENT)")
    ap.add_argument("--seconds", default="3", help="the movie is skipped (with its fade) after this")
    ap.add_argument("--pause", help="open the movie pause at this second and walk it to skip")
    args = ap.parse_args()

    d = ROOT / "build" / "hdtrans" / args.event.replace(":", "_")
    if d.exists():
        if (d / "hd").exists():
            os.rmdir(d / "hd")  # the junction only
        shutil.rmtree(d)
    d.mkdir(parents=True)
    for f in ("khcom.exe", "SDL3.dll"):
        shutil.copy2(ROOT / args.build / f, d / f)
    subprocess.run(["cmd", "/c", "mklink", "/J", str(d / "hd"), str(Path(args.hd))], check=True,
                   stdout=subprocess.DEVNULL)
    (d / "kh_com.ini").write_text("config_version=2\nview_mode=0\nhd_movies=1\nhd_scenes=1\nhd_music=1\n")
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="disk", SDL_AUDIO_DISK_OUTPUT_FILE="out.raw",
               KHCOM_EVENT=args.event, KHCOM_QUIT="400", KHCOM_HDMOVIE_SECONDS=args.seconds,
               KHCOM_PRESENTSHOTS="0-400", KHCOM_KEYS=",".join(f"{f}-{f + 3}:A" for f in range(30, 400, 40)))
    if args.pause:
        env["KHCOM_HDMOVIE_PAUSETEST"] = args.pause
    env.pop("KHCOM_FAST", None)  # real time: the sound only exists against the clock
    subprocess.run([str(d / "khcom.exe"), "--language", "es", "--rom", str(ROOT / "roms/B8CP.gba")], cwd=d, env=env,
                   timeout=300)
    os.rmdir(d / "hd")
    print("".join(l for l in (d / "kh_com.log").read_text(errors="replace").splitlines(True) if "hd " in l))

    import numpy as np
    x = np.fromfile(d / "out.raw", dtype=np.float32).reshape(-1, 2)
    rate = 48000
    step = np.abs(np.diff(x, axis=0)).max(axis=1)
    blk = rate // 20
    rms = [float(np.sqrt((x[i:i + blk] ** 2).mean())) for i in range(0, len(x) - blk, blk)]
    print(f"{len(x) / rate:.2f} s; loudness every 50 ms (x100):")
    print(" ".join(f"{r * 100:.0f}" for r in rms))
    for i in np.argsort(step)[-6:][::-1]:
        lo, hi = max(0, i - 480), min(len(step), i + 480)
        print(f"step {step[i]:.3f} at {i / rate:.3f} s (local median {np.median(step[lo:hi]):.4f})")


if __name__ == "__main__":
    main()
