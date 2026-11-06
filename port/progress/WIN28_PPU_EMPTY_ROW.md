# Windows2.8: transparent4bpp tile-row fastpath, source-only

Parent task `win28-vrr-field-pacing-20261106`; subtask
`win28-field-row-fastpath-20261106`, isolated base27ad033d. Report: another PC,
RX6900XT,1440p144Hz, world less fluid than arenas and worse at x2/resolution.
The GPU model alone does not identify the bottleneck. No reporter-device
performance or new executable has been measured in this pass.

The existing `DrawTextBg` world-extension path already handles one tile row
at a time. When all four4bpp row bytes are zero, its nibble/color loop still
runs up to eight times. The new byte-only predicate detects that case and
writes exactly the same uint16 `TRANSPARENT` values in the clipped run.
Nonempty rows retain the previous decoder. No byte memset for0x8000.

The optimization stays after the existing world/exterior/mosaic/window-map
guards, VRAM bounds checks and font-tag eligibility. `sInWorld` is still set
for the complete run before either decoder.8bpp, glyph registration, flipped
rows, clipped runs, active palettes, native GBA pixels, actors and timing keep
their existing paths. Empty rows do not consult palette slot0, even if its
color is opaque. No cross-frame cache, allocation or threaded ABI is added.

Correctness before runtime:

- Missing-branch regression observed before implementation.
- Three pure Python tests interpret the actual new C branch/helper:
  73584patterns/offsets/runs1..8/both flips, opaque palette0, prefix/suffix
  ownership and544world/font/VRAM/8bpp/mosaic guard combinations pass.
-18450additional before/after C-AST comparisons use local readonly ROM
  rows from095/097/120, including empty, shaded, flipped and palette rows.
  The complete PPU source outside this branch and its include is unchanged.
- `git diff --check` passes. No compiler, C fixture, game or import generator
  was run. Private evidence: `pruebas/pacing-v28-6900xt-20261106/`.

This proves source/data equivalence for the exercised cases, **not a measured
performance gain**. The additional test can cost time on mostly opaque rows;
actual dynamic field occupancy and native CPU timing remain unmeasured.

`port/tools/bench_ppu_empty_row.c` is prepared but **not compiled or executed**.
When compilation is authorized, compile it with the relevant target's
optimization flags and `-Iport/include`. It warms both paths, compares identical
data/checksums with five alternating-order rounds, and reports CPU seconds for
0/50/90/100% target-zero mixes with exact row counts. It is a synthetic decoder
microbenchmark; it cannot certify game FPS, PPU-frame latency or GPU behavior.
Retain compiler/CPU/source/configuration and all samples, then measure the same
real field/arena workloads at matching canvas/quality before claiming speedup.

Separate findings in `FIELD-FINDINGS.md`: existing FPS counts pacing cycles
even when drawing/presentation is skipped; guest Windows64 PPU is serial;
late-deadline reset can add a full period after a long stall. Root handles the
FPS/VRR work. This patch changes neither Pace/deadlines,59.7275Hz logic,
frameSkip policy nor guest thread support, and does not reduce view or quality.
