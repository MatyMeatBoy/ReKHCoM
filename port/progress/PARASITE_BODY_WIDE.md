# Parasite Cage body at the camera edge

The boss combines OBJ limbs with a torso drawn through BG1. `BosPcDraw`
loads that torso with `LoadBgMap`, rather than registering a map-block world.
The wide PPU therefore made it transparent outside the original 240x160
picture even though the limbs still appeared there. Moving Sora left exposes
the cut at the original right edge.

Register the current authored torso map through the existing `PortSetBgWorld`
path, with its unwrapped screen-to-map origin. Return its transparent tile
outside the finite 32x32 map, so the extension never repeats the torso. Clear
the registration for inactive animation and task destruction. Existing live
VRAM validation and screen/UI exclusions remain in control. Cartridge drawing,
actor/collision/AI state and the native viewport are unchanged.

Verification from base `6895c747569fe4f794126d3fb741b983e9f45e94`:

- Actual draw-function CPU test failed before the change because it omitted
  body-world registration. It now passes 943 camera positions, finite map
  bounds, animation changes and inactive/destructor cleanup.
- EU native32 software A/B, debug battle entry 197, left movement: frame 1500
  restores 2606 exterior pixels at `[280,21,318,147)` on a 320x180 canvas.
  All 38400 pixels of the original GBA region are identical. Frame 1000 restores
  2701 exterior pixels; the centered frame 500 remains entirely identical.
- Original 240x160 is entirely identical. Auto x3 (427x240) restores the same
  missing body at the native edge; Auto x4 right movement remains identical
  where no clipping occurs.
- Left movement followed by the existing real level-up UI fixture: frames
  1440/1500/1590/1900 are entirely identical to the baseline. This checks UI
  exclusion while the boss task is retained, not a natural EXP victory.
- All 43 torso sets in each EU/US/JP cartridge copy have map entry 0 equal to 0
  and a transparent tile 0. No proprietary bytes are included here.

Local evidence is under `pruebas/parasite-left-cull/`: CPU RED/GREEN logs,
incremental-build provenance, A/B images/pixel comparisons, camera/zoom matrix,
level-up controls and regional blank-tile checks. The relink reads 357 existing
objects without modifying them and replaces only `bos_pc.o` in a private
executable. Native64 and physical console/device QA remain separate checks;
the shared presentation change applies wherever `PLATFORM_PC` is enabled.
Published v2.2 packages remain immutable.

Run the ROM-free CPU regression with:

```
python port/tools/test_parasite_body_world.py
```
