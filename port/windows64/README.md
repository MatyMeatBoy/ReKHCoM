# Windows x64 alpha

Native PE x86-64 SDL3 host, using the normal Windows GPU driver. The shared
x32 game image retains 32-bit pointers and game layouts with x86-64 instructions.

```powershell
python port/windows64/build_windows64.py --regions eu us jp --jobs 1 --test
python port/windows64/build_windows64.py --host-only --test
build/windows64/out/rekhcom64.exe build/windows64/out/guest-eu.bin --data-dir DATA --rom OWN_ROM --language es
```

Package files: rekhcom64.exe, SDL3.dll (x64), guest-eu.bin/us/jp.bin.
Exclude test_windows64.exe, test_movie.exe and all test data; never package ROMs or saves.
Keep Windows32 available. Remaining limitations are listed below.

## Launcher contract

--data and --data-dir select the same persistent data root as Windows32.
--rom overrides KHCOM_ROM. --language sets KHCOM_LANGUAGE, passed to the
shared guest main as --language; this needs the updated platform_ps5.c image.
--boot-status PATH removes a stale marker before checks, then writes, flushes,
commits and closes guest-entered immediately before entering guest code.
A marker creation error returns 4 without booting the guest. Once the marker
exists, never retry Windows32, including after guest exit 81 or a guest crash.
Pre-guest failures: 2 image/arguments, 3 low allocation, 4 SDL/renderer/marker.
Loader failures happen before host code. The launcher must use a unique marker.

## Persistence and UTF-8

No architecture suffix is introduced. The current shared guest uses exactly
the native save names: EU kh_com.sav, US kh_com_us.sav, JP kh_com_jp.sav;
bank N uses *_bankN.sav. kh_com.ini and all assets use the same data root.
Windows filesystem calls convert UTF-8 paths to UTF-16. Save replacement uses
MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH), including an existing save file.
This preserves in-game SRAM saves. The guest uses state_ps5.c for quickstates,
with strict per-build validation: native32 and x32 snapshots are not interchangeable.
Optional KHCOM_HOST_KEY_EVENT queries forward real key-down/up events to the guest,
including F5 save, F8 load and F6/F7 slot selection when enabled in Cheats.
Unsupported hosts return -1 and keep the existing gamepad-only path.
Keyboard-to-pad emulation is disabled after negotiation so remapping does not
produce duplicate actions; queue overflow releases held actions via focus-lost.

## ABI and renderer

Every bridge entry is a typed SysV wrapper; SDL, Win32 and libc use native
Microsoft x64. Boot is explicitly SysV. VirtualAlloc requests fixed low memory
and fails without replacing existing mappings. The shared texture slot helper
rejects stale handles and publishes a new generation after renderer reset.
Requires the host_texture_slots.h change from 1fe850997.

## Verification

Host compiled with LLVM MinGW and SDL3 x64. --test checks:
- RDI/RSI/XMM6/XMM15 and stack canary across MS64 to SysV.
- Mixed integer/float arguments, double returns, fifth native integer argument.
- Math, file, directory, environment and thread wrappers.
- Low address allocation collision preserves existing contents.
- SDL renderer reset invalidates old handles without freeing a new allocation.
- UTF-8 save path and replacement of an existing save with new contents.
- Process contract: stale marker cleanup, error2/error4 without marker,
  and synthetic guest return81 with a durable marker (no ROM needed).

Existing EU image smoke tests, isolated folders and silent audio:
- Native Direct3D11: room10901 / doorpose0 / A240, clean exit500.
  Frame400 reviewed: Room Synthesis displays correctly.
- Event107: field -> Event frame121 -> battle frame1182, exit1800 rc0.
  Partial smoke, not a complete won story chain.
- Monstro Shadow100: battle121 stage5, battle frame62, exit500 rc0.
These runs lacked HD assets/fonts. Repeat with final images and full resources.
KHCOM_HOST_HIDDEN=1 hides smoke windows; KHCOM_HOST_SHOT=400 captures frame400.

## Media Foundation movies

HD music uses shared/hd_music_host.h. Native Windows64 HD movies reuse the
Windows32 Media Foundation decoder and subtitle source. prepare_movie_core.py
extracts those functions at build time with explicit source-anchor checks;
no generated source is packaged. H.264/AAC decoding needs no bundled ffmpeg.
NV12 uses the native GPU; software renderers use RGB. Missing/corrupt video
returns 0 before any picture so the normal ROM fallback remains available.
Movie @second offsets, subtitle text/font, sound/fades and the file timeline
match the shared decoder. A/B outside pause never skip. START/Escape pauses;
Continue or Skip -> Yes/No is implemented in the host without calling guest code.
The host pause UI is a separate implementation, localized in five languages.
It has keyboard/gamepad navigation. No guest pointers or unsafe guest reentry.
Viewport, clip and scale are restored on exit. Real renderer resets invalidate
host texture handles and publish an epoch; normal movies retain guest textures.

```powershell
python port/windows64/test_movies.py --font PATH_TO_KHMenu.otf
```

The test uses ffmpeg only to generate a three-second synthetic clip, never at
runtime. D3D11 tests: normal decode/subtitles (90 frames), seek @2 (shorter),
Skip confirmed, cancel/continue (all 90 frames), device reset, Unicode path,
software RGB fallback, corrupt and missing files. Logs/report and screenshots
are under build/windows64/movie-tests. A real EU guest also returned from
ROP to room10901; frame400 reviewed, clean exit450. Final guest images still
need end-to-end quickstate/input validation after rebuilding.

## Remaining limitations

UDP currently reports no socket. PPU uses one core. No Windows32 fault-recovery
handler is installed: invalid guest accesses remain observable crashes.
Do not silently prefer this runtime when these missing features are required.
Device profile queries use the common read-only host provider. Requires its
port_device_profile_host.h and port_device_profile.h integration before build.
