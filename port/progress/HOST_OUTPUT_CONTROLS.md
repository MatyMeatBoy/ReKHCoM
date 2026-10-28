# Negotiated SDL host output controls

The profiling pilot proved different active battle canvases at the same zoom:
native Windows1920x1080 produced480x270, while the64-bit host's fixed1280x720
produced320x180. The guest also reported VSync applied although its setter only
updated a local field and the host forced interval1. These are output contracts,
not evidence of an arithmetic, scene or device-specific crash.

Use optional OUT1 commands0x230..237 through the existing HF_thread slot, without
changing table layout/signatures or occupying MOV1/PRV1 commands. Probe the exact
magic and capability bits before setters. Acknowledgement requires the real SDL
operation and observed state; getters query the host rather than cached intent.
Desktop Windows/Linux support sizing; Android64 advertises no sizing capability.
SDL failures and old hosts return unsupported/false, preserving their output.
Creating the guest window remains possible with a fixed-size/legacy host.
The producer exposes the VSync option only when the actual guest renderer can
query it. Its SDL shim's GetRenderer(NULL) returns that renderer; this patch is
not applied to native SDL callers. Unsupported/legacy hosts keep the row hidden.

Implementation: shared protocol and SDL host command header, dispatcher wiring
in Windows/Linux/Android64, and negotiated SDL guest window/VSync calls. Apply
the guest's requested initial size (including its normal user configuration),
then dynamic settings through the same commands. Do not add a test-only renderer
that would conceal unsupported user controls or change gameplay resolution rules.

Validation sequence: observed no-op setters/fixed-window baseline; CPU tests of
real production functions with accepted/rejected SDL operations and old hosts;
compile affected guest/host targets using existing toolchains. Preserve closed
baseline binaries/caches and relink privately. Finally run two controlled GPU
pilots only in the integrator's window, verify actual canvas/window/VSync, then
resume equal-quality timing separately. CPU acknowledgements alone do not close
the profiling parent, certify physical Android output or demonstrate a gain.

CPU validation: the old production window setter failed the expected-false
contract; the old producer also hid VSync for a supported renderer. The new
production functions/shared header pass54 injected checks (desktop and fixed
surface), plus supported/unsupported visibility through both producer variants.
Windows host links; Linux/Android64 hosts pass actual cross-toolchain syntax
checks. Guest SDL/UI translation units compile for x32, ARM64_32 Android/Linux/
Switch and ARMv7 Vita; the console UI variant was checked separately. A private
Windows guest relink replaces only SDL shim, emitted UI and the already-merged
movie-volume TU relative to the preserved07d5 baseline. The earlier GPU validation was pending; it is supplemented by the current
Windows functional evidence below. Published packages remain unchanged.


## Windows functional validation, 28 October 2026

Two real SDL Monstro-HD runs passed on source `ea341949894bfd686b041e938323816a1c2e8ba0`: baseline `3988eaac3` plus the product change from this PR (`4eb48259`). Eight of its nine file blobs match exactly; the Linux host also retains the independently integrated startup diagnostics. The current PE/guest ELF supplied the observed state/screen symbol addresses.

Both owned processes reached actual GameState world5/stage5/floor0. At a measured 1920x1080 client (DPI120), the canvas was480x270; the existing resize action then requested1600x900 and both canvases became400x225. The x64 host acknowledged both requested/actual window sizes and VSync requested0/actual0. Each architecture logged20 HD draw entries and two output captures, with the same controlled HD flags/view4/zoom8. Captures were inspected; native and host frame counters are separate, so this does not claim pixel-identical combat poses.

Private evidence: `pruebas/v225-out1-functional/attempt-20261028-185056/VERIFIED-SUMMARY.json`, current symbol/source/build receipts in `releases/v2.2.5-candidate/out1-functional/`. Two earlier observer failures are retained: omitted native `--data-dir` ignored the intended INI, then omitted A input left the debug battle menu waiting. The observer rejected both instead of accepting the wrong scenario. All test children are terminal.

These were functional FAST runs, not FPS/latency measurements, a full HD import audit, a dynamic UI VSync-toggle test, or Linux/Android/console hardware certification. This evidence does not close the profiling parent.
