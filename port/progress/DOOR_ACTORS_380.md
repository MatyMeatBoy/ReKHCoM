# Door synthesis actors (#380)

The automatic view composed the spotlight/menu at screen scale, cleared the
world pass, and left Sora and the door in that cleared world pass because their
sprites were world-tagged. Fixed view0 showed both; automatic view4 lost both
with normal and HD text. This is separate from the earlier door crash fixes.

Use one effective screen classification during the synthesis spotlight for
native objects, HD replacements, still HD parts and HD glyph coverage. Preserve
the persistent tags, object-window handling, priority and blend paths. Outside
that spotlight/menu combination the existing classification is unchanged.

Verified on Windows EU at1280x720: real room10901/doorpose0 followed by actual
A input opens the kind and value selectors (frames380/470); B cancels back to
the field(frame650). Original auto captures have0 orange actor pixels, while
fixed view0 is a positive control. The focused image regression checks Sora and
the door separately; a coloured card menu alone cannot pass it.

Final production diff was built into native32 and guest64. Four hidden D3D11
runs (32/64 × normal/HD) show both actors and valid imported HD map cards/text.
Both selector captures passed the actor checks and were visually inspected.
The earlier four32 dummy runs also preserve the returned field image byte for
byte atframe650, and fixed view0 remains unchanged. HD inputs are private test
imports; they are not distributed. No reporter-device, Linux or Android GPU
confirmation is claimed by these Windows checks.

Reproduction inputs: KHCOM_MODE=60:room:10901,120:fieldfreeze,200:doorpose:0;
KHCOM_KEYS=240-242:A,420-422:A,500-502:B,560-562:B; shots280,380,470,650;
quit660. Test with port/tools/test_door_actor_visibility.py on each capture
directory. Private evidence is retained outside the checkout in
pruebas/issue380-door, including binary hashes and RED/GREEN images.
