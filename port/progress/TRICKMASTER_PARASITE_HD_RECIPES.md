# Trickmaster and Parasite Cage card illustration coverage

The CardDef producer audit found four illustration bindings missing from the
911-row HD registry: both collectible large portraits and both small portraits
for Trickmaster and Parasite Cage. The monster battle producer is
`EnemyCardDraw` (`src/card/card_enemy.c`), which draws `cardDef->gfx` as the
large enemy card. These monsters do not use the humanoid Riku three-card stack.
The small bindings also exist in the collectible CardDefs; their native frames,
backs, numbers and game logic remain separate from the HD illustration.

Four recipes now use the exact owned remaster sources:

| Figure | Source | DDS SHA256 |
| --- | --- | --- |
| Trickmaster | CARD/0137/CA0137.RTM:ce_037_tm2.dds | 717fff72f3cb014c3640d6bd93ac3a8a16e6beba40cc9f1e507dc5ba25ea2be6 |
| Parasite Cage | CARD/0138/CA0138.RTM:ce_038_tm2.dds | 5e42e781ffdd25a0d48e29cb4ae3cdc8287176a4a253f8d4ad9869a800e09d10 |

Both DDS images are 256x256, 262272 bytes. Native and remaster portraits were
visually compared. The recipes reuse the established Oogie large/small crop,
placement, corner erasure and integer scale; only the source and native
bindings differ. Large output is 256x256; small output is 128x256. There is no
invented artwork, smoothing, resolution reduction or change to native values.
The original 911 TSV rows are byte-identical, followed by four new rows.

CPU validation on 2026-10-29:

- Generated C/C++ recipe tables are current (`gen_hd_sprites_inc.py --check`).
- `check_hd_sprites.py`: 915 rows, 0 problems against the regional ROM symbols.
  This validates registered bindings, not importer output or runtime use.
- `test_card_illustration_coverage.py` preprocesses the real CardDefs separately
  for EU, US and JP. Coverage is EU 99 large / 118 small and US/JP 99 / 107,
  including the 11 exact EU aliases only when their canonical row exists.
- Removing either a real large/small recipe or the Axel A EU alias makes the
  reverse coverage check fail. Before these four recipes, four producers were
  uncovered. This prevents a complete cache check hiding absent producers.
- Python delta generation produced four PNGs: large Trickmaster 60637 bytes,
  large Parasite Cage 58571, small Trickmaster 20038, small Parasite Cage 20288.
  Total encoded size is 159534 bytes; potential decoded RGBA is 786432 bytes.
  These are output sizes, not measured VRAM use, loading latency or stutter.

Local private evidence lives in `pruebas/atlas-v23-boss-stack`: source hashes,
native/reference images, four-recipe-delta.json, python-four-delta.json and
check-hd-bindings-new915.log. Art/ROM/package data are not committed. The
canonical 911-output cache remains read-only and unchanged.

Pending before delivery as a working clean build: regenerate the C++ importer,
prove four stale outputs become zero in a separate 915-output cache, prove the
previous 911 outputs are byte-identical, then activate and capture the actual
monster-card consumers with HD off as control. No runtime or performance
approval for these four new bindings is claimed by this source-only change.
