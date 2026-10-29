# Issue459: safe-startup library selection

**29/10 update:** the reporter confirmed the corrected compatible path boots
with `--safe-startup` and that the HD lettering test restores the fonts.
Fresh failed-normal-launch logs now stop at native64 `window/begin`, after
SDL3.4.18 initializes X11/PipeWire successfully. The accompanying game log is
from an earlier successful safe32 session; it is not normal64 guest entry.
HD lettering has a separate reproducible
[portable warmup correction](issue459-hdtext.md). The issue remains open.

The reporter's readable logs contain two independent conditions: native32 safe
startup resolves host GL libraries against the older bundled libc and fails for
missing `GLIBC_2.38`; the Steam Linux Runtime run enters the game but cannot find
FFmpeg/FFprobe, so it uses cartridge movies. A later screenshot also confirms the
optional Python sprite importer lacks NumPy/Pillow. These are distinct dependency
paths, not evidence of a lost import or a division fault.

## Startup correction

Software rendering does not make an incompatible C-library/driver pair valid.
Safe startup now preflights an available system32 loader and its library paths
before choosing the bundled fallback. It still forces native32 and software for
one session, preserves saved architecture/configuration, and never retries after
a game session starts. Explicit `REKHCOM_GPU=0` without safe startup retains its
previous bundled behavior. Failed loader preflight errors remain in the log.

This changes the x86 Linux32 wrapper route. It is not a renderer/driver change for
Android, Windows or consoles, and it does not certify ARM/Steam Deck hardware.
The runtime's actual library resolution matters; Valve likewise distinguishes
[host and runtime library paths](https://github.com/ValveSoftware/steam-runtime/blob/master/templates/run.sh)
and documents [Steam Linux Runtime compatibility environments](https://github.com/ValveSoftware/steam-runtime/blob/master/doc/steamlinuxruntime-known-issues.md).

## Verification

- Before the patch, the two new wrapper-selection regressions fail; the explicit
  non-safe bundled control passes. With the patch, all29 launcher tests pass.
- A real ELF32 analogue gives a host DSO a versioned-provider requirement absent
  from the older bundled provider. The published wrapper selects the old provider
  and the loader reports `LIB_2.38 not found`; the candidate wrapper selects the
  compatible system fixture and executes `_start`, preserving its exit17.
  This exercises the actual dynamic loader/version mechanism, not the physical
  reporter's GL libraries or a full Steam container.
- The actual closed v2.2.5 Linux archive's native32 game/assets, with only the
  candidate wrapper replaced, reaches its directed title with software rendering
  in the512MiB offline Linux VM; saved architecture preferences remain identical.
- VM shutdown was verified and the protected base-image identity stayed unchanged.

The fix is separate from the remaining media tools and optional importer setup.
The reporter also confirmed v2.2.5 still fails to launch; it predates this wrapper
correction. Keep #459 open until the candidate is tested on that device. No
resolution, codec, bitrate or imported content was changed by this patch.

## Normal64 window stall: bounded recovery candidate

The last flushed boundary narrows the failure to window creation; it does not
prove whether the process stalls or terminates inside a specific X11 call.
SDL's X11 show path includes a blocking `XIfEvent(MapNotify)` wait. Both packaged
architectures use the same SDL version, X11 configuration and pixel-density
flag, so those alone do not explain the difference. The loader explicitly
ignores the reported wrong-ELF-class overlay warning; SDL initialization then
succeeds. Do not attribute the failure to that warning without further evidence.

The candidate separates hidden window creation and `SDL_ShowWindow` milestones,
keeping show before renderer creation and keeping the preferred GPU renderer.
A 30-second deadline bounds native window/renderer preparation. Timeout and
guest entry compete for one atomic gate; the monitoring thread is joined before
the entry marker is written. It never survives into gameplay and cannot restart
an entered game. Timeout logs through the raw file descriptor and exits75,
without calling SDL video teardown from a second thread.

Only Auto, exit75 and an absent entry marker select compatible32 software,
using the already-confirmed safe path without changing saved preferences.
Force64 reports the failure; post-entry errors never retry. Other existing
bootstrap error policies remain unchanged. Failed entry-marker write/close
now aborts before guest code as well, matching the existing Windows64 contract.

Focused checks: `test_startup_deadline.py` uses real SDL threads/semaphores and
atomics, including expiry/entry races, post-entry survival, cancellation and
allocation failures. `test_linux64_startup_stages.py` compiles the actual startup
block with injected SDL failures/nonreturns. `test_linux64_boot_marker.py`
injects open/write/close failures into the actual guest-entry boundary; the
baseline incorrectly enters after a failed write. Launcher tests cover Auto,
Force64, entered games and child-only software recovery.

The Debian512MiB snapshot passed those real SDL thread cases, entered the
directed title through the actual candidate64 host and the closed v2.2.5 Linux
guest, and recovered an injected `SDL_CreateWindow` nonreturn through the actual
wrapper in30.18seconds (exit75, absent marker, compatible32 stub reached).
The fallback consumer in that last case is deliberately a recording stub; the
real compatible32 game was verified separately above and by the reporter.
Dummy video/software drawing does not certify X11, GPU or physical SteamOS.
The protected VM base size/mtime stayed unchanged and the owned VM shut down.

This is recovery for a bounded startup phase, not a confirmed repair of the
physical X11/Steam cause. Keep the issue open for the next reporter test.
