# Issue480: generated-room spring investigation

The [Android report](https://github.com/MatyMeatBoy/ReKHCoM/issues/480) remains open.
The recording shows a spring launch, but its exact room seed/layout and APK
version are not available yet. The earlier report from the same author listed
Tab S6 Lite, Android13 and USA; that is prior context, not confirmation of the
current configuration. Version, view/HD mode and a pre-room save were requested.

## Current evidence

The source path is platform selection (`MapFindPlatformStairs`, `MapMarkJumpSpot`),
spring creation/contact (`Task_MapGmk_Jump_*`), then the Sora/Riku gimmick jump and
ground/collision probes. Sprite ordering is a separate consumer. No change to
these physics, terrain-generation or drawing rules is justified by the report yet.

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

Next: replay the exact reporter save/layout on the reported APK and compare its
contact/height/landing state with the original before proposing a fix.
