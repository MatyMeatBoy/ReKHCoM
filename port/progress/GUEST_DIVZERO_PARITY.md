# Cartridge division compatibility in x32 hosts

This change restores one verified cartridge contract: signed and unsigned
integer quotient/remainder are zero when the divisor is zero. It does not
declare every GBA behavior or every 64-bit instruction covered.

## Original cartridge evidence, not a comment assumption

`test_cartridge_division.py --rom <private EU dump>` executes the original Thumb
bytes at the EU funcmap offsets: `__divsi3`11c10c, `__div0`11c1a0,
`__modsi3`11c1a4, `__udivsi3`11c274 and `__umodsi3`11c2ec. A bounded interpreter
implements only the Thumb subset these routines use, rejects unknown opcodes
and checks192 signed/unsigned quotient/remainder cases. Nonzero controls agree
with truncation toward zero, including signs and32-bit wrapping; all four
routines returnR0=0 for zero divisors. The actual zero paths call `__div0`
(return to LR), explicitly put0 inR0, and return. No cartridge bytes are shipped.
Private EU ROM SHA256:
`a28761aff292e7f7a5d6dcff1c1ba11eefdbe15836ec0deb8be275fec0519bd4`.

## Which32-bit protections were absent

| Runtime / owner | Division | Stray memory / bus | Limit |
| --- | --- | --- | --- |
| Windowsi386 `port/src/port_biosfault.c` | `BiosFaultHandler` calls `SkipDivision` for EXE DIV0 exceptions; decodes8/16/32-bit DIV/IDIV and zeros quotient/remainder. | `RedirectAccess`+`BusTarget` move an address register and single-step, then restore it. DLL faults are excluded. | Legacy decoder does not inspect the actual divisor; do not copy its policy blindly. |
| Linuxi386 same file | `BiosSignal`+`SkipDivision` on SIGFPE/FPE_INTDIV. | `RedirectAccess`, SIGTRAP pending-state restoration. | Different signal/runtime boundaries from x32; not a safe host-wide64-bit handler. |
| Shared x32 guest before this change | `build_ps5.py` excludes desktop `port_biosfault`; `port_biosfault_ps5.c` is only a stub. | Known `BIOS_PTR` translations remain active in source. | Stub's old claim that every host handled faults was false for Windows/Linux. |
| Real PS5 host `app/ps5_host.c` | `OnFpe` decodes DIV/IDIV within guest region and skips it. | `OnSegv`/`OnTrap` have a separate bus/register decoder. | No real-zero check; region includes more than text, and REX.W handling is not equivalent to a32-bit contract. Not copied here. |
| Windowsx64 before this change | `crash_report.h` records first fault and leaves it fatal. | No general bus redirection. | Crash reporting is not GBA emulation. |
| Linuxx64 before this change | No SIGFPE compatibility handler. | No general bus redirection. | Same flat x32 image does not imply PS5's host handlers are present. |

## New Windows/Linux contract

The optional `GDV1` extension after the existing nine-word guest header holds
the linker's exact `__text_start`/`__text_end`. Original header fields and host
bridge ABI do not change; older hosts ignore the extension. New hosts reject
missing/invalid metadata for emulation and keep their fatal behavior. New guest
builds are needed to enable this path; old save-state build IDs remain checked.
Text metadata must fit the file and initialized image, and contain boot. Heap,
stack, data and host/DLL instructions are never eligible just because their
pages are executable.

`guest_divzero.h` decodes signed/unsigned DIV/IDIV, register and mapped-memory
operands, at8/16/32/64-bit widths. It handles high-byte registers, REX extensions,
SIB base/index/scale, signed displacements, RIP/EIP-relative and address-size32.
The actual operand at that width must be zero. Memory is readable only inside
the exact GBA/guest regions successfully mapped by the host; host/DLL memory
is excluded. Every instruction byte must fit the initialized guest text and
the architectural15-byte limit. FS/GS host TLS segments are outside the flat
GBA contract and rejected, as are unrelated/invalid opcodes.

Windows requires a matching exception address and division/overflow exception.
The latter is needed because Windows classifies the real zero AH operand as
C0000095 in the runtime test; decoding still rejects every nonzero divisor.
Linux requires SIGFPE/FPE_INTDIV. Quotient/remainder are zeroed at the correct
width: AL/AH for byte, AX/DX for word, EAX/EDX for dword and RAX/RDX for qword.
Unaffected high register bits and flags are preserved. RIP advances by the
complete bounded instruction.
INT_MIN/-1, nonzero unsigned quotient overflow, unsupported instructions and
host faults pass through unchanged. There is no per-frame check or arithmetic
hot-path instrumentation, no disabling64-bit execution, and no general AV
recovery. Windows invokes no CRT/OS/logging code on the guest fault stack;
Linux installs an alternate signal stack if none already exists and uses the
previous action for rejected signals.

## Verification

- `test_guest_divzero.py`: actual mapped x32-style code at10000000 faults
  through the production Windows VEH. RED without adapter: unsigned divide
  exitsC0000094. GREEN: DIV and IDIV zero return quotient/remainder0; division
  by2 returns61/remainder1; signed overflow remainsC0000095 and outside-text
  zero remainsC0000094.
- 20480 prefix/opcode/ModRM/divisor context cases exercise the real
  adapter and decoder. Rejections preserve the complete CONTEXT, including
  wrong exception address/kind, truncated bytes and invalid text ranges.
  Header tests reject legacy/missing metadata and file/image/boot violations.
- `test_guest_divzero_linux.c`: compiled against the existing Debian sysroot
  and executed in the existing x86-64 Debian VM,1GiB/2CPU, no network/GPU.
  Real SIGFPE signed/unsigned zero and nonzero controls pass; overflow and
  outside-text remain fatal SIGFPE. A wrong si_code forwards without context
  mutation. SA_ONSTACK and the actual alternate stack are checked.
- `test_guest_divzero_operands.py`:250 real Windows execution cases cover all
  four widths, DIV/IDIV, byte/high-byte/REX registers, memory base/SIB/index-only,
  positive and negative8/32-bit displacement, R13 base, RIP-relative and
  address-size32. Each valid mode has zero and nonzero controls; unapproved
  divisor memory retains the original fatal exception. The emitted equivalent
  Linux fork runner passes the same250 cases in the existing Debian runtime.
  The earlier register32-only block failed this probe with C0000094 on byte
  division; it is not presented as general fallback coverage.
- Full Windows/Linux host translation units pass cross-target syntax checks
  with existing SDL headers. Linux AArch64 syntax also passes: the shared Linux
  source excludes the x86 signal adapter for that architecture. This uses the
  existing AArch64 sysroot and platform-neutral SDL declarations, not a linked
  AArch64 SDL runtime. No new SDK or VM was created.

## Explicit remaining work

Disassembly of the read-only existing EU x32 ELF finds526 DIV/IDIV instructions,
including69 memory operands and16-bit divisions. The general decoder now covers
those flat register/memory instruction forms; it does not require enumerating
each source-level division. There is no claim that all526 are naturally reachable or ever
have divisor0. The exact scan is a code inventory, not526 runtime successes.

General GBA bus/open-bus/NULL parity is still absent from Windows/Linux64 hosts
except translated source sites. It needs a separate mechanism-specific audit;
arbitrary access violations must not be resumed. ARM64 hardware quotient-zero
behavior also does not by itself establish remainder-zero equivalence.
This work does not identify Moto374 or Samsung272 hardware causes.

Original Thumb signed INT_MIN/-1 wraps to INT_MIN in the tested cartridge.
This block deliberately does not emulate nonzero x86 overflow: Windows still
reportsC0000095 and Linux retains SIGFPE. That known difference is not silently
classified as divisor0, and needs a separate explicit arithmetic contract.
