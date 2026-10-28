# Olympia: one native cell at the HD illustration edge

At fractional output scales a native white cell can remain visible beside
Olympia's filtered HD picture. The native cell is `(14,19)` in its 16×32
bitmap, or `(6,-1)` in the authored frame whose origin is `(-8,-20)`.
The HD illustration itself also has white detail in that area; removing that
detail changes the artwork and does not address the native residue.

The PPU now substitutes black for this one native underlay cell only after
the corresponding HD part has successfully queued. Both world/screen and
HUD native-pixel routes use the same rule. Source coordinates already follow
the OAM flip/affine transform, so the cell follows the card. Missing, rejected,
disabled or not-ready HD keeps every native pixel. Palette and BLDY effects
continue through their existing later stages; transparency is unchanged.
No texture, importer recipe, card value digit or other card changes.

Verification:

- `python port/tools/test_olympia_underlay_seam.py`: the production native
  colour branches fail before the correction and pass afterward: 40,960
  pixel cases covering world/HUD, accepted/fallback, source coordinates,
  other tiles, palette colours and transparency. The catalog confirms values
  0–9 share the same illustration, separately from the value sprites.
- `python port/tools/test_stock_card_frames.py`: existing frame/foreground
  fallback contract passes.
- The complete PPU translation unit passes native i686 and x32 guest syntax
  checks for EU, US and JP. The shared PPU applies the rule across ports;
  these syntax checks do not certify execution on every device.
- A private SDL software rendering fixture uses the production `Quad`,
  native-nearest and HD-linear textures as separate layers at ten scales and
  three offsets. The residue appears in four of thirty transforms. At scale
  4.5 and integer offset the correction changes exactly four output pixels
  from white to black, preserving the HD highlight; the production colour
  helper reproduces the same result. No composited-image downsampling was
  used as evidence. Full game/GPU capture remains for the integrator.

The older full-frame `test_ppu_conformance.py` fixture currently fails to link
numerous existing port functions such as `PortHdSpriteFind` and
`PortEventBackdropProfile`; that unrelated fixture was not repaired or counted
as a pass in this change.

The separately reported horizontal crown/illustration seam across attack
cards is a different scope. Its native palette colours must be checked per
card; Simba's blue edge is not a justification for a universal black line.
