#!/usr/bin/env python3
"""Package existing Windows builds using an explicit, ROM-free file list.

The package holds the launcher (ReKHCoM.exe, the only executable at the
root) and, in bin/, one game executable per cartridge version, built with
port/configure_pc.py --version eu|us|jp.
"""
import argparse
import hashlib
import re
import zipfile
from pathlib import Path
from core_assets import core_assets

ROOT = Path(__file__).resolve().parents[2]


def check_core_fonts(files):
    """Reject stale game binaries that would lose their fonts when copied alone."""
    fonts = {name: files["assets/fonts/" + name].read_bytes()
             for name in ("KHMenu.otf", "ComicHearts.otf")}
    for name, data in fonts.items():
        if not data.startswith(b"OTTO"):
            raise ValueError(f"invalid core font: {name}")
    for name in [f"bin/khcom_{region}.exe" for region in ("eu", "us", "jp")] + ['ReKHCoM.exe']:
        binary = files[name].read_bytes()
        for font, data in fonts.items():
            if data not in binary:
                raise ValueError(f"{name} has no embedded {font}; rebuild this region before packaging")


def check_core_assets(files):
    expected = {name: source.read_bytes() for name, source in core_assets().items()}
    for binary_name in [f'bin/khcom_{r}.exe' for r in ('eu', 'us', 'jp')] + ['ReKHCoM.exe']:
        binary = files[binary_name].read_bytes()
        for name, data in expected.items():
            if not data or data not in binary:
                raise ValueError(f'{binary_name} has no embedded {name}; rebuild before packaging')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--build", type=Path, default=ROOT / "build/pc", help="European build (and launcher)")
    parser.add_argument("--build-us", type=Path, default=ROOT / "build/pc-us")
    parser.add_argument("--build-jp", type=Path, default=ROOT / "build/pc-jp")
    parser.add_argument("--sdl", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=ROOT / "build/releases")
    parser.add_argument("--nightly-notice", type=Path,
                        help="include a test-build notice as NIGHTLY-TESTING.md")
    args = parser.parse_args()
    if not re.fullmatch(r"v\d+\.\d+(?:\.\d+)?(?:-[A-Za-z0-9.-]+)?", args.version):
        parser.error("version must look like v0.1 or v0.1.1")
    files = {
        "ReKHCoM.exe": args.build / "ReKHCoM.exe",
        "bin/khcom_eu.exe": args.build / "khcom.exe",
        "bin/khcom_us.exe": args.build_us / "khcom.exe",
        "bin/khcom_jp.exe": args.build_jp / "khcom.exe",
        "bin/SDL3.dll": args.build / "SDL3.dll",
        "SDL3.dll": args.build / "SDL3.dll",
        "README.md": ROOT / "README.md",
        "CHANGELOG.md": ROOT / "CHANGELOG.md",
        "LICENSE.md": ROOT / "LICENSE.md",
        "SDL3-LICENSE.txt": args.sdl / "LICENSE.txt",
        "licenses/imgui-LICENSE.txt": ROOT / "port/decompui/third_party/imgui/LICENSE.txt",
        "licenses/rcheevos-LICENSE.txt": ROOT / "port/third_party/rcheevos/LICENSE",
        "assets/boxart.jpg": ROOT / "port/decompui/assets/boxart.jpg",
        "assets/steam_header.jpg": ROOT / "port/decompui/assets/steam_header.jpg",
        "assets/steam_hero.jpg": ROOT / "port/decompui/assets/steam_hero.jpg",
        "assets/logo.png": ROOT / "port/decompui/assets/logo.png",
        "assets/NOTICE.md": ROOT / "port/decompui/assets/NOTICE.md",
        "assets/achievements/badges.png": ROOT / "port/assets/achievements/badges.png",
        "assets/fonts/KHMenu.otf": ROOT / "port/assets/fonts/KHMenu.otf",
        "assets/fonts/ComicHearts.otf": ROOT / "port/assets/fonts/ComicHearts.otf",
        "assets/fonts/NOTICE.md": ROOT / "port/assets/fonts/NOTICE.md",
        "assets/hd-hud/pilot/enemy-hp-frame-v2.png": ROOT / "port/assets/hd-hud/pilot/enemy-hp-frame-v2.png",
        "assets/hd-hud/pilot/label-hp-v2.png": ROOT / "port/assets/hd-hud/pilot/label-hp-v2.png",
        "assets/hd-hud/pilot/label-vt-v2.png": ROOT / "port/assets/hd-hud/pilot/label-vt-v2.png",
        "assets/hd-hud/pilot/label-lp-v1.png": ROOT / "port/assets/hd-hud/pilot/label-lp-v1.png",
        "assets/hd-hud/pilot/label-pv-v3.png": ROOT / "port/assets/hd-hud/pilot/label-pv-v3.png",
    }
    files.update(core_assets())
    for source in files.values():
        if not source.is_file():
            parser.error(f"missing package input: {source}")
    try:
        check_core_fonts(files)
        check_core_assets(files)
    except ValueError as error:
        parser.error(str(error))
    if args.nightly_notice:
        if not args.nightly_notice.is_file():
            parser.error(f"missing nightly notice: {args.nightly_notice}")
        files["NIGHTLY-TESTING.md"] = args.nightly_notice
    args.out.mkdir(parents=True, exist_ok=True)
    archive = args.out / f"ReKHCoM-{args.version}-windows-x86.zip"
    # No versioned top folder: an update is extracted over the old install
    # (saves and kh_com.ini are not in the package, so they are kept).
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, source in files.items():
            z.write(source, name)
        z.writestr("roms/", "")
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    checksum = archive.with_suffix(".zip.sha256")
    checksum.write_text(f"{digest}  {archive.name}\n", encoding="ascii")
    print(f"Created {archive} ({archive.stat().st_size:,} bytes)")
    print(f"SHA-256: {digest}")
    print("Package files: " + ", ".join(files))


if __name__ == "__main__":
    main()
