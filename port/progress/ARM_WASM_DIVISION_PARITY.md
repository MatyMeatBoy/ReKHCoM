# Cartridge DIV/REM parity: bounded CPU audit and Web lowering

The original EU Thumb helper interpreter (`port/tools/test_cartridge_division.py`)
passed 192 controls from a private cartridge: signed/unsigned quotient and
remainder, zero divisors, negative operands and signed MIN/-1. Its contract is
zero quotient/remainder for divisor 0, signed MIN/-1 quotient MIN/remainder 0,
and truncation toward zero with a remainder carrying the dividend's sign.
No cartridge bytes or player data belong in this report or its tests.

## Observed target-code differences

The same four variable-operand C functions were compiled with O2, fwrapv and
the relevant arithmetic flags/targets; recorded commands identify the exact
compilers. Wasm ran in Node; ARM instructions ran in diskless QEMU, without a
game, operating-system image, renderer or device. All 16 nonzero controls per
target matched, except the separately listed Wasm signed overflow.

| CPU probe | Raw differences in 28 cases | Guarded prototype |
| --- | --- | --- |
| Wasm32 |13: 12 zero-divisor traps and signed MIN/-1 division trap |28/28 match |
| AArch64 arm64_32/cortex-a57 |4: signed/unsigned nonzero dividend remainder by 0 returns dividend |28/28 match |
| ARMv7 Android NDK21 compiler-rt + existing idiv0 returning 0 |6: zero-divisor remainders read an unwritten stack slot |28/28 match |

For ARMv7, the compiled `__aeabi_*divmod` wrappers obtain remainder from a
stack output slot. Its runtime's zero branch calls `__aeabi_idiv0` without
filling that slot; overriding only the quotient's idiv0 cannot establish the
remainder contract. This is an Android compiler-rt observation, not verification
of VitaSDK/libgcc or every ARM32 runtime.

The bare-metal output driver needed FP disabled and strict/aligned integer
loads because it does not initialize a normal OS MMU/FP environment. These are
driver corrections, not changes to the arithmetic probes. The AArch64 probe
retained emitted arm64_32 instructions; only Mach-O section/comment directives
were translated for the ELF QEMU test container.

## Implemented scope

Only Web **game translation units** now emit frontend LLVM IR before optimization,
lower scalar i32 DIV/REM into internal always-inline cartridge-contract helpers,
then run O2 as before. Hosts, port code, SDL and importers keep their previous
compilation path. The transformer participates in the cache fingerprint.
The frontend keeps its optimized-mode attributes via `-disable-llvm-passes`,
avoiding an O0/optnone pipeline that would suppress later optimization.

Guards protect the division itself and signed overflow before executing the
native operator. Post-optimization guards can arrive after C undefined behavior
has already altered a caller, so they are insufficient as a general strategy.
This does not catch arbitrary faults, change RNG calls/seeds, replace original
negative-remainder semantics, or hide malformed resource data.

The portable lowering prototype passed the 84 combined CPU cases. The committed
Wasm test covers the same 192 arithmetic combinations as the cartridge oracle,
plus two caller-branch controls. Actual Web compilation of acgtrans and bg_anim
accepted all six transformed sites. Unit checks cover metadata, exact flags,
game-only scope, unchanged float/i64/vector operations and visible rejection of
unsupported scalar forms.

The real fixed-address/callback/Asyncify Wasm ABI probe also passes through the
new game IR pipeline, including zero quotient and remainder checks. A complete
EU build is being validated separately; CPU probes do not certify browser
startup or a player-flow regression.

## Limits and next targets

ARM builders remain unchanged by this Web child; the prototype establishes a
candidate mechanism, not delivery on Android/Switch/Vita/Anbernic. Porting the
pipeline requires each real producer, runtime/cache boundary and ABI to be
checked. Literal C expressions already folded to poison by the frontend and
non-i32/vector operations are outside this lowering; do not replace arbitrary
poison with zero. A raw operator counterexample proves a compatibility gap,
not reachability from a player save, a cause for an unrelated crash report,
or universal crash freedom. Existing published v2.2 bytes remain immutable.
