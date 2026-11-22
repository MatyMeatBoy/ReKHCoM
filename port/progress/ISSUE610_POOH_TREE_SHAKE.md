# Pooh tree collision: shared camera shake without a battle camera

Issue: https://github.com/MatyMeatBoy/ReKHCoM/issues/610

Pooh's final bee-chase collision calls `BtlMapStartShake()`. Its map task
then advances the shared shake through `BtlMapUpdateShake()`. After 32 updates,
that function resets the battle camera rotation even though Pooh mode has no
allocated battle camera. Event initialization explicitly clears `gBtlWork`.

The fix checks that pointer before resetting its rotation. The shake waveform,
duration and offsets remain unchanged. A valid battle camera still resets at
the same update. This is a pointer guard rather than a battle-mode guard,
because event-mode battles can legitimately own a battle camera.

## Regression evidence

`port/tools/test_pooh_shared_shake.py` extracts the actual three shake functions
and pattern from `src/btl/btl_map.c`, then compiles an isolated C fixture. Run
with explicit `--cc <compiler>` and `--output <private-output-directory>`.
It covers idle/active shake with null/valid cameras, preserves sentinel bytes,
checks all 36 updates and verifies the valid camera rotation reset.

Before the fix, native Windows 32-bit and 64-bit fixtures fault with
`0xC0000005` at update 32 in the null/active case. After the fix, all four cases
pass on both architectures. All three nonfaulting controls have byte-identical
CSV traces before and after.

Private evidence lives under `pruebas/issue610-pooh-tree-crash-v29/` outside
the repository. Runtime probes seed a private checkpoint at the final bee-chase
leg; real movement, collision, shake, tail and camera tasks run afterward.
They do not replace those tasks with simulated outcomes, and do not establish
a full player-input playthrough or confirmation on the reporter's computer.

Both current Windows runtime paths already redirect NULL-page game writes to
a BIOS stand-in, matching ignored BIOS writes on GBA. Consequently the before
runtime probes complete: the logs show a real collision, all 30 tail fall
updates reaching ground, and shake completion with a null camera. The native
32-bit before log records `bios: write 00000018` at that point. This compatibility
can mask the invalid access; it does not make the native dereference safe.
Both after probes also reach the tail on the ground and complete normally;
the native 32-bit after log no longer reports the BIOS write at `0x18`.

Source coverage: Linux i386 installs the native signal-based bus handler;
Linux x64 installs the guest BIOS handler, including ignored writes. Android
32-bit and Linux ARM provide only explicit BIOS pointer helpers, with no
fault recovery. Android64 uses the AArch64 shared guest's no-op installer and
a host crash logger that restores the default signal handler. These latter
paths therefore lack the Windows masking for this unguarded write, but have
not been tested on their physical devices in this pass.

This establishes an invalid access in the reported transition, but does not
reproduce the reporter's visible crash on current Windows or establish its
sole cause. The public report does not identify platform/version. Keep issue
610 open pending that evidence or a matching reproduction. Other platforms
require their own runtime checks. No original ROM or user save is modified.
