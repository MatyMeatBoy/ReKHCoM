# ReKHCoM for PS4 (alpha)

**Alpha, never run on a console yet.** A fake-signed package for a PS4 with a homebrew enabler that allows
read/write/execute memory (GoldHEN).

## How it works

The game needs 32-bit pointers and the GBA regions at their hardware addresses. The PS5 build already solves that
with an x32 image of the game (`port/ps5/guest`, see `port/ps5/README.md`), and the PS4 runs the same x86-64 code.
`ps4_host.c` is the PS4 program around that image:

- maps the GBA address space (`0x02000000`-`0x0E010000`) and the image at `0x10000000` as RWX memory;
- serves the bridge (`port/ps5/shared/bridge_list.h`): files under `/data/ReKHCom`, a software rasterizer for the
  game's quads into SDL2's window surface, 48 kHz sound through `sceAudioOut`, the first controller through `scePad`;
- recovers stray pointers and divisions by zero the way the cartridge does, with the PS5 host's handler.

Without `-DREKHCOM_PS4` the same file builds a headless Linux test host (`build_ps4.py test-host`).

## Install

1. Install `ReKHCoM-vX-ps4-alpha.pkg` (Package Installer / GoldHEN).
2. Copy your own cartridge dump (EU, US or JP, any name ending in `.gba`) to `/data/ReKHCom/` (FTP).
3. Start ReKHCoM. The log is `/data/ReKHCom/logs/ReKHCoM.log`; saves and settings sit next to the ROM.

Controls: Cross/Circle/Square/Triangle, L1/R1, D-pad or left stick, Options = Start, touchpad = Select;
touchpad + L3 saves a state, touchpad + R3 loads it.

## Build

```
python port/ps4/build_ps4.py            # guest images (eu us jp), host, package
python port/ps4/build_ps4.py host       # host and package only
```

Needs the OpenOrbis toolchain v0.5.4 (`OO_PS4_TOOLCHAIN`, default `D:/01Claude/ReKHCoM-build/ps4/toolchain`), clang/ld.lld
(the repository's llvm-mingw on Windows) and .NET 6+ for PkgTool. Output: `port/ps4/dist/ReKHCoM-ps4-alpha.pkg`.

## Not available yet

No automatic updates, no HD text in the Japanese version (no CJK font), one controller.
