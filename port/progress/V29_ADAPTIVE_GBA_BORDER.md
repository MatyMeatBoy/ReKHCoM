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
Original240x160 may use its actual smaller picture aperture. At matching
legacy geometry (1920x1080 gives1200x800 at360,140) the exact original texture
uses one full-image draw, with its transparent hole over the authored picture.

The three embedded PNG designs remain unchanged. The existing runtime map
cards are still drawn from the player's ROM. A cached1x216 material column
selects native colours by row majority atx52..67 outside semantic art. Four
material quads fill the surrounding space; four decorative strips retain one
uniform scale and four simple bevel strips span the aperture. Two geometry
submissions emit at most72vertices total across both calls, below the guest
bridge's300vertex limit. No per-frame pixel loop or repeated/stretched card
illustrations. The only new material texture is864RGBA bytes before backend
overhead; creation occurs on selection/reset, never per frame.

If decoding/texture creation fails, no border viewport is reserved and the
normal game remains. Old caches are not destroyed before replacements exist.
Both textures join renderer reset ownership. No original ROM/SAV, HD import,
Atlas data or bitmap assets are changed.

## Validation so far

- Five Python tests execute the actual geometry-header C AST and UI contracts;
  RED recorded before implementation, then GREEN. Requires test-only pycparser.
- Explicitly authorized actual C fixtures passed with native Windows32/64:
  576 Border0 OutputRect cases bit-identical to baseline8f04d2b9;
  1008 BorderON combinations across all six views/seven aspects, tiny/portrait
  and Autoceil cases uniform and contained; all adaptive quads outside content
  and at most two submissions; exact legacy one-draw geometry.
- Actual cache code with mocked SDL passes decode/first and second texture
  allocation failures, bounded retries, native material colours, old-cache
  preservation and complete release. These are not real GPU/device-loss tests.
- Diff whitespace checks pass. Private outputs/hashes/claim receipt are under
  pruebas/v29-adaptive-gba-border outside the repository.

Commands for the C fixture require explicit --cc and --output in
port/tools/test_gba_border_renderer.py. It extracts actual OutputRect/CrtScale,
PortConfigView and baseline8f04's implementation; the renderer/cache inc and
geometry header are included directly, not copied as a stale model.

Full translation-unit compilation, SDL GPU screenshots, HD HUD/journal/menu
layering, CRT physical output and device reset/failure runtime remain pending
the parent's coordinated CPU window. No gameplay or performance claim follows
from mocked SDL fixtures. Do not merge before that review/QA.
