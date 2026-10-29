# World-card additive glow (#517)

Goal: retain the HD world illustration and the original GBA additive light,
including its timing, masks and native foreground. No palette recolour or
gameplay change. The existing EU32 reproduction isolates Traverse Town in
Original view, normal versus HD; the error is not exclusive to widescreen.

Before the candidate fix, the synthetic production-PPU probe produced cyan instead of yellow:
green light blends with the blue card back because the red illustration's
native sprite was removed before composition. This is independent of any ROM
or imported artwork. The full-frame HD over-layer also copies that already
composited native colour above the HD replacement.

The synthetic colours are a counterexample, not a reason to recolour every
cyan pixel. The original USA cartridge was also run in installed BizHawk,
using the existing VBM until world entry and ordinary A inputs afterward.
It reached the selector at frame33521 and the glow at33883, preserving ROM,
movie, installed config and emulator hashes. Its native glow contains a cyan
inner component too; the complete illustration stays visible in front of it.
The candidate retains that authored light and the full HD illustration.

Implementation plan, without workers:

1. Keep the native world-illustration pixels in a separate fallback scanline
   while replacing their visible image. Preserve the native additive result
   in a bounded fallback canvas. Verify red + green yields yellow there.
2. For this screen's full-weight additive BG effect, split the foreground
   light from the underlying card. Render HD first, then add the light, then
   restore higher-priority native foreground. Do not flatten the light into
   an opaque copy of the blue back. Other blend modes keep their current path.
3. Allocate only bounded buffers during the affected effect. If buffers,
   additive capability, upload or drawing fail, present the native world
   illustration and original light for that frame. Never hide the original
   before the replacement is usable. Record the fallback without log spam.
4. Exercise the actual PPU with synthetic colours, windows, priorities,
   inactive effects and allocation failure; exercise the presenter with
   failed allocation/upload/draw. Compare real normal/HD sequences in
   Original/Auto and 32/64 bits, including return/white fade and other worlds.
5. Review the final diff, integrate under the shared lock, and deliver a new
   immutable test identity. Reporter and physical-device confirmation remain
   separate from local checks.

Validation before integration:

- The production-PPU regression and native/bridge presenter fault-injection
  fixtures pass, including allocation/upload/draw failures, bounds, windows,
  inactive effects, canvas ownership and stale geometry after resize.
- All 206 native/semantic PPU oracles report zero mismatches; the expanded
  sprite-storage regression also passes.
- With HD off, 27 full-window frames match the previous test515 byte for byte.
- Windows64 Direct3D11 and Linux64 Xvfb/Mesa OpenGL run the affected selector
  in Original and Auto views, including Traverse Town and Agrabah, and exit
  cleanly. The Linux matrix is a VM check, not physical Steam Deck evidence.
- The native EU32 and common EU guest compile. This is shared presentation
  code, but these checks do not certify every console or Android driver.

Private reproducible evidence is in `pruebas/issue517-worldcard/`, including
`after-stability/PPU-206.json`, `after-stability/SD-PARITY.json` and
`linux64-check/RESULT.json`. No ROM, original movie or imported artwork is
part of this change. The native fallback deliberately remains in use with
LCD colour correction or a host lacking the additive-blend protocol.

Status: candidate validated locally; integration, an immutable test delivery
and reporter confirmation remain separate gates.
