# Native divisor audit: reachable lifecycle states

Scope: main `038d92627bf2792465dd7eb143b7da6e0c856638`, Windows x64 native
integer division. This is a bounded source/CPU-consumer audit, not a claim that
all divisions or every battle have been exercised. MapInspect/Moogle scroll,
Status and Deck/Exchange boundaries already have separate checks.

## Confirmed: enemy-card name after the effective effect is cleared

`HCEffectName_1` divides by `gHcEffectDefs[effectiveEffect].count << 8` before
checking whether its name task should terminate. Of the 55 EU definitions,
only `HC_EFFECT_NONE` has count zero.

The normal `ApplySoraHcEffect` producer turns Dispel/Mimic into NONE outside
human/versus battles. Parasite Cage's playable enemy card supplies Dispel
(`card_catalog.c`). The card-display lifecycle still creates `HCEffectName`
through `SoraHeartlessCardShow`. `ApplyRikuHcEffect` can likewise clear effects
with Dispel; battle teardown also clears the effective effect while an old
name/counter can remain. The intended outcome is an inactive name task, not a
blink calculation using the absent effect's zero count.

The PC-only guard retires that task and clears its matching inactive card
effect/replacement flag. Active effect arithmetic remains unchanged. No global
division wrapper, exception recovery or GBA behavior change is introduced.

Verification: `test_hc_effect_name_lifecycle.py` extracts the full production
Sora/Riku effect producers and `HCEffectName_1`, plus real IDs and EU counts.
Task/render hooks and minimal surrounding objects are test doubles. The
unmodified source faults with `0xC0000094` for both Sora and Riku producer-to-NONE
cases. The guarded source passes 16 cases: both sides, Dispel/NONE, battle-end
stale positive counters, expiration, replacement, and positive counts 1/2/3
around the Incrementor capacity of 2. Count 3 is a boundary control, not a
claim that this producer naturally exceeds its configured count.

Run without game assets:

```text
python port/tools/test_hc_effect_name_lifecycle.py --work <private-evidence-directory>
```

The CPU regression proves the producer/consumer failure; a packaged gameplay
activation of the Parasite Cage card remains a separate runtime check.

## Other inspected denominators

| Function/path | Precondition and evidence | Result / next check |
| --- | --- | --- |
| Player/other HP gauge updates | PC branches reject `maxHp <= 0` and extra-bar `maxHp <= 280`; main-bar divisor is constant 280. Enemy gauge likewise guards maximum HP. Current HP zero alone does not zero these denominators. | Existing local guards; no new fix. |
| `task_btl_escape_1` | `task_btl_escape_0` assigns `progressMax = 0x5A00`; no other writes in that lifecycle. | Constant positive divisor after task init. |
| `CreateBtlPrizeTasks{Capped}` | All current source callers provide one of 1400, 199, 60, 30, 10, 5, 1. Remaining count zero only affects the numerator. | No reachable zero value found. |
| `DropEnemyPrizes` chance | Switch supplies 0, 2, 3, 4; zero is rejected before division. Room adjustment maps positive cases to 5, 7, 10; `100/chance` stays positive before modulo. | No zero denominator in inspected producer domain. |
| `CopyNumberTiles` | Digits clamped to 6; zero digits skips the division loop. Each executed scale is at least 10, so `scale/10 >= 1`. | Empty/1/6-digit source contract safe. |
| `ShuffleCardSlots` | Both `% count` expressions are inside `i < count`; count zero skips them. | No zero denominator in empty loop. |
| Collision hit centroid | `sumX/Y/Z /= n` only inside `n = hitCount; if (n > 0)`. | Empty hit set rejected locally. |
| Worldselect ring | Slot-count division only inside `i < slotCount`. Sine magnitude is bounded by 256, making `ringY` about -40..40, never 140. | Slot zero skips; scale denominator remains nonzero. |
| Moogle pack flip | FLIP enters with positive 8/60 timer; when decrement reaches zero the same update moves to RISE and resets timer to 5. | No zero FLIP timer found; not the trade-in scroll defect. |

Unresolved audit candidates, not confirmed bugs: enemy-card random selection
(`gEnemyCardCounts`), enemy action interval times live enemy count, spline
interval/knots, background animation frame-byte/scale producers and boss motion
timers. Their valid producer domains and scheduling need dedicated evidence
before adding guards. Corrupt-input possibilities alone are not proof of a
normal gameplay failure.
