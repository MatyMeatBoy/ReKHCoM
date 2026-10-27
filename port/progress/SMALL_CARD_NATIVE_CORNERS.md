# Preserve the small-card frame's two closing cells

The HD importer places the illustration in native cells `(1,16)..(14,30)`.
Its first row covers two original frame-closing cells at `(1,16)` and
`(14,16)`. All 105 current small-card recipes share that box and placement.
The native colours differ: attack cards, green item cards and blue summon
cards must not receive a shared black replacement.

The existing foreground layers now preserve only those two visible native
cells over an accepted HD part. The world/screen path copies the finished
native pixel only when the OAM occupancy identifies that same card; the HUD
path retains its native colour and existing alpha/effect processing. Source
coordinates follow flip/affine transforms, including partial OAM pieces.
The back-to-front HUD compositor clears an earlier foreground contribution
when a later visible HD pixel takes ownership, then restores that later
sprite's own corner if applicable. A lower card's corner cannot remain above
the illustration of an overlapping front card.
There are no texture, illustration, palette, importer or value-digit edits.
HD-off, missing, rejected and not-ready parts retain their existing fallback.

An alpha-mask candidate was rejected: linear sampling affected adjacent HD
pixels and did not reliably retain the native cells at fractional scales.
The foreground approach uses the existing native layer sampling instead.

Validation:

- `test_small_card_native_corners.py`: production foreground branches fail
  before and pass after this change. Covers the current 105 recipe layouts,
  varied colours, world/screen/HUD, accepted/off HD, flips, explicit affine
  identity/2x/180-degree cases, partial pieces and OAM ownership. A separate
  two-OAM HUD sequence failed before the ownership-clear correction and now
  passes with a front HD illustration, front native corner, native fallback
  and another HD sprite path. An optional
  private `KHCOM_CARD_CORNER_INVENTORY` corpus also passes with the original
  corner colours decoded from 99 native card sprites.
- `test_stock_card_frames.py`: existing frame/fallback/foreground behavior
  passes with the new extracted helper dependency.
- Complete PPU syntax checks pass for native i686 and x32 guest EU/US/JP.
- A private SDL software renderer fixture draws the native and HD layers
  separately using production `Quad` and `SmallCardNativeCorner`. It covers
  93 available real imported illustrations at ten scales and three offsets.
  Before, 93,093 output pixels belonging to the two native corner footprints
  differ from the native reference; after, none do. No output pixels outside
  those footprints change. The footprints come from the native SDL sampling
  itself, rather than rounding an analytical coordinate estimate.

The other six sprites in the 99-native corpus lack HD files in that private
fixture; they are fallback controls, not six additional HD render passes.
No full-game/hardware validation is claimed by the software fixture. The
integrator still needs gameplay capture of this exact foreground change.

This change does not recolour the separate horizontal row-15 seam or alter
the illustration. Kingdom Key's white point and Olympia's lower right native
residue are separate corrections.
