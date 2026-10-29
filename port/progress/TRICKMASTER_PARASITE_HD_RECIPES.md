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

The actual C++ importer was rebuilt with one private decompui object and
read-only shared inputs. Launcher SHA256:
`9aa394701e4ce0b93b458d29e85b218bdda6dd300727cd7d03e179d81e716477`.
In a normal copy of the canonical cache, `--hd-stale` reported four before
`--hd-import <owned Steam installation> --sprites-only`, then zero. Import
exited zero; all previous 911 outputs remained byte-identical. All 912 source
cache files, including recipes.txt, and the installation identity stayed
unchanged. New C++ outputs preserve Python alpha exactly; RGB differs by at
most one level per channel. C++ PNG bytes are 89971/86257 for the two large
portraits and 26331/26838 for the two small portraits, 229397 total encoded
and 786432 potential decoded RGBA bytes. No RAM/VRAM residency is inferred.

Actual producer validation used one private HDsprites object and a mixed
EU32 relink with read-only root objects (journal P2, pre-floor/pre-512 PPU).
Game SHA256:
`2d2975648d4a865d3c22937960984f40e6566667708ec114c03f8f04554b6112`.
All 16 serial SDLdummy software runs exited zero, producing 42 real captures.
Each used BACKGROUND_TEST=1, one PPU thread and BelowNormal scheduling.
Monster battles used the existing debug battle entries 200/197 and their
real EnemyCardDraw; collectible small-card cases used deckuse CardDefs
543/542 and the real Mode_Deck. These are explicit test fixtures, not a claim
of a natural playthrough. Runtime logs name all four new HD bindings.

| Consumer | Frame | Old/new HD-on difference bounds | Old/new HD-off |
| --- | --- | --- | --- |
| Trickmaster large | 480 | 721,680 to 816,790 | whole-frame RGBA identical |
| Parasite Cage large | 780 | 721,676 to 816,786 | whole-frame RGBA identical |
| Trickmaster small | 300 | 152,232 to 228,314 | whole-frame RGBA identical |
| Parasite Cage small | 300 | 152,232 to 228,314 | whole-frame RGBA identical |

Only the illustrated portrait regions changed. Native crowns, frames and
value producers were not modified. Evidence is in cpp915/import-verification.json,
runtime-comparison.json, launcher-build-receipt.json and game-build-receipt.json.
The launcher receipt transparently notes a local receipt-basename collision
and reconstruction from the successful verified run; no extra build was run.
All reused inputs are individually hashed. Private images are not committed.

Atlas verification children (pending, no inherited human approval):
`eb5bb35f7667483bb8b508090a2353e0`, `ca9f79411f154e89aee28b538d3fcb0e`,
`819be5082470475390fd3ae596654bd4`, `62d8a64505594a1d8747f15f333dad5a`.
The post-verification API freeze at 2026-10-29T17:50:11Z has 209 assets,
82 families and 78 accepted versions; snapshot SHA256
`137af7a20df734bbead19fbb8e4b1c595112df20a6532ff3847e005d876a1851`.

This certifies the four EU32 software producer cases and isolated C++ import.
Clean packaging, GPU64, Android, all AI moves and comparable loading/stutter
measurements remain outside this verification. The existing canonical cache
still needs the normal user import/update to acquire the four added outputs.
