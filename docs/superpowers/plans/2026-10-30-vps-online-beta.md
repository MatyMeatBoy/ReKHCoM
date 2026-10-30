# VPS online beta Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Offer the original online battle through an opt-in, preconfigured VPS beta.
**Architecture:** Harden the existing Python UDP relay, retain the game's deterministic link protocol, add a launcher preset, deploy a constrained container in the sole authorized service.
**Tech Stack:** Python standard library, C/SDL, C++/ImGui, Docker, Easypanel.
**Spec:** docs/superpowers/specs/2026-10-30-vps-online-beta-design.md

## Global Constraints

- Only animaciones-evi/recomp-games. Never ERP or other projects, no global VPS configuration.
- Original multiplayer only; coop and web transports are future work.
- Preserve existing saved selection, manual relay, direct IP and Steam.
- Single local build job, below-normal priority. No workers, protected installation or original data changes.
- PR and queue CAS; no direct main push or release replacement.

## Review Focus

- Occupied role must not be replaced by a new address; expire individual dead peers.
- Moving a member to another room must not leave a forwarding address in the old room.
- Public list replies must not amplify spoofed UDP requests; cap replies by request length.
- Per-IP accounting must be bounded and allow two clients behind one NAT.
- Persisted custom endpoints and beta selection must remain distinct after reopening.

### Task 1: Relay hardening

**Files:** port/tools/netlink_relay.py; port/tools/test_netlink_relay.py; port/src/port_netlink.c.
**Interfaces:** Relay(clock=time.monotonic, max_rooms=128, peer_timeout=30, rate=600, burst=1200, max_ips=4096); existing J/O/A/L/R/D frames retained.

- [x] Add failing behavioral tests for role collision, address move, individual expiration, malformed messages, max_rooms, bounded rate tracking, same-NAT clients and version filtering.
- [x] Run python -m unittest discover -s port/tools -p test_netlink_relay.py -v and record RED.
- [x] Implement validated frames <=513 bytes, monotonic expiry, bounded per-IP/global token buckets, role rejection until timeout, consistent membership cleanup and nonamplifying listing (2-byte legacy request returns empty R; padded request gets bounded rooms).
- [x] Pad the client list request to PORT_NETLINK_MAX; verify serialized response fits the existing receive buffer.
- [x] Run behavioral tests, real loopback UDP tests and malformed datagram fuzz, record GREEN and commit.

### Task 2: Launcher preset and deployment bundle

**Files:** port/decompui/decompui.cpp; port/decompui/netlink_preset.h; port/tools/test_netlink_preset.py; port/server/Dockerfile; port/server/README.md.
**Interfaces:** header-only preset helpers return endpoint spec and identify exact preset; launcher adds mode9, retains modes0..8. Endpoint45.8.207.150 UDP47800 only after public probe.

- [x] Write and run a failing compiled C++ preset roundtrip test (off/manual/Steam unchanged, exact beta recognized).
- [x] Implement opt-in preset with fixed endpoint, beta explanation, existing name/delay/stats controls, and localized strings in the existing i18n workflow.
- [x] Package Python relay only in non-root container, no ROM/assets/secrets. Document immutable deployment, resource limits, one replica and rollback.
- [x] Run compiled helper tests, launcher build/screenshot and relevant existing launcher checks; commit.

### Task 3: Authorized deployment and game validation

**Files:** evidence under pruebas/vps-online-beta; port/server/README.md; plan ledger.

- [x] Configure source and resources only on the existing recomp-games service, deploy reviewed bundle, publish sole required UDP port through its Advanced page.
- [x] Verify public UDP two-client join/list/forward/isolation and limits. Record endpoint and image/source revision, no sensitive logs.
- [x] Verify two actual game processes through public relay, with paired state hashes and clean exits; distinguish this from cross-region/physical-device validation.
- [x] Review whole diff, run relevant checks, open PR and record task evidence. Integrate only with approval and queue lock.

## Ledger

- Design approved by user30Oct: implement with needed improvements and research OpenRCT2/OpenTTD/decomps.
- Ruling: execute inline without a second plan approval — user explicitly says implement; prior no-workers mandate preserved.
- Ruling: retain UDP, not TCP copy — existing battle protocol already handles retransmit, lockstep and rollback.
- Research: OpenRCT2 FAQ explains TCP fits its low-update deterministic simulation; OpenTTD12 article separates coordinator/NAT/relay; SM64CoopDX wiki separates CoopNet and direct connection. These inform design, no code copied.
- Ruling: reject occupied-role replacement until30s inactivity — legacy protocol has no proof of reconnection ownership, so instant rebinding would allow room theft.

- Task1:13 behavioral/socket tests pass;8 original failures and2 absent limit interfaces observed RED. Public UDP host/list/join/both directions/role collision PASS.
- Task2:7 compiled production launcher cases PASS;2 native Linux CLI preservation cases PASS. Real WinEU32 game+launcher build381targets PASS, BelowNormal/jobs1.
- Task3: source3c32d87 deployed in authorized service only. Pinned Python digest and source SHA checked by build;128MB/0.25CPU/one replica/capabilities dropped.
- Investigation: first lobby+impairment run FAIL, preserved in game-public-lobby-impaired. NetWait initialized waitStart=now|1, unsigned subtraction could make an even first tick look like a huge wait; test-only auto-giveup25s fired immediately. Production-consumer timer regression reproduced RED, fixed even/odd/zero/expiry/wrap cases GREEN. Rebuild4targets PASS.
- Ruling: add Linux native menu/CLI preset as well as shared C++/Android UI; no physical Android claim.
- Ruling: no delegated reviewer because the user explicitly prohibits workers; review whole diff inline.

- Final game validation:game-public-normal PASS; timer-corrected game-public-lobby-impaired-final PASS, both actual games10state checks/0desync and1200battle frames reported. Simulated4frame lag+0..6jitter+5%loss. Both exit0; the remaining peer reports pulled cable after the first controlled KHCOM_QUIT, separated from live-play assertions. Previous failed/teardown-conflated evidence preserved.
- Launcher visual QA: real32-bit executable captured offscreen at1180x4000 to show the existing scrolled settings without moving the user desktop. Beta row, explanation, name, delay and stats fit. Source consumer also compiled64-bit.
- Review: rejected role stealing, bounded state/UDP replies, monotonic leases and correct index removal verified; no source/authentication/ROM/installation/other-service access introduced. Source image and room scope remain original two-player protocol.
