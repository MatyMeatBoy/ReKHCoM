# Colored card value fallback

The actual premium/modified ROM tiles use gBStatesPalette. Modified values have
white ink and blue edging; premium values use yellow/orange ink and dark red
edging. The generic HUD font producer selects one maximum RGB tint, consuming
all bright native pixels. That is not a faithful fallback for these multicolor
glyphs. Imported digit recipes already preserve their intended colors.

HudDigitKeepsPalette prevents only these two source tile sets from entering the
single-tint font fallback. The normal QueueHdPart path remains unchanged: valid
HD art still replaces the complete native value; missing/rejected art retains
the original native glyph, including its outline and gradient. Normal white
font fallback and card positions are unchanged.

`python port/tools/test_card_digit_palette_fallback.py --region eu` reads the
private original ROM (override `--rom`) and parses its manifest sprite records,
tile bytes and palette. It compiles the real ObjColor, registration, HD-cover and
ink-consumption blocks. EU/US/JP each cover 0..9, hd_hud on/off and HD art queued
or missing. The pre-fix producer loses native colored ink; after the fix every
native nontransparent pixel survives in fallback and HD art still suppresses it.
No ROM bytes or generated images are committed.

## GPU comparison for the coordinator

Use two isolated data/asset directories and the same copied runtime, private ROM,
unchanged saved deck and normal input schedule. Keep the source import untouched.
Copy the imported sprites from `D:/01Claude/ReKHCoM-build/hd-completo/sprites` into
both directories; in the partial copy omit only PNG names beginning with
`gCardPremiumValueDigitTiles_` or `gCardModifiedValueDigitTiles_` (including `_play`
variants). All other assets stay identical. Set hd_cards=1, hd_hud=1 and use
KHCOM_HD_HUD_DEV=1, KHCOM_HDSPRITES_LOG=1 with real hidden Direct3D output.

The present run must log the appropriate premium/modified digit art and show the
approved gold/orange or white/blue treatment. The partial run must retain the
native multicolor glyph, not a flattened white/gold font. Check the selected
bottom card and top 1/2/3-card stock, Sora and the opponent. Repeat after returning
to the battle/asset reload. Source draw paths share the same producer, but this
fixture does not constitute runtime actor, affine or physical GPU coverage.

A debug battle entry alone does not guarantee colored cards. Use a known premium
card in the isolated saved deck and a value-modifying battle effect; verify the
actual card traits/capture before marking that case covered. No runtime images
were produced in this lane because the coordinator owns serial GPU QA.
