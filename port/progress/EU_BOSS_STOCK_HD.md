# EU boss stock portraits use the existing HD illustration

Reported29Oct: the boss's large active card was HD while its small stocked
cards stayed pixelated. Reproduced in an actual Axel2 battle (debug entry187,
EU32,HDcards1) with three stocked cards2,2,8 at frame900. These are opponent
cards, not Sora's hand, deck menu or Moogle.

## Cause and change

The EU CardDef table uses distinct small-card sources for attack variants:
AxelA/B,HadesA/B,LarxeneA/B/C,RikuA/B/C/D. Their tiles and frame addresses
are different from the collectible portrait registered by the HD table.
For example, AxelA tiles090f2788/frame090f2774; AxelB090f289c/090f2888;
canonical Axel tiles090f2674. Runtime sprite exports confirm both variants.
PortHdSpriteFind could not match those exact source tuples.

Only11 exact EU (tiles,frame) pairs normalize to the existing character
portrait's registered HD identity. No image, TSV recipe, generated registry,
importer output, texture state or gameplay/OAM data is added or changed.
The canonical ID is reused, so the alias needs no duplicate texture/cache.
US/JP lookup behavior stays unchanged. An unknown frame/tiles pair, HDoff or
an unavailable canonical texture still returns0 and preserves native pixels.
PortHdSpriteKeepsCartridge(group6) retains the native crown, coloured border
and value notch; the existing imported portrait covers only its HD window.

Native ROM checks verify all11 actual EU layouts equal their canonical
portrait exactly:16x32 at(-8,-20), including OAM shape, tile index and flips.
The native coloured frames differ, as intended; they are not canonicalized
in the GBA renderer. Only host lookup inputs are normalized.

## Verification

- Actual PortHdSpriteFind C contract: RED11missing aliases before fix;
  GREEN for11EU exact tuples, wrong frames/tiles, null input, HDoff,
  missing textures, canonical IDs reused, plus US/JP unchanged.
- Native OAM contract: all11 distinct EU sources verified against the
  original ROM and symbols in read-only mode. No extracted art in repo.
- One private HDsprites compile+relink: compile0/1.766s,link0/1.219s;
 356existingEU32inputs hashed before/after and unchanged. Mixed provenance
  includes integratorP2/pre-floor/pre-gates512 objects, unrelated to Axel.
 Test exe59ec4e0cf559c9f343360b024d079757df2ab242b2e3a781fc2c6ea120cbaace.
- Actual Axel2EU32,SDLdummy/software,BELOW_NORMAL,1PPUthread,
  KHCOM_BACKGROUND_TEST1. SameROM/config/keys andframes840/900/960:
  newHD on + oldHD off + newHD off, all exit0. OriginalHD-on reference
  is the initial real battle, exit0,17frames, with runtime source exports.
- HD-on changed only the top-right stack portrait region:840bbox
  (1300,80)-(1404,140),900bbox(1252,72)-(1404,140). Frame960 is identical.
  Runtime logs at840/900 identify gAxelSmallCardHD layer1. At900 allthree
  portraits areHD while visible values2,2,8 and red crowns remain.
- HDoff is RGBA-identical old/new3/3 whole frames.

Local evidence: pruebas/atlas-v23-boss-stack, including baseline.json,
native-variants.json, runtime-comparison.json, build-receipt.json, logs and
PNG captures. Current911recipe/importoutputs remain untouched. Tests do
not certifyGPU/64/Android or a clean package, or every AI move of11variants.
Only Axel has a fresh real-battle reproduction here; all11 have CPU/native
layout coverage. This is not a stutter optimization or a quality reduction.

The reverse producer audit also found Trickmaster/ParasiteCage small-card
sources absent from the HD recipe table. They are separate unresolved
asset-coverage cases; no HD portrait is guessed or substituted for them by
this patch. Do not infer all card producers are covered just because all
existing911recipes have current outputs.
