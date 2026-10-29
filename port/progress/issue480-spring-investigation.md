# Issue480: generated-room spring investigation

The [Android report](https://github.com/MatyMeatBoy/ReKHCoM/issues/480) remains open.
The reporter supplied the save and confirmed **2.2 pre beta**, 284x160, zoom4x,
1.5HD disabled and HD text enabled. The earlier report from the same author
listed Tab S6 Lite, Android13 and USA; the device/OS remain prior context.
The save is USA, 6F Halloween Town, room11, seed `19CEE409`.

## Current evidence

The source path is platform selection (`MapFindPlatformStairs`, `MapMarkJumpSpot`),
spring creation/contact (`Task_MapGmk_Jump_*`), then the Sora/Riku gimmick jump and
ground/collision probes. Sprite ordering is a separate consumer. No change to
these physics, terrain-generation or drawing rules is justified by the report.

## Verified initialization defect and scope

`TaskCreate` obtains work from `EwramAlloc` without clearing it. The spring
initializer omitted `FldObj.height`, while its update passes that field to
`FldObjIsOutOfView`, which can disable the collider. A negative value left in
recycled work can therefore make an on-screen spring unusable. The correction
sets the flat spring's object height to zero for every `PLATFORM_PC` target;
it does not alter its jump height, terrain generation, angle or jump physics,
and leaves the original cartridge build unchanged.

The real initializer/update/culling bodies fail a CPU control with A5-filled
work: height becomes -23131 and the visible collider is disabled. Explicit
initialization passes 00/A5/FF/7F fills, all three orientations and off-screen
culling controls. Other callers of this culling routine in `map_tasks.c` use
object initializers that already assign `def->height` (GP00..GP09, MapGmk00 and
the barrel family); no global allocator or culling workaround was added.

With the exact reporter save, a private native32 executable that poisons only
new spring work before the original initializer reproduces the missing boost:
Sora performs a normal jump and remains on the lower floor. With only the
height initialization changed, both springs reach their upper platforms and
the sampled positions, ground and jump states match the unpoisoned controls.
The private injection object/executable is excluded from shipping builds.

Clean Windows32 and guest64 runs already traversed both springs before the
correction. Thus this proves a real defect under a recycled-work precondition,
**not** that it caused every symptom in the Android recording. The original
save stays immutable; tests use copies. Physical Android confirmation remains
required, and issue480 stays open.

Directed controls reached the upper ground in different generated Halloween Town
rooms, early/later floors and shapes; Agrabah and Monstro controls also landed.
The same seed11/shape1/room3 transition has matching sampled position, ground,
angle, jump phase and velocity in Windows32 and its 64-bit guest path. Those
controls do not reproduce the reporter's room or certify Android behavior.

Two early acquisition attempts timed out during startup/capture and are excluded
from game-failure claims. An early B press also produced a normal jump before
the spring was armed. Waiting for contact makes that control land: it is a
harness-precondition failure, not evidence of the reported bug.

## Reusable directed check

`springpose:N` sets only the starting position at an existing spring; ordinary
B input and the real field/collision tasks execute the transition. `springlog`
records spring placement, contact, height and actor/collider state. Both commands
are explicit PC harness actions; neither runs during normal play.

After building an EU native game with these commands and placing its SDL library:

```sh
python port/tools/test_map_spring.py --exe build/pc/khcom.exe --rom roms/B8CP.gba --out build/spring-check-32
```

For the 64-bit host, pass `--exe build/windows64/out/rekhcom64.exe` and
`--guest build/windows64/out/guest-eu.bin`. The default fixture is MAP-check world
index5, floor3, room3, shape1, seed11 and spring0. Override these when the actual
save identifies the failing layout. Every output directory must be new; existing
saves/settings are never reused. The directory stays private; share sanitized
`RESULT.json`, not ROMs or the complete runtime directory.

The check requires an existing spring, contact armed before B, observed gimmick
jump and a grounded idle actor at the spring's expected upper height. A missing
spring or premature input returns `INVALID_FIXTURE`, even if the process exits0.
Software rendering/audio here do not verify visual order, audible continuity,
the full HD import, an original GBA run or a physical Android device.

CPU regression (requires configured USA generated headers and runnable i686
MinGW output):

```sh
python port/tools/test_map_spring_init.py --cc /path/to/i686-w64-mingw32-clang --out build/spring-init-check
```

Next: deliver a separately identified Android test and replay the reported
history/room there. Do not replace the immutable v2.2.5 artifacts or close the
issue based only on Windows and synthetic allocation controls.
