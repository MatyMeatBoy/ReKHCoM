# Web KHMenu spacing — 2026-10-29

The lowercase a/r and six accented a glyphs encoded absolute advance widths as Type2 deltas. With nominalWidthX=714 their CFF advances exceeded hmtx by714 units. Browsers exposed the mismatch; disabling kerning did not fix it.

The generator now encodes advance minus nominalWidthX (or omits defaultWidthX). All glyph outlines, hmtx metrics, cmap, naming and positioning tables remain identical. The regenerated font repairs eight advances. Native and Web packages share this source font.

Validation: test_font_kh_legible.py compared the original approved font with two generator replays; every CFF width agrees with hmtx and outlines/metrics/positioning remain unchanged. test_web_font_assets.py verifies stale build fonts refresh and CSS cache identity tracks their actual bytes. Desktop and390px mobile browser checks passed in English and Spanish, with no horizontal overflow.

Publication must refresh the font and CSS/HTML cache identities. A frontend-only deployment retains the existing game engines and their original provenance; it does not claim a new game build or stable2.3.
