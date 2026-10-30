# Three scene framing modes implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. No implementation workers.

**Goal:** Add Plano original alongside existing Ajuste and Extendido for2.3.
**Architecture:** Append enum/config value2; reuse the existing close-camera consumer and independent dialogue compositor. Extend both selectors and six-language copy without changing PortConfig ABI or honors admission.
**Tech Stack:** C/C++, Python compiled-consumer tests, SDL3, llvm-mingw.
**Spec:** docs/superpowers/specs/2026-10-30-scene-three-modes-design.md

## Global constraints

Ajuste0 remains default and honors-aware; Extendido1 uses user zoom everywhere;
Plano original2 closes every engine scene. Auto view only; HUD/portraits stay
independent; gameplay/movies and existing settings preserved. No release. Build
jobs1/BelowNormal, one game/build workload. Protected originals/installation,
VPS/ERP/other projects untouched; no desktop control.

## Review focus

- Legacy INI0/1 and absent key retain meaning; invalid values clamp safely.
- Forward/backward native navigation reaches and wraps all three choices.
- Exact honors grants affect only Ajuste; Original/CRT/field remain isolated.
- Complete portraits survive both sides; native menu copy fits the layout.
- Translation output remains limited to changed scene strings.

## Task1: policy, persistence and selectors

- [x] Extend failing tests in test_scene_framing.py, test_scene_presenter.py,
  test_scene_launcher.py and add actual native switch consumer coverage.
- [x] Run RED for value2, honors bypass, persistence and selector reachability.
- [x] Update scene_framing.h, port_config.c, port_ui.c, decompui.cpp and i18n
  source/table. No renderer/art input changes or new approval pins required.
- [x] Run policy/config/presenter/launcher/native-selector consumers GREEN.
- [x] Commit the tested change.

## Task2: native evidence and integration

- [x] Build EU32/launcher jobs1 and Windows64 guest/host with ABI checks.
- [x] Capture honors121 across three modes: Ajuste=Extendido, Plano original
  close. Capture ordinary007: Ajuste=Plano original, full portrait intact.
- [x] Capture native options third value and verify notice/label fit.
- [ ] Read-only review, address findings, update progress/queue and open PR.
- [ ] Integrate under queue lock after checks; retain useful QA evidence.

## Ledger

- RED retained for policy2, presenter honors2, launcher count3 and native selector.
  Initial native test extraction crossed a shared case label; tightened to exact
  immediate consumer lines, then observed actual two-row failure.
- GREEN policy/config/presenter/native selector on32/64, launcher64, grants
  unchanged. EU32 game+launcher build6/6 passes, jobs1/BelowNormal.
- One read-only fresh reviewer found no product defects. Launcher test claim
  clarified: it tests three-choice row wiring, native selector proves wrapping.
- Native32 captures active; no desktop interaction and no official RA activity.

- run01 native32 six cases all exit0: honors121 Ajuste=Extendido, forcedclose2
  differs; ordinary007 Ajuste=Plano original exactly; honors text crop unchanged.
  Phil portrait complete visually; Spanish selector/help fit without overlap.
- Windows64 build started only after native32 finished; jobs1/BelowNormal.

- run02 native64 all exit0: forced close honors121 output exactly matches
  native32, including portrait/dialogue. Guest+host ABI/boot tests pass.
  Final launcher consumer GREEN; Ninja no-work. Reviewer found no defects.
