# Preview latency observer implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Close the async preview evidence gap with measured, generation-aware request-to-fallback/HD-ready/presentation observations in the real Windows32/64 gallery.

**Architecture:** Preserve the existing selection, 75ms debounce, one decoder and serial/path-generation guards. Add only an opt-in bounded observer if the existing adapters cannot expose the required boundaries; distinguish CPU readiness, texture submission and actual renderer present return. Run cancellation contracts from the actual production functions before runtime tests. Trace files and fixture media stay in private test folders.

**Tech Stack:** C/SDL3, existing Windows32/64 hosts and game, Python fixtures and actual-function contract harness.

**Spec:** Coordinator assignment to optiport-v22-async; port/progress/OPTIPORT_FULL_PASS_20261028.md area9.

## Constraints

- No change to gameplay, media quality, 75ms debounce, defaults or protected installations.
- No Windows runtime/benchmark before coordinator WINDOWS_FUNCTIONAL gate; no VM without a lease.
- Reuse closed Windows32/64 producers from the profiling lane, with source/recipe/hash receipts. Do not duplicate its build.
- Fresh private cache directories define cold; warm uses the same validated input/generation. Do not claim cold from a process restart alone.
- GBA, HD and hybrid labels require observed provider state. Missing/corrupt HD keeps GBA fallback; never count fallback as HD success.
- Bounded in-memory events; disabled observer performs no clocks or per-frame file I/O. Presentation return is not physical scanout.

## Tasks

- [x] Re-read current source, queue and latest OptiPort394c34ff. Preserve Linux producer07d5 branch and all exact-TAR evidence.
- [x] Run production cancellation contracts: eight positive scenarios and two deliberately broken controls. Both negative controls must fail at their intended assertion.
- [ ] Record byte/source equivalence of the extracted381 functions and current producer, along with actual compile flags.
- [ ] Implement observer test first: request/serial/event/path generation, immediate GBA fallback, 74/75ms worker boundary, HD-ready, upload/draw and presentation boundary; cap events and verify observer-off path.
- [ ] Add minimal opt-in observation and private adapter only where needed; retain thread-affinity and avoid worker-thread writes into the renderer buffer.
- [ ] Prepare tiny legal valid/missing/corrupt media and EN/JP immutable-generation fixtures without altering original media.
- [ ] After WINDOWS_FUNCTIONAL, run matched cold/warm A-B-A workloads for GBA/HD/hybrid32/64; record timestamps, provider transitions, source and media hashes, sample count and observer overhead.
- [ ] Exercise stale completion, close during decode, rapid selection, generation changes, unavailable/corrupt input and deliberate final shutdown. Keep lifecycle contracts separate from elapsed-time evidence.
- [ ] Report measured distributions and precise limitations; integrate only a reviewable change with meaningful evidence. No percentage of bugs eliminated.

## Review focus

The same event with a changed media generation must invalidate the old result. Closing must not synchronously join a pending decoder; final shutdown deliberately does. A GBA fallback must stay visible until replacement readiness, while an obsolete HD completion must never replace a newer GBA/HD selection. A clock around outer VBlank includes unrelated work and is not preview latency.
