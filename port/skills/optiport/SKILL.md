---
name: optiport
description: Use when the user invokes OptiPort or requests systematic stability and performance improvements for a recompilation, decompilation or source port, especially with optional assets or constrained devices.
---

# OptiPort

Help people improve their game without requiring programming knowledge. Infer build commands, architecture, backends and constraints from the repository before asking questions. Explain results in plain language; ask only for missing information that changes the work. Do not present a technical checklist as work the user must perform.

Convert findings into tested improvements throughout the investigation. Within the user's authorization, implement, verify, commit and push coherent blocks as they become ready; do not leave the work as an audit report until the entire pass ends. Follow repository PR and integration conventions. Sending test builds or publishing releases requires authorization for that action; this skill does not grant it. If credentials, hardware or tools are unavailable, continue independent work and state the specific limitation.

## Evidence and delivery

Establish protected installations, active branches, target devices, exact asset/configuration versions and known reproductions. Test in isolated copies. Keep a living work board with evidence and unresolved work. Delegate independent bounded work when authorized; preserve other agents' changes.

Classify each finding before fixing it: memory/ABI correctness, unavailable resource, unbounded work, lifecycle error, or measured performance cost. A crash during a load is not evidence of OOM. Preserve logs, match addresses to the exact binary and reproduce the reported transition rather than a similar intro. Record failure before the fix and the corresponding success afterward when feasible.

Ship independently reviewable blocks continuously. Run checks appropriate to the changed paths, then integrate through the authorized workflow. For test builds, record source SHA, configuration, target architecture, hashes and known limits. Include no ROMs, personal saves or private logs. Describe reporter confirmation separately from local regression success.

## Areas to cover

| Area | Decisions and evidence |
| --- | --- |
| Portability | Respect original data alignment, integer widths, endian and emulated hardware semantics. Do not assume native ABI alignment for fixed cartridge or disc data. Upgrade cached generated glue as well as clean builds. |
| Safe fallback | Check allocation, decoder, texture/upload and audio queue results. Only suppress original rendering/audio after the replacement is usable. Existence of a file is insufficient. Preserve story, input and logic speed. |
| Profiling | Compare identical deterministic baseline/HD workloads, cold and warm. Record load latency, frame p50/p95/p99, stalls and peak memory. Distinguish process RSS/private from VRAM and software rendering from device GPU. |
| Resource budgets | Validate metadata before decoding; account for overlapping buffers, CPU copies and texture pairs. Bound caches and evict only inactive resources. Check partial failures and release ownership exactly once. |
| Async and preloading | Move measured blocking work off critical paths where safe. Honor renderer/audio thread affinity. Use bounded queues, cancellation and generation IDs so late results cannot resurrect obsolete scenes. |
| Streaming | Bound video/audio buffers and producer work. Apply backpressure when consumers lag; detect decoder EOF, zero progress, seek failure and rejected output. Avoid unlimited retries and blocking teardown. |
| Frame pacing | Measure regularity as well as average FPS. Budget work across frames and reuse resources. Reduce optional quality or presentation frequency before altering emulated simulation timing. |
| Lifecycle | Verify focus loss/resume, suspend, device reset and scene transitions. Recreate invalid resources safely and cancel stale work. Do not catch arbitrary memory corruption and resume it. |
| Progress protection | Check existing save errors and atomic replacement. Keep validated backups/recovery separate from manual slots. Do not promise crash recovery from incompatible savestates or partial writes. |
| Observability | Retain current log plus bounded history (five previous sessions is a useful starting point), record fallback cause without per-frame spam, and preserve logs if rotation fails. Exclude secrets and personal data. |

## Verification and progress

Use the actual changed producer/consumer in fault-injection tests where possible: missing/corrupt/truncated/oversized input, exhausted pool, failed upload, stalled decoder, full queue and scene cancellation. Include valid resources as controls. Sanitizers and binary inspection complement device testing; neither proves every platform is fixed.

For a broad pass, read [the verification layers](references/verification-layers.md). Select meaningful checks for the actual targets; do not claim all platforms are verified because one build succeeds.

Report progress as completed **work**, never a percentage of stability or bugs eliminated. Define the denominator and completion criteria in the board, keep estimates visibly provisional and record scope additions. Separate implemented, verified, pushed, integrated and confirmed on reporter device. Continue the remaining authorized work after each delivery.

Grow the skill from verified lessons: describe the trigger, failure mechanism, useful check and limits. Keep project-specific paths, tickets and transient progress in that project's board; promote a reusable lesson only when it changes decisions. Preserve existing user constraints and review new guidance for conflicts.
