#!/usr/bin/env python3
"""Configure the native Windows build of Kingdom Hearts: Chain of Memories.

    python port/configure_pc.py [--version eu|us|jp] [--toolchain DIR] [--sdl DIR] [--debug]
    ninja -f build/pc/build.ninja

Each cartridge version (European, USA, Japanese) is its own build; the
default output directory is build/pc for eu and build/pc-<version> otherwise.

The game is built for 32-bit x86 so pointers keep their GBA size; see
port/include/port/port.h for the overall design.
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import ninja_syntax  # noqa: E402

VERSION = "eu"
BUILD = ROOT / "build" / "pc"
GEN = ROOT / "build" / VERSION / "gen"
PC_GEN = ROOT / "build" / VERSION / "pc"
VERSIONS = ("eu", "us", "jp")


def rel(path):
    return os.path.relpath(path, ROOT).replace("\\", "/")


def find_default(name, pattern):
    base = ROOT.parent / "toolchain"
    if base.is_dir():
        hits = sorted(base.glob(pattern))
        if hits:
            return hits[-1]
    sys.exit(f"error: {name} not found; pass --{name.lower()}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--toolchain", type=Path, help="llvm-mingw directory")
    parser.add_argument("--sdl", type=Path, help="SDL3 mingw development package")
    parser.add_argument("--debug", action="store_true", help="no optimisation, console window")
    parser.add_argument("--version", default="eu", choices=VERSIONS, help="cartridge version (default: eu)")
    parser.add_argument("--out", help="output directory (default: build/pc, or build/pc-<version>)")
    parser.add_argument("--reloc", action="store_true",
                        help="relocatable GBA space (PORT_GBA_RELOC, as on Vita/Switch), checked on Windows (build/pc-reloc-<version>)")
    parser.add_argument("--hd2d", action="store_true",
                        help="alternative build with the experimental HD-2D look on by default (build/pc-hd2d)")
    args = parser.parse_args()

    global BUILD, VERSION, GEN, PC_GEN
    VERSION = args.version
    GEN = ROOT / "build" / VERSION / "gen"
    PC_GEN = ROOT / "build" / VERSION / "pc"
    default_out = "build/pc" if VERSION == "eu" else f"build/pc-{VERSION}"
    if args.reloc:
        default_out = f"build/pc-reloc-{VERSION}"
    BUILD = ROOT / (args.out or (default_out + "-hd2d" if args.hd2d else default_out))
    toolchain = args.toolchain or find_default("toolchain", "llvm-mingw-*")
    sdl = args.sdl or find_default("sdl", "SDL3-*")
    sdl_arch = sdl / "i686-w64-mingw32"
    cc = toolchain / "bin" / "i686-w64-mingw32-clang.exe"
    if not cc.exists():
        sys.exit(f"error: {cc} does not exist")

    # Asset glue (headers, text fragments, ROM symbol table).
    if not (PC_GEN / "rom_symbols.s").exists() or not (PC_GEN / "rom_symbols_reloc.s").exists():
        env = dict(os.environ, PYTHONUTF8="1")
        subprocess.check_call([sys.executable, str(ROOT / "port" / "tools" / "gen_pc_assets.py"),
                               "--version", VERSION], env=env)

    include_dirs = ["include"] + sorted(rel(p) for p in (ROOT / "include").iterdir()
                                        if p.is_dir() and p.name != "gba")
    includes = " ".join(f"-I{d}" for d in include_dirs + [rel(GEN), "port/include"])
    opt = "-O0 -g" if args.debug else "-O2 -g"
    common = (f"{opt} -std=gnu89 -funsigned-char -fno-strict-aliasing -fwrapv -fno-common "
              f"-mno-ms-bitfields -DVERSION_{VERSION.upper()} -DPLATFORM_PC"
              + (" -DPORT_GBA_RELOC" if args.reloc else ""))
    game_cflags = f"{common} -w -include port/game_prefix.h {includes}"
    # Port sources see the game headers only through #include "...", so names
    # such as malloc.h cannot shadow the system headers.
    quoted = " ".join(f"-iquote {d}" for d in include_dirs + [rel(GEN)])
    port_cflags = (f"{opt} -std=gnu11 -Wall -Wno-unused-function -Wno-missing-braces -mno-ms-bitfields "
                   f"-DVERSION_{VERSION.upper()} -DPLATFORM_PC {quoted} -Iport/include -I{rel(sdl_arch / 'include')}"
                   + (" -DPORT_HD2D" if args.hd2d else "") + (" -DPORT_GBA_RELOC" if args.reloc else ""))
    port_cflags += " -DPORT_RETROACHIEVEMENTS -DRC_CLIENT_SUPPORTS_HASH -Iport/third_party/rcheevos/include"
    # the PPU draws the rows beyond the GBA picture on several cores (port_ppu.c, RenderOuterRows)
    port_cflags += " -DPORT_PPU_THREADS"
    ldflags = (f"-L{rel(sdl_arch / 'lib')} -lSDL3 -lgdi32 -lwinhttp -lcrypt32 -lws2_32 -lmfplat -lmfreadwrite -lmfuuid -lole32 -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic -Wl,--disable-dynamicbase -Wl,--large-address-aware "
               + ("" if args.debug else "-Wl,--subsystem,windows"))
    if args.reloc:
        # above the bus range, which the check build reserves without access (port_memory.c)
        ldflags += " -Wl,--image-base=0x10000000"

    BUILD.mkdir(parents=True, exist_ok=True)
    with open(BUILD / "build.ninja", "w") as f:
        n = ninja_syntax.Writer(f)
        n.variable("ninja_required_version", "1.10")
        n.variable("builddir", rel(BUILD))
        n.variable("cc", "\"" + str(cc).replace("\\", "/") + "\"")
        n.variable("game_cflags", game_cflags)
        n.variable("port_cflags", port_cflags)
        n.variable("ldflags", ldflags)
        n.newline()
        n.rule("cc_game", "$cc $game_cflags -MD -MF $out.d -c $in -o $out", description="CC $in",
               depfile="$out.d", deps="gcc")
        n.rule("cc_port", "$cc $port_cflags -MD -MF $out.d -c $in -o $out", description="CC $in",
               depfile="$out.d", deps="gcc")
        n.rule("as", "$cc -c $in -o $out", description="AS $in")
        n.variable("windres", "\"" + str(toolchain / "bin" / "i686-w64-mingw32-windres.exe").replace("\\", "/") + "\"")
        n.rule("rc", "$windres -I port/decompui/assets $in -O coff -o $out", description="RC $in")
        n.rule("link", "$cc @$out.rsp -o $out $ldflags", rspfile="$out.rsp", rspfile_content="$in",
               description="LINK $out")
        n.rule("copy", 'cmd /c copy /y "$in_win" "$out_win" >nul', description="COPY $out")
        n.newline()

        objs = []
        ra_objs = []
        ra_root = ROOT / "port/third_party/rcheevos/src"
        ra_sources = [ra_root / name for name in ("rc_client.c", "rc_compat.c", "rc_util.c", "rc_version.c")]
        for directory in ("rcheevos", "rapi", "rhash"):
            ra_sources += sorted((ra_root / directory).glob("*.c"))
        ra_sources += [ROOT / "port/achievements/ra_client.c"]
        for src in ra_sources:
            obj = "$builddir/ra/" + rel(src).replace("/", "_")[:-2] + ".o"
            n.build(obj, "cc_port", rel(src))
            ra_objs.append(obj)
        objs += ra_objs
        for src in sorted((ROOT / "src").rglob("*.c")):
            obj = f"$builddir/game/{rel(src)[4:-2]}.o"
            n.build(obj, "cc_game", rel(src))
            objs.append(obj)
        for src in sorted((ROOT / "port" / "src").glob("*.c")):
            obj = f"$builddir/port/{src.stem}.o"
            n.build(obj, "cc_port", rel(src))
            objs.append(obj)
        n.build("$builddir/port/rom_relocs.o", "cc_port", rel(PC_GEN / "rom_relocs.c"))
        objs.append("$builddir/port/rom_relocs.o")
        asm = [PC_GEN / "rom_symbols_reloc.s"] if args.reloc else [ROOT / "port" / "src" / "port_symbols.s", PC_GEN / "rom_symbols.s"]
        for src in asm:
            obj = f"$builddir/port/{src.stem}_s.o"
            n.build(obj, "as", rel(src))
            objs.append(obj)
        # the icon (port/decompui/assets/rekhcom.ico), for the game and the launcher
        n.build("$builddir/port/rekhcom_rc.o", "rc", "port/decompui/rekhcom.rc",
                implicit=["port/decompui/assets/rekhcom.ico"])
        objs.append("$builddir/port/rekhcom_rc.o")
        n.newline()
        n.build("$builddir/khcom.exe", "link", objs)
        dll = sdl_arch / "bin" / "SDL3.dll"
        n.build("$builddir/SDL3.dll", "copy", rel(dll),
                variables={"in_win": str(dll), "out_win": str(BUILD / "SDL3.dll")})
        targets = ["$builddir/khcom.exe", "$builddir/SDL3.dll"]
        if VERSION == "eu":
            targets += decompui(n, toolchain, sdl_arch, args.debug, ra_objs)
        n.default(targets)
    print(f"configured {rel(BUILD / 'build.ninja')} ({'debug' if args.debug else 'release'})")


def decompui(n, toolchain, sdl_arch, debug, ra_objs):
    """The launcher (port/decompui): Dear ImGui on SDL3 + OpenGL, C++."""
    ui = ROOT / "port" / "decompui"
    imgui = ui / "third_party" / "imgui"
    cxx = toolchain / "bin" / "i686-w64-mingw32-clang++.exe"
    n.variable("cxx", "\"" + str(cxx).replace("\\", "/") + "\"")
    n.variable("ui_cxxflags", f"{'-O0 -g' if debug else '-O2'} -std=c++17 -I{rel(imgui)} -I{rel(ui / 'third_party')} "
                              f"-I{rel(sdl_arch / 'include')} -DRC_CLIENT_SUPPORTS_HASH -Iport/third_party/rcheevos/include -Iport/include")
    n.variable("ui_ldflags", f"-static-libstdc++ -static-libgcc -L{rel(sdl_arch / 'lib')} -lSDL3 -lopengl32 -lcomdlg32 -lbcrypt -lshell32 -lole32 -luuid -lwinhttp -lcrypt32 -lmfplat -lmfreadwrite -lmfuuid -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic "
                             + ("" if debug else "-mwindows"))
    n.rule("cxx_ui", "$cxx $ui_cxxflags -MD -MF $out.d -c $in -o $out", description="CXX $in",
           depfile="$out.d", deps="gcc")
    n.rule("link_ui", "$cxx @$out.rsp -o $out $ui_ldflags", rspfile="$out.rsp", rspfile_content="$in",
           description="LINK $out")
    sources = [ui / "decompui.cpp"] + [imgui / f for f in ("imgui.cpp", "imgui_draw.cpp", "imgui_tables.cpp",
                                                          "imgui_widgets.cpp")]
    sources += [imgui / "backends" / "imgui_impl_sdl3.cpp", imgui / "backends" / "imgui_impl_opengl3.cpp"]
    objs = []
    for src in sources:
        obj = f"$builddir/decompui/{src.stem}.o"
        n.build(obj, "cxx_ui", rel(src))
        objs.append(obj)
    objs.append("$builddir/port/rekhcom_rc.o")
    n.build("$builddir/decompui/offline_achievements.o", "cc_port", "port/achievements/offline_achievements.c",
            implicit=["port/achievements/official_set_eu.inc"])
    objs += ra_objs + ["$builddir/decompui/offline_achievements.o"]
    # miniz: the HD importer writes PNG (hd_import.inc)
    n.build("$builddir/decompui/miniz_unit.o", "cc_port", "port/decompui/miniz_unit.c")
    objs.append("$builddir/decompui/miniz_unit.o")
    n.build("$builddir/ReKHCoM.exe", "link_ui", objs)
    art = ui / "assets" / "boxart.jpg"
    n.rule("copy_asset", 'cmd /c (if not exist "$dir_win" mkdir "$dir_win") & copy /y "$in_win" "$out_win" >nul',
           description="COPY $out")
    n.build("$builddir/assets/boxart.jpg", "copy_asset", rel(art),
            variables={"in_win": str(art), "out_win": str(BUILD / "assets" / "boxart.jpg"),
                       "dir_win": str(BUILD / "assets")})
    logo = ui / "assets" / "logo.png"
    n.build("$builddir/assets/logo.png", "copy_asset", rel(logo),
            variables={"in_win": str(logo), "out_win": str(BUILD / "assets" / "logo.png"),
                       "dir_win": str(BUILD / "assets")})
    font = ROOT / "port" / "assets" / "fonts" / "KHMenu.otf"
    n.build("$builddir/assets/fonts/KHMenu.otf", "copy_asset", rel(font),
            variables={"in_win": str(font), "out_win": str(BUILD / "assets" / "fonts" / "KHMenu.otf"),
                       "dir_win": str(BUILD / "assets" / "fonts")})
    font2 = ROOT / "port" / "assets" / "fonts" / "ComicHearts.otf"
    n.build("$builddir/assets/fonts/ComicHearts.otf", "copy_asset", rel(font2),
            variables={"in_win": str(font2), "out_win": str(BUILD / "assets" / "fonts" / "ComicHearts.otf"),
                       "dir_win": str(BUILD / "assets" / "fonts")})
    hud = ROOT / "port/assets/hd-hud/pilot/enemy-hp-frame-v2.png"
    n.build("$builddir/assets/hd-hud/pilot/enemy-hp-frame-v2.png", "copy_asset", rel(hud),
            variables={"in_win": str(hud), "out_win": str(BUILD / "assets/hd-hud/pilot/enemy-hp-frame-v2.png"),
                       "dir_win": str(BUILD / "assets/hd-hud/pilot")})
    labels = []
    for name in ("label-hp-v2.png", "label-vt-v2.png", "label-lp-v1.png", "label-pv-v3.png"):
        source = ROOT / "port/assets/hd-hud/pilot" / name
        target = "$builddir/assets/hd-hud/pilot/" + name
        n.build(target, "copy_asset", rel(source),
                variables={"in_win": str(source), "out_win": str(BUILD / "assets/hd-hud/pilot" / name),
                           "dir_win": str(BUILD / "assets/hd-hud/pilot")})
        labels.append(target)
    # Only reviewed generated pieces enter build/runtime asset folders.
    sys.path.insert(0, str(ROOT / "port/tools"))
    from review_hd_hud import approved_pieces
    manifest = ROOT / "port/assets/hd-hud/sprites/manifest.json"
    if manifest.is_file():
        for piece in approved_pieces(manifest):
            source = manifest.parent / piece["file"]
            target = "$builddir/assets/hd-hud/sprites/" + piece["file"]
            n.build(target, "copy_asset", rel(source),
                    variables={"in_win": str(source), "out_win": str(BUILD / "assets/hd-hud/sprites" / piece["file"]),
                               "dir_win": str(BUILD / "assets/hd-hud/sprites")})
            labels.append(target)
    return ["$builddir/ReKHCoM.exe", "$builddir/assets/boxart.jpg", "$builddir/assets/logo.png",
            "$builddir/assets/hd-hud/pilot/enemy-hp-frame-v2.png",
            "$builddir/assets/fonts/KHMenu.otf",
            "$builddir/assets/fonts/ComicHearts.otf"] + labels


if __name__ == "__main__":
    main()
