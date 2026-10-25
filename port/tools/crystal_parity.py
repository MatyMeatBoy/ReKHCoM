"""Crystal/room parity, original vs port, seed by seed (queue 72).

For each seed: the user's BizHawk save with only the current room's seed
changed (save_reseed.py) is loaded on the original (BizHawk, USA; the user's
SaveRAM is backed up, set aside and restored, sha1-checked) and on the port
(US build); the crystal's position is read from each (the original's EWRAM,
the port's harness log) and compared.

    python port/tools/crystal_parity.py 0x2242C2F1 0x12345679 0x0BADF00D
"""
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BH = ROOT.parent / 'toolchain/bizhawk'
SAVE = BH / 'GBA/SaveRAM/Kingdom Hearts - Chain of Memories (USA).SaveRAM'
BACKUP = ROOT.parent / 'respaldos/saves/bizhawk/saveram/Kingdom Hearts - Chain of Memories (USA).SaveRAM'
OUT = ROOT / 'build/crystal-parity'
KEYS = '900-902:Start,1000-1002:A,1100-1102:A,1200-1202:A,1300-1302:A,1400-1402:A'


def sha1(p):
    return hashlib.sha1(Path(p).read_bytes()).hexdigest()


def original(seed, sav):
    out = OUT / f'gba-{seed:08x}'
    out.mkdir(parents=True, exist_ok=True)
    aside = BACKUP.parent / 'USA.SaveRAM.parity-aside'
    bak = Path(str(SAVE) + '.bak')
    bak_aside = BACKUP.parent / 'USA.SaveRAM.bak.parity-aside'
    assert sha1(SAVE) == sha1(BACKUP), 'user save differs from its backup: stop'
    shutil.move(SAVE, aside)
    if bak.exists():
        shutil.move(bak, bak_aside)
    try:
        shutil.copy2(sav, SAVE)
        env = dict(os.environ, KHCOM_BH_SLOT='0', KHCOM_BH_SEED='0', KHCOM_BH_KEYS=KEYS, KHCOM_BH_EVERY='2200',
                   KHCOM_BH_END='2200', KHCOM_BH_OUT=str(out))
        subprocess.run([str(BH / 'EmuHawk.exe'), '--lua=' + str(ROOT / 'port/tools/bizhawk_reseed.lua'),
                        str(ROOT / 'roms/B8CE.gba')], cwd=BH, env=env, timeout=900)
    finally:
        for p in (SAVE, bak):
            if p.exists():
                p.unlink()
        shutil.move(aside, SAVE)
        if bak_aside.exists():
            shutil.move(bak_aside, bak)
        assert sha1(SAVE) == sha1(BACKUP), 'user save restore failed'
    ewram = (out / 'rs_ewram.bin').read_bytes()
    return ewram


def port(seed, sav):
    out = OUT / f'port-{seed:08x}'
    out.mkdir(parents=True, exist_ok=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(ROOT / 'build/pc-us' / f, out / f)
    (out / 'kh_com_us.sav').write_bytes(Path(sav).read_bytes()[:65536])
    (out / 'kh_com.log').unlink(missing_ok=True)
    keys = KEYS.replace('Start', 'START')
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_KEYS=keys,
               KHCOM_MODE='2100:gmkcount,2101:gmkall', KHCOM_SHOTS='2190', KHCOM_QUIT='2200')
    subprocess.run([str(out / 'khcom.exe'), '--rom', str(ROOT / 'roms/B8CE.gba'), '--data-dir', str(out)],
                   cwd=out, env=env, timeout=600, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    log = (out / 'kh_com.log').read_text(errors='replace')
    return re.findall(r'save crystal at (-?\d+),(-?\d+),(-?\d+) room=(\d+) seed=([0-9a-f]+)', log), log


def gimmicks(log):
    out = []
    for d, f, x, y, z in re.findall(r'gmk \d+ def=([0-9a-f]+) flags=([0-9a-f]+) at (-?\d+),(-?\d+),(-?\d+)', log):
        out.append((int(d, 16), int(f, 16), int(x), int(y), int(z)))
    return out


def original_list(ewram, first):
    # the cartridge's own gMapGmkPlacements: find the port's first entry (pos + def), read 16 entries from there
    d, f, x, y, z = first
    at = ewram.find(struct.pack('<iiiI', x, y, z, d))
    if at < 0:
        return None
    base = at - 4
    out = []
    for i in range(16):
        fl, = struct.unpack_from('<H', ewram, base + 20 * i)
        px, py, pz, pd = struct.unpack_from('<iiiI', ewram, base + 20 * i + 4)
        out.append((pd, fl, px, py, pz))
    return out


ok = True
for text in sys.argv[1:]:
    seed = int(text, 0)
    sav = OUT / f'{seed:08x}.SaveRAM'
    OUT.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(ROOT / 'port/tools/save_reseed.py'), str(BACKUP), str(sav), hex(seed)], check=True)
    ewram = original(seed, sav)
    crystals, log = port(seed, sav)
    print(f'seed {seed:08x}: port crystals {crystals}')
    gm = gimmicks(log)
    gba = original_list(ewram, gm[0]) if gm else None
    if gba is None:
        print('   original: placement list not found'); ok = False
    else:
        same = gba[:len(gm)] == gm
        print(f'   all gimmicks ({len(gm)}): {"SAME as the cartridge" if same else "DIFFERENT"}')
        for a, b in zip(gm, gba):
            print(f'      port def={a[0]:08x} at {a[2]},{a[3]},{a[4]}   cartridge def={b[0]:08x} at {b[2]},{b[3]},{b[4]}')
        ok &= same
    for x, y, z, room, s in crystals:
        hit = ewram.find(struct.pack('<ii', int(x), int(y))) >= 0
        print(f'   original has a crystal object at ({x}, {y}): {"YES" if hit else "NO"}')
        ok &= hit
    if not crystals:
        print('   port showed no crystal in this room (seed may give a room without one)')
print('PARITY' if ok else 'MISMATCH')
