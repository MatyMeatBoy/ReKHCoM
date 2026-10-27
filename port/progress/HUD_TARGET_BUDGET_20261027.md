# Gauge scratch targets — 2026-10-27

Task `hud-target-budget`, owner `codex-hud-budget`, branch
`fix/hud-target-budget`, base `249c55e88`.

The gauge uses optional RGBA render targets to smooth its edges. SS4 needs
`(16 + 4 + 1) * width * height * 4` bytes across its three cached targets;
SS2 needs `(4 + 1) * width * height * 4`. With the existing 680×280 layout area
and 0.375 unit conversion, a HUD scale of 13.5 produces a 3443×1418 base area:
SS4 would request about 391 MiB before renderer allocation overhead.

The new scratch-payload ceilings are 32 MiB for desktop and 12 MiB for Android
and other constrained hosted builds. These are explicit optional-cache policy
limits, not measured total VRAM limits or evidence that the Monstro crash was
an OOM. They permit SS2 at HUD scales 6.75 (~23.3 MiB, desktop) and 4.5
(~10.4 MiB, mobile), while larger HUDs draw the same HD geometry directly.
Freestanding guests already draw directly because their SDL shim has no render
targets; this remains unchanged.

Before allocating, the complete chain must fit both the byte ceiling and
`SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER`. Unknown renderer limits use direct
drawing. Float dimensions are checked before integer conversion; target
dimensions are checked before scaling; byte sums use checked uint64 accounting.
Resizing releases the old chain first. Partial allocation failure releases the
partial chain before trying SS2/direct. A usable SS2 chain is reused at the same
geometry after a larger allocation failed. Shutdown releases all target slots.

Evidence: the real production draw/cache/shutdown functions run in a counted
renderer harness, without a game, ROM, installed save, real GPU or heavy build.
Baseline failed 18 cases; an additional test first reproduced repeated rejected
SS4 allocation despite an existing usable SS2 cache. All 30 desktop/mobile cases
now pass under ASan/UBSan: valid supersampling/cache reuse, byte budgets, 4K-sized
output, renderer limits, unknown maximum, float/integer overflow, NaN/infinity,
partial/total allocation failure, resize overlap and shutdown ownership. The six
existing layout bounds/readiness tests also pass under ASan/UBSan.

Actual GPU allocation overhead, visual quality, GPU timing and device-reset
recovery are not measured by this harness. No lifecycle reset behavior changed.
