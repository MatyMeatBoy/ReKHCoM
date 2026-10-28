# Enemy-card random initializer: zero-index remainder

`UseRandomEnemyCard` forwards its byte-sized index unchanged to
`EnemyUsecardRandomInit`. With a valid enemy table containing more than one
card, index zero enters the `index < count` branch and computes `random % 0`.
The cartridge remainder helper returns zero, selecting the first card and
still consuming the random value. Unhandled x86 arithmetic faults; AArch64
SDIV/MSUB instead retains the random dividend as remainder, which can index
outside the card table. WebAssembly can trap on the unguarded operation.

Use the existing `MOD0` contract at this expression. Positive-index selection,
the second random value used for bobbing, and the rest of the initializer stay
unchanged. The source correction is shared by the native and guest platforms;
the original cartridge build retains its original arithmetic helper.

## Verification

`python port/tools/test_enemy_random_zero.py` executes the full production
initializer against an oracle differing only in this remainder. It uses the
actual 32-bit `CardDisplayArgs`, `CardDisplayWork` and `CardDef` headers and
synthetic valid tables with 1/3/9/10 cards. It compares every output byte,
selected card pointer and random-call count across 8,160 nonzero cases and
32 zero-index cases, including input values 0 and 65,535 from the RNG.

The same compiled probe against the preceding source passes the nonzero
controls, then terminates with integer divide-by-zero on index zero:

```text
python port/tools/test_enemy_random_zero.py --source-ref 14e44feccf3c3d15f0163bd213760d539009c08d --expect-zero-crash
```

The existing EU Thumb interpreter independently passes 192 signed/unsigned
quotient/remainder controls against an owned cartridge, including zero. A
production Android64 translation-unit compile with its actual arm64_32 flags
also succeeds; disassembly retains the RNG call and adds a zero branch before
SDIV/MSUB. This is compile/disassembly evidence, not execution on an Android
device. Private logs and objects stay outside the repository.

This is preventive parity for a concrete API input. No current story-script
call with index zero or reporter crash has been attributed to it. Enemy kind
NONE, invalid tables and unrelated remainder sites are outside this check.
