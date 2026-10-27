# Overlapping HD edge strips: commit coverage after acceptance

The synthesis requirement body has two native OAM pieces sharing one row.
Drawing its translucent HD picture in both places produces a pale horizontal
band. The edge-strip trim in PR404 removes that repeated strip.

Integrator review reproduced a fallback defect in the initial proposal: a
16x16 piece starting at source y=-8 was recorded before the sprite queue
rejected it as outside the image. A subsequent valid 16x16 piece at y=0
lost its top8 rows despite the first HD part never having drawn.

Coverage now records only successful `PortHdSpriteQueue` calls, after source
bounds, transform, decode/upload and queue-capacity validation. The key also
matches opacity, palette fade, OAM priority and clip range. Different drawing
states are not silently merged. Flipped and affine pieces keep their existing
path. The64-entry record bound safely declines additional bookkeeping.

This is edge-strip trimming, not a general union-of-rectangles compositor.
Full containment and interior intersections retain their previous draw and
per-OAM ownership behavior. Omitting those calls would require separately
preserving ownership of counters and foreground pixels; it is not necessary
to fix the reported repeated boundary row.

`test_hd_piece_acceptance.py` executes the production PPU queue, sprite queue
and loader. The original proposal fails; the corrected source passes source
bounds rejection, decoder/upload failure and recovery, full queue, row/column
UV and geometry, zero/different alpha, dark/white fade, layer, priority, clip,
flipped/affine paths, retained containment and saturated bookkeeping controls.
The existing first-frame fallback and card seam/corner regressions also pass.

Current full-game validation is pending. The separately reported world-card
opening must be checked through its actual consumer before claiming that
transition is fixed. CPU coverage alone does not establish the visual result.
