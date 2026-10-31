# Modern online VS implementation plan

> **For agentic workers:** Use superpowers:executing-plans inline. No implementation workers; the only worker authorized by the user cleaned temporary duplicates.

**Goal:** Add an explicit classic/modern toggle and replace the beta's virtual cable with asynchronous preparation and direct immutable frame inputs, preserving original VS rules and full-speed simulation.
**Architecture:** A pure bounded protocol core owns received input/reliable-control history outside snapshots. A small transport adapter uses PortNetlink datagrams. A game adapter reuses original menu/deck validation and button-state routines but never SioTransferFrames/NetSlotsFrame during modern play. Local rollback uses original game snapshots with a modern logical-frame counter.
**Tech stack:** C99/C++17, SDL3 already bundled, Python standard-library tests and network shaper; no new server or library installation.
**Spec:** docs/superpowers/specs/2026-10-30-modern-online-vs-design.md, approved by user30Oct with explicit testing toggle.

## Global constraints

- Preserve classic code/defaults and all existing transport choices. The motor toggle is independent of endpoint and persists in online-engine.cfg without PortConfig ABI changes.
- Only original two-player VS; no story coop. Only animaciones-evi/recomp-games if a server change is proven necessary.
- No protected ROM/SAV/installation/release edits; QA uses owned ROM read-only and fresh evidence directories.
- jobs1/BelowNormal/one workload, no further Computer Use after user Escape. Native game capture is optional and must preserve wall-clock timing.
- Protocol frames<=512bytes; fixed-width little-endian values, no pointers or raw padded C structs on wire. Classic rooms3 and modern rooms4 remain separate.

## Review focus

1. A local key already published for a logical frame must never change on resimulation; duplicate/reordered remote packets must not rewrite confirmed inputs.
2. Menus must progress asynchronously without confusing a late profile/deck message with the next transaction or consuming stale ready events.
3. Snapshot allocation/old-history/desync failures must not silently fall back to the classic wait path.
4. Both sides must apply the same seed/settings and logical frame0 despite different wall-clock startup times and role-local views.
5. Configuration roundtrips must preserve classic/manual endpoints and default to classic, with no ABI or offline regression.

### Task1: toggle and pure protocol

**Files:** new port/include/port/port_online_config.h; new port/include/port/port_online_protocol.h; new port/src/port_online_protocol.c; tests port/tools/test_online_protocol.py and test_online_config.py.
**Interfaces:** configuration parses classic/modern text, default classic. Protocol state ModernPeer is heap-owned; ModernInit(role,region,session), ModernReceive(data,len,now), ModernPump(now,send,context), ModernQueueControl(kind,id,payload,len), ModernReadControl, ModernInput(frame,local,&keys0,&keys1), ModernTakeRollback, ModernConfirmHash.

- [ ] Write compiled failing tests for independent direct local input, immutable resim, missing-remote prediction, exact earliest correction, stale/out-of-order/duplicate frames, malformed/oversize/version/nonce/role rejection, reliable-control delivery and ACK/retransmit/wrap. RED before implementation.
- [ ] Implement bounded input history256frames and reliable queue32messages, deterministic retransmit on monotonic time. Packet header KHM2/version4/type/role/session/sequence/ACK, type-specific explicitly encoded payload. No socket, sleep, pacing or game globals in this core.
- [ ] Run compiled C tests (including simulated240/300ms, jitter/loss schedule), fix and commit only when GREEN.

### Task2: transport/session and launcher

**Files:** new port/include/port/port_online.h; new port/src/port_online.c; port/decompui/decompui.cpp; port/linux/launcher/rekhcom_launcher.py; existing i18n files/table; port/src/port_lobby.c.
**Interfaces:** PortOnlineModern reads env KHCOM_ONLINE_ENGINE or online-engine.cfg. PortOnlinePump uses bounded PortNetlinkRecv/Send and existing provider. Session provides role/ready/control/input/hash/error and frame/rollback query adapters. Modern peer clocks are measured with timestamp echoes; control message ACK never waits in a frame.

- [ ] Add production-consumer toggle roundtrip tests, run RED; implement and run GREEN for both values/default/manual endpoint and Linux CLI, update only new localization rows.
- [ ] Add modern protocol4 lobby selection/announcement, preserve classic3 and all existing providers. Handshake refuses region/version mismatches before game admission.
- [ ] Unit/socket-test the transport bridge and confirm the existing relay forwards the opaque protocol unchanged; do not rebuild VPS without a demonstrated need.

### Task3: asynchronous original-VS preparation

**Files:** game adapter src/sio/modern_online.inc included by src/sio/chara.c; hooks in src/sio/chara.c and src/sio/mode_sio.c.
**Interfaces:** ModernConnectUpdate handles network/user acceptance without SioRunStateMachine. Profile and deck transactions have typed reliable IDs and explicit serializers; original validators/builders are reused. Menu commands are semantic reliable events/latest settings, not cable words/slot clocks. ModernBattleBarrier verifies canonical descriptor/hash and reserves rollback before starting mode_vsbattle.

- [ ] Compiled adapter/serializer tests reject truncated profile/deck, stale transaction ID and mismatched settings; prove no SioTransferFrames call in modern selection. Preserve classic consumer tests.
- [ ] Implement connect/profile/deck/settings/ready/start lifecycle, keep original rendering/UI responsive. Character fields are hp/maxHp/level/win/lose/ap/u64stocks/u64stocks2/worldFlags/seed; deck serializer uses original documented card fields, not padding.
- [ ] Enter the original VS with the same descriptor and seed; initialize the modern logical frame counter at the same engine boundary for both roles. Test normal user flow, cancel, ready ordering and rematch.

### Task4: direct battle inputs and rollback

**Files:** adapter above; src/sio/sio.c query dispatch; port/src/port_rollback.c; src/sio/mode_vsbattle.c hash/initialization.
**Interfaces:** Modern frame counter is in captured game memory; immutable input history/received packets are outside it. Modern SioLinkUpdate updates SioKeyStateUpdateA/B from the direct pair, clears only compatibility receive-empty bookkeeping, and advances exactly one new logical frame per normal simulation frame. NetRollbackSession/NetLinkFrame/NetLinkMade/NetTakeRollback dispatch to the selected motor.

- [ ] Write real-consumer tests proving local input does not return through the parent, replay does not publish new keys, and no packet wait or pacing yield is invoked in modern battle.
- [ ] Adapt existing snapshots to64modern/32classic frames without changing legacy default, reserve before battle. Treat snapshot allocation/too-old correction/desync as an explicit session error with responsive UI, never classic lockstep.
- [ ] Keep rendering/audio/external effects out of resimulation; expand canonical confirmed-state coverage beyond position/HP/RNG and test role ordering, actions, deck and timers.
- [ ] Test deterministic recorded controls and randomized rollback schedules, confirmed-frame hash equality and input-to-simulation response before claiming smoothness.

### Task5: measured A/B and native video

**Files:** existing fullspeed report; reusable QA shaper/run/CSV/video evidence under pruebas/vps-online-beta/chile-spain-smooth; server README only if needed.

- [ ] Build affected WindowsEU32 targets (jobs1/BelowNormal), run classic control and modern candidate through the same real VPS with calibrated240and300ms RTT, then jitter/loss and asymmetric delay.
- [ ] Each hot phase>=1200frames per player:>=99.9% of59.7275Hz, p99<25ms, no>=50ms gaps,0blocked network frames,0desync/crash/fallback. Log correction depth/magnitude separately.
- [ ] Compare capture off/on, encode wall-clock-correct video of both games without desktop takeover; if unavailable, report video pending rather than flattening pauses.
- [ ] Extend build/consumer verification to32/64/regions as warranted, clearly distinguish physical-device/cross-country evidence. Review, small PRs, merge underCAS lock, distinct private beta ZIP; preserve previous ZIP.

## Ledger

- OptiPort online requested30Oct: implementation8c4202b03 pushed to PR529. Measured protocol-byte reduction51.5/23.4/17.0% at24/240/300ms modeled workloads, same final1200inputs/peer and datagram count,32/64 identical. Added explicit replayPending/finalization to prevent hash validation after dequeue but before replay; RED/GREEN. EU32 build7/7 + no-work, protocol/bridge/transport/snapshot tests pass. Still no modern gameplay routing or actual modern fullspeed/video certification.
- Lessons integrated into OptiPort22/main35dfad84092b951e72753bd235fcaf8a4d1bc7d4 under coordination lock (released). Installed Codex copy verified29canonical Markdown files; changed files/metadata backed up. Details and reproduction in port/progress/ONLINE_OPTIPORT.md. Benchmark builds only the workload entrypoint so older baseline implementations do not require later regression-only APIs; final baseline/candidate reproduction passes.

- User confirms30Oct that2.3 will ship with these online tests, and explicitly asks to keep improving and pushing. Foundation branch pushed to origin as MatyMeatBoy. Keep any PR draft until async preparation, battle routing and actual VPS240/300ms validation are complete; this is not permission to publish an unverified release.
- Expanded pure-core tests: asymmetric75/225ms paths, jitter+/-35ms, deterministic burst loss plus1/13loss and actual17ms pump cadence, and reliable sequence wrap. New RED/GREEN regression: a packet from an old session with mismatched version/region must be rejected before it can fail the live session. Production transport and game-input consumer tests also pass after the fix.

- Foundation commit6aca86db9: pure protocol, shared engine configuration, Windows launcher consumer, Linux CLI, transport bridge and profile/deck codecs. Compiled tests pass for240/300ms simulated latency/loss, immutable inputs, bounded history and explicit encodings. This is not gameplay integration or in-game fullspeed evidence.
- Additional regression fixes pending next commit: reject future packets that would alias retained local ring entries; bound history by missing remote inputs as well as local ACKs; defer state-hash send/compare while a correction is pending. WindowsEU32 jobs1/BelowNormal foundation build started; no product-source edits during compilation.
- Launcher room filters now distinguish classic3/modern4 for relay and Steam; Linux desktop menu includes both engine choices. No VPS edits or new package publication.
- Remaining work is Tasks3/4 and actual Task5 game validation/video. The selector alone must not be presented as a working modern battle engine.
- WindowsEU32 foundation build completed372/372, subsequent ninja reports no work. Added actual game-side direct-input bridge (compiled into chara.c but not yet routed from SioLinkUpdate) and tested immediate controls, immutable replay, role ordering and no replay network I/O. Production snapshot allocator now reserves64 for modern/32 for classic; compiled tests verify allocation failure and an out-of-history correction become explicit modern errors. Final incremental build in progress. The async preparation/barrier and routing remain unfinished; no new beta ZIP or in-game claim.

- User explicitly approved new design and required a toggle for testing. Proceed inline without another approval question.
- Baseline old motor at239ms:59.470Hz/99.569% speed, nine~33.485ms intervals in35s; original fullspeed report RED. 8400battle frames/60checks/0desync/0packet waits do not by themselves satisfy the new acceptance.
- Cleanup worker removed only reconstructed duplicate binaries and an empty failed video,140008656logical bytes; manifests and original evidence retained. No implementation delegation.
- No game product source has changed at plan start; starting revision0b0185ee7 contains only diagnostics/design.
