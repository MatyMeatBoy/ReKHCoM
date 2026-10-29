# Tutorial actors and the renderer's object-part budget (#515)

The automatic view lost Sora and the door during the value tutorial after
selecting Moment's Reprieve. This also reproduces with normal graphics. The
original view is a positive control: both actors remain visible there.

`SpriteWork` limits logical entries to128, but an entry can contain several
OAM parts. The failing caption has84 entries and134 visible parts. The old
emitter advanced beyond the cartridge's1024-byte OAM block, while the PPU
consumed only its first128 slots. The wider view admits parts clipped away in
the original viewport. Long captions expose the mismatch; the game has not
deleted either actor. No crash attribution is made from this visual failure.

Keep the native128 slots, affine matrices, SpriteWork layout and cartridge
save data unchanged. Store additional parts in a separate3KiB array with a
512-part rendering budget. Both the emitter and the renderer use that same
bound. Exhaustion stops emission and logs once instead of writing past either
store. Ordinary frames still iterate128 slots; additional slots are visited
only when emitted. This is shared port code, independent of OS and host bits.

The extension includes positions, source tags, HD parts, glyph coverage,
HUD ownership and priority keys. Slot indices and `slot+1` ownership use16bits;
priority keys reserve9bits for the slot. Affine matrix reads and HBlank OAM
snapshots continue to use the original hardware addresses. Spill attributes
and matching replacement tags participate in PPU snapshots/replay. Native
snapshot loaders already require the exact game build; this does not migrate
raw snapshots between builds. BIOS OAM reset and direct movie-subtitle frames
clear the extension. Subtitle lengths are already clamped to at most96 total
parts; no other raw OAM writer was found in the game source.

## Verification

- Actual emitter extracted into a CPU harness: old code overwrites native
  canaries (RED); candidate handles256 parts, preserves matrix words, resets
  spill and stops a600-part fixture at512 without crossing either canary.
- Full production PPU: last spill slot, native/extra priority ties, reset,
  native affine addresses, HD glyph300, HUD owner512 and a pixel/metadata
  snapshot roundtrip pass. All206 existing independent native pixel and
  semantic oracles pass, including their wide-compositor variants.
- Actual Windows EU32 and EU64 runs select Moment's Reprieve with real keys,
  display the tutorial and synthesize the room. Both actors pass independent
  orange-pixel checks in caption frames1100/1200. A frozen earlier64 control
  fails with0 Sora pixels at1100. This frozen control predates other unrelated
  integrations, so it is not a whole-frame equivalence baseline.
- Matched EU32 controls: seven original-view frames remain whole-RGBA exact.
  Normal/HD Auto controls change only the restored actors and the previously
  truncated right end of the header in800/900; the other five frames are exact.
- Windows64 ABI and boot-marker contracts pass. The first ABI run failed its
  temporary-file rename; the unchanged binary passed with a fresh workspace
  TEMP directory. Source checks and the full HUD-canvas failure/recovery
  fixture pass. No physical Linux/Android/console confirmation is claimed.

Private ROMs, imported artwork, captures and detailed logs remain outside Git.
`OAM_PARTS_515_QA.json` records tested binary/source identities and limitations.
The issue remains open for the targeted build and reporter confirmation.
