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
`binarm64/guest-{eu,us,jp}.macho` and `libarm64/`. Copy those directories into
the universal Linux bundle alongside `bin/lib` (x86), `bin64/lib64` (x86-64)
and `binarmhf/libarmhf` (ARM32). The wrapper selects CPU architecture using
`uname -m`, honors the existing automatic/force32/force64 preference and boot
marker contract, and never launches an x86 guest on ARM.

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
