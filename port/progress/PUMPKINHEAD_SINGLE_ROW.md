# One remaining row under Pumpkinhead's crown

The user's b91e56c7 capture shows Pumpkinhead, initially mistaken for Olympia.
The native black horizontal border is correct and remains unchanged. The thin
coloured line immediately beneath it comes from the native illustration's first
interior row, visible through the HD image's filtered transparent top edge.

Only accepted HD Pumpkinhead replacements neutralize native authored row16,
x2..13, through the existing shared `SmallCardNativeColor` helper. The native
closing corners x1/x14, other native rows, every HD texel, all other card types
and HD-off/missing/failed replacement paths keep their existing behavior. This
does not add a black stripe over the HD artwork or change the illustration.

Validation:

* Updated production-branch fixture fails before the change, passes afterwards:
  368640 pixel/color cases, 18 card types, world and HUD routes, both source
  origins, accepted/rejected HD, palette colors and transparency. Catalog checks
  verify that ordinary values0..9 share the same Pumpkinhead image.
* Olympia's 40960 branch checks, 105 small-card corner/recipe checks and both
  first-frame replacement-fallback budgets pass.
* Native32EU full build passes. Private matched D3D11 binaries differ only in
  this condition enabled/disabled. In the same synthetic deck, frames1800,
  1900 and1960 each change **47 pixels, all on the single row y310, x338..392**.
  Forty-five interior pixels become black; the two edge samples preserve their
  existing blend with the original closing corners. Zero changes occur outside
  that row, including the value5 glyph and the eleven other visible cards.
  All449 staged PNG files are byte-identical between runs.

Private evidence, outside Git: `pruebas/card-final-horizontal-line/` contains
`MATCHED-RESULTS.json`, `BEFORE-real-pumpkin5.png`, `AFTER-real-pumpkin5.png`
and `validate_matched.py`. No source art, original save, protected installation
or published build was modified. The real-game comparison covers native32EU
at the current 1280x720 output; other targets use the same shared PPU helper.

Other small cards have native first-interior illustration colors too. Their
sampling could expose a similar edge at another output transform, but this
patch deliberately changes only the card with the reproduced report.
