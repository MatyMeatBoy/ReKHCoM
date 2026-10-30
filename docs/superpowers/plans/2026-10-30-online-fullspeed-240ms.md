# Full-speed original multiplayer at240ms RTT

> **For agentic workers:** Use superpowers:executing-plans inline; user prohibits workers.

**Goal:** Preserve59.7275Hz simulation on both peers at the measured Chile–Spain route boundary; connection/hash checks alone do not satisfy the user.
**Architecture:** Keep deterministic input prediction, rollback and bounded ring protection. Remove only the network's deliberate whole-frame presentation-clock extension if controlled A/B proves it causes the slowdown. No relay/server changes are needed for this client pacing issue.
**Tech Stack:** Existing C/SDL game, Python UDP test shaper, existing per-frameCSV and real-wall-clock recording.
**Spec:** User30Oct explicitly requires full speed on both sides independent of ping; current test envelope is about240ms, not arbitrary outages or slower hardware.

## Constraints and review focus

- Only the existing authorized VPS service, own disposable QA directories and owned ROM read-only. No other projects, ERP, installation or releases.
- Preserve VS state hashes, menu lockstep, prediction/snapshot bounds and safe handling of a genuinely missing peer. Never claim zero input travel time.
- Jobs1/BelowNormal/one workload. No Computer Use actions after the user stopped it with Escape; video must be captured without taking over the desktop or remain pending.
- Measure warm battle separately from menu/startup, resize operations, capture overhead and teardown. Require both sides independently.
- Keep target source/binary hashes and record failures, not only the successful trial.

## Execution

- [x] RED: grade the baseline real-gameCSV with port/tools/test_netlink_fullspeed_report.py. Require>=99.9% original clock, p99<25ms, no>=50ms intervals,0network-waited frames,>=10state checks and0desync per side.
- [ ] Remove the deliberate PortLinkYieldFrame call and its now-unused API, leaving bounded prediction and rollback unchanged. Build only affected targets withjobs1/BelowNormal.
- [ ] Repeat the same real-VPS wall-clock-shaped240ms movement/attack workload, headless for nonintrusive measurement. Compare measured simulation clock and sync with the saved baseline.
- [ ] If full speed/hash checks pass, run recorded gameplay using native game capture with wall-clock frame timestamps or a consent-compatible window recording; verify duration and frame pacing. Do not flatten pauses into a fixed-FPS montage.
- [ ] Review, PR, merge under CAS lock only if evidence supports the change; preserve the old ZIP and create a distinct test delivery if needed.

## Evidence so far

Baseline run-01:239.20ms mean end-to-end relay RTT,242.68ms calibration max;8400battle frames and60state checks/0desync per peer, no packet-waited frames. Warm35s sample:both59.470Hz, nine near-double33.485ms intervals while logic/PPU/present stayed low. This is below the full-speed criterion. GDI recording initially failed on odd981px window height; video is not delivered or represented as verified.
