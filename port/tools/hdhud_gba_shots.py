"""GBA-view captures of the battle HUD and the deck menu (zoom x4) to compare with Re:CoM's textures."""
import os, subprocess, shutil, sys
from pathlib import Path
REPO = Path(sys.argv[1]); OUT = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("D:/01Claude/ReKHCoM-build/hdhud") / "gba"
CASES = {"battle": {"KHCOM_BATTLE": "0:1:0", "KHCOM_KEYS": "150-153:A"},
         "boss": {"KHCOM_BATTLE": "10:4:0", "KHCOM_KEYS": "150-153:A"},
         "deck": {"KHCOM_MODE": "60:Mode_Deck", "KHCOM_KEYS": "200-203:A,260-263:RIGHT,300-303:A"}}
for name, extra in CASES.items():
    d = OUT / name
    d.mkdir(parents=True, exist_ok=True)
    for f in ("khcom.exe", "SDL3.dll"): shutil.copy2(REPO / "build/pc-eu" / f, d / f)
    (d / "kh_com.ini").write_text("config_version=11\nview_mode=0\nzoom=8\nhd_text=0\n")
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", KHCOM_FAST="1",
               KHCOM_SHOTS="250,400,600", KHCOM_QUIT="610", **extra)
    subprocess.run([str(d / "khcom.exe"), "--language", "es", "--rom", str(REPO / "roms/B8CP.gba")], cwd=d, env=env,
                   timeout=300, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print(name, sorted(p.name for p in d.glob("shot_*.bmp")))
