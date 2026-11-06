# x32 PPU multicore implementation plan

> Execute inline, one CPU-heavy validation at a time; one extra worker is investigating issue610.

**Goal:** render every59,7275Hz frame at1440p/x2 without changing pixels or game speed.

**Architecture:** reuse the existing row-independent PPU pool used on native desktop/Switch. Add the missing x32 low-stack/TLS support and SDL native host thread/semaphore implementation. Keep old hosts single-threaded. GPU presentation already runs on hardware; a new GPU PPU would require its own accuracy/backend project and is not the first measured fix.

**Constraints:** private QA copies, original ROM/SAV/install/Atlas intact; jobs1/BELOW_NORMAL; preserve original viewport, HD/non-HD, effects, row events, unsafe-scene serial fallback. No release before verification. Border adaptation follows performance.

- [x] Regression: actual host op0 currently returns1; test worker stack pointer below4GiB, independent initialized/zero TLS, and simultaneous guest-to-host calls.
- [x] Extend thread_host.h with SDL mode, explicit SysV guest entry on Win64; same0..4 protocol, no slots/table changes. Linux64 shares mode. Other hosts unchanged.
- [x] Add x32 worker stacks aligned512KiB, stack-derived slot0..7, per-slot host stack, thread entry assembly. Preserve main stack/ABI. Add emulated TLS controls using guest heap locks, correct size/alignment/template and one context per slot.
- [x] Compile port_ppu.c with PORT_PPU_THREADS/-femulated-tls only in desktop x32 producer. Other producers and native builds stay compatible.
- [x] Guest smoke mode verifies stack/TLS/bridge concurrency before enabling gameplay pool; compile and execute Windows64 ABI/boot/BIOS and pool tests, then pixel-exact single/multicore verification including effects and transitions.
- [x] Compare paced field/arena x2/x4, HD on/off at identical canvas/camera; present count, skipped frames, work and interval distribution. Reject changes without consumer improvement.
- [ ] Validate Linux64 build/runtime and unchanged remaining platform paths; document physical device limitations. Review then PR/integrate with CAS lock.

Measured baseline: field-x2 D3D11 hidden1280x720 canvas, ~18,6ms PPU,19/60frames skipped,~41presentations/s despite~59,7logic. Small empty-row shortcut did not produce a clear workload gain and was removed. Evidence in pruebas/pacing-v28-6900xt-20261106.

Final merged-source Windows64 EU build and ABI tests pass at4b4a90727. Five paced cases (field x2 native/HD, field x4 native/HD, HD arena x2) each presented331/331 warm frames with zero skips. Mean image-submission gap16.742ms; worst case p99=17.268ms. The live HD case also crossed field-to-battle with VRR host eligibility confirmed. PPU work dropped to approximately8ms. These are API submissions on local Ryzen7435HS/D3D11, not physical scanout measurements or validation of the user's RX6900XT.

Seventeen exact-pixel verification cases passed before the final integration of approved art: all six views HD off/on, scenes095/081/120, arena effects and active affine mosaic. Final-source Linux64 EU crossbuild passes; Linux runtime and remaining platform gates are still pending. Scheduler fixtures cover native/browser clocks, skipped/failed presents, UI exit, late wakeups, and nonblocking link polling. Independent final source review found no blocking defect; no release is certified by that review.

Native Windows32 EU also builds and passes the same17 pixel comparisons on the merged-art source. Enable its existing inner-row pool by default too, preserving explicit env0 and unsafe-scene/single-core fallback. Actual selector fixtures fail before/pass after for native Windows, native relocated Windows, desktop x32, Switch and other targets. An off/default/default/off field-x2 comparison delivers331/331 images in every run: PPU means8.71/4.07/4.16/9.41ms, worst default-case present p99=17.016ms. Logs confirm default activation without an environment override. These runs record3.37–4.45 logical cores used by other applications; earlier high-load runs are retained rather than represented as resolved outliers. Linux32/Android build threading flags are unchanged.
