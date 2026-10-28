# Full-white HD sprite fade endpoint

World-card selection keeps drawing the selected world during its white fade and
the 60-frame wait before entering the event. The native palette reaches white,
but HD rendering drew the coloured image followed by its white copy with the
same translucent edge alpha. On white background, a base edge of alpha A retains
`A * (1-A) * (colour-white)`: a grey/coloured outline despite a completed fade.
This also affects translucent details, not just pixels along the outer contour.

At the full-white endpoint only, omit the coloured base and draw its existing
white copy. Its alpha, UVs, clipping, queued ownership and fade amount remain
unchanged. There are no texture/asset edits, additional allocations, new bridge
commands or gameplay timing changes. The same endpoint rule applies to queued
world/screen/HUD sprites, owned foreground HUD pieces, title art and the banner
margin extension that use this two-pass composition. Menu-light procedural
effects and other independent renderers are outside this patch.

For intermediate fades the previous two-pass approximation is intentionally
unchanged. This fixes the completed transition without introducing a different
appearance throughout normal rendering or changing the existing fade curve.

## Verification

`python port/tools/test_hd_white_endpoint.py` executes the production draw paths
with real SDL types and a small source-over pixel consumer. It covers palette
and global white, backgrounds 0/37/128/255, texture alpha
0/1/63/127/191/254/255, opaque/translucent queued opacity and four source colours.
The actual palette detector maps a fully white palette to 255. All 7,840 checks
pass: full white has no contribution from the base colour; palette amounts
0/64/128/254 and global amounts 0/4/8/15 retain the preceding result. The owned
HUD and banner-edge paths are also exercised. The preceding source fails the
endpoint assertion; this is a CPU compositing regression, not a device test.

Production queue acceptance/native fallback regressions also pass. A native EU
32-bit incremental build passes with one job and below-normal process priority.
Its memory supervisor records at least 3.18 GiB physically free during the build.

Private D3D11 reproduction of the actual Traverse Town selection transition
shows the reported outline at frame 480 with HD enabled and a uniform white
frame with HD disabled, followed by the next event at frame 517. A corrected
D3D11 run reaches the identical native uniform-white frame: 15,893 pixels change
only within the world silhouette's bounds. The other 17 sampled frames,
including tutorial, unfaded selection and the following event, are byte-identical
to the previous HD build. The successful run keeps at least 3.218 GiB physical
RAM free and exits normally.

An earlier corrected capture was cancelled by the memory guard below 3 GiB;
that incomplete attempt is not evidence of the correction. Additional
world/cancel/return controls are pending the same memory margin. No Steam Deck
device test is claimed.
