#!/usr/bin/env python3
"""Compare Aerith's scene in wide framing and the proposed native GBA frame.

Run from the repository root, normally through `Probar Aerith - encuadre.bat`.
The two runs use the same event, frame schedule and inputs for visual review.
"""
import json
import os
import shutil
import subprocess
from pathlib import Path

from test_events import event_lists

ROOT = Path(__file__).resolve().parents[2]
FRAMES = (1000, 1080, 1160, 1260, 1360, 1460, 1580, 1670, 1770)
QUIT = 1780
WINDOW = "1920x1080"


def main():
    build = ROOT / "build" / "codex28-eu"
    out = ROOT / "build" / "scene-aerith-review"
    rom = ROOT / "roms" / "B8CP.gba"
    exe_source = build / "khcom.exe"
    dll_source = build / "SDL3.dll"
    if not exe_source.is_file() or not dll_source.is_file() or not rom.is_file():
        raise SystemExit("Falta build/codex28-eu/khcom.exe, SDL3.dll o roms/B8CP.gba")
    event = next(((l, i) for l, i, name in event_lists() if name == "EVENT_009_1F_TRAVERSE_TOWN_E4"), None)
    if event is None:
        raise SystemExit("No se encontró el evento 009 de Aerith en el selector")
    list_id, index = event
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    results = []
    for label, view_mode, native_frame in (("gba-reference", 0, None),
                                           ("wide-before", 4, "off"),
                                           ("native-after", 4, "on")):
        run = out / label
        run.mkdir()
        shutil.copy2(exe_source, run / "khcom.exe")
        shutil.copy2(dll_source, run / "SDL3.dll")
        (run / "kh_com.ini").write_text(
            f"config_version=11\nview_mode={view_mode}\nzoom=6\n"
            "hd_text=0\nvsync=0\nfullscreen=0\n",
            encoding="ascii",
        )
        keys = ",".join(f"{f}-{f+3}:A" for f in range(30, QUIT, 40))
        env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy",
                   KHCOM_FAST="1", KHCOM_EVENT=f"{list_id}:{index}",
                   KHCOM_WINDOW=WINDOW, KHCOM_KEYS=keys,
                   KHCOM_SHOTS=",".join(map(str, FRAMES)), KHCOM_QUIT=str(QUIT))
        if native_frame:
            env["KHCOM_AERITH_NATIVE_FRAME"] = native_frame
        else:
            env.pop("KHCOM_AERITH_NATIVE_FRAME", None)
        subprocess.run([str(run / "khcom.exe"), "--language", "es", "--rom", str(rom)],
                       cwd=run, env=env, timeout=180, check=True,
                       stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)
        for frame in FRAMES:
            shot = run / f"shot_{frame:05d}.bmp"
            if not shot.is_file():
                raise SystemExit(f"Falta captura {label} {frame}")
            results.append((label, frame, shot))

    try:
        from PIL import Image, ImageChops
    except ImportError:
        print(f"Capturas BMP listas en {out}; Pillow no está disponible para el informe visual.")
        return

    rows = []
    for frame in FRAMES:
        gba_path = next(p for label, f, p in results if label == "gba-reference" and f == frame)
        wide_path = next(p for label, f, p in results if label == "native-after" and f == frame)
        with Image.open(gba_path) as a, Image.open(wide_path) as b:
            a, b = a.convert("RGB"), b.convert("RGB")
            rows.append({"frame": frame, "gba_size": list(a.size), "wide_size": list(b.size)})
        with Image.open(next(p for label, f, p in results if label == "wide-before" and f == frame)) as before, \
             Image.open(next(p for label, f, p in results if label == "native-after" and f == frame)) as after:
            diff = ImageChops.difference(before.convert("RGB"), after.convert("RGB"))
            rows[-1]["feature_change_bounds"] = diff.getbbox()
        for label in ("gba-reference", "wide-before", "native-after"):
            with Image.open(next(p for name, f, p in results if name == label and f == frame)) as image:
                image.convert("RGB").save(out / f"aerith-{frame:04d}-{label}.png")
    report = {"event": "009 Traverse Town E4", "window": WINDOW,
              "zoom": 6, "frames": rows,
              "review": "wide-before shows current framing; native-after tests the fixed GBA composition; gba-reference shows cartridge output."}
    (out / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    print(f"Revisa las capturas PNG y report.json en: {out}")


if __name__ == "__main__":
    main()
