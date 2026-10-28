# Overlapping HD edge strips: commit coverage after acceptance

The synthesis requirement body has two native OAM pieces sharing one row.
Drawing its translucent HD picture twice adds an extra blend along that edge.
The edge-strip trim in PR404 removes that repeated blend. It does not remove
the room background intentionally visible through the translucent body.

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

Private native32 EU D3D11 before/after runs at the corrected combined source
reach both synthesis selectors and return normally. At the inspected frame,
the body pieces meet at the same output boundary rather than sharing four
output rows; geometry and UV logs identify the actual consumers. The isolated
edge comparison distinguishes this small blend change from the background band.

Layer isolation also rejects a broader claim: the remaining visible horizontal
edge is present with both HD pieces transparent. Its transmitted background
matches one 35%-alpha body composition in 2511 channel comparisons. It is the
room spotlight/floor edge, not another glyph pass. Making that interior opaque
would be a separate visual change and must preserve the real digit, which is
currently drawn before the body. The private guarded proposal is not part of
this correction or of the published pre3 packages.

After combining current main, the acceptance, first-frame fallback, attack
seam, Olympia and native-corner regressions pass again. Full containment,
interior intersections, flipped/affine pieces and different blend/owner states
remain unchanged. The separately reported world-card opening still requires
its own actual-consumer comparison; synthesis evidence does not certify it.
Other platform/device execution and final-package verification remain separate.
