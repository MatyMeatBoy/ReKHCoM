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
name/counter can remain. The original GBA/32-bit zero quotient is zero, after which the existing checks
choose whether to keep or finish the task. An early return that also clears
card state is not equivalent: a new effect can be pending in that state.

The fix uses the existing PC `DIV0` helper at these two calculation sites only.
It returns the GBA-compatible zero quotient, then leaves every lifecycle check,
state mutation, blink clamp and task return unchanged. Non-PC source paths are
unchanged. No global division wrapper or exception recovery is introduced.

Verification: `test_hc_effect_name_lifecycle.py` extracts production effect
producers, the name consumer and shuffling predicate, real IDs and EU counts.
Task/render hooks and minimal surrounding objects are test doubles. The
unmodified native source faults with `0xC0000094` independently for Sora/Riku
producer-to-NONE cases. The corrected source passes 16 named lifecycle checks.

The additional `--compare-gba` mode takes the original consumer from immutable
commit `038d92627bf2792465dd7eb143b7da6e0c856638`, substitutes the independently
verified GBA zero quotient only at its two divide sites, and compares complete
work/state/battle objects, return values, visibility, blink interval, timer and
task-update/digit hooks with the fixed consumer. All 7,724 cases agree, including
NONE/old/new pending state, replacement, Dispel/Mimic/random-shuffle IDs and
counter 0/1/capacity-1/capacity/capacity+1. All 54 non-NONE effect definitions
also receive positive/expiration controls. Mixed states and above-capacity
counts are boundary controls, not claims that every combination occurs during
normal gameplay. The original early-return proposal failed this differential
check and was replaced before integration.

```text
python port/tools/test_hc_effect_name_lifecycle.py --work <private-evidence-directory>
python port/tools/test_hc_effect_name_lifecycle.py --compare-gba --work <private-evidence-directory>
```

EU/US/JP non-PC preprocessed consumers are byte-identical to the original.
A packaged gameplay activation of the Parasite Cage card remains a separate
runtime check; the CPU fixture proves the producer/consumer failure directly.

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
