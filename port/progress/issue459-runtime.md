# Issue459: safe-startup library selection

**29/10 update:** the reporter confirmed the corrected compatible path boots
with `--safe-startup`. Normal startup still fails; fresh failed-launch logs were
requested. HD lettering has a separate reproducible
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
