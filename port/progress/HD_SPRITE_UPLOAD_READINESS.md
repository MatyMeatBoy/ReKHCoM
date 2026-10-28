# HD sprite upload readiness before native omission

Keep the original eager preparation of the base and white textures. The lazy
white proposal was measured and withdrawn because it moved material work into
the first flash. The remaining change corrects acceptance and fallback without
claiming a performance improvement or adding alpha/scratch storage.

## Required resource before acceptance

The guest SDL shim defers host upload until `HostTex`. A non-null CPU texture
does not prove that its host allocation/upload succeeded. The local
`PortBridgeTextureReady` helper uses the existing command, filter and renderer
epoch before the base HD picture is accepted. When a palette fade or global
OBJ brighten requests white, Queue checks its white texture too. A rejected
entry keeps native pixels and the original fade; existing CPU pixels remain
available for host-upload retry.

This adds no bridge ABI command/table entry. Native SDL, including its software
renderer, retains its normal immediate texture creation and does not call the
bridge helper. The current shim's direct legacy and GPU-target draw paths both
consume `HostTex`; neither has a CPU texture-only draw path that can bypass it.
Blocked output is rejected. Layer2 retains its palette-only brighten behavior.

Native white textures still exist before the first fade, and Queue adds no
white conversion or allocation there. The guest already requested white host
upload on its first white draw; its existing upload is checked before omission
instead of failing after the native pixels have been discarded.

## Resource and failure checks

The original two-RGBA reservation, 32 MiB native / 12 MiB relocatable guest,
stays unchanged. Decode uses one temporary RGBA plane, modified in place to
prepare white. The owned decode-buffer probe peaks at 4 bytes per source
pixel and returns to zero after success or each failure. There is no retained
alpha plane or newly deferred RGBA scratch allocation. Reservation/owned-buffer
counters are not measurements of process RSS, driver replicas or VRAM.

Production loader/queue/PPU probes cover base/white readiness rejection,
native-pixel retention, later retry without decoding/creating CPU copies again,
palette/global brighten, unaffected effects/layers, decoder allocation,
surface/create failures, unchanged budget/pinning and repeat unload. Replaying
the old non-null-object decision fails the new bridge-rejection assertion.
The full shim and real host slot consumer execute actual `h_tex_create`
allocation/upload rejection, both filters, renderer generations and cleanup
on direct legacy/software and GPU-target routes. These are injected CPU tests,
not physical console/mobile-device verification.

## Measured alternative rejected

Private production `Load`/queue/`Quad` probes compare eager and lazy policies
with generated RGBA PNGs and real SDL D3D11. Four frames per size (white
0/64/128/255 with alpha/darkening) are byte-identical. API wall-time medians:

| Source size / rounds | Eager Load | Lazy Load | Eager first-fade queue | Lazy first-fade queue |
| --- | --- | --- | --- | --- |
| 128×128 / 5 | 0.747 ms | 0.732 ms | 0.0005 ms | 0.289 ms |
| 512×512 / 5 | 3.943 ms | 4.030 ms | 0.0006 ms | 0.895 ms |
| 2048×2048 / 3 | 22.696 ms | 17.082 ms | 0.0012 ms | 8.743 ms |

At the permitted maximum, a 5.6 ms Load reduction moved 8.7 ms into the first
flash. Texture creations changed from two to one initially and returned to two
at the fade; those counts did not justify introducing that flash-time cost.
The lazy code was removed. A native software-renderer control also produced
four byte-identical frames without any bridge upload calls.

The final eager/readiness policy was then compared against current main at
2048×2048 for three rounds. Both created two textures at Load and still had two
after the fade; first-fade queue medians were 0.0011 / 0.0014 ms, with all four
frames byte-identical. The Load implementation itself is unchanged from main;
the final policy adds no deferred white construction at that point.

Evidence stays private under `pruebas/hd-lazy-white-qa/`,
`pruebas/hd-lazy-white-max-qa/` and `pruebas/hd-lazy-white-software-qa/`.
The final-policy comparison is in `pruebas/hd-eager-readiness-qa/`.
These probes are not whole-menu frame measurements or GPU timers. Source art
and published packages are unchanged; physical device and long-session testing
remain separate gates.
