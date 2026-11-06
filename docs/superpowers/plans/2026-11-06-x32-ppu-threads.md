# x32 PPU multicore implementation plan

> Execute inline, one CPU-heavy validation at a time; one extra worker is investigating issue610.

**Goal:** render every59,7275Hz frame at1440p/x2 without changing pixels or game speed.

**Architecture:** reuse the existing row-independent PPU pool used on native desktop/Switch. Add the missing x32 low-stack/TLS support and SDL native host thread/semaphore implementation. Keep old hosts single-threaded. GPU presentation already runs on hardware; a new GPU PPU would require its own accuracy/backend project and is not the first measured fix.

**Constraints:** private QA copies, original ROM/SAV/install/Atlas intact; jobs1/BELOW_NORMAL; preserve original viewport, HD/non-HD, effects, row events, unsafe-scene serial fallback. No release before verification. Border adaptation follows performance.

- [ ] Regression: actual host op0 currently returns1; test worker stack pointer below4GiB, independent initialized/zero TLS, and simultaneous guest-to-host calls.
- [ ] Extend thread_host.h with SDL mode, explicit SysV guest entry on Win64; same0..4 protocol, no slots/table changes. Linux64 shares mode. Other hosts unchanged.
- [ ] Add x32 worker stacks aligned512KiB, stack-derived slot0..7, per-slot host stack, thread entry assembly. Preserve main stack/ABI. Add emulated TLS controls using guest heap locks, correct size/alignment/template and one context per slot.
- [ ] Compile port_ppu.c with PORT_PPU_THREADS/-femulated-tls only in desktop x32 producer. Other producers and native builds stay compatible.
- [ ] Guest smoke mode verifies stack/TLS/bridge concurrency before enabling gameplay pool; compile and execute Windows64 ABI/boot/BIOS and pool tests, then pixel-exact single/multicore verification including effects and transitions.
- [ ] Compare paced field/arena x2/x4, HD on/off at identical canvas/camera; present count, skipped frames, work and interval distribution. Reject changes without consumer improvement.
- [ ] Validate Linux64 build/runtime and unchanged remaining platform paths; document physical device limitations. Review then PR/integrate with CAS lock.

Measured baseline: field-x2 D3D11 hidden1280x720 canvas, ~18,6ms PPU,19/60frames skipped,~41presentations/s despite~59,7logic. Small empty-row shortcut did not produce a clear workload gain and was removed. Evidence in pruebas/pacing-v28-6900xt-20261106.
