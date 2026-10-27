# Linux ARM64 experimental bridge host

The host is Linux AArch64 LP64; the game uses AArch64 instructions with 32-bit
pointers (`arm64_32-apple-watchos`, Mach-O). This preserves existing guest data
layouts. It requires a 64-bit ARM CPU and operating system. ARM32 remains a
separate `armhf` build.

The host includes the desktop Linux SDL implementation, including texture
handle generations, renderer reset recovery, HD audio and ffmpeg movie playback.
The guest reuses the Switch toolchain with Linux bridge multimedia enabled,
without Switch options, touch controls or PPU worker threads.

Build serially from the repository root:

```powershell
python port/tools/linux_prepare.py --arch arm64 --jobs 1
python port/linuxarm64/build_linuxarm64.py --jobs 1
```

`--component host` and `--component guest` build either half independently.
The builder writes `build/linuxarm64/out/binarm64/rekhcom-arm64`,
`binarm64/guest-{eu,us,jp}.macho` and `libarm64/`. These are experimental
components, not a complete validated release archive. No ARM64 package is
added to normal release staging and normal preparation does not start ARM64
work unless `--arch arm64` is explicit. Current desktop/Deck x86+x64 and armhf
Anbernic artifact names keep their existing meaning.

For a future isolated test folder, put those components alongside the common
assets, icon, `roms/`, `ReKHCoM.sh` and `LINUX.md` from the matching source
package. Optional ARM32 binaries/loader must be placed in separate
`binarmhf/` and `libarmhf/` directories; never reuse x86 `bin/lib` as ARM32.
The wrapper selects CPU architecture using `uname -m`, honors automatic /
force32 / force64 and the ready-marker contract, and never launches x86 on ARM.
Do not relabel a desktop archive as ARM64 or distribute this manual staging
as validated hardware support.

The loader reserves free addresses below 4 GiB without replacing existing
mappings. It validates Mach-O commands, segment boundaries, pointer rebases and
bridge entry addresses, then applies segment permissions and flushes the
instruction cache. Set `KHCOM_IMAGE_MOVE=1` to exercise relocation. A bootstrap
failure before the ready marker may fall back to ARM32 in automatic mode;
once the guest begins, its exit or crash never starts another session.

Current limits:

- Cross-compiled host syntax, LP64/ILP32 ABI objects, loader logic and wrapper
  architecture selection are checked. A complete ARM64 SDL/game build and a
  game session on Linux AArch64 still require integration validation.
- Mach-O segments are 16 KiB aligned. Linux 4 KiB and 16 KiB page sizes satisfy
  this alignment; kernels using 64 KiB pages reject the image safely. This is
  not a claim that all 4/16 KiB devices have been tested.
- Forced ARM32 on an ARM64 kernel requires that kernel's AArch32 support.
  The bundled ARM32 loader is checked before entering the game.
- SDL driver availability, execution policy and suitable low address space
  remain system requirements. No FPS claims have been established.

The small `port/tools/test_linuxarm64_loader.c` probe tests relocation decoding,
invalid streams and pointer sliding without executing game instructions. It
can run on Windows with LLVM or on POSIX. The Windows path uses VirtualAlloc
only for a 64 KiB scratch region, not as a simulation of the Linux mmap loader.

## Integration evidence — 27 October 2026

Source spike9300d2992 is preserved on codex/linux-arm64-feasibility. This PR
cherry-picks only that commit onto current main (rebased after34132bd71), retaining renderer
reset/texture ownership, HD streaming, safe desktop entry and runtime changes.
Nested khcom-worktrees toolchain discovery is supported. Guest configuration
is idempotent and leaves touch/PPU-thread options disabled for this target.

Actual lightweight checks: ARM64 ELF LP64 host syntax using existing Debian
arm64 ABI sysroot and SDL headers; ELF AArch64 LP64 / Mach-O arm64_32 ILP32
objects with pointer/bridge assertions; guest UI/gallery/movie syntax;
builder contract mocks; portable loader ULEB/opcode/bounds/slide tests; shipped
wrapper architecture dispatch and no second session after guest entry.
Malformed ULEB offset plus ADD_ADDR_IMM wrapped to zero in the old rebaser
(RED); offsets/additions now reject overflow (GREEN). These checks execute
no game instructions. Sysroot/SDK files are read only; no new dependency build.

Still required before promoting this target: full serial SDL/host and EU/US/JP
guest link, validate complete actual Mach-O segment/rebase layout (including
forced relocation), then boot/title/scene/audio/input/save/quit tests in isolated
AArch64 Linux or an emulator, followed by real device GPU/driver/performance QA.
Current loader tests mock mappings on Windows; they do not prove mmap/protection
or instruction execution on Linux. No FPS, driver, handheld or release claim.

Re-run lightweight contracts without building dependencies or the game:

```powershell
python port/tools/test_linuxarm64_build.py
python port/tools/test_linuxarm64_loader.py
python port/tools/test_linux_launcher.py --shell "C:/Program Files/Git/bin/sh.exe"
```

On Linux use `--shell /bin/sh`. Host syntax needs an existing arm64 sysroot
and SDL3 headers; absence is a missing prerequisite, not a successful build.

### Preserved whole-image review fixture

The same loader test command now also runs `test_linuxarm64_image_win.c` on
Windows, preserving the independent review fixture from save_corpus_research.
It invokes production LoadArm64Image with a complete synthetic Mach-O: preferred
and forced-move addresses, relocated boot/table/stack, valid protection ranges,
overlap/file bounds rejected before mapping and invalid table rolling mapping
back. No guest instructions run. VirtualAlloc keeps pages RW; the mprotect
adapter validates requests/ranges and refuses WX, but does NOT test Linux
mmap/mprotect, executable permission enforcement or AArch64 execution. POSIX
runs the portable decoder probe and explicitly skips this Windows-only adapter.
This expands logic coverage, not runtime/hardware readiness.
