# Adaptive GBA border, all views

The existing Border0..3 option is visible independently of view, aspect, zoom
and HD/native settings. Preset values and persisted IDs remain unchanged.
The existing Aspect option includes GBA3:2; selecting a border never selects
an aspect, zoom or graphics preset.

A usable cached border reserves a centered viewport with20/27 of each aspect
area dimension (160/216 of the original frame). The same fraction keeps that
selected aspect. Auto derives its canvas from this useful area and the chosen
zoom. Uniform destination fitting handles ceil rounding/tiny outputs without
cropping. With a border explicitly enabled, Stretch and CRT fit the source
canvas uniformly; with Border0 their previous behavior is unchanged.

All picture/HUD/text/journal layers share the useful viewport. Geometry lies
outside it, and touch controls/toasts keep their existing output placement.
Fixed views fit the shared viewport to the current configured canvas, before resolution state changes. The final frame follows the actual picture destination; menu, journal, HUD and screen-overlay layers retain their complete UI viewport, including the journal footer. At matching
legacy geometry (1920x1080 gives1200x800 at360,140) the exact original texture
uses one full-image draw, with its transparent hole over the authored picture.

The three embedded PNG designs remain unchanged. The existing runtime map
cards are still drawn from the player's ROM. A cached1x216 material column
selects native colours by row majority atx52..67 outside semantic art. Four
material quads fill the surrounding space; four decorative strips retain one
uniform scale and four simple bevel strips span the aperture. Two geometry
submissions emit at most72vertices total across both calls, below the guest
bridge's300vertex limit. No per-frame pixel loop or repeated/stretched card
illustrations. The cached material column uses864RGBA bytes; a384x216 alpha cache copies only native ornament pixels and bevels (331776RGBA bytes). Full-height side strips preserve cards crossing old strip joins; only bevel/material spans stretch. The original full texture is retained for exact legacy drawing. Cache creation occurs on selection/reset, never per frame.

If decoding/texture creation fails, no border viewport is reserved and the
normal game remains. Old caches are not destroyed before replacements exist.
All three textures join renderer reset ownership. No original ROM/SAV, HD import,
Atlas data or bitmap assets are changed.

## Validation so far

- Seven Python tests execute the actual geometry-header/cache-prefix C AST and UI contracts;
  RED recorded before implementation, then GREEN. Requires test-only pycparser.
- Explicitly authorized actual C fixtures passed with native Windows32/64:
  576 Border0 OutputRect cases bit-identical to baseline8f04d2b9;
  1008 BorderON combinations across all six views/seven aspects, tiny/portrait
  and Autoceil cases uniform and contained; all adaptive quads outside content
  and at most two submissions; exact legacy one-draw geometry.
- Actual cache code with mocked SDL passes decode/all three texture
  allocation failures, bounded retries, native material colours, old-cache
  preservation and complete release. These are not real GPU/device-loss tests.
- Review found failed2 -> cached1 ->2 was suppressed until Off/reset. The cache
  hit now records its selected choice; the actual C-prefix Python test failed
  before and passes after. A matching C transition fixture is prepared; its
  rerun passed on native32/64 in the next authorized window.
- Updated isolated Windows64 EU full build --test passed on main d2254cc0 plus this patch: ABI, mixed arguments, mapping, synthetic boot and BIOS checks. Guest SHA256 8b98666926260be182980579d154bfee6f29e0dad26d1ca296e407b93d99b137.
- Final C32/64 fixtures also cover288 view transitions with deliberately stale previous-frame resolution/scales. Fixed-view picture/viewport gaps stay below one enclosing pixel; no inner letterbox remains in the geometry.
- Five real hidden D3D11 EU runs exited0,511/511 accepted presents each: Wide/GBA border1, Extended/4:3 border3, Auto/GBA border2, legacy Original1920x1080 and Auto with existing HD resources. Unedited screenshots show the frame adjoining the picture and complete ornaments/map cards. The HD field case has no battle HUD and does not certify combat HUD layering.
- Private evidence and unedited captures are in pruebas/v29-adaptive-gba-border. Capture readback and heavy unrelated system load make these visual tests unsuitable as performance benchmarks.
- PNG assets, ROM and source save hashes remain unchanged; each runtime used its own save copy. Diff whitespace checks pass.

Commands for the C fixture require explicit --cc and --output in
port/tools/test_gba_border_renderer.py. It extracts actual OutputRect/CrtScale,
PortConfigView and baseline8f04's implementation; the renderer/cache inc and
geometry header are included directly, not copied as a stale model.

Other target compilation, real journal/menu/combat-HUD layering, CRT physical output and device reset/failure runtime remain pending parent QA. No physical monitor/VRR or performance claim follows from these visual checks. Do not merge before parent review.
