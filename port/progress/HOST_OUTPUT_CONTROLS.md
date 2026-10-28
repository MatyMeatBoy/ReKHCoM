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
movie-volume TU relative to the preserved07d5 baseline. GPU validation remains
pending; old binaries and published packages are unchanged.
