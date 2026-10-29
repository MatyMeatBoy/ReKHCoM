# World-card additive glow (#517)

Goal: retain the HD world illustration and the original GBA additive light,
including its timing, masks and native foreground. No palette recolour or
gameplay change. The existing EU32 reproduction isolates Traverse Town in
Original view, normal versus HD; the error is not exclusive to widescreen.

The focused production-PPU probe currently produces cyan instead of yellow:
green light blends with the blue card back because the red illustration's
native sprite was removed before composition. This is independent of any ROM
or imported artwork. The full-frame HD over-layer also copies that already
composited native colour above the HD replacement.

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

Status: diagnostic and implementation in progress; no fix claimed yet.
