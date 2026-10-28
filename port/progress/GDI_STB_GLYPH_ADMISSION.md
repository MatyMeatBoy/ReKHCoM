# GDI/stb glyph upload admission

The GDI and stb producers marked a glyph ready before creating its texture and
ignored `SDL_UpdateTexture` rejection. A non-null texture could therefore hide
the original glyph without containing accepted pixels. The corresponding
FreeType/Web guard did not cover these producers.

Both now require successful creation and update before retaining the texture.
Allocation/create/update refusal clears the glyph readiness, destroys a rejected
texture once and releases the temporary bitmap. A generation-scoped latch stops
further allocation/log retries. Shutdown resets the latch after normal teardown
ownership; no preference is changed. The existing portable PPU readiness gate
now applies to Windows GDI as well, so the next frame retains original text.

This does not repair the first PPU pass that already omitted its original glyph;
that frame can still miss text. Empty glyph bitmaps remain legitimate cached
spaces and do not disable HD text. Accepted ARGB pixels, metrics and options are
unchanged. Reinitialization owns recovery; there is no per-frame retry loop.

## Focused evidence

`python port/tools/test_gdi_stb_upload_admission.py --out <private-directory>`
extracts each actual GlyphTexture producer and the actual PPU admission slice.
Both i686 and x86_64 CPU fixtures cover accepted pixels/metrics, allocation
refusal, texture creation refusal, update refusal, previously latched failure,
and empty glyphs. Controls also verify no repeated allocations/logging after
failure and admission after an explicitly reset generation. Shutdown reset is
source-checked; this is not a whole-engine lifecycle test.

The unchanged producers failed16 of24 cases; the guarded producers pass24/24.
GDI complete translation units additionally pass syntax checks against actual
SDL3 headers for both Windows architectures. No full stb target link, game,
GPU, VM or physical platform run is attributed to these CPU checks.

## Separate bridge boundary

The guest SDL shim copies pixels into guest storage and invokes void
`h_tex_update`; its successful return does not acknowledge host GPU admission.
This change handles refusals reported by the SDL API, including its existing
local rejections. It adds no ABI/capability and does not claim rejected host
uploads can now be observed. Host acknowledgement requires its own negotiated
contract and legacy compatibility checks. Neither this gap nor the CPU probes
establish a physical device crash cause.
