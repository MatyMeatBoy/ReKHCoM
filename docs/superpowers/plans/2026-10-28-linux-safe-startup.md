# Linux safe startup implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Offer issue330 a manual reversible native32/software startup attempt without editing saved runtime preferences, replacing personal data or inventing a physical-device fix.

**Architecture:** Add a prefix CLI option to the existing wrapper. Parse options before release migration; safe startup skips that migration, selects the existing platform-native32 path, forces bundled libraries and SDL software rendering in this child process only. Default startup, CPU checks, argument escaping and the post-guest retry boundary remain intact. The coordinator's separate457 exit/boot logging block must be preserved.

**Tech Stack:** POSIX sh, existing Python wrapper tests with Git sh/native sh, existing SDL3 software backend.

**Spec:** Coordinator issue330-startup-compatible-fallback assignment; issue330 remains externally unconfirmed.

## Constraints

- Only --safe-startup explicitly activates the profile. It wins over runtime-mode flags for this launch, and is consumed before game arguments. No global environment or Steam configuration changes.
- REKHCOM_GPU=0 bypasses system32 GPU libraries; SDL_RENDER_DRIVER=software selects the actual renderer. No window size, media, HD quality or gameplay rules change.
- The wrapper must not migrate or write decompui.ini in safe mode; it must not touch kh_com.ini or SAV. Ordinary game save/settings behavior is unchanged.
- Keep exact quoting for paths containing spaces, dollar/backtick, apostrophe and percent. Do not force Proton for native ELF or disable the Steam overlay globally.
- No automatic retries after a guest boot marker or intentional137/143 exits. No GPU/QEMU until coordinator grants a lease.

## Tasks

- [x] RED: exercise the shipped wrapper with fake owned32/64 children; safe profile wins over forced64, consumes only its own flag, forwards literal arguments, sets the child-only environment and preserves settings/save sentinels including an outdated version marker.
- [x] Implement the smallest parser/profile change and clear log message; leave desktop registration escaping unchanged.
- [x] GREEN: full existing shell suite, safe/normal sequential invocations, both argument orders, CPU guard and preboot/postboot/137/143 controls. Verify no unexpected child or settings mutation.
- [x] Document three reporter steps and limits. A software renderer can be slower; success or failure does not identify the original cause.
- [ ] Submit a small PR with exact source/test evidence; coordinate457 before the test archive is built. Any real runtime validation is a separately leased final step.
