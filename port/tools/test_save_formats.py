"""Every GBA save format the importer promises, built from a real save and read back through the parser
(and, with --launcher, through the real launcher's import harness).

    python port/tools/test_save_formats.py [--base <kh_com.sav>] [--launcher <build dir with ReKHCoM.exe>]
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import save_formats as sf  # noqa: E402

ap = argparse.ArgumentParser(description=__doc__)
ap.add_argument("--base", type=Path, help="a real 64 KB kh_com*.sav (read only); default: a synthetic one")
ap.add_argument("--launcher", type=Path, help="build folder with ReKHCoM.exe: also import every file through it")
ap.add_argument("--keep", type=Path, help="leave the generated files in this folder")
args = ap.parse_args()


def synthetic():
    data = bytearray(b"\xFF" * 0x10000)
    for at in (0x10, 0x50):
        data[at:at + 24] = sf.SIG_EU
        data[at + 0x1C:at + 0x20] = bytes([7, 0, 4, 0])
        sf.seal(data, at, 0x40)
    for at in (0x90, 0xFA4, 0x2F20, 0x3E34, 0x4D48, 0x5C5C):
        data[at:at + 24] = sf.SIG_EU
        data[at + 0x1C:at + 0xF14] = (bytes(range(256)) * 16)[:0xF14 - 0x1C]
        data[at + 0xAC:at + 0xAE] = b"\x01\x00"
        sf.seal(data, at, 0xF14)
    for at in (0x1EC0, 0x22D8, 0x26F0, 0x2B08):
        data[at:at + 24] = sf.SIG_EU
        data[at + 0x1C:at + 0x418] = (bytes(range(255, 0, -1)) * 8)[:0x418 - 0x1C]
        sf.seal(data, at, 0x418)
    return bytes(data)


compiler = next((ROOT.parent / "toolchain").glob("llvm-mingw-*/bin/i686-w64-mingw32-clang++.exe"))
work = Path(tempfile.mkdtemp(prefix="save-formats-", dir=args.keep.parent if args.keep else None)) if not args.keep else args.keep
work.mkdir(parents=True, exist_ok=True)
cli = work / "save_convert_cli.exe"
subprocess.run([str(compiler), "-std=c++17", "-O2", "-static", "-DNOMINMAX", "-I" + str(ROOT / "port/decompui"),
                str(ROOT / "port/tools/save_convert_cli.cpp"), "-o", str(cli)], check=True)

base = args.base.read_bytes() if args.base else synthetic()
assert len(base) == 0x10000 and base[16:40] == sf.SIG_EU
files = work / "files"
files.mkdir(exist_ok=True)
cases = sf.build(base)
failures = []


def convert(path, region):
    out = work / "out.sav"
    if out.exists():
        out.unlink()
    r = subprocess.run([str(cli), str(path), str(out), str(region)], capture_output=True, text=True)
    return r.returncode, r.stdout.strip(), (out.read_bytes() if out.exists() else None)


def blocks_equal_ignoring_region(a, b):
    """same block data, whatever signature, checksum and header language"""
    for at, size in sf.BLOCKS:
        x, y = bytearray(a[at:at + size]), bytearray(b[at:at + size])
        for blk in (x, y):
            blk[0:26] = b"\0" * 26
            if size == 0x40:
                blk[0x1E:0x20] = b"\0\0"
        if x != y:
            return False
    return True


for name, (ext, data, expect) in cases.items():
    path = files / f"{name}.{ext}"
    path.write_bytes(data)
    code, line, out = convert(path, 0)
    if expect == "same":
        ok = code == 0 and out == base
    elif expect == "region":
        ok = code == 0 and blocks_equal_ignoring_region(out, base) and out[16:40] == sf.SIG_EU
        c1, _, o1 = convert(path, 1)
        ok = ok and c1 == 0 and o1[16:40] == sf.SIG_US and blocks_equal_ignoring_region(o1, base)
        for at, size in sf.BLOCKS:
            if base[at:at + 24] == sf.SIG_EU and sf.checksum(base[at:at + size]) == 0:
                ok = ok and sf.checksum(out[at:at + size]) == 0 and sf.checksum(o1[at:at + size]) == 0
    else:
        ok = code != 0 and out is None
    print(f"{'OK  ' if ok else 'FAIL'} {name}.{ext:7} {len(data):7} bytes  {line}")
    if not ok:
        failures.append(name)

# the user's own copies: bank files and backups of every region must import as themselves
for region in (1, 2):
    path = files / f"own_region{region}.sav"
    path.write_bytes(sf.to_region(base, region))
    code, line, out = convert(path, region)
    ok = code == 0 and out == sf.to_region(base, region)
    print(f"{'OK  ' if ok else 'FAIL'} own save of region {region} into its own region  {line}")
    if not ok:
        failures.append(path.name)

# damaged containers must never crash or hang the parser: a result or a refusal, nothing else
import random  # noqa: E402
rng = random.Random(2024)
fuzzed = 0
for name, (ext, data, expect) in cases.items():
    for variant in range(6):
        mutated = bytearray(data)
        if variant < 2:
            mutated = mutated[:rng.randrange(1, len(mutated))]
        else:
            for _ in range(1 + variant * 3):
                mutated[rng.randrange(len(mutated))] ^= 1 << rng.randrange(8)
        path = files / "fuzz.bin"
        path.write_bytes(bytes(mutated))
        r = subprocess.run([str(cli), str(path), str(work / "fuzz.out"), "0"], capture_output=True, timeout=60)
        fuzzed += 1
        if r.returncode not in (0, 1):
            print(f"FAIL fuzz {name} variant {variant}: exit {r.returncode}")
            failures.append(f"fuzz:{name}:{variant}")
print(f"OK   {fuzzed} damaged files: no crash")

if args.launcher:
    build = args.launcher.resolve()
    runs = work / "launcher"
    for name, (ext, data, expect) in cases.items():
        if expect == "region":
            continue
        run = runs / name
        run.mkdir(parents=True, exist_ok=True)
        for item in ("ReKHCoM.exe", "SDL3.dll"):
            shutil.copy2(build / item, run / item)
        if not (run / "assets").exists():
            shutil.copytree(build / "assets", run / "assets")
        (run / "kh_com.ini").write_text("config_version=11\nsave_bank=2\naspect=6\n")
        (run / "shots").mkdir(exist_ok=True)
        target = run / "kh_com_bank2.sav"
        sentinel = b"previous" + b"\xFF" * (0x10000 - 8)
        target.write_bytes(sentinel)
        env = {k: v for k, v in os.environ.items() if not k.startswith(("DECOMPUI_", "KHCOM_", "SDL_"))}
        env.update(DECOMPUI_SHOT=str(run / "shots"), DECOMPUI_IMPORT_SAVE=str(files / f"{name}.{ext}"),
                   DECOMPUI_SAVE_REGION="0", DECOMPUI_IMPORT_CONFIRM="1",
                   SDL_VIDEO_DRIVER="offscreen", SDL_AUDIO_DRIVER="dummy")
        subprocess.run([str(run / "ReKHCoM.exe")], cwd=run, env=env, check=True, timeout=120, creationflags=0x08000000)
        expected = base if expect == "same" else sentinel
        ok = target.read_bytes() == expected
        print(f"{'OK  ' if ok else 'FAIL'} launcher {name}")
        if not ok:
            failures.append("launcher:" + name)

if not args.keep:
    shutil.rmtree(work, ignore_errors=True)
if failures:
    print("FAILED:", ", ".join(failures))
    sys.exit(1)
print(f"Save formats: {len(cases)} formats PASS")
