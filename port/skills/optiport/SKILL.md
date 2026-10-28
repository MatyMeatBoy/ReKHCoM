---
name: optiport
description: AI skill for incremental debugging and optimization of software, source ports, recompilations, decompilations. Use when the user invokes OptiPort to optimize any software, or requests systematic stability, performance or 32-to-64-bit migration work for a recompilation, decompilation or source port. The method applies to engineering, design, web, desktop, data processing and other software.
---

# OptiPort

## Load the latest skill before applying it

At the start of each OptiPort pass or resumed session, check the latest committed
Markdown in the default branch of the canonical repository:
[MatyMeatBoy/OptiPort](https://github.com/MatyMeatBoy/OptiPort) (`main` unless the
repository reports a different default). Refresh that remote reference, or use
read-only repository access if no local clone is available. Do not create another
checkout just to check a version.

Compare the instructions actually loaded from `SKILL.md`, and the reference
modules needed for the task, with that upstream revision. An up-to-date source
checkout does not prove that an installed or already loaded copy is current.
**If they differ, load the latest upstream `SKILL.md` and relevant Markdown
modules before continuing, and synchronize the installed copy being used.**
Record the source commit so the check is traceable.

Read upstream files without switching or resetting an active working tree
(for example, with `git show` after fetching). Preserve local edits and useful
additions with a backup/diff or a three-way merge; never overwrite them blindly.
If the repository cannot be reached, state that freshness could not be verified
and use the last verified local copy, without claiming it is the latest or
blocking unrelated project work.

Help people improve their software without requiring programming knowledge. Infer build commands, architecture, backends, domain and constraints from the repository before asking questions. Explain results in plain language; ask only for missing information that changes the work. Do not present a technical checklist as work the user must perform. Use [domain-aware optimization](references/domain-aware-optimization.md) to identify what must remain correct before changing performance. Select checks from the actual application contracts.

Convert findings into tested improvements throughout the investigation. Within the user's authorization, implement, verify, commit and push coherent blocks as they become ready; do not leave the work as an audit report until the entire pass ends. Follow repository PR and integration conventions. Sending test builds or publishing releases requires authorization for that action; this skill does not grant it. If credentials, hardware or tools are unavailable, continue independent work and state the specific limitation.

## Professional presentation

Present and apply OptiPort exclusively as a professional method for general software optimization. Use product-specific workflows and neutral technical examples in every title, report and explanation. Exclude unrelated sector references and carry this rule into future guidance and examples.

For each application, use [the general-software protocol](references/software-general.md): record a comparable before/after workload, preserve correctness and report only measured gains. An implementation without a comparable measurement is not evidence of a performance improvement.

## Evidence and delivery

Establish protected installations, active branches, target devices, exact asset/configuration versions and known reproductions. Test in isolated copies. Keep a living work board with evidence and unresolved work. Delegate independent bounded work when authorized; preserve other agents' changes.

Prioritize reported crashes, progress/data corruption and blockers before optimization or polish, weighing severity, reproducibility and affected users. A confirmed root cause can expose a shared defect: derive focused sibling checks from that mechanism and preserve a reusable lesson when verified. Bound the investigation by evidence, resources and physical constraints; do not chase every possible bug or let cosmetic work displace critical failures.

Classify the failure mechanism before fixing it, such as memory/ABI correctness, unavailable resource, unbounded work, lifecycle error or measured performance cost. A crash during a load is not evidence of OOM. Preserve logs, match addresses to the exact binary and reproduce the reported transition rather than a similar opening screen. Record failure before the fix and the corresponding success afterward when feasible.

Ship independently reviewable blocks continuously. Run checks appropriate to the changed paths, then integrate through the authorized workflow. For test builds, record source SHA, configuration, target architecture, hashes and known limits. Include no proprietary inputs, personal documents or private logs. Describe reporter confirmation separately from local regression success.

When preparing any delivery, apply [release readiness](references/release-readiness.md) to that delivery's scope. This is a required evidence check, not an additional permission request. Treat a reproducible failure in the promised scope as blocking; distinguish an explicitly limited prerelease from a verified fix.

## Areas to cover

| Area | Decisions and evidence |
| --- | --- |
| Portability | Respect original data alignment, integer widths, endian and format semantics. Do not assume native ABI alignment for fixed-layout external data. Upgrade cached generated glue as well as clean builds. |
| 32/64-bit migration | Separate host architecture from serialized data/address widths. Audit ABI boundaries and dependencies, retain a compatible 32-bit path when needed, and measure feature-equivalent builds before recommending 64-bit. See [migration guidance](references/architecture-migration.md). |
| Safe fallback | Check allocation, decoder, texture/upload and audio queue results. Only suppress original rendering/audio after the replacement is usable. Existence of a file is insufficient. Preserve results, input, workflow and logical timing. |
| Profiling | Compare baseline and change with identical deterministic workloads and equivalent optional quality, cold and warm. Record load latency, frame p50/p95/p99, stalls and peak memory. Distinguish process RSS/private from VRAM and software rendering from device GPU. |
| Resource budgets | Validate metadata before decoding; account for overlapping buffers, CPU copies and texture pairs. Bound caches and evict only inactive resources. Check partial failures and release ownership exactly once. |
| Async and preloading | Move measured blocking work off critical paths where safe. Honor renderer/audio thread affinity. Use bounded queues, cancellation and generation IDs so late results cannot resurrect obsolete scenes. |
| Streaming | Bound video/audio buffers and producer work. Apply backpressure when consumers lag; detect decoder EOF, zero progress, seek failure and rejected output. Avoid unlimited retries and blocking teardown. |
| Frame pacing | Measure regularity as well as average FPS. Budget work across frames and reuse resources. Reduce optional quality or presentation frequency before altering authoritative processing semantics. |
| Lifecycle | Verify focus loss/resume, suspend, device reset and scene transitions. Recreate invalid resources safely and cancel stale work. Do not catch arbitrary memory corruption and resume it. |
| Progress protection | Check existing save errors and atomic replacement. Keep validated backups/recovery separate from manual slots. Do not promise crash recovery from incompatible snapshots or partial writes. |
| Observability | Retain current log plus history bounded for the project's needs, record fallback cause without per-frame spam, and preserve logs if rotation fails. Exclude secrets and personal data. |
| Workspace and storage | Keep code, releases, test evidence and shared inputs understandable to people. Measure allocated bytes without recounting links; deduplicate immutable inputs safely and size reusable VM environments from guest usage and workload peaks. See [workspace and storage](references/workspace-storage.md). |

## Verification and progress

Use this skill as a selective reference bank. Load only the module that changes the next decision; the workflow remains usable offline. External repository research is optional, not a prerequisite for fixes the current code already explains.

| Need | Read when relevant |
| --- | --- |
| Portable control layouts, bindings and tutorial button prompts | [Control layouts](references/control-layouts.md) |
| Adapt presentation from a remake/remaster while preserving original rules | [Remaster equivalence](references/remaster-equivalence.md) |
| Adapt optional work to weaker hardware without penalizing stronger devices | [Adaptive performance](references/adaptive-performance.md) |
| Implement through understood engine internals or decompiled source | [Engine-informed implementation](references/engine-informed-implementation.md) |
| Recover source, verify matching binaries or evaluate decompilation progress | [Decompilation contracts](references/decompilation-contracts.md); load only for binary/source recovery work |
| Clarify controls, status, translations or responsive previews | [UX wording and localization](references/ux-localization.md) |
| Coordinate authorized workers or finish isolated workspaces | [Coordination and workspace hygiene](references/coordination-workspaces.md) |
| Organize folders for people, reduce disk usage or right-size VM environments | [Workspace and storage](references/workspace-storage.md) |
| Engineering, design or another requested software domain | [Domain-aware optimization](references/domain-aware-optimization.md); optional [domain sources](references/domain-sources.md) |
| Find evidence efficiently or resume an investigation | [Progressive retrieval](references/progressive-retrieval.md) |
| Reproduce a scene, transition or timing failure | [Reproduction harnesses](references/reproduction-harnesses.md) |
| Audio speed, queued units, bridge compatibility or producer starvation | [Audio stream contracts](references/audio-stream-contracts.md) |
| Linux software starts in a terminal but fails through a launcher or Steam Deck Gaming Mode | [Linux launch context](references/linux-launch-context.md) |
| Locate load, frame or memory costs before optimizing | [Profiling](references/profiling.md) |
| General coverage, devices and long sessions | [Verification layers](references/verification-layers.md) |
| Host/data architecture and 32/64-bit migration | [Architecture migration](references/architecture-migration.md) |
| Known resource, media, save, log or font mechanisms | [Verified lessons](references/verified-lessons.md) |
| Find and adapt useful patterns from other projects  | [Repository research](references/repository-research.md); optional [source catalog](references/source-catalog.md) |
| Prepare a delivery | [Release readiness](references/release-readiness.md), required for that delivery |

Use the actual changed producer/consumer in fault-injection tests where possible: missing/corrupt/truncated/oversized input, exhausted pool, failed upload, stalled decoder, full queue and scene cancellation. Include valid resources as controls. Sanitizers and binary inspection complement device testing; neither proves every platform is fixed.

Select meaningful checks for the actual targets; do not claim all platforms are verified because one build succeeds. Apply a verified lesson's mechanism to the current code rather than assuming another project's patch or numeric budget fits this engine.

Report progress as completed **work**, never a percentage of stability or bugs eliminated. Define the denominator and completion criteria in the board, keep estimates visibly provisional and record scope additions. Separate implemented, verified, pushed, integrated and confirmed on reporter device. Continue the remaining authorized work after each delivery.

Grow the skill from verified lessons: describe the trigger, failure mechanism, useful check and limits. Keep project-specific paths, tickets and transient progress in that project's board; promote a reusable lesson only when it changes decisions. Preserve existing user constraints and review new guidance for conflicts.
